/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsqdecoder.h"

#include <vector>

namespace {
const qint32 SQ_SYMBOL_COUNT = 0x275;                    // 629
const qint32 SQ_NODE_COUNT = SQ_SYMBOL_COUNT * 2;        // 0x4EA, nodes 1..0x4E9
const qint32 SQ_ROOT = 1;
const qint32 SQ_END_SYMBOL = 0x100;
const qint32 SQ_FIRST_MATCH = 0x101;
const qint32 SQ_LENGTH_COUNT = 0x3E;                     // 62 lengths, 3..64
const qint32 SQ_SLOT_COUNT = 6;
const qint32 SQ_WINDOW_SIZE = 0x8000;
const qint32 SQ_WINDOW_MASK = SQ_WINDOW_SIZE - 1;
const quint16 SQ_REBALANCE_WEIGHT = 2000;

class SqStream {
public:
    explicit SqStream(const QByteArray &baPacked)
        : m_pData(reinterpret_cast<const quint8 *>(baPacked.constData())), m_nSize(baPacked.size()), m_nPosition(0), m_nBitBuffer(0), m_nBitCount(0)
    {
        m_freq.assign(SQ_NODE_COUNT, 0);
        m_parent.assign(SQ_NODE_COUNT, 0);
        m_left.assign(SQ_NODE_COUNT, 0);
        m_right.assign(SQ_NODE_COUNT, 0);
        for (qint32 i = 2; i < SQ_NODE_COUNT; i++) {
            m_freq[i] = 1;
            m_parent[i] = i >> 1;
        }
        for (qint32 i = 1; i < SQ_SYMBOL_COUNT; i++) {
            m_left[i] = i * 2;
            m_right[i] = i * 2 + 1;
        }
        qint32 nBase = 0;
        for (qint32 i = 0; i < SQ_SLOT_COUNT; i++) {
            m_extraBits[i] = i * 2 + 4;
            m_base[i] = nBase;
            nBase += (1 << m_extraBits[i]);
        }
    }

    qint32 getBit()
    {
        if (m_nBitCount == 0) {
            if (m_nPosition >= m_nSize) return -1;
            m_nBitBuffer = m_pData[m_nPosition++];
            m_nBitCount = 7;
        } else {
            m_nBitCount--;
        }
        const qint32 nBit = (m_nBitBuffer >> 7) & 1;
        m_nBitBuffer = static_cast<quint8>(m_nBitBuffer << 1);
        return nBit;
    }

    qint32 getBits(qint32 nCount)
    {
        qint32 nResult = 0;
        qint32 nMask = 1;
        for (qint32 i = 0; i < nCount; i++) {
            const qint32 nBit = getBit();
            if (nBit < 0) return -1;
            if (nBit) nResult |= nMask;
            nMask <<= 1;
        }
        return nResult;
    }

    // Decodes one symbol and lets the model absorb it.  Returns -1 on input
    // exhaustion and -2 when the tree walk leaves the valid node range, which
    // corrupt input can otherwise turn into an out-of-bounds read.
    qint32 decodeSymbol()
    {
        qint32 nNode = SQ_ROOT;
        for (qint32 nSteps = 0; nNode < SQ_SYMBOL_COUNT; nSteps++) {
            if (nSteps > SQ_NODE_COUNT) return -2;
            const qint32 nBit = getBit();
            if (nBit < 0) return -1;
            nNode = (nBit == 0) ? m_left[nNode] : m_right[nNode];
            if ((nNode < 2) || (nNode >= SQ_NODE_COUNT)) return -2;
        }
        update(nNode);
        return nNode - SQ_SYMBOL_COUNT;
    }

    qint32 extraBits(qint32 nSlot) const
    {
        return m_extraBits[nSlot];
    }
    qint32 base(qint32 nSlot) const
    {
        return m_base[nSlot];
    }

private:
    // Recomputes the weights from nNode up to the root, then halves everything
    // once the root reaches SQ_REBALANCE_WEIGHT.
    void propagate(qint32 nNode, qint32 nSibling)
    {
        for (qint32 nSteps = 0; nSteps <= SQ_NODE_COUNT; nSteps++) {
            if ((nNode < 2) || (nNode >= SQ_NODE_COUNT) || (nSibling < 1) || (nSibling >= SQ_NODE_COUNT)) return;
            const qint32 nChild = nNode;
            nNode = m_parent[nNode];
            if ((nNode < 1) || (nNode >= SQ_NODE_COUNT)) return;
            m_freq[nNode] = static_cast<quint16>(m_freq[nChild] + m_freq[nSibling]);
            if (nNode == SQ_ROOT) break;
            const qint32 nGrandParent = m_parent[nNode];
            if ((nGrandParent < 1) || (nGrandParent >= SQ_NODE_COUNT)) return;
            nSibling = m_left[nGrandParent];
            if (nSibling == nNode) nSibling = m_right[nGrandParent];
        }

        if (m_freq[SQ_ROOT] == SQ_REBALANCE_WEIGHT) {
            for (qint32 i = 1; i < SQ_NODE_COUNT; i++) m_freq[i] = static_cast<quint16>(m_freq[i] >> 1);
        }
    }

