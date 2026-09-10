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
#include "xsqxdecoder.h"

#include <QVector>
#include <QtEndian>

#include <cstring>

namespace {
const qint32 SQX_NMAIN = 0x136;
const qint32 SQX_NPRE = 0x13;
const qint32 SQX_NLEN = 0x19;
const qint32 SQX_NDIST = 0x38;
const qint32 SQX_MAXLEN_PRE = 7;
const qint32 SQX_MAXLEN = 15;
const qint32 SQX_MAX_DICT = 0x400000;
const qint64 SQX_OVERSHOOT = 0x10000;
const qint64 SQX_CANCEL_STEP = 0x10000;
const quint32 SQX_TABLE_MAGIC = 0x54585153;  // 'SQXT'

// Distance base/extra for the len == 2 symbol class.
const qint32 SQX_L2BASE[8] = {0, 4, 8, 16, 32, 64, 128, 192};
const qint32 SQX_L2EXTRA[8] = {2, 2, 3, 4, 5, 6, 6, 6};
// Distance base/extra for the len == 3 symbol class.
const qint32 SQX_L3BASE[15] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
const qint32 SQX_L3EXTRA[15] = {0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
// Match length base/extra.
const qint32 SQX_LENBASE[26] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 16, 20, 24, 32, 40, 48, 64, 80, 96, 128, 160, 192, 224, 0, 0};
const qint32 SQX_LENEXTRA[26] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 5, 5, 0, 0};
// Distance base/extra, dictionary < 2 MiB.
const qint32 SQX_D48BASE[49] = {0,      1,      2,      3,      4,      6,      8,      12,     16,     24,     32,     48,     64,
                                96,     128,    192,    256,    384,    512,    768,    1024,   1536,   2048,   3072,   4096,   6144,
                                8192,   12288,  16384,  24576,  32768,  49152,  65536,  98304,  131072, 196608, 262144, 327680, 393216,
                                458752, 524288, 589824, 655360, 720896, 786432, 851968, 917504, 983040, 1048576};
const qint32 SQX_D48EXTRA[49] = {0, 0, 0, 0, 1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8, 8, 9, 9, 10, 10, 11,
                                 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 0};
// Distance base/extra, dictionary == 2 MiB.
const qint32 SQX_D50BASE[51] = {0,       1,       2,       3,       4,       6,       8,       12,      16,      24,      32,     48,   64,
                                96,      128,     192,     256,     384,     512,     768,     1024,    1536,    2048,    3072,   4096, 6144,
                                8192,    12288,   16384,   24576,   32768,   49152,   65536,   98304,   131072,  196608,  262144, 393216,
                                524288,  655360,  786432,  917504,  1048576, 1179648, 1310720, 1441792, 1572864, 1703936, 1835008, 1966080,
                                2097152};
const qint32 SQX_D50EXTRA[51] = {0, 0, 0, 0, 1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  9, 9, 10, 10, 11, 11,
                                 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 0};
// Distance base/extra, dictionary == 4 MiB.
const qint32 SQX_D52BASE[53] = {0,       1,       2,       3,       4,       6,       8,       12,      16,      24,      32,      48,
                                64,      96,      128,     192,     256,     384,     512,     768,     1024,    1536,    2048,    3072,
                                4096,    6144,    8192,    12288,   16384,   24576,   32768,   49152,   65536,   98304,   131072,  196608,
                                262144,  393216,  524288,  786432,  1048576, 1310720, 1572864, 1835008, 2097152, 2359296, 2621440, 2883584,
                                3145728, 3407872, 3670016, 3932160, 4194304};
const qint32 SQX_D52EXTRA[53] = {0, 0, 0, 0, 1,  1,  2,  2,  3,  3,  4,  4,  5,  5,  6,  6,  7,  7,  8,  8,  9,  9, 10, 10, 11, 11, 12,
                                 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 0};

// MSB-first inside little-endian 32-bit words.
class SqxBits {
public:
    SqxBits(const QByteArray &baData);

    bool peek(qint32 nCount, quint32 *pnValue);
    bool get(qint32 nCount, quint32 *pnValue);
    void skip(qint32 nCount);

private:
    QByteArray m_baWords;
    qint64 m_nWordCount;
    qint64 m_nBitOffset;
};

