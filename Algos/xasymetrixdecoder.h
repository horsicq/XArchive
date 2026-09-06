/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XASYMETRIXDECODER_H
#define XASYMETRIXDECODER_H

#include "xbinary.h"

// Block framing of the Asymetrix ToolBook Setup disk-set archive.  A member is
// not one compressed stream: it is a chain of independent 4096-byte blocks,
// each introduced by {u16 method, u32 packedLength} and each carrying its own
// PKWARE DCL header and window.  The uncompressed length of a block is never
// stored - it is implied to be min(4096, remaining) - so the chain can only be
// decoded with the member's declared uncompressed size in hand.
class XAsymetrixDecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                       QByteArray *pbaUnpacked,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XASYMETRIXDECODER_H
