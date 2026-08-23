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
#ifndef XARCDECODER_H
#define XARCDECODER_H

#include "xbinary.h"

#include <QObject>

/* Decoders for the SEA ARC compression methods above plain storage.
 *
 * ARC layers two independent stages.  The outer stage is always the same
 * byte-oriented run-length expansion (`putc_unp` in the original sources); the
 * inner stage is nothing, a static Huffman tree, or LZW.  Which methods use the
 * run-length stage is a property of the method id, not of the data:
 *
 *   3  packed          run-length only
 *   4  squeezed        Huffman  -> run-length
 *   5  crunched (old)  LZW, fixed width, no run-length stage
 *   6  crunched        LZW, fixed width -> run-length
 *   7  crunched        as 6; only the encoder's hash differs, so it decodes identically
 *   8  crunched        LZW, dynamic width -> run-length
 *   9  squashed        LZW, dynamic width, no run-length stage
 *
 * The two stages are composed inside one decoder rather than being chained
 * through the shared multi-method path, because only the final byte count may be
 * compared against the record's declared original size. */

class XArcDecoder : public QObject {
    Q_OBJECT

public:
    explicit XArcDecoder(QObject *parent = nullptr);

    // nMethod is the SEA ARC method id, 3..9.
    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, qint32 nMethod, XBinary::PDSTRUCT *pPdStruct = nullptr);
};

#endif  // XARCDECODER_H
