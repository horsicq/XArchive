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
#ifndef XPANORAMADECODER_H
#define XPANORAMADECODER_H

#include "../xbinary.h"

// Panorama - a whole-file XOR over a 1024-byte keystream generated from one
// 32-bit seed by the classic Borland LCG multiplier 0x8088405.
//
// THE PAYLOAD IS RAR, NOT TAR.  Everything under the cipher is an ordinary
// RAR 4.x archive, signature "Rar!" 1a 07 00.
//
// THE SEED SOLVES DIRECTLY OUT OF THAT KNOWN PLAINTEXT:
//
//     seed = u32le(data[0:4]) ^ 0x21726152                 // "Rar!"
//
// and the remaining three signature bytes are the confirmation:
//
//     (u32le(data[4:8]) ^ (seed * 0x8088405)) & 0xffffff == 0x71a
//
// The reference searches an 18-entry table of multipliers (17 values times the
// file size, plus one literal) to arrive at the same number.  That table is
// PURE OBFUSCATION of the single constant above and is deliberately NOT ported:
// the first test alone determines the seed, because the plaintext's first four
// bytes are always the RAR signature.
//
// KEYSTREAM
//     k[0] = seed, k[n+1] = k[n] * 0x8088405 (mod 2^32)
//     the 256 words are stored little-endian into a 1024-byte pad
//     plain[i] = cipher[i] ^ pad[i & 0x3ff]
//
// The index is the ABSOLUTE FILE OFFSET, so the transform is a pure
// position-keyed stream cipher: seeking is free and the whole file decrypts in
// one pass.  That is also why the reader publishes the whole file as its single
// record - the pad phase is tied to offset 0.
class XPanoramaDecoder {
public:
    static const quint32 MULTIPLIER = 0x8088405;
    static const quint32 RAR_SIGNATURE_LOW = 0x21726152;   // "Rar!"
    static const quint32 RAR_SIGNATURE_HIGH = 0x71A;       // 1a 07 00
    static const qint32 KEYSTREAM_SIZE = 1024;

    // baHeader must hold at least the first eight bytes of the file.
    static bool seedFromHeader(const QByteArray &baHeader, quint32 *pnSeed);

    static QByteArray seedToProperty(quint32 nSeed);
    static bool propertyToSeed(const QByteArray &baProperty, quint32 *pnSeed);
    static QString methodToString();

    // baPacked is the WHOLE FILE from offset 0; the result has the same length.
    static bool decode(const QByteArray &baPacked, quint32 nSeed, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XPANORAMADECODER_H
