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
#include "ximpdecoder.h"

#include <QtEndian>

namespace {
const qint32 IMP_EMPTY = -0x120;
const qint32 IMP_SLACK = 0x200;
const qint32 IMP_LITERAL_SYMBOLS = 0x120;
const qint32 IMP_CL_SYMBOLS = 0x15;
const qint32 IMP_BWT_SYMBOLS = 0x102;
const qint32 IMP_MAX_MULTIPLIER = 0x1000000;
const qint64 IMP_MAX_WINDOW = 0x80000;

const qint32 g_arrDistSizes[3] = {0x2a, 0x0e, 0x1c};
const qint32 g_arrDistOffsets[3] = {0x120, 0x14a, 0x158};

const quint32 g_arrLenBase[20] = {11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227};
const quint8 g_arrLenExtra[20] = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};
const quint32 g_arrLenMask[20] = {1, 1, 1, 1, 3, 3, 3, 3, 7, 7, 7, 7, 15, 15, 15, 15, 31, 31, 31, 31};

const quint32 g_arrDistBase[36] = {5,     7,     9,     13,    17,    25,    33,    49,    65,     97,     129,    193,
                                   257,   385,   513,   769,   1025,  1537,  2049,  3073,  4097,   6145,   8193,   12289,
                                   16385, 24577, 32769, 49153, 65537, 98305, 131073, 196609, 262145, 393217, 524289, 786433};
const quint8 g_arrDistExtra[36] = {1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9,
                                   10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18};
const quint32 g_arrDistMask[36] = {1,     1,     3,     3,     7,     7,     15,     15,     31,     31,     63,     63,
                                   127,   127,   255,   255,   511,   511,   1023,   1023,   2047,   2047,   4095,   4095,
                                   8191,  8191,  16383, 16383, 32767, 32767, 65535,  65535,  131071, 131071, 262143, 262143};

const quint32 g_arrWindows[3][8] = {{0x8000, 0x10000, 0x12000, 0x18000, 0x1e000, 0x24000, 0x2a000, 0x30000},
                                    {0x8000, 0x10000, 0x12000, 0x18000, 0x1e000, 0x24000, 0x2a000, 0x30000},
                                    {0x8000, 0x20000, 0x30000, 0x40000, 0x50000, 0x60000, 0x70000, 0x80000}};

// ---------------------------------------------------------------- CRC-32 ---
class ImpCrcTable {
public:
    ImpCrcTable()
    {
        for (qint32 i = 0; i < 256; ++i) {
            quint32 nValue = (quint32)i;
            for (qint32 j = 0; j < 8; ++j) nValue = (nValue >> 1) ^ ((nValue & 1) ? 0xedb88320U : 0);
            m_arrTable[i] = nValue;
        }
    }

    quint32 at(qint32 nIndex) const
    {
        return m_arrTable[nIndex];
    }

private:
    quint32 m_arrTable[256];
};

const ImpCrcTable g_crcTable;

// ------------------------------------------------------------ bit reader ---
// LSB first over a 32-bit window, refilled to 25 bits at a time, reading zeros
// past the end of the buffer.
class ImpBitReader {
public:
    ImpBitReader(const quint8 *pData, qint64 nSize, qint64 nPosition)
        : m_pData(pData), m_nSize(nSize), m_nPosition(nPosition), m_nBuffer(0), m_nCount(0), m_bError(false)
    {
        if ((m_nPosition < 0) || (m_nPosition > m_nSize)) {
            m_nPosition = m_nSize;
            m_bError = true;
        }
    }

    quint32 peek(qint32 nBits)
    {
        if (m_nCount < 0) {
            m_bError = true;
            return 0;
        }
        if (m_nCount < nBits) {
            while (m_nCount < 25) {
                const quint32 nByte = (m_nPosition < m_nSize) ? m_pData[m_nPosition] : 0;
                ++m_nPosition;
                m_nBuffer |= nByte << m_nCount;
                m_nCount += 8;
            }
        }

        return m_nBuffer;
    }

    void drop(qint32 nBits)
    {
        m_nCount -= nBits;
        if (nBits >= 32) m_nBuffer = 0;
        else m_nBuffer >>= nBits;
        if (m_nCount < 0) m_bError = true;
    }

    quint32 get(qint32 nBits)
    {
        if (nBits <= 0) return 0;
        const quint32 nValue = peek(nBits) & ((nBits >= 32) ? 0xffffffffU : ((1U << nBits) - 1));
        drop(nBits);

        return nValue;
    }

    void align()
    {
        drop(m_nCount & 7);
    }

    // the offset of the next unconsumed byte (the stored-block rewind rule)
    qint64 bytePosition() const
    {
        return m_nPosition - (m_nCount >> 3);
    }

