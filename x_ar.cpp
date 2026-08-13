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
#include "x_ar.h"

#include <limits>
#include <new>

namespace {
bool arWriteAll(QIODevice *pDevice, const char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct,
                qint64 *pnWritten = nullptr)
{
    if (pnWritten) *pnWritten = 0;
    if (!pDevice || !pDevice->isWritable() || (nSize < 0) || ((nSize > 0) && !pData) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nWritten = 0;
    while ((nWritten < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nResult = pDevice->write(pData + nWritten, nSize - nWritten);
        if ((nResult <= 0) || (nResult > (nSize - nWritten))) return false;
        nWritten += nResult;
        if (pnWritten) *pnWritten = nWritten;
    }

    return (nWritten == nSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool arCanAppendAt(QIODevice *pDevice, qint64 nOffset)
{
    if (!pDevice || !pDevice->isWritable() || (nOffset < 0)) return false;
    if (pDevice->isSequential()) return true;
    return XBinary::isResizeEnable(pDevice) && (pDevice->pos() == nOffset) && (pDevice->size() == nOffset);
}

bool arRollbackWrite(QIODevice *pDevice, qint64 nStartPosition)
{
    return pDevice && !pDevice->isSequential() && (nStartPosition >= 0) && XBinary::isResizeEnable(pDevice) &&
           XBinary::resize(pDevice, nStartPosition) && pDevice->seek(nStartPosition);
}

bool arParseDecimalField(const char *pData, qint32 nSize, qint64 *pValue)
{
    if (!pData || (nSize <= 0) || !pValue) return false;

    QByteArray baValue(pData, nSize);
    baValue = baValue.trimmed();
    if (baValue.isEmpty()) return false;

    qint64 nValue = 0;
    for (char cValue : baValue) {
        if ((cValue < '0') || (cValue > '9')) return false;
        const qint32 nDigit = cValue - '0';
        if (nValue > (((std::numeric_limits<qint64>::max)() - nDigit) / 10)) return false;
        nValue = (nValue * 10) + nDigit;
    }

    *pValue = nValue;
    return true;
}

bool arGetRecordSize(qint64 nDataSize, qint64 *pRecordSize)
{
    if ((nDataSize < 0) || !pRecordSize ||
        (nDataSize > ((std::numeric_limits<qint64>::max)() - 61))) {
        return false;
    }

    *pRecordSize = 60 + nDataSize + (nDataSize & 1);
    return true;
}

bool arGetBsdNameLength(const char *pData, qint32 nSize, qint64 nRecordDataSize, qint32 *pNameLength)
{
    if (!pData || (nSize <= 0) || !pNameLength || (nRecordDataSize < 0)) return false;
    *pNameLength = 0;

    QByteArray baName(pData, nSize);
    baName = baName.trimmed();
    if (!baName.startsWith("#1/")) return true;

    QByteArray baLength = baName.mid(3);
    if (baLength.endsWith('/')) baLength.chop(1);
    if (baLength.isEmpty()) return false;

    qint64 nNameLength = 0;
    if (!arParseDecimalField(baLength.constData(), baLength.size(), &nNameLength) ||
        (nNameLength <= 0) || (nNameLength > nRecordDataSize) ||
        (nNameLength > (std::numeric_limits<qint32>::max)())) {
        return false;
    }

    *pNameLength = (qint32)nNameLength;
    return true;
}
}  // namespace

XBinary::XCONVERT _TABLE_XAr_STRUCTID[] = {
    {X_Ar::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {X_Ar::STRUCTID_FRECORD, "FRECORD", QString("FRECORD")},
    {X_Ar::STRUCTID_SIGNATURE, "Signature", QObject::tr("Signature")},
};

X_Ar::X_Ar(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool X_Ar::isPackStateConsistent(const PACK_STATE *pState, const AR_PACK_CONTEXT *pContext)
{
    if (!pState || !pContext || (pState->pContext != pContext) || !pState->pDevice ||
        !pState->pDevice->isWritable() || (pContext->nStartOffset < 0) ||
        (pContext->nStartOffset > ((std::numeric_limits<qint64>::max)() - 8)) ||
        (pContext->nCurrentOffset < pContext->nStartOffset) ||
        (pContext->nNumberOfRecords < 0) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) ||
        (pState->nNumberOfRecords != pContext->nNumberOfRecords)) {
        return false;
    }

    if (!pContext->bFailed && (pContext->nCurrentOffset < (pContext->nStartOffset + 8))) return false;
    if (pState->pDevice->isSequential()) return true;

    return arCanAppendAt(pState->pDevice, pContext->nCurrentOffset);
}

bool X_Ar::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<X_Ar> guardedArchive(this);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nTotalSize = guardedArchive->getSize();
    if (!guardedArchive || (nTotalSize < 8)) return false;

    _MEMORY_MAP memoryMap = guardedArchive->XBinary::getSimpleMemoryMap();
    if (!guardedArchive) return false;
    const bool bHasSignature = guardedArchive->compareSignature(
        &memoryMap, "'!<arch>'0a", 0, pPdStruct);
    if (!guardedArchive || !bHasSignature) return false;

    qint64 nOffset = 8;
    while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nTotalSize - nOffset) < (qint64)sizeof(FRECORD)) return false;

        const FRECORD frecord = guardedArchive->readFRECORD(nOffset);
        if (!guardedArchive) return false;
        if ((frecord.endChar[0] != 0x60) || (frecord.endChar[1] != 0x0a)) return false;

        qint64 nDataSize = 0;
        qint64 nRecordSize = 0;
        qint32 nBsdNameLength = 0;
        if (!arParseDecimalField(frecord.fileSize, sizeof(frecord.fileSize), &nDataSize) ||
            !arGetRecordSize(nDataSize, &nRecordSize) ||
            !arGetBsdNameLength(frecord.fileId, sizeof(frecord.fileId), nDataSize, &nBsdNameLength) ||
            (nRecordSize > (nTotalSize - nOffset))) {
            return false;
        }

        nOffset += nRecordSize;
    }

    return (nOffset == nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool X_Ar::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    X_Ar x_ar(pDevice);

    return x_ar.isValid(pPdStruct);
}

QString X_Ar::getFileFormatExt()
{
    return "ar";
}

QList<XBinary::MAPMODE> X_Ar::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::FT X_Ar::getFileType()
{
    return XBinary::FT_AR;
}

qint32 X_Ar::getType()
{
    return TYPE_PACKAGE;
}

QString X_Ar::getMIMEString()
{
    return "application/x-archive";
}

QString X_Ar::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XAr_STRUCTID, sizeof(_TABLE_XAr_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString X_Ar::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XAr_STRUCTID, sizeof(_TABLE_XAr_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 X_Ar::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XAr_STRUCTID, sizeof(_TABLE_XAr_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// bool X_Ar::readTableInit(const DATA_RECORDS_OPTIONS &dataRecordsOptions, void **ppUserData, PDSTRUCT *pPdStruct)
// {
//     if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_FRECORD) {
//         qint64 nStartOffset = locationToOffset(dataRecordsOptions.pMemoryMap, dataRecordsOptions.dataHeaderFirst.locType,
//         dataRecordsOptions.dataHeaderFirst.nLocation);

//         if (nStartOffset != -1) {
//             TABLE_LIST *pTableList = new TABLE_LIST;
//             qint64 nOffset = nStartOffset;
//             qint64 nTotalSize = getSize();

//             while ((nOffset + (qint64)sizeof(FRECORD) <= nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
//                 FRECORD frecord = readFRECORD(nOffset);

//                 // Validate member terminator
//                 if (!((frecord.endChar[0] == 0x60) && (frecord.endChar[1] == 0x0a))) {
//                     break;
//                 }

//                 QString sSize = QString(frecord.fileSize);
//                 sSize.resize(sizeof(frecord.fileSize));
//                 qint64 nRecordSize = sSize.trimmed().toLongLong();
//                 if (nRecordSize < 0) {
//                     break;
//                 }

//                 QString sName = QString(frecord.fileId);
//                 sName.resize(sizeof(frecord.fileId));
//                 sName = sName.trimmed();

//                 qint64 nDataSize = nRecordSize;

//                 // BSD style filename stored in data area immediately after header
//                 if (sName.section('/', 0, 0) == "#1") {
//                     qint32 nFileNameLength = sName.section('/', 1, 1).toInt();
//                     if (nFileNameLength > 0) {
//                         if (nRecordSize >= nFileNameLength) {
//                             nDataSize = nRecordSize - (qint64)nFileNameLength;
//                         } else {
//                             // Malformed entry; avoid underflow
//                             nDataSize = 0;
//                         }
//                     }
//                 }

//                 if (nDataSize < 0) {
//                     nDataSize = 0;
//                 }

//                 XBinary::OFFSETSIZE os = {};
//                 os.nOffset = nOffset;
//                 os.nSize = nDataSize;
//                 pTableList->listOffsetsSizes.append(os);

//                 qint64 nStep = (qint64)sizeof(FRECORD) + (qint64)S_ALIGN_UP(nRecordSize, 2);
//                 nOffset += nStep;
//             }

//             *ppUserData = (void *)pTableList;
//         }
//     }

//     return true;
// }

// QList<XBinary::DATA_HEADER> X_Ar::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;
//         _dataHeadersOptions.nID = STRUCTID_SIGNATURE;
//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_SIGNATURE) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(dataHeadersOptions.nID));
//                 dataHeader.listRecords.append(getDataRecord(0, 8, "Magic", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.nSize = 8;
//                 listResult.append(dataHeader);

//                 DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                 _dataHeadersOptions.nLocation += 8;
//                 _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                 _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//                 _dataHeadersOptions.nID = STRUCTID_FRECORD;
//                 _dataHeadersOptions.nCount = _getNumberOfStreams(8, pPdStruct);
//                 listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//             } else if (dataHeadersOptions.nID == STRUCTID_FRECORD) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(dataHeadersOptions.nID));

//                 dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileId), sizeof(((FRECORD *)0)->fileId), "FileId", VT_CHAR_ARRAY, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileMod), sizeof(((FRECORD *)0)->fileMod), "FileMod", VT_CHAR_ARRAY, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, ownerId), sizeof(((FRECORD *)0)->ownerId), "OwnerId", VT_CHAR_ARRAY, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, groupId), sizeof(((FRECORD *)0)->groupId), "GroupId", VT_CHAR_ARRAY, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileMode), sizeof(((FRECORD *)0)->fileMode), "FileMode", VT_CHAR_ARRAY, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, fileSize), sizeof(((FRECORD *)0)->fileSize), "FileSize", VT_CHAR_ARRAY, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(FRECORD, endChar), sizeof(((FRECORD *)0)->endChar), "EndChar", VT_BYTE_ARRAY, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.nSize = sizeof(FRECORD);

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> X_Ar::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_SIGNATURE;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_SIGNATURE) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, 8)) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_SIGNATURE);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = 8;
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_SIGNATURE, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_SIGNATURE), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> X_Ar::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_SIGNATURE) {
        listResult.append({"Magic", 0, 8, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
    } else if (nStructID == STRUCTID_FRECORD) {
        listResult.append({"FileId", (qint32)offsetof(FRECORD, fileId), (qint32)sizeof(((FRECORD *)0)->fileId), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"FileMod", (qint32)offsetof(FRECORD, fileMod), (qint32)sizeof(((FRECORD *)0)->fileMod), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"OwnerId", (qint32)offsetof(FRECORD, ownerId), (qint32)sizeof(((FRECORD *)0)->ownerId), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"GroupId", (qint32)offsetof(FRECORD, groupId), (qint32)sizeof(((FRECORD *)0)->groupId), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"FileMode", (qint32)offsetof(FRECORD, fileMode), (qint32)sizeof(((FRECORD *)0)->fileMode), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"FileSize", (qint32)offsetof(FRECORD, fileSize), (qint32)sizeof(((FRECORD *)0)->fileSize), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"EndChar", (qint32)offsetof(FRECORD, endChar), (qint32)sizeof(((FRECORD *)0)->endChar), XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
    }

    return listResult;
}

// qint32 X_Ar::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords,
//                           void *pUserData, PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(pUserData)
//     qint32 nResult = 0;

//     if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_FRECORD) {
//         TABLE_LIST *pTableList = (TABLE_LIST *)pUserData;

//         if (pTableList) {
//             if (nRow < pTableList->listOffsetsSizes.count()) {
//                 OFFSETSIZE os = pTableList->listOffsetsSizes.at(nRow);

//                 XBinary::readTableRow(nRow, LT_OFFSET, os.nOffset, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
//                 nResult = os.nSize;
//             }
//         }
//     } else {
//         nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
//     }

//     return nResult;
// }

// void X_Ar::readTableFinalize(const DATA_RECORDS_OPTIONS &dataRecordsOptions, void *pUserData, PDSTRUCT *pPdStruct)
// {
//     if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_FRECORD) {
//         TABLE_LIST *pTableList = (TABLE_LIST *)pUserData;

//         if (pTableList) {
//             delete pTableList;
//         }
//     }
// }

quint64 X_Ar::_getNumberOfStreams(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;
    const qint64 nTotalSize = getSize();
    if ((nOffset < 0) || (nOffset > nTotalSize)) return 0;

    while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nTotalSize - nOffset) < (qint64)sizeof(FRECORD)) break;

        const FRECORD frecord = readFRECORD(nOffset);
        qint64 nDataSize = 0;
        qint64 nRecordSize = 0;
        qint32 nBsdNameLength = 0;
        if ((frecord.endChar[0] != 0x60) || (frecord.endChar[1] != 0x0a) ||
            !arParseDecimalField(frecord.fileSize, sizeof(frecord.fileSize), &nDataSize) ||
            !arGetRecordSize(nDataSize, &nRecordSize) ||
            !arGetBsdNameLength(frecord.fileId, sizeof(frecord.fileId), nDataSize, &nBsdNameLength) ||
            (nRecordSize > (nTotalSize - nOffset))) {
            break;
        }

        nOffset += nRecordSize;
        nResult++;
    }

    return nResult;
}

