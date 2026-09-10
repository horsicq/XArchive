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
#include "xsydexsfxarchive.h"

#include "xlzhufdecoder.h"

#include <QFileInfo>
#include <QUuid>

#include <memory>
#include <new>

namespace {
const qint64 SYDEX_MAX_OVERLAY = 64LL * 1024 * 1024;
const qint64 SYDEX_MAX_IMAGE = 64LL * 1024 * 1024;
// The stage-1 text stream never runs to even 3 KiB in the reference corpus;
// this bounds the search for the stage-2 header far above that and keeps the
// probe constant-time on anything that is not a Sydex extractor.
const qint64 SYDEX_MAX_STUB_SCAN = 0x20000;
const qint32 SYDEX_HEADER_SIZE = 33;
const qint32 SYDEX_MIN_TRACK_SIZE = 128;
const qint32 SYDEX_MAX_TRACK_SIZE = 0x8000;

quint16 sxLe16(const char *pData)
{
    const uchar *pByte = reinterpret_cast<const uchar *>(pData);
    return quint16(pByte[0]) | (quint16(pByte[1]) << 8);
}

qint16 sxSLe16(const char *pData)
{
    return (qint16)sxLe16(pData);
}

quint8 sxByte(const char *pData)
{
    return *reinterpret_cast<const uchar *>(pData);
}

class SYDEX_PARSE_RESULT {
public:
    SYDEX_PARSE_RESULT(const QPointer<QIODevice> &pDevice, qint64 nSavedPosition) : m_pDevice(pDevice), m_nSavedPosition(nSavedPosition)
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

XSydexSFXArchive::XSydexSFXArchive(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    setIsArchive(true);
}

XSydexSFXArchive::~XSydexSFXArchive()
{
    const QSet<UNPACK_CONTEXT *> contexts = m_setContexts;
    m_setContexts.clear();
    for (QSet<UNPACK_CONTEXT *>::const_iterator it = contexts.begin(); it != contexts.end(); ++it) delete *it;
}

bool XSydexSFXArchive::isDeviceReplacementAllowed() const
{
    return m_setContexts.isEmpty();
}

bool XSydexSFXArchive::_parse(QList<FILE_ENTRY> *pEntries, qint64 *pnSourceSize, bool bDecode, PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pnSourceSize) *pnSourceSize = 0;
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<QIODevice> pDevice(getDevice());
    if (!pDevice || !pDevice->isOpen() || !pDevice->isReadable() || pDevice->isSequential()) return false;
    const qint64 nSourceSize = pDevice->size();
    const qint64 nSavedPosition = pDevice->pos();
    if ((nSourceSize < 0x40) || (nSavedPosition < 0)) return false;

    const SYDEX_PARSE_RESULT restoreAndReturn(pDevice, nSavedPosition);

    // The overlay begins where the DOS image ends.  These files also carry an
    // NE header, but the NE image ends at the very same place, which is what
    // the extractor stub itself relies on.
    if (!pDevice->seek(0)) return false;
    const QByteArray baDos = pDevice->read(0x40);
    if ((baDos.size() != 0x40) || (baDos.at(0) != 'M') || (baDos.at(1) != 'Z')) return restoreAndReturn(false);
    const quint16 nLastPageBytes = sxLe16(baDos.constData() + 2);
    const quint16 nPages = sxLe16(baDos.constData() + 4);
    if (!nPages || (nLastPageBytes > 511)) return restoreAndReturn(false);
    const qint64 nImageEnd = qint64(nPages - 1) * 512 + (nLastPageBytes ? nLastPageBytes : 512);
    if ((nImageEnd < 0x40) || (nImageEnd > nSourceSize)) return restoreAndReturn(false);
    const qint64 nOverlaySize = nSourceSize - nImageEnd;
    if ((nOverlaySize < (10 + SYDEX_HEADER_SIZE)) || (nOverlaySize > SYDEX_MAX_OVERLAY)) return restoreAndReturn(false);

