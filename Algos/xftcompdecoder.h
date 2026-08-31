/*
 * IBM OS/2 PACK2 fT19 decoder.
 *
 * Clean-room implementation derived from observed PACK2 streams and decoder
 * behavior. Copyright (c) 2026 hors<horsicq@gmail.com>.
 *
 * MIT License
 */
#ifndef XFTCOMPDECODER_H
#define XFTCOMPDECODER_H

#include "xbinary.h"

class XFtcompDecoder
{
public:
    // Production DATAPROCESS_STATE adapter. The packed extent and declared
    // uncompressed size must both be finite and are consumed exactly.
    static bool decompress(XBinary::DATAPROCESS_STATE *pState,
                           XBinary::PDSTRUCT *pPdStruct = nullptr);

    // packed starts with 80 60 00 00 "fT19" and is verified here.
    static bool decode(const QByteArray &packed, qint64 nRawSize,
                       QByteArray *pUnpacked,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XFTCOMPDECODER_H
