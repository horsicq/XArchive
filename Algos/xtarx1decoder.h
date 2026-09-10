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
#ifndef XTARX1DECODER_H
#define XTARX1DECODER_H

#include "xbinary.h"

// TARX 1 (QNX .tarx): a 32-bit stream cipher wrapped around an ordinary POSIX
// tar.  Nothing is compressed - the entire format is the cipher.
//
//   state = base = key
//   for every cipher byte c:
//       plain = c ^ (state & 0xff)
//       state = (state >> 8) ^ TAB[((state + base + plain) ^ state) & 0xff]
//
// TAB is the standard reflected CRC-32 table (polynomial 0xEDB88320) and the
// state is fed by the PLAINTEXT, so the keystream cannot be produced ahead of
// the data and no member can be decrypted without running the cipher from the
// first byte of the container.  That is why the reader hands every member the
// same stream and a byte count to discard.
//
// THE KEY IS NOT STORED.  It is recovered from the 100 header bytes at file
// offset 4 by running the cipher BACKWARDS: the four bytes at h[0x5f] and the
// checksum byte at h[99] pin the state at the end of that window for a given
// low key byte, the state is then unwound one byte at a time through the
// inverse table INV[TAB[i] >> 24] = i and the carry mixer, and the candidate is
// accepted only when the unwound state's low byte reproduces the byte the
// candidate started from.  All 256 low bytes are tried and the key is taken
// only when EXACTLY ONE of them closes that loop - a second match would mean
// the window does not determine the key and guessing between them would emit a
// plausible-looking but wrong archive.
class XTARX1Decoder {
public:
    // baHeader is the 100 bytes at file offset 4.
    static bool recoverKey(const QByteArray &baHeader, quint32 *pnKey);

    static bool decrypt(const QByteArray &baCipher, quint32 nKey, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Whole-buffer entry point.  baPacked is the ciphertext from file offset 4,
    // i.e. it starts with the same 100 bytes recoverKey() needs; nSkipSize is
    // how much plaintext belongs to the members ahead of this one.
    static bool decode(const QByteArray &baPacked, qint64 nSkipSize, qint64 nUncompressedSize, QByteArray *pbaResult,
                       XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTARX1DECODER_H
