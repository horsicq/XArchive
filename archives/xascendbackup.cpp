/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xascendbackup.h"

#include "Algos/xdcldecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// uint16le nameLen | char name[nameLen] | uint32le packedSize | payload.
const qint64 ASCEND_NAME_LENGTH_SIZE = 2;
const qint64 ASCEND_PACKED_SIZE_SIZE = 4;
const qint64 ASCEND_FIXED_HEADER_SIZE =
    ASCEND_NAME_LENGTH_SIZE + ASCEND_PACKED_SIZE_SIZE;
// The names are DOS 8.3 and never carry a path, so 12 is a hard ceiling.  Read
// as a uint16 (not uint8 + flag byte): the high byte is a structural zero on
// every record and throwing it away would lose the cheapest reject this format
// offers.
const qint32 ASCEND_MAX_NAME_SIZE = 12;
const qint32 ASCEND_MAX_STEM_SIZE = 8;
const qint32 ASCEND_MAX_EXT_SIZE = 3;
// The smallest conceivable record: 2 + 1 name byte + 4 + a 3-byte DCL stream.
const qint64 ASCEND_MIN_ARCHIVE_SIZE = ASCEND_FIXED_HEADER_SIZE + 1 + 3;
const qint64 ASCEND_MIN_PACKED_SIZE = 3;
const qint32 ASCEND_MAX_MEMBERS = 100000;
// A single-record file is indistinguishable from a random length/name/size
// coincidence, so require the chain to prove itself at least twice.
const qint32 ASCEND_MIN_MEMBERS = 2;
// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext length
// is attacker-controlled through the bitstream, so the decoder must stay bounded.
const qint64 ASCEND_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// PKWARE DCL prelude: literal mode (0 binary / 1 Huffman) then dictionary bits.
// Only 4..6 (1K/2K/4K) are legal.  Ascend always writes 1/6, but the gate accepts
// the whole legal range because that is what the stream format permits.
const quint8 ASCEND_DCL_MAX_LITERAL_MODE = 1U;
const quint8 ASCEND_DCL_MIN_DICT_BITS = 4U;
const quint8 ASCEND_DCL_MAX_DICT_BITS = 6U;

bool ascendRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool ascendIsNameCharacter(char cCharacter)
{
    const quint8 nCharacter = static_cast<quint8>(cCharacter);
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;
    // The remainder of the DOS 8.3 charset.  Path separators, spaces and every
    // byte outside 0x20..0x7e are excluded on purpose: this format has no
    // directories, so a name containing one is a mis-parse, not a subfolder.
    switch (nCharacter) {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '-':
        case '@':
        case '^':
        case '_':
        case '`':
        case '{':
        case '}':
        case '~': return true;
        default: return false;
    }
}

bool ascendIsValidName(const QByteArray &baName)
{
    const qint32 nSize = static_cast<qint32>(baName.size());
    if ((nSize < 1) || (nSize > ASCEND_MAX_NAME_SIZE)) return false;

    qint32 nStemLength = 0;
    qint32 nExtLength = 0;
    bool bHasDot = false;
    for (qint32 i = 0; i < nSize; i++) {
        const char cCharacter = baName.at(i);
        if (cCharacter == '.') {
            // A second dot, or a leading one, cannot occur in a DOS 8.3 name.
            if (bHasDot || (i == 0)) return false;
            bHasDot = true;
            continue;
        }
        if (!ascendIsNameCharacter(cCharacter)) return false;
        if (bHasDot) {
            nExtLength++;
        } else {
            nStemLength++;
        }
    }

    if ((nStemLength < 1) || (nStemLength > ASCEND_MAX_STEM_SIZE)) return false;
    if (nExtLength > ASCEND_MAX_EXT_SIZE) return false;
    // A trailing dot with no extension is not something the Ascend writer emits.
    if (bHasDot && (nExtLength < 1)) return false;
    return true;
}

bool ascendIsDclPrelude(const QByteArray &baPrelude)
{
    if (baPrelude.size() < 2) return false;
    const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
    return (nLiteralMode <= ASCEND_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= ASCEND_DCL_MIN_DICT_BITS) &&
           (nDictBits <= ASCEND_DCL_MAX_DICT_BITS);
}
}  // namespace

