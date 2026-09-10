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
#ifndef XXEDITPACKDECODER_H
#define XXEDITPACKDECODER_H

#include "../xbinary.h"

// The CMS COPYFILE PACK / XEDIT PACK byte codec (the payload of an XEDIT PACK
// container, i.e. everything after its 8-byte header).
//
// It is a plain run/literal coder with no dictionary and no bit packing:
//
//   0x00..0x77  write (c + 1) EBCDIC blanks (0x40)
//   0x78  u8  n         write (n + 1) blanks
//   0x79  u16 n         write (n + 1) blanks
//   0x7A  u8  n, u8 b   write b, (n + 1) times
//   0x7B  u16 n, u8 b   write b, (n + 1) times
//   0x7C..0x7F          exact aliases of 0x78..0x7B
//   0x80..0xF7          copy (c - 0x80 + 1) literal bytes
//   0xF8  u8  n         copy (n + 1) literal bytes
//   0xF9  u16 n         copy (n + 1) literal bytes
//   0xFC, 0xFD          exact aliases of 0xF8, 0xF9
//   0xFF                end of stream
//   0xFA, 0xFB, 0xFE    malformed; the walk stops, keeping what it produced
//
// Three details are load bearing and none of them is guessable:
//
//  * EVERY COUNT IS BIASED BY ONE.  A stored n means n + 1 items, in all ten
//    counted opcodes as well as in the two count-in-the-opcode ranges.  Drop
//    the bias and the output is short by one byte per opcode, which still
//    "looks" like text and only shows up on a byte compare.
//  * THE 16-BIT COUNTS ARE BIG ENDIAN.  This is a mainframe format; the
//    container header is big endian too, and a little-endian read produces a
//    plausible-but-wrong length rather than an error.
//  * A TRUNCATED STREAM IS NOT AN ERROR.  The reference extractor writes
//    whatever it had produced when the input ran out, and - the part that is
//    easy to get wrong - a LITERAL COPY that runs past the end still appends
//    the bytes that ARE there before it gives up, while a run opcode whose
//    operands are incomplete appends nothing at all.  measure() and decode()
//    must agree on that or the two disagree on the length of a damaged member.
//
// The output stays in EBCDIC; no code-page translation is applied, because the
// reference extractor applies none.
class XXEditPackDecoder {
public:
    // Hard ceiling on the plaintext.  A crafted stream of three-byte run
    // opcodes expands by up to 0x10000:3, so the length has to be capped
    // independently of the input size.
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x10000000;

    // Walk the stream without storing anything and report the plaintext
    // length.  The container does NOT store it, so this is how the reader
    // learns the member size.  False means the walk overflowed the ceiling or
    // produced nothing.
    static bool measure(const QByteArray &baPacked, qint64 *pnUncompressedSize, XBinary::PDSTRUCT *pPdStruct = nullptr);

    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XXEDITPACKDECODER_H
