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
#include "xchieflzdecoder.h"

namespace {
const qint32 CHIEFLZ_SYMBOLS = 629;
const qint32 CHIEFLZ_NODES = CHIEFLZ_SYMBOLS * 2 - 1;   // 1257
const qint32 CHIEFLZ_ROOT = 1;
const qint32 CHIEFLZ_END_SYMBOL = 0x100;
const qint32 CHIEFLZ_FIRST_MATCH = 0x101;
const qint32 CHIEFLZ_LENGTH_SPAN = 62;
const qint32 CHIEFLZ_MIN_LENGTH = 3;
const qint32 CHIEFLZ_BUCKETS = 6;
const quint16 CHIEFLZ_MAX_ROOT_FREQ = 2000U;
const qint32 CHIEFLZ_WINDOW = 0x8000;

const qint32 g_arrExtraBits[CHIEFLZ_BUCKETS] = {4, 6, 8, 10, 12, 14};
const qint32 g_arrBaseDistance[CHIEFLZ_BUCKETS] = {0, 16, 80, 336, 1360, 5456};

// MSB-first out of a little-endian 16-bit word.
class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nWord(0), m_nCount(0)
    {
    }

    qint32 readBit()
    {
        if (m_nCount == 0) {
            if ((m_nPosition + 2) > m_nSize) return -1;
            m_nWord = (quint16)((quint16)m_pData[m_nPosition] | ((quint16)m_pData[m_nPosition + 1] << 8));
            m_nPosition += 2;
            m_nCount = 16;
        }
        const qint32 nBit = (qint32)((m_nWord >> 15) & 1U);
        m_nWord = (quint16)(m_nWord << 1);
        --m_nCount;
        return nBit;
    }

    // The value is assembled LSB-first from bits that arrive MSB-first.
    qint32 readBits(qint32 nBits)
    {
        qint32 nValue = 0;
        for (qint32 i = 0; i < nBits; ++i) {
            const qint32 nBit = readBit();
            if (nBit < 0) return -1;
            if (nBit) nValue |= (1 << i);
        }
        return nValue;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint16 m_nWord;
    qint32 m_nCount;
};

struct MODEL {
    quint16 freq[CHIEFLZ_NODES + 1];
    quint16 parent[CHIEFLZ_NODES + 1];
    quint16 child0[CHIEFLZ_NODES + 1];
    quint16 child1[CHIEFLZ_NODES + 1];
};

void modelInit(MODEL *pModel)
{
    memset(pModel, 0, sizeof(MODEL));
    for (qint32 i = 2; i <= CHIEFLZ_NODES; ++i) {
        pModel->freq[i] = 1;
        pModel->parent[i] = (quint16)(i >> 1);
    }
    for (qint32 i = 1; i < CHIEFLZ_SYMBOLS; ++i) {
        pModel->child0[i] = (quint16)(i * 2);
        pModel->child1[i] = (quint16)(i * 2 + 1);
    }
}

// Re-sum weights from `nNode` up to the root, then halve everything if the
// root has just reached the ceiling.
void modelPropagate(MODEL *pModel, qint32 nNode, qint32 nSibling)
{
    while (true) {
        const qint32 nChild = nNode;
        nNode = pModel->parent[nNode];
        pModel->freq[nNode] = (quint16)(pModel->freq[nChild] + pModel->freq[nSibling]);
        if (nNode == CHIEFLZ_ROOT) break;
        const qint32 nGrand = pModel->parent[nNode];
        nSibling = pModel->child0[nGrand];
        if (nSibling == nNode) nSibling = pModel->child1[nGrand];
    }
    if (pModel->freq[CHIEFLZ_ROOT] == CHIEFLZ_MAX_ROOT_FREQ) {
        for (qint32 i = 1; i <= CHIEFLZ_NODES; ++i) pModel->freq[i] = (quint16)(pModel->freq[i] >> 1);
    }
}

void modelUpdate(MODEL *pModel, qint32 nLeaf)
{
    ++pModel->freq[nLeaf];
    qint32 nParent = pModel->parent[nLeaf];
    if (nParent == CHIEFLZ_ROOT) return;

    qint32 nSibling = pModel->child0[nParent];
    if (nLeaf == nSibling) nSibling = pModel->child1[nParent];
    modelPropagate(pModel, nLeaf, nSibling);

    qint32 nNode = nLeaf;
    while (nParent != CHIEFLZ_ROOT) {
        const qint32 nGrand = pModel->parent[nParent];
        const qint32 nLeft = pModel->child0[nGrand];
        qint32 nUncle = nLeft;
        if (nParent == nUncle) nUncle = pModel->child1[nGrand];

        if (pModel->freq[nUncle] < pModel->freq[nNode]) {
            if (nParent == nLeft) {
                pModel->child1[nGrand] = (quint16)nNode;
            } else {
                pModel->child0[nGrand] = (quint16)nNode;
            }
            qint32 nOwn = pModel->child0[nParent];
            if (nNode == nOwn) {
                nOwn = pModel->child1[nParent];
                pModel->child0[nParent] = (quint16)nUncle;
            } else {
                pModel->child1[nParent] = (quint16)nUncle;
            }
            pModel->parent[nUncle] = (quint16)nParent;
            pModel->parent[nNode] = (quint16)nGrand;
            modelPropagate(pModel, nUncle, nOwn);
            nNode = nUncle;
        }

        nNode = pModel->parent[nNode];
        nParent = pModel->parent[nNode];
    }
}

qint32 decodeSymbol(MODEL *pModel, BitReader *pReader)
{
    qint32 nNode = CHIEFLZ_ROOT;
    while (nNode < CHIEFLZ_SYMBOLS) {
        const qint32 nBit = pReader->readBit();
        if (nBit < 0) return -1;
        nNode = nBit ? pModel->child1[nNode] : pModel->child0[nNode];
        if ((nNode <= 0) || (nNode > CHIEFLZ_NODES)) return -1;
    }
    modelUpdate(pModel, nNode);
    return nNode - CHIEFLZ_SYMBOLS;
}
}  // namespace

