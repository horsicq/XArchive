/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xcreateinstallsfx.h"

#include <QDateTime>
#include <QSet>

#include <new>

#include "xpe.h"

namespace {
// The container always sits AT the PE overlay; nothing is searched for.
const qint64 CISFX_MIN_CONTAINER = 0x40;
// A CreateInstall installer is a desktop setup program.  The ceiling only
// exists so a corrupt carrier cannot ask for an unbounded read.
const qint64 CISFX_MAX_CONTAINER = Q_INT64_C(0x10000000);
// Windows file attributes carried by a type 1 record.
const quint32 CISFX_ATTR_READONLY = 0x00000001;
const quint32 CISFX_ATTR_HIDDEN = 0x00000002;
const quint32 CISFX_ATTR_SYSTEM = 0x00000004;
const quint32 CISFX_ATTR_ARCHIVE = 0x00000020;
// FILETIME bounds: 1970-01-01 and roughly 2107.  A record outside them is not
// published as a nonsense date.
const quint64 CISFX_FILETIME_MIN = Q_UINT64_C(116444736000000000);
const quint64 CISFX_FILETIME_MAX = Q_UINT64_C(160000000000000000);
}  // namespace

XCreateInstallSFX::XCreateInstallSFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCreateInstallSFX::~XCreateInstallSFX()
{
}

// Detection is deliberately cheap: the carrier has to be a PE with an overlay,
// and the overlay has to open with the constant compressed form of the
// runtime's MZ header.  The record walk costs a full decode and belongs in
// parseContext(), not here.
bool XCreateInstallSFX::locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pnContainerOffset || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < CISFX_MIN_CONTAINER) return false;

    XPE pe(getDevice());
    if (!pe.isValid(pPdStruct)) return false;
    const qint64 nContainerOffset = pe.getOverlayOffset(pPdStruct);
    if ((nContainerOffset <= 0) || (nContainerOffset >= nInputSize)) return false;

    const qint64 nContainerSize = nInputSize - nContainerOffset;
    if ((nContainerSize < CISFX_MIN_CONTAINER) || (nContainerSize > CISFX_MAX_CONTAINER)) return false;

    const QByteArray baHead = read_array_process(nContainerOffset, XCreateInstallDecoder::SIGNATURE_SIZE, pPdStruct);
    if ((baHead.size() != XCreateInstallDecoder::SIGNATURE_SIZE)) return false;
    if (!XCreateInstallDecoder::checkSignature((const quint8 *)baHead.constData(), (qint64)baHead.size())) return false;

    *pnContainerOffset = nContainerOffset;
    return true;
}

bool XCreateInstallSFX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (!locateContainer(&context.nContainerOffset, pPdStruct)) return false;

    const qint64 nPayloadSize = context.nInputSize - context.nContainerOffset;
    const QByteArray baPayload = read_array_process(context.nContainerOffset, nPayloadSize, pPdStruct);
    if (((qint64)baPayload.size() != nPayloadSize)) return false;

    qint64 nArchiveSize = 0;
    if (!XCreateInstallDecoder::walkContainer(baPayload, context.nContainerOffset, &context.listMembers, &nArchiveSize, pPdStruct)) return false;
    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nContainerOffset + nArchiveSize;
    context.nRuntimeSize = context.listMembers.at(0).nUncompressedSize;

    // The walk publishes payload-relative offsets; every consumer of a member
    // wants a file offset.
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        context.listMembers[i].nStreamOffset += context.nContainerOffset;
    }

    // Nothing stops a container from carrying the same name twice.  None of the
    // reach set does, but a second record must not silently overwrite the
    // first, so a repeat falls back to an indexed name.
    QSet<QString> setUsedNames;
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        QString sName = context.listMembers.at(i).sFileName;
        if (setUsedNames.contains(sName.toCaseFolded())) sName = QString("%1.%2").arg(sName).arg(i, 4, 10, QChar('0'));
        if (setUsedNames.contains(sName.toCaseFolded())) return false;
        setUsedNames.insert(sName.toCaseFolded());
        context.listMembers[i].sFileName = sName;
    }

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

