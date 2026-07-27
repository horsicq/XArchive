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
#ifndef XLZXDECODER_H
#define XLZXDECODER_H

#include "xbinary.h"

// LZX decoder implemented from the documented Microsoft LZX format
// (CAB SDK / [MS-PATCH]). Two container variants are supported:
//  - CAB: continuous folder stream, 24-bit block sizes, optional Intel E8
//    header, bitstream realignment at every 32KB output frame.
//  - WIM: independent chunks (32KB window), 1-bit default-size block header,
//    Intel E8 post-processing always applied with fixed file size 12000000.
class XLZXDecoder {
public:
    // baCompressed: concatenated compressed folder stream (CFDATA payloads joined)
    static bool decompressCABFolder(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint64 nUncompressedSize, qint32 nWindowBits,
                                    XBinary::PDSTRUCT *pPdStruct = nullptr);

    // One independent WIM chunk (chunk uncompressed size <= 32768)
    static bool decompressWIMChunk(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint32 nUncompressedSize);
};

#endif  // XLZXDECODER_H
