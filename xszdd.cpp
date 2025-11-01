/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xszdd.h"
#include "Algos/xlzssdecoder.h"

static XBinary::XCONVERT _TABLE_XSZDD_STRUCTID[] = {{XSZDD::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                    {XSZDD::STRUCTID_SZDD_HEADER, "SZDD_HEADER", QString("SZDD_HEADER")}};

XSZDD::XSZDD(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSZDD::~XSZDD()
{
}

bool XSZDD::isValid(QIODevice *pDevice)
{
    XSZDD xszdd(pDevice);

    return xszdd.isValid();
}

bool XSZDD::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(SZDD_HEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getSimpleMemoryMap();
        if (compareSignature(&memoryMap, "'SZDD'88F027'3A'", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

XSZDD::SZDD_HEADER XSZDD::_read_SZDD_HEADER(qint64 nOffset)
{
    SZDD_HEADER header = {};
    read_array(nOffset, (char *)&header, sizeof(SZDD_HEADER));
    return header;
}

XBinary::FT XSZDD::getFileType()
{
    return XBinary::FT_SZDD;
}

XBinary::MODE XSZDD::getMode()
{
    return XBinary::MODE_DATA;
}

QString XSZDD::getMIMEString()
{
    return "application/x-ms-compress";
}

qint32 XSZDD::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XSZDD::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XSZDD::getArch()
{
    return QString();
}

QString XSZDD::getFileFormatExt()
{
    return "SZDD";
}

QString XSZDD::getFileFormatExtsString()
{
    return "SZDD (*.szdd)";
}

qint64 XSZDD::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XSZDD::isSigned()
{
    return false;
}

XBinary::OSNAME XSZDD::getOsName()
{
    return XBinary::OSNAME_MULTIPLATFORM;
}

QString XSZDD::getOsVersion()
{
    return QString();
}

QString XSZDD::getVersion()
{
    return QString();
}

bool XSZDD::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XSZDD::getMapModesList()
{
    QList<MAPMODE> list;
    list.append(MAPMODE_REGIONS);
    return list;
}

XBinary::_MEMORY_MAP XSZDD::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)
    Q_UNUSED(pPdStruct)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    qint32 nIndex = 0;

    // Add file header
    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = -1;
    recHeader.nOffset = 0;
    recHeader.nSize = sizeof(SZDD_HEADER);
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = tr("SZDD Header");
    result.listRecords.append(recHeader);

    SubDevice sd(getDevice(), sizeof(SZDD_HEADER), -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZSS_SZDD);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, _read_SZDD_HEADER(0).uncompressed_size);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = 0;
        state.nInputLimit = -1;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bool bResult = XLZSSDecoder::decompress(&state, pPdStruct);

        Q_UNUSED(bResult)

        _MEMORY_RECORD memoryRecord = {};

        memoryRecord.nOffset = sizeof(SZDD_HEADER);
        memoryRecord.nAddress = -1;
        memoryRecord.nSize = state.nCountInput;
        memoryRecord.filePart = FILEPART_REGION;

        result.listRecords.append(memoryRecord);

        sd.close();
    }

    _handleOverlay(&result);

    return result;
}

QString XSZDD::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSZDD_STRUCTID, sizeof(_TABLE_XSZDD_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XSZDD::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = DHMODE_HEADER;

        _dataHeadersOptions.nID = STRUCTID_SZDD_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else if (dataHeadersOptions.nID == STRUCTID_SZDD_HEADER) {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            DATA_HEADER dataHeader = {};
            dataHeader.dsID_parent = dataHeadersOptions.dsID_parent;
            dataHeader.dsID.sGUID = generateUUID();
            dataHeader.dsID.fileType = dataHeadersOptions.pMemoryMap->fileType;
            dataHeader.dsID.nID = dataHeadersOptions.nID;
            dataHeader.locType = dataHeadersOptions.locType;
            dataHeader.nLocation = dataHeadersOptions.nLocation;
            dataHeader.sName = structIDToString(dataHeadersOptions.nID);
            dataHeader.dhMode = dataHeadersOptions.dhMode;
            dataHeader.nSize = sizeof(SZDD_HEADER);

            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, signature), 8, "signature", VT_BYTE_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, compression_mode), 1, "compression_mode", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, missing_char), 1, "missing_char", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, uncompressed_size), 4, "uncompressed_size", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));

            listResult.append(dataHeader);
        }
    }

    return listResult;
}

quint64 XSZDD::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return 1;  // Only one file per archive
}

