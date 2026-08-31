/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xancient.h"

#include "Algos/xancientdecoder.h"

#include <QPointer>

#include <limits>
#include <memory>
#include <new>

namespace {
bool addWithin(qint64 nLeft, qint64 nRight, qint64 nLimit)
{
    return (nLeft >= 0) && (nRight >= 0) && (nLeft <= nLimit) &&
           (nRight <= (nLimit - nLeft));
}

XAncientDecoder::TYPE decoderType(XBinary::FT fileType)
{
    switch (fileType) {
        case XBinary::FT_DMS: return XAncientDecoder::TYPE_DMS;
        case XBinary::FT_PP20: return XAncientDecoder::TYPE_POWERPACKER;
        case XBinary::FT_RNC: return XAncientDecoder::TYPE_RNC;
        case XBinary::FT_TPWM: return XAncientDecoder::TYPE_TPWM;
        case XBinary::FT_FREEZE: return XAncientDecoder::TYPE_FREEZE;
        case XBinary::FT_UNIX_PACK: return XAncientDecoder::TYPE_UNIX_PACK;
        default: return XAncientDecoder::TYPE_UNKNOWN;
    }
}

XBinary::FT archiveType(XAncientDecoder::TYPE type)
{
    switch (type) {
        case XAncientDecoder::TYPE_DMS: return XBinary::FT_DMS;
        case XAncientDecoder::TYPE_POWERPACKER: return XBinary::FT_PP20;
        case XAncientDecoder::TYPE_RNC: return XBinary::FT_RNC;
        case XAncientDecoder::TYPE_TPWM: return XBinary::FT_TPWM;
        case XAncientDecoder::TYPE_FREEZE: return XBinary::FT_FREEZE;
        case XAncientDecoder::TYPE_UNIX_PACK: return XBinary::FT_UNIX_PACK;
        default: return XBinary::FT_UNKNOWN;
    }
}
}  // namespace

XAncient::XAncient(QIODevice *pDevice, FT fileTypeHint)
    : XArchive(pDevice), m_fileTypeHint(fileTypeHint)
{
}

XBinary::FT XAncient::candidateFileType(const QByteArray &baHeader)
{
    return archiveType(XAncientDecoder::identify(baHeader));
}

XBinary::FT XAncient::fileTypeForMethod(const QString &sMethod)
{
    if (sMethod.startsWith(QStringLiteral("DMS:"))) return FT_DMS;
    if (sMethod.startsWith(QStringLiteral("RNC1:")) ||
        sMethod.startsWith(QStringLiteral("RNC2:"))) return FT_RNC;
    if (sMethod.startsWith(QStringLiteral("TPWM:"))) return FT_TPWM;
    if (sMethod.startsWith(QStringLiteral("PP:"))) return FT_PP20;
    if (sMethod.startsWith(QStringLiteral("F:"))) return FT_FREEZE;
    if (sMethod.startsWith(QStringLiteral("z:"))) return FT_UNIX_PACK;
    return FT_UNKNOWN;
}

