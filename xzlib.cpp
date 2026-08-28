/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#include "xzlib.h"
#include "xdecompress.h"
#include "Algos/xdeflatedecoder.h"

#include <limits>
#include <memory>
#include <new>

namespace {
class XZlibDiscardDevice : public QIODevice {
public:
    bool isSequential() const override
    {
        return true;
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)
        Q_UNUSED(nMaxSize)

        return -1;
    }

    qint64 writeData(const char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)

        return nMaxSize;
    }
};

class XZlibAdlerOutputDevice : public QIODevice {
public:
    explicit XZlibAdlerOutputDevice(QIODevice *pOutputDevice) : m_pOutputDevice(pOutputDevice), m_nA(1), m_nB(0)
    {
    }

    bool isSequential() const override
    {
        return true;
    }

    quint32 getAdler32() const
    {
        return (m_nB << 16) | m_nA;
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)
        Q_UNUSED(nMaxSize)

        return -1;
    }

    qint64 writeData(const char *pData, qint64 nMaxSize) override
    {
        if (!m_pOutputDevice || (nMaxSize < 0) || ((nMaxSize > 0) && !pData)) {
            return -1;
        }

        qint64 nWritten = m_pOutputDevice->write(pData, nMaxSize);
        if (!m_pOutputDevice) return -1;
        if ((nWritten < 0) || (nWritten > nMaxSize)) return -1;

        for (qint64 i = 0; i < nWritten; i++) {
            m_nA += (quint8)pData[i];
            if (m_nA >= 65521) {
                m_nA -= 65521;
            }

            m_nB += m_nA;
            if (m_nB >= 65521) {
                m_nB -= 65521;
            }
        }

        return nWritten;
    }

private:
    QPointer<QIODevice> m_pOutputDevice;
    quint32 m_nA;
    quint32 m_nB;
};

class XZlibAdlerInputDevice : public QIODevice {
public:
    explicit XZlibAdlerInputDevice(QIODevice *pInputDevice)
        : m_pInputDevice(pInputDevice), m_nA(1), m_nB(0), m_nCount(0)
    {
    }

    bool isSequential() const override
    {
        if (!m_pInputDevice) return true;
        const bool bResult = m_pInputDevice->isSequential();
        return !m_pInputDevice || bResult;
    }

    qint64 pos() const override
    {
        if (!m_pInputDevice) return -1;
        const qint64 nResult = m_pInputDevice->pos();
        return m_pInputDevice ? nResult : -1;
    }

    qint64 size() const override
    {
        if (!m_pInputDevice) return -1;
        const qint64 nResult = m_pInputDevice->size();
        return m_pInputDevice ? nResult : -1;
    }

    qint64 bytesAvailable() const override
    {
        if (!m_pInputDevice) return 0;
        const qint64 nResult = m_pInputDevice->bytesAvailable();
        return m_pInputDevice ? (nResult + QIODevice::bytesAvailable()) : 0;
    }

    bool atEnd() const override
    {
        if (!m_pInputDevice) return true;
        const bool bResult = m_pInputDevice->atEnd();
        return !m_pInputDevice || bResult;
    }

    bool seek(qint64 nPosition) override
    {
        if (!m_pInputDevice || m_pInputDevice->isSequential() || !m_pInputDevice || (nPosition < 0)) {
            return false;
        }

        const qint64 nCurrentPosition = m_pInputDevice->pos();
        if (!m_pInputDevice || (nCurrentPosition < 0)) return false;

        // The bounded read helper synchronizes a seekable device to its
        // already-current position before and after each read.  Accept that
        // no-op without discarding the Adler state.  A real rewind is safe
        // only to zero, where the checksum can be restarted from its seed.
        if (nPosition != nCurrentPosition) {
            if ((nPosition != 0) || !m_pInputDevice->seek(0) || !m_pInputDevice) return false;
            m_nA = 1;
            m_nB = 0;
            m_nCount = 0;
        } else if (nPosition == 0) {
            m_nA = 1;
            m_nB = 0;
            m_nCount = 0;
        }

        return QIODevice::seek(nPosition);
    }

