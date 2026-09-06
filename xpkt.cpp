/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpkt.h"

#include <QPointer>
#include <QtEndian>

#include <new>

#include "Algos/xpktdecoder.h"

namespace {
const qint64 PKT_HEADER_SIZE = 0x3a;
const qint64 PKT_MESSAGE_HEADER_SIZE = 14;
const quint16 PKT_TYPE_2 = 2U;
const qint32 PKT_MAX_MESSAGES = 200000;
// A single packed message is bounded by the FidoNet transport in practice; the
// cap here only keeps a corrupt packet from being walked forever.
const qint64 PKT_MAX_MESSAGE_SIZE = 0x1000000;

quint16 pktWord(const uchar *pData, qint64 nOffset)
{
    return qFromLittleEndian<quint16>(pData + nOffset);
}
}  // namespace

XPKT::XPKT(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPKT::~XPKT()
{
}

bool XPKT::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPKT> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Header plus at least the fixed part of one message; the cross-checks below
    // all read inside that window.
    if (context.nInputSize < PKT_HEADER_SIZE + PKT_MESSAGE_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, PKT_HEADER_SIZE + PKT_MESSAGE_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != PKT_HEADER_SIZE + PKT_MESSAGE_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    // Headerless format: the whole detector is these cross-checks.
    if (pktWord(pHeader, 0x12) != PKT_TYPE_2) return false;
    if (pktWord(pHeader, 0x3a) != PKT_TYPE_2) return false;
    if (pktWord(pHeader, 0x00) != pktWord(pHeader, 0x3c)) return false;
    if (pktWord(pHeader, 0x02) != pktWord(pHeader, 0x3e)) return false;
    if (pktWord(pHeader, 0x14) != pktWord(pHeader, 0x40)) return false;
    if (pktWord(pHeader, 0x16) != pktWord(pHeader, 0x42)) return false;

    context.nYear = pktWord(pHeader, 0x04);
    context.nMonth = pktWord(pHeader, 0x06);
    context.nDay = pktWord(pHeader, 0x08);
    context.nHour = pktWord(pHeader, 0x0a);
    context.nMinute = pktWord(pHeader, 0x0c);
    context.nSecond = pktWord(pHeader, 0x0e);
    // The month field is 0-based in FTS-0001.
    if ((context.nYear <= 1899) || (context.nYear >= 3000)) return false;
    if (context.nMonth > 11) return false;
    if ((context.nDay < 1) || (context.nDay > 31)) return false;
    if ((context.nHour > 23) || (context.nMinute > 59) || (context.nSecond > 59)) return false;

    context.nOrigNode = pktWord(pHeader, 0x00);
    context.nDestNode = pktWord(pHeader, 0x02);
    context.nOrigNet = pktWord(pHeader, 0x14);
    context.nDestNet = pktWord(pHeader, 0x16);
    if ((context.nDestNode == 0) || (context.nOrigNet == 0) || (context.nDestNet == 0)) return false;

    qint64 nOffset = PKT_HEADER_SIZE;
    while (isPdStructNotCanceled(pPdStruct)) {
        // The packet terminator is a u16 0; a short tail is treated as one too,
        // which is how U3 stops on packets that were cut mid-terminator.
        if (context.nInputSize - nOffset < 3) break;
        if (context.listMessages.size() >= PKT_MAX_MESSAGES) return false;

        const QByteArray baMessageHeader = read_array_process(nOffset, PKT_MESSAGE_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baMessageHeader.size() < 2) break;
        const uchar *pMessage = reinterpret_cast<const uchar *>(baMessageHeader.constData());
        if (pktWord(pMessage, 0) == 0) break;
        if (baMessageHeader.size() != PKT_MESSAGE_HEADER_SIZE) return false;
        if (pktWord(pMessage, 0) != PKT_TYPE_2) return false;

        // Walk the five NUL-terminated strings to find the record's end.  They
        // are read in bounded chunks so a corrupt packet cannot pull the whole
        // file into memory before the size cap fires.
        qint64 nScan = nOffset + PKT_MESSAGE_HEADER_SIZE;
        qint32 nFieldsFound = 0;
        const qint32 nChunkSize = 0x1000;
        while ((nFieldsFound < 5) && isPdStructNotCanceled(pPdStruct)) {
            if (nScan >= context.nInputSize) return false;
            if ((nScan - nOffset) > PKT_MAX_MESSAGE_SIZE) return false;
            const qint32 nChunk = qint32(qMin<qint64>(nChunkSize, context.nInputSize - nScan));
            const QByteArray baChunk = read_array_process(nScan, nChunk, pPdStruct);
            if (!guardedThis || !guardedSource || (baChunk.size() != nChunk)) return false;
            for (qint32 i = 0; (i < nChunk) && (nFieldsFound < 5); ++i) {
                if (baChunk.at(i) == '\0') {
                    ++nFieldsFound;
                    if (nFieldsFound == 5) nScan += i + 1;
                }
            }
            if (nFieldsFound < 5) nScan += nChunk;
        }
        if (nFieldsFound < 5) return false;

        MESSAGE message = {};
        message.nRecordOffset = nOffset;
        message.nRecordSize = nScan - nOffset;
        message.nOrigNode = pktWord(pMessage, 0x02);
        message.nDestNode = pktWord(pMessage, 0x04);
        message.nOrigNet = pktWord(pMessage, 0x06);
        message.nDestNet = pktWord(pMessage, 0x08);
        message.nAttribute = pktWord(pMessage, 0x0a);
        message.nCost = pktWord(pMessage, 0x0c);
        if (message.nRecordSize > PKT_MAX_MESSAGE_SIZE) return false;

        // Publishing a size the decoder cannot reproduce would make extraction
        // fail at the last check, so measure the rendering here from the very
        // same helper the codec uses.
        const QByteArray baRecord = read_array_process(message.nRecordOffset, qint32(message.nRecordSize), pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != message.nRecordSize)) return false;
        message.nUncompressedSize = XPKTDecoder::renderedSize(baRecord);
        if (message.nUncompressedSize < 0) return false;

        message.sFileName = QStringLiteral("%1.txt").arg(context.listMessages.size() + 1, 3, 10, QLatin1Char('0'));
        context.listMessages.append(message);
        nOffset = nScan;
    }

    if (context.listMessages.isEmpty()) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    context.nArchiveSize = nOffset;
    *pContext = context;
    return true;
}

