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
#ifndef XLZDIETDECODER_H
#define XLZDIETDECODER_H

#include "xbinary.h"

/*--
   "lZdIeT" chunked container payload.

   The decoder is handed the WHOLE container, because the chunk table is part of
   the payload description rather than of any single member:

     0x00  char   szMagic[6]        "lZdIeT"
     0x06  qint32 nUncompressedSize
     0x20  { qint32 nChunkOffset; quint16 nChunkSize; } [250]
                                    terminated by nChunkOffset == -1
     0x5FC first chunk

   nChunkSize counts a 6-byte per-chunk preamble that is NOT part of the code
   stream, so the stream is nChunkSize - 6 bytes long at nChunkOffset + 6.

   Each chunk is an INDEPENDENT textbook LZW stream, packed LSB-first, with a
   256-entry root alphabet, clear code 0x100, end code 0x101, first assignable
   code 0x102, code width 9 growing to a hard maximum of 10 bits (1024 entries;
   the dictionary simply stops growing there - there is no automatic reset).
   The width step happens BEFORE the code that needs it is read, with no
   "early change" slack.  Chunk state - dictionary, width, previous code - is
   rebuilt from scratch for every chunk; only the output stream is continuous.

   The last chunk decodes past the declared size (the compressor pads it out to
   its block boundary), so decoding stops at nUncompressedSize exactly like the
   reference extractor's size-limited output stream does.

   Written from the container layout and the decoder's own state machine; no
   third-party code is used here, so the file stays inside the tree's MIT
   boundary.
--*/

class XLZDIETDecoder {
public:
    static const qint64 LZDIET_HEADER_SIZE = 0x5FC;
    static const qint64 LZDIET_TABLE_OFFSET = 0x20;
    static const qint32 LZDIET_MAX_CHUNKS = 250;
    static const qint64 LZDIET_ENTRY_SIZE = 6;
    static const qint64 LZDIET_CHUNK_PREAMBLE = 6;

    // baContainer is the whole file; nUncompressedSize is the header's own
    // declared size.  Returns true only when exactly that many bytes came out.
    static bool decode(const QByteArray &baContainer, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XLZDIETDECODER_H
