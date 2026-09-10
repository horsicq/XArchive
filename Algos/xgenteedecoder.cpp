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
#include "xgenteedecoder.h"

#include <QtEndian>

namespace {
const qint32 GENTEE_WINDOW = 0x8000;
const qint32 GENTEE_MAIN_SYMBOLS = 0x112;
const qint32 GENTEE_DISTANCE_SYMBOLS = 0x22;
const qint32 GENTEE_LENGTH_SYMBOLS = 0xed;
const qint32 GENTEE_DISTANCE_SLOTS = 0x1e;
const qint32 GENTEE_RECENT = 4;
const qint32 GENTEE_MAX_WEIGHT = 0x200;
const qint32 GENTEE_MIN_LENGTH = 3;
const qint32 GENTEE_LENGTH_ESCAPE = 0x11;
const qint32 GENTEE_MAX_BLOCK = 0x10000;
const qint32 GENTEE_MAX_RUNTIME_BLOCK = 0x100000;
const qint32 GENTEE_ARCHIVE_HEADER_SIZE = 0x14;
const qint32 GENTEE_ARCHIVE_HEADER_FLAG = 14;
const qint32 GENTEE_SKIPPED_BLOCKS = 2;
const qint32 GENTEE_COMMAND_HEADER = 3;
const qint32 GENTEE_RECORD_MIN_SIZE = 0x1e;
const qint32 GENTEE_RECORD_SIZE_OFFSET = 4;
const qint32 GENTEE_RECORD_TIME_OFFSET = 8;
const qint32 GENTEE_RECORD_STORED_OFFSET = 0x19;
const qint32 GENTEE_RECORD_NAME_OFFSET = 0x1f;
const quint16 GENTEE_TAG_END = 0x87f0;
const quint16 GENTEE_TAG_FILE = 0x87f4;
const qint32 GENTEE_MAX_RECORDS = 1000000;
const qint32 GENTEE_PROGRESS_MASK = 0xffff;

const qint32 GENTEE_DISTANCE_BITS[GENTEE_DISTANCE_SLOTS] = {0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4,  4,  5,  5,  6,
                                                            6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

struct NODE {
    qint32 nSymbol;
    qint32 nWeight;
    NODE *pParent;
    NODE *pLeft;
    NODE *pRight;
};

// An adaptive Huffman tree.  The node array is sized once and never grows, so
// the parent/child pointers into it stay valid for the tree's whole life.
class Tree {
public:
    Tree() : m_nSymbols(0), m_nNodes(0), m_pRoot(nullptr)
    {
    }

    void init(qint32 nSymbols)
    {
        m_nSymbols = nSymbols;
        m_listNodes.resize(nSymbols * 2);
        for (qint32 i = 0; i < nSymbols * 2; i++) {
            NODE &node = m_listNodes[i];
            node.nSymbol = (i < nSymbols) ? i : 0;
            // The initial weights are 1..n, so the starting tree is skewed
            // towards the low symbols exactly as the reference builder leaves
            // it; a flat 1 everywhere gives a different tree and decodes
            // garbage from the first symbol on.
            node.nWeight = (i < nSymbols) ? (i + 1) : 0;
            node.pParent = nullptr;
            node.pLeft = nullptr;
            node.pRight = nullptr;
        }
        build();
    }

    NODE *root() const
    {
        return m_pRoot;
    }

    void update(qint32 nSymbol)
    {
        NODE *pNode = &m_listNodes[nSymbol];
        while (pNode) {
            NODE *pParent = pNode->pParent;
            if (pParent && pParent->pParent) {
                NODE *pGrand = pParent->pParent;
                NODE *pUncle = (pParent == pGrand->pLeft) ? pGrand->pRight : pGrand->pLeft;
                if (pUncle->nWeight <= pNode->nWeight) swapNodes(pNode, pUncle);
            }
            pNode->nWeight++;
            pNode = pNode->pParent;
        }
        if (m_pRoot && (m_pRoot->nWeight >= GENTEE_MAX_WEIGHT)) {
            for (qint32 i = 0; i < m_nNodes; i++) m_listNodes[i].nWeight >>= 1;
        }
    }

private:
    void build()
    {
        const qint32 nTotal = m_nSymbols * 2 - 1;
        for (qint32 i = 0; i < nTotal; i++) m_listNodes[i].pParent = nullptr;

        qint32 nCount = m_nSymbols;
        NODE *pLast = nullptr;
        while (nCount < nTotal) {
            NODE *pMin1 = nullptr;
            NODE *pMin2 = nullptr;
            for (qint32 i = 0; i < nCount; i++) {
                NODE *pNode = &m_listNodes[i];
                if (pNode->pParent) continue;
                if (!pMin1 || (pNode->nWeight < pMin1->nWeight)) {
                    // The displaced minimum only becomes the second minimum
                    // when that slot is still empty.  It looks like a bug in
                    // the reference builder and it changes the tree shape, so
                    // it has to be reproduced, not corrected.
                    if (pMin1 && !pMin2) pMin2 = pMin1;
                    pMin1 = pNode;
                } else if (!pMin2 || (pNode->nWeight < pMin2->nWeight)) {
                    pMin2 = pNode;
                }
            }
            if (!pMin1 || !pMin2) break;
            NODE *pNew = &m_listNodes[nCount];
            pNew->nSymbol = nCount;
            pNew->nWeight = pMin1->nWeight + pMin2->nWeight;
            pNew->pLeft = pMin1;
            pNew->pRight = pMin2;
            pMin1->pParent = pNew;
            pMin2->pParent = pNew;
            nCount++;
            pLast = pNew;
        }
        m_nNodes = nCount;
        if (pLast) pLast->pParent = nullptr;
        m_pRoot = pLast;
    }

    // Exchanges two nodes and, when the swapped-in node's heavier child
    // outweighs the sibling left behind, repeats the exchange one level down.
    void swapNodes(NODE *pA, NODE *pB)
    {
        bool bExtra = true;
        while (true) {
            NODE *pParentA = pA->pParent;
            NODE *pSibling = (pA == pParentA->pLeft) ? pParentA->pRight : pParentA->pLeft;
            NODE *pHeavy = pB->pLeft;
            if (pHeavy && (pB->pLeft->nWeight <= pB->pRight->nWeight)) pHeavy = pB->pRight;

            if (pA == pParentA->pLeft) pParentA->pLeft = pB;
            else pParentA->pRight = pB;

            NODE *pParentB = pB->pParent;
            if (pParentB->pLeft == pB) pParentB->pLeft = pA;
            else pParentB->pRight = pA;

            pA->pParent = pParentB;
            pB->pParent = pParentA;

            if (!bExtra || !pHeavy || !pSibling || (pHeavy->nWeight <= pSibling->nWeight)) break;
            bExtra = false;
            pB->nWeight += (pSibling->nWeight - pHeavy->nWeight);
            pA = pHeavy;
            pB = pSibling;
        }
    }

    qint32 m_nSymbols;
    qint32 m_nNodes;
    NODE *m_pRoot;
    QVector<NODE> m_listNodes;
};

// MSB-first, one byte at a time: a block therefore starts on a byte boundary
// and the bits left over at its end are dropped.
class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize, qint64 nPosition) : m_pData(pData), m_nSize(nSize), m_nPosition(nPosition), m_nCount(0), m_nByte(0)
    {
    }

