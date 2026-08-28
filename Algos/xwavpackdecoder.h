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
#ifndef XWAVPACKDECODER_H
#define XWAVPACKDECODER_H

#include "xbinary.h"

// Decoder for ZIP compression method 97 (WavPack, APPNOTE 5.9): the member's
// compressed data is a complete WavPack stream produced from the original
// file in lossless mode with the source header/trailer stored as wrapper
// data. Decoding restores the original file byte for bit by writing the
// stored leading wrapper, the re-formatted decoded samples, and the stored
// trailing wrapper. Built on Algos/wavpackdeclib.cpp, the decoder-only
// amalgamation of WavPack (BSD-3-Clause, (c) David Bryant).
class XWavPackDecoder : public QObject {
    Q_OBJECT

public:
    explicit XWavPackDecoder(QObject *parent = nullptr);

    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct);
};

#endif  // XWAVPACKDECODER_H