bool XChiefLZDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    MODEL model;
    modelInit(&model);
    BitReader reader((const quint8 *)baPacked.constData(), baPacked.size());

    QByteArray baWindow(CHIEFLZ_WINDOW, (char)0);
    quint8 *pWindow = (quint8 *)baWindow.data();
    qint32 nPosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    while (baOut.size() < nUncompressedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint32 nSymbol = decodeSymbol(&model, &reader);
        if (nSymbol < 0) break;
        if (nSymbol == CHIEFLZ_END_SYMBOL) break;

        if (nSymbol < CHIEFLZ_END_SYMBOL) {
            pWindow[nPosition] = (quint8)nSymbol;
            baOut.append((char)(quint8)nSymbol);
            nPosition = (nPosition + 1) & (CHIEFLZ_WINDOW - 1);
            continue;
        }

        const qint32 nCode = nSymbol - CHIEFLZ_FIRST_MATCH;
        const qint32 nLength = (nCode % CHIEFLZ_LENGTH_SPAN) + CHIEFLZ_MIN_LENGTH;
        const qint32 nBucket = nCode / CHIEFLZ_LENGTH_SPAN;
        if (nBucket >= CHIEFLZ_BUCKETS) return false;

        const qint32 nExtra = reader.readBits(g_arrExtraBits[nBucket]);
        if (nExtra < 0) break;

        const qint32 nDistance = g_arrBaseDistance[nBucket] + nExtra + nLength;
        qint32 nSource = (nPosition - nDistance) & (CHIEFLZ_WINDOW - 1);
        for (qint32 i = 0; i < nLength; ++i) {
            const quint8 nByte = pWindow[nSource];
            pWindow[nPosition] = nByte;
            baOut.append((char)nByte);
            nPosition = (nPosition + 1) & (CHIEFLZ_WINDOW - 1);
            nSource = (nSource + 1) & (CHIEFLZ_WINDOW - 1);
        }
    }

    if (baOut.size() > nUncompressedSize) baOut.truncate((qint32)nUncompressedSize);
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
