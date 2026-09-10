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
#ifndef XLIMDECODER_H
#define XLIMDECODER_H

#include "../xbinary.h"

// LIM method 1 - a blocked Huffman + LZ77, deflate-shaped but with its own
// tables and two details that are easy to get wrong.
//
// A block is a 14-bit symbol count (zero ends the stream), then three
// canonical Huffman tables read in sequence, then that many symbols.  Symbols
// below 0x100 are literals; 0x100 and up index a 41-entry table where the
// first thirteen entries are DISTANCE bases with (index - 1) extra bits and a
// fixed length of three, and the rest are LENGTH values whose high byte, when
// non-zero, is the extra-bit count for the low byte - with a second table
// supplying the distance.
//
// THE STREAM BIT IS THE COMPLEMENT OF THE CODE BIT.  The reference builds its
// decode tree putting a 0 bit in the first child slot and a 1 bit in the
// second, then reads them from the opposite slots, so a canonical code walked
// against this stream must invert every bit.  Reading it the ordinary way
// still traverses a valid-looking tree often enough to decode a few hundred
// plausible bytes before failing, which reads as a table bug rather than a
// polarity one.
//
// THE WINDOW CARRIES A SECOND, PARALLEL ARRAY of per-position match-length
// history.  A match's real length is its table length PLUS the history value
// at the source position (and four more if that sum overflows a byte); after
// copying, the history is cleared across the destination and a descending ramp
// is written at the source.  That feedback is part of the format - dropping it
// yields lengths that are short by a variable amount.
class XLIMDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XLIMDECODER_H
