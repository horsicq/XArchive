/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhfe.h"

#include <QPointer>

#include <new>

namespace {
// The whole container has to be in memory to decode it (the track LUT points
// all over the file), so cap what will be accepted.
const qint64 HFE_MAX_FILE_SIZE = 0x8000000;  // 128 MB
const qint64 HFE_MIN_FILE_SIZE = 1024;
}  // namespace

XHFE::XHFE(QIODevice *pDevice) : XArchive(pDevice)
{
}

XHFE::~XHFE()
{
}

bool XHFE::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XHFE> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < HFE_MIN_FILE_SIZE) ||
        (context.nInputSize > HFE_MAX_FILE_SIZE)) {
        return false;
    }

    const QByteArray baFile =
        read_array_process(0, qint32(context.nInputSize), pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baFile.size() != qint32(context.nInputSize))) {
        return false;
    }

    // A bounded trial decode of cylinder 0: the magic alone is not enough to
    // promise a readable image, and it is what fixes the geometry (and hence
    // the size) of the member this class publishes.
    if (!XHFEDecoder::probeGeometry(baFile, &context.geometry, pPdStruct)) {
        return false;
    }
    if (!guardedThis || !guardedSource) return false;

    QString sBaseName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (sBaseName.isEmpty()) sBaseName = QStringLiteral("hfe_disk");
    context.sImageName = sBaseName + QStringLiteral(".img");

    *pContext = context;
    return true;
}

bool XHFE::isValid(PDSTRUCT *pPdStruct)
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

bool XHFE::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHFE archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHFE::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHFE(pDevice);
}

QList<QString> XHFE::getSearchSignatures()
{
    return {QStringLiteral("'HXCPICFE'00")};
}

XBinary::FT XHFE::getFileType()
{
    return FT_HFE;
}

XBinary::MODE XHFE::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XHFE::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XHFE::getArch()
{
    return QString();
}

QString XHFE::getFileFormatExt()
{
    return QStringLiteral("hfe");
}

QString XHFE::getFileFormatExtsString()
{
    return QStringLiteral("HxC Floppy Emulator image (*.hfe)");
}

QString XHFE::getMIMEString()
{
    return QStringLiteral("application/x-hfe");
}

QString XHFE::getVersion()
{
    // Byte +0x08 is the format revision and is 0 in every v1 image.
    return QStringLiteral("1");
}

qint64 XHFE::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XHFE::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XHFE::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART> XHFE::getFileParts(quint32 nFileParts, qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if (nFileParts & FILEPART_HEADER) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = 24;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        // The member's source is the whole container: the track LUT scatters
        // the flux across the file, so there is no contiguous range to point
        // at.
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sImageName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nInputSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.geometry.nImageSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_HFE);
        part.mapProperties.insert(
            FPART_PROP_REPORTEDMETHOD,
            QStringLiteral("MFM %1/%2/%3")
                .arg(context.geometry.nTracks)
                .arg(context.geometry.nSides)
                .arg(context.geometry.nSectorsPerTrack));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XHFE::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XHFE::initUnpack(UNPACK_STATE *pState,
                      const QMap<UNPACK_PROP, QVariant> &mapProperties,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XHFE> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("HxC Floppy Emulator image; MFM flux decoded to a sector image"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XHFE::infoCurrent(UNPACK_STATE *pState,
                                         PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nInputSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sImageName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nInputSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->geometry.nImageSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_HFE);
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        QStringLiteral("MFM %1/%2/%3")
            .arg(pContext->geometry.nTracks)
            .arg(pContext->geometry.nSides)
            .arg(pContext->geometry.nSectorsPerTrack));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The per-sector CRC-16s are verified inside the codec; the container has
    // no checksum of its own.
    return result;
}

bool XHFE::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nInputSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XHFE::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