bool XAncient::describe(const QByteArray &baPacked, FT fileTypeHint,
                        STREAM_INFO *pInfo)
{
    if (!pInfo || (baPacked.size() < 2) ||
        (baPacked.size() > XAncientDecoder::MAX_PACKED_SIZE))
        return false;

    // Bullfrog titles prefix an otherwise ordinary RNC1 stream with the
    // eight-byte publisher tag "BULLFROG".  Keep that carrier detail in the
    // archive layer: the shared RNC decoder continues to receive a canonical
    // stream and therefore needs no relaxed magic or offset-dependent paths.
    qint64 nStreamOffset = 0;
    QByteArray baDecoderInput = baPacked;
    if (baPacked.size() >= 12 && baPacked.startsWith("BULLFROG") &&
        baPacked.mid(8, 4) == QByteArray("RNC\x01", 4)) {
        nStreamOffset = 8;
        baDecoderInput = baPacked.mid(8);
    }

    const FT candidate = candidateFileType(baDecoderInput.left(4));
    if ((candidate == FT_UNKNOWN) ||
        ((fileTypeHint != FT_UNKNOWN) && (candidate != fileTypeHint)))
        return false;

    XAncientDecoder::INFO decoderInfo;
    if (XAncientDecoder::describe(baDecoderInput, decoderType(candidate),
                                  &decoderInfo)) {
        STREAM_INFO info;
        info.sMethod = decoderInfo.method;
        info.fileType = fileTypeForMethod(info.sMethod);
        if ((info.fileType == FT_UNKNOWN) || (info.fileType != candidate) ||
            ((fileTypeHint != FT_UNKNOWN) && (info.fileType != fileTypeHint)))
            return false;

        info.nPackedSize = decoderInfo.packedSize > 0
                               ? decoderInfo.packedSize
                               : baPacked.size();
        info.nRawSize = decoderInfo.rawSize;
        info.nImageSize = decoderInfo.imageSize;
        info.nImageOffset = decoderInfo.imageOffset;
        info.nStreamOffset = nStreamOffset;

        const qint64 nMaxPacked = XAncientDecoder::MAX_PACKED_SIZE;
        const qint64 nMaxRaw = XAncientDecoder::MAX_RAW_SIZE;
        if ((info.nPackedSize <= 0) ||
            (info.nPackedSize > baDecoderInput.size()) ||
            (info.nPackedSize > nMaxPacked) || (info.nRawSize > nMaxRaw) ||
            (info.nImageSize > nMaxRaw) || (info.nImageOffset < 0) ||
            ((info.nRawSize >= 0) && (info.nImageSize >= 0) &&
             !addWithin(info.nImageOffset, info.nRawSize, info.nImageSize)))
            return false;

        *pInfo = info;
        return true;
    }
    return false;
}

bool XAncient::readPackedData(QByteArray *pData, PDSTRUCT *pPdStruct)
{
    if (!pData || !isPdStructNotCanceled(pPdStruct)) return false;
    pData->clear();
    QPointer<XAncient> guardedThis(this);
    const qint64 nSize = getSize();
    if (!guardedThis || (nSize < 2) ||
        (nSize > XAncientDecoder::MAX_PACKED_SIZE) ||
        (nSize > (std::numeric_limits<int>::max)()))
        return false;
    *pData = read_array_process(0, nSize, pPdStruct);
    return guardedThis && (pData->size() == nSize) &&
           isPdStructNotCanceled(pPdStruct);
}

bool XAncient::isValid(QIODevice *pDevice, FT fileTypeHint, PDSTRUCT *pPdStruct)
{
    XAncient archive(pDevice, fileTypeHint);
    return archive.isValid(pPdStruct);
}

bool XAncient::isValid(PDSTRUCT *pPdStruct)
{
    QByteArray baPacked;
    STREAM_INFO info;
    return readPackedData(&baPacked, pPdStruct) &&
           describe(baPacked, m_fileTypeHint, &info) &&
           isPdStructNotCanceled(pPdStruct);
}

XBinary::FT XAncient::detectFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice || !isPdStructNotCanceled(pPdStruct)) return FT_UNKNOWN;
    XAncient archive(pDevice);
    QByteArray baPacked;
    STREAM_INFO info;
    return archive.readPackedData(&baPacked, pPdStruct) &&
                   describe(baPacked, FT_UNKNOWN, &info)
               ? info.fileType
               : FT_UNKNOWN;
}

XBinary::FT XAncient::getFileType()
{
    return (m_fileTypeHint != FT_UNKNOWN)
               ? m_fileTypeHint
               : detectFileType(getDevice(), nullptr);
}

XBinary::MODE XAncient::getMode()
{
    return MODE_DATA;
}

qint32 XAncient::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XAncient::getEndian()
{
    return ENDIAN_BIG;
}

