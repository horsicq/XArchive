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
#include "xecmpacked.h"

#include <new>

static XBinary::XCONVERT _TABLE_XECMPACKED_STRUCTID[] = {{XEcmPacked::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                         {XEcmPacked::STRUCTID_ECM_HEADER, "ECM_HEADER", QString("ECM_HEADER")}};

namespace {
// 45 43 4D 00 - fixed for every EmmaSetup packed member.
const char ECM_SIGNATURE[4] = {'E', 'C', 'M', '\x00'};
const qint64 ECM_HEADER_SIZE = 38;
// Offset 0x10 .. 0x24 is a run of twenty zero bytes in every member of the
// reference corpus.  It is the check that separates this container from Neill
// Corlett's unrelated "ECM\0" files, whose token stream treats 0x00 as the
// end-of-stream marker and therefore cannot carry such a run.
const qint64 ECM_RESERVED_OFFSET = 16;
const qint64 ECM_RESERVED_SIZE = 20;
// Same ceiling the shared Okumura LZSS decoder enforces; a pathological stream
// must not turn into an attacker-chosen allocation.
const qint64 ECM_MAX_UNPACKED_SIZE = 0x10000000;  // 256 MiB
const qint64 ECM_SCAN_CHUNK = 0x10000;
}  // namespace

XEcmPacked::XEcmPacked(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XEcmPacked::measureStream(qint64 nOffset, qint64 nSize, qint64 *pnUncompressedSize, PDSTRUCT *pPdStruct)
{
    QPointer<XEcmPacked> guardedThis(this);

    if (!pnUncompressedSize) return false;

    *pnUncompressedSize = 0;

    if (nSize <= 0) return false;

    QByteArray baChunk;
    qint64 nChunkOffset = 0;  // stream-relative offset of baChunk[0]
    qint64 nPosition = 0;     // stream-relative cursor
    qint64 nUnpacked = 0;
    quint32 nFlags = 0;
    qint32 nFlagBits = 0;

    while (nPosition < nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        // Every step needs one byte; a match token needs two.
        const qint64 nWanted = qMin<qint64>(2, nSize - nPosition);
        qint64 nAvailable = 0;
        if ((nPosition >= nChunkOffset) && (nPosition <= nChunkOffset + baChunk.size())) {
            nAvailable = nChunkOffset + baChunk.size() - nPosition;
        }

        if (nAvailable < nWanted) {
            nChunkOffset = nPosition;
            const qint64 nToRead = qMin<qint64>(ECM_SCAN_CHUNK, nSize - nPosition);
            baChunk = guardedThis->read_array(nOffset + nPosition, nToRead);
            if (!guardedThis) return false;
            if (baChunk.size() != nToRead) return false;
        }

        const quint8 *pChunk = reinterpret_cast<const quint8 *>(baChunk.constData());
        const qint64 nIndex = nPosition - nChunkOffset;

        if (nFlagBits == 0) {
            nFlags = pChunk[nIndex];
            nFlagBits = 8;
            ++nPosition;
            continue;
        }

        if (nFlags & 1U) {
            ++nPosition;
            ++nUnpacked;
        } else {
            // A genuine stream never truncates a two-byte match token: it ends
            // either on a flag-group boundary or with the input exhausted
            // between tokens.  Anything else is not this format.
            if (nPosition + 2 > nSize) return false;
            const quint32 nSecond = pChunk[nIndex + 1];
            nUnpacked += static_cast<qint64>(nSecond & 0x0fU) + 3;
            nPosition += 2;
        }

        nFlags >>= 1;
        --nFlagBits;

        if (nUnpacked > ECM_MAX_UNPACKED_SIZE) return false;
    }

    if (nUnpacked <= 0) return false;

    *pnUncompressedSize = nUnpacked;

    return true;
}

bool XEcmPacked::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XEcmPacked> guardedThis(this);

    if (!pContext) return false;

    *pContext = CONTEXT();
    pContext->nHeaderSize = ECM_HEADER_SIZE;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedThis || !guardedSource) return false;

    const qint64 nTotalSize = guardedThis->getSize();
    if (!guardedThis) return false;

    // Header plus at least a flag byte and one token byte.
    if (nTotalSize < ECM_HEADER_SIZE + 2) return false;

    const QByteArray baSignature = guardedThis->read_array(0, 4);
    if (!guardedThis) return false;
    if (baSignature.size() != 4) return false;
    if (memcmp(baSignature.constData(), ECM_SIGNATURE, 4) != 0) return false;

    const QByteArray baReserved = guardedThis->read_array(ECM_RESERVED_OFFSET, ECM_RESERVED_SIZE);
    if (!guardedThis) return false;
    if (baReserved.size() != ECM_RESERVED_SIZE) return false;
    for (qint64 i = 0; i < ECM_RESERVED_SIZE; ++i) {
        if (baReserved.at(static_cast<qint32>(i)) != 0) return false;
    }

    pContext->nVersion = guardedThis->read_uint16(4);
    if (!guardedThis) return false;

    pContext->nCompressedOffset = ECM_HEADER_SIZE;
    pContext->nCompressedSize = nTotalSize - ECM_HEADER_SIZE;

    qint64 nUncompressedSize = 0;
    const bool bMeasured = guardedThis->measureStream(pContext->nCompressedOffset, pContext->nCompressedSize, &nUncompressedSize, pPdStruct);
    if (!guardedThis || !bMeasured) return false;

    pContext->nUncompressedSize = nUncompressedSize;

    // The container carries no member name; the installer supplies it out of
    // band.  Keep the device's complete file name, extension included.
    const QString sDeviceFileName = XBinary::getDeviceFileName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    if (!sDeviceFileName.isEmpty()) {
        pContext->sFileName = QFileInfo(sDeviceFileName).fileName();
    }
    if (pContext->sFileName.isEmpty()) pContext->sFileName = QStringLiteral("ecm_data");

    return true;
}

