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
#include "xwim.h"

static XBinary::XCONVERT _TABLE_XWIM_STRUCTID[] = {{XWIM::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XWIM::STRUCTID_WIM_HEADER, "WIM_HEADER", QString("WIM header")}};

static quint16 read_uint16_le(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 2 > baData.size())) {
        return 0;
    }

    const uchar *p = reinterpret_cast<const uchar *>(baData.constData() + nOffset);

    return (quint16)p[0] | ((quint16)p[1] << 8);
}

static quint32 read_uint32_le(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 4 > baData.size())) {
        return 0;
    }

    const uchar *p = reinterpret_cast<const uchar *>(baData.constData() + nOffset);

    return (quint32)p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24);
}

static quint64 read_uint64_le(const QByteArray &baData, qint64 nOffset)
{
    if ((nOffset < 0) || (nOffset + 8 > baData.size())) {
        return 0;
    }

    quint64 nResult = 0;
    const uchar *p = reinterpret_cast<const uchar *>(baData.constData() + nOffset);

    for (qint32 i = 7; i >= 0; i--) {
        nResult <<= 8;
        nResult |= p[i];
    }

    return nResult;
}

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

XWIM::XWIM(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWIM::~XWIM()
{
}

bool XWIM::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;
    qint64 nSize = getSize();

    if (nSize >= WIM_HEADER_SIZE_OLD) {
        QByteArray baSignature = read_array(0, WIM_SIGNATURE_SIZE);

        if ((baSignature.size() == WIM_SIGNATURE_SIZE) && (baSignature == QByteArray("MSWIM\0\0\0", WIM_SIGNATURE_SIZE))) {
            quint32 nHeaderSize = read_uint32(8);
            quint32 nVersion = read_uint32(0x0C);

            if ((nHeaderSize >= WIM_HEADER_SIZE_OLD) && (nHeaderSize <= WIM_HEADER_SIZE_NEW) && ((qint64)nHeaderSize <= nSize) &&
                _isSupportedVersion(nVersion, nHeaderSize)) {
                if (nHeaderSize == WIM_HEADER_SIZE_OLD) {
                    bResult = true;
                } else {
                    quint16 nPartNumber = read_uint16(0x28);
                    quint16 nNumberOfParts = read_uint16(0x2A);
                    bResult = (nPartNumber != 0) && (nNumberOfParts != 0) && (nPartNumber <= nNumberOfParts);
                }
            }
        }
    }

    return bResult;
}

bool XWIM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWIM xwim(pDevice);

    return xwim.isValid(pPdStruct);
}

XBinary::FT XWIM::getFileType()
{
    return FT_WIM;
}

QString XWIM::getMIMEString()
{
    return "application/x-ms-wim";
}

QString XWIM::getFileFormatExt()
{
    return "wim";
}

QString XWIM::getFileFormatExtsString()
{
    return "WIM (*.wim *.swm *.esd *.ppkg)";
}

qint64 XWIM::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return getSize();
}

XBinary::MODE XWIM::getMode()
{
    return MODE_DATA;
}

qint32 XWIM::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XWIM::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::OSNAME XWIM::getOsName()
{
    return OSNAME_WINDOWS;
}

QString XWIM::getVersion()
{
    QString sResult;

    if (getSize() >= WIM_HEADER_SIZE_OLD) {
        quint32 nVersion = read_uint32(0x0C);
        sResult = QString("%1.%2.%3").arg((nVersion >> 16) & 0xFF).arg((nVersion >> 8) & 0xFF).arg(nVersion & 0xFF);
    }

    return sResult;
}

QString XWIM::getArch()
{
    return QString();
}

QList<XBinary::MAPMODE> XWIM::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XWIM::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    return _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);
}