    bool isError() const
    {
        return m_bError;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nBuffer;
    qint32 m_nCount;
    bool m_bError;
};

// -------------------------------------------------------- huffman helpers ---
void impMakeCodes(const quint8 *pLengths, qint32 nCount, QVector<quint32> *plistCodes)
{
    qint32 arrCount[17];
    qint32 arrNext[17];
    for (qint32 i = 0; i < 17; ++i) {
        arrCount[i] = 0;
        arrNext[i] = 0;
    }
    // lengths run up to 16, so masking them to four bits would silently turn a
    // 16-bit code into an absent one
    for (qint32 i = 0; i < nCount; ++i) {
        if (pLengths[i] > 16) continue;
        arrCount[pLengths[i]]++;
    }
    arrCount[0] = 0;
    quint32 nCode = 0;
    for (qint32 nBit = 1; nBit < 17; ++nBit) {
        nCode = (nCode + (quint32)arrCount[nBit - 1]) * 2;
        arrNext[nBit] = (qint32)nCode;
    }
    plistCodes->fill(0, nCount);
    for (qint32 i = 0; i < nCount; ++i) {
        const qint32 nLength = (pLengths[i] > 16) ? 0 : pLengths[i];
        if (!nLength) continue;
        quint32 nCurrent = (quint32)arrNext[nLength];
        arrNext[nLength] = (qint32)(nCurrent + 1);
        quint32 nReversed = 0;
        for (qint32 j = 0; j < nLength; ++j) {
            nReversed = nReversed * 2 + (nCurrent & 1);
            nCurrent >>= 1;
        }
        (*plistCodes)[i] = nReversed;
    }
}

// The reference implementation - a direct lookup table of nBits bits plus an overflow tree
bool impBuild(const QVector<quint32> &listCodes, const quint8 *pLengths, qint32 nBits, qint32 nCount, bool bWantTree, QVector<qint32> *plistTable,
              QVector<qint32> *plistTree)
{
    const qint32 nSize = 1 << nBits;
    plistTable->fill(IMP_EMPTY, nSize);
    if (bWantTree) plistTree->fill(IMP_EMPTY, nCount * 2);
    else plistTree->clear();

    qint32 nNode = 1;
    for (qint32 nSymbol = 0; nSymbol < nCount; ++nSymbol) {
        const qint32 nLength = pLengths[nSymbol];
        if ((nLength == 0) || (nLength > 16)) continue;
        const qint32 nSpan = 1 << nLength;
        const quint32 nCode = listCodes.at(nSymbol);
        if (nSize < nSpan) {
            if (!bWantTree) continue;
            qint32 nRemaining = nLength - nBits - 1;
            const qint32 nIndex = (qint32)(nCode & (quint32)(nSize - 1));
            quint32 nRest = nCode >> nBits;
            qint32 nCurrent = 0;
            qint32 nNext = 0;
            if (plistTable->at(nIndex) == IMP_EMPTY) {
                (*plistTable)[nIndex] = -nNode;
                nCurrent = nNode;
                nNext = nNode + 1;
            } else {
                nCurrent = -plistTable->at(nIndex);
                nNext = nNode;
                if ((nCurrent < 1) || (nCurrent >= nCount)) return false;
                qint32 nGuard = 0;
                while (plistTree->at(nCurrent * 2 + (qint32)(nRest & 1)) >= 0) {
                    nCurrent = plistTree->at(nCurrent * 2 + (qint32)(nRest & 1));
                    nRest >>= 1;
                    --nRemaining;
                    if ((nCurrent < 1) || (nCurrent >= nCount)) return false;
                    if (++nGuard > nCount) return false;
                }
            }
            while (true) {
                nNode = nNext;
                if (nRemaining == 0) break;
                if (nRemaining < 0) return false;
                --nRemaining;
                if (nNode >= nCount) return false;
                if ((nCurrent < 1) || (nCurrent >= nCount)) return false;
                (*plistTree)[nCurrent * 2 + (qint32)(nRest & 1)] = nNode;
                nRest >>= 1;
                nCurrent = nNode;
                nNext = nNode + 1;
            }
            if ((nCurrent < 1) || (nCurrent >= nCount)) return false;
            (*plistTree)[nCurrent * 2 + (qint32)(nRest & 1)] = -nSymbol;
        } else {
            for (qint32 c = (qint32)nCode; c < nSize; c += nSpan) (*plistTable)[c] = nSymbol;
        }
    }

    return true;
}

// -1 on a bad code
qint32 impDecodeSymbol(ImpBitReader *pReader, const QVector<qint32> &listTable, const QVector<qint32> &listTree, qint32 nBits, qint32 nCount)
{
    qint32 nSymbol = listTable.at((qint32)(pReader->peek(nBits) & (quint32)((1 << nBits) - 1)));
    if (nSymbol < 0) {
        if (nSymbol == IMP_EMPTY) return -1;
        quint32 nRest = pReader->peek(16) >> nBits;
        nSymbol = -nSymbol;
        qint32 nGuard = 0;
        while (true) {
            if ((nSymbol < 1) || ((nSymbol * 2 + 1) >= listTree.size())) return -1;
            nSymbol = listTree.at(nSymbol * 2 + (qint32)(nRest & 1));
            nRest >>= 1;
            if (nSymbol <= 0) break;
            if (++nGuard > nCount) return -1;
        }
        if (nSymbol == IMP_EMPTY) return -1;
        nSymbol = -nSymbol;
    }
    if ((nSymbol < 0) || (nSymbol >= nCount)) return -1;

    return nSymbol;
}

// -------------------------------------------------- code length array ------
void impDefaultLengths(quint8 *pOut, qint64 nWindow)
{
    for (qint32 i = 0x00; i < 0xf4; ++i) pOut[i] = 8;
    for (qint32 i = 0xf4; i < 0x105; ++i) pOut[i] = 9;
    pOut[0x100] = 0;
    for (qint32 i = 0x105; i < 0x10a; ++i) pOut[i] = 10;
    for (qint32 i = 0x10a; i < 0x120; ++i) pOut[i] = 11;
    if (nWindow < 0x2001) {
        for (qint32 i = 0x120; i < 0x124; ++i) pOut[i] = 4;
        for (qint32 i = 0x124; i < 0x13c; ++i) pOut[i] = 5;
        for (qint32 i = 0x13c; i < 0x14a; ++i) pOut[i] = 0;
    } else {
        for (qint32 i = 0x120; i < 0x136; ++i) pOut[i] = 5;
        for (qint32 i = 0x136; i < 0x14a; ++i) pOut[i] = 6;
    }
    for (qint32 i = 0x14a; i < 0x158; ++i) pOut[i] = 4;
    pOut[0x14a] = 3;
    pOut[0x14b] = 3;
}

bool impReadLengths(ImpBitReader *pReader, QByteArray *pbaOut, qint32 nStride, qint32 nTables, qint64 nWindow)
{
    quint8 *pOut = (quint8 *)pbaOut->data();
    if (nTables == 0) {
        if (pbaOut->size() < 0x158) return false;
        impDefaultLengths(pOut, nWindow);
        return true;
    }

    quint8 arrCodeLengths[IMP_CL_SYMBOLS];
    memset(arrCodeLengths, 0, sizeof(arrCodeLengths));
    qint32 nPosition = 0;
    qint32 nMultiplier = 1;
    while (nPosition < IMP_CL_SYMBOLS) {
        const qint32 nValue = (qint32)pReader->get(5);
        if (pReader->isError()) return false;
        if (nValue < 2) {
            nPosition += nMultiplier << nValue;
            // the reference lets the multiplier double without bound; saturating
            // it is equivalent, because every count it feeds is clamped to a
            // range far below the saturation value
            if (nMultiplier < IMP_MAX_MULTIPLIER) nMultiplier *= 2;
        } else {
            if (nValue > 0x11) return false;
            arrCodeLengths[nPosition] = (quint8)(nValue - 1);
            ++nPosition;
            nMultiplier = 1;
        }
    }

    QVector<quint32> listCodes;
    QVector<qint32> listTable;
    QVector<qint32> listTree;
    impMakeCodes(arrCodeLengths, IMP_CL_SYMBOLS, &listCodes);
    if (!impBuild(listCodes, arrCodeLengths, 6, IMP_CL_SYMBOLS, true, &listTable, &listTree)) return false;

    const qint64 nEnd = (qint64)nStride * nTables;
    if (nEnd > pbaOut->size()) return false;

    qint32 nPrevious = 0;
    qint32 nRepeatMultiplier = 1;
    qint32 nZeroMultiplier = 1;
    qint64 p = 0;
    while (p < nEnd) {
        const qint32 nSymbol = impDecodeSymbol(pReader, listTable, listTree, 6, IMP_CL_SYMBOLS);
        if (nSymbol < 0) return false;
        pReader->drop(arrCodeLengths[nSymbol]);
        if (pReader->isError()) return false;

        if (nSymbol < 0x13) {
            if (nSymbol < 0x11) {
                if ((p >= nStride) && (pOut[p - nStride] != 0)) nPrevious = pOut[p - nStride];
                nPrevious = nSymbol + nPrevious;
                if (nPrevious > 0x10) nPrevious -= 0x11;
                pOut[p] = (quint8)nPrevious;
                ++p;
                nRepeatMultiplier = 1;
                nZeroMultiplier = 1;
            } else {
                qint64 nRun = (qint64)nZeroMultiplier << (nSymbol - 0x11);
                while (nRun && (p < nEnd)) {
                    --nRun;
                    pOut[p] = 0;
                    ++p;
                }
                if (nZeroMultiplier < IMP_MAX_MULTIPLIER) nZeroMultiplier *= 2;
                nRepeatMultiplier = 1;
            }
        } else {
            qint64 nRun = (qint64)nRepeatMultiplier << (nSymbol - 0x13);
            while (nRun && (p < nEnd)) {
                --nRun;
                if ((p >= nStride) && (pOut[p - nStride] != 0)) nPrevious = pOut[p - nStride];
                pOut[p] = (quint8)nPrevious;
                ++p;
            }
            if (nRepeatMultiplier < IMP_MAX_MULTIPLIER) nRepeatMultiplier *= 2;
            nZeroMultiplier = 1;
        }
    }

    return true;
}

// ------------------------------------------------------- the delta filter --
struct ImpFilterRecord {
    qint64 nPosition;
    qint32 nDistance;
};

typedef QVector<ImpFilterRecord> ImpFilterState;

// The reference implementation. A source index below zero cannot happen for a well formed
// stream; the reference would read outside its buffer, so it reads as zero.
void impDeltaApply(const ImpFilterState &listRecords, quint8 *pBuffer, qint64 nBufferSize, qint64 nTotal)
{
    for (qint32 i = 0; i < listRecords.size(); ++i) {
        const qint32 nDistance = listRecords.at(i).nDistance;
        if (nDistance == 0) continue;
        const qint64 nFrom = listRecords.at(i).nPosition;
        qint64 nTo = ((i + 1) < listRecords.size()) ? listRecords.at(i + 1).nPosition : nTotal;
        if (nTo > nBufferSize) nTo = nBufferSize;
        for (qint64 p = nFrom; p < nTo; ++p) {
            if (p < 0) continue;
            const quint8 nAddend = ((p - nDistance) >= 0) ? pBuffer[p - nDistance] : 0;
            pBuffer[p] = (quint8)(pBuffer[p] + nAddend);
        }
    }
}

// The reference implementation - undo the filter over the window that will be kept
void impDeltaUnapply(const ImpFilterState &listRecords, quint8 *pBuffer, qint64 nBufferSize, qint64 nDictLength, qint64 nKeep)
{
    qint64 nLow = nDictLength - nKeep;
    if (nLow < 0) nLow = 0;
    for (qint32 i = listRecords.size() - 1; i >= 0; --i) {
        const qint32 nDistance = listRecords.at(i).nDistance;
        if (nDistance == 0) continue;
        qint64 nFrom = listRecords.at(i).nPosition;
        if (nFrom < nLow) nFrom = nLow;
        qint64 nTo = ((i + 1) < listRecords.size()) ? listRecords.at(i + 1).nPosition : nDictLength;
        if (nTo > nBufferSize) nTo = nBufferSize;
        for (qint64 p = nTo - 1; p >= nFrom; --p) {
            if (p < 0) break;
            const quint8 nAddend = ((p - nDistance) >= 0) ? pBuffer[p - nDistance] : 0;
            pBuffer[p] = (quint8)(pBuffer[p] - nAddend);
        }
        if (listRecords.at(i).nPosition <= nLow) return;
    }
}

// -------------------------------------------------------- LZ77 + huffman ---
// The reference implementation. Decodes into pBuffer[nStart.. nStart+nBlockLength).
// *pnProduced gets the number of bytes actually produced.
bool impLzDecode(ImpBitReader *pReader, quint8 *pBuffer, qint64 nBufferSize, qint64 nStart, qint64 nBlockLength, ImpFilterState *pListRecords,
                 bool bApplyFilter, qint64 *pnProduced, XBinary::PDSTRUCT *pPdStruct)
{
    if (bApplyFilter) pListRecords->clear();
    if ((nStart < 0) || (nBlockLength < 0) || ((nStart + nBlockLength) > nBufferSize)) return false;

    const qint32 nMode = (qint32)pReader->get(2);
    if (nMode == 0) return false;
    const qint32 nBig = (nMode > 2) ? 1 : 0;
    const qint32 nStride = nBig ? 0x174 : 0x158;

    qint64 nOut = nStart;
    const qint64 nEnd = nStart + nBlockLength;
    qint32 nTables = (qint32)pReader->get(6);
    if (pReader->isError()) return false;

    const qint64 nAlloc = nTables ? ((qint64)nStride * nTables) : 0x158;
    if (nAlloc > 0x00400000) return false;
    QByteArray baLengths((int)(nAlloc + nStride + IMP_SLACK), (char)0);
    if (!impReadLengths(pReader, &baLengths, nStride, nTables, nStart + nBlockLength)) return false;
    const quint8 *pLengths = (const quint8 *)baLengths.constData();
    const qint64 nLengthsSize = baLengths.size();

    qint64 nPointer = -nStride;
    qint64 nRecent0 = 0;
    qint64 nRecent1 = 0;
    qint32 nSymbol = 0x100;
    if (nTables == 0) nTables = 1;

    QVector<quint32> listCodes;
    QVector<qint32> listLiteralTable;
    QVector<qint32> listLiteralTree;
    QVector<qint32> listDistTable[3];
    QVector<qint32> listDistTree[3];
    bool arrDistReady[3] = {false, false, false};
    bool bLiteralReady = false;

    while (nOut < nEnd) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (nSymbol == 0x100) {
            if (nTables == 0) break;
            --nTables;
            nPointer += nStride;
            const qint64 nRow = nPointer;
            if ((nRow < 0) || ((nRow + IMP_LITERAL_SYMBOLS) > nLengthsSize)) return false;
            impMakeCodes(pLengths + nRow, IMP_LITERAL_SYMBOLS, &listCodes);
            if (!impBuild(listCodes, pLengths + nRow, 9, IMP_LITERAL_SYMBOLS, true, &listLiteralTable, &listLiteralTree)) return false;
            bLiteralReady = true;
            nPointer += IMP_LITERAL_SYMBOLS;
            for (qint32 i = 0; i < nMode; ++i) {
                const qint32 nCount = g_arrDistSizes[i];
                if ((nPointer < 0) || ((nPointer + nCount) > nLengthsSize)) return false;
                impMakeCodes(pLengths + nPointer, nCount, &listCodes);
                if (!impBuild(listCodes, pLengths + nPointer, 7, nCount, true, &listDistTable[i], &listDistTree[i])) return false;
                arrDistReady[i] = true;
                nPointer += nCount;
            }
            nPointer -= nStride;
        }
        if (!bLiteralReady) return false;

        nSymbol = impDecodeSymbol(pReader, listLiteralTable, listLiteralTree, 9, IMP_LITERAL_SYMBOLS);
        if (nSymbol < 0) return false;
        // mode 1 leaves nPointer 0x0e bytes below the row - a real off-by-one
        // in the original, whose stride is hard coded 0x158 while only 0x14a
        // bytes of it are used
        const qint64 nLengthIndex = nPointer + nSymbol;
        if (nLengthIndex >= nLengthsSize) return false;
        pReader->drop((nLengthIndex >= 0) ? pLengths[nLengthIndex] : 0);
        if (pReader->isError()) return false;

        if (nSymbol < 0x100) {
            pBuffer[nOut] = (quint8)nSymbol;
            ++nOut;
            continue;
        }
        if (nSymbol == 0x100) continue;

        qint64 nLength = 0;
        if (nSymbol < 0x10a) {
            nLength = nSymbol - 0xff;
        } else if (nSymbol < 0x11e) {
            const qint32 k = nSymbol - 0x10a;
            const quint32 nValue = pReader->peek(5);
            pReader->drop(g_arrLenExtra[k]);
            nLength = (nValue & g_arrLenMask[k]) + g_arrLenBase[k];
        } else if (nSymbol == 0x11e) {
            nLength = (pReader->peek(15) & 0x7fff) + 0x103;
            pReader->drop(15);
        } else {
            // 0x11f: a delta post-filter record
            const qint32 nDistance = (qint32)pReader->get(3);
            ImpFilterRecord record;
            record.nPosition = nOut;
            record.nDistance = nDistance;
            pListRecords->append(record);
            continue;
        }
        if (pReader->isError()) return false;

        const qint32 nWhich = ((nLength == 2) ? 1 : 0) + ((nBig & ((nLength == 3) ? 1 : 0)) * 2);
        if ((nWhich < 0) || (nWhich > 2) || !arrDistReady[nWhich]) return false;
        const qint32 nDistSymbol = impDecodeSymbol(pReader, listDistTable[nWhich], listDistTree[nWhich], 7, g_arrDistSizes[nWhich]);
        if (nDistSymbol < 0) return false;
        const qint64 nDistIndex = nPointer + nDistSymbol + g_arrDistOffsets[nWhich];
        if (nDistIndex >= nLengthsSize) return false;
        pReader->drop((nDistIndex >= 0) ? pLengths[nDistIndex] : 0);
        if (pReader->isError()) return false;

        qint64 nDistance = 0;
        if (nDistSymbol == 0) {
            nDistance = nRecent0;
        } else if (nDistSymbol == 1) {
            nDistance = nRecent1;
            nRecent1 = nRecent0;
            nRecent0 = nDistance;
        } else if (nDistSymbol < 6) {
            nDistance = nDistSymbol - 1;
        } else {
            const qint32 k = nDistSymbol - 6;
            if (k >= 36) return false;
            const quint32 nValue = pReader->peek(18);
            pReader->drop(g_arrDistExtra[k]);
            nDistance = (nValue & g_arrDistMask[k]) + g_arrDistBase[k];
            nRecent1 = nRecent0;
            nRecent0 = nDistance;
        }
        if (pReader->isError()) return false;

        if ((nDistance <= nOut) && (nDistance >= 0)) {
            while (nLength && (nOut < nEnd)) {
                --nLength;
                pBuffer[nOut] = pBuffer[nOut - nDistance];
                ++nOut;
            }
        }
    }