SqxBits::SqxBits(const QByteArray &baData)
{
    const qint32 nSize = baData.size();
    const qint32 nPadded = ((nSize + 3) & ~3) + 16;
    m_baWords = QByteArray(nPadded, (char)0);
    if (nSize > 0) memcpy(m_baWords.data(), baData.constData(), (size_t)nSize);
    m_nWordCount = nPadded / 4;
    m_nBitOffset = 0;
}

bool SqxBits::peek(qint32 nCount, quint32 *pnValue)
{
    const qint64 nWord = m_nBitOffset >> 5;
    const qint32 nShift = (qint32)(m_nBitOffset & 31);
    if ((nWord < 0) || ((nWord + 1) >= m_nWordCount)) return false;
    const quint8 *pData = (const quint8 *)m_baWords.constData();
    quint32 nAccumulator = (quint32)(qFromLittleEndian<quint32>(pData + nWord * 4) << nShift);
    if (nShift) nAccumulator |= (quint32)(qFromLittleEndian<quint32>(pData + nWord * 4 + 4) >> (32 - nShift));
    *pnValue = nAccumulator >> ((32 - nCount) & 31);

    return true;
}

void SqxBits::skip(qint32 nCount)
{
    m_nBitOffset += nCount;
}

bool SqxBits::get(qint32 nCount, quint32 *pnValue)
{
    if (!peek(nCount, pnValue)) return false;
    m_nBitOffset += nCount;

    return true;
}

class SqxHuff {
public:
    SqxHuff();

    void init(qint32 nCount, qint32 nMaxLen);
    bool build();
    bool decode(SqxBits *pBits, qint32 *pnSymbol);
    void setLength(qint32 nIndex, qint32 nLength);
    qint32 getCount() const;

    QVector<qint32> m_listLengths;

private:
    qint32 m_nCount;
    qint32 m_nMaxLen;
    QVector<quint16> m_listLut;
};

SqxHuff::SqxHuff()
{
    m_nCount = 0;
    m_nMaxLen = 0;
}

void SqxHuff::init(qint32 nCount, qint32 nMaxLen)
{
    m_nCount = nCount;
    m_nMaxLen = nMaxLen;
    m_listLengths.fill(0, nCount);
    m_listLut.fill(0, 1 << nMaxLen);
}

qint32 SqxHuff::getCount() const
{
    return m_nCount;
}

void SqxHuff::setLength(qint32 nIndex, qint32 nLength)
{
    if ((nIndex >= 0) && (nIndex < m_listLengths.size())) m_listLengths[nIndex] = nLength;
}

bool SqxHuff::build()
{
    qint32 nCounts[17];
    qint32 nStart[18];
    qint32 nStep[18];
    memset(nCounts, 0, sizeof(nCounts));
    memset(nStart, 0, sizeof(nStart));
    memset(nStep, 0, sizeof(nStep));

    for (qint32 i = 0; i < m_nCount; i++) {
        const qint32 nLength = m_listLengths.at(i);
        if ((nLength < 0) || (nLength > 16)) return false;
        nCounts[nLength]++;
    }
    // The running-start arithmetic is deliberately 16-bit, as in the reference.
    for (qint32 nLength = 1; nLength <= m_nMaxLen; nLength++) {
        nStart[nLength + 1] = (nStart[nLength] + (nCounts[nLength] << (m_nMaxLen - nLength))) & 0xffff;
    }
    if (nStart[m_nMaxLen + 1] != ((1 << m_nMaxLen) & 0xffff)) return false;

    for (qint32 nLength = 1; nLength <= m_nMaxLen; nLength++) {
        nStep[nLength] = 1 << (m_nMaxLen - nLength);
    }

    m_listLut.fill(0, 1 << m_nMaxLen);
    quint16 *pLut = m_listLut.data();
    for (qint32 nSymbol = 0; nSymbol < m_nCount; nSymbol++) {
        const qint32 nLength = m_listLengths.at(nSymbol);
        if (!nLength) continue;
        const qint32 nFrom = nStart[nLength];
        const qint32 nTo = nFrom + nStep[nLength];
        if (nTo > (1 << m_nMaxLen)) return false;
        for (qint32 k = nFrom; k < nTo; k++) pLut[k] = (quint16)nSymbol;
        nStart[nLength] = nTo;
    }

    return true;
}