XAscendBackup::XAscendBackup(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAscendBackup::~XAscendBackup()
{
}

bool XAscendBackup::scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XAscendBackup> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baPacked = read_array_process(
        pMember->nDataOffset, pMember->nCompressedSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baPacked.size() != pMember->nCompressedSize) {
        return false;
    }

    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()),
            pMember->nCompressedSize, ASCEND_MAX_UNCOMPRESSED_SIZE, &nConsumed,
            &nRawSize)) {
        return false;
    }
    // VERIFIED INVARIANT on the reference volume: for all 46 members the declared
    // packed size is exactly the bitstream boundary the decoder stops at.  A
    // mismatch means the record chain and the payload disagree, so the recovered
    // length cannot be trusted for extraction.
    if (nConsumed != pMember->nCompressedSize) return false;
    if ((nRawSize < 0) || (nRawSize > ASCEND_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }

    pMember->nUncompressedSize = nRawSize;
    pMember->bUncompressedSizeKnown = true;
    return true;
}

bool XAscendBackup::parseContext(CONTEXT *pContext, bool bScanSizes,
                                 PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XAscendBackup> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ASCEND_MIN_ARCHIVE_SIZE) return false;

    qint64 nOffset = 0;
    bool bClosedOnEof = false;
    while (context.listMembers.size() < ASCEND_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!ascendRangeWithin(context.nInputSize, nOffset,
                               ASCEND_NAME_LENGTH_SIZE)) {
            return false;
        }
        const QByteArray baNameLength = read_array_process(
            nOffset, ASCEND_NAME_LENGTH_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baNameLength.size() != ASCEND_NAME_LENGTH_SIZE) {
            return false;
        }
        const qint64 nNameSize = static_cast<qint64>(
            qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(
                baNameLength.constData())));
        if ((nNameSize < 1) || (nNameSize > ASCEND_MAX_NAME_SIZE)) return false;

        const qint64 nHeaderSize = ASCEND_FIXED_HEADER_SIZE + nNameSize;
        if (!ascendRangeWithin(context.nInputSize, nOffset, nHeaderSize)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, nHeaderSize, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baHeader.size() != nHeaderSize) {
            return false;
        }

        // The name is NOT NUL-terminated; the length word is the only authority.
        const QByteArray baName =
            baHeader.mid(static_cast<int>(ASCEND_NAME_LENGTH_SIZE),
                         static_cast<int>(nNameSize));
        if (!ascendIsValidName(baName)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nHeaderSize;
        member.nDataOffset = nOffset + nHeaderSize;
        member.nCompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(
                reinterpret_cast<const uchar *>(baHeader.constData()) +
                ASCEND_NAME_LENGTH_SIZE + nNameSize));
        member.sFileName = QString::fromLatin1(baName);
        // The container never stores the plaintext length; it is filled in below
        // only when the caller asked for it.
        member.nUncompressedSize = 0;
        member.bUncompressedSizeKnown = false;

        if (member.nCompressedSize < ASCEND_MIN_PACKED_SIZE) return false;
        if (!ascendRangeWithin(context.nInputSize, member.nDataOffset,
                               member.nCompressedSize)) {
            return false;
        }

        const QByteArray baPrelude =
            read_array_process(member.nDataOffset, 2, pPdStruct);
        if (!guardedThis || !guardedSource || baPrelude.size() != 2 ||
            !ascendIsDclPrelude(baPrelude)) {
            return false;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;

        // There is no terminator record and no global size field: the chain ends
        // by landing exactly on EOF.  Slack is a reject, not an overlay - the
        // exact tiling IS the signature here, and tolerating a tail (or a
        // continuation into a .001 volume) would destroy the whole margin that
        // makes a magic-less structural gate safe.
        if (nOffset == context.nInputSize) {
            bClosedOnEof = true;
            break;
        }
    }

    if (!bClosedOnEof || (context.listMembers.size() < ASCEND_MIN_MEMBERS)) {
        return false;
    }

    // Only now, on a file that already tiles exactly with 8.3 names and DCL
    // preludes throughout, is it worth paying for a real decode.  Doing this
    // before the chain closed would run a decoder over every probed file.
    if (!scanMemberSize(&context.listMembers.first(), pPdStruct) ||
        !guardedThis || !guardedSource) {
        return false;
    }

    if (bScanSizes) {
        const qint32 nCount = static_cast<qint32>(context.listMembers.size());
        for (qint32 i = 1; i < nCount; i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            // A member the decoder cannot measure stays in the listing with its
            // size unknown; methodToHandleMethod() then reports it as UNKNOWN so
            // extraction refuses it instead of writing a truncated file.
            scanMemberSize(&context.listMembers[i], pPdStruct);
            if (!guardedThis || !guardedSource) return false;
        }
    }

    context.nArchiveSize = nOffset;
    context.nFirstMemberOffset = context.listMembers.first().nHeaderOffset;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XAscendBackup::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XAscendBackup::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAscendBackup archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAscendBackup::createInstance(QIODevice *pDevice, bool bIsImage,
                                       XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAscendBackup(pDevice);
}

