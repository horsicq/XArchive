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
#include "xtivolidecoder.h"

#include <string.h>

namespace {

const qint32 N_RING_SIZE = 0x1000;
const quint32 N_RING_MASK = 0x0fff;
const qint32 N_TAIL_SIZE = 8;
const qint32 N_MAX_BLOCK = 0x4000;
const qint32 N_MAX_BLOCKS = 0x100000;

// ---------------------------------------------------------------------------
// MD5.  Carried locally instead of leaning on QCryptographicHash because the
// format needs a SNAPSHOT of a still-running context once per block, and the
// "finalise a copy, keep hashing" behaviour of the Qt class is an
// implementation detail rather than a documented guarantee.
// ---------------------------------------------------------------------------
const quint32 g_nMd5K[64] = {0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501, 0x698098d8, 0x8b44f7af,
                             0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821, 0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
                             0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8, 0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8,
                             0x676f02d9, 0x8d2a4c8a, 0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
                             0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665, 0xf4292244, 0x432aff97,
                             0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1, 0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
                             0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};

const qint32 g_nMd5S[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                            4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};

struct MD5_CONTEXT {
    quint32 nState[4];
    quint64 nLength;
    quint8 nBuffer[64];
};

void md5Transform(quint32 *pnState, const quint8 *pBlock)
{
    quint32 nWords[16];
    for (qint32 i = 0; i < 16; ++i) {
        nWords[i] = (quint32)pBlock[i * 4] | ((quint32)pBlock[i * 4 + 1] << 8) | ((quint32)pBlock[i * 4 + 2] << 16) | ((quint32)pBlock[i * 4 + 3] << 24);
    }

    quint32 nA = pnState[0];
    quint32 nB = pnState[1];
    quint32 nC = pnState[2];
    quint32 nD = pnState[3];

    for (qint32 i = 0; i < 64; ++i) {
        quint32 nF = 0;
        qint32 nIndex = 0;
        if (i < 16) {
            nF = (nB & nC) | ((~nB) & nD);
            nIndex = i;
        } else if (i < 32) {
            nF = (nD & nB) | ((~nD) & nC);
            nIndex = (5 * i + 1) & 15;
        } else if (i < 48) {
            nF = nB ^ nC ^ nD;
            nIndex = (3 * i + 5) & 15;
        } else {
            nF = nC ^ (nB | (~nD));
            nIndex = (7 * i) & 15;
        }
        nF = (quint32)(nF + nA + g_nMd5K[i] + nWords[nIndex]);
        nA = nD;
        nD = nC;
        nC = nB;
        nB = (quint32)(nB + ((nF << g_nMd5S[i]) | (nF >> (32 - g_nMd5S[i]))));
    }

    pnState[0] = (quint32)(pnState[0] + nA);
    pnState[1] = (quint32)(pnState[1] + nB);
    pnState[2] = (quint32)(pnState[2] + nC);
    pnState[3] = (quint32)(pnState[3] + nD);
}

void md5Init(MD5_CONTEXT *pContext)
{
    pContext->nState[0] = 0x67452301;
    pContext->nState[1] = 0xefcdab89;
    pContext->nState[2] = 0x98badcfe;
    pContext->nState[3] = 0x10325476;
    pContext->nLength = 0;
    memset(pContext->nBuffer, 0, sizeof(pContext->nBuffer));
}

void md5Update(MD5_CONTEXT *pContext, const quint8 *pData, qint64 nSize)
{
    qint32 nFill = (qint32)(pContext->nLength & 63);
    for (qint64 i = 0; i < nSize; ++i) {
        pContext->nBuffer[nFill] = pData[i];
        ++nFill;
        if (nFill == 64) {
            md5Transform(pContext->nState, pContext->nBuffer);
            nFill = 0;
        }
    }
    pContext->nLength += (quint64)nSize;
}

// Snapshot: the caller's context keeps running, exactly like hashlib's
// md5.digest() on a context that is still being fed.
void md5Digest(const MD5_CONTEXT *pContext, quint8 *pDigest)
{
    MD5_CONTEXT context = *pContext;
    const quint64 nBits = context.nLength * 8;
    qint32 nFill = (qint32)(context.nLength & 63);

    context.nBuffer[nFill] = 0x80;
    ++nFill;
    if (nFill > 56) {
        while (nFill < 64) {
            context.nBuffer[nFill] = 0;
            ++nFill;
        }
        md5Transform(context.nState, context.nBuffer);
        nFill = 0;
    }
    while (nFill < 56) {
        context.nBuffer[nFill] = 0;
        ++nFill;
    }
    for (qint32 i = 0; i < 8; ++i) context.nBuffer[56 + i] = (quint8)((nBits >> (8 * i)) & 0xff);
    md5Transform(context.nState, context.nBuffer);

    for (qint32 i = 0; i < 4; ++i) {
        pDigest[i * 4 + 0] = (quint8)(context.nState[i] & 0xff);
        pDigest[i * 4 + 1] = (quint8)((context.nState[i] >> 8) & 0xff);
        pDigest[i * 4 + 2] = (quint8)((context.nState[i] >> 16) & 0xff);
        pDigest[i * 4 + 3] = (quint8)((context.nState[i] >> 24) & 0xff);
    }
}

}  // namespace

