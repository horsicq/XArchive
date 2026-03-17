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

#include "xblake2sp.h"

#include <cstring>

// BLAKE2s IV constants (same as SHA-256 initial values)
static const quint32 g_blake2s_IV[8] = {0x6A09E667UL, 0xBB67AE85UL, 0x3C6EF372UL, 0xA54FF53AUL, 0x510E527FUL, 0x9B05688CUL, 0x1F83D9ABUL, 0x5BE0CD19UL};

// BLAKE2s sigma permutations
static const quint8 g_blake2s_sigma[10][16] = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
                                               {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4}, {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
                                               {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13}, {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
                                               {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11}, {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
                                               {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5}, {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0}};

quint32 XBlake2sp::_rotr32(quint32 nValue, quint32 nBits)
{
    return (nValue >> nBits) | (nValue << (32 - nBits));
}

quint32 XBlake2sp::_load32le(const quint8 *pData)
{
    return ((quint32)pData[0]) | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16) | ((quint32)pData[3] << 24);
}

void XBlake2sp::_store32le(quint8 *pDst, quint32 nValue)
{
    pDst[0] = (quint8)(nValue);
    pDst[1] = (quint8)(nValue >> 8);
    pDst[2] = (quint8)(nValue >> 16);
    pDst[3] = (quint8)(nValue >> 24);
}

// G mixing function
#define BLAKE2S_G(r, i, a, b, c, d)                   \
    do {                                              \
        a = a + b + m[g_blake2s_sigma[r][2 * i + 0]]; \
        d = XBlake2sp::_rotr32(d ^ a, 16);            \
        c = c + d;                                    \
        b = XBlake2sp::_rotr32(b ^ c, 12);            \
        a = a + b + m[g_blake2s_sigma[r][2 * i + 1]]; \
        d = XBlake2sp::_rotr32(d ^ a, 8);             \
        c = c + d;                                    \
        b = XBlake2sp::_rotr32(b ^ c, 7);             \
    } while (0)

#define BLAKE2S_ROUND(r)                           \
    do {                                           \
        BLAKE2S_G(r, 0, v[0], v[4], v[8], v[12]);  \
        BLAKE2S_G(r, 1, v[1], v[5], v[9], v[13]);  \
        BLAKE2S_G(r, 2, v[2], v[6], v[10], v[14]); \
        BLAKE2S_G(r, 3, v[3], v[7], v[11], v[15]); \
        BLAKE2S_G(r, 4, v[0], v[5], v[10], v[15]); \
        BLAKE2S_G(r, 5, v[1], v[6], v[11], v[12]); \
        BLAKE2S_G(r, 6, v[2], v[7], v[8], v[13]);  \
        BLAKE2S_G(r, 7, v[3], v[4], v[9], v[14]);  \
    } while (0)

void XBlake2sp::_blake2sCompress(Blake2sState *pState, const quint8 *pBlock, bool bLast)
{
    quint32 m[16];
    quint32 v[16];

    for (qint32 i = 0; i < 16; i++) {
        m[i] = _load32le(pBlock + i * 4);
    }

    for (qint32 i = 0; i < 8; i++) {
        v[i] = pState->h[i];
    }

    v[8] = g_blake2s_IV[0];
    v[9] = g_blake2s_IV[1];
    v[10] = g_blake2s_IV[2];
    v[11] = g_blake2s_IV[3];
    v[12] = pState->t[0] ^ g_blake2s_IV[4];
    v[13] = pState->t[1] ^ g_blake2s_IV[5];
    v[14] = pState->f[0] ^ g_blake2s_IV[6];
    v[15] = pState->f[1] ^ g_blake2s_IV[7];

    BLAKE2S_ROUND(0);
    BLAKE2S_ROUND(1);
    BLAKE2S_ROUND(2);
    BLAKE2S_ROUND(3);
    BLAKE2S_ROUND(4);
    BLAKE2S_ROUND(5);
    BLAKE2S_ROUND(6);
    BLAKE2S_ROUND(7);
    BLAKE2S_ROUND(8);
    BLAKE2S_ROUND(9);

    for (qint32 i = 0; i < 8; i++) {
        pState->h[i] ^= v[i] ^ v[i + 8];
    }
}

void XBlake2sp::_blake2sInit(Blake2sState *pState, quint32 nOutLen, const quint8 *pParams)
{
    memset(pState, 0, sizeof(Blake2sState));

    for (qint32 i = 0; i < 8; i++) {
        pState->h[i] = g_blake2s_IV[i];
    }

    // XOR parameter block (8 x 4 bytes = 32 bytes) into IV
    for (qint32 i = 0; i < 8; i++) {
        pState->h[i] ^= _load32le(pParams + i * 4);
    }
}

void XBlake2sp::_blake2sUpdate(Blake2sState *pState, const quint8 *pData, quint32 nSize)
{
    while (nSize > 0) {
        quint32 nLeft = BLOCK_SIZE - pState->bufLen;

        if (nSize > nLeft) {
            // Fill the buffer and compress
            memcpy(pState->buf + pState->bufLen, pData, nLeft);
            pState->bufLen += nLeft;
            pData += nLeft;
            nSize -= nLeft;

            // Increment counter
            pState->t[0] += BLOCK_SIZE;
            if (pState->t[0] < (quint32)BLOCK_SIZE) {
                pState->t[1]++;
            }

            _blake2sCompress(pState, pState->buf, false);
            pState->bufLen = 0;
        } else {
            memcpy(pState->buf + pState->bufLen, pData, nSize);
            pState->bufLen += nSize;
            nSize = 0;
        }
    }
}

void XBlake2sp::_blake2sFinal(Blake2sState *pState, quint8 *pDigest, quint32 nOutLen)
{
    // Increment counter for remaining bytes
    pState->t[0] += pState->bufLen;
    if (pState->t[0] < pState->bufLen) {
        pState->t[1]++;
    }

    // Set last block flag
    pState->f[0] = 0xFFFFFFFFUL;

    // Pad remaining buffer with zeros
    if (pState->bufLen < (quint32)BLOCK_SIZE) {
        memset(pState->buf + pState->bufLen, 0, BLOCK_SIZE - pState->bufLen);
    }

    _blake2sCompress(pState, pState->buf, true);

    // Output digest
    quint8 buffer[32];
    for (qint32 i = 0; i < 8; i++) {
        _store32le(buffer + i * 4, pState->h[i]);
    }

    memcpy(pDigest, buffer, nOutLen);
}

XBlake2sp::XBlake2sp()
{
    m_nBufLen = 0;
}

void XBlake2sp::init()
{
    m_nBufLen = 0;

    // Initialize 8 leaf nodes for BLAKE2sp
    // Parameter block for BLAKE2sp leaf:
    //   byte 0: digest length = 32
    //   byte 1: key length = 0
    //   byte 2: fanout = 8
    //   byte 3: depth = 2
    //   bytes 4-7: leaf length = 0 (unlimited)
    //   bytes 8-13: node offset (leaf index, 6 bytes)
    //   byte 14: node depth = 0 (leaf)
    //   byte 15: inner length = 32
    //   bytes 16-31: salt + personalization = 0

    for (qint32 i = 0; i < PARALLEL_DEGREE; i++) {
        quint8 params[32];
        memset(params, 0, sizeof(params));

        params[0] = DIGEST_SIZE;      // digest length
        params[1] = 0;                // key length
        params[2] = PARALLEL_DEGREE;  // fanout
        params[3] = 2;                // depth

        // leaf length = 0 (bytes 4-7)
        // node offset = i (bytes 8-13, 6 bytes LE)
        params[8] = (quint8)i;
        // bytes 9-13 are already 0

        params[14] = 0;            // node depth = 0 (leaf level)
        params[15] = DIGEST_SIZE;  // inner length = 32

        _blake2sInit(&m_states[i], DIGEST_SIZE, params);
    }
}

void XBlake2sp::update(const quint8 *pData, qint64 nSize)
{
    // Buffer data and distribute full 512-byte blocks to the 8 leaf nodes
    // Each leaf gets 64 bytes per 512-byte block (round-robin distribution)

    while (nSize > 0) {
        qint64 nSpace = (qint64)(PARALLEL_DEGREE * BLOCK_SIZE) - m_nBufLen;

        if (nSize >= nSpace) {
            // Fill buffer to 512 bytes
            memcpy(m_buf + m_nBufLen, pData, (size_t)nSpace);
            pData += nSpace;
            nSize -= nSpace;

            // Distribute: leaf i gets bytes [i*64 .. (i+1)*64)
            for (qint32 i = 0; i < PARALLEL_DEGREE; i++) {
                _blake2sUpdate(&m_states[i], m_buf + i * BLOCK_SIZE, BLOCK_SIZE);
            }

            m_nBufLen = 0;
        } else {
            memcpy(m_buf + m_nBufLen, pData, (size_t)nSize);
            m_nBufLen += nSize;
            nSize = 0;
        }
    }
}

void XBlake2sp::final(quint8 *pDigest)
{
    // Distribute any remaining buffered data to the leaf nodes
    // The remaining data is distributed in 64-byte chunks to respective leaves
    qint64 nRemaining = m_nBufLen;
    qint32 nLeafIdx = 0;

    while (nRemaining > 0) {
        qint64 nChunk = qMin((qint64)BLOCK_SIZE, nRemaining);
        _blake2sUpdate(&m_states[nLeafIdx], m_buf + (m_nBufLen - nRemaining), (quint32)nChunk);
        nRemaining -= nChunk;
        nLeafIdx++;
    }

    // Finalize all 8 leaf nodes
    quint8 leafDigests[PARALLEL_DEGREE][DIGEST_SIZE];

    for (qint32 i = 0; i < PARALLEL_DEGREE; i++) {
        _blake2sFinal(&m_states[i], leafDigests[i], DIGEST_SIZE);
    }

    // Initialize root node
    // Root node parameter block:
    //   byte 0: digest length = 32
    //   byte 1: key length = 0
    //   byte 2: fanout = 8
    //   byte 3: depth = 2
    //   bytes 4-7: leaf length = 0
    //   bytes 8-13: node offset = 0
    //   byte 14: node depth = 1 (root level)
    //   byte 15: inner length = 32
    Blake2sState rootState;
    quint8 rootParams[32];
    memset(rootParams, 0, sizeof(rootParams));

    rootParams[0] = DIGEST_SIZE;      // digest length
    rootParams[1] = 0;                // key length
    rootParams[2] = PARALLEL_DEGREE;  // fanout
    rootParams[3] = 2;                // depth

    // leaf length = 0 (bytes 4-7)
    // node offset = 0 (bytes 8-13)

    rootParams[14] = 1;            // node depth = 1 (root)
    rootParams[15] = DIGEST_SIZE;  // inner length = 32

    _blake2sInit(&rootState, DIGEST_SIZE, rootParams);

    // Feed each leaf digest into the root
    for (qint32 i = 0; i < PARALLEL_DEGREE; i++) {
        if (i == PARALLEL_DEGREE - 1) {
            // Last leaf: set the last node flag before final compress
            rootState.f[1] = 0xFFFFFFFFUL;  // last node
        }
        _blake2sUpdate(&rootState, leafDigests[i], DIGEST_SIZE);
    }

    _blake2sFinal(&rootState, pDigest, DIGEST_SIZE);
}

QByteArray XBlake2sp::hash(QIODevice *pDevice)
{
    QByteArray baResult;

    if (pDevice && pDevice->isReadable()) {
        XBlake2sp blake;
        blake.init();

        qint64 nPos = pDevice->pos();
        pDevice->seek(0);

        const qint32 nBufSize = 4096;
        quint8 buf[4096];

        while (!pDevice->atEnd()) {
            qint64 nRead = pDevice->read((char *)buf, nBufSize);
            if (nRead > 0) {
                blake.update(buf, nRead);
            } else {
                break;
            }
        }

        pDevice->seek(nPos);

        quint8 digest[DIGEST_SIZE];
        blake.final(digest);

        baResult = QByteArray((const char *)digest, DIGEST_SIZE);
    }

    return baResult;
}

QByteArray XBlake2sp::hash(const QByteArray &baData)
{
    XBlake2sp blake;
    blake.init();
    blake.update((const quint8 *)baData.constData(), baData.size());

    quint8 digest[DIGEST_SIZE];
    blake.final(digest);

    return QByteArray((const char *)digest, DIGEST_SIZE);
}