    quint32 getAdler32() const
    {
        return (m_nB << 16) | m_nA;
    }

    qint64 getCount() const
    {
        return m_nCount;
    }

protected:
    qint64 readData(char *pData, qint64 nMaximumSize) override
    {
        if (!m_pInputDevice || (nMaximumSize < 0) || ((nMaximumSize > 0) && !pData)) return -1;

        const qint64 nRead = m_pInputDevice->read(pData, nMaximumSize);
        if (!m_pInputDevice) return -1;
        if ((nRead <= 0) || (nRead > nMaximumSize)) return nRead;
        if (m_nCount > ((std::numeric_limits<qint64>::max)() - nRead)) return -1;

        for (qint64 i = 0; i < nRead; i++) {
            m_nA += (quint8)pData[i];
            if (m_nA >= 65521) m_nA -= 65521;
            m_nB += m_nA;
            if (m_nB >= 65521) m_nB -= 65521;
        }
        m_nCount += nRead;
        return nRead;
    }

    qint64 writeData(const char *, qint64) override
    {
        return -1;
    }

private:
    QPointer<QIODevice> m_pInputDevice;
    quint32 m_nA;
    quint32 m_nB;
    qint64 m_nCount;
};

bool zlibWriteAll(QIODevice *pDevice, const char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct,
                  qint64 *pnWritten = nullptr)
{
    if (pnWritten) *pnWritten = 0;
    if (!pDevice || !pDevice->isWritable() || (nSize < 0) || ((nSize > 0) && !pData) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nWritten = 0;
    while ((nWritten < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nResult = pDevice->write(pData + nWritten, nSize - nWritten);
        if ((nResult <= 0) || (nResult > (nSize - nWritten))) return false;
        nWritten += nResult;
        if (pnWritten) *pnWritten = nWritten;
    }

    return (nWritten == nSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool zlibCanAppendAt(QIODevice *pDevice, qint64 nOffset)
{
    if (!pDevice || !pDevice->isWritable() || (nOffset < 0)) return false;
    if (pDevice->isSequential()) return true;
    return XBinary::isResizeEnable(pDevice) && (pDevice->pos() == nOffset) && (pDevice->size() == nOffset);
}

bool zlibRollbackWrite(QIODevice *pDevice, qint64 nOffset)
{
    return pDevice && !pDevice->isSequential() && (nOffset >= 0) && XBinary::isResizeEnable(pDevice) &&
           XBinary::resize(pDevice, nOffset) && pDevice->seek(nOffset);
}
}  // namespace

XZlib::XZlib(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XZlib::isPackStateConsistent(const PACK_STATE *pState, const ZLIB_PACK_CONTEXT *pContext)
{
    if (!pState || !pContext || (pState->pContext != pContext) || !pState->pDevice ||
        !pState->pDevice->isWritable() || (pContext->nStartOffset < 0) ||
        (pContext->nStartOffset > ((std::numeric_limits<qint64>::max)() - 8)) ||
        (pContext->nDataOffset != (pContext->nStartOffset + 2)) ||
        (pContext->nCurrentOffset < pContext->nStartOffset) ||
        (pContext->nNumberOfRecords < 0) || (pContext->nNumberOfRecords > 1) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) ||
        (pState->nNumberOfRecords != pContext->nNumberOfRecords) ||
        (pContext->bDataAdded != (pContext->nNumberOfRecords == 1))) {
        return false;
    }

    if (!pContext->bFailed && (pContext->nCurrentOffset < pContext->nDataOffset)) return false;
    if (pState->pDevice->isSequential()) return true;

    return zlibCanAppendAt(pState->pDevice, pContext->nCurrentOffset);
}

bool XZlib::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (getSize() < 6) {
        return false;
    }

    const quint8 nCMF = read_uint8(0);
    const quint8 nFLG = read_uint8(1);
    const quint16 nHeader = ((quint16)nCMF << 8) | nFLG;
    const quint8 nCompressionMethod = nCMF & 0x0f;
    const quint8 nWindowInfo = nCMF >> 4;
    const bool bHasPresetDictionary = (nFLG & 0x20) != 0;

    // RFC 1950: CM=8 is DEFLATE, CINFO must not exceed the 32 KiB window
    // value, and the complete two-byte header must be divisible by 31.
    // Preset dictionaries are valid zlib, but are not supported by this
    // decoder because the required dictionary bytes are not available here.
    return (nCompressionMethod == 8) && (nWindowInfo <= 7) && ((nHeader % 31) == 0) && !bHasPresetDictionary;
}

bool XZlib::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZlib xzlib(pDevice);

    return xzlib.isValid(pPdStruct);
}

qint64 XZlib::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XZlib::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XZlib::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    _MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_REGIONS;  // Default mode for Zlib
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_FOOTER | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_REGION, pPdStruct);
    } else if (mapMode == MAPMODE_DATA) {
        result = _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    }

    return result;
}

