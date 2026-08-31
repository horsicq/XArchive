/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XVISEDEFLATEDECODER_H
#define XVISEDEFLATEDECODER_H

#include "xbinary.h"

// Windows Installer VISE obfuscates each raw-Deflate stream by exchanging
// adjacent bytes.  This bounded adapter is shared by the structural scanner
// and normal XArchive extraction.
class XViseDeflateDecoder
{
public:
    static bool decode(const QByteArray &packed, qint64 expectedSize,
                       QByteArray *output, qint64 *rawSize = nullptr,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XVISEDEFLATEDECODER_H
