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
#ifndef XVMSSAVESETDECODER_H
#define XVMSSAVESETDECODER_H

#include "../xbinary.h"

#include <QList>

// OpenVMS BACKUP save set - the block/record chaining, the file-attribute
// parser and the member-assembly pipeline.  The container plumbing is
// XVMSSaveSetArchive's business; everything that knows the FORMAT is here.
//
// THERE IS NO CODEC AND NO KEY MATERIAL.  A save set is block and record
// chaining, nothing more; the "DCX compressed variant" this family is often
// assumed to need does not exist, and the one transform that does exist is the
// VMS variable-length-record to CRLF conversion, which is a text reformat, not
// a compressor.  This class lives in Algos anyway because the dispatch that
// calls decode() sits in the shared core, which must link without the archive
// classes.
//
// A save set is a flat sequence of fixed-size BLOCKS.  Every block opens with a
// 0x100-byte block header (BBH) and the rest of it holds a packed sequence of
// 0x10-byte record headers (BRH), each followed by its payload.  Records never
// span a block: the block header's blocksize bounds them.
//
// BBH (0x100 bytes, little-endian) - these are also the detection tests:
//   +0x00  u16  size       == 0x0100
//   +0x02  u16  opsys      in {0x400, 0x800, 0x1000}
//   +0x04  u16  subsys     == 1                (BACKUP)
//   +0x06  u16  applic     in {1, 2}           (2 = filler, block skipped)
//   +0x10  u64  == 0       +0x18  u64  == 0    (spares)
//   +0x20  u32  == 0x00010101                  (struclev 0x0101, volnum 1)
//   +0x28  i32  blocksize  > 0x100
//   +0xec  u64  == 0       +0xf4  u64  == 0    +0xfc  u16  == 0
// The first block's blocksize is remembered; an applic==2 block with blocksize
// 0 inherits it.
//
// BRH (0x10 bytes):
//   +0x00  u16 rsize   +0x02 u16 rtype   +0x04 u32 flags
//   +0x08  u32 address +0x0c u32 spare (validated as 0)
//
// rtype 3 starts a file; 0, 1, 4 and 0x0b are skipped, anything else ends the
// walk.  An rtype-3 payload is u16 0x0101 then a list of (u16 len, u16 tag)
// plus len bytes:
//   tag 0x2a  file name
//   tag 0x33  u32 flags; 0x2000 means DIRECTORY and the data is discarded
//   tag 0x34  0x20-byte FAT: +0x00/+0x01 rtype/rattrib (both 2 = variable
//             record file), +0x08 efblk high word, +0x0a efblk low word (a
//             VAX word-swapped longword), +0x0c ffbyte;
//             size = (efblk - 1) * 512 + ffbyte
//   tags 0x36 / 0x37  8-byte dates
// The reference bounds that loop with the FULL rsize - it does not subtract the
// two magic bytes - and then seeks to the record end, so the over-read is
// harmless; that is reproduced exactly rather than "fixed".
//
// THE BODY IS THE PAYLOADS OF THE FOLLOWING rtype-4 RECORDS CONCATENATED
// (interleaved rtype-0 records are skipped) until `size` bytes exist; the last
// record is truncated to fit.
//
// SPAN AND RESUME - why a member is addressable at all.  A member does NOT
// occupy one contiguous file range: block and record headers sit inside it.  A
// record therefore publishes the member's whole SPAN (first body record header
// to last body byte) and memberProperties() packs the three numbers that let
// the walk restart at the span start without re-reading the archive from byte
// zero.  The alternative - handing the whole file to every member, the
// xtivoliarchive.cpp pattern - costs O(archive) per member, which on the
// largest reference sample is 228 members times 30 MB; resuming makes it
// O(member).
//
// The property blob is 24 bytes, little-endian, and this file is its ONLY
// definition:
//   +0x00  i64  walker remaining-in-block count at the span start
//   +0x08  i64  the FAT-derived body size to collect
//   +0x10  i32  the inherited first-block blocksize
//   +0x14  u8   1 when the body is a variable-record stream
//   +0x15  3 bytes padding
//
// If the FAT says rtype == 2 and rattrib == 2 the body is a variable-length
// record stream and is converted in place: u16 len, len bytes, emit them plus
// CRLF, skip one pad byte when len is odd.  THE CONVERTED LENGTH IS THE
// REPORTED FILE SIZE, which is why walk() has to run that conversion for those
// members instead of trusting the FAT.
//
// DIRECTORY ENTRIES ARE NOT PUBLISHED.  Their body records still have to be
// consumed or the walk desynchronises, but the reference emits no member for
// them and neither does walk().
class XVMSSaveSetDecoder {
public:
    struct MEMBER {
        qint64 nHeaderOffset;   // the rtype-3 BRH
        qint64 nBodyOffset;     // first body record header
        qint64 nBodySpanSize;   // bytes of file the body records occupy
        qint64 nRawSize;        // the FAT-derived size
        qint64 nReportedSize;   // after the variable-record conversion
        qint64 nRemaining;      // walker remaining-in-block at nBodyOffset
        qint32 nBlockSize;      // the inherited first-block blocksize
        bool bVarRec;
        QString sFileName;
    };

    static const qint64 BLOCK_HEADER_SIZE = 0x100;
    static const qint64 RECORD_HEADER_SIZE = 0x10;
    static const qint32 PROPERTY_SIZE = 24;

    // The first block header, which is the whole detection test.
    static bool isBlockHeaderValid(const QByteArray &baHeader);

    // Enumerate every stored file.  Stops at the first malformed record and
    // keeps what it already has, which is what the reference does.
    static bool walk(const QByteArray &baData, QList<MEMBER> *pListMembers, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // The 24-byte resume blob described above.
    static QByteArray memberProperties(const MEMBER &member);

    // Dispatch entry point.  baPacked is the member's span, baProperty is what
    // memberProperties() built, and the result is the assembled (and, for a
    // variable-record file, converted) body.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, const QByteArray &baProperty, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XVMSSAVESETDECODER_H
