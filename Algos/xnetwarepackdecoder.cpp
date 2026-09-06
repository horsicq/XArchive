/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xnetwarepackdecoder.h"

#include <limits>
#include <new>

namespace {

// The pre-order table holds at most 256 nodes, so the binary decoding tree
// built from it holds at most 2 * 256 - 1 of them.  Three trees share one pool.
const qint32 NWP_MAX_NODES = (2 * XNetWarePackDecoder::MAX_TABLE_ENTRIES) - 1;
const qint32 NWP_POOL_SIZE = 3 * NWP_MAX_NODES;
const qint32 NWP_TREE_COUNT = 3;
// A code cannot be longer than the number of nodes on the path to a leaf.
const qint32 NWP_MAX_CODE_BITS = NWP_MAX_NODES;
// Nothing in either container is anywhere near this; the ceiling keeps the
// decode inside what a QByteArray can address.
const qint64 NWP_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(768) * 1024 * 1024;

// LSB-first bit reader.  Bytes are shifted in above whatever is already
// buffered and fields are taken from the bottom, so the first bit read is the
// least significant bit of the value.
struct NwpBitReader {
    const quint8 *pData;
    qint64 nSize;
    qint64 nPosition;
    quint32 nAccumulator;
    qint32 nBitCount;
    bool bStarved;

    void init(const quint8 *pInput, qint64 nInputSize)
    {
        pData = pInput;
        nSize = nInputSize;
        nPosition = 0;
        nAccumulator = 0;
        nBitCount = 0;
        bStarved = false;
    }

    bool read(qint32 nBits, quint32 *pnValue)
    {
        while (nBitCount < nBits) {
            if (nPosition >= nSize) {
                bStarved = true;
                return false;
            }
            nAccumulator |= static_cast<quint32>(pData[nPosition]) << nBitCount;
            nPosition++;
            nBitCount += 8;
        }

        *pnValue = nAccumulator & ((static_cast<quint32>(1) << nBits) - 1);
        nAccumulator >>= nBits;
        nBitCount -= nBits;

        return true;
    }
};

struct NwpTableEntry {
    quint8 nSymbol;
    quint8 nDepth;
};

struct NwpNode {
    qint32 nLeft;
    qint32 nRight;
    quint8 nSymbol;
    bool bLeaf;
};

struct NwpTrees {
    NwpNode nodes[NWP_POOL_SIZE];
    qint32 nNodeCount;
    qint32 nRoot[NWP_TREE_COUNT];

    void init()
    {
        nNodeCount = 0;
        for (qint32 i = 0; i < NWP_TREE_COUNT; i++) nRoot[i] = -1;
    }