bool SqxHuff::decode(SqxBits *pBits, qint32 *pnSymbol)
{
    quint32 nIndex = 0;
    if (!pBits->peek(m_nMaxLen, &nIndex)) return false;
    if ((qint64)nIndex >= (qint64)m_listLut.size()) return false;
    const qint32 nSymbol = m_listLut.at((qint32)nIndex);
    if (nSymbol >= m_nCount) return false;
    const qint32 nLength = m_listLengths.at(nSymbol);
    if (nLength == 0) return false;
    pBits->skip(nLength);
    *pnSymbol = nSymbol;

    return true;
}

// Deflate-style RLE over the 19-symbol pre-table.
bool sqxReadLengthsPre(SqxBits *pBits, SqxHuff *pPre, SqxHuff *pTable)
{
    const qint32 nCount = pTable->getCount();
    qint32 i = 0;
    while (i < nCount) {
        qint32 nSymbol = 0;
        if (!pPre->decode(pBits, &nSymbol)) return false;
        if (nSymbol < 0x10) {
            pTable->m_listLengths[i] = nSymbol;
            i++;
        } else if (nSymbol == 0x10) {
            quint32 nExtra = 0;
            if (!pBits->get(2, &nExtra)) return false;
            qint32 nRepeat = (qint32)nExtra + 3;
            if (i == 0) return false;
            while ((i < nCount) && nRepeat) {
                pTable->m_listLengths[i] = pTable->m_listLengths.at(i - 1);
                i++;
                nRepeat--;
            }
        } else {
            quint32 nExtra = 0;
            qint32 nRepeat = 0;
            if (nSymbol == 0x11) {
                if (!pBits->get(3, &nExtra)) return false;
                nRepeat = (qint32)nExtra + 3;
            } else {
                if (!pBits->get(7, &nExtra)) return false;
                nRepeat = (qint32)nExtra + 11;
            }
            while ((i < nCount) && nRepeat) {
                pTable->m_listLengths[i] = 0;
                i++;
                nRepeat--;
            }
        }
    }

    return pTable->build();
}

// 0 same, 10 +1, 111 abs4, 1100 -1, 1101 +2.
bool sqxReadLengthsDelta(SqxBits *pBits, SqxHuff *pTable)
{
    const qint32 nCount = pTable->getCount();
    quint32 nBit = 0;
    quint32 nValue = 0;
    if (!pBits->get(4, &nValue)) return false;
    qint32 nCurrent = (qint32)nValue;
    pTable->m_listLengths[0] = nCurrent;

    for (qint32 i = 1; i < nCount; i++) {
        if (!pBits->get(1, &nBit)) return false;
        if (nBit == 0) {
            pTable->m_listLengths[i] = nCurrent;
            continue;
        }
        if (!pBits->get(1, &nBit)) return false;
        if (nBit == 0) {
            nCurrent += 1;
            pTable->m_listLengths[i] = nCurrent;
            continue;
        }
        if (!pBits->get(1, &nBit)) return false;
        if (nBit == 1) {
            if (!pBits->get(4, &nValue)) return false;
            nCurrent = (qint32)nValue;
            pTable->m_listLengths[i] = nCurrent;
            continue;
        }
        if (!pBits->get(1, &nBit)) return false;
        if (nBit == 0) {
            nCurrent -= 1;
            if (nCurrent < 0) return false;
        } else {
            nCurrent += 2;
        }
        pTable->m_listLengths[i] = nCurrent;
    }

    return pTable->build();
}

class SqxState {
public:
    SqxState();

    bool init();
    bool unpackMember(const QByteArray &baPacked, qint64 nUncompressedSize, quint16 nFlags, quint8 nFilter, QByteArray *pbaOut,
                      XBinary::PDSTRUCT *pPdStruct);

private:
    bool readBlockHeader(SqxBits *pBits, qint32 *pnCount, qint32 *pnMode, qint32 *pnThreshold);
    void put(quint8 nByte);
    void copy(qint64 nLength, qint32 nDistance);

    QByteArray m_baWindow;
    quint8 *m_pWindow;
    qint32 m_nDictSize;
    qint32 m_nPosition;
    qint64 m_nLastLength;
    qint32 m_nLastDistance;
    quint32 m_nRepIndex;
    qint32 m_nRep[4];
    QByteArray *m_pbaOut;
    qint64 m_nProduced;
    SqxHuff m_pre;
    SqxHuff m_tableA;
    SqxHuff m_tableB;
    SqxHuff m_tableLength;
    SqxHuff m_tableDistance;
};