    if (!pDevice->seek(nImageEnd)) return restoreAndReturn(false);
    const QByteArray baOverlay = pDevice->read(nOverlaySize);
    if (baOverlay.size() != nOverlaySize) return restoreAndReturn(false);
    const char *pOverlay = baOverlay.constData();

    // Stage 1: the "WB" text block.  Every field the reference detector tests
    // is tested here, including the first word of the stream itself.
    if ((pOverlay[0] != 'W') || (pOverlay[1] != 'B')) return restoreAndReturn(false);
    if (sxLe16(pOverlay + 2) >= 0x200) return restoreAndReturn(false);
    if (sxLe16(pOverlay + 4) == 0) return restoreAndReturn(false);
    if (sxLe16(pOverlay + 6) == 0) return restoreAndReturn(false);
    const quint16 nStreamMagic = sxLe16(pOverlay + 8);
    if ((nStreamMagic != 0xE4D5) && (nStreamMagic != 0xE5CC)) return restoreAndReturn(false);

    // Stage 2: find the self-checking image header behind the text block.
    const qint64 nScanLimit = qMin(nOverlaySize - SYDEX_HEADER_SIZE, SYDEX_MAX_STUB_SCAN);
    qint64 nHeaderOffset = -1;
    qint32 nTrackSize = 0;
    qint32 nTrackCount = 0;
    qint32 nCommentSize = 0;

    for (qint64 nScan = 10; (nScan <= nScanLimit) && (nHeaderOffset == -1); nScan++) {
        if ((nScan & 0xFFF) == 0) {
            if (!isPdStructNotCanceled(pPdStruct)) return restoreAndReturn(false);
        }
        if ((pOverlay[nScan] != 'S') || (pOverlay[nScan + 1] != 'X') || (pOverlay[nScan + 2] != 'D')) continue;

        const char *pHeader = pOverlay + nScan;
        const quint16 nStoredCRC = sxLe16(pHeader + 0x1f);
        if (nStoredCRC != XBinary::_getCRC16(pHeader, 0x1f, 0, XBinary::_getCRC16Table())) continue;

        const qint32 nCandidateTrack = sxLe16(pHeader + 3);
        const qint32 nSectors = sxByte(pHeader + 5);
        const qint32 nHeads = sxByte(pHeader + 6);
        const qint32 nCylinders = sxByte(pHeader + 7);
        const qint32 nStoredCylinders = sxByte(pHeader + 8);
        const qint32 nComment = sxSLe16(pHeader + 0x13);

        if (sxSLe16(pHeader + 0x0f) != 0) continue;  // encrypted image
        if ((nCandidateTrack < SYDEX_MIN_TRACK_SIZE) || (nCandidateTrack > SYDEX_MAX_TRACK_SIZE)) continue;
        if ((nSectors < 1) || (nHeads < 1) || (nCylinders < 1)) continue;
        if ((nStoredCylinders < 1) || (nStoredCylinders > nCylinders)) continue;
        if (nComment < 0) continue;

        nHeaderOffset = nScan;
        nTrackSize = nCandidateTrack;
        nTrackCount = nHeads * nStoredCylinders;
        nCommentSize = nComment;
    }

    if (nHeaderOffset == -1) return restoreAndReturn(false);

    const qint64 nImageSize = qint64(nTrackCount) * qint64(nTrackSize);
    if ((nImageSize <= 0) || (nImageSize > SYDEX_MAX_IMAGE)) return restoreAndReturn(false);

    qint64 nPosition = nHeaderOffset + SYDEX_HEADER_SIZE + nCommentSize;
    if ((nPosition < 0) || (nPosition > nOverlaySize)) return restoreAndReturn(false);

    QString sName = QFileInfo(getDeviceFileName(pDevice.data())).completeBaseName();
    if (!pDevice) return false;
    if (sName.isEmpty()) sName = QStringLiteral("image");
    sName += QStringLiteral(".img");