bool XEcmPacked::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XEcmPacked> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = guardedThis->parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return guardedThis && bResult;
}

bool XEcmPacked::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEcmPacked xecmpacked(pDevice);

    return xecmpacked.isValid(pPdStruct);
}

XEcmPacked::ECM_HEADER XEcmPacked::_read_ECM_HEADER(qint64 nOffset)
{
    ECM_HEADER header = {};
    read_array(nOffset, reinterpret_cast<char *>(&header), sizeof(ECM_HEADER));
    return header;
}

XBinary::FT XEcmPacked::getFileType()
{
    return XBinary::FT_ECM_PACK;
}

XBinary::MODE XEcmPacked::getMode()
{
    return XBinary::MODE_DATA;
}

QString XEcmPacked::getMIMEString()
{
    return "application/x-emmasetup-ecm";
}

qint32 XEcmPacked::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XEcmPacked::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XEcmPacked::getArch()
{
    return QString();
}

QString XEcmPacked::getFileFormatExt()
{
    return "ECM";
}

QString XEcmPacked::getFileFormatExtsString()
{
    return "EmmaSetup packed (*.*)";
}

qint64 XEcmPacked::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XEcmPacked::isSigned()
{
    return false;
}

XBinary::OSNAME XEcmPacked::getOsName()
{
    return XBinary::OSNAME_WINDOWS;
}

QString XEcmPacked::getOsVersion()
{
    return QString();
}

QString XEcmPacked::getVersion()
{
    QPointer<XEcmPacked> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bParsed = guardedThis->parseContext(&context, nullptr);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!guardedThis || !bParsed) return QString();

    return QString("0x%1").arg(context.nVersion, 4, 16, QChar('0'));
}