SqxState::SqxState()
{
    m_pWindow = nullptr;
    m_nDictSize = 0;
    m_nPosition = 0;
    m_nLastLength = 0;
    m_nLastDistance = 0;
    m_nRepIndex = 0;
    m_nRep[0] = 0;
    m_nRep[1] = 0;
    m_nRep[2] = 0;
    m_nRep[3] = 0;
    m_pbaOut = nullptr;
    m_nProduced = 0;
}

// A replayed member only has to rebuild the window, so pbaOut may be null and
// the bytes are then counted rather than kept.
void SqxState::put(quint8 nByte)
{
    m_pWindow[m_nPosition] = nByte;
    if (m_pbaOut) m_pbaOut->append((char)nByte);
    m_nProduced++;
    m_nPosition++;
    if (m_nPosition >= m_nDictSize) m_nPosition = 0;
}

void SqxState::copy(qint64 nLength, qint32 nDistance)
{
    m_nRep[m_nRepIndex & 3] = nDistance;
    m_nRepIndex++;
    const qint32 nMask = m_nDictSize - 1;
    qint32 nPosition = m_nPosition;
    for (qint64 i = 0; i < nLength; i++) {
        const quint8 nByte = m_pWindow[(nPosition - nDistance) & nMask];
        m_pWindow[nPosition] = nByte;
        if (m_pbaOut) m_pbaOut->append((char)nByte);
        nPosition++;
        if (nPosition >= m_nDictSize) nPosition = 0;
    }
    m_nPosition = nPosition;
    m_nProduced += nLength;
}

bool SqxState::readBlockHeader(SqxBits *pBits, qint32 *pnCount, qint32 *pnMode, qint32 *pnThreshold)
{
    quint32 nBit = 0;
    quint32 nValue = 0;
    if (!pBits->get(1, &nBit)) return false;
    // Kind 1 is the second ("direct") coder; it is not implemented and the
    // member fails rather than producing plausible garbage.
    if (nBit != 0) return false;

    if (!pBits->get(15, &nValue)) return false;
    *pnCount = (qint32)nValue;
    if (!pBits->get(1, &nBit)) return false;
    *pnMode = (qint32)nBit;

    if (*pnMode == 1) {
        if (!pBits->get(9, &nValue)) return false;
        *pnThreshold = (qint32)nValue;
        for (qint32 i = 0; i < SQX_NPRE; i++) {
            if (!pBits->get(4, &nValue)) return false;
            m_pre.setLength(i, (qint32)nValue);
        }
        if (!m_pre.build()) return false;
        if (!sqxReadLengthsPre(pBits, &m_pre, &m_tableA)) return false;
        if (!sqxReadLengthsPre(pBits, &m_pre, &m_tableB)) return false;
        if (!sqxReadLengthsDelta(pBits, &m_tableLength)) return false;
        if (!sqxReadLengthsDelta(pBits, &m_tableDistance)) return false;

        return true;
    }

    *pnThreshold = 0;
    if (!pBits->get(1, &nBit)) return false;
    if (nBit == 0) {
        for (qint32 i = 0; i < SQX_NPRE; i++) {
            if (!pBits->get(4, &nValue)) return false;
            m_pre.setLength(i, (qint32)nValue);
        }
        if (!m_pre.build()) return false;
        if (!sqxReadLengthsPre(pBits, &m_pre, &m_tableA)) return false;
        if (!sqxReadLengthsPre(pBits, &m_pre, &m_tableLength)) return false;
        if (!sqxReadLengthsPre(pBits, &m_pre, &m_tableDistance)) return false;

        return true;
    }

    if (!sqxReadLengthsDelta(pBits, &m_tableA)) return false;
    if (!sqxReadLengthsDelta(pBits, &m_tableLength)) return false;
    if (!sqxReadLengthsDelta(pBits, &m_tableDistance)) return false;

    return true;
}

