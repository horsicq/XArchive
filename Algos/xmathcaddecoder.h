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
#ifndef XMATHCADDECODER_H
#define XMATHCADDECODER_H

#include <QIODevice>
#include <QObject>

#include "xbinary.h"

// MathSoft MathCAD ".MCDCOMPRESSION" worksheet stream.
//
// The payload that follows the 15-byte ASCII magic is a bit-oriented LZSS with
// a 4096-byte ring window.  Bits are consumed MSB-first inside each byte:
//
//   1 + <8 bits>              literal byte
//   0 + <12 bits> + <4 bits>  ring position (1-based) and length-2
//   0 + 000000000000          end of stream (position 0 is the terminator)
//
// The window pointer starts at index 1, not 0, which is what makes position 0
// available as the terminator; a decoder that starts it at 0 reproduces the
// right byte count and the wrong bytes for every single file.  There is no
// stored uncompressed size, so the terminator is the only stop condition.
class XMathCadDecoder : public QObject {
    Q_OBJECT

public:
    explicit XMathCadDecoder(QObject *parent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XMATHCADDECODER_H