static bool zlibCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XZlib::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    const qint64 fileSize = getSize();
    if (fileSize <= 0) return listResult;

    ZLIB_UNPACK_CONTEXT context = {};

    if (!_getStreamInfo(&context, pPdStruct)) {
        return listResult;
    }

    const qint64 nFormatSize = context.nFooterOffset + 4;

    // Zlib header (2 bytes)
    if ((nFileParts & FILEPART_HEADER) && zlibCanAppend(nLimit, listResult)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = context.nHeaderSize;
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
    }

    // Compressed data region
    if ((nFileParts & FILEPART_REGION) && zlibCanAppend(nLimit, listResult)) {
        FPART region = {};
        region.filePart = FILEPART_REGION;
        region.nFileOffset = context.nHeaderSize;
        region.nFileSize = context.nCompressedSize;
        region.nVirtualAddress = XADDR_MAX;
        region.sName = tr("Data");
        listResult.append(region);
    }

    // Footer (4-byte Adler32)
    if ((nFileParts & FILEPART_FOOTER) && zlibCanAppend(nLimit, listResult) && context.bFooterValid) {
        FPART footer = {};
        footer.filePart = FILEPART_FOOTER;
        footer.nFileOffset = context.nFooterOffset;
        footer.nFileSize = 4;
        footer.nVirtualAddress = XADDR_MAX;
        footer.sName = tr("Adler32");
        listResult.append(footer);
    }

    // Data: the zlib member itself, excluding any trailing overlay.
    if ((nFileParts & FILEPART_DATA) && zlibCanAppend(nLimit, listResult)) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nFormatSize;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if ((nFileParts & FILEPART_OVERLAY) &&
        zlibCanAppend(nLimit, listResult) && (nFormatSize < fileSize)) {
        FPART overlay = {};
        overlay.filePart = FILEPART_OVERLAY;
        overlay.nFileOffset = nFormatSize;
        overlay.nFileSize = fileSize - nFormatSize;
        overlay.nVirtualAddress = XADDR_MAX;
        overlay.sName = tr("Overlay");
        listResult.append(overlay);
    }

    return listResult;
}

XBinary::FT XZlib::getFileType()
{
    return FT_ZLIB;
}

QString XZlib::getVersion()
{
    QString sResult;

    quint16 nHeader = read_uint16(0);
    // 0x0178 no compression
    if (nHeader == 0x7801) {
        sResult = "fast";
    } else if (nHeader == 0x789C) {
        sResult = "default";
    } else if (nHeader == 0x78DA) {
        sResult = "best";
    }

    return sResult;
}

QString XZlib::getFileFormatExt()
{
    return "zlib";
}

QString XZlib::getFileFormatExtsString()
{
    return "Zlib (*.zlib)";
}

QString XZlib::getMIMEString()
{
    return "application/x-zlib";
}