// getSearchSignatures() is deliberately NOT overridden: the first two bytes of
// the file are a name length (0x0B, 0x0A, 0x05, 0x04 ... all occur in the very
// same archive), so any byte pattern registered for this format would be wrong.

XBinary::FT XAscendBackup::getFileType()
{
    return FT_ASCEND_BACKUP;
}

XBinary::MODE XAscendBackup::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAscendBackup::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XAscendBackup::getArch()
{
    return QString();
}

QString XAscendBackup::getFileFormatExt()
{
    return QStringLiteral("000");
}

QString XAscendBackup::getFileFormatExtsString()
{
    return QStringLiteral("Ascend backup volume (*.000)");
}

QString XAscendBackup::getMIMEString()
{
    return QStringLiteral("application/x-ascend-backup");
}

QString XAscendBackup::getVersion()
{
    // The "4.2b" product string lives inside the decompressed *.FIL members, not
    // in the container, and the container itself is unversioned.
    return QString();
}

qint64 XAscendBackup::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAscendBackup::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAscendBackup::getMemoryMap(MAPMODE mapMode,
                                                 PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XAscendBackup::methodToString(const MEMBER &member)
{
    if (!member.bUncompressedSizeKnown) {
        return QStringLiteral("PKWARE DCL Implode/TTCOMP (undecodable)");
    }
    return QStringLiteral("PKWARE DCL Implode/TTCOMP");
}

XBinary::HANDLE_METHOD XAscendBackup::methodToHandleMethod(const MEMBER &member)
{
    // decPkwareDcl() consumes the packed buffer with its own two-byte prelude
    // still attached, but it takes the plaintext length as an INPUT.  Without a
    // measured length the only honest answer is UNKNOWN.
    if (!member.bUncompressedSizeKnown) return HANDLE_METHOD_UNKNOWN;
    return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
}

bool XAscendBackup::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XAscendBackup::getFileParts(quint32 nFileParts,
                                                  qint32 nLimit,
                                                  PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return result;

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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      methodToHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member));
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
        // parseContext() only accepts a chain that lands exactly on EOF, so this
        // branch cannot fire today; it is kept so the part list stays correct if
        // the acceptance rule is ever relaxed for multi-volume sets.
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

QMap<XBinary::UNPACK_PROP, QVariant> XAscendBackup::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAscendBackup::initUnpack(UNPACK_STATE *pState,
                               const QMap<UNPACK_PROP, QVariant> &mapProperties,
                               PDSTRUCT *pPdStruct)
{
    QPointer<XAscendBackup> guardedThis(this);
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    // bScanSizes = true: the extraction path needs the plaintext length of every
    // member, and the container does not store it.
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Ascend backup volume; PKWARE DCL Implode members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XAscendBackup::infoCurrent(UNPACK_STATE *pState,
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
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                methodToHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No MTIME, attribute or checksum properties: the six header bytes plus the
    // name account for every byte of the record, and the format carries none.
    return result;
}

bool XAscendBackup::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XAscendBackup::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