    qint32 allocate()
    {
        if (nNodeCount >= NWP_POOL_SIZE) return -1;
        const qint32 nIndex = nNodeCount;
        nNodeCount++;
        nodes[nIndex].nLeft = -1;
        nodes[nIndex].nRight = -1;
        nodes[nIndex].nSymbol = 0;
        nodes[nIndex].bLeaf = false;
        return nIndex;
    }
};

// Pre-order description reader.  nCount siblings sit at nDepth; each one is a
// unary child count, an 8-bit symbol, and then its own children one level down.
bool nwpReadTable(NwpBitReader *pReader, NwpTableEntry *pTable, qint32 *pnCount, qint32 nCount, qint32 nDepth)
{
    // The 256-entry ceiling bounds the recursion, but a depth that no longer
    // fits the stored byte could never be matched by the builder anyway.
    if ((nCount < 0) || (nDepth > 0xFF)) return false;

    for (qint32 i = 0; i < nCount; i++) {
        qint32 nChildren = 0;

        for (;;) {
            quint32 nBit = 0;
            if (!pReader->read(1, &nBit)) return false;
            if (nBit) break;
            nChildren++;
            // No node can have more children than the table can hold entries.
            if (nChildren > XNetWarePackDecoder::MAX_TABLE_ENTRIES) return false;
        }

        quint32 nSymbol = 0;
        if (!pReader->read(8, &nSymbol)) return false;

        if (*pnCount >= XNetWarePackDecoder::MAX_TABLE_ENTRIES) return false;
        pTable[*pnCount].nSymbol = static_cast<quint8>(nSymbol);
        pTable[*pnCount].nDepth = static_cast<quint8>(nDepth);
        (*pnCount)++;

        if (!nwpReadTable(pReader, pTable, pnCount, nChildren, nDepth + 1)) return false;
    }

    return true;
}

// Turns the pre-order (symbol, depth) list into the binary decoding tree.  A
// range is split at the LAST entry whose depth equals the current level: that
// entry and everything after it become the 1 branch one level deeper, and
// everything before it stays on the 0 branch at the same level.  A single
// remaining entry is the leaf that carries its symbol.
bool nwpBuildTree(NwpTrees *pTrees, qint32 nNode, const NwpTableEntry *pTable, qint32 nStart, qint32 nCount, qint32 nDepth)
{
    if ((nNode < 0) || (nCount <= 0)) return false;

    if (nCount == 1) {
        pTrees->nodes[nNode].bLeaf = true;
        pTrees->nodes[nNode].nSymbol = pTable[nStart].nSymbol;
        return true;
    }

    // Depths past a byte cannot appear in the table, so a range that deep is a
    // malformed description rather than a tree with an unreachable branch.
    if (nDepth > 0xFF) return false;

    for (qint32 i = nStart + nCount - 1; i >= nStart; i--) {
        if (static_cast<qint32>(pTable[i].nDepth) != nDepth) continue;

        // A split that leaves nothing on the 0 branch would build a node with
        // no children at all; refuse it instead of publishing a tree that dies
        // only if that branch happens to be taken.
        if (i <= nStart) return false;

        const qint32 nLeft = pTrees->allocate();
        const qint32 nRight = pTrees->allocate();
        if ((nLeft < 0) || (nRight < 0)) return false;

        pTrees->nodes[nNode].nLeft = nLeft;
        pTrees->nodes[nNode].nRight = nRight;

        if (!nwpBuildTree(pTrees, nLeft, pTable, nStart, i - nStart, nDepth)) return false;

        return nwpBuildTree(pTrees, nRight, pTable, i, (nStart + nCount) - i, nDepth + 1);
    }

    return false;
}

bool nwpReadTree(NwpBitReader *pReader, NwpTrees *pTrees, qint32 nTreeIndex, bool bStrictTables)
{
    NwpTableEntry table[XNetWarePackDecoder::MAX_TABLE_ENTRIES];
    qint32 nCount = 0;

    // One root node at depth 0; everything else hangs off it.
    if (!nwpReadTable(pReader, table, &nCount, 1, 0)) return false;
    if (nCount <= 0) return false;

    // A code table cannot give one symbol two codes, and no stream in the
    // 4250-file reference corpus does.  Random bytes parsed as a tree almost
    // always do, which is what makes this the useful detection gate; the
    // decode path deliberately does NOT enforce it, so a stream the original
    // tool would read is never lost to a rule the original tool never had.
    if (bStrictTables) {
        bool bSeen[256];
        for (qint32 i = 0; i < 256; i++) bSeen[i] = false;
        for (qint32 i = 0; i < nCount; i++) {
            if (bSeen[table[i].nSymbol]) return false;
            bSeen[table[i].nSymbol] = true;
        }
    }

    const qint32 nRoot = pTrees->allocate();
    if (nRoot < 0) return false;
    pTrees->nRoot[nTreeIndex] = nRoot;

    // The builder starts one level below the root entry's own depth.
    return nwpBuildTree(pTrees, nRoot, table, 0, nCount, 1);
}

bool nwpDecodeSymbol(NwpBitReader *pReader, const NwpTrees *pTrees, qint32 nTreeIndex, quint32 *pnSymbol)
{
    qint32 nNode = pTrees->nRoot[nTreeIndex];

    for (qint32 nStep = 0; nStep <= NWP_MAX_CODE_BITS; nStep++) {
        if (nNode < 0) return false;

        if (pTrees->nodes[nNode].bLeaf) {
            *pnSymbol = pTrees->nodes[nNode].nSymbol;
            return true;
        }

        quint32 nBit = 0;
        if (!pReader->read(1, &nBit)) return false;

        nNode = nBit ? pTrees->nodes[nNode].nRight : pTrees->nodes[nNode].nLeft;
    }

    return false;
}

// Shared walk.  pbaUnpacked == nullptr runs the probe: nothing is materialised
// beyond the ring window and starvation is reported through pbStarved so the
// caller can tell a truncated sample from a broken stream.
bool nwpRun(const QByteArray &baPacked, qint64 nOutputSize, QByteArray *pbaUnpacked, bool *pbStarved, bool bStrictTables)
{
    if (pbStarved) *pbStarved = false;
    if ((nOutputSize < 0) || (nOutputSize > NWP_MAX_UNCOMPRESSED_SIZE)) return false;
    if (nOutputSize > static_cast<qint64>((std::numeric_limits<qint32>::max)())) return false;

    if (pbaUnpacked) {
        pbaUnpacked->clear();
        pbaUnpacked->reserve(static_cast<qint32>(nOutputSize));
    }

    if (baPacked.isEmpty()) return false;

    // Note there is deliberately no early exit for nOutputSize == 0: even a
    // zero-length member has to carry three parsable trees, and returning
    // success before reading them would report OK on bytes never looked at.

    NwpBitReader reader;
    reader.init(reinterpret_cast<const quint8 *>(baPacked.constData()), baPacked.size());

    NwpTrees *pTrees = new (std::nothrow) NwpTrees;
    if (!pTrees) return false;
    pTrees->init();

    bool bResult = true;
    for (qint32 i = 0; bResult && (i < NWP_TREE_COUNT); i++) {
        bResult = nwpReadTree(&reader, pTrees, i, bStrictTables);
    }

    QByteArray baWindow;
    if (bResult) {
        baWindow = QByteArray(XNetWarePackDecoder::WINDOW_SIZE, char(0));
        bResult = (baWindow.size() == XNetWarePackDecoder::WINDOW_SIZE);
    }

    if (bResult) {
        quint8 *pWindow = reinterpret_cast<quint8 *>(baWindow.data());
        qint32 nPosition = 0;
        qint64 nRemaining = nOutputSize;

        // Every iteration consumes at least the flag bit, so the bounded input
        // is what terminates this loop; no separate iteration budget is needed.
        while (bResult && (nRemaining > 0)) {
            quint32 nFlag = 0;
            if (!reader.read(1, &nFlag)) {
                bResult = false;
                break;
            }

            if (nFlag) {
                quint32 nSymbol = 0;
                if (!nwpDecodeSymbol(&reader, pTrees, 0, &nSymbol)) {
                    bResult = false;
                    break;
                }

                pWindow[nPosition] = static_cast<quint8>(nSymbol);
                nPosition = (nPosition + 1) & (XNetWarePackDecoder::WINDOW_SIZE - 1);
                if (pbaUnpacked) pbaUnpacked->append(static_cast<char>(nSymbol));
                nRemaining--;
            } else {
                quint32 nLength = 0;
                if (!nwpDecodeSymbol(&reader, pTrees, 1, &nLength)) {
                    bResult = false;
                    break;
                }
                if (nLength == XNetWarePackDecoder::LENGTH_ESCAPE) {
                    if (!reader.read(XNetWarePackDecoder::ESCAPE_LENGTH_BITS, &nLength)) {
                        bResult = false;
                        break;
                    }
                }

                quint32 nLow = 0;
                if (!reader.read(XNetWarePackDecoder::DISTANCE_LOW_BITS, &nLow)) {
                    bResult = false;
                    break;
                }

                quint32 nHigh = 0;
                if (!nwpDecodeSymbol(&reader, pTrees, 2, &nHigh)) {
                    bResult = false;
                    break;
                }

                // A zero-length token would make no progress at all; no stream
                // in either corpus emits one, and accepting it here would only
                // spin until the input ran out.
                if (nLength == 0) {
                    bResult = false;
                    break;
                }

                const quint32 nDistance = nLow + (nHigh * 32U);

                // Distance 0 aims the copy source at the write pointer itself,
                // so every byte "copied" is the stale window cell it is about
                // to overwrite: the walk still terminates and still produces
                // exactly the declared length, but the bytes are invented.
                // None of the 4250 reference streams emits it, and a decoder
                // that accepts it hands the caller a wrong file at a success
                // exit code - the one failure this class must not have.
                if (nDistance == 0) {
                    bResult = false;
                    break;
                }

                qint32 nSource = static_cast<qint32>((static_cast<quint32>(nPosition) - nDistance) & (XNetWarePackDecoder::WINDOW_SIZE - 1));

                qint64 nCopy = static_cast<qint64>(nLength);
                if (nCopy > nRemaining) nCopy = nRemaining;

                for (qint64 i = 0; i < nCopy; i++) {
                    const quint8 nByte = pWindow[nSource];
                    pWindow[nPosition] = nByte;
                    nPosition = (nPosition + 1) & (XNetWarePackDecoder::WINDOW_SIZE - 1);
                    nSource = (nSource + 1) & (XNetWarePackDecoder::WINDOW_SIZE - 1);
                    if (pbaUnpacked) pbaUnpacked->append(static_cast<char>(nByte));
                    nRemaining--;
                }
            }
        }

        if (bResult && (nRemaining != 0)) bResult = false;
    }

    if (pbStarved) *pbStarved = reader.bStarved;

    delete pTrees;

    if (!bResult && pbaUnpacked) pbaUnpacked->clear();

    return bResult;
}

}  // namespace

