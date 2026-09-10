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
#ifndef XZIEDECODER_H
#define XZIEDECODER_H

#include "../xbinary.h"

// ZIE - the "ProtectIt/2" (OS/2) encrypted ZIP wrapper.  There is no
// compression here at all: strip the 0x118-byte header, undo a 16-byte
// repeating XOR, and what is left is an ordinary ZIP.
//
// HEADER (0x118 bytes, then the encrypted payload)
//   +0x000  u32       0x32544950 = "PIT2"
//   +0x004  16 bytes  obfuscated key
//   +0x014  13 bytes  original file name, NUL padded, validated as an 8.3-ish
//                     name (the reference's own check, ported verbatim)
//   +0x024 .. 0x118   zero padding
//
// THE KEY.  The reference XORs the blob at +4 with TWO 16-byte tables held in
// its data section.  Both XORs land on the same 16 bytes, so the pair is just a
// split constant, and the constant is the product name:
//
//     table_a ^ table_b == "ProtectIt/2 OS/2"
//     key[i] = header[4 + i] ^ "ProtectIt/2 OS/2"[i]      for i in 0..15
//
// The split into two tables is pure obfuscation and is not reproduced.
//
// THE CIPHER is a 16-byte repeating XOR whose PHASE IS DERIVED FROM THE PAYLOAD
// LENGTH:
//
//     size = (filesize - 0x118) rounded DOWN to a multiple of 4
//     rot  = (0x10 - (size & 0xf)) & 0xf
//     rot += 8 for the first 0xC0000 bytes when size >= 0xC0000
//     plain[pos] = cipher[pos] ^ key[(pos + rot) & 0xf]
//
// and the last `payload % 4` bytes are left IN THE CLEAR.
//
// TRUNCATED ARCHIVES - the reason resolveMethod() exists.  Because the phase
// comes from the LENGTH, a truncated .zie cannot be decrypted by the length
// rule, and 5 of the 13 reference samples are truncated (no end-of-central
// directory).  The reference extracts ZERO files from all five.  Their true
// phase is 0, and using it recovers 11-20 CRC-verified members out of each, so
// the phase is resolved by KNOWN PLAINTEXT instead: the archive underneath has
// to start with "PK" 03 04, the length-derived phase is tried first and phase 0
// is the fallback.  That reproduces the reference on the eight intact samples
// and beats it on the other five.
class XZIEDecoder {
public:
    struct METHOD {
        quint8 nKey[16];
        qint32 nBase;      // 0..15, the phase the payload was encrypted with
        bool bRecovered;   // true when the length rule failed and phase 0 was used
    };

    static const qint64 HEADER_SIZE = 0x118;
    static const qint64 LARGE_PAYLOAD_SIZE = 0xC0000;

    // Magic plus the reference's 8.3-ish name check on +0x14.
    static bool isValidHeader(const QByteArray &baHeader);
    static QString fileNameFromHeader(const QByteArray &baHeader);

    // baProbe holds the first four payload bytes (file offset 0x118).
    static bool resolveMethod(const QByteArray &baHeader, const QByteArray &baProbe, qint64 nPayloadSize, METHOD *pMethod);

    static QByteArray methodToProperty(const METHOD &method);
    static bool propertyToMethod(const QByteArray &baProperty, METHOD *pMethod);
    static QString methodToString(const METHOD &method);

    // baPacked is the payload, i.e. everything from file offset 0x118 on.  The
    // result has exactly the same length.
    static bool decode(const QByteArray &baPacked, const METHOD &method, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XZIEDECODER_H