QMap<XBinary::UNPACK_PROP, QVariant> XZlib::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XZlib::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XZlib> guardedArchive(this);
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState && !m_bUnpackOperationInProgress &&
        ((!pState->pContext && pState->baUnpackSourceToken.isEmpty()) || guardedArchive->ownsUnpackSource(pState))) {
        if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
        UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
        if (!operationGuard.isAcquired()) return false;

        const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
        if (!guardedArchive || !bBound) return false;

        ZLIB_UNPACK_CONTEXT *pContext = new ZLIB_UNPACK_CONTEXT();

        const bool bStreamInfo = guardedArchive->_getStreamInfo(
            pContext, pPdStruct, &mapProperties);
        if (!guardedArchive) {
            delete pContext;
            return false;
        }
        if (!bStreamInfo) {
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            return false;
        }

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = guardedArchive->getSize();
        if (!guardedArchive) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;  // zlib contains single compressed stream
        pState->pContext = pContext;
        pState->mapUnpackProperties = mapProperties;

        bResult = guardedArchive->validateAndFinalizeUnpackSource(
            pState, pContext, pPdStruct);
        if (!guardedArchive) return false;
        if (!bResult) {
            pState->pContext = nullptr;
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
        }
    }

    return bResult;
}

bool XZlib::_getStreamInfo(ZLIB_UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct,
                           const QMap<UNPACK_PROP, QVariant> *pUnpackProperties)
{
    QPointer<XZlib> guardedArchive(this);
    if (!pContext) return false;
    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive || !bValid) {
        return false;
    }

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    *pContext = ZLIB_UNPACK_CONTEXT();
    pContext->nHeaderSize = 2;
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedArchive || !guardedSource) return false;
    pContext->sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedArchive || !guardedSource) return false;

    const qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedSource) return false;
    const qint64 nAvailablePayloadSize = nFileSize - pContext->nHeaderSize;
    SubDevice inputDevice(guardedSource.data(), pContext->nHeaderSize, nAvailablePayloadSize);
    XZlibDiscardDevice discardDevice;

    if (!inputDevice.open(QIODevice::ReadOnly) || !discardDevice.open(QIODevice::WriteOnly)) {
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    if (pUnpackProperties) state.mapUnpackProperties = *pUnpackProperties;
    state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
    state.pDeviceInput = &inputDevice;
    state.pDeviceOutput = &discardDevice;
    state.nInputOffset = 0;
    state.nInputLimit = nAvailablePayloadSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    const bool bDecompressed = XDeflateDecoder::decompress(&state, pPdStruct);

    discardDevice.close();
    inputDevice.close();

    if (!guardedArchive || !guardedSource || !bDecompressed ||
        (state.nCountInput <= 0) || (state.nCountInput > nAvailablePayloadSize) || (state.nCountOutput < 0)) {
        return false;
    }

    const qint64 nFooterOffset = pContext->nHeaderSize + state.nCountInput;

    if ((nFooterOffset < pContext->nHeaderSize) || (nFooterOffset > (nFileSize - 4))) {
        return false;
    }

    quint8 footer[4] = {};

    const qint64 nRead = guardedArchive->read_array(
        nFooterOffset, (char *)footer, sizeof(footer));
    if (!guardedArchive || !guardedSource ||
        (nRead != sizeof(footer))) {
        return false;
    }

    pContext->nCompressedSize = state.nCountInput;
    pContext->nUncompressedSize = state.nCountOutput;
    pContext->nFooterOffset = nFooterOffset;
    pContext->nAdler32 =
        ((quint32)footer[0] << 24) | ((quint32)footer[1] << 16) | ((quint32)footer[2] << 8) | (quint32)footer[3];
    pContext->bFooterValid = true;

    return true;
}

XBinary::ARCHIVERECORD XZlib::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XZlib> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    ZLIB_UNPACK_CONTEXT *pContext = (ZLIB_UNPACK_CONTEXT *)pState->pContext;

    // Fill ARCHIVERECORD
    result.nStreamOffset = pContext->nHeaderSize;
    result.nStreamSize = pContext->nCompressedSize;
    // result.nDecompressedOffset = 0;
    // result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);

    if (pContext->bFooterValid) {
        result.mapProperties.insert(FPART_PROP_RESULTCRC, pContext->nAdler32);
        result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_ADLER32);
    }

    return result;
}

