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
#include "xwpkdecoder.h"

namespace {

const qint32 N_WINDOW = 0x1000;
const qint32 N_MAX_SYMBOLS = 0x13B;
const qint32 N_SORT_STACK_A = 0x400;
const qint32 N_SORT_STACK_B = 0x3E4;

// LHA "-lh1-" position tables.  Kept local so this codec links on its own.
const quint8 g_dCode[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e, 0x0e, 0x0f, 0x0f, 0x0f, 0x0f,
    0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13,
    0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
    0x18, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1b, 0x1c, 0x1c, 0x1d, 0x1d, 0x1e, 0x1e, 0x1f, 0x1f,
    0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23, 0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27,
    0x28, 0x28, 0x29, 0x29, 0x2a, 0x2a, 0x2b, 0x2b, 0x2c, 0x2c, 0x2d, 0x2d, 0x2e, 0x2e, 0x2f, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f};

const quint8 g_dLen[16] = {3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8};

struct ENTRY {
    qint32 nSymbol;
    qint32 nCode;
    qint32 nLength;
};

// MSB-first bit reader.  Past the end of the buffer it shifts in zero bytes but
// still advances the read pointer, because consumed() has to report the byte
// count the original reader would have taken - that is what the directory's
// CRC-32 is computed over.
class Bits {
public:
    Bits(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nAcc(0), m_nBits(0)
    {
    }

    qint32 bit()
    {
        if (m_nBits < 1) {
            if (m_nPosition >= m_nSize) return -1;
            fill(1);
        }
        --m_nBits;
        const qint32 nResult = (qint32)((m_nAcc >> m_nBits) & 1);
        m_nAcc &= ((quint32)1 << m_nBits) - 1;

        return nResult;
    }

    qint32 bits(qint32 nCount)
    {
        if (nCount <= 0) return 0;
        if (m_nBits < nCount) {
            if (m_nPosition >= m_nSize) return -1;
            fill(nCount);
        }
        m_nBits -= nCount;
        const qint32 nResult = (qint32)((m_nAcc >> m_nBits) & (((quint32)1 << nCount) - 1));
        m_nAcc &= ((quint32)1 << m_nBits) - 1;

        return nResult;
    }

    quint32 peek16()
    {
        fill(16);

        return (m_nAcc >> (m_nBits - 16)) & 0xFFFF;
    }

    bool drop(qint32 nCount)
    {
        if ((nCount <= 0) || (nCount > m_nBits)) return false;
        m_nBits -= nCount;
        m_nAcc &= ((quint32)1 << m_nBits) - 1;

        return true;
    }

    qint64 consumed() const
    {
        return m_nPosition - (m_nBits >> 3);
    }

private:
    void fill(qint32 nNeed)
    {
        while (m_nBits < nNeed) {
            if (m_nPosition >= m_nSize) {
                m_nAcc = m_nAcc << 8;
            } else {
                m_nAcc = (m_nAcc << 8) | m_pData[m_nPosition];
            }
            ++m_nPosition;
            m_nBits += 8;
        }
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nAcc;
    qint32 m_nBits;
};

qint32 compareEntries(const ENTRY *pTable, qint32 nLeft, qint32 nRight)
{
    return pTable[nLeft].nLength - pTable[nRight].nLength;
}

void swapEntries(ENTRY *pTable, qint32 nLeft, qint32 nRight)
{
    const ENTRY entry = pTable[nLeft];
    pTable[nLeft] = pTable[nRight];
    pTable[nRight] = entry;
}

// The reference implementation: a simple quicksort over an explicit stack. Instruction-for-
// instruction port - the point of this function is its tie-break order inside a
// run of equal lengths, so nothing here may be "cleaned up".
void sortA(ENTRY *pTable, qint32 nCount)
{
    qint32 stackBase[N_SORT_STACK_A];
    qint32 stackCount[N_SORT_STACK_A];
    qint32 nSP = 0;
    qint32 nBase = 0;
    qint32 nCnt = nCount;

    while (true) {
        bool bAgain = true;

        while (bAgain) {
            bAgain = false;

            if (nCnt > 1) {
                if (nCnt == 2) {
                    if (compareEntries(pTable, nBase, nBase + 1) > 0) swapEntries(pTable, nBase, nBase + 1);
                } else {
                    swapEntries(pTable, nBase, nBase + (nCnt >> 1));

                    qint32 nIndex = 1;
                    qint32 nLess = 0;
                    qint32 nEqualEdge = 0;

                    while (nIndex < nCnt) {
                        const qint32 nResult = compareEntries(pTable, nBase + nIndex, nBase);
                        if (nResult < 1) {
                            ++nLess;
                            if (nIndex != nLess) swapEntries(pTable, nBase + nIndex, nBase + nLess);
                        }
                        if (nResult != 0) nEqualEdge = nLess;
                        ++nIndex;
                    }

                    if ((nLess != (nCnt - 1)) || (nEqualEdge != 0)) {
                        if (nLess != 0) swapEntries(pTable, nBase + nLess, nBase);
                        nCnt = nCnt - nLess - 1;
                        nIndex = (nLess == nEqualEdge) ? nLess : (nEqualEdge + 1);

                        if (nSP == N_SORT_STACK_A) return;

                        if (nIndex < nCnt) {
                            stackBase[nSP] = nBase + nLess + 1;
                            stackCount[nSP] = nCnt;
                            nCnt = nIndex;
                        } else {
                            stackBase[nSP] = nBase;
                            stackCount[nSP] = nIndex;
                            nBase = nBase + nLess + 1;
                        }

                        ++nSP;
                        bAgain = true;
                    }
                }
            }
        }

        if (nSP == 0) return;
        --nSP;
        nBase = stackBase[nSP];
        nCnt = stackCount[nSP];
    }
}

qint32 med3(ENTRY *pTable, qint32 nA, qint32 nB, qint32 nC)
{
    qint32 nResult = compareEntries(pTable, nA, nB);

    if (nResult < 1) {
        nResult = compareEntries(pTable, nA, nC);
        if (nResult < 0) {
            nA = nB;
            if (compareEntries(pTable, nB, nC) > 0) nA = nC;
        }
    } else {
        nResult = compareEntries(pTable, nA, nC);
        if (nResult > 0) {
            nA = nC;
            if (compareEntries(pTable, nB, nC) > 0) nA = nB;
        }
    }

    return nA;
}

// The reference implementation: the BSD qsort shape - shell sort below 16 elements, median of
// three (or of nine) above it.  Same warning as sortA: the tie-break order is
// the format.
void sortB(ENTRY *pTable, qint32 nCount)
{
    qint32 stackBase[N_SORT_STACK_B];
    qint32 stackCount[N_SORT_STACK_B];
    qint32 nSP = 0;
    qint32 nBase = 0;
    qint32 nCnt = nCount;

    while (true) {
        while (nCnt < 2) {
            if (nSP == 0) return;
            --nSP;
            nBase = stackBase[nSP];
            nCnt = stackCount[nSP];
        }

        if (nCnt < 0x10) {
            qint32 nGap = 3;

            while (nGap > 0) {
                qint32 i = nBase;

                while (true) {
                    i += nGap;
                    qint32 j = i;
                    if (i >= (nBase + nCnt)) break;
                    while ((j > nBase) && (compareEntries(pTable, j - nGap, j) > 0)) {
                        swapEntries(pTable, j, j - nGap);
                        j -= nGap;
                    }
                }

                nGap -= 2;
            }

            if (nSP == 0) return;
            --nSP;
            nBase = stackBase[nSP];
            nCnt = stackCount[nSP];
            continue;
        }

        qint32 nMid = nBase + (nCnt >> 1);

        if (nCnt > 0x1D) {
            qint32 nHi = nBase + nCnt - 1;
            qint32 nLo = nBase;

            if (nCnt > 0x2A) {
                const qint32 nStep = nCnt >> 3;
                nLo = med3(pTable, nBase, nBase + nStep, nBase + nStep * 2);
                nMid = med3(pTable, nMid - nStep, nMid, nMid + nStep);
                nHi = med3(pTable, nHi - nStep * 2, nHi - nStep, nHi);
            }

            nMid = med3(pTable, nLo, nMid, nHi);
        }

        qint32 pc = nBase + nCnt - 1;
        qint32 pa = nBase;
        qint32 pb = nBase;
        qint32 pd = pc;

        while (true) {
            while (pb <= pd) {
                const qint32 nResult = compareEntries(pTable, pb, nMid);
                if (nResult > 0) break;
                if (nResult == 0) {
                    qint32 nKeep = pb;
                    if (nMid != pa) {
                        nKeep = nMid;
                        if (nMid == pb) nKeep = pa;
                    }
                    swapEntries(pTable, pa, pb);
                    ++pa;
                    nMid = nKeep;
                }
                ++pb;
            }

            while (pb <= pd) {
                const qint32 nResult = compareEntries(pTable, pd, nMid);
                if (nResult < 0) break;
                if (nResult == 0) {
                    qint32 nKeep = pc;
                    if (nMid != pd) {
                        nKeep = nMid;
                        if (nMid == pc) nKeep = pd;
                    }
                    swapEntries(pTable, pd, pc);
                    --pc;
                    nMid = nKeep;
                }
                --pd;
            }

            if (pd < pb) break;

            qint32 nKeep = pd;
            if (nMid != pb) {
                nKeep = nMid;
                if (nMid == pd) nKeep = pb;
            }
            nMid = nKeep;
            swapEntries(pTable, pb, pd);
            ++pb;
            --pd;
        }

        const qint32 nEnd = nBase + nCnt;
        qint32 nSpan = qMin(pa - nBase, pb - pa);
        for (qint32 i = 0; i < nSpan; ++i) swapEntries(pTable, nBase + i, (pb - nSpan) + i);
        nSpan = qMin(pc - pd, (nEnd - pc) - 1);
        for (qint32 i = 0; i < nSpan; ++i) swapEntries(pTable, pb + i, (nEnd - nSpan) + i);

        const qint32 nLeft = pb - pa;
        const qint32 nRight = pc - pd;

        if (nSP == N_SORT_STACK_B) return;

        if (nRight < nLeft) {
            if (nLeft < 2) {
                if (nSP == 0) return;
                --nSP;
                nBase = stackBase[nSP];
                nCnt = stackCount[nSP];
                continue;
            }
            stackBase[nSP] = nBase;
            stackCount[nSP] = nLeft;
            nBase = nEnd - nRight;
            nCnt = nRight;
        } else {
            stackBase[nSP] = nEnd - nRight;
            stackCount[nSP] = nRight;
            nCnt = nLeft;
        }

        ++nSP;
    }
}

// Insertion sort - stable, i.e. it keeps the symbols in a run of equal lengths
// in the order they were transmitted.  Kept only so a caller can probe all three
// orders; no archive of the reference corpus wants it.
void sortStable(ENTRY *pTable, qint32 nCount)
{
    for (qint32 i = 1; i < nCount; ++i) {
        const ENTRY entry = pTable[i];
        qint32 j = i - 1;
        while ((j >= 0) && (pTable[j].nLength > entry.nLength)) {
            pTable[j + 1] = pTable[j];
            --j;
        }
        pTable[j + 1] = entry;
    }
}

// Shared by both methods: the distance is a byte through the -lh1- tables plus
// d_len[i >> 4] - 2 raw bits, and the source index carries the classic LZSS -1.
qint32 decodeDistance(Bits *pBits, qint32 nPosition)
{
    qint32 i = pBits->bits(8);
    if (i < 0) return -1;

    const qint32 nPrefix = g_dCode[i & 0xFF];
    const qint32 nExtra = (qint32)g_dLen[(i & 0xFF) >> 4] - 2;

    if (nExtra > 0) {
        const qint32 nTail = pBits->bits(nExtra);
        if (nTail < 0) return -1;
        i = ((i << nExtra) + nTail) & 0xFF;
    }

    return (nPosition - (nPrefix * 0x40 + (i & 0x3F)) - 1) & (N_WINDOW - 1);
}

// The code-length table: one count byte, then count + 1 control bytes that
// either assign a length to a run of symbols or skip a run of unused ones.
bool buildTable(Bits *pBits, QVector<ENTRY> *pvecTable)
{
    const qint32 nControls = pBits->bits(8);
    if (nControls < 0) return false;

    qint32 nSymbol = 0;

    for (qint32 i = 0; i <= nControls; ++i) {
        const qint32 nByte = pBits->bits(8);
        if (nByte < 0) return false;

        if ((nByte & 0x80) == 0) {
            const qint32 nLength = (nByte & 0x0F) + 1;
            const qint32 nRun = (nByte >> 4) + 1;

            for (qint32 n = 0; n < nRun; ++n) {
                if (pvecTable->count() == N_MAX_SYMBOLS) return false;
                ENTRY entry;
                entry.nSymbol = nSymbol;
                entry.nCode = 0;
                entry.nLength = nLength;
                pvecTable->append(entry);
                ++nSymbol;
            }
        } else {
            nSymbol += (nByte & 0x7F) + 1;
        }
    }

    return nSymbol < N_MAX_SYMBOLS;
}

}  // namespace

bool XWPKDecoder::decodeMethodA(const QByteArray &baPacked, SORTER sorter, qint64 nUncompressedSize, QByteArray *pbaResult, qint64 *pnConsumed,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (pnConsumed) *pnConsumed = 0;
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    Bits bits((const quint8 *)baPacked.constData(), baPacked.size());

    QVector<ENTRY> vecTable;
    if (!buildTable(&bits, &vecTable)) return false;

    const qint32 nCount = vecTable.count();
    ENTRY *pTable = vecTable.data();

    if (sorter == SORTER_A) sortA(pTable, nCount);
    else if (sorter == SORTER_B) sortB(pTable, nCount);
    else sortStable(pTable, nCount);

    // Canonical codes are assigned from the LAST entry backwards, in a 16-bit
    // left-justified space.
    qint32 nCode = 0;
    for (qint32 i = nCount - 1; i >= 0; --i) {
        pTable[i].nCode = nCode & 0xFFFF;
        nCode = (nCode + (1 << (16 - pTable[i].nLength))) & 0xFFFF;
    }

    // 16-bit direct lookup, filled shortest code first so that a short code wins
    // over anything a longer one would have claimed - which is what the
    // original's ascending linear scan does.
    QVector<qint32> vecSymbol(0x10000, -1);
    QByteArray baWidth(0x10000, (char)0);
    quint8 *pWidth = (quint8 *)baWidth.data();

    for (qint32 nWidth = 1; nWidth <= 16; ++nWidth) {
        const qint32 nSpan = 1 << (16 - nWidth);

        for (qint32 i = 0; i < nCount; ++i) {
            if (pTable[i].nLength != nWidth) continue;
            const qint32 nStart = pTable[i].nCode;
            if ((nStart < 0) || (nStart >= 0x10000)) return false;
            if (vecSymbol.at(nStart) >= 0) continue;
            if ((nStart + nSpan) > 0x10000) return false;

            for (qint32 n = 0; n < nSpan; ++n) {
                vecSymbol[nStart + n] = pTable[i].nSymbol;
                pWidth[nStart + n] = (quint8)nWidth;
            }
        }
    }

    QByteArray baWindow(N_WINDOW, (char)0x20);
    quint8 *pWindow = (quint8 *)baWindow.data();

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize + 0x40);

