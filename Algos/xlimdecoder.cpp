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
#include "xlimdecoder.h"

namespace {
const qint32 LIM_WINDOW = 0x8000;
const qint32 LIM_MAIN_SYMBOLS = 0x129;   // 256 literals + 41 match codes
const qint32 LIM_DIST_SYMBOLS = 0x1d;
const qint32 LIM_LENGTH_SYMBOLS = 20;
const qint32 LIM_SHORT_CODES = 13;

const quint16 g_arrCode[41] = {0,   1,   2,   4,   8,   16,  32,  64,   128,  256,  512,  1024, 2048, 4,    5,
                               6,   7,   8,   9,   10,  11,  268, 270,  272,  274,  532,  536,  540,  544,  804,
                               812, 820, 828, 1092, 1108, 1124, 1140, 1412, 1444, 1476, 1508};
const quint8 g_arrDistanceExtra[LIM_DIST_SYMBOLS] = {0, 0, 1, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5, 6, 6,
                                                     7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
const quint16 g_arrDistanceBase[LIM_DIST_SYMBOLS] = {0,   1,   2,    4,    6,    8,    12,   16,   24,   32,
                                                     48,  64,  96,   128,  192,  256,  384,  512,  768,  1024,
                                                     1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576};
const quint8 g_arrDefaultDistanceLengths[LIM_DIST_SYMBOLS] = {4, 6, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
                                                              5, 5, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5};

class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nAccumulator(0), m_nCount(0)
    {
    }

    // -1 means the stream is exhausted
    qint32 get(qint32 nBits)
    {
        if (nBits == 0) return 0;
        while (m_nCount < nBits) {
            if (m_nPosition > m_nSize) return -1;
            const quint32 nByte = (m_nPosition < m_nSize) ? m_pData[m_nPosition] : 0;
            ++m_nPosition;
            m_nAccumulator = (m_nAccumulator + (nByte << (24 - m_nCount))) & 0xffffffffU;
            m_nCount += 8;
        }
        const qint32 nValue = (qint32)(m_nAccumulator >> (32 - nBits));
        m_nAccumulator = (m_nAccumulator << nBits) & 0xffffffffU;
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

// Canonical Huffman: codes are assigned in increasing length then increasing
// symbol.  Decoding inverts each stream bit - see the header.
class Huffman {
public:
    bool build(const quint8 *pLengths, qint32 nCount)
    {
        m_nMaxLength = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            if (pLengths[i] > 32) return false;
            if (pLengths[i] > m_nMaxLength) m_nMaxLength = pLengths[i];
        }
        m_mapCodes.clear();
        quint32 nCode = 0;
        for (qint32 nLength = 1; nLength <= m_nMaxLength; ++nLength) {
            for (qint32 i = 0; i < nCount; ++i) {
                if (pLengths[i] == nLength) {
                    m_mapCodes.insert(((quint64)nLength << 32) | nCode, (quint16)i);
                    ++nCode;
                }
            }
            nCode <<= 1;
        }
        return true;
    }

    qint32 decode(BitReader *pReader) const
    {
        quint32 nCode = 0;
        for (qint32 nLength = 1; nLength <= m_nMaxLength; ++nLength) {
            const qint32 nBit = pReader->get(1);
            if (nBit < 0) return -1;
            nCode = (nCode << 1) | (quint32)(1 - nBit);
            const QHash<quint64, quint16>::const_iterator it = m_mapCodes.constFind(((quint64)nLength << 32) | nCode);
            if (it != m_mapCodes.constEnd()) return (qint32)it.value();
        }
        return -1;
    }

private:
    QHash<quint64, quint16> m_mapCodes;
    qint32 m_nMaxLength = 0;
};

// The reference implementation - the variable-length encoding the code-length table uses
qint32 readLengthCode(BitReader *pReader)
{
    qint32 nValue = pReader->get(2);
    if (nValue < 0) return -1;
    if (nValue == 1) return 3;
    const qint32 nBit = pReader->get(1);
    if (nBit < 0) return -1;
    nValue = nValue * 2 + nBit;
    if (nValue == 1) return 2;
    if (nValue == 7) {
        nValue = 6;
        while (true) {
            const qint32 nNext = pReader->get(1);
            if (nNext < 0) return -1;
            ++nValue;
            if (nNext != 0) break;
        }
        if (nValue == 0x0d) return 1;
        if (nValue > 0x0d) return nValue - 1;
    }
    return nValue;
}

// The reference implementation
bool readTables(BitReader *pReader, Huffman *pMain, Huffman *pDistance)
{
    const qint32 nStart = pReader->get(3);
    qint32 nCount = pReader->get(4);
    if ((nStart < 0) || (nCount < 0)) return false;

    quint8 arrLengths[LIM_LENGTH_SYMBOLS];
    memset(arrLengths, 0, sizeof(arrLengths));
    qint32 nValue = readLengthCode(pReader);
    if (nValue < 0) return false;
    arrLengths[0] = (quint8)nValue;
    qint32 nIndex = nStart;
    while (nCount != 0) {
        ++nIndex;
        nValue = readLengthCode(pReader);
        if ((nValue < 0) || (nIndex >= LIM_LENGTH_SYMBOLS)) return false;
        arrLengths[nIndex] = (quint8)nValue;
        --nCount;
    }
    for (qint32 k = 17; k <= 19; ++k) {
        nValue = readLengthCode(pReader);
        if (nValue < 0) return false;
        arrLengths[k] = (quint8)nValue;
    }

    Huffman lengthTree;
    if (!lengthTree.build(arrLengths, LIM_LENGTH_SYMBOLS)) return false;

    QByteArray baMainLengths(LIM_MAIN_SYMBOLS, (char)0);
    quint8 *pMainLengths = (quint8 *)baMainLengths.data();
    qint32 nAt = 0;
    while (nAt < LIM_MAIN_SYMBOLS) {
        const qint32 nSymbol = lengthTree.decode(pReader);
        if (nSymbol < 0) return false;
        if (nSymbol < 0x11) {
            pMainLengths[nAt] = (quint8)nSymbol;
            ++nAt;
            continue;
        }
        qint32 nExtraBits = 7;
        qint32 nBase = 0x17;
        if (nSymbol == 0x11) {
            nExtraBits = 2;
            nBase = 3;
        } else if (nSymbol == 0x12) {
            nExtraBits = 4;
            nBase = 7;
        }
        const qint32 nExtra = pReader->get(nExtraBits);
        if ((nExtra < 0) || (nAt == 0)) return false;
        const quint8 nPrevious = pMainLengths[nAt - 1];
        for (qint32 k = 0; k < (nBase + nExtra); ++k) {
            if (nAt >= LIM_MAIN_SYMBOLS) return false;
            pMainLengths[nAt] = nPrevious;
            ++nAt;
        }
    }
    if (!pMain->build(pMainLengths, LIM_MAIN_SYMBOLS)) return false;

    const qint32 nFlag = pReader->get(1);
    if (nFlag < 0) return false;
    quint8 arrDistanceLengths[LIM_DIST_SYMBOLS];
    if (nFlag == 0) {
        memset(arrDistanceLengths, 0, sizeof(arrDistanceLengths));
        const qint32 nHow = pReader->get(5);
        if (nHow < 0) return false;
        for (qint32 i = 0; i < nHow; ++i) {
            const qint32 nBit = pReader->get(1);
            if (nBit < 0) return false;
            qint32 nLength = 0;
            if (nBit == 0) {
                const qint32 nSmall = pReader->get(1);
                if (nSmall < 0) return false;
                nLength = nSmall + 4;
            } else {
                nLength = readLengthCode(pReader);
                if (nLength < 0) return false;
                if (nLength > 3) nLength += 2;
            }
            if (i >= LIM_DIST_SYMBOLS) return false;
            arrDistanceLengths[i] = (quint8)nLength;
        }
    } else {
        memcpy(arrDistanceLengths, g_arrDefaultDistanceLengths, sizeof(arrDistanceLengths));
    }

    return pDistance->build(arrDistanceLengths, LIM_DIST_SYMBOLS);
}
}  // namespace

bool XLIMDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    BitReader reader((const quint8 *)baPacked.constData(), baPacked.size());
    QByteArray baWindow(LIM_WINDOW, (char)0);
    QByteArray baHistory(LIM_WINDOW, (char)0);
    quint8 *pWindow = (quint8 *)baWindow.data();
    quint8 *pHistory = (quint8 *)baHistory.data();
    qint32 nPosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    bool bDone = false;
    while (!bDone) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nBlock = reader.get(14);
        if (nBlock <= 0) break;

