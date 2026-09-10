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
#include "xzxzipdecoder.h"

#include <QtEndian>

namespace {
const qint32 ZXZIP_RING_SIZE = 0x2000;
const qint32 ZXZIP_SHRINK_CODES = 0x2000;
const qint32 ZXZIP_SHRINK_FIRST = 0x101;
const qint32 ZXZIP_SHRINK_ESCAPE = 0x100;
const qint32 ZXZIP_SHRINK_MAX_WIDTH = 13;
const qint32 ZXZIP_MODE_C_LIMIT = 0x1600;

// Static Huffman code-length tables, recovered from the reference at
// VAs 0x8200ac..0x820154 (stored RLE'd: byte0 is the RLE byte count and each
// byte b means ((b>>4)+1) symbols of length (b&0xf)+1).
const quint8 g_arrALen[64] = {1,  3,  3,  4,  5,  5,  5,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9,  9,  10, 10,
                              10, 10, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13,
                              14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 16, 16, 10};

const quint8 g_arrAOff[64] = {2, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                              7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

const quint8 g_arrBLen[64] = {3,  2,  3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,  9,  9,  9,
                              9,  9,  9,  9,  10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
                              11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 6};

const quint8 g_arrBOff[64] = {3, 3, 4, 4, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                              7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

const quint8 g_arrCLit[256] = {
    11, 12, 12, 12, 12, 12, 12, 12, 12, 8,  7,  12, 12, 7,  12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 12, 12, 12, 12, 12,
    4,  10, 8,  12, 10, 12, 10, 8,  7,  7,  8,  9,  7,  6,  7,  8,  7,  6,  7,  7,  7,  7,  8,  7,  7,  8,  8,  12, 11, 7,  9,  11,
    12, 6,  7,  6,  6,  5,  7,  8,  8,  6,  11, 9,  6,  7,  6,  6,  7,  11, 6,  6,  6,  7,  9,  8,  9,  9,  11, 8,  11, 9,  12, 8,
    12, 5,  6,  6,  6,  5,  6,  6,  6,  5,  11, 7,  5,  6,  5,  5,  6,  10, 5,  5,  5,  5,  8,  7,  8,  8,  10, 11, 11, 12, 12, 12,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    13, 12, 13, 13, 13, 12, 13, 13, 13, 12, 13, 13, 13, 13, 12, 13, 13, 13, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13};

const quint8 g_arrCLen[64] = {2,  3,  3,  3,  4,  4,  4,  5,  5,  6,  6,  7,  7,  7,  7,  8,  8,  8,  8,  9,  9,  9,
                              9,  9,  9,  9,  10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11,
                              11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 6};

const quint8 g_arrCOff[64] = {3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
                              7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

// LSB first, bytes appended above the cursor.  -1 means exhausted.
class ZxBits {
public:
    ZxBits(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nAccumulator(0), m_nCount(0)
    {
    }

    qint32 get(qint32 nBits)
    {
        while (m_nCount < nBits) {
            if (m_nPosition >= m_nSize) return -1;
            m_nAccumulator |= ((quint32)m_pData[m_nPosition]) << m_nCount;
            ++m_nPosition;
            m_nCount += 8;
        }
        const qint32 nValue = (qint32)(m_nAccumulator & ((1U << nBits) - 1));
        m_nAccumulator >>= nBits;
        m_nCount -= nBits;

        return nValue;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nAccumulator;
    qint32 m_nCount;
};

// The binary decode tree exactly as the reference builds and walks it: nodes
// are pairs of children indexed by the CODE bit, a leaf is (symbol | 0x8000)
// and 0 means "no child".  decode() inverts the stream bit.
class ZxHuffman {
public:
    ZxHuffman() : m_bValid(false)
    {
    }

    bool build(const quint8 *pLengths, qint32 nCount)
    {
        m_listChild0.clear();
        m_listChild1.clear();
        m_listChild0.append(0);
        m_listChild1.append(0);
        qint32 nMaxLength = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            if (pLengths[i] > 24) return false;
            if (pLengths[i] > nMaxLength) nMaxLength = pLengths[i];
        }
        quint32 nCode = 0;
        for (qint32 nLength = 1; nLength <= nMaxLength; ++nLength) {
            for (qint32 nSymbol = 0; nSymbol < nCount; ++nSymbol) {
                if (pLengths[nSymbol] == nLength) {
                    if (!insert(nCode, nLength, nSymbol)) return false;
                    ++nCode;
                }
            }
            nCode <<= 1;
        }
        m_bValid = true;

        return true;
    }

    // -1 on a bad code or an exhausted stream
    qint32 decode(ZxBits *pBits) const
    {
        if (!m_bValid) return -1;
        qint32 nCurrent = 0;
        for (qint32 nStep = 0; nStep < 32; ++nStep) {
            const qint32 nBit = pBits->get(1);
            if (nBit < 0) return -1;
            const qint32 nValue = (nBit == 0) ? m_listChild1.at(nCurrent) : m_listChild0.at(nCurrent);
            if (nValue == 0) return -1;
            if (nValue & 0x8000) return nValue & 0x7fff;
            if ((nValue < 0) || (nValue >= m_listChild0.size())) return -1;
            nCurrent = nValue;
        }

        return -1;
    }

private:
    bool insert(quint32 nCode, qint32 nLength, qint32 nSymbol)
    {
        qint32 nCurrent = 0;
        for (qint32 nPosition = nLength - 1; nPosition >= 0; --nPosition) {
            QVector<qint32> *pChildren = ((nCode >> nPosition) & 1) ? &m_listChild1 : &m_listChild0;
            const qint32 nValue = pChildren->at(nCurrent);
            if (nValue & 0x8000) return false;  // a leaf where a branch is needed
            if (nValue == 0) {
                if (nPosition == 0) {
                    (*pChildren)[nCurrent] = nSymbol | 0x8000;
                } else {
                    m_listChild0.append(0);
                    m_listChild1.append(0);
                    const qint32 nNode = m_listChild0.size() - 1;
                    // pChildren may have been reallocated by the appends
                    QVector<qint32> *pFresh = ((nCode >> nPosition) & 1) ? &m_listChild1 : &m_listChild0;
                    (*pFresh)[nCurrent] = nNode;
                    nCurrent = nNode;
                }
            } else {
                if (nPosition == 0) return false;  // prefix clash
                if (nValue >= m_listChild0.size()) return false;
                nCurrent = nValue;
            }
        }

        return true;
    }

    QVector<qint32> m_listChild0;
    QVector<qint32> m_listChild1;
    bool m_bValid;
};

// method 3
bool zxLzhDecode(const QByteArray &baPacked, qint64 nOutputSize, quint8 nSubMethod, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    const quint8 *pLengthTable = 0;
    const quint8 *pOffsetTable = 0;
    const quint8 *pLiteralTable = 0;
    qint32 nBias = 2;
    qint32 nShift = 6;
    if (nSubMethod == 0) {
        pLengthTable = g_arrALen;
        pOffsetTable = g_arrAOff;
    } else if (nOutputSize < ZXZIP_MODE_C_LIMIT) {
        pLengthTable = g_arrBLen;
        pOffsetTable = g_arrBOff;
    } else {
        pLengthTable = g_arrCLen;
        pOffsetTable = g_arrCOff;
        pLiteralTable = g_arrCLit;
        nBias = 3;
        nShift = 7;
    }

    ZxHuffman lengthTree;
    ZxHuffman offsetTree;
    ZxHuffman literalTree;
    if (!lengthTree.build(pLengthTable, 64)) return false;
    if (!offsetTree.build(pOffsetTable, 64)) return false;
    if (pLiteralTable && !literalTree.build(pLiteralTable, 256)) return false;

    ZxBits bits((const quint8 *)baPacked.constData(), baPacked.size());
    QByteArray baRing(ZXZIP_RING_SIZE, (char)0);
    quint8 *pRing = (quint8 *)baRing.data();
    qint32 nPosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nOutputSize);
    qint64 nLeft = nOutputSize;

    while (nLeft > 0) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nFlag = bits.get(1);
        if (nFlag < 0) return false;

        if (nFlag == 0) {
            const qint32 nLow = bits.get(nShift);
            if (nLow < 0) return false;
            const qint32 nHigh = offsetTree.decode(&bits);
            if (nHigh < 0) return false;
            const qint32 nDistance = nLow + (nHigh << nShift);
            qint32 nLength = lengthTree.decode(&bits);
            if (nLength < 0) return false;
            if (nLength == 0x3f) {
                const qint32 nExtra = bits.get(8);
                if (nExtra < 0) return false;
                nLength += nExtra;
            }
            qint32 nSource = (nPosition - nDistance - 1) & (ZXZIP_RING_SIZE - 1);
            for (qint32 i = 0; i < (nLength + nBias); ++i) {
                const quint8 nByte = pRing[nSource];
                nSource = (nSource + 1) & (ZXZIP_RING_SIZE - 1);
                pRing[nPosition] = nByte;
                nPosition = (nPosition + 1) & (ZXZIP_RING_SIZE - 1);
                baOut.append((char)nByte);
                --nLeft;
            }
        } else {
            qint32 nByte = 0;
            if (pLiteralTable) {
                nByte = literalTree.decode(&bits);
                if (nByte < 0) return false;
            } else {
                nByte = bits.get(8);
                if (nByte < 0) return false;
            }
            pRing[nPosition] = (quint8)nByte;
            nPosition = (nPosition + 1) & (ZXZIP_RING_SIZE - 1);
            baOut.append((char)(quint8)nByte);
            --nLeft;
        }
    }

    *pbaResult = baOut;

    return nLeft <= 0;
}

// method 2 - PKZIP "Shrink"
bool zxShrinkDecode(const QByteArray &baPacked, qint64 nOutputSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    ZxBits bits((const quint8 *)baPacked.constData(), baPacked.size());

    QVector<qint32> listPrefix(ZXZIP_SHRINK_CODES, 0);
    QVector<quint8> listSuffix(ZXZIP_SHRINK_CODES, 0);
    QVector<quint8> listFree(ZXZIP_SHRINK_CODES, 0);
    QVector<quint8> listUsed(ZXZIP_SHRINK_CODES, 0);
    QVector<quint8> listStack(ZXZIP_SHRINK_CODES, 0);
    for (qint32 i = ZXZIP_SHRINK_FIRST; i < ZXZIP_SHRINK_CODES; ++i) listFree[i] = 1;

    qint32 nWidth = 9;
    qint32 nNextFree = ZXZIP_SHRINK_FIRST;
    qint32 nPrevious = 0;
    bool bHavePrevious = false;

    QByteArray baOut;
    baOut.reserve((qint32)nOutputSize);
    qint64 nLeft = nOutputSize;

    while (nLeft > 0) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nCode = bits.get(nWidth);
        if (nCode < 0) return false;
        if ((nCode < 0) || (nCode >= ZXZIP_SHRINK_CODES)) return false;
        if (listFree.at(nCode)) return false;

        if (nCode == ZXZIP_SHRINK_ESCAPE) {
            const qint32 nSub = bits.get(nWidth);
            if (nSub < 0) return false;
            if (nSub == 1) {
                if (nWidth < ZXZIP_SHRINK_MAX_WIDTH) ++nWidth;
            } else if (nSub == 2) {
                // partial clear: free every code that has no children
                if (bHavePrevious && (nNextFree >= 1)) listFree[nNextFree - 1] = 1;
                for (qint32 i = 0; i < ZXZIP_SHRINK_CODES; ++i) listUsed[i] = 0;
                for (qint32 i = ZXZIP_SHRINK_FIRST; i < ZXZIP_SHRINK_CODES; ++i) {
                    if (!listFree.at(i)) {
                        const qint32 nParent = listPrefix.at(i);
                        if ((nParent >= 0) && (nParent < ZXZIP_SHRINK_CODES)) listUsed[nParent] = 1;
                    }
                }
                for (qint32 i = ZXZIP_SHRINK_FIRST; i < ZXZIP_SHRINK_CODES; ++i) {
                    if (!listUsed.at(i)) listFree[i] = 1;
                }
                nNextFree = ZXZIP_SHRINK_FIRST;
                while ((nNextFree < ZXZIP_SHRINK_CODES) && (listFree.at(nNextFree) == 0)) ++nNextFree;
                if (nNextFree < ZXZIP_SHRINK_CODES) {
                    bHavePrevious = true;
                    listFree[nNextFree] = 0;
                    listPrefix[nNextFree] = nPrevious;
                    ++nNextFree;
                }
            } else {
                return false;
            }
            continue;
        }

        qint32 nStackSize = 0;
        qint32 c = nCode;
        qint32 nFix = -1;
        qint32 nSeen = -1;
        while (c > 0xff) {
            if ((c >= ZXZIP_SHRINK_CODES) || (nStackSize >= (ZXZIP_SHRINK_CODES - 1))) return false;
            if (c == (nNextFree - 1)) nSeen = nStackSize;
            listStack[nStackSize++] = listSuffix.at(c);
            nFix = nSeen;
            c = listPrefix.at(c);
        }
        if ((c < 0) || (nStackSize >= ZXZIP_SHRINK_CODES)) return false;
        listStack[nStackSize++] = (quint8)c;
        if (bHavePrevious && (nNextFree >= 1)) {
            listSuffix[nNextFree - 1] = (quint8)c;
            if (nFix >= 0) listStack[nFix] = (quint8)c;
        }
        for (qint32 i = nStackSize - 1; i >= 0; --i) {
            baOut.append((char)listStack.at(i));
            --nLeft;
        }

        while ((nNextFree < ZXZIP_SHRINK_CODES) && (listFree.at(nNextFree) == 0)) ++nNextFree;
        bHavePrevious = (nNextFree < ZXZIP_SHRINK_CODES);
        nPrevious = nCode;
        if (bHavePrevious) {
            listFree[nNextFree] = 0;
            listPrefix[nNextFree] = nCode;
            ++nNextFree;
        }
    }

    *pbaResult = baOut;

    return nLeft == 0;
}

}  // namespace

QByteArray XZXZIPDecoder::packProperties(const QByteArray &baEntry)
{
    if (baEntry.size() != ENTRY_SIZE) return QByteArray();

    return baEntry;
}

bool XZXZIPDecoder::memberSize(const QByteArray &baEntry, qint64 *pnDataSize, qint64 *pnPaddedSize)
{
    if (!pnDataSize || !pnPaddedSize) return false;
    if (baEntry.size() != ENTRY_SIZE) return false;
    const uchar *pEntry = (const uchar *)baEntry.constData();

    const char cType = (char)pEntry[8];
    const qint64 nStart = qFromLittleEndian<quint16>(pEntry + 9);
    const qint64 nLength = qFromLittleEndian<quint16>(pEntry + 11);
    const qint64 nSectors = pEntry[13];

    qint64 nSize = ((cType == 'B') || (cType == 'b')) ? (nStart + 4) : nLength;
    qint64 nRounded = (nSize + 0xff) & ~(qint64)0xff;
    if (nRounded != (nSectors * 256)) {
        nSize = nSectors * 256;
        nRounded = nSize;
    }
    if ((nSize < 0) || (nRounded < 0) || (nRounded > MAX_UNCOMPRESSED_SIZE)) return false;

    *pnDataSize = nSize;
    *pnPaddedSize = nRounded;

    return true;
}

QByteArray XZXZIPDecoder::hobetaHeader(const QByteArray &baEntry)
{
    if (baEntry.size() != ENTRY_SIZE) return QByteArray();

    QByteArray baHeader = baEntry.left(13);
    baHeader.append((char)0);
    baHeader.append((char)(quint8)baEntry.at(13));  // the sector count

    quint32 nSum = 0;
    for (qint32 i = 0; i < 15; ++i) nSum += (quint8)baHeader.at(i);
    const quint16 nCheck = (quint16)(((nSum & 0xffff) * 0x101 + 0x69) & 0xffff);
    baHeader.append((char)(quint8)(nCheck & 0xff));
    baHeader.append((char)(quint8)((nCheck >> 8) & 0xff));

    return baHeader;
}

bool XZXZIPDecoder::decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                           XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baProperties.size() != ENTRY_SIZE) return false;

    qint64 nDataSize = 0;
    qint64 nPaddedSize = 0;
    if (!memberSize(baProperties, &nDataSize, &nPaddedSize)) return false;

    const QByteArray baHeader = hobetaHeader(baProperties);
    if (baHeader.size() != HOBETA_SIZE) return false;

    const quint8 nMethod = (quint8)baProperties.at(0x14);
    const quint8 nSubMethod = (quint8)baProperties.at(0x15);

    QByteArray baData;
    if (nMethod == METHOD_STORE) {
        if (baPacked.size() != nDataSize) return false;
        baData = baPacked;
    } else if (nMethod == METHOD_SHRINK) {
        if (!zxShrinkDecode(baPacked, nDataSize, &baData, pPdStruct)) return false;
    } else if (nMethod == METHOD_LZH) {
        if (!zxLzhDecode(baPacked, nDataSize, nSubMethod, &baData, pPdStruct)) return false;
    } else {
        // method 1 is unreachable - see the header
        return false;
    }
    if (baData.size() < nDataSize) return false;
    if (baData.size() > nPaddedSize) return false;

    QByteArray baOut = baHeader;
    baOut.append(baData);
    if (baOut.size() < (HOBETA_SIZE + nPaddedSize)) baOut.append(QByteArray((int)(HOBETA_SIZE + nPaddedSize - baOut.size()), (char)0));

    if ((nUncompressedSize >= 0) && (baOut.size() != nUncompressedSize)) return false;
    *pbaResult = baOut;

    return true;
}