    qint32 nPosition = 0;
    qint64 nGuard = 0;

    while (baOut.size() < nUncompressedSize) {
        ++nGuard;
        if ((nGuard & 0xFFF) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        const quint32 nPeek = bits.peek16();
        const qint32 nSymbol = vecSymbol.at((qint32)nPeek);
        if (nSymbol < 0) return false;
        if (!bits.drop((qint32)pWidth[nPeek])) return false;

        if (nSymbol < 0x100) {
            baOut.append((char)(quint8)nSymbol);
            pWindow[nPosition] = (quint8)nSymbol;
            nPosition = (nPosition + 1) & (N_WINDOW - 1);
        } else {
            qint32 nSource = decodeDistance(&bits, nPosition);
            if (nSource < 0) return false;

            // A match is symbol - 0xFD bytes, so 3 .. 0x3D.  The original does
            // not stop mid-match at the stored size; the overshoot is capped by
            // the longest match, and the final size check below rejects it.
            const qint32 nLength = nSymbol - 0xFD;

            for (qint32 n = 0; n < nLength; ++n) {
                if (baOut.size() >= (nUncompressedSize + 0x40)) return false;
                const quint8 nByte = pWindow[nSource];
                baOut.append((char)nByte);
                pWindow[nPosition] = nByte;
                nPosition = (nPosition + 1) & (N_WINDOW - 1);
                nSource = (nSource + 1) & (N_WINDOW - 1);
            }
        }
    }

    if (pnConsumed) *pnConsumed = bits.consumed();
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XWPKDecoder::decodeMethodB(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, qint64 *pnConsumed, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (pnConsumed) *pnConsumed = 0;
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    Bits bits((const quint8 *)baPacked.constData(), baPacked.size());

    QByteArray baWindow(N_WINDOW, (char)0x20);
    quint8 *pWindow = (quint8 *)baWindow.data();

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize + 0x40);

    qint32 nPosition = 0;
    qint64 nGuard = 0;

    while (baOut.size() < nUncompressedSize) {
        ++nGuard;
        if ((nGuard & 0xFFF) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        const qint32 nFlag = bits.bit();
        if (nFlag < 0) return false;

        if (nFlag == 0) {
            const qint32 nByte = bits.bits(8);
            if (nByte < 0) return false;
            pWindow[nPosition] = (quint8)nByte;
            baOut.append((char)(quint8)nByte);
            nPosition = (nPosition + 1) & (N_WINDOW - 1);
        } else {
            // Six raw bits of length, then the shared distance encoding.  A zero
            // length copies nothing; the bit reader running dry is what ends
            // such a stream.
            const qint32 nLength = bits.bits(6);
            if (nLength < 0) return false;

            qint32 nSource = decodeDistance(&bits, nPosition);
            if (nSource < 0) return false;

            for (qint32 n = 0; n < nLength; ++n) {
                if (baOut.size() >= (nUncompressedSize + 0x40)) return false;
                const quint8 nByte = pWindow[nSource];
                pWindow[nPosition] = nByte;
                baOut.append((char)nByte);
                nSource = (nSource + 1) & (N_WINDOW - 1);
                nPosition = (nPosition + 1) & (N_WINDOW - 1);
            }
        }
    }

    if (pnConsumed) *pnConsumed = bits.consumed();
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
