/*
 * RTPatch adaptive Huffman/LZSS decoder.
 *
 * Codec design and reference implementation:
 * Copyright (c) 2026 Sandy Carter
 * https://github.com/bwrsandman/rtptool (MIT License)
 *
 * C++ adaptation:
 * Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XRTPATCHDECODER_H
#define XRTPATCHDECODER_H

#include "xbinary.h"

class XRTPatchDecoder
{
public:
    static bool decode(const QByteArray &packed, qint64 expectedSize,
                       QByteArray *output,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XRTPATCHDECODER_H
