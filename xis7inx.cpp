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
#include "xis7inx.h"

#include <new>

static XBinary::XCONVERT _TABLE_XIS7INX_STRUCTID[] = {{XIS7Inx::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                      {XIS7Inx::STRUCTID_IS7INX_HEADER, "IS7INX_HEADER", QString("IS7INX_HEADER")}};

// The obfuscated image of the constant plaintext prefix "aLuZ\0\0Copyright ".
static const quint8 IS7INX_SIGNATURE[16] = {0x74, 0xC4, 0x2C, 0x84, 0xE1, 0xE5, 0xD4, 0x28, 0x10, 0xFB, 0x00, 0x20, 0x3C, 0x24, 0xFB, 0x4D};

static const qint64 IS7INX_SIGNATURE_SIZE = 16;

// The reference tool names the single member unconditionally; keep the same
// label so extraction matches it name for name.
static const char IS7INX_MEMBER_NAME[] = "Setup.ini";

XIS7Inx::XIS7Inx(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XIS7Inx::_readAndCheckHeader(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QPointer<XIS7Inx> guardedArchive(this);

    QPointer<QIODevice> guardedDevice(guardedArchive->getDevice());
    if (!guardedArchive || !guardedDevice) return false;

    // Detection probes a device the caller still owns: remember where it was
    // and put it back before returning.
    const qint64 nSavedPosition = guardedDevice->pos();

    const qint64 nSize = guardedArchive->getSize();
    if (!guardedArchive || !guardedDevice) return false;

    bool bResult = false;

    if (nSize > IS7INX_SIGNATURE_SIZE) {
        const QByteArray baHeader = guardedArchive->read_array(0, (qint32)IS7INX_SIGNATURE_SIZE);

        if (guardedArchive && guardedDevice && (baHeader.size() == (qint32)IS7INX_SIGNATURE_SIZE)) {
            bResult = (memcmp(baHeader.constData(), IS7INX_SIGNATURE, (size_t)IS7INX_SIGNATURE_SIZE) == 0);
        }
    }

    if (guardedDevice) guardedDevice->seek(nSavedPosition);

    return guardedArchive && guardedDevice && bResult;
}

bool XIS7Inx::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIS7Inx xis7inx(pDevice);

    return xis7inx.isValid(pPdStruct);
}

bool XIS7Inx::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    return _readAndCheckHeader(pPdStruct);
}

XIS7Inx::IS7INX_HEADER XIS7Inx::_read_IS7INX_HEADER(qint64 nOffset)
{
    IS7INX_HEADER header = {};
    read_array(nOffset, reinterpret_cast<char *>(&header), sizeof(IS7INX_HEADER));
    return header;
}

XBinary::FT XIS7Inx::getFileType()
{
    return XBinary::FT_IS7_INX;
}

XBinary::MODE XIS7Inx::getMode()
{
    return XBinary::MODE_DATA;
}

QString XIS7Inx::getMIMEString()
{
    return "application/x-installshield-inx";
}

qint32 XIS7Inx::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XIS7Inx::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XIS7Inx::getArch()
{
    return QString();
}

QString XIS7Inx::getFileFormatExt()
{
    return "inx";
}

QString XIS7Inx::getFileFormatExtsString()
{
    return "InstallShield compiled InstallScript (*.inx)";
}

qint64 XIS7Inx::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XIS7Inx::isSigned()
{
    return false;
}

XBinary::OSNAME XIS7Inx::getOsName()
{
    return XBinary::OSNAME_WINDOWS;
}

QString XIS7Inx::getOsVersion()
{
    return QString();
}

QString XIS7Inx::getVersion()
{
    return QString();
}

