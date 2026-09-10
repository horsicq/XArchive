/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xcopyqm.h"

#include <QUuid>

#include <limits>
#include <memory>
#include <new>

namespace {
const qint64 COPYQM_MAX_OVERLAY = 64LL * 1024 * 1024;
const qint64 COPYQM_MAX_OUTPUT = 64LL * 1024 * 1024;
const quint32 COPYQM_MAX_SCREENS = 1000;
const quint32 COPYQM_MAX_LINES_PER_SCREEN = 100000;

quint16 cqLe16(const char *p)
{
    const uchar *u = reinterpret_cast<const uchar *>(p);
    return quint16(u[0]) | (quint16(u[1]) << 8);
}

quint32 cqLe32(const char *p)
{
    const uchar *u = reinterpret_cast<const uchar *>(p);
    return quint32(u[0]) | (quint32(u[1]) << 8) | (quint32(u[2]) << 16) | (quint32(u[3]) << 24);
}

class COPYQM_PARSE_RESULT {
public:
    COPYQM_PARSE_RESULT(const QPointer<QIODevice> &pDevice, qint64 nSavedPosition)
        : m_pDevice(pDevice), m_nSavedPosition(nSavedPosition)
    {
    }

    bool operator()(bool bResult) const
    {
        if (!m_pDevice || !m_pDevice->seek(m_nSavedPosition)) return false;
        return bResult;
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nSavedPosition;
};
}  // namespace

XCopyQM::XCopyQM(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    setIsArchive(true);
}

XCopyQM::~XCopyQM()
{
    const QSet<UNPACK_CONTEXT *> contexts = m_setContexts;
    m_setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) delete pContext;
}

bool XCopyQM::isDeviceReplacementAllowed() const
{
    return m_setContexts.isEmpty();
}