        Huffman mainTree;
        Huffman distanceTree;
        if (!readTables(&reader, &mainTree, &distanceTree)) break;

        for (qint32 nLeft = nBlock; nLeft > 0; --nLeft) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const qint32 nSymbol = mainTree.decode(&reader);
            if (nSymbol < 0) {
                bDone = true;
                break;
            }
            if (nSymbol < 0x100) {
                pWindow[nPosition] = (quint8)nSymbol;
                pHistory[nPosition] = 0;
                nPosition = (nPosition + 1) & (LIM_WINDOW - 1);
                baOut.append((char)(quint8)nSymbol);
                continue;
            }

            const qint32 k = nSymbol - 0x100;
            if (k >= 41) {
                bDone = true;
                break;
            }
            qint32 nDistance = 0;
            qint32 nBaseLength = 0;
            if (k < LIM_SHORT_CODES) {
                nDistance = g_arrCode[k];
                if ((nSymbol & 0xff) > 1) {
                    const qint32 nExtra = reader.get(k - 1);
                    if (nExtra < 0) {
                        bDone = true;
                        break;
                    }
                    nDistance += nExtra;
                }
                nBaseLength = 3;
            } else {
                qint32 nValue = g_arrCode[k];
                if (nValue > 0xff) {
                    const qint32 nExtra = reader.get(nValue >> 8);
                    if (nExtra < 0) {
                        bDone = true;
                        break;
                    }
                    nValue = (nValue & 0xff00) | (((nValue & 0xff) + nExtra) & 0xff);
                }
                nBaseLength = nValue & 0xff;
                const qint32 nDistanceSymbol = distanceTree.decode(&reader);
                if ((nDistanceSymbol < 0) || (nDistanceSymbol >= LIM_DIST_SYMBOLS)) {
                    bDone = true;
                    break;
                }
                nDistance = g_arrDistanceBase[nDistanceSymbol];
                if (g_arrDistanceExtra[nDistanceSymbol]) {
                    const qint32 nExtra = reader.get(g_arrDistanceExtra[nDistanceSymbol]);
                    if (nExtra < 0) {
                        bDone = true;
                        break;
                    }
                    nDistance += nExtra;
                }
            }

