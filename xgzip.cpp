/* Copyright (c) 2022-2025 hors<horsicq@gmail.com>
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
#include "xgzip.h"
#include "Algos/xdeflatedecoder.h"

XBinary::XCONVERT _TABLE_XGZIP_STRUCTID[] = {{XGzip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XGzip::STRUCTID_GZIP_HEADER, "GZIP_HEADER", QString("GZIP header")},
                                             {XGzip::STRUCTID_STREAM, "STREAM", QString("Stream")}};

XGzip::XGzip(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGzip::~XGzip()
{
}

bool XGzip::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(GZIP_HEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "1F8B08", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XGzip::isValid(QIODevice *pDevice)
{
    XGzip xgzip(pDevice);

    return xgzip.isValid();
}

quint64 XGzip::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return 1;  // Always 1
}

qint64 XGzip::getNumberOfArchiveRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return 1;  // Always 1
}

QList<XArchive::RECORD> XGzip::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)  // Always 1

    QList<RECORD> listResult;

    RECORD record = {};

    qint64 nOffset = 0;

    GZIP_HEADER gzipHeader = {};

    read_array(nOffset, (char *)&gzipHeader, sizeof(GZIP_HEADER));

    if (gzipHeader.nCompressionMethod == 8)  // TODO consts
    {
        record.spInfo.compressMethod = COMPRESS_METHOD_DEFLATE;  // TODO more
    }

    nOffset += sizeof(GZIP_HEADER);

    if (gzipHeader.nFileFlags & 8)  // File name
    {
        record.spInfo.sRecordName = read_ansiString(nOffset);
        nOffset += record.spInfo.sRecordName.size() + 1;
    }

    SubDevice sd(getDevice(), nOffset, getSize() - nOffset);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DECOMPRESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, record.spInfo.compressMethod);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = 0;
        state.nInputLimit = getSize() - nOffset;
        state.nDecompressedOffset = 0;
        state.nDecompressedLimit = -1;

        bool bResult = XDeflateDecoder::decompress(&state, pPdStruct);

        Q_UNUSED(bResult)

        record.nHeaderOffset = 0;
        record.nHeaderSize = nOffset;
        record.nDataOffset = nOffset;
        record.nDataSize = state.nCountInput;
        record.spInfo.nUncompressedSize = state.nCountOutput;
        record.spInfo.sRecordName = XBinary::getDeviceFileBaseName(getDevice());  // TODO, use from header

        sd.close();
    }

    // TODO

    listResult.append(record);

    return listResult;
}

qint64 XGzip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XGzip::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XGzip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};

    result.fileType = getFileType();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = getTypeAsString();
    result.nBinarySize = getSize();

    _MEMORY_RECORD memoryRecordHeader = {};
    _MEMORY_RECORD memoryRecord = {};
    _MEMORY_RECORD memoryRecordFooter = {};

    qint64 nOffset = 0;

    GZIP_HEADER gzipHeader = {};

    read_array(nOffset, (char *)&gzipHeader, sizeof(GZIP_HEADER));

    COMPRESS_METHOD cm = COMPRESS_METHOD_DEFLATE;

    if (gzipHeader.nCompressionMethod == 8)  // TODO consts
    {
        cm = COMPRESS_METHOD_DEFLATE;  // TODO more
    }

    nOffset += sizeof(GZIP_HEADER);

    if (gzipHeader.nFileFlags & 8)  // File name
    {
        QString sFileName = read_ansiString(nOffset);
        nOffset += sFileName.size() + 1;
    }

    memoryRecordHeader.nOffset = 0;
    memoryRecordHeader.nAddress = -1;
    memoryRecordHeader.nSize = nOffset;
    memoryRecordHeader.sName = tr("Header");
    memoryRecordHeader.filePart = FILEPART_HEADER;

    result.listRecords.append(memoryRecordHeader);

    SubDevice sd(getDevice(), nOffset, getSize() - nOffset);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DECOMPRESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, cm);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = 0;
        state.nInputLimit = getSize() - nOffset;
        state.nDecompressedOffset = 0;
        state.nDecompressedLimit = -1;

        bool bResult = XDeflateDecoder::decompress(&state, pPdStruct);

        Q_UNUSED(bResult)

        memoryRecord.nOffset = nOffset;
        memoryRecord.nAddress = -1;
        memoryRecord.nSize = state.nCountInput;
        memoryRecord.filePart = FILEPART_REGION;

        sd.close();
    }

    // TODO

    result.listRecords.append(memoryRecord);

    memoryRecordFooter.nOffset = memoryRecord.nOffset + memoryRecord.nSize;
    memoryRecordFooter.nAddress = -1;
    memoryRecordFooter.nSize = 8;
    memoryRecordFooter.sName = tr("Footer");
    memoryRecordFooter.filePart = FILEPART_FOOTER;

    result.listRecords.append(memoryRecordFooter);

    _handleOverlay(&result);

    return result;
}

QString XGzip::getFileFormatExt()
{
    return "gz";
}

QString XGzip::getFileFormatExtsString()
{
    return "GZIP (*.gz)";
}

QString XGzip::getMIMEString()
{
    return "application/gzip";
}

XBinary::FT XGzip::getFileType()
{
    return FT_GZIP;
}

QList<XBinary::FPART> XGzip::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    const qint64 fileSize = getSize();
    if (fileSize <= 0) return listResult;

    qint64 nOffset = 0;
    GZIP_HEADER gzipHeader = {};
    if (isOffsetValid(0 + (qint64)sizeof(GZIP_HEADER) - 1)) {
        read_array(nOffset, (char *)&gzipHeader, sizeof(GZIP_HEADER));
    }

    // Header
    if (nFileParts & FILEPART_HEADER) {
        // Account for optional filename if present
        qint64 headerSize = (qint64)sizeof(GZIP_HEADER);
        qint64 optOffset = headerSize;
        if (gzipHeader.nFileFlags & 8) {
            QString sFileName = read_ansiString(optOffset);
            optOffset += sFileName.size() + 1;
            headerSize = optOffset;
        }
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qBound<qint64>(0, headerSize, fileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    // Region: compressed stream payload (best-effort)
    if (nFileParts & FILEPART_REGION) {
        qint64 payloadOffset = (qint64)sizeof(GZIP_HEADER);
        if (gzipHeader.nFileFlags & 8) {
            QString sFileName = read_ansiString(payloadOffset);
            payloadOffset += sFileName.size() + 1;
        }
        qint64 payloadSize = qMax<qint64>(0, fileSize - payloadOffset);
        // Exclude the standard 8-byte footer (CRC32 + ISIZE) if present
        if (payloadSize >= 8) payloadSize -= 8;

        FPART region = {};
        region.filePart = FILEPART_REGION;
        region.nFileOffset = payloadOffset;
        region.nFileSize = payloadSize;
        region.nVirtualAddress = -1;
        region.sName = tr("Stream");
        listResult.append(region);
    }

    // Footer
    if (nFileParts & FILEPART_FOOTER) {
        if (fileSize >= 8) {
            FPART footer = {};
            footer.filePart = FILEPART_FOOTER;
            footer.nFileOffset = fileSize - 8;
            footer.nFileSize = 8;
            footer.nVirtualAddress = -1;
            footer.sName = tr("Footer");
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

    Q_UNUSED(pPdStruct)
    return listResult;
}

XBinary::MODE XGzip::getMode()
{
    return MODE_DATA;
}

qint32 XGzip::getType()
{
    return TYPE_GZ;
}

XBinary::ENDIAN XGzip::getEndian()
{
    return ENDIAN_LITTLE;  // Gzip is little-endian
}

QString XGzip::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_GZ: sResult = QString("GZ"); break;
    }

    return sResult;
}

XBinary::OSNAME XGzip::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XGzip::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XGZIP_STRUCTID, sizeof(_TABLE_XGZIP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XGzip::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_GZIP_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        if (isPdStructNotCanceled(pPdStruct)) {
            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_GZIP_HEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XGzip::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = sizeof(GZIP_HEADER);

                dataHeader.listRecords.append(
                    getDataRecord(offsetof(GZIP_HEADER, nMagic), 2, "nMagic", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(GZIP_HEADER, nCompressionMethod), 1, "nCompressionMethod", VT_UINT8, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(GZIP_HEADER, nFileFlags), 1, "nFileFlags", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(GZIP_HEADER, nTimeStamp), 4, "nTimeStamp", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(GZIP_HEADER, nCompressionFlags), 1, "nCompressionFlags", VT_UINT8, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(GZIP_HEADER, nOS), 1, "nOS", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

XGzip::GZIP_HEADER XGzip::_read_GZIP_HEADER(qint64 nOffset)
{
    GZIP_HEADER result = {};

    read_array(nOffset, (char *)&result, sizeof(GZIP_HEADER));

    return result;
}

bool XGzip::initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Validate GZIP file
        if (!isValid(pPdStruct)) {
            return false;
        }

        // Create and initialize context
        GZIP_UNPACK_CONTEXT *pContext = new GZIP_UNPACK_CONTEXT;

        // Read header to get metadata
        qint64 nOffset = 0;
        GZIP_HEADER gzipHeader = _read_GZIP_HEADER(nOffset);
        nOffset += sizeof(GZIP_HEADER);

        // Check for optional filename
        if (gzipHeader.nFileFlags & 8) {
            pContext->sFileName = read_ansiString(nOffset);
            nOffset += pContext->sFileName.size() + 1;
        } else {
            pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());
        }

        pContext->nHeaderSize = nOffset;

        // Decompress to get sizes
        qint64 nFileSize = getSize();
        SubDevice sd(getDevice(), nOffset, nFileSize - nOffset);

        if (sd.open(QIODevice::ReadOnly)) {
            XBinary::DECOMPRESS_STATE state = {};
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);
            state.pDeviceInput = &sd;
            state.pDeviceOutput = nullptr;
            state.nInputOffset = 0;
            state.nInputLimit = -1;
            state.nDecompressedOffset = 0;
            state.nDecompressedLimit = -1;

            bool bResult = XDeflateDecoder::decompress(&state, pPdStruct);

            if (bResult) {
                pContext->nCompressedSize = state.nCountInput;
                pContext->nUncompressedSize = state.nCountOutput;
            } else {
                pContext->nCompressedSize = nFileSize - nOffset;
                pContext->nUncompressedSize = 0;
            }

            sd.close();
        }

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;  // GZIP contains single compressed stream
        pState->pContext = pContext;

        bResult = true;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XGzip::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    GZIP_UNPACK_CONTEXT *pContext = (GZIP_UNPACK_CONTEXT *)pState->pContext;

    // Fill ARCHIVERECORD
    result.nStreamOffset = pContext->nHeaderSize;
    result.nStreamSize = pContext->nCompressedSize;
    result.nDecompressedOffset = 0;
    result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);

    return result;
}

bool XGzip::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    GZIP_UNPACK_CONTEXT *pContext = (GZIP_UNPACK_CONTEXT *)pState->pContext;

    // Decompress entire GZIP stream to output device
    qint64 nFileSize = getSize();
    SubDevice sd(getDevice(), pContext->nHeaderSize, nFileSize - pContext->nHeaderSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DECOMPRESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_DEFLATE);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDevice;
        state.nInputOffset = 0;
        state.nInputLimit = getSize();
        state.nDecompressedOffset = 0;
        state.nDecompressedLimit = -1;

        bResult = XDeflateDecoder::decompress(&state, pPdStruct);

        sd.close();
    }

    return bResult;
}

bool XGzip::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    // Move to next record
    pState->nCurrentIndex++;

    // GZIP has only one record, so moving to next always returns false
    // This indicates end of archive
    bResult = false;

    return bResult;
}

bool XGzip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    // Delete format-specific context
    if (pState->pContext) {
        GZIP_UNPACK_CONTEXT *pContext = (GZIP_UNPACK_CONTEXT *)pState->pContext;
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
