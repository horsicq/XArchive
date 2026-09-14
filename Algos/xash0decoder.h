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
#ifndef XASH0DECODER_H
#define XASH0DECODER_H

#include "../xbinary.h"

// Codec of the Nintendo "ASH0" single-stream container (Wii System Menu,
// Animal Crossing: City Folk, My Pokemon Ranch).  Format understanding derived
// from ASH0-tools by Garhoogin and NinjaCheetah (MIT) and from the ASH Extractor
// 0.1 disassembly of the Wii System Menu decoder by crediar; no code of either
// is reused.
//
// The packed buffer handed to decode() is the WHOLE file image from offset 0,
// magic included, because the header carries the two values the codec needs:
//
//   0x00  'A' 'S' 'H' '0'
//   0x04  u32 BE  bits 23..0 = plaintext length (1 .. 0xFFFFFF); bits 31..24 =
//                 a top byte the System Menu masks off and never reads
//   0x08  u32 BE  absolute offset of the distance bit stream (>= 0x10)
//   0x0C          symbol bit stream, up to the distance offset
//   dist          distance bit stream, to the end of the file
//
// Both streams are read as big-endian 32-bit words, bit 31 first, which is
// bit-identical to "bytes in file order, MSB first".  Each stream opens with a
// serialised Huffman tree - pre-order, bit 1 = internal node (left subtree then
// right subtree follow), bit 0 = leaf followed by `width` bits of value - and
// continues with the codes.  Symbol leaves are 9 bits: 0x000-0x0FF literals,
// 0x100-0x1FF match lengths 3..258.  Distance leaves are 11 bits (System Menu,
// City Folk) or 15 bits (Pokemon Ranch), value + 1 = distance.  Nothing in the
// file records which distance width was used.
//
// The reference readers are EAGER: they fetch the next word the moment the
// 32nd bit of the current one is consumed, so a file whose distance stream is
// an exact multiple of 32 bits makes ashdec read past EOF and reject its own
// compressor's output (11 of 400 measured).  This reader is LAZY - it fetches
// only when a bit is needed and fails only when a needed bit lies past the
// stream limit - so it accepts every file the eager one accepts plus those.
// The symbol reader is additionally limited to the bytes before the distance
// offset; a valid file never needs a symbol bit from there.
//
// Width detection.  The plaintext LENGTH depends on the symbol stream alone, so
// a wrong distance width can decode to the declared size with wrong bytes.  A
// candidate is "tight" when the distance reader's last consumed bit lies in the
// last 4-byte word of the distance stream.  Reading WIDER than the truth
// consumes four extra bits per leaf, desyncs the tree and dies early on an
// illegal distance; reading NARROWER under-consumes and yields small, plausible
// distances.  The auto policy therefore tries 15 first, then 11, accepting the
// first tight decode; failing that, a lone loose decode; failing that, the
// wider loose one (flagged bAmbiguous).  Measured on 800 + 24 generated files:
// right bytes every time.  The reader stores the width it found in
// FPART_PROP_WINDOWSIZE (2048 / 32768) so the extraction path can try it first.
//
// Hostile-input checks: header sanity (magic, size >= 1, distance offset within
// [0x10, size - 4]), an expansion-ratio guard before any allocation, lazy
// reader overruns, tree stack underflow (a leaf with nothing to attach it to),
// too many internal nodes (table overflow in the reference), duplicate leaves,
// copy length past the declared end, copy distance before the start, and
// cancellation through the PDSTRUCT every 0x4000 symbols.
class XASH0Decoder {
public:
    struct HEADER {
        qint64 nUncompressedSize;
        qint64 nDistOffset;
        quint8 nTopByte;
    };

    struct RESULT {
        qint32 nSymBits;
        qint32 nDistBits;
        bool bTight;
        bool bAmbiguous;
        quint8 nTopByte;
    };

    // Header checks over the first nHeaderSize bytes of a container that is
    // nPackedSize bytes long.  Shared by the reader's cheap parse.
    static bool parseHeader(const quint8 *pHeader, qint64 nHeaderSize, qint64 nPackedSize, HEADER *pResult);

    // Dispatcher entry: baPacked is the whole file image from offset 0.  The
    // distance width is auto-detected.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Same with a width hint: nDistBitsHint 11 or 15 is tried FIRST, then the
    // other one; 0 = plain auto order (15 then 11).  pResult may be null.
    static bool decodeEx(const QByteArray &baPacked, qint64 nUncompressedSize, qint32 nDistBitsHint, QByteArray *pbaResult, RESULT *pResult,
                         XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Reader-side trial decode of the detection gate: the bytes are discarded,
    // the RESULT carries the width for the FPART properties.
    static bool probe(const QByteArray &baPacked, RESULT *pResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Fixed-width core used by the three above: one attempt, no search.
    static bool decodeWithBits(const QByteArray &baPacked, qint64 nUncompressedSize, qint32 nSymBits, qint32 nDistBits, QByteArray *pbaResult,
                               bool *pbTight, XBinary::PDSTRUCT *pPdStruct);
};

#endif  // XASH0DECODER_H
