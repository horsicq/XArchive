/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XFLSDECODER_H
#define XFLSDECODER_H

#include "xbinary.h"

// Decoder for the adaptive phrase codec used by IBM SaveRam/SaveRam2 FLS
// archives. Compressed member extents include the leading 'S' stream tag.
class XFLSDecoder {
public:
    static bool decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XFLSDECODER_H
