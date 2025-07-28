/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#ifndef XIT214_H
#define XIT214_H

#include "xbinary.h"

class XIT214 : public QObject {
    Q_OBJECT

    struct STATE {
        char *ibuf;
        quint32 bitlen;
        quint8 bitnum;
        qint32 nInputBufferSize;
        char *pBufferIn;
    };

public:
    explicit XIT214(QObject *parent = nullptr);
    static bool decompress(XBinary::DECOMPRESS_STATE *pDecompressState, quint8 nBits, bool bIs215, XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    static quint32 readbits(STATE *pState, quint8 n);
    static bool readBlock(STATE *pState, XBinary::DECOMPRESS_STATE *pDecompressState);
};

#endif  // XIT214_H
