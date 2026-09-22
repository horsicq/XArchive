/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xstork.h"

#include <QVector>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 STORK_RECORD_SIZE = 0x12;
const qint64 STORK_NAME_FIELD_SIZE = 12;
const char STORK_TERMINATOR = '$';
// The first member of every Stork archive is the runtime's own association
// table.  It is the only fixed byte sequence the format has and the reference
// implementation keys its detector on it.
const char STORK_ANCHOR[] = {0x0a, '@', 'A', 'S', 'S', 'O', 'C', '.', 'S', 'A', 'V'};
const qint64 STORK_ANCHOR_SIZE = 11;
// A DOS install set never carries more than a few dozen members; the cap only
// guards the allocation against a runaway walk.
const qint32 STORK_MAX_MEMBERS = 100000;
// Upper bound on what a single member may expand to while being measured.
const qint64 STORK_MAX_OUTPUT = 512LL * 1024 * 1024;

// ---------------------------------------------------------------------------
// PKWARE DCL implode, size-only.
//
// Identical bit syntax to the shared decPkwareDcl in xdecompress.cpp; the only
// difference is that nothing is written out, because the Stork record does not
// carry the uncompressed size and the shared decoder needs to be told it up
// front.  Distances are still validated against the produced length, so a
// stream that would decode incorrectly cannot measure successfully.
// ---------------------------------------------------------------------------
const qint32 STORK_DCL_MAX_BITS = 13;

const quint8 STORK_DCL_LITERAL_REP[] = {11,  124, 8,   7,   28,  7,  188, 13, 76, 4,  10, 8,  12, 10, 12, 10, 8,   23,  8,   9,   7,   6,   7,  8,
                                        7,   6,   55,  8,   23,  24, 12,  11, 7,  9,  11, 12, 6,  7,  22, 5,  7,   24,  6,   11,  9,   6,   7,  22,
                                        7,   11,  38,  7,   9,   8,  25,  11, 8,  11, 9,  12, 8,  12, 5,  38, 5,   38,  5,   11,  7,   5,   6,  21,
                                        6,   10,  53,  8,   7,   24, 10,  27, 44, 253, 253, 253, 252, 252, 252, 13, 12,  45,  12,  45,  12,  61, 12,
                                        45,  44,  173};
const quint8 STORK_DCL_LENGTH_REP[] = {2, 35, 36, 53, 38, 23};
const quint8 STORK_DCL_DISTANCE_REP[] = {2, 20, 53, 230, 247, 151, 248};

const qint32 STORK_DCL_BASE_LENGTH[16] = {3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264};
const qint32 STORK_DCL_EXTRA_LENGTH[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};

struct StorkDclHuffman {
    qint32 count[STORK_DCL_MAX_BITS + 1];
    QVector<qint32> symbol;
    bool bValid;

    StorkDclHuffman() : bValid(false)
    {
        memset(count, 0, sizeof(count));
    }

    void construct(const quint8 *pRep, qint32 nRepSize)
    {
        QVector<qint32> listLengths;
        for (qint32 i = 0; i < nRepSize; i++) {
            const qint32 nRepeat = (pRep[i] >> 4) + 1;
            const qint32 nLength = pRep[i] & 15;
            for (qint32 j = 0; j < nRepeat; j++) listLengths.append(nLength);
        }
        for (qint32 i = 0; i < listLengths.size(); i++) {
            const qint32 nLength = listLengths.at(i);
            if ((nLength < 0) || (nLength > STORK_DCL_MAX_BITS)) return;
            count[nLength]++;
        }
        qint32 nLeft = 1;
        for (qint32 nLength = 1; nLength <= STORK_DCL_MAX_BITS; nLength++) {
            nLeft <<= 1;
            nLeft -= count[nLength];
            if (nLeft < 0) return;
        }
        QVector<qint32> listOffsets(STORK_DCL_MAX_BITS + 2, 0);
        for (qint32 nLength = 1; nLength < STORK_DCL_MAX_BITS; nLength++) {
            listOffsets[nLength + 1] = listOffsets.at(nLength) + count[nLength];
        }
        symbol.resize(listLengths.size());
        for (qint32 i = 0; i < listLengths.size(); i++) {
            const qint32 nLength = listLengths.at(i);
            if (nLength != 0) {
                symbol[listOffsets.at(nLength)] = i;
                listOffsets[nLength] = listOffsets.at(nLength) + 1;
            }
        }
        bValid = true;
    }
};

struct StorkDclTables {
    StorkDclHuffman literal;
    StorkDclHuffman length;
    StorkDclHuffman distance;
    bool bValid;

