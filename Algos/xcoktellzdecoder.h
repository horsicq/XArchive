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
#ifndef XCOKTELLZDECODER_H
#define XCOKTELLZDECODER_H

#include <QObject>
#include <QIODevice>
#include "xbinary.h"

// Coktel Vision STK/ITK LZSS (classic Okumura LZSS). Differs from the MS SZDD variant
// (XLZSSDecoder) only in the window origin: 4096-byte window initialised to 0x20, the
// initial write position is 4078 (N - F), and match offsets carry no bias. The input stream
// is the raw LZSS data (the 4-byte uncompressed-size prefix is stripped by the caller, which
// passes the uncompressed size via FPART_PROP_UNCOMPRESSEDSIZE).
class XCoktelLZDecoder : public QObject {
    Q_OBJECT

public:
    explicit XCoktelLZDecoder(QObject *parent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XCOKTELLZDECODER_H
