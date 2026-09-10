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
#ifndef XC64WRAPTORDECODER_H
#define XC64WRAPTORDECODER_H

#include "xbinary.h"

// The LZSS of the Commodore 64 "Wraptor" container.  MSB-first bits, a 4096
// byte ring buffer that starts ZERO FILLED (early matches legitimately copy
// those zeros, so the buffer is part of the format and not just an
// optimisation) and an offset field whose width GROWS during the stream:
//
//   bit 0            an 8-bit literal
//   bit 1            `width` bits of offset, starting at 8
//                      offset 0   one more bit: 0 ends the stream, 1 widens
//                                 the offset field by one and the stream
//                                 continues; reaching 13 is an error
//                      offset > 0 five bits of length, then offset - 1 is the
//                                 ring position to copy from, advancing per
//                                 byte
//
// Both literals and copied bytes go into the ring at the write cursor.
//
// NEITHER THE MEMBER LENGTH NOR THE PACKED LENGTH IS STORED ANYWHERE in the
// container, so scan() exists to walk a stream and report both.  The walk is
// how the reader finds the next member at all, which is why identification of
// this format costs a full pass over every member.
class XC64WraptorDecoder {
public:
    // Walks a stream without keeping the output.  *pnConsumed is the number of
    // packed bytes the stream occupies, which is where the member's trailing
    // checksum begins.
    static bool scan(const quint8 *pData, qint64 nSize, qint64 nOffset, qint64 nMaxOutputSize, qint64 *pnConsumed, qint64 *pnRawSize,
                     XBinary::PDSTRUCT *pPdStruct = nullptr);

    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XC64WRAPTORDECODER_H