QString XWIM::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XWIM_STRUCTID, sizeof(_TABLE_XWIM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XWIM::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XWIM_STRUCTID, sizeof(_TABLE_XWIM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XWIM::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XWIM_STRUCTID, sizeof(_TABLE_XWIM_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XWIM::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap ? dataHeadersOptions.pMemoryMap->fileType : getFileType();
//         _dataHeadersOptions.nID = STRUCTID_WIM_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else if (dataHeadersOptions.nID == STRUCTID_WIM_HEADER) {
//         qint64 nStartOffset = -1;

//         if (dataHeadersOptions.pMemoryMap) {
//             nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);
//         } else if (dataHeadersOptions.locType == LT_OFFSET) {
//             nStartOffset = dataHeadersOptions.nLocation;
//         }

//         if (nStartOffset != -1) {
//             quint32 nHeaderSize = read_uint32(nStartOffset + 8);

//             if ((nHeaderSize >= WIM_HEADER_SIZE_OLD) && (nHeaderSize <= WIM_HEADER_SIZE_NEW)) {
//                 ENDIAN endian = dataHeadersOptions.pMemoryMap ? dataHeadersOptions.pMemoryMap->endian : getEndian();
//                 quint32 nVersion = read_uint32(nStartOffset + 0x0C);
//                 bool bHasNewFields = (nVersion == 0x00000E00) || (nVersion >= 0x00010D00) || (nHeaderSize == WIM_HEADER_SIZE_NEW);
//                 DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XWIM::structIDToString(dataHeadersOptions.nID));
//                 dataHeader.nSize = nHeaderSize;

//                 dataHeader.listRecords.append(getDataRecord(0x00, 8, "signature", VT_CHAR_ARRAY, DRF_UNKNOWN, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x08, 4, "headerSize", VT_UINT32, DRF_SIZE, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x0C, 4, "version", VT_UINT32, DRF_VERSION, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x10, 4, "flags", VT_UINT32, DRF_UNKNOWN, endian));
//                 dataHeader.listRecords.append(getDataRecord(0x14, 4, "chunkSize", VT_UINT32, DRF_SIZE, endian));

//                 if (nHeaderSize > WIM_HEADER_SIZE_OLD) {
//                     dataHeader.listRecords.append(getDataRecord(0x18, 16, "guid", VT_BYTE_ARRAY, DRF_UNKNOWN, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x28, 2, "partNumber", VT_UINT16, DRF_UNKNOWN, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x2A, 2, "numberOfParts", VT_UINT16, DRF_COUNT, endian));

//                     if (bHasNewFields) {
//                         dataHeader.listRecords.append(getDataRecord(0x2C, 4, "numberOfImages", VT_UINT32, DRF_COUNT, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x30, WIM_RESOURCE_SIZE, "offsetTableResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x48, WIM_RESOURCE_SIZE, "xmlResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x60, WIM_RESOURCE_SIZE, "bootMetadataResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x78, 4, "bootIndex", VT_UINT32, DRF_UNKNOWN, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x7C, WIM_RESOURCE_SIZE, "integrityResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     } else {
//                         dataHeader.listRecords.append(getDataRecord(0x2C, WIM_RESOURCE_SIZE, "offsetTableResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x44, WIM_RESOURCE_SIZE, "xmlResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                         dataHeader.listRecords.append(getDataRecord(0x5C, WIM_RESOURCE_SIZE, "bootMetadataResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     }
//                 } else {
//                     dataHeader.listRecords.append(getDataRecord(0x18, WIM_RESOURCE_SIZE, "offsetTableResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x30, WIM_RESOURCE_SIZE, "xmlResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                     dataHeader.listRecords.append(getDataRecord(0x48, WIM_RESOURCE_SIZE, "bootMetadataResource", VT_BYTE_ARRAY, DRF_OFFSET, endian));
//                 }

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::FPART> XWIM::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;
    qint64 nFileSize = getSize();

    if (nFileSize <= 0) {
        return listResult;
    }

    WIM_HEADER header = readWIMHeader();
    qint64 nHeaderSize = header.nHeaderSize;

    if ((nHeaderSize < WIM_HEADER_SIZE_OLD) || (nHeaderSize > WIM_HEADER_SIZE_NEW) || (nHeaderSize > nFileSize)) {
        nHeaderSize = qMin<qint64>(WIM_HEADER_SIZE_OLD, nFileSize);
    }

    qint64 nMaxKnownEnd = nHeaderSize;

    if (nFileParts & FILEPART_HEADER) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = nHeaderSize;
        part.nVirtualAddress = -1;
        part.sName = tr("Header");
        listResult.append(part);
    }

    _appendResourcePart(&listResult, nFileParts, header.offsetTableResource, tr("Offset Table"), nLimit);
    _appendResourcePart(&listResult, nFileParts, header.xmlResource, tr("XML Data"), nLimit);
    _appendResourcePart(&listResult, nFileParts, header.bootMetadataResource, tr("Boot Metadata"), nLimit);
    _appendResourcePart(&listResult, nFileParts, header.integrityResource, tr("Integrity"), nLimit);

    QList<RESOURCE_INFO> listResources;
    listResources.append(header.offsetTableResource);
    listResources.append(header.xmlResource);
    listResources.append(header.bootMetadataResource);
    listResources.append(header.integrityResource);

    for (qint32 i = 0; i < listResources.count(); i++) {
        RESOURCE_INFO resourceInfo = listResources.at(i);

        if ((resourceInfo.nOffset < (quint64)nFileSize) && (resourceInfo.nPackSize > 0)) {
            nMaxKnownEnd = qMax<qint64>(nMaxKnownEnd, qMin<qint64>((qint64)(resourceInfo.nOffset + resourceInfo.nPackSize), nFileSize));
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && (nMaxKnownEnd < nFileSize)) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = nMaxKnownEnd;
        part.nFileSize = nFileSize - nMaxKnownEnd;
        part.nVirtualAddress = -1;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QList<XBinary::PM_INFO> XWIM::unpackImplemented()
{
    QList<PM_INFO> listResult;

    listResult.append(createPMInfo(HANDLE_METHOD_STORE));

    return listResult;
}

bool XWIM::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || !isValid(pPdStruct)) {
        return false;
    }

    WIM_UNPACK_CONTEXT *pContext = new WIM_UNPACK_CONTEXT;
    WIM_HEADER header = readWIMHeader();
    QList<STREAM_INFO> listStreams = _readStreamInfoList(header, pPdStruct);

    for (qint32 i = 0; (i < listStreams.count()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        const STREAM_INFO &streamInfo = listStreams.at(i);

        if ((streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA) && _isResourceStored(streamInfo.resourceInfo)) {
            QByteArray baMetadata = _readStoredResource(streamInfo.resourceInfo, pPdStruct);

            if (!baMetadata.isEmpty()) {
                if (_parseMetadata(baMetadata, listStreams, &(pContext->listRecords))) {
                    break;
                }
            }
        }
    }

    if (pContext->listRecords.isEmpty()) {
        for (qint32 i = 0; (i < listStreams.count()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            const STREAM_INFO &streamInfo = listStreams.at(i);

            if (!(streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA) && _isResourceStored(streamInfo.resourceInfo)) {
                WIM_RECORD record = {};
                record.sFileName = QString("stream_%1.bin").arg(i, 4, 10, QChar('0'));
                record.bIsFolder = false;
                record.nStreamOffset = (qint64)streamInfo.resourceInfo.nOffset;
                record.nStreamSize = (qint64)streamInfo.resourceInfo.nPackSize;
                record.nUncompressedSize = (qint64)streamInfo.resourceInfo.nUnpackSize;
                record.handleMethod = HANDLE_METHOD_STORE;

                pContext->listRecords.append(record);
            }
        }
    }

    if (!pContext->listRecords.isEmpty()) {
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = pContext->listRecords.count();
        pState->mapUnpackProperties = mapProperties;
        pState->pContext = pContext;

        bResult = true;
    } else {
        delete pContext;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XWIM::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    WIM_UNPACK_CONTEXT *pContext = (WIM_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const WIM_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = record.nStreamOffset;
    result.nStreamSize = record.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, record.handleMethod);

    if (record.bIsFolder) {
        result.mapProperties.insert(FPART_PROP_ISFOLDER, true);
    }

    if (record.mtDateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, record.mtDateTime);
    }

    return result;
}

bool XWIM::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        pState->nCurrentIndex++;
        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XWIM::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        WIM_UNPACK_CONTEXT *pContext = (WIM_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}

QList<XBinary::FPART_PROP> XWIM::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_ISFOLDER);
    listResult.append(FPART_PROP_DATETIME);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
}

XWIM::WIM_HEADER XWIM::readWIMHeader(qint64 nOffset)
{
    WIM_HEADER result = {};

    if ((nOffset < 0) || (getSize() < nOffset + WIM_HEADER_SIZE_OLD)) {
        return result;
    }

    result.nHeaderSize = read_uint32(nOffset + 0x08);
    result.nVersion = read_uint32(nOffset + 0x0C);
    result.nFlags = read_uint32(nOffset + 0x10);
    result.nChunkSize = read_uint32(nOffset + 0x14);

    if (result.nHeaderSize == WIM_HEADER_SIZE_OLD) {
        result.nPartNumber = 1;
        result.nNumberOfParts = 1;
        result.offsetTableResource = readResourceInfo(nOffset + 0x18);
        result.xmlResource = readResourceInfo(nOffset + 0x30);
        result.bootMetadataResource = readResourceInfo(nOffset + 0x48);
    } else if (result.nHeaderSize >= 0x74) {
        result.baGuid = read_array(nOffset + 0x18, 16);
        result.nPartNumber = read_uint16(nOffset + 0x28);
        result.nNumberOfParts = read_uint16(nOffset + 0x2A);

        qint64 nResourceOffset = nOffset + 0x2C;

        if ((result.nVersion == 0x00000E00) || (result.nVersion >= 0x00010D00) || (result.nHeaderSize == WIM_HEADER_SIZE_NEW)) {
            result.nNumberOfImages = read_uint32(nOffset + 0x2C);
            nResourceOffset = nOffset + 0x30;
        }

        result.offsetTableResource = readResourceInfo(nResourceOffset);
        result.xmlResource = readResourceInfo(nResourceOffset + 0x18);
        result.bootMetadataResource = readResourceInfo(nResourceOffset + 0x30);

        if ((result.nVersion == 0x00000E00) || (result.nVersion >= 0x00010D00) || (result.nHeaderSize == WIM_HEADER_SIZE_NEW)) {
            result.nBootIndex = read_uint32(nResourceOffset + 0x48);
            result.integrityResource = readResourceInfo(nResourceOffset + 0x4C);
        }
    }

    return result;
}

XWIM::RESOURCE_INFO XWIM::readResourceInfo(qint64 nOffset)
{
    RESOURCE_INFO result = {};

    if ((nOffset >= 0) && (getSize() >= nOffset + WIM_RESOURCE_SIZE)) {
        quint64 nPackSizeAndFlags = read_uint64(nOffset);
        result.nFlags = (quint8)(nPackSizeAndFlags >> 56);
        result.nPackSize = nPackSizeAndFlags & 0x00FFFFFFFFFFFFFFULL;
        result.nOffset = read_uint64(nOffset + 8);
        result.nUnpackSize = read_uint64(nOffset + 16);
    }

    return result;
}

QString XWIM::compressionMethodToString()
{
    QString sResult = "Store";
    quint32 nFlags = read_uint32(0x10);

    if (nFlags & HEADER_FLAG_COMPRESSION) {
        if (nFlags & HEADER_FLAG_LZX) {
            sResult = "LZX";
        } else if (nFlags & HEADER_FLAG_LZMS) {
            sResult = "LZMS";
        } else if ((nFlags & HEADER_FLAG_XPRESS) || (nFlags & HEADER_FLAG_XPRESS2)) {
            sResult = "XPRESS";
        } else {
            sResult = tr("Compressed");
        }
    }

    return sResult;
}

bool XWIM::_isSupportedVersion(quint32 nVersion, quint32 nHeaderSize) const
{
    bool bResult = false;

    if ((nVersion == 0x00000E00) && (nHeaderSize == WIM_HEADER_SIZE_NEW)) {
        bResult = true;
    } else if ((nVersion >= 0x00010900) && (nVersion <= 0x00020E00)) {
        if (nVersion <= 0x00010A00) {
            bResult = (nHeaderSize == WIM_HEADER_SIZE_OLD);
        } else if ((nVersion == 0x00010B00) && (nHeaderSize == WIM_HEADER_SIZE_OLD)) {
            bResult = true;
        } else {
            bResult = (nHeaderSize >= 0x74) && (nHeaderSize <= WIM_HEADER_SIZE_NEW);
        }
    }

    return bResult;
}

void XWIM::_appendResourcePart(QList<FPART> *pListResult, quint32 nFileParts, const RESOURCE_INFO &resourceInfo, const QString &sName, qint32 nLimit)
{
    if (!(nFileParts & FILEPART_REGION)) {
        return;
    }

    if ((nLimit != -1) && (pListResult->count() >= nLimit)) {
        return;
    }

    qint64 nFileSize = getSize();

    if ((resourceInfo.nPackSize > 0) && (resourceInfo.nOffset < (quint64)nFileSize)) {
        qint64 nPartSize = qMin<qint64>((qint64)resourceInfo.nPackSize, nFileSize - (qint64)resourceInfo.nOffset);

        if (nPartSize > 0) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = (qint64)resourceInfo.nOffset;
            part.nFileSize = nPartSize;
            part.nVirtualAddress = -1;
            part.sName = sName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)resourceInfo.nPackSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)resourceInfo.nUnpackSize);

            pListResult->append(part);
        }
    }
}

