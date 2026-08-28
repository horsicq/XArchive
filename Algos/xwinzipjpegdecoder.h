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
#ifndef XWINZIPJPEGDECODER_H
#define XWINZIPJPEGDECODER_H

#include "xbinary.h"

// Decoder for ZIP compression method 96 (WinZip JPEG recompression).
//
// Implemented from the official public specification "JPEG Compression -
// Method 96" (WinZip Computing, 2008, https://www.winzip.com/static/wz/docs/
// wz-jpg-comp.pdf) and the log-domain binary arithmetic coder of the expired
// U.S. patent 4,791,403 which that specification incorporates by reference.
// The compressed stream is a Properties Header followed by bundles of
// LZMA-compressed JPEG metadata and arithmetic-coded DCT scan data; decoding
// reproduces the original JPEG file bit for bit by re-applying the original
// Huffman entropy coding, including restart markers and byte stuffing.
class XWinZipJPEGDecoder : public QObject {
    Q_OBJECT

public:
    explicit XWinZipJPEGDecoder(QObject *parent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct);
};

#endif  // XWINZIPJPEGDECODER_H
