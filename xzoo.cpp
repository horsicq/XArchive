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

static XBinary::XCONVERT _TABLE_XZOO_STRUCTID[] = {{XZOO::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")}, {XZOO::STRUCTID_HEADER, "HEADER", QString("HEADER")}};

static const qint64 N_ZOO_MAGIC_OFFSET = 20;
static const qint64 N_ZOO_ENTRY_FIXED_SIZE = 51;  // fixed part up to and including the 13-byte short name

static quint16 zooReadLe16(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 2) > baData.size())) return 0;
    return static_cast<quint16>(static_cast<quint8>(baData.at(nOffset)) | (static_cast<quint16>(static_cast<quint8>(baData.at(nOffset + 1))) << 8));
}

static quint32 zooReadLe32(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || ((nOffset + 4) > baData.size())) return 0;
    return static_cast<quint32>(static_cast<quint8>(baData.at(nOffset)) | (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 1))) << 8) |
                                (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 2))) << 16) |
                                (static_cast<quint32>(static_cast<quint8>(baData.at(nOffset + 3))) << 24));
}

XZOO::XZOO(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XZOO::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XZOO> guardedArchive(this);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    bool bResult = false;

    const qint64 nSize = guardedArchive->getSize();
    if (!guardedArchive) return false;
    if (nSize >= 34) {
        const QByteArray baHeader = guardedArchive->read_array(20, 8);
        if (!guardedArchive) return false;
        if ((baHeader.size() == 8) && (zooReadLe32(baHeader, 0) == ZOO_MAGIC)) {
            qint64 nPosEnt = zooReadLe32(baHeader, 4);
            if ((nPosEnt > 0) && (nPosEnt < nSize)) {
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
    QPointer<XZOO> guardedArchive(this);
    if (!pListRecords) return false;
    const QByteArray baPosition = guardedArchive->read_array(24, 4);
    if (!guardedArchive || (baPosition.size() != 4)) return false;
    qint64 nPos = zooReadLe32(baPosition, 0);
    qint64 nFileSize = guardedArchive->getSize();
    if (!guardedArchive) return false;
    qint32 nGuard = 0;

    while ((nPos > 0) && (nPos + N_ZOO_ENTRY_FIXED_SIZE <= nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (nGuard++ > 100000) {
            break;  // corrupt/looping chain
        }

        const QByteArray baEntry = guardedArchive->read_array(nPos, N_ZOO_ENTRY_FIXED_SIZE);
        if (!guardedArchive || (baEntry.size() != N_ZOO_ENTRY_FIXED_SIZE)) return false;
        if (zooReadLe32(baEntry, 0) != ZOO_MAGIC) {
            break;
        }

        quint8 nType = static_cast<quint8>(baEntry.at(4));
        quint8 nMethod = static_cast<quint8>(baEntry.at(5));
        qint64 nPosNext = zooReadLe32(baEntry, 6);
        qint64 nPosData = zooReadLe32(baEntry, 10);
        quint16 nDosDate = zooReadLe16(baEntry, 14);
        quint16 nDosTime = zooReadLe16(baEntry, 16);
        quint16 nCRC16 = zooReadLe16(baEntry, 18);
        qint64 nSizeOrig = zooReadLe32(baEntry, 20);
        qint64 nSizeNow = zooReadLe32(baEntry, 24);
        quint8 nMajVer = static_cast<quint8>(baEntry.at(28));
        quint8 nDeleted = static_cast<quint8>(baEntry.at(30));

        const QByteArray baShortName = baEntry.mid(38, 13);
        const qint32 nShortEnd = baShortName.indexOf('\0');
        QString sName = QString::fromLatin1(baShortName.constData(), (nShortEnd >= 0) ? nShortEnd : baShortName.size());

        // Version 2+ entries may carry a long-name field in the variable part.
        if (nMajVer >= 2) {
            const QByteArray baVariablePrefix = guardedArchive->read_array(nPos + N_ZOO_ENTRY_FIXED_SIZE, 7);
            if (!guardedArchive) return false;
            if (baVariablePrefix.size() != 7) break;
            quint16 nLVar = zooReadLe16(baVariablePrefix, 0);
            if (nLVar > 0) {
                quint8 nLNamU = static_cast<quint8>(baVariablePrefix.at(5));
                if ((nLNamU > 0) && (nLNamU < 256)) {
                    const QByteArray baLongName = guardedArchive->read_array(nPos + 58, nLNamU);
                    if (!guardedArchive) return false;
                    const qint32 nLongEnd = baLongName.indexOf('\0');
                    QString sLongName = QString::fromLatin1(baLongName.constData(), (nLongEnd >= 0) ? nLongEnd : baLongName.size());
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
            record.nCRC16 = nCRC16;
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
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = (nPosEnt > 0 && nPosEnt <= getSize()) ? nPosEnt : 34;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("ZOO ") + tr("Header");
    result.listRecords.append(recHeader);

    if ((nPosEnt > 0) && (nPosEnt < getSize())) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
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

static bool zooCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XZOO::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nPosEnt = read_uint32(24);
    if ((nPosEnt <= 0) || (nPosEnt > getSize())) {
        nPosEnt = 34;
    }

    if ((nFileParts & FILEPART_HEADER) && zooCanAppend(nLimit, listResult)) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = nPosEnt;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && zooCanAppend(nLimit, listResult) && (nPosEnt < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nPosEnt;
        record.nFileSize = getSize() - nPosEnt;
        record.nVirtualAddress = XADDR_MAX;
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
    QPointer<XZOO> guardedArchive(this);
    if (!pState || m_bUnpackOperationInProgress || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState))) {
        return false;
    }
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;

    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive) return false;
    if (!bValid) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    ZOO_UNPACK_CONTEXT *pContext = new (std::nothrow) ZOO_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    const bool bParsed = guardedArchive->_parseEntries(&(pContext->listRecords), pPdStruct);
    if (!guardedArchive) {
        delete pContext;
        return false;
    }
    if (!bParsed || !isPdStructNotCanceled(pPdStruct)) {
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        return false;
    }

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.count();
    pState->nTotalSize = guardedArchive->getSize();
    if (!guardedArchive) {
        *pState = UNPACK_STATE();
        delete pContext;
        return false;
    }
    pState->nCurrentOffset = 0;
    pState->mapUnpackProperties = mapProperties;

    if (!guardedArchive->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedArchive) return false;
        pState->pContext = nullptr;
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XZOO::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XZOO> guardedArchive(this);

    ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
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
    result.mapProperties.insert(FPART_PROP_RESULTCRC, record.nCRC16);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16);

    if (record.mtDateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, record.mtDateTime);
    }

    return result;
}

bool XZOO::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZOO> guardedArchive(this);

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XZOO::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XZOO> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    ZOO_UNPACK_CONTEXT *pContext = static_cast<ZOO_UNPACK_CONTEXT *>(pState->pContext);
    guardedArchive->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    if (!guardedArchive) return false;

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
    QPointer<XZOO> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XZOO::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XZOO> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
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
