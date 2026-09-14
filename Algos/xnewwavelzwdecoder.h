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
#ifndef XNEWWAVELZWDECODER_H
#define XNEWWAVELZWDECODER_H

#include "../xbinary.h"

// Codec of the HP NewWave "<FC>LZW" single-member container - the family the
// ARC8 corpus and the reference tool both call LZWD.  A twelve-bit LZW that is
// textbook in every respect but one:
//
//  * codes are MSB-first and nine bits wide to start; clear is 0x100, end is
//    0x101 and the first phrase slot is 0x102.  The stream ALWAYS opens with a
//    clear, which is why the byte at offset 11 of every member is 0x80 - that
//    is 0x100 in nine MSB-first bits;
//  * the width steps up ONE code early, when the next free slot would be the
//    last one the current width can name (TIFF's "early change") rather than
//    when it no longer fits;
//  * WHEN THE TABLE FILLS THE CODER RESTARTS ITSELF.  The moment the free slot
//    reaches 0xFFF the dictionary, the code width and the previous phrase are
//    all dropped exactly as if a clear had been read, and the encoder then
//    writes an explicit clear AT THE NEW WIDTH, nine bits, which the decoder
//    sees as a no-op.  Handle that as a plain step to a thirteenth bit, or
//    defer the restart to 0x1000, and every member bigger than about 6.5 KB
//    desyncs - always at the same code, because the width schedule is fixed.
//    That is how the rule was found: 24 of a 65-member probe set broke at the
//    identical bit offset.
//
// The last point is why this is not a set of XSharedLZWDecoder::OPTIONS.  That
// decoder answers a full table by widening past nMaxBits and has no flag for
// the restart; folding this in as one more option is the natural follow-up once
// that shared file is not being edited by several workers at once.
class XNewWaveLZWDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XNEWWAVELZWDECODER_H
