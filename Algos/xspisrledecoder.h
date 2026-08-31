/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XSPISRLEDECODER_H
#define XSPISRLEDECODER_H

#include "xbinary.h"

// GP-Install/SPIS byte-run decoder. A literal byte establishes the current
// value. For 0x94,count, count >= 2 emits count-1 copies, zero emits a literal
// 0x94, and one emits nothing. Every escape token leaves 0x94 as the current
// value, matching the original GP-Install expander.
class XSPISRLEDecoder : public QObject {
    Q_OBJECT

public:
    explicit XSPISRLEDecoder(QObject *pParent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSPISRLEDECODER_H