QList<XArchive::RECORD> XSZDD::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)  // Always 1

    QList<RECORD> listResult;

    RECORD record = {};

    qint64 nOffset = 0;

    nOffset += sizeof(SZDD_HEADER);

    SubDevice sd(getDevice(), sizeof(SZDD_HEADER), -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZSS_SZDD);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, _read_SZDD_HEADER(0).uncompressed_size);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = nullptr;
        state.nInputOffset = 0;
        state.nInputLimit = -1;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bool bResult = XLZSSDecoder::decompress(&state, pPdStruct);

        Q_UNUSED(bResult)

        record.nHeaderOffset = 0;
        record.nHeaderSize = nOffset;
        record.nDataOffset = nOffset;
        record.nDataSize = state.nCountInput;
        record.spInfo.nUncompressedSize = state.nCountOutput;
        record.spInfo.sRecordName = XBinary::getDeviceFileBaseName(getDevice());
        record.spInfo.compressMethod = COMPRESS_METHOD_LZSS_SZDD;
        ;

        sd.close();
    }

    // TODO

    listResult.append(record);

    return listResult;
}

QList<XBinary::FPART> XSZDD::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = sizeof(SZDD_HEADER);
        record.nVirtualAddress = -1;
        record.sName = tr("Header");

        listResult.append(record);
    }

    qint64 nTotalSize = getSize();
    qint64 nDataOffset = sizeof(SZDD_HEADER);

    if (nFileParts & FILEPART_REGION) {
        if (nDataOffset < nTotalSize) {
            FPART record = {};

            record.filePart = FILEPART_REGION;
            record.nFileOffset = nDataOffset;
            record.nFileSize = nTotalSize - nDataOffset;
            record.nVirtualAddress = -1;
            record.sName = tr("Compressed Data");

            listResult.append(record);
        }
    }

    return listResult;
}

bool XSZDD::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)
    
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Validate SZDD file
        if (!isValid(pPdStruct)) {
            return false;
        }

        // Create and initialize context
        SZDD_UNPACK_CONTEXT *pContext = new SZDD_UNPACK_CONTEXT;

        // Read header
        qint64 nOffset = 0;
        SZDD_HEADER szddHeader = _read_SZDD_HEADER(nOffset);
        nOffset += sizeof(SZDD_HEADER);

        pContext->nHeaderSize = nOffset;
        pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());
        pContext->nUncompressedSize = szddHeader.uncompressed_size;

        // Decompress to get actual compressed size
        qint64 nFileSize = getSize();
        SubDevice sd(getDevice(), nOffset, nFileSize - nOffset);

        if (sd.open(QIODevice::ReadOnly)) {
            XBinary::DATAPROCESS_STATE state = {};
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZSS_SZDD);
            state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
            state.pDeviceInput = &sd;
            state.pDeviceOutput = nullptr;
            state.nInputOffset = 0;
            state.nInputLimit = -1;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;

            bool bDecompressResult = XLZSSDecoder::decompress(&state, pPdStruct);

            if (bDecompressResult) {
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
        pState->nNumberOfRecords = 1;  // SZDD contains single compressed stream
        pState->pContext = pContext;

        bResult = true;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XSZDD::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    SZDD_UNPACK_CONTEXT *pContext = (SZDD_UNPACK_CONTEXT *)pState->pContext;

    // Fill ARCHIVERECORD
    result.nStreamOffset = pContext->nHeaderSize;
    result.nStreamSize = pContext->nCompressedSize;
    result.nDecompressedOffset = 0;
    result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZSS_SZDD);

    return result;
}

bool XSZDD::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    SZDD_UNPACK_CONTEXT *pContext = (SZDD_UNPACK_CONTEXT *)pState->pContext;

    // Decompress entire SZDD stream to output device
    qint64 nFileSize = getSize();
    SubDevice sd(getDevice(), pContext->nHeaderSize, nFileSize - pContext->nHeaderSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZSS_SZDD);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDevice;
        state.nInputOffset = 0;
        state.nInputLimit = -1;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bResult = XLZSSDecoder::decompress(&state, pPdStruct);

        sd.close();
    }

    return bResult;
}

bool XSZDD::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    // Move to next record
    pState->nCurrentIndex++;

    // SZDD has only one record, so moving to next always returns false
    // This indicates end of archive
    bResult = false;

    return bResult;
}

bool XSZDD::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    // Delete format-specific context
    if (pState->pContext) {
        SZDD_UNPACK_CONTEXT *pContext = (SZDD_UNPACK_CONTEXT *)pState->pContext;
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