bool XTivoliDecoder::decodeNative(const QByteArray &baPacked, qint64 nOffset, qint64 nPackedSize, qint64 nMaxOutput, QByteArray *pbaResult, qint64 *pnNextOffset,
                                  XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nOffset < 0) || (nOffset > baPacked.size())) return false;
    if ((nPackedSize < 0) || (nMaxOutput < 0) || (nMaxOutput > 0x7fffffff)) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nSize = baPacked.size();
    qint64 nPosition = nOffset;
    qint64 nRemaining = nPackedSize;

    // Preset to 0x00. A distance may reach into the untouched part of the ring
    // on purpose, so the fill value is part of the format.
    QByteArray baWindow(N_RING_SIZE, (char)0);
    quint8 *pWindow = (quint8 *)baWindow.data();
    quint32 nWindowPosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)qMin<qint64>(nMaxOutput, 0x10000));

    quint32 nFlags = 0;
    qint32 nFlagCount = 0;
    qint32 nCheck = 0;

    // The budget is charged for the flag words too, and an item started with
    // one byte left is still decoded in full - which is exactly how the reader
    // can end up a few bytes past nOffset + nPackedSize.
    while (nRemaining > 0) {
        ++nCheck;
        if ((nCheck & 0xffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        if (nFlagCount == 0) {
            if ((nPosition + 2) > nSize) return false;
            nFlags = (quint32)pData[nPosition] | ((quint32)pData[nPosition + 1] << 8);
            nPosition += 2;
            nRemaining -= 2;
            nFlagCount = 16;
        }

        const quint32 nBit = nFlags & 1;
        nFlags >>= 1;
        --nFlagCount;

        if (nBit == 0) {
            if (nPosition >= nSize) return false;
            const quint8 nByte = pData[nPosition];
            ++nPosition;
            --nRemaining;
            if ((qint64)baOut.size() >= nMaxOutput) return false;
            pWindow[nWindowPosition] = nByte;
            baOut.append((char)nByte);
            nWindowPosition = (nWindowPosition + 1) & N_RING_MASK;
        } else {
            if ((nPosition + 2) > nSize) return false;
            const quint32 nB0 = pData[nPosition];
            const quint32 nB1 = pData[nPosition + 1];
            nPosition += 2;
            nRemaining -= 2;

            const qint32 nLength = (qint32)(nB0 & 0x0f) + 1;
            const quint32 nDistance = ((nB0 >> 4) << 8) | nB1;
            // No -1 bias: the source is (position - distance), not
            // (position - distance - 1).
            quint32 nSource = (nWindowPosition - nDistance) & N_RING_MASK;

            for (qint32 i = 0; i < nLength; ++i) {
                if ((qint64)baOut.size() >= nMaxOutput) return false;
                const quint8 nByte = pWindow[nSource];
                pWindow[nWindowPosition] = nByte;
                baOut.append((char)nByte);
                nSource = (nSource + 1) & N_RING_MASK;
                nWindowPosition = (nWindowPosition + 1) & N_RING_MASK;
            }
        }
    }

    *pbaResult = baOut;
    if (pnNextOffset) *pnNextOffset = nPosition;

    return true;
}

bool XTivoliDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    qint64 nNextOffset = 0;
    if (!decodeNative(baPacked, 0, baPacked.size(), nUncompressedSize, pbaResult, &nNextOffset, pPdStruct)) {
        pbaResult->clear();
        return false;
    }

    return (qint64)pbaResult->size() == nUncompressedSize;
}

bool XTivoliDecoder::unwrapBlocks(const QByteArray &baFile, qint64 nOffset, QByteArray *pbaResult, bool bVerifyDigest, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nOffset < 0) || (nOffset > baFile.size())) return false;

    const quint8 *pData = (const quint8 *)baFile.constData();
    const qint64 nSize = baFile.size();
    qint64 nPosition = nOffset;

    MD5_CONTEXT context;
    md5Init(&context);

    QByteArray baOut;
    QByteArray baBlock;
    qint32 nBlocks = 0;

    while (nPosition < nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        // A trailing odd byte is a clean end of chain, not a failure.
        if ((nPosition + 2) > nSize) break;

        const qint32 nHeader = (qint32)(((quint32)pData[nPosition] << 8) | pData[nPosition + 1]);
        nPosition += 2;

        if ((nHeader & 0x8000) == 0) {
            if (nHeader == 0) break;
            qint64 nNextOffset = 0;
            if (!decodeNative(baFile, nPosition, nHeader, N_MAX_BLOCK, &baBlock, &nNextOffset, pPdStruct)) return false;
            if (baBlock.size() < N_TAIL_SIZE) return false;
            nPosition = nNextOffset;
        } else {
            const qint32 nLength = nHeader & 0x7fff;
            // A stored block must have at least one payload byte on top of the
            // 8 digest bytes.
            if ((nLength < (N_TAIL_SIZE + 1)) || (nLength > N_MAX_BLOCK)) return false;
            if ((nPosition + nLength) > nSize) return false;
            baBlock = baFile.mid((qint32)nPosition, nLength);
            nPosition += nLength;
        }

        const qint32 nBodySize = baBlock.size() - N_TAIL_SIZE;
        md5Update(&context, (const quint8 *)baBlock.constData(), nBodySize);

        if (bVerifyDigest) {
            quint8 nDigest[16];
            md5Digest(&context, nDigest);
            if (memcmp(nDigest, baBlock.constData() + nBodySize, N_TAIL_SIZE) != 0) return false;
        }

        baOut.append(baBlock.constData(), nBodySize);

        ++nBlocks;
        if (nBlocks > N_MAX_BLOCKS) return false;
    }

    *pbaResult = baOut;

    return true;
}

