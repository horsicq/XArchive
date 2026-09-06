/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSQDECODER_H
#define XSQDECODER_H

#include "xbinary.h"

// Codec of the "SQ" squeezed single-file container (53 51 AC AE).
//
// It is NOT the classic CP/M-DOS Squeeze (0xFF76) codec: there is no stored
// Huffman node table at all.  The stream is an LZ77 over a 32 KiB circular
// window whose symbols come from an ADAPTIVE Huffman tree that both sides
// rebuild as they go, so nothing about the model is transmitted:
//
//   * 629 symbols: 0..255 literals, 256 end-of-stream, 257..628 matches.
//   * A match symbol s encodes length ((s-257) % 62) + 3, i.e. 3..64, and a
//     distance slot (s-257) / 62 in 0..5.
//   * Slot k is followed by 2k+4 raw LSB-first extra bits; the distance is
//     base[k] + extra + length with base = {0, 16, 80, 336, 1360, 5456}, i.e.
//     the slot ranges tile 0..21839 and what is coded is the gap in front of
//     the match, not the raw back-distance.
//   * Bits are MSB first inside each byte.
//
// The tree is a complete binary tree over nodes 1..1257: 1..628 internal,
// 629..1257 the leaves in symbol order.  Every node starts with weight 1 and
// decoding walks 0=left / 1=right from the root.  After each symbol the leaf's
// weight is incremented, the weights are recomputed up to the root, and at
// every level the node on the decode path is swapped with its lighter uncle -
// a self-adjusting variant of adaptive Huffman, not Vitter's FGK.  When the
// root weight reaches 2000 all weights are halved.
//
// The stream stores no output length, so the size is discovered by decoding;
// measure() runs the identical loop without materializing the output.
class XSQDecoder {
public:
    // nRawSize >= 0 requires the stream to produce exactly that many bytes.
    static bool decode(const QByteArray &baPacked, qint64 nRawSize, QByteArray *pbaUnpacked, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Decodes without keeping the output and reports the produced length.
    // Fails when the stream would exceed nMaxSize.
    static bool measure(const QByteArray &baPacked, qint64 nMaxSize, qint64 *pnRawSize, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSQDECODER_H
