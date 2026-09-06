/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XEALIBDECODER_H
#define XEALIBDECODER_H

#include "xbinary.h"

// EALIB method 1: Okumura LZSS, 4 KiB ring, F = 18, THRESHOLD = 3, one
// LSB-first flag byte per eight tokens.
//
// Deliberately NOT XLZSSDecoder (the SZDD path, ring cursor at N - 16) and
// deliberately NOT XAMPKDecoder::decodeLZSS: that one pre-sets the ring to
// 0x20 (spaces) outside the last F slots, while EALIB's producer clears the
// whole ring to 0x00.  Both variants emit exactly the right number of bytes
// from a stream that references the pre-history, so the difference is silent
// corruption rather than a failure.
//
// Method 0 and method 3 are stored and never reach this file; method 4 is a
// plain PKWARE DCL implode stream handled by HANDLE_METHOD_PKWARE_DCL_IMPLODE.
class XEALIBDecoder {
public:
    static bool decodeLZSS(const QByteArray &baPacked,
                           qint64 nUncompressedSize,
                           QByteArray *pbaUnpacked);
};

#endif  // XEALIBDECODER_H