    StorkDclTables()
    {
        literal.construct(STORK_DCL_LITERAL_REP, static_cast<qint32>(sizeof(STORK_DCL_LITERAL_REP)));
        length.construct(STORK_DCL_LENGTH_REP, static_cast<qint32>(sizeof(STORK_DCL_LENGTH_REP)));
        distance.construct(STORK_DCL_DISTANCE_REP, static_cast<qint32>(sizeof(STORK_DCL_DISTANCE_REP)));
        bValid = literal.bValid && length.bValid && distance.bValid;
    }
};

class StorkDclBits {
public:
    explicit StorkDclBits(const QByteArray &baData)
        : m_pBytes(reinterpret_cast<const uchar *>(baData.constData())), m_nSize(baData.size()), m_nPosition(0), m_nBuffer(0), m_nBitCount(0)
    {
    }

    bool read(qint32 nCount, qint32 *pnValue)
    {
        if (!pnValue || (nCount < 0) || (nCount > 16)) return false;
        while (m_nBitCount < nCount) {
            if (m_nPosition >= m_nSize) return false;
            m_nBuffer |= static_cast<quint32>(m_pBytes[m_nPosition++]) << m_nBitCount;
            m_nBitCount += 8;
        }
        *pnValue = nCount ? static_cast<qint32>(m_nBuffer & ((1U << nCount) - 1U)) : 0;
        m_nBuffer >>= nCount;
        m_nBitCount -= nCount;
        return true;
    }

private:
    const uchar *m_pBytes;
    qint32 m_nSize;
    qint32 m_nPosition;
    quint32 m_nBuffer;
    qint32 m_nBitCount;
};

bool storkDclSymbol(StorkDclBits *pBits, const StorkDclHuffman &table, qint32 *pnSymbol)
{
    if (!pBits || !pnSymbol) return false;
    qint32 nCode = 0;
    qint32 nFirst = 0;
    qint32 nIndex = 0;
    for (qint32 nLength = 1; nLength <= STORK_DCL_MAX_BITS; nLength++) {
        qint32 nBit = 0;
        if (!pBits->read(1, &nBit)) return false;
        nCode |= nBit ^ 1;
        const qint32 nCount = table.count[nLength];
        if (nCode < nFirst + nCount) {
            const qint32 nSymbolIndex = nIndex + nCode - nFirst;
            if ((nSymbolIndex < 0) || (nSymbolIndex >= table.symbol.size())) return false;
            *pnSymbol = table.symbol.at(nSymbolIndex);
            return true;
        }
        nIndex += nCount;
        nFirst = (nFirst + nCount) << 1;
        nCode <<= 1;
    }
    return false;
}
}  // namespace

qint64 XStork::measurePayload(const QByteArray &baPacked)
{
    if (baPacked.size() < 3) return -1;
    static const StorkDclTables tables;
    if (!tables.bValid) return -1;

    StorkDclBits bits(baPacked);
    qint32 nLiteralMode = 0;
    qint32 nDictionaryBits = 0;
    if (!bits.read(8, &nLiteralMode) || !bits.read(8, &nDictionaryBits)) return -1;
    if ((nLiteralMode < 0) || (nLiteralMode > 1) || (nDictionaryBits < 4) || (nDictionaryBits > 6)) return -1;

    qint64 nProduced = 0;
    for (;;) {
        qint32 nIsMatch = 0;
        if (!bits.read(1, &nIsMatch)) return -1;
        if (!nIsMatch) {
            qint32 nLiteral = 0;
            if (nLiteralMode) {
                if (!storkDclSymbol(&bits, tables.literal, &nLiteral)) return -1;
            } else {
                if (!bits.read(8, &nLiteral)) return -1;
            }
            if ((nLiteral < 0) || (nLiteral > 255)) return -1;
            nProduced++;
            if (nProduced > STORK_MAX_OUTPUT) return -1;
            continue;
        }
        qint32 nLengthSymbol = 0;
        if (!storkDclSymbol(&bits, tables.length, &nLengthSymbol)) return -1;
        if ((nLengthSymbol < 0) || (nLengthSymbol >= 16)) return -1;
        qint32 nExtra = 0;
        if (!bits.read(STORK_DCL_EXTRA_LENGTH[nLengthSymbol], &nExtra)) return -1;
        const qint32 nLength = STORK_DCL_BASE_LENGTH[nLengthSymbol] + nExtra;
        if (nLength == 519) break;
        qint32 nDistanceSymbol = 0;
        if (!storkDclSymbol(&bits, tables.distance, &nDistanceSymbol)) return -1;
        if ((nDistanceSymbol < 0) || (nDistanceSymbol >= 64)) return -1;
        const qint32 nLowBits = (nLength == 2) ? 2 : nDictionaryBits;
        qint32 nDistanceLow = 0;
        if (!bits.read(nLowBits, &nDistanceLow)) return -1;
        const qint64 nDistance = (static_cast<qint64>(nDistanceSymbol) << nLowBits) + nDistanceLow + 1;
        if ((nDistance <= 0) || (nDistance > nProduced)) return -1;
        nProduced += nLength;
        if (nProduced > STORK_MAX_OUTPUT) return -1;
    }

    return nProduced;
}

