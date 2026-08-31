/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFPAKDECODER_H
#define XFPAKDECODER_H

#include "xbinary.h"

// FoxPro Distribution Kit FPPF payloads are raw PKZIP Implode streams with
// a 4 KiB dictionary and uncoded literals (the two-tree variant).  FPAK does
// not carry a ZIP general-purpose flag, so keep that fixed profile explicit
// at the format boundary instead of making callers guess it.
class XFpakDecoder
{
public:
    static bool decode(const QByteArray &packed, qint64 expectedSize,
                       QByteArray *output, qint64 *consumedSize = nullptr,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XFPAKDECODER_H