bool XPKT::isValid(PDSTRUCT *pPdStruct)
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

bool XPKT::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPKT archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPKT::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPKT(pDevice);
}

XBinary::FT XPKT::getFileType()
{
    return FT_PKT;
}

XBinary::MODE XPKT::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPKT::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPKT::getArch()
{
    return QString();
}

QString XPKT::getFileFormatExt()
{
    return QStringLiteral("pkt");
}

QString XPKT::getFileFormatExtsString()
{
    return QStringLiteral("FidoNet mail packet (*.pkt)");
}

QString XPKT::getMIMEString()
{
    return QStringLiteral("application/x-fidonet-packet");
}

QString XPKT::getVersion()
{
    return QStringLiteral("2");
}

qint64 XPKT::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPKT::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPKT::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XPKT::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPKT::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PKT_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MESSAGE &message : context.listMessages) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = message.nRecordOffset;
            part.nFileSize = message.nRecordSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = message.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, message.nRecordSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, message.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKT);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store (packed message)"));
            result.append(part);
        }
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
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, result.size())) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XPKT::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPKT::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XPKT> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMessages.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("FidoNet mail packet (type 2); messages rendered as text"));
    pState->nCurrentOffset = pContext->listMessages.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMessages.size();
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

XBinary::ARCHIVERECORD XPKT::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMessages.size())) return ARCHIVERECORD();
    const MESSAGE &message = pContext->listMessages.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != message.nRecordOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = message.nRecordOffset;
    result.nStreamSize = message.nRecordSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, message.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, message.nRecordSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, message.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKT);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store (packed message)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XPKT::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMessages.size())) return false;
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMessages.size()) {
        pState->nCurrentOffset = pContext->listMessages.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMessages.size());
}

bool XPKT::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
