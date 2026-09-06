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
#ifndef XEAREFPACKDECODER_H
#define XEAREFPACKDECODER_H

#include "xbinary.h"

// Electronic Arts RefPack (a.k.a. QFS) whole-file LZ77 decoder.
//
// Container: a two-byte signature word whose SECOND byte is the constant 0xFB
// and whose FIRST byte carries the flags (0x10 base; 0x01 = size fields are
// four bytes wide instead of three, 0x80 = a compressed-size field precedes the
// uncompressed-size field).  All 62 corpus samples use the plain 0x10 form:
// 10 FB followed by a three-byte BIG-endian uncompressed size.  The command
// stream starts immediately after the header - there is no alignment padding
// and no separate table.
//
// Command grammar, keyed on the first byte of each command:
//   0x00-0x7F  two bytes:   literals = b0 & 3
//                           copy     = ((b0 & 0x1C) >> 2) + 3
//                           distance = ((b0 & 0x60) << 3) + b1 + 1
//   0x80-0xBF  three bytes: literals = (b1 >> 6) & 3
//                           copy     = (b0 & 0x3F) + 4
//                           distance = ((b1 & 0x3F) << 8) + b2 + 1
//   0xC0-0xDF  four bytes:  literals = b0 & 3
//                           copy     = ((b0 & 0x0C) << 6) + b3 + 5
//                           distance = ((b0 & 0x10) << 12) + (b1 << 8) + b2 + 1
//   0xE0-0xFB  one byte:    literals = ((b0 & 0x1F) << 2) + 4, no copy
//   0xFC-0xFF  one byte:    literals = b0 & 3, terminator
// Every copy is byte-by-byte so overlapping runs (distance smaller than the
// copy length) expand correctly; the distance may never exceed the number of
// bytes already produced - unlike some LZSS relatives there is no pre-filled
// window, which is what makes the grammar a usable validity probe.
//
// Byte-verified against the reference extractor on the whole 62-file corpus
// (62/62 identical, and in every case the command stream ends exactly on EOF
// with the produced length equal to the declared uncompressed size).
namespace XEARefPackDecoder
{
// Largest packed container / unpacked payload this decoder will buffer.
const qint64 REFPACK_MAX_INPUT_SIZE = 0x10000000;   // 256 MiB
const qint64 REFPACK_MAX_OUTPUT_SIZE = 0x20000000;  // 512 MiB

// Parsed header. nHeaderSize is the number of bytes before the first command.
struct HEADER {
    qint64 nHeaderSize;
    qint64 nUnpackedSize;
    qint64 nPackedSize;  // -1 when the container carries no packed-size field
    bool bLargeSizes;    // four-byte size fields
    bool bHasPackedSize;
};

// Reads and range-checks the signature word plus the size fields.  Returns
// false for anything that is not a RefPack header; nTotalSize is used only to
// bound the declared sizes and may be passed as -1 when it is unknown.
bool readHeader(const char *pData, qint64 nDataSize, qint64 nTotalSize, HEADER *pHeader);

// Grammar-only walk used by XEARefPack::isValid().  It counts output bytes
// instead of materialising them, so it is cheap enough to run on arbitrary
// files.  pnProduced/pnConsumed report how far it got.
//
// bComplete says baStream covers the container to its last byte; only then is
// the terminator command required.
bool probeStream(const QByteArray &baStream, const HEADER &header, bool bComplete, qint64 nProduceLimit, qint64 *pnProduced, qint64 *pnConsumed);

// Full decode.  packed must start at the signature word.  nExpectedSize is the
// caller's declared output size (pass -1 to trust the header).
bool decode(const QByteArray &packed, qint64 nExpectedSize, QByteArray *pOutput, qint64 *pConsumedSize = nullptr, XBinary::PDSTRUCT *pPdStruct = nullptr);
}  // namespace XEARefPackDecoder

#endif  // XEAREFPACKDECODER_H
