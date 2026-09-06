/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlofi.h"

#include "Algos/xlofidecoder.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <limits>
#include <new>

namespace {
const qint64 LOFI_INDEX_OFFSET = 0x30;
const qint64 LOFI_MAX_INDEX_ENTRIES = 64 * 1024 * 1024;
}  // namespace

XLOFI::XLOFI(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLOFI::~XLOFI()
{
}

bool XLOFI::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XLOFI> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < LOFI_INDEX_OFFSET + 16) return false;

    // Read the fixed part first so the index length is known before the index
    // itself is pulled in; a bogus entry count must not turn into a huge read.
    const QByteArray baFixed = read_array_process(0, LOFI_INDEX_OFFSET, pPdStruct);
    if (!guardedThis || !guardedSource || (baFixed.size() != LOFI_INDEX_OFFSET)) return false;

    XLOFIDecoder::GEOMETRY probe = {};
    {
        const uchar *pFixed = reinterpret_cast<const uchar *>(baFixed.constData());
        const qint64 nEntries = static_cast<qint64>(qFromBigEndian<quint32>(pFixed + 0x28));
        if ((nEntries <= 1) || (nEntries > LOFI_MAX_INDEX_ENTRIES)) return false;
        const qint64 nHeaderSize = LOFI_INDEX_OFFSET + nEntries * 8;
        if (nHeaderSize > context.nInputSize) return false;
        const QByteArray baHeader = read_array_process(0, nHeaderSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baHeader.size() != nHeaderSize)) return false;
        // Every other rule - the algorithm name, the geometry bounds and the
        // ascending index - lives in the decoder so the two cannot disagree.
        if (!XLOFIDecoder::parseGeometry(baHeader, context.nInputSize, &probe, nullptr)) return false;
    }

    context.nSegmentSize = probe.nSegmentSize;
    context.nSegmentCount = probe.nIndexEntries - 1;
    context.nDataOffset = probe.nDataOffset;
    context.nDataSize = probe.nDataSize;
    context.nImageSize = probe.nImageSize;
    context.nArchiveSize = probe.nDataOffset + probe.nDataSize;
    // The whole-buffer decoder needs the image to fit a QByteArray.
    if (context.nImageSize > qint64((std::numeric_limits<qint32>::max)())) return false;

    context.sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (context.sFileName.isEmpty()) context.sFileName = QStringLiteral("lofi_image");
    context.sFileName += QStringLiteral(".img");

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XLOFI::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XLOFI::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLOFI archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLOFI::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLOFI(pDevice);
}

QList<QString> XLOFI::getSearchSignatures()
{
    // "lzma" plus the first eight bytes of the 32-byte zero pad.
    return {QStringLiteral("'lzma'0000000000000000")};
}

XBinary::FT XLOFI::getFileType()
{
    return FT_LOFI;
}

XBinary::MODE XLOFI::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLOFI::getEndian()
{
    return ENDIAN_BIG;
}

QString XLOFI::getArch()
{
    return QString();
}

QString XLOFI::getFileFormatExt()
{
    return QStringLiteral("lofi");
}

QString XLOFI::getFileFormatExtsString()
{
    return QStringLiteral("Solaris compressed lofi image (*.lofi *.zlib)");
}

QString XLOFI::getMIMEString()
{
    return QStringLiteral("application/x-solaris-lofi");
}

QString XLOFI::getVersion()
{
    return QStringLiteral("lzma");
}

qint64 XLOFI::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XLOFI::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XLOFI::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XLOFI::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XLOFI::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDataOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        // The decoder re-reads the header and the index, so the stream starts at
        // offset 0 of the container rather than at the first segment.
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nArchiveSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nImageSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LOFI);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZMA (segmented)"));
        part.mapProperties.insert(FPART_PROP_WINDOWSIZE, context.nSegmentSize);
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nDataSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Segments");
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
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLOFI::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLOFI::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XLOFI> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Solaris lofi image"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nArchiveSize;
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

XBinary::ARCHIVERECORD XLOFI::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != 0) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nArchiveSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nArchiveSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nImageSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LOFI);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZMA (segmented)"));
    result.mapProperties.insert(FPART_PROP_WINDOWSIZE, pContext->nSegmentSize);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XLOFI::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nArchiveSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XLOFI::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
