/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xzpaksfxarchive.h"

#include "Algos/xdcldecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

// Everything between the two BEGIN/END markers below is the container parser and
// nothing else: every acceptance test this reader applies to a file lives in
// here.  The corpus harness slices this exact region out of this file and
// compiles it against a device shim, so the measurement runs the shipped gates
// instead of a re-description of them.  Keep new gates inside the markers.
// ===== BEGIN ZPAKSFX PARSER =====
namespace {
const qint64 ZPAKSFX_HEADER_SIZE = 8;
// "-ZPAK" 00 | u16 version | the version gate byte, which is the first payload
// byte for version 1 and the first record's tag for version 2.
const qint64 ZPAKSFX_HEADER_PROBE_SIZE = 10;
const qint64 ZPAKSFX_V1_TRAILER_SIZE = 22;
const qint64 ZPAKSFX_V2_TRAILER_SIZE = 38;
// char name[102] | i32 packedSize | i32 reserved | u16 dosDate | u16 dosTime
const qint64 ZPAKSFX_V1_NAME_SIZE = 102;
const qint64 ZPAKSFX_V1_ENTRY_SIZE = ZPAKSFX_V1_NAME_SIZE + 12;
// tag | unused | i32 packedSize | writer scratch[4] | u16 dosDate |
// u16 dosTime | i16 nameSize.  The four bytes at +6 are deliberately NOT read:
// see the layout note in the header for what the reference implementation does and
// does not load out of this record.
const qint64 ZPAKSFX_V2_RECORD_SIZE = 16;
const quint8 ZPAKSFX_V2_RECORD_TAG = 0x0aU;
// Version 2 leaves the head of an unwritten record between the last payload and
// the trailer.  Every corpus file leaves exactly six bytes; accept nothing
// larger, because slack is otherwise indistinguishable from a mis-parse.
const qint64 ZPAKSFX_V2_MAX_GAP = 6;
// A stream cannot be shorter than its own two prelude bytes plus a single byte
// holding the start of the end-of-stream code.
const qint64 ZPAKSFX_MIN_PACKED_SIZE = 3;
// The version 1 count is a u16, so this is a hard ceiling there; version 2
// spends an i32 on it, and this keeps the member list bounded.
const qint32 ZPAKSFX_MAX_MEMBERS = 65535;
// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext length
// is controlled by the bitstream, so the decoder must stay bounded.
const qint64 ZPAKSFX_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// PKWARE DCL prelude: literal mode (0 binary / 1 Huffman) then dictionary bits.
// Only 4..6 (1K/2K/4K) are legal.  Every member of the reference corpus writes
// 0/6, but the gate accepts the whole legal range because the format does.
const quint8 ZPAKSFX_DCL_MAX_LITERAL_MODE = 1U;
const quint8 ZPAKSFX_DCL_MIN_DICT_BITS = 4U;
const quint8 ZPAKSFX_DCL_MAX_DICT_BITS = 6U;

bool zpakSfxIsMagic(const QByteArray &baHeader, quint16 nVersion)
{
    if (baHeader.size() < ZPAKSFX_HEADER_PROBE_SIZE) return false;
    if (!baHeader.startsWith("-ZPAK")) return false;
    if (static_cast<quint8>(baHeader.at(5)) != 0U) return false;
    const quint16 nStored = qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(baHeader.constData()) + 6);
    if (nStored != nVersion) return false;

    const quint8 nGate = static_cast<quint8>(baHeader.at(8));
    if (nVersion == 2U) return nGate == ZPAKSFX_V2_RECORD_TAG;

    // Version 1 puts the first member's payload straight after the header, so
    // its gate is the DCL prelude's literal mode plus the dictionary code.
    const quint8 nDictBits = static_cast<quint8>(baHeader.at(9));
    return (nGate <= ZPAKSFX_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= ZPAKSFX_DCL_MIN_DICT_BITS) &&
           (nDictBits <= ZPAKSFX_DCL_MAX_DICT_BITS);
}
}  // namespace

