/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XCORELLTECDECODER_H
#define XCORELLTECDECODER_H

#include "xbinary.h"

// Codec of the Corel/LEAD Technologies "LTEC" installer archive (SETUP.LTA).
//
// The payload is an LHA-family static-Huffman LZ77 stream: NC=510/CBIT=9
// literal+length table, NT=19/TBIT=5 pre-table with the i_special=3 two-bit
// zero run, a position table read with PBIT=5, THRESHOLD=3 and MAXMATCH=256.
// Two things stop it from being any of the existing HANDLE_METHOD_LZH*:
//
//  1. Each block opens with SIXTEEN BITS that carry no information.  The
//     first block of every one of the 67 corpus archives has them as 0x0000;
//     later blocks have whatever the encoder's shift register held.  Proof
//     they are not data: two blocks that carry the same members at the same
//     sizes (99_blpjaqnwhkpplbbf / 99_dibesvgmysaruwrd, block 1, 0xdef0e raw
//     from 0x5f126 packed) are byte-identical from the third byte on and
//     differ only in those two.  The LHA block-symbol count follows them.
//
//  2. The position alphabet is not fixed at -lh5-'s 14 / -lh6-'s 16 /
//     -lh7-'s 17.  Its declared symbol count varies from block to block and
//     runs past 17, so a distance can need more than sixteen extra bits and
//     a 16-bit LHA bit window cannot express it.  LTEC_NP is the alphabet
//     ceiling; the reader is 32-bit-wide accordingly.
//
// The container is SOLID: one block holds several consecutive members and a
// member starts at an arbitrary byte offset inside the block's plaintext.
// decode() therefore takes that offset alongside the block size and returns
// only the member's slice, stopping the LZ loop as soon as the slice is
// complete.
class XCorelLtecDecoder {
public:
    // Position alphabet ceiling.  Measured maximum over the corpus is below
    // this; a block declaring more symbols is rejected rather than truncated.
    static qint32 positionAlphabetSize();

    // Builds the 8-byte FPART_PROP_COMPRESSPROPERTIES blob the dispatch hands
    // back to decode(): u32 block plaintext size, u32 member offset in it.
    static QByteArray packProperties(qint64 nBlockSize, qint64 nOffsetInBlock);

    // packed starts at the block's first byte (the two uninformative ones
    // included) and MUST run a few bytes past the block's directory extent:
    // consecutive blocks overlap, so the last symbols of a block live in the
    // first bytes of the next one (measured worst case +16 bits).  Handing
    // over exactly the directory extent decodes the tail of most blocks
    // wrongly while everything before it still looks correct - XCorelLtec
    // publishes BLOCK::nStreamSize for this reason.  Only the final block of
    // an archive may fall short, and there the reader serves zeroes inside a
    // 16-bit margin, which is what the reference decode does too.
    //
    // baProperties is the blob above; when it is empty the whole block is
    // decoded and nUncompressedSize must be the block size itself.
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize,
                       const QByteArray &baProperties, QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Bounded trial decode used by XCorelLtec::isValid().  Succeeds only if
    // the first nProbeSize bytes of the block come out of a well-formed
    // stream; nProbeSize is clamped to nBlockSize.
    static bool probe(const QByteArray &packed, qint64 nBlockSize,
                      qint64 nProbeSize,
                      XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XCORELLTECDECODER_H