bool XEcmPacked::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XEcmPacked::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XEcmPacked::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bParsed = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!bParsed) {
        return result;
    }

    qint32 nIndex = 0;

    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = XADDR_MAX;
    recHeader.nOffset = 0;
    recHeader.nSize = context.nHeaderSize;
    recHeader.nIndex = nIndex++;
    recHeader.filePart = FILEPART_HEADER;
    recHeader.sName = QString("ECM ") + tr("Header");
    result.listRecords.append(recHeader);

    if (context.nCompressedSize > 0) {
        _MEMORY_RECORD recData = {};
        recData.nAddress = XADDR_MAX;
        recData.nOffset = context.nCompressedOffset;
        recData.nSize = context.nCompressedSize;
        recData.nIndex = nIndex++;
        recData.filePart = FILEPART_REGION;
        recData.sName = tr("Compressed Data");
        result.listRecords.append(recData);
    }

    _handleOverlay(&result);

    return result;
}

QString XEcmPacked::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XECMPACKED_STRUCTID, sizeof(_TABLE_XECMPACKED_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XEcmPacked::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XECMPACKED_STRUCTID, sizeof(_TABLE_XECMPACKED_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XEcmPacked::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XECMPACKED_STRUCTID, sizeof(_TABLE_XECMPACKED_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XEcmPacked::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_ECM_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_ECM_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_ECM_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(ECM_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_ECM_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_ECM_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XEcmPacked::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_ECM_HEADER) {
        listResult.append({"signature", (qint32)offsetof(ECM_HEADER, signature), 4, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"version", (qint32)offsetof(ECM_HEADER, version), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"field_06", (qint32)offsetof(ECM_HEADER, field_06), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"field_0a", (qint32)offsetof(ECM_HEADER, field_0a), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"field_0c", (qint32)offsetof(ECM_HEADER, field_0c), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"reserved", (qint32)offsetof(ECM_HEADER, reserved), 20, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"field_24", (qint32)offsetof(ECM_HEADER, field_24), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    }

    return listResult;
}

QList<XBinary::FPART> XEcmPacked::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) {
        return listResult;
    }

    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bParsed = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!bParsed) {
        return listResult;
    }

    if ((nFileParts & FILEPART_HEADER) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = context.nHeaderSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_STREAM) && (context.nCompressedSize > 0) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_STREAM;
        record.nFileOffset = context.nCompressedOffset;
        record.nFileSize = context.nCompressedSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nCompressedSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_AMPK_LZSS);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QString("LZSS"));
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && (context.nCompressedSize > 0) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = context.nCompressedOffset;
        record.nFileSize = context.nCompressedSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Compressed Data");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_DATA) && ((nLimit == -1) || (listResult.size() < nLimit))) {
        FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = context.nHeaderSize + context.nCompressedSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    return listResult;
}

bool XEcmPacked::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XEcmPacked> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    const bool bParsed = guardedThis->parseContext(pContext, pPdStruct);
    if (!bParsed || !guardedThis || !guardedSource) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("EmmaSetup packed member; Okumura LZSS (4 KiB window, F = 18)"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = guardedThis->getSize();
    if (!guardedThis) {
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XEcmPacked::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();

    QPointer<XEcmPacked> guardedThis(this);

    if (!pState || !isPdStructNotCanceled(pPdStruct) || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nCompressedOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_AMPK_LZSS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QString("LZSS"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XEcmPacked::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<XEcmPacked> guardedThis(this);

    if (!pState || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) {
        return false;
    }

    ++pState->nCurrentIndex;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        return true;
    }

    pState->nCurrentOffset = pContext->nCompressedOffset + pContext->nCompressedSize;

    return false;
}

bool XEcmPacked::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<QString> XEcmPacked::getSearchSignatures()
{
    return {"'ECM'00"};
}

XBinary *XEcmPacked::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XEcmPacked(pDevice);
}

bool XEcmPacked::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XEcmPacked> guardedThis(this);
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

void *XEcmPacked::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XEcmPacked> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XEcmPacked::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
