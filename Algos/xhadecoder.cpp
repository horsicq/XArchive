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
#include "xhadecoder.h"

namespace {
const qint32 HA_CONTEXTS = 10000;
const qint32 HA_POOL = 0x7ff8;
const qint32 HA_HASH_SIZE = 0x4000;
const qint32 HA_NIL = 0xffff;
const qint32 HA_ESCAPE = 0x100;
const qint32 HA_MAX_TOTAL = 7999;
const qint32 HA_ORDER4_BUDGET = 0x9c4;

// Witten-Neal-Cleary decoder, 16-bit registers.
class Arith {
public:
    Arith(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nHigh(0xffff), m_nLow(0), m_nBuffer(0)
    {
        m_nCode = (quint16)((nextByte() << 8) | nextByte());
    }

    quint32 target(quint32 nTotal) const
    {
        if (nTotal == 0) return 0;
        const quint32 nRange = (quint32)(m_nHigh - m_nLow) + 1U;
        return (quint32)((((quint64)((quint16)(m_nCode - m_nLow) + 1U) * nTotal) - 1U) / nRange);
    }

    void update(quint32 nLow, quint32 nHigh, quint32 nTotal)
    {
        if (nTotal == 0) return;
        const quint32 nRange = (quint32)(m_nHigh - m_nLow) + 1U;
        m_nHigh = (quint16)(((nRange * nHigh) / nTotal) + m_nLow - 1U);
        m_nLow = (quint16)(m_nLow + ((nRange * nLow) / nTotal));
        while (((m_nHigh ^ m_nLow) & 0x8000U) == 0) {
            m_nLow = (quint16)(m_nLow << 1);
            m_nHigh = (quint16)((m_nHigh << 1) | 1U);
            m_nCode = (quint16)(m_nCode << 1);
            shiftIn();
        }
        while ((m_nLow & 0x4000U) && !(m_nHigh & 0x4000U)) {
            m_nLow = (quint16)((m_nLow & 0x3fffU) << 1);
            m_nHigh = (quint16)((m_nHigh << 1) | 0x8001U);
            m_nCode = (quint16)((m_nCode << 1) ^ 0x8000U);
            shiftIn();
        }
    }

private:
    quint32 nextByte()
    {
        if (m_nPosition >= m_nSize) return 0;
        return m_pData[m_nPosition++];
    }

    void shiftIn()
    {
        m_nBuffer = (quint16)(m_nBuffer << 1);
        if ((m_nBuffer & 0xffU) == 0) {
            if (m_nPosition >= m_nSize) {
                m_nBuffer = 0x100;
            } else {
                m_nBuffer = (quint16)((m_pData[m_nPosition] * 2U) | 1U);
                ++m_nPosition;
            }
        }
        m_nCode = (quint16)(m_nCode | ((m_nBuffer >> 8) & 1U));
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint16 m_nHigh;
    quint16 m_nLow;
    quint16 m_nCode;
    quint16 m_nBuffer;
};

class Model {
public:
    Model(const quint8 *pData, qint64 nSize) : m_arith(pData, nSize)
    {
        m_vecOrder.fill(0xff, HA_CONTEXTS);
        m_vecLowCount.fill(0, HA_CONTEXTS);
        m_vecNovel.fill(0, HA_CONTEXTS);
        m_vecTotal.fill(0, HA_CONTEXTS);
        m_vecRescale.fill(0, HA_CONTEXTS);
        m_vecContextBytes.fill(0, HA_CONTEXTS * 4);
        m_vecHashNext.fill((quint16)HA_NIL, HA_CONTEXTS);
        m_vecLruPrev.fill(0, HA_CONTEXTS);
        m_vecLruNext.fill(0, HA_CONTEXTS);
        m_vecFreq.fill(0, HA_POOL);
        m_vecSymbol.fill(0, HA_POOL);
        m_vecSymbolNext.fill((quint16)HA_NIL, HA_POOL);
        m_vecBuckets.fill((quint16)HA_NIL, HA_HASH_SIZE);

        m_nMaxOrder = 4;
        for (qint32 i = 0; i < 5; ++i) m_arrIncrement[i] = (i == 0) ? 0x10 : 0x0f;
        m_nOrder4Budget = HA_ORDER4_BUDGET;
        m_nRunCount = 0;
        m_nPoolScan = 0;

        for (qint32 i = 0; i < HA_CONTEXTS; ++i) {
            m_vecLruNext[i] = (quint16)(i + 1);
            m_vecLruPrev[i] = (quint16)(i - 1);
        }
        m_nLruHead = 0;
        m_nLruTail = HA_CONTEXTS - 1;
        for (qint32 i = HA_CONTEXTS; i < (HA_POOL - 1); ++i) m_vecSymbolNext[i] = (quint16)(i + 1);
        m_vecSymbolNext[HA_POOL - 1] = (quint16)HA_NIL;
        m_nFreeSymbol = HA_CONTEXTS;

        for (qint32 i = 0; i < 4; ++i) m_arrContext[i] = 0;
        memset(m_arrExcluded, 0, sizeof(m_arrExcluded));
        m_nExcludedCount = 0;
        m_nEscapeDepth = 0;
        m_nScanOrder = 0;
        for (qint32 i = 0; i < 6; ++i) m_arrContextHash[i] = 0;

        // MINSTD (Schrage) seeded at 10 - the mixing table is part of the format
        m_vecHash.resize(HA_HASH_SIZE);
        qint32 x = 10;
        for (qint32 i = 0; i < HA_HASH_SIZE; ++i) {
            x = (x % 127773) * 16807 + (x / 127773) * -2836;
            if (x < 1) x += 0x7fffffff;
            m_vecHash[i] = (quint16)(x & 0x3fff);
        }
    }

    bool run(qint64 nLimit, QByteArray *pbaOut, XBinary::PDSTRUCT *pPdStruct);

private:
    qint32 hashOf(const quint8 *pBytes, qint32 nOrder) const
    {
        qint32 h = 0;
        if (nOrder > 0) h = m_vecHash[pBytes[0]];
        if (nOrder > 1) h = m_vecHash[(pBytes[1] + h) & 0x3fff];
        if (nOrder > 2) h = m_vecHash[(pBytes[2] + h) & 0x3fff];
        if (nOrder > 3) h = m_vecHash[(pBytes[3] + h) & 0x3fff];
        return h;
    }

    qint32 findDeepest();
    qint32 findNext();
    qint32 escapeFrequency(qint32 nLowCount, qint32 nContext) const;
    qint32 decodeNoExclusion(qint32 nContext);
    qint32 decodeWithExclusion(qint32 nContext);
    qint32 decodeFallback();
    void touch(qint32 nContext);
    void reclaimSymbols();
    void updateModel(qint32 nSymbol);
    void addContext(qint32 nOrder, qint32 nSymbol);

    Arith m_arith;
    QVector<quint8> m_vecOrder;
    QVector<quint8> m_vecLowCount;
    QVector<quint8> m_vecNovel;
    QVector<quint16> m_vecTotal;
    QVector<quint8> m_vecRescale;
    QVector<quint8> m_vecContextBytes;
    QVector<quint16> m_vecHashNext;
    QVector<quint16> m_vecLruPrev;
    QVector<quint16> m_vecLruNext;
    QVector<quint16> m_vecFreq;
    QVector<quint8> m_vecSymbol;
    QVector<quint16> m_vecSymbolNext;
    QVector<quint16> m_vecBuckets;
    QVector<quint16> m_vecHash;

    qint32 m_nMaxOrder;
    quint8 m_arrIncrement[5];
    qint32 m_nOrder4Budget;
    qint32 m_nRunCount;
    qint32 m_nPoolScan;
    qint32 m_nLruHead;
    qint32 m_nLruTail;
    qint32 m_nFreeSymbol;
    quint8 m_arrContext[4];
    quint8 m_arrExcluded[0x100];
    quint8 m_arrExcludedList[0x100];
    qint32 m_nExcludedCount;
    qint32 m_arrEscapeContext[8];
    qint32 m_arrEscapeNode[8];
    qint32 m_nEscapeDepth;
    qint32 m_arrContextHash[6];
    qint32 m_nScanOrder;
};

qint32 Model::findDeepest()
{
    m_arrContextHash[0] = 0;
    m_arrContextHash[1] = m_vecHash[m_arrContext[0]];
    m_arrContextHash[2] = m_vecHash[(m_arrContext[1] + m_arrContextHash[1]) & 0x3fff];
    m_arrContextHash[3] = m_vecHash[(m_arrContext[2] + m_arrContextHash[2]) & 0x3fff];
    m_arrContextHash[4] = m_vecHash[(m_arrContext[3] + m_arrContextHash[3]) & 0x3fff];
    m_nEscapeDepth = 0;
    while (m_nExcludedCount) {
        --m_nExcludedCount;
        m_arrExcluded[m_arrExcludedList[m_nExcludedCount]] = 0;
    }
    m_nScanOrder = 5;
    return findNext();
}

qint32 Model::findNext()
{
    for (qint32 nOrder = m_nScanOrder - 1; nOrder >= 0; --nOrder) {
        qint32 n = m_vecBuckets[m_arrContextHash[nOrder]];
        while (n != HA_NIL) {
            if (m_vecOrder[n] == nOrder) {
                if (nOrder == 0) {
                    m_nScanOrder = nOrder;
                    return n;
                }
                const quint8 *p = m_vecContextBytes.constData() + (n * 4);
                bool bMatch = false;
                if (nOrder == 1) {
                    bMatch = (m_arrContext[0] == p[0]);
                } else if (nOrder == 2) {
                    bMatch = (m_arrContext[1] == p[1]) && (m_arrContext[0] == p[0]);
                } else if (nOrder == 3) {
                    bMatch = (m_arrContext[2] == p[2]) && (m_arrContext[1] == p[1]) && (m_arrContext[0] == p[0]);
                } else if (nOrder == 4) {
                    bMatch = (m_arrContext[3] == p[3]) && (m_arrContext[2] == p[2]) && (m_arrContext[1] == p[1]) && (m_arrContext[0] == p[0]);
                }
                if (bMatch) {
                    m_nScanOrder = nOrder;
                    return n;
                }
            }
            n = m_vecHashNext[n];
        }
    }
    return HA_NIL;
}

qint32 Model::escapeFrequency(qint32 nLowCount, qint32 nContext) const
{
    if (m_vecTotal[nContext] == 1) return (m_arrIncrement[m_vecOrder[nContext]] < 0x10) ? 1 : 2;
    const qint32 nNovel = m_vecNovel[nContext];
    if (nNovel == 0xff) return 1;
    qint32 nValue = nLowCount;
    if ((nNovel != 0) && (m_vecTotal[nContext] <= ((nNovel + 1) * 2))) {
        nValue = (qint32)(((quint32)nLowCount * (quint32)((nNovel + 1) * 2)) / (quint32)m_vecTotal[nContext]);
        if ((nNovel + 1) == m_vecTotal[nContext]) nValue += ((nNovel + 1) >> 1);
    }
    if (nValue == 0) nValue = 1;
    return nValue;
}

qint32 Model::decodeNoExclusion(qint32 nContext)
{
    const qint32 nEscape = escapeFrequency(m_vecLowCount[nContext], nContext);
    qint32 nTotal = m_vecTotal[nContext];
    qint32 nShift = 0;
    if (m_nRunCount >= 5) nShift = ((nTotal < 5) && (m_nRunCount == 10)) ? 2 : 1;
    nTotal = nTotal << nShift;

    const quint32 nTarget = m_arith.target((quint32)(nTotal + nEscape));
    qint32 nAccumulated = 0;
    qint32 nHit = 0;
    qint32 n = nContext;
    while (n != HA_NIL) {
        if ((qint32)(nTarget >> nShift) < (nAccumulated + m_vecFreq[n])) {
            nHit = m_vecFreq[n] << nShift;
            break;
        }
        nAccumulated += m_vecFreq[n];
        n = m_vecSymbolNext[n];
    }
    nAccumulated = nAccumulated << nShift;

    m_nEscapeDepth = 1;
    if (n == HA_NIL) {
        m_arith.update((quint32)nTotal, (quint32)(nTotal + nEscape), (quint32)(nTotal + nEscape));
        if ((m_vecTotal[nContext] == 1) && (m_arrIncrement[m_vecOrder[nContext]] < 0x20)) ++m_arrIncrement[m_vecOrder[nContext]];
        qint32 nLast = nContext;
        for (qint32 m = nContext; m != HA_NIL; m = m_vecSymbolNext[m]) {
            nLast = m;
            m_arrExcludedList[m_nExcludedCount] = m_vecSymbol[m];
            ++m_nExcludedCount;
            m_arrExcluded[m_vecSymbol[m]] = 1;
        }
        m_arrEscapeContext[0] = nContext | 0x8000;
        m_arrEscapeNode[0] = nLast;
        m_nRunCount = 0;
        return HA_ESCAPE;
    }
    m_arith.update((quint32)nAccumulated, (quint32)(nAccumulated + nHit), (quint32)(nTotal + nEscape));
    if ((m_vecTotal[nContext] == 1) && (m_arrIncrement[m_vecOrder[nContext]] != 0)) --m_arrIncrement[m_vecOrder[nContext]];
    m_arrEscapeContext[0] = nContext;
    m_arrEscapeNode[0] = n;
    if (m_nRunCount < 10) ++m_nRunCount;
    return m_vecSymbol[n];
}

qint32 Model::decodeWithExclusion(qint32 nContext)
{
    qint32 nTotal = 0;
    qint32 nLowCount = 0;
    for (qint32 n = nContext; n != HA_NIL; n = m_vecSymbolNext[n]) {
        if (m_arrExcluded[m_vecSymbol[n]] == 0) {
            nTotal += m_vecFreq[n];
            if (m_vecFreq[n] < 3) ++nLowCount;
        }
    }
    const qint32 nEscape = escapeFrequency(nLowCount, nContext);
    const quint32 nTarget = m_arith.target((quint32)(nTotal + nEscape));

    qint32 nAccumulated = 0;
    qint32 nHit = 0;
    qint32 nFound = HA_NIL;
    for (qint32 n = nContext; n != HA_NIL; n = m_vecSymbolNext[n]) {
        if (m_arrExcluded[m_vecSymbol[n]] == 0) {
            if ((qint32)nTarget < (nAccumulated + m_vecFreq[n])) {
                nHit = m_vecFreq[n];
                nFound = n;
                break;
            }
            nAccumulated += m_vecFreq[n];
        }
    }

    if (nFound == HA_NIL) {
        m_arith.update((quint32)nTotal, (quint32)(nTotal + nEscape), (quint32)(nTotal + nEscape));
        if ((m_vecTotal[nContext] == 1) && (m_arrIncrement[m_vecOrder[nContext]] < 0x20)) ++m_arrIncrement[m_vecOrder[nContext]];
        qint32 nLast = 0;
        for (qint32 m = nContext; m != HA_NIL; m = m_vecSymbolNext[m]) {
            nLast = m;
            if (m_arrExcluded[m_vecSymbol[m]] == 0) {
                m_arrExcludedList[m_nExcludedCount] = m_vecSymbol[m];
                ++m_nExcludedCount;
                m_arrExcluded[m_vecSymbol[m]] = 1;
            }
        }
        m_arrEscapeContext[m_nEscapeDepth] = nContext | 0x8000;
        m_arrEscapeNode[m_nEscapeDepth] = nLast;
        ++m_nEscapeDepth;
        return HA_ESCAPE;
    }
    m_arith.update((quint32)nAccumulated, (quint32)(nAccumulated + nHit), (quint32)(nTotal + nEscape));
    if ((m_vecTotal[nContext] == 1) && (m_arrIncrement[m_vecOrder[nContext]] != 0)) --m_arrIncrement[m_vecOrder[nContext]];
    m_arrEscapeNode[m_nEscapeDepth] = nFound;
    m_arrEscapeContext[m_nEscapeDepth] = nContext;
    ++m_nEscapeDepth;
    ++m_nRunCount;
    return m_vecSymbol[nFound];
}

qint32 Model::decodeFallback()
{
    const qint32 nTotal = 0x101 - m_nExcludedCount;
    const quint32 nTarget = m_arith.target((quint32)nTotal);
    qint32 nAccumulated = 0;
    qint32 i = 0;
    while (i < 0x100) {
        if (m_arrExcluded[i] == 0) {
            if ((qint32)nTarget < (nAccumulated + 1)) break;
            ++nAccumulated;
        }
        ++i;
    }
    m_arith.update((quint32)nAccumulated, (quint32)(nAccumulated + 1), (quint32)nTotal);
    return i;
}

void Model::touch(qint32 nContext)
{
    if (nContext == m_nLruHead) return;
    if (nContext == m_nLruTail) {
        m_nLruTail = m_vecLruPrev[nContext];
    } else {
        m_vecLruPrev[m_vecLruNext[nContext]] = m_vecLruPrev[nContext];
        m_vecLruNext[m_vecLruPrev[nContext]] = m_vecLruNext[nContext];
    }
    m_vecLruPrev[m_nLruHead] = (quint16)nContext;
    m_vecLruNext[nContext] = (quint16)m_nLruHead;
    m_nLruHead = nContext;
}

void Model::reclaimSymbols()
{
    while (true) {
        do {
            ++m_nPoolScan;
            if (m_nPoolScan == HA_CONTEXTS) m_nPoolScan = 0;
        } while (m_vecSymbolNext[m_nPoolScan] == HA_NIL);

        qint32 i = 0;
        while (i <= m_nEscapeDepth) {
            if ((m_arrEscapeContext[i] & 0x7fff) == m_nPoolScan) break;
            ++i;
        }
        if (i <= m_nEscapeDepth) continue;

        const qint32 c = m_nPoolScan;
        qint32 nLowest = m_vecFreq[c];
        for (qint32 n = m_vecSymbolNext[c]; n != HA_NIL; n = m_vecSymbolNext[n]) {
            if (m_vecFreq[n] < nLowest) nLowest = m_vecFreq[n];
        }
        ++nLowest;

        if (m_vecFreq[c] < nLowest) {
            qint32 n = m_vecSymbolNext[c];
            while ((m_vecFreq[n] < nLowest) && (m_vecSymbolNext[n] != HA_NIL)) n = m_vecSymbolNext[n];
            m_vecFreq[c] = m_vecFreq[n];
            m_vecSymbol[c] = m_vecSymbol[n];
            const qint32 nAfter = m_vecSymbolNext[n];
            m_vecSymbolNext[n] = (quint16)m_nFreeSymbol;
            m_nFreeSymbol = m_vecSymbolNext[c];
            m_vecSymbolNext[c] = (quint16)nAfter;
            if (nAfter == HA_NIL) {
                m_vecNovel[c] = 0;
                m_vecTotal[c] = m_vecFreq[c];
                m_vecLowCount[c] = (m_vecFreq[c] < 3) ? 1 : 0;
                return;
            }
        }

        m_vecFreq[c] = (quint16)(m_vecFreq[c] / nLowest);
        m_vecTotal[c] = m_vecFreq[c];
        m_vecLowCount[c] = (m_vecFreq[c] < 3) ? 1 : 0;
        m_vecNovel[c] = 0;
        qint32 nPrevious = c;
        qint32 n = m_vecSymbolNext[c];
        while (n != HA_NIL) {
            // the drop test is on the ORIGINAL frequency, before dividing, and
            // every surviving symbol counts as novel again
            if (m_vecFreq[n] < nLowest) {
                m_vecSymbolNext[nPrevious] = m_vecSymbolNext[n];
                m_vecSymbolNext[n] = (quint16)m_nFreeSymbol;
                m_nFreeSymbol = n;
                n = m_vecSymbolNext[nPrevious];
                continue;
            }
            m_vecNovel[c] = (quint8)(m_vecNovel[c] + 1);
            m_vecFreq[n] = (quint16)(m_vecFreq[n] / nLowest);
            m_vecTotal[c] = (quint16)(m_vecTotal[c] + m_vecFreq[n]);
            if (m_vecFreq[n] < 3) m_vecLowCount[c] = (quint8)(m_vecLowCount[c] + 1);
            nPrevious = n;
            n = m_vecSymbolNext[n];
        }
        return;
    }
}

void Model::updateModel(qint32 nSymbol)
{
    while (m_nEscapeDepth) {
        --m_nEscapeDepth;
        qint32 nNode = m_arrEscapeNode[m_nEscapeDepth];
        qint32 nContext = m_arrEscapeContext[m_nEscapeDepth];
        if ((nContext & 0x8000) == 0) {
            ++m_vecFreq[nNode];
            if (m_vecFreq[nNode] == 3) --m_vecLowCount[nContext];
        } else {
            nContext &= 0x7fff;
            if (m_nFreeSymbol == HA_NIL) reclaimSymbols();
            m_vecSymbolNext[nNode] = (quint16)m_nFreeSymbol;
            nNode = m_vecSymbolNext[nNode];
            m_nFreeSymbol = m_vecSymbolNext[m_nFreeSymbol];
            m_vecSymbolNext[nNode] = (quint16)HA_NIL;
            m_vecFreq[nNode] = 1;
            m_vecSymbol[nNode] = (quint8)nSymbol;
            m_vecNovel[nContext] = (quint8)(m_vecNovel[nContext] + 1);
            m_vecLowCount[nContext] = (quint8)(m_vecLowCount[nContext] + 1);
        }
        ++m_vecTotal[nContext];
        const qint32 nTotal = m_vecTotal[nContext];
        const qint32 nDivisor = m_vecNovel[nContext] + 1;
        if ((m_vecFreq[nNode] * 2) < (nTotal / nDivisor)) {
            m_vecRescale[nContext] = (quint8)(m_vecRescale[nContext] - 1);
        } else if (m_vecRescale[nContext] < 4) {
            ++m_vecRescale[nContext];
        }
        if ((m_vecRescale[nContext] == 0) || (m_vecTotal[nContext] > HA_MAX_TOTAL)) {
            m_vecRescale[nContext] = (quint8)(m_vecRescale[nContext] + 1);
            m_vecLowCount[nContext] = 0;
            m_vecTotal[nContext] = 0;
            for (qint32 n = nContext; n != HA_NIL; n = m_vecSymbolNext[n]) {
                if (m_vecFreq[n] < 2) {
                    ++m_vecTotal[nContext];
                    ++m_vecLowCount[nContext];
                } else {
                    m_vecFreq[n] = (quint16)(m_vecFreq[n] >> 1);
                    m_vecTotal[nContext] = (quint16)(m_vecTotal[nContext] + m_vecFreq[n]);
                    if (m_vecFreq[n] < 3) ++m_vecLowCount[nContext];
                }
            }
        }
    }
}

void Model::addContext(qint32 nOrder, qint32 nSymbol)
{
    const qint32 c = m_nLruTail;
    m_vecLruPrev[m_nLruHead] = (quint16)c;
    m_nLruTail = m_vecLruPrev[c];
    m_vecLruNext[c] = (quint16)m_nLruHead;
    m_nLruHead = c;

    if (m_vecOrder[c] != 0xff) {
        if (m_vecOrder[c] == 4) {
            --m_nOrder4Budget;
            if (m_nOrder4Budget == 0) m_nMaxOrder = 3;
        }
        const qint32 h = hashOf(m_vecContextBytes.constData() + (c * 4), m_vecOrder[c]);
        if (m_vecBuckets[h] == c) {
            m_vecBuckets[h] = m_vecHashNext[c];
        } else {
            qint32 p = m_vecBuckets[h];
            while ((p != HA_NIL) && (m_vecHashNext[p] != c)) p = m_vecHashNext[p];
            if (p != HA_NIL) m_vecHashNext[p] = m_vecHashNext[c];
        }
        if (m_vecSymbolNext[c] != HA_NIL) {
            qint32 n = m_vecSymbolNext[c];
            while (m_vecSymbolNext[n] != HA_NIL) n = m_vecSymbolNext[n];
            m_vecSymbolNext[n] = (quint16)m_nFreeSymbol;
            m_nFreeSymbol = m_vecSymbolNext[c];
        }
    }

    m_vecSymbolNext[c] = (quint16)HA_NIL;
    m_vecLowCount[c] = 1;
    m_vecTotal[c] = 1;
    m_vecFreq[c] = 1;
    m_vecSymbol[c] = (quint8)nSymbol;
    m_vecRescale[c] = 4;
    m_vecNovel[c] = 0;
    m_vecOrder[c] = (quint8)nOrder;
    m_vecContextBytes[c * 4 + 0] = m_arrContext[0];
    m_vecContextBytes[c * 4 + 1] = m_arrContext[1];
    m_vecContextBytes[c * 4 + 2] = m_arrContext[2];
    m_vecContextBytes[c * 4 + 3] = m_arrContext[3];
    const qint32 h = hashOf(m_arrContext, nOrder);
    m_vecHashNext[c] = m_vecBuckets[h];
    m_vecBuckets[h] = (quint16)c;
}

bool Model::run(qint64 nLimit, QByteArray *pbaOut, XBinary::PDSTRUCT *pPdStruct)
{
    pbaOut->reserve((qint32)nLimit);
    while (pbaOut->size() < nLimit) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        qint32 nContext = findDeepest();
        const qint32 nOrder = (nContext == HA_NIL) ? 0 : (m_vecOrder[nContext] + 1);
        qint32 nMaxOrder = m_nMaxOrder + 1;
        qint32 nSymbol = 0;

        while (nContext != HA_NIL) {
            nSymbol = (m_nExcludedCount == 0) ? decodeNoExclusion(nContext) : decodeWithExclusion(nContext);
            if (nSymbol != HA_ESCAPE) {
                touch(nContext);
                break;
            }
            nContext = findNext();
        }
        if (nContext == HA_NIL) nSymbol = decodeFallback();
        if (nSymbol == HA_ESCAPE) break;

        updateModel(nSymbol);
        while (nOrder < nMaxOrder) {
            --nMaxOrder;
            addContext(nMaxOrder, nSymbol);
        }
        pbaOut->append((char)(quint8)nSymbol);
        m_arrContext[3] = m_arrContext[2];
        m_arrContext[2] = m_arrContext[1];
        m_arrContext[1] = m_arrContext[0];
        m_arrContext[0] = (quint8)nSymbol;
    }
    return true;
}

// ---------------------------------------------------------------------------
// HA method 1, "ASC" - LZ77 over a 31200-byte circular window, entropy coded
// with five implicit cumulative-frequency trees plus a 4-state binary
// literal/match context, over the same arithmetic coder as HSC above.
// Written from the reference decompressor (VA 0x004bd290 and friends).
// ---------------------------------------------------------------------------
const qint32 ASC_WSIZE = 31200;  // 0x79e0
const qint32 ASC_MINLEN = 3;
const qint32 ASC_SLCODES = 16;
const qint32 ASC_LLLEN = 16;
const qint32 ASC_LTCODES = 64;
const qint32 ASC_CTCODES = 256;
const qint32 ASC_PTCODES = 16;
const qint32 ASC_LTSTEP = 8;
const qint32 ASC_CTSTEP = 1;
const qint32 ASC_PTSTEP = 24;
const qint32 ASC_TTSTEP = 40;
const qint32 ASC_LTMAX = 6000;
const qint32 ASC_CTMAX = 1000;
const qint32 ASC_PTMAX = 6000;
const qint32 ASC_TTMAX = 6000;
const qint32 ASC_CPLEN = 8;
const qint32 ASC_LPLEN = 4;
// code SLCODES-1 is reserved for the longest match: 15 + 48 * 16 = 783
const qint32 ASC_MAXLEN = (ASC_SLCODES - 1) + ((ASC_LTCODES - ASC_SLCODES) * ASC_LLLEN);

// Implicit binary cumulative-frequency tree over 2 * nCount 16-bit counters;
// leaves live at [nCount .. 2 * nCount - 1], the running total at [1].
class AscTree {
public:
    void init(qint32 nCount, quint16 nLeafValue)
    {
        m_nCount = nCount;
        m_vecTree.fill(0, nCount * 2);
        for (qint32 i = nCount; i < (nCount * 2); ++i) m_vecTree[i] = nLeafValue;
        rebuild();
    }

