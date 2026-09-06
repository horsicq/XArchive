/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xnextstepdiskimage.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 NEXTSTEP_HEADER_SIZE = 0x2E;
const quint32 NEXTSTEP_SECTOR_SIZE = 512;
// The container was only ever used for removable media; a whole optical
// cartridge is still far below this, and the cap keeps a garbage geometry from
// asking for an absurd extent.
const qint64 NEXTSTEP_MAX_IMAGE_SIZE = qint64(1) << 32;
}  // namespace

XNextStepDiskImage::XNextStepDiskImage(QIODevice *pDevice) : XArchive(pDevice)
{
}

XNextStepDiskImage::~XNextStepDiskImage()
{
}

bool XNextStepDiskImage::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XNextStepDiskImage> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < NEXTSTEP_HEADER_SIZE + qint64(NEXTSTEP_SECTOR_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, NEXTSTEP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != NEXTSTEP_HEADER_SIZE)) return false;
    const uchar *pRaw = reinterpret_cast<const uchar *>(baHeader.constData());

    context.nVersion = qFromBigEndian<quint32>(pRaw);
    // Everything NeXT shipped carries 2 here; a plain sanity bound is enough,
    // the geometry cross-checks below carry the detection.
    if ((context.nVersion == 0) || (context.nVersion > 0x10)) return false;

    context.nBytesPerSector = qFromBigEndian<quint16>(pRaw + 0x04);
    if (context.nBytesPerSector != NEXTSTEP_SECTOR_SIZE) return false;
    if (qFromBigEndian<quint16>(pRaw + 0x22) != NEXTSTEP_SECTOR_SIZE) return false;
    if (qFromBigEndian<quint32>(pRaw + 0x1E) != NEXTSTEP_SECTOR_SIZE) return false;

    context.nCylinders = qFromBigEndian<quint32>(pRaw + 0x06);
    context.nHeads = qFromBigEndian<quint32>(pRaw + 0x0A);
    context.nSectorsPerTrack = qFromBigEndian<quint32>(pRaw + 0x24);
    context.nTotalSectors = qFromBigEndian<quint32>(pRaw + 0x2A);
    const quint32 nImageSize = qFromBigEndian<quint32>(pRaw + 0x16);
    const quint32 nImageBlock = qFromBigEndian<quint32>(pRaw + 0x1A);

    if ((context.nCylinders == 0) || (context.nCylinders > 0xFFFF)) return false;
    if ((context.nHeads == 0) || (context.nHeads > 0xFF)) return false;
    if ((context.nSectorsPerTrack == 0) || (context.nSectorsPerTrack > 0xFF)) return false;
    if (context.nTotalSectors == 0) return false;
    if (nImageBlock != 1) return false;

    // The three geometry fields have to multiply out to the sector count, and
    // the sector count times the sector size has to be the stored byte size.
    // That is the whole of the detection for a container with no magic.
    const qint64 nGeometrySectors = qint64(context.nCylinders) * qint64(context.nHeads) * qint64(context.nSectorsPerTrack);
    if (nGeometrySectors != qint64(context.nTotalSectors)) return false;

    const qint64 nComputedSize = qint64(context.nTotalSectors) * qint64(context.nBytesPerSector);
    if (nComputedSize != qint64(nImageSize)) return false;
    if ((nComputedSize <= 0) || (nComputedSize > NEXTSTEP_MAX_IMAGE_SIZE)) return false;

    context.nImageOffset = NEXTSTEP_HEADER_SIZE;
    context.nImageSize = nComputedSize;
    if (context.nImageSize > context.nInputSize - context.nImageOffset) return false;
    context.nArchiveSize = context.nImageOffset + context.nImageSize;

    context.sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (context.sFileName.isEmpty()) {
        context.sFileName = QStringLiteral("disk");
    }
    context.sFileName.append(QStringLiteral(".img"));

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XNextStepDiskImage::isValid(PDSTRUCT *pPdStruct)
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

bool XNextStepDiskImage::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNextStepDiskImage archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XNextStepDiskImage::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XNextStepDiskImage(pDevice);
}

XBinary::FT XNextStepDiskImage::getFileType()
{
    return FT_NEXTSTEP_DISKIMAGE;
}

XBinary::MODE XNextStepDiskImage::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XNextStepDiskImage::getEndian()
{
    return ENDIAN_BIG;
}

QString XNextStepDiskImage::getArch()
{
    return QString();
}

QString XNextStepDiskImage::getFileFormatExt()
{
    return QStringLiteral("diskimage");
}

QString XNextStepDiskImage::getFileFormatExtsString()
{
    return QStringLiteral("NeXTSTEP disk image (*.diskimage)");
}

QString XNextStepDiskImage::getMIMEString()
{
    return QStringLiteral("application/x-nextstep-diskimage");
}

QString XNextStepDiskImage::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XNextStepDiskImage::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XNextStepDiskImage::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XNextStepDiskImage::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XNextStepDiskImage::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XNextStepDiskImage::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = NEXTSTEP_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nImageOffset;
        part.nFileSize = context.nImageSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nImageSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nImageSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nImageOffset;
        part.nFileSize = context.nImageSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
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

QMap<XBinary::UNPACK_PROP, QVariant> XNextStepDiskImage::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XNextStepDiskImage::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XNextStepDiskImage> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("NeXTSTEP disk image"));
    pState->nCurrentOffset = pContext->nImageOffset;
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

XBinary::ARCHIVERECORD XNextStepDiskImage::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex != 0)) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nImageOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nImageOffset;
    result.nStreamSize = pContext->nImageSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nImageSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nImageSize);
    // The payload is the raw sector image; the container never compresses.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XNextStepDiskImage::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XNextStepDiskImage::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
