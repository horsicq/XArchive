/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSLSDECODER_H
#define XSLSDECODER_H

#include "xbinary.h"

// Codec of the "\x1fS/L?SOA_" single-file container shipped with the WinSense
// DOS/Windows helpdesk product (.SL$, and the .DAT/.HLP/.GRP data files that
// carry the same wrapper).
//
// It is Okumura/Yoshizaki LZHUF - LZSS driven by an adaptive Huffman coder -
// in a parameter set that no existing handler in this tree decodes:
//
//     window N       = 8192, ring index masked with 0x1fff
//     max match F    = 90, THRESHOLD = 2  -> match lengths 3..92
//     N_CHAR         = 256 - THRESHOLD + F = 344 (NO stop code)
//     T = 687, R = 686, MAX_FREQ = 0x8000
//     ring prefilled with 0x20, ring cursor starts at 0 (not N - F)
//     position code  = 13 BIT: (d_code[b] << 7) | (b & 0x7f), with
//                      d_len[b >> 4] - 1 extra bits shifted into b
//
// That 13-bit position field is the part that separates it from every other
// LZHUF dialect here: XHZLDecoder (the same the reference implementation codec function) uses F = 60 and
// the classic 6-low-bit position code, so it decodes this family into garbage.
// d_code/d_len are the stock Okumura tables, verified byte-for-byte against
// The reference implementation's own copies at 0x007d14c0 / 0x007d15c0.
//
// The stream carries no terminator: decoding runs until the caller-supplied
// plaintext length has been produced, and a match that would overrun it is
// clipped, exactly as the reference implementation does when its "has stop code" flag is
// clear.
//
// Ported from the reference implementation /
// The reference implementation with the
// configuration SLS's worker passes at the reference implementation, and validated byte-exact
// against the reference implementation's own extraction over the whole SLS corpus (16/16 files).
class XSLSDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    XSLSDecoder() = delete;
};

#endif  // XSLSDECODER_H
