/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XNETWAREPACKDECODER_H
#define XNETWAREPACKDECODER_H

#include <QByteArray>
#include <QtGlobal>

// Novell "Packed File" codec, format version 0x01 / method 0x0A.
//
// The same stream shape carries both ARC4 NetWare corpora: the Personal
// NetWare / Novell DOS "Packed File " single-member container (31-byte header,
// stream at 0x1F) and the NetWare installation-disk container's "PackedData"
// chunk (u8 version, u8 method, u32 uncompressed size, then the stream).  Both
// declare 01 0A and both feed the bytes that follow straight into this decoder.
//
// BIT ORDER IS LSB-FIRST.  Bytes enter an accumulator at the top and codes are
// taken from the bottom, so a multi-bit field's FIRST-read bit is its LEAST
// significant one.  Reading the stream MSB-first produces a plausible-looking
// first Huffman table and then desynchronises - that is the trap in this
// format.
//
// Layout of the stream:
//
//   [tree 0]  literal alphabet          symbols are plaintext bytes
//   [tree 1]  match-length alphabet     symbol IS the length; 0xFE escapes
//   [tree 2]  match-distance high part  distance = low5 + symbol * 32
//   [token stream]
//
// Each tree is a self-describing pre-order walk of a general (n-ary) tree in
// which EVERY node - internal nodes included - carries an 8-bit symbol:
//
//   node := <children count, unary: k zero bits then a 1> <symbol: 8 bits>
//           <child node> x k
//
// The walk is entered once at depth 0, so the whole description is one root
// node and its descendants, capped at 256 nodes.  Decoding maps that n-ary
// shape onto a binary code: at a node with k children, a 1 bit descends into
// the LAST remaining child and a 0 bit moves to the previous sibling, so the
// node's own symbol is the all-zeros continuation after its k children have
// been passed.  Equivalently: a code is the path taken by binary-splitting the
// pre-order range at the LAST entry whose stored depth equals the current one.
//
// Tokens, after the three trees:
//
//   bit 1 -> literal: one symbol from tree 0, appended and pushed to the window
//   bit 0 -> match:   length  = symbol from tree 1,
//                               and if that symbol is 0xFE, length = next 13 bits
//                     low     = next 5 bits
//                     high    = symbol from tree 2
//                     distance = low + high * 32
//                     copy `length` bytes from window[(pos - distance)]
//
// The window is 16384 bytes, ZERO-INITIALISED, and both the write pointer and
// the copy source wrap on 0x3FFF.  Zero initialisation is observable: a match
// may legally reach behind the start of the output and must then produce NUL
// bytes rather than fail.  Decoding stops on the declared uncompressed size,
// which is the only end-of-stream signal the format has.
//
// Observed over the whole 1713-file NetWare1 + 2537-file NetWare2 corpus:
// tables reach the full 256 entries and depth 18, match lengths reach 4095 and
// distances reach 4095, and no stream ever emits a zero length, a zero
// distance, or a match that would overrun the declared size.
class XNetWarePackDecoder {
public:
    enum {
        WINDOW_SIZE = 0x4000,
        MAX_TABLE_ENTRIES = 256,
        LENGTH_ESCAPE = 0xFE,
        ESCAPE_LENGTH_BITS = 13,
        DISTANCE_LOW_BITS = 5
    };

    // Decodes a complete stream.  Succeeds only when exactly nUncompressedSize
    // bytes come out; a short or over-long decode is a failure, never a
    // partially published result.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked);

    // Bounded structural probe for detection.  Parses the three trees and walks
    // at most nMaxOutput plaintext bytes.  bComplete says baSample is the whole
    // stream; when it is only a prefix, running out of input after real
    // progress is accepted, because that is truncation of the sample and not a
    // grammar violation.
    static bool probe(const QByteArray &baSample, qint64 nUncompressedSize, bool bComplete, qint64 nMaxOutput);
};

#endif  // XNETWAREPACKDECODER_H
