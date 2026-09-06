/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XJGPAKDECODER_H
#define XJGPAKDECODER_H

#include "xbinary.h"

// JGsoft (Jan Goyvaerts / HelpScribble) "JGPAK" member codec.
//
// This is Haruyasu Yoshizaki's LZHUF in its stock F=60 / THRESHOLD=2 shape:
// an adaptive (dynamic) Huffman coder over 314 symbols -- 256 literals plus
// 257..313 standing for match lengths 3..60 -- followed by the classic LZHUF
// position code (an 8-bit lookahead selecting one of 64 high-order buckets,
// then 0..6 further bits, giving a 12-bit distance).
//
// It is NOT interchangeable with the LZHUF variants this tree already carries:
//
//   * HANDLE_METHOD_ARCV_LZHUF60 has 315 symbols because it reserves 256 as an
//     explicit end-of-stream marker, which shifts every length symbol by one
//     and therefore builds a different initial tree.  JGPAK has no end marker
//     at all: the member's declared uncompressed size is the only terminator.
//   * HANDLE_METHOD_ARCV_LZHUF/60 run a 4 KiB ring whose cursor starts at
//     N - T; JGPAK runs an 8 KiB ring whose cursor starts at 0.  The window is
//     larger than the 12-bit distances can reach, which is deliberate: it makes
//     every reference that points before the start of the member land on
//     never-written ring bytes, and the whole ring is primed with 0x20.
//
// Reference: U3 handler "JGPAK" (class adb, entry A487), worker at VA
// 0x0061eb80, decompressor entry at 0x0061ea80 calling the shared LZHUF engine
// at 0x004edae0 with (windowMode=1, lengthProfile=1, threshold=0, endMarker=0)
// -- i.e. 12-bit positions, F=60, THRESHOLD=2, no end marker.
class XJGPAKDecoder {
public:
    // Decodes one member.  nUncompressedSize is the size the directory record
    // declares and is produced exactly; the stream carries no end marker, so a
    // truncated member simply runs out of bits and the decoder fails rather
    // than emitting a short buffer.
    static bool decode(const QByteArray &packed, qint64 nUncompressedSize,
                       QByteArray *pOutput,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XJGPAKDECODER_H
