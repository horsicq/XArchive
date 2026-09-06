/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsq.h"

#include "Algos/xsqdecoder.h"

#include <QDate>
#include <QDateTime>
#include <QPointer>
#include <QTime>

#include <new>

namespace {
const qint64 SQ_MAGIC_SIZE = 4;
const qint64 SQ_DATE_SIZE = 6;
// The name is a DOS path in every known producer; the cap keeps the scan that
// looks for its terminator bounded on non-SQ input.
const qint64 SQ_MAX_NAME = 260;
// The container stores no output length, so a ceiling has to come from here.
const qint64 SQ_MAX_OUTPUT = 256LL * 1024 * 1024;

bool sqIsMagic(const char *pData)
{
    return (static_cast<quint8>(pData[0]) == 0x53) && (static_cast<quint8>(pData[1]) == 0x51) && (static_cast<quint8>(pData[2]) == 0xAC) &&
           (static_cast<quint8>(pData[3]) == 0xAE);
}
}  // namespace

XSQ::XSQ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSQ::~XSQ()
{
}

// The stored name is a bare DOS file name, but it is raw bytes: '\' and '/' are
// escaped rather than treated as separators, because a single-member container
// must never be able to write outside the extraction root.
QString XSQ::sanitizeName(const QByteArray &baRawName)
{
    QString sResult;
    for (qint32 i = 0; i < baRawName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRawName.at(i));
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else if (nCharacter == 0x20) {
            sResult.append(QLatin1Char(' '));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    while (sResult.endsWith(QLatin1Char(' '))) sResult.chop(1);
    return sResult;
}

bool XSQ::parseHeader(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSQ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nUncompressedSize = -1;
    context.nInputSize = guardedSource->size();
    // magic + at least a one-character name + NUL + date + one payload byte.
    if (context.nInputSize < SQ_MAGIC_SIZE + 2 + SQ_DATE_SIZE + 1) return false;

    const qint64 nProbeSize = qMin(context.nInputSize, SQ_MAGIC_SIZE + SQ_MAX_NAME + 1 + SQ_DATE_SIZE);
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baProbe.size() != nProbeSize)) return false;
    const char *pProbe = baProbe.constData();

    if (!sqIsMagic(pProbe)) return false;

    qint64 nNameLength = 0;
    while ((SQ_MAGIC_SIZE + nNameLength < nProbeSize) && (pProbe[SQ_MAGIC_SIZE + nNameLength] != '\0')) {
        // The reference detector refuses any byte below 0x20 inside the name;
        // that single rule is what keeps a random 53 51 AC AE from matching.
        if (static_cast<quint8>(pProbe[SQ_MAGIC_SIZE + nNameLength]) < 0x20) return false;
        nNameLength++;
        if (nNameLength > SQ_MAX_NAME) return false;
    }
    if (nNameLength < 1) return false;
    const qint64 nTerminator = SQ_MAGIC_SIZE + nNameLength;
    if (nTerminator >= nProbeSize) return false;
    if (pProbe[nTerminator] != '\0') return false;
    if (nTerminator + 1 + SQ_DATE_SIZE >= context.nInputSize) return false;
    if (nTerminator + 1 + SQ_DATE_SIZE > nProbeSize) return false;

    const quint8 *pDate = reinterpret_cast<const quint8 *>(pProbe) + nTerminator + 1;
    const quint8 nMonth = pDate[0];
    const quint8 nDay = pDate[1];
    const quint8 nYear = pDate[2];
    const quint8 nHour = pDate[3];
    const quint8 nMinute = pDate[4];
    const quint8 nSecond2 = pDate[5];
    if ((nMonth < 1) || (nMonth > 12)) return false;
    if ((nDay < 1) || (nDay > 31)) return false;
    if (nHour > 23) return false;
    if (nMinute > 59) return false;
    if (nSecond2 > 29) return false;

    const QDate date(1980 + static_cast<qint32>(nYear), static_cast<qint32>(nMonth), static_cast<qint32>(nDay));
    if (date.isValid()) {
        context.dtModified = QDateTime(date, QTime(static_cast<qint32>(nHour), static_cast<qint32>(nMinute), static_cast<qint32>(nSecond2) * 2));
    }

    context.sFileName = sanitizeName(baProbe.mid(static_cast<qint32>(SQ_MAGIC_SIZE), static_cast<qint32>(nNameLength)));
    if (context.sFileName.isEmpty()) return false;

    context.nStreamOffset = nTerminator + 1 + SQ_DATE_SIZE;
    context.nStreamSize = context.nInputSize - context.nStreamOffset;
    context.nArchiveSize = context.nInputSize;
    if (context.nStreamSize <= 0) return false;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSQ::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;
    QPointer<XSQ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    CONTEXT context = {};
    if (!parseHeader(&context, pPdStruct) || !guardedThis || !guardedSource) return false;
    if (context.nStreamSize > 0x7FFFFFFF) return false;

    const QByteArray baPacked = read_array_process(context.nStreamOffset, context.nStreamSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baPacked.size() != context.nStreamSize)) return false;

    qint64 nRawSize = 0;
    if (!XSQDecoder::measure(baPacked, SQ_MAX_OUTPUT, &nRawSize, pPdStruct)) return false;
    if (nRawSize <= 0) return false;
    context.nUncompressedSize = nRawSize;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSQ::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseHeader(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSQ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSQ archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSQ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSQ(pDevice);
}

QList<QString> XSQ::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("5351ACAE"));
    return listResult;
}

XBinary::FT XSQ::getFileType()
{
    return FT_SQ;
}

XBinary::MODE XSQ::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSQ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSQ::getArch()
{
    return QString();
}

QString XSQ::getFileFormatExt()
{
    return QStringLiteral("sq_");
}

QString XSQ::getFileFormatExtsString()
{
    return QStringLiteral("SQ squeezed file (*.sq_ *.sq)");
}

QString XSQ::getMIMEString()
{
    return QStringLiteral("application/x-sq");
}

QString XSQ::getVersion()
{
    return QString();
}

qint64 XSQ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseHeader(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSQ::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSQ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSQ::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSQ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nStreamOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SQ);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("SQ adaptive Huffman LZ"));
        if (context.dtModified.isValid()) part.mapProperties.insert(FPART_PROP_DATETIME, context.dtModified);
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XSQ::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSQ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSQ> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || (pContext->nUncompressedSize < 0)) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("SQ squeezed file"));
    pState->nCurrentOffset = pContext->nStreamOffset;
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

XBinary::ARCHIVERECORD XSQ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex != 0) || (pContext->nUncompressedSize < 0)) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nStreamOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SQ);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("SQ adaptive Huffman LZ"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (pContext->dtModified.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, pContext->dtModified);
    return result;
}

bool XSQ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSQ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
