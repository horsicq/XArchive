/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XBSNDECODER_H
#define XBSNDECODER_H

#include "xbinary.h"

// PhysTechSoft BSA.EXE ("BSN" archive) member codec.
//
// The bit format is LHA -lh6- static Huffman: NC=510/CBIT=9 literal table,
// NT=19/TBIT=5 pre-table with the i_special=3 run, NP=16/PBIT=5 position
// table, THRESHOLD=3, and a 32 KiB dictionary.  XLZHDecoder already speaks
// exactly this dialect, but it cannot be reused here: a BSN archive is SOLID.
// The Huffman state and the bit reader restart byte-aligned at every member's
// first payload byte, while the 32 KiB window carries the PRECEDING members'
// plaintext.  Roughly 58% of the corpus decodes to garbage without that
// history (measured: 355 of 853 members survive a fresh window), so the
// history is an explicit input here rather than an implicit decoder state.
class XBSNDecoder
{
public:
    // Number of dictionary bytes a member can reach back into.  Callers only
    // need to retain this much preceding plaintext.
    static qint32 windowSize();

    // baHistory is the plaintext that precedes this member in the archive.
    // Only its final windowSize() bytes are consulted; an empty history
    // decodes the non-solid case.  nUncompressedSize is authoritative: the
    // decoder stops there and refuses a stream that would run past it.
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize,
                       const QByteArray &baHistory, QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XBSNDECODER_H
