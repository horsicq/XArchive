/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Archive adapter for the bounded legacy-codec decoder.
 */
#include "xdearkarchive.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QTemporaryDir>

#include <limits>
#include <new>

#include "Algos/xdearkdecoder.h"
#include "xzip.h"

namespace {

const qint64 DEARK_DEFAULT_MAX_ENTRY_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint64 DEARK_DEFAULT_MAX_TOTAL_SIZE = Q_INT64_C(2) * 1024 * 1024 * 1024;
const qint64 DEARK_DEFAULT_MAX_ENTRY_COUNT = 100000;
const qint64 DEARK_HARD_MAX_ENTRY_COUNT = 250000;

QMutex &dearkMutex()
{
    static QMutex mutex;
    return mutex;
}

qint64 boundedDearkLimit(qint64 configured, qint64 fallback, qint64 hardMaximum)
{
    const qint64 value = configured < 0 ? fallback : configured;
    return qMax<qint64>(1, qMin(value, hardMaximum));
}

}  // namespace

XDearkArchive::DEARK_UNPACK_CONTEXT::DEARK_UNPACK_CONTEXT()
    : pTemporaryDir(nullptr), pZipFile(nullptr), pInnerArchive(nullptr), innerState(), sModule()
{
}

XDearkArchive::DEARK_UNPACK_CONTEXT::~DEARK_UNPACK_CONTEXT()
{
    if (pInnerArchive) {
        pInnerArchive->finishUnpack(&innerState, nullptr);
        delete pInnerArchive;
        pInnerArchive = nullptr;
    }
    if (pZipFile) {
        pZipFile->close();
        delete pZipFile;
        pZipFile = nullptr;
    }
    delete pTemporaryDir;
    pTemporaryDir = nullptr;
}

XDearkArchive::XDearkArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDearkArchive::~XDearkArchive()
{
}

bool XDearkArchive::isValid(PDSTRUCT *pPdStruct)
{
    UNPACK_STATE state = {};
    const bool bResult = initUnpack(&state, QMap<UNPACK_PROP, QVariant>(), pPdStruct);
    const bool bFinished = finishUnpack(&state, nullptr);
    return bResult && bFinished;
}

bool XDearkArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDearkArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XDearkArchive::getFileType()
{
    return FT_DEARK_LEGACY_ARCHIVE;
}

XBinary::MODE XDearkArchive::getMode()
{
    return MODE_DATA;
}

QString XDearkArchive::getMIMEString()
{
    return QStringLiteral("application/x-deark-legacy-archive");
}

QString XDearkArchive::getFileFormatExt()
{
    return QStringLiteral("archive");
}

QString XDearkArchive::getFileFormatExtsString()
{
    return QStringLiteral("Legacy archive or compressed stream");
}

QList<QString> XDearkArchive::getSearchSignatures()
{
    return QList<QString>() << "0D0A1A'CAZIP'" << "'EDILZSS'" << "'MRNZ'88F02733" << "'Stirling'" << "'SIT!'" << "'StuffIt'";
}