bool XNetWarePackDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) return false;

    if (!nwpRun(baPacked, nUncompressedSize, pbaUnpacked, nullptr, false)) {
        pbaUnpacked->clear();
        return false;
    }

    // The declared size is the only end-of-stream signal there is; publishing a
    // buffer that does not match it exactly would be publishing a guess.
    if (static_cast<qint64>(pbaUnpacked->size()) != nUncompressedSize) {
        pbaUnpacked->clear();
        return false;
    }

    return true;
}

bool XNetWarePackDecoder::probe(const QByteArray &baSample, qint64 nUncompressedSize, bool bComplete, qint64 nMaxOutput)
{
    if ((nUncompressedSize < 0) || (nMaxOutput <= 0)) return false;
    if (baSample.isEmpty()) return false;

    qint64 nTarget = nUncompressedSize;
    bool bTruncatedTarget = false;
    if (nTarget > nMaxOutput) {
        nTarget = nMaxOutput;
        bTruncatedTarget = true;
    }

    bool bStarved = false;
    if (nwpRun(baSample, nTarget, nullptr, &bStarved, true)) return true;

    // Only a starved reader is forgiven, and only when the caller admitted the
    // walk could not have finished: the sample is a prefix of the stream, or
    // the target was capped below the declared size.  A grammar failure with
    // input still in hand is a reject.
    return bStarved && (!bComplete || bTruncatedTarget);
}
