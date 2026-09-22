/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xibmzpak.h"

#include "Algos/xdcldecoder.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 IBMZPAK_HEADER_SIZE = 8;
const qint64 IBMZPAK_NAME_SIZE = 80;
// char name[80] | quint32 packedSize | quint16 dosDate | quint16 dosTime
const qint64 IBMZPAK_ENTRY_SIZE = IBMZPAK_NAME_SIZE + 8;
const qint64 IBMZPAK_COUNT_SIZE = 2;
// A stream cannot be shorter than its own two prelude bytes plus a single byte
// holding the start of the end-of-stream code.
const qint64 IBMZPAK_MIN_PACKED_SIZE = 3;
// The member count is a quint16, so this is a hard ceiling, not a policy.
const qint32 IBMZPAK_MAX_MEMBERS = 65535;
// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext length
// is controlled by the bitstream, so the decoder must stay bounded.
const qint64 IBMZPAK_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// PKWARE DCL prelude: literal mode (0 binary / 1 Huffman) then dictionary bits.
// Only 4..6 (1K/2K/4K) are legal.  Every member of the reference corpus writes
// 0/6, but the gate accepts the whole legal range because the stream format does.
const quint8 IBMZPAK_DCL_MAX_LITERAL_MODE = 1U;
const quint8 IBMZPAK_DCL_MIN_DICT_BITS = 4U;
const quint8 IBMZPAK_DCL_MAX_DICT_BITS = 6U;
const quint16 IBMZPAK_VERSION = 1U;

bool ibmZPakRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

bool ibmZPakIsMagic(const QByteArray &baHeader)
{
    if (baHeader.size() < IBMZPAK_HEADER_SIZE) return false;
    if (!baHeader.startsWith("-ZPAK")) return false;
    if (static_cast<quint8>(baHeader.at(5)) != 0U) return false;
    const quint16 nVersion = qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(baHeader.constData()) + 6);
    return nVersion == IBMZPAK_VERSION;
}

bool ibmZPakIsDclPrelude(const QByteArray &baPrelude)
{
    if (baPrelude.size() < 2) return false;
    const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
    return (nLiteralMode <= IBMZPAK_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= IBMZPAK_DCL_MIN_DICT_BITS) &&
           (nDictBits <= IBMZPAK_DCL_MAX_DICT_BITS);
}

// The stored names are either a DOS path rooted at the install target
// ("\MYDEL.BAT") or a bare 8.3 name space-padded inside a fixed buffer
// ("epfw_dos.pif ").  Everything outside printable ASCII is a mis-parse.
bool ibmZPakIsNameField(const QByteArray &baField, QString *pName)
{
    if (baField.size() != IBMZPAK_NAME_SIZE) return false;

    const int nTerminator = baField.indexOf('\0');
    if (nTerminator <= 0) return false;
    for (int i = nTerminator; i < baField.size(); i++) {
        // A single stale byte behind the terminator would mean the 80-byte field
        // is not the fixed, zero-filled buffer this parser assumes.
        if (baField.at(i) != '\0') return false;
    }
    for (int i = 0; i < nTerminator; i++) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if ((nCharacter < 0x20U) || (nCharacter > 0x7eU)) return false;
        if ((nCharacter == '/') || (nCharacter == ':') || (nCharacter == '*') ||
            (nCharacter == '?') || (nCharacter == '"') || (nCharacter == '<') ||
            (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }

    QString sName = QString::fromLatin1(baField.constData(), nTerminator);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));

    QStringList listParts;
    const QStringList listRaw = sName.split(QLatin1Char('/'));
    const qint32 nCount = static_cast<qint32>(listRaw.size());
    for (qint32 i = 0; i < nCount; i++) {
        // The trailing blanks come from the writer's fixed 8.3 buffer, not from
        // the name; keep interior spaces, drop the padding.
        const QString sPart = listRaw.at(i).trimmed();
        if (sPart.isEmpty()) {
            // A leading '\' is how the archive spells "install root"; an empty
            // component anywhere else means a doubled separator, which the
            // writer never emits.
            if (i == 0) continue;
            return false;
        }
        if ((sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) {
            return false;
        }
        listParts.append(sPart);
    }
    if (listParts.isEmpty()) return false;

    if (pName) *pName = listParts.join(QLatin1Char('/'));
    return true;
}
}  // namespace

