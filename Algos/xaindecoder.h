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
#ifndef XAINDECODER_H
#define XAINDECODER_H

#include "../xbinary.h"

// AIN's LZH: an LHA-shaped blocked Huffman + LZ77 over a 0x8000 window, read
// LSB-first (bytes are OR'd into the accumulator at the current bit count and
// values are taken from the low bits).
//
// A block is two canonical Huffman tables, each preceded by a 19-symbol
// pre-tree in the LHA manner: 5 bits give 19 minus the pre-tree symbol count,
// then 3 bits per length with 7 extended by unary continuation; 9 bits then
// give the symbol count as (alphabet size minus n), and pre-tree symbols 1 and
// 2 are runs of zero lengths (4 bits + 3, and 9 bits + 0x14), 0 is a single
// zero and anything else is the length minus two.
//
// The first alphabet holds 272 symbols: below 0x100 a literal, 0x100 and 0x101
// a distance of one or two, and from 0x102 a distance whose top bit is implied
// - symbol 0x100 + n + 1 reads n extra bits and ORs in 1 << n.  All distances
// are then incremented, so the alphabet exactly spans the window.  The second
// alphabet holds 254 symbols and gives the match length plus three.
//
// 0x3FFF read in the widest distance field (symbol 0x10F, 14 extra bits) is an
// ESCAPE, not a distance: one more bit says whether the block ends and new
// tables follow (0) or the whole stream is finished (1).  There is also a
// single throw-away bit before the very first block.
//
// The table builder is worth reading in the original: it
// walks the code space recursively and turns a node into a leaf whenever a
// symbol of exactly that length is still unspent, which yields canonical codes
// without ever forming one.  Two consequences that are easy to miss:
//
//   * within one code length the symbols are consumed in DECREASING index
//     order, because the length buckets are singly linked lists built by
//     pushing symbols in increasing order;
//   * codes longer than eight bits are not in the 256-entry lookup table -
//     the entry for their 8-bit prefix has 0x8000 set and holds a node index
//     into a bit-at-a-time tree instead.
//
// AIN is SOLID: consecutive members share one stream, so decoding member k
// means decoding everything before it in the same group.  nSkipSize is that
// leading amount, which the reader publishes per record.
class XAINDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nSkipSize, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XAINDECODER_H
