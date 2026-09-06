/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xniddecoder.h"

#include <string.h>

namespace {

const qint32 NID_NSYM = 0x101;  // 257 symbols per model
const qint32 NID_RING_SIZE = 0x10000;
const qint32 NID_RING_MASK = NID_RING_SIZE - 1;
const qint32 NID_WEIGHT_STEP = 0x20;
const quint32 NID_WEIGHT_LIMIT = 0xffdd;
const qint32 NID_MAX_TREE_DEPTH = 0x10;
// A single member of the reference corpus tops out at a few megabytes; the cap
// only stops a corrupt directory size from asking for a huge allocation.
const qint64 NID_MAX_UNPACKED_SIZE = qint64(768) * 1024 * 1024;

// LSB-first bit reader over the block payload.
class NidBits {
public:
    NidBits(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nBuffer(0), m_nCount(0), m_bError(false)
    {
    }

    qint32 bit()
    {
        if (m_nCount == 0) {
            if (m_nPos >= m_nSize) {
                m_bError = true;
                return -1;
            }
            m_nBuffer = m_pData[m_nPos++];
            m_nCount = 8;
        }
        const qint32 nResult = static_cast<qint32>(m_nBuffer & 1);
        m_nBuffer >>= 1;
        --m_nCount;
        return nResult;
    }

    qint32 bits(qint32 nCount)
    {
        qint32 nResult = 0;
        for (qint32 i = 0; i < nCount; i++) {
            const qint32 nBit = bit();
            if (nBit < 0) return -1;
            nResult |= nBit << i;
        }
        return nResult;
    }

    bool isError() const
    {
        return m_bError;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nBuffer;
    qint32 m_nCount;
    bool m_bError;
};

// Semi-adaptive model: a weight ordered symbol list that freezes into a static
// Huffman tree once nMaxCount symbols have been decoded through it.
class NidModel {
public:
    explicit NidModel(qint32 nMaxCount) : m_nMaxCount(nMaxCount), m_nCounter(0), m_bUseList(true), m_nRoot(0)
    {
        for (qint32 i = 0; i < NID_NSYM; i++) {
            m_nWeight[i + 1] = 1;
            m_nSymbolAt[i] = static_cast<quint16>(i);
            m_nPositionOf[i] = static_cast<quint16>(i);
            m_nChild0[i] = 0;
            m_nChild1[i] = 0;
        }
        m_nWeight[0] = 0;
        m_nWeight[NID_NSYM + 1] = 0;
        m_nTotal = static_cast<quint16>(NID_NSYM);
    }

    // Returns the decoded symbol, or -1 on a bit-reader failure / broken tree.
    qint32 decode(NidBits *pBits)
    {
        if (!m_bUseList) {
            qint32 nNode = m_nRoot;
            for (qint32 nSteps = 0; nSteps < 64; nSteps++) {
                const qint32 nBit = pBits->bit();
                if (nBit < 0) return -1;
                if ((nNode < 0) || (nNode >= NID_NSYM)) return -1;
                nNode = (nBit == 0) ? m_nChild0[nNode] : m_nChild1[nNode];
                if (nNode >= NID_NSYM) return nNode - NID_NSYM;
            }
            return -1;
        }

        qint32 nIndex = 1;
        quint32 nRange = m_nTotal;
        for (qint32 nSteps = 0; nSteps < 64; nSteps++) {
            const quint32 nHalf = nRange >> 1;
            qint32 nScan = nIndex;
            qint32 nNext = nIndex + 1;
            quint32 nSum = 0;
            while (true) {
                if ((nScan < 1) || (nScan > NID_NSYM)) return -1;
                nSum += m_nWeight[nScan];
                nNext = nScan + 1;
                if (nSum >= nHalf) break;
                nScan = nNext;
            }
            const qint32 nBit = pBits->bit();
            if (nBit < 0) return -1;
            if (nBit) {
                if (nSum > nRange) return -1;
                nSum = nRange - nSum;
                nIndex = nNext;
            }
            nRange = nSum;
            if ((nIndex < 1) || (nIndex > NID_NSYM)) return -1;
            if (nRange <= m_nWeight[nIndex]) {
                const qint32 nSymbol = m_nSymbolAt[nIndex - 1];
                const qint32 nCounter = m_nCounter++;
                if (nCounter < m_nMaxCount) {
                    update(nIndex - 1);
                } else {
                    if (!buildTree()) return -1;
                    m_bUseList = false;
                }
                return nSymbol;
            }
        }
        return -1;
    }

private:
    void rescale(quint32 nInitial)
    {
        quint32 nAccumulator = nInitial;
        for (qint32 i = 1; i <= NID_NSYM; i++) {
            m_nWeight[i] = static_cast<quint16>((m_nWeight[i] + 1) >> 1);
            nAccumulator += m_nWeight[i];
        }
        m_nTotal = static_cast<quint16>(nAccumulator);
    }

    void update(qint32 nPosition)
    {
        const quint16 nSymbol = m_nSymbolAt[nPosition];
        m_nTotal = static_cast<quint16>(m_nTotal + NID_WEIGHT_STEP);
        if (m_nTotal > NID_WEIGHT_LIMIT) rescale(NID_WEIGHT_STEP);
        m_nWeight[nPosition + 1] = static_cast<quint16>(m_nWeight[nPosition + 1] + NID_WEIGHT_STEP);
        const quint16 nNewWeight = m_nWeight[nPosition + 1];

        qint32 i = nPosition;
        while ((i >= 1) && (nNewWeight > m_nWeight[i])) {
            m_nSymbolAt[i] = m_nSymbolAt[i - 1];
            m_nWeight[i + 1] = m_nWeight[i];
            m_nPositionOf[m_nSymbolAt[i - 1]] = static_cast<quint16>(i);
            --i;
        }
        if (nPosition != i) {
            m_nWeight[i + 1] = nNewWeight;
            m_nSymbolAt[i] = nSymbol;
            m_nPositionOf[nSymbol] = static_cast<quint16>(i);
        }
    }

    qint32 build(qint32 nStart, qint32 nCount, quint32 nTotal, qint32 nDepth)
    {
        if (nCount == 1) return m_nSymbolAt[nStart] + NID_NSYM;
        if (nDepth >= NID_MAX_TREE_DEPTH) return -1;
        if ((nCount <= 0) || (nStart < 0) || (nStart + nCount > NID_NSYM)) return -1;

        qint32 nSplit = nStart;
        quint32 nLeftWeight = 0;
        if ((nTotal >> 1) != 0) {
            qint32 i = 0;
            while (true) {
                ++nSplit;
                if (nStart + i + 1 > NID_NSYM) return -1;
                nLeftWeight += m_nWeight[nStart + i + 1];
                ++i;
                if (nLeftWeight >= (nTotal >> 1)) break;
            }
        }
        const qint32 nLeft = build(nStart, nSplit - nStart, nLeftWeight, nDepth + 1);
        if (nLeft < 0) return -1;
        if ((nSplit < 0) || (nSplit >= NID_NSYM)) return -1;
        m_nChild0[nSplit] = nLeft;
        const qint32 nRight = build(nSplit, (nCount + nStart) - nSplit, nTotal - nLeftWeight, nDepth + 1);
        if (nRight < 1) return -1;
        m_nChild1[nSplit] = nRight;
        return nSplit;
    }

    bool buildTree()
    {
        // A too-deep code forces a weight rescale and a retry; the reference
        // implementation loops here without a bound, but the weights strictly
        // shrink so a handful of rounds always settles it.
        for (qint32 nAttempt = 0; nAttempt < 32; nAttempt++) {
            const qint32 nRoot = build(0, NID_NSYM, m_nTotal, 0);
            m_nRoot = nRoot;
            if (nRoot >= 0) return true;
            rescale(0);
        }
        return false;
    }

    qint32 m_nMaxCount;
    qint32 m_nCounter;
    bool m_bUseList;
    qint32 m_nRoot;
    quint16 m_nTotal;
    quint16 m_nWeight[NID_NSYM + 2];
    quint16 m_nSymbolAt[NID_NSYM];
    quint16 m_nPositionOf[NID_NSYM];
    qint32 m_nChild0[NID_NSYM];
    qint32 m_nChild1[NID_NSYM];
};

bool nidDecodeBlock(const quint8 *pData, qint64 nSize, QByteArray *pbaOut, qint64 nRemaining)
{
    NidBits bits(pData, nSize);
    const qint32 nMagic = bits.bits(3);
    qint32 nMode = 0;
    if (nMagic == 6) {
        nMode = 1;
    } else if (nMagic == 7) {
        nMode = 2;
    } else {
        return false;
    }

    NidModel modelLength(3000);
    NidModel modelDistance(5000);
    NidModel modelLiteral(3000);

    QByteArray baRing(NID_RING_SIZE, char(0));
    quint8 *pRing = reinterpret_cast<quint8 *>(baRing.data());
    qint32 nPosition = 0;
    qint64 nProduced = 0;
    // A block never expands past what the member still owes; the guard keeps a
    // corrupt stream from growing the output without bound.
    const qint64 nLimit = (nRemaining > 0) ? nRemaining : 0;

    while (true) {
        const qint32 nSymbol = modelLength.decode(&bits);
        if (nSymbol < 0) return false;
        const qint32 nLength = nSymbol + 2;
        if (nLength > 0x101) {
            if (nLength == 0x102) break;
            return false;
        }
        if (nLength == 0x101) {
            const qint32 nByte = (nMode == 2) ? modelLiteral.decode(&bits) : bits.bits(8);
            if ((nByte < 0) || (nByte > 0xff)) return false;
            if (++nProduced > nLimit) return false;
            pRing[nPosition] = static_cast<quint8>(nByte);
            nPosition = (nPosition + 1) & NID_RING_MASK;
            if (nPosition == 0) pbaOut->append(baRing);
        } else {
            qint32 nHigh = 0;
            if (nLength > 2) {
                nHigh = modelDistance.decode(&bits);
                if (nHigh < 0) return false;
            }
            const qint32 nLow = bits.bits(8);
            if (nLow < 0) return false;
            qint32 nSource = (nPosition - (nHigh * 0x100 + nLow)) & NID_RING_MASK;
            for (qint32 i = 0; i < nLength; i++) {
                if (++nProduced > nLimit) return false;
                pRing[nPosition] = pRing[nSource];
                nSource = (nSource + 1) & NID_RING_MASK;
                nPosition = (nPosition + 1) & NID_RING_MASK;
                if (nPosition == 0) pbaOut->append(baRing);
            }
        }
    }
    if (nPosition) pbaOut->append(baRing.constData(), nPosition);
    return true;
}

}  // namespace

bool XNIDDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked || (nUncompressedSize < 0) || (nUncompressedSize > NID_MAX_UNPACKED_SIZE)) return false;
    pbaUnpacked->clear();
    if (nUncompressedSize == 0) return true;

