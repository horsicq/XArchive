/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XEALZWDECODER_H
#define XEALZWDECODER_H

#include "xbinary.h"

// LZW as used by the Electronic Arts DOS ".PEA" member codec (method 1).
//
// It is a 12-bit LZW dialect that no existing handler in this tree decodes:
//
//   bit order       LSB-first, codes packed low bits first
//   code width      starts at 9, widened by the READER before each fetch when
//                   maxCodeForWidth < nextFreeCode (i.e. NO early change)
//   max width       12; at width 12 the "max code" becomes 4096 exactly, so
//                   the table stops growing instead of widening again
//   0x100           CLEAR: resets width to 9 and nextFreeCode to 0x102, and
//                   the code that FOLLOWS must be a bare literal (< 0x100)
//                   which is emitted and becomes the previous code
//   0x101           END of stream
//   first free      0x102
//   no block align  the bit stream is NOT padded to a byte after a CLEAR
//
// Every EA stream in the corpus opens with a CLEAR, which is what supplies the
// initial previous-code; the reference implementation's own decoder relies on exactly that and would
// otherwise seed the dictionary from an uninitialised register.  A stream that
// does not open with CLEAR is therefore rejected here.
//
// On a malformed stream the decoder stops and keeps what it has already
// produced, which is what the reference implementation does (its worker reports the member as an error
// but the partially written file survives); the caller decides whether a short
// result is acceptable. Ported from the reference implementation /
// The reference implementation with the configuration EA's worker passes at the reference implementation
// (maxbits 12, EOF code on, CLEAR on, no block align, LSB-first, no early
// change), and validated byte-exact against the reference implementation over the whole EA corpus
// (212/212 members, 27 archives).
class XEALzwDecoder {
public:
    enum {
        EA_LZW_MAX_BITS = 12,
        EA_LZW_CLEAR = 0x100,
        EA_LZW_END = 0x101,
        EA_LZW_FIRST_CODE = 0x102,
        EA_LZW_MAX_CODES = 1 << EA_LZW_MAX_BITS
    };

    // Decodes the whole stream.  Succeeds only when exactly nUncompressedSize
    // bytes were produced; a short or over-long result is a failure, but
    // *pOutput still holds what was decoded so callers that want the reference implementation's
    // "partial file plus error" behaviour can use it.
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    XEALzwDecoder() = delete;
};

#endif  // XEALZWDECODER_H
