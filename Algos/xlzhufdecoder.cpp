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
#include "xlzhufdecoder.h"

namespace {

// LHA's classic "-lh1-" position tables: the six-bit distance prefix and the
// number of bits the prefix was coded in.  Both LZHUF and LHA build these at run
// time from the value ranges {0} {1..3} {4..11} {12..23} {24..47} {48..63} coded
// in 3,4,5,6,7,8 bits; spelling them out avoids a static initialiser.
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

// The adaptive Huffman tree plus its MSB-first bit reader.  Nothing here is
// output related - the ring buffer and the length rules live in decode(),
// because that is where the embedders differ.
class Lzhuf {
public:
    Lzhuf(const quint8 *pData, qint64 nSize, const XLZHUFDecoder::OPTIONS &options)
        : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nBuffer(0), m_nCount(0), m_bEOF(false), m_nNChar(options.nNChar), m_nT(options.nNChar * 2 - 1),
          m_nR(options.nNChar * 2 - 2), m_nMaxFreq(options.nMaxFreq), m_nDistVariant(options.nDistVariant), m_bReconstruct(options.bReconstruct)
    {
        m_vecFreq.resize(m_nT + 1);
        m_vecSon.resize(m_nT);
        m_vecPrnt.resize(m_nT + m_nNChar);
        startHuff();
    }

    bool isEOF() const
    {
        return m_bEOF;
    }

    // -1 on a short stream or a corrupt tree.
    qint32 decodeChar()
    {
        qint32 nCode = m_vecSon.at(m_nR);

        while (nCode < m_nT) {
            const qint32 nBit = getBit();
            if (m_bEOF) return -1;
            const qint32 nIndex = nCode + nBit;
            if ((nIndex < 0) || (nIndex >= m_nT)) return -1;
            nCode = m_vecSon.at(nIndex);
        }

        nCode -= m_nT;
        if ((nCode < 0) || (nCode >= m_nNChar)) return -1;
        update(nCode);

        return nCode;
    }

    // -1 on a short stream.
    qint32 decodePosition()
    {
        qint32 nByte = getByte();
        if (m_bEOF) return -1;
        nByte &= 0xFF;

        const qint32 nBits = g_dLen[nByte >> 4];
        qint32 nBase = 0;
        qint32 nExtra = 0;
        qint32 nMask = 0;

        if (m_nDistVariant == 0) {
            nBase = ((qint32)g_dCode[nByte]) << 5;
            nExtra = nBits - 3;
            nMask = 0x1F;
        } else if (m_nDistVariant == 1) {
            nBase = ((qint32)g_dCode[nByte]) << 6;
            nExtra = nBits - 2;
            nMask = 0x3F;
        } else {
            nBase = ((qint32)g_dCode[nByte]) << 7;
            nExtra = nBits - 1;
            nMask = 0x7F;
        }

        while (nExtra > 0) {
            --nExtra;
            nByte = ((nByte << 1) + getBit()) & 0xFFFF;
            if (m_bEOF) return -1;
        }

        return nBase | (nByte & nMask);
    }

private:
    void fill()
    {
        quint32 nByte = 0;

        if (m_nPosition < m_nSize) {
            nByte = m_pData[m_nPosition];
            ++m_nPosition;
        } else {
            m_bEOF = true;
        }

        m_nBuffer = m_nBuffer | (nByte << ((8 - m_nCount) & 0x1F));
        m_nCount += 8;
    }

    qint32 getBit()
    {
        while (m_nCount == 0) {
            fill();
            if (m_bEOF) break;
        }

        const quint32 nValue = m_nBuffer;
        m_nBuffer = m_nBuffer << 1;
        --m_nCount;

        return (nValue & 0x8000) ? 1 : 0;
    }

    qint32 getByte()
    {
        while (m_nCount < 8) {
            fill();
            if (m_bEOF) break;
        }

        const quint32 nValue = m_nBuffer;
        m_nBuffer = m_nBuffer << 8;
        m_nCount -= 8;

        return (qint32)((nValue >> 8) & 0xFF);
    }

    void startHuff()
    {
        for (qint32 i = 0; i < m_nNChar; ++i) {
            m_vecFreq[i] = 1;
            m_vecSon[i] = i + m_nT;
            m_vecPrnt[i + m_nT] = i;
        }

        qint32 i = 0;
        qint32 j = m_nNChar;

        while (j <= m_nR) {
            m_vecFreq[j] = (quint16)(m_vecFreq.at(i) + m_vecFreq.at(i + 1));
            m_vecSon[j] = i;
            m_vecPrnt[i] = j;
            m_vecPrnt[i + 1] = j;
            i += 2;
            ++j;
        }

        m_vecFreq[m_nT] = 0xFFFF;
        m_vecPrnt[m_nR] = 0;
    }

