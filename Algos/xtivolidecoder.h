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
#ifndef XTIVOLIDECODER_H
#define XTIVOLIDECODER_H

#include "../xbinary.h"

// Tivoli Filepack Block (.PKT) - the "compress=native" codec, the block chain
// that carries it, and the member slice on top.  The cpio payload that the
// unwrapped stream turns out to be is the CALLER's business; the codec and
// chain layers below start at a block header and stop at the end of the chain.
//
// The container is a 79-byte ASCII header line and then the chain:
//
//   "    79 TFPB-v2.01 Tivoli Filepack Block.  fpname=\"-\" cksum=md5 "
//   "compress=native\n"
//
// (the leading field is the record length, 79, right justified in six columns).
//
// TRAP - THE MEMBERS DO NOT EXIST ANYWHERE IN THE FILE.  A member's bytes only
// appear once the whole block chain has been unwrapped: the chain is a single
// stream, blocks are not aligned to members, and each block's last eight bytes
// are a running-MD5 tail rather than payload.  There is therefore no file
// extent that addresses a member, and no way to decode one member without
// decoding every block before it.  A record consequently publishes the WHOLE
// FILE as its stream (the xuleadarchive.cpp pattern) plus the member's position
// inside the DECODED stream as its compress-properties blob, the eight little
// endian bytes memberProperties() builds:
//
//   u32 offset  - the member's first byte inside the unwrapped stream
//   u32 size    - its length there, which is also its uncompressed size
//
// decodeMember() is that whole path in one call and is what a dispatch should
// use; it unwraps and then slices, so two members of one archive each cost a
// full unwrap.  The alternative - publishing the block extents and letting the
// caller stitch - cannot work, because a member routinely starts in the middle
// of a block whose own decode depends on the blocks before it.
//
// compress=native
// ---------------------------------
// LZ77 over a 4096-byte ring buffer PRESET TO 0x00 - not 0x20, not the LZSS
// habit of an ASCII fill - with 16-bit LITTLE-endian flag words consumed LSB
// first, 16 items per word:
//
//   flag 0 -> one literal byte
//   flag 1 -> two bytes b0,b1:
//               length   = (b0 & 0x0f) + 1        (1..16)
//               distance = ((b0 >> 4) << 8) | b1  (12 bits)
//               source   = (position - distance) & 0xfff
//
// THE TRAP IS IN THAT LAST LINE: there is NO -1 bias on the distance.  Nearly
// every other 12-bit LZ77 in this tree computes (position - distance - 1), so
// the natural thing to write here is wrong by one byte per match and the output
// only diverges once the first match lands - far enough in that a short sample
// can still look plausible.  A distance of 0 is therefore legal and means "the
// byte about to be written", which is how the format spells a run.
//
// The codec is driven by a COMPRESSED byte budget, not by an output length: it
// stops when the block's input allowance is spent.  The budget is charged for
// the flag words as well as the items, and because a budget can run out in the
// middle of an item the reader legitimately overshoots it by up to three bytes;
// the caller must resume from the returned offset, never from offset + budget.
//
// Block chain
// -----------------------------
//   2 bytes BIG endian, h:
//     h == 0              end of chain
//     (h & 0x8000) == 0   compressed block, h = compressed byte budget
//     (h & 0x8000) != 0   stored block, length = h & 0x7fff
//
// The last 8 BYTES OF EACH BLOCK ARE NOT PAYLOAD.  They are the first 8 bytes
// of the MD5 digest of every payload emitted so far - ONE running MD5 context
// over the whole stream, snapshotted and finalised per block, not a per-block
// digest - so the digest can only be checked by hashing incrementally in block
// order.  Feeding the digest bytes back into the hash, or restarting the
// context per block, both produce a mismatch on the second block only, which is
// why a one-block sample proves nothing.
class XTivoliDecoder {
public:
    enum {
        RING_SIZE = 0x1000,
        // The reference implementation decompresses a block into a fixed 0x4000 buffer, so that is a hard
        // format limit and not a guess; without it a 0x7fff-byte budget could
        // expand to a quarter of a megabyte.
        MAX_BLOCK_SIZE = 0x4000,
        DIGEST_TAIL_SIZE = 8,
        // Length of the ASCII header line the block chain starts after.
        HEADER_SIZE = 0x4f,
        // Size of the compress-properties blob memberProperties() builds.
        MEMBER_PROPERTIES_SIZE = 8
    };

    // The whole of baPacked is one "compress=native" stream. Succeeds only when
    // exactly nUncompressedSize bytes come out.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // One block body. nPackedSize is the COMPRESSED byte budget, nMaxOutput
    // caps the expansion. *pnNextOffset gets the offset actually reached, which
    // may be up to three bytes past nOffset + nPackedSize (see above).
    static bool decodeNative(const QByteArray &baPacked, qint64 nOffset, qint64 nPackedSize, qint64 nMaxOutput, QByteArray *pbaResult, qint64 *pnNextOffset,
                             XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Walk the block chain from nOffset (i.e. from just past the 79-byte header
    // line) and concatenate the payloads. bVerifyDigest checks the running MD5
    // tail of every block, which is the only integrity signal the format has.
    static bool unwrapBlocks(const QByteArray &baFile, qint64 nOffset, QByteArray *pbaResult, bool bVerifyDigest = true, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // The reference detector does not compare the whole header line, only six
    // anchors in it; a producer is free to vary the fpname field between them.
    static bool checkHeader(const QByteArray &baFile);

    // checkHeader() plus unwrapBlocks() over everything after the header line.
    static bool unwrapFile(const QByteArray &baFile, QByteArray *pbaInner, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // The eight byte compress-properties blob described above, and its inverse.
    static QByteArray memberProperties(qint64 nStreamOffset, qint64 nSize);
    static bool parseMemberProperties(const QByteArray &baProperties, qint64 *pnStreamOffset, qint64 *pnSize);

    // Whole path for one member: unwrap the file, then slice.  baPacked is the
    // ENTIRE container and baProperty is the blob above; an empty blob means
    // "the entire unwrapped stream".  Returns false unless exactly
    // nUncompressedSize bytes came out.
    static bool decodeMember(const QByteArray &baPacked, qint64 nUncompressedSize, const QByteArray &baProperty, QByteArray *pbaResult,
                             XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTIVOLIDECODER_H
