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
#ifndef XTERSEDECODER_H
#define XTERSEDECODER_H

#include "../xbinary.h"

// IBM TERSE (TRSMAIN / AMATERSE) - the codec behind a "tersed" MVS data set.
//
// THIS IS NOT the public "SPACK/PACK static Huffman" TERSE description.  There
// is no Huffman table anywhere in the stream.  What the container holds is a
// 12-bit MSB-first code stream driving a BINARY PAIR TREE with LRU node
// recycling:
//
//   * code 0 ends the stream, otherwise the symbol is code - 1;
//   * symbol < 0x100 is a literal byte, symbol == 0x100 is a no-op, and
//     symbol > 0x100 is an internal node expanded PRE-ORDER through LT then RT;
//   * after every code the ring of nodes 0x101..0xFFE is advanced to the first
//     zero-weight entry, that entry's old children have their weights
//     DECREMENTED, LT is set to the previous code and RT to the current one,
//     both of those have their weights INCREMENTED, and the node is moved to
//     the MRU tail of the ring.
//
// Weights are 16-bit and WRAP: decrementing a zero weight yields 0xFFFF, which
// is what keeps a freshly recycled node out of the free list until it is
// released again.  That wrap is load bearing, not an overflow bug.
//
// There is NO character-set translation.  Two of the three reference samples
// are EBCDIC and one is ASCII; translating either way would corrupt the output,
// so the decoded bytes are emitted exactly as produced.
//
// The container is a 12-byte header (only 4 bytes for the 0xA5698901 magic
// variant) followed by the raw code stream.  There is no member name, no record
// framing and no stored output length, so the length has to be measured by
// decoding - see measure().  detect() reproduces the reference detector: one of
// three header shapes plus a sanity probe over the first five 12-bit codes.
class XTERSEDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x20000000;

    // Returns the header size (4 or 12), or -1 when this is not TERSE.
    static qint32 detect(const QByteArray &baFile);

    // Decodes into a counter to learn the length the container never stores.
    static bool measure(const QByteArray &baFile, qint64 *pnUncompressedSize, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // baPacked is the WHOLE file, header included: the header size is part of
    // the format detection and a record cannot describe "everything but the
    // first twelve bytes" without losing that.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTERSEDECODER_H
