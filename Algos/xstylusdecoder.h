/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XSTYLUSDECODER_H
#define XSTYLUSDECODER_H

#include "xbinary.h"

// Codec of the Stylus "DP"/SDC dictionary container (XStylus).
//
// Two things keep this from being the SZDD LZSS the library already carries:
//
//   * every byte of the compressed stream is XORed with 0xB5 before it is
//     interpreted (the container's only obfuscation), and
//   * the 4 KiB ring starts zero-filled with the write cursor at 0, and the
//     stored match position is biased by +18 (= F) instead of SZDD's +16,
//     which is what an encoder that started its cursor at N - F leaves behind.
//
// Otherwise it is textbook LZSS: an LSB-first flag byte per eight items, a
// literal byte for a set bit, and a two-byte (position, length) pair for a
// clear one, length = (b1 & 0x0F) + 3.  The stream has no end marker; input
// exhaustion terminates it, and the container's CRC-32 confirms the result.
class XStylusDecoder : public QObject {
    Q_OBJECT

public:
    explicit XStylusDecoder(QObject *parent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState,
                           XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Number of plaintext bytes the stream produces, or -1 on failure.  The
    // container stores no uncompressed size, so XStylus uses this to publish
    // one instead of reporting an unknown length.
    static qint64 measure(const quint8 *pData, qint64 nSize,
                          XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XSTYLUSDECODER_H
