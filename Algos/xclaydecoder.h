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
#ifndef XCLAYDECODER_H
#define XCLAYDECODER_H

#include "../xbinary.h"

// Codec of the Clay ("Clay"/"ClayE") member stream.  LZ77 over a small ring
// with four FIXED, LSB-first prefix codes that the format never transmits;
// they are reproduced here as tables.
//
// Stream layout
//   byte 0  literal mode.  0 = literals are plain 8-bit, 1 = literals come
//           through the 256-entry literal code.  No other value is legal.
//   byte 1  window exponent, only 4, 5 or 6.  The ring is 0x40 << e bytes,
//           so 1 KiB, 2 KiB or 4 KiB, and the same e is the width of the
//           low half of a distance.
//   then    a bit stream, LSB-first.
//
// Token
//   bit 0 -> literal (see byte 0).
//   bit 1 -> match.  A 16-entry length class gives base + extra bits; the
//           value 0x207 (the largest the table can express) is the end of
//           stream marker, not a length.  A 64-entry distance class gives
//           the high half; the low half is `e` bits, except for a length of
//           exactly 2, where it is 2 bits and the class is scaled by 4.
//           The match source is (writePos - distance - 1) masked to the ring,
//           and it is copied byte by byte, so a match may overlap itself.
//
// reconstructed from the format's decompressor (VA 0x0043e3d0 / 0x0043eca0) and confirmed
// against every one of the 398 reference archives: all decode, and 393 are
// byte-identical to the reference implementation's output - the other five differ only because the reference implementation
// collapses members that share a name when it writes them to disk.
class XClayDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XCLAYDECODER_H