// The stored names are DOS paths rooted at the install target and they mix the
// two separators inside a single name ("\INSTALL/DLL\FXSINDEX.DL_"), so both are
// folded to '/' before the components are checked.  Everything outside
// printable ASCII is a mis-parse.
bool XZPakSFXArchive::isNameField(const QByteArray &baName, QString *pName)
{
    if (baName.isEmpty()) return false;

    const qint32 nSize = static_cast<qint32>(baName.size());
    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if ((nCharacter < 0x20U) || (nCharacter > 0x7eU)) return false;
        if ((nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') ||
            (nCharacter == '"') || (nCharacter == '<') || (nCharacter == '>') ||
            (nCharacter == '|')) {
            return false;
        }
    }

    QString sName = QString::fromLatin1(baName.constData(), nSize);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));

    QStringList listParts;
    const QStringList listRaw = sName.split(QLatin1Char('/'));
    const qint32 nCount = static_cast<qint32>(listRaw.size());
    for (qint32 i = 0; i < nCount; i++) {
        // The trailing blanks come from the writer's fixed 8.3 buffer, not from
        // the name; keep interior spaces, drop the padding.
        const QString sPart = listRaw.at(i).trimmed();
        if (sPart.isEmpty()) {
            // A leading separator is how the archive spells "install root"; an
            // empty component anywhere else means a doubled separator, which
            // the writer never emits.
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

bool XZPakSFXArchive::isDclPreludeAt(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    QPointer<XZPakSFXArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baPrelude = read_array_process(nOffset, 2, pPdStruct);
    if (!guardedThis || !guardedSource || (baPrelude.size() != 2)) return false;

    const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
    return (nLiteralMode <= ZPAKSFX_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= ZPAKSFX_DCL_MIN_DICT_BITS) &&
           (nDictBits <= ZPAKSFX_DCL_MAX_DICT_BITS);
}

bool XZPakSFXArchive::readHeaderAt(qint64 nOffset, qint64 nLimit,
                                   quint16 nVersion, PDSTRUCT *pPdStruct)
{
    QPointer<XZPakSFXArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    // A self-extractor always carries a stub, so a zero offset here is a
    // trailer that does not describe this container - and refusing it keeps
    // this reader structurally unable to take a plain file away from XIBMZPak.
    if (nOffset <= 0) return false;
    if (nOffset > nLimit - ZPAKSFX_HEADER_PROBE_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(nOffset, ZPAKSFX_HEADER_PROBE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    return zpakSfxIsMagic(baHeader, nVersion);
}

bool XZPakSFXArchive::walkVersion1(CONTEXT *pContext, qint32 nNumberOfMembers,
                                   PDSTRUCT *pPdStruct)
{
    QPointer<XZPakSFXArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint64 nDirectorySize =
        static_cast<qint64>(nNumberOfMembers) * ZPAKSFX_V1_ENTRY_SIZE;
    const qint64 nDirectoryOffset = pContext->nTrailerOffset - nDirectorySize;
    // The payload has to leave room for at least one minimal stream, so the
    // directory can never start at or before the archive header.
    if (nDirectoryOffset < pContext->nArchiveOffset + ZPAKSFX_HEADER_SIZE +
                               ZPAKSFX_MIN_PACKED_SIZE) {
        return false;
    }

    const QByteArray baDirectory =
        read_array_process(nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baDirectory.size() != nDirectorySize)) {
        return false;
    }

    qint64 nOffset = pContext->nArchiveOffset + ZPAKSFX_HEADER_SIZE;
    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nEntryOffset =
            static_cast<qint64>(i) * ZPAKSFX_V1_ENTRY_SIZE;
        const QByteArray baEntry =
            baDirectory.mid(static_cast<int>(nEntryOffset),
                            static_cast<int>(ZPAKSFX_V1_ENTRY_SIZE));
        if (baEntry.size() != ZPAKSFX_V1_ENTRY_SIZE) return false;

        const QByteArray baField =
            baEntry.left(static_cast<int>(ZPAKSFX_V1_NAME_SIZE));
        const int nTerminator = baField.indexOf('\0');
        if (nTerminator <= 0) return false;
        for (int j = nTerminator; j < baField.size(); j++) {
            // A single stale byte behind the terminator would mean the 102-byte
            // field is not the fixed, zero-filled buffer this parser assumes.
            if (baField.at(j) != '\0') return false;
        }

        MEMBER member = {};
        if (!isNameField(baField.left(nTerminator), &member.sFileName)) {
            return false;
        }

        const uchar *pEntry =
            reinterpret_cast<const uchar *>(baEntry.constData());
        member.nCompressedSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(
                pEntry + ZPAKSFX_V1_NAME_SIZE)));
        // Version 1 - and ONLY version 1 - carries a named reserved dword that
        // the reference implementation loads and rejects when non-zero.
        const quint32 nReserved =
            qFromLittleEndian<quint32>(pEntry + ZPAKSFX_V1_NAME_SIZE + 4);
        member.nDosDate =
            qFromLittleEndian<quint16>(pEntry + ZPAKSFX_V1_NAME_SIZE + 8);
        member.nDosTime =
            qFromLittleEndian<quint16>(pEntry + ZPAKSFX_V1_NAME_SIZE + 10);
        member.nHeaderOffset = nDirectoryOffset + nEntryOffset;
        member.nDataOffset = nOffset;
        member.nUncompressedSize = 0;
        member.bUncompressedSizeKnown = false;

        if (nReserved != 0U) return false;
        if (member.nCompressedSize < ZPAKSFX_MIN_PACKED_SIZE) return false;
        if (member.nCompressedSize > nDirectoryOffset - nOffset) return false;
        if (!isDclPreludeAt(member.nDataOffset, pPdStruct)) return false;
        if (!guardedThis || !guardedSource) return false;

        pContext->listMembers.append(member);
        nOffset += member.nCompressedSize;
    }

    // The streams carry no terminator of their own: the running sum landing
    // exactly on the first directory byte is this layout's only integrity check,
    // and slack there would mean the directory does not describe the payload.
    if (nOffset != nDirectoryOffset) return false;

    pContext->nDirectoryOffset = nDirectoryOffset;
    return true;
}