    const quint8 *pData = reinterpret_cast<const quint8 *>(baPacked.constData());
    const qint64 nSize = baPacked.size();
    qint64 nPosition = 0;
    qint64 nRemaining = nUncompressedSize;

    while (nRemaining > 0) {
        if (nPosition + 5 > nSize) return false;
        const quint32 nFlags = static_cast<quint32>(pData[nPosition + 1]) | (static_cast<quint32>(pData[nPosition + 2]) << 8);
        const qint64 nBlockSize = static_cast<qint64>(pData[nPosition + 3]) | (static_cast<qint64>(pData[nPosition + 4]) << 8);
        const qint64 nBody = nPosition + 5;
        if (nBody + nBlockSize > nSize) return false;

        const qint64 nBefore = pbaUnpacked->size();
        if (nFlags & 0x800) {
            if (!nidDecodeBlock(pData + nBody, nBlockSize, pbaUnpacked, nRemaining)) return false;
        } else {
            pbaUnpacked->append(baPacked.constData() + nBody, static_cast<qint32>(nBlockSize));
        }
        const qint64 nProduced = pbaUnpacked->size() - nBefore;
        if (nProduced < 0) return false;
        nRemaining -= nProduced;
        nPosition = nBody + nBlockSize;
        if (nFlags & 0x100) break;
    }

    return (nRemaining == 0) && (pbaUnpacked->size() == nUncompressedSize);
}
