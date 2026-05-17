/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xlz4.h"

XBinary::XCONVERT _TABLE_XLZ4_STRUCTID[] = {{XLZ4::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XLZ4::STRUCTID_LZ4_FRAME_HEADER, "LZ4_FRAME_HEADER", QString("LZ4 frame header")}};

XLZ4::XLZ4(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZ4::~XLZ4()
{
}

bool XLZ4::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return (getSize() >= (qint64)sizeof(LZ4_FRAME_HEADER)) && (read_uint32(0, false) == 0x184D2204);
}

bool XLZ4::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XLZ4 lz4(pDevice);

    return lz4.isValid();
}

XBinary::MODE XLZ4::getMode()
{
    return MODE_DATA;
}

qint32 XLZ4::getType()
{
    return TYPE_LZ4;
}

QString XLZ4::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    if (nType == TYPE_LZ4) {
        sResult = QString("LZ4");
    }

    return sResult;
}

XBinary::ENDIAN XLZ4::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZ4::getFileFormatExt()
{
    return "lz4";
}

QString XLZ4::getFileFormatExtsString()
{
    return "lz4";
}

XBinary::FT XLZ4::getFileType()
{
    return FT_LZ4;
}

qint64 XLZ4::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLZ4::getMIMEString()
{
    return "application/x-lz4";
}

XBinary::OSNAME XLZ4::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLZ4::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XLZ4::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XLZ4::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZ4_STRUCTID, sizeof(_TABLE_XLZ4_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLZ4::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLZ4_STRUCTID, sizeof(_TABLE_XLZ4_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLZ4::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLZ4_STRUCTID, sizeof(_TABLE_XLZ4_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XLZ4::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_LZ4_FRAME_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else if (dataHeadersOptions.nID == STRUCTID_LZ4_FRAME_HEADER) {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLZ4::structIDToString(dataHeadersOptions.nID));
//             dataHeader.nSize = sizeof(LZ4_FRAME_HEADER);
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ4_FRAME_HEADER, nMagic), 4, "nMagic", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ4_FRAME_HEADER, nFLG), 1, "nFLG", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZ4_FRAME_HEADER, nBD), 1, "nBD", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(
//                 getDataRecord(offsetof(LZ4_FRAME_HEADER, nHeaderChecksum), 1, "nHeaderChecksum", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XLZ4::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZ4_FRAME_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LZ4_FRAME_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(LZ4_FRAME_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZ4_FRAME_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(LZ4_FRAME_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZ4_FRAME_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZ4_FRAME_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLZ4::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZ4_FRAME_HEADER) {
        listResult.append({"nMagic", (qint32)offsetof(LZ4_FRAME_HEADER, nMagic), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nFLG", (qint32)offsetof(LZ4_FRAME_HEADER, nFLG), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nBD", (qint32)offsetof(LZ4_FRAME_HEADER, nBD), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nHeaderChecksum", (qint32)offsetof(LZ4_FRAME_HEADER, nHeaderChecksum), 1, XFRECORD_FLAG_NONE, VT_UINT8});
    }

    return listResult;
}

QList<XBinary::FPART> XLZ4::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;
    qint64 nFileSize = getSize();

    if ((nFileParts & FILEPART_HEADER) && (nFileSize > 0)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>((qint64)sizeof(LZ4_FRAME_HEADER), nFileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    if ((nFileParts & FILEPART_STREAM) && (nFileSize > (qint64)sizeof(LZ4_FRAME_HEADER))) {
        FPART stream = {};
        stream.filePart = FILEPART_STREAM;
        stream.nFileOffset = sizeof(LZ4_FRAME_HEADER);
        stream.nFileSize = nFileSize - sizeof(LZ4_FRAME_HEADER);
        stream.nVirtualAddress = -1;
        stream.sName = tr("Stream");
        listResult.append(stream);
    }

    return listResult;
}

QList<QString> XLZ4::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("04224D18");

    return listResult;
}

XBinary *XLZ4::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLZ4(pDevice);
}
