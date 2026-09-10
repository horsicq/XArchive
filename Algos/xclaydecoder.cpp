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
#include "xclaydecoder.h"

namespace {

// The four fixed prefix codes.  Every code is matched against the LOW bits of
// the reader's look-ahead, so the stored value already carries the reversed
// bit order that an LSB-first reader needs; a code of width w matches when
// (lookahead & ((1 << w) - 1)) equals it.
static const quint16 g_arrLiteralCode[256] = {
    1168, 4064, 2016, 3040, 992, 3552, 1504, 2528, 480, 184, 98, 3808, 1760, 34, 2784, 736,
    3296, 1248, 2272, 224, 3936, 1888, 2912, 864, 3424, 1376, 4672, 2400, 352, 3680, 1632, 2656,
    15, 592, 56, 608, 80, 3168, 912, 216, 66, 2, 88, 432, 124, 41, 60, 152,
    92, 9, 28, 108, 44, 76, 24, 12, 116, 232, 104, 1120, 144, 52, 176, 1808,
    2144, 49, 84, 17, 33, 23, 20, 168, 40, 1, 784, 304, 62, 100, 30, 46,
    36, 1296, 14, 54, 22, 68, 48, 200, 464, 208, 272, 72, 1552, 336, 96, 136,
    4000, 7, 38, 6, 58, 27, 26, 42, 10, 11, 528, 4, 19, 50, 3, 29,
    18, 400, 13, 21, 5, 25, 8, 120, 240, 112, 656, 1040, 16, 1952, 2976, 928,
    576, 7232, 3136, 5184, 1088, 6208, 2112, 4160, 64, 8064, 3968, 6016, 1920, 7040, 2944, 4992,
    896, 7552, 3456, 5504, 1408, 6528, 2432, 4480, 384, 7808, 3712, 5760, 1664, 6784, 2688, 4736,
    640, 7296, 3200, 5248, 1152, 6272, 2176, 4224, 128, 7936, 3840, 5888, 1792, 6912, 2816, 4864,
    3488, 1440, 2464, 416, 3744, 1696, 2720, 672, 3232, 1184, 2208, 160, 3872, 1824, 2848, 800,
    3360, 1312, 2336, 288, 3616, 1568, 2592, 544, 3104, 1056, 2080, 32, 4032, 1984, 3008, 960,
    3520, 1472, 2496, 448, 3776, 1728, 2752, 704, 3264, 1216, 2240, 192, 3904, 1856, 2880, 832,
    768, 3392, 7424, 3328, 5376, 1344, 1280, 6400, 2304, 2368, 4352, 256, 7680, 3584, 320, 5632,
    1536, 6656, 3648, 1600, 2624, 2560, 4608, 512, 7168, 3072, 5120, 1024, 6144, 2048, 4096, 0
};

static const quint8 g_arrLiteralBits[256] = {
    11, 12, 12, 12, 12, 12, 12, 12, 12, 8, 7, 12, 12, 7, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 13, 12, 12, 12, 12, 12, 4, 10, 8, 12, 10, 12, 10, 8, 7, 7, 8, 9, 7, 6, 7, 8,
    7, 6, 7, 7, 7, 7, 8, 7, 7, 8, 8, 12, 11, 7, 9, 11, 12, 6, 7, 6, 6, 5, 7, 8,
    8, 6, 11, 9, 6, 7, 6, 6, 7, 11, 6, 6, 6, 7, 9, 8, 9, 9, 11, 8, 11, 9, 12, 8,
    12, 5, 6, 6, 6, 5, 6, 6, 6, 5, 11, 7, 5, 6, 5, 5, 6, 10, 5, 5, 5, 5, 8, 7,
    8, 8, 10, 11, 11, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 13, 12, 13, 13, 13, 12, 13, 13, 13, 12, 13, 13, 13, 13, 12, 13,
    13, 13, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13
};

static const quint8 g_arrLengthCode[16] = {
    5, 3, 1, 6, 10, 2, 12, 20, 4, 24, 8, 48, 16, 32, 64, 0
};

static const quint8 g_arrLengthBits[16] = {
    3, 2, 3, 3, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7
};

static const quint16 g_arrLengthBase[16] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264
};

static const quint8 g_arrLengthExtra[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8
};

static const quint8 g_arrDistanceCode[64] = {
    3, 13, 5, 25, 9, 17, 1, 62, 30, 46, 14, 54, 22, 38, 6, 58,
    26, 42, 10, 50, 18, 34, 66, 2, 124, 60, 92, 28, 108, 44, 76, 12,
    116, 52, 84, 20, 100, 36, 68, 4, 120, 56, 88, 24, 104, 40, 72, 8,
    240, 112, 176, 48, 208, 80, 144, 16, 224, 96, 160, 32, 192, 64, 128, 0
};

static const quint8 g_arrDistanceBits[64] = {
    2, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
};
const qint32 N_END_OF_STREAM_LENGTH = 0x207;

