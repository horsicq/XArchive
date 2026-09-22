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
#include "xap4archive.h"

#include <QFileInfo>
#include <QSet>

#include <new>

namespace {
// 00 01 NN 01 NN
const qint64 AP4_TOC_HEADER_SIZE = 5;
// u32 BE start | u32 BE size | u8 flag
const qint64 AP4_TOC_RECORD_SIZE = 9;
// One header, one record and one four-byte MPEG frame header.
const qint64 AP4_MIN_INPUT_SIZE =
    AP4_TOC_HEADER_SIZE + AP4_TOC_RECORD_SIZE + 4;
// The 5 bytes of the first non-empty TOC header must lie entirely below this
// offset (the last accepted header start is therefore 65531).  The prefix
// length is unknown (see the header comment), so a fixed offset cannot be
// required; one buffered read is comfortably above any plausible vendor
// header and keeps the random-hit rate of the 5-byte pattern at ~1.5e-5 per
// file before record validation.
const qint64 AP4_FIRST_TOC_WINDOW = 64 * 1024;
// Bomb guard: bytes scanned for further TOCs behind the first one.
const qint64 AP4_MAX_TOC_REGION_SIZE = Q_INT64_C(64) * 1024 * 1024;
const qint32 AP4_MAX_TOCS = 4096;
const qint32 AP4_MAX_MEMBERS = 65535;
// Per-member probe: the all-zero test of the gate and the MP3 classification
// share this one read.  It must cover the largest legal MPEG frame plus a
// second header (2881 + 4) so the second-frame confirmation never runs off
// the probe.
const qint64 AP4_PROBE_SIZE = 64 * 1024;
const qint64 AP4_MPEG_HEADER_SIZE = 4;
const qint64 AP4_SCAN_CHUNK_SIZE = 64 * 1024;
const qint64 AP4_COPY_BUFFER_SIZE = 64 * 1024;
const quint8 AP4_MPEG_SYNC_BYTE = 0xFFU;

// MPEG audio frame header fields that matter for the frame-length formula and
// for the second-frame agreement test.
struct AP4_MPEG_HEADER {
    qint32 nVersionBits;   // 0 = MPEG 2.5, 1 = reserved, 2 = MPEG 2, 3 = MPEG 1
    qint32 nLayerBits;     // 0 = reserved, 1 = Layer III, 2 = Layer II, 3 = Layer I
    qint32 nBitrateIndex;  // 1..14
    qint32 nSampleIndex;   // 0..2
    qint32 nPadding;       // 0..1
};

// Frees the private staging device on every exit of unpackCurrent().
struct AP4_STAGE_HOLDER {
    QIODevice *pStage;

