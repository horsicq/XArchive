/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarcv4.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const quint16 ARCV4_VERSION = 0x0400U;  // 0x0110 is ARCV 1.10, 0x0200 is ARCV 2
const quint16 ARCV4_SUBVARIANT_PATHS = 1U;   // stores absolute build paths
const quint16 ARCV4_SUBVARIANT_MACROS = 5U;  // adds "$(SystemDir)\..." macros

// The archive header has no length field anywhere; the first chunk always
// starts at this fixed offset.
const qint64 ARCV4_HEADER_SIZE = 0x79C;
const qint64 ARCV4_ZERO_BLOCK_OFFSET = 0x08;
const qint64 ARCV4_ZERO_BLOCK_SIZE = 0x84;
const qint64 ARCV4_FILL_BLOCK_OFFSET = 0x9C;
// The reserved block is 1280 bytes of 0xAA on every sample, but gating on the
// whole run would reject any writer generation that filled it differently.
// Probing the first 64 bytes already contributes 512 bits of fixed structure.
const qint64 ARCV4_FILL_PROBE_SIZE = 64;
const qint64 ARCV4_TAIL_ZERO_OFFSET = 0x59C;
const qint64 ARCV4_TAIL_ZERO_PROBE_SIZE = 16;

const qint64 ARCV4_CHUNK_PROLOGUE_SIZE = 16;
const qint64 ARCV4_FILE_HEADER_SIZE = 16;
const qint64 ARCV4_DATA_HEADER_SIZE = 32;
const qint64 ARCV4_EOFM_HEADER_SIZE = 16;
// 4 + tagLen + 4 + nameLen + the 32-byte fixed tail.
const qint64 ARCV4_FILE_BODY_MIN_SIZE = 40;
const qint64 ARCV4_FILE_BODY_MAX_SIZE = 65536;

const quint8 ARCV4_CHUNK_TYPE_FILE = 1U;
const quint8 ARCV4_CHUNK_TYPE_EOFM = 3U;
const quint8 ARCV4_DATA_TYPE_SPANNED = 1U;  // last member of a split volume
const quint8 ARCV4_DATA_TYPE_NORMAL = 5U;

const quint32 ARCV4_METHOD_STORED = 0U;
const quint32 ARCV4_METHOD_COMPRESSED = 2U;
// packedSize is not authoritative: a spanned member stores 0xFFFFFFFF there.
// Widening that to qint64 and reading it would walk off the end of the file.
const quint32 ARCV4_PACKEDSIZE_SPANNED = 0xFFFFFFFFU;

const qint32 ARCV4_MAX_MEMBERS = 100000;
const quint32 ARCV4_MAX_TAG_SIZE = 256U;
const quint32 ARCV4_MAX_NAME_SIZE = 4096U;

bool arcv4RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool arcv4IsFilled(const QByteArray &baData, qint64 nOffset, qint64 nSize,
                   char cValue)
{
    if (nOffset < 0 || nSize < 0 || baData.size() < nOffset + nSize) {
        return false;
    }
    for (qint64 i = 0; i < nSize; i++) {
        if (baData.at(static_cast<int>(nOffset + i)) != cValue) return false;
    }
    return true;
}

bool arcv4IsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        // The name is a byte string in the builder machine's ANSI code page,
        // not ASCII.  "Catalunya_R\xe0dio_(Spanish)" is a real member name in
        // the reference corpus, and rejecting that 0xe0 threw away the entire
        // 48-member archive it lives in.  Control bytes and DEL still cannot
        // occur in a Windows path, so those stay fatal.
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
    }
    return true;
}
}  // namespace

XARCV4::XARCV4(QIODevice *pDevice) : XArchive(pDevice)
{
}

XARCV4::~XARCV4()
{
}

