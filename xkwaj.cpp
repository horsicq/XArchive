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
#include "xkwaj.h"

#include <new>

static XBinary::XCONVERT _TABLE_XKWAJ_STRUCTID[] = {{XKWAJ::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                    {XKWAJ::STRUCTID_KWAJ_HEADER, "KWAJ_HEADER", QString("KWAJ_HEADER")}};

XKWAJ::XKWAJ(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XKWAJ::failUnpackInitialization(XKWAJ *pArchive, UNPACK_STATE *pState)
{
    if (pArchive) pArchive->releaseUnpackSource(pState);
    if (pState) *pState = UNPACK_STATE();
    return false;
}

bool XKWAJ::hasExtensionBytes(qint64 nExtensionOffset, qint64 nDataOffset, qint64 nSize)
{
    return (nSize >= 0) && (nExtensionOffset <= nDataOffset) &&
           (nSize <= (nDataOffset - nExtensionOffset));
}

bool XKWAJ::skipExtensionBytes(qint64 nDataOffset, qint64 nSize, qint64 *pExtensionOffset)
{
    if (!pExtensionOffset || !hasExtensionBytes(*pExtensionOffset, nDataOffset, nSize)) return false;
    *pExtensionOffset += nSize;
    return true;
}

bool XKWAJ::readBoundedExtensionString(qint64 nDataOffset, qint64 *pExtensionOffset,
                                       qint32 nMaximumBytes, QString *pString)
{
    if (!pExtensionOffset || !pString || !hasExtensionBytes(*pExtensionOffset, nDataOffset, 1)) return false;
    const qint32 nReadSize = (qint32)qMin<qint64>(nMaximumBytes, nDataOffset - *pExtensionOffset);
    const QByteArray baString = read_array(*pExtensionOffset, nReadSize);
    if (baString.size() != nReadSize) return false;
    const qint32 nTerminator = baString.indexOf('\0');
    if (nTerminator < 0) return false;
    *pString = QString::fromLatin1(baString.constData(), nTerminator);
    return skipExtensionBytes(nDataOffset, nTerminator + 1, pExtensionOffset);
}

bool XKWAJ::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    bool bResult = false;

    if (getSize() >= (qint64)sizeof(KWAJ_HEADER)) {
        // 4B 57 41 4A 88 F0 27 D1
        if ((read_uint8(0) == 0x4B) && (read_uint8(1) == 0x57) && (read_uint8(2) == 0x41) && (read_uint8(3) == 0x4A) && (read_uint8(4) == 0x88) &&
            (read_uint8(5) == 0xF0) && (read_uint8(6) == 0x27) && (read_uint8(7) == 0xD1)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XKWAJ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XKWAJ xkwaj(pDevice);

    return xkwaj.isValid(pPdStruct);
}

XBinary::HANDLE_METHOD XKWAJ::_compTypeToMethod(quint16 nCompType)
{
    switch (nCompType) {
        case COMP_TYPE_STORE: return HANDLE_METHOD_STORE;
        case COMP_TYPE_XOR: return HANDLE_METHOD_KWAJ_XOR;
        // KWAJ uses QBasic's +18 LZSS variant, not ordinary SZDD (+16).
        // Keep it on its dedicated route.
        case COMP_TYPE_SZDD: return HANDLE_METHOD_KWAJ_LZSS;
        case COMP_TYPE_LZH: return HANDLE_METHOD_KWAJ_LZH;
        case COMP_TYPE_MSZIP: return HANDLE_METHOD_KWAJ_MSZIP;
        default: return HANDLE_METHOD_UNKNOWN;
    }
}

XBinary::FT XKWAJ::getFileType()
{
    return FT_KWAJ;
}

XBinary::MODE XKWAJ::getMode()
{
    return MODE_DATA;
}

QString XKWAJ::getMIMEString()
{
    return "application/x-ms-compress-kwaj";
}

qint32 XKWAJ::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XKWAJ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XKWAJ::getArch()
{
    return QString();
}

QString XKWAJ::getFileFormatExt()
{
    return "kwaj";
}

QString XKWAJ::getFileFormatExtsString()
{
    return "KWAJ (*.kwaj)";
}

qint64 XKWAJ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

XBinary::OSNAME XKWAJ::getOsName()
{
    return OSNAME_MSDOS;
}

QString XKWAJ::getVersion()
{
    return QString();
}

QList<XBinary::MAPMODE> XKWAJ::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XKWAJ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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
    qint64 nDataOffset = read_uint16(offsetof(KWAJ_HEADER, data_offset));

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = (nDataOffset > 0 && nDataOffset <= getSize()) ? nDataOffset : (qint64)sizeof(KWAJ_HEADER);
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("KWAJ ") + tr("Header");
    result.listRecords.append(recHeader);

    if ((nDataOffset > 0) && (nDataOffset < getSize())) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = nDataOffset;
        recData.nSize = getSize() - nDataOffset;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Compressed Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XKWAJ::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XKWAJ_STRUCTID, sizeof(_TABLE_XKWAJ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XKWAJ::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XKWAJ_STRUCTID, sizeof(_TABLE_XKWAJ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XKWAJ::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XKWAJ_STRUCTID, sizeof(_TABLE_XKWAJ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XKWAJ::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_KWAJ_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_KWAJ_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_KWAJ_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(KWAJ_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_KWAJ_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_KWAJ_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XKWAJ::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_KWAJ_HEADER) {
        listResult.append({"signature", (qint32)offsetof(KWAJ_HEADER, signature), 8, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"comp_type", (qint32)offsetof(KWAJ_HEADER, comp_type), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"data_offset", (qint32)offsetof(KWAJ_HEADER, data_offset), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"header_flags", (qint32)offsetof(KWAJ_HEADER, header_flags), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    }

    return listResult;
}

static bool kwajCanAppendPart(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XKWAJ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nDataOffset = read_uint16(offsetof(KWAJ_HEADER, data_offset));
    if ((nDataOffset <= 0) || (nDataOffset > getSize())) {
        nDataOffset = sizeof(KWAJ_HEADER);
    }

    if ((nFileParts & FILEPART_HEADER) && kwajCanAppendPart(nLimit, listResult)) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = nDataOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && kwajCanAppendPart(nLimit, listResult) && (nDataOffset < getSize())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = nDataOffset;
        record.nFileSize = getSize() - nDataOffset;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Compressed Data");
        listResult.append(record);
    }

    return listResult;
}

QList<QString> XKWAJ::getSearchSignatures()
{
    return {"'KWAJ'88F027D1"};
}

XBinary *XKWAJ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XKWAJ(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XKWAJ::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XKWAJ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XKWAJ> guardedThis(this);
    if (m_bUnpackOperationInProgress) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState)) {
        return false;
    }
    KWAJ_UNPACK_CONTEXT *pOldContext = static_cast<KWAJ_UNPACK_CONTEXT *>(pState->pContext);
    guardedThis->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pOldContext;
    *pState = UNPACK_STATE();
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = guardedThis->bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) {
        return false;
    }

    const bool bValid = guardedThis->isValid(pPdStruct);
    if (!guardedThis) return false;
    if (!bValid) {
        guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nFileSize = guardedThis->getSize();
    if (!guardedThis || (nFileSize < (qint64)sizeof(KWAJ_HEADER))) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    const quint16 nCompType = guardedThis->read_uint16(offsetof(KWAJ_HEADER, comp_type));
    if (!guardedThis) return failUnpackInitialization(guardedThis.data(), pState);
    const qint64 nDataOffset = guardedThis->read_uint16(offsetof(KWAJ_HEADER, data_offset));
    if (!guardedThis) return failUnpackInitialization(guardedThis.data(), pState);
    const quint16 nHeaderFlags = guardedThis->read_uint16(offsetof(KWAJ_HEADER, header_flags));
    if (!guardedThis) return failUnpackInitialization(guardedThis.data(), pState);

    const quint16 nKnownHeaderFlags =
        HDR_FLAG_HASLENGTH | HDR_FLAG_HASUNKNOWN1 | HDR_FLAG_HASUNKNOWN2 | HDR_FLAG_HASFILENAME | HDR_FLAG_HASFILEEXT | HDR_FLAG_HASEXTRATEXT;
    if ((nCompType > COMP_TYPE_MSZIP) || (nDataOffset < (qint64)sizeof(KWAJ_HEADER)) || (nDataOffset > nFileSize) || ((nHeaderFlags & ~nKnownHeaderFlags) != 0)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    qint64 nUncompressedSize = 0;
    const bool bHasUncompressedSize = (nHeaderFlags & HDR_FLAG_HASLENGTH) != 0;

    // Every optional extension is bounded by data_offset.  Variable strings
    // are searched only within their format-defined 8.3 limits so malformed
    // metadata can never consume payload bytes.
    qint64 nExtOffset = sizeof(KWAJ_HEADER);

    if (bHasUncompressedSize) {
        if (!hasExtensionBytes(nExtOffset, nDataOffset, 4)) return failUnpackInitialization(guardedThis.data(), pState);
        nUncompressedSize = guardedThis->read_uint32(nExtOffset);
        if (!guardedThis || !skipExtensionBytes(nDataOffset, 4, &nExtOffset)) return failUnpackInitialization(guardedThis.data(), pState);
    }
    if ((nHeaderFlags & HDR_FLAG_HASUNKNOWN1) && !skipExtensionBytes(nDataOffset, 2, &nExtOffset)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }
    if (nHeaderFlags & HDR_FLAG_HASUNKNOWN2) {
        if (!hasExtensionBytes(nExtOffset, nDataOffset, 2)) return failUnpackInitialization(guardedThis.data(), pState);
        const quint16 nLength = guardedThis->read_uint16(nExtOffset);
        if (!guardedThis || !skipExtensionBytes(nDataOffset, 2, &nExtOffset) ||
            !skipExtensionBytes(nDataOffset, nLength, &nExtOffset)) {
            return failUnpackInitialization(guardedThis.data(), pState);
        }
    }

    QString sName;
    if ((nHeaderFlags & HDR_FLAG_HASFILENAME) &&
        !guardedThis->readBoundedExtensionString(nDataOffset, &nExtOffset, 9, &sName)) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }
    if (nHeaderFlags & HDR_FLAG_HASFILEEXT) {
        QString sExt;
        if (!guardedThis->readBoundedExtensionString(nDataOffset, &nExtOffset, 4, &sExt))
            return failUnpackInitialization(guardedThis.data(), pState);
        if (!sExt.isEmpty()) sName += QString(".") + sExt;
    }
    if (nHeaderFlags & HDR_FLAG_HASEXTRATEXT) {
        if (!hasExtensionBytes(nExtOffset, nDataOffset, 2)) return failUnpackInitialization(guardedThis.data(), pState);
        const quint16 nLength = guardedThis->read_uint16(nExtOffset);
        if (!guardedThis || !skipExtensionBytes(nDataOffset, 2, &nExtOffset) ||
            !skipExtensionBytes(nDataOffset, nLength, &nExtOffset)) {
            return failUnpackInitialization(guardedThis.data(), pState);
        }
    }

    if (nExtOffset > nDataOffset) {
        return failUnpackInitialization(guardedThis.data(), pState);
    }

    if (sName.isEmpty()) {
        sName = XBinary::getDeviceFileBaseName(guardedThis->getDevice());
        if (!guardedThis) return failUnpackInitialization(guardedThis.data(), pState);
        if (sName.isEmpty()) sName = "kwaj_data";
    }

    const qint64 nDataSize = nFileSize - nDataOffset;
    bool bUncompressedSizeDefined = bHasUncompressedSize;
    if (!bUncompressedSizeDefined && ((nCompType == COMP_TYPE_STORE) || (nCompType == COMP_TYPE_XOR))) {
        nUncompressedSize = nDataSize;
        bUncompressedSizeDefined = true;
    }

    KWAJ_UNPACK_CONTEXT *pContext = new (std::nothrow) KWAJ_UNPACK_CONTEXT;
    if (!pContext) {
        guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->nDataOffset = nDataOffset;
    pContext->nDataSize = nDataSize;
    pContext->compressMethod = guardedThis->_compTypeToMethod(nCompType);
    pContext->nUncompressedSize = nUncompressedSize;
    pContext->bUncompressedSizeDefined = bUncompressedSizeDefined;
    pContext->sFileName = sName;

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->nCurrentOffset = nDataOffset;
    pState->nTotalSize = getSize();
    pState->mapUnpackProperties = mapProperties;

    if (!guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XKWAJ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XKWAJ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) return result;
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    KWAJ_UNPACK_CONTEXT *pContext = (KWAJ_UNPACK_CONTEXT *)pState->pContext;

    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nDataSize);
    if (pContext->bUncompressedSizeDefined) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    }
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->compressMethod);

    return result;
}

bool XKWAJ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XKWAJ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!pState || !pState->pContext) return false;
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return false;
}

bool XKWAJ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XKWAJ> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState)) return false;
    KWAJ_UNPACK_CONTEXT *pContext = static_cast<KWAJ_UNPACK_CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    guardedThis->releaseUnpackSource(pState);
    if (!guardedThis) return false;
    delete pContext;
    if (!guardedThis) return false;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

bool XKWAJ::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XKWAJ> guardedThis(this);
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

void *XKWAJ::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XKWAJ> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XKWAJ::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