    void update(qint32 nLeaf)
    {
        m_freq[nLeaf] = static_cast<quint16>(m_freq[nLeaf] + 1);
        qint32 nParent = m_parent[nLeaf];
        if (nParent == SQ_ROOT) return;
        if ((nParent < 1) || (nParent >= SQ_NODE_COUNT)) return;

        qint32 nSibling = m_left[nParent];
        if (nLeaf == nSibling) nSibling = m_right[nParent];
        propagate(nLeaf, nSibling);

        qint32 nNode = nLeaf;
        qint32 nUp = nParent;
        for (qint32 nSteps = 0; nSteps <= SQ_NODE_COUNT; nSteps++) {
            if ((nUp < 1) || (nUp >= SQ_NODE_COUNT)) return;
            const qint32 nGrandParent = m_parent[nUp];
            if ((nGrandParent < 1) || (nGrandParent >= SQ_NODE_COUNT)) return;
            const qint32 nGrandLeft = m_left[nGrandParent];
            qint32 nUncle = nGrandLeft;
            if (nUp == nUncle) nUncle = m_right[nGrandParent];
            if ((nUncle < 2) || (nUncle >= SQ_NODE_COUNT)) return;

            if (m_freq[nUncle] < m_freq[nNode]) {
                // Lift the heavier node one level by exchanging it with its
                // lighter uncle.
                if (nUp == nGrandLeft) {
                    m_right[nGrandParent] = nNode;
                } else {
                    m_left[nGrandParent] = nNode;
                }
                qint32 nOther = m_left[nUp];
                if (nNode == nOther) {
                    nOther = m_right[nUp];
                    m_left[nUp] = nUncle;
                } else {
                    m_right[nUp] = nUncle;
                }
                m_parent[nUncle] = nUp;
                m_parent[nNode] = nGrandParent;
                if ((nOther < 1) || (nOther >= SQ_NODE_COUNT)) return;
                propagate(nUncle, nOther);
                nNode = nUncle;
            }

            if ((nNode < 2) || (nNode >= SQ_NODE_COUNT)) return;
            nNode = m_parent[nNode];
            if ((nNode < 1) || (nNode >= SQ_NODE_COUNT)) return;
            nUp = m_parent[nNode];
            if (nUp == SQ_ROOT) return;
        }
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint8 m_nBitBuffer;
    qint32 m_nBitCount;
    std::vector<quint16> m_freq;
    std::vector<qint32> m_parent;
    std::vector<qint32> m_left;
    std::vector<qint32> m_right;
    qint32 m_extraBits[SQ_SLOT_COUNT];
    qint32 m_base[SQ_SLOT_COUNT];
};

// One shared engine: pbaUnpacked == nullptr only measures.  nLimit is the hard
// output ceiling and is always enforced.
bool sqRun(const QByteArray &baPacked, qint64 nLimit, QByteArray *pbaUnpacked, qint64 *pnProduced, XBinary::PDSTRUCT *pPdStruct)
{
    if ((nLimit < 0) || !pnProduced) return false;
    if (baPacked.isEmpty()) return false;

    SqStream stream(baPacked);
    std::vector<quint8> window(SQ_WINDOW_SIZE, 0);
    qint32 nCursor = 0;
    qint64 nProduced = 0;
    qint32 nGuard = 0;

    for (;;) {
        if ((++nGuard & 0xFFFF) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        const qint32 nSymbol = stream.decodeSymbol();
        if (nSymbol < 0) return false;
        if (nSymbol == SQ_END_SYMBOL) break;

        if (nSymbol < SQ_END_SYMBOL) {
            if (nProduced >= nLimit) return false;
            window[nCursor] = static_cast<quint8>(nSymbol);
            nCursor = (nCursor + 1) & SQ_WINDOW_MASK;
            nProduced++;
            if (nCursor == 0) {
                if (pbaUnpacked) pbaUnpacked->append(reinterpret_cast<const char *>(&window[0]), SQ_WINDOW_SIZE);
            }
        } else {
            const qint32 nCode = nSymbol - SQ_FIRST_MATCH;
            const qint32 nLength = (nCode % SQ_LENGTH_COUNT) + 3;
            const qint32 nSlot = nCode / SQ_LENGTH_COUNT;
            if ((nSlot < 0) || (nSlot >= SQ_SLOT_COUNT)) return false;
            const qint32 nExtra = stream.getBits(stream.extraBits(nSlot));
            if (nExtra < 0) return false;
            if (nProduced > nLimit - nLength) return false;

            qint32 nSource = (nCursor - (nExtra + nLength + stream.base(nSlot))) & SQ_WINDOW_MASK;
            for (qint32 i = 0; i < nLength; i++) {
                window[nCursor] = window[nSource];
                nSource = (nSource + 1) & SQ_WINDOW_MASK;
                nCursor = (nCursor + 1) & SQ_WINDOW_MASK;
                nProduced++;
                if (nCursor == 0) {
                    if (pbaUnpacked) pbaUnpacked->append(reinterpret_cast<const char *>(&window[0]), SQ_WINDOW_SIZE);
                }
            }
        }
    }

    if (nCursor != 0) {
        if (pbaUnpacked) pbaUnpacked->append(reinterpret_cast<const char *>(&window[0]), nCursor);
    }
    *pnProduced = nProduced;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
}  // namespace

bool XSQDecoder::decode(const QByteArray &baPacked, qint64 nRawSize, QByteArray *pbaUnpacked, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUnpacked || (nRawSize < 0) || (nRawSize > 0x7FFFFFFF)) return false;
    pbaUnpacked->clear();
    qint64 nProduced = 0;
    if (!sqRun(baPacked, nRawSize, pbaUnpacked, &nProduced, pPdStruct)) return false;
    return (nProduced == nRawSize) && (static_cast<qint64>(pbaUnpacked->size()) == nRawSize);
}

bool XSQDecoder::measure(const QByteArray &baPacked, qint64 nMaxSize, qint64 *pnRawSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnRawSize || (nMaxSize < 0)) return false;
    qint64 nProduced = 0;
    if (!sqRun(baPacked, nMaxSize, nullptr, &nProduced, pPdStruct)) return false;
    *pnRawSize = nProduced;
    return true;
}
