/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNIDDECODER_H
#define XNIDDECODER_H

#include "xbinary.h"

// The whole-member codec of the "NI" (.NID / .DAT / .PAC) DOS install set.
//
// A member is a chain of blocks, each introduced by a five byte frame:
//   quint8  nUnknown          not read by the reference extractor
//   quint16 nFlags            0x0800 = block is compressed, 0x0100 = last block
//   quint16 nCompressedSize   bytes of block payload behind the frame
// An uncompressed block is nCompressedSize verbatim bytes.  A compressed block
// carries its own self-terminating stream, so its expanded size is not stored
// anywhere; the chain ends at the 0x0100 flag and the member is complete when
// the expanded bytes reach the size the directory entry declares.
//
// The compressed stream is an LZ77 over three semi-adaptive Huffman models:
//
//   header    3 bits, LSB first, value 6 = mode 1, value 7 = mode 2.  Any other
//             value is a hard error.
//   length    257 symbols.  L = symbol + 2.  L in [2, 0x100] is a match,
//             L == 0x101 introduces a literal, L == 0x102 ends the stream.
//   literal   mode 2 decodes the byte through a third model, mode 1 reads it as
//             eight raw LSB-first bits.
//   distance  the high byte comes from the second model (implicitly 0 when
//             L == 2), the low byte is eight raw bits; the match is copied out
//             of a 64 KiB ring that starts as zeros.
//
// Each model is "semi-adaptive": for its first N symbols (3000 for the length
// and literal models, 5000 for the distance model) decoding walks a weight
// ordered list, halving the remaining weight per bit and bumping the decoded
// symbol by 32; on symbol N+1 a static Huffman tree is built from the weights
// and used, frozen, for the rest of the stream.  Every block resets all models
// and the ring, so blocks are independent.
class XNIDDecoder {
public:
    // baPacked is the member extent starting at its first block frame.
    static bool decode(const QByteArray &baPacked,
                       qint64 nUncompressedSize,
                       QByteArray *pbaUnpacked);
};

#endif  // XNIDDECODER_H
