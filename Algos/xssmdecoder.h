/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSSMDECODER_H
#define XSSMDECODER_H

#include "xbinary.h"

// Decoder for Pegasus/Accusoft PICTools SSM opcode modules.  Methods 3 and 5
// share the token grammar and literal permutation; they differ only in the
// implicit eight-byte DOS MZ prefix.
class XSSMDecoder
{
public:
    static bool decompress(XBinary::DATAPROCESS_STATE *pState,
                           qint32 nMethod,
                           XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSSMDECODER_H
