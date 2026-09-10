/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlzhcxp.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

#include "Algos/xlzhcxpdecoder.h"

namespace {
const qint64 LZHCXP_HEADER_SIZE = 2;
const quint16 LZHCXP_MAGIC = 0x5A4CU;  // 'L','Z'
// A trial decode is part of detection, so it has to stay bounded.
const qint64 LZHCXP_MAX_INPUT_SIZE = 0x4000000;   // 64 MB
const qint64 LZHCXP_MAX_OUTPUT_SIZE = 0x8000000;  // 128 MB
}  // namespace

XLZHCXP::XLZHCXP(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZHCXP::~XLZHCXP()
{
}

bool XLZHCXP::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XLZHCXP> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < 6) ||
        (context.nInputSize > LZHCXP_MAX_INPUT_SIZE)) {
        return false;
    }

    const QByteArray baHeader = read_array_process(0, 6, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != 6)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromLittleEndian<quint16>(pHeader) != LZHCXP_MAGIC) return false;

    const qint32 nFirstBlockSize = qint32(pHeader[2]);
    if (nFirstBlockSize == 0) return false;
    // The first payload byte carries the low 8 bits of the first 10-bit code,
    // and that code has to be CLEAR (0x200), so this byte is 0 ...
    if (pHeader[3] != 0) return false;
    // ... and the two bits that complete it have to be 0b10, while the two top
    // bits of the second code have to be clear (it is a literal).
    if ((qFromLittleEndian<quint16>(pHeader + 4) & 0xC03U) != 2) return false;
    if ((qint64(nFirstBlockSize) + 4) > context.nInputSize) return false;
    if (nFirstBlockSize != 0xFF) {
        // Anything but a full-size first block means there is only one block,
        // so the file ends right after it plus the zero terminator.
        if ((qint64(nFirstBlockSize) + 4) != context.nInputSize) return false;
        const QByteArray baTerminator =
            read_array_process(3 + nFirstBlockSize, 1, pPdStruct);
        if (!guardedThis || !guardedSource || (baTerminator.size() != 1) ||
            (baTerminator.at(0) != 0)) {
            return false;
        }
    }

    context.nStreamOffset = LZHCXP_HEADER_SIZE;
    context.nStreamSize = context.nInputSize - LZHCXP_HEADER_SIZE;

    const QByteArray baStream = read_array_process(
        context.nStreamOffset, context.nStreamSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baStream.size() != context.nStreamSize)) {
        return false;
    }

    // Nothing in the container states the decompressed size, so the only way
    // to publish one - and the only real proof that this is an LZ stream at
    // all - is to decode it here.
    QByteArray baUnpacked;
    if (!XLzhcxpDecoder::decode(baStream, -1, LZHCXP_MAX_OUTPUT_SIZE,
                                &baUnpacked)) {
        return false;
    }
    if (baUnpacked.isEmpty()) return false;
    context.nUncompressedSize = baUnpacked.size();

    context.sFileName =
        QFileInfo(XBinary::getDeviceFileName(guardedSource.data())).fileName();
    if (!guardedThis || !guardedSource) return false;
    if (context.sFileName.isEmpty()) {
        context.sFileName = QStringLiteral("lzhcxp_data");
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return true;
}

bool XLZHCXP::isValid(PDSTRUCT *pPdStruct)
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

bool XLZHCXP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZHCXP archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLZHCXP::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLZHCXP(pDevice);
}

XBinary::FT XLZHCXP::getFileType()
{
    return FT_LZHCXP;
}

XBinary::MODE XLZHCXP::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLZHCXP::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZHCXP::getArch()
{
    return QString();
}

QString XLZHCXP::getFileFormatExt()
{
    return QString();
}

QString XLZHCXP::getFileFormatExtsString()
{
    return QStringLiteral("LZ compressed file (*.*)");
}

QString XLZHCXP::getMIMEString()
{
    return QStringLiteral("application/x-lzhcxp");
}

QString XLZHCXP::getVersion()
{
    return QStringLiteral("1");
}

qint64 XLZHCXP::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XLZHCXP::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XLZHCXP::getMemoryMap(MAPMODE mapMode,
                                           PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XLZHCXP::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XLZHCXP::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LZHCXP_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nStreamSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  HANDLE_METHOD_LZHCXP);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("LZW (block framed)"));
        result.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) &&
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XLZHCXP::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLZHCXP::initUnpack(UNPACK_STATE *pState,
                         const QMap<UNPACK_PROP, QVariant> &mapProperties,
                         PDSTRUCT *pPdStruct)
{
    QPointer<XLZHCXP> guardedThis(this);
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
        FPART_PROP_INFO, tr("LZ single-file container; block-framed LZW"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
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

XBinary::ARCHIVERECORD XLZHCXP::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex != 0)) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nStreamOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_LZHCXP);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("LZW (block framed)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No CRC and no timestamp exist in this container.
    return result;
}

bool XLZHCXP::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nArchiveSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XLZHCXP::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