XStork::XStork(QIODevice *pDevice) : XArchive(pDevice)
{
}

XStork::~XStork()
{
}

// Only the first nNameLength bytes of the 12-byte field are meaningful; the
// producer pads with whatever was in its buffer.  Trailing spaces are dropped
// (some members are stored as "PKUNZIP.EXE ") and everything the host
// filesystem would reject is escaped as %XX rather than folded to '_'.
QString XStork::rawNameToString(const char *pRawName, qint32 nLength, qint32 nIndex)
{
    while ((nLength > 0) && ((pRawName[nLength - 1] == ' ') || (pRawName[nLength - 1] == '\0'))) nLength--;

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

bool XStork::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < STORK_RECORD_SIZE + 3) return false;

    const QByteArray baAnchor = read_array_process(0, STORK_ANCHOR_SIZE, pPdStruct);
    if (!guardedSource || (baAnchor.size() != STORK_ANCHOR_SIZE)) return false;
    if (memcmp(baAnchor.constData(), STORK_ANCHOR, STORK_ANCHOR_SIZE) != 0) return false;

    qint64 nOffset = 0;
    qint32 nIndex = 0;
    while (nOffset < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nIndex >= STORK_MAX_MEMBERS) return false;
        if (nOffset + STORK_RECORD_SIZE > context.nInputSize) return false;

        const QByteArray baRecord = read_array_process(nOffset, STORK_RECORD_SIZE, pPdStruct);
        if (!guardedSource || (baRecord.size() != STORK_RECORD_SIZE)) return false;
        const uchar *pRecord = reinterpret_cast<const uchar *>(baRecord.constData());

        const qint32 nNameLength = static_cast<qint32>(pRecord[0]);
        if ((nNameLength < 1) || (nNameLength > static_cast<qint32>(STORK_NAME_FIELD_SIZE))) return false;
        if (baRecord.at(0x11) != STORK_TERMINATOR) return false;
        const qint64 nPackedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRecord + 0x0d));
        // The chain has no terminator record, so a member that would run past
        // the end of the file is the proof that this is not a Stork archive.
        if (nPackedSize < 3) return false;
        if (nPackedSize > context.nInputSize - (nOffset + STORK_RECORD_SIZE)) return false;

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nDataOffset = nOffset + STORK_RECORD_SIZE;
        member.nCompressedSize = nPackedSize;
        member.nUncompressedSize = -1;
        member.sFileName = rawNameToString(baRecord.constData() + 1, nNameLength, nIndex);

        const QByteArray baPacked = read_array_process(member.nDataOffset, member.nCompressedSize, pPdStruct);
        if (!guardedSource || (baPacked.size() != member.nCompressedSize)) return false;
        member.nUncompressedSize = measurePayload(baPacked);
        // The very first payload is the trial decode that makes a headerless
        // format safe to claim; later members are allowed to be damaged and are
        // still listed, they simply cannot be unpacked.
        if ((nIndex == 0) && (member.nUncompressedSize < 0)) return false;

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
        nIndex++;
    }

    if (context.listMembers.isEmpty()) return false;
    // The chain must tile the file exactly.
    if (nOffset != context.nInputSize) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XStork::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XStork::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XStork archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XStork::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XStork(pDevice);
}

QList<QString> XStork::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("0A'@ASSOC.SAV'"));
    return listResult;
}

XBinary::FT XStork::getFileType()
{
    return FT_STORK;
}

XBinary::MODE XStork::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XStork::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XStork::getArch()
{
    return QString();
}

QString XStork::getFileFormatExt()
{
    return QStringLiteral("stk");
}

QString XStork::getFileFormatExtsString()
{
    return QStringLiteral("Stork install archive (*.stk *.hd!)");
}

QString XStork::getMIMEString()
{
    return QStringLiteral("application/x-stork");
}

QString XStork::getVersion()
{
    return QString();
}

qint64 XStork::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XStork::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XStork::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XStork::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XStork::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = STORK_RECORD_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
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
            if (member.nUncompressedSize >= 0) {
                part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
            } else {
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
            }
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL Implode"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XStork::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XStork::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Stork install archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XStork::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    if (member.nUncompressedSize >= 0) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    } else {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
    }
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL Implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XStork::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nInputSize;
    }

    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XStork::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
