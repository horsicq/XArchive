/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xfmc1.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 FMC1_MAGIC_SIZE = 4;
const qint64 FMC1_RECORD_SIZE = 24;
const qint64 FMC1_NAME_SIZE = 12;

const qint32 FMC1_MAX_MEMBERS = 100000;

// The corpus tops out at 109 KB unpacked from a 45 KB member; the caps only
// bound a corrupt size field.
const qint64 FMC1_MAX_COMPRESSED_SIZE = Q_INT64_C(256) * 1024 * 1024;
const qint64 FMC1_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(256) * 1024 * 1024;

bool fmc1RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XFMC1::XFMC1(QIODevice *pDevice)
    : XArchive(pDevice), m_bContextCached(false), m_bContextValid(false), m_context(), m_pCachedDevice(nullptr), m_nCachedSize(-1)
{
}

XFMC1::~XFMC1()
{
}

// The 12-byte field is NOT cleanly padded: "fm.doc" is followed by a NUL and
// then stale bytes (0x14 0x20 ...).  Only the run up to the first NUL is part
// of the name, and only that run may be validated.
bool XFMC1::isValidRawName(const char *pRawName)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(FMC1_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;
    if (nLength == 0) return false;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
    }
    return true;
}

// Path separators and Windows reserved punctuation are escaped as %XX rather
// than folded to '_': escaping is reversible and cannot collapse two distinct
// members onto one output file.
QString XFMC1::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(FMC1_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;
    while ((nLength > 0) && (pRawName[nLength - 1] == ' ')) nLength--;

    QString sResult;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("record%1").arg(nIndex);
    }
    return sResult;
}

// Walk the LZSS token stream and count the bytes it would emit.  This is the
// ONLY way to learn a member's uncompressed size - the container never stores
// it - and it mirrors the reference decompressor's termination rules exactly:
// the stream ends when the payload is spent, a truncated token simply stops the
// walk, and nothing is emitted past that point.
qint64 XFMC1::measureLzss(const QByteArray &baPacked)
{
    const quint8 *pData = reinterpret_cast<const quint8 *>(baPacked.constData());
    const qint64 nSize = baPacked.size();

    qint64 nPosition = 0;
    qint64 nRemaining = nSize;
    qint64 nProduced = 0;
    quint32 nFlags = 0;

    for (;;) {
        if (nRemaining < 1) break;
        quint8 nByte = pData[nPosition];
        nPosition++;
        nRemaining--;

        nFlags >>= 1;
        if ((nFlags & 0x100U) == 0) {
            // The flag byte and the first token byte are read back to back; a
            // flag byte with nothing behind it ends the stream.
            if (nRemaining == 0) break;
            nFlags = static_cast<quint32>(nByte) | 0xff00U;
            nByte = pData[nPosition];
            nPosition++;
            nRemaining--;
        }

        if (nFlags & 1U) {
            nProduced++;
        } else {
            if (nRemaining < 1) break;
            const quint8 nSecond = pData[nPosition];
            nPosition++;
            nRemaining--;
            nProduced += static_cast<qint64>(nSecond & 0x0fU) + 3;
        }
        if (nProduced > FMC1_MAX_UNCOMPRESSED_SIZE) return -1;
    }

    return nProduced;
}