bool XCopyQM::_parse(QList<FILE_ENTRY> *pEntries, qint64 *pnSourceSize, PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pnSourceSize) *pnSourceSize = 0;
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<QIODevice> pDevice(getDevice());
    if (!pDevice || !pDevice->isOpen() || !pDevice->isReadable() || pDevice->isSequential()) return false;
    const qint64 sourceSize = pDevice->size();
    const qint64 savedPosition = pDevice->pos();
    if ((sourceSize < 0x40) || (savedPosition < 0)) return false;

    const COPYQM_PARSE_RESULT restoreAndReturn(pDevice, savedPosition);

    if (!pDevice->seek(0)) return false;
    const QByteArray dos = pDevice->read(0x40);
    if ((dos.size() != 0x40) || (dos.at(0) != 'M') || (dos.at(1) != 'Z')) return restoreAndReturn(false);
    const quint16 lastPageBytes = cqLe16(dos.constData() + 2);
    const quint16 pages = cqLe16(dos.constData() + 4);
    if (!pages || (lastPageBytes > 511)) return restoreAndReturn(false);
    const qint64 imageEnd = qint64(pages - 1) * 512 + (lastPageBytes ? lastPageBytes : 512);
    if ((imageEnd < 0x40) || (imageEnd > sourceSize) || ((sourceSize - imageEnd) < 22) || ((sourceSize - imageEnd) > COPYQM_MAX_OVERLAY)) {
        return restoreAndReturn(false);
    }

    if (!pDevice->seek(imageEnd)) return restoreAndReturn(false);
    const QByteArray overlay = pDevice->read(sourceSize - imageEnd);
    if ((overlay.size() != (sourceSize - imageEnd)) || !overlay.startsWith("TX")) return restoreAndReturn(false);
    const qint64 overlaySize = overlay.size();
    const quint32 nodeCount = cqLe16(overlay.constData() + 2);
    const quint32 screenCount = cqLe16(overlay.constData() + 4);
    const quint32 bodySize = cqLe32(overlay.constData() + 6);
    if ((nodeCount < 3) || !(nodeCount & 1) || (nodeCount > 255) || !screenCount || (screenCount > COPYQM_MAX_SCREENS) ||
        (bodySize != quint32(overlaySize - 10)) || (cqLe16(overlay.constData() + 10) != nodeCount)) {
        return restoreAndReturn(false);
    }

    const qint64 nodeTable = 12;
    const qint64 directory = nodeTable + qint64(nodeCount) * 2;
    // Two directory layouts exist: the newer one prepends a 2-byte word to each
    // record, so the stride is 8 with lines at +2 and start at +4, and the older
    // one is 6 with lines at +0 and start at +2.  Probe both and require exactly
    // one to validate, as the reference extractor does.
    qint64 nRecordStride = 0;
    qint64 dataStart = 0;
    qint64 dataSize = 0;
    {
        const qint64 arrStrides[2] = {8, 6};
        qint32 nValidCount = 0;
        for (qint32 nVariant = 0; nVariant < 2; ++nVariant) {
            const qint64 nStride = arrStrides[nVariant];
            const qint64 nStart = directory + qint64(screenCount) * nStride;
            if ((directory < nodeTable) || (nStart < directory) || (nStart >= overlaySize)) continue;
            const qint64 nSize = overlaySize - nStart;
            const qint64 nLinesOffset = (nStride == 8) ? 2 : 0;
            const qint64 nStartOffset = (nStride == 8) ? 4 : 2;
            bool bValid = true;
            quint32 nPrevious = 0;
            for (quint32 i = 0; bValid && (i < screenCount); ++i) {
                const qint64 nRow = directory + qint64(i) * nStride;
                if ((nRow + nStride) > overlaySize) {
                    bValid = false;
                    break;
                }
                const quint32 nLines = cqLe16(overlay.constData() + nRow + nLinesOffset);
                const quint32 nStartValue = cqLe32(overlay.constData() + nRow + nStartOffset);
                if (!nLines || (nLines > COPYQM_MAX_LINES_PER_SCREEN) || (nStartValue >= quint64(nSize)) ||
                    ((i > 0) && (nStartValue <= nPrevious))) {
                    bValid = false;
                    break;
                }
                nPrevious = nStartValue;
            }
            if (bValid) {
                ++nValidCount;
                nRecordStride = nStride;
                dataStart = nStart;
                dataSize = nSize;
            }
        }
        if (nValidCount != 1) return restoreAndReturn(false);
    }

    // Reject invalid indices and cyclic-root shapes before decoding.  A leaf
    // has a zero first byte; an internal node has two one-based child indices.
    for (quint32 i = 0; i < nodeCount; ++i) {
        const quint8 a = quint8(overlay.at(qint32(nodeTable + qint64(i) * 2)));
        const quint8 b = quint8(overlay.at(qint32(nodeTable + qint64(i) * 2 + 1)));
        if (a && ((a > nodeCount) || !b || (b > nodeCount))) return restoreAndReturn(false);
    }
    if (quint8(overlay.at(qint32(nodeTable + qint64(nodeCount - 1) * 2))) == 0) return restoreAndReturn(false);

    QList<quint32> starts;
    QList<quint32> lineCounts;
    starts.reserve(screenCount);
    lineCounts.reserve(screenCount);
    quint32 previous = 0;
    for (quint32 i = 0; i < screenCount; ++i) {
        const qint64 row = directory + qint64(i) * nRecordStride;
        const quint32 lines = cqLe16(overlay.constData() + row + ((nRecordStride == 8) ? 2 : 0));
        const quint32 start = cqLe32(overlay.constData() + row + ((nRecordStride == 8) ? 4 : 2));
        if (!lines || (lines > COPYQM_MAX_LINES_PER_SCREEN) || (start >= quint64(dataSize)) || ((i > 0) && (start <= previous))) {
            return restoreAndReturn(false);
        }
        starts.append(start);
        lineCounts.append(lines);
        previous = start;
    }
    // Newer CopyQM tools put a six-byte screen-geometry prefix in the data
    // area (the same little-endian word repeated three times); older tools
    // begin the first line immediately. Directory offsets include the prefix.
    if (starts.first() != 0) {
        if ((starts.first() != 6) || (dataSize < 6)) return restoreAndReturn(false);
        const quint16 geometry = cqLe16(overlay.constData() + dataStart);
        if (!geometry || (cqLe16(overlay.constData() + dataStart + 2) != geometry) ||
            (cqLe16(overlay.constData() + dataStart + 4) != geometry)) {
            return restoreAndReturn(false);
        }
    }

    QList<FILE_ENTRY> entries;
    entries.reserve(screenCount);
    const qint64 outputCap = qMin<qint64>(COPYQM_MAX_OUTPUT, qMax<qint64>(overlaySize, 1) * 64);
    qint64 totalOutput = 0;

    for (quint32 screen = 0; screen < screenCount; ++screen) {
        if (!isPdStructNotCanceled(pPdStruct)) return restoreAndReturn(false);
        const qint64 screenStart = starts.at(screen);
        const qint64 screenEnd = (screen + 1 < screenCount) ? starts.at(screen + 1) : dataSize;
        if ((screenStart < 0) || (screenEnd <= screenStart) || (screenEnd > dataSize)) return restoreAndReturn(false);

        QByteArray decoded;
        qint64 cursor = screenStart;
        for (quint32 line = 0; line < lineCounts.at(screen); ++line) {
            if (cursor >= screenEnd) return restoreAndReturn(false);
            const quint8 recordSize = quint8(overlay.at(qint32(dataStart + cursor)));
            if ((recordSize < 2) || (recordSize > (screenEnd - cursor))) return restoreAndReturn(false);

            const qint64 bitData = dataStart + cursor + 1;
            const qint32 bitCount = (qint32(recordSize) - 1) * 8;
            qint32 bitPosition = 0;
            bool terminated = false;
            bool indentPending = false;

            while (!terminated) {
                quint32 node = nodeCount;  // one based
                quint32 depth = 0;
                while (true) {
                    if (!node || (node > nodeCount) || (++depth > nodeCount)) return restoreAndReturn(false);
                    const qint64 nodeOffset = nodeTable + qint64(node - 1) * 2;
                    const quint8 a = quint8(overlay.at(qint32(nodeOffset)));
                    const quint8 b = quint8(overlay.at(qint32(nodeOffset + 1)));
                    if (!a) {
                        const quint8 symbol = b;
                        if (indentPending) {
                            if (symbol < 0x20) return restoreAndReturn(false);
                            const qint32 spaces = qint32(symbol) - 0x20;
                            if ((totalOutput + decoded.size() + spaces) > outputCap) return restoreAndReturn(false);
                            decoded.append(QByteArray(spaces, ' '));
                            indentPending = false;
                        } else if (symbol == 0x09) {
                            indentPending = true;
                        } else if (symbol == 0x02) {
                            // Display-attribute toggle: the reference implementation intentionally drops it.
                        } else if (symbol == 0x0a) {
                            decoded.append('\n');
                            terminated = true;
                        } else if ((symbol < 0x20) && (symbol != 0x0c)) {
                            return restoreAndReturn(false);
                        } else {
                            decoded.append(char(symbol));
                        }
                        if ((totalOutput + decoded.size()) > outputCap) return restoreAndReturn(false);
                        break;
                    }
                    if (bitPosition >= bitCount) return restoreAndReturn(false);
                    const quint8 packed = quint8(overlay.at(qint32(bitData + (bitPosition >> 3))));
                    const bool bit = (packed & (0x80U >> (bitPosition & 7))) != 0;
                    ++bitPosition;
                    node = bit ? b : a;
                }
            }
            if (indentPending) return restoreAndReturn(false);
            cursor += recordSize;
        }
        if (cursor != screenEnd) return restoreAndReturn(false);

        FILE_ENTRY entry;
        entry.sName = QStringLiteral("%1.txt").arg(screen + 1, 2, 10, QChar('0'));
        entry.baData = decoded;
        entry.nStreamOffset = imageEnd + dataStart + screenStart;
        entry.nStreamSize = screenEnd - screenStart;
        entries.append(entry);
        totalOutput += decoded.size();
        if (totalOutput > outputCap) return restoreAndReturn(false);
    }

    if (entries.size() != qint32(screenCount)) return restoreAndReturn(false);
    if (pEntries) *pEntries = entries;
    if (pnSourceSize) *pnSourceSize = sourceSize;
    return restoreAndReturn(true);
}