XIBMZPak::XIBMZPak(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIBMZPak::~XIBMZPak()
{
}

bool XIBMZPak::scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    const QByteArray baPacked = read_array_process(
        pMember->nDataOffset, pMember->nCompressedSize, pPdStruct);
    if (baPacked.size() != pMember->nCompressedSize) {
        return false;
    }

    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()),
            pMember->nCompressedSize, IBMZPAK_MAX_UNCOMPRESSED_SIZE, &nConsumed,
            &nRawSize)) {
        return false;
    }
    // VERIFIED INVARIANT over the 1774-file / 2085-member reference corpus: the
    // directory's packedSize is exactly the bitstream boundary the decoder stops
    // at.  A mismatch means the directory and the payload disagree, so the
    // recovered length cannot be trusted for extraction.
    if (nConsumed != pMember->nCompressedSize) return false;
    if ((nRawSize < 0) || (nRawSize > IBMZPAK_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }

    pMember->nUncompressedSize = nRawSize;
    pMember->bUncompressedSizeKnown = true;
    return true;
}

bool XIBMZPak::parseContext(CONTEXT *pContext, bool bScanSizes,
                            PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < IBMZPAK_HEADER_SIZE + IBMZPAK_MIN_PACKED_SIZE +
                                 IBMZPAK_ENTRY_SIZE + IBMZPAK_COUNT_SIZE) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, IBMZPAK_HEADER_SIZE, pPdStruct);
    if (!ibmZPakIsMagic(baHeader)) {
        return false;
    }
    context.nVersion = IBMZPAK_VERSION;

    const QByteArray baCount = read_array_process(
        context.nInputSize - IBMZPAK_COUNT_SIZE, IBMZPAK_COUNT_SIZE, pPdStruct);
    if (baCount.size() != IBMZPAK_COUNT_SIZE) {
        return false;
    }
    const qint32 nNumberOfMembers = static_cast<qint32>(
        qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar *>(baCount.constData())));
    if ((nNumberOfMembers < 1) || (nNumberOfMembers > IBMZPAK_MAX_MEMBERS)) {
        return false;
    }

    const qint64 nDirectorySize =
        static_cast<qint64>(nNumberOfMembers) * IBMZPAK_ENTRY_SIZE;
    const qint64 nDirectoryOffset =
        context.nInputSize - IBMZPAK_COUNT_SIZE - nDirectorySize;
    // The payload has to leave room for at least one minimal stream, so the
    // directory can never start at or before the fixed header.
    if (nDirectoryOffset < IBMZPAK_HEADER_SIZE + IBMZPAK_MIN_PACKED_SIZE) {
        return false;
    }
    context.nDirectoryOffset = nDirectoryOffset;

    const QByteArray baDirectory =
        read_array_process(nDirectoryOffset, nDirectorySize, pPdStruct);
    if (baDirectory.size() != nDirectorySize) {
        return false;
    }

    qint64 nOffset = IBMZPAK_HEADER_SIZE;
    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nEntryOffset = static_cast<qint64>(i) * IBMZPAK_ENTRY_SIZE;
        const QByteArray baEntry = baDirectory.mid(
            static_cast<int>(nEntryOffset), static_cast<int>(IBMZPAK_ENTRY_SIZE));
        if (baEntry.size() != IBMZPAK_ENTRY_SIZE) return false;

        MEMBER member = {};
        // Fixed-width name field: hand the checker all 80 bytes.  Reading one
        // byte short would reject a name that fills the buffer to its terminator.
        if (!ibmZPakIsNameField(
                baEntry.left(static_cast<int>(IBMZPAK_NAME_SIZE)),
                &member.sFileName)) {
            return false;
        }

        const uchar *pEntry =
            reinterpret_cast<const uchar *>(baEntry.constData());
        member.nCompressedSize = static_cast<qint64>(
            qFromLittleEndian<quint32>(pEntry + IBMZPAK_NAME_SIZE));
        member.nDosDate =
            qFromLittleEndian<quint16>(pEntry + IBMZPAK_NAME_SIZE + 4);
        member.nDosTime =
            qFromLittleEndian<quint16>(pEntry + IBMZPAK_NAME_SIZE + 6);
        member.nDirectoryOffset = nDirectoryOffset + nEntryOffset;
        member.nDataOffset = nOffset;
        member.nUncompressedSize = 0;
        member.bUncompressedSizeKnown = false;

        if (member.nCompressedSize < IBMZPAK_MIN_PACKED_SIZE) return false;
        if (!ibmZPakRangeWithin(nDirectoryOffset, member.nDataOffset,
                                member.nCompressedSize)) {
            return false;
        }

        const QByteArray baPrelude =
            read_array_process(member.nDataOffset, 2, pPdStruct);
        if (baPrelude.size() != 2 ||
            !ibmZPakIsDclPrelude(baPrelude)) {
            return false;
        }

        context.listMembers.append(member);
        nOffset += member.nCompressedSize;
    }

    // The streams carry no headers and no terminator: the running sum landing
    // exactly on the first directory byte is this format's only integrity check,
    // and slack there would mean the directory does not describe the payload.
    if (nOffset != nDirectoryOffset) return false;

    if (bScanSizes) {
        const qint32 nCount = static_cast<qint32>(context.listMembers.size());
        for (qint32 i = 0; i < nCount; i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            // A member the decoder cannot measure stays in the listing with its
            // size unknown; methodToHandleMethod() then reports it as UNKNOWN so
            // extraction refuses it instead of writing a truncated file.
            scanMemberSize(&context.listMembers[i], pPdStruct);
            if (!guardedSource) return false;
        }
    }

    context.nArchiveSize = context.nInputSize;
    context.nFirstMemberOffset = context.listMembers.first().nDataOffset;
    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XIBMZPak::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XIBMZPak::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIBMZPak archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIBMZPak::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIBMZPak(pDevice);
}

