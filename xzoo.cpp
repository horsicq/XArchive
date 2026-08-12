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
#include "xzoo.h"

#include <new>

static XBinary::XCONVERT _TABLE_XZOO_STRUCTID[] = {{XZOO::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XZOO::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

static const qint64 N_ZOO_MAGIC_OFFSET = 20;
static const qint64 N_ZOO_ENTRY_FIXED_SIZE = 51;  // fixed part up to and including the 13-byte short name

XZOO::XZOO(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XZOO::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    bool bResult = false;

    if (getSize() >= 34) {
        if (read_uint32(N_ZOO_MAGIC_OFFSET) == ZOO_MAGIC) {
            qint64 nPosEnt = read_uint32(24);
            if ((nPosEnt > 0) && (nPosEnt < getSize())) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XZOO::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZOO xzoo(pDevice);

    return xzoo.isValid(pPdStruct);
}

XBinary::HANDLE_METHOD XZOO::_methodToHandle(quint8 nMethod)
{
    switch (nMethod) {
        case METHOD_STORE: return HANDLE_METHOD_STORE;
        case METHOD_LZD: return HANDLE_METHOD_ZOO_LZD;
        case METHOD_LZH: return HANDLE_METHOD_ZOO_LZH;
        default: return HANDLE_METHOD_UNKNOWN;
    }
}

bool XZOO::_parseEntries(QList<ZOO_RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    qint64 nPos = read_uint32(24);  // posent
    qint64 nFileSize = getSize();
    qint32 nGuard = 0;

    while ((nPos > 0) && (nPos + N_ZOO_ENTRY_FIXED_SIZE <= nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (nGuard++ > 100000) {
            break;  // corrupt/looping chain
        }

        if (read_uint32(nPos) != ZOO_MAGIC) {
            break;
        }

        quint8 nType = read_uint8(nPos + 4);
        quint8 nMethod = read_uint8(nPos + 5);
        qint64 nPosNext = read_uint32(nPos + 6);
        qint64 nPosData = read_uint32(nPos + 10);
        quint16 nDosDate = read_uint16(nPos + 14);
        quint16 nDosTime = read_uint16(nPos + 16);
        qint64 nSizeOrig = read_uint32(nPos + 20);
        qint64 nSizeNow = read_uint32(nPos + 24);
        quint8 nMajVer = read_uint8(nPos + 28);
        quint8 nDeleted = read_uint8(nPos + 30);

        QString sName = read_ansiString(nPos + 38, 13);

        // Version 2+ entries may carry a long-name field in the variable part.
        if (nMajVer >= 2) {
            quint16 nLVar = read_uint16(nPos + 51);
            if (nLVar > 0) {
                quint8 nLNamU = read_uint8(nPos + 56);
                if ((nLNamU > 0) && (nLNamU < 256)) {
                    QString sLongName = read_ansiString(nPos + 58, nLNamU);
                    if (!sLongName.isEmpty()) {
                        sName = sLongName;
                    }
                }
            }
        }

        if ((nDeleted != 1) && !sName.isEmpty() && (nPosData > 0) && (nPosData + nSizeNow <= nFileSize)) {
            ZOO_RECORD record;
            record.sFileName = sName;
            record.nDataOffset = nPosData;
            record.nCompressedSize = nSizeNow;
            record.nUncompressedSize = nSizeOrig;
            record.nMethod = nMethod;
            record.bIsFolder = false;
            Q_UNUSED(nType)

            // DOS date/time → QDateTime
            qint32 nYear = ((nDosDate >> 9) & 0x7F) + 1980;
            qint32 nMonth = (nDosDate >> 5) & 0x0F;
            qint32 nDay = nDosDate & 0x1F;
            qint32 nHour = (nDosTime >> 11) & 0x1F;
            qint32 nMinute = (nDosTime >> 5) & 0x3F;
            qint32 nSecond = (nDosTime & 0x1F) * 2;

            QDate date(nYear, nMonth, nDay);
            QTime time(nHour, nMinute, nSecond);
            if (date.isValid() && time.isValid()) {
                record.mtDateTime = QDateTime(date, time, Qt::UTC);
            }

            pListRecords->append(record);
        }

        if (nPosNext == 0) {
            break;
        }

        if (nPosNext <= nPos) {
            break;  // must advance forward
        }

        nPos = nPosNext;
    }

    return !pListRecords->isEmpty();
}

XBinary::FT XZOO::getFileType()
{
    return FT_ZOO;
}

XBinary::MODE XZOO::getMode()
{
    return MODE_DATA;
}

QString XZOO::getMIMEString()
{
    return "application/x-zoo";
}

qint32 XZOO::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XZOO::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZOO::getArch()
{
    return QString();
}

QString XZOO::getFileFormatExt()
{
    return "zoo";
}

QString XZOO::getFileFormatExtsString()
{
    return "ZOO (*.zoo)";
}

qint64 XZOO::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

XBinary::OSNAME XZOO::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XZOO::getVersion()
{
    return QString("%1.%2").arg(read_uint8(32)).arg(read_uint8(33));
}

QList<XBinary::MAPMODE> XZOO::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XZOO::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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
    qint64 nPosEnt = read_uint32(24);

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = -1;
    recHeader.nOffset = 0;
    recHeader.nSize = (nPosEnt > 0 && nPosEnt <= getSize()) ? nPosEnt : 34;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("ZOO ") + tr("Header");
    result.listRecords.append(recHeader);

    if ((nPosEnt > 0) && (nPosEnt < getSize())) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = -1;
        recData.nOffset = nPosEnt;
        recData.nSize = getSize() - nPosEnt;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Entries");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XZOO::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XZOO_STRUCTID, sizeof(_TABLE_XZOO_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XZOO::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XZOO_STRUCTID, sizeof(_TABLE_XZOO_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XZOO::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XZOO_STRUCTID, sizeof(_TABLE_XZOO_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XZOO::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = 34;
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XZOO::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_HEADER) {
        listResult.append({"text", 0, 20, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"magic", 20, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"posent", 24, 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append({"posmot", 28, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"majver", 32, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"minver", 33, 1, XFRECORD_FLAG_NONE, VT_UINT8});
    }

    return listResult;
}

QList<XBinary::FPART> XZOO::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    const auto canAppend = [&]() -> bool { return (nLimit == -1) || (listResult.size() < nLimit); };

    qint64 nPosEnt = read_uint32(24);
    if ((nPosEnt <= 0) || (nPosEnt > getSize())) {
        nPosEnt = 34;
    }

    if ((nFileParts & FILEPART_HEADER) && canAppend()) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = nPosEnt;
        record.nVirtualAddress = -1;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && canAppend() && (nPosEnt < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nPosEnt;
        record.nFileSize = getSize() - nPosEnt;
        record.nVirtualAddress = -1;
        record.sName = tr("Entries");
        listResult.append(record);
    }

    return listResult;
}

QList<QString> XZOO::getSearchSignatures()
{
    // Magic 0xFDC4A7DC (LE) at offset 20
    return {"........................................'DCA7C4FD'"};
}

XBinary *XZOO::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XZOO(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XZOO::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XZOO::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) {
        return false;
    }

    finishUnpack(pState, nullptr);

    if (!isPdStructNotCanceled(pPdStruct) || !isValid(pPdStruct)) {
        return false;
    }

    ZOO_UNPACK_CONTEXT *pContext = new (std::nothrow) ZOO_UNPACK_CONTEXT;
    if (!pContext) {
        finishUnpack(pState, nullptr);
        return false;
    }

    if (!_parseEntries(&(pContext->listRecords), pPdStruct) || !isPdStructNotCanceled(pPdStruct)) {
        delete pContext;
        finishUnpack(pState, nullptr);
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nTotalSize = getSize();
    pState->nCurrentOffset = 0;
    pState->mapUnpackProperties = mapProperties;

    return true;
}

XBinary::ARCHIVERECORD XZOO::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    ZOO_UNPACK_CONTEXT *pContext = (ZOO_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= pContext->listRecords.count()) {
        return result;
    }

    const ZOO_RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);

    result.nStreamOffset = record.nDataOffset;
    result.nStreamSize = record.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, record.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, _methodToHandle(record.nMethod));

    if (record.mtDateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, record.mtDateTime);
    }

    return result;
}

bool XZOO::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XZOO::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        ZOO_UNPACK_CONTEXT *pContext = (ZOO_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

bool XZOO::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XZOO::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XZOO::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
