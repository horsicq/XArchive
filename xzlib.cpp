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

    qint64 nOffset = 0;

    // Zlib header (2 bytes)
    if (nFileParts & FILEPART_HEADER) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = 2;
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
        nOffset = 2;
    }

    // Compressed data region
    if (nFileParts & FILEPART_REGION) {
        // Compressed data: from offset 2 to (fileSize - 4) for the 4-byte Adler32 footer
        qint64 regionOffset = 2;
        qint64 regionSize = qMax<qint64>(0, fileSize - regionOffset - 4);

        FPART region = {};
        region.filePart = FILEPART_REGION;
        region.nFileOffset = regionOffset;
        region.nFileSize = regionSize;
        region.nVirtualAddress = -1;
        region.sName = tr("Data");
        listResult.append(region);
        nOffset = regionOffset + regionSize;
    }

    // Footer (4-byte Adler32)
    if (nFileParts & FILEPART_FOOTER) {
        if (fileSize >= 6) {  // At least 2 (header) + 4 (footer)
            FPART footer = {};
            footer.filePart = FILEPART_FOOTER;
            footer.nFileOffset = fileSize - 4;
            footer.nFileSize = 4;
            footer.nVirtualAddress = -1;
            footer.sName = tr("Adler32");
            listResult.append(footer);
        }
    }

    // Data: entire file
    if (nFileParts & FILEPART_DATA) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = fileSize;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    // Overlay: handled by _handleOverlay if there is extra data beyond the Adler32
    if (nFileParts & FILEPART_OVERLAY) {
        // For Zlib, overlay would be any data after the Adler32 footer
        // Typically there shouldn't be any, but we handle it just in case
        qint64 overlayOffset = fileSize - 4;
        if (overlayOffset < fileSize) {
            // This case typically doesn't happen in valid zlib files
            // but we can add it if needed
        }
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

bool XZlib::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

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
            XBinary::DATAPROCESS_STATE state = {};
            // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD1, HANDLE_METHOD_DEFLATE);
            state.pDeviceInput = &sd;
            QBuffer tempBuffer;
            tempBuffer.open(QIODevice::WriteOnly);
            state.pDeviceOutput = &tempBuffer;
            state.nInputOffset = 0;
            state.nInputLimit = nCompressedDataSize;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;

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
    // result.nDecompressedOffset = 0;
    // result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD1, HANDLE_METHOD_ZLIB);

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
        XBinary::DATAPROCESS_STATE state = {};
        // Use raw DEFLATE since SubDevice skips the 2-byte zlib header
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD1, HANDLE_METHOD_DEFLATE);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDevice;
        state.nInputOffset = 0;
        state.nInputLimit = sd.size();
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

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

    quint8 nFLG = 0;
    // Set compression level bits (bits 6-7)
    if (nCompressionLevel <= 2) {
        nFLG = 0x01;  // Fast compression
    } else if (nCompressionLevel >= 7) {
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
