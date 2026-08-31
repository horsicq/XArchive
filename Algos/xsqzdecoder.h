/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSQZDECODER_H
#define XSQZDECODER_H

#include "xbinary.h"

// Decoder for Squeeze It (SQZ) methods 1-4.  The four methods share the
// archive's 14-bit-block static-Huffman stream; odd methods use the compact
// distance map and methods 3/4 use the extended match-length map.
class XSQZDecoder {
public:
    static bool decompress(XBinary::DATAPROCESS_STATE *pState, qint32 nMethod, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSQZDECODER_H
