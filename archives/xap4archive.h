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
#ifndef XAP4ARCHIVE_H
#define XAP4ARCHIVE_H

#include "xarchive.h"

// AP4 - the audio package of the Viaton / FLTRP "Waiyantong" reading pens and
// of the Kids&Us "Talking Pen".  Nothing public documents the layout; the
// format understanding below was derived by studying the behaviour of the only
// public unpacker (unap4 1.0, AGPL) and by running it on synthetic files.  No
// code, comment or identifier of that tool is reproduced here.
//
// The container has NO magic, NO checksum, NO member names, NO timestamps and
// no member count at a fixed place.  All integers in the table of contents are
// BIG endian.
//
//   [prefix]           unknown bytes, length >= 0 (real files: >= 1, see below)
//   [TOC #1] ... [TOC #k]   located by pattern scan, not by position
//   [gap / padding]
//   [member data]      MP3 frames, some XOR-obfuscated with a per-member key
//
// TOC header - 5 bytes, matched by pattern:
//
//   +0  u8   00
//   +1  u8   01
//   +2  u8   NN   record count
//   +3  u8   01
//   +4  u8   NN   record count again - must equal +2
//
// A header with NN == 0 is an empty TOC: its 5 bytes are consumed and the scan
// carries on.  NN records of 9 bytes follow a non-empty header:
//
//   +0  u32 BE  member start offset, absolute from the file start
//   +4  u32 BE  member size in bytes
//   +8  u8      flag - never interpreted by the reference tool; reported only
//
// After a TOC the scan for the next header resumes right behind its last
// record, so TOCs may be contiguous or separated by arbitrary bytes.  The scan
// never runs into member data: every accepted member start becomes a ceiling
// for the header search, and every TOC must end at or before the smallest
// accepted member start.
//
// Record disposition (reference behaviour, tightened where noted):
//   * size == 0                      -> not a member (skipped)
//   * start + size > file size       -> not a member (skipped, not fatal)
//   * start == 0                     -> reject (would overlap the prefix/TOC)
//   * exact duplicate (start, size)  -> listed once
//   * overlaps any TOC extent        -> reject (structural nonsense)
//   * every byte 0x00                -> dropped (the reference tool drops them)
//
// Member classification (the reference tool's XOR acceptance test is always
// true because of an operator typo; the rule implemented here is the one it
// evidently intended):
//   * starts with an MPEG audio frame header, or with an ID3v2 tag ("ID3")
//     -> plain MP3, HANDLE_METHOD_STORE, extension .mp3
//   * else, with key = b0 ^ 0xFF (never 0 here), the first four bytes XORed
//     with key form a valid MPEG audio frame header, the member holds at
//     least one full frame of that header AND - when the member is long
//     enough to hold two frames - the header of the second frame agrees in
//     version, layer and sample rate -> XOR-obfuscated MP3, materialised by
//     this class through HANDLE_METHOD_ARCHIVE_STREAM, extension .mp3
//     (a member too short for one frame of the header it XOR-claims is not
//     evidence of an MP3; the plain path above keeps the reference rule)
//   * else an unknown blob, HANDLE_METHOD_STORE, extension .unk
// The MPEG header test requires sync FF Ex, a non-reserved version and layer,
// a bitrate index in 1..14 (free format is rejected: reading pens use fixed
// bitrates) and a valid sample-rate index.  With the XOR hypothesis the first
// byte passes by construction, so the second-frame confirmation is what makes
// the XOR path a real gate.
//
// Detection (isValid) requires ALL of: file size >= 18; a TOC header with
// NN >= 1 whose 5 bytes lie entirely inside the first 64 KiB (the reference
// tool cannot process a TOC at
// offset 0, so real files carry a prefix of unknown length, and the prefix
// length cannot be fixed - a bounded window is the only usable gate); every
// TOC inside the file, none overlapping a member, all below every member; at
// least one member whose first 64 KiB is not all zero; the FIRST such member
// classifies as MP3 (plain or XOR, second-frame confirmation included); TOC
// and member counts within the caps.  Nothing else is validated: the prefix
// bytes, the flag byte, gaps between TOCs, later members (they may be .unk)
// and partial overlaps between members (a code table may legitimately alias
// one clip twice) are all left alone.
//
// The all-zero test is the one rule whose full form needs a complete member
// read.  The gate probes the first 64 KiB of each member; the full parse used
// for listing and unpacking keeps reading a member only while every byte so
// far is zero, so the cost is bounded by the zero prefix, not the member.  The
// "first member must be MP3" rule is evaluated on the same 64 KiB probe in
// both modes so that a file accepted by isValid is always accepted by
// initUnpack.
//
// There is no compression: STORE and XOR members are 1:1, so ratio caps are
// moot.  Bomb guards are the 64 KiB first-TOC window, the 64 MiB TOC region
// cap, the TOC/member count caps and the per-member output-size policy.
//
// File parts: plain and unknown members are FILEPART_STREAM (STORE-able
// extents); XOR members are FILEPART_REGION only, carrying an info text with
// the key.  They therefore appear in the REGIONS memory map as regions and
// are absent from the STREAMS map, which is by definition the STORE-able set.
//
// Member names follow the reference tool so that its outputs and ours are
// byte- and name-identical: "<stem> 0x<start>-0x<end> (<size>).<ext>" with
// stem = container name without its last extension, start/end in lower-case
// hex zero-padded to the width of the container size, size in decimal.
class XAP4Archive final : public XArchive {
    Q_OBJECT

public:
    explicit XAP4Archive(QIODevice *pDevice = nullptr);
    ~XAP4Archive() override;

    bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false,
                            XADDR nModuleAddress = -1) override;

    QList<QString> getSearchSignatures() override;

    FT getFileType() override;
    MODE getMode() override;
    ENDIAN getEndian() override;
    QString getArch() override;
    qint32 getType() override;
    QString getFileFormatExt() override;
    QString getFileFormatExtsString() override;
    QString getMIMEString() override;
    qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    QList<MAPMODE> getMapModesList() override;
    _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN,
                             PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1,
                              PDSTRUCT *pPdStruct = nullptr) override;

    QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    bool initUnpack(UNPACK_STATE *pState,
                    const QMap<UNPACK_PROP, QVariant> &mapProperties,
                    PDSTRUCT *pPdStruct = nullptr) override;
    ARCHIVERECORD infoCurrent(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct = nullptr) override;
    bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                       PDSTRUCT *pPdStruct = nullptr) override;
    bool moveToNext(UNPACK_STATE *pState,
                    PDSTRUCT *pPdStruct = nullptr) override;
    bool finishUnpack(UNPACK_STATE *pState,
                      PDSTRUCT *pPdStruct = nullptr) override;
    QList<FPART_PROP> getAvailableFPARTProperties() override;

private:
    struct MEMBER {
        qint64 nOffset;
        qint64 nSize;
        quint8 nFlag;
        quint8 nXorKey;  // 0 = stored verbatim (plain MP3 or unknown blob)
        bool bIsMp3;
        qint32 nTocIndex;
        qint32 nRecordIndex;
        QString sFileName;
    };

    struct TOC {
        qint64 nOffset;
        qint64 nSize;
        qint32 nRecordCount;
    };

    struct CONTEXT {
        qint64 nInputSize;
        qint64 nArchiveSize;  // end of the furthest member (or of the last TOC)
        qint64 nPrefixSize;   // offset of the first non-empty TOC
        qint64 nHeaderSize;   // end of the last TOC: prefix + TOC chain
        QList<TOC> listTocs;
        QList<MEMBER> listMembers;
    };

    // One buffered window of the header scan so a run of empty TOCs does not
    // cost one device read per five bytes.
    struct SCAN_CACHE {
        qint64 nOffset;
        QByteArray baData;
    };

    // bGateOnly = true is the detection gate: it probes 64 KiB per member and
    // stops classifying after the first member that is not all zero.  The
    // full parse (listing, unpacking) additionally drops every member whose
    // whole extent is zero and classifies every survivor.
    bool parseContext(CONTEXT *pContext, bool bGateOnly, PDSTRUCT *pPdStruct);
    bool findTocHeader(SCAN_CACHE *pCache, qint64 nFrom, qint64 nEnd,
                       qint64 nInputSize, qint64 *pnFound,
                       quint8 *pnRecordCount, PDSTRUCT *pPdStruct);
    bool probeMember(const MEMBER &member, bool bGateOnly,
                     QByteArray *pbaProbe, bool *pbProbeNull, bool *pbAllNull,
                     PDSTRUCT *pPdStruct);
    static bool canAppendPart(qint32 nLimit, qint32 nCurrentCount);
};

#endif  // XAP4ARCHIVE_H
