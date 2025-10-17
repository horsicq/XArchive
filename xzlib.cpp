/* Copyright (c) 2023-2025 hors<horsicq@gmail.com>
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

XZlib::XZlib(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XZlib::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= 6) {
        quint16 nHeader = read_uint16(0, true);  // Read as big-endian
        // Check zlib header:
        // 0x7801 = no/low compression
        // 0x789C = default compression
        // 0x78DA = best compression
        if (nHeader == 0x7801) {
            bResult = true;
        } else if (nHeader == 0x789C) {
            bResult = true;
        } else if (nHeader == 0x78DA) {
            bResult = true;
        }
    }

    return bResult;
}

bool XZlib::isValid(QIODevice *pDevice)
{
    XZlib xzlib(pDevice);

    return xzlib.isValid();
}

quint64 XZlib::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return 1;  // Always 1
}

qint64 XZlib::getNumberOfArchiveRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return 1;  // Always 1
}

QList<XArchive::RECORD> XZlib::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)  // Always 1

    QList<RECORD> listResult;

    RECORD record = {};

    qint64 nOffset = 0;

    record.spInfo.compressMethod = COMPRESS_METHOD_ZLIB;

    nOffset += 2;

    SubDevice sd(getDevice(), nOffset, -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DECOMPRESS_STATE state = {};
        // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = 0;
        state.nInputLimit = -1;
        state.nDecompressedOffset = 0;
        state.nDecompressedLimit = -1;

        XDecompress decompressor;
        bool bResult = decompressor.decompress(&state, pPdStruct);

        Q_UNUSED(bResult)

        record.nHeaderOffset = 0;
        record.nHeaderSize = nOffset;
        record.nDataOffset = nOffset;
        record.nDataSize = state.nCountInput;
        record.spInfo.nUncompressedSize = state.nCountOutput;
        record.spInfo.sRecordName = XBinary::getDeviceFileBaseName(getDevice());

        sd.close();
    }

    // TODO

    listResult.append(record);

    return listResult;
}

qint64 XZlib::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XZlib::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XZlib::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint32 nIndex = 0;

    _MEMORY_RECORD memoryRecordHeader = {};
    _MEMORY_RECORD memoryRecord = {};
    _MEMORY_RECORD memoryRecordFooter = {};

    qint64 nOffset = 0;

    COMPRESS_METHOD cm = COMPRESS_METHOD_ZLIB;

    nOffset += 2;  // TODO consts

    memoryRecordHeader.nOffset = 0;
    memoryRecordHeader.nAddress = -1;
    memoryRecordHeader.nSize = nOffset;
    memoryRecordHeader.sName = tr("Header");
    memoryRecordHeader.filePart = FILEPART_HEADER;
    memoryRecordHeader.nIndex = nIndex++;

    result.listRecords.append(memoryRecordHeader);

    SubDevice sd(getDevice(), nOffset, -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DECOMPRESS_STATE state = {};
        // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = 0;
        state.nInputLimit = -1;
        state.nDecompressedOffset = 0;
        state.nDecompressedLimit = -1;

        XDecompress decompressor;
        bool bResult = decompressor.decompress(&state, pPdStruct);

        Q_UNUSED(bResult)

        memoryRecord.nOffset = nOffset;
        memoryRecord.nAddress = -1;
        memoryRecord.nSize = state.nCountInput;
        memoryRecord.filePart = FILEPART_REGION;
        memoryRecord.sName = tr("Data");
        memoryRecord.nIndex = nIndex++;

        sd.close();
    }

    // TODO

    result.listRecords.append(memoryRecord);

    memoryRecordFooter.nOffset = memoryRecord.nOffset + memoryRecord.nSize;
    memoryRecordFooter.nAddress = -1;
    memoryRecordFooter.nSize = 4;
    memoryRecordFooter.sName = tr("Footer");
    memoryRecordFooter.filePart = FILEPART_FOOTER;
    memoryRecordFooter.nIndex = nIndex++;

    result.listRecords.append(memoryRecordFooter);

    _handleOverlay(&result);

    return result;
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

bool XZlib::initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Validate zlib file
        if (!isValid(pPdStruct)) {
            return false;
        }

        // Create and initialize context
        ZLIB_UNPACK_CONTEXT *pContext = new ZLIB_UNPACK_CONTEXT;

        // Zlib format: 2-byte header + DEFLATE compressed data + 4-byte Adler32
        qint64 nOffset = 0;
        qint64 nFileSize = getSize();

        // Header size is always 2 bytes
        pContext->nHeaderSize = 2;
        nOffset += 2;

        // Get filename from device
        pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());

        // Decompress to get sizes
        // Note: compressed data size = total size - header (2) - Adler32 (4)
        qint64 nCompressedDataSize = nFileSize - nOffset - 4;
        SubDevice sd(getDevice(), nOffset, nCompressedDataSize);

        if (sd.open(QIODevice::ReadOnly)) {
            XBinary::DECOMPRESS_STATE state = {};
            // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);
            state.pDeviceInput = &sd;
            QBuffer tempBuffer;
            tempBuffer.open(QIODevice::WriteOnly);
            state.pDeviceOutput = &tempBuffer;
            state.nInputOffset = 0;
            state.nInputLimit = nCompressedDataSize;
            state.nDecompressedOffset = 0;
            state.nDecompressedLimit = -1;

            bool bDecompress = XDeflateDecoder::decompress(&state, pPdStruct);

            tempBuffer.close();

            if (bDecompress) {
                pContext->nCompressedSize = state.nCountInput;
                pContext->nUncompressedSize = state.nCountOutput;

                // Read Adler32 checksum (4 bytes after compressed data)
                pContext->nAdler32 = read_uint32(2 + state.nCountInput, true);
            } else {
                pContext->nCompressedSize = nCompressedDataSize;
                pContext->nUncompressedSize = 0;
                pContext->nAdler32 = 0;
            }

            sd.close();
        }

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;  // zlib contains single compressed stream
        pState->pContext = pContext;

        bResult = true;
    }

    return bResult;
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
    result.nDecompressedOffset = 0;
    result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_ZLIB);

    if (pContext->nAdler32 != 0) {
        result.mapProperties.insert(FPART_PROP_CRC_VALUE, pContext->nAdler32);
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

    // Decompress entire zlib stream to output device
    qint64 nFileSize = getSize();
    SubDevice sd(getDevice(), pContext->nHeaderSize, pContext->nCompressedSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DECOMPRESS_STATE state = {};
        // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDevice;
        state.nInputOffset = 0;
        state.nInputLimit = sd.size();
        state.nDecompressedOffset = 0;
        state.nDecompressedLimit = -1;

        bResult = XDeflateDecoder::decompress(&state, pPdStruct);

        sd.close();
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

bool XZlib::initPack(PACK_STATE *pState, QIODevice *pDestDevice, void *pOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pDestDevice || !pDestDevice->isWritable()) {
        return false;
    }

    // Set the output device in the state
    pState->pDevice = pDestDevice;

    // Create and initialize pack context
    ZLIB_PACK_CONTEXT *pContext = new ZLIB_PACK_CONTEXT;
    pContext->pOutputDevice = pDestDevice;
    pContext->bDataAdded = false;

    // Determine compression level from options
    // Default to level 6 (default compression)
    pContext->nCompressionLevel = 6;

    if (pOptions) {
        // Options could be a pointer to compression level (qint32)
        qint32 *pLevel = (qint32 *)pOptions;
        if (*pLevel >= 0 && *pLevel <= 9) {
            pContext->nCompressionLevel = *pLevel;
        }
    }

    // Write zlib header (2 bytes)
    // Format: CMF (Compression Method and Flags) + FLG (Flags)
    quint8 nCMF = 0x78;  // CM=8 (DEFLATE), CINFO=7 (32K window)

    quint8 nFLG = 0;
    // Set compression level bits (bits 6-7)
    if (pContext->nCompressionLevel <= 2) {
        nFLG = 0x01;  // Fast compression
    } else if (pContext->nCompressionLevel >= 7) {
        nFLG = 0xDA;  // Best compression
    } else {
        nFLG = 0x9C;  // Default compression
    }

    // Calculate FCHECK to make header checksum valid (must be multiple of 31)
    quint16 nHeader = (nCMF << 8) | nFLG;
    quint8 nFCHECK = 31 - (nHeader % 31);
    nFLG = (nFLG & 0xE0) | (nFCHECK & 0x1F);

    QByteArray baHeader;
    baHeader.append((char)nCMF);
    baHeader.append((char)nFLG);

    if (pDestDevice->write(baHeader) != baHeader.size()) {
        delete pContext;
        return false;
    }

    // Initialize state
    pState->nCurrentOffset = 2;  // After header
    pState->nNumberOfRecords = 0;
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
    XBinary::DECOMPRESS_STATE compressState = {};
    compressState.pDeviceInput = &inputBuffer;
    compressState.pDeviceOutput = pState->pDevice;
    compressState.nInputOffset = 0;
    compressState.nInputLimit = baUncompressed.size();

    bool bCompress = XDeflateDecoder::compress(&compressState, pPdStruct, pContext->nCompressionLevel);

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
