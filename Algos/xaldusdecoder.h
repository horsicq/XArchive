/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XALDUSDECODER_H
#define XALDUSDECODER_H

#include <QByteArray>
#include <QtGlobal>

// Aldus/Adobe Setup install-disk payload decoder.
//
// The three Aldus generations share one container: a 22-byte payload
// sub-header, a table of big-endian compressed block lengths, and the blocks
// laid end to end.  Only the per-block codec differs, and the generation is
// named by the 16-byte file magic, which is NOT part of the payload - hence
// three entry points rather than one auto-detecting call.
//
// Every block is decoded from a fresh dictionary/window: the payload is not a
// single stream, so no existing whole-stream handle method can be pointed at
// it.  Each block's compressed length is rounded up to an even value, which
// leaves up to one unread padding byte per block; the bit-level decoders here
// stop on the codec's own end condition and ignore that tail instead of
// demanding that all input be consumed.
//
// baPacked is the payload slice - the container bytes from the header size
// (the absolute payload offset) through the last byte of the last block.
class XAldusDecoder {
public:
    // "ALDUS LZW   1.00": TIFF/PDF-style LZW, MSB-first, 9..12 bits,
    // early change, Clear 256, EOD 257.
    static bool decodeLZW(const QByteArray &baPacked, qint64 nUncompressedSize,
                          QByteArray *pbaUnpacked);
    // "ALDUS PKZP  2.00": one complete PKWARE DCL implode stream per block.
    static bool decodePKZP(const QByteArray &baPacked, qint64 nUncompressedSize,
                           QByteArray *pbaUnpacked);
    // "ADOBE LZSH  3.00": [LE16 CRC-16/ARC of the plaintext][u8 method][data],
    // method 0 stored, method 1 an LHA -lh5- static-Huffman stream.
    static bool decodeLZSH(const QByteArray &baPacked, qint64 nUncompressedSize,
                           QByteArray *pbaUnpacked);
};

#endif  // XALDUSDECODER_H
