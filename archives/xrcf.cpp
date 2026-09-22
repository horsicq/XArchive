/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xrcf.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 RCF_HEADER_SIZE = 10;
const qint64 RCF_ENTRY_SIZE = 17;
const qint64 RCF_NAME_FIELD = 12;
const qint32 RCF_MAX_MEMBERS = 65535;

// --------------------------------------------------------------------------
// PKWARE DCL "implode" length probe.
//
// RCF stores only the packed size, so the plaintext length has to be recovered
// from the stream itself before HANDLE_METHOD_PKWARE_DCL_IMPLODE can be asked
// to produce it (that decoder consumes an exact expected size).  This walk
// decodes the token stream and counts output bytes without materialising them;
// distances are only range checked, never followed, so no window is needed.
//
// The three code tables are the fixed PKWARE ones, stored run length encoded as
// (repeat-1) << 4 | bit-length, the same shape Mark Adler's public domain blast
// uses.
// --------------------------------------------------------------------------
const qint32 RCF_DCL_MAXBITS = 13;

const quint8 RCF_LITLEN[] = {11,  124, 8,   7,   28,  7,   188, 13,  76,  4,   10,  8,   12,  10,  12,  10,  8,   23,  8,   9,   7,   6,  7,
                             8,   7,   6,   55,  8,   23,  24,  12,  11,  7,   9,   11,  12,  6,   7,   22,  5,   7,   24,  6,   11,  9,  6,
                             7,   22,  7,   11,  38,  7,   9,   8,   25,  11,  8,   11,  9,   12,  8,   12,  5,   38,  5,   38,  5,   11, 7,
                             5,   6,   21,  6,   10,  53,  8,   7,   24,  10,  27,  44,  253, 253, 253, 252, 252, 252, 13,  12,  45,  12, 45,
                             12,  61,  12,  45,  44,  173};
const quint8 RCF_LENLEN[] = {2, 35, 36, 53, 38, 23};
const quint8 RCF_DISTLEN[] = {2, 20, 53, 230, 247, 151, 248};

const qint32 RCF_BASE[16] = {3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264};
const qint32 RCF_EXTRA[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};

struct RcfHuffman {
    qint16 count[RCF_DCL_MAXBITS + 1];
    qint16 symbol[256];
    qint32 nSymbols;
};

void rcfConstruct(RcfHuffman *pHuffman, const quint8 *pRep, qint32 nRepSize)
{
    quint8 bitLength[256];
    qint32 nSymbol = 0;
    for (qint32 i = 0; i < nRepSize; i++) {
        qint32 nRepeat = (pRep[i] >> 4) + 1;
        const quint8 nLength = pRep[i] & 0x0f;
        while ((nRepeat-- > 0) && (nSymbol < 256)) {
            bitLength[nSymbol++] = nLength;
        }
    }
    pHuffman->nSymbols = nSymbol;
    for (qint32 i = 0; i <= RCF_DCL_MAXBITS; i++) pHuffman->count[i] = 0;
    for (qint32 i = 0; i < nSymbol; i++) pHuffman->count[bitLength[i]]++;

    qint16 offsets[RCF_DCL_MAXBITS + 1];
    offsets[1] = 0;
    for (qint32 nLength = 1; nLength < RCF_DCL_MAXBITS; nLength++) {
        offsets[nLength + 1] = static_cast<qint16>(offsets[nLength] + pHuffman->count[nLength]);
    }
    for (qint32 i = 0; i < nSymbol; i++) {
        if (bitLength[i] != 0) pHuffman->symbol[offsets[bitLength[i]]++] = static_cast<qint16>(i);
    }
}

