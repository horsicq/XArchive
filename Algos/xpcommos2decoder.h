/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPCOMMOS2DECODER_H
#define XPCOMMOS2DECODER_H

#include <QByteArray>
#include <QtGlobal>

// IBM Personal Communications for OS/2 (PCOMM 4.x) install-diskette payload.
//
// The packed file carries NO header of any kind: the last character of the
// 8.3 name is replaced by '_' and the file body is the raw token stream, so
// recognition is purely structural and the plaintext size only exists once
// the stream has been walked.
//
// The stream is a sequence of independent blocks, each producing at most
// 16384 plaintext bytes.  Match sources never cross a block boundary, which
// is what lets the encoder spend an absolute 16-bit offset on a far match.
// Reading a control byte C:
//   C >= 0xE1  literal run of (C - 0xE0) bytes, 1..31
//   C == 0xE0  end of the current block; the next block starts empty
//   C >= 0x20  two-byte token: length (C >> 5) + 2, i.e. 3..8,
//              distance back = ((C & 0x1F) << 8 | B1) + 1, i.e. 1..8192
//   C <  0x20  three-byte token: length C + 4, i.e. 4..35, and the source is
//              the ABSOLUTE offset B1 | (B2 << 8) counted from the start of
//              the current block - not a distance back.  Getting this wrong
//              is the whole difficulty of the format: reading it as a
//              two-byte token yields distances far past the output produced
//              so far and the decode dies within a few dozen bytes.
// A complete file ends with two 0xE0 bytes: one closing the last data block
// and one closing an empty terminator block, and every block before the last
// data block is exactly 16384 bytes.  Verified over the whole 582-file
// corpus.
class XPCommOS2Decoder {
public:
    enum {
        BLOCK_SIZE = 16384
    };

    // Walks the token stream without materializing any output and reports the
    // exact plaintext size.  This is both the size oracle for the archive
    // record and the detection gate: the block-size and full-consumption
    // invariants it enforces are what keeps a headerless format from matching
    // arbitrary binaries.
    static bool scan(const char *pData, qint64 nSize, qint64 *pnUncompressedSize);
    static bool scan(const QByteArray &baPacked, qint64 *pnUncompressedSize);

    // nUncompressedSize must be the value scan() reported; the decode refuses
    // to publish anything that does not reproduce it exactly.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked);
};

#endif  // XPCOMMOS2DECODER_H