bool XZlib::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZlib> guardedArchive(this);

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedOutput || !guardedSource ||
        !guardedArchive->isUnpackOutputSupported(guardedOutput.data()) || !guardedArchive ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    ZLIB_UNPACK_CONTEXT *pContext = (ZLIB_UNPACK_CONTEXT *)pState->pContext;
    if ((pContext->nCompressedSize < 0) ||
        (pContext->nUncompressedSize < 0)) return false;
    const qint64 nUncompressedSize = pContext->nUncompressedSize;
    if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            nUncompressedSize)) return false;

    // This override bypasses the base decode chain's per-entry gate; account
    // the member here. Produced bytes are charged by _writeDevice through
    // state.spOutputBudget; the publish copy is never charged.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(
        nUncompressedSize, pPdStruct));
    if (!guardedArchive || !pStage || !guardedOutput || !guardedSource ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) return false;

    const bool bCheckCRC = pContext->bFooterValid && XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, XBinary::CRC_TYPE_ADLER32);
    XZlibAdlerOutputDevice adlerOutputDevice(pStage.get());
    QIODevice *pDecompressOutput = pStage.get();

    if (bCheckCRC) {
        // Match the decoder's normal output positioning before wrapping the
        // destination in a sequential checksum device.
        if (!pStage->seek(0)) return false;

        if (!adlerOutputDevice.open(QIODevice::WriteOnly)) {
            XBinary::setPdStructErrorString(pPdStruct, tr("Cannot initialize CRC check"));
            return false;
        }

        pDecompressOutput = &adlerOutputDevice;
    }

    SubDevice sd(guardedSource.data(), pContext->nHeaderSize, pContext->nCompressedSize);

    bool bResult = false;
    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE,
                                   nUncompressedSize);
        state.mapUnpackProperties = pState->mapUnpackProperties;
        state.spOutputBudget = pState->spOutputBudget;
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDecompressOutput;
        state.nInputOffset = 0;
        state.nInputLimit = sd.size();
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bResult = XDeflateDecoder::decompress(&state, pPdStruct) &&
                  guardedArchive && guardedOutput && guardedSource &&
                  (state.nCountOutput == nUncompressedSize);

        sd.close();
    }

    if (bCheckCRC) {
        adlerOutputDevice.close();

        if (bResult && (adlerOutputDevice.getAdler32() != pContext->nAdler32)) {
            XBinary::setPdStructErrorString(pPdStruct, tr("CRC check failed"));
            bResult = false;
        }
    }

    return bResult && guardedArchive && guardedOutput && guardedSource &&
           guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
           guardedArchive->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState,
                               pPdStruct);
}

bool XZlib::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZlib> guardedArchive(this);

    bool bResult = false;

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) {
        return false;
    }

    // Move to next record
    pState->nCurrentIndex++;

    // zlib has only one record, so moving to next always returns false
    // This indicates end of archive
    bResult = false;

    return bResult;
}

bool XZlib::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZlib> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    ZLIB_UNPACK_CONTEXT *pContext =
        static_cast<ZLIB_UNPACK_CONTEXT *>(pState->pContext);
    guardedArchive->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    if (!guardedArchive) return false;

    // Reset state fields
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

