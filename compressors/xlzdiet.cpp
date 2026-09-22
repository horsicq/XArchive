/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlzdiet.h"
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 LZDIET_MAGIC_SIZE = 6;
const qint64 LZDIET_HEADER_SIZE = 0x5FC;
const qint64 LZDIET_TABLE_OFFSET = 0x20;
const qint64 LZDIET_ENTRY_SIZE = 6;
const qint32 LZDIET_MAX_CHUNKS = 250;
const qint64 LZDIET_CHUNK_PREAMBLE = 6;
// A 2 GiB payload is far past anything this container was built for and keeps
// every size computation inside qint32 where the decoder needs it.
const qint64 LZDIET_MAX_UNCOMPRESSED = 0x40000000;

bool lzdietRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XLZDIET::XLZDIET(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZDIET::~XLZDIET()
{
}

bool XLZDIET::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Header plus at least one chunk with its 6-byte preamble.
    if (context.nInputSize < LZDIET_HEADER_SIZE + LZDIET_CHUNK_PREAMBLE) return false;

    const QByteArray baHeader = read_array_process(0, LZDIET_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != LZDIET_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (memcmp(pHeader, "lZdIeT", LZDIET_MAGIC_SIZE) != 0) return false;

    context.nUncompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + 6));
    if ((context.nUncompressedSize <= 0) || (context.nUncompressedSize > LZDIET_MAX_UNCOMPRESSED)) return false;

    // The reference detector's structural anchor: the first chunk sits exactly
    // behind the header and is at least as long as its own preamble.
    if (static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + LZDIET_TABLE_OFFSET)) != LZDIET_HEADER_SIZE) return false;
    if (qFromLittleEndian<quint16>(pHeader + LZDIET_TABLE_OFFSET + 4) < LZDIET_CHUNK_PREAMBLE) return false;

    qint64 nExpectedOffset = LZDIET_HEADER_SIZE;
    qint64 nEnd = LZDIET_HEADER_SIZE;
    bool bTerminated = false;
    for (qint32 i = 0; i < LZDIET_MAX_CHUNKS; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pEntry = pHeader + LZDIET_TABLE_OFFSET + (static_cast<qint64>(i) * LZDIET_ENTRY_SIZE);
        const qint64 nChunkOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pEntry));
        const qint64 nChunkSize = static_cast<qint64>(qFromLittleEndian<quint16>(pEntry + 4));

        if (nChunkOffset == -1) {
            bTerminated = true;
            break;
        }
        if (nChunkSize < LZDIET_CHUNK_PREAMBLE) return false;
        // Chunks are stored in ascending, non-overlapping table order.  They are
        // usually back to back, but a rewritten .PAD leaves small gaps (two of
        // the sixteen reference files do), so only the ordering may be enforced,
        // not exact tiling.
        if (nChunkOffset < nExpectedOffset) return false;
        if (!lzdietRangeWithin(context.nInputSize, nChunkOffset, nChunkSize)) return false;

        nExpectedOffset = nChunkOffset + nChunkSize;
        nEnd = nExpectedOffset;
        context.nNumberOfChunks++;
    }
    if (!bTerminated && (context.nNumberOfChunks != LZDIET_MAX_CHUNKS)) return false;
    if (context.nNumberOfChunks < 1) return false;

    context.nArchiveSize = nEnd;
    const QString sBaseName = XBinary::getDeviceFileBaseName(guardedSource);
    if (!guardedSource) return false;
    const QString sSuffix = XBinary::getDeviceFileCompleteSuffix(guardedSource);
    if (!guardedSource) return false;
    // The container stores no member name, so the payload keeps the container's
    // own file name - the .PAD data file it was made from - exactly as the
    // reference extractor names it.
    if (sBaseName.isEmpty()) {
        context.sFileName = QStringLiteral("lzdiet.bin");
    } else if (sSuffix.isEmpty()) {
        context.sFileName = sBaseName;
    } else {
        context.sFileName = sBaseName + QLatin1Char('.') + sSuffix;
    }

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XLZDIET::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XLZDIET::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZDIET archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLZDIET::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLZDIET(pDevice);
}

QList<QString> XLZDIET::getSearchSignatures()
{
    return {QStringLiteral("'lZdIeT'")};
}

XBinary::FT XLZDIET::getFileType()
{
    return FT_LZDIET;
}

XBinary::MODE XLZDIET::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLZDIET::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZDIET::getArch()
{
    return QString();
}

QString XLZDIET::getFileFormatExt()
{
    return QStringLiteral("pad");
}

QString XLZDIET::getFileFormatExtsString()
{
    return QStringLiteral("lZdIeT container (*.pad)");
}

QString XLZDIET::getMIMEString()
{
    return QStringLiteral("application/x-lzdiet");
}

QString XLZDIET::getVersion()
{
    return QString();
}

qint64 XLZDIET::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XLZDIET::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XLZDIET::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XLZDIET::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XLZDIET::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LZDIET_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        // The stream is the WHOLE container: the decoder needs the chunk table
        // that lives in the header, not just the chunk bytes.
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nArchiveSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZDIET);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("lZdIeT chunked LZW"));
        listResult.append(part);
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

    if ((nFileParts & FILEPART_OVERLAY) && (context.nInputSize > context.nArchiveSize) && canAppendPart(nLimit, listResult.size())) {
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

QList<XBinary::FPART_PROP> XLZDIET::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_STREAMOFFSET, FPART_PROP_STREAMSIZE};
}

QMap<XBinary::UNPACK_PROP, QVariant> XLZDIET::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLZDIET::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("lZdIeT container"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XLZDIET::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nArchiveSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nArchiveSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZDIET);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("lZdIeT chunked LZW"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XLZDIET::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // Advance FIRST, then report: with a single record the caller expects
    // 0 -> 1 plus a false return, not a refusal to move.
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = 0;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XLZDIET::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