            const qint32 nSource = (~(nDistance - nPosition)) & (LIM_WINDOW - 1);
            const qint32 nWide = nBaseLength + pHistory[nSource];
            qint32 nLength = nWide & 0xff;
            if (nWide > 0xff) nLength = (nLength + 4) & 0xff;

            qint32 p = nPosition;
            for (qint32 i = 0; i < nLength; ++i) {
                pHistory[p] = 0;
                p = (p + 1) & (LIM_WINDOW - 1);
            }
            qint32 q = nSource;
            for (qint32 v = (nLength - 3) & 0xff; v != 0; --v) {
                pHistory[q] = (quint8)v;
                q = (q + 1) & (LIM_WINDOW - 1);
            }

            qint32 s = nSource;
            for (qint32 i = 0; i < nLength; ++i) {
                const quint8 nByte = pWindow[s];
                pWindow[nPosition] = nByte;
                baOut.append((char)nByte);
                nPosition = (nPosition + 1) & (LIM_WINDOW - 1);
                s = (s + 1) & (LIM_WINDOW - 1);
            }
            if (baOut.size() >= nUncompressedSize) {
                bDone = true;
                break;
            }
        }
    }

    if (baOut.size() > nUncompressedSize) baOut.truncate((qint32)nUncompressedSize);
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
