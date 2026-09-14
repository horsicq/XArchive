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
#ifndef XCHARCDECODER_H
#define XCHARCDECODER_H

#include "../xbinary.h"

// ChArc - the member codec of S.Chernivetsky's ChArc 1.1 / 1.2 (SP "Dialog",
// Moscow, 1990), the archiver behind the .CHZ container and its "ChSFX
// (small)" self-extractor.  The container - the "SChF" / "SChD" / "SChd"
// record chain - is XCHZ's business; this class starts at a member's stored
// payload and ends with the member's bytes.  Only method 1 comes here; method
// 0 is stored and never reaches a codec.
//
// WHAT IT IS.  An ORDER-1 CONTEXT-MODELLED LZ77 with STATIC per-context
// Huffman codes.  It is NOT adaptive: every table in the stream is transmitted
// in a model header that precedes the first coded byte, and nothing is updated
// afterwards.  There are 259 contexts - one per possible previous byte, plus
// three shared ones at 0x100 (match length AND distance high byte), 0x101
// (distance low byte) and 0x102 (the fallback table every "mode 0" context
// borrows).
//
// BITS.  MSB-first out of a 16-bit accumulator that is topped up ONE BYTE at a
// time, and only when the request does not fit.  No request is ever wider than
// eight bits, which is what makes a single-byte refill sufficient; reading it
// as a conventional 32-bit LSB-first bit reader desynchronises immediately.
//
// THE TABLE FORMAT IS NOT CANONICAL DEFLATE.  A table is a flat byte blob of
// [count][symbol x count] blocks, one block per code length starting at length
// zero, and the decoder walks it with a running "available codes" counter:
// the symbols of a length take the LAST `count` codes of the range still open
// at that length, not the first.  Swapping that convention decodes a plausible
// prefix and then diverges, so it cannot be inferred from a short sample.
//
// THE MODEL HEADER, in order:
//   1 bit   bAllLiteral - 1 means "no tables at all, every context reads raw
//           eight-bit bytes"; 0 means the tables follow.
//   if 0:   4 x 3 bits are the code lengths of symbols 0..3 of a META table,
//           which is then used to read 259 two-bit-ish context MODES (0 =
//           borrow the fallback table, 1 or 2 = raw eight-bit bytes, 3 = a
//           table of its own follows).
//   1 bit   bEscapeTable - 1 means an escape byte for all 256 contexts follows
//           as eight bits, then a run of {1 bit continue, 8 bit value, 8 bit
//           index} overrides.  0 leaves every escape byte at zero.
//   if the first bit was 0: 5 bits give the highest meta symbol, then that many
//           plus one 4-bit code lengths REBUILD the meta table - the same
//           table now codes the per-context code lengths.
//   then one pass over contexts 0x00..0xFF, then 0x100 and 0x101, then 0x102,
//           each mode-3 context spending 256 meta symbols on its own lengths.
//           The 0x00..0xFF and 0x102 passes SKIP any symbol whose own context
//           mode is 2 and leave its length at zero; the 0x100 / 0x101 passes
//           do not.  That asymmetry is load-bearing.
//
// THE CODED STREAM.  3 bits of minimum-length bias, then one raw eight-bit
// byte (the first output byte, which also seeds the context).  After that, per
// step: decode a symbol in the current context.  If it is NOT that context's
// escape byte it is a literal.  If it IS, decode a length code in context
// 0x100: zero means "the escape byte itself, as a literal", otherwise the
// match length is code + bias + 1 and the distance arrives as a low byte in
// context 0x101 and a high byte in context 0x100.  The window is a flat 64 KiB
// ring indexed modulo 0x10000 - there is no "unused window" guard, so a
// distance may legitimately reach back into bytes this member never wrote.
// The context after a match is the LAST byte copied.
//
// Recovered from the reference implementation at VA 0x00574960 and its helpers
// (0x00574540 model header, 0x005741d0 table build, 0x005742b0 symbol decode,
// 0x005743a0 per-context table, 0x00574060 bit reader).  Verified over all 124
// members of the ten-carrier reference corpus: every one decodes to exactly
// its declared length, and the self-describing ones prove the bytes - the
// CHARC.EXE member comes out as a 33306-byte MZ image and its own CHARC.TXT
// documentation states "the size of charc.exe is 33306 bytes".
class XChArcDecoder {
public:
    // baPacked is one member's stored payload, nUncompressedSize its declared
    // decoded length.  Returns false unless exactly that many bytes came out.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XCHARCDECODER_H
