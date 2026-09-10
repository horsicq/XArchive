/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRSVKDECODER_H
#define XRSVKDECODER_H

#include <QByteArray>

#include "xbinary.h"

// Member codec of the "RSVKDATA" / "DLIBDATA" container.  It is a complete
// block-sorting compressor, structurally a cousin of the pre-bzip2 "bzip 0.21"
// pipeline but with its own tuning and no bzip container fields at all:
//
//   Witten/Neal/Cleary (CACM 1987) 16-bit adaptive arithmetic decoder
//     -> Fenwick-style structured model over the MTF alphabet
//     -> RUNA/RUNB bijective zero-run code
//     -> inverse move-to-front
//     -> inverse Burrows-Wheeler with an explicit sentinel row
//
// A member is a chain of blocks, each
//
//     +0x00 char[4] "DATA"
//     +0x04 u32     CRC-32 (zlib polynomial) of the block's plaintext
//     +0x08 u32     size of the arithmetic-coded payload that follows
//     +0x0c u32     BWT primary index (the row the inverse walk starts on)
//     +0x10 u32     row index of the BWT sentinel ("hole")
//     +0x14 ...     payload
//
// The plaintext length of a block is NOT stored; it falls out of the decode
// (the coded symbol stream ends on the 0x101 end-of-block symbol).  Blocks are
// 200 KiB of plaintext except the last one of a member.
//
// Model parameters, all recovered from the reference implementation and then pinned against the
// per-block CRC-32s of the whole sample corpus:
//
//   selector model  symbols 0..8, increment 0x20, Max_frequency 0x1000
//   group 2         symbols 2..3      increment 1, Max_frequency 0x100
//   group 3         symbols 4..7      increment 1, Max_frequency 0x100
//   group 4         symbols 8..15     increment 1, Max_frequency 0x80
//   group 5         symbols 16..31    increment 1, Max_frequency 0x400
//   group 6         symbols 32..63    increment 1, Max_frequency 0x800
//   group 7         symbols 64..127   increment 1, Max_frequency 0x1000 (*)
//   group 8         symbols 128..257  increment 1, Max_frequency 0x2000 (*)
//
// Selector 0 and 1 ARE the symbols RUNA and RUNB; selectors 2..8 pick the
// sub-model whose decoded offset is added to the group's first symbol.  Symbol
// 0x101 ends the block, symbols 2..0x100 carry the MTF index (symbol - 1).
//
// (*) Groups 7 and 8 never reach their rescale threshold anywhere in the
// available corpus - the highest cumulative total observed for them is far
// below either candidate - so their table entries could only be bounded from
// below, not read off.  The values above are the smallest powers of two that
// are consistent with every block in the corpus and that continue the
// 0x400/0x800 progression of groups 5 and 6.  Groups 2..6 are exact: each was
// pinned by a block that rescales that model, and no other value decodes it.
class XRSVKDecoder {
public:
    // baPacked must hold the member's complete chain of "DATA" blocks starting
    // at offset 0.  The CRC-32 of every block is verified.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    XRSVKDecoder() = delete;
};

#endif  // XRSVKDECODER_H