bool XWIM::_isResourceStored(const RESOURCE_INFO &resourceInfo) const
{
    return ((resourceInfo.nFlags & (RESOURCE_FLAG_COMPRESSED | RESOURCE_FLAG_SOLID)) == 0) && (resourceInfo.nPackSize == resourceInfo.nUnpackSize);
}

QByteArray XWIM::_readStoredResource(const RESOURCE_INFO &resourceInfo, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QByteArray baResult;

    if (!_isResourceStored(resourceInfo)) {
        return baResult;
    }

    if ((resourceInfo.nPackSize == 0) || (resourceInfo.nPackSize > (quint64)INT_MAX)) {
        return baResult;
    }

    if ((resourceInfo.nOffset >= (quint64)getSize()) || (resourceInfo.nOffset + resourceInfo.nPackSize > (quint64)getSize())) {
        return baResult;
    }

    baResult = read_array((qint64)resourceInfo.nOffset, (qint64)resourceInfo.nPackSize);

    return baResult;
}

QList<XWIM::STREAM_INFO> XWIM::_readStreamInfoList(const WIM_HEADER &header, PDSTRUCT *pPdStruct)
{
    QList<STREAM_INFO> listResult;

    QByteArray baOffsetTable = _readStoredResource(header.offsetTableResource, pPdStruct);
    qint32 nNumberOfStreams = baOffsetTable.size() / WIM_STREAM_INFO_SIZE;

    for (qint32 i = 0; i < nNumberOfStreams; i++) {
        qint64 nOffset = (qint64)i * WIM_STREAM_INFO_SIZE;
        STREAM_INFO streamInfo = {};
        quint64 nPackSizeAndFlags = read_uint64_le(baOffsetTable, nOffset);

        streamInfo.resourceInfo.nFlags = (quint8)(nPackSizeAndFlags >> 56);
        streamInfo.resourceInfo.nPackSize = nPackSizeAndFlags & 0x00FFFFFFFFFFFFFFULL;
        streamInfo.resourceInfo.nOffset = read_uint64_le(baOffsetTable, nOffset + 8);
        streamInfo.resourceInfo.nUnpackSize = read_uint64_le(baOffsetTable, nOffset + 16);
        streamInfo.nPartNumber = read_uint16_le(baOffsetTable, nOffset + 24);
        streamInfo.nRefCount = read_uint32_le(baOffsetTable, nOffset + 26);
        streamInfo.baHash = baOffsetTable.mid(nOffset + 30, WIM_HASH_SIZE);

        listResult.append(streamInfo);
    }

    return listResult;
}

