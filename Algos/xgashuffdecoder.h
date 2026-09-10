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
#ifndef XGASHUFFDECODER_H
#define XGASHUFFDECODER_H

#include "xbinary.h"

#include <vector>

// Headerless single-file Huffman container found on Atari ST / FidoNet media
// (samples keep their original extension: .HUF / .HUM / .TXT).
//
// Layout, entirely little-endian, no magic:
//
//   +0  u32  uncompressed size
//   +4  u16  node count N          (2 * leaves - 1 in every well-formed sample)
//   +6  u16  root node index       (index into the node table, often 0)
//   +8       bit stream, read MSB-first inside each byte
//
// The bit stream opens with a flat table of N + 1 node records:
//
//   bit 1 -> LEAF     : 8 further bits, read LSB-FIRST, are the byte value
//   bit 0 -> INTERNAL : two 10-bit fields, each read LSB-FIRST.  A field is
//                       always odd and encodes 2 * childIndex + 1.  The FIRST
//                       field is the child taken on a set bit, the SECOND the
//                       child taken on a clear bit.
//
// A leaf record is therefore 9 bits and an internal record 21 bits, so the
// table is exactly 15 * N + 3 bits wide whenever N == 2 * leaves - 1.  The last
// record is a spare that the tree never references; it must still be consumed
// because the payload starts immediately behind it.
//
// Decoding is plain static Huffman: start at node[root], follow child1 on a set
// bit and child0 on a clear bit until a leaf is reached, emit its byte, repeat
// until the declared uncompressed size is produced.  There is no LZ stage and
// no post-filter.
//
// Verified byte-exact against the reference implementation on all 82 corpus members.
class XGasHuffDecoder : public QObject {
    Q_OBJECT

public:
    explicit XGasHuffDecoder(QObject *parent = nullptr);

    static const qint64 GAS_HEADER_SIZE = 8;
    static const qint64 GAS_MAX_INPUT_SIZE = 0x08000000;   // 128 MiB
    static const qint64 GAS_MAX_OUTPUT_SIZE = 0x10000000;  // 256 MiB
    // A child field is 10 bits wide and encodes 2 * index + 1, so the largest
    // representable node index is 511 - which is also 2 * 256 - 1, the biggest
    // tree a byte alphabet can produce.
    static const qint32 GAS_MAX_NODES = 511;
    static const qint32 GAS_MAX_RECORDS = GAS_MAX_NODES + 1;
    // Bytes trial-decoded by checkStream(); enough to catch a wrong tree while
    // keeping detection cheap on unrelated files.
    static const qint32 GAS_PROBE_BYTES = 4096;
    // Symbols a truncated probe must still produce before it may stop early.
    static const qint32 GAS_MIN_PROBE_BYTES = 64;

    struct GAS_NODE {
        bool bLeaf;
        quint8 nSymbol;
        quint16 nChild1;
        quint16 nChild0;
    };

    // Strict structural check used by both XGasHuff::isValid() and the
    // decoder.  The format has no magic, so this must trial-decode.
    //
    // pData/nSize may be a PREFIX of the container: nFullSize is the real
    // container length and drives every size relation, while nSize bounds what
    // may be touched.  When the prefix runs out mid-probe the trial decode
    // stops instead of failing, provided it already produced GAS_MIN_PROBE_BYTES.
    static bool checkStream(const char *pData, qint64 nSize, qint64 nFullSize, quint32 *pnUncompressedSize, quint32 *pnNodeCount, quint32 *pnRootIndex,
                            std::vector<GAS_NODE> *pListNodes, qint64 *pnPayloadBitOffset);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XGASHUFFDECODER_H