    AP4_STAGE_HOLDER() : pStage(nullptr)
    {
    }
    ~AP4_STAGE_HOLDER()
    {
        XBinary::freeFileBuffer(&pStage);
    }

private:
    AP4_STAGE_HOLDER(const AP4_STAGE_HOLDER &);
    AP4_STAGE_HOLDER &operator=(const AP4_STAGE_HOLDER &);
};

bool ap4RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// Both extents are non-empty and already known to fit the file, so the sums
// cannot overflow.
bool ap4RangesOverlap(qint64 nOffset1, qint64 nSize1, qint64 nOffset2,
                      qint64 nSize2)
{
    return (nOffset1 < (nOffset2 + nSize2)) && (nOffset2 < (nOffset1 + nSize1));
}

quint32 ap4ReadBE32(const uchar *pData)
{
    return (static_cast<quint32>(pData[0]) << 24) |
           (static_cast<quint32>(pData[1]) << 16) |
           (static_cast<quint32>(pData[2]) << 8) |
           static_cast<quint32>(pData[3]);
}

bool ap4IsTocHeader(const uchar *pData)
{
    return (pData[0] == 0x00U) && (pData[1] == 0x01U) && (pData[3] == 0x01U) &&
           (pData[2] == pData[4]);
}

bool ap4IsAllZero(const char *pData, qint64 nSize)
{
    for (qint64 i = 0; i < nSize; ++i) {
        if (pData[i] != 0) return false;
    }
    return true;
}

// Number of hex digits the reference tool pads offsets to: the width of the
// container size itself.
qint32 ap4HexDigits(qint64 nValue)
{
    qint32 nDigits = 1;
    while (nValue >= 16) {
        nValue /= 16;
        ++nDigits;
    }
    return nDigits;
}

bool ap4ParseMpegHeader(const uchar *pData, quint8 nKey,
                        AP4_MPEG_HEADER *pHeader)
{
    if (!pData || !pHeader) return false;

    const quint8 nByte0 = static_cast<quint8>(pData[0] ^ nKey);
    const quint8 nByte1 = static_cast<quint8>(pData[1] ^ nKey);
    const quint8 nByte2 = static_cast<quint8>(pData[2] ^ nKey);

    if (nByte0 != AP4_MPEG_SYNC_BYTE) return false;
    if ((nByte1 & 0xE0U) != 0xE0U) return false;

    const qint32 nVersionBits = (nByte1 >> 3) & 3;
    if (nVersionBits == 1) return false;  // reserved
    const qint32 nLayerBits = (nByte1 >> 1) & 3;
    if (nLayerBits == 0) return false;  // reserved
    const qint32 nBitrateIndex = nByte2 >> 4;
    // 0 is free format (reading pens use fixed bitrates), 15 is invalid.
    if ((nBitrateIndex < 1) || (nBitrateIndex > 14)) return false;
    const qint32 nSampleIndex = (nByte2 >> 2) & 3;
    if (nSampleIndex == 3) return false;

    AP4_MPEG_HEADER header = {};
    header.nVersionBits = nVersionBits;
    header.nLayerBits = nLayerBits;
    header.nBitrateIndex = nBitrateIndex;
    header.nSampleIndex = nSampleIndex;
    header.nPadding = (nByte2 >> 1) & 1;
    *pHeader = header;
    return true;
}

// Standard MPEG 1 / MPEG 2 / MPEG 2.5 frame length in bytes, 0 if the fields
// do not describe a frame.
qint64 ap4MpegFrameLength(const AP4_MPEG_HEADER &header)
{
    // Rows: Layer I, Layer II, Layer III.  Columns: bitrate index 0..14 (kbps).
    static const qint32 arrBitrateMpeg1[3][15] = {
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}};
    static const qint32 arrBitrateMpeg2[3][15] = {
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}};
    // Rows: version bits 0..3 (MPEG 2.5, reserved, MPEG 2, MPEG 1).
    static const qint32 arrSampleRate[4][3] = {{11025, 12000, 8000},
                                               {0, 0, 0},
                                               {22050, 24000, 16000},
                                               {44100, 48000, 32000}};

    if ((header.nVersionBits < 0) || (header.nVersionBits > 3) ||
        (header.nVersionBits == 1) || (header.nLayerBits < 1) ||
        (header.nLayerBits > 3) || (header.nBitrateIndex < 1) ||
        (header.nBitrateIndex > 14) || (header.nSampleIndex < 0) ||
        (header.nSampleIndex > 2)) {
        return 0;
    }

    // Layer bits 3/2/1 = Layer I/II/III -> table row 0/1/2.
    const qint32 nLayerRow = 3 - header.nLayerBits;
    const bool bIsMpeg1 = (header.nVersionBits == 3);
    const qint64 nBitrate =
        static_cast<qint64>(bIsMpeg1 ? arrBitrateMpeg1[nLayerRow][header.nBitrateIndex]
                                     : arrBitrateMpeg2[nLayerRow][header.nBitrateIndex]) *
        1000;
    const qint64 nSampleRate =
        arrSampleRate[header.nVersionBits][header.nSampleIndex];
    if ((nBitrate <= 0) || (nSampleRate <= 0)) return 0;

    qint64 nResult = 0;
    if (nLayerRow == 0) {
        nResult = ((12 * nBitrate / nSampleRate) + header.nPadding) * 4;
    } else if (nLayerRow == 1) {
        nResult = (144 * nBitrate / nSampleRate) + header.nPadding;
    } else {
        nResult = ((bIsMpeg1 ? 144 : 72) * nBitrate / nSampleRate) +
                  header.nPadding;
    }
    return nResult;
}