    if (bApplyFilter) impDeltaApply(*pListRecords, pBuffer, nBufferSize, nStart + nBlockLength);
    if (pnProduced) *pnProduced = nOut - nStart;

    return true;
}

// ------------------------------------------------------------ BWT method ---
// The reference implementation. NOTE: it writes nBlockLength + 1 bytes into the buffer.
bool impBwtDecode(ImpBitReader *pReader, quint8 *pBuffer, qint64 nBufferSize, qint64 nStart, qint64 nBlockLength, XBinary::PDSTRUCT *pPdStruct)
{
    if ((nStart < 0) || (nBlockLength < 0) || ((nStart + nBlockLength + 1) > nBufferSize)) return false;

    const quint32 nAdd = pReader->get(8);
    const qint32 nTableCount = (qint32)pReader->get(3);
    if (pReader->isError() || (nTableCount < 1)) return false;

    QVector<QVector<qint32> > listTables;
    QVector<QVector<qint32> > listTrees;
    QList<QByteArray> listLengths;
    for (qint32 i = 0; i < nTableCount; ++i) {
        QByteArray baLengths(IMP_BWT_SYMBOLS + IMP_SLACK, (char)0);
        if (!impReadLengths(pReader, &baLengths, IMP_BWT_SYMBOLS, 1, 0)) return false;
        QVector<quint32> listCodes;
        QVector<qint32> listTable;
        QVector<qint32> listTree;
        impMakeCodes((const quint8 *)baLengths.constData(), IMP_BWT_SYMBOLS, &listCodes);
        if (!impBuild(listCodes, (const quint8 *)baLengths.constData(), 9, IMP_BWT_SYMBOLS, true, &listTable, &listTree)) return false;
        listTables.append(listTable);
        listTrees.append(listTree);
        listLengths.append(baLengths);
    }

    pReader->align();

    qint32 arrMtf[256];
    for (qint32 i = 0; i < 256; ++i) arrMtf[i] = i;
    qint32 arrSelector[8];
    for (qint32 i = 0; i < 8; ++i) arrSelector[i] = i;
    QVector<qint64> listCounts(256, 0);

    qint64 nOut = nStart;
    const qint64 nEnd = nStart + nBlockLength + 1;
    qint32 nGroup = 0x32;
    qint32 nCurrent = 0;
    qint64 nRunMultiplier = 1;
    qint64 nPrimary = 0;

    while (nOut < nEnd) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (nGroup == 0x32) {
            quint32 nValue = pReader->peek(nTableCount);
            qint32 j = 0;
            while (nValue & 1) {
                ++j;
                nValue >>= 1;
                if (j > nTableCount) break;
            }
            if (j >= nTableCount) return false;
            pReader->drop(j + 1);
            nCurrent = arrSelector[j];
            if (j) {
                while (j) {
                    arrSelector[j] = arrSelector[j - 1];
                    --j;
                }
                arrSelector[0] = nCurrent;
            }
            if ((nCurrent < 0) || (nCurrent >= nTableCount)) return false;
            nGroup = 0;
            continue;
        }
        ++nGroup;

        const qint32 nSymbol = impDecodeSymbol(pReader, listTables.at(nCurrent), listTrees.at(nCurrent), 9, IMP_BWT_SYMBOLS);
        if (nSymbol < 0) return false;
        pReader->drop((quint8)listLengths.at(nCurrent).at(nSymbol));
        if (pReader->isError()) return false;

        if (nSymbol < 2) {
            qint64 nRun = nRunMultiplier << nSymbol;
            if (nEnd < (nOut + nRun)) nRun = nEnd - nOut;
            const qint32 nByte = arrMtf[0];
            listCounts[nByte] += nRun;
            for (qint64 i = 0; i < nRun; ++i) {
                pBuffer[nOut] = (quint8)nByte;
                ++nOut;
            }
            if (nRunMultiplier < (qint64)IMP_MAX_MULTIPLIER) nRunMultiplier <<= 1;
        } else if (nSymbol < 0x101) {
            qint32 k = nSymbol - 1;
            const qint32 nByte = arrMtf[k];
            pBuffer[nOut] = (quint8)nByte;
            ++nOut;
            while (k) {
                arrMtf[k] = arrMtf[k - 1];
                --k;
            }
            arrMtf[0] = nByte;
            listCounts[nByte] += 1;
            nRunMultiplier = 1;
        } else {
            nPrimary = nOut - nStart;
            ++nOut;
            nRunMultiplier = 1;
        }
    }

    if ((nPrimary < 0) || (nPrimary > nBlockLength)) return false;
    if (nBlockLength > 0x00ffffff) return false;

    // inverse BWT
    QVector<qint64> listCumulative(257, 0);
    listCumulative[0] = 1;
    for (qint32 i = 0; i < 256; ++i) listCumulative[i + 1] = listCumulative.at(i) + listCounts.at(i);
    QVector<quint32> listTransform((qint32)(nBlockLength + 1), 0);
    QVector<qint64> listIndex(listCumulative);
    for (qint64 i = 0; i <= nBlockLength; ++i) {
        if (i == nPrimary) continue;
        const qint32 nByte = pBuffer[nStart + i];
        const qint64 nSlot = listIndex.at(nByte);
        if ((nSlot < 0) || (nSlot > 0x00ffffff)) return false;
        listTransform[(qint32)i] = ((quint32)nByte << 24) | (quint32)nSlot;
        listIndex[nByte] = nSlot + 1;
    }
    listTransform[(qint32)nPrimary] = 0;

    quint32 nPointer = 0;
    for (qint64 i = nBlockLength - 1; i >= 0; --i) {
        const qint32 nSlot = (qint32)(nPointer & 0xffffff);
        if ((nSlot < 0) || (nSlot > nBlockLength)) return false;
        nPointer = listTransform.at(nSlot);
        pBuffer[nStart + i] = (quint8)(((nPointer >> 24) + nAdd) & 0xff);
    }

    return true;
}

