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
#ifndef XIMPDECODER_H
#define XIMPDECODER_H

#include "../xbinary.h"

// IMP (Technelysium "IMP\n") member codecs.
//
// IMP IS NOT AN ARITHMETIC CODER, whatever its reputation says.  The routine at
// the heart of it is a BLOCK DRIVER over one SOLID stream: each
// block opens with a 4-bit method, a 1-bit last flag, a 20-bit unpacked size
// and a 20-bit packed size, all LSB-first, and the next block starts `packed`
// bytes further on.  The methods are
//
//   0  stored (the bytes start at the reader's next unconsumed byte)
//   1  a Deflate relative: LZ77 with LZX-style REPEATED OFFSETS (distance
//      symbol 0 reuses r0, symbol 1 swaps in r1) and canonical Huffman tables
//      whose code lengths are DELTA CODED AGAINST THE PREVIOUS TABLE ROW, plus
//      a run/zero encoding with doubling multipliers.  Symbol 0x11F is not a
//      match at all: it records a DELTA POST-FILTER span, applied after the
//      block and un-applied over whatever window the next block keeps.
//   2  bzip2-shaped: BWT + MTF + RLE with per-group Huffman tables and a
//      unary-coded, move-to-front selector.
//   3  the same coder as 1
//
// THE BWT DECODER WRITES unpacked + 1 BYTES, one past the block, which is why
// every dictionary allocation here carries the original's +0x200 of slack.
// Dropping that slack corrupts the byte after the block - or crashes.
//
// THE DIRECTORY IS ITSELF COMPRESSED: it is a chain of "IMPDE\0" chunks, each
// with a 5-bit method (only 0 stored and 1 coded are legal), a 20-bit unpacked
// size (at most 0x2000) and a 20-bit packed size, decoded with the same LZ77
// coder.  A member record never straddles a chunk: if the tail of a chunk is
// too short for one, the walk moves to the next chunk.
//
// Every checksum in the container is a CRC-32 TRUNCATED TO 16 BITS, computed
// over the record with the checksum field itself zeroed.
//
// The per-member stream carries an 11-byte in-stream header (u32 size, u16 name
// length, u16 version, u16 crc16, u8 attributes) followed by that many name
// bytes, and only then the payload.
class XIMPDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x40000000;
    static const qint32 DIRECTORY_RECORD_SIZE = 0x26;

    static quint32 crc32(const QByteArray &baData);

    // The chained IMPDE directory chunks.  baDirectory must start at the first
    // "IMPDE\0"; chunk boundaries are preserved because a member record never
    // straddles one.
    static bool decodeDirectory(const QByteArray &baDirectory, qint32 nRecords, QList<QByteArray> *plistChunks, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Properties as published in FPART_PROP_COMPRESSPROPERTIES:
    //   +0  u32 magic 'IMPS'
    //   +4  u64 the member's offset inside the DECODED solid stream
    //   +12 u64 the member's decoded size
    //   +20 u8  the member attribute byte (bits 1..2 select the x86 branch
    //           converter: 2 -> 16 bit, 4 -> 32 bit)
    // baPacked starts at the stream's own "IMPLH\0" signature, so the first
    // block header sits at +6.
    static QByteArray packProperties(qint64 nStreamOffset, qint64 nSize, quint8 nAttributes);
    static bool unpackProperties(const QByteArray &baProperties, qint64 *pnStreamOffset, qint64 *pnSize, quint8 *pnAttributes);

    static bool decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XIMPDECODER_H