bool XFMC1::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XFMC1> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if (nInputSize < FMC1_MAGIC_SIZE + FMC1_RECORD_SIZE) return false;

    // Cache key only.  The smallest legitimate container is 29 bytes (magic +
    // record + a one-byte payload), so the read is clamped to the file size
    // instead of demanding a full 32 bytes.
    const qint64 nPrefixSize = qMin<qint64>(32, nInputSize);
    const QByteArray baPrefix = read_array_process(0, nPrefixSize, pPdStruct);
    if (!guardedThis || !guardedSource || (static_cast<qint64>(baPrefix.size()) != nPrefixSize)) return false;
    if (!baPrefix.startsWith(QByteArrayLiteral("FMC1"))) return false;

    if (m_bContextCached && (m_pCachedDevice == guardedSource.data()) && (m_nCachedSize == nInputSize) && (m_baCachedPrefix == baPrefix)) {
        if (!m_bContextValid) return false;
        *pContext = m_context;
        return isPdStructNotCanceled(pPdStruct);
    }

    m_bContextCached = true;
    m_bContextValid = false;
    m_context = CONTEXT();
    m_pCachedDevice = guardedSource.data();
    m_nCachedSize = nInputSize;
    m_baCachedPrefix = baPrefix;

    CONTEXT context = {};
    context.nInputSize = nInputSize;

    qint64 nOffset = FMC1_MAGIC_SIZE;
    while (nOffset < nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= FMC1_MAX_MEMBERS) return false;
        if (nOffset + FMC1_RECORD_SIZE > nInputSize) return false;

        const QByteArray baRecord = read_array_process(nOffset, FMC1_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != FMC1_RECORD_SIZE)) return false;
        const char *pRecord = baRecord.constData();
        const uchar *pRaw = reinterpret_cast<const uchar *>(pRecord);

        if (!isValidRawName(pRecord)) return false;
        if (qFromLittleEndian<qint16>(pRaw + 0x12) != 0) return false;
        const qint64 nCompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x14));
        if ((nCompressedSize <= 0) || (nCompressedSize > FMC1_MAX_COMPRESSED_SIZE)) return false;

        const qint64 nDataOffset = nOffset + FMC1_RECORD_SIZE;
        if (!fmc1RangeWithin(nInputSize, nDataOffset, nCompressedSize)) return false;

        const QByteArray baPacked = read_array_process(nDataOffset, nCompressedSize, pPdStruct);
        if (!guardedThis || !guardedSource || (static_cast<qint64>(baPacked.size()) != nCompressedSize)) return false;
        const qint64 nUncompressedSize = measureLzss(baPacked);
        if (nUncompressedSize <= 0) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nDosDateTime = (static_cast<quint32>(qFromLittleEndian<quint16>(pRaw + 0x0e)) << 16) |
                              static_cast<quint32>(qFromLittleEndian<quint16>(pRaw + 0x0c));
        member.sFileName = rawNameToString(pRecord, context.listMembers.size());
        context.listMembers.append(member);

        nOffset = nDataOffset + nCompressedSize;
    }

    // The chain has to tile the container exactly; the reference reader fails
    // the archive on any leftover byte.
    if (context.listMembers.isEmpty() || (nOffset != nInputSize)) return false;

    context.nArchiveSize = nInputSize;
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    m_bContextValid = true;
    m_context = context;
    *pContext = context;
    return guardedThis && guardedSource;
}

bool XFMC1::isValid(PDSTRUCT *pPdStruct)
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

bool XFMC1::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFMC1 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XFMC1::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFMC1(pDevice);
}

QList<QString> XFMC1::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'FMC1'"));
    return listResult;
}

XBinary::FT XFMC1::getFileType()
{
    return FT_FMC1;
}

XBinary::MODE XFMC1::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XFMC1::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XFMC1::getArch()
{
    return QString();
}

QString XFMC1::getFileFormatExt()
{
    return QStringLiteral("cmp");
}

QString XFMC1::getFileFormatExtsString()
{
    return QStringLiteral("Form Master archive (*.cmp)");
}

QString XFMC1::getMIMEString()
{
    return QStringLiteral("application/x-fmc1");
}

QString XFMC1::getVersion()
{
    return QStringLiteral("1");
}

qint64 XFMC1::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XFMC1::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XFMC1::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XFMC1::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XFMC1::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = FMC1_MAGIC_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = FMC1_RECORD_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_AMPK_LZSS);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZSS"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = FMC1_RECORD_SIZE + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
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

QMap<XBinary::UNPACK_PROP, QVariant> XFMC1::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XFMC1::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XFMC1> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Form Master archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XFMC1::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_AMPK_LZSS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZSS"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    const quint16 nDosDate = static_cast<quint16>(member.nDosDateTime >> 16);
    const quint16 nDosTime = static_cast<quint16>(member.nDosDateTime & 0xFFFFU);
    const QDateTime dtMTime(QDate(((nDosDate >> 9) & 0x7F) + 1980, (nDosDate >> 5) & 0x0F, nDosDate & 0x1F),
                            QTime((nDosTime >> 11) & 0x1F, (nDosTime >> 5) & 0x3F, (nDosTime & 0x1F) * 2));
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XFMC1::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XFMC1::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