// ------------------------------------------------- x86 branch converter ----
// The reference implementation. Converts x86 CALL rel operands
// back from absolute to relative.  The last nWidth bytes are never converted.
// NOTE: no corpus sample sets the attribute bits that enable this, so it is
// transcribed from the decompilation but not empirically validated.
QByteArray impBranchConvert(const QByteArray &baData, qint32 nWidth, qint64 nTotal)
{
    QByteArray baResult = baData;
    quint8 *pData = (quint8 *)baResult.data();
    const qint64 nLimit = (qint64)baResult.size() - nWidth;
    qint64 i = 0;
    while (i < nLimit) {
        const quint8 nByte = pData[i];
        ++i;
        if (nByte != 0xe8) continue;
        if (nWidth == 2) {
            if ((i + 2) > baResult.size()) break;
            const quint16 nValue = qFromLittleEndian<quint16>(pData + i);
            qToLittleEndian<quint16>((quint16)(nValue - (quint16)(i & 0xffff)), pData + i);
            i += 2;
        } else {
            if ((i + 4) > baResult.size()) break;
            const qint32 nValue = (qint32)qFromLittleEndian<quint32>(pData + i);
            quint32 nResult = 0;
            if ((nValue < 0) && (-i <= (qint64)nValue)) nResult = (quint32)(nTotal - nValue - i - 1);
            else if ((quint32)nValue < (quint32)nTotal) nResult = (quint32)(nValue - i);
            else nResult = (quint32)nValue;
            qToLittleEndian<quint32>(nResult, pData + i);
            i += 4;
        }
    }

    return baResult;
}

