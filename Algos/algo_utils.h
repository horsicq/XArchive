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
#ifndef ALGO_UTILS_H
#define ALGO_UTILS_H

#include "xalgo_local.h"
#include "xucldecoder.h"
#include "xbinary.h"

#include <QIODevice>
#include <QByteArray>

class Algo_utils {
public:
    struct QIODeviceByteInStream {
        IByteIn vt;
        QIODevice *pDevice;
        bool bError;
    };

    static int ascii85ReadByte(XBinary::DATAPROCESS_STATE *pState);
    static void ascii85WriteBytes(XBinary::DATAPROCESS_STATE *pState, const unsigned char *pBuffer, int nSize);

    static void *szAlloc(ISzAllocPtr pAlloc, size_t nSize);
    static void szFree(ISzAllocPtr pAlloc, void *pAddress);
    static ISzAlloc *lzmaAlloc();
    static bool decompressLZMA(CLzmaDec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct);
    static bool decompressLZMA2(CLzma2Dec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct);

    static bool xzReadVarInt(const QByteArray &baData, qint32 &nPos, quint64 &nValue);
    static void applyBCJX86Decode(QByteArray &baData);

    static unsigned deflate64ReadFunc(void *pInDesc, unsigned char **ppBuffer);
    static int deflate64WriteFunc(void *pOutDesc, unsigned char *pBuffer, unsigned nSize);
    static bool compressDeflate(XBinary::DATAPROCESS_STATE *pCompressState, XBinary::PDSTRUCT *pPdStruct, int nCompressionLevel, int nWindowBits);

    static bool getUclMethodFromState(const XBinary::DATAPROCESS_STATE *pDecompressState, XUCLDecoder::METHOD *pMethod);
    static bool readInputData(XBinary::DATAPROCESS_STATE *pDecompressState, QByteArray *pbaInput, XBinary::PDSTRUCT *pPdStruct);

    static QByteArray hmacSha1(const QByteArray &baKey, const QByteArray &baMessage);
    static ISzAlloc *ppmdAlloc();
    static Byte readFromQIODeviceStream(const IByteIn *pStream);
    static size_t readFromState(void *pState, void *pBuffer, size_t nSize);
    static size_t writeToState(void *pState, const void *pBuffer, size_t nSize);
};

#endif  // ALGO_UTILS_H
