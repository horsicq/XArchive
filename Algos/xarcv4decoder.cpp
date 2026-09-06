/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarcv4decoder.h"

#include <limits>
#include <vector>

namespace {
// 256 literals + the end marker + 6 distance buckets * 498 lengths.
const qint32 ARCV4_SYMBOLS = 3245;                       // 0x0CAD
const qint32 ARCV4_NODES = ARCV4_SYMBOLS * 2 - 1;        // 6489
const qint32 ARCV4_ROOT = 1;
const qint32 ARCV4_END_SYMBOL = 256;
const qint32 ARCV4_FIRST_MATCH = 257;
const qint32 ARCV4_LENGTH_SPAN = 498;  // 0x1F2 lengths per distance bucket
const qint32 ARCV4_MIN_LENGTH = 3;
const qint32 ARCV4_BUCKETS = 6;
// The writer halves every weight the moment the root reaches this value.
const quint16 ARCV4_MAX_ROOT_FREQ = 2000U;

// Only two of the three arrays are indexed by every node: the child arrays are
// written for the internal nodes alone.  They are still sized for the whole
// tree because a restructuring moves leaves into internal slots.
struct ARCV4Model {
    std::vector<quint16> freq;
    std::vector<quint16> parent;
    std::vector<quint16> child0;
    std::vector<quint16> child1;

    ARCV4Model()
        : freq(ARCV4_NODES + 1, 0),
          parent(ARCV4_NODES + 1, 0),
          child0(ARCV4_NODES + 1, 0),
          child1(ARCV4_NODES + 1, 0)
    {
        // Weight 1 on EVERY node from 2 up, internal ones included; the root
        // is left alone because the first update overwrites it before anything
        // reads it.
        for (qint32 i = 2; i <= ARCV4_NODES; ++i) {
            freq[i] = 1U;
            parent[i] = quint16(i >> 1);
        }
        for (qint32 i = 1; i < ARCV4_SYMBOLS; ++i) {
            child0[i] = quint16(i * 2);
            child1[i] = quint16(i * 2 + 1);
        }
    }
};

class ARCV4BitReader {
public:
    ARCV4BitReader(const uchar *pData, qint64 nSize)
        : m_pData(pData), m_nSize(nSize) {}

    // -1 on end of input.  Bits leave each byte LSB first.
    qint32 readBit()
    {
        if (m_nBitCount == 0) {
            if (m_nPosition >= m_nSize) return -1;
            m_nBuffer = m_pData[m_nPosition++];
            m_nBitCount = 7;
        } else {
            --m_nBitCount;
        }
        const qint32 nBit = qint32(m_nBuffer & 1U);
        m_nBuffer = quint8(m_nBuffer >> 1);
        return nBit;
    }

    // -1 on end of input; the value is assembled LSB first as well.
    qint32 readBits(qint32 nCount)
    {
        qint32 nValue = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            const qint32 nBit = readBit();
            if (nBit < 0) return -1;
            if (nBit) nValue |= (1 << i);
        }
        return nValue;
    }

private:
    const uchar *m_pData = nullptr;
    qint64 m_nSize = 0;
    qint64 m_nPosition = 0;
    quint8 m_nBuffer = 0;
    qint32 m_nBitCount = 0;
};

// Recomputes the weights from nNode's parent up to the root, each one as the
// sum of its two children, then applies the global halving when the root hits
// the limit.  nSibling is nNode's sibling; the caller has it in hand already.
// Returns false only if the parent chain fails to reach the root, which cannot
// happen on a well-formed tree and means the model is corrupt.
bool arcv4Propagate(ARCV4Model *pModel, qint32 nNode, qint32 nSibling)
{
    for (qint32 nStep = 0; nStep <= ARCV4_NODES; ++nStep) {
        const qint32 nChild = nNode;
        nNode = pModel->parent[nNode];
        if (nNode < ARCV4_ROOT || nNode > ARCV4_NODES) return false;
        pModel->freq[nNode] =
            quint16(pModel->freq[nChild] + pModel->freq[nSibling]);
        if (nNode == ARCV4_ROOT) {
            if (pModel->freq[ARCV4_ROOT] == ARCV4_MAX_ROOT_FREQ) {
                for (qint32 i = ARCV4_ROOT; i <= ARCV4_NODES; ++i) {
                    pModel->freq[i] = quint16(pModel->freq[i] >> 1);
                }
            }
            return true;
        }
        const qint32 nGrand = pModel->parent[nNode];
        if (nGrand < ARCV4_ROOT || nGrand > ARCV4_NODES) return false;
        nSibling = pModel->child0[nGrand];
        if (nSibling == nNode) nSibling = pModel->child1[nGrand];
        if (nSibling < ARCV4_ROOT || nSibling > ARCV4_NODES) return false;
    }
    return false;
}