// ----------------------------------------------------------- the stream ----
// The reference implementation - walks the chained compressed blocks of one solid stream.
class ImpStream {
public:
    ImpStream(const QByteArray &baData, qint64 nOffset)
        : m_baData(baData), m_nPosition(nOffset), m_nDictLength(0), m_nOutBase(0), m_nBlockLength(0), m_bFirst(true), m_bEof(false)
    {
    }

    bool read(qint64 nFrom, qint64 nLength, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
    {
        pbaResult->clear();
        if ((nFrom < 0) || (nLength < 0) || (nLength > XIMPDecoder::MAX_UNCOMPRESSED_SIZE)) return false;
        qint64 nPosition = nFrom;
        while (pbaResult->size() < nLength) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            if (nPosition < m_nOutBase) return false;  // the stream cannot rewind
            if (nPosition < (m_nOutBase + m_nBlockLength)) {
                const qint64 nLow = m_nDictLength + (nPosition - m_nOutBase);
                qint64 nAvailable = m_nBlockLength - (nPosition - m_nOutBase);
                const qint64 nWanted = nLength - pbaResult->size();
                if (nAvailable > nWanted) nAvailable = nWanted;
                if ((nLow < 0) || ((nLow + nAvailable) > m_baDictionary.size())) return false;
                pbaResult->append(m_baDictionary.constData() + nLow, (int)nAvailable);
                nPosition += nAvailable;
            } else {
                if (!nextBlock(pPdStruct)) break;
            }
        }

        return pbaResult->size() == nLength;
    }

private:
    bool grow(qint64 nSize)
    {
        if ((nSize < 0) || (nSize > 0x20000000)) return false;
        if (m_baDictionary.size() < nSize) m_baDictionary.append(QByteArray((int)(nSize - m_baDictionary.size()), (char)0));

        return m_baDictionary.size() >= nSize;
    }

