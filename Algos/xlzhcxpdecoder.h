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
#ifndef XLZHCXPDECODER_H
#define XLZHCXPDECODER_H

#include <QByteArray>
#include <QtGlobal>

// Codec of the "LZ FF 00" single-file container (XLZHCXP).
//
// The payload that follows the two-byte "LZ" magic is NOT a flat bit stream: it
// is a chain of BLOCKS, each one a length byte followed by that many bytes of
// bit-stream, and a zero-length block ends the file.  The framing is invisible
// to the codec above it - the bit reader simply skips over the length bytes -
// which is why a naive reader that starts at a fixed offset decodes garbage.
//
// The codec itself is LZW, LSB-first, with an unusual code space:
//
//   0x000..0x0FF  literals
//   0x100..0x1FF  never emitted; meeting one is a hard error
//   0x200         CLEAR - reset width/next-code, then the code that follows is
//                 written out directly and becomes the new previous code
//   0x201         END
//   0x202..0xFFF  dictionary entries
//
// The code width starts at 10 bits and is widened (to a maximum of 12) at the
// moment the next free code reaches 1 << width, checked BEFORE each code is
// read.  The dictionary stops growing at 0x1000 instead of forcing a reset.
// Running out of input is a normal end of stream, exactly as in the original.
class XLzhcxpDecoder {
public:
    // baInput starts at the first block-length byte, i.e. at offset 2 of the
    // container.  nMaxOutput caps the produced size (< 0 means "no cap");
    // nExpectedSize, when >= 0, additionally requires an exact match.
    static bool decode(const QByteArray &baInput, qint64 nExpectedSize,
                       qint64 nMaxOutput, QByteArray *pbaResult);
};

#endif  // XLZHCXPDECODER_H