bool ap4HeadersAgree(const AP4_MPEG_HEADER &first,
                     const AP4_MPEG_HEADER &second)
{
    return (first.nVersionBits == second.nVersionBits) &&
           (first.nLayerBits == second.nLayerBits) &&
           (first.nSampleIndex == second.nSampleIndex);
}

// The MPEG start test for one XOR hypothesis (nKey = 0 is the plain case).
// pData holds the first nProbeSize bytes of a member of nMemberSize bytes.
bool ap4IsMp3Start(const uchar *pData, qint64 nProbeSize, qint64 nMemberSize,
                   quint8 nKey)
{
    if (!pData || (nProbeSize < AP4_MPEG_HEADER_SIZE)) return false;

    AP4_MPEG_HEADER first = {};
    if (!ap4ParseMpegHeader(pData, nKey, &first)) return false;
    const qint64 nFrameLength = ap4MpegFrameLength(first);
    if (nFrameLength <= AP4_MPEG_HEADER_SIZE) return false;

    // With the XOR hypothesis the first byte passes by construction, so a
    // member that cannot even hold one frame of the header it claims is not
    // evidence of anything: reject it rather than XOR-garble a blob into an
    // .mp3.  The plain path keeps the reference tool's single-header rule.
    if ((nKey != 0) && (nMemberSize < nFrameLength)) return false;
    // Too short to hold a second header: the single header is accepted.
    if (nMemberSize < (nFrameLength + AP4_MPEG_HEADER_SIZE)) return true;
    // The probe always covers the largest legal frame; a shorter probe means
    // a short read, which fails closed.
    if (nProbeSize < (nFrameLength + AP4_MPEG_HEADER_SIZE)) return false;

    AP4_MPEG_HEADER second = {};
    if (!ap4ParseMpegHeader(pData + nFrameLength, nKey, &second)) return false;
    return ap4HeadersAgree(first, second);
}

// Plain MP3 (frame header or ID3v2 tag) -> bIsMp3, key 0; XOR-obfuscated MP3
// -> bIsMp3, key b0 ^ 0xFF; anything else -> unknown blob, key 0.
void ap4ClassifyMember(const QByteArray &baProbe, qint64 nMemberSize,
                       bool *pbIsMp3, quint8 *pnKey)
{
    if (!pbIsMp3 || !pnKey) return;
    *pbIsMp3 = false;
    *pnKey = 0;

    const qint64 nProbeSize = baProbe.size();
    const uchar *pData = reinterpret_cast<const uchar *>(baProbe.constData());
    if ((nProbeSize >= 3) && (pData[0] == 'I') && (pData[1] == 'D') &&
        (pData[2] == '3')) {
        *pbIsMp3 = true;
        return;
    }
    if (nProbeSize < AP4_MPEG_HEADER_SIZE) return;

    if (ap4IsMp3Start(pData, nProbeSize, nMemberSize, 0)) {
        *pbIsMp3 = true;
        return;
    }
    // The XOR hypothesis assumes the plaintext starts with the sync byte.  A
    // member that already starts with FF but failed the plain test would get
    // key 0, which is not a hypothesis at all.
    const quint8 nKey = static_cast<quint8>(pData[0] ^ AP4_MPEG_SYNC_BYTE);
    if (nKey == 0) return;
    if (ap4IsMp3Start(pData, nProbeSize, nMemberSize, nKey)) {
        *pbIsMp3 = true;
        *pnKey = nKey;
    }
}

quint64 ap4ExtentKey(qint64 nOffset, qint64 nSize)
{
    return (static_cast<quint64>(static_cast<quint32>(nOffset)) << 32) |
           static_cast<quint64>(static_cast<quint32>(nSize));
}
}  // namespace

XAP4Archive::XAP4Archive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAP4Archive::~XAP4Archive()
{
}