// One 4 MiB window serves every member of the archive; only the cursor and the
// dictionary size move, exactly as in the reference.
bool SqxState::init()
{
    m_baWindow = QByteArray(SQX_MAX_DICT, (char)0);
    if (m_baWindow.size() != SQX_MAX_DICT) return false;
    m_pWindow = (quint8 *)m_baWindow.data();
    m_nDictSize = SQX_MAX_DICT;
    m_nPosition = 0;
    m_nLastLength = 0;
    m_nLastDistance = 0;
    m_nRepIndex = 0;
    m_nRep[0] = 0;
    m_nRep[1] = 0;
    m_nRep[2] = 0;
    m_nRep[3] = 0;

    m_pre.init(SQX_NPRE, SQX_MAXLEN_PRE);
    m_tableA.init(SQX_NMAIN, SQX_MAXLEN);
    m_tableB.init(SQX_NMAIN, SQX_MAXLEN);
    m_tableLength.init(SQX_NLEN, SQX_MAXLEN);
    m_tableDistance.init(SQX_NDIST, SQX_MAXLEN);

    return true;
}

bool SqxState::unpackMember(const QByteArray &baPacked, qint64 nUncompressedSize, quint16 nFlags, quint8 nFilter, QByteArray *pbaOut,
                            XBinary::PDSTRUCT *pPdStruct)
{
    if (!m_pWindow) return false;

    qint32 nDictSize = 1 << ((((qint32)nFlags >> 8) & 0xf) + 15);
    if (nDictSize > SQX_MAX_DICT) nDictSize = SQX_MAX_DICT;
    m_nDictSize = nDictSize;
    m_pbaOut = pbaOut;
    m_nProduced = 0;

    // The reference applies the non-solid reset BEFORE it gives up on the
    // filter, so the side effect happens either way.
    if (!(nFlags & 4)) {
        m_nLastLength = 0;
        m_nLastDistance = 0;
        m_nRepIndex = 0;
        m_nRep[0] = 0;
        m_nRep[1] = 0;
        m_nRep[2] = 0;
        m_nRep[3] = 0;
        m_nPosition = 0;
    }
    // No clamp here on purpose: when a solid member inherits a cursor past the
    // new dictionary size the reference writes one byte at that position - the
    // window is a full 4 MiB - and only then wraps to zero.

    // The b0 preprocessor filter is not implemented.
    if (nFilter) return false;

    const qint32 *pDistanceBase = SQX_D48BASE;
    const qint32 *pDistanceExtra = SQX_D48EXTRA;
    qint32 nDistanceCount = 49;
    const qint32 nDictKB = nDictSize >> 10;
    if (nDictKB == 0x800) {
        pDistanceBase = SQX_D50BASE;
        pDistanceExtra = SQX_D50EXTRA;
        nDistanceCount = 51;
    } else if (nDictKB == 0x1000) {
        pDistanceBase = SQX_D52BASE;
        pDistanceExtra = SQX_D52EXTRA;
        nDistanceCount = 53;
    }

    SqxBits bits(baPacked);
    const qint64 nOutputCap = nUncompressedSize + SQX_OVERSHOOT;
    qint64 nRemaining = nUncompressedSize;
    qint32 nCount = 0;
    qint32 nMode = 0;
    qint32 nThreshold = 0;
    qint32 nPrevious = 0;
    qint64 nIterations = 0;

    while (nRemaining >= 1) {
        nIterations++;
        if ((nIterations % SQX_CANCEL_STEP) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }
        if (m_nProduced > nOutputCap) return false;

        if (nCount == 0) {
            if (!readBlockHeader(&bits, &nCount, &nMode, &nThreshold)) return false;
            nPrevious = nThreshold;
        }

        qint32 nSymbol = 0;
        if (nMode == 0) {
            if (!m_tableA.decode(&bits, &nSymbol)) return false;
        } else if (nPrevious < (nThreshold + 1)) {
            if (!m_tableA.decode(&bits, &nSymbol)) return false;
            nPrevious = nSymbol;
        } else {
            if (!m_tableB.decode(&bits, &nSymbol)) return false;
            nPrevious = nSymbol;
        }
        nCount--;

        quint32 nExtra = 0;
        if (nSymbol < 0x100) {
            put((quint8)nSymbol);
            nRemaining -= 1;
        } else if (nSymbol == 0x100) {
            // A repeat of the previous match before any match exists would
            // spin forever; the reference stream never does it.
            if (m_nLastLength <= 0) return false;
            copy(m_nLastLength, m_nLastDistance);
            nRemaining -= m_nLastLength;
        } else if (nSymbol < 0x105) {
            m_nLastDistance = m_nRep[(m_nRepIndex - (quint32)(nSymbol - 0x100)) & 3];
            qint32 nLengthSymbol = 0;
            if (!m_tableLength.decode(&bits, &nLengthSymbol)) return false;
            if (nLengthSymbol == 0x18) {
                if (!bits.get(14, &nExtra)) return false;
                m_nLastLength = (qint64)nExtra + 0x101;
            } else {
                if ((nLengthSymbol < 0) || (nLengthSymbol >= 26)) return false;
                qint64 nLength = SQX_LENBASE[nLengthSymbol];
                if (SQX_LENEXTRA[nLengthSymbol]) {
                    if (!bits.get(SQX_LENEXTRA[nLengthSymbol], &nExtra)) return false;
                    nLength += nExtra;
                }
                nLength += 2;
                if (m_nLastDistance > 0x3fff) nLength += 1;
                if (m_nLastDistance > 0x3ffff) nLength += 1;
                m_nLastLength = nLength;
            }
            nRemaining -= m_nLastLength;
            copy(m_nLastLength, m_nLastDistance);
        } else if (nSymbol < 0x10d) {
            const qint32 i = nSymbol - 0x105;
            qint32 nDistance = SQX_L2BASE[i];
            if (SQX_L2EXTRA[i]) {
                if (!bits.get(SQX_L2EXTRA[i], &nExtra)) return false;
                nDistance += (qint32)nExtra;
            }
            nRemaining -= 2;
            copy(2, nDistance + 1);
        } else if (nSymbol < 0x11c) {
            const qint32 i = nSymbol - 0x10d;
            qint32 nDistance = SQX_L3BASE[i];
            if (SQX_L3EXTRA[i]) {
                if (!bits.get(SQX_L3EXTRA[i], &nExtra)) return false;
                nDistance += (qint32)nExtra;
            }
            nRemaining -= 3;
            copy(3, nDistance + 1);
        } else {
            const qint32 i = nSymbol - 0x11c;
            qint64 nLength = 0;
            if (nSymbol == 0x134) {
                if (!bits.get(14, &nExtra)) return false;
                nLength = (qint64)nExtra + 0x101;
            } else {
                if ((i < 0) || (i >= 26)) return false;
                nLength = SQX_LENBASE[i];
                if (SQX_LENEXTRA[i]) {
                    if (!bits.get(SQX_LENEXTRA[i], &nExtra)) return false;
                    nLength += nExtra;
                }
                nLength += 4;
            }
            qint32 nDistanceSymbol = 0;
            if (!m_tableDistance.decode(&bits, &nDistanceSymbol)) return false;
            if ((nDistanceSymbol < 0) || (nDistanceSymbol >= nDistanceCount)) return false;
            qint64 nDistance = pDistanceBase[nDistanceSymbol];
            if (pDistanceExtra[nDistanceSymbol]) {
                if (!bits.get(pDistanceExtra[nDistanceSymbol], &nExtra)) return false;
                nDistance += nExtra;
            }
            nDistance += 1;
            if ((nSymbol != 0x134) && (nDistance > 0x3ffff)) nLength += 1;
            if (nDistance > 0x7fffffff) return false;
            nRemaining -= nLength;
            copy(nLength, (qint32)nDistance);
        }
    }

    return true;
}
}  // namespace

