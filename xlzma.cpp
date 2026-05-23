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
#include "xlzma.h"

XBinary::XCONVERT _TABLE_XLZMA_STRUCTID[] = {{XLZMA::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                             {XLZMA::STRUCTID_LZMA_ALONE_HEADER, "LZMA_ALONE_HEADER", QString("LZMA alone header")}};

static bool _isValidLZMAProperties(quint8 nProperties)
{
    quint8 nValue = nProperties;
    quint8 nLC = nValue % 9;
    nValue /= 9;
    quint8 nLP = nValue % 5;
    quint8 nPB = nValue / 5;

    return (nLC <= 8) && (nLP <= 4) && (nPB <= 4);
}

XLZMA::XLZMA(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZMA::~XLZMA()
{
}

bool XLZMA::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= (qint64)sizeof(LZMA_ALONE_HEADER)) {
        quint8 nProperties = read_uint8(0);
        quint8 nDictByte0 = read_uint8(1);
        quint8 nDictByte1 = read_uint8(2);
        quint8 nDictByte2 = read_uint8(3);
        quint32 nDictionarySize = read_uint32(1, false);

        bResult = (nProperties == 0x5D) && (nDictByte0 == 0x00) && (nDictByte1 == 0x00) && (nDictByte2 == 0x00) && _isValidLZMAProperties(nProperties) &&
                  (nDictionarySize != 0);
    }

    return bResult;
}

bool XLZMA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XLZMA lzma(pDevice);

    return lzma.isValid();
}

XBinary::MODE XLZMA::getMode()
{
    return MODE_DATA;
}

qint32 XLZMA::getType()
{
    return TYPE_LZMA_ALONE;
}

QString XLZMA::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    if (nType == TYPE_LZMA_ALONE) {
        sResult = QString("LZMA");
    }

    return sResult;
}

XBinary::ENDIAN XLZMA::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZMA::getFileFormatExt()
{
    return "lzma";
}

QString XLZMA::getFileFormatExtsString()
{
    return "lzma";
}

XBinary::FT XLZMA::getFileType()
{
    return FT_LZMA;
}

qint64 XLZMA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XLZMA::getMIMEString()
{
    return "application/x-lzma";
}

XBinary::OSNAME XLZMA::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XLZMA::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XLZMA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XLZMA::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLZMA_STRUCTID, sizeof(_TABLE_XLZMA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLZMA::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLZMA_STRUCTID, sizeof(_TABLE_XLZMA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLZMA::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLZMA_STRUCTID, sizeof(_TABLE_XLZMA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XLZMA::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_LZMA_ALONE_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         if (isPdStructNotCanceled(pPdStruct)) {
//             listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//         }
//     } else if (dataHeadersOptions.nID == STRUCTID_LZMA_ALONE_HEADER) {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XLZMA::structIDToString(dataHeadersOptions.nID));
//             dataHeader.nSize = sizeof(LZMA_ALONE_HEADER);
//             dataHeader.listRecords.append(getDataRecord(offsetof(LZMA_ALONE_HEADER, nProperties), 1, "nProperties", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(
//                 getDataRecord(offsetof(LZMA_ALONE_HEADER, nDictionarySize), 4, "nDictionarySize", VT_UINT32, DRF_SIZE, ENDIAN_LITTLE));
//             dataHeader.listRecords.append(
//                 getDataRecord(offsetof(LZMA_ALONE_HEADER, nUncompressedSize), 8, "nUncompressedSize", VT_UINT64, DRF_UNKNOWN, ENDIAN_LITTLE));
//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XLZMA::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_LZMA_ALONE_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_LZMA_ALONE_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(LZMA_ALONE_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_LZMA_ALONE_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(LZMA_ALONE_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_LZMA_ALONE_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_LZMA_ALONE_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XLZMA::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_LZMA_ALONE_HEADER) {
        listResult.append({"nProperties", (qint32)offsetof(LZMA_ALONE_HEADER, nProperties), 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"nDictionarySize", (qint32)offsetof(LZMA_ALONE_HEADER, nDictionarySize), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"nUncompressedSize", (qint32)offsetof(LZMA_ALONE_HEADER, nUncompressedSize), 8, XFRECORD_FLAG_NONE, VT_UINT64});
    }

    return listResult;
}

QList<XBinary::FPART> XLZMA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;
    qint64 nFileSize = getSize();

    if ((nFileParts & FILEPART_HEADER) && (nFileSize > 0)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>((qint64)sizeof(LZMA_ALONE_HEADER), nFileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    if ((nFileParts & FILEPART_STREAM) && (nFileSize > (qint64)sizeof(LZMA_ALONE_HEADER))) {
        FPART stream = {};
        stream.filePart = FILEPART_STREAM;
        stream.nFileOffset = sizeof(LZMA_ALONE_HEADER);
        stream.nFileSize = nFileSize - sizeof(LZMA_ALONE_HEADER);
        stream.nVirtualAddress = -1;
        stream.sName = tr("Stream");
        listResult.append(stream);
    }

    return listResult;
}

QList<QString> XLZMA::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("5D000000");

    return listResult;
}

XBinary *XLZMA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLZMA(pDevice);
}