bool XZlib::initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pDevice || !pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    bool bLevelValid = true;
    const qint32 nCompressionLevel = mapProperties.contains(PACK_PROP_COMPRESSIONLEVEL)
                                               ? mapProperties.value(PACK_PROP_COMPRESSIONLEVEL).toInt(&bLevelValid)
                                               : 6;
    if (!bLevelValid || (nCompressionLevel < -1) || (nCompressionLevel > 9)) return false;

    qint64 nStartOffset = pDevice->pos();
    if (!zlibCanAppendAt(pDevice, nStartOffset) ||
        (nStartOffset > ((std::numeric_limits<qint64>::max)() - 8))) {
        return false;
    }

    ZLIB_PACK_CONTEXT *pContext = new (std::nothrow) ZLIB_PACK_CONTEXT();
    if (!pContext) return false;

    // Reinitializing an unfinished seekable member aborts it atomically.  A
    // different destination can also replace an irrecoverable sequential
    // member, but the same sequential stream cannot be made valid again.
    ZLIB_PACK_CONTEXT *pOldContext = static_cast<ZLIB_PACK_CONTEXT *>(pState->pContext);
    if (pOldContext) {
        QIODevice *pOldDevice = pState->pDevice;
        if (pOldDevice && !pOldDevice->isSequential()) {
            if (!isPackStateConsistent(pState, pOldContext) ||
                !zlibRollbackWrite(pOldDevice, pOldContext->nStartOffset)) {
                delete pContext;
                return false;
            }
        } else if (pOldDevice == pDevice) {
            delete pContext;
            return false;
        }

        delete pOldContext;
        pState->pContext = nullptr;
    }

    nStartOffset = pDevice->pos();
    if (!zlibCanAppendAt(pDevice, nStartOffset) ||
        (nStartOffset > ((std::numeric_limits<qint64>::max)() - 8))) {
        delete pContext;
        *pState = PACK_STATE();
        return false;
    }

    // Determine compression level from options
    const qint32 nHeaderLevel = (nCompressionLevel == -1) ? 6 : nCompressionLevel;

    // Write zlib header (2 bytes)
    // Format: CMF (Compression Method and Flags) + FLG (Flags)
    quint8 nCMF = 0x78;  // CM=8 (DEFLATE), CINFO=7 (32K window)

    quint8 nFLevel = 0;

    // Set compression-level hint bits (FLEVEL, bits 6-7). FCHECK is added
    // below after its five bits have been cleared.
    if (nHeaderLevel <= 1) {
        nFLevel = 0;  // Fastest compression
    } else if (nHeaderLevel <= 5) {
        nFLevel = 1;
    } else if (nHeaderLevel <= 6) {
        nFLevel = 2;  // Default compression
    } else {
        nFLevel = 3;  // Best compression
    }

    // Calculate FCHECK to make header checksum valid (must be multiple of 31)
    quint8 nFLG = nFLevel << 6;
    quint16 nHeader = (nCMF << 8) | nFLG;
    quint8 nFCHECK = (31 - (nHeader % 31)) % 31;
    nFLG |= nFCHECK;

    QByteArray baHeader;
    baHeader.append((char)nCMF);
    baHeader.append((char)nFLG);

    pContext->nStartOffset = nStartOffset;
    pContext->nDataOffset = nStartOffset + 2;
    pContext->nCurrentOffset = nStartOffset;
    pContext->nNumberOfRecords = 0;
    pContext->bDataAdded = false;
    pContext->bFailed = false;

    pState->pDevice = pDevice;
    pState->mapProperties = mapProperties;
    pState->nCurrentOffset = nStartOffset;
    pState->nNumberOfRecords = 0;
    pState->pContext = pContext;

    qint64 nHeaderWritten = 0;
    if (!zlibWriteAll(pDevice, baHeader.constData(), baHeader.size(), pPdStruct, &nHeaderWritten)) {
        if (zlibRollbackWrite(pDevice, nStartOffset) || (nHeaderWritten == 0)) {
            delete pContext;
            *pState = PACK_STATE();
        } else {
            pContext->bFailed = true;
            pContext->nCurrentOffset = nStartOffset + nHeaderWritten;
            pState->nCurrentOffset = pContext->nCurrentOffset;
        }
        return false;
    }

    // Initialize state
    pContext->nCurrentOffset = pContext->nDataOffset;
    pState->nCurrentOffset = pContext->nDataOffset;

    return true;
}

static bool zlibFailAddDeviceWrite(XBinary::PACK_STATE *pState, qint64 *pnContextCurrentOffset, qint32 *pnContextNumberOfRecords, bool *pbContextFailed,
                                   qint64 nDataOffset, qint64 nWritten)
{
    if (zlibRollbackWrite(pState->pDevice, nDataOffset)) {
        *pnContextCurrentOffset = nDataOffset;
        *pnContextNumberOfRecords = 0;
        pState->nCurrentOffset = nDataOffset;
        pState->nNumberOfRecords = 0;
        return false;
    }

    if (nWritten <= 0) return false;
    const qint64 nPosition = pState->pDevice ? pState->pDevice->pos() : -1;
    *pbContextFailed = true;
    if (pState->pDevice && pState->pDevice->isSequential()) {
        *pnContextCurrentOffset = nDataOffset + nWritten;
    } else if (nPosition >= nDataOffset) {
        *pnContextCurrentOffset = nPosition;
    } else if (nDataOffset <= ((std::numeric_limits<qint64>::max)() - nWritten)) {
        *pnContextCurrentOffset = nDataOffset + nWritten;
    }
    pState->nCurrentOffset = *pnContextCurrentOffset;
    return false;
}

