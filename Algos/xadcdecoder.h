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
#ifndef XADCDECODER_H
#define XADCDECODER_H

#include <QObject>
#include <QIODevice>
#include "xbinary.h"

// Apple Data Compression (ADC): the byte-oriented LZ77 variant behind the
// UDCO ("compressed", ADC) disk image flavour, blkx stripe type 0x80000004.
// Written from the public description of the chunk grammar; no reference
// implementation was consulted.  The first byte of each chunk selects the kind:
//
//   1xxxxxxx                      literal run, x + 1 bytes follow (1..128)
//   01llllll dddddddd dddddddd    copy l + 4 bytes (4..67) from distance d + 1 (1..65536)
//   00lllldd dddddddd             copy l + 3 bytes (3..18) from distance d + 1 (1..1024)
//
// Copies may overlap their own output (distance 1 is a byte run).  The stream
// has no end marker: decoding stops when nProcessedLimit bytes were produced,
// or, with no limit, when the bounded input is exhausted.  A copy reaching
// before the start of the output, a chunk that would overrun the limit, and an
// input that ends inside a chunk all fail closed.  nCountInput reports the
// bytes actually decoded, so a caller can require exact consumption.
class XADCDecoder : public QObject {
    Q_OBJECT

public:
    explicit XADCDecoder(QObject *parent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XADCDECODER_H
