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
#include "xsha256decoder.h"

#include <string.h>

XSha256Decoder::XSha256Decoder(QObject *parent) : QObject(parent)
{
}

void XSha256Decoder::init(Context *pContext)
{
    pContext->count = 0;
    pContext->state[0] = 0x6a09e667;
    pContext->state[1] = 0xbb67ae85;
    pContext->state[2] = 0x3c6ef372;
    pContext->state[3] = 0xa54ff53a;
    pContext->state[4] = 0x510e527f;
    pContext->state[5] = 0x9b05688c;
    pContext->state[6] = 0x1f83d9ab;
    pContext->state[7] = 0x5be0cd19;
}

quint32 XSha256Decoder::rotr(quint32 x, quint32 n)
{
    return (x >> n) | (x << (32 - n));
}

quint32 XSha256Decoder::getBe32(const quint8 *p)
{
    return ((quint32)p[0] << 24) | ((quint32)p[1] << 16) | ((quint32)p[2] << 8) | ((quint32)p[3]);
}

void XSha256Decoder::setBe32(quint8 *p, quint32 v)
{
    p[0] = (quint8)(v >> 24);
    p[1] = (quint8)(v >> 16);
    p[2] = (quint8)(v >> 8);
    p[3] = (quint8)(v);
}

void XSha256Decoder::transform(quint32 state[8], const quint8 data[64])
{
    // SHA-256 constants (K array)
    static const quint32 K[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                                  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                                  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                                  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                                  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                                  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                                  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                                  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

    quint32 W[64];
    quint32 a, b, c, d, e, f, g, h;
    quint32 T1, T2;
    qint32 i;

    // Prepare message schedule
    for (i = 0; i < 16; i++) {
        W[i] = getBe32(data + i * 4);
    }

    for (i = 16; i < 64; i++) {
        quint32 s0 = rotr(W[i - 15], 7) ^ rotr(W[i - 15], 18) ^ (W[i - 15] >> 3);
        quint32 s1 = rotr(W[i - 2], 17) ^ rotr(W[i - 2], 19) ^ (W[i - 2] >> 10);
        W[i] = W[i - 16] + s0 + W[i - 7] + s1;
    }

    // Initialize working variables
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];

    // Main loop
    for (i = 0; i < 64; i++) {
        quint32 S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        quint32 S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        quint32 ch = (e & f) ^ ((~e) & g);
        quint32 maj = (a & b) ^ (a & c) ^ (b & c);

        T1 = h + S1 + ch + K[i] + W[i];
        T2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    // Add the working variables back into state
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void XSha256Decoder::update(Context *pContext, const quint8 *pData, qint32 nSize)
{
    if (nSize == 0) {
        return;
    }

    quint32 nPos = (quint32)pContext->count & 63;
    quint32 nNum = 64 - nPos;

    pContext->count += nSize;

    if (nNum > (quint32)nSize) {
        memcpy(pContext->buffer + nPos, pData, nSize);
        return;
    }

    if (nPos != 0) {
        nSize -= nNum;
        memcpy(pContext->buffer + nPos, pData, nNum);
        pData += nNum;
        transform(pContext->state, pContext->buffer);
    }

    qint32 nNumBlocks = nSize >> 6;
    for (qint32 i = 0; i < nNumBlocks; i++) {
        transform(pContext->state, pData);
        pData += 64;
    }

    nSize &= 63;
    if (nSize != 0) {
        memcpy(pContext->buffer, pData, nSize);
    }
}

void XSha256Decoder::final(Context *pContext, quint8 *pDigest)
{
    quint32 nPos = (quint32)pContext->count & 63;
    pContext->buffer[nPos++] = 0x80;

    if (nPos > 56) {
        while (nPos != 64) {
            pContext->buffer[nPos++] = 0;
        }
        transform(pContext->state, pContext->buffer);
        nPos = 0;
    }

    memset(&pContext->buffer[nPos], 0, 56 - nPos);

    quint64 nNumBits = pContext->count << 3;
    setBe32(pContext->buffer + 56, (quint32)(nNumBits >> 32));
    setBe32(pContext->buffer + 60, (quint32)(nNumBits));

    transform(pContext->state, pContext->buffer);

    for (qint32 i = 0; i < 8; i++) {
        setBe32(pDigest + i * 4, pContext->state[i]);
    }

    init(pContext);
}
