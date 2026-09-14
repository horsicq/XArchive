/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XNINTENDOLZDECODER_H
#define XNINTENDOLZDECODER_H

#include "../xbinary.h"

// Codec of the Nintendo "LZ77" family: the BIOS/SDK LZ streams of the GBA, DS,
// DSi, 3DS and Wii, known as LZ10 (type byte 0x10) and LZ11 (type byte 0x11).
// Format understanding derived from the wiibrew.org LZ77 page, GBATEK's
// LZ77UnCompReadNormalWrite description, libWiiPy (MIT) and the DSDecmp LZ11
// format notes; nothing was copied from any of them.
//
// Header (little-endian u32 viewed as a whole; bits 4-7 = 1 for LZ77):
//
//   +0  u8   type byte           0x10 = LZ10, 0x11 = LZ11 (the reserved
//                                low nibble set)
//   +1  u24  plaintext length    little endian
//   +4  u32  length extension    ONLY when the 24-bit field is 0 (the DSDecmp
//                                form for files past 16 MiB); must be 1 ..
//                                0x7FFFFFFF
//
// An optional four-byte "LZ77" tag may precede the header (Wii tooling); the
// header parser reports it and the header size it implies.
//
// Item stream: a flag byte, MSB first, then eight items; a 0 bit is one
// literal byte, a 1 bit is a back reference into what has already been
// produced.  The loop is driven by the declared plaintext length, checked
// after EVERY item, so the unused slots of the last flag byte are never read.
//
//   LZ10 reference, 2 bytes:  b0 = LLLL dddd, b1 = dddddddd
//       length   = (b0 >> 4) + 3                  3 .. 18
//       distance = (((b0 & 0xF) << 8) | b1) + 1   1 .. 4096
//
//   LZ11 reference, 2/3/4 bytes selected by the indicator I = b0 >> 4:
//       I >= 2:  length = I + 1                                        3 .. 16
//       I == 0:  length = (((b0 & 0xF) << 4) | (b1 >> 4)) + 0x11       17 .. 272
//       I == 1:  length = (((b0 & 0xF) << 12) | (b1 << 4) | (b2 >> 4)) + 0x111
//                                                                  273 .. 65808
//       the distance is always the last 12 bits of the reference, plus 1.
//
// Error policy - each of these is a hard failure (false, empty output), where
// the reference decoders differ (libWiiPy and ndspy silently emit garbage on
// the first two, DSDecmp produces MORE than the declared length on the third):
//   E1  the input ends before the declared length is reached;
//   E2  a reference reaches back before the start of the output;
//   E3  a reference would overshoot the declared length.  No Nintendo encoder
//       produces such a stream (every one computes the match length against
//       the bytes left), and the exact-length equality is the only integrity
//       check this format has, so it is enforced strictly.
// Trailing bytes behind the stream are the container's business; the decoder
// never reads them and reports how many bytes it consumed instead.
//
// scan() is the trial decode used by the detection gate: it applies the same
// rules but keeps only a 4 KiB history ring (the maximum distance), so a header
// that claims 512 MiB costs time bounded by that length, never memory.
class XNintendoLZDecoder {
public:
    enum VARIANT {
        VARIANT_LZ10 = 0x10,
        VARIANT_LZ11 = 0x11
    };

    struct HEADER {
        bool bHasTag;              // "LZ77" present at +0
        VARIANT variant;           // from the type byte
        qint64 nUncompressedSize;  // 24-bit field, or the 32-bit extension
        qint64 nHeaderSize;        // 4, 8 (tag or extension) or 12 (both)
        bool bExtendedLength;      // the 24-bit field was 0
    };

    // Parses the optional tag and the header only.  false when nSize is too
    // small, the type byte is not 0x10/0x11, the 24-bit length is 0 without
    // an extension behind it, or the extension is 0 or above 0x7FFFFFFF.
    static bool parseHeader(const quint8 *pData, qint64 nSize, HEADER *pHeader);

    // Whole-buffer shape used by the XDecompress dispatcher.  baPacked is the
    // ITEM STREAM ONLY (the container reader publishes offsets past the header).
    // Returns true only when exactly nUncompressedSize bytes were produced.  A
    // length the packed bytes could not possibly encode (LZ10 above 9x, LZ11
    // above 16384x, plus a small slack) is refused before anything is allocated.
    static bool decode(const QByteArray &baPacked, VARIANT variant, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decodeLZ10(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static bool decodeLZ11(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Trial decode for a detection gate: same rules as decode() over a 4 KiB
    // history ring.  *pnConsumed receives the number of stream bytes read when
    // the declared length was reached (trailing bytes excluded).
    static bool scan(const quint8 *pData, qint64 nSize, VARIANT variant, qint64 nUncompressedSize, qint64 *pnConsumed, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XNINTENDOLZDECODER_H
