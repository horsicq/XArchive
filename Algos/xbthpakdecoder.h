/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBTHPAKDECODER_H
#define XBTHPAKDECODER_H

#include "xbinary.h"

// Byte Pair Encoding (Philip Gage, C/C++ Users Journal, February 1994) as used
// by the "PAK" single-member container of the Park Place Productions installer
// (INSTBTH.EXE).  The payload is a chain of self-contained blocks; each block
// carries its own pair table, then a BIG-endian 16-bit packed length inside an
// otherwise little-endian container, then that many packed bytes.  There is no
// end marker: the chain simply lands on EOF.
//
// Written from the byte layout recovered from the samples; no third-party code
// is vendored.  The historical C reference has three latent memory-safety bugs
// (a 30-entry expansion stack, an unchecked table index and a zero-length block
// that spins forever); none of them is reproduced here.
class XBTHPAKDecoder {
public:
    static bool decode(const QByteArray &packed, qint64 expectedSize,
                       QByteArray *output,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Structural walk used by the detection gate: parses every block's pair
    // table and skips its packed bytes without expanding anything, so a
    // candidate is rejected without an attacker-controlled allocation.
    // nBlockLimit <= 0 walks the whole chain.  With bRequireExactEnd the walk
    // must land exactly on the end of the payload.
    static bool scanChain(const QByteArray &payload, bool bRequireExactEnd,
                          qint32 nBlockLimit, qint64 *pnConsumed,
                          qint32 *pnBlockCount,
                          XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XBTHPAKDECODER_H