bool XZPakSFXArchive::walkVersion2(CONTEXT *pContext, qint32 nNumberOfMembers,
                                   PDSTRUCT *pPdStruct)
{
    QPointer<XZPakSFXArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    qint64 nOffset = pContext->nArchiveOffset + ZPAKSFX_HEADER_SIZE;
    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset > pContext->nTrailerOffset - ZPAKSFX_V2_RECORD_SIZE) {
            return false;
        }

        const QByteArray baRecord =
            read_array_process(nOffset, ZPAKSFX_V2_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baRecord.size() != ZPAKSFX_V2_RECORD_SIZE)) {
            return false;
        }
        if (static_cast<quint8>(baRecord.at(0)) != ZPAKSFX_V2_RECORD_TAG) {
            return false;
        }

        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());
        MEMBER member = {};
        member.nCompressedSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pRecord + 2)));
        // pRecord + 6 is the writer-scratch hole.  It is NOT reserved-zero: it
        // holds a per-archive constant in three of this family's nine version 2
        // archives, the reference implementation never loads it, and checking it
        // rejects those archives and everything inside them.  Do not add a gate
        // here.
        member.nDosDate = qFromLittleEndian<quint16>(pRecord + 10);
        member.nDosTime = qFromLittleEndian<quint16>(pRecord + 12);
        const qint64 nNameSize = static_cast<qint64>(
            static_cast<qint16>(qFromLittleEndian<quint16>(pRecord + 14)));

        if (member.nCompressedSize < ZPAKSFX_MIN_PACKED_SIZE) return false;
        if (nNameSize <= 0) return false;
        const qint64 nDataOffset =
            nOffset + ZPAKSFX_V2_RECORD_SIZE + nNameSize;
        if (nNameSize > pContext->nTrailerOffset - nOffset -
                            ZPAKSFX_V2_RECORD_SIZE) {
            return false;
        }
        if (member.nCompressedSize > pContext->nTrailerOffset - nDataOffset) {
            return false;
        }

        const QByteArray baName = read_array_process(
            nOffset + ZPAKSFX_V2_RECORD_SIZE, nNameSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baName.size() != nNameSize)) {
            return false;
        }
        // The size counts the terminator, and only the bytes in front of it are
        // the name; a record with no printable head is a mis-parse.
        const int nTerminator = baName.indexOf('\0');
        const QByteArray baTrimmed =
            (nTerminator >= 0) ? baName.left(nTerminator) : baName;
        if (!isNameField(baTrimmed, &member.sFileName)) return false;

        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nUncompressedSize = 0;
        member.bUncompressedSizeKnown = false;
        if (!isDclPreludeAt(member.nDataOffset, pPdStruct)) return false;
        if (!guardedThis || !guardedSource) return false;

        pContext->listMembers.append(member);
        nOffset = nDataOffset + member.nCompressedSize;
    }

    // Version 2 has no directory to land on, so the gap in front of the trailer
    // is the only structural check the layout offers.
    if (nOffset > pContext->nTrailerOffset) return false;
    if (pContext->nTrailerOffset - nOffset > ZPAKSFX_V2_MAX_GAP) return false;

    pContext->nDirectoryOffset = -1;
    return true;
}

