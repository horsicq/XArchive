/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native archive reader for IMD and Compaq QRST sector images.
 */
#include "xlibdskarchive.h"

#include <QPointer>

#include <limits>
#include <memory>
#include <new>

#include "Algos/xlegacydiskdecoder.h"

namespace {
const qint64 DEFAULT_MAX_DISK_SIZE = Q_INT64_C(2) * 1024 * 1024 * 1024;
}  // namespace

XLibDskArchive::XLibDskArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLibDskArchive::~XLibDskArchive()
{
}

QString XLibDskArchive::identifyDriver(QIODevice *pDevice)
{
    if (!pDevice || !pDevice->isOpen() || !pDevice->isReadable() ||
        pDevice->isSequential() || pDevice->size() < 16)
        return QString();
    const qint64 position = pDevice->pos();
    if (!pDevice->seek(0)) return QString();
    const QByteArray header = pDevice->read(1024);
    pDevice->seek(position);

    return XLegacyDiskDecoder::identify(header);
}

bool XLibDskArchive::isValid(PDSTRUCT *pPdStruct)
{
    UNPACK_STATE state = {};
    const bool result = initUnpack(&state, QMap<UNPACK_PROP, QVariant>(), pPdStruct);
    const bool finished = finishUnpack(&state, nullptr);
    return result && finished;
}

bool XLibDskArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLibDskArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XLibDskArchive::getFileType()
{
    return FT_LIBDSK_IMAGE;
}

XBinary::MODE XLibDskArchive::getMode()
{
    return MODE_DATA;
}

QString XLibDskArchive::getMIMEString()
{
    return QStringLiteral("application/x-legacy-disk-image");
}

QString XLibDskArchive::getFileFormatExt()
{
    return QStringLiteral("img");
}

QString XLibDskArchive::getFileFormatExtsString()
{
    return QStringLiteral("ImageDisk or Compaq QRST disk image (*.imd;*.qrst)");
}

QList<QString> XLibDskArchive::getSearchSignatures()
{
    return {QStringLiteral("'IMD '"), QStringLiteral("'QRST'")};
}

XBinary *XLibDskArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                        XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLibDskArchive(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XLibDskArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLibDskArchive::initUnpack(UNPACK_STATE *pState,
                                const QMap<UNPACK_PROP, QVariant> &mapProperties,
                                PDSTRUCT *pPdStruct)
{
    QPointer<XLibDskArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress)
        return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct))
        return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct))
        return false;

    CONTEXT *context = new (std::nothrow) CONTEXT;
    bool result = false;
    if (!context) goto failed;
    context->sDriver = identifyDriver(guardedSource.data());
    if (context->sDriver.isEmpty()) goto failed;

    {
        OUTPUT_POLICY policy = {};
        if (!resolveUnpackOutputPolicy(mapProperties, &policy)) goto failed;
        qint64 maxOutput = policy.nMaxEntryOutputSize < 0
                               ? DEFAULT_MAX_DISK_SIZE
                               : qMin(policy.nMaxEntryOutputSize,
                                      DEFAULT_MAX_DISK_SIZE);
        if (policy.nMaxTotalOutputSize >= 0)
            maxOutput = qMin(maxOutput, policy.nMaxTotalOutputSize);
        if (maxOutput <= 0) goto failed;

        const qint64 sourceSize = guardedSource->size();
        if (sourceSize < 1 || sourceSize > (std::numeric_limits<int>::max)())
            goto failed;
        const QByteArray source = read_array_process(0, sourceSize, pPdStruct);
        if (source.size() != sourceSize || !guardedThis || !guardedSource)
            goto failed;

        XLegacyDiskDecoder::RESULT decodeResult;
        QString decodeError;
        result = XLegacyDiskDecoder::decode(source, maxOutput, &decodeResult,
                                             &decodeError);
        if (!result || decodeResult.rawImage.isEmpty() ||
            decodeResult.rawImage.size() > maxOutput) {
            if (!decodeError.isEmpty())
                setPdStructErrorString(pPdStruct, decodeError);
            goto failed;
        }
        context->rawImage = decodeResult.rawImage;
        context->nRawSize = context->rawImage.size();
        context->sDriver = decodeResult.driver;
        context->sInfo = QStringLiteral(
            "%1: %2 cylinders, %3 heads, %4 sectors/track, %5 bytes/sector")
            .arg(context->sDriver.toUpper())
            .arg(decodeResult.cylinders)
            .arg(decodeResult.heads)
            .arg(decodeResult.sectorsPerTrack)
            .arg(decodeResult.sectorSize);
        if (decodeResult.recoveredSectors > 0)
            context->sInfo += QStringLiteral("; %1 damaged or missing sectors recovered")
                                  .arg(decodeResult.recoveredSectors);
        QString base = fixFileName(getDeviceFileBaseName(guardedSource.data()));
        if (base.isEmpty()) base = QStringLiteral("disk-image");
        context->sName = base + QStringLiteral(".img");
        pState->mapUnpackProperties = mapProperties;
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, context->sInfo);
        pState->nCurrentOffset = 0;
        pState->nTotalSize = sourceSize;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;
        pState->pContext = context;
    }
    if (!validateAndFinalizeUnpackSource(pState, context, pPdStruct)) goto failed;
    return true;

failed:
    if (guardedThis) releaseUnpackSource(pState);
    delete context;
    *pState = UNPACK_STATE();
    return false;
}

XBinary::ARCHIVERECORD XLibDskArchive::infoCurrent(UNPACK_STATE *pState,
                                                   PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || pState->nCurrentIndex != 0 ||
        pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return ARCHIVERECORD();
    CONTEXT *context = static_cast<CONTEXT *>(pState->pContext);
    if (!context || context->rawImage.size() != context->nRawSize ||
        context->nRawSize <= 0)
        return ARCHIVERECORD();
    ARCHIVERECORD record = {};
    record.nStreamOffset = 0;
    record.nStreamSize = pState->nTotalSize;
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context->sName);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pState->nTotalSize);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context->nRawSize);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                context->sDriver.toUpper());
    if (!markArchiveStreamRecord(&record, 0)) return ARCHIVERECORD();
    return record;
}

bool XLibDskArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                   PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XLibDskArchive> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!operationGuard.isAcquired() || !pState || !guardedOutput ||
        pState->nCurrentIndex != 0 || pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        devicesAlias(getDevice(), guardedOutput.data()))
        return false;
    CONTEXT *context = static_cast<CONTEXT *>(pState->pContext);
    if (!context || context->rawImage.size() != context->nRawSize ||
        context->nRawSize <= 0 ||
        !isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   context->nRawSize))
        return false;
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(0, context->sName) &&
            pState->spOutputBudget->isEnforcing())
            return false;
        if (!pState->spOutputBudget->debit(context->nRawSize) &&
            pState->spOutputBudget->isEnforcing())
            return false;
    }

    std::unique_ptr<QIODevice> stage(createFileBuffer(context->nRawSize,
                                                       pPdStruct));
    if (!stage || stage->write(context->rawImage) != context->nRawSize ||
        !stage->seek(0) || !guardedThis || !guardedOutput ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    const bool result = publishUnpackOutput(stage.get(), guardedOutput.data(),
                                            pState, pPdStruct);
    if (result && guardedThis) pState->nCurrentOffset = context->nRawSize;
    return result && guardedThis;
}

bool XLibDskArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        pState->nCurrentIndex != 0 || pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;
    pState->nCurrentIndex = 1;
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XLibDskArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;
    CONTEXT *context = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete context;
    *pState = UNPACK_STATE();
    return true;
}
