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
#ifndef XWPKDECODER_H
#define XWPKDECODER_H

#include "../xbinary.h"

// WPK - the Watcom installer "pack" archive (magic 03 24 01 01 / 03 24 33 01).
// Its directory record flag byte at +0x10 carries the member's method in bit 7:
// clear -> method A, set -> method B.  Both methods are LZSS over a 4096-byte
// ring buffer preset to 0x20 (' '), read MSB first, and both share LHA's "-lh1-"
// position tables for the distance:
//
//     i    = next 8 bits
//     n    = d_len[i >> 4] - 2 further bits, shifted into i (byte arithmetic)
//     dist = d_code[i] * 64 + (i & 0x3F)
//     src  = (pos - dist - 1) & 0xFFF
//
// Method A adds a Huffman coded literal/length alphabet of at most 0x13B
// symbols whose code lengths are transmitted first, as one count byte followed
// by count + 1 control bytes:
//
//     b & 0x80 == 0 : the next (b >> 4) + 1 symbols all get length (b & 0xF) + 1
//     b & 0x80 != 0 : skip (b & 0x7F) + 1 symbols, which get no table entry
//
// Method B has no Huffman stage at all: one flag bit, then either eight literal
// bits or a six-bit raw match length followed by the same distance encoding.
//
// Three things here are load bearing and none of them is guessable:
//
//  * THE CODE-LENGTH SORT IS UNSTABLE AND ITS TIE-BREAK IS PART OF THE FORMAT.
//    Sorting the table by length ascending is not enough: the order the sort
//    happens to leave symbols in WITHIN a run of equal lengths is exactly what
//    decides which symbol gets which canonical code.  So the two unstable
//    quicksorts the reference implementation carries are transliterated
//    instruction for instruction rather than replaced - a "clean" std::sort or
//    any stable sort produces a table that is wrong for every real member, and
//    wrong quietly, because it still decodes to something of plausible length.
//    SORTER_STABLE is kept only to complete the probe; nothing wants it.
//  * WHICH SORTER AN ARCHIVE WANTS IS NOT IN ITS HEADER.  All 127 archives of
//    the reference corpus carry magic 0x01012403, yet 122 of them (394 members)
//    decode with SORTER_A and the other 5 (9 members) only with SORTER_B.  The
//    probe below is therefore mandatory, not an optimisation: picking SORTER_A
//    because it works on the first archive you try silently corrupts the rest.
//  * CANONICAL CODES ARE HANDED OUT BACKWARDS, from the LAST table entry to the
//    first, in a 16-bit left-justified space: code starts at 0 and grows by
//    1 << (16 - length) as it walks down.  Assigning them forwards the usual way
//    inverts the whole alphabet.
//  * A MATCH LENGTH IS symbol - 0xFD, not symbol - 0x100 or symbol - 0xFF, so
//    the first match symbol means three bytes.
//
// The directory's CRC-32 field covers the COMPRESSED bytes the decoder
// consumed, not the plaintext, which is why decode reports *pnConsumed: the
// caller CRCs baPacked's first *pnConsumed bytes and compares.  That is also the
// only way to run the sorter probe: on the archive's first method A member try
// SORTER_A, SORTER_B, SORTER_STABLE in that order and keep the first whose
// output is nUncompressedSize bytes long AND whose consumed-prefix CRC matches,
// then reuse that choice for the rest of the archive.  Magic 0x01332403 is
// documented to force SORTER_B without probing, but no archive of the reference
// corpus carries it, so that shortcut is untested.
class XWPKDecoder {
public:
    enum SORTER {
        SORTER_A = 0,      // simple quicksort with an explicit stack
        SORTER_B,          // BSD-style qsort: shell sort < 16, median of 3/9
        SORTER_STABLE      // stable by length; wrong for every archive seen so far
    };

    // Method A: Huffman coded, bit 7 of the directory flag byte clear.
    static bool decodeMethodA(const QByteArray &baPacked, SORTER sorter, qint64 nUncompressedSize, QByteArray *pbaResult, qint64 *pnConsumed = nullptr,
                              XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Method B: plain LZSS, bit 7 of the directory flag byte set.
    static bool decodeMethodB(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, qint64 *pnConsumed = nullptr,
                              XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XWPKDECODER_H