void XCreateInstallSFX::fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    const XCreateInstallDecoder::RECORD &member = context.listMembers.at(nIndex);

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    // A zero-length member carries no stream at all, whatever its method byte
    // says, so it is a store of nothing rather than an empty coded stream.
    if (member.bStored || (member.nUncompressedSize == 0)) {
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    } else {
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_CREATEINSTALL);
        pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("CreateInstall"));
    }
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
    // The record's second byte.  It is 0 in 252 of the 254 records of the reach
    // set and 1 in the other two (both ordinary files); what it selects is not
    // established, so it is published verbatim rather than interpreted.
    pMapProperties->insert(FPART_PROP_FLAGS, (qint64)member.nFlag);

    if (member.nAttributes) {
        pMapProperties->insert(FPART_PROP_ISREADONLY, (member.nAttributes & CISFX_ATTR_READONLY) != 0);
        pMapProperties->insert(FPART_PROP_ISHIDDEN, (member.nAttributes & CISFX_ATTR_HIDDEN) != 0);
        pMapProperties->insert(FPART_PROP_ISSYSTEM, (member.nAttributes & CISFX_ATTR_SYSTEM) != 0);
        pMapProperties->insert(FPART_PROP_ISARCHIVE, (member.nAttributes & CISFX_ATTR_ARCHIVE) != 0);
    }

    if ((member.nFileTime > CISFX_FILETIME_MIN) && (member.nFileTime < CISFX_FILETIME_MAX)) {
        const qint64 nMSecs = (qint64)((member.nFileTime - CISFX_FILETIME_MIN) / Q_UINT64_C(10000));
        const QDateTime dtModified = QDateTime::fromMSecsSinceEpoch(nMSecs, Qt::UTC);
        pMapProperties->insert(FPART_PROP_DATETIME, dtModified);
        pMapProperties->insert(FPART_PROP_MTIME, dtModified);
    }
}

bool XCreateInstallSFX::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    qint64 nContainerOffset = 0;
    const bool bResult = locateContainer(&nContainerOffset, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XCreateInstallSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCreateInstallSFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XCreateInstallSFX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCreateInstallSFX(pDevice);
}

XBinary::FT XCreateInstallSFX::getFileType()
{
    return FT_CREATEINSTALL_SFX;
}

XBinary::MODE XCreateInstallSFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XCreateInstallSFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCreateInstallSFX::getArch()
{
    return QString();
}

QString XCreateInstallSFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XCreateInstallSFX::getFileFormatExtsString()
{
    return QStringLiteral("CreateInstall installer (*.exe)");
}

QString XCreateInstallSFX::getMIMEString()
{
    return QStringLiteral("application/x-createinstall-installer");
}

QString XCreateInstallSFX::getVersion()
{
    return QString();
}

qint64 XCreateInstallSFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XCreateInstallSFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XCreateInstallSFX::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XCreateInstallSFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XCreateInstallSFX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nContainerOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = context.listMembers.at(i).nStreamOffset;
            part.nFileSize = context.listMembers.at(i).nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = context.listMembers.at(i).sFileName;
            fillMemberProperties(context, i, &part.mapProperties);
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XCreateInstallSFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCreateInstallSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("CreateInstall installer"));
    pState->nCurrentOffset = pContext->listMembers.at(0).nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XCreateInstallSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const qint32 nIndex = pState->nCurrentIndex;
    if (pState->nCurrentOffset != pContext->listMembers.at(nIndex).nStreamOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->listMembers.at(nIndex).nStreamOffset;
    result.nStreamSize = pContext->listMembers.at(nIndex).nStreamSize;
    fillMemberProperties(*pContext, nIndex, &result.mapProperties);
    return result;
}

bool XCreateInstallSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nStreamOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XCreateInstallSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
