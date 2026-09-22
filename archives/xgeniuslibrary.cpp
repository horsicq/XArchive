/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xgeniuslibrary.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 GPL_HEADER_SIZE = 0x20;
const qint64 GPL_ENTRY_SIZE = 0x36;
const qint64 GPL_COUNT_OFFSET = 0x14;
const qint32 GPL_MAX_MEMBERS = 100000;
// The reference implementation rejects a name length outside 1..0x400.
const qint32 GPL_MAX_NAME_SIZE = 0x400;
const quint8 GPL_METHOD_STORE = 0;
const quint8 GPL_METHOD_DCL_BLOCKS = 1;
// Member sizes are 64-bit fields, so they have to be bounded before they are
// used as buffer sizes.
const qint64 GPL_MAX_MEMBER_SIZE = Q_INT64_C(512) * 1024 * 1024;

bool gplRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// The name field is counted, not NUL terminated, but the writer stores the NUL
// inside the count; the name is the run in front of it.  Nothing in this corpus
// carries a path component, so a separator is a reject rather than something to
// strip: a name this reader cannot publish verbatim is not a name it understands.
bool gplReadName(const QByteArray &baName, QString *psResult)
{
    qint32 nEnd = baName.indexOf('\0');
    if (nEnd < 0) nEnd = baName.size();
    if (nEnd < 1) return false;

    const QByteArray baTrimmed = baName.left(nEnd);
    for (qint32 i = 0; i < baTrimmed.size(); ++i) {
        const quint8 nCharacter = static_cast<quint8>(baTrimmed.at(i));
        if ((nCharacter < 0x20U) || (nCharacter > 0x7EU)) return false;
        const char cCharacter = static_cast<char>(nCharacter);
        if ((cCharacter == '\\') || (cCharacter == '/') || (cCharacter == ':') || (cCharacter == '*') || (cCharacter == '?') || (cCharacter == '"') ||
            (cCharacter == '<') || (cCharacter == '>') || (cCharacter == '|')) {
            return false;
        }
    }

    *psResult = QString::fromLatin1(baTrimmed);
    return true;
}
}  // namespace

XGeniusLibrary::XGeniusLibrary(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGeniusLibrary::~XGeniusLibrary()
{
}

bool XGeniusLibrary::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (GPL_HEADER_SIZE + GPL_ENTRY_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, GPL_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != GPL_HEADER_SIZE)) return false;
    if (baHeader.left(14) != QByteArray("GENIUS LIBRARY", 14)) return false;
    if (baHeader.at(14) != '\0') return false;

    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    const qint32 nCount = static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + GPL_COUNT_OFFSET));
    if ((nCount < 1) || (nCount > GPL_MAX_MEMBERS)) return false;

    // +0x10..+0x11 is 0x0000 in 44/44 of the corpus.  +0x12..+0x13 is
    // uninitialised writer memory: 22 distinct values over those same 44 files
    // ("60" once, 0x0000 twice, otherwise fragments of unrelated strings left in
    // the writer's buffer - "_S", "pl", "DI", "*.", "_9", from names like
    // "_gpl.dir" and "*.DXF").  It is not a version, so it is neither validated
    // nor published.
    context.sVersion = QString();

    qint64 nOffset = GPL_HEADER_SIZE;
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!gplRangeWithin(context.nInputSize, nOffset, GPL_ENTRY_SIZE)) return false;

        const QByteArray baEntry = read_array_process(nOffset, GPL_ENTRY_SIZE, pPdStruct);
        if ((baEntry.size() != GPL_ENTRY_SIZE)) return false;

        const uchar *pEntry = reinterpret_cast<const uchar *>(baEntry.constData());
        const qint64 nDeclaredDataOffset = static_cast<qint64>(qFromLittleEndian<quint64>(pEntry));
        const qint64 nCompressedSize = static_cast<qint64>(qFromLittleEndian<quint64>(pEntry + 0x08));
        const qint64 nUncompressedSize = static_cast<qint64>(qFromLittleEndian<quint64>(pEntry + 0x10));
        const quint64 nWriteTime = qFromLittleEndian<quint64>(pEntry + 0x28);
        const quint8 nMethod = static_cast<quint8>(baEntry.at(0x33));
        const qint32 nNameSize = static_cast<qint32>(qFromLittleEndian<quint16>(pEntry + 0x34));

        if ((nCompressedSize < 0) || (nUncompressedSize < 0)) return false;
        if ((nCompressedSize > GPL_MAX_MEMBER_SIZE) || (nUncompressedSize > GPL_MAX_MEMBER_SIZE)) return false;
        if ((nNameSize < 1) || (nNameSize > GPL_MAX_NAME_SIZE)) return false;
        if (!gplRangeWithin(context.nInputSize, nOffset + GPL_ENTRY_SIZE, nNameSize)) return false;

        const QByteArray baName = read_array_process(nOffset + GPL_ENTRY_SIZE, nNameSize, pPdStruct);
        if ((baName.size() != nNameSize)) return false;

        MEMBER member = {};
        if (!gplReadName(baName, &member.sFileName)) return false;

        member.nHeaderOffset = nOffset;
        member.nHeaderSize = GPL_ENTRY_SIZE + nNameSize;
        member.nDataOffset = nOffset + member.nHeaderSize;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nWriteTime = nWriteTime;
        member.nMethod = nMethod;

        // The entry states where its data starts; it has to be where the walk
        // already is.  Over the reference corpus it always is, and a mismatch
        // means this is not the record chain it claims to be.
        if (nDeclaredDataOffset != member.nDataOffset) return false;
        if (!gplRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nOffset;
    *pContext = context;

    return true;
}