    qint32 bit()
    {
        if (m_nCount == 0) {
            if (m_nPosition >= m_nSize) return -1;
            m_nByte = m_pData[m_nPosition];
            m_nPosition++;
            m_nCount = 7;
        } else {
            m_nCount--;
        }
        const qint32 nResult = (m_nByte >> 7) & 1;
        m_nByte = (quint8)(m_nByte << 1);
        return nResult;
    }

    // The extra distance bits arrive low bit first.
    qint32 value(qint32 nBits)
    {
        qint32 nResult = 0;
        qint32 nMask = 1;
        for (qint32 i = 0; i < nBits; i++) {
            const qint32 nBit = bit();
            if (nBit < 0) return -1;
            if (nBit) nResult |= nMask;
            nMask <<= 1;
        }
        return nResult;
    }

    qint64 position() const
    {
        return m_nPosition;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    qint32 m_nCount;
    quint8 m_nByte;
};

class State {
public:
    State()
    {
        reset();
    }

    void reset()
    {
        m_treeMain.init(GENTEE_MAIN_SYMBOLS);
        m_treeDistance.init(GENTEE_DISTANCE_SYMBOLS);
        m_treeLength.init(GENTEE_LENGTH_SYMBOLS);
        for (qint32 i = 0; i < GENTEE_RECENT; i++) m_nRecent[i] = i;
        m_nWindowPos = 0;
        m_baWindow.fill((char)0, GENTEE_WINDOW);
    }

