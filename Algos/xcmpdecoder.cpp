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
#include "xcmpdecoder.h"
#include "xsharedlzwdecoder.h"

namespace {

const qint32 N_WINDOW_SIZE = 0x800;
const qint32 N_WINDOW_MASK = N_WINDOW_SIZE - 1;

// The method 1 framing works in blocks of this size - the same value the
// container validates in the word at header offset 0x37.  A frame that reaches
// it is a block the compressor could not shrink, so it was stored raw.
const qint32 N_BLOCK_SIZE = 0x1000;

// MSB-first reader over a 32-bit accumulator, matching the reference bit
// reader exactly: it keeps feeding zero bytes once the input is exhausted and
// only fails one byte later, which is what lets a truncated stream produce the
// same prefix the reference implementation writes.
class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nAccumulator(0), m_nCount(0)
    {
    }

    bool read(qint32 nBits, quint32 *pnValue)
    {
        if (nBits == 0) {
            *pnValue = 0;
            return true;
        }
        while (m_nCount < nBits) {
            quint32 nByte = 0;
            if (m_nPosition > m_nSize) return false;
            if (m_nPosition < m_nSize) nByte = m_pData[m_nPosition];
            ++m_nPosition;
            m_nAccumulator = (m_nAccumulator + (nByte << (24 - m_nCount))) & 0xffffffffU;
            m_nCount += 8;
        }
        *pnValue = m_nAccumulator >> (32 - nBits);
        m_nAccumulator = (m_nAccumulator << nBits) & 0xffffffffU;
        m_nCount -= nBits;
        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nAccumulator;
    qint32 m_nCount;
};

bool readLength(BitReader *pReader, qint32 *pnLength)
{
    quint32 nValue = 0;
    if (!pReader->read(2, &nValue)) return false;
    qint32 nLength = (qint32)nValue;
    if (nLength == 3) {
        quint32 nMore = 0;
        if (!pReader->read(2, &nMore)) return false;
        nLength += (qint32)nMore;
        if (nMore == 3) {
            quint32 nNibble = 0;
            do {
                if (!pReader->read(4, &nNibble)) return false;
                nLength += (qint32)nNibble;
            } while (nNibble == 15);
        }
    }
    *pnLength = nLength;
    return true;
}

}  // namespace

bool XCMPDecoder::decodeFramedLZW(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    XSharedLZWDecoder::OPTIONS options;
    options.nMaxBits = 16;
    options.bHasClearCode = true;
    options.bHasEndCode = true;
    options.bMsbFirst = true;
    options.bUnRle90 = false;
    options.bBlockPadding = false;
    options.nWidthStepBias = 1;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    const quint8 *pData = (const quint8 *)baPacked.constData();
    qint64 nOffset = 0;
    while ((nOffset + 2) <= baPacked.size()) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nFrame = (qint32)pData[nOffset] | ((qint32)pData[nOffset + 1] << 8);
        nOffset += 2;
        if ((nFrame == 0) || ((nOffset + nFrame) > baPacked.size())) break;
        if (nFrame >= N_BLOCK_SIZE) {
            // A block the compressor could not shrink is stored whole, so the
            // frame is one block of plain bytes.  Handing it to the LZW decoder
            // instead - which is what the reference implementation does - turns
            // it into a few bytes of noise and loses the rest of the member.
            baOut.append(baPacked.mid((qint32)nOffset, nFrame));
        } else {
            QByteArray baChunk;
            // Each frame is a complete stream that ends on its own end code, so
            // the per-frame limit is only a ceiling, never the stopping rule.
            // An empty frame is not the end of the member either: the frame
            // count is what ends the loop, and nOffset always moves forward.
            XSharedLZWDecoder::decode(baPacked.mid((qint32)nOffset, nFrame), options, nUncompressedSize, &baChunk, pPdStruct);
            baOut.append(baChunk);
        }
        nOffset += nFrame;
        if (baOut.size() >= nUncompressedSize) break;
    }

    if (baOut.size() > nUncompressedSize) baOut.truncate((qint32)nUncompressedSize);
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XCMPDecoder::decodeLZSS(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    QByteArray baWindow(N_WINDOW_SIZE, (char)0);
    quint8 *pWindow = (quint8 *)baWindow.data();
    qint32 nPosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    BitReader reader((const quint8 *)baPacked.constData(), baPacked.size());

    while (baOut.size() < nUncompressedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        quint32 nToken = 0;
        if (!reader.read(9, &nToken)) break;

        if (nToken < 0x100) {
            pWindow[nPosition] = (quint8)nToken;
            baOut.append((char)(quint8)nToken);
            nPosition = (nPosition + 1) & N_WINDOW_MASK;
            continue;
        }

        const quint32 nLow = nToken & 0xff;
        qint32 nDistance = 0;

        if (nLow == 0x81) {
            qint32 nLength = 0;
            if (!readLength(&reader, &nLength)) break;
            nLength += 2;
            const quint8 nByte = pWindow[(nPosition - 1) & N_WINDOW_MASK];
            for (qint32 i = 0; i < nLength; ++i) {
                pWindow[nPosition] = nByte;
                baOut.append((char)nByte);
                nPosition = (nPosition + 1) & N_WINDOW_MASK;
            }
            continue;
        }

        if (nLow < 0x80) {
            quint32 nExtra = 0;
            if (!reader.read(4, &nExtra)) break;
            nDistance = (qint32)(nLow * 16 + nExtra);
        } else {
            nDistance = (qint32)(nLow & 0x7f);
            if (nDistance == 0) break;
        }

        qint32 nLength = 0;
        if (!readLength(&reader, &nLength)) break;
        nLength += 2;
        for (qint32 i = 0; i < nLength; ++i) {
            const quint8 nByte = pWindow[(nPosition - nDistance) & N_WINDOW_MASK];
            pWindow[nPosition] = nByte;
            baOut.append((char)nByte);
            nPosition = (nPosition + 1) & N_WINDOW_MASK;
        }
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