// The adaptive step: bump the decoded leaf, fix the weights along its path,
// then climb swapping the node with its uncle whenever the uncle is lighter.
bool arcv4Update(ARCV4Model *pModel, qint32 nNode)
{
    pModel->freq[nNode] = quint16(pModel->freq[nNode] + 1U);
    qint32 nParent = pModel->parent[nNode];
    if (nParent < ARCV4_ROOT || nParent > ARCV4_NODES) return false;
    if (nParent == ARCV4_ROOT) return true;

    qint32 nSibling = pModel->child0[nParent];
    if (nSibling == nNode) nSibling = pModel->child1[nParent];
    if (nSibling < ARCV4_ROOT || nSibling > ARCV4_NODES) return false;
    if (!arcv4Propagate(pModel, nNode, nSibling)) return false;

    for (qint32 nStep = 0; nStep <= ARCV4_NODES; ++nStep) {
        const qint32 nGrand = pModel->parent[nParent];
        if (nGrand < ARCV4_ROOT || nGrand > ARCV4_NODES) return false;
        const qint32 nLeft = pModel->child0[nGrand];
        qint32 nUncle = nLeft;
        if (nParent == nLeft) nUncle = pModel->child1[nGrand];
        if (nUncle < ARCV4_ROOT || nUncle > ARCV4_NODES) return false;

        if (pModel->freq[nUncle] < pModel->freq[nNode]) {
            // The uncle slot under the grandparent takes over the node...
            if (nParent == nLeft) {
                pModel->child1[nGrand] = quint16(nNode);
            } else {
                pModel->child0[nGrand] = quint16(nNode);
            }
            // ...and the node's own slot under the parent takes the uncle.
            qint32 nNodeSibling = pModel->child0[nParent];
            if (nNode == nNodeSibling) {
                nNodeSibling = pModel->child1[nParent];
                pModel->child0[nParent] = quint16(nUncle);
            } else {
                pModel->child1[nParent] = quint16(nUncle);
            }
            if (nNodeSibling < ARCV4_ROOT || nNodeSibling > ARCV4_NODES) {
                return false;
            }
            pModel->parent[nUncle] = quint16(nParent);
            pModel->parent[nNode] = quint16(nGrand);
            if (!arcv4Propagate(pModel, nUncle, nNodeSibling)) return false;
            nNode = nUncle;
        }

        nNode = pModel->parent[nNode];
        if (nNode < ARCV4_ROOT || nNode > ARCV4_NODES) return false;
        nParent = pModel->parent[nNode];
        if (nParent < ARCV4_ROOT || nParent > ARCV4_NODES) return false;
        if (nParent == ARCV4_ROOT) return true;
    }
    return false;
}

// Walks the tree bit by bit; -1 on end of input, -2 on a corrupt model.
qint32 arcv4DecodeSymbol(ARCV4Model *pModel, ARCV4BitReader *pReader)
{
    qint32 nNode = ARCV4_ROOT;
    for (qint32 nStep = 0; nStep <= ARCV4_NODES; ++nStep) {
        const qint32 nBit = pReader->readBit();
        if (nBit < 0) return -1;
        nNode = (nBit == 0) ? pModel->child0[nNode] : pModel->child1[nNode];
        if (nNode < ARCV4_ROOT || nNode > ARCV4_NODES) return -2;
        if (nNode >= ARCV4_SYMBOLS) {
            if (!arcv4Update(pModel, nNode)) return -2;
            return nNode - ARCV4_SYMBOLS;
        }
    }
    return -2;
}
}  // namespace

bool XARCV4Decoder::decode(const QByteArray &packed, qint64 nUncompressedSize,
                           QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || (nUncompressedSize < 0) ||
        (nUncompressedSize > qint64((std::numeric_limits<qint32>::max)())) ||
        packed.isEmpty()) {
        return false;
    }

    qint32 nExtraBits[ARCV4_BUCKETS] = {};
    qint32 nBaseDistance[ARCV4_BUCKETS] = {};
    qint32 nAccumulator = 0;
    for (qint32 i = 0; i < ARCV4_BUCKETS; ++i) {
        nExtraBits[i] = 2 * i + 4;
        // The writer keeps this table in 16-bit slots; the last base is 5456,
        // so nothing ever wraps, but mirror the width anyway.
        nBaseDistance[i] = qint32(quint16(nAccumulator));
        nAccumulator += (1 << nExtraBits[i]);
    }

    ARCV4Model model;
    ARCV4BitReader reader(reinterpret_cast<const uchar *>(packed.constData()),
                          packed.size());

    // The declared size is exact, so the result is allocated once up front and
    // filled through a raw pointer; nothing here may write past nProduced.
    const qint32 nTargetSize = qint32(nUncompressedSize);
    QByteArray baResult(nTargetSize, 0);
    char *pResult = baResult.data();
    qint32 nProduced = 0;

    for (;;) {
        if (((nProduced & 0x3fff) == 0) &&
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint32 nSymbol = arcv4DecodeSymbol(&model, &reader);
        if (nSymbol < 0) return false;
        if (nSymbol == ARCV4_END_SYMBOL) break;
        if (nSymbol < ARCV4_END_SYMBOL) {
            if (nProduced >= nTargetSize) return false;
            pResult[nProduced++] = char(quint8(nSymbol));
            continue;
        }

        const qint32 nCode = nSymbol - ARCV4_FIRST_MATCH;
        const qint32 nLength = nCode % ARCV4_LENGTH_SPAN + ARCV4_MIN_LENGTH;
        const qint32 nBucket = nCode / ARCV4_LENGTH_SPAN;
        if ((nBucket < 0) || (nBucket >= ARCV4_BUCKETS)) return false;
        const qint32 nExtra = reader.readBits(nExtraBits[nBucket]);
        if (nExtra < 0) return false;
        // The length is part of the encoded distance, so a match can never
        // read a byte it is about to write.
        const qint32 nDistance = nBaseDistance[nBucket] + nExtra + nLength;
        const qint32 nSource = nProduced - nDistance;
        if ((nSource < 0) || (nLength > nTargetSize - nProduced)) return false;
        for (qint32 i = 0; i < nLength; ++i) {
            pResult[nProduced + i] = pResult[nSource + i];
        }
        nProduced += nLength;
    }

    if (nProduced != nTargetSize) return false;
    *pOutput = baResult;
    return true;
}
