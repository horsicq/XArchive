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
#include "xnewwavelzwdecoder.h"

namespace {

const qint32 N_CLEAR_CODE = 0x100;
const qint32 N_END_CODE = 0x101;
const qint32 N_FIRST_SLOT = 0x102;
const qint32 N_MAX_BITS = 12;
const qint32 N_CAPACITY = 1 << N_MAX_BITS;  // 0x1000
const qint32 N_CANCEL_MASK = 0xffff;        // cancellation is checked this often

// MSB-first code reader.  A code that STARTS past the last byte ends the
// stream; one that merely runs past it is still readable, because the encoder
// pads the tail with zero bits.
class CodeReader {
public:
    CodeReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nBits(nSize * 8), m_nBitPosition(0), m_nWidth(9)
    {
    }

    qint32 width() const
    {
        return m_nWidth;
    }

    void setWidth(qint32 nWidth)
    {
        m_nWidth = nWidth;
    }

    bool next(qint32 *pnCode)
    {
        if (m_nBitPosition >= m_nBits) return false;

        quint32 nValue = 0;

        for (qint32 i = 0; i < m_nWidth; ++i) {
            const qint64 nBit = m_nBitPosition + i;
            const quint32 nSet = (nBit >= m_nBits) ? 0 : ((m_pData[nBit >> 3] >> (7 - (nBit & 7))) & 1);
            nValue = (nValue << 1) | nSet;
        }

        m_nBitPosition += m_nWidth;
        *pnCode = (qint32)nValue;

        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nBits;
    qint64 m_nBitPosition;
    qint32 m_nWidth;
};

}  // namespace

bool XNewWaveLZWDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;
    if (baPacked.isEmpty()) return (nUncompressedSize == 0);

    QVector<qint32> vecPrefix(N_CAPACITY, 0);
    QByteArray baSuffix(N_CAPACITY, (char)0);
    quint8 *pSuffix = (quint8 *)baSuffix.data();
    for (qint32 i = 0; i < 256; ++i) pSuffix[i] = (quint8)i;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    CodeReader reader((const quint8 *)baPacked.constData(), baPacked.size());

    QByteArray baStack;
    qint32 nFree = N_FIRST_SLOT;
    qint32 nPrevious = -1;
    quint8 nPreviousFirst = 0;
    qint32 nCounter = 0;

    while (baOut.size() < nUncompressedSize) {
        ++nCounter;
        if ((nCounter & N_CANCEL_MASK) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        qint32 nCode = 0;
        if (!reader.next(&nCode)) break;

        if (nCode == N_END_CODE) break;

        if (nCode == N_CLEAR_CODE) {
            nFree = N_FIRST_SLOT;
            reader.setWidth(9);
            nPrevious = -1;
            continue;
        }

        if (nPrevious < 0) {
            // The first code of a group is always a literal: the table is empty.
            if (nCode > 0xff) return false;
            baOut.append((char)(quint8)nCode);
            nPrevious = nCode;
            nPreviousFirst = (quint8)nCode;
        } else {
            baStack.resize(0);
            qint32 nCurrent = nCode;

            if (nCode >= nFree) {
                // KwKwK: only the one slot about to be created may be named.
                if (nCode > nFree) return false;
                baStack.append((char)nPreviousFirst);
                nCurrent = nPrevious;
            }

            qint32 nGuard = 0;
            while (nCurrent > 0xff) {
                if ((nCurrent >= N_CAPACITY) || (++nGuard > N_CAPACITY)) return false;
                baStack.append((char)pSuffix[nCurrent]);
                nCurrent = vecPrefix[nCurrent];
            }
            baStack.append((char)(quint8)nCurrent);

            for (qint32 i = baStack.size() - 1; i >= 0; --i) baOut.append(baStack.at(i));

            if (nFree < N_CAPACITY) {
                vecPrefix[nFree] = nPrevious;
                pSuffix[nFree] = (quint8)nCurrent;
                ++nFree;
            }

            nPrevious = nCode;
            nPreviousFirst = (quint8)nCurrent;
        }

        // Early width change - and, at the top width, the self-restart that
        // takes the place of a thirteenth bit.  The explicit clear the encoder
        // writes next is then read at nine bits and does nothing.
        if ((nFree + 1) >= (1 << reader.width())) {
            if (reader.width() < N_MAX_BITS) {
                reader.setWidth(reader.width() + 1);
            } else {
                nFree = N_FIRST_SLOT;
                reader.setWidth(9);
                nPrevious = -1;
            }
        }
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
