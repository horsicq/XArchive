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
#ifndef XTOPSPEEDDECODER_H
#define XTOPSPEEDDECODER_H

#include "../xbinary.h"

// TopSpeed (JPI) distribution-disk compression - one member, no directory and
// NO MAGIC NUMBER of any kind.
//
// The file is a bare chain of blocks running to EOF:
//
//   +0  u16  checksum, the low 16 bits of the plain arithmetic SUM of this
//            block's on-disk payload bytes
//   +2  u16  uncompressed length, 1..0x3800
//   +4  u16  compressed length; when it is >= the uncompressed length the
//            block is STORED instead
//   +6       payload, min(uncompressed, compressed) bytes
//
// The member is every block's output concatenated.  Because there is no
// signature, identification IS the walk: every checksum must match and the
// chain must land exactly on the end of the file.  That is a strong enough
// test on its own - it produced no false positive over 6593 files drawn from
// the other 355 families of the reference corpus.
//
// A trap in the header: in a STORED block the compressed length is the
// hypothetical compressed size, NOT an on-disk length, and the corpus contains
// stored blocks whose value exceeds the cap that applies to real compressed
// blocks.  The cap may only be tested on the compressed branch.
//
// The codec is 12-bit LZW with NO clear code and NO variable code width; the
// dictionary is reset by exhaustion.  Codes are packed two per three bytes,
// high nibbles first:
//
//   b0 = ((codeA >> 8) << 4) | (codeB >> 8)
//   b1 = codeA & 0xFF
//   b2 = codeB & 0xFF
//
// Two details in the dictionary are easy to get wrong and both are exercised
// by the reference corpus (22726 KwKwK cases, 897 resets):
//
//   * the prefix of entry `next + 1` is written from the code just read,
//     BEFORE that code is decoded.  That off-by-one write is exactly what
//     makes the KwKwK branch resolve through prefix[next] to the previous
//     code;
//   * the full-dictionary test is evaluated AFTER reading the next code, so
//     entries 0x100..0xFFE are used and the code that trips the reset becomes
//     the first literal of the new dictionary.  Nothing is cleared - entries
//     are overwritten in order.
class XTopSpeedDecoder {
public:
    // Walk the block chain. Returns false unless every checksum matches and the
    // chain ends exactly at nFileSize. *pnUncompressedSize gets the sum of the
    // block output sizes.
    static bool measure(const QByteArray &baFile, qint64 nFileSize, qint64 *pnUncompressedSize);

    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XTOPSPEEDDECODER_H