bool XZlib::addDevice(PACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pState->pDevice->isWritable() ||
        !pDevice || !pDevice->isReadable() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    ZLIB_PACK_CONTEXT *pContext = static_cast<ZLIB_PACK_CONTEXT *>(pState->pContext);

    // Zlib format only supports one compressed stream
    if (pContext->bDataAdded || pContext->bFailed || !isPackStateConsistent(pState, pContext) ||
        (pContext->nCurrentOffset != pContext->nDataOffset) || (pContext->nNumberOfRecords != 0)) {
        return false;  // Already added data
    }

    qint64 nInputLimit = -1;
    if (pDevice->isSequential()) {
        // The historical API packs a complete device, not an arbitrary suffix.
        if (pDevice->pos() != 0) return false;
    } else {
        nInputLimit = pDevice->size();
        if ((nInputLimit < 0) || !pDevice->seek(0)) return false;
    }

    XZlibAdlerInputDevice adlerInputDevice(pDevice);
    if (!adlerInputDevice.open(QIODevice::ReadOnly)) return false;

    // Compress using DEFLATE (raw, not zlib wrapper since we add our own header)
    XBinary::DATAPROCESS_STATE compressState = {};
    compressState.pDeviceInput = &adlerInputDevice;
    compressState.pDeviceOutput = pState->pDevice;
    compressState.nInputOffset = 0;
    compressState.nInputLimit = nInputLimit;

    bool bLevelValid = true;
    const qint32 nCompressionLevel = pState->mapProperties.contains(PACK_PROP_COMPRESSIONLEVEL)
                                               ? pState->mapProperties.value(PACK_PROP_COMPRESSIONLEVEL).toInt(&bLevelValid)
                                               : 6;
    if (!bLevelValid || (nCompressionLevel < -1) || (nCompressionLevel > 9)) {
        adlerInputDevice.close();
        return false;
    }

    const bool bCompress = XDeflateDecoder::compress(&compressState, pPdStruct, nCompressionLevel);
    const quint32 nAdler32 = adlerInputDevice.getAdler32();
    const qint64 nAdlerInputCount = adlerInputDevice.getCount();
    adlerInputDevice.close();

    const qint64 nDataOffset = pContext->nDataOffset;

    const qint64 nCompressedSize = compressState.nCountOutput;
    if (!bCompress || (nCompressedSize < 0) || (nAdlerInputCount != compressState.nCountInput) ||
        ((nInputLimit != -1) && (nAdlerInputCount != nInputLimit)) ||
        (nDataOffset > ((std::numeric_limits<qint64>::max)() - nCompressedSize)) ||
        ((nDataOffset + nCompressedSize) > ((std::numeric_limits<qint64>::max)() - 4))) {
        return zlibFailAddDeviceWrite(pState, &pContext->nCurrentOffset, &pContext->nNumberOfRecords, &pContext->bFailed, nDataOffset, compressState.nCountOutput);
    }

    // Write Adler32 checksum (4 bytes, big-endian)
    quint32 nAdler32BE = qToBigEndian(nAdler32);
    QByteArray baAdler32((const char *)&nAdler32BE, 4);

    qint64 nFooterWritten = 0;
    if (!zlibWriteAll(pState->pDevice, baAdler32.constData(), baAdler32.size(), pPdStruct, &nFooterWritten)) {
        return zlibFailAddDeviceWrite(pState, &pContext->nCurrentOffset, &pContext->nNumberOfRecords, &pContext->bFailed, nDataOffset, nCompressedSize + nFooterWritten);
    }

    // Update state
    pContext->nCurrentOffset = nDataOffset + nCompressedSize + 4;
    pContext->nNumberOfRecords = 1;
    pContext->bDataAdded = true;
    pState->nCurrentOffset = pContext->nCurrentOffset;
    pState->nNumberOfRecords = pContext->nNumberOfRecords;

    return true;
}

