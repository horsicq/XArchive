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
#ifndef XQDADECODER_H
#define XQDADECODER_H

#include "xbinary.h"

// QDA's packed members are Philip Gage BYTE PAIR ENCODING (C/C++ Users Journal,
// February 1994), not an LZ: a code stands for a PAIR of codes and expands
// recursively through a per-block table.  The payload is a chain of blocks that
// simply ends when the packed bytes run out.
//
// Per block:
//   * a run-length coded pair table.  A control byte above 0x7f skips
//     (byte - 0x7f) slots, leaving them as literals, and then behaves as a
//     control byte of 0; otherwise it introduces (byte + 1) entries.  For each
//     entry the LEFT byte is read, and a RIGHT byte follows ONLY when the left
//     byte differs from the slot's own index - an entry that names itself is
//     the literal marker and carries no second byte.  The table is complete
//     once 256 slots are accounted for, and that test is made both after the
//     skip and after the entry run.
//   * a LITTLE-ENDIAN i32 block length, then that many packed bytes, each
//     expanded through a stack.
//
// TWO THINGS DIFFER from the otherwise identical XBTHPAKDecoder and they are
// why this is a separate decoder rather than a call into that one: QDA's block
// length is a little-endian 32-bit field where BTHPAK's is a big-endian 16-bit
// one, and QDA's RIGHT table SURVIVES ACROSS BLOCKS while only the left table
// is reset.  Reusing the other decoder decodes block 0 and then diverges.
class XQDADecoder {
public:
    static bool decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XQDADECODER_H
