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
        if (!m_pOutputDevice) {
            return -1;
        }

        qint64 nWritten = m_pOutputDevice->write(pData, nMaxSize);

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
    QIODevice *m_pOutputDevice;
    quint32 m_nA;
    quint32 m_nB;
};
}  // namespace

XZlib::XZlib(QIODevice *pDevice) : XArchive(pDevice)
{
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

    return xzlib.isValid();
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

QList<XBinary::FPART> XZlib::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    const qint64 fileSize = getSize();
    if (fileSize <= 0) return listResult;

    ZLIB_UNPACK_CONTEXT context = {};

    if (!_getStreamInfo(&context, pPdStruct)) {
        return listResult;
    }

    const qint64 nFormatSize = context.nFooterOffset + 4;

    // Zlib header (2 bytes)
    if (nFileParts & FILEPART_HEADER) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = context.nHeaderSize;
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    // Compressed data region
    if (nFileParts & FILEPART_REGION) {
        FPART region = {};
        region.filePart = FILEPART_REGION;
        region.nFileOffset = context.nHeaderSize;
        region.nFileSize = context.nCompressedSize;
        region.nVirtualAddress = -1;
        region.sName = tr("Data");
        listResult.append(region);
    }

    // Footer (4-byte Adler32)
    if ((nFileParts & FILEPART_FOOTER) && context.bFooterValid) {
        FPART footer = {};
        footer.filePart = FILEPART_FOOTER;
        footer.nFileOffset = context.nFooterOffset;
        footer.nFileSize = 4;
        footer.nVirtualAddress = -1;
        footer.sName = tr("Adler32");
        listResult.append(footer);
    }

    // Data: the zlib member itself, excluding any trailing overlay.
    if (nFileParts & FILEPART_DATA) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = nFormatSize;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if ((nFileParts & FILEPART_OVERLAY) && (nFormatSize < fileSize)) {
        FPART overlay = {};
        overlay.filePart = FILEPART_OVERLAY;
        overlay.nFileOffset = nFormatSize;
        overlay.nFileSize = fileSize - nFormatSize;
        overlay.nVirtualAddress = -1;
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
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        ZLIB_UNPACK_CONTEXT *pContext = new ZLIB_UNPACK_CONTEXT();

        if (!_getStreamInfo(pContext, pPdStruct)) {
            delete pContext;
            return false;
        }

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;  // zlib contains single compressed stream
        pState->pContext = pContext;
        pState->mapUnpackProperties = mapProperties;

        bResult = true;
    }

    return bResult;
}

bool XZlib::_getStreamInfo(ZLIB_UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isValid(pPdStruct)) {
        return false;
    }

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    *pContext = ZLIB_UNPACK_CONTEXT();
    pContext->nHeaderSize = 2;
    pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());

    const qint64 nFileSize = getSize();
    const qint64 nAvailablePayloadSize = nFileSize - pContext->nHeaderSize;
    SubDevice inputDevice(getDevice(), pContext->nHeaderSize, nAvailablePayloadSize);
    XZlibDiscardDevice discardDevice;

    if (!inputDevice.open(QIODevice::ReadOnly) || !discardDevice.open(QIODevice::WriteOnly)) {
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
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

    if (!bDecompressed || (state.nCountInput <= 0) || (state.nCountInput > nAvailablePayloadSize) || (state.nCountOutput < 0)) {
        return false;
    }

    const qint64 nFooterOffset = pContext->nHeaderSize + state.nCountInput;

    if ((nFooterOffset < pContext->nHeaderSize) || (nFooterOffset > (nFileSize - 4))) {
        return false;
    }

    quint8 footer[4] = {};

    if (read_array(nFooterOffset, (char *)footer, sizeof(footer)) != sizeof(footer)) {
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
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
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
    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    ZLIB_UNPACK_CONTEXT *pContext = (ZLIB_UNPACK_CONTEXT *)pState->pContext;

    const bool bCheckCRC = pContext->bFooterValid && XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, XBinary::CRC_TYPE_ADLER32);
    XZlibAdlerOutputDevice adlerOutputDevice(pDevice);
    QIODevice *pDecompressOutput = pDevice;

    if (bCheckCRC) {
        // Match the decoder's normal output positioning before wrapping the
        // destination in a sequential checksum device.
        pDevice->seek(0);

        if (!adlerOutputDevice.open(QIODevice::WriteOnly)) {
            XBinary::setPdStructErrorString(pPdStruct, tr("Cannot initialize CRC check"));
            return false;
        }

        pDecompressOutput = &adlerOutputDevice;
    }

    SubDevice sd(getDevice(), pContext->nHeaderSize, pContext->nCompressedSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDecompressOutput;
        state.nInputOffset = 0;
        state.nInputLimit = sd.size();
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bResult = XDeflateDecoder::decompress(&state, pPdStruct);

        sd.close();
    }

    if (bCheckCRC) {
        adlerOutputDevice.close();

        if (bResult && (adlerOutputDevice.getAdler32() != pContext->nAdler32)) {
            XBinary::setPdStructErrorString(pPdStruct, tr("CRC check failed"));
            bResult = false;
        }
    }

    return bResult;
}