bool XCopyQM::isValid(PDSTRUCT *pPdStruct)
{
    return _parse(nullptr, nullptr, pPdStruct);
}

bool XCopyQM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCopyQM x(pDevice);
    return x.isValid(pPdStruct);
}

XBinary::FT XCopyQM::getFileType()
{
    return FT_MSDOS_COPYQM;
}

XBinary::MODE XCopyQM::getMode()
{
    return MODE_16;
}

QString XCopyQM::getArch()
{
    return QStringLiteral("8086");
}

QString XCopyQM::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XCopyQM::getFileFormatExtsString()
{
    return QStringLiteral("CopyQM-family DOS executable (*.exe)");
}

QString XCopyQM::getMIMEString()
{
    return QStringLiteral("application/x-msdos-program");
}

QList<QString> XCopyQM::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("4D5A");
}

XBinary *XCopyQM::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    return new XCopyQM(pDevice, bIsImage, nModuleAddress);
}

bool XCopyQM::_isContextCurrent(const UNPACK_STATE *pState, const UNPACK_CONTEXT *pContext)
{
    return pState && pContext && m_setContexts.contains(const_cast<UNPACK_CONTEXT *>(pContext)) && (pState->pContext == pContext) &&
           (pContext->pOwnerState == pState) && !pState->baUnpackSourceToken.isEmpty() && (pState->baUnpackSourceToken == pContext->baToken) &&
           (pContext->pSourceDevice.data() == getDevice()) && (pContext->nDeviceGeneration == getDeviceGeneration()) &&
           (pState->nTotalSize == pContext->nSourceSize) && (pState->nNumberOfRecords == pContext->listEntries.size()) &&
           (pState->nCurrentIndex == pContext->nCurrentIndex) && (pState->nCurrentOffset == pContext->nCurrentOffset);
}