    quint16 total() const
    {
        return m_vecTree[1];
    }

    quint16 freq(qint32 nIndex) const
    {
        if ((nIndex < 0) || (nIndex >= m_nCount)) return 0;
        return m_vecTree[m_nCount + nIndex];
    }

    // bump one leaf and its ancestors, rescale on overflow
    void add(qint32 nIndex, quint16 nStep, quint16 nMaxTotal)
    {
        if ((nIndex < 0) || (nIndex >= m_nCount)) return;
        qint32 i = m_nCount + nIndex;
        while (i != 0) {
            m_vecTree[i] = (quint16)(m_vecTree[i] + nStep);
            i >>= 1;
        }
        if (m_vecTree[1] >= nMaxTotal) rescale();
    }

    // zero one leaf and subtract its weight from its ancestors
    void remove(qint32 nIndex)
    {
        if ((nIndex < 0) || (nIndex >= m_nCount)) return;
        qint32 i = m_nCount + nIndex;
        const quint16 nValue = m_vecTree[i];
        while (i != 0) {
            m_vecTree[i] = (quint16)(m_vecTree[i] - nValue);
            i >>= 1;
        }
    }

    // Descend from node 2; returns the leaf index, *pnLow gets its cumulative low
    qint32 find(quint32 nTarget, quint32 *pnLow) const
    {
        qint32 nNode = 2;
        quint32 nCum = 0;
        while (true) {
            if ((nCum + m_vecTree[nNode]) <= nTarget) {
                nCum += m_vecTree[nNode];
                ++nNode;
            }
            if (nNode > (m_nCount - 1)) break;
            nNode <<= 1;
            if (nNode >= (m_nCount * 2)) return -1;  // cannot happen on a sane tree
        }
        if (nNode >= (m_nCount * 2)) return -1;
        *pnLow = nCum;
        return nNode - m_nCount;
    }

private:
    void rebuild()
    {
        for (qint32 i = m_nCount - 1; i > 0; --i) m_vecTree[i] = (quint16)(m_vecTree[i * 2] + m_vecTree[i * 2 + 1]);
    }