    // pbaOutput may be null: the bytes are then only pushed through the
    // window, which is what skipping a member costs.
    bool unpack(BitReader *pReader, qint64 nSize, QByteArray *pbaOutput, XBinary::PDSTRUCT *pPdStruct)
    {
        quint8 *pWindow = (quint8 *)m_baWindow.data();
        qint64 nCounter = 0;
        while (nSize > 0) {
            if (((nCounter++) & GENTEE_PROGRESS_MASK) == 0) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            }
            qint32 nSymbol = symbol(pReader, &m_treeMain);
            if (nSymbol < 0) return false;
            if (nSymbol < 0x100) {
                if (pbaOutput) pbaOutput->append((char)(quint8)nSymbol);
                pWindow[m_nWindowPos] = (quint8)nSymbol;
                m_nWindowPos = (m_nWindowPos + 1) & (GENTEE_WINDOW - 1);
                nSize--;
                continue;
            }
            nSymbol -= 0x100;
            if (nSymbol >= GENTEE_LENGTH_ESCAPE) {
                nSymbol = symbol(pReader, &m_treeLength);
                if (nSymbol < 0) return false;
                nSymbol += GENTEE_LENGTH_ESCAPE;
            }
            const qint32 nLength = nSymbol + GENTEE_MIN_LENGTH;

            const qint32 nDistanceSymbol = symbol(pReader, &m_treeDistance);
            if (nDistanceSymbol < 0) return false;
            qint32 nDistance = 0;
            if (nDistanceSymbol < GENTEE_DISTANCE_SLOTS) {
                qint32 nBase = 0;
                for (qint32 i = 0; i < nDistanceSymbol; i++) nBase += (1 << GENTEE_DISTANCE_BITS[i]);
                const qint32 nExtra = pReader->value(GENTEE_DISTANCE_BITS[nDistanceSymbol]);
                if (nExtra < 0) return false;
                nDistance = nBase + nExtra;
            } else {
                nDistance = m_nRecent[nDistanceSymbol - GENTEE_DISTANCE_SLOTS];
            }

            qint32 nIndex = 0;
            while ((nIndex < GENTEE_RECENT) && (m_nRecent[nIndex] != nDistance)) nIndex++;
            if (nIndex > GENTEE_RECENT - 1) nIndex = GENTEE_RECENT - 1;
            for (; nIndex > 0; nIndex--) m_nRecent[nIndex] = m_nRecent[nIndex - 1];
            m_nRecent[0] = nDistance;

            if (nSize < nLength) return false;
            nSize -= nLength;
            qint32 nSource = (m_nWindowPos - nLength - nDistance) & (GENTEE_WINDOW - 1);
            for (qint32 i = 0; i < nLength; i++) {
                const quint8 nByte = pWindow[nSource];
                if (pbaOutput) pbaOutput->append((char)nByte);
                pWindow[m_nWindowPos] = nByte;
                nSource = (nSource + 1) & (GENTEE_WINDOW - 1);
                m_nWindowPos = (m_nWindowPos + 1) & (GENTEE_WINDOW - 1);
            }
        }
        return true;
    }

private:
    qint32 symbol(BitReader *pReader, Tree *pTree)
    {
        NODE *pNode = pTree->root();
        if (!pNode) return -1;
        do {
            const qint32 nBit = pReader->bit();
            if (nBit < 0) return -1;
            pNode = (nBit == 0) ? pNode->pLeft : pNode->pRight;
            if (!pNode) return -1;
        } while (pNode->pLeft);
        const qint32 nSymbol = pNode->nSymbol;
        pTree->update(nSymbol);
        return nSymbol;
    }

    Tree m_treeMain;
    Tree m_treeDistance;
    Tree m_treeLength;
    qint32 m_nRecent[GENTEE_RECENT];
    qint32 m_nWindowPos;
    QByteArray m_baWindow;
};

bool genteeReadUInt32(const QByteArray &baPayload, qint64 *pnPosition, quint32 *pnValue)
{
    if ((*pnPosition < 0) || (*pnPosition + 4 > (qint64)baPayload.size())) return false;
    *pnValue = qFromLittleEndian<quint32>((const uchar *)baPayload.constData() + *pnPosition);
    *pnPosition += 4;
    return true;
}

