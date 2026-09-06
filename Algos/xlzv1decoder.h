/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XLZV1DECODER_H
#define XLZV1DECODER_H

#include "xbinary.h"

/*--
   "LZV1" single-file container payload.

   The decoder is given the WHOLE container, because the dictionary ceiling is a
   header field:

     0x00 char  szMagic[4]     "LZV1"
     0x04 quint8 fixed[6]      5D 19 01 AD 00 00 in every known file
     0x0A quint16 nMaxCodes    BIG endian, 0x100 < value < 0x4000
     0x0C code stream

   The stream is an LZW variant with three properties that rule out every LZW
   decoder already in the tree:

     * bits are consumed MSB-first out of whole bytes;
     * codes are PHASED-IN binary, not fixed width.  With nNext codes defined
       and nLimit == the current power of two, a code is read as nBits bits and
       mapped by  v = raw + nLimit - nNext - 1; when that is negative one extra
       bit is taken and  v = 2 * (raw + nLimit) - nNext - 1 + bit.  The width
       grows when nLimit * 2 <= nNext + 1;
     * there is no clear code and no end code.  When the dictionary reaches
       nMaxCodes - 1 entries the coder simply restarts - 8-bit root alphabet,
       nLimit = nNext = 0x100 - and the stream ends when the input runs out.

   A dictionary entry keeps THREE fields: the previous code, the last character
   of the string, and its first character; the new entry is built from the
   previous code and the first character of the code that follows it, which is
   also how the KwKwK case (v == nNext) is resolved.

   Written from the container layout and the decoder's own state machine; no
   third-party code is used here, so the file stays inside the tree's MIT
   boundary.
--*/

class XLZV1Decoder {
public:
    static const qint64 LZV1_HEADER_SIZE = 12;
    static const qint32 LZV1_MAX_CODES_LIMIT = 0x4000;
    // The container stores no unpacked size, so the decoder streams; this is
    // the ceiling it refuses to pass.
    static const qint64 LZV1_MAX_OUTPUT_SIZE = 0x40000000;  // 1 GiB

    // Header-only sanity check shared with XLZV1::isValid().
    static bool checkHeader(const char *pHeader, qint64 nHeaderSize, qint32 *pnMaxCodes);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XLZV1DECODER_H