// LSB-first reader.  Reading past the end yields zero bits and is reported so
// the caller can stop; the encoder pads the final byte, so the last token of a
// well formed stream never needs them.
class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nAccumulator(0), m_nCount(0)
    {
    }

    bool fill(qint32 nBits)
    {
        while (m_nCount < nBits) {
            if (m_nPosition >= m_nSize) return false;
            m_nAccumulator |= (quint64)m_pData[m_nPosition] << m_nCount;
            ++m_nPosition;
            m_nCount += 8;
        }
        return true;
    }

    quint32 peek(qint32 nBits) const
    {
        return (quint32)(m_nAccumulator & (((quint64)1 << nBits) - 1));
    }

    void drop(qint32 nBits)
    {
        m_nAccumulator >>= nBits;
        m_nCount -= nBits;
    }

    bool read(qint32 nBits, quint32 *pnValue)
    {
        if (nBits == 0) {
            *pnValue = 0;
            return true;
        }
        if (!fill(nBits)) return false;
        *pnValue = peek(nBits);
        drop(nBits);
        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint64 m_nAccumulator;
    qint32 m_nCount;
};

// Linear search over one of the fixed tables.  The tables are tiny (16, 64 and
// 256 entries) and a member is at most a few hundred kilobytes, so building a
// decode tree buys nothing measurable here.
bool decodeSymbol(BitReader *pReader, const quint8 *pBits, const quint16 *pCodes16, const quint8 *pCodes8, qint32 nCount, qint32 nMaxBits, qint32 *pnSymbol)
{
    if (!pReader->fill(nMaxBits)) return false;
    const quint32 nLookahead = pReader->peek(nMaxBits);
    for (qint32 i = 0; i < nCount; ++i) {
        const qint32 nWidth = pBits[i];
        const quint32 nCode = pCodes16 ? pCodes16[i] : pCodes8[i];
        if ((nLookahead & ((1U << nWidth) - 1)) == nCode) {
            pReader->drop(nWidth);
            *pnSymbol = i;
            return true;
        }
    }
    return false;
}

}  // namespace

bool XClayDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;
    if (baPacked.size() < 2) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    const quint8 nLiteralMode = pData[0];
    const quint8 nWindowExponent = pData[1];
    if (nLiteralMode > 1) return false;
    if ((nWindowExponent < 4) || (nWindowExponent > 6)) return false;
    const bool bHuffmanLiterals = (nLiteralMode != 0);

    const qint32 nWindowSize = 0x40 << nWindowExponent;
    const quint32 nWindowMask = (quint32)(nWindowSize - 1);

    QByteArray baWindow(nWindowSize, (char)0);
    quint8 *pWindow = (quint8 *)baWindow.data();
    quint32 nWritePosition = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    BitReader reader(pData + 2, baPacked.size() - 2);

    while (baOut.size() < nUncompressedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        quint32 nFlag = 0;
        if (!reader.read(1, &nFlag)) break;

        if (nFlag == 0) {
            quint32 nLiteral = 0;
            if (bHuffmanLiterals) {
                qint32 nSymbol = 0;
                if (!decodeSymbol(&reader, g_arrLiteralBits, g_arrLiteralCode, 0, 256, 15, &nSymbol)) break;
                nLiteral = (quint32)nSymbol;
            } else {
                if (!reader.read(8, &nLiteral)) break;
            }
            baOut.append((char)(quint8)nLiteral);
            pWindow[nWritePosition] = (quint8)nLiteral;
            nWritePosition = (nWritePosition + 1) & nWindowMask;
            continue;
        }

        qint32 nLengthClass = 0;
        if (!decodeSymbol(&reader, g_arrLengthBits, 0, g_arrLengthCode, 16, 7, &nLengthClass)) break;
        quint32 nLengthExtra = 0;
        if (!reader.read(g_arrLengthExtra[nLengthClass], &nLengthExtra)) break;
        const qint32 nLength = (qint32)g_arrLengthBase[nLengthClass] + (qint32)nLengthExtra;
        if (nLength == N_END_OF_STREAM_LENGTH) break;

        qint32 nDistanceClass = 0;
        if (!decodeSymbol(&reader, g_arrDistanceBits, 0, g_arrDistanceCode, 64, 14, &nDistanceClass)) break;

        quint32 nDistanceLow = 0;
        quint32 nDistance = 0;
        if (nLength == 2) {
            if (!reader.read(2, &nDistanceLow)) break;
            nDistance = ((quint32)nDistanceClass * 4) + nDistanceLow;
        } else {
            if (!reader.read(nWindowExponent, &nDistanceLow)) break;
            nDistance = ((quint32)nDistanceClass << nWindowExponent) + nDistanceLow;
        }

        quint32 nSource = (nWritePosition - nDistance - 1) & nWindowMask;
        for (qint32 i = 0; i < nLength; ++i) {
            const quint8 nByte = pWindow[nSource];
            baOut.append((char)nByte);
            pWindow[nWritePosition] = nByte;
            nWritePosition = (nWritePosition + 1) & nWindowMask;
            nSource = (nSource + 1) & nWindowMask;
        }
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