XBinary::_MEMORY_MAP X_Ar::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;  // Default mode
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

X_Ar::FRECORD X_Ar::readFRECORD(qint64 nOffset)
{
    FRECORD record = {};

    // Read the fixed header in one callback boundary.  Besides being cheaper,
    // this prevents a hostile QIODevice from deleting the archive between
    // field reads and leaving this method to dereference a dead owner.
    (void)read_array(nOffset, reinterpret_cast<char *>(&record),
                     sizeof(record));

    return record;
}

static bool arCanAppendPart(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> X_Ar::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !isValid(pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const qint64 fileSize = getSize();
    qint64 nOffset = 8;

    // Header magic
    if ((nFileParts & FILEPART_HEADER) && arCanAppendPart(nLimit, listResult)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = 8;
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
    }

    qint32 nIndex = 0;
    while ((nOffset < fileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const FRECORD frecord = readFRECORD(nOffset);
        qint64 nDataFieldSize = 0;
        qint64 nRecordSize = 0;
        qint32 nFileNameLength = 0;
        if (!arParseDecimalField(frecord.fileSize, sizeof(frecord.fileSize), &nDataFieldSize) ||
            !arGetRecordSize(nDataFieldSize, &nRecordSize) ||
            !arGetBsdNameLength(frecord.fileId, sizeof(frecord.fileId), nDataFieldSize, &nFileNameLength) ||
            (nRecordSize > (fileSize - nOffset))) {
            return QList<FPART>();
        }

        qint64 dataOffset = nOffset + (qint64)sizeof(FRECORD) + nFileNameLength;
        qint64 dataSize = nDataFieldSize - nFileNameLength;
        QByteArray baOriginalName(frecord.fileId, sizeof(frecord.fileId));
        while (baOriginalName.endsWith(' ')) baOriginalName.chop(1);
        QString sOriginalName;
        if (nFileNameLength > 0) {
            const QByteArray baEmbeddedName = read_array(nOffset + sizeof(FRECORD), nFileNameLength);
            if (baEmbeddedName.size() != nFileNameLength) return QList<FPART>();
            sOriginalName = QString::fromUtf8(baEmbeddedName);
        } else {
            if ((baOriginalName.size() > 1) && baOriginalName.endsWith('/')) baOriginalName.chop(1);
            sOriginalName = QString::fromUtf8(baOriginalName);
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return QList<FPART>();

        if ((nFileParts & FILEPART_HEADER) && arCanAppendPart(nLimit, listResult)) {
            FPART h = {};
            h.filePart = FILEPART_HEADER;
            h.nFileOffset = nOffset;
            h.nFileSize = (qint64)sizeof(FRECORD) + (qint64)nFileNameLength;  // FRECORD + optional BSD filename
            h.nVirtualAddress = XADDR_MAX;
            h.sName = tr("Header");
            listResult.append(h);
        }

        if ((nFileParts & FILEPART_STREAM) && arCanAppendPart(nLimit, listResult)) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = dataOffset;
            part.nFileSize = dataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Record") + QString(" %1").arg(nIndex);
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, sOriginalName);
            // Properties: ar stores raw bytes, no compression
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, dataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, dataSize);
            // Optional: modification time (ASCII epoch seconds in header)
            QString sMod = QString::fromLatin1(frecord.fileMod, sizeof(frecord.fileMod)).trimmed();
            bool bOk = false;
            quint64 nModSecs = sMod.toULongLong(&bOk, 10);
            if (bOk) {
                QDateTime dt = XBinary::valueToTime((qint64)nModSecs, XBinary::DT_TYPE_UNIXTIME);
                part.mapProperties.insert(FPART_PROP_DATETIME, dt);
            }
            listResult.append(part);
        }

        nOffset += nRecordSize;
        nIndex++;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return QList<FPART>();

    // Total parsed data area
    if ((nFileParts & FILEPART_DATA) && arCanAppendPart(nLimit, listResult)) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 8;
        data.nFileSize = nOffset - 8;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    return XBinary::isPdStructNotCanceled(pPdStruct) ? listResult : QList<FPART>();
}

bool X_Ar::initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pDevice || !pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nStartPosition = pDevice->pos();
    if (!arCanAppendAt(pDevice, nStartPosition) ||
        (nStartPosition > ((std::numeric_limits<qint64>::max)() - 8))) {
        return false;
    }

    AR_PACK_CONTEXT *pNewContext = new (std::nothrow) AR_PACK_CONTEXT();
    if (!pNewContext) return false;

    AR_PACK_CONTEXT *pOldContext = static_cast<AR_PACK_CONTEXT *>(pState->pContext);
    if (pOldContext) {
        QIODevice *pOldDevice = pState->pDevice;
        if (pOldDevice && !pOldDevice->isSequential()) {
            if (!isPackStateConsistent(pState, pOldContext) ||
                !arRollbackWrite(pOldDevice, pOldContext->nStartOffset)) {
                delete pNewContext;
                return false;
            }
        } else if (pOldDevice == pDevice) {
            delete pNewContext;
            return false;
        }

        delete pOldContext;
        *pState = PACK_STATE();
    }

    nStartPosition = pDevice->pos();
    if (!arCanAppendAt(pDevice, nStartPosition) ||
        (nStartPosition > ((std::numeric_limits<qint64>::max)() - 8))) {
        delete pNewContext;
        *pState = PACK_STATE();
        return false;
    }

    pNewContext->nStartOffset = nStartPosition;
    pNewContext->nCurrentOffset = nStartPosition;
    pNewContext->nNumberOfRecords = 0;
    pNewContext->bFailed = false;

    pState->pDevice = pDevice;
    pState->mapProperties = mapProperties;
    pState->nCurrentOffset = nStartPosition;
    pState->nNumberOfRecords = 0;
    pState->pContext = pNewContext;

    // Write AR signature: "!<arch>\n"
    const QByteArray baSignature = QByteArrayLiteral("!<arch>\n");
    qint64 nSignatureWritten = 0;
    if (!arWriteAll(pDevice, baSignature.constData(), baSignature.size(), pPdStruct, &nSignatureWritten)) {
        if (arRollbackWrite(pDevice, nStartPosition) || (nSignatureWritten == 0)) {
            delete pNewContext;
            *pState = PACK_STATE();
        } else {
            pNewContext->bFailed = true;
            pNewContext->nCurrentOffset = nStartPosition + nSignatureWritten;
            pState->nCurrentOffset = pNewContext->nCurrentOffset;
        }
        return false;
    }

    // Initialize state
    pNewContext->nCurrentOffset = nStartPosition + baSignature.size();
    pState->nCurrentOffset = pNewContext->nCurrentOffset;

    return true;
}

