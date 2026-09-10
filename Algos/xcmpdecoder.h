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
#ifndef XCMPDECODER_H
#define XCMPDECODER_H

#include "../xbinary.h"

// The two codecs of the single-member ".CMP" container (header word 0x007F).
//
// Method 1 frames the payload: a 16-bit little-endian byte count, then that
// many bytes of 16-bit-max LZW (MSB-first, clear code 0x100, end code 0x101),
// repeated until the member is consumed.  Each frame restarts the dictionary.
//
// Method 2 is an LZSS over a 2048-byte ring driven by 9-bit tokens read
// MSB-first:
//   token < 0x100          literal byte
//   low byte == 0x80       end of stream
//   low byte == 0x81       repeat the byte just written, length as below
//   low byte  < 0x80       distance = low * 16 + four more bits
//   otherwise              distance = low & 0x7F
// The length is a two-bit value; a value of 3 takes two more bits, and if
// those are also 3 it keeps adding nibbles while each nibble is 15.  Two is
// then added, so the shortest match is two bytes.
//
// Both were read out of the reference decompressors (VA 0x005418c0 and
// 0x005194a0).  Two things the reference gets wrong are corrected here.  A
// method 1 frame whose byte count reaches the 4096-byte block size is a STORED
// block and is copied out whole; the reference feeds it to the LZW decoder and
// loses that block and, when the block decodes to nothing, the rest of the
// member.  And the newer container variant does not use the method 2 LZSS at
// all - it stores a PKWARE DCL stream, which XCMPArchive routes to
// HANDLE_METHOD_PKWARE_DCL_IMPLODE instead of calling decodeLZSS.
class XCMPDecoder {
public:
    // Method 1: framed LZW.  A frame of 4096 bytes or more is a stored block
    // and is copied out verbatim; anything shorter is an LZW stream.  Stops
    // when the frames run out or the declared size is reached, whichever comes
    // first - an empty frame is not a stopping condition, the frames after it
    // still carry data.
    static bool decodeFramedLZW(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Method 2: 9-bit LZSS.  Returns true only when the declared size is met;
    // whatever was produced is still handed back so a truncated member yields
    // the same prefix the reference implementation writes.
    static bool decodeLZSS(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XCMPDECODER_H
