/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBORLANDPACKDECODER_H
#define XBORLANDPACKDECODER_H

#include "xbinary.h"

// Headerless Unix-compress (LZW) stream decoder.
//
// The stream is bit-for-bit the payload of a .Z file with the two-byte
// 1F 9D magic removed: byte 0 is the ordinary compress flags byte
// (bits 0..4 = maxbits, bit 7 = block-compress) and the LSB-first
// variable-width code stream starts at byte 1.  XCompressDecoder cannot be
// pointed at such a stream because its entry point hard-requires the magic
// and the two bytes simply do not exist in the container.
class XBorlandPackDecoder {
public:
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XBORLANDPACKDECODER_H
