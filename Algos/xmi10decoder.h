/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XMI10DECODER_H
#define XMI10DECODER_H

#include "xbinary.h"

// Decoder for the backward byte-oriented LZ stream used by the Amiga MI10
// cruncher.  MI10 is unrelated to the Microsoft compression formats with
// similarly short names.
class XMI10Decoder : public QObject
{
    Q_OBJECT

public:
    explicit XMI10Decoder(QObject *pParent = nullptr);
    static bool decompress(XBinary::DATAPROCESS_STATE *pState,
                           XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XMI10DECODER_H