bool X_Ar::failAddFileWrite(PACK_STATE *pState, AR_PACK_CONTEXT *pContext, qint64 nStartPosition, qint64 nRecordWritten)
{
    if (arRollbackWrite(pState->pDevice, nStartPosition)) return false;

    if ((nRecordWritten > 0) || !pState->pDevice->isSequential()) {
        pContext->bFailed = true;
        if (nRecordWritten <= ((std::numeric_limits<qint64>::max)() - nStartPosition)) {
            pContext->nCurrentOffset = nStartPosition + nRecordWritten;
            pState->nCurrentOffset = pContext->nCurrentOffset;
        }
    }
    return false;
}

bool X_Ar::addFile(PACK_STATE *pState, const QString &sFilePath, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pState->pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    AR_PACK_CONTEXT *pContext = static_cast<AR_PACK_CONTEXT *>(pState->pContext);
    if (pContext->bFailed || !isPackStateConsistent(pState, pContext) ||
        (pContext->nNumberOfRecords == (std::numeric_limits<qint32>::max)())) {
        return false;
    }

    const qint64 nStartPosition = pContext->nCurrentOffset;
    qint64 nRecordWritten = 0;

    // Check if file exists and is readable
    QFileInfo fileInfo(sFilePath);

    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        return false;
    }

    // AR format traditionally stores only basenames (no directory paths)
    QString sBaseName = fileInfo.fileName();
    qint64 nFileSize = fileInfo.size();
    quint32 nMode = 0;
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    qint64 nMTime = fileInfo.lastModified().toSecsSinceEpoch();
#else
    qint64 nMTime = fileInfo.lastModified().toMSecsSinceEpoch() / 1000;
