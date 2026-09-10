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
#include "xaindecoder.h"

namespace {
const qint32 AIN_WINDOW = 0x8000;
const qint32 AIN_MAIN_SYMBOLS = 0x110;
const qint32 AIN_LENGTH_SYMBOLS = 0xfe;
const qint32 AIN_PRE_SYMBOLS = 0x13;
const qint32 AIN_TREE_ENTRIES = 1024;
const qint32 AIN_MAX_LENGTH = 16;

class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nBuffer(0), m_nCount(0)
    {
    }

    bool fill(qint32 nBits)
    {
        while (m_nCount < nBits) {
            if (m_nPosition >= m_nSize) return false;
            m_nBuffer |= ((quint64)m_pData[m_nPosition]) << m_nCount;
            ++m_nPosition;
            m_nCount += 8;
        }
        return true;
    }

    void drop(qint32 nBits)
    {
        m_nBuffer >>= nBits;
        m_nCount -= nBits;
    }

    quint32 peek8() const
    {
        return (quint32)(m_nBuffer & 0xff);
    }

    quint32 peek1() const
    {
        return (quint32)(m_nBuffer & 1);
    }

    // -1 means the stream is exhausted
    qint64 get(qint32 nBits)
    {
        if (nBits == 0) return 0;
        if (!fill(nBits)) return -1;
        const quint64 nValue = m_nBuffer & ((((quint64)1) << nBits) - 1);
        drop(nBits);
        return (qint64)nValue;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint64 m_nBuffer;
    qint32 m_nCount;
};

struct TREE {
    quint16 table[256];
    quint16 tree[AIN_TREE_ENTRIES];
};

struct BUILDSTATE {
    qint32 arrCounts[AIN_MAX_LENGTH + 1];
    QVector<qint32> listOrder;
    qint32 nNextNode;
    qint32 nNextSymbol;
    bool bFailed;
};

void buildWalk(BUILDSTATE *pState, TREE *pTree, qint32 nDepth, qint32 nNode, quint32 nPrefix)
{
    if (pState->bFailed) return;
    if ((nDepth > AIN_MAX_LENGTH) || (nNode < 0) || (nNode >= AIN_TREE_ENTRIES)) {
        pState->bFailed = true;
        return;
    }

    --pState->arrCounts[nDepth];
    if (pState->arrCounts[nDepth] < 0) {
        const qint32 nChild = pState->nNextNode;
        if ((nChild + 1) >= AIN_TREE_ENTRIES) {
            pState->bFailed = true;
            return;
        }
        pState->nNextNode += 2;
        pTree->tree[nNode] = (quint16)nChild;
        if (nDepth == 8) pTree->table[nPrefix >> 8] = (quint16)(nChild | 0x8000);
        buildWalk(pState, pTree, nDepth + 1, nChild, (nPrefix >> 1) & 0xffff);
        buildWalk(pState, pTree, nDepth + 1, nChild + 1, ((nPrefix >> 1) | 0x8000) & 0xffff);
        return;
    }

    if (pState->nNextSymbol >= pState->listOrder.size()) {
        pState->bFailed = true;
        return;
    }
    const qint32 nSymbol = pState->listOrder.at(pState->nNextSymbol);
    ++pState->nNextSymbol;
    pTree->tree[nNode] = (quint16)((-nSymbol) & 0xffff);
    if (nDepth < 9) {
        qint32 nIndex = (nDepth > 0) ? (qint32)(nPrefix >> (16 - nDepth)) : 0;
        const quint16 nEntry = (quint16)((nSymbol | (nDepth << 10)) & 0xffff);
        while (nIndex < 0x100) {
            pTree->table[nIndex] = nEntry;
            nIndex += (1 << nDepth);
        }
    }
}

// counting sort into length buckets, a Kraft check, then the
// recursive walk above.
bool buildTree(const qint32 *pLengths, qint32 nCount, TREE *pTree)
{
    BUILDSTATE state;
    memset(state.arrCounts, 0, sizeof(state.arrCounts));
    state.nNextNode = 0;
    state.nNextSymbol = 0;
    state.bFailed = false;

    QVector<qint32> listHead(AIN_MAX_LENGTH + 1, -1);
    QVector<qint32> listNext(nCount, -1);
    for (qint32 i = 0; i < nCount; ++i) {
        const qint32 nLength = pLengths[i];
        if ((nLength < 0) || (nLength > AIN_MAX_LENGTH)) return false;
        ++state.arrCounts[nLength];
        listNext[i] = listHead.at(nLength);
        listHead[nLength] = i;
    }

    quint32 nTotal = 0;
    for (qint32 nLength = 1; nLength <= AIN_MAX_LENGTH; ++nLength) {
        nTotal += ((quint32)state.arrCounts[nLength]) << (16 - nLength);
    }
    if (nTotal != 0x10000) return false;

    // the buckets are pushed in increasing symbol order, so walking one gives
    // DECREASING symbols - that ordering is part of the format
    state.listOrder.reserve(nCount);
    for (qint32 nLength = 1; nLength <= AIN_MAX_LENGTH; ++nLength) {
        qint32 i = listHead.at(nLength);
        while (i >= 0) {
            state.listOrder.append(i);
            i = listNext.at(i);
        }
    }
    if (state.listOrder.isEmpty()) return false;

    memset(pTree->table, 0, sizeof(pTree->table));
    memset(pTree->tree, 0, sizeof(pTree->tree));
    state.arrCounts[0] = 0;
    buildWalk(&state, pTree, 0, 0, 0);

    return !state.bFailed;
}

// -1 means the stream is exhausted or the tree is broken
qint32 decodeSymbol(BitReader *pReader, const TREE *pTree)
{
    if (!pReader->fill(8)) return -1;
    const quint16 nEntry = pTree->table[pReader->peek8()];
    if (nEntry < 0x8000) {
        const qint32 nLength = nEntry >> 10;
        if (nLength == 0) return -1;
        pReader->drop(nLength);
        return (qint32)(nEntry & 0x3ff);
    }

    qint32 nNode = (qint32)(nEntry & 0x3ff);
    pReader->drop(8);
    if (!pReader->fill(16)) return -1;
    for (qint32 nGuard = 0; nGuard < AIN_MAX_LENGTH; ++nGuard) {
        const qint32 nIndex = nNode + (qint32)pReader->peek1();
        if ((nIndex < 0) || (nIndex >= AIN_TREE_ENTRIES)) return -1;
        const qint32 nValue = (qint16)pTree->tree[nIndex];
        pReader->drop(1);
        nNode = nValue;
        if (nValue <= 0) return -nValue;
        if (!pReader->fill(1)) return -1;
    }

    return -1;
}

// The reference implementation
bool readTable(BitReader *pReader, qint32 nSymbols, TREE *pTree)
{
    qint32 arrPre[AIN_PRE_SYMBOLS];
    memset(arrPre, 0, sizeof(arrPre));

    qint64 nValue = pReader->get(5);
    if (nValue < 0) return false;
    qint32 nCount = AIN_PRE_SYMBOLS - (qint32)nValue;
    if (nCount < 0) nCount = 0;
    for (qint32 i = 0; i < nCount; ++i) {
        nValue = pReader->get(3);
        if (nValue < 0) return false;
        qint32 nLength = (qint32)nValue;
        if (nLength == 7) {
            while (true) {
                const qint64 nBit = pReader->get(1);
                if (nBit < 0) return false;
                if (nBit == 0) break;
                ++nLength;
                if (nLength > AIN_MAX_LENGTH) return false;
            }
        }
        arrPre[i] = nLength;
    }

    TREE preTree;
    if (!buildTree(arrPre, AIN_PRE_SYMBOLS, &preTree)) return false;

    nValue = pReader->get(9);
    if (nValue < 0) return false;
    nCount = nSymbols - (qint32)nValue;
    if (nCount < 0) nCount = 0;

    QVector<qint32> listLengths(nSymbols, 0);
    qint32 nAt = 0;
    while (nAt < nCount) {
        const qint32 nSymbol = decodeSymbol(pReader, &preTree);
        if (nSymbol < 0) return false;
        if (nSymbol >= 3) {
            if (nAt >= nSymbols) return false;
            listLengths[nAt] = nSymbol - 2;
            ++nAt;
            continue;
        }
        qint32 nRun = 1;
        if (nSymbol == 1) {
            nValue = pReader->get(4);
            if (nValue < 0) return false;
            nRun = (qint32)nValue + 3;
        } else if (nSymbol == 2) {
            nValue = pReader->get(9);
            if (nValue < 0) return false;
            nRun = (qint32)nValue + 0x14;
        }
        for (qint32 i = 0; i < nRun; ++i) {
            if (nAt >= nSymbols) return false;
            listLengths[nAt] = 0;
            ++nAt;
        }
    }

    return buildTree(listLengths.constData(), nSymbols, pTree);
}
}  // namespace

