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
#include "xash0decoder.h"

#include <QVector>

namespace {
const qint64 ASH0_HEADER_SIZE = 12;
// Header plus one symbol word plus one distance word.
const qint64 ASH0_MIN_SIZE = 20;
const qint64 ASH0_SYM_STREAM_OFFSET = 12;
const qint64 ASH0_MIN_DIST_OFFSET = 16;
const qint64 ASH0_WORD_SIZE = 4;
const qint32 ASH0_SYM_BITS = 9;
const qint32 ASH0_DIST_BITS_DEFAULT = 11;
const qint32 ASH0_DIST_BITS_RANCH = 15;
// Widths the tables can be sized for; the format only ever uses 9 / 11 / 15.
const qint32 ASH0_MIN_TREE_BITS = 1;
const qint32 ASH0_MAX_TREE_BITS = 16;
const quint32 ASH0_SIZE_MASK = 0x00FFFFFFU;
// The size field is 24 bits, so this is inherent; kept as a named constant.
const qint64 ASH0_MAX_UNCOMPRESSED_SIZE = 0x00FFFFFF;
// Decompression-bomb guard.  Two-leaf trees make a 258-byte match cost one
// symbol bit plus one distance bit, i.e. 1032 plaintext bytes per packed byte;
// 16 MiB - 1 of zeros measures 1030.5.  One-leaf (zero-bit) trees would be
// unbounded, which is why the tree reader rejects them.
const qint64 ASH0_MAX_RATIO = 1032;
const qint64 ASH0_RATIO_SLACK = 4096;
const quint32 ASH0_LITERAL_LIMIT = 0x100;
const qint64 ASH0_MIN_MATCH = 3;
const quint32 ASH0_FLAG_RIGHT = 0x80000000U;
const quint32 ASH0_FLAG_LEFT = 0x40000000U;
const quint32 ASH0_INDEX_MASK = 0x3FFFFFFFU;
const qint32 ASH0_CANCEL_MASK = 0x3FFF;
// A decode is tight when the distance reader ended inside the last word.
const qint64 ASH0_TIGHT_SLACK_BYTES = 4;
// A Huffman tree names each symbol once and ashcomp pads with DISTINCT unused
// symbols.  Relax this if a real Nintendo sample ever shows a duplicate.
const bool ASH0_REJECT_DUPLICATE_LEAF = true;

// Lazy MSB-first bit reader over one stream of the packed buffer.
struct BITREADER {
    const quint8 *pData;
    qint64 nStart;   // first byte of this stream
    qint64 nLimit;   // one past the last byte this stream may touch
    qint64 nBitPos;  // bits consumed so far, from nStart
};

bool ash0InitReader(BITREADER *pReader, const quint8 *pData, qint64 nStart, qint64 nLimit, qint64 nBufferSize)
{
    if (!pReader || !pData) return false;
    if ((nStart < 0) || (nLimit > nBufferSize) || (nStart + ASH0_WORD_SIZE > nLimit)) return false;
    pReader->pData = pData;
    pReader->nStart = nStart;
    pReader->nLimit = nLimit;
    pReader->nBitPos = 0;
    return true;
}

bool ash0ReadBit(BITREADER *pReader, quint32 *pnBit)
{
    const qint64 nByte = pReader->nStart + (pReader->nBitPos >> 3);
    if (nByte >= pReader->nLimit) return false;  // reader overrun
    *pnBit = static_cast<quint32>((pReader->pData[nByte] >> (7 - (pReader->nBitPos & 7))) & 1);
    ++pReader->nBitPos;
    return true;
}

bool ash0ReadBits(BITREADER *pReader, qint32 nCount, quint32 *pnValue)
{
    quint32 nValue = 0;
    for (qint32 i = 0; i < nCount; ++i) {
        quint32 nBit = 0;
        if (!ash0ReadBit(pReader, &nBit)) return false;
        nValue = (nValue << 1) | nBit;
    }
    *pnValue = nValue;
    return true;
}

qint64 ash0BytesTouched(const BITREADER *pReader)
{
    return (pReader->nBitPos + 7) >> 3;
}

// Leaves are their own values 0 .. nMax - 1; internal nodes are numbered from
// nMax upwards in creation order, so an index >= nMax is always internal.
struct TREE {
    QVector<quint32> vecLeft;
    QVector<quint32> vecRight;
    quint32 nRoot;
    quint32 nMax;
    quint32 nTable;
};

bool ash0ReadTree(BITREADER *pReader, qint32 nWidth, TREE *pTree)
{
    if ((nWidth < ASH0_MIN_TREE_BITS) || (nWidth > ASH0_MAX_TREE_BITS)) return false;

    const quint32 nMax = 1U << nWidth;
    const quint32 nTable = 2 * nMax - 1;
    pTree->nMax = nMax;
    pTree->nTable = nTable;
    pTree->nRoot = 0;
    pTree->vecLeft = QVector<quint32>(static_cast<qint32>(nTable), 0);
    pTree->vecRight = QVector<quint32>(static_cast<qint32>(nTable), 0);

    // Pending child slots: index | FLAG_RIGHT / FLAG_LEFT.  The right slot is
    // pushed first and the left slot last, so the left subtree - which comes
    // first in the pre-order serialisation - is attached first.
    QVector<quint32> vecStack;
    vecStack.reserve(static_cast<qint32>(2 * nMax));
    QVector<quint8> vecSeen(static_cast<qint32>(nMax), 0);
    quint32 nNext = nMax;

    for (;;) {
        quint32 nBit = 0;
        if (!ash0ReadBit(pReader, &nBit)) return false;  // overrun inside the tree
        if (nBit) {
            // A tree over <= nMax leaves has <= nMax - 1 internal nodes; the
            // System Menu's tables hold exactly that many.
            if (nNext >= nTable) return false;  // too many internal nodes
            if (static_cast<quint32>(vecStack.size()) + 2 > 2 * nMax) return false;  // cannot happen given the line above
            vecStack.append(nNext | ASH0_FLAG_RIGHT);
            vecStack.append(nNext | ASH0_FLAG_LEFT);
            ++nNext;
        } else {
            quint32 nValue = 0;
            if (!ash0ReadBits(pReader, nWidth, &nValue)) return false;  // overrun inside a leaf
            // A leaf with nothing to attach it to is a one-leaf tree: the
            // reference under-reads its stack there and no encoder emits it.
            if (vecStack.isEmpty()) return false;
            if (nValue >= nMax) return false;  // impossible - a leaf is exactly nWidth bits
            if (ASH0_REJECT_DUPLICATE_LEAF) {
                if (vecSeen.at(static_cast<qint32>(nValue))) return false;  // duplicate leaf
                vecSeen[static_cast<qint32>(nValue)] = 1;
            }
            for (;;) {
                const quint32 nEntry = vecStack.last();
                vecStack.removeLast();
                const quint32 nIndex = nEntry & ASH0_INDEX_MASK;
                if ((nIndex < nMax) || (nIndex >= nTable)) return false;
                if (nEntry & ASH0_FLAG_RIGHT) {
                    // Right slot filled: node nIndex is complete and becomes
                    // the value for ITS parent's pending slot.
                    pTree->vecRight[static_cast<qint32>(nIndex)] = nValue;
                    nValue = nIndex;
                    if (vecStack.isEmpty()) {
                        pTree->nRoot = nValue;
                        return true;
                    }
                } else {
                    // Left slot filled: the right subtree is read next.
                    pTree->vecLeft[static_cast<qint32>(nIndex)] = nValue;
                    break;
                }
            }
        }
    }
}

// Walks one code from the root to a leaf.  Children are always previously
// completed subtrees, so every step goes to a smaller index and the walk is
// finite; the guard is insurance, not a requirement.
bool ash0ReadCode(BITREADER *pReader, const TREE *pTree, quint32 *pnValue)
{
    quint32 nNode = pTree->nRoot;
    quint32 nGuard = 0;
    while (nNode >= pTree->nMax) {
        if ((nNode >= pTree->nTable) || (++nGuard > pTree->nTable)) return false;
        quint32 nBit = 0;
        if (!ash0ReadBit(pReader, &nBit)) return false;
        nNode = nBit ? pTree->vecRight.at(static_cast<qint32>(nNode)) : pTree->vecLeft.at(static_cast<qint32>(nNode));
    }
    *pnValue = nNode;
    return true;
}

bool ash0DecodeStreams(const quint8 *pData, qint64 nPackedSize, const XASH0Decoder::HEADER &header, qint32 nSymBits, qint32 nDistBits,
                       QByteArray *pbaResult, bool *pbTight, XBinary::PDSTRUCT *pPdStruct)
{
    BITREADER symReader = {};
    BITREADER distReader = {};
    if (!ash0InitReader(&symReader, pData, ASH0_SYM_STREAM_OFFSET, header.nDistOffset, nPackedSize)) return false;
    if (!ash0InitReader(&distReader, pData, header.nDistOffset, nPackedSize, nPackedSize)) return false;

    TREE symTree;
    TREE distTree;
    if (!ash0ReadTree(&symReader, nSymBits, &symTree)) return false;
    if (!ash0ReadTree(&distReader, nDistBits, &distTree)) return false;

    const qint64 nSize = header.nUncompressedSize;
    QByteArray baOut(static_cast<qint32>(nSize), 0);
    quint8 *pOut = reinterpret_cast<quint8 *>(baOut.data());
    qint64 nOut = 0;
    qint32 nCounter = 0;

    while (nOut < nSize) {
        ++nCounter;
        if ((nCounter & ASH0_CANCEL_MASK) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        quint32 nSymbol = 0;
        if (!ash0ReadCode(&symReader, &symTree, &nSymbol)) return false;

        if (nSymbol < ASH0_LITERAL_LIMIT) {
            pOut[nOut] = static_cast<quint8>(nSymbol);
            ++nOut;
            continue;
        }

        quint32 nDistSymbol = 0;
        if (!ash0ReadCode(&distReader, &distTree, &nDistSymbol)) return false;

        const qint64 nLength = static_cast<qint64>(nSymbol - ASH0_LITERAL_LIMIT) + ASH0_MIN_MATCH;
        const qint64 nDistance = static_cast<qint64>(nDistSymbol) + 1;
        if (nLength > nSize - nOut) return false;  // copy past the declared end
        if (nDistance > nOut) return false;        // copy before the start
        // Byte-wise so that nDistance < nLength reproduces the RLE idiom.
        for (qint64 i = 0; i < nLength; ++i) {
            pOut[nOut] = pOut[nOut - nDistance];
            ++nOut;
        }
    }

    if (nOut != nSize) return false;

    const qint64 nDistStreamBytes = nPackedSize - header.nDistOffset;
    *pbTight = (nDistStreamBytes - ash0BytesTouched(&distReader)) < ASH0_TIGHT_SLACK_BYTES;
    *pbaResult = baOut;
    return true;
}
}  // namespace

bool XASH0Decoder::parseHeader(const quint8 *pHeader, qint64 nHeaderSize, qint64 nPackedSize, HEADER *pResult)
{
    if (!pHeader || !pResult) return false;
    if (nHeaderSize < ASH0_HEADER_SIZE) return false;
    if (nPackedSize < ASH0_MIN_SIZE) return false;
    if ((pHeader[0] != 'A') || (pHeader[1] != 'S') || (pHeader[2] != 'H') || (pHeader[3] != '0')) return false;

    const quint32 nSizeWord = (static_cast<quint32>(pHeader[4]) << 24) | (static_cast<quint32>(pHeader[5]) << 16) | (static_cast<quint32>(pHeader[6]) << 8) |
                              static_cast<quint32>(pHeader[7]);
    const quint32 nDistWord = (static_cast<quint32>(pHeader[8]) << 24) | (static_cast<quint32>(pHeader[9]) << 16) | (static_cast<quint32>(pHeader[10]) << 8) |
                              static_cast<quint32>(pHeader[11]);

    HEADER header = {};
    header.nTopByte = static_cast<quint8>(nSizeWord >> 24);
    header.nUncompressedSize = static_cast<qint64>(nSizeWord & ASH0_SIZE_MASK);
    header.nDistOffset = static_cast<qint64>(nDistWord);

    // Zero makes the System Menu's do/while counter underflow; no encoder
    // produces it and an empty member would be written at exit 0.
    if ((header.nUncompressedSize < 1) || (header.nUncompressedSize > ASH0_MAX_UNCOMPRESSED_SIZE)) return false;
    // The symbol stream always starts at 0x0C and needs a whole first word;
    // the distance stream needs a whole word before EOF.  4-alignment is NOT
    // required: the System Menu never checks it.
    if ((header.nDistOffset < ASH0_MIN_DIST_OFFSET) || (header.nDistOffset > nPackedSize - ASH0_WORD_SIZE)) return false;
    // Ratio guard before any allocation.
    if (header.nUncompressedSize > ((nPackedSize - ASH0_HEADER_SIZE) * ASH0_MAX_RATIO) + ASH0_RATIO_SLACK) return false;

    *pResult = header;
    return true;
}

bool XASH0Decoder::decodeWithBits(const QByteArray &baPacked, qint64 nUncompressedSize, qint32 nSymBits, qint32 nDistBits, QByteArray *pbaResult,
                                  bool *pbTight, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || !pbTight) return false;
    pbaResult->clear();
    *pbTight = false;

