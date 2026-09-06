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
#ifndef XIRWINPACDECODER_H
#define XIRWINPACDECODER_H

#include <QIODevice>
#include <QObject>

#include "xbinary.h"

// Irwin Magnetic Systems "IrwinPac" installation-file stream.
//
// The input handed to this decoder starts at the first chunk header, i.e. right
// after the 20-byte "IrwinPac" container header, and runs to the end of the
// file.  It is a chain of independent blocks:
//
//   +0  u16  flag         0 = stored, 1 = compressed
//   +2  u16  chunk size   INCLUDING this 10-byte header
//   +4  u16  unpacked size of the block (16384 for every block but the last)
//   +6  4    uninitialised encoder scratch - constant per file, never read
//   +10      payload (chunk size - 10 bytes)
//
// A compressed payload is a bit stream consumed MSB-first inside each byte and
// restarted byte-aligned at every block; matches never reach across a block
// boundary, so the whole history a block needs is the block itself:
//
//   0 + <8 bits>                  literal byte
//   1 + 1 + <7 bits>              match, distance 1..127
//   1 + 0 + <11 bits>             match, distance 1..2047
//   then the length, a nibble-escalating code:
//       <2 bits> v, v < 3        -> 2 + v
//       else <2 bits> v, v < 3   -> 5 + v
//       else <4 bits> v, v < 15  -> 8 + v, and while v == 15 add 15 to the base
//                                   and read the next nibble
//
// The encoder pads the tail of every block with about a dozen spare bytes, so
// the block's unpacked size - not stream exhaustion - is the stop condition.
class XIrwinPacDecoder : public QObject {
    Q_OBJECT

public:
    explicit XIrwinPacDecoder(QObject *parent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XIRWINPACDECODER_H
