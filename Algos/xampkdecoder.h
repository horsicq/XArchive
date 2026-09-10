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
    //
    // bAllowTruncated: treat running out of input as NORMAL TERMINATION and
    // return the bytes decoded so far, instead of failing.  This is only
    // correct for containers where a short member is an expected, recoverable
    // state - MS COMPRESS.EXE "SZ " files, two of which are physically
    // truncated and whose reference extractor emits their partial output.  An
    // AMPK member that runs short is genuinely corrupt, so AMPK keeps the
    // default and still fails.
    //
    // REGISTRATION NOTE.  XMSCompressSZ publishes HANDLE_METHOD_SZ_LZSS (not
    // HANDLE_METHOD_AMPK_LZSS) so the flag can be threaded from the SZ reader
    // alone.  That method must NOT be added to the whole-buffer method list of
    // XDecompress::decompress: the common tail of that block writes the output
    // only when `unpacked.size() == nUncompressedSize`, which is exactly the
    // condition a truncated member cannot meet, so a partial result would be
    // discarded there.  Give it its own arm that calls
    //     XAMPKDecoder::decodeLZSS(packed, nUncompressedSize, &unpacked, true)
    // and writes `unpacked` whenever the call succeeds, however short it is.
    static bool decodeLZSS(const QByteArray &baPacked,
                           qint64 nUncompressedSize,
                           QByteArray *pbaUnpacked,
                           bool bAllowTruncated = false);

    // Method 1: Okumura LZARI, an LZSS layer over an adaptive binary arithmetic
    // coder.  The coder primes 17 bits before the first symbol, so it reads one
    // or two bytes past the member's declared compressed size; those reads must
    // return zero bits rather than fail.
    static bool decodeLZARI(const QByteArray &baPacked,
                            qint64 nUncompressedSize,
                            QByteArray *pbaUnpacked);
};

#endif  // XAMPKDECODER_H