bool XWIM::_parseMetadata(const QByteArray &baMetadata, const QList<STREAM_INFO> &listStreams, QList<WIM_RECORD> *pListRecords)
{
    if (!pListRecords || (baMetadata.size() < 8)) {
        return false;
    }

    QMap<QByteArray, STREAM_INFO> mapStreams;

    for (qint32 i = 0; i < listStreams.count(); i++) {
        const STREAM_INFO &streamInfo = listStreams.at(i);

        if (!(streamInfo.resourceInfo.nFlags & RESOURCE_FLAG_METADATA) && !_isEmptyHash(streamInfo.baHash) && !mapStreams.contains(streamInfo.baHash)) {
            mapStreams.insert(streamInfo.baHash, streamInfo);
        }
    }

    quint32 nTotalLength = read_uint32_le(baMetadata, 0);
    qint64 nDirOffset = 8;

    if (nTotalLength != 0) {
        if ((nTotalLength < 8) || (nTotalLength > (quint32)baMetadata.size())) {
            return false;
        }

        quint32 nNumberOfSecurityEntries = read_uint32_le(baMetadata, 4);

        if (nNumberOfSecurityEntries > ((nTotalLength - 8) >> 3)) {
            return false;
        }

        qint64 nEntryOffset = 8;
        quint32 nSecurityEnd = 8 + nNumberOfSecurityEntries * 8;

        for (quint32 i = 0; i < nNumberOfSecurityEntries; i++, nEntryOffset += 8) {
            quint64 nSecuritySize = read_uint64_le(baMetadata, nEntryOffset);

            if (nSecuritySize > (quint64)nTotalLength - nSecurityEnd) {
                return false;
            }

            nSecurityEnd += (quint32)nSecuritySize;
        }

        nDirOffset = (nSecurityEnd + 7) & ~7;
    }

    if (nDirOffset > baMetadata.size()) {
        return false;
    }

    QSet<qint64> stVisited;

    return _parseMetadataDir(baMetadata, nDirOffset, QString(), mapStreams, pListRecords, &stVisited);
}

