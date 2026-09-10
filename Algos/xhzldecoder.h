/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHZLDECODER_H
#define XHZLDECODER_H

#include <QByteArray>

#include "xbinary.h"

// Haruyasu Yoshizaki / Haruhiko Okumura LZHUF (LZSS driven by an adaptive
// Huffman coder) in the exact sub-variant used by the "!HZL" single-file
// compressor and by the headerless DOS "JBF" archiver.  It is NOT the same
// stream as HANDLE_METHOD_ARCV_LZHUF / HANDLE_METHOD_ARCV_LZHUF60: those two
// carry a 256 stop code (N_CHAR 287/315), start the ring cursor at N - F and
// use a 4 KiB window.  This one has
//
//     window N        = 8192  (the ring index is masked with 0x1fff)
//     max match F     = 60
//     THRESHOLD       = 2      -> match lengths 3..62, symbol = length + 253
//     N_CHAR          = 314    (256 literals + 58 length codes, NO stop code)
//     T = 627, R = 626
//     ring prefilled with 0x20, ring cursor starts at 0 (not N - F)
//     position code   = classic 6-low-bit table (d_code << 6 | i & 0x3f),
//                       i.e. distances stay below 4096 even though the ring
// is 8 KiB, which is what the reference implementation does
//
// Decoding therefore always runs to the caller-supplied plaintext length; the
// stream carries no terminator. Ported from the reference implementation /
// The reference implementation, and validated byte-exact against
// The reference implementation's own extraction over the whole HZL (5/5) and JBF (39/39) corpora.
class XHZLDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint32 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    XHZLDecoder() = delete;
};

#endif  // XHZLDECODER_H