    if (!bDecode) {
        if (pEntries) {
            FILE_ENTRY entry;
            entry.sName = sName;
            entry.nStreamOffset = nImageEnd + nHeaderOffset;
            entry.nStreamSize = nOverlaySize - nHeaderOffset;
            pEntries->append(entry);
        }
        if (pnSourceSize) *pnSourceSize = nSourceSize;
        return restoreAndReturn(true);
    }

    const XLZHUFDecoder::OPTIONS options = XLZHUFDecoder::getOptions(1, 1, 0, false, false, true);

    QByteArray baImage;
    baImage.reserve((qint32)nImageSize);

    for (qint32 nTrack = 0; nTrack < nTrackCount; nTrack++) {
        if (!isPdStructNotCanceled(pPdStruct)) return restoreAndReturn(false);
        if ((nPosition + 4) > nOverlaySize) return restoreAndReturn(false);

        const quint16 nStoredCRC = sxLe16(pOverlay + nPosition);
        const quint16 nBlockSize = sxLe16(pOverlay + nPosition + 2);
        nPosition += 4;

        QByteArray baTrack;

        if (!(nBlockSize & 0x8000)) {
            const qint64 nPacked = nBlockSize;
            if ((nPacked == 0) || ((nPosition + nPacked) > nOverlaySize)) return restoreAndReturn(false);
            const QByteArray baPacked = QByteArray(pOverlay + nPosition, (qint32)nPacked);
            if (!XLZHUFDecoder::decode(baPacked, options, nTrackSize, &baTrack, pPdStruct)) return restoreAndReturn(false);
            nPosition += nPacked;
        } else {
            // CopyQM-style RLE, framed by a byte count rather than an end mark.
            qint64 nRemaining = 0x10000 - (qint64)nBlockSize;
            if ((nPosition + nRemaining) > nOverlaySize) return restoreAndReturn(false);
            const qint64 nBlockEnd = nPosition + nRemaining;
            baTrack.reserve(nTrackSize);

            while (nRemaining > 0) {
                if (nRemaining < 2) return restoreAndReturn(false);
                const quint16 nRun = sxLe16(pOverlay + nPosition);
                nPosition += 2;
                nRemaining -= 2;

                if (!(nRun & 0x8000)) {
                    const qint64 nLiteral = nRun;
                    if (nRemaining < nLiteral) return restoreAndReturn(false);
                    if ((baTrack.size() + nLiteral) > nTrackSize) return restoreAndReturn(false);
                    if (nLiteral) baTrack.append(pOverlay + nPosition, (qint32)nLiteral);
                    nPosition += nLiteral;
                    nRemaining -= nLiteral;
                } else {
                    const qint64 nRepeat = 0x10000 - (qint64)nRun;
                    if (nRemaining < 1) return restoreAndReturn(false);
                    if ((baTrack.size() + nRepeat) > nTrackSize) return restoreAndReturn(false);
                    const char nByte = pOverlay[nPosition];
                    nPosition += 1;
                    nRemaining -= 1;
                    baTrack.append(QByteArray((qint32)nRepeat, nByte));
                }
            }

            if (nPosition != nBlockEnd) return restoreAndReturn(false);
            if (baTrack.size() != nTrackSize) return restoreAndReturn(false);
        }

        if (nStoredCRC != XBinary::_getCRC16(baTrack.constData(), baTrack.size(), 0, XBinary::_getCRC16Table())) return restoreAndReturn(false);

        baImage.append(baTrack);
    }

    if (baImage.size() != nImageSize) return restoreAndReturn(false);

    if (pEntries) {
        FILE_ENTRY entry;
        entry.sName = sName;
        entry.baData = baImage;
        entry.nStreamOffset = nImageEnd + nHeaderOffset;
        entry.nStreamSize = nPosition - nHeaderOffset;
        pEntries->append(entry);
    }
    if (pnSourceSize) *pnSourceSize = nSourceSize;