XBinary::HANDLE_METHOD XGeniusLibrary::memberHandleMethod(const MEMBER &member)
{
    if (member.nMethod == GPL_METHOD_STORE) {
        // A stored member is the plaintext verbatim, so the two sizes have to
        // agree; if they do not, the record is not something this reader can
        // reproduce and must not be published as stored bytes.
        if (member.nCompressedSize == member.nUncompressedSize) return HANDLE_METHOD_STORE;
        return HANDLE_METHOD_UNKNOWN;
    }
    if (member.nMethod == GPL_METHOD_DCL_BLOCKS) {
        if (member.nUncompressedSize > 0) return HANDLE_METHOD_GENIUS_BLOCKS;
        return HANDLE_METHOD_UNKNOWN;
    }
    return HANDLE_METHOD_UNKNOWN;
}

QString XGeniusLibrary::memberReportedMethod(const MEMBER &member)
{
    if (member.nMethod == GPL_METHOD_STORE) return QStringLiteral("Stored");
    if (member.nMethod == GPL_METHOD_DCL_BLOCKS) return QStringLiteral("PKWARE DCL Implode blocks");
    return QStringLiteral("Method %1").arg(member.nMethod);
}

bool XGeniusLibrary::isValid(PDSTRUCT *pPdStruct)
{
    // getRecords-style probing displaces the caller's cursor, so snapshot it.
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XGeniusLibrary::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGeniusLibrary archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGeniusLibrary::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGeniusLibrary(pDevice);
}

QList<QString> XGeniusLibrary::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'GENIUS LIBRARY'00");
}

XBinary::FT XGeniusLibrary::getFileType()
{
    return FT_GENIUS_LIBRARY;
}

XBinary::MODE XGeniusLibrary::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGeniusLibrary::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGeniusLibrary::getArch()
{
    return QString();
}

qint32 XGeniusLibrary::getType()
{
    return TYPE_ARCHIVE;
}

QString XGeniusLibrary::getFileFormatExt()
{
    return QStringLiteral("gpl");
}

QString XGeniusLibrary::getFileFormatExtsString()
{
    return QStringLiteral("Genius Library (*.gpl)");
}

QString XGeniusLibrary::getMIMEString()
{
    return QStringLiteral("application/x-genius-library");
}

QString XGeniusLibrary::getVersion()
{
    // The container states no version.  What sits at +0x10 is 0x0000 followed by
    // two bytes of uninitialised writer memory, so there is nothing to report and
    // no reason to pay for a parse to report it.
    return QString();
}

qint64 XGeniusLibrary::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGeniusLibrary::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XGeniusLibrary::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XGeniusLibrary::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XGeniusLibrary::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = GPL_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, memberHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, memberReportedMethod(member));
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
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XGeniusLibrary::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGeniusLibrary::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XGeniusLibrary::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, memberHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, memberReportedMethod(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtModified = winFileTimeToQDateTime(member.nWriteTime);
    if (dtModified.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dtModified);
    return result;
}

bool XGeniusLibrary::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XGeniusLibrary::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XGeniusLibrary::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_DATETIME;
}
