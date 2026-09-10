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
#ifndef XDISKIMAGEDECODER_H
#define XDISKIMAGEDECODER_H

#include "../xbinary.h"

// Three block-structured disk-image codecs.  All three are given the WHOLE
// container file, because in each case the block index sits in the header and
// the reader has no way to hand a decoder two disjoint ranges.
//
// APRICOT - "ACT Apricot disk image".  After a 128-byte preamble the file is a
// chain of 16-byte chunk headers: u16 kind, i16 tag (always 0xE31D), i16
// subtype, u16 header length (at least 16, any excess is padding to skip) and
// i32 data length.  Only kind 1 carries image data; subtype 0x9E90 means that
// many literal bytes follow and 0x3E5A means a u16 count plus a fill byte.
//
// CISO - PSP compressed ISO.  Header: magic, u32 header size, u64 total bytes,
// u32 block size, u8 version, u8 index shift.  Then one u32 per block plus a
// terminator; the top bit means the block is stored rather than deflated, and
// each index value is shifted left by the index-shift field.
//
// CLOOP - a Linux compressed loop image, which begins with a shell script so
// the file can mount itself.  The binary header starts at offset 0x80: u32
// block size and u32 block count, both big endian, then one big-endian u64
// offset per block plus a terminator.  Blocks are zlib streams.
class XDiskImageDecoder {
public:
    // Apricot does not store its decoded length, so measure it first.
    static bool measureApricot(const QByteArray &baPacked, qint64 *pnUncompressedSize);
    static bool decodeApricot(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    static bool decodeCiso(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decodeCloop(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XDISKIMAGEDECODER_H