    void reconst()
    {
        qint32 j = 0;

        for (qint32 i = 0; i < m_nT; ++i) {
            if (m_vecSon.at(i) >= m_nT) {
                m_vecFreq[j] = (quint16)((m_vecFreq.at(i) + 1) >> 1);
                m_vecSon[j] = m_vecSon.at(i);
                ++j;
            }
        }

        qint32 i = 0;
        j = m_nNChar;

        while (j < m_nT) {
            const quint16 nFreq = (quint16)(m_vecFreq.at(i) + m_vecFreq.at(i + 1));
            m_vecFreq[j] = nFreq;

            qint32 k = j - 1;
            // The frequencies below j are already sorted, so this walk always
            // stops; the k >= 0 bound only matters for a tree corrupted by a bad
            // stream, where the original would index off the front of the array.
            while ((k >= 0) && (nFreq < m_vecFreq.at(k))) --k;
            ++k;

            for (qint32 n = j; n > k; --n) {
                m_vecFreq[n] = m_vecFreq.at(n - 1);
                m_vecSon[n] = m_vecSon.at(n - 1);
            }

            m_vecFreq[k] = nFreq;
            m_vecSon[k] = i;

            i += 2;
            ++j;
        }

        for (qint32 n = 0; n < m_nT; ++n) {
            const qint32 k = m_vecSon.at(n);
            if (k < m_nT) {
                m_vecPrnt[k] = n;
                m_vecPrnt[k + 1] = n;
            } else {
                m_vecPrnt[k] = n;
            }
        }
    }