    return restoreAndReturn(true);
}

bool XSydexSFXArchive::isValid(PDSTRUCT *pPdStruct)
{
    return _parse(nullptr, nullptr, false, pPdStruct);
}

bool XSydexSFXArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSydexSFXArchive x(pDevice);
    return x.isValid(pPdStruct);
}

XBinary::FT XSydexSFXArchive::getFileType()
{
    return FT_SYDEX_SFX;
}

XBinary::MODE XSydexSFXArchive::getMode()
{
    return MODE_16;
}

QString XSydexSFXArchive::getArch()
{
    return QStringLiteral("8086");
}

QString XSydexSFXArchive::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XSydexSFXArchive::getFileFormatExtsString()
{
    return QStringLiteral("Sydex self-extracting disk image (*.exe)");
}

QString XSydexSFXArchive::getMIMEString()
{
    return QStringLiteral("application/x-msdos-program");
}

QList<QString> XSydexSFXArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("4D5A");
}

XBinary *XSydexSFXArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    return new XSydexSFXArchive(pDevice, bIsImage, nModuleAddress);
}

bool XSydexSFXArchive::_isContextCurrent(const UNPACK_STATE *pState, const UNPACK_CONTEXT *pContext)
{
    return pState && pContext && m_setContexts.contains(const_cast<UNPACK_CONTEXT *>(pContext)) && (pState->pContext == pContext) &&
           (pContext->pOwnerState == pState) && !pState->baUnpackSourceToken.isEmpty() && (pState->baUnpackSourceToken == pContext->baToken) &&
           (pContext->pSourceDevice.data() == getDevice()) && (pContext->nDeviceGeneration == getDeviceGeneration()) &&
           (pState->nTotalSize == pContext->nSourceSize) && (pState->nNumberOfRecords == pContext->listEntries.size()) &&
           (pState->nCurrentIndex == pContext->nCurrentIndex) && (pState->nCurrentOffset == pContext->nCurrentOffset);
}

bool XSydexSFXArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    if (pState->pContext || !pState->baUnpackSourceToken.isEmpty()) {
        UNPACK_CONTEXT *pOld = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!_isContextCurrent(pState, pOld)) return false;
        m_setContexts.remove(pOld);
        delete pOld;
    }
    *pState = UNPACK_STATE();

    QList<FILE_ENTRY> listEntries;
    qint64 nSourceSize = 0;
    if (!_parse(&listEntries, &nSourceSize, true, pPdStruct) || listEntries.isEmpty()) return false;

    std::unique_ptr<UNPACK_CONTEXT> pContext(new (std::nothrow) UNPACK_CONTEXT);
    if (!pContext) return false;
    pContext->listEntries = listEntries;
    pContext->pSourceDevice = getDevice();
    pContext->pOwnerState = pState;
    pContext->nDeviceGeneration = getDeviceGeneration();
    pContext->nSourceSize = nSourceSize;
    pContext->baToken = QUuid::createUuid().toRfc4122();
    if (!pContext->pSourceDevice || pContext->baToken.isEmpty()) return false;

    pState->nTotalSize = nSourceSize;
    pState->nNumberOfRecords = listEntries.size();
    pState->mapUnpackProperties = mapProperties;
    pState->pContext = pContext.get();
    pState->baUnpackSourceToken = pContext->baToken;
    m_setContexts.insert(pContext.release());

    return true;
}

XBinary::ARCHIVERECORD XSydexSFXArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties[FPART_PROP_INFO] = QStringLiteral("Sydex LZHUF/RLE");

    return result;
}

bool XSydexSFXArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
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
    const bool bWritten = writeUnpackData(&writeState, pDevice, entry.baData, pPdStruct);
    if (!bWritten || !_isContextCurrent(pState, pContext) || (pDevice->size() != entry.baData.size())) {
        resize(pDevice, 0);
        pDevice->seek(0);
        return false;
    }
    pContext->nCurrentOffset = writeState.nCurrentOffset;
    pState->nCurrentOffset = writeState.nCurrentOffset;

    return true;
}

bool XSydexSFXArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSydexSFXArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