    bool nextBlock(XBinary::PDSTRUCT *pPdStruct)
    {
        if (m_bEof) return false;
        ImpBitReader reader((const quint8 *)m_baData.constData(), m_baData.size(), m_nPosition);
        const qint32 nMethod = (qint32)reader.get(4);
        if (nMethod > 3) return false;
        qint32 nLast = (qint32)reader.get(1);
        const qint64 nUnpacked = (qint64)reader.get(20);
        const qint64 nPacked = (qint64)reader.get(20);
        if (reader.isError()) return false;

        m_nDictLength += m_nBlockLength;
        m_nOutBase += m_nBlockLength;
        m_nBlockLength = nUnpacked;

        if ((nMethod == 1) || (nMethod == 3)) {
            if (m_bFirst) {
                reader.drop(5);
            } else {
                const qint32 nRow = (qint32)reader.get(2);
                const qint32 nColumn = (qint32)reader.get(3);
                if (reader.isError() || (nRow > 2)) return false;
                const qint64 nWindow = g_arrWindows[nRow][nColumn];
                impDeltaUnapply(m_listRecords, (quint8 *)m_baDictionary.data(), m_baDictionary.size(), m_nDictLength, nWindow);
                if (nWindow < m_nDictLength) {
                    memmove((quint8 *)m_baDictionary.data(), (const quint8 *)m_baDictionary.constData() + (m_nDictLength - nWindow), (size_t)nWindow);
                    m_nDictLength = nWindow;
                }
            }
            if (!grow(m_nDictLength + nUnpacked + IMP_SLACK)) return false;
            qint64 nProduced = 0;
            if (!impLzDecode(&reader, (quint8 *)m_baDictionary.data(), m_baDictionary.size(), m_nDictLength, nUnpacked, &m_listRecords, true, &nProduced,
                             pPdStruct)) {
                return false;
            }
            if (nProduced < nUnpacked) {
                m_nBlockLength = nProduced;
                nLast = 1;
            }
        } else {
            if (!grow(nUnpacked + IMP_SLACK)) return false;
            if (!nLast) {
                qint64 nKeep = IMP_MAX_WINDOW - nUnpacked;
                if (nKeep < 0) nKeep = 0;
                else if ((((qint64)m_baDictionary.size() - IMP_SLACK) - nUnpacked) < nKeep) nKeep = ((qint64)m_baDictionary.size() - IMP_SLACK) - nUnpacked;
                if (nKeep < 0) nKeep = 0;
                impDeltaUnapply(m_listRecords, (quint8 *)m_baDictionary.data(), m_baDictionary.size(), m_nDictLength, nKeep);
                if (nKeep < m_nDictLength) {
                    memmove((quint8 *)m_baDictionary.data(), (const quint8 *)m_baDictionary.constData() + (m_nDictLength - nKeep), (size_t)nKeep);
                    m_nDictLength = nKeep;
                }
            } else {
                m_nDictLength = 0;
            }
            if (!grow(m_nDictLength + nUnpacked + IMP_SLACK)) return false;
            m_listRecords.clear();
            if (nMethod == 2) {
                if (!impBwtDecode(&reader, (quint8 *)m_baDictionary.data(), m_baDictionary.size(), m_nDictLength, nUnpacked, pPdStruct)) return false;
            } else {
                const qint64 nSource = reader.bytePosition();
                if ((nSource < 0) || (nSource > m_baData.size())) return false;
                const qint64 nAvailable = qMin(nUnpacked, (qint64)m_baData.size() - nSource);
                if (nAvailable > 0) {
                    memcpy((quint8 *)m_baDictionary.data() + m_nDictLength, (const quint8 *)m_baData.constData() + nSource, (size_t)nAvailable);
                }
                if (nAvailable < nUnpacked) memset((quint8 *)m_baDictionary.data() + m_nDictLength + nAvailable, 0, (size_t)(nUnpacked - nAvailable));
            }
        }

        m_bEof = (nLast != 0);
        m_bFirst = false;
        m_nPosition += nPacked;
        if ((nPacked <= 0) && !m_bEof) return false;  // a zero-length block would spin

        return true;
    }