QList<QString> XIBMZPak::getSearchSignatures()
{
    // Pin the reserved zero and the version word too: "-ZPAK" alone is short
    // enough to turn up inside unrelated payloads.
    return {QStringLiteral("'-ZPAK'000100")};
}

XBinary::FT XIBMZPak::getFileType()
{
    return FT_IBM_ZPAK;
}

XBinary::MODE XIBMZPak::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIBMZPak::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIBMZPak::getArch()
{
    return QString();
}

QString XIBMZPak::getFileFormatExt()
{
    // The packed files keep the DOS convention of replacing the last extension
    // character with an underscore (AV.IN_, NAT.EX_), so the container has no
    // extension of its own to report.
    return QString();
}

QString XIBMZPak::getFileFormatExtsString()
{
    return QStringLiteral("IBM ZPAK archive");
}

QString XIBMZPak::getMIMEString()
{
    return QStringLiteral("application/x-ibm-zpak");
}

QString XIBMZPak::getVersion()
{
    return QString::number(IBMZPAK_VERSION);
}

qint64 XIBMZPak::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XIBMZPak::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIBMZPak::getMemoryMap(MAPMODE mapMode,
                                            PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_TABLE, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QString XIBMZPak::methodToString(const MEMBER &member)
{
    if (!member.bUncompressedSizeKnown) {
        return QStringLiteral("PKWARE DCL Implode/TTCOMP (undecodable)");
    }
    return QStringLiteral("PKWARE DCL Implode/TTCOMP");
}

XBinary::HANDLE_METHOD XIBMZPak::methodToHandleMethod(const MEMBER &member)
{
    // decPkwareDcl() consumes the packed buffer with its two-byte prelude still
    // attached, but it takes the plaintext length as an INPUT.  Without a
    // measured length the only honest answer is UNKNOWN.
    if (!member.bUncompressedSizeKnown) return HANDLE_METHOD_UNKNOWN;
    return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
}

bool XIBMZPak::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIBMZPak::getFileParts(quint32 nFileParts, qint32 nLimit,
                                             PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = IBMZPAK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    const qint32 nCount = static_cast<qint32>(context.listMembers.size());
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
            break;
        }
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
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
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_TABLE) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_TABLE;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nInputSize - context.nDirectoryOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XIBMZPak::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIBMZPak::initUnpack(UNPACK_STATE *pState,
                          const QMap<UNPACK_PROP, QVariant> &mapProperties,
                          PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) {
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
    // bScanSizes = true: the extraction path needs the plaintext length of every
    // member, and the container does not store it anywhere.
    if (!parseContext(pContext, true, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("IBM ZPAK archive; PKWARE DCL Implode members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XIBMZPak::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) {
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
    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        const QDateTime dtModified =
            dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        }
    }
    // No checksum property: the container carries none, anywhere.
    return result;
}

bool XIBMZPak::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nDirectoryOffset;
    return false;
}

bool XIBMZPak::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
