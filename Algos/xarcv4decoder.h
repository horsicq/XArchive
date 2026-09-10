/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XARCV4DECODER_H
#define XARCV4DECODER_H

#include "xbinary.h"

// Eschalon Setup ARCV 4.00, member method 2.
//
// Despite the shared "ARCV" tag this is NOT the Yoshizaki LZHUF that ARCV 1.10
// and ARCV 2.00 use (XDecompress::decompressArcvLzhuf): every parameter is
// different -- window, symbol count, bit order and the tree update rule -- so
// none of the HANDLE_METHOD_ARCV_* codecs can stand in for it.  The scheme is
// an order-0 adaptive Huffman coder over a single 3245-symbol alphabet that
// carries literals, the end marker and the LZ77 lengths together:
//
//   * symbols 0..255      one literal byte
//   * symbol  256         end of stream
//   * symbols 257..3244   a match; with k = symbol - 257,
//                           length = k % 498 + 3          (3..500)
//                           bucket = k / 498              (0..5)
//                         after which `bucket`-many extra bits are read and
//                           distance = base[bucket] + extra + length
//                         with 4/6/8/10/12/14 extra bits per bucket and
//                         base = 0, 16, 80, 336, 1360, 5456.  Because the
//                         length is folded into the distance the source run
//                         never overlaps the destination.
//
// Bits are consumed LSB-first inside each byte (the opposite of LZHUF), the
// history window is 32 KiB, and the maximum reachable distance is 22339.
//
// The Huffman tree is unusual and has to be reproduced exactly or the very
// first restructuring desynchronises the stream:
//
//   * Nodes are 1-based in a heap layout: node i starts with children 2i and
//     2i+1, so 1..3244 are internal and 3245..6489 are the leaves, leaf
//     symbol s living at node s + 3245.
//   * Every node from 2 upwards starts with weight 1 -- including the internal
//     ones, which therefore do NOT start out as the sum of their subtree.  The
//     sums are established lazily, only along the paths the updates walk.
//   * An update bumps the leaf, recomputes the weights along its path to the
//     root, then walks back up swapping the current node with its uncle
//     whenever the uncle is lighter.
//   * When the root weight reaches exactly 2000 every weight is halved.
//
// Reference: handler "ARCV4" (class dta), decompressor at VA 0x00577010
// with the model at 0x005769b0 / 0x00576c20 / 0x00576ae0 / 0x00576e70 /
// 0x00576f40 / 0x00576fc0.
class XARCV4Decoder {
public:
    // Decodes one method-2 member.  nUncompressedSize is the size the FILE
    // chunk declares and is enforced exactly: the stream must reach its end
    // marker having produced precisely that many bytes.  A member whose DATA
    // record is only the leading fragment of a split volume can never satisfy
    // that and must not be routed here.
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XARCV4DECODER_H