const qint64 XSQXDecoder::MAX_UNCOMPRESSED_SIZE;
const qint32 XSQXDecoder::ENTRY_SIZE;
const qint32 XSQXDecoder::TABLE_HEADER_SIZE;

QByteArray XSQXDecoder::startTable()
{
    QByteArray baTable(TABLE_HEADER_SIZE, (char)0);
    quint8 *pData = (quint8 *)baTable.data();
    qToLittleEndian<quint32>(SQX_TABLE_MAGIC, pData);

    return baTable;
}

bool XSQXDecoder::appendEntry(QByteArray *pbaTable, qint64 nDataOffset, qint64 nPackedSize, qint64 nUncompressedSize, quint16 nFlags, quint8 nFilter,
                              quint8 nMethod)
{
    if (!pbaTable) return false;
    if (pbaTable->size() < TABLE_HEADER_SIZE) return false;
    if ((nDataOffset < 0) || (nPackedSize < 0) || (nUncompressedSize < 0)) return false;
    if (pbaTable->size() > ((qint32)0x7fffffff - ENTRY_SIZE)) return false;

    QByteArray baEntry(ENTRY_SIZE, (char)0);
    quint8 *pEntry = (quint8 *)baEntry.data();
    qToLittleEndian<quint64>((quint64)nDataOffset, pEntry);
    qToLittleEndian<quint64>((quint64)nPackedSize, pEntry + 8);
    qToLittleEndian<quint64>((quint64)nUncompressedSize, pEntry + 16);
    qToLittleEndian<quint16>(nFlags, pEntry + 24);
    pEntry[26] = nFilter;
    pEntry[27] = nMethod;
    pbaTable->append(baEntry);

    quint8 *pData = (quint8 *)pbaTable->data();
    qToLittleEndian<quint32>((quint32)((pbaTable->size() - TABLE_HEADER_SIZE) / ENTRY_SIZE), pData + 4);

    return true;
}