bool XAINDecoder::decode(const QByteArray &baPacked, qint64 nSkipSize, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nSkipSize < 0) || (nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    BitReader reader((const quint8 *)baPacked.constData(), baPacked.size());
    QByteArray baWindow(AIN_WINDOW, (char)0);
    quint8 *pWindow = (quint8 *)baWindow.data();
    qint32 nWritePosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    qint64 nProduced = 0;

    // a single throw-away bit precedes the first block
    if (reader.get(1) < 0) return false;

    TREE mainTree;
    TREE lengthTree;
    if (!readTable(&reader, AIN_MAIN_SYMBOLS, &mainTree)) return false;
    if (!readTable(&reader, AIN_LENGTH_SYMBOLS, &lengthTree)) return false;

    const qint64 nWanted = nSkipSize + nUncompressedSize;
    while (nProduced < nWanted) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint32 nSymbol = decodeSymbol(&reader, &mainTree);
        if (nSymbol < 0) break;

        if (nSymbol < 0x100) {
            pWindow[nWritePosition] = (quint8)nSymbol;
            nWritePosition = (nWritePosition + 1) & (AIN_WINDOW - 1);
            if (nProduced >= nSkipSize) baOut.append((char)(quint8)nSymbol);
            ++nProduced;
            continue;
        }

        qint32 nDistance = nSymbol - 0x100;
        if (nDistance > 1) {
            const qint32 nExtraBits = nSymbol - 0x101;
            const qint64 nExtra = reader.get(nExtraBits);
            if (nExtra < 0) break;
            if (nExtra == 0x3fff) {
                // the escape: another block, or the end of the stream
                const qint64 nBit = reader.get(1);
                if (nBit != 0) break;
                if (!readTable(&reader, AIN_MAIN_SYMBOLS, &mainTree)) break;
                if (!readTable(&reader, AIN_LENGTH_SYMBOLS, &lengthTree)) break;
                continue;
            }
            nDistance = (qint32)nExtra | (1 << nExtraBits);
        }

        qint32 nReadPosition = (nWritePosition - (nDistance + 1)) & (AIN_WINDOW - 1);
        const qint32 nLengthSymbol = decodeSymbol(&reader, &lengthTree);
        if (nLengthSymbol < 0) break;

        for (qint32 i = 0; i < (nLengthSymbol + 3); ++i) {
            const quint8 nByte = pWindow[nReadPosition];
            pWindow[nWritePosition] = nByte;
            nWritePosition = (nWritePosition + 1) & (AIN_WINDOW - 1);
            nReadPosition = (nReadPosition + 1) & (AIN_WINDOW - 1);
            if (nProduced >= nSkipSize) baOut.append((char)nByte);
            ++nProduced;
            if (nProduced >= nWanted) break;
        }
    }

    if (baOut.size() > nUncompressedSize) baOut.truncate((qint32)nUncompressedSize);
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