// One block: the decoded size, then the bit stream.  The position is left on
// the first byte the bit reader did not take.
bool genteeBlock(const QByteArray &baPayload, qint64 *pnPosition, State *pState, qint64 nSizeLimit, QByteArray *pbaOutput, XBinary::PDSTRUCT *pPdStruct)
{
    quint32 nDecodedSize = 0;
    qint64 nPosition = *pnPosition;
    if (!genteeReadUInt32(baPayload, &nPosition, &nDecodedSize)) return false;
    if ((nSizeLimit > 0) && ((qint64)nDecodedSize > nSizeLimit)) return false;
    if (pbaOutput) pbaOutput->reserve((qint32)nDecodedSize);

    BitReader reader((const quint8 *)baPayload.constData(), (qint64)baPayload.size(), nPosition);
    if (!pState->unpack(&reader, (qint64)nDecodedSize, pbaOutput, pPdStruct)) return false;
    *pnPosition = reader.position();
    return true;
}

QString genteeName(const char *pRecord, qint64 nRecordSize)
{
    qint64 nEnd = GENTEE_RECORD_NAME_OFFSET;
    while ((nEnd < nRecordSize) && pRecord[nEnd]) nEnd++;
    if (nEnd >= nRecordSize) return QString();

    QString sResult;
    for (qint64 i = GENTEE_RECORD_NAME_OFFSET; i < nEnd; i++) {
        const quint16 nCharacter = (quint8)pRecord[i];
        if ((nCharacter == '\\') || (nCharacter == '/')) {
            sResult.append(QLatin1Char('/'));
            continue;
        }
        const bool bSafe = (nCharacter >= 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != ':') && (nCharacter != '*') &&
                           (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QChar(nCharacter));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            while (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    return sResult;
}

// The single walk both scan() and decode() ride on.  nRequestedIndex >= 0 asks
// for one member's bytes and stops there; everything before it still has to be
// decoded because the member chain is solid.
bool genteeWalk(const QByteArray &baPayload, QList<XGenteeDecoder::MEMBER> *plistMembers, qint64 nRequestedIndex, QByteArray *pbaRequested, qint64 *pnArchiveSize,
                XBinary::PDSTRUCT *pPdStruct)
{
    qint64 nPosition = 0;
    State stateCommand;
    State stateData;

    // The runtime image opens the payload and its decoder state is dropped
    // afterwards; the command chain starts from scratch behind the 20-byte
    // archive header.
    if (!genteeBlock(baPayload, &nPosition, &stateCommand, GENTEE_MAX_RUNTIME_BLOCK, nullptr, pPdStruct)) return false;
    stateCommand.reset();

    if (nPosition + GENTEE_ARCHIVE_HEADER_SIZE > (qint64)baPayload.size()) return false;
    const uchar *pHeader = (const uchar *)baPayload.constData() + nPosition;
    if (qFromLittleEndian<quint16>(pHeader + GENTEE_ARCHIVE_HEADER_FLAG) != 0) return false;
    nPosition += GENTEE_ARCHIVE_HEADER_SIZE;

    for (qint32 i = 0; i < GENTEE_SKIPPED_BLOCKS; i++) {
        if (!genteeBlock(baPayload, &nPosition, &stateCommand, 0, nullptr, pPdStruct)) return false;
    }

    bool bFound = false;
    qint64 nIndex = 0;
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (nIndex >= GENTEE_MAX_RECORDS) break;

        qint64 nProbe = nPosition;
        quint32 nCommandSize = 0;
        if (!genteeReadUInt32(baPayload, &nProbe, &nCommandSize)) break;
        if ((nCommandSize <= 2) || ((qint64)nCommandSize > GENTEE_MAX_BLOCK)) break;

        QByteArray baCommand;
        if (!genteeBlock(baPayload, &nPosition, &stateCommand, GENTEE_MAX_BLOCK, &baCommand, pPdStruct)) break;
        if (baCommand.size() < GENTEE_COMMAND_HEADER) break;

        const quint16 nTag = qFromLittleEndian<quint16>((const uchar *)baCommand.constData());
        if (nTag == GENTEE_TAG_END) {
            if (pnArchiveSize) *pnArchiveSize = nPosition;
            return (nRequestedIndex < 0) || bFound;
        }
        if (nTag != GENTEE_TAG_FILE) continue;

        const char *pRecord = baCommand.constData() + GENTEE_COMMAND_HEADER;
        const qint64 nRecordSize = (qint64)baCommand.size() - GENTEE_COMMAND_HEADER;
        if (nRecordSize <= GENTEE_RECORD_MIN_SIZE) break;

        XGenteeDecoder::MEMBER member;
        member.nAttributes = qFromLittleEndian<quint32>((const uchar *)pRecord);
        member.nSize = (qint64)qFromLittleEndian<qint32>((const uchar *)pRecord + GENTEE_RECORD_SIZE_OFFSET);
        member.nFileTime = qFromLittleEndian<quint64>((const uchar *)pRecord + GENTEE_RECORD_TIME_OFFSET);
        member.bStored = ((quint8)pRecord[GENTEE_RECORD_STORED_OFFSET] == 0);
        member.sFileName = genteeName(pRecord, nRecordSize);
        member.nDataOffset = nPosition;
        member.nDataEnd = nPosition;
        if ((member.nSize < 0) || member.sFileName.isEmpty()) break;

        const bool bWanted = (nRequestedIndex == nIndex);
        if (member.bStored) {
            if (nPosition + member.nSize > (qint64)baPayload.size()) break;
            if (bWanted && pbaRequested) *pbaRequested = baPayload.mid((qint32)nPosition, (qint32)member.nSize);
            nPosition += member.nSize;
        } else {
            QByteArray baData;
            const bool bCollect = (bWanted || (plistMembers != nullptr));
            if (!genteeBlock(baPayload, &nPosition, &stateData, 0, bCollect ? &baData : nullptr, pPdStruct)) break;
            if (bCollect && ((qint64)baData.size() != member.nSize)) break;
            if (bWanted && pbaRequested) *pbaRequested = baData;
        }
        member.nDataEnd = nPosition;

        if (plistMembers) plistMembers->append(member);
        nIndex++;
        if (bWanted) {
            bFound = true;
            break;
        }
    }

    if (nRequestedIndex >= 0) return bFound;

    // A chain that stopped early is a truncated carrier, not a foreign format:
    // keep the members that were decoded whole.
    if (pnArchiveSize) *pnArchiveSize = nPosition;
    return (plistMembers != nullptr) && (!plistMembers->isEmpty());
}
}  // namespace

