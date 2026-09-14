/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarcv2sfx.h"

#include <QPointer>
#include <QtEndian>

#include <new>

#include "Algos/xarcv2decoder.h"
#include "xdecompress.h"

namespace {
const qint64 ARCV2SFX_ARCHIVE_HEADER_SIZE = 14;
const qint64 ARCV2SFX_BLOCK_PREFIX_SIZE = 17;
// "BLCK" + version + headerSize + flags + bytesOnThisVolume + nameLength is 17
// bytes; the seven dwords that follow the name close the header, so
// blockHeaderSize is always 45 + nameLength.  The size field COUNTS the four
// magic bytes, i.e. the payload starts at blockOffset + blockHeaderSize.
const qint64 ARCV2SFX_BLOCK_FIXED_SIZE = 45;
const quint16 ARCV2SFX_VERSION = 0x0200U;
const quint16 ARCV2SFX_ARCHIVE_HEADER_FIELD = 0x000eU;
const quint32 ARCV2SFX_FLAG_WHOLE = 0x01U;
const quint32 ARCV2SFX_FLAG_SPLIT_HEAD = 0x02U;
const quint32 ARCV2SFX_FLAG_SPLIT_TAIL = 0x08U;
const quint32 ARCV2SFX_FLAG_STORED = 0x10U;
const quint32 ARCV2SFX_FLAG_COMPRESSED = 0x20U;
const quint32 ARCV2SFX_FLAG_KNOWN = 0x3bU;
const quint32 ARCV2SFX_VOLUME_SINGLE = 1U;
const quint32 ARCV2SFX_VOLUME_FIRST = 2U;
const quint32 ARCV2SFX_VOLUME_MIDDLE = 4U;
const quint32 ARCV2SFX_VOLUME_LAST = 8U;
const qint32 ARCV2SFX_MAX_MEMBERS = 200000;
// A built setup writes two containers.  The bound is generous rather than
// exact because nothing in the format forbids a third, but it is there so a
// stream of coincidental headers cannot turn into an unbounded walk.
const qint32 ARCV2SFX_MAX_ARCHIVES = 64;
// The archive can never begin inside the DOS header of its own carrier.
const qint64 ARCV2SFX_MIN_STUB_SIZE = 64;
// The carrier is a 16-bit NE setup stub; every sample starts its first
// container inside the first 32 KiB.  The window is three orders of magnitude
// wider than that so a larger stub still resolves, and bounded so detection
// cost stays linear in a constant rather than in file size.
const qint64 ARCV2SFX_MAX_SCAN_SIZE = qint64(16) << 20;
const qint32 ARCV2SFX_MAX_CANDIDATES = 64;
// Scramble probe budget.  Candidates are tried smallest-payload first, so the
// common case costs a few kilobytes of decoding.
const qint32 ARCV2SFX_PROBE_MAX_CANDIDATES = 8;
const qint32 ARCV2SFX_PROBE_REQUIRED_VOTES = 2;
const qint64 ARCV2SFX_PROBE_MAX_PACKED = qint64(16) << 20;
const qint64 ARCV2SFX_PROBE_MAX_UNPACKED = qint64(64) << 20;
// Bound on the one CRC check performed during validation.
const qint64 ARCV2SFX_CRC_ANCHOR_LIMIT = qint64(1) << 20;

const char ARCV2SFX_ARCHIVE_MAGIC[8] = {'A', 'R', 'C', 'V', '\x00', '\x02',
                                        '\x0e', '\x00'};

bool arcv2sfxRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool arcv2sfxIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}

bool arcv2sfxIsValidVolumeFlags(quint32 nVolumeFlags)
{
    return (nVolumeFlags == ARCV2SFX_VOLUME_SINGLE) ||
           (nVolumeFlags == ARCV2SFX_VOLUME_FIRST) ||
           (nVolumeFlags == ARCV2SFX_VOLUME_MIDDLE) ||
           (nVolumeFlags == ARCV2SFX_VOLUME_LAST);
}

