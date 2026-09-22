/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xgentee.h"

#include <QDateTime>
#include <QtEndian>

#include <new>

#include "../Formats/exec/xpe.h"

namespace {
// The eight bytes are the first eight bytes of the runtime block's compressed
// stream; the size word in front of them is the block's decoded size.
const char GENTEE_SIGNATURE[] = "AB67A736FF4DFB6F";
const qint64 GENTEE_HEADER_SIZE = 12;
const qint64 GENTEE_MIN_PAYLOAD = 0x100;
// A Gentee installer is a desktop setup program; the ceiling only exists so a
// corrupt carrier cannot ask for an unbounded read.
const qint64 GENTEE_MAX_PAYLOAD = 0x20000000;
// The payload starts AT the overlay in 17 of the 18 reference carriers and
// 5,260 bytes into it in the eighteenth (a rebuilt resource section pushes it
// along).  Bounding the search keeps this detector's cost off the size of an
// unrelated executable's overlay.
const qint64 GENTEE_MAX_SEARCH = 0x100000;
}  // namespace

XGentee::XGentee(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGentee::~XGentee()
{
}

// The payload always sits in the PE overlay.  Searching from the overlay
// instead of from byte zero keeps an unrelated executable that happens to
// carry the same eight bytes in its code or resources out of the format.
bool XGentee::locatePayload(qint64 *pnPayloadOffset, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pnPayloadOffset || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < GENTEE_MIN_PAYLOAD) return false;

    XPE pe(getDevice());
    if (!pe.isValid(pPdStruct)) return false;
    const qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if ((nOverlayOffset < 0) || (nOverlayOffset + GENTEE_HEADER_SIZE > nInputSize)) return false;

    qint64 nSearchSize = nInputSize - nOverlayOffset;
    if (nSearchSize > GENTEE_MAX_SEARCH) nSearchSize = GENTEE_MAX_SEARCH;
    const qint64 nFound = find_signature(nOverlayOffset, nSearchSize, GENTEE_SIGNATURE, nullptr, pPdStruct);
    if (nFound < nOverlayOffset + 4) return false;

    const qint64 nPayloadOffset = nFound - 4;
    const QByteArray baHeader = read_array_process(nPayloadOffset, GENTEE_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != GENTEE_HEADER_SIZE)) return false;
    if (!XGenteeDecoder::isPayloadHeader(baHeader.constData(), baHeader.size())) return false;

    *pnPayloadOffset = nPayloadOffset;
    return true;
}

// Nothing in a block announces its packed length, so the member table only
// exists once the whole chain has been decoded: this walk IS the unpack.
bool XGentee::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (!locatePayload(&context.nPayloadOffset, pPdStruct)) return false;

    const qint64 nPayloadSize = context.nInputSize - context.nPayloadOffset;
    if ((nPayloadSize <= 0) || (nPayloadSize > GENTEE_MAX_PAYLOAD)) return false;

    const QByteArray baPayload = read_array_process(context.nPayloadOffset, nPayloadSize, pPdStruct);
    if ((baPayload.size() != nPayloadSize)) return false;

    qint64 nArchiveSize = 0;
    if (!XGenteeDecoder::scan(baPayload, &context.listMembers, &nArchiveSize, pPdStruct)) return false;
    if (context.listMembers.isEmpty()) return false;

    if (nArchiveSize <= 0) nArchiveSize = nPayloadSize;
    context.nArchiveSize = context.nPayloadOffset + nArchiveSize;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

// A stored member is its own extent; a compressed one is the payload from its
// first byte through the end of that member's block, because everything in
// between feeds the shared decoder state.
qint64 XGentee::memberStreamOffset(const CONTEXT &context, qint32 nIndex)
{
    const XGenteeDecoder::MEMBER &member = context.listMembers.at(nIndex);
    if (member.bStored) return context.nPayloadOffset + member.nDataOffset;
    return context.nPayloadOffset;
}

qint64 XGentee::memberStreamSize(const CONTEXT &context, qint32 nIndex)
{
    const XGenteeDecoder::MEMBER &member = context.listMembers.at(nIndex);
    if (member.bStored) return member.nSize;
    return member.nDataEnd;
}

void XGentee::fillMemberProperties(const CONTEXT &context, qint32 nIndex, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    const XGenteeDecoder::MEMBER &member = context.listMembers.at(nIndex);

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, memberStreamSize(context, nIndex));
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    if (member.bStored) {
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    } else {
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_GENTEE);
        pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Gentee"));
        pMapProperties->insert(FPART_PROP_COMPRESSPROPERTIES, XGenteeDecoder::indexToProperty(nIndex));
        // deliberately NOT FPART_PROP_ISSOLID - that property means "this record
        // is a substream of a 7z-style solid folder" and routes XDecompress into
        // a path that sizes the block from FPART_PROP_STREAMUNPACKEDSIZE or
        // FPART_PROP_SOLIDFOLDERINDEX.  A Gentee record has neither, so the
        // property was refused outright and every compressed member failed.
        // The solid stream is handled inside XGenteeDecoder, which replays the
        // preceding members from the index in FPART_PROP_COMPRESSPROPERTIES -
        // exactly the arrangement XQuantum uses.
    }
    // Every 0x87f4 record is a file; the reference implementation has no directory
    // record and the attribute word carries FILE_ATTRIBUTE_ARCHIVE throughout.
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);

    // The record's timestamp is a Windows FILETIME.  Anything outside the
    // range the format could have been used in is dropped rather than
    // published as a nonsense date.
    if ((member.nFileTime > Q_UINT64_C(116444736000000000)) && (member.nFileTime < Q_UINT64_C(160000000000000000))) {
        const qint64 nMSecs = (qint64)((member.nFileTime - Q_UINT64_C(116444736000000000)) / Q_UINT64_C(10000));
        pMapProperties->insert(FPART_PROP_DATETIME, QDateTime::fromMSecsSinceEpoch(nMSecs, Qt::UTC));
    }
}

bool XGentee::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    qint64 nPayloadOffset = 0;
    const bool bResult = locatePayload(&nPayloadOffset, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XGentee::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGentee archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGentee::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGentee(pDevice);
}

QList<QString> XGentee::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QString(GENTEE_SIGNATURE));
    return listResult;
}

XBinary::FT XGentee::getFileType()
{
    return FT_GENTEE;
}

XBinary::MODE XGentee::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGentee::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGentee::getArch()
{
    return QString();
}

QString XGentee::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XGentee::getFileFormatExtsString()
{
    return QStringLiteral("Gentee installer (*.exe)");
}

QString XGentee::getMIMEString()
{
    return QStringLiteral("application/x-gentee-installer");
}

QString XGentee::getVersion()
{
    return QString();
}

qint64 XGentee::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGentee::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XGentee::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XGentee::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XGentee::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nPayloadOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const XGenteeDecoder::MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = memberStreamOffset(context, i);
            part.nFileSize = memberStreamSize(context, i);
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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

QMap<XBinary::UNPACK_PROP, QVariant> XGentee::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGentee::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Gentee installer"));
    pState->nCurrentOffset = memberStreamOffset(*pContext, 0);
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

XBinary::ARCHIVERECORD XGentee::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const qint32 nIndex = pState->nCurrentIndex;
    if (pState->nCurrentOffset != memberStreamOffset(*pContext, nIndex)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = memberStreamOffset(*pContext, nIndex);
    result.nStreamSize = memberStreamSize(*pContext, nIndex);
    fillMemberProperties(*pContext, nIndex, &result.mapProperties);
    return result;
}

bool XGentee::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = memberStreamOffset(*pContext, pState->nCurrentIndex);
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XGentee::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