class RcfDclBits {
public:
    RcfDclBits(const uchar *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nBitBuffer(0), m_nBitCount(0)
    {
    }

    bool read(qint32 nBits, qint32 *pnValue)
    {
        qint32 nValue = m_nBitBuffer;
        while (m_nBitCount < nBits) {
            if (m_nPosition >= m_nSize) return false;
            nValue |= static_cast<qint32>(m_pData[m_nPosition]) << m_nBitCount;
            m_nPosition++;
            m_nBitCount += 8;
        }
        m_nBitBuffer = nValue >> nBits;
        m_nBitCount -= nBits;
        *pnValue = nValue & ((1 << nBits) - 1);
        return true;
    }

    // Codes are stored inverted and LSB first, exactly as blast decodes them.
    bool decode(const RcfHuffman *pHuffman, qint32 *pnSymbol)
    {
        qint32 nCode = 0;
        qint32 nFirst = 0;
        qint32 nIndex = 0;
        qint32 nLength = 1;
        qint32 nBitBuffer = m_nBitBuffer;
        qint32 nLeft = m_nBitCount;
        qint32 nNext = 1;
        for (;;) {
            while (nLeft > 0) {
                nLeft--;
                nCode |= (nBitBuffer & 1) ^ 1;
                nBitBuffer >>= 1;
                const qint32 nCount = pHuffman->count[nNext];
                nNext++;
                if (nCode < nFirst + nCount) {
                    m_nBitBuffer = nBitBuffer;
                    m_nBitCount = (m_nBitCount - nLength) & 7;
                    *pnSymbol = pHuffman->symbol[nIndex + (nCode - nFirst)];
                    return true;
                }
                nIndex += nCount;
                nFirst += nCount;
                nFirst <<= 1;
                nCode <<= 1;
                nLength++;
                if (nNext > RCF_DCL_MAXBITS) return false;
            }
            nLeft = (RCF_DCL_MAXBITS + 1) - nLength;
            if (nLeft <= 0) return false;
            if (m_nPosition >= m_nSize) return false;
            nBitBuffer = m_pData[m_nPosition];
            m_nPosition++;
            if (nLeft > 8) nLeft = 8;
        }
    }

private:
    const uchar *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    qint32 m_nBitBuffer;
    qint32 m_nBitCount;
};

bool rcfMeasureDcl(const QByteArray &packed, qint64 *pnRawSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (packed.size() < 3) return false;
    static RcfHuffman s_literal;
    static RcfHuffman s_length;
    static RcfHuffman s_distance;
    static bool s_bBuilt = false;
    if (!s_bBuilt) {
        rcfConstruct(&s_literal, RCF_LITLEN, static_cast<qint32>(sizeof(RCF_LITLEN)));
        rcfConstruct(&s_length, RCF_LENLEN, static_cast<qint32>(sizeof(RCF_LENLEN)));
        rcfConstruct(&s_distance, RCF_DISTLEN, static_cast<qint32>(sizeof(RCF_DISTLEN)));
        s_bBuilt = true;
    }

    RcfDclBits bits(reinterpret_cast<const uchar *>(packed.constData()), packed.size());
    qint32 nLiteralMode = 0;
    qint32 nDictionaryBits = 0;
    if (!bits.read(8, &nLiteralMode) || !bits.read(8, &nDictionaryBits)) return false;
    if ((nLiteralMode < 0) || (nLiteralMode > 1)) return false;
    if ((nDictionaryBits < 4) || (nDictionaryBits > 6)) return false;

    qint64 nProduced = 0;
    for (;;) {
        if (((nProduced & 0x3fff) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        qint32 nIsMatch = 0;
        if (!bits.read(1, &nIsMatch)) return false;
        if (!nIsMatch) {
            qint32 nLiteral = 0;
            if (nLiteralMode) {
                if (!bits.decode(&s_literal, &nLiteral)) return false;
            } else if (!bits.read(8, &nLiteral)) {
                return false;
            }
            if ((nLiteral < 0) || (nLiteral > 255)) return false;
            nProduced++;
            continue;
        }
        qint32 nLengthSymbol = 0;
        if (!bits.decode(&s_length, &nLengthSymbol) || (nLengthSymbol < 0) || (nLengthSymbol >= 16)) return false;
        qint32 nExtra = 0;
        if (!bits.read(RCF_EXTRA[nLengthSymbol], &nExtra)) return false;
        const qint32 nLength = RCF_BASE[nLengthSymbol] + nExtra;
        if (nLength == 519) break;  // end of stream
        qint32 nDistanceSymbol = 0;
        if (!bits.decode(&s_distance, &nDistanceSymbol) || (nDistanceSymbol < 0) || (nDistanceSymbol >= 64)) return false;
        const qint32 nLowBits = (nLength == 2) ? 2 : nDictionaryBits;
        qint32 nDistanceLow = 0;
        if (!bits.read(nLowBits, &nDistanceLow)) return false;
        const qint64 nDistance = (static_cast<qint64>(nDistanceSymbol) << nLowBits) + nDistanceLow + 1;
        if (nDistance > nProduced) return false;
        nProduced += nLength;
        if (nProduced > 0x40000000LL) return false;
    }
    *pnRawSize = nProduced;
    return true;
}

bool rcfIsValidRawName(const quint8 *pEntry)
{
    const qint32 nLength = pEntry[0];
    if ((nLength < 1) || (nLength > RCF_NAME_FIELD)) return false;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = pEntry[1 + i];
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XRCF::XRCF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRCF::~XRCF()
{
}

bool XRCF::parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < RCF_HEADER_SIZE + 2 + RCF_ENTRY_SIZE + 2) return false;

    const QByteArray baHeader = read_array_process(0, RCF_HEADER_SIZE + 2, pPdStruct);
    if (baHeader.size() != RCF_HEADER_SIZE + 2) return false;
    const quint8 *pHeader = reinterpret_cast<const quint8 *>(baHeader.constData());

    // "\x03RCF" with 0xA5 added to each tag byte, then "\x03" "1.0", then 00 00.
    if ((pHeader[0] != 0x03) || (pHeader[1] != 0xf7) || (pHeader[2] != 0xe8) || (pHeader[3] != 0xeb)) return false;
    if ((pHeader[4] != 0x03) || (pHeader[5] != '1') || (pHeader[6] != '.') || (pHeader[7] != '0')) return false;
    if ((pHeader[8] != 0) || (pHeader[9] != 0)) return false;
    // Bytes 10 and 11 are the first member's PKWARE DCL header.
    if (pHeader[10] > 1) return false;
    if ((pHeader[11] < 4) || (pHeader[11] > 6)) return false;

    context.nArchiveSize = context.nInputSize;

    const QByteArray baCount = read_array_process(context.nInputSize - 2, 2, pPdStruct);
    if (baCount.size() != 2) return false;
    const qint32 nCount = static_cast<qint32>(qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baCount.constData())));
    if ((nCount < 1) || (nCount > RCF_MAX_MEMBERS)) return false;