    QByteArray m_baData;
    QByteArray m_baDictionary;
    qint64 m_nPosition;
    qint64 m_nDictLength;
    qint64 m_nOutBase;
    qint64 m_nBlockLength;
    bool m_bFirst;
    bool m_bEof;
    ImpFilterState m_listRecords;
};

}  // namespace

quint32 XIMPDecoder::crc32(const QByteArray &baData)
{
    quint32 nCrc = 0xffffffffU;
    const quint8 *pData = (const quint8 *)baData.constData();
    for (qint32 i = 0; i < baData.size(); ++i) nCrc = g_crcTable.at((qint32)((nCrc ^ pData[i]) & 0xff)) ^ (nCrc >> 8);

    return nCrc ^ 0xffffffffU;
}

bool XIMPDecoder::decodeDirectory(const QByteArray &baDirectory, qint32 nRecords, QList<QByteArray> *plistChunks, XBinary::PDSTRUCT *pPdStruct)
{
    if (!plistChunks || (nRecords < 0)) return false;
    plistChunks->clear();

    qint64 nOffset = 0;
    qint64 nTotal = 0;
    const qint64 nSize = baDirectory.size();
    while (true) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (((nSize - nOffset) < 6) || (baDirectory.mid((int)nOffset, 6) != QByteArray("IMPDE\0", 6))) break;

        ImpBitReader reader((const quint8 *)baDirectory.constData(), nSize, nOffset + 6);
        const qint32 nMethod = (qint32)reader.get(5);
        if ((nMethod & 0x0f) >= 2) return false;
        const qint64 nUnpacked = (qint64)reader.get(20);
        if (nUnpacked > 0x2000) return false;
        const qint64 nPacked = (qint64)reader.get(20);
        if (reader.isError()) return false;

        QByteArray baChunk((int)nUnpacked, (char)0);
        if ((nMethod & 0x0f) == 1) {
            reader.drop(5);
            ImpFilterState listRecords;
            qint64 nProduced = 0;
            if (!impLzDecode(&reader, (quint8 *)baChunk.data(), baChunk.size(), 0, nUnpacked, &listRecords, false, &nProduced, pPdStruct)) return false;
        } else {
            const qint64 nSource = reader.bytePosition();
            if ((nSource < 0) || (nSource > nSize)) return false;
            const qint64 nAvailable = qMin(nUnpacked, nSize - nSource);
            if (nAvailable > 0) memcpy((quint8 *)baChunk.data(), (const quint8 *)baDirectory.constData() + nSource, (size_t)nAvailable);
        }
        plistChunks->append(baChunk);
        nTotal += baChunk.size();

        if (nPacked <= 0) break;
        nOffset += nPacked + 6;
        if ((nTotal >= ((qint64)nRecords * DIRECTORY_RECORD_SIZE)) || (nOffset >= nSize)) break;
    }

    return !plistChunks->isEmpty();
}

