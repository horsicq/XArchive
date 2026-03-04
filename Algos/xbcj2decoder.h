/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#ifndef XBCJ2DECODER_H
#define XBCJ2DECODER_H

#include "xbinary.h"

// BCJ2 inverse transform decoder (7-Zip x86 4-stream filter)
// Codec ID: 0x03 0x03 0x01 0x1B
//
// BCJ2 takes 4 input streams:
//   Stream 0 (main):  LZMA-compressed main binary data (E8/E9 operands stripped)
//   Stream 1 (call):  LZMA-compressed absolute E8 (CALL) operand addresses
//   Stream 2 (jmp):   LZMA-compressed absolute E9 (JMP)  operand addresses
//   Stream 3 (range): Raw range-coder stream (BCJ2 probability model)
// and produces 1 output stream: the original binary data.
class XBCJ2Decoder : public QObject {
    Q_OBJECT

public:
    explicit XBCJ2Decoder(QObject *parent = nullptr);

    // Reconstruct original binary from 4 BCJ2 input streams.
    // pMainStream  : decompressed main data (from LZMA1)
    // pCallStream  : decompressed call (E8) addresses (from LZMA2)
    // pJmpStream   : decompressed jmp  (E9) addresses (from LZMA3)
    // pRangeStream : raw range-coder data (4th pack stream, not LZMA)
    // pOutput      : destination device (must be open for writing)
    // nOutputSize  : expected number of output bytes
    static bool decompress(QIODevice *pMainStream, QIODevice *pCallStream, QIODevice *pJmpStream,
                           QIODevice *pRangeStream, QIODevice *pOutput, qint64 nOutputSize,
                           XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    // Range-coder state used only by BCJ2 probability decoding
    struct RC_STATE {
        QIODevice *pStream;
        quint32 nRange;
        quint32 nCode;
        bool bEof;
    };

    static bool _rcInit(RC_STATE *pRC);
    static void _rcNormalize(RC_STATE *pRC);
    static quint32 _rcDecodeBit(RC_STATE *pRC, quint32 *pProb);
};

#endif  // XBCJ2DECODER_H