bool XZlib::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
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
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    // Delete format-specific context
    if (pState->pContext) {
        ZLIB_UNPACK_CONTEXT *pContext = (ZLIB_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    // Reset state fields
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}

bool XZlib::initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    pState->pDevice = pDevice;
    pState->mapProperties = mapProperties;

    // Create and initialize pack context
    ZLIB_PACK_CONTEXT *pContext = new ZLIB_PACK_CONTEXT;
    pContext->bDataAdded = false;

    // Determine compression level from options
    // Default to level 6 (default compression)
    qint32 nCompressionLevel = mapProperties.value(PACK_PROP_COMPRESSIONLEVEL, 6).toInt();

    // Write zlib header (2 bytes)
    // Format: CMF (Compression Method and Flags) + FLG (Flags)
    quint8 nCMF = 0x78;  // CM=8 (DEFLATE), CINFO=7 (32K window)

    quint8 nFLevel = 0;

    // Set compression-level hint bits (FLEVEL, bits 6-7). FCHECK is added
    // below after its five bits have been cleared.
    if (nCompressionLevel <= 2) {
        nFLevel = 0;  // Fastest compression
    } else if (nCompressionLevel >= 7) {
        nFLevel = 3;  // Best compression
    } else {
        nFLevel = 2;  // Default compression
    }

    // Calculate FCHECK to make header checksum valid (must be multiple of 31)
    quint8 nFLG = nFLevel << 6;
    quint16 nHeader = (nCMF << 8) | nFLG;
    quint8 nFCHECK = (31 - (nHeader % 31)) % 31;
    nFLG |= nFCHECK;

    QByteArray baHeader;
    baHeader.append((char)nCMF);
    baHeader.append((char)nFLG);

    if (pState->pDevice->write(baHeader) != baHeader.size()) {
        delete pContext;
        return false;
    }

    // Initialize state
    pState->nCurrentOffset = 2;  // After header
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    return true;
}

bool XZlib::addDevice(PACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pDevice) {
        return false;
    }

    ZLIB_PACK_CONTEXT *pContext = (ZLIB_PACK_CONTEXT *)pState->pContext;

    // Zlib format only supports one compressed stream
    if (pContext->bDataAdded) {
        return false;  // Already added data
    }

    // Get device size
    qint64 nDeviceSize = pDevice->size();
    if (nDeviceSize <= 0) {
        return false;
    }

    // Compress data from device using DEFLATE
    pDevice->seek(0);

    QByteArray baUncompressed = pDevice->readAll();
    if (baUncompressed.isEmpty()) {
        return false;
    }

    // Calculate Adler32 checksum of uncompressed data using static method
    quint32 nAdler32 = XBinary::getAdler32(pDevice, pPdStruct);

    // Prepare input buffer for compression
    QBuffer inputBuffer;
    inputBuffer.setData(baUncompressed);
    if (!inputBuffer.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Compress using DEFLATE (raw, not zlib wrapper since we add our own header)
    XBinary::DATAPROCESS_STATE compressState = {};
    compressState.pDeviceInput = &inputBuffer;
    compressState.pDeviceOutput = pState->pDevice;
    compressState.nInputOffset = 0;
    compressState.nInputLimit = baUncompressed.size();

    qint32 nCompressionLevel = pState->mapProperties.value(PACK_PROP_COMPRESSIONLEVEL, 6).toInt();

    bool bCompress = XDeflateDecoder::compress(&compressState, pPdStruct, nCompressionLevel);

    inputBuffer.close();

    if (!bCompress) {
        return false;
    }

    qint64 nCompressedSize = compressState.nCountOutput;

    // Write Adler32 checksum (4 bytes, big-endian)
    quint32 nAdler32BE = qToBigEndian(nAdler32);
    QByteArray baAdler32((const char *)&nAdler32BE, 4);

    if (pState->pDevice->write(baAdler32) != baAdler32.size()) {
        return false;
    }

    // Update state
    pState->nCurrentOffset += nCompressedSize + 4;  // Compressed data + Adler32
    pState->nNumberOfRecords = 1;
    pContext->bDataAdded = true;

    return true;
}

bool XZlib::addFile(PACK_STATE *pState, const QString &sFileName, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext) {
        return false;
    }

    ZLIB_PACK_CONTEXT *pContext = (ZLIB_PACK_CONTEXT *)pState->pContext;

    // Zlib format only supports one compressed stream
    if (pContext->bDataAdded) {
        return false;  // Already added data
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
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
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
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext || !pState->pDevice) {
        return false;
    }

    ZLIB_PACK_CONTEXT *pContext = (ZLIB_PACK_CONTEXT *)pState->pContext;

    // If no data was added, write an empty DEFLATE stream
    if (!pContext->bDataAdded) {
        // Empty DEFLATE block: 0x03 0x00
        QByteArray baEmpty;
        baEmpty.append((char)0x03);
        baEmpty.append((char)0x00);

        if (pState->pDevice->write(baEmpty) != baEmpty.size()) {
            delete pContext;
            return false;
        }

        // Adler32 of empty data is 1
        quint32 nAdler32BE = qToBigEndian((quint32)1);
        QByteArray baAdler32((const char *)&nAdler32BE, 4);

        if (pState->pDevice->write(baAdler32) != baAdler32.size()) {
            delete pContext;
            return false;
        }

        pState->nCurrentOffset += 2 + 4;  // Empty block + Adler32
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
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XZlib::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
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
