/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xash0.h"

#include "Algos/xash0decoder.h"

#include <QFileInfo>
#include <QPointer>

#include <new>

namespace {
// 'ASH0' | quint32 sizeWord | quint32 distOffset
const qint64 ASH0_HEADER_SIZE = 12;
// Header plus one symbol word plus one distance word.
const qint64 ASH0_MIN_SIZE = 20;
const qint32 ASH0_SYM_BITS = 9;
const qint32 ASH0_DIST_BITS_DEFAULT = 11;
const qint32 ASH0_DIST_BITS_RANCH = 15;
// The plaintext is at most 16 MiB - 1 (24-bit length) and nine-bit literals
// bound a sane encoder's expansion to about 1.13x, so a container beyond this
// cannot be a real ASH0 file; refusing it keeps the trial decode from reading
// gigabytes of something else that happens to start with the magic.
const qint64 ASH0_MAX_PACKED_SIZE = Q_INT64_C(64) * 1024 * 1024;
const char ASH0_EXT_SUFFIX[] = ".ash";
}  // namespace

XASH0::XASH0(QIODevice *pDevice) : XArchive(pDevice)
{
}

XASH0::~XASH0()
{
}

QString XASH0::memberName(const QString &sContainerName)
{
    QString sResult = sContainerName;
    if (sResult.isEmpty()) return QStringLiteral("ash0_data");

    const QString sSuffix = QString::fromLatin1(ASH0_EXT_SUFFIX);
    // Strip one trailing ".ash" - but never down to an empty name.
    if ((sResult.size() > sSuffix.size()) && sResult.endsWith(sSuffix, Qt::CaseInsensitive)) {
        sResult.chop(sSuffix.size());
    }
    return sResult;
}

QString XASH0::reportedMethod(qint32 nDistBits)
{
    if (nDistBits == ASH0_DIST_BITS_RANCH) return QStringLiteral("ASH0 9/15");
    return QStringLiteral("ASH0 9/11");
}

qint64 XASH0::windowSize(qint32 nDistBits)
{
    if (nDistBits == ASH0_DIST_BITS_RANCH) return Q_INT64_C(1) << ASH0_DIST_BITS_RANCH;
    return Q_INT64_C(1) << ASH0_DIST_BITS_DEFAULT;
}

bool XASH0::parseContext(CONTEXT *pContext, bool bVerifyPayload, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XASH0> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < ASH0_MIN_SIZE) || (context.nInputSize > ASH0_MAX_PACKED_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, ASH0_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != ASH0_HEADER_SIZE)) return false;

    // Magic, length 1..0xFFFFFF, distance offset in [0x10, size - 4] and the
    // expansion-ratio guard all live in the decoder's header parse so the two
    // can never disagree.
    XASH0Decoder::HEADER header = {};
    if (!XASH0Decoder::parseHeader(reinterpret_cast<const quint8 *>(baHeader.constData()), baHeader.size(), context.nInputSize, &header)) {
        return false;
    }
    context.nUncompressedSize = header.nUncompressedSize;
    context.nDistOffset = header.nDistOffset;
    context.nTopByte = header.nTopByte;
    context.nDistBits = 0;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    QString sContainerName;
    if (!sDeviceName.isEmpty()) {
        sContainerName = QFileInfo(sDeviceName).fileName();
    }
    context.sFileName = memberName(sContainerName);

    if (bVerifyPayload) {
        const QByteArray baPacked = read_array_process(0, context.nInputSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baPacked.size() != context.nInputSize)) return false;

        // The whole gate: both streams must parse, every match must be legal,
        // the output must reach exactly the declared length, and the width
        // search must settle (xash0decoder.h).  No checksum exists to do it
        // any cheaper.
        XASH0Decoder::RESULT result = {};
        if (!XASH0Decoder::probe(baPacked, &result, pPdStruct)) return false;
        if (!guardedThis || !guardedSource) return false;
        if ((result.nDistBits != ASH0_DIST_BITS_DEFAULT) && (result.nDistBits != ASH0_DIST_BITS_RANCH)) return false;
        if (result.nSymBits != ASH0_SYM_BITS) return false;
        context.nDistBits = result.nDistBits;
        context.bTight = result.bTight;
        context.bAmbiguous = result.bAmbiguous;
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XASH0::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // Four magic bytes and a 24-bit length have no checksum behind them; only
    // a trial decode that reproduces the declared length keeps this class from
    // claiming unrelated files and writing garbage at exit 0.
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XASH0::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XASH0 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XASH0::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XASH0(pDevice);
}

QList<QString> XASH0::getSearchSignatures()
{
    // Nothing else in the header is fixed: +4 carries the unknown top byte and
    // a variable length, +8 a variable offset.
    return QList<QString>() << QStringLiteral("'ASH0'");
}

XBinary::FT XASH0::getFileType()
{
    return FT_ASH0;
}

XBinary::MODE XASH0::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XASH0::getEndian()
{
    return ENDIAN_BIG;
}

QString XASH0::getArch()
{
    return QString();
}

qint32 XASH0::getType()
{
    return TYPE_ARCHIVE;
}

QString XASH0::getFileFormatExt()
{
    return QStringLiteral("ash");
}

QString XASH0::getFileFormatExtsString()
{
    return QStringLiteral("Nintendo ASH0 (*.ash)");
}

QString XASH0::getMIMEString()
{
    return QStringLiteral("application/x-nintendo-ash0");
}

qint64 XASH0::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    // The distance stream runs to EOF and there is no inner end marker, so
    // the container is the whole file.
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XASH0::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XASH0::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XASH0::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XASH0::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    // Strong parse: the plaintext length and the width reported here must be
    // ones the decoder reproduces.
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ASH0_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        // The stream is the whole file: the codec reads +4 and +8 itself.
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nInputSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ASH0);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethod(context.nDistBits));
        part.mapProperties.insert(FPART_PROP_WINDOWSIZE, windowSize(context.nDistBits));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XASH0::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XASH0::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XASH0> guardedThis(this);
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
    // bVerifyPayload = true: the decoder takes the plaintext length as an
    // INPUT and the width as a hint, so both must be ones it reproduces.
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO, QString(QStringLiteral("Nintendo ASH0; symbol tree %1 bits, distance tree %2 bits; size word top byte 0x%3"))
                             .arg(ASH0_SYM_BITS)
                             .arg(pContext->nDistBits)
                             .arg(static_cast<quint32>(pContext->nTopByte), 2, 16, QLatin1Char('0')));
    pState->nCurrentOffset = 0;  // the member's data offset: the stream is the whole file
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

XBinary::ARCHIVERECORD XASH0::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();
    // The cursor offset is part of the contract: the single member's data
    // offset is 0.
    if (pState->nCurrentOffset != 0) return ARCHIVERECORD();
    if ((pContext->nDistBits != ASH0_DIST_BITS_DEFAULT) && (pContext->nDistBits != ASH0_DIST_BITS_RANCH)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nInputSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nInputSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ASH0);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethod(pContext->nDistBits));
    result.mapProperties.insert(FPART_PROP_WINDOWSIZE, windowSize(pContext->nDistBits));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No checksum and no time stamp property: the container carries neither.
    return result;
}

bool XASH0::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;
    // Mode A: the only record is the last one, so the index moves past it, the
    // cursor lands on the end of the container and the call reports "no more".
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XASH0::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XASH0::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_WINDOWSIZE << FPART_PROP_ISFOLDER;
}
