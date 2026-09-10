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
#include "xsharedlzwdecoder.h"

namespace {

const qint32 N_CLEAR_CODE = 0x100;
const qint32 N_END_CODE = 0x101;

// Emits either straight to the result or through the ARC RLE90 filter.
class Sink {
public:
    Sink(QByteArray *pbaOut, bool bUnRle90) : m_pbaOut(pbaOut), m_bFilter(bUnRle90), m_bEscaped(false), m_nLast(0)
    {
    }

    void push(quint8 nByte)
    {
        if (!m_bFilter) {
            m_pbaOut->append((char)nByte);
            return;
        }
        if (!m_bEscaped) {
            if (nByte == 0x90) {
                m_bEscaped = true;
            } else {
                m_nLast = nByte;
                m_pbaOut->append((char)nByte);
            }
            return;
        }
        m_bEscaped = false;
        if (nByte == 0) {
            // An escaped literal 0x90 is emitted but does NOT become the run byte.
            m_pbaOut->append((char)(quint8)0x90);
        } else {
            for (qint32 i = 1; i < (qint32)nByte; ++i) m_pbaOut->append((char)m_nLast);
        }
    }

    qint32 produced() const
    {
        return m_pbaOut->size();
    }

private:
    QByteArray *m_pbaOut;
    bool m_bFilter;
    bool m_bEscaped;
    quint8 m_nLast;
};

// Code reader: owns the width, the free-slot counter and the group counter,
// because the padding rule depends on all three.
class CodeReader {
public:
    CodeReader(const quint8 *pData, qint64 nSize, const XSharedLZWDecoder::OPTIONS &options, qint32 nFirstFree)
        : m_pData(pData), m_nBits(nSize * 8), m_nBitPosition(0), m_options(options), m_nWidth(9), m_nMaxCode(0x1ff), m_nFree(nFirstFree),
          m_nCapacity(1 << options.nMaxBits), m_nGroupCount(0)
    {
    }

    qint32 free() const
    {
        return m_nFree;
    }

    void addEntry()
    {
        if (m_nFree < m_nCapacity) ++m_nFree;
    }

    bool reset(qint32 nFirstFree)
    {
        if (m_options.bBlockPadding && !pad()) return false;
        m_nWidth = 9;
        m_nMaxCode = 0x1ff;
        m_nFree = nFirstFree;
        return true;
    }

    bool next(qint32 *pnCode)
    {
        if (m_nMaxCode < (m_nFree + m_options.nWidthStepBias)) {
            if (m_options.bBlockPadding && !pad()) return false;
            ++m_nWidth;
            m_nMaxCode = (m_nWidth == m_options.nMaxBits) ? m_nCapacity : ((1 << m_nWidth) - 1);
        }
        // A code that runs past the final byte is still readable - the encoder
        // pads the tail with zero bits - but a code that STARTS past it is not.
        if (m_nBitPosition >= m_nBits) return false;
        quint32 nValue = 0;
        if (m_options.bMsbFirst) {
            for (qint32 i = 0; i < m_nWidth; ++i) {
                const qint64 nBit = m_nBitPosition + i;
                const quint32 nSet = (nBit >= m_nBits) ? 0 : ((m_pData[nBit >> 3] >> (7 - (nBit & 7))) & 1);
                nValue = (nValue << 1) | nSet;
            }
        } else {
            for (qint32 i = 0; i < m_nWidth; ++i) {
                const qint64 nBit = m_nBitPosition + i;
                const quint32 nSet = (nBit >= m_nBits) ? 0 : ((m_pData[nBit >> 3] >> (nBit & 7)) & 1);
                nValue |= nSet << i;
            }
        }
        m_nBitPosition += m_nWidth;
        ++m_nGroupCount;
        *pnCode = (qint32)nValue;
        return true;
    }

private:
    bool pad()
    {
        const qint64 nSkip = (qint64)((8 - (m_nGroupCount & 7)) & 7) * m_nWidth;
        m_nBitPosition += nSkip;
        m_nGroupCount = 0;
        return m_nBitPosition <= m_nBits;
    }

    const quint8 *m_pData;
    qint64 m_nBits;
    qint64 m_nBitPosition;
    XSharedLZWDecoder::OPTIONS m_options;
    qint32 m_nWidth;
    qint32 m_nMaxCode;
    qint32 m_nFree;
    qint32 m_nCapacity;
    qint32 m_nGroupCount;
};

}  // namespace

bool XSharedLZWDecoder::decode(const QByteArray &baPacked, const OPTIONS &options, qint64 nUncompressedSize, QByteArray *pbaResult,
                           XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((options.nMaxBits < 9) || (options.nMaxBits > 16)) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    qint32 nFirstFree = 0;
    if (!options.bHasClearCode) {
        nFirstFree = 0x100;
    } else if (!options.bHasEndCode) {
        nFirstFree = N_END_CODE;
    } else {
        nFirstFree = 0x102;
    }

    const qint32 nCapacity = 1 << options.nMaxBits;
    QVector<qint32> vecPrefix(nCapacity, 0);
    QByteArray baSuffix(nCapacity, (char)0);
    quint8 *pSuffix = (quint8 *)baSuffix.data();
    for (qint32 i = 0; i < 256; ++i) pSuffix[i] = (quint8)i;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    Sink sink(&baOut, options.bUnRle90);
    CodeReader reader((const quint8 *)baPacked.constData(), baPacked.size(), options, nFirstFree);

    QByteArray baStack;
    qint32 nPrevious = -1;
    quint8 nPreviousFirst = 0;

    while (sink.produced() < nUncompressedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        qint32 nCode = 0;
        if (!reader.next(&nCode)) break;
        if (options.bHasEndCode && (nCode == N_END_CODE)) break;

        if (options.bHasClearCode && (nCode == N_CLEAR_CODE)) {
            if (!reader.reset(nFirstFree)) break;
            if (!reader.next(&nCode)) break;
            if (nCode > 0xff) break;
            nPrevious = nCode;
            nPreviousFirst = (quint8)nCode;
            sink.push((quint8)nCode);
            continue;
        }

        if (nPrevious < 0) {
            if (nCode > 0xff) return false;
            nPrevious = nCode;
            nPreviousFirst = (quint8)nCode;
            sink.push((quint8)nCode);
            continue;
        }

        baStack.resize(0);
        qint32 nCurrent = nCode;
        if (nCode >= reader.free()) {
            baStack.append((char)nPreviousFirst);
            nCurrent = nPrevious;
        }
        qint32 nGuard = 0;
        while (nCurrent > 0xff) {
            if ((nCurrent >= nCapacity) || (++nGuard > nCapacity)) return false;
            baStack.append((char)pSuffix[nCurrent]);
            nCurrent = vecPrefix[nCurrent];
        }
        baStack.append((char)(quint8)nCurrent);
        for (qint32 i = baStack.size() - 1; i >= 0; --i) sink.push((quint8)baStack.at(i));

        if (reader.free() < nCapacity) {
            vecPrefix[reader.free()] = nPrevious;
            pSuffix[reader.free()] = (quint8)nCurrent;
            reader.addEntry();
        }
        nPrevious = nCode;
        nPreviousFirst = (quint8)nCurrent;
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XSharedLZWDecoder::unRle90(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    Sink sink(&baOut, true);
    for (qint32 i = 0; i < baPacked.size(); ++i) sink.push((quint8)baPacked.at(i));
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
