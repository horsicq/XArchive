/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlzv1.h"

#include "Algos/xlzv1decoder.h"
#include <new>

namespace {
const qint64 LZV1_HEADER_SIZE = 12;
// Header plus at least one root code byte.
const qint64 LZV1_MIN_FILE_SIZE = 13;
}  // namespace

XLZV1::XLZV1(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZV1::~XLZV1()
{
}

bool XLZV1::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < LZV1_MIN_FILE_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, LZV1_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != LZV1_HEADER_SIZE)) return false;

    // Ten fixed bytes plus a ranged dictionary ceiling; the same check the
    // decoder repeats before it touches the stream.
    if (!XLZV1Decoder::checkHeader(baHeader.constData(), baHeader.size(), &context.nMaxCodes)) return false;

    // The stream is defined to end at EOF, so the whole file is the member and
    // no overlay can be measured without decoding.
    context.nStreamSize = context.nInputSize;

    const QString sBaseName = XBinary::getDeviceFileBaseName(guardedSource);
    if (!guardedSource) return false;
    const QString sSuffix = XBinary::getDeviceFileCompleteSuffix(guardedSource);
    if (!guardedSource) return false;
    // The container carries no member name; the reference extractor falls back
    // to the container's own file name and so does this class.
    if (sBaseName.isEmpty()) {
        context.sFileName = QStringLiteral("lzv1.bin");
    } else if (sSuffix.isEmpty()) {
        context.sFileName = sBaseName;
    } else {
        context.sFileName = sBaseName + QLatin1Char('.') + sSuffix;
    }

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XLZV1::isValid(PDSTRUCT *pPdStruct)
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

bool XLZV1::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZV1 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLZV1::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLZV1(pDevice);
}

QList<QString> XLZV1::getSearchSignatures()
{
    // 'LZV1' plus the six constant bytes that follow it.
    return {QStringLiteral("'LZV1'5D1901AD0000")};
}

XBinary::FT XLZV1::getFileType()
{
    return FT_LZV1;
}

XBinary::MODE XLZV1::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLZV1::getEndian()
{
    // The one multi-byte header field (nMaxCodes) is big endian.
    return ENDIAN_BIG;
}

QString XLZV1::getArch()
{
    return QString();
}

QString XLZV1::getFileFormatExt()
{
    return QStringLiteral("lzv");
}

QString XLZV1::getFileFormatExtsString()
{
    return QStringLiteral("LZV1 compressed file (*.lzv)");
}

QString XLZV1::getMIMEString()
{
    return QStringLiteral("application/x-lzv1");
}

QString XLZV1::getVersion()
{
    return QString();
}

qint64 XLZV1::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nStreamSize : 0;
}

QList<XBinary::MAPMODE> XLZV1::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XLZV1::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XLZV1::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XLZV1::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LZV1_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        // The header is part of the stream: nMaxCodes is a decoder parameter.
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = 0;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        // No FPART_PROP_UNCOMPRESSEDSIZE: see the class comment.
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZV1);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZV1 LZW"));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QList<XBinary::FPART_PROP> XLZV1::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD, FPART_PROP_STREAMOFFSET,
            FPART_PROP_STREAMSIZE};
}

QMap<XBinary::UNPACK_PROP, QVariant> XLZV1::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLZV1::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("LZV1 compressed file"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nStreamSize;
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

XBinary::ARCHIVERECORD XLZV1::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    // FPART_PROP_UNCOMPRESSEDSIZE is deliberately absent: the container does not
    // record it, and XArchive::unpackCurrent already stages a record with no
    // declared size and publishes it only after the decoder reports success.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZV1);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZV1 LZW"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XLZV1::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nStreamSize;
    return false;
}

bool XLZV1::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