bool XAP4Archive::findTocHeader(SCAN_CACHE *pCache, qint64 nFrom, qint64 nEnd,
                                qint64 nInputSize, qint64 *pnFound,
                                quint8 *pnRecordCount, PDSTRUCT *pPdStruct)
{
    if (!pCache || !pnFound || !pnRecordCount) return false;
    *pnFound = -1;
    *pnRecordCount = 0;

    QIODevice *guardedSource = getDevice();
    if ((nFrom < 0) || (nEnd > nInputSize)) return false;

    qint64 nPos = nFrom;
    while ((nPos + AP4_TOC_HEADER_SIZE) <= nEnd) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint64 nCacheEnd =
            (pCache->nOffset < 0) ? -1 : (pCache->nOffset + pCache->baData.size());
        if ((pCache->nOffset < 0) || (nPos < pCache->nOffset) ||
            ((nPos + AP4_TOC_HEADER_SIZE) > nCacheEnd)) {
            // Refill from nPos: the window that straddled the old chunk end is
            // re-read at the start of the new one.
            const qint64 nChunkSize = qMin(AP4_SCAN_CHUNK_SIZE, nInputSize - nPos);
            if (nChunkSize < AP4_TOC_HEADER_SIZE) return true;
            pCache->baData = read_array_process(nPos, nChunkSize, pPdStruct);
            if ((pCache->baData.size() != nChunkSize)) {
                pCache->nOffset = -1;
                pCache->baData.clear();
                return false;
            }
            pCache->nOffset = nPos;
        }

        const qint64 nWindowEnd = pCache->nOffset + pCache->baData.size();
        const uchar *pData =
            reinterpret_cast<const uchar *>(pCache->baData.constData()) +
            (nPos - pCache->nOffset);
        while (((nPos + AP4_TOC_HEADER_SIZE) <= nEnd) &&
               ((nPos + AP4_TOC_HEADER_SIZE) <= nWindowEnd)) {
            if (ap4IsTocHeader(pData)) {
                *pnFound = nPos;
                *pnRecordCount = pData[4];
                return true;
            }
            ++nPos;
            ++pData;
        }
    }
    return true;
}

