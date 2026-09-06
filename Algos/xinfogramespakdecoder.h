/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XINFOGRAMESPAKDECODER_H
#define XINFOGRAMESPAKDECODER_H

#include "xbinary.h"

namespace XInfogramesPakDecoder
{
// Infogrames/AITD .PAK member method 1.  The payload is an "imploded" stream
// in the PKZIP method-6 family: two Shannon-Fano trees of 64 symbols each
// (length tree first, then distance tree) written as run-length pairs, then a
// bit stream of 1-bit literal flags.  It is NOT interchangeable with the ZIP
// explode decoder: the container stores no general-purpose flag word, the
// dictionary is fixed at 4K (6 low distance bits), literals are always raw
// 8-bit (there is no literal tree) and yet the minimum match length is 2 -
// the combination standard PKZIP never emits.  The trailing bytes of a copy
// may reach in front of the member; the window is pre-filled with zeroes.
//
// Byte-verified against the archives' own PKZIP CRC-16 fragments: 5626 of the
// 5978 records that carry one, with every remaining mismatch confined to a
// single corpus file that stores byte-identical payloads under conflicting
// checksums.
bool decode(const QByteArray &packed, qint64 nExpectedSize, QByteArray *pOutput,
            qint64 *pConsumedSize = nullptr,
            XBinary::PDSTRUCT *pPdStruct = nullptr);
}  // namespace XInfogramesPakDecoder

#endif  // XINFOGRAMESPAKDECODER_H