QByteArray XIMPDecoder::packProperties(qint64 nStreamOffset, qint64 nSize, quint8 nAttributes)
{
    QByteArray baResult(21, (char)0);
    uchar *pData = (uchar *)baResult.data();
    memcpy(pData, "IMPS", 4);
    qToLittleEndian<quint64>((quint64)nStreamOffset, pData + 4);
    qToLittleEndian<quint64>((quint64)nSize, pData + 12);
    pData[20] = nAttributes;

    return baResult;
}

bool XIMPDecoder::unpackProperties(const QByteArray &baProperties, qint64 *pnStreamOffset, qint64 *pnSize, quint8 *pnAttributes)
{
    if (!pnStreamOffset || !pnSize || !pnAttributes) return false;
    if (baProperties.size() != 21) return false;
    if (baProperties.left(4) != QByteArray("IMPS", 4)) return false;
    const uchar *pData = (const uchar *)baProperties.constData();
    *pnStreamOffset = (qint64)qFromLittleEndian<quint64>(pData + 4);
    *pnSize = (qint64)qFromLittleEndian<quint64>(pData + 12);
    *pnAttributes = pData[20];

    return (*pnStreamOffset >= 0) && (*pnSize >= 0) && (*pnSize <= MAX_UNCOMPRESSED_SIZE);
}

bool XIMPDecoder::decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                         XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();

    qint64 nStreamOffset = 0;
    qint64 nSize = 0;
    quint8 nAttributes = 0;
    if (!unpackProperties(baProperties, &nStreamOffset, &nSize, &nAttributes)) return false;
    if ((nUncompressedSize >= 0) && (nSize != nUncompressedSize)) return false;
    if (baPacked.size() < 6) return false;

    // the block chain starts just past the stream's own six-byte signature
    ImpStream stream(baPacked, 6);

    QByteArray baHead;
    if (!stream.read(nStreamOffset, 11, &baHead, pPdStruct)) return false;
    const qint64 nSkip = qFromLittleEndian<quint16>((const uchar *)baHead.constData() + 4);

    QByteArray baPayload;
    if (!stream.read(nStreamOffset + 11 + nSkip, nSize, &baPayload, pPdStruct)) return false;

    if (nAttributes & 0x06) baPayload = impBranchConvert(baPayload, (nAttributes & 4) ? 4 : 2, nSize);
    if (baPayload.size() != nSize) return false;

    *pbaResult = baPayload;

    return true;
}