bool XIS7Inx::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XIS7Inx::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XIS7Inx::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    if (!_readAndCheckHeader(pPdStruct)) {
        return result;
    }

    const qint64 nTotalSize = getSize();
    qint32 nIndex = 0;

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = IS7INX_SIGNATURE_SIZE;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("IS7 INX ") + tr("Header");
    result.listRecords.append(recHeader);

    if (nTotalSize > IS7INX_SIGNATURE_SIZE) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = IS7INX_SIGNATURE_SIZE;
        recData.nSize = nTotalSize - IS7INX_SIGNATURE_SIZE;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XIS7Inx::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XIS7INX_STRUCTID, sizeof(_TABLE_XIS7INX_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XIS7Inx::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XIS7INX_STRUCTID, sizeof(_TABLE_XIS7INX_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XIS7Inx::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XIS7INX_STRUCTID, sizeof(_TABLE_XIS7INX_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XIS7Inx::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_IS7INX_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_IS7INX_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_IS7INX_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(IS7INX_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_IS7INX_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_IS7INX_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XIS7Inx::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_IS7INX_HEADER) {
        listResult.append({"signature", (qint32)offsetof(IS7INX_HEADER, signature), 16, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
    }

    return listResult;
}

static bool is7inxCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XIS7Inx::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    if (nFileParts == 0) {
        return listResult;
    }

    if (!_readAndCheckHeader(pPdStruct)) {
        return listResult;
    }

    const qint64 nTotalSize = getSize();

    if ((nFileParts & FILEPART_HEADER) && is7inxCanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = IS7INX_SIGNATURE_SIZE;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && is7inxCanAppend(nLimit, listResult)) {
        // The obfuscation covers the file from offset 0 (the signature bytes
        // are themselves obfuscated plaintext), so the member is the whole
        // container and the codec is length preserving.
        FPART record = {};

        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = nTotalSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = QString(IS7INX_MEMBER_NAME);
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, QString(IS7INX_MEMBER_NAME));
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IS7_INX);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nTotalSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nTotalSize);

        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XIS7Inx::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XIS7Inx::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XIS7Inx> guardedArchive(this);
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState && !m_bUnpackOperationInProgress && ((!pState->pContext && pState->baUnpackSourceToken.isEmpty()) || guardedArchive->ownsUnpackSource(pState))) {
        if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
        UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
        if (!operationGuard.isAcquired()) return false;

        if (!isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
        if (!guardedArchive || !bBound) return false;

        const bool bValid = guardedArchive->_readAndCheckHeader(pPdStruct);
        if (!guardedArchive) return false;
        if (!bValid) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }

        const qint64 nSize = guardedArchive->getSize();
        if (!guardedArchive) return false;

        IS7INX_UNPACK_CONTEXT *pContext = new (std::nothrow) IS7INX_UNPACK_CONTEXT;
        if (!pContext) {
            guardedArchive->releaseUnpackSource(pState);
            return false;
        }

        pContext->nTotalSize = nSize;
        pContext->sFileName = QString(IS7INX_MEMBER_NAME);

        pState->nCurrentOffset = 0;
        pState->nTotalSize = nSize;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;
        pState->pContext = pContext;
        pState->mapUnpackProperties = mapProperties;

        bResult = guardedArchive->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
        if (!guardedArchive) return false;
        if (!bResult) {
            pState->pContext = nullptr;
            guardedArchive->releaseUnpackSource(pState);
            delete pContext;
            *pState = UNPACK_STATE();
        }
    }

    return bResult;
}

XBinary::ARCHIVERECORD XIS7Inx::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XIS7Inx> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    IS7INX_UNPACK_CONTEXT *pContext = reinterpret_cast<IS7INX_UNPACK_CONTEXT *>(pState->pContext);

    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nTotalSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nTotalSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IS7_INX);

    return result;
}

bool XIS7Inx::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XIS7Inx> guardedArchive(this);

    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;

    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XIS7Inx::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XIS7Inx> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    IS7INX_UNPACK_CONTEXT *pContext = static_cast<IS7INX_UNPACK_CONTEXT *>(pState->pContext);
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

QList<QString> XIS7Inx::getSearchSignatures()
{
    return {"74C42C84E1E5D42810FB00203C24FB4D"};
}

XBinary *XIS7Inx::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XIS7Inx(pDevice);
}

bool XIS7Inx::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XIS7Inx> guardedThis(this);
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

void *XIS7Inx::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XIS7Inx> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XIS7Inx::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