#endif

#ifdef Q_OS_WIN
    nMode = 0644;  // owner read/write, group/others read
#else
    QFile::Permissions permissions = fileInfo.permissions();

    if (permissions & QFile::ReadOwner) nMode |= 0400;
    if (permissions & QFile::WriteOwner) nMode |= 0200;
    if (permissions & QFile::ExeOwner) nMode |= 0100;
    if (permissions & QFile::ReadGroup) nMode |= 0040;
    if (permissions & QFile::WriteGroup) nMode |= 0020;
    if (permissions & QFile::ExeGroup) nMode |= 0010;
    if (permissions & QFile::ReadOther) nMode |= 0004;
    if (permissions & QFile::WriteOther) nMode |= 0002;
    if (permissions & QFile::ExeOther) nMode |= 0001;
#endif

    // Check if we need BSD-style long filename format
    QByteArray baFileName = sBaseName.toUtf8();
    bool bUseBsdFormat = (baFileName.size() > 15);

    if (baFileName.isEmpty() || (nFileSize < 0) ||
        (bUseBsdFormat && ((qint64)baFileName.size() > ((std::numeric_limits<qint64>::max)() - nFileSize)))) {
        return false;
    }

    const qint64 nTotalDataSize = nFileSize + (bUseBsdFormat ? baFileName.size() : 0);
    qint64 nRecordSize = 0;
    if ((QString::number(nTotalDataSize).size() > (qint32)sizeof(((FRECORD *)0)->fileSize)) ||
        (QString::number(nMTime).size() > (qint32)sizeof(((FRECORD *)0)->fileMod)) ||
        (QString::number(nMode, 8).size() > (qint32)sizeof(((FRECORD *)0)->fileMode)) ||
        !arGetRecordSize(nTotalDataSize, &nRecordSize) ||
        (nStartPosition > ((std::numeric_limits<qint64>::max)() - nRecordSize))) {
        return false;
    }

    QFile file(sFilePath);
    if (!file.open(QIODevice::ReadOnly)) return false;

    FRECORD header = createHeader(sBaseName, nFileSize, nMode, nMTime);

    // Write header
    qint64 nWritten = 0;
    if (!arWriteAll(pState->pDevice, reinterpret_cast<const char *>(&header), sizeof(FRECORD), pPdStruct, &nWritten)) {
        nRecordWritten += nWritten;
        file.close();
        return failAddFileWrite(pState, pContext, nStartPosition, nRecordWritten);
    }
    nRecordWritten += nWritten;

    // If using BSD format, write the filename first
    if (bUseBsdFormat) {
        nWritten = 0;
        if (!arWriteAll(pState->pDevice, baFileName.constData(), baFileName.size(), pPdStruct, &nWritten)) {
            nRecordWritten += nWritten;
            file.close();
            return failAddFileWrite(pState, pContext, nStartPosition, nRecordWritten);
        }
        nRecordWritten += nWritten;
    }

    // Write file content
    qint64 nBytesWritten = 0;

    while (nBytesWritten < nFileSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
        QByteArray baBuffer = file.read(qMin((qint64)0x10000, nFileSize - nBytesWritten));

        if (baBuffer.isEmpty()) {
            file.close();
            return failAddFileWrite(pState, pContext, nStartPosition, nRecordWritten);
        }

        nWritten = 0;
        if (!arWriteAll(pState->pDevice, baBuffer.constData(), baBuffer.size(), pPdStruct, &nWritten)) {
            nRecordWritten += nWritten;
            file.close();
            return failAddFileWrite(pState, pContext, nStartPosition, nRecordWritten);
        }

        nRecordWritten += nWritten;
        nBytesWritten += baBuffer.size();
    }

    file.close();

    if ((nBytesWritten != nFileSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return failAddFileWrite(pState, pContext, nStartPosition, nRecordWritten);
    }

    // Add padding if total size (filename + file content for BSD, or just file content for standard) is odd
    if (nTotalDataSize % 2 != 0) {
        char cPadding = '\n';
        nWritten = 0;
        if (!arWriteAll(pState->pDevice, &cPadding, 1, pPdStruct, &nWritten)) {
            nRecordWritten += nWritten;
            return failAddFileWrite(pState, pContext, nStartPosition, nRecordWritten);
        }
        nRecordWritten += nWritten;
    }

    // Update state
    if ((nRecordWritten != nRecordSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) return failAddFileWrite(pState, pContext, nStartPosition, nRecordWritten);

    pContext->nCurrentOffset = nStartPosition + nRecordSize;
    pContext->nNumberOfRecords++;
    pState->nCurrentOffset = pContext->nCurrentOffset;
    pState->nNumberOfRecords = pContext->nNumberOfRecords;

    return true;
}

bool X_Ar::addFolder(PACK_STATE *pState, const QString &sDirectoryPath, PDSTRUCT *pPdStruct)
{
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;

    if (!pState || !pState->pContext || !pState->pDevice || !pState->pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    AR_PACK_CONTEXT *pContext = static_cast<AR_PACK_CONTEXT *>(pState->pContext);
    if (pContext->bFailed || !isPackStateConsistent(pState, pContext)) return false;

    // Check if directory exists
    if (!XBinary::isDirectoryExists(sDirectoryPath)) {
        return false;
    }

    // Enumerate all files in directory
    QList<QString> listFiles;
    XBinary::findFiles(sDirectoryPath, &listFiles, true, 0, pPdStruct);

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    qint32 nNumberOfFiles = listFiles.count();

    // Add each file
    for (qint32 i = 0; (i < nNumberOfFiles) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        QString sFilePath = listFiles.at(i);
        QFileInfo fileInfo(sFilePath);

        // Skip directories (AR stores files only)
        if (fileInfo.isDir()) {
            continue;
        }

        // Add file to archive
        if (!addFile(pState, sFilePath, pPdStruct)) {
            return false;
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool X_Ar::finishPack(PACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pState->pDevice->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    AR_PACK_CONTEXT *pContext = static_cast<AR_PACK_CONTEXT *>(pState->pContext);
    if (pContext->bFailed || !isPackStateConsistent(pState, pContext)) return false;

    // AR archives don't require any terminator
    delete pContext;
    pState->pContext = nullptr;

    return true;
}

X_Ar::FRECORD X_Ar::createHeader(const QString &sFileName, qint64 nFileSize, quint32 nMode, qint64 nMTime)
{
    FRECORD header;

    // Zero out the entire header
    memset(&header, 0x20, sizeof(FRECORD));  // AR uses spaces for padding

    QByteArray baName = sFileName.toUtf8();

    // Check if filename fits in standard 16-byte field (minus 1 for trailing '/')
    // If not, we'll use BSD-style format with #1/<length>
    if (baName.size() <= 15) {
        // Standard format: filename with trailing '/'
        qint32 nNameLen = baName.size();
        if (nNameLen > 0) {
            memcpy(header.fileId, baName.constData(), nNameLen);
        }
        // Add trailing '/' as per AR format
        if (nNameLen < (qint32)sizeof(header.fileId)) {
            header.fileId[nNameLen] = '/';
        }
    } else {
        // BSD-style format: #1/<length>/
        // The actual filename will be stored at the beginning of the file data
        QString sBsdFormat = QString("#1/%1").arg(baName.size());
        QByteArray baBsdFormat = sBsdFormat.toLatin1();
        qint32 nBsdLen = qMin(baBsdFormat.size(), (qint32)sizeof(header.fileId));
        if (nBsdLen > 0) {
            memcpy(header.fileId, baBsdFormat.constData(), nBsdLen);
        }
        // Add trailing '/' if there's room
        if (nBsdLen < (qint32)sizeof(header.fileId)) {
            header.fileId[nBsdLen] = '/';
        }
        // Adjust file size to include the embedded filename
        nFileSize += baName.size();
    }

    // Write mtime as decimal string
    QString sMTime = QString::number(nMTime);
    QByteArray baMTime = sMTime.toLatin1();
    qint32 nMTimeLen = qMin(baMTime.size(), (qint32)sizeof(header.fileMod));
    if (nMTimeLen > 0) {
        memcpy(header.fileMod, baMTime.constData(), nMTimeLen);
    }

    // OwnerID and GroupID (default to 0)
    QByteArray baOwnerId = "0";
    memcpy(header.ownerId, baOwnerId.constData(), baOwnerId.size());

    QByteArray baGroupId = "0";
    memcpy(header.groupId, baGroupId.constData(), baGroupId.size());

    // File mode as octal string
    QString sMode = QString::number(nMode, 8);
    QByteArray baMode = sMode.toLatin1();
    qint32 nModeLen = qMin(baMode.size(), (qint32)sizeof(header.fileMode));
    if (nModeLen > 0) {
        memcpy(header.fileMode, baMode.constData(), nModeLen);
    }

    // File size as decimal string
    QString sSize = QString::number(nFileSize);
    QByteArray baSize = sSize.toLatin1();
    qint32 nSizeLen = qMin(baSize.size(), (qint32)sizeof(header.fileSize));
    if (nSizeLen > 0) {
        memcpy(header.fileSize, baSize.constData(), nSizeLen);
    }

    // End characters
    header.endChar[0] = 0x60;
    header.endChar[1] = 0x0a;

    return header;
}

QMap<XBinary::UNPACK_PROP, QVariant> X_Ar::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool X_Ar::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<X_Ar> guardedArchive(this);

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;
    if (!pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pState)) return false;
    guardedArchive->releaseUnpackSource(pState);
    *pState = UNPACK_STATE();
    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;
    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive) {
        *pState = UNPACK_STATE();
        return false;
    }
    if (!bValid) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 8;
    pState->nTotalSize = guardedArchive->getSize();
    if (!guardedArchive) {
        *pState = UNPACK_STATE();
        return false;
    }
    pState->nCurrentIndex = 0;
    pState->pContext = nullptr;

    qint64 nOffset = 8;
    while ((nOffset < pState->nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((pState->nTotalSize - nOffset) < (qint64)sizeof(FRECORD)) {
            guardedArchive->releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
            return false;
        }

        const FRECORD header = guardedArchive->readFRECORD(nOffset);
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
        qint64 nFileSize = 0;
        qint64 nRecordSize = 0;
        qint32 nBsdNameLength = 0;
        if ((header.endChar[0] != 0x60) || (header.endChar[1] != 0x0a) ||
            !arParseDecimalField(header.fileSize, sizeof(header.fileSize), &nFileSize) ||
            !arGetRecordSize(nFileSize, &nRecordSize) ||
            !arGetBsdNameLength(header.fileId, sizeof(header.fileId), nFileSize, &nBsdNameLength) ||
            (nRecordSize > (pState->nTotalSize - nOffset)) ||
            (pState->nNumberOfRecords == (std::numeric_limits<qint32>::max)())) {
            guardedArchive->releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
            return false;
        }
        pState->nNumberOfRecords++;
        nOffset += nRecordSize;
    }

    if ((nOffset != pState->nTotalSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const bool bFinalized = guardedArchive->validateAndFinalizeUnpackSource(
        pState, pPdStruct);
    if (!guardedArchive) {
        *pState = UNPACK_STATE();
        return false;
    }
    if (!bFinalized) {
        guardedArchive->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD X_Ar::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<X_Ar> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (pState && guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
        (pState->nCurrentIndex >= 0) && (pState->nCurrentIndex < pState->nNumberOfRecords) &&
        (pState->nCurrentOffset >= 8)) {
        const qint64 nCurrentSize = guardedArchive->getSize();
        if (!guardedArchive || (pState->nTotalSize != nCurrentSize) ||
            (pState->nCurrentOffset > (pState->nTotalSize - (qint64)sizeof(FRECORD)))) {
            return result;
        }
        const FRECORD header = guardedArchive->readFRECORD(pState->nCurrentOffset);
        if (!guardedArchive) return XBinary::ARCHIVERECORD();
        qint64 nFileSize = 0;
        qint64 nRecordSize = 0;
        qint32 nFileNameLength = 0;
        if ((header.endChar[0] != 0x60) || (header.endChar[1] != 0x0a) ||
            !arParseDecimalField(header.fileSize, sizeof(header.fileSize), &nFileSize) ||
            !arGetRecordSize(nFileSize, &nRecordSize) ||
            !arGetBsdNameLength(header.fileId, sizeof(header.fileId), nFileSize, &nFileNameLength) ||
            (nRecordSize > (pState->nTotalSize - pState->nCurrentOffset))) {
            return result;
        }

        // Extract file name
        QByteArray baFileName(header.fileId, sizeof(header.fileId));
        while (baFileName.endsWith(' ')) baFileName.chop(1);
        QString sFileName;

        // Handle BSD-style long names
        if (nFileNameLength > 0) {
            const QByteArray baEmbeddedName = guardedArchive->read_array(
                pState->nCurrentOffset + sizeof(FRECORD), nFileNameLength);
            if (!guardedArchive) return XBinary::ARCHIVERECORD();
            if (baEmbeddedName.size() != nFileNameLength) return XBinary::ARCHIVERECORD();
            sFileName = QString::fromUtf8(baEmbeddedName);
            result.nStreamOffset = pState->nCurrentOffset + sizeof(FRECORD) + nFileNameLength;
            result.nStreamSize = nFileSize - nFileNameLength;
        } else {
            // Remove trailing '/' if present
            if ((baFileName.size() > 1) && baFileName.endsWith('/')) baFileName.chop(1);
            sFileName = QString::fromUtf8(baFileName);

            result.nStreamOffset = pState->nCurrentOffset + sizeof(FRECORD);
            result.nStreamSize = nFileSize;
        }

        // result.nDecompressedOffset = 0;
        // result.nDecompressedSize = result.nStreamSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE);

        // Parse file mode (octal)
        QString sMode = QString::fromLatin1(header.fileMode, sizeof(header.fileMode)).trimmed();
        quint32 nMode = sMode.toUInt(nullptr, 8);
        result.mapProperties.insert(XBinary::FPART_PROP_FILEMODE, nMode);

        // Parse UID (decimal)
        QString sUid = QString::fromLatin1(header.ownerId, sizeof(header.ownerId)).trimmed();
        quint32 nUid = sUid.toUInt();
        result.mapProperties.insert(XBinary::FPART_PROP_UID, nUid);

        // Parse GID (decimal)
        QString sGid = QString::fromLatin1(header.groupId, sizeof(header.groupId)).trimmed();
        quint32 nGid = sGid.toUInt();
        result.mapProperties.insert(XBinary::FPART_PROP_GID, nGid);

        // Size
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, result.nStreamSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);

        // Parse mtime (decimal)
        QString sMTime = QString::fromLatin1(header.fileMod, sizeof(header.fileMod)).trimmed();
        qint64 nMTime = sMTime.toLongLong();
        QDateTime dateTime = XBinary::valueToTime((qint64)nMTime, XBinary::DT_TYPE_UNIXTIME);
        result.mapProperties.insert(XBinary::FPART_PROP_DATETIME, dateTime);
    }

    return result;
}

bool X_Ar::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<X_Ar> guardedArchive(this);

    if (!pState || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pState->nCurrentOffset < 8)) {
        return false;
    }

    const qint64 nCurrentSize = guardedArchive->getSize();
    if (!guardedArchive || (pState->nTotalSize != nCurrentSize) ||
        (pState->nCurrentOffset > (pState->nTotalSize - (qint64)sizeof(FRECORD)))) return false;

    const FRECORD header = guardedArchive->readFRECORD(pState->nCurrentOffset);
    if (!guardedArchive) return false;
    qint64 nFileSize = 0;
    qint64 nRecordSize = 0;
    if ((header.endChar[0] != 0x60) || (header.endChar[1] != 0x0a) ||
        !arParseDecimalField(header.fileSize, sizeof(header.fileSize), &nFileSize) ||
        !arGetRecordSize(nFileSize, &nRecordSize) ||
        (nRecordSize > (pState->nTotalSize - pState->nCurrentOffset))) {
        return false;
    }

    pState->nCurrentOffset += nRecordSize;
    pState->nCurrentIndex++;

    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool X_Ar::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) return false;
    // AR enumeration has no heap context, but it still owns all public cursor
    // and property state.  XArchive::getRecords() treats cleanup failure as an
    // incomplete enumeration, so the inherited false-returning stub used to
    // discard every otherwise valid AR/DEB record list.
    releaseUnpackSource(pState);
    *pState = UNPACK_STATE();
    return true;
}

QList<QString> X_Ar::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'!<arch>\n'");

    return listResult;
}

XBinary *X_Ar::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new X_Ar(pDevice);
}

bool X_Ar::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<X_Ar> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *X_Ar::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<X_Ar> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void X_Ar::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