bool XAP4Archive::probeMember(const MEMBER &member, bool bGateOnly,
                              QByteArray *pbaProbe, bool *pbProbeNull,
                              bool *pbAllNull, PDSTRUCT *pPdStruct)
{
    if (!pbaProbe || !pbProbeNull || !pbAllNull) return false;
    *pbProbeNull = false;
    *pbAllNull = false;

    QIODevice *guardedSource = getDevice();
    if ((member.nOffset < 0) || (member.nSize <= 0)) {
        return false;
    }

    const qint64 nProbeSize = qMin(member.nSize, AP4_PROBE_SIZE);
    *pbaProbe = read_array_process(member.nOffset, nProbeSize, pPdStruct);
    if ((pbaProbe->size() != nProbeSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pbProbeNull = ap4IsAllZero(pbaProbe->constData(), nProbeSize);
    *pbAllNull = *pbProbeNull;
    if (bGateOnly || !*pbProbeNull || (member.nSize <= nProbeSize)) return true;

    // Full parse: keep reading only while every byte so far is zero, so the
    // cost is bounded by the zero prefix rather than by the member.
    QByteArray baChunk(static_cast<qint32>(AP4_COPY_BUFFER_SIZE), '\0');
    if (baChunk.size() != AP4_COPY_BUFFER_SIZE) return false;
    qint64 nPos = member.nOffset + nProbeSize;
    const qint64 nEnd = member.nOffset + member.nSize;
    while (nPos < nEnd) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint64 nChunkSize = qMin(AP4_COPY_BUFFER_SIZE, nEnd - nPos);
        if (read_array_process(nPos, baChunk.data(), nChunkSize, pPdStruct) !=
            nChunkSize) {
            return false;
        }
        if (!guardedSource) return false;
        if (!ap4IsAllZero(baChunk.constData(), nChunkSize)) {
            *pbAllNull = false;
            break;
        }
        nPos += nChunkSize;
    }
    return true;
}

bool XAP4Archive::parseContext(CONTEXT *pContext, bool bGateOnly,
                               PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < AP4_MIN_INPUT_SIZE) return false;

    SCAN_CACHE cache = {};
    cache.nOffset = -1;
    qint64 nScanPos = 0;
    // Smallest accepted member start: no TOC header may lie at or above it.
    qint64 nScanLimit = context.nInputSize;
    qint64 nFirstTocOffset = -1;
    QSet<quint64> setExtents;
    QList<MEMBER> listAccepted;

    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const bool bFirst = (nFirstTocOffset < 0);
        qint64 nSearchEnd = nScanLimit;
        if (bFirst) {
            nSearchEnd = qMin(nSearchEnd, AP4_FIRST_TOC_WINDOW);
        } else {
            nSearchEnd = qMin(nSearchEnd, nFirstTocOffset + AP4_MAX_TOC_REGION_SIZE);
        }

        qint64 nFound = -1;
        quint8 nRecordCount = 0;
        if (!findTocHeader(&cache, nScanPos, nSearchEnd, context.nInputSize,
                           &nFound, &nRecordCount, pPdStruct)) {
            return false;
        }
        if (nFound < 0) {
            if (bFirst) return false;
            break;
        }
        if (nRecordCount == 0) {
            // An empty TOC: consumed, not counted.
            nScanPos = nFound + AP4_TOC_HEADER_SIZE;
            continue;
        }

        const qint64 nRecordsSize = AP4_TOC_RECORD_SIZE * nRecordCount;
        const qint64 nTocSize = AP4_TOC_HEADER_SIZE + nRecordsSize;
        const qint64 nTocEnd = nFound + nTocSize;
        // A truncated TOC would be read as garbage by the reference tool; a
        // TOC reaching into member data is nonsense either way.
        if ((nTocEnd > context.nInputSize) || (nTocEnd > nScanLimit)) {
            return false;
        }

        const QByteArray baRecords =
            read_array_process(nFound + AP4_TOC_HEADER_SIZE, nRecordsSize, pPdStruct);
        if ((baRecords.size() != nRecordsSize)) {
            return false;
        }

        TOC toc = {};
        toc.nOffset = nFound;
        toc.nSize = nTocSize;
        toc.nRecordCount = nRecordCount;
        context.listTocs.append(toc);
        if (context.listTocs.size() > AP4_MAX_TOCS) return false;
        const qint32 nTocIndex = context.listTocs.size() - 1;

        const uchar *pRecords =
            reinterpret_cast<const uchar *>(baRecords.constData());
        for (qint32 i = 0; i < nRecordCount; ++i) {
            const uchar *pRecord = pRecords + (i * AP4_TOC_RECORD_SIZE);
            const qint64 nStart = ap4ReadBE32(pRecord);
            const qint64 nSize = ap4ReadBE32(pRecord + 4);
            const quint8 nFlag = pRecord[8];

            if (nSize == 0) continue;
            if (nStart > (context.nInputSize - nSize)) continue;
            // Offset 0 would overlap the prefix and the TOC region; the
            // reference tool never sees such a record in a real file.
            if (nStart == 0) return false;
            const quint64 nExtentKey = ap4ExtentKey(nStart, nSize);
            if (setExtents.contains(nExtentKey)) continue;
            setExtents.insert(nExtentKey);

            for (qint32 j = 0; j < context.listTocs.size(); ++j) {
                const TOC &tocCheck = context.listTocs.at(j);
                if (ap4RangesOverlap(nStart, nSize, tocCheck.nOffset, tocCheck.nSize)) {
                    return false;
                }
            }

            MEMBER member = {};
            member.nOffset = nStart;
            member.nSize = nSize;
            member.nFlag = nFlag;
            member.nXorKey = 0;
            member.bIsMp3 = false;
            member.nTocIndex = nTocIndex;
            member.nRecordIndex = i;
            listAccepted.append(member);
            if (listAccepted.size() > AP4_MAX_MEMBERS) return false;

            nScanLimit = qMin(nScanLimit, nStart);
            context.nArchiveSize = qMax(context.nArchiveSize, nStart + nSize);
        }

        if (bFirst) {
            nFirstTocOffset = nFound;
            context.nPrefixSize = nFound;
        }
        nScanPos = nTocEnd;
    }

    if (context.listTocs.isEmpty() || listAccepted.isEmpty()) return false;
    // The TOC region precedes every member.
    for (qint32 i = 0; i < context.listTocs.size(); ++i) {
        const TOC &toc = context.listTocs.at(i);
        if ((toc.nOffset + toc.nSize) > nScanLimit) return false;
    }
    const TOC &tocLast = context.listTocs.last();
    context.nHeaderSize = tocLast.nOffset + tocLast.nSize;
    context.nArchiveSize = qMax(context.nArchiveSize, context.nHeaderSize);
    if (!ap4RangeWithin(context.nInputSize, 0, context.nArchiveSize)) return false;

    // Classification.  The first member whose probe is not all zero must be
    // an MP3; that rule is evaluated on the same probe in both modes so the
    // gate and the full parse can never disagree on it.
    bool bFirstProbedSeen = false;
    for (qint32 i = 0; i < listAccepted.size(); ++i) {
        MEMBER member = listAccepted.at(i);
        QByteArray baProbe;
        bool bProbeNull = false;
        bool bAllNull = false;
        if (!probeMember(member, bGateOnly, &baProbe, &bProbeNull, &bAllNull,
                         pPdStruct)) {
            return false;
        }
        if (bAllNull) continue;

        if (!bProbeNull) {
            ap4ClassifyMember(baProbe, member.nSize, &member.bIsMp3,
                              &member.nXorKey);
            if (!bFirstProbedSeen) {
                bFirstProbedSeen = true;
                if (!member.bIsMp3) return false;
            }
        }
        context.listMembers.append(member);
        // The gate has everything it needs once the first probed member has
        // passed; the remaining members are only material for a listing.
        if (bGateOnly && bFirstProbedSeen) break;
    }
    if (!bFirstProbedSeen || context.listMembers.isEmpty()) return false;

    // Names: the reference tool derives them from the container name and the
    // member extent, so they are reproduced here byte for byte.
    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource);
    if (!guardedSource) return false;
    QString sStem;
    if (!sDeviceName.isEmpty()) {
        sStem = QFileInfo(sDeviceName).completeBaseName();
    }
    if (sStem.isEmpty()) sStem = QStringLiteral("ap4");
    sStem = fixFileName(sStem);
    if (sStem.isEmpty()) sStem = QStringLiteral("ap4");
    const qint32 nWidth = ap4HexDigits(context.nInputSize);

    QSet<QString> setNames;
    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        MEMBER &member = context.listMembers[i];
        // The stem is concatenated, not substituted: a container name holding
        // a %N marker must not be rewritten by the later arg() calls.
        const QString sBase =
            sStem + QStringLiteral(" 0x%1-0x%2 (%3)")
                        .arg(member.nOffset, nWidth, 16, QLatin1Char('0'))
                        .arg(member.nOffset + member.nSize, nWidth, 16,
                             QLatin1Char('0'))
                        .arg(member.nSize);
        const QString sExt = member.bIsMp3 ? QStringLiteral("mp3")
                                           : QStringLiteral("unk");
        QString sName = sBase + QLatin1Char('.') + sExt;
        // Names embed start, end and size and exact duplicates were collapsed,
        // so this loop cannot fire; it is kept so a collision can never
        // overwrite a sibling on disk.
        qint32 nSuffix = 2;
        while (setNames.contains(sName)) {
            sName = sBase + QStringLiteral(" (%1).").arg(nSuffix++) + sExt;
        }
        setNames.insert(sName);
        member.sFileName = sName;
    }

    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XAP4Archive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // There is no magic at all: the gate is the whole TOC chain plus the MP3
    // classification of the first member, including the second-frame check.
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XAP4Archive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAP4Archive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAP4Archive::createInstance(QIODevice *pDevice, bool bIsImage,
                                     XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAP4Archive(pDevice);
}

