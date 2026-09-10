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
#ifndef XWINTERSOFTDECODER_H
#define XWINTERSOFTDECODER_H

#include "../xbinary.h"

// Wintersoft "**++" - the two codecs of the container. Both are verbatim ports of the reference listings in Mark
// Nelson's "The Data Compression Book"; the archiver was evidently built
// straight on top of them.  The 8-byte file header ("**++" plus an ASCII method
// tag, "LZW " or "HUFF") and the size-prefixed member walk belong to the
// caller - the method tag is per FILE, not per member, so one call site picks
// the codec once and uses it for every member.
//
// LZW15V - decodeLZW15V()
// -----------------------
// MSB-first, variable width 9..15 bits, first assignable code 0x103, table cap
// 0x8000.  The reserved codes are where this dialect parts company with every
// other LZW in the tree, and getting them wrong desynchronises the stream
// rather than corrupting a byte here and there:
//
//   0x100 END    stop
//   0x101 BUMP   widen the code by one bit.  THE WIDTH IS EXPLICIT.  A decoder
//                that widens on its own when nextCode reaches (1 << width) -
//                the Unix compress / GIF habit - reads the very next code at
//                the wrong width and never recovers.
//   0x102 FLUSH  reset the dictionary to nextCode 0x103 / width 9, and the code
//                that FOLLOWS is a plain literal that restarts the string, not
//                an ordinary lookup.
//
// The bump threshold starts at 0x1ff and becomes (1 << width) - 1 on each bump,
// except at width 15 where it is pinned to 0x8000 so no further bump can fire.
// The reference implementation also auto-bumps before reading whenever threshold < nextCode; against a
// Nelson-compatible encoder that is exactly one code short every time and can
// never fire, but it is reproduced here so a stream produced by some other
// encoder decodes the way the reference implementation decodes it.
//
// AHUFF - decodeAHUFF()
// ---------------------
// Adaptive (FGK-style) Huffman over 0..255 plus 0x100 END and 0x101 ESCAPE.
// The tree starts holding only END and ESCAPE; a byte not yet in the tree
// arrives as ESCAPE followed by EIGHT RAW BITS, after which the ESCAPE leaf is
// split to make room for it.  Weights on the path to the root are bumped after
// every symbol and nodes are swapped to keep the array sorted by descending
// weight; when the ROOT weight reaches 0x8000 the whole tree is rebuilt with
// every leaf weight halved as (w + 1) >> 1.
//
// UNVERIFIED: all 25 samples of the reference corpus are "LZW " with a single
// member, so decodeLZW15V() is byte-proven and decodeAHUFF() IS NOT.  It is a
// careful transcription of the decompiled the reference implementation routine (== Nelson's AHUFF.C) and
// nothing more; the rebuild path in particular has never been executed against
// real data, since it only fires after 0x8000 symbols.  Treat a first failure
// here as a bug in this file, not in the sample.
class XWintersoftDecoder {
public:
    enum {
        LZW_END_OF_STREAM = 0x100,
        LZW_BUMP_CODE = 0x101,
        LZW_FLUSH_CODE = 0x102,
        LZW_FIRST_CODE = 0x103,
        LZW_TABLE_CAP = 0x8000,
        LZW_MIN_CODE_BITS = 9,
        LZW_MAX_CODE_BITS = 15,
        AHUFF_END_OF_STREAM = 0x100,
        AHUFF_ESCAPE = 0x101,
        AHUFF_MAX_WEIGHT = 0x8000
    };

    // Nelson LZW15V. Succeeds only when exactly nUncompressedSize bytes come
    // out; the byte supply is the whole of baPacked, i.e. the member's stored
    // compressed size, which is authoritative in this container.
    static bool decodeLZW15V(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Nelson AHUFF. See the UNVERIFIED note above.
    static bool decodeAHUFF(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XWINTERSOFTDECODER_H
