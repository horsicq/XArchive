/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XIS11DECODER_H
#define XIS11DECODER_H

#include "xbinary.h"

// InstallShield-family *.??$ single-stream compressor (XIS11 container).
//
// The payload is a Unix-compress (LZW) code stream with NO header at all: the
// producer hard-wires maxbits = 12 and block-compress mode, so neither the
// 1F 9D magic nor the flags byte that XCompressDecoder / XBorlandPackDecoder
// require is present in the container.  Bit packing is LSB first, codes start
// at 9 bits, code 256 is CLEAR, the first assignable code is 257 and the code
// width steps up whenever the next free code passes the current all-ones
// value.  As in compress, both a width change and a CLEAR discard the tail of
// the current eight-code group.
//
// The uncompressed size is not stored anywhere in the container, so
// decodeStream() is the primitive: it decodes until the bounded input runs
// out.  XIS11 calls it once while listing to learn each member's size, and
// decode() is the fixed-size entry point used by the XDecompress dispatch.
class XIS11Decoder {
public:
    // Decodes the whole packed member.  nMaxOutput is a hard ceiling that
    // guards against a corrupt stream expanding without bound; pass -1 for the
    // built-in limit.
    static bool decodeStream(const QByteArray &packed, QByteArray *pOutput, qint64 nMaxOutput = -1, XBinary::PDSTRUCT *pPdStruct = nullptr);

    static bool decode(const QByteArray &packed, qint64 nUncompressedSize, QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XIS11DECODER_H