    // halve every leaf above 1, then recompute the interior
    void rescale()
    {
        for (qint32 i = (m_nCount * 2) - 1; i >= m_nCount; --i) {
            if (m_vecTree[i] > 1) m_vecTree[i] = (quint16)(m_vecTree[i] >> 1);
        }
        rebuild();
    }

    qint32 m_nCount;
    QVector<quint16> m_vecTree;
};

class AscModel {
public:
    AscModel(const quint8 *pData, qint64 nSize) : m_arith(pData, nSize)
    {
        m_baWindow.fill((char)0, ASC_WSIZE);
        m_pWindow = (quint8 *)m_baWindow.data();
        m_nWindowPos = 0;

        m_treeLength.init(ASC_LTCODES, 0);
        m_treeNewLength.init(ASC_LTCODES, 1);
        m_treePosition.init(ASC_PTCODES, 0);
        m_treeChar.init(ASC_CTCODES, 0);
        m_treeNewChar.init(ASC_CTCODES, 1);
        m_treePosition.add(0, ASC_PTSTEP, ASC_PTMAX);

        for (qint32 i = 0; i < 4; ++i) {
            m_arrLiteralWeight[i] = ASC_TTSTEP;
            m_arrMatchWeight[i] = ASC_TTSTEP;
        }
        m_nContext = 0;
        m_nOutSize = 0;
        m_nPosLimit = 1;
        m_nPosCode = 1;
        m_nCharEscape = 1;
        m_nLengthEscape = ASC_LTSTEP;
        m_pbaOut = nullptr;
    }

