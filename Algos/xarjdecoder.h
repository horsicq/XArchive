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
#ifndef XARJDECODER_H
#define XARJDECODER_H

#include "xbinary.h"

class XArjDecoder : public QObject {
    Q_OBJECT

public:
    explicit XArjDecoder(QObject *pParent = nullptr);

    // Decompress ARJ methods 1-3 (Huffman + LZSS)
    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
    // Decompress ARJ method 4 (fastest, simple LZSS)
    static bool decompressFastest(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    static const qint32 MAXDICBIT = 16;
    static const qint32 DDICSIZ = 26624;
    static const qint32 THRESHOLD = 3;
    static const qint32 MAXMATCH = 256;
    static const qint32 CODE_BIT = 16;
    static const qint32 NT = CODE_BIT + 3;         // 19
    static const qint32 PBIT = 5;
    static const qint32 TBIT = 5;
    static const qint32 NC = 255 + MAXMATCH + 2 - THRESHOLD;  // 510
    static const qint32 NP = MAXDICBIT + 1;         // 17
    static const qint32 CBIT = 9;
    static const qint32 CTABLESIZE = 4096;
    static const qint32 PTABLESIZE = 256;
    static const qint32 STRTP = 9;
    static const qint32 STOPP = 13;
    static const qint32 STRTL = 0;
    static const qint32 STOPL = 7;
    static const qint32 NPT = 19;  // max(NT, NP) = max(19, 17)

    struct ArjDecodeState {
        quint16 nBitBuf;
        quint8 nSubBitBuf;
        qint32 nBitCount;
        quint16 nBlockSize;
        qint16 nGetLen;
        qint16 nGetBuf;
        quint16 arrLeft[2 * NC - 1];
        quint16 arrRight[2 * NC - 1];
        quint8 arrCLen[NC];
        quint16 arrCTable[CTABLESIZE];
        quint8 arrPtLen[NPT];
        quint16 arrPtTable[PTABLESIZE];
        quint8 *pText;
        bool bError;
        // Input buffer
        const quint8 *pBuf;
        qint32 nBufAvail;
        quint32 nCompLeft;
        // I/O devices for refilling
        QIODevice *pInput;
        qint64 nInputBytesRead;
        quint8 *pReadBuffer;
        qint32 nReadBufferSize;
    };

    static void refillInputBuffer(ArjDecodeState *pState);
    static bool fillBuf(ArjDecodeState *pState, qint32 nBits);
    static quint16 getBits(ArjDecodeState *pState, qint32 nBits);
    static bool initGetBits(ArjDecodeState *pState);

    static bool makeTable(ArjDecodeState *pState, qint32 nChar, quint8 *pBitLen,
                          qint32 nTableBits, quint16 *pTable, qint32 nTableSize);
    static bool readPtLen(ArjDecodeState *pState, qint32 nCount, qint32 nBitWidth, qint32 nSpecial);
    static bool readCLen(ArjDecodeState *pState);
    static quint16 decodeC(ArjDecodeState *pState);
    static quint16 decodeP(ArjDecodeState *pState);

    // Method 4 helpers
    static quint16 decodeLen(ArjDecodeState *pState);
    static quint16 decodePtr(ArjDecodeState *pState);
};

#endif  // XARJDECODER_H