QString XAncient::getFileFormatExt()
{
    switch (getFileType()) {
        case FT_DMS: return QStringLiteral("dms");
        case FT_PP20: return QStringLiteral("pp20");
        case FT_RNC: return QStringLiteral("rnc");
        case FT_TPWM: return QStringLiteral("tpwm");
        case FT_FREEZE: return QStringLiteral("f");
        case FT_UNIX_PACK: return QStringLiteral("z");
        default: return QString();
    }
}

QString XAncient::getFileFormatExtsString()
{
    switch (getFileType()) {
        case FT_DMS: return QStringLiteral("Disk Masher System (*.dms)");
        case FT_PP20: return QStringLiteral("PowerPacker (*.pp;*.pp20;*.px20)");
        case FT_RNC: return QStringLiteral("Rob Northen compression (*.rnc)");
        case FT_TPWM: return QStringLiteral("Turbo Packer (*.tpwm)");
        case FT_FREEZE: return QStringLiteral("Freeze/Melt (*.f;*.freeze)");
        case FT_UNIX_PACK: return QStringLiteral("Unix Pack (*.z)");
        default: return QString();
    }
}

QString XAncient::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

qint64 XAncient::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return isValid(pPdStruct) ? getSize() : 0;
}

XBinary::OSNAME XAncient::getOsName()
{
    switch (getFileType()) {
        case FT_DMS:
        case FT_PP20:
        case FT_TPWM: return OSNAME_AMIGA;
        default: return OSNAME_MULTIPLATFORM;
    }
}

QString XAncient::getVersion()
{
    return QString();
}

QList<QString> XAncient::getSearchSignatures()
{
    switch (getFileType()) {
        case FT_DMS: return {QStringLiteral("'DMS!'")};
        case FT_PP20:
            return {QStringLiteral("'PP11'"), QStringLiteral("'PP20'"),
                    QStringLiteral("'PX20'"), QStringLiteral("'CHFC'"),
                    QStringLiteral("'DEN!'"), QStringLiteral("'DXS9'"),
                    QStringLiteral("'H.D.'"), QStringLiteral("'RVV!'")};
        case FT_RNC:
            return {QStringLiteral("'RNC'01"), QStringLiteral("'RNC'02"),
                    QStringLiteral("'...'01"),
                    QStringLiteral("'BULLFROGRNC'01")};
        case FT_TPWM: return {QStringLiteral("'TPWM'")};
        case FT_FREEZE: return {QStringLiteral("1F9E"), QStringLiteral("1F9F")};
        case FT_UNIX_PACK: return {QStringLiteral("1F1E"), QStringLiteral("1F1F")};
        default: return {};
    }
}

XBinary *XAncient::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAncient(pDevice, m_fileTypeHint);
}

QMap<XBinary::UNPACK_PROP, QVariant> XAncient::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

QString XAncient::outputName(QIODevice *pDevice, FT fileType)
{
    QString sName = XBinary::getDeviceFileBaseName(pDevice);
    if (sName.isEmpty()) sName = QStringLiteral("unpacked");
    if (fileType == FT_DMS) sName += QStringLiteral(".adf");
    return XBinary::fixFileName(sName);
}

bool XAncient::initUnpack(UNPACK_STATE *pState,
                          const QMap<UNPACK_PROP, QVariant> &mapProperties,
                          PDSTRUCT *pPdStruct)
{
    QPointer<XAncient> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;

    UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct) ||
        !bindUnpackSource(pState, pPdStruct))
        return false;

    QByteArray baPacked;
    STREAM_INFO info;
    if (!readPackedData(&baPacked, pPdStruct) || !guardedThis ||
        !describe(baPacked, m_fileTypeHint, &info) ||
        !isPdStructNotCanceled(pPdStruct)) {
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pContext->info = info;
    pContext->sFileName = outputName(getDevice(), info.fileType);
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = baPacked.size();
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XAncient::infoCurrent(UNPACK_STATE *pState,
                                             PDSTRUCT *pPdStruct)
{
    QPointer<XAncient> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1))
        return ARCHIVERECORD();

    const UNPACK_CONTEXT *pContext =
        static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->info.nStreamOffset;
    result.nStreamSize = pContext->info.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->info.nPackedSize);
    if (pContext->info.nImageSize >= 0)
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                    pContext->info.nImageSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                pContext->info.sMethod);
    if (!markArchiveStreamRecord(&result, 0)) return ARCHIVERECORD();
    return result;
}