bool XCopyQM::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    if (pState->pContext || !pState->baUnpackSourceToken.isEmpty()) {
        UNPACK_CONTEXT *pOld = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!_isContextCurrent(pState, pOld)) return false;
        m_setContexts.remove(pOld);
        delete pOld;
    }
    *pState = UNPACK_STATE();

    QList<FILE_ENTRY> entries;
    qint64 sourceSize = 0;
    if (!_parse(&entries, &sourceSize, pPdStruct) || entries.isEmpty()) return false;

    std::unique_ptr<UNPACK_CONTEXT> pContext(new (std::nothrow) UNPACK_CONTEXT);
    if (!pContext) return false;
    pContext->listEntries = entries;
    pContext->pSourceDevice = getDevice();
    pContext->pOwnerState = pState;
    pContext->nDeviceGeneration = getDeviceGeneration();
    pContext->nSourceSize = sourceSize;
    pContext->baToken = QUuid::createUuid().toRfc4122();
    if (!pContext->pSourceDevice || pContext->baToken.isEmpty()) return false;

    pState->nTotalSize = sourceSize;
    pState->nNumberOfRecords = entries.size();
    pState->mapUnpackProperties = mapProperties;
    pState->pContext = pContext.get();
    pState->baUnpackSourceToken = pContext->baToken;
    m_setContexts.insert(pContext.release());
    return true;
}

XBinary::ARCHIVERECORD XCopyQM::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext) return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext) || (pContext->nCurrentIndex < 0) || (pContext->nCurrentIndex >= pContext->listEntries.size())) return result;
    const FILE_ENTRY &entry = pContext->listEntries.at(pContext->nCurrentIndex);
    result.nStreamOffset = entry.nStreamOffset;
    result.nStreamSize = entry.nStreamSize;
    result.mapProperties[FPART_PROP_ORIGINALNAME] = entry.sName;
    result.mapProperties[FPART_PROP_COMPRESSEDSIZE] = entry.nStreamSize;
    result.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = qint64(entry.baData.size());
    result.mapProperties[FPART_PROP_ISFOLDER] = false;
    result.mapProperties[FPART_PROP_INFO] = QStringLiteral("CopyQM Huffman");
    return result;
}

bool XCopyQM::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pDevice || !pDevice->isOpen() || !pDevice->isWritable() || pDevice->isSequential() ||
        (pDevice->openMode() & (QIODevice::Append | QIODevice::Text)) || !isResizeEnable(pDevice) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext) || devicesAlias(pContext->pSourceDevice.data(), pDevice) || (pContext->nCurrentIndex < 0) ||
        (pContext->nCurrentIndex >= pContext->listEntries.size())) {
        return false;
    }
    const FILE_ENTRY &entry = pContext->listEntries.at(pContext->nCurrentIndex);
    if (pState->spOutputBudget && !pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sName) && pState->spOutputBudget->isEnforcing()) {
        setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (!resize(pDevice, 0) || !pDevice->seek(0)) return false;
    UNPACK_STATE writeState = *pState;
    writeState.nCurrentOffset = 0;
    const bool written = writeUnpackData(&writeState, pDevice, entry.baData, pPdStruct);
    if (!written || !_isContextCurrent(pState, pContext) || (pDevice->size() != entry.baData.size())) {
        resize(pDevice, 0);
        pDevice->seek(0);
        return false;
    }
    pContext->nCurrentOffset = writeState.nCurrentOffset;
    pState->nCurrentOffset = writeState.nCurrentOffset;
    return true;
}

bool XCopyQM::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext) || (pContext->nCurrentIndex < 0) || (pContext->nCurrentIndex >= pContext->listEntries.size())) return false;
    ++pContext->nCurrentIndex;
    pContext->nCurrentOffset = 0;
    pState->nCurrentIndex = pContext->nCurrentIndex;
    pState->nCurrentOffset = 0;
    return pContext->nCurrentIndex < pContext->listEntries.size();
}

bool XCopyQM::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState) return false;
    if (!pState->pContext && pState->baUnpackSourceToken.isEmpty()) {
        *pState = UNPACK_STATE();
        return true;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext)) return false;
    m_setContexts.remove(pContext);
    *pState = UNPACK_STATE();
    delete pContext;
    return true;
}
