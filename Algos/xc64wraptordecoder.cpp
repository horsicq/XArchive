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
#include "xc64wraptordecoder.h"

#include <limits>

namespace {
const qint32 C64_WINDOW_SIZE = 0x1000;
const qint32 C64_WINDOW_MASK = 0x0fff;
const qint32 C64_START_WIDTH = 8;
const qint32 C64_MAX_WIDTH = 0x0d;
const qint32 C64_LENGTH_BITS = 5;

// MSB-first, multi-bit values accumulated most significant bit first.
class C64BitReader {
public:
    C64BitReader(const quint8 *pData, qint64 nSize, qint64 nPosition) : m_pData(pData), m_nSize(nSize), m_nPosition(nPosition), m_nAccumulator(0), m_nBits(0)
    {
    }

    qint64 pos() const
    {
        return m_nPosition;
    }

    // -1 on exhaustion
    qint32 bit()
    {
        if (m_nBits == 0) {
            if (m_nPosition >= m_nSize) return -1;
            m_nAccumulator = m_pData[m_nPosition];
            ++m_nPosition;
            m_nBits = 8;
        }
        const qint32 nValue = (m_nAccumulator >> 7) & 1;
        m_nAccumulator = (quint32)((m_nAccumulator << 1) & 0xff);
        --m_nBits;
        return nValue;
    }

    bool bits(qint32 nCount, qint32 *pnValue)
    {
        qint32 nValue = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            const qint32 nBit = bit();
            if (nBit < 0) return false;
            nValue = (nValue * 2) + nBit;
        }
        *pnValue = nValue;
        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nAccumulator;
    qint32 m_nBits;
};

// One member stream.  pOutput may be null, in which case the walk only counts -
// the ring buffer still has to be maintained because matches read from it.
bool c64Run(const quint8 *pData, qint64 nSize, qint64 nOffset, quint8 *pOutput, qint64 nOutputCapacity, qint64 *pnProduced, qint64 *pnConsumed,
            XBinary::PDSTRUCT *pPdStruct)
{
    if (!pData || !pnProduced || !pnConsumed || (nOffset < 0) || (nOffset > nSize)) return false;

    quint8 nWindow[C64_WINDOW_SIZE];
    for (qint32 i = 0; i < C64_WINDOW_SIZE; ++i) nWindow[i] = 0;

    C64BitReader reader(pData, nSize, nOffset);
    qint32 nWritePosition = 0;
    qint32 nWidth = C64_START_WIDTH;
    qint64 nProduced = 0;
    bool bComplete = false;

    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint32 nTag = reader.bit();
        if (nTag < 0) break;

        if (nTag == 0) {
            qint32 nLiteral = 0;
            if (!reader.bits(8, &nLiteral)) break;
            if (nProduced >= nOutputCapacity) return false;
            if (pOutput) pOutput[nProduced] = (quint8)nLiteral;
            ++nProduced;
            nWindow[nWritePosition] = (quint8)nLiteral;
            nWritePosition = (nWritePosition + 1) & C64_WINDOW_MASK;
            continue;
        }

        qint32 nMatchOffset = 0;
        if (!reader.bits(nWidth, &nMatchOffset)) break;

        if (nMatchOffset == 0) {
            const qint32 nEscape = reader.bit();
            if (nEscape < 0) break;
            if (nEscape == 0) {
                bComplete = true;
                break;
            }
            ++nWidth;
            if (nWidth >= C64_MAX_WIDTH) break;
            continue;
        }

        qint32 nLength = 0;
        if (!reader.bits(C64_LENGTH_BITS, &nLength)) break;
        --nMatchOffset;

        for (qint32 i = 0; i < nLength; ++i) {
            const quint8 nByte = nWindow[nMatchOffset & C64_WINDOW_MASK];
            if (nProduced >= nOutputCapacity) return false;
            if (pOutput) pOutput[nProduced] = nByte;
            ++nProduced;
            nWindow[nWritePosition] = nByte;
            nWritePosition = (nWritePosition + 1) & C64_WINDOW_MASK;
            ++nMatchOffset;
        }
    }

    // Running out of bits is not an end of stream: only the explicit escape is.
    if (!bComplete) return false;

    *pnProduced = nProduced;
    *pnConsumed = reader.pos();

    return true;
}
}  // namespace

bool XC64WraptorDecoder::scan(const quint8 *pData, qint64 nSize, qint64 nOffset, qint64 nMaxOutputSize, qint64 *pnConsumed, qint64 *pnRawSize,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnConsumed || !pnRawSize || (nMaxOutputSize <= 0)) return false;

    qint64 nProduced = 0;
    qint64 nEnd = 0;
    if (!c64Run(pData, nSize, nOffset, nullptr, nMaxOutputSize, &nProduced, &nEnd, pPdStruct)) return false;

    *pnConsumed = nEnd - nOffset;
    *pnRawSize = nProduced;

    return true;
}

bool XC64WraptorDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > (qint64)(std::numeric_limits<qint32>::max)())) return false;

    QByteArray baOut((qint32)nUncompressedSize, (char)0);
    if (baOut.size() != (qint32)nUncompressedSize) return false;

    qint64 nProduced = 0;
    qint64 nEnd = 0;
    if (!c64Run((const quint8 *)baPacked.constData(), baPacked.size(), 0, (quint8 *)baOut.data(), nUncompressedSize, &nProduced, &nEnd, pPdStruct)) {
        return false;
    }
    if (nProduced != nUncompressedSize) return false;

    *pbaResult = baOut;

    return true;
}