    void update(qint32 nSymbol)
    {
        if ((qint32)m_vecFreq.at(m_nR) == m_nMaxFreq) {
            // Zoom's decoder simply stops re-weighting from here; the textbook
            // one halves everything and rebuilds.  Neither is reached by the
            // current corpora but they are not interchangeable.
            if (!m_bReconstruct) return;
            reconst();
        }

        qint32 c = m_vecPrnt.at(nSymbol + m_nT);

        do {
            m_vecFreq[c] = (quint16)(m_vecFreq.at(c) + 1);
            const quint16 nFreq = m_vecFreq.at(c);
            qint32 l = c + 1;

            if (m_vecFreq.at(l) < nFreq) {
                // freq[T] is the 0xFFFF sentinel that stops this walk, so l may
                // legitimately reach T - one past the last node - before the
                // decrement below pulls it back into range.
                l = c + 2;
                while ((l <= m_nT) && (m_vecFreq.at(l) < nFreq)) ++l;
                --l;
                if ((l < 0) || (l >= m_nT)) return;

                m_vecFreq[c] = m_vecFreq.at(l);
                m_vecFreq[l] = nFreq;

                const qint32 i = m_vecSon.at(c);
                m_vecPrnt[i] = l;
                if (i < m_nT) m_vecPrnt[i + 1] = l;

                const qint32 j = m_vecSon.at(l);
                m_vecSon[l] = i;
                m_vecPrnt[j] = c;
                if (j < m_nT) m_vecPrnt[j + 1] = c;

                m_vecSon[c] = j;
                c = l;
            }

            c = m_vecPrnt.at(c);
        } while (c != 0);
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nBuffer;
    qint32 m_nCount;
    bool m_bEOF;
    qint32 m_nNChar;
    qint32 m_nT;
    qint32 m_nR;
    qint32 m_nMaxFreq;
    qint32 m_nDistVariant;
    bool m_bReconstruct;
    QVector<quint16> m_vecFreq;
    QVector<qint32> m_vecSon;
    QVector<qint32> m_vecPrnt;
};

}  // namespace

XLZHUFDecoder::OPTIONS XLZHUFDecoder::getZTCOptions()
{
    // F = 60, THRESHOLD = 2 -> N_CHAR = 0x100 - (2 - 60) = 314, T = 627, R = 626.
    OPTIONS result;
    result.nDistVariant = 1;
    result.nNChar = 314;
    result.nRingSize = 0x2000;
    result.nRingFill = 0x20;
    result.nMaxFreq = 0x8000;
    result.nEOFCode = -1;
    result.bShiftAboveEOF = false;
    result.nLengthBias = 2;
    result.nMatchBias = 1;
    result.bReconstruct = true;

    return result;
}

XLZHUFDecoder::OPTIONS XLZHUFDecoder::getZoomOptions()
{
    // N_CHAR 317 does NOT follow from F/THRESHOLD the way the reference
    // implementation's variants do, and neither do the two biases below - see
    // the header for why each of them is silently destructive if guessed.
    OPTIONS result;
    result.nDistVariant = 1;
    result.nNChar = 0x13D;
    result.nRingSize = 0x1000;
    result.nRingFill = 0x00;
    result.nMaxFreq = 0x8000;
    result.nEOFCode = 0x13C;
    result.bShiftAboveEOF = false;
    result.nLengthBias = 0;
    result.nMatchBias = 0;
    result.bReconstruct = false;

    return result;
}

XLZHUFDecoder::OPTIONS XLZHUFDecoder::getOptions(qint32 nDistVariant, qint32 nFSel, qint32 nThrSel, bool bHasEOF, bool bBigFreq, bool bFillZero)
{
    OPTIONS result;

    qint32 nF = 0x5A;
    if (nFSel == 0) nF = 0x20;
    else if (nFSel == 1) nF = 0x3C;
    else if (nFSel == 2) nF = 0x3D;

    const qint32 nThreshold = (nThrSel == 1) ? 3 : 2;

    result.nDistVariant = nDistVariant;
    result.nNChar = 0x100 - (nThreshold - nF) + (bHasEOF ? 1 : 0);
    result.nRingSize = 0x2000;
    result.nRingFill = bFillZero ? 0x00 : 0x20;
    result.nMaxFreq = bBigFreq ? 0xD000 : 0x8000;
    result.nEOFCode = bHasEOF ? 0x100 : -1;
    result.bShiftAboveEOF = bHasEOF;
    result.nLengthBias = nThreshold;
    result.nMatchBias = 1;
    result.bReconstruct = true;

    return result;
}

bool XLZHUFDecoder::decode(const QByteArray &baPacked, const OPTIONS &options, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;
    if ((options.nDistVariant < 0) || (options.nDistVariant > 2)) return false;
    if ((options.nNChar < 0x101) || (options.nNChar > 0x4000)) return false;
    if ((options.nRingSize < 0x100) || (options.nRingSize > 0x10000)) return false;
    if ((options.nRingSize & (options.nRingSize - 1)) != 0) return false;
    if ((options.nLengthBias < 0) || (options.nLengthBias > 0x100)) return false;
    if ((options.nMatchBias < 0) || (options.nMatchBias > 1)) return false;
    if (options.nEOFCode >= options.nNChar) return false;

    const bool bHasEOF = (options.nEOFCode >= 0);
    const qint32 nMask = options.nRingSize - 1;

    QByteArray baRing(options.nRingSize, (char)(quint8)options.nRingFill);
    quint8 *pRing = (quint8 *)baRing.data();

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    Lzhuf huf((const quint8 *)baPacked.constData(), baPacked.size(), options);

    qint32 nRing = 0;
    qint64 nGuard = 0;
    bool bFinished = !bHasEOF;

    while (bHasEOF || (baOut.size() < nUncompressedSize)) {
        ++nGuard;
        if ((nGuard & 0xFFF) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        qint32 nCode = huf.decodeChar();
        if (nCode < 0) break;

        if (nCode < 0x100) {
            if (baOut.size() >= nUncompressedSize) return false;
            baOut.append((char)(quint8)nCode);
            pRing[nRing] = (quint8)nCode;
            nRing = (nRing + 1) & nMask;
            continue;
        }

        if (bHasEOF) {
            if (nCode == options.nEOFCode) {
                bFinished = true;
                break;
            }
            if (options.bShiftAboveEOF && (nCode > options.nEOFCode)) --nCode;
        }

        const qint32 nDistance = huf.decodePosition();
        if (nDistance < 0) break;

        qint32 nSource = (nRing - nDistance - options.nMatchBias) & nMask;
        qint64 nLength = (qint64)nCode - 0xFF + options.nLengthBias;
        if (nLength <= 0) return false;

        // Without an end symbol the stored size is the only stop condition, so a
        // final match that overshoots is truncated; with one, an overshoot is a
        // corrupt stream.
        if (!bHasEOF) {
            if (nLength > (nUncompressedSize - baOut.size())) nLength = nUncompressedSize - baOut.size();
        }

        while (nLength > 0) {
            if (baOut.size() >= nUncompressedSize) return false;
            const quint8 nByte = pRing[nSource];
            baOut.append((char)nByte);
            pRing[nRing] = nByte;
            nRing = (nRing + 1) & nMask;
            nSource = (nSource + 1) & nMask;
            --nLength;
        }
    }

    if (!bFinished) return false;

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