bool XAncient::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                             PDSTRUCT *pPdStruct)
{
    QPointer<XAncient> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !pDevice || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1) ||
        !isPdStructNotCanceled(pPdStruct))
        return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedOutput || !guardedSource ||
        !isUnpackOutputSupported(guardedOutput.data()) ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;

    const UNPACK_CONTEXT *pContext =
        static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    QByteArray baPacked;
    if (!readPackedData(&baPacked, pPdStruct) || !guardedThis ||
        !guardedOutput || !guardedSource ||
        (baPacked.size() != pState->nTotalSize) ||
        !isUnpackSourceCurrent(pState, pPdStruct))
        return false;

    QByteArray raw;
    qint64 nImageSize = -1;
    qint64 nImageOffset = 0;
    XAncientDecoder::INFO decoderInfo;
    XAncientDecoder::DECODE_ERROR decoderError = XAncientDecoder::ERROR_NONE;
    if ((pContext->info.nStreamOffset < 0) ||
        (pContext->info.nStreamOffset > baPacked.size())) {
        return false;
    }
    const QByteArray baDecoderInput = baPacked.mid(
        qint32(pContext->info.nStreamOffset));
    if (!XAncientDecoder::decode(baDecoderInput,
                                 decoderType(pContext->info.fileType),
                                 &raw, &decoderInfo, &decoderError, true)) {
        if (decoderError == XAncientDecoder::ERROR_VERIFICATION) {
            XBinary::setPdStructErrorString(
                pPdStruct,
                tr("Legacy stream checksum verification failed"));
        } else if (decoderError == XAncientDecoder::ERROR_MEMORY) {
            XBinary::setPdStructErrorString(
                pPdStruct, tr("Not enough memory to decompress legacy stream"));
        } else {
            XBinary::setPdStructErrorString(
                pPdStruct, tr("Legacy stream decompression failed"));
        }
        return false;
    }
    const FT actualType = fileTypeForMethod(decoderInfo.method);
    if (actualType != pContext->info.fileType || actualType == FT_UNKNOWN)
        return false;
    nImageSize = decoderInfo.imageSize >= 0 ? decoderInfo.imageSize : raw.size();
    nImageOffset = decoderInfo.imageOffset;

    const qint64 nRawSize = raw.size();
    const qint64 nMaxRaw = XAncientDecoder::MAX_RAW_SIZE;
    if ((nRawSize < 0) || (nRawSize > nMaxRaw) || (nImageSize < 0) ||
        (nImageSize > nMaxRaw) || !addWithin(nImageOffset, nRawSize, nImageSize) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            nImageSize))
        return false;

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(0, pContext->sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(nImageSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(
        XBinary::createFileBuffer(nImageSize, pPdStruct));
    if (!pStage || !guardedThis || !guardedOutput || !guardedSource ||
        !pStage->seek(nImageOffset))
        return false;
    if ((nRawSize > 0) &&
        (pStage->write(raw) != nRawSize))
        return false;
    if (!pStage->seek(0) || !guardedThis ||
        !guardedOutput || !guardedSource ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct))
        return false;

    const bool bResult = publishUnpackOutput(
        pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (bResult && guardedThis) pState->nCurrentOffset = nImageSize;
    return bResult && guardedThis;
}

bool XAncient::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XAncient> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1))
        return false;
    pState->nCurrentIndex = 1;
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XAncient::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XAncient::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE,
            FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD};
}