QByteArray XSQXDecoder::finishTable(const QByteArray &baTable, qint32 nTargetIndex)
{
    QByteArray baResult(baTable);
    if (baResult.size() < TABLE_HEADER_SIZE) return QByteArray();
    quint8 *pData = (quint8 *)baResult.data();
    qToLittleEndian<quint32>((quint32)nTargetIndex, pData + 8);

    return baResult;
}

bool XSQXDecoder::decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                         XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;
    if (baProperties.size() < TABLE_HEADER_SIZE) return false;

    const quint8 *pTable = (const quint8 *)baProperties.constData();
    if (qFromLittleEndian<quint32>(pTable) != SQX_TABLE_MAGIC) return false;
    const qint64 nEntryCount = (qint64)qFromLittleEndian<quint32>(pTable + 4);
    const qint64 nTargetIndex = (qint64)qFromLittleEndian<quint32>(pTable + 8);
    if ((nEntryCount <= 0) || (nTargetIndex < 0) || (nTargetIndex >= nEntryCount)) return false;
    if (baProperties.size() < (TABLE_HEADER_SIZE + nEntryCount * ENTRY_SIZE)) return false;

    SqxState state;
    if (!state.init()) return false;

    QByteArray baOut;
    qint64 nReplayed = 0;

    for (qint64 i = 0; i <= nTargetIndex; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const quint8 *pEntry = pTable + TABLE_HEADER_SIZE + i * ENTRY_SIZE;
        const qint64 nDataOffset = (qint64)qFromLittleEndian<quint64>(pEntry);
        const qint64 nPackedSize = (qint64)qFromLittleEndian<quint64>(pEntry + 8);
        const qint64 nMemberSize = (qint64)qFromLittleEndian<quint64>(pEntry + 16);
        const quint16 nFlags = qFromLittleEndian<quint16>(pEntry + 24);
        const quint8 nFilter = pEntry[26];
        const quint8 nMethod = pEntry[27];

        if ((nDataOffset < 0) || (nPackedSize < 0) || (nMemberSize < 0) || (nMemberSize > MAX_UNCOMPRESSED_SIZE)) return false;
        if ((nDataOffset > baPacked.size()) || (nPackedSize > ((qint64)baPacked.size() - nDataOffset))) return false;
        // Methods 5 and up use a different coder entirely.
        if ((nMethod < 1) || (nMethod > 4)) return false;

        nReplayed += nMemberSize;
        if (nReplayed > (MAX_UNCOMPRESSED_SIZE * 4)) return false;

        const bool bIsTarget = (i == nTargetIndex);
        const QByteArray baMember = baPacked.mid((qint32)nDataOffset, (qint32)nPackedSize);
        QByteArray baMemberOut;
        // Members in front of the target only have to rebuild the window, so
        // their bytes are never materialised.
        if (bIsTarget) baMemberOut.reserve((qint32)qMin<qint64>(nMemberSize + 1, 0x400000));
        // A failing member is not fatal to the replay: the reference keeps the
        // state as the failure left it and moves on.  Only the target member's
        // own result has to be complete.
        const bool bOk = state.unpackMember(baMember, nMemberSize, nFlags, nFilter, bIsTarget ? &baMemberOut : nullptr, pPdStruct);
        if (bIsTarget) {
            if (!bOk) return false;
            if ((qint64)baMemberOut.size() < nMemberSize) return false;
            if (nMemberSize != nUncompressedSize) return false;
            baOut = baMemberOut;
        }
    }

    baOut.truncate((qint32)nUncompressedSize);
    if ((qint64)baOut.size() != nUncompressedSize) return false;
    *pbaResult = baOut;

    return true;
}