bool XZPakSFXArchive::scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XZPakSFXArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baPacked = read_array_process(
        pMember->nDataOffset, pMember->nCompressedSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baPacked.size() != pMember->nCompressedSize)) {
        return false;
    }

    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()),
            pMember->nCompressedSize, ZPAKSFX_MAX_UNCOMPRESSED_SIZE, &nConsumed,
            &nRawSize)) {
        return false;
    }
    // VERIFIED INVARIANT over the 16-file / 524-member reference corpus: the
    // stored compressed size is exactly the bitstream boundary the decoder stops
    // at.  A mismatch means the record and the payload disagree, so the
    // recovered length cannot be trusted for extraction.
    if (nConsumed != pMember->nCompressedSize) return false;
    if ((nRawSize < 0) || (nRawSize > ZPAKSFX_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }

    pMember->nUncompressedSize = nRawSize;
    pMember->bUncompressedSizeKnown = true;
    return true;
}

bool XZPakSFXArchive::parseContext(CONTEXT *pContext, bool bScanSizes,
                                   PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XZPakSFXArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    CONTEXT context = {};
    context.nInputSize = nInputSize;
    context.nDirectoryOffset = -1;
    bool bParsed = false;

    // Version 2 is tried first because its trailer is the longer one: a version
    // 1 trailer read at the version 2 offset lands 16 bytes early and cannot
    // then produce an offset that points at a version 2 header.
    if (nInputSize >= ZPAKSFX_V2_TRAILER_SIZE + ZPAKSFX_HEADER_SIZE +
                          ZPAKSFX_MIN_PACKED_SIZE) {
        const qint64 nTrailerOffset = nInputSize - ZPAKSFX_V2_TRAILER_SIZE;
        const QByteArray baTrailer = read_array_process(
            nTrailerOffset, ZPAKSFX_V2_TRAILER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baTrailer.size() == ZPAKSFX_V2_TRAILER_SIZE) {
            const uchar *pTrailer =
                reinterpret_cast<const uchar *>(baTrailer.constData());
            const qint32 nNumberOfMembers =
                static_cast<qint32>(qFromLittleEndian<quint32>(pTrailer));
            const qint64 nArchiveOffset = static_cast<qint64>(
                static_cast<qint32>(qFromLittleEndian<quint32>(pTrailer + 4)));
            if ((nNumberOfMembers >= 1) &&
                (nNumberOfMembers <= ZPAKSFX_MAX_MEMBERS) &&
                readHeaderAt(nArchiveOffset, nTrailerOffset, 2U, pPdStruct)) {
                if (!guardedThis || !guardedSource) return false;
                context.nVersion = 2U;
                context.nArchiveOffset = nArchiveOffset;
                context.nTrailerOffset = nTrailerOffset;
                bParsed = walkVersion2(&context, nNumberOfMembers, pPdStruct);
                if (!guardedThis || !guardedSource) return false;
            }
        }
    }

    if (!bParsed && (nInputSize >= ZPAKSFX_V1_TRAILER_SIZE +
                                       ZPAKSFX_HEADER_SIZE +
                                       ZPAKSFX_MIN_PACKED_SIZE)) {
        context = CONTEXT();
        context.nInputSize = nInputSize;
        context.nDirectoryOffset = -1;

        const qint64 nTrailerOffset = nInputSize - ZPAKSFX_V1_TRAILER_SIZE;
        const QByteArray baTrailer = read_array_process(
            nTrailerOffset, ZPAKSFX_V1_TRAILER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baTrailer.size() == ZPAKSFX_V1_TRAILER_SIZE) {
            const uchar *pTrailer =
                reinterpret_cast<const uchar *>(baTrailer.constData());
            const qint32 nNumberOfMembers =
                static_cast<qint32>(qFromLittleEndian<quint16>(pTrailer));
            const qint64 nArchiveOffset = static_cast<qint64>(
                static_cast<qint32>(qFromLittleEndian<quint32>(pTrailer + 2)));
            if ((nNumberOfMembers >= 1) &&
                readHeaderAt(nArchiveOffset, nTrailerOffset, 1U, pPdStruct)) {
                if (!guardedThis || !guardedSource) return false;
                context.nVersion = 1U;
                context.nArchiveOffset = nArchiveOffset;
                context.nTrailerOffset = nTrailerOffset;
                bParsed = walkVersion1(&context, nNumberOfMembers, pPdStruct);
                if (!guardedThis || !guardedSource) return false;
            }
        }
    }

    if (!bParsed || context.listMembers.isEmpty()) return false;

    if (bScanSizes) {
        const qint32 nCount = static_cast<qint32>(context.listMembers.size());
        for (qint32 i = 0; i < nCount; i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            // A member the decoder cannot measure stays in the listing with its
            // size unknown; methodToHandleMethod() then reports it as UNKNOWN so
            // extraction refuses it instead of writing a truncated file.
            scanMemberSize(&context.listMembers[i], pPdStruct);
            if (!guardedThis || !guardedSource) return false;
        }
    }

    context.nArchiveSize = context.nInputSize;
    context.nFirstMemberOffset = context.listMembers.first().nDataOffset;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

QString XZPakSFXArchive::methodToString(const MEMBER &member)
{
    if (!member.bUncompressedSizeKnown) {
        return QStringLiteral("PKWARE DCL Implode (undecodable)");
    }
    return QStringLiteral("PKWARE DCL Implode");
}

XBinary::HANDLE_METHOD XZPakSFXArchive::methodToHandleMethod(
    const MEMBER &member)
{
    // decPkwareDcl() consumes the packed buffer with its two-byte prelude still
    // attached, but it takes the plaintext length as an INPUT.  Without a
    // measured length the only honest answer is UNKNOWN.
    if (!member.bUncompressedSizeKnown) return HANDLE_METHOD_UNKNOWN;
    return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
}
// ===== END ZPAKSFX PARSER =====

XZPakSFXArchive::XZPakSFXArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZPakSFXArchive::~XZPakSFXArchive()
{
}

bool XZPakSFXArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XZPakSFXArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZPakSFXArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZPakSFXArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                         XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZPakSFXArchive(pDevice);
}

