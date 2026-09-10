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
#ifndef XPAKLEODECODER_H
#define XPAKLEODECODER_H

#include "../xbinary.h"

// LEOLZW - the codec of PAKLEO (.PLL), "LEOLZW - (c) Leonardus Leonardi 1993".
//
// Classic LZW over an 8-bit alphabet, codes assembled MSB FIRST out of a
// one-byte-at-a-time refill, INITIAL WIDTH 9, first free code 0x103, table
// capacity 0x8000.  Three control codes, and the one that matters is 0x101:
//
//   0x100  end of stream
//   0x101  WIDEN the code by one bit, capped at 15.  The width is NEVER bumped
//          implicitly by the free-code counter - it only ever moves when this
//          code appears in the stream.  A textbook "step when the next free
//          slot no longer fits" coder desynchronises at the first 512-code
//          boundary, which is why this codec cannot ride the shared
//          XSharedLZWDecoder.
//   0x102  clear: the table goes back to 0x103, the width back to 9, and the
//          very next code is read at 9 bits and is a plain literal.
//
// A new entry is prefix = previous code, suffix = FIRST byte of the phrase the
// current code expands to; KwKwK is handled the usual way.  Decoding stops when
// nUncompressedSize bytes have been produced, on 0x100, or on a short read.
//
// CORPUS NOTE.  Both PAKLEO files in the reference corpus are STORED, so this
// path is not exercised by the corpus at all; it was validated separately
// against the reference extractor using synthetic LEOLZW archives.
class XPAKLEODecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x40000000;

    // Returns false unless exactly nUncompressedSize bytes came out.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XPAKLEODECODER_H
