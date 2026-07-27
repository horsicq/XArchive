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
#ifndef XXPRESSDECODER_H
#define XXPRESSDECODER_H

#include "xbinary.h"

// Microsoft XPRESS decompressors ([MS-XCA]).
//  - Plain: LZ77 with a nibble flag stream ("XPRESS LZ77", MS-XCA 2.1).
//  - Huffman: LZ77 + a 512-symbol Huffman table per chunk (MS-XCA 2.2), the
//    variant used by WIM XPRESS resources.
// Both decode a single chunk whose uncompressed size is known by the caller.
class XXPressDecoder {
public:
    static bool decompressPlain(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint32 nUncompressedSize);
    static bool decompressHuffman(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint32 nUncompressedSize);
};

#endif  // XXPRESSDECODER_H