bool XTivoliDecoder::checkHeader(const QByteArray &baFile)
{
    if (baFile.size() < (qint64)HEADER_SIZE) return false;
    if (baFile.mid(0, 4) != QByteArray("    ", 4)) return false;
    if (baFile.mid(4, 4) != QByteArray("79 T", 4)) return false;
    if (baFile.mid(8, 4) != QByteArray("FPB-", 4)) return false;
    if (baFile.mid(0x3c, 4) != QByteArray("d5 c", 4)) return false;
    if (baFile.mid(0x4c, 2) != QByteArray("ve", 2)) return false;
    if (baFile.at(0x4e) != (char)0x0a) return false;

    return true;
}

bool XTivoliDecoder::unwrapFile(const QByteArray &baFile, QByteArray *pbaInner, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaInner) return false;
    pbaInner->clear();
    if (!checkHeader(baFile)) return false;

    return unwrapBlocks(baFile, (qint64)HEADER_SIZE, pbaInner, true, pPdStruct);
}

QByteArray XTivoliDecoder::memberProperties(qint64 nStreamOffset, qint64 nSize)
{
    QByteArray baResult((qint32)MEMBER_PROPERTIES_SIZE, (char)0);
    if ((nStreamOffset < 0) || (nSize < 0)) return baResult;

    quint8 *pData = (quint8 *)baResult.data();
    const quint32 nOffset32 = (quint32)nStreamOffset;
    const quint32 nSize32 = (quint32)nSize;
    pData[0] = (quint8)(nOffset32 & 0xff);
    pData[1] = (quint8)((nOffset32 >> 8) & 0xff);
    pData[2] = (quint8)((nOffset32 >> 16) & 0xff);
    pData[3] = (quint8)((nOffset32 >> 24) & 0xff);
    pData[4] = (quint8)(nSize32 & 0xff);
    pData[5] = (quint8)((nSize32 >> 8) & 0xff);
    pData[6] = (quint8)((nSize32 >> 16) & 0xff);
    pData[7] = (quint8)((nSize32 >> 24) & 0xff);

    return baResult;
}

bool XTivoliDecoder::parseMemberProperties(const QByteArray &baProperties, qint64 *pnStreamOffset, qint64 *pnSize)
{
    if (!pnStreamOffset || !pnSize || (baProperties.size() != (qint32)MEMBER_PROPERTIES_SIZE)) return false;

    const quint8 *pData = (const quint8 *)baProperties.constData();
    *pnStreamOffset = (qint64)((quint32)pData[0] | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16) | ((quint32)pData[3] << 24));
    *pnSize = (qint64)((quint32)pData[4] | ((quint32)pData[5] << 8) | ((quint32)pData[6] << 16) | ((quint32)pData[7] << 24));

    return true;
}

bool XTivoliDecoder::decodeMember(const QByteArray &baPacked, qint64 nUncompressedSize, const QByteArray &baProperty, QByteArray *pbaResult,
                                  XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    QByteArray baInner;
    if (!unwrapFile(baPacked, &baInner, pPdStruct)) return false;

    // An empty blob means the record is the whole unwrapped stream, which is
    // what a container that turned out not to be cpio publishes.
    if (baProperty.isEmpty()) {
        if ((qint64)baInner.size() != nUncompressedSize) return false;
        *pbaResult = baInner;
        return true;
    }

    qint64 nStreamOffset = 0;
    qint64 nSize = 0;
    if (!parseMemberProperties(baProperty, &nStreamOffset, &nSize)) return false;
    if (nSize != nUncompressedSize) return false;
    if ((nStreamOffset < 0) || (nSize < 0) || (nStreamOffset > baInner.size()) || (nSize > (baInner.size() - nStreamOffset))) return false;

    *pbaResult = baInner.mid((qint32)nStreamOffset, (qint32)nSize);

    return (qint64)pbaResult->size() == nUncompressedSize;
}
