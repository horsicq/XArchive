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
#ifndef XZCMPDECODER_H
#define XZCMPDECODER_H

#include "../xbinary.h"

// Zcmp - the payload codec of the Solaris "compressed file" wrapper.  The
// 40-byte big-endian header and the seek index behind it are XZcmpArchive's
// business; this class starts at the first byte of real data.
//
// The record publishes the DATA REGION - the bytes after the header and after
// the skipped (blockCount + 1) * 8 byte index - and that region is blockCount
// zlib streams BACK TO BACK, each inflating to exactly the header's block size
// except the last, which yields the remainder.
//
// TRAP - THERE ARE NO LENGTHS IN FRONT OF THE STREAMS.  A block ends where zlib
// says it ends and the next one starts at the very next byte, so the only way
// to find the boundary is to inflate and read total_in back off the stream.
// That is why this drives inflateInit2(&s, 15) per stream itself instead of
// handing the whole region to a generic one-shot zlib method, which would stop
// at the end of the first block and report the first block size as the answer.
class XZcmpDecoder {
public:
    enum {
        // Output staging buffer per inflate() call.
        CHUNK_SIZE = 0x10000
    };

    // Inflate the back-to-back zlib streams of the DATA region and concatenate
    // them. Returns false unless exactly nUncompressedSize bytes came out.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XZCMPDECODER_H
