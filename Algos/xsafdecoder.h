/*
 * Stac Electronics SAF archive member codec.
 *
 * Clean-room implementation derived from observed SAF streams and reference
 * decoder behavior. Copyright (c) 2026 hors<horsicq@gmail.com>.
 *
 * MIT License
 */
#ifndef XSAFDECODER_H
#define XSAFDECODER_H

#include "xbinary.h"

// LZ77 over a 2 KiB ring buffer with a fixed, table-less token encoding, read
// MSB first:
//
//   9 bit token t
//     t <  0x100        literal byte t
//     t >= 0x100, c = t & 0xff
//       c == 0x81       run of the previous byte (distance 1)
//       c >  0x80       match at distance (c & 0x7f)
//       c == 0x80       end of stream
//       c <  0x80       match at distance c * 16 + (next 4 bits)
//
//   match length: 2 bits L; if L == 3 add 2 more bits e, and if e == 3 keep
//   adding 4 bit nibbles while each is 15.  The emitted length is L + 2.
//
// The window starts zeroed and the writer flushes it every 2048 bytes, so a
// match may legally reach back into the not-yet-written part of the ring.
class XSAFDecoder {
public:
    // nMethod is the member header's method byte: 3 means the packed extent is
    // one stream, anything else means it is a chain of [qint32 length][stream]
    // chunks, each with its own fresh window.
    static bool decode(const QByteArray &packed, qint64 nRawSize, qint32 nMethod,
                       QByteArray *pUnpacked, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSAFDECODER_H
