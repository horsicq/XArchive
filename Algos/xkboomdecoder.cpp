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
#include "xkboomdecoder.h"

namespace {
const qint32 N_NIL = 0x1fff;
const qint32 N_CAPACITY = 0x2000;
const qint32 N_LAST_SLOT = 0x1ffe;
const qint32 N_FIRST_SLOT = 0x100;
const qint32 N_SCRATCH_TOP = 100;   // phrases are written down from here
const qint32 N_MAX_PHRASE = 100;
const qint32 N_MAX_EXTEND = 4;      // trie bytes added per token

// 13-bit codes off the top of a 32-bit accumulator, refilled a
// little-endian 16-bit word at a time.
class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nAccumulator(0), m_nCount(0)
    {
    }

    bool next(qint32 *pnCode)
    {
        if (m_nCount < 13) {
            if ((m_nPosition + 2) > m_nSize) return false;
            const quint32 nWord = (quint32)m_pData[m_nPosition] | ((quint32)m_pData[m_nPosition + 1] << 8);
            m_nPosition += 2;
            m_nAccumulator = (m_nAccumulator + (nWord << (16 - m_nCount))) & 0xffffffffU;
            m_nCount += 16;
        }
        *pnCode = (qint32)((m_nAccumulator >> 19) & 0x1fffU);
        m_nAccumulator = (m_nAccumulator << 13) & 0xffffffffU;
        m_nCount -= 13;
        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nAccumulator;
    qint32 m_nCount;
};
}  // namespace

bool XKBoomDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    QVector<quint16> vecSymbol(N_CAPACITY, (quint16)N_NIL);
    QVector<quint16> vecChild(N_CAPACITY, (quint16)N_NIL);
    QVector<quint16> vecParent(N_CAPACITY, (quint16)N_NIL);
    QVector<quint16> vecNext(N_CAPACITY, (quint16)N_NIL);
    QVector<quint16> vecPrevious(N_CAPACITY, (quint16)N_NIL);
    for (qint32 i = 0; i < 256; ++i) vecSymbol[i] = (quint16)i;

    qint32 nFree = N_FIRST_SLOT;
    qint32 nCursor = N_FIRST_SLOT;
    qint32 nPreviousCode = N_NIL;
    qint32 nPreviousLength = 0;

    BitReader reader((const quint8 *)baPacked.constData(), baPacked.size());
    QByteArray baScratch(256, (char)0);
    quint8 *pScratch = (quint8 *)baScratch.data();

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    while (baOut.size() < nUncompressedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        qint32 nCode = 0;
        if (!reader.next(&nCode)) break;

        qint32 nLength = 0;
        qint32 nNode = nCode;
        qint32 nGuard = 0x4000;
        while (nNode != N_NIL) {
            if ((nNode >= N_CAPACITY) || (nLength > N_MAX_PHRASE) || (--nGuard == 0)) return false;
            pScratch[N_SCRATCH_TOP - nLength] = (quint8)vecSymbol[nNode];
            ++nLength;
            nNode = vecParent[nNode];
        }
        const qint32 nStart = (N_SCRATCH_TOP + 1) - nLength;
        baOut.append((const char *)(pScratch + nStart), nLength);

        if (nPreviousCode != N_NIL) {
            bool bMustAllocate = false;
            qint32 nCurrent = nPreviousCode;
            for (qint32 i = 0; (i < nLength) && (i < N_MAX_EXTEND) && ((nPreviousLength + i) < N_MAX_PHRASE); ++i) {
                if ((nStart + i) > 0xff) return false;
                const quint8 nByte = pScratch[nStart + i];

                if (!bMustAllocate) {
                    qint32 nMatch = vecChild[nCurrent];
                    while ((nMatch != N_NIL) && (vecSymbol[nMatch] != nByte)) nMatch = vecNext[nMatch];
                    if (nMatch != N_NIL) {
                        nCurrent = nMatch;
                        continue;
                    }
                    bMustAllocate = true;
                }

                qint32 nNew = 0;
                if (nFree > N_LAST_SLOT) {
                    // The table is full: sweep for a childless node to reuse,
                    // skipping the node currently being extended.
                    qint32 nSweep = 0;
                    while (true) {
                        if (nCursor > N_LAST_SLOT) nCursor = N_FIRST_SLOT;
                        if ((nCursor != nCurrent) && (vecChild[nCursor] == N_NIL)) break;
                        ++nCursor;
                        if (++nSweep > N_CAPACITY) return false;
                    }
                    nNew = nCursor;
                    nCursor = nNew + 1;
                    const quint16 nAfter = vecNext[nNew];
                    if (vecPrevious[nNew] == N_NIL) {
                        if (vecParent[nNew] != N_NIL) vecChild[vecParent[nNew]] = nAfter;
                    } else {
                        vecNext[vecPrevious[nNew]] = nAfter;
                    }
                    if (nAfter != N_NIL) vecPrevious[nAfter] = vecPrevious[nNew];
                } else {
                    nNew = nFree;
                    ++nFree;
                }

                vecSymbol[nNew] = nByte;
                vecParent[nNew] = (quint16)nCurrent;
                vecChild[nNew] = (quint16)N_NIL;
                vecPrevious[nNew] = (quint16)N_NIL;
                const quint16 nHead = vecChild[nCurrent];
                vecNext[nNew] = nHead;
                if (nHead != N_NIL) vecPrevious[nHead] = (quint16)nNew;
                vecChild[nCurrent] = (quint16)nNew;

                nCurrent = nNew;
            }
        }

        nPreviousCode = nCode;
        nPreviousLength = nLength;
    }

    // The last phrase may run past the declared size; the reference lets that
    // extra byte reach the file, this drops it.
    if (baOut.size() > nUncompressedSize) baOut.truncate((qint32)nUncompressedSize);
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
