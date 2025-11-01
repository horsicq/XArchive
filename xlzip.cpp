/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#include "xlzip.h"

XBinary::XCONVERT _TABLE_XLZIP_STRUCTID[] = {{XLzip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XLzip::STRUCTID_LZIP_HEADER, "LZIP_HEADER", QString("LZIP header")},
                                             {XLzip::STRUCTID_MEMBER_HEADER, "MEMBER_HEADER", QString("Member header")},
                                             {XLzip::STRUCTID_COMPRESSED_DATA, "COMPRESSED_DATA", QString("Compressed data")},
                                             {XLzip::STRUCTID_FOOTER, "FOOTER", QString("Footer")}};

XLzip::XLzip(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLzip::~XLzip()
{
}

bool XLzip::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 6) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "'LZIP'", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XLzip::isValid(QIODevice *pDevice)
{
    bool bResult = false;

    if (pDevice && pDevice->seek(0)) {
        char magic[4];
        if (pDevice->read(magic, 4) == 4) {
            if ((magic[0] == 'L') && (magic[1] == 'Z') && (magic[2] == 'I') && (magic[3] == 'P')) {
                bResult = true;
            }
        }
    }

    return bResult;
}

XBinary::MODE XLzip::getMode()
{
    return MODE_DATA;
}

qint32 XLzip::getType()
{
    return TYPE_LZ;
}

QString XLzip::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_LZ: sResult = QString("LZ"); break;
    }

    return sResult;
}

QString XLzip::getFileFormatExt()
{
    return "lz";
}

XBinary::FT XLzip::getFileType()
{
    return FT_LZIP;
}

QString XLzip::getFileFormatExtsString()
{
    return "lz";
}

qint64 XLzip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLzip::getMIMEString()
{
    return "application/x-lzip";
}

XBinary::ENDIAN XLzip::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::OSNAME XLzip::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLzip::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XLzip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    } else if (mapMode == MAPMODE_DATA) {
        result = _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    }

    return result;
}

QString XLzip::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZIP_STRUCTID, sizeof(_TABLE_XLZIP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XLzip::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_LZIP_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        if (isPdStructNotCanceled(pPdStruct)) {
            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_LZIP_HEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLzip::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = 6;  // Minimum header size

                dataHeader.listRecords.append(getDataRecord(0, 4, "magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(4, 1, "version", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(5, 1, "dictSizeCode", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

quint64 XLzip::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return 1;
}

QList<XArchive::RECORD> XLzip::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listResult;
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    // Placeholder: Lzip decompression would go here if a decoder is available
    RECORD record = {};
    record.nHeaderOffset = 0;
    record.nHeaderSize = 6;
    record.nDataOffset = 6;
    record.nDataSize = getSize() - 13;  // Approximate
    record.spInfo.sRecordName = XBinary::getDeviceFileBaseName(getDevice());
    record.spInfo.compressMethod = COMPRESS_METHOD_LZMA;  // Lzip uses LZMA
    record.sUUID = generateUUID();

    listResult.append(record);

    return listResult;
}

QList<XBinary::FPART> XLzip::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    const qint64 nFileSize = getSize();
    if (nFileSize <= 0) return listResult;

    if (nFileParts & FILEPART_HEADER) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = 6;
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    qint64 nDataStart = 6;
    qint64 nDataSize = nFileSize - 13;  // Excluding header (6) and footer (7)

    if (nFileParts & FILEPART_STREAM) {
        FPART region = {};
        region.filePart = FILEPART_STREAM;
        region.nFileOffset = nDataStart;
        region.nFileSize = nDataSize;
        region.nVirtualAddress = -1;
        region.sName = tr("Stream");
        region.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZMA);
        listResult.append(region);
    }

    if (nFileParts & FILEPART_DATA) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = nDataStart;
        data.nFileSize = nDataSize;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nFileSize > nDataStart + nDataSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = nDataStart + nDataSize;
            ov.nFileSize = nFileSize - (nDataStart + nDataSize);
            ov.nVirtualAddress = -1;
            ov.sName = tr("Footer");
            listResult.append(ov);
        }
    }

    return listResult;
}

XLzip::LZIP_HEADER XLzip::_read_LZIP_HEADER(qint64 nOffset)
{
    LZIP_HEADER result = {};

    read_array(nOffset, result.magic, 4);
    result.nVersion = read_uint8(nOffset + 4);
    result.nDictSizeCode = read_uint8(nOffset + 5);

    return result;
}

quint32 XLzip::_getDictionarySize(quint8 nDictSizeCode)
{
    // Dictionary size = 2^(n+16) - 35
    // Clamped to reasonable values
    if (nDictSizeCode > 28) {
        return 0;  // Invalid
    }

    quint32 nSize = (1U << (nDictSizeCode + 16)) - 35;
    return nSize;
}

bool XLzip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)
    
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        if (!isValid(pPdStruct)) {
            return false;
        }

        LZIP_UNPACK_CONTEXT *pContext = new LZIP_UNPACK_CONTEXT;
        pContext->nHeaderSize = 6;
        pContext->nCompressedSize = getSize() - 13;  // Approximate
        pContext->nUncompressedSize = 0;             // Would need to decompress
        pContext->nCRC32 = 0;

        // Read uncompressed size from footer
        qint64 nFooterPos = getSize() - 8;
        if (nFooterPos > 0) {
            pContext->nUncompressedSize = read_uint64(nFooterPos, true);
        }

        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;
        pState->pContext = pContext;

        bResult = true;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XLzip::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && pState->pContext) {
        LZIP_UNPACK_CONTEXT *pContext = (LZIP_UNPACK_CONTEXT *)pState->pContext;

        result.nStreamOffset = pContext->nHeaderSize;
        result.nStreamSize = pContext->nCompressedSize;
        result.nDecompressedOffset = 0;
        result.nDecompressedSize = pContext->nUncompressedSize;

        result.mapProperties[XBinary::FPART_PROP_ORIGINALNAME] = XBinary::getDeviceFileBaseName(getDevice());
        result.mapProperties[XBinary::FPART_PROP_COMPRESSEDSIZE] = pContext->nCompressedSize;
        result.mapProperties[XBinary::FPART_PROP_UNCOMPRESSEDSIZE] = pContext->nUncompressedSize;
        result.mapProperties[XBinary::FPART_PROP_CRC_VALUE] = pContext->nCRC32;
    }

    return result;
}

bool XLzip::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    LZIP_UNPACK_CONTEXT *pContext = (LZIP_UNPACK_CONTEXT *)pState->pContext;

    // Decompress entire lzip stream to output device
    qint64 nFileSize = getSize();
    SubDevice sd(getDevice(), pContext->nHeaderSize, pContext->nCompressedSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZMA);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDevice;
        state.nInputOffset = 0;
        state.nInputLimit = -1;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bResult = XLZMADecoder::decompress(&state, pPdStruct);

        sd.close();
    }

    return bResult;
}

bool XLzip::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords - 1)) {
        pState->nCurrentIndex++;
        bResult = true;
    }

    return bResult;
}

bool XLzip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        delete (LZIP_UNPACK_CONTEXT *)pState->pContext;
        pState->pContext = nullptr;
        bResult = true;
    }

    return bResult;
}
