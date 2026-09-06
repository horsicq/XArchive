/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XAMPKDECODER_H
#define XAMPKDECODER_H

#include "xbinary.h"

// The two Okumura codecs an AMPK container uses that no other XFileUnpacker
// decoder already covers.  Methods 0 and 3 of the same container are handled by
// HANDLE_METHOD_STORE and HANDLE_METHOD_LZH1 and never reach this file.
class XAMPKDecoder {
public:
    // Method 2: Okumura LZSS, 4 KiB window, F = 18, one LSB-first flag byte per
    // eight tokens.  Deliberately NOT XLZSSDecoder (the SZDD path): that one
    // starts the ring cursor at N - 16 instead of N - 18, which yields exactly
    // the right number of output bytes with the wrong content.
    static bool decodeLZSS(const QByteArray &baPacked,
                           qint64 nUncompressedSize,
                           QByteArray *pbaUnpacked);

    // Method 1: Okumura LZARI, an LZSS layer over an adaptive binary arithmetic
    // coder.  The coder primes 17 bits before the first symbol, so it reads one
    // or two bytes past the member's declared compressed size; those reads must
    // return zero bits rather than fail.
    static bool decodeLZARI(const QByteArray &baPacked,
                            qint64 nUncompressedSize,
                            QByteArray *pbaUnpacked);
};

#endif  // XAMPKDECODER_H
