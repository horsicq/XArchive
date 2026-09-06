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
#ifndef XLZPIS2DECODER_H
#define XLZPIS2DECODER_H

#include "xbinary.h"

// "LZPIS2" chunked container codec (DOS/Windows installer payloads; the last
// character of the packed file's extension is replaced by '$').
//
// Container:  the 6-byte ASCII magic, then a chain of chunks, each introduced
// by { u16 LE uncompressed size (1..0x1000), u16 LE compressed size } followed
// by that many compressed bytes.  The chain tiles the file exactly to EOF.
//
// Chunk codec: LZHUF - Okumura/Yoshizaki LZSS driven by an adaptive Huffman
// tree - with this family's own parameters, and EVERY CHUNK IS INDEPENDENT
// (fresh tree, fresh ring buffer, byte-aligned bit stream).  Re-using the tree
// across chunks decodes chunk 0 and then produces garbage, which is exactly
// how the parameters were pinned:
//
//   window N = 8192, F = 66, THRESHOLD = 2, ring cursor starts at N - F
//   N_CHAR   = 320 = 256 literals + 64 match-length symbols; T = 639, R = 638
//   symbol <  256          -> literal byte
//   symbol >= 256          -> match, length = symbol - 255 + THRESHOLD
//   symbol == 319 (max)    -> escape: 8 extra raw bits are ADDED to the length
//   match position         -> 7-bit high part through the prefix code below,
//                             then 6 raw low bits; distance = position + 1
//
// The position prefix code is canonical and complete over 512 (9-bit) units:
//   len 3 x 2, len 4 x 2, len 5 x 4, len 6 x 6, len 7 x 20, len 8 x 34,
//   len 9 x 60  ->  128 high values.  This is NOT the stock LZHUF p_len/p_code
//   table (that one is 1/3/8/12/24/16 over a 4K window), so the existing LZH
//   handlers cannot decode this family.
//
// Derived by reverse engineering the 98-file corpus against a reference
// extractor; the Python model this port mirrors reproduces all 98 files
// byte-exactly (794 chunks).
class XLzpis2Decoder : public QObject {
    Q_OBJECT

public:
    explicit XLzpis2Decoder(QObject *parent = nullptr);

    static const qint64 LZPIS2_MAGIC_SIZE = 6;
    static const qint64 LZPIS2_CHUNK_HEADER_SIZE = 4;
    static const qint64 LZPIS2_MAX_CHUNK_SIZE = 0x1000;
    static const qint64 LZPIS2_MAX_INPUT_SIZE = 0x10000000;   // 256 MiB
    static const qint64 LZPIS2_MAX_OUTPUT_SIZE = 0x40000000;  // 1 GiB

    // One chunk. nOutSize is the chunk's declared uncompressed size and must be
    // produced exactly; the stream may not consume more than nInSize bytes.
    static bool decodeChunk(const quint8 *pIn, qint64 nInSize, quint8 *pOut, qint64 nOutSize);

    // Walks the whole container from nInputOffset (the "LZPIS2" magic itself).
    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XLZPIS2DECODER_H