    const qint64 nDirectorySize = (static_cast<qint64>(nCount) * RCF_ENTRY_SIZE) + 2;
    if ((nDirectorySize + RCF_HEADER_SIZE) > context.nInputSize) return false;
    context.nDirectoryOffset = context.nInputSize - nDirectorySize;

    const QByteArray baDirectory = read_array_process(context.nDirectoryOffset, nDirectorySize - 2, pPdStruct);
    if (baDirectory.size() != (nDirectorySize - 2)) return false;
    const quint8 *pDirectory = reinterpret_cast<const quint8 *>(baDirectory.constData());

    // The payload extents must tile the span between the header and the
    // directory exactly; together with the name checks this is what keeps a
    // four byte magic from matching anything else.
    qint64 nOffset = RCF_HEADER_SIZE;
    QList<MEMBER> listMembers;
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const quint8 *pEntry = pDirectory + (RCF_ENTRY_SIZE * i);
        if (!rcfIsValidRawName(pEntry)) return false;
        const qint32 nPackedSize = qFromLittleEndian<qint32>(pEntry + 13);
        if (nPackedSize < 0) return false;
        if (nPackedSize > (context.nDirectoryOffset - nOffset)) return false;

        MEMBER member = {};
        member.nDataOffset = nOffset;
        member.nPackedSize = nPackedSize;
        member.nRawSize = -1;
        member.sFileName = QString::fromLatin1(reinterpret_cast<const char *>(pEntry + 1), static_cast<int>(pEntry[0]));
        listMembers.append(member);
        nOffset += nPackedSize;
    }
    if (nOffset != context.nDirectoryOffset) return false;

    if (bHeaderOnly) {
        context.listMembers = listMembers;
        *pContext = context;
        return true;
    }

    // Measure every stream so the generic DCL decoder can be handed an exact
    // uncompressed size.  A member that does not decode is fatal: publishing a
    // wrong size would silently truncate the extraction.
    for (qint32 i = 0; i < listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        MEMBER &member = listMembers[i];
        const QByteArray packed = read_array_process(member.nDataOffset, member.nPackedSize, pPdStruct);
        if (packed.size() != member.nPackedSize) return false;
        qint64 nRawSize = 0;
        if (!rcfMeasureDcl(packed, &nRawSize, pPdStruct)) return false;
        member.nRawSize = nRawSize;
    }

    context.listMembers = listMembers;
    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XRCF::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // The structural pass (magic, directory, exact tiling) is enough to answer
    // "is this an RCF?"; the per member measuring walk is deferred to listing.
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XRCF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRCF archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRCF::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRCF(pDevice);
}

QList<QString> XRCF::getSearchSignatures()
{
    return {QStringLiteral("03F7E8EB")};
}

XBinary::FT XRCF::getFileType()
{
    return FT_RCF;
}

XBinary::MODE XRCF::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRCF::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRCF::getArch()
{
    return QString();
}

QString XRCF::getFileFormatExt()
{
    return QStringLiteral("rcf");
}

QString XRCF::getFileFormatExtsString()
{
    return QStringLiteral("RCF installer archive (*.rcf)");
}

QString XRCF::getMIMEString()
{
    return QStringLiteral("application/x-rcf");
}

QString XRCF::getVersion()
{
    return QStringLiteral("1.0");
}

qint64 XRCF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, true, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XRCF::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRCF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XRCF::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XRCF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, false, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = RCF_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nPackedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nRawSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nPackedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nInputSize - context.nDirectoryOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
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

QMap<XBinary::UNPACK_PROP, QVariant> XRCF::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRCF::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, false, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("RCF installer archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XRCF::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nRawSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XRCF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nDirectoryOffset;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XRCF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
