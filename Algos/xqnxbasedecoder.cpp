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
#include "xqnxbasedecoder.h"

#include <limits>

namespace {

// Largest match offset the original accepts before it declares the stream
// corrupt fail").
const quint32 QNXB_MAX_M_OFF = 0x1000002U;
// A block length word is a u16, so no single stream can be longer than this.
const qint32 QNXB_MAX_BLOCK = 0xffff;

class QnxBitReader {
public:
    QnxBitReader(const uchar *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nState(0), m_bError(false)
    {
    }

    qint32 bit()
    {
        m_nState = (m_nState * 2U) & 0xffffffffU;
        if ((m_nState & 0xffU) == 0) {
            if (m_nPos >= m_nSize) {
                m_bError = true;
                return 0;
            }
            m_nState = (m_nState & 0xffffff00U) | (quint32)m_pData[m_nPos];
            ++m_nPos;
            m_nState = (m_nState * 2U + 1U) & 0xffffffffU;
        }
        return (qint32)((m_nState >> 8) & 1U);
    }

    quint32 byte()
    {
        if (m_nPos >= m_nSize) {
            m_bError = true;
            return 0;
        }
        const quint32 nResult = (quint32)m_pData[m_nPos];
        ++m_nPos;
        return nResult;
    }

    bool isError() const
    {
        return m_bError;
    }
    qint64 pos() const
    {
        return m_nPos;
    }

private:
    const uchar *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nState;
    bool m_bError;
};

// One complete NRV2B stream.  Writes into the caller's output buffer, which
// also carries the match history; returns false on a malformed stream, true
// when the end marker was reached.  *pnConsumed receives the number of input
// bytes the stream used.
bool qnxDecodeBlock(const uchar *pData, qint64 nSize, uchar *pOut, qint64 nOutCapacity, qint64 *pnOutPosition, qint64 *pnConsumed)
{
    QnxBitReader reader(pData, nSize);
    quint32 nLastOffset = 1;
    qint64 nOutPosition = *pnOutPosition;

    while (true) {
        while (true) {
            const qint32 nBit = reader.bit();
            if (reader.isError()) return false;
            if (!nBit) break;
            const quint32 nLiteral = reader.byte();
            if (reader.isError()) return false;
            if (nOutPosition >= nOutCapacity) return false;
            pOut[nOutPosition++] = (uchar)nLiteral;
        }

        quint32 nOffset = 1;
        while (true) {
            nOffset = nOffset * 2U + (quint32)reader.bit();
            if (reader.isError()) return false;
            if (nOffset > QNXB_MAX_M_OFF) return false;
            const qint32 nBit = reader.bit();
            if (reader.isError()) return false;
            if (nBit) break;
        }

        if (nOffset == 2) {
            nOffset = nLastOffset;
        } else {
            const quint32 nExtra = reader.byte();
            if (reader.isError()) return false;
            nOffset = (nOffset - 3U) * 0x100U + nExtra;
            if (nOffset == 0xffffffffU) {
                *pnOutPosition = nOutPosition;
                *pnConsumed = reader.pos();
                return true;
            }
            ++nOffset;
            nLastOffset = nOffset;
        }

        const qint32 nHigh = reader.bit();
        if (reader.isError()) return false;
        const qint32 nLow = reader.bit();
        if (reader.isError()) return false;
        qint64 nLength = (qint64)nHigh * 2 + nLow + 1;
        if (nLength == 1) {
            while (true) {
                nLength = nLength * 2 + reader.bit();
                if (reader.isError()) return false;
                if (nLength > 0x7fffffff) return false;
                const qint32 nBit = reader.bit();
                if (reader.isError()) return false;
                if (nBit) break;
            }
            nLength += 3;
        }
        if (nOffset > 0xd00U) ++nLength;

        if ((qint64)nOffset > nOutPosition) return false;
        if (nLength > (nOutCapacity - nOutPosition)) return false;

        qint64 nSource = nOutPosition - (qint64)nOffset;
        for (qint64 i = 0; i < nLength; ++i) {
            pOut[nOutPosition++] = pOut[nSource++];
        }
    }
}

}  // namespace

bool XQNXBaseDecoder::decodeImage(const QByteArray &baPacked, qint64 nMaxOutput, qint64 nStopAfter, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) return false;
    pbaUnpacked->clear();
    if ((nMaxOutput <= 0) || (nMaxOutput > (qint64)(std::numeric_limits<qint32>::max)())) return false;

    QByteArray baOutput((qint32)nMaxOutput, 0);
    if ((qint64)baOutput.size() != nMaxOutput) return false;
    uchar *pOutput = (uchar *)baOutput.data();
    qint64 nOutPosition = 0;

    const uchar *pData = (const uchar *)baPacked.constData();
    const qint64 nSize = baPacked.size();
    qint64 nPosition = 0;

    while (true) {
        if ((nStopAfter >= 0) && (nOutPosition >= nStopAfter)) break;
        if (nPosition + 2 > nSize) return false;
        const qint32 nBlockSize = ((qint32)pData[nPosition] << 8) | (qint32)pData[nPosition + 1];
        nPosition += 2;
        if (nBlockSize == 0) break;
        if (nBlockSize > QNXB_MAX_BLOCK) return false;
        if (nBlockSize > nSize - nPosition) return false;

        qint64 nConsumed = 0;
        if (!qnxDecodeBlock(pData + nPosition, nBlockSize, pOutput, nMaxOutput, &nOutPosition, &nConsumed)) return false;
        // Each block's declared length must match what the end marker consumed;
        // a mismatch means the chain is not really a QNX block chain.
        if (nConsumed != (qint64)nBlockSize) return false;
        nPosition += nBlockSize;
    }

    baOutput.resize((qint32)nOutPosition);
    *pbaUnpacked = baOutput;
    return true;
}

bool XQNXBaseDecoder::decodeRange(const QByteArray &baPacked, qint64 nOffset, qint64 nSize, qint64 nMaxOutput, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) return false;
    pbaUnpacked->clear();
    if ((nOffset < 0) || (nSize < 0)) return false;
    if (nOffset > nMaxOutput) return false;
    if (nSize > nMaxOutput - nOffset) return false;

    QByteArray baImage;
    if (!XQNXBaseDecoder::decodeImage(baPacked, nMaxOutput, nOffset + nSize, &baImage)) return false;
    if ((qint64)baImage.size() < nOffset + nSize) return false;

    *pbaUnpacked = baImage.mid((qint32)nOffset, (qint32)nSize);
    return true;
}