XBinary *XDearkArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDearkArchive(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XDearkArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XDearkArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XDearkArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct) || !guardedThis || !guardedSource) return false;

    DEARK_UNPACK_CONTEXT *pContext = new (std::nothrow) DEARK_UNPACK_CONTEXT();
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    bool bResult = false;
    const qint64 nSourceSize = guardedSource->size();
    OUTPUT_POLICY policy = {};
    if ((nSourceSize <= 0) || !XBinary::resolveUnpackOutputPolicy(mapProperties, &policy)) goto init_failed;

    pContext->pTemporaryDir = new (std::nothrow) QTemporaryDir(QDir::tempPath() + QStringLiteral("/xdeark-XXXXXX"));
    if (!pContext->pTemporaryDir || !pContext->pTemporaryDir->isValid()) goto init_failed;

    {
        QFile inputFile(QDir(pContext->pTemporaryDir->path()).filePath(QStringLiteral("input.bin")));
        if (!inputFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) goto init_failed;
        bResult = XBinary::copyDeviceMemory(guardedSource.data(), 0, &inputFile, 0, nSourceSize, pPdStruct);
        if (!guardedThis || !guardedSource || !bResult || (inputFile.size() != nSourceSize)) goto init_failed;
        inputFile.close();
    }

    {
        const QString inputPath = QDir(pContext->pTemporaryDir->path()).filePath(QStringLiteral("input.bin"));
        const QString outputPath = QDir(pContext->pTemporaryDir->path()).filePath(QStringLiteral("output.zip"));
        const qint64 maxFiles = boundedDearkLimit(policy.nMaxEntryCount, DEARK_DEFAULT_MAX_ENTRY_COUNT, DEARK_HARD_MAX_ENTRY_COUNT);
        const qint64 maxFileSize = boundedDearkLimit(policy.nMaxEntryOutputSize, DEARK_DEFAULT_MAX_ENTRY_SIZE, (std::numeric_limits<qint64>::max)());
        const qint64 maxTotalSize = boundedDearkLimit(policy.nMaxTotalOutputSize, DEARK_DEFAULT_MAX_TOTAL_SIZE, (std::numeric_limits<qint64>::max)());
        XDearkDecoder::LIMITS limits;
        limits.maxFiles = maxFiles;
        limits.maxFileSize = maxFileSize;
        limits.maxTotalSize = maxTotalSize;
        XDearkDecoder::RESULT decoderResult;
        {
            QMutexLocker locker(&dearkMutex());
            bResult = XDearkDecoder::extractToZip(inputPath, outputPath,
                                                  limits, &decoderResult);
        }
        pContext->sModule = decoderResult.module;
        if (!guardedThis || !guardedSource || !bResult ||
            !XDearkDecoder::isSupportedModule(pContext->sModule)) {
            if (!decoderResult.errorMessage.isEmpty()) XBinary::setPdStructErrorString(pPdStruct, decoderResult.errorMessage);
            goto init_failed;
        }

        const QFileInfo outputInfo(outputPath);
        const QString canonicalRoot = QFileInfo(pContext->pTemporaryDir->path()).canonicalFilePath();
        const QString canonicalOutput = outputInfo.canonicalFilePath();
        if (!outputInfo.exists() || !outputInfo.isFile() || outputInfo.isSymLink() || outputInfo.size() <= 0 || canonicalRoot.isEmpty() || canonicalOutput.isEmpty() ||
            !canonicalOutput.startsWith(canonicalRoot + QLatin1Char('/'))) {
            goto init_failed;
        }
        pContext->pZipFile = new (std::nothrow) QFile(canonicalOutput);
        if (!pContext->pZipFile || !pContext->pZipFile->open(QIODevice::ReadOnly)) goto init_failed;
    }

    pContext->pInnerArchive = new (std::nothrow) XZip(pContext->pZipFile);
    if (!pContext->pInnerArchive || !pContext->pInnerArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct) || !guardedThis || !guardedSource ||
        (pContext->innerState.nNumberOfRecords <= 0) || (pContext->innerState.nCurrentIndex != 0)) {
        goto init_failed;
    }

    pState->mapUnpackProperties = pContext->innerState.mapUnpackProperties;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, QStringLiteral("Deark module: %1").arg(pContext->sModule));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = nSourceSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->pContext = pContext;
    bResult = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bResult) goto init_failed;
    return true;

init_failed:
    if (guardedThis) guardedThis->releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return false;
}

XBinary::ARCHIVERECORD XDearkArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XDearkArchive> guardedThis(this);
    if (!operationGuard.isAllowed() || !pState || !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return ARCHIVERECORD();
    DEARK_UNPACK_CONTEXT *pContext = static_cast<DEARK_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pInnerArchive || !pContext->pZipFile ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) return ARCHIVERECORD();
    ARCHIVERECORD record = pContext->pInnerArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) || record.mapProperties.isEmpty() ||
        !markArchiveStreamRecord(&record, pState->nCurrentIndex)) return ARCHIVERECORD();
    return record;
}

bool XDearkArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XDearkArchive> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState || !pDevice || !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) || devicesAlias(getDevice(), pDevice)) return false;
    DEARK_UNPACK_CONTEXT *pContext = static_cast<DEARK_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pInnerArchive || !pContext->pZipFile ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) return false;
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    const bool bResult = pContext->pInnerArchive->unpackCurrent(&pContext->innerState, pDevice, pPdStruct);
    if (!guardedThis || !bResult || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex)) return false;
    pState->nCurrentOffset = 0;
    return true;
}

bool XDearkArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XDearkArchive> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState || !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    DEARK_UNPACK_CONTEXT *pContext = static_cast<DEARK_UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pContext->pInnerArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;
    const qint32 previous = pState->nCurrentIndex;
    const bool bResult = pContext->pInnerArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords)) return false;
    if (bResult) {
        if ((pContext->innerState.nCurrentIndex != previous + 1) || (pContext->innerState.nCurrentIndex >= pState->nNumberOfRecords)) return false;
        pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
        pState->nCurrentOffset = 0;
        return true;
    }
    if ((previous + 1) != pState->nNumberOfRecords) return false;
    pState->nCurrentIndex = pState->nNumberOfRecords;
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XDearkArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    DEARK_UNPACK_CONTEXT *pContext = static_cast<DEARK_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