QList<QString> XAP4Archive::getSearchSignatures()
{
    // No magic: 00 01 NN 01 NN sits at an unknown offset and is not a usable
    // signature.  Detection relies entirely on isValid().
    return QList<QString>();
}

XBinary::FT XAP4Archive::getFileType()
{
    return FT_AP4;
}

XBinary::MODE XAP4Archive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAP4Archive::getEndian()
{
    return ENDIAN_BIG;
}

QString XAP4Archive::getArch()
{
    return QString();
}

qint32 XAP4Archive::getType()
{
    return TYPE_ARCHIVE;
}

QString XAP4Archive::getFileFormatExt()
{
    return QStringLiteral("ap4");
}

QString XAP4Archive::getFileFormatExtsString()
{
    return QStringLiteral("AP4 reading-pen audio container (*.ap4)");
}

QString XAP4Archive::getMIMEString()
{
    return QStringLiteral("application/x-ap4");
}

qint64 XAP4Archive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    // nArchiveSize is computed before the all-zero drop, so the cheap parse
    // reports the same extent as the full one.
    return parseContext(&context, true, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAP4Archive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XAP4Archive::getMemoryMap(MAPMODE mapMode,
                                               PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        // XOR members are REGION parts (never STREAM: STORE would hand out the
        // obfuscated bytes), so the regions map must request them explicitly.
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_REGION,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XAP4Archive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XAP4Archive::getFileParts(quint32 nFileParts,
                                                qint32 nLimit,
                                                PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    // The full parse: a listing must not carry a member the unpack path drops.
    if (!parseContext(&context, false, pPdStruct)) return listResult;
    if (context.listMembers.isEmpty()) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size())) &&
        ap4RangeWithin(context.nInputSize, 0, context.nHeaderSize)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if (nFileParts & FILEPART_STREAM) {
        // Only members whose bytes are the plaintext.  An XOR member cannot be
        // a STREAM part: STORE would hand out the obfuscated bytes and the
        // archive-stream contract is refused on FPART routes.
        for (qint32 i = 0; i < context.listMembers.size(); ++i) {
            const MEMBER &member = context.listMembers.at(i);
            if (member.nXorKey != 0) continue;
            if (!canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) break;
            if (!ap4RangeWithin(context.nInputSize, member.nOffset, member.nSize)) {
                continue;
            }
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            listResult.append(part);
        }
    }

    if (nFileParts & FILEPART_REGION) {
        for (qint32 i = 0; i < context.listMembers.size(); ++i) {
            const MEMBER &member = context.listMembers.at(i);
            if (member.nXorKey == 0) continue;
            if (!canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) break;
            if (!ap4RangeWithin(context.nInputSize, member.nOffset, member.nSize)) {
                continue;
            }
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(
                FPART_PROP_INFO,
                QStringLiteral("XOR 0x%1")
                    .arg(QString::number(member.nXorKey, 16)
                             .rightJustified(2, QLatin1Char('0'))
                             .toUpper()));
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size())) &&
        ap4RangeWithin(context.nInputSize, 0, context.nArchiveSize)) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XAP4Archive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAP4Archive::initUnpack(UNPACK_STATE *pState,
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
    if (!finishUnpack(pState, nullptr) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    // Malformed output-limit properties fail here rather than at extraction.
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &policy)) {
        releaseUnpackSource(pState);
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, false, pPdStruct) || pContext->listMembers.isEmpty() ||
        (pContext->nInputSize != guardedSource->size())) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    qint32 nXorMembers = 0;
    for (qint32 i = 0; i < pContext->listMembers.size(); ++i) {
        if (pContext->listMembers.at(i).nXorKey != 0) ++nXorMembers;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        QStringLiteral("AP4: %1 TOC(s), %2 member(s), %3 XOR-obfuscated, %4 prefix byte(s)")
            .arg(pContext->listTocs.size())
            .arg(pContext->listMembers.size())
            .arg(nXorMembers)
            .arg(pContext->nPrefixSize));
    pState->nCurrentOffset = pContext->listMembers.first().nOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XAP4Archive::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext ||
        (pState->nNumberOfRecords != pContext->listMembers.size()) ||
        (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if ((pState->nCurrentOffset != member.nOffset) || (member.nSize <= 0) ||
        !ap4RangeWithin(pContext->nInputSize, member.nOffset, member.nSize) ||
        member.sFileName.isEmpty()) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(
        FPART_PROP_INFO,
        QStringLiteral("TOC %1 record %2, flag 0x%3")
            .arg(member.nTocIndex)
            .arg(member.nRecordIndex)
            .arg(QString::number(member.nFlag, 16).rightJustified(2, QLatin1Char('0')).toUpper()));
    // No checksum and no time stamp property: the container carries neither.

    if (member.nXorKey == 0) {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                    QStringLiteral("Stored"));
        return result;
    }

    // The plaintext only exists once this object has applied the key, so the
    // record is routed back here (HANDLE_METHOD_ARCHIVE_STREAM is set by the
    // marker together with the identity token).
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        QStringLiteral("XOR 0x%1")
            .arg(QString::number(member.nXorKey, 16)
                     .rightJustified(2, QLatin1Char('0'))
                     .toUpper()));
    return markArchiveStreamRecord(&result, pState->nCurrentIndex)
               ? result
               : ARCHIVERECORD();
}