bool arcv2sfxIsValidFlags(quint32 nFlags)
{
    // Only the low byte is ever used, exactly one method bit is set, and
    // exactly one span state is named.
    if (nFlags & ~ARCV2SFX_FLAG_KNOWN) return false;
    const quint32 nMethod =
        nFlags & (ARCV2SFX_FLAG_STORED | ARCV2SFX_FLAG_COMPRESSED);
    if ((nMethod != ARCV2SFX_FLAG_STORED) &&
        (nMethod != ARCV2SFX_FLAG_COMPRESSED)) {
        return false;
    }
    const quint32 nSpan =
        nFlags & (ARCV2SFX_FLAG_WHOLE | ARCV2SFX_FLAG_SPLIT_HEAD |
                  ARCV2SFX_FLAG_SPLIT_TAIL);
    return (nSpan == ARCV2SFX_FLAG_WHOLE) ||
           (nSpan == ARCV2SFX_FLAG_SPLIT_HEAD) ||
           (nSpan == ARCV2SFX_FLAG_SPLIT_TAIL);
}
}  // namespace

XARCV2SFX::XARCV2SFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XARCV2SFX::~XARCV2SFX()
{
}

quint8 XARCV2SFX::scrambleSeed(SCRAMBLE scramble)
{
    return (scramble == SCRAMBLE_DELTA_TRIAL) ? XARCV2Decoder::SEED_TRIAL
                                              : XARCV2Decoder::SEED_RELEASE;
}

bool XARCV2SFX::isStored(quint32 nFlags)
{
    return (nFlags & ARCV2SFX_FLAG_STORED) != 0;
}

bool XARCV2SFX::isCompressed(quint32 nFlags)
{
    return (nFlags & ARCV2SFX_FLAG_COMPRESSED) != 0;
}

bool XARCV2SFX::isSplitFragment(quint32 nFlags)
{
    return (nFlags & (ARCV2SFX_FLAG_SPLIT_HEAD | ARCV2SFX_FLAG_SPLIT_TAIL)) !=
           0;
}

bool XARCV2SFX::isExecutableCarrier(PDSTRUCT *pPdStruct)
{
    // The only thing this family is ever found in is its own MZ carrier.  A
    // bare .ARV, or a container with unrelated junk in front of it, belongs to
    // FT_ARCV2 and must not be taken here.
    const QByteArray baSignature = read_array_process(0, 2, pPdStruct);
    if (baSignature.size() != 2) return false;
    return (baSignature.at(0) == 'M' && baSignature.at(1) == 'Z') ||
           (baSignature.at(0) == 'Z' && baSignature.at(1) == 'M');
}

qint64 XARCV2SFX::nextCandidateOffset(qint64 nFrom, PDSTRUCT *pPdStruct)
{
    const qint64 nInputSize = getSize();
    if ((nFrom < 0) || (nFrom >= nInputSize)) return -1;
    qint64 nScanSize = nInputSize - nFrom;
    if (nFrom + nScanSize > ARCV2SFX_MAX_SCAN_SIZE) {
        nScanSize = ARCV2SFX_MAX_SCAN_SIZE - nFrom;
    }
    if (nScanSize < static_cast<qint64>(sizeof(ARCV2SFX_ARCHIVE_MAGIC))) {
        return -1;
    }
    return find_array(nFrom, nScanSize, ARCV2SFX_ARCHIVE_MAGIC,
                      static_cast<qint64>(sizeof(ARCV2SFX_ARCHIVE_MAGIC)),
                      pPdStruct);
}

