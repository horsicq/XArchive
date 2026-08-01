/* Copyright (c) 2022-2026 hors<horsicq@gmail.com>
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

namespace {
class GzipDiscardDevice : public QIODevice {
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
}  // namespace

XBinary::XCONVERT _TABLE_XGZIP_STRUCTID[] = {{XGzip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XGzip::STRUCTID_GZIP_HEADER, "GZIP_HEADER", QString("GZIP header")},
                                             {XGzip::STRUCTID_STREAM, "STREAM", QString("Stream")}};

static XBinary::PM_INFO createPMInfo(XBinary::HANDLE_METHOD hm0, XBinary::HANDLE_METHOD hm1 = XBinary::HANDLE_METHOD_UNKNOWN,
                                     XBinary::HANDLE_METHOD hm2 = XBinary::HANDLE_METHOD_UNKNOWN, XBinary::HANDLE_METHOD hm3 = XBinary::HANDLE_METHOD_UNKNOWN)
{
    XBinary::PM_INFO result = {};

    result.hm[0] = hm0;
    result.hm[1] = hm1;
    result.hm[2] = hm2;
    result.hm[3] = hm3;

    return result;
}

XGzip::XGzip(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGzip::~XGzip()
{
}

bool XGzip::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    qint64 nHeaderSize = 0;
    bool bResult = _getHeaderInfo(&nHeaderSize);

    return bResult;
}

bool XGzip::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGzip xgzip(pDevice);

    return xgzip.isValid(pPdStruct);
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

    GZIP_UNPACK_CONTEXT context = {};
    const bool bMemberInfo = _getFirstMemberInfo(&context, pPdStruct);
    const qint64 nOffset = bMemberInfo ? context.nHeaderSize : getHeaderSize();

    memoryRecordHeader.nOffset = 0;
    memoryRecordHeader.nAddress = -1;
    memoryRecordHeader.nSize = nOffset;
    memoryRecordHeader.sName = tr("Header");
    memoryRecordHeader.filePart = FILEPART_HEADER;

    result.listRecords.append(memoryRecordHeader);

    memoryRecord.nOffset = nOffset;
    memoryRecord.nAddress = -1;
    memoryRecord.nSize = bMemberInfo ? context.nCompressedSize : 0;
    memoryRecord.sName = tr("Stream");
    memoryRecord.filePart = FILEPART_REGION;

    result.listRecords.append(memoryRecord);

    if (bMemberInfo && context.bFooterValid) {
        memoryRecordFooter.nOffset = memoryRecord.nOffset + memoryRecord.nSize;
        memoryRecordFooter.nAddress = -1;
        memoryRecordFooter.nSize = 8;
        memoryRecordFooter.sName = tr("Footer");
        memoryRecordFooter.filePart = FILEPART_FOOTER;

        result.listRecords.append(memoryRecordFooter);
    }

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

    GZIP_UNPACK_CONTEXT context = {};
    const bool bMemberInfo = _getFirstMemberInfo(&context, pPdStruct);
    const qint64 headerSize = bMemberInfo ? context.nHeaderSize : getHeaderSize();

    // Header
    if (nFileParts & FILEPART_HEADER) {
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
        const qint64 payloadOffset = headerSize;
        const qint64 payloadSize = bMemberInfo ? context.nCompressedSize : 0;

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
        if (bMemberInfo && context.bFooterValid) {
            FPART footer = {};
            footer.filePart = FILEPART_FOOTER;
            footer.nFileOffset = context.nHeaderSize + context.nCompressedSize;
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

QString XGzip::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XGZIP_STRUCTID, sizeof(_TABLE_XGZIP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XGzip::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XGZIP_STRUCTID, sizeof(_TABLE_XGZIP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XGzip::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_GZIP_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_GZIP_HEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XGzip::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = sizeof(GZIP_HEADER);

//                 dataHeader.listRecords.append(getDataRecord(offsetof(GZIP_HEADER, nMagic), 2, "nMagic", VT_UINT16, DRF_UNKNOWN,
//                 dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(
//                     getDataRecord(offsetof(GZIP_HEADER, nCompressionMethod), 1, "nCompressionMethod", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(GZIP_HEADER, nFileFlags), 1, "nFileFlags", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(GZIP_HEADER, nTimeStamp), 4, "nTimeStamp", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(GZIP_HEADER, nCompressionFlags), 1, "nCompressionFlags", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(GZIP_HEADER, nOS), 1, "nOS", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XGzip::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_GZIP_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_GZIP_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(GZIP_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_GZIP_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(GZIP_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_GZIP_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_GZIP_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XGzip::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_GZIP_HEADER) {
        listResult.append({"nMagic", (qint32)offsetof(GZIP_HEADER, nMagic), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"nCompressionMethod", (qint32)offsetof(GZIP_HEADER, nCompressionMethod), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nFileFlags", (qint32)offsetof(GZIP_HEADER, nFileFlags), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nTimeStamp", (qint32)offsetof(GZIP_HEADER, nTimeStamp), 4, XFRECORD_FLAG_UNIXTIME, VT_UINT32});
        listResult.append({"nCompressionFlags", (qint32)offsetof(GZIP_HEADER, nCompressionFlags), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nOS", (qint32)offsetof(GZIP_HEADER, nOS), 1, XFRECORD_FLAG_NONE, VT_UINT8});
    }

    return listResult;
}

XGzip::GZIP_HEADER XGzip::_read_GZIP_HEADER(qint64 nOffset)
{
    GZIP_HEADER result = {};

    read_array(nOffset, (char *)&result, sizeof(GZIP_HEADER));

    return result;
}

qint64 XGzip::getHeaderSize()
{
    qint64 nResult = 0;

    _getHeaderInfo(&nResult);

    return nResult;
}

bool XGzip::_getHeaderInfo(qint64 *pHeaderSize, QString *pFileName)
{
    const qint64 nFixedHeaderSize = (qint64)sizeof(GZIP_HEADER);
    const qint64 nFooterSize = 8;
    const qint64 nFileSize = getSize();

    if (pHeaderSize) {
        *pHeaderSize = 0;
    }

    if (pFileName) {
        pFileName->clear();
    }

    if (nFileSize < (nFixedHeaderSize + nFooterSize)) {
        return false;
    }

    QByteArray baHeader = read_array(0, nFixedHeaderSize);

    if (baHeader.size() != nFixedHeaderSize) {
        return false;
    }

    const quint8 nID1 = (quint8)baHeader.at(0);
    const quint8 nID2 = (quint8)baHeader.at(1);
    const quint8 nCompressionMethod = (quint8)baHeader.at(2);
    const quint8 nFlags = (quint8)baHeader.at(3);

    // Bits 5..7 are reserved by RFC 1952 and must be zero.
    if ((nID1 != 0x1f) || (nID2 != 0x8b) || (nCompressionMethod != 8) || (nFlags & 0xe0)) {
        return false;
    }

    qint64 nOffset = nFixedHeaderSize;
    const qint64 nHeaderLimit = nFileSize - nFooterSize;

    auto hasHeaderBytes = [&nOffset, nHeaderLimit](qint64 nSize) -> bool {
        return (nSize >= 0) && (nOffset >= 0) && (nOffset <= nHeaderLimit) && (nSize <= (nHeaderLimit - nOffset));
    };

    auto readZeroTerminatedField = [this, &nOffset, nHeaderLimit](QString *pValue) -> bool {
        QByteArray baValue;
        const qint32 nMaxStoredStringSize = 0x10000;

        while (nOffset < nHeaderLimit) {
            const qint64 nChunkSize = qMin<qint64>(0x1000, nHeaderLimit - nOffset);
            QByteArray baChunk = read_array(nOffset, nChunkSize);

            if (baChunk.size() != nChunkSize) {
                return false;
            }

            const qint32 nTerminatorIndex = baChunk.indexOf('\0');
            const qint32 nValueSize = (nTerminatorIndex == -1) ? baChunk.size() : nTerminatorIndex;

            if (pValue && (baValue.size() < nMaxStoredStringSize)) {
                const qint32 nCopySize = qMin(nValueSize, nMaxStoredStringSize - baValue.size());

                if (nCopySize > 0) {
                    baValue.append(baChunk.constData(), nCopySize);
                }
            }

            nOffset += nValueSize;

            if (nTerminatorIndex != -1) {
                nOffset++;  // Include the terminating zero byte.

                if (pValue) {
                    *pValue = QString::fromLatin1(baValue);
                }

                return true;
            }
        }

        return false;
    };

    // FEXTRA: two-byte little-endian XLEN followed by XLEN bytes.
    if (nFlags & 0x04) {
        if (!hasHeaderBytes(2)) {
            return false;
        }

        QByteArray baLength = read_array(nOffset, 2);

        if (baLength.size() != 2) {
            return false;
        }

        const quint16 nExtraLength = (quint16)(quint8)baLength.at(0) | ((quint16)(quint8)baLength.at(1) << 8);
        nOffset += 2;

        if (!hasHeaderBytes(nExtraLength)) {
            return false;
        }

        nOffset += nExtraLength;
    }

    // FNAME and FCOMMENT are zero-terminated ISO-8859-1 byte strings.
    if ((nFlags & 0x08) && !readZeroTerminatedField(pFileName)) {
        return false;
    }

    if ((nFlags & 0x10) && !readZeroTerminatedField(nullptr)) {
        return false;
    }

    // FHCRC is the low 16 bits of the CRC32 of all preceding header bytes.
    if (nFlags & 0x02) {
        if (!hasHeaderBytes(2)) {
            return false;
        }

        nOffset += 2;
    }

    if (pHeaderSize) {
        *pHeaderSize = nOffset;
    }

    return true;
}

bool XGzip::_getFirstMemberInfo(GZIP_UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) {
        return false;
    }

    *pContext = GZIP_UNPACK_CONTEXT();

    if (!_getHeaderInfo(&pContext->nHeaderSize, &pContext->sFileName)) {
        return false;
    }

    if (pContext->sFileName.isEmpty()) {
        pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());
    }

    const qint64 nFileSize = getSize();
    const qint64 nRemainingSize = nFileSize - pContext->nHeaderSize;

    if (nRemainingSize <= 0) {
        return false;
    }

    SubDevice sd(getDevice(), pContext->nHeaderSize, nRemainingSize);
    GzipDiscardDevice discardDevice;

    if (!sd.open(QIODevice::ReadOnly) || !discardDevice.open(QIODevice::WriteOnly)) {
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
    state.pDeviceInput = &sd;
    state.pDeviceOutput = &discardDevice;
    state.nInputOffset = 0;
    state.nInputLimit = nRemainingSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    const bool bDecoded = XDeflateDecoder::decompress(&state, pPdStruct);

    discardDevice.close();
    sd.close();

    if (!bDecoded || (state.nCountInput <= 0) || (state.nCountInput > nRemainingSize) || (state.nCountOutput < 0)) {
        return false;
    }

    pContext->nCompressedSize = state.nCountInput;
    pContext->nUncompressedSize = state.nCountOutput;

    const qint64 nFooterOffset = pContext->nHeaderSize + pContext->nCompressedSize;

    if ((nFooterOffset >= pContext->nHeaderSize) && (nFooterOffset <= (nFileSize - 8))) {
        QByteArray baFooter = read_array(nFooterOffset, 8);

        if (baFooter.size() == 8) {
            pContext->nCRC32 = (quint32)(quint8)baFooter.at(0) | ((quint32)(quint8)baFooter.at(1) << 8) | ((quint32)(quint8)baFooter.at(2) << 16) |
                               ((quint32)(quint8)baFooter.at(3) << 24);
            pContext->bFooterValid = true;
        }
    }

    return true;
}

QList<XBinary::PM_INFO> XGzip::unpackImplemented()
{
    QList<PM_INFO> listResult;

    listResult.append(createPMInfo(HANDLE_METHOD_DEFLATE));

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XGzip::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XGzip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

        GZIP_UNPACK_CONTEXT *pContext = new GZIP_UNPACK_CONTEXT();

        if (!_getFirstMemberInfo(pContext, pPdStruct)) {
            delete pContext;
            return false;
        }

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;  // GZIP contains single compressed stream
        pState->mapUnpackProperties = mapProperties;
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
    // result.nDecompressedOffset = 0;
    // result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);

    if (pContext->bFooterValid) {
        result.mapProperties.insert(FPART_PROP_RESULTCRC, pContext->nCRC32);
        result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    }

    return result;
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

QList<XBinary::FPART_PROP> XGzip::getAvailableFPARTProperties()
{
    QList<XBinary::FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
}

QList<QString> XGzip::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("1F8B08");

    return listResult;
}

XBinary *XGzip::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XGzip(pDevice);
}

bool XGzip::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XGzip::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XGzip::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
