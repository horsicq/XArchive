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
#ifndef XSQUASHFSDECODER_H
#define XSQUASHFSDECODER_H

#include "../xbinary.h"

#include <QList>

// SquashFS - versions 1, 2, 3 and 4.
//
// SQUASHFS IS NOT ONE LAYOUT.  A v4-only reader covers four of the seven
// reference images; the rest are v3.0 and v2.1.  Three superblock shapes, all
// packed:
//
//   v4      96 bytes, 64-bit tables from +0x20.
//   v3     119 bytes (0x77) - 32-bit legacy pointers live at 0x08..0x1B,
//          mkfs_time sits UNALIGNED at 0x27, and the 64-bit tables only start
//          at 0x3F.
//   v1/v2   63 bytes (0x3F), same prefix but 32-bit table pointers.
//
// The inode common prefix is 3 bytes on v1, 4 on v2/v3 and 8 on v4.  Directory
// headers are 4, 9 and 12 bytes and directory entries 3, 5 and 8 - and BOTH
// the header's entry count AND the entry's name length are stored BIASED BY
// ONE.
//
// Only inode types 1 DIR, 2 FILE, 8 LDIR and 9 LREG are walked.  Symlinks,
// devices, fifos and sockets are skipped entirely, which is why the emitted
// file count is far below the superblock's inode count.
//
// In a block list, bit 0x1000000 means the block is STORED raw and the low 24
// bits are its on-disk length (v1 packs the same flag as 0x8000 in a u16 and
// it is rescaled).  A ZERO entry is a sparse block: it occupies nothing on
// disk and expands to a block of zeros.
//
// COMPRESSION.  v1..v3 have no compressor field at all, and one reference
// image ("99_fbcpxequjzpilioj_romdisk") is a v3 whose data blocks are LZMA -
// the OpenWrt squashfs-lzma fork keeps the 'hsqs' magic.  Its blocks start
// 5D 00 00 08 00: five props bytes and NO 8-byte size field.  Assuming zlib
// there yields 1,334 bytes out of 13.5 MB, so the first byte of every block is
// sniffed instead (0x78 zlib, 0x5D LZMA, FD 37 7A 58 5A 00 xz).  XZ, LZO, LZ4
// and ZSTD are not decoded here; those blocks fail cleanly.
class XSquashFSDecoder {
public:
    static const qint64 MAX_UNCOMPRESSED_SIZE = 0x40000000;
    static const qint64 MAX_INPUT_SIZE = 0x20000000;
    static const qint32 CHUNK_SIZE = 32;
    static const qint32 DESCRIPTOR_HEADER_SIZE = 24;

    struct SUPERBLOCK {
        qint32 nMajor;
        qint32 nMinor;
        qint32 nCompressor;
        bool bBigEndian;
        qint64 nInodes;
        qint64 nBlockSize;
        qint64 nBytesUsed;
        qint64 nRootInode;
        qint64 nInodeTable;
        qint64 nDirectoryTable;
        qint64 nFragmentTable;
        qint64 nFragments;
    };

    struct MEMBER {
        QString sFileName;
        qint64 nUncompressedSize;
        qint64 nSpanOffset;
        qint64 nSpanSize;
        QByteArray baDescriptor;
    };

    // Magic plus a major version of 1..4 is the whole of the reference
    // detector; nothing else is checked, there is no checksum anywhere.
    static bool parseSuperBlock(const QByteArray &baHeader, qint64 nFileSize, SUPERBLOCK *pSuperBlock);

    // Walks the inode tree.  Returns true when at least one regular file was
    // found; a walk that dies half way keeps everything collected so far,
    // which is what the reference does.
    static bool listMembers(const QByteArray &baFile, QList<MEMBER> *pListMembers, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // The chunk list published as FPART_PROP_COMPRESSPROPERTIES.  Offsets are
    // RELATIVE to the byte range the record publishes.
    //
    //   +0  u32 magic 'SQFD'
    //   +4  u32 chunk count
    //   +8  u64 member size
    //   +16 u32 compressor hint
    //   +20 u32 reserved
    //   then one 32-byte chunk per entry:
    //     +0  u64 relative offset
    //     +8  u64 on-disk length
    //     +16 u64 output length
    //     +24 u32 bytes to drop from the front of the produced block
    //     +28 u32 kind: 0 stored, 1 compressed, 2 sparse
    static bool decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSQUASHFSDECODER_H