bool XARCV4::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XARCV4> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ARCV4_HEADER_SIZE + ARCV4_EOFM_HEADER_SIZE) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, ARCV4_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baHeader.size() != ARCV4_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if (memcmp(pHeader, "ARCV", 4) != 0) return false;
    if (qFromLittleEndian<quint16>(pHeader + 4) != ARCV4_VERSION) return false;
    context.nSubVariant = qFromLittleEndian<quint16>(pHeader + 6);
    if ((context.nSubVariant != ARCV4_SUBVARIANT_PATHS) &&
        (context.nSubVariant != ARCV4_SUBVARIANT_MACROS)) {
        return false;
    }
    // "ARCV" plus a version word is only six bytes of magic, and the sibling
    // ARCV 2 family shares them.  The fixed zero/0xAA/zero runs below are what
    // actually separates v4 from anything else that starts with the tag.
    if (!arcv4IsFilled(baHeader, ARCV4_ZERO_BLOCK_OFFSET,
                       ARCV4_ZERO_BLOCK_SIZE, '\0') ||
        !arcv4IsFilled(baHeader, ARCV4_FILL_BLOCK_OFFSET,
                       ARCV4_FILL_PROBE_SIZE, static_cast<char>(0xAA)) ||
        !arcv4IsFilled(baHeader, ARCV4_TAIL_ZERO_OFFSET,
                       ARCV4_TAIL_ZERO_PROBE_SIZE, '\0')) {
        return false;
    }

    qint64 nOffset = ARCV4_HEADER_SIZE;
    while (context.listMembers.size() < ARCV4_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!arcv4RangeWithin(context.nInputSize, nOffset,
                              ARCV4_CHUNK_PROLOGUE_SIZE)) {
            return false;
        }
        const QByteArray baChunk = read_array_process(
            nOffset, ARCV4_CHUNK_PROLOGUE_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baChunk.size() != ARCV4_CHUNK_PROLOGUE_SIZE) {
            return false;
        }
        const uchar *pChunk =
            reinterpret_cast<const uchar *>(baChunk.constData());
        // Only the low byte of the type word is a chunk kind.  Its upper 24
        // bits are a per-archive constant (0x021A00, 0x0075F1, 0x0074F1,
        // 0x0074F4 observed) and must not be part of any gate.
        const quint8 nChunkType =
            static_cast<quint8>(qFromLittleEndian<quint32>(pChunk + 4) & 0xffU);
        const quint32 nChunkHeaderSize = qFromLittleEndian<quint32>(pChunk + 8);
        const quint32 nChunkRecordSize =
            qFromLittleEndian<quint32>(pChunk + 12);

        if (memcmp(pChunk, "EOFM", 4) == 0) {
            if (context.listMembers.isEmpty() ||
                (nChunkType != ARCV4_CHUNK_TYPE_EOFM) ||
                (nChunkHeaderSize != ARCV4_EOFM_HEADER_SIZE) ||
                (nChunkRecordSize != 0)) {
                return false;
            }
            context.nArchiveSize = nOffset + ARCV4_EOFM_HEADER_SIZE;
            context.nFirstMemberOffset =
                context.listMembers.first().nHeaderOffset;
            *pContext = context;
            return guardedThis && guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }
        if (memcmp(pChunk, "FILE", 4) != 0) return false;
        if ((nChunkType != ARCV4_CHUNK_TYPE_FILE) ||
            (nChunkHeaderSize != ARCV4_FILE_HEADER_SIZE) ||
            (nChunkRecordSize < ARCV4_FILE_BODY_MIN_SIZE) ||
            (nChunkRecordSize > ARCV4_FILE_BODY_MAX_SIZE)) {
            return false;
        }

        const qint64 nBodyOffset = nOffset + ARCV4_FILE_HEADER_SIZE;
        const qint64 nBodySize = static_cast<qint64>(nChunkRecordSize);
        if (!arcv4RangeWithin(context.nInputSize, nBodyOffset, nBodySize)) {
            return false;
        }
        const QByteArray baBody =
            read_array_process(nBodyOffset, nBodySize, pPdStruct);
        if (!guardedThis || !guardedSource || baBody.size() != nBodySize) {
            return false;
        }
        const uchar *pBody =
            reinterpret_cast<const uchar *>(baBody.constData());

        // The group tag is a length-prefixed string ("Main" on every member
        // seen, but the field is clearly variable) and the name that follows
        // it is NOT NUL-terminated.  Both lengths are bounded against the
        // record before anything is copied out.
        const quint32 nTagSize = qFromLittleEndian<quint32>(pBody);
        if (nTagSize > ARCV4_MAX_TAG_SIZE ||
            static_cast<qint64>(nTagSize) + 4 > nBodySize) {
            return false;
        }
        qint64 nBodyPos = 4 + static_cast<qint64>(nTagSize);
        if (nBodySize - nBodyPos < 4) return false;
        const quint32 nNameSize = qFromLittleEndian<quint32>(pBody + nBodyPos);
        nBodyPos += 4;
        if (nNameSize > ARCV4_MAX_NAME_SIZE ||
            static_cast<qint64>(nNameSize) > nBodySize - nBodyPos) {
            return false;
        }
        const QByteArray baName =
            baBody.mid(static_cast<int>(nBodyPos), static_cast<int>(nNameSize));
        nBodyPos += static_cast<qint64>(nNameSize);
        if (!arcv4IsValidName(baName)) return false;
        // The fixed 32-byte tail must fill the record exactly.  Trailing bytes
        // would mean the tail fields are being read at the wrong offsets.
        if (nBodySize - nBodyPos != 32) return false;

        const uchar *pTail = pBody + nBodyPos;
        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nUncompressedSize = qFromLittleEndian<quint32>(pTail);
        const quint32 nMemberFlags = qFromLittleEndian<quint32>(pTail + 4);
        member.nMTime = qFromLittleEndian<quint64>(pTail + 8);
        member.nFileVersionLS = qFromLittleEndian<quint32>(pTail + 16);
        member.nFileVersionMS = qFromLittleEndian<quint32>(pTail + 20);
        const quint32 nPackedSize = qFromLittleEndian<quint32>(pTail + 24);
        member.nMethod = qFromLittleEndian<quint32>(pTail + 28);
        // NOT reserved: a small per-member install-flag word.  It is 0 on
        // 864 of the 867 members in the reference corpus and 1 or 3 on the
        // other three, all inside one Eschalon Setup EPSF payload, and
        // demanding zero rejected that whole archive.  Bounding it to a byte
        // keeps the field contributing to the gate without asserting a meaning
        // for bits no sample exercises.
        if (nMemberFlags > 0xffU) return false;
        if ((member.nMethod != ARCV4_METHOD_STORED) &&
            (member.nMethod != ARCV4_METHOD_COMPRESSED)) {
            return false;
        }
        member.sGroupName =
            QString::fromLatin1(baBody.mid(4, static_cast<int>(nTagSize)));
        member.sFileName = QString::fromLatin1(baName)
                               .replace(QLatin1Char('\\'), QLatin1Char('/'));

        // Every FILE chunk is immediately followed by its DATA chunk; the
        // format has no index, so a break in that alternation is fatal.
        const qint64 nDataChunkOffset = nBodyOffset + nBodySize;
        if (!arcv4RangeWithin(context.nInputSize, nDataChunkOffset,
                              ARCV4_DATA_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baData = read_array_process(
            nDataChunkOffset, ARCV4_DATA_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baData.size() != ARCV4_DATA_HEADER_SIZE) {
            return false;
        }
        const uchar *pData =
            reinterpret_cast<const uchar *>(baData.constData());
        if (memcmp(pData, "DATA", 4) != 0) return false;
        const quint8 nDataType =
            static_cast<quint8>(qFromLittleEndian<quint32>(pData + 4) & 0xffU);
        if ((nDataType != ARCV4_DATA_TYPE_NORMAL) &&
            (nDataType != ARCV4_DATA_TYPE_SPANNED)) {
            return false;
        }
        if (qFromLittleEndian<quint32>(pData + 8) != ARCV4_DATA_HEADER_SIZE) {
            return false;
        }
        member.nCompressedSize = qFromLittleEndian<quint32>(pData + 12);
        if (qFromLittleEndian<quint32>(pData + 16) != 0 ||
            qFromLittleEndian<quint32>(pData + 24) != 0 ||
            qFromLittleEndian<quint32>(pData + 28) != 0) {
            return false;
        }
        member.nCRC32 = qFromLittleEndian<quint32>(pData + 20);
        member.nHeaderSize = nDataChunkOffset + ARCV4_DATA_HEADER_SIZE - nOffset;
        member.nDataOffset = nDataChunkOffset + ARCV4_DATA_HEADER_SIZE;
        member.bSpanned = (nPackedSize == ARCV4_PACKEDSIZE_SPANNED) ||
                          (nDataType == ARCV4_DATA_TYPE_SPANNED);
        // Size the payload from the DATA record, never from packedSize.
        if (!member.bSpanned &&
            (static_cast<qint64>(nPackedSize) != member.nCompressedSize)) {
            return false;
        }
        if (!arcv4RangeWithin(context.nInputSize, member.nDataOffset,
                              member.nCompressedSize)) {
            return false;
        }
        // A stored member is the anchor that keeps the walk honest: with no
        // codec involved its two size fields must agree exactly.
        if ((member.nMethod == ARCV4_METHOD_STORED) && !member.bSpanned &&
            (member.nCompressedSize != member.nUncompressedSize)) {
            return false;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    return false;
}

bool XARCV4::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XARCV4::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARCV4 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XARCV4::createInstance(QIODevice *pDevice, bool bIsImage,
                                XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XARCV4(pDevice);
}

QList<QString> XARCV4::getSearchSignatures()
{
    // Include the version word: "ARCV" alone also matches ARCV 1.10 and the
    // much larger ARCV 2 family.
    return {QStringLiteral("'ARCV'0004")};
}

XBinary::FT XARCV4::getFileType()
{
    return FT_ARCV4;
}

XBinary::MODE XARCV4::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XARCV4::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XARCV4::getArch()
{
    return QString();
}

QString XARCV4::getFileFormatExt()
{
    return QStringLiteral("arv");
}

QString XARCV4::getFileFormatExtsString()
{
    return QStringLiteral("Eschalon Setup ARCV 4 (*.arv)");
}

QString XARCV4::getMIMEString()
{
    return QStringLiteral("application/x-arcv4");
}

QString XARCV4::getVersion()
{
    return QStringLiteral("4.00");
}

qint64 XARCV4::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XARCV4::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XARCV4::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XARCV4::methodToString(quint32 nMethod, bool bSpanned)
{
    QString sName;
    if (nMethod == ARCV4_METHOD_STORED) {
        sName = QStringLiteral("Stored");
    } else if (nMethod == ARCV4_METHOD_COMPRESSED) {
        sName = QStringLiteral("Adaptive Huffman + LZ77");
    } else {
        sName = QStringLiteral("Unknown");
    }
    if (bSpanned) {
        sName += QStringLiteral(", continued on the next volume");
    }
    return QStringLiteral("ARCV4 %1 %2").arg(nMethod).arg(sName);
}

XBinary::HANDLE_METHOD XARCV4::methodToHandleMethod(quint32 nMethod,
                                                    bool bSpanned)
{
    // A spanned member holds only the leading fragment of its stream, so even
    // a stored one must not be advertised as extractable.
    if (bSpanned) return HANDLE_METHOD_UNKNOWN;
    if (nMethod == ARCV4_METHOD_STORED) return HANDLE_METHOD_STORE;
    // Method 2 is XARCV4Decoder (Algos/xarcv4decoder.h): adaptive Huffman over a
    // 3245-symbol alphabet driving LZ77 with a 32 KiB window, LSB-first bits.
    // It shares nothing with the LZHUF behind HANDLE_METHOD_ARCV_LZHUF except
    // the family name.
    if (nMethod == ARCV4_METHOD_COMPRESSED) return HANDLE_METHOD_ARCV4_M2;
    return HANDLE_METHOD_UNKNOWN;
}

bool XARCV4::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XARCV4::getFileParts(quint32 nFileParts, qint32 nLimit,
                                           PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ARCV4_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Archive header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(
                FPART_PROP_HANDLEMETHOD,
                methodToHandleMethod(member.nMethod, member.bSpanned));
            part.mapProperties.insert(
                FPART_PROP_REPORTEDMETHOD,
                methodToString(member.nMethod, member.bSpanned));
            part.mapProperties.insert(FPART_PROP_TYPE, member.nMethod);
            // The CRC covers the UNPACKED member (verified against every
            // member the reference extractor could produce), so it may be
            // published as a result checksum.
            if (!member.bSpanned) {
                part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
                part.mapProperties.insert(
                    FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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
    if ((nFileParts & FILEPART_OVERLAY) &&
        context.nArchiveSize < context.nInputSize &&
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

QMap<XBinary::UNPACK_PROP, QVariant> XARCV4::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XARCV4::initUnpack(UNPACK_STATE *pState,
                        const QMap<UNPACK_PROP, QVariant> &mapProperties,
                        PDSTRUCT *pPdStruct)
{
    QPointer<XARCV4> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("ARCV 4.00 installer archive; a member continued on the next "
           "installation volume cannot be completed from this file"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XARCV4::infoCurrent(UNPACK_STATE *pState,
                                           PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_STREAMOFFSET, member.nDataOffset);
    result.mapProperties.insert(FPART_PROP_STREAMSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        methodToHandleMethod(member.nMethod, member.bSpanned));
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        methodToString(member.nMethod, member.bSpanned));
    result.mapProperties.insert(FPART_PROP_TYPE, member.nMethod);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!member.bSpanned) {
        result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
        result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                    CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    } else {
        // The remaining bytes live on an installation volume that is not part
        // of this file, so this member can never be completed from it alone.
        result.mapProperties.insert(
            FPART_PROP_INFO,
            tr("Split member; continued on the next installation volume"));
    }
    // sGroupName is deliberately not published as FPART_PROP_PREFIX: that
    // property is prepended to the output path (see XTAR), and the ARCV group
    // tag ("Main") is an installer grouping label, not a directory.
    const QDateTime dtModified = winFileTimeToQDateTime(member.nMTime);
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }
    return result;
}

bool XARCV4::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XARCV4::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