XBinary::FT XZPakSFXArchive::getFileType()
{
    return FT_ZPAK_SFX;
}

XBinary::MODE XZPakSFXArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZPakSFXArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZPakSFXArchive::getArch()
{
    return QString();
}

QString XZPakSFXArchive::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XZPakSFXArchive::getFileFormatExtsString()
{
    return QStringLiteral("ZPAK self-extracting installer");
}

QString XZPakSFXArchive::getMIMEString()
{
    return QStringLiteral("application/x-zpak-sfx");
}

QString XZPakSFXArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XZPakSFXArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XZPakSFXArchive::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XZPakSFXArchive::getMemoryMap(MAPMODE mapMode,
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

bool XZPakSFXArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZPakSFXArchive::getFileParts(quint32 nFileParts,
                                                    qint32 nLimit,
                                                    PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        // The extraction stub and the archive header are one leading region:
        // nothing in front of the first payload is a member.
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveOffset + ZPAKSFX_HEADER_SIZE;
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
        // Version 1 puts a directory in front of the trailer; version 2 has
        // only the trailer, so the table starts there.
        part.nFileOffset = (context.nDirectoryOffset >= 0)
                               ? context.nDirectoryOffset
                               : context.nTrailerOffset;
        part.nFileSize = context.nInputSize - part.nFileOffset;
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

QMap<XBinary::UNPACK_PROP, QVariant>
XZPakSFXArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZPakSFXArchive::initUnpack(UNPACK_STATE *pState,
                                 const QMap<UNPACK_PROP, QVariant> &mapProperties,
                                 PDSTRUCT *pPdStruct)
{
    QPointer<XZPakSFXArchive> guardedThis(this);
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
    // bScanSizes = true: the extraction path needs the plaintext length of every
    // member, and the container does not store it anywhere.
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
        tr("ZPAK self-extracting installer; PKWARE DCL Implode members"));
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

XBinary::ARCHIVERECORD XZPakSFXArchive::infoCurrent(UNPACK_STATE *pState,
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

bool XZPakSFXArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nTrailerOffset;
    return false;
}

bool XZPakSFXArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
