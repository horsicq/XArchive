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
#ifndef XCHIEFLZDECODER_H
#define XCHIEFLZDECODER_H

#include "../xbinary.h"

// ChiefLZ method 4.
//
// The probability model is the SAME order-0 adaptive Huffman that ARCV4
// method 2 uses (see Algos/xarcv4decoder.*): a 1-based heap where the leaf of
// symbol s lives at node s + SYMBOLS, every node starts at weight one, an
// update swaps a node with its lighter uncle, and all weights halve the moment
// the root reaches exactly 2000.  Two things differ, and they are why this is
// a separate decoder rather than a parameter on that one:
//
//   * the alphabet is 629 symbols, not 3245 - 256 literals, an end marker at
//     256, then six distance buckets of SIXTY-TWO lengths each, so a match is
//     3..64 bytes rather than 3..500;
//   * THE BITS ARRIVE MSB-FIRST OUT OF A LITTLE-ENDIAN 16-BIT WORD, where
//     ARCV4 reads them LSB-first out of single bytes.  Reading this stream the
//     ARCV4 way decodes a couple of hundred plausible bytes and then diverges
//     into zeros, which looks like a broken model rather than a bit order.
//
// The distance buckets are shared: extra widths {4,6,8,10,12,14} over bases
// {0,16,80,336,1360,5456}, and the match length is FOLDED INTO the distance
// (dist = base + extra + length), so a match never overlaps its own output.
// 32 KiB window.  Recovered from VA 0x0059d670 and byte-exact on all ten
// reference archives.
class XChiefLZDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XCHIEFLZDECODER_H
