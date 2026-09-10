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
#include "xxeditpackdecoder.h"

namespace {
const quint8 XEP_BLANK = 0x40;  // the EBCDIC space

// One walker drives both measure() and decode(): if the two ever disagreed on
// how a damaged stream ends, the reader would publish a length the extractor
// cannot reproduce and every member of that file would silently fail.
class Walker {
public:
    Walker(const QByteArray &baPacked, QByteArray *pbaOut, qint64 nCeiling)
        : m_pData((const quint8 *)baPacked.constData()), m_nSize(baPacked.size()), m_nPosition(0), m_pbaOut(pbaOut), m_nProduced(0), m_nCeiling(nCeiling),
          m_bOverflow(false), m_bTruncated(false)
    {
    }

    qint64 produced() const
    {
        return m_nProduced;
    }

    bool run(XBinary::PDSTRUCT *pPdStruct)
    {
        while (true) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

            quint8 nOpcode = 0;
            if (!readByte(&nOpcode)) break;

            if (nOpcode <= 0x77) {
                if (!fill(XEP_BLANK, (qint64)nOpcode + 1)) return false;
            } else if ((nOpcode == 0x78) || (nOpcode == 0x7c)) {
                quint8 nCount = 0;
                if (!readByte(&nCount)) break;
                if (!fill(XEP_BLANK, (qint64)nCount + 1)) return false;
            } else if ((nOpcode == 0x79) || (nOpcode == 0x7d)) {
                quint32 nCount = 0;
                if (!readWord(&nCount)) break;
                if (!fill(XEP_BLANK, (qint64)nCount + 1)) return false;
            } else if ((nOpcode == 0x7a) || (nOpcode == 0x7e)) {
                quint8 nCount = 0;
                quint8 nByte = 0;
                // The count is read first and the run byte second; when the run
                // byte is missing NOTHING is emitted for this opcode.
                if (!readByte(&nCount)) break;
                if (!readByte(&nByte)) break;
                if (!fill(nByte, (qint64)nCount + 1)) return false;
            } else if ((nOpcode == 0x7b) || (nOpcode == 0x7f)) {
                quint32 nCount = 0;
                quint8 nByte = 0;
                if (!readWord(&nCount)) break;
                if (!readByte(&nByte)) break;
                if (!fill(nByte, (qint64)nCount + 1)) return false;
            } else if ((nOpcode >= 0x80) && (nOpcode <= 0xf7)) {
                if (!copy((qint64)(nOpcode - 0x80) + 1)) return false;
                if (m_bTruncated) break;
            } else if ((nOpcode == 0xf8) || (nOpcode == 0xfc)) {
                quint8 nCount = 0;
                if (!readByte(&nCount)) break;
                if (!copy((qint64)nCount + 1)) return false;
                if (m_bTruncated) break;
            } else if ((nOpcode == 0xf9) || (nOpcode == 0xfd)) {
                quint32 nCount = 0;
                if (!readWord(&nCount)) break;
                if (!copy((qint64)nCount + 1)) return false;
                if (m_bTruncated) break;
            } else {
                // 0xFF is the clean end; 0xFA / 0xFB / 0xFE are malformed and
                // stop the walk the same way, keeping the output so far.
                break;
            }
        }

        return !m_bOverflow;
    }

private:
    bool readByte(quint8 *pnValue)
    {
        if (m_nPosition >= m_nSize) return false;
        *pnValue = m_pData[m_nPosition];
        ++m_nPosition;
        return true;
    }

    // Big endian, high byte first - this is a mainframe format.
    bool readWord(quint32 *pnValue)
    {
        quint8 nHigh = 0;
        quint8 nLow = 0;
        if (!readByte(&nHigh)) return false;
        if (!readByte(&nLow)) return false;
        *pnValue = ((quint32)nHigh << 8) | (quint32)nLow;
        return true;
    }

    bool reserve(qint64 nCount)
    {
        if (nCount < 0) return false;
        if (nCount > (m_nCeiling - m_nProduced)) {
            m_bOverflow = true;
            return false;
        }
        return true;
    }

    bool fill(quint8 nByte, qint64 nCount)
    {
        if (!reserve(nCount)) return false;
        if (m_pbaOut) {
            for (qint64 i = 0; i < nCount; ++i) m_pbaOut->append((char)nByte);
        }
        m_nProduced += nCount;
        return true;
    }

    // A literal copy that runs past the end still appends the bytes that ARE
    // present and only then ends the walk.  That is what the reference
    // extractor does, and a truncated member decodes differently otherwise.
    bool copy(qint64 nCount)
    {
        if (!reserve(nCount)) return false;
        qint64 nAvailable = m_nSize - m_nPosition;
        if (nAvailable < 0) nAvailable = 0;
        const qint64 nTaken = (nCount <= nAvailable) ? nCount : nAvailable;
        if (m_pbaOut && (nTaken > 0)) m_pbaOut->append((const char *)(m_pData + m_nPosition), (int)nTaken);
        m_nProduced += nTaken;
        m_nPosition += nCount;
        m_bTruncated = (nTaken != nCount);
        return true;
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    QByteArray *m_pbaOut;
    qint64 m_nProduced;
    qint64 m_nCeiling;
    bool m_bOverflow;
    bool m_bTruncated;
};

}  // namespace

// Out-of-class definitions so the constants may be odr-used (passed by
// reference into templates such as qMin) from any translation unit.
const qint64 XXEditPackDecoder::MAX_UNCOMPRESSED_SIZE;

bool XXEditPackDecoder::measure(const QByteArray &baPacked, qint64 *pnUncompressedSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnUncompressedSize) return false;
    *pnUncompressedSize = 0;
    if (baPacked.isEmpty()) return false;

    Walker walker(baPacked, nullptr, MAX_UNCOMPRESSED_SIZE);
    if (!walker.run(pPdStruct)) return false;
    if (walker.produced() <= 0) return false;

    *pnUncompressedSize = walker.produced();

    return true;
}

bool XXEditPackDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    Walker walker(baPacked, &baOut, MAX_UNCOMPRESSED_SIZE);
    if (!walker.run(pPdStruct)) return false;

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
