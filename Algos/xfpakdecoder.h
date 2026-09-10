/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFPAKDECODER_H
#define XFPAKDECODER_H

#include "xbinary.h"

// FoxPro Distribution Kit FPPF payloads are PKZIP Implode streams, and the
// FPPF header carries the two fields that pick the profile: the compression
// method (0 = stored, 6 = imploded) and the PKZIP general-purpose flag word.
// Only two of the flag bits reach the codec:
//
//   0x02  8 KiB dictionary - seven low distance bits instead of six
//   0x04  a literal Shannon-Fano tree is present (256 symbols, read BEFORE
//         the length and distance trees) and the minimum match length
//         becomes 3 instead of 2
//
// 0x08 (data descriptor) says nothing about the bit stream and is ignored.
//
// PASS THE HEADER FIELDS IN, DO NOT GUESS THEM.  A stream decoded with the
// wrong profile does not fail - it desyncs and produces plausible garbage,
// which is why every caller must forward the FPPF method and flags verbatim
// and check the member CRC-32 afterwards.
class XFpakDecoder
{
public:
    enum {
        METHOD_STORED = 0,
        METHOD_IMPLODED = 6,
        // PKZIP general-purpose bit flags that select the Implode profile.
        FLAG_DICTIONARY_8K = 0x0002,
        FLAG_LITERAL_TREE = 0x0004
    };

    static bool decode(const QByteArray &packed, quint16 method, quint16 flags,
                       qint64 expectedSize, QByteArray *output,
                       qint64 *consumedSize = nullptr,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XFPAKDECODER_H