bool XZlib::addFile(PACK_STATE *pState, const QString &sFileName, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pState->pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Open file
    QFile file(sFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Use addDevice to compress and add file data
    bool bResult = addDevice(pState, &file, pPdStruct);

    file.close();

    return bResult;
}

bool XZlib::addFolder(PACK_STATE *pState, const QString &sDirectoryPath, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pState->pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Zlib format only supports one compressed stream
    // Cannot add multiple files/folders - this is a limitation of the format
    // Return false to indicate this operation is not supported
    Q_UNUSED(sDirectoryPath)

    return false;
}

bool XZlib::finishPack(PACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pState->pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    ZLIB_PACK_CONTEXT *pContext = static_cast<ZLIB_PACK_CONTEXT *>(pState->pContext);
    if (pContext->bFailed || !isPackStateConsistent(pState, pContext)) return false;

    // If no data was added, write an empty DEFLATE stream
    if (!pContext->bDataAdded) {
        // Empty DEFLATE block: 0x03 0x00
        QByteArray baEmpty;
        baEmpty.append((char)0x03);
        baEmpty.append((char)0x00);

        // Adler32 of empty data is 1
        quint32 nAdler32BE = qToBigEndian((quint32)1);
        QByteArray baAdler32((const char *)&nAdler32BE, 4);
        const QByteArray baEmptyMember = baEmpty + baAdler32;
        const qint64 nDataOffset = pContext->nDataOffset;

        qint64 nEmptyWritten = 0;
        if (!zlibWriteAll(pState->pDevice, baEmptyMember.constData(), baEmptyMember.size(), pPdStruct, &nEmptyWritten)) {
            if (zlibRollbackWrite(pState->pDevice, nDataOffset)) {
                pContext->nCurrentOffset = nDataOffset;
                pContext->nNumberOfRecords = 0;
                pState->nCurrentOffset = nDataOffset;
                pState->nNumberOfRecords = 0;
            } else if (nEmptyWritten > 0) {
                pContext->bFailed = true;
                const qint64 nPosition = pState->pDevice->pos();
                if (pState->pDevice->isSequential()) {
                    pContext->nCurrentOffset = nDataOffset + nEmptyWritten;
                } else if (nPosition >= nDataOffset) {
                    pContext->nCurrentOffset = nPosition;
                } else {
                    pContext->nCurrentOffset = nDataOffset + nEmptyWritten;
                }
                pState->nCurrentOffset = pContext->nCurrentOffset;
            }
            return false;
        }

        pContext->nCurrentOffset = nDataOffset + baEmptyMember.size();
        pContext->nNumberOfRecords = 1;
        pContext->bDataAdded = true;
        pState->nCurrentOffset = pContext->nCurrentOffset;
        pState->nNumberOfRecords = pContext->nNumberOfRecords;
    }

    // Clean up context
    delete pContext;
    pState->pContext = nullptr;

    return true;
}

QList<QString> XZlib::getSearchSignatures()
{
    QList<QString> listResult;

    // Include every supported RFC 1950 header: DEFLATE, no preset
    // dictionary, window sizes 256 B through 32 KiB, and all FLEVEL hints.
    for (qint32 nWindowInfo = 0; nWindowInfo <= 7; nWindowInfo++) {
        quint8 nCMF = (quint8)((nWindowInfo << 4) | 8);

        for (qint32 nFLevel = 0; nFLevel <= 3; nFLevel++) {
            quint8 nFLG = (quint8)(nFLevel << 6);
            quint16 nHeader = ((quint16)nCMF << 8) | nFLG;
            nFLG |= (quint8)((31 - (nHeader % 31)) % 31);

            QString sSignature = QString("%1%2")
                                     .arg((quint32)nCMF, 2, 16, QChar('0'))
                                     .arg((quint32)nFLG, 2, 16, QChar('0'))
                                     .toUpper();
            listResult.append(sSignature);
        }
    }

    return listResult;
}

XBinary *XZlib::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XZlib(pDevice);
}

bool XZlib::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XZlib> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XZlib::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XZlib> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XZlib::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