    bool run(qint64 nLimit, QByteArray *pbaOut, XBinary::PDSTRUCT *pPdStruct);

private:
    void contextRescale(qint32 nContext)
    {
        quint16 nValue = (quint16)(m_arrLiteralWeight[nContext] >> 1);
        m_arrLiteralWeight[nContext] = nValue ? nValue : 1;
        nValue = (quint16)(m_arrMatchWeight[nContext] >> 1);
        m_arrMatchWeight[nContext] = nValue ? nValue : 1;
    }

    void putLiteral(quint8 nChar)
    {
        m_pWindow[m_nWindowPos] = nChar;
        m_pbaOut->append((char)nChar);
        ++m_nWindowPos;
        if (m_nWindowPos == ASC_WSIZE) m_nWindowPos = 0;
    }

    bool putMatch(qint32 nLength, qint32 nPosition)
    {
        qint32 nSource = 0;
        if (nPosition < m_nWindowPos) {
            nSource = m_nWindowPos - nPosition - 1;
        } else {
            nSource = m_nWindowPos + ASC_WSIZE - nPosition - 1;
        }
        if ((nSource < 0) || (nSource >= ASC_WSIZE)) return false;
        for (qint32 i = 0; i < nLength; ++i) {
            const quint8 nChar = m_pWindow[nSource];
            m_pWindow[m_nWindowPos] = nChar;
            m_pbaOut->append((char)nChar);
            ++m_nWindowPos;
            if (m_nWindowPos == ASC_WSIZE) m_nWindowPos = 0;
            ++nSource;
            if (nSource == ASC_WSIZE) nSource = 0;
        }
        return true;
    }