bool XGenteeDecoder::isPayloadHeader(const char *pData, qint64 nSize)
{
    if (!pData || (nSize < 12)) return false;
    const uchar *pHeader = (const uchar *)pData;
    if (qFromLittleEndian<quint32>(pHeader + 4) != 0x36a767ab) return false;
    if (qFromLittleEndian<quint32>(pHeader + 8) != 0x6ffb4dff) return false;
    const quint32 nRuntimeSize = qFromLittleEndian<quint32>(pHeader);
    return (nRuntimeSize > 0) && ((qint64)nRuntimeSize < GENTEE_MAX_RUNTIME_BLOCK);
}

bool XGenteeDecoder::scan(const QByteArray &baPayload, QList<MEMBER> *plistMembers, qint64 *pnArchiveSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!plistMembers) return false;
    if (!isPayloadHeader(baPayload.constData(), (qint64)baPayload.size())) return false;
    return genteeWalk(baPayload, plistMembers, -1, nullptr, pnArchiveSize, pPdStruct);
}

bool XGenteeDecoder::decode(const QByteArray &baPacked, qint64 nMemberIndex, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || (nMemberIndex < 0)) return false;
    if (!isPayloadHeader(baPacked.constData(), (qint64)baPacked.size())) return false;

    QByteArray baResult;
    if (!genteeWalk(baPacked, nullptr, nMemberIndex, &baResult, nullptr, pPdStruct)) return false;
    *pbaResult = baResult;
    return (qint64)baResult.size() == nUncompressedSize;
}

QByteArray XGenteeDecoder::indexToProperty(qint64 nMemberIndex)
{
    QByteArray baResult(8, (char)0);
    qToLittleEndian<quint64>((quint64)nMemberIndex, (uchar *)baResult.data());
    return baResult;
}

bool XGenteeDecoder::propertyToIndex(const QByteArray &baProperty, qint64 *pnMemberIndex)
{
    if (!pnMemberIndex || (baProperty.size() != 8)) return false;
    const quint64 nValue = qFromLittleEndian<quint64>((const uchar *)baProperty.constData());
    if (nValue > (quint64)GENTEE_MAX_RECORDS) return false;
    *pnMemberIndex = (qint64)nValue;
    return true;
}
