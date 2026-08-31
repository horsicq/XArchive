/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDNDECODER_H
#define XDNDECODER_H

#include "xbinary.h"

// Decoder for the raw-DEFLATE variant used by classic Dos Navigator
// distribution archives. Its dynamic-block count fields have a nonstandard
// order; stored and fixed-Huffman blocks use the regular DEFLATE layout.
class XDNDecoder {
public:
    static bool decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XDNDECODER_H
