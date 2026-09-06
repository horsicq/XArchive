/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XROMPAQDECODER_H
#define XROMPAQDECODER_H

#include "xbinary.h"

// Compaq ROMPAQ multi-part firmware image.
//
// A multi-part image is not one compressed stream: it is a chain of banks,
// each introduced by its own complete 0x48-byte ROMPAQ header and each
// carrying an independent PKWARE Data Compression Library stream (the same
// codec XDclDecoder already implements).  The header of a bank stores that
// bank's PACKED length at offset 0x3F; the unpacked length is not stored
// anywhere per bank - only the whole image's size, in the first header at
// offset 0x00 - so the chain is decoded until the input runs out and then
// checked against that total.
//
// Single-part images carry no chain at all and are dispatched straight to
// HANDLE_METHOD_PKWARE_DCL_IMPLODE; this decoder is only for the chained form.
class XRomPaqDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       QByteArray *pbaUnpacked,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XROMPAQDECODER_H