    const qint64 nPackedSize = baPacked.size();
    const quint8 *pData = reinterpret_cast<const quint8 *>(baPacked.constData());
    HEADER header = {};
    if (!parseHeader(pData, nPackedSize, nPackedSize, &header)) return false;
    // The dispatcher's length and the header's masked length must agree: the
    // header is the codec's only termination and the record's only promise.
    if (nUncompressedSize != header.nUncompressedSize) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    return ash0DecodeStreams(pData, nPackedSize, header, nSymBits, nDistBits, pbaResult, pbTight, pPdStruct);
}

bool XASH0Decoder::decodeEx(const QByteArray &baPacked, qint64 nUncompressedSize, qint32 nDistBitsHint, QByteArray *pbaResult, RESULT *pResult,
                            XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();

    RESULT result = {};
    result.nSymBits = ASH0_SYM_BITS;
    result.nDistBits = 0;
    result.bTight = false;
    result.bAmbiguous = false;
    result.nTopByte = 0;
    if (pResult) *pResult = result;

    // The header is parsed once more here only to publish the top byte; every
    // attempt re-validates it itself.
    {
        HEADER header = {};
        const qint64 nPackedSize = baPacked.size();
        if (!parseHeader(reinterpret_cast<const quint8 *>(baPacked.constData()), nPackedSize, nPackedSize, &header)) return false;
        result.nTopByte = header.nTopByte;
    }

    // Wider first: a wrong-15 decode almost always dies inside the tree or at
    // the first match, a wrong-11 decode can survive with plausible distances.
    qint32 nFirst = ASH0_DIST_BITS_RANCH;
    qint32 nSecond = ASH0_DIST_BITS_DEFAULT;
    if (nDistBitsHint == ASH0_DIST_BITS_DEFAULT) {
        nFirst = ASH0_DIST_BITS_DEFAULT;
        nSecond = ASH0_DIST_BITS_RANCH;
    } else if (nDistBitsHint == ASH0_DIST_BITS_RANCH) {
        nFirst = ASH0_DIST_BITS_RANCH;
        nSecond = ASH0_DIST_BITS_DEFAULT;
    } else if (nDistBitsHint != 0) {
        return false;
    }

    QByteArray baFirst;
    bool bFirstTight = false;
    const bool bFirstOk = decodeWithBits(baPacked, nUncompressedSize, ASH0_SYM_BITS, nFirst, &baFirst, &bFirstTight, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (bFirstOk && bFirstTight) {
        result.nDistBits = nFirst;
        result.bTight = true;
        *pbaResult = baFirst;
        if (pResult) *pResult = result;
        return true;
    }

    QByteArray baSecond;
    bool bSecondTight = false;
    const bool bSecondOk = decodeWithBits(baPacked, nUncompressedSize, ASH0_SYM_BITS, nSecond, &baSecond, &bSecondTight, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    result.bAmbiguous = bFirstOk && bSecondOk;
    if (bSecondOk && bSecondTight) {
        result.nDistBits = nSecond;
        result.bTight = true;
        *pbaResult = baSecond;
        if (pResult) *pResult = result;
        return true;
    }
    // Neither is tight: a lone loose decode wins (real files may carry
    // trailing padding we cannot know about); two loose decodes keep the
    // candidate order's preference.
    if (bFirstOk) {
        result.nDistBits = nFirst;
        *pbaResult = baFirst;
        if (pResult) *pResult = result;
        return true;
    }
    if (bSecondOk) {
        result.nDistBits = nSecond;
        *pbaResult = baSecond;
        if (pResult) *pResult = result;
        return true;
    }
    return false;
}

bool XASH0Decoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    return decodeEx(baPacked, nUncompressedSize, 0, pbaResult, nullptr, pPdStruct);
}

bool XASH0Decoder::probe(const QByteArray &baPacked, RESULT *pResult, XBinary::PDSTRUCT *pPdStruct)
{
    const qint64 nPackedSize = baPacked.size();
    HEADER header = {};
    if (!parseHeader(reinterpret_cast<const quint8 *>(baPacked.constData()), nPackedSize, nPackedSize, &header)) return false;

    QByteArray baDiscard;
    return decodeEx(baPacked, header.nUncompressedSize, 0, &baDiscard, pResult, pPdStruct);
}