bool XWIM::_parseMetadataDir(const QByteArray &baMetadata, qint64 nOffset, const QString &sParent, const QMap<QByteArray, STREAM_INFO> &mapStreams,
                             QList<WIM_RECORD> *pListRecords, QSet<qint64> *pStVisited)
{
    const qint32 nAlignMask = 7;

    if (!pListRecords || !pStVisited || ((nOffset & nAlignMask) != 0)) {
        return false;
    }

    qint64 nCurrentOffset = nOffset;

    while (nCurrentOffset >= 0) {
        if ((nCurrentOffset + 8) > baMetadata.size()) {
            return false;
        }

        if (pStVisited->contains(nCurrentOffset)) {
            return false;
        }

        pStVisited->insert(nCurrentOffset);

        quint64 nLength = read_uint64_le(baMetadata, nCurrentOffset);

        if (nLength == 0) {
            return true;
        }

        if ((nLength & nAlignMask) || (nLength < WIM_DIR_ENTRY_SIZE) || (nLength > (quint64)baMetadata.size() - (quint64)nCurrentOffset)) {
            return false;
        }

        quint16 nAltStreams = read_uint16_le(baMetadata, nCurrentOffset + WIM_DIR_ENTRY_SIZE - 6);
        quint16 nShortNameSize = read_uint16_le(baMetadata, nCurrentOffset + WIM_DIR_ENTRY_SIZE - 4);
        quint16 nFileNameSize = read_uint16_le(baMetadata, nCurrentOffset + WIM_DIR_ENTRY_SIZE - 2);

        if ((nShortNameSize & 1) || (nFileNameSize & 1)) {
            return false;
        }

        quint32 nShortNameSizeWithTerminator = nShortNameSize ? (quint32)nShortNameSize + 2 : 0;
        quint32 nFileNameSizeWithTerminator = nFileNameSize ? (quint32)nFileNameSize + 2 : 0;

        if (((WIM_DIR_ENTRY_SIZE + nFileNameSizeWithTerminator + nShortNameSizeWithTerminator + nAlignMask) & ~nAlignMask) > nLength) {
            return false;
        }

        WIM_RECORD record = _createRecordFromMetadataItem(baMetadata, nCurrentOffset, sParent, mapStreams);

        if (!record.sFileName.isEmpty()) {
            pListRecords->append(record);
        }

        qint64 nSubdirOffset = (qint64)read_uint64_le(baMetadata, nCurrentOffset + 0x10);
        qint64 nNextOffset = nCurrentOffset + (qint64)nLength;

        for (quint16 i = 0; i < nAltStreams; i++) {
            if ((nNextOffset + 8) > baMetadata.size()) {
                return false;
            }

            quint64 nAltLength = read_uint64_le(baMetadata, nNextOffset);

            if ((nAltLength & nAlignMask) || (nAltLength < 0x28) || (nAltLength > (quint64)baMetadata.size() - (quint64)nNextOffset)) {
                return false;
            }

            nNextOffset += (qint64)nAltLength;
        }

        if (record.bIsFolder && (nSubdirOffset != 0)) {
            QString sChildParent = record.sFileName.isEmpty() ? sParent : record.sFileName;

            if (!_parseMetadataDir(baMetadata, nSubdirOffset, sChildParent, mapStreams, pListRecords, pStVisited)) {
                return false;
            }
        }

        nCurrentOffset = nNextOffset;
    }

    return true;
}

