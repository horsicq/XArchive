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
#ifndef XZOOMDECODER_H
#define XZOOMDECODER_H

#include "../xbinary.h"

// Zoom - an Amiga floppy imager (magic "ZOM5").  A member is a whole floppy: the
// product is one .adf image of (lastCylinder - firstCylinder + 1) * 0x2C00
// bytes, where 0x2C00 is a cylinder of 2 tracks * 11 sectors * 512 bytes, and
// sectors the file system never used are not stored at all - they are emitted as
// zeros.  Everything multi-byte in the container is BIG-ENDIAN.
//
// File header, 0x4C bytes:
//   0x00   4  magic "ZOM5"
//   0x04   1  first cylinder
//   0x05   1  last cylinder
//   0x06   1  format version, must be 5
//   0x1C   4  length N of a trailing note block, 0 = none; when it is set the
//             header is followed by N + 4 bytes (note plus its checksum) that
//             are skipped
//   0x24   1  non zero = password protected, which the reference reader
//             refuses outright
//   0x48   4  checksum over bytes 0..0x47
//
// Then chunk records back to back.  A chunk record is a 42-byte header followed
// by its payload:
//
//    0    5  five cylinder numbers, one per slot; 0xFF = slot unused
//    5    1  padding
//    6   20  five u32 sector bitmasks; bit k (k = 0..21, LSB first) set means
//            "sector k of that cylinder is stored"
//   0x1A  2  u16 packed payload length
//   0x1C  2  u16 length after the LZHUF stage, 0 = no RLE stage
//   0x1E  2  u16 final decoded length
//   0x20  2  u16 flag, != 0 = the payload is LZHUF compressed
//   0x22  4  u32 checksum of the payload
//   0x26  4  u32 checksum of bytes 0 .. 0x25 of the record
//
// TRAP 1 - TWO STAGES, IN THE OPPOSITE ORDER FROM THE PACKER'S.  The packer
// applies RLE and THEN LZHUF, so the unpacker must run LZHUF first and RLE
// second: XLZHUFDecoder with XLZHUFDecoder::getZoomOptions(), driven by the
// "length after the LZHUF stage" field, and only then decode() below for the
// RLE.  Running them the other way round consumes the RLE header as Huffman
// bits and produces nothing recognisable.  The middle length being zero means
// there is no RLE stage at all and the LZHUF output is already the answer -
// seven of the reference corpus's 340 chunks are like that.
//
// TRAP 2 - ZOOM'S LZHUF IS NOT THE TEXTBOOK ONE.  Its match length is
// symbol - 255 with no THRESHOLD added, and its match source is (r - distance)
// with NO -1 adjustment; the ring is 4096 zero-filled bytes and the alphabet is
// 317 symbols ending at 0x13C.  getZoomOptions() carries exactly that; the
// defaults, or ZTC's options, decode the first few bytes and then diverge.
//
// TRAP 3 - THE HOLES ARE PART OF THE IMAGE.  Sectors the file system never used
// are not stored at all, and neither are whole cylinders.  A record's decoded
// bytes are the concatenation of only the STORED sectors, in slot order and
// ascending sector order within a slot; every unstored sector is 0x200 zero
// bytes in the product and every skipped cylinder is 0x2C00 of them, including
// the run from the last record's cylinder up to lastCylinder.  Simply
// concatenating the decoded chunks yields a shorter file whose contents are
// shifted from the first hole onward.
//
// decodeImage() is that whole path in one call - the chunk index is spread
// across the records themselves and no member extent exists, so a record
// publishes the WHOLE FILE as its stream (the xuleadarchive.cpp pattern).
//
// UNVERIFIED, because the reference corpus does not exercise it: a first
// cylinder other than 0 (the reference compares record cylinder numbers against
// a counter that starts at 0 whatever firstCylinder says, which is what is
// reproduced here), a non-empty note block, the password flag, and a chunk with
// the LZHUF flag clear.  The two checksums are parsed and not verified, exactly
// as in the reference.
//
// The RLE stream is self describing and its header is the trap: the first three
// bytes are a BIG-ENDIAN 24-bit copy of the expected output length, so the
// caller's length and the stream's own must agree before a single byte is
// emitted; a stream whose header disagrees is not "close enough to try", it is a
// desynchronised chunk.  Byte 3 is the escape value, chosen per chunk, and it is
// a perfectly ordinary data byte the rest of the time - which is why escape,0
// exists to spell a literal one.
//
//   escape, 0          -> emit one literal escape byte
//   escape, n (n != 0) -> emit n + 1 copies of the byte that follows
//   anything else      -> emit it literally
//
// Note the run length is n + 1, not n: a run byte of 1 means two bytes out.
class XZoomDecoder {
public:
    enum {
        HEADER_SIZE = 0x4c,
        CHUNK_HEADER_SIZE = 42,
        CYLINDER_SIZE = 0x2c00,
        SECTOR_SIZE = 0x200,
        SECTORS_PER_CYLINDER = 22,
        SLOTS = 5,
        SLOT_UNUSED = 0xff
    };

    // The RLE stage.  baPacked is the LZHUF output of one chunk INCLUDING its
    // four-byte header; nUncompressedSize is the record's final decoded length,
    // which must match the header's own BE24 length.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // The whole path for the image: baPacked is the ENTIRE container and the
    // .adf image comes back.  Returns false unless exactly nUncompressedSize
    // bytes - one cylinder per cylinder of the range the header declares - were
    // produced.
    static bool decodeImage(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XZOOMDECODER_H