    qint32 decodeChar();
    qint32 decodeLength();
    qint32 decodePosition();

    Arith m_arith;
    QByteArray m_baWindow;
    quint8 *m_pWindow;
    qint32 m_nWindowPos;
    AscTree m_treeLength;
    AscTree m_treeNewLength;
    AscTree m_treePosition;
    AscTree m_treeChar;
    AscTree m_treeNewChar;
    quint16 m_arrLiteralWeight[4];
    quint16 m_arrMatchWeight[4];
    qint32 m_nContext;
    qint32 m_nOutSize;
    quint16 m_nPosLimit;
    qint32 m_nPosCode;
    quint16 m_nCharEscape;
    quint16 m_nLengthEscape;
    QByteArray *m_pbaOut;
};

// Two-level "seen / not yet seen" model: a hit in the seen tree codes the byte
// directly, an escape codes it from the not-yet-seen tree, which then loses the
// symbol and bumps its still-unseen neighbours.
qint32 AscModel::decodeChar()
{
    quint32 nLow = 0;
    qint32 nChar = 0;
    const quint32 nSeen = m_treeChar.total();
    const quint32 nTotal = nSeen + m_nCharEscape;
    quint32 nTarget = m_arith.target(nTotal);

    if (nTarget < nSeen) {
        nChar = m_treeChar.find(nTarget, &nLow);
        if (nChar < 0) return -1;
        m_arith.update(nLow, nLow + m_treeChar.freq(nChar), nTotal);
    } else {
        m_arith.update(nSeen, nTotal, nTotal);
        const quint32 nNewTotal = m_treeNewChar.total();
        if (nNewTotal == 0) return -1;
        nTarget = m_arith.target(nNewTotal);
        nChar = m_treeNewChar.find(nTarget, &nLow);
        if (nChar < 0) return -1;
        m_arith.update(nLow, nLow + m_treeNewChar.freq(nChar), nNewTotal);
        m_treeNewChar.remove(nChar);
        if (m_treeNewChar.total() == 0) {
            m_nCharEscape = 0;
        } else {
            m_nCharEscape = (quint16)(m_nCharEscape + 1);
        }
        // NOTE: the upper bound is exclusive here.  HA's encoder is inclusive -
        // a real asymmetry in the original, not a transcription slip.
        const qint32 nFirst = (nChar < ASC_CPLEN) ? 0 : (nChar - ASC_CPLEN);
        const qint32 nBound = ((nChar + ASC_CPLEN) > (ASC_CTCODES - 2)) ? (ASC_CTCODES - 1) : (nChar + ASC_CPLEN);
        for (qint32 i = nFirst; i < nBound; ++i) {
            if (m_treeNewChar.freq(i) != 0) m_treeNewChar.add(i, ASC_CTSTEP, ASC_CTMAX);
        }
    }

    m_treeChar.add(nChar, ASC_CTSTEP, ASC_CTMAX);
    if (m_treeChar.freq(nChar) == (3 * ASC_CTSTEP)) {
        m_nCharEscape = (m_nCharEscape < 2) ? 1 : (quint16)(m_nCharEscape - 1);
    }

    return nChar;
}

qint32 AscModel::decodeLength()
{
    quint32 nLow = 0;
    qint32 nCode = 0;
    const quint32 nSeen = m_treeLength.total();
    const quint32 nTotal = nSeen + m_nLengthEscape;
    quint32 nTarget = m_arith.target(nTotal);

    if (nTarget < nSeen) {
        nCode = m_treeLength.find(nTarget, &nLow);
        if (nCode < 0) return -1;
        m_arith.update(nLow, nLow + m_treeLength.freq(nCode), nTotal);
    } else {
        m_arith.update(nSeen, nTotal, nTotal);
        const quint32 nNewTotal = m_treeNewLength.total();
        if (nNewTotal == 0) return -1;
        nTarget = m_arith.target(nNewTotal);
        nCode = m_treeNewLength.find(nTarget, &nLow);
        if (nCode < 0) return -1;
        m_arith.update(nLow, nLow + m_treeNewLength.freq(nCode), nNewTotal);
        m_treeNewLength.remove(nCode);
        if (m_treeNewLength.total() == 0) {
            m_nLengthEscape = 0;
        } else {
            m_nLengthEscape = (quint16)(m_nLengthEscape + ASC_LTSTEP);
        }
        const qint32 nFirst = (nCode < ASC_LPLEN) ? 0 : (nCode - ASC_LPLEN);
        const qint32 nBound = ((nCode + ASC_LPLEN) > (ASC_LTCODES - 2)) ? (ASC_LTCODES - 1) : (nCode + ASC_LPLEN);
        for (qint32 i = nFirst; i < nBound; ++i) {
            if (m_treeNewLength.freq(i) != 0) m_treeNewLength.add(i, 1, ASC_LTMAX);
        }
    }

    m_treeLength.add(nCode, ASC_LTSTEP, ASC_LTMAX);
    if (m_treeLength.freq(nCode) == (3 * ASC_LTSTEP)) {
        m_nLengthEscape = (m_nLengthEscape < (ASC_LTSTEP + 1)) ? 1 : (quint16)(m_nLengthEscape - ASC_LTSTEP);
    }

    qint32 nLength = nCode;
    if (nCode == (ASC_SLCODES - 1)) {
        nLength = ASC_MAXLEN;
    } else if (nCode > (ASC_SLCODES - 1)) {
        const quint32 nExtra = m_arith.target(ASC_LLLEN);
        if (nExtra >= (quint32)ASC_LLLEN) return -1;
        m_arith.update(nExtra, nExtra + 1, ASC_LLLEN);
        nLength = ((nCode - ASC_SLCODES) * ASC_LLLEN) + (qint32)nExtra + (ASC_SLCODES - 1);
    }

    return nLength + ASC_MINLEN;
}

// 16 buckets that go live as the output grows; bucket k covers [2^(k-1), 2^k)
// with uniform extra bits, and the top live bucket is clamped to the output.
qint32 AscModel::decodePosition()
{
    while (m_nPosLimit < m_nOutSize) {
        if (m_nPosCode < ASC_PTCODES) m_treePosition.add(m_nPosCode, ASC_PTSTEP, ASC_PTMAX);
        ++m_nPosCode;
        m_nPosLimit = (quint16)(m_nPosLimit << 1);
        if (m_nPosLimit == 0) break;
    }

    quint32 nLow = 0;
    const quint32 nTotal = m_treePosition.total();
    if (nTotal == 0) return -1;
    const quint32 nTarget = m_arith.target(nTotal);
    qint32 nCode = m_treePosition.find(nTarget, &nLow);
    if (nCode < 0) return -1;
    m_arith.update(nLow, nLow + m_treePosition.freq(nCode), nTotal);
    m_treePosition.add(nCode, ASC_PTSTEP, ASC_PTMAX);

    qint32 nPosition = nCode;
    if (nCode > 1) {
        const qint32 nBase = (qint32)((quint16)(1 << nCode) >> 1);
        quint32 nRange = (quint32)nBase;
        if (nBase == (qint32)(m_nPosLimit >> 1)) nRange = (quint32)(quint16)(m_nOutSize - (m_nPosLimit >> 1));
        if (nRange == 0) return -1;
        const quint32 nExtra = m_arith.target(nRange);
        if (nExtra >= nRange) return -1;
        m_arith.update(nExtra, nExtra + 1, nRange);
        nPosition = (qint32)nExtra + nBase;
    }

    if ((nPosition < 0) || (nPosition >= ASC_WSIZE)) return -1;

    return nPosition;
}

bool AscModel::run(qint64 nLimit, QByteArray *pbaOut, XBinary::PDSTRUCT *pPdStruct)
{
    m_pbaOut = pbaOut;
    pbaOut->reserve((qint32)nLimit);

    while (pbaOut->size() < nLimit) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const quint32 nSum = (quint32)m_arrLiteralWeight[m_nContext] + m_arrMatchWeight[m_nContext];
        const quint32 nTarget = m_arith.target(nSum + 1);

        if (nTarget < m_arrLiteralWeight[m_nContext]) {
            m_arith.update(0, m_arrLiteralWeight[m_nContext], nSum + 1);
            m_arrLiteralWeight[m_nContext] = (quint16)(m_arrLiteralWeight[m_nContext] + ASC_TTSTEP);
            if (nSum > (quint32)(ASC_TTMAX - 1)) contextRescale(m_nContext);
            m_nContext = (m_nContext * 2) & 3;

            const qint32 nChar = decodeChar();
            if (nChar < 0) return false;
            putLiteral((quint8)nChar);
            if (m_nOutSize < ASC_WSIZE) ++m_nOutSize;
            continue;
        }

        if (nSum <= nTarget) break;  // end-of-stream slot

        m_arith.update(m_arrLiteralWeight[m_nContext], nSum, nSum + 1);
        m_arrMatchWeight[m_nContext] = (quint16)(m_arrMatchWeight[m_nContext] + ASC_TTSTEP);
        if (nSum > (quint32)(ASC_TTMAX - 1)) contextRescale(m_nContext);
        m_nContext = ((m_nContext * 2) & 3) | 1;

        const qint32 nPosition = decodePosition();
        if (nPosition < 0) return false;
        const qint32 nLength = decodeLength();
        if (nLength < 0) return false;

        if (m_nOutSize < ASC_WSIZE) {
            m_nOutSize += nLength;
            if (m_nOutSize > ASC_WSIZE) m_nOutSize = ASC_WSIZE;
        }

        if (!putMatch(nLength, nPosition)) return false;
    }

    return true;
}
}  // namespace

bool XHADecoder::decodeHSC(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    Model model((const quint8 *)baPacked.constData(), baPacked.size());
    QByteArray baOut;
    if (!model.run(nUncompressedSize, &baOut, pPdStruct)) return false;
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XHADecoder::decodeASC(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    AscModel model((const quint8 *)baPacked.constData(), baPacked.size());
    QByteArray baOut;
    if (!model.run(nUncompressedSize, &baOut, pPdStruct)) return false;
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
