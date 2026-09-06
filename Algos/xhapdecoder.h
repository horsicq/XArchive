/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XHAPDECODER_H
#define XHAPDECODER_H

#include "xbinary.h"

// The codec of Harri Hirvola's HAP archiver (method 0x16 members).
//
// It is a finite-context (PPM style) model driven by a 16-bit Witten-Neal-
// Cleary arithmetic decoder.  Five orders are kept, from order 4 down to
// order 0; decoding starts at the highest order that has a context and escapes
// down a level at a time until a symbol is coded.  Each context is a run of
// (symbol, count) pairs inside one shared 32 KiB arena, allocated out of
// size-class free lists and copied to a bigger slot when it has to grow.
// Symbols already seen in a longer context are excluded from the shorter ones
// (their count byte is masked to zero and restored before the next symbol).
// Counts are 8 bit and every count in a context is halved when one of them
// saturates or when the context total would reach 0x3fff.
//
// There is no header and no self-terminating end symbol: the member's stored
// uncompressed size is what stops the loop, and the two bytes that prime the
// arithmetic decoder are the first two bytes of the member payload.
//
// Method 0x15 members are stored (compressed size equals uncompressed size) and
// never reach this file.
class XHAPDecoder {
public:
    static bool decode(const QByteArray &baPacked,
                       qint64 nUncompressedSize,
                       QByteArray *pbaUnpacked);
};

#endif  // XHAPDECODER_H