bool XAP4Archive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    QIODevice *guardedOutput = pDevice;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        devicesAlias(guardedSource, guardedOutput) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if ((nCurrentSize != pContext->nInputSize) ||
        (pState->nTotalSize != nCurrentSize) ||
        (pState->nNumberOfRecords != pContext->listMembers.size()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    if ((pState->nCurrentOffset != member.nOffset) || (member.nSize <= 0) ||
        !ap4RangeWithin(nCurrentSize, member.nOffset, member.nSize)) {
        return false;
    }

    if (member.nXorKey == 0) {
        // Stored member: the inherited STORE path copies the real extent.  It
        // acquires the operation guard itself, so ours must go first.
        operationGuard.release();
        return XArchive::unpackCurrent(pState, pDevice, pPdStruct);
    }

    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, member.nSize)) {
        setPdStructErrorString(pPdStruct,
                               tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                 member.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(member.nSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    // Decode into private storage; the caller's device only sees a complete,
    // verified member through publishUnpackOutput().
    AP4_STAGE_HOLDER stage;
    stage.pStage = createUnpackFileBuffer(member.nSize, pState->mapUnpackProperties,
                                          pPdStruct);
    if (!stage.pStage || (stage.pStage->size() != member.nSize) ||
        !stage.pStage->seek(0) || !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }

    QByteArray baBuffer(static_cast<qint32>(AP4_COPY_BUFFER_SIZE), '\0');
    if (baBuffer.size() != AP4_COPY_BUFFER_SIZE) return false;

    qint64 nCopied = 0;
    while (nCopied < member.nSize) {
        if (!isUnpackSourceCurrent(pState, pPdStruct) ||
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint64 nChunkSize = qMin(AP4_COPY_BUFFER_SIZE, member.nSize - nCopied);
        if (read_array_process(member.nOffset + nCopied, baBuffer.data(),
                               nChunkSize, pPdStruct) != nChunkSize) {
            return false;
        }
        if (!guardedSource) return false;

        uchar *pChunk = reinterpret_cast<uchar *>(baBuffer.data());
        for (qint64 i = 0; i < nChunkSize; ++i) {
            pChunk[i] = static_cast<uchar>(pChunk[i] ^ member.nXorKey);
        }
        if (safeWriteData(stage.pStage, nCopied, baBuffer.constData(), nChunkSize,
                          pPdStruct) != nChunkSize) {
            return false;
        }
        nCopied += nChunkSize;
    }

    if ((stage.pStage->size() != member.nSize) || !stage.pStage->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    // The cursor stays on the record start, exactly as the inherited STORE
    // path leaves it: infoCurrent() checks it and moveToNext() advances it.
    const bool bPublished = publishUnpackOutput(stage.pStage, guardedOutput,
                                                pState, pPdStruct);
    return bPublished;
}

bool XAP4Archive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext ||
        (pState->nNumberOfRecords != pContext->listMembers.size()) ||
        (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XAP4Archive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XAP4Archive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME
                               << FPART_PROP_COMPRESSEDSIZE
                               << FPART_PROP_UNCOMPRESSEDSIZE
                               << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD
                               << FPART_PROP_ISFOLDER << FPART_PROP_INFO;
}