bool XARCV2SFX::parseChain(CONTEXT *pContext, qint64 nStubOffset,
                           PDSTRUCT *pPdStruct)
{
    QPointer<XARCV2SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint64 nInputSize = pContext->nInputSize;
    qint64 nOffset = nStubOffset;
    qint32 nArchiveIndex = 0;

    while ((nOffset < nInputSize) && isPdStructNotCanceled(pPdStruct)) {
        if (nArchiveIndex >= ARCV2SFX_MAX_ARCHIVES) return false;
        if (!arcv2sfxRangeWithin(nInputSize, nOffset,
                                 ARCV2SFX_ARCHIVE_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baArchiveHeader = read_array_process(
            nOffset, ARCV2SFX_ARCHIVE_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baArchiveHeader.size() != ARCV2SFX_ARCHIVE_HEADER_SIZE) {
            return false;
        }
        const uchar *pArchiveHeader =
            reinterpret_cast<const uchar *>(baArchiveHeader.constData());
        // The version word is the whole separation from the two sibling
        // containers: ARCV 1.10 is 0x0110 (FT_ARCV, a "CHNK" segment) and the
        // third generation is 0x0400.  A bare "ARCV" prefix test would swallow
        // both.
        if ((qFromLittleEndian<quint32>(pArchiveHeader) != 0x56435241U) ||
            (qFromLittleEndian<quint16>(pArchiveHeader + 4) !=
             ARCV2SFX_VERSION) ||
            (qFromLittleEndian<quint16>(pArchiveHeader + 6) !=
             ARCV2SFX_ARCHIVE_HEADER_FIELD)) {
            return false;
        }
        const quint32 nVolumeFlags =
            qFromLittleEndian<quint32>(pArchiveHeader + 8);
        if (!arcv2sfxIsValidVolumeFlags(nVolumeFlags)) return false;
        if (nArchiveIndex == 0) {
            pContext->nDiskNumber =
                qFromLittleEndian<quint16>(pArchiveHeader + 12);
        }

        nOffset += ARCV2SFX_ARCHIVE_HEADER_SIZE;
        qint32 nMembersInArchive = 0;

        while ((nOffset + ARCV2SFX_BLOCK_PREFIX_SIZE <= nInputSize) &&
               (pContext->listMembers.size() < ARCV2SFX_MAX_MEMBERS) &&
               isPdStructNotCanceled(pPdStruct)) {
            const QByteArray baPrefix = read_array_process(
                nOffset, ARCV2SFX_BLOCK_PREFIX_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource ||
                baPrefix.size() != ARCV2SFX_BLOCK_PREFIX_SIZE) {
                return false;
            }
            const uchar *pPrefix =
                reinterpret_cast<const uchar *>(baPrefix.constData());
            // Not a member header: this container is finished.  The outer loop
            // decides whether what follows is the next container in the chain
            // or garbage, and rejects the stream if it is garbage.
            if (qFromLittleEndian<quint32>(pPrefix) != 0x4b434c42U) break;
            if (qFromLittleEndian<quint16>(pPrefix + 4) != ARCV2SFX_VERSION) {
                return false;
            }
            const quint16 nBlockHeaderSize =
                qFromLittleEndian<quint16>(pPrefix + 6);
            const quint32 nFlags = qFromLittleEndian<quint32>(pPrefix + 8);
            const quint32 nBytesOnThisVolume =
                qFromLittleEndian<quint32>(pPrefix + 12);
            const quint32 nNameLength = pPrefix[16];
            if ((nNameLength == 0) ||
                (nBlockHeaderSize !=
                 ARCV2SFX_BLOCK_FIXED_SIZE + qint64(nNameLength)) ||
                !arcv2sfxIsValidFlags(nFlags)) {
                return false;
            }
            if (!arcv2sfxRangeWithin(nInputSize, nOffset, nBlockHeaderSize)) {
                return false;
            }

            const QByteArray baHeader =
                read_array_process(nOffset, nBlockHeaderSize, pPdStruct);
            if (!guardedThis || !guardedSource ||
                baHeader.size() != nBlockHeaderSize) {
                return false;
            }
            const QByteArray baName =
                baHeader.mid(static_cast<qint32>(ARCV2SFX_BLOCK_PREFIX_SIZE),
                             static_cast<qint32>(nNameLength));
            if (!arcv2sfxIsValidName(baName)) return false;

            const uchar *pFields = reinterpret_cast<const uchar *>(
                baHeader.constData() + ARCV2SFX_BLOCK_PREFIX_SIZE +
                nNameLength);
            MEMBER member = {};
            member.nArchiveIndex = nArchiveIndex;
            member.nHeaderOffset = nOffset;
            member.nHeaderSize = nBlockHeaderSize;
            member.nDataOffset = nOffset + nBlockHeaderSize;
            member.nDataSize = nBytesOnThisVolume;
            member.nFlags = nFlags;
            member.nUncompressedSize = qFromLittleEndian<quint32>(pFields);
            member.nCompressedSize = qFromLittleEndian<quint32>(pFields + 4);
            member.nAttributes = qFromLittleEndian<quint32>(pFields + 8);
            member.nDosDateTime = qFromLittleEndian<quint32>(pFields + 12);
            member.nFileVersionMS = qFromLittleEndian<quint32>(pFields + 16);
            member.nFileVersionLS = qFromLittleEndian<quint32>(pFields + 20);
            member.nPackedCRC32 = qFromLittleEndian<quint32>(pFields + 24);
            member.sFileName =
                QString::fromLatin1(baName).replace(QLatin1Char('\\'),
                                                    QLatin1Char('/'));

            if (!arcv2sfxRangeWithin(nInputSize, member.nDataOffset,
                                     member.nDataSize)) {
                return false;
            }
            // Walk with bytesOnThisVolume, never with compressedSize: a
            // split-head block writes compressedSize as 0 and a
            // compressedSize-driven walk would stall on it forever.  For a
            // whole member the two agree, and that agreement is a cheap
            // structural anchor.
            if (!isSplitFragment(nFlags) &&
                (member.nCompressedSize != member.nDataSize)) {
                return false;
            }
            // A stored member is the anchor that keeps the flag reading
            // honest: its two size fields must match exactly.
            if (isStored(nFlags) && !isSplitFragment(nFlags) &&
                (member.nUncompressedSize != member.nDataSize)) {
                return false;
            }

            pContext->listMembers.append(member);
            ++nMembersInArchive;
            nOffset = member.nDataOffset + member.nDataSize;
        }

        // An empty container is not a container.  Without this a run of
        // coincidental headers could walk forward forever on zero progress.
        if (nMembersInArchive == 0) return false;
        ++nArchiveIndex;
    }

    if ((nOffset != nInputSize) || pContext->listMembers.isEmpty() ||
        !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pContext->nStubOffset = nStubOffset;
    pContext->nArchiveCount = nArchiveIndex;
    pContext->nFirstMemberOffset = pContext->listMembers.first().nHeaderOffset;
    return true;
}

bool XARCV2SFX::parseContext(CONTEXT *pContext, bool bProbeScramble,
                             PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XARCV2SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.scramble = SCRAMBLE_NONE;
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ARCV2SFX_MIN_STUB_SIZE +
                                 ARCV2SFX_ARCHIVE_HEADER_SIZE +
                                 ARCV2SFX_BLOCK_FIXED_SIZE) {
        return false;
    }
    if (!isExecutableCarrier(pPdStruct) || !guardedThis || !guardedSource) {
        return false;
    }

    bool bFound = false;
    qint64 nFrom = ARCV2SFX_MIN_STUB_SIZE;
    for (qint32 i = 0; (i < ARCV2SFX_MAX_CANDIDATES) && !bFound &&
                       isPdStructNotCanceled(pPdStruct);
         ++i) {
        const qint64 nCandidate = nextCandidateOffset(nFrom, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (nCandidate < 0) break;

        CONTEXT candidate = {};
        candidate.scramble = SCRAMBLE_NONE;
        candidate.nInputSize = context.nInputSize;
        if (parseChain(&candidate, nCandidate, pPdStruct)) {
            context = candidate;
            bFound = true;
        } else if (!guardedThis || !guardedSource) {
            return false;
        }
        nFrom = nCandidate + 1;
    }
    if (!bFound) return false;

    // One bounded CRC check.  The stored value is JAMCRC -- CRC-32 with the
    // final inversion omitted -- over the packed bytes, so it is folded back
    // here rather than exposed as RESULTCRC, which would apply it to
    // plaintext.  Split fragments carry the CRC of the whole member, not of
    // the fragment, so they can never match and are skipped.
    for (const MEMBER &member : context.listMembers) {
        if (isSplitFragment(member.nFlags) || (member.nDataSize <= 0) ||
            (member.nDataSize > ARCV2SFX_CRC_ANCHOR_LIMIT)) {
            continue;
        }
        const quint32 nCalculatedCRC =
            _getCRC32(member.nDataOffset, member.nDataSize, 0xffffffffU,
                      _getCRC32Table_EDB88320(), pPdStruct) ^
            0xffffffffU;
        if (!guardedThis || !guardedSource ||
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        if (nCalculatedCRC != member.nPackedCRC32) return false;
        break;
    }

    if (bProbeScramble) {
        context.scramble = probeScramble(context.listMembers,
                                         &context.bScrambleProbed, pPdStruct);
        if (!guardedThis || !guardedSource ||
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
    }

    *pContext = context;
    return true;
}

bool XARCV2SFX::streamEndsAtDeclaredLength(const MEMBER &member,
                                           SCRAMBLE scramble,
                                           PDSTRUCT *pPdStruct)
{
    const qint64 nLength = member.nUncompressedSize;
    if ((nLength <= 0) || (nLength >= ARCV2SFX_PROBE_MAX_UNPACKED)) {
        return false;
    }

    QPointer<XARCV2SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    QByteArray baPacked =
        read_array_process(member.nDataOffset, member.nDataSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baPacked.size() != member.nDataSize) {
        return false;
    }
    if (scramble != SCRAMBLE_NONE) {
        QByteArray baFiltered;
        if (!XARCV2Decoder::descramble(baPacked, scrambleSeed(scramble),
                                       &baFiltered)) {
            return false;
        }
        baPacked = baFiltered;
    }

    // "Reaches the declared length" is on its own a weak test -- a stream read
    // in the wrong mode gets there often enough to matter.  Asking for one
    // more byte is what pins the end on the format's own EOF symbol: a correct
    // stream hits that symbol and refuses, while a wrong reading still has
    // plenty of bits left and happily produces more.
    QByteArray baUnpacked;
    if (!XDecompress::decompressArcvLzhuf(baPacked,
                                          static_cast<qint32>(nLength), false,
                                          &baUnpacked, pPdStruct) ||
        (baUnpacked.size() != nLength) || !guardedThis || !guardedSource) {
        return false;
    }
    QByteArray baOverrun;
    return !XDecompress::decompressArcvLzhuf(
        baPacked, static_cast<qint32>(nLength + 1), false, &baOverrun,
        pPdStruct);
}

XARCV2SFX::SCRAMBLE XARCV2SFX::probeScramble(const QList<MEMBER> &listMembers,
                                             bool *pbProbed,
                                             PDSTRUCT *pPdStruct)
{
    if (pbProbed) *pbProbed = false;

    // Order candidates by payload size so the cheapest members are tried
    // first.  Small members are also the ambiguous ones -- a wrong-mode decode
    // of a short stream can still reach the declared length -- so the walk
    // naturally escalates to bigger, decisive members when it has to.
    QList<const MEMBER *> listCandidates;
    for (const MEMBER &member : listMembers) {
        if (!isCompressed(member.nFlags) || isSplitFragment(member.nFlags) ||
            (member.nDataSize <= 0) || (member.nUncompressedSize <= 0) ||
            (member.nDataSize > ARCV2SFX_PROBE_MAX_PACKED) ||
            (member.nUncompressedSize > ARCV2SFX_PROBE_MAX_UNPACKED)) {
            continue;
        }
        qint32 nInsertion = 0;
        while ((nInsertion < listCandidates.size()) &&
               (listCandidates.at(nInsertion)->nDataSize <=
                member.nDataSize)) {
            ++nInsertion;
        }
        if (nInsertion >= ARCV2SFX_PROBE_MAX_CANDIDATES) continue;
        listCandidates.insert(nInsertion, &member);
        while (listCandidates.size() > ARCV2SFX_PROBE_MAX_CANDIDATES) {
            listCandidates.removeLast();
        }
    }

    // The three readings a payload can be written in: unfiltered, and the two
    // prefix-XOR seeds the scrambled writers use.  A wrong seed is not a near
    // miss -- it XORs every byte of the filtered stream by a constant, so the
    // LZHUF bitstream is wrong from its first symbol -- which is exactly why
    // one stream can separate them.
    static const SCRAMBLE arrModes[] = {SCRAMBLE_NONE, SCRAMBLE_DELTA,
                                        SCRAMBLE_DELTA_TRIAL};
    const qint32 nModeCount =
        static_cast<qint32>(sizeof(arrModes) / sizeof(arrModes[0]));
    qint32 arrVotes[sizeof(arrModes) / sizeof(arrModes[0])] = {};

    for (const MEMBER *pMember : listCandidates) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        // The container has no scramble flag, so the stream itself has to
        // decide.  A candidate only votes when exactly one of the readings
        // holds together; when several do the member is too short to separate
        // them and the next, larger one is tried instead.
        qint32 nHolding = 0;
        qint32 nWinner = -1;
        for (qint32 i = 0; i < nModeCount; ++i) {
            if (streamEndsAtDeclaredLength(*pMember, arrModes[i], pPdStruct)) {
                ++nHolding;
                nWinner = i;
            }
        }
        if (nHolding != 1) continue;
        ++arrVotes[nWinner];
        // Two agreeing votes settle it.  A single vote can be wrong only if
        // that member is damaged, which is why one is not taken until the
        // candidates run out.
        if (arrVotes[nWinner] >= ARCV2SFX_PROBE_REQUIRED_VOTES) break;
    }

    qint32 nBestVotes = 0;
    qint32 nBestCount = 0;
    qint32 nBestIndex = -1;
    for (qint32 i = 0; i < nModeCount; ++i) {
        if (arrVotes[i] > nBestVotes) {
            nBestVotes = arrVotes[i];
            nBestCount = 1;
            nBestIndex = i;
        } else if ((arrVotes[i] == nBestVotes) && (nBestVotes > 0)) {
            ++nBestCount;
        }
    }
    if ((nBestVotes == 0) || (nBestCount != 1)) {
        // No compressed member decoded in any reading -- an all-stored archive,
        // or one whose payloads are damaged -- or the readings tied.  Plain is
        // the right default: it means stored members are written out verbatim.
        return SCRAMBLE_NONE;
    }
    if (pbProbed) *pbProbed = true;
    return arrModes[nBestIndex];
}

bool XARCV2SFX::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // Validation never probes: the probe decompresses real payloads, which has
    // no place in a type-detection pass.
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XARCV2SFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARCV2SFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XARCV2SFX::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XARCV2SFX(pDevice);
}

QList<QString> XARCV2SFX::getSearchSignatures()
{
    // The carrier's own signature; the container lives behind it at a variable
    // offset, so the archive magic is not usable as a file signature here.
    return {QStringLiteral("'MZ'")};
}

XBinary::FT XARCV2SFX::getFileType()
{
    return FT_ARCV2SFX;
}

XBinary::MODE XARCV2SFX::getMode()
{
    return MODE_16SEG;
}

XBinary::ENDIAN XARCV2SFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XARCV2SFX::getArch()
{
    return QStringLiteral("286");
}

QString XARCV2SFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XARCV2SFX::getFileFormatExtsString()
{
    return QStringLiteral("Eschalon Setup ARCV 2 self-extractor (*.exe)");
}

QString XARCV2SFX::getMIMEString()
{
    return QStringLiteral("application/x-msdos-program");
}

QString XARCV2SFX::getVersion()
{
    return QStringLiteral("2.00");
}

qint64 XARCV2SFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    // The carrier is part of the format: the last container ends at end of
    // file, so the whole input is the file format.
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XARCV2SFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XARCV2SFX::getMemoryMap(MAPMODE mapMode,
                                             PDSTRUCT *pPdStruct)
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

QString XARCV2SFX::methodToString(quint32 nFlags, SCRAMBLE scramble)
{
    QString sName;
    if (isSplitFragment(nFlags)) {
        sName = (nFlags & ARCV2SFX_FLAG_SPLIT_HEAD)
                    ? QStringLiteral("Split (continues on the next volume)")
                    : QStringLiteral(
                          "Split (continuation of the previous volume)");
    } else {
        const QString sScramble =
            (scramble == SCRAMBLE_NONE)
                ? QString()
                : QStringLiteral(", XOR-delta scrambled (seed 0x%1)")
                      .arg(scrambleSeed(scramble), 2, 16, QLatin1Char('0'));
        sName = (isStored(nFlags) ? QStringLiteral("Stored")
                                  : QStringLiteral("LZHUF")) +
                sScramble;
    }
    return QStringLiteral("ARCV2 0x%1 %2")
        .arg(nFlags, 2, 16, QLatin1Char('0'))
        .arg(sName);
}

XBinary::HANDLE_METHOD XARCV2SFX::methodToHandleMethod(quint32 nFlags,
                                                       SCRAMBLE scramble)
{
    // A fragment holds either a truncated stream or a headerless
    // continuation of one; neither is decodable from a single volume, so it is
    // listed and left alone rather than handed to a decoder that would emit
    // partial output.
    if (isSplitFragment(nFlags)) return HANDLE_METHOD_UNKNOWN;
    if (isStored(nFlags)) {
        if (scramble == SCRAMBLE_DELTA) return HANDLE_METHOD_ARCV_XOR_DELTA;
        if (scramble == SCRAMBLE_DELTA_TRIAL) {
            return HANDLE_METHOD_ARCV_XOR_DELTA_TRIAL;
        }
        return HANDLE_METHOD_STORE;
    }
    if (isCompressed(nFlags)) {
        if (scramble == SCRAMBLE_DELTA) {
            return HANDLE_METHOD_ARCV2_LZHUF_DELTA;
        }
        if (scramble == SCRAMBLE_DELTA_TRIAL) {
            return HANDLE_METHOD_ARCV2_LZHUF_DELTA_TRIAL;
        }
        return HANDLE_METHOD_ARCV_LZHUF;
    }
    return HANDLE_METHOD_UNKNOWN;
}

bool XARCV2SFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XARCV2SFX::getFileParts(quint32 nFileParts,
                                              qint32 nLimit,
                                              PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nStubOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Self-extractor stub");
        result.append(part);
    }

    qint32 nArchiveIndex = -1;
    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if ((nFileParts & FILEPART_HEADER) &&
            (member.nArchiveIndex != nArchiveIndex)) {
            nArchiveIndex = member.nArchiveIndex;
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset =
                member.nHeaderOffset - ARCV2SFX_ARCHIVE_HEADER_SIZE;
            part.nFileSize = ARCV2SFX_ARCHIVE_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Archive header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_HEADER) &&
            canAppendPart(nLimit, result.size())) {
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
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(
                FPART_PROP_HANDLEMETHOD,
                methodToHandleMethod(member.nFlags, context.scramble));
            part.mapProperties.insert(
                FPART_PROP_REPORTEDMETHOD,
                methodToString(member.nFlags, context.scramble));
            part.mapProperties.insert(FPART_PROP_TYPE, member.nFlags);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = context.nStubOffset;
        part.nFileSize = context.nInputSize - context.nStubOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XARCV2SFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XARCV2SFX::initUnpack(UNPACK_STATE *pState,
                           const QMap<UNPACK_PROP, QVariant> &mapProperties,
                           PDSTRUCT *pPdStruct)
{
    QPointer<XARCV2SFX> guardedThis(this);
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
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    QString sInfo =
        QStringLiteral("Eschalon Setup ARCV 2.00 self-extractor; disk %1; %2 "
                       "container(s) behind a %3-byte stub")
            .arg(pContext->nDiskNumber)
            .arg(pContext->nArchiveCount)
            .arg(pContext->nStubOffset);
    if (pContext->scramble != SCRAMBLE_NONE) {
        sInfo += QStringLiteral("; XOR-delta scrambled (seed 0x%1)")
                     .arg(scrambleSeed(pContext->scramble), 2, 16,
                          QLatin1Char('0'));
    } else if (!pContext->bScrambleProbed) {
        // Nothing decoded either way, so the scramble state was assumed.  Say
        // so: this is the case where a writer that guesses wrong produces
        // plausible-looking but wrong bytes.
        sInfo += tr("; scramble state not determined, assuming plain");
    }
    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, sInfo);
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nInputSize;
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

XBinary::ARCHIVERECORD XARCV2SFX::infoCurrent(UNPACK_STATE *pState,
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
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        methodToHandleMethod(member.nFlags, pContext->scramble));
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        methodToString(member.nFlags, pContext->scramble));
    result.mapProperties.insert(FPART_PROP_TYPE, member.nFlags);
    result.mapProperties.insert(FPART_PROP_FLAGS, member.nAttributes);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_ISREADONLY,
                                (member.nAttributes & 0x01U) != 0);
    result.mapProperties.insert(FPART_PROP_ISHIDDEN,
                                (member.nAttributes & 0x02U) != 0);
    result.mapProperties.insert(FPART_PROP_ISSYSTEM,
                                (member.nAttributes & 0x04U) != 0);
    result.mapProperties.insert(FPART_PROP_ISARCHIVE,
                                (member.nAttributes & 0x20U) != 0);
    const quint16 nDosDate =
        static_cast<quint16>((member.nDosDateTime >> 16) & 0xffffU);
    const quint16 nDosTime =
        static_cast<quint16>(member.nDosDateTime & 0xffffU);
    if (isValidDosDateTime(nDosDate, nDosTime)) {
        const QDateTime dtModified = dosDateTimeToQDateTime(nDosDate, nDosTime);
        if (dtModified.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        }
    }
    if (isSplitFragment(member.nFlags)) {
        result.mapProperties.insert(
            FPART_PROP_INFO,
            tr("Member is split across volumes; not extractable from this "
               "volume alone"));
    }
    return result;
}

bool XARCV2SFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nInputSize;
    return false;
}

bool XARCV2SFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