XWIM::WIM_RECORD XWIM::_createRecordFromMetadataItem(const QByteArray &baMetadata, qint64 nOffset, const QString &sParent,
                                                     const QMap<QByteArray, STREAM_INFO> &mapStreams)
{
    WIM_RECORD record = {};

    quint32 nAttributes = read_uint32_le(baMetadata, nOffset + 8);
    quint16 nFileNameSize = read_uint16_le(baMetadata, nOffset + WIM_DIR_ENTRY_SIZE - 2);
    QString sName = _readUTF16LEString(baMetadata, nOffset + WIM_DIR_ENTRY_SIZE, nFileNameSize);

    sName.replace(QLatin1Char('/'), QLatin1Char('_'));
    sName.replace(QLatin1Char('\\'), QLatin1Char('_'));

    if (!sName.isEmpty()) {
        record.sFileName = sParent.isEmpty() ? sName : (sParent + QLatin1Char('/') + sName);
    } else {
        record.sFileName = sParent;
    }

    record.bIsFolder = (nAttributes & 0x10) != 0;
    record.nStreamOffset = 0;
    record.nStreamSize = 0;
    record.nUncompressedSize = 0;
    record.handleMethod = HANDLE_METHOD_STORE;
    record.mtDateTime = winFileTimeToQDateTime(read_uint64_le(baMetadata, nOffset + 0x38));

    QByteArray baHash = baMetadata.mid(nOffset + 0x40, WIM_HASH_SIZE);

    if (!_isEmptyHash(baHash) && mapStreams.contains(baHash)) {
        STREAM_INFO streamInfo = mapStreams.value(baHash);
        record.nStreamOffset = (qint64)streamInfo.resourceInfo.nOffset;
        record.nStreamSize = (qint64)streamInfo.resourceInfo.nPackSize;
        record.nUncompressedSize = (qint64)streamInfo.resourceInfo.nUnpackSize;
        record.handleMethod = _isResourceStored(streamInfo.resourceInfo) ? HANDLE_METHOD_STORE : HANDLE_METHOD_UNKNOWN;
    }

    return record;
}

QString XWIM::_readUTF16LEString(const QByteArray &baData, qint64 nOffset, qint32 nSize)
{
    QString sResult;

    if ((nSize > 0) && ((nSize & 1) == 0) && (nOffset >= 0) && (nOffset + nSize <= baData.size())) {
        QByteArray baString = baData.mid(nOffset, nSize);
        sResult = QString::fromUtf16(reinterpret_cast<const ushort *>(baString.constData()), nSize / 2);
    }

    return sResult;
}

bool XWIM::_isEmptyHash(const QByteArray &baHash)
{
    if (baHash.size() < WIM_HASH_SIZE) {
        return true;
    }

    for (qint32 i = 0; i < WIM_HASH_SIZE; i++) {
        if (baHash.at(i) != 0) {
            return false;
        }
    }

    return true;
}

QList<QString> XWIM::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'MSWIM'000000");

    return listResult;
}

XBinary *XWIM::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XWIM(pDevice);
}
