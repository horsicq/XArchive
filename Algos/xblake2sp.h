/* Copyright (c) 2024-2026 hors<horsicq@gmail.com>
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

#ifndef XBLAKE2SP_H
#define XBLAKE2SP_H

#include <QtGlobal>
#include <QIODevice>
#include <QByteArray>

class XBlake2sp {
public:
    static const qint32 DIGEST_SIZE = 32;     // 256-bit digest
    static const qint32 BLOCK_SIZE = 64;      // BLAKE2s block size
    static const qint32 PARALLEL_DEGREE = 8;  // 8-way parallelism

    struct Blake2sState {
        quint32 h[8];       // Hash state
        quint32 t[2];       // Counter (low, high)
        quint32 f[2];       // Finalization flags
        quint8 buf[64];     // Input buffer
        quint32 bufLen;     // Buffered input length
    };

    XBlake2sp();

    void init();
    void update(const quint8 *pData, qint64 nSize);
    void final(quint8 *pDigest);

    static QByteArray hash(QIODevice *pDevice);
    static QByteArray hash(const QByteArray &baData);

private:
    static void _blake2sInit(Blake2sState *pState, quint32 nOutLen, const quint8 *pParams);
    static void _blake2sCompress(Blake2sState *pState, const quint8 *pBlock, bool bLast);
    static void _blake2sFinal(Blake2sState *pState, quint8 *pDigest, quint32 nOutLen);
    static void _blake2sUpdate(Blake2sState *pState, const quint8 *pData, quint32 nSize);

    static quint32 _rotr32(quint32 nValue, quint32 nBits);
    static quint32 _load32le(const quint8 *pData);
    static void _store32le(quint8 *pDst, quint32 nValue);

    Blake2sState m_states[PARALLEL_DEGREE];  // 8 parallel leaf states
    quint8 m_buf[PARALLEL_DEGREE * BLOCK_SIZE];  // 512-byte input buffer
    qint64 m_nBufLen;
};

#endif  // XBLAKE2SP_H
