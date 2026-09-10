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
#include "xtarx1decoder.h"

#include <QtEndian>

#include <limits>

namespace {
const qint32 TARX_HEADER_SIZE = 100;
const qint32 TARX_CHECK_OFFSET = 0x5f;
const qint32 TARX_CHECK_BYTE = 99;
const quint32 TARX_CRC32_POLYNOMIAL = 0xedb88320;

struct TARX_TABLES {
    quint32 nTable[256];
    quint8 nInverse[256];
};

void tarxBuildTables(TARX_TABLES *pTables)
{
    for (qint32 i = 0; i < 256; ++i) {
        quint32 nValue = (quint32)i;
        for (qint32 j = 0; j < 8; ++j) {
            nValue = (nValue >> 1) ^ ((nValue & 1) ? TARX_CRC32_POLYNOMIAL : 0);
        }
        pTables->nTable[i] = nValue;
    }
    for (qint32 i = 0; i < 256; ++i) pTables->nInverse[i] = 0;
    // The top bytes of the reflected CRC-32 table are a permutation of 0..255,
    // so this really is an inverse and not a lossy fold.
    for (qint32 i = 0; i < 256; ++i) pTables->nInverse[pTables->nTable[i] >> 24] = (quint8)i;
}

// The carry mixer the key search unwinds through: a bitwise ripple that mixes
// the candidate key byte, the recovered table index and the known plaintext
// byte.  It is transliterated rather than simplified because the i == 8 pass
// deliberately masks the carry with zero, which any "obvious" rewrite loses.
quint8 tarxMix(quint32 nKey, quint32 nIndex, quint32 nPlain)
{
    quint32 nX = (nKey ^ nPlain ^ nIndex) & 0xff;
    quint32 nCarry = 0;
    for (qint32 i = 1; i <= 8; ++i) {
        nCarry = nX ^ nCarry;
        nX = nCarry;
        const quint32 nM = ((quint32)1 << i) & 0xff;
        const quint32 nMask = (nM - 1) & 0xff;
        nCarry = (nCarry + (nKey & nMask) + ((nPlain & nMask) ^ nCarry)) & nM;
    }

    return (quint8)nX;
}
}  // namespace

bool XTARX1Decoder::recoverKey(const QByteArray &baHeader, quint32 *pnKey)
{
    if (!pnKey || (baHeader.size() < TARX_HEADER_SIZE)) return false;

    TARX_TABLES tables;
    tarxBuildTables(&tables);

    const quint8 *pHeader = (const quint8 *)baHeader.constData();
    const quint32 nAnchor = qFromLittleEndian<quint32>(pHeader + TARX_CHECK_OFFSET);

    quint32 nFound = 0;
    qint32 nMatches = 0;

    for (qint32 nCandidate = 0; nCandidate < 256; ++nCandidate) {
        const quint32 nKey = (quint32)nCandidate;
        quint32 nT[4];
        for (qint32 i = 0; i < 4; ++i) {
            const quint32 nByte = (nAnchor >> (i * 8)) & 0xff;
            nT[i] = tables.nTable[((nByte + nKey) ^ nByte) & 0xff];
        }
        const quint32 nCheck = ((nT[0] >> 24) ^ (nT[1] >> 16) ^ (nT[2] >> 8) ^ nT[3]) & 0xff;
        if (nCheck != pHeader[TARX_CHECK_BYTE]) continue;

        quint32 nState = (nT[0] << 8) ^ (nT[1] << 16) ^ (nT[2] << 24) ^ nAnchor;
        for (qint32 i = TARX_CHECK_OFFSET - 1; i >= 0; --i) {
            const quint8 nIndex = tables.nInverse[nState >> 24];
            nState = ((nState ^ tables.nTable[nIndex]) << 8) + tarxMix(nKey, nIndex, pHeader[i]);
        }
        if ((nState & 0xff) == nKey) {
            nFound = nState;
            ++nMatches;
        }
    }

    if (nMatches != 1) return false;

    *pnKey = nFound;

    return true;
}

bool XTARX1Decoder::decrypt(const QByteArray &baCipher, quint32 nKey, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baCipher.isEmpty()) return false;

    TARX_TABLES tables;
    tarxBuildTables(&tables);

    QByteArray baOut(baCipher.size(), (char)0);
    if (baOut.size() != baCipher.size()) return false;

    const quint8 *pCipher = (const quint8 *)baCipher.constData();
    quint8 *pOut = (quint8 *)baOut.data();
    const qint64 nSize = baCipher.size();
    quint32 nState = nKey;

    for (qint64 i = 0; i < nSize; ++i) {
        if ((i & 0xfffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }
        const quint32 nPlain = (quint32)(pCipher[i] ^ (quint8)(nState & 0xff));
        pOut[i] = (quint8)nPlain;
        nState = (nState >> 8) ^ tables.nTable[((nState + nKey + nPlain) ^ nState) & 0xff];
    }

    *pbaResult = baOut;

    return true;
}

bool XTARX1Decoder::decode(const QByteArray &baPacked, qint64 nSkipSize, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nSkipSize < 0) || (nUncompressedSize < 0) || (nUncompressedSize > (qint64)(std::numeric_limits<qint32>::max)())) return false;
    if (baPacked.size() < TARX_HEADER_SIZE) return false;
    if (nSkipSize > ((qint64)baPacked.size() - nUncompressedSize)) return false;

    quint32 nKey = 0;
    if (!recoverKey(baPacked.left(TARX_HEADER_SIZE), &nKey)) return false;

    TARX_TABLES tables;
    tarxBuildTables(&tables);

    QByteArray baOut((qint32)nUncompressedSize, (char)0);
    if (baOut.size() != (qint32)nUncompressedSize) return false;

    const quint8 *pCipher = (const quint8 *)baPacked.constData();
    quint8 *pOut = (quint8 *)baOut.data();
    // The cipher state depends on the plaintext, so the discarded prefix has to
    // be run through it as well; only the storing stops early.
    const qint64 nEnd = nSkipSize + nUncompressedSize;
    quint32 nState = nKey;

    for (qint64 i = 0; i < nEnd; ++i) {
        if ((i & 0xfffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }
        const quint32 nPlain = (quint32)(pCipher[i] ^ (quint8)(nState & 0xff));
        if (i >= nSkipSize) pOut[i - nSkipSize] = (quint8)nPlain;
        nState = (nState >> 8) ^ tables.nTable[((nState + nKey + nPlain) ^ nState) & 0xff];
    }

    *pbaResult = baOut;

    return true;
}
