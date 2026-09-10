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
#include "xteledeskdecoder.h"

#include <new>
#include <string.h>

namespace {

const qint64 TELEDESK_MAX_IMAGE_SIZE = Q_INT64_C(64) * 1024 * 1024;
const qint32 TELEDESK_MAX_TRACKS = 1 << 20;
const qint32 TELEDESK_MAX_SECTOR_SIZE = 0x4000;

// TeleDisk CRC16: poly 0xA097, MSB first, init 0, no reflection, no final xor.
struct TeleDeskCrcTable {
    quint16 nTable[256];

    TeleDeskCrcTable()
    {
        for (qint32 i = 0; i < 256; i++) {
            quint16 nValue = (quint16)(i << 8);
            for (qint32 j = 0; j < 8; j++) {
                if (nValue & 0x8000) {
                    nValue = (quint16)((nValue << 1) ^ 0xA097);
                } else {
                    nValue = (quint16)(nValue << 1);
                }
            }
            nTable[i] = nValue;
        }
    }
};

const TeleDeskCrcTable &teleDeskCrcTable()
{
    static const TeleDeskCrcTable table;
    return table;
}

// Every reader hands back EXACTLY nSize bytes or fails; a short read is the end
// of the archive as far as the image builder is concerned.
class TeleDeskReader {
public:
    TeleDeskReader()
    {
    }
    virtual ~TeleDeskReader()
    {
    }

    virtual bool read(qint32 nSize, QByteArray *pbaResult) = 0;

private:
    TeleDeskReader(const TeleDeskReader &);
    TeleDeskReader &operator=(const TeleDeskReader &);
};

// Signature "TD": the payload is stored verbatim.
class TeleDeskRawReader : public TeleDeskReader {
public:
    TeleDeskRawReader(const QByteArray &baData, qint64 nPosition) : m_baData(baData), m_nPosition(nPosition)
    {
    }

    bool read(qint32 nSize, QByteArray *pbaResult) override
    {
        if (!pbaResult || (nSize < 0)) return false;
        if ((m_nPosition < 0) || (nSize > (m_baData.size() - m_nPosition))) return false;
        *pbaResult = m_baData.mid((qint32)m_nPosition, nSize);
        if (pbaResult->size() != nSize) return false;
        m_nPosition += nSize;
        return true;
    }

private:
    QByteArray m_baData;
    qint64 m_nPosition;
};

// Signature "td", version 10..19: blocked 12-bit LSB-first LZW.
class TeleDeskLzwReader : public TeleDeskReader {
public:
    TeleDeskLzwReader(const QByteArray &baData, qint64 nPosition)
        : m_baData(baData), m_nPosition(nPosition), m_nAccumulator(0), m_nBitCount(0), m_nLeft(0), m_nNext(0), m_nStackPointer(0), m_nLast(0), m_nPrevious(0),
          m_bDead(false)
    {
        memset(m_nPrefix, 0, sizeof(m_nPrefix));
        memset(m_nSuffix, 0, sizeof(m_nSuffix));
        memset(m_nStack, 0, sizeof(m_nStack));
    }

    bool read(qint32 nSize, QByteArray *pbaResult) override
    {
        if (!pbaResult || (nSize < 0)) return false;
        pbaResult->clear();
        if (nSize == 0) return true;
        if (m_bDead) return false;

        while (pbaResult->size() < nSize) {
            if (m_nStackPointer > 0) {
                while ((m_nStackPointer > 0) && (pbaResult->size() < nSize)) {
                    m_nStackPointer--;
                    pbaResult->append((char)m_nStack[m_nStackPointer]);
                }
                if (pbaResult->size() >= nSize) break;
            }

            if (m_nLeft < 1) {
                qint32 nLow = nextByte();
                qint32 nHigh = nextByte();
                if ((nLow < 0) || (nHigh < 0)) return fail();
                m_nLeft = nLow | (nHigh << 8);
                if (m_nLeft > 0x3000) return fail();
                for (qint32 i = 0; i < 256; i++) {
                    m_nPrefix[i] = 0;
                    m_nSuffix[i] = (quint8)i;
                }
                m_nNext = 0x100;
                const qint32 nCode = nextCode();
                if (nCode < 0) return fail();
                m_nLeft -= 3;
                m_nPrevious = nCode;
                m_nLast = nCode;
                pbaResult->append((char)(quint8)(nCode & 0xFF));
                m_nStackPointer = 0;
                continue;
            }

            const qint32 nCode = nextCode();
            if (nCode < 0) return fail();
            m_nLeft -= 3;
            const qint32 nCurrent = nCode;
            qint32 nWalk = nCode;
            if (nWalk >= m_nNext) {
                if (m_nStackPointer > 0xFFF) return fail();
                m_nStack[m_nStackPointer] = (quint8)(m_nLast & 0xFF);
                m_nStackPointer++;
                nWalk = m_nPrevious;
            }
            while (nWalk > 0xFF) {
                if ((m_nStackPointer > 0xFFF) || (nWalk > 0xFFF)) return fail();
                m_nStack[m_nStackPointer] = m_nSuffix[nWalk];
                m_nStackPointer++;
                nWalk = m_nPrefix[nWalk];
            }
            if (nWalk < 0) return fail();
            m_nLast = nWalk;
            if (m_nStackPointer > 0xFFF) return fail();
            m_nStack[m_nStackPointer] = (quint8)(nWalk & 0xFF);
            m_nStackPointer++;
            while ((m_nStackPointer > 0) && (pbaResult->size() < nSize)) {
                m_nStackPointer--;
                pbaResult->append((char)m_nStack[m_nStackPointer]);
            }
            if (m_nNext < 4096) {
                m_nPrefix[m_nNext] = (quint16)(m_nPrevious & 0xFFFF);
                m_nSuffix[m_nNext] = (quint8)(m_nLast & 0xFF);
                m_nNext++;
            }
            m_nPrevious = nCurrent;
        }

        return (pbaResult->size() == nSize);
    }

private:
    bool fail()
    {
        m_bDead = true;
        return false;
    }

    qint32 nextByte()
    {
        if ((m_nPosition < 0) || (m_nPosition >= m_baData.size())) return -1;
        const qint32 nResult = (quint8)m_baData.at((qint32)m_nPosition);
        m_nPosition++;
        return nResult;
    }

    qint32 nextCode()
    {
        while (m_nBitCount <= 11) {
            const qint32 nByte = nextByte();
            if (nByte < 0) return -1;
            m_nAccumulator |= ((quint32)nByte) << m_nBitCount;
            m_nBitCount += 8;
        }
        const qint32 nResult = (qint32)(m_nAccumulator & 0xFFF);
        m_nAccumulator >>= 12;
        m_nBitCount -= 12;
        return nResult;
    }

    QByteArray m_baData;
    qint64 m_nPosition;
    quint32 m_nAccumulator;
    qint32 m_nBitCount;
    qint32 m_nLeft;
    qint32 m_nNext;
    quint16 m_nPrefix[4096];
    quint8 m_nSuffix[4096];
    quint8 m_nStack[4096];
    qint32 m_nStackPointer;
    qint32 m_nLast;
    qint32 m_nPrevious;
    bool m_bDead;
};

// Signature "td", version 20..21: Okumura LZHUF over ONE stream for the whole
// archive, produced one literal or one whole match at a time.
class TeleDeskLzhufReader : public TeleDeskReader {
public:
    static const qint32 N = 4096;
    static const qint32 F = 60;
    static const qint32 THRESHOLD = 2;
    static const qint32 N_CHAR = 256 - THRESHOLD + F;  // 314
    static const qint32 T = N_CHAR * 2 - 1;            // 627
    static const qint32 R = T - 1;                     // 626
    static const qint32 MAX_FREQ = 0x8000;

    TeleDeskLzhufReader(const QByteArray &baData, qint64 nPosition)
        : m_baData(baData), m_nPosition(nPosition), m_nBitBuffer(0), m_nBitCount(0), m_bEof(false), m_bDead(false), m_nRing(N - F), m_nPending(0)
    {
        buildPositionTables();
        startHuff();
        memset(m_nText, 0x20, sizeof(m_nText));
    }

    bool read(qint32 nSize, QByteArray *pbaResult) override
    {
        if (!pbaResult || (nSize < 0)) return false;
        pbaResult->clear();
        if (nSize == 0) return true;
        if (m_bDead) return false;

        while (m_nPending < nSize) {
            if (!step()) {
                m_bDead = true;
                return false;
            }
        }
        *pbaResult = m_baPending.left(nSize);
        m_baPending.remove(0, nSize);
        m_nPending -= nSize;

        return (pbaResult->size() == nSize);
    }

private:
    void buildPositionTables()
    {
        // Canonical lzhuf.c d_code[256]/d_len[256]: 1 symbol of length 3, 3 of
        // 4, 8 of 5, 12 of 6, 24 of 7 and 16 of 8 - 64 symbols over 256 slots.
        const qint32 nCounts[6] = {1, 3, 8, 12, 24, 16};
        qint32 nIndex = 0;
        qint32 nSymbol = 0;
        for (qint32 nLength = 3; nLength <= 8; nLength++) {
            const qint32 nSpan = 1 << (8 - nLength);
            for (qint32 i = 0; i < nCounts[nLength - 3]; i++) {
                for (qint32 j = 0; j < nSpan; j++) {
                    if (nIndex >= 256) return;
                    m_nDCode[nIndex] = (quint8)nSymbol;
                    m_nDLen[nIndex] = (quint8)nLength;
                    nIndex++;
                }
                nSymbol++;
            }
        }
    }

    void startHuff()
    {
        for (qint32 i = 0; i < N_CHAR; i++) {
            m_nFreq[i] = 1;
            m_nSon[i] = i + T;
            m_nPrnt[i + T] = i;
        }
        qint32 i = 0;
        qint32 j = N_CHAR;
        while (j <= R) {
            m_nFreq[j] = m_nFreq[i] + m_nFreq[i + 1];
            m_nSon[j] = i;
            m_nPrnt[i] = j;
            m_nPrnt[i + 1] = j;
            i += 2;
            j++;
        }
        m_nFreq[T] = 0xFFFF;
        m_nPrnt[R] = 0;
    }

    void reconst()
    {
        qint32 j = 0;
        for (qint32 i = 0; i < T; i++) {
            if (m_nSon[i] >= T) {
                m_nFreq[j] = (m_nFreq[i] + 1) / 2;
                m_nSon[j] = m_nSon[i];
                j++;
            }
        }
        qint32 i = 0;
        for (qint32 k = N_CHAR; k < T; k++) {
            const qint32 f = m_nFreq[i] + m_nFreq[i + 1];
            m_nFreq[k] = f;
            qint32 l = k - 1;
            while ((l >= 0) && (f < m_nFreq[l])) l--;
            l++;
            for (qint32 m = k; m > l; m--) {
                m_nFreq[m] = m_nFreq[m - 1];
                m_nSon[m] = m_nSon[m - 1];
            }
            m_nFreq[l] = f;
            m_nSon[l] = i;
            i += 2;
        }
        for (qint32 n = 0; n < T; n++) {
            const qint32 k = m_nSon[n];
            if (k >= T) {
                m_nPrnt[k] = n;
            } else {
                m_nPrnt[k] = n;
                m_nPrnt[k + 1] = n;
            }
        }
    }

    void update(qint32 c)
    {
        if (m_nFreq[R] == MAX_FREQ) reconst();
        c = m_nPrnt[c + T];
        do {
            m_nFreq[c]++;
            const qint32 k = m_nFreq[c];
            qint32 l = c + 1;
            if ((l < T) && (k > m_nFreq[l])) {
                while (((l + 1) < T) && (k > m_nFreq[l + 1])) l++;
                m_nFreq[c] = m_nFreq[l];
                m_nFreq[l] = k;
                const qint32 i = m_nSon[c];
                m_nPrnt[i] = l;
                if (i < T) m_nPrnt[i + 1] = l;
                const qint32 j = m_nSon[l];
                m_nSon[l] = i;
                m_nPrnt[j] = c;
                if (j < T) m_nPrnt[j + 1] = c;
                m_nSon[c] = j;
                c = l;
            }
            c = m_nPrnt[c];
        } while (c != 0);
    }

    // A refill past the end of the input is a genuine end of stream, NOT a
    // request for zero padding: TeleDisk relies on that to stop.
    qint32 getBit()
    {
        if (m_nBitCount == 0) {
            if ((m_nPosition < 0) || (m_nPosition >= m_baData.size())) {
                m_bEof = true;
                return 0;
            }
            m_nBitBuffer = (quint8)m_baData.at((qint32)m_nPosition);
            m_nPosition++;
            m_nBitCount = 8;
        }
        m_nBitCount--;
        return (m_nBitBuffer >> m_nBitCount) & 1;
    }

    qint32 peekByte()
    {
        qint32 nValue = 0;
        for (qint32 i = 0; i < 8; i++) {
            nValue = (nValue << 1) | getBit();
            if (m_bEof) return -1;
        }
        return nValue;
    }

    bool step()
    {
        qint32 c = m_nSon[R];
        while (c < T) {
            c += getBit();
            if (m_bEof) return false;
            if ((c < 0) || (c >= T)) return false;
            c = m_nSon[c];
        }
        c -= T;
        if ((c < 0) || (c >= N_CHAR)) return false;
        update(c);

        if (c < 256) {
            appendByte((quint8)c);
            return true;
        }

        const qint32 i = peekByte();
        if (i < 0) return false;
        const qint32 nHigh = ((qint32)m_nDCode[i]) << 6;
        qint32 j = (qint32)m_nDLen[i] - 2;
        qint32 nValue = i;
        while (j > 0) {
            j--;
            nValue = ((nValue << 1) | getBit()) & 0xFFFF;
            if (m_bEof) return false;
        }
        const qint32 nPosition = nHigh | (nValue & 0x3F);
        qint32 nSource = (m_nRing - nPosition - 1) & (N - 1);
        const qint32 nLength = c - 253;  // c - 255 + THRESHOLD
        for (qint32 k = 0; k < nLength; k++) {
            const quint8 nByte = m_nText[nSource];
            nSource = (nSource + 1) & (N - 1);
            appendByte(nByte);
        }

        return true;
    }

    void appendByte(quint8 nByte)
    {
        m_baPending.append((char)nByte);
        m_nPending++;
        m_nText[m_nRing] = nByte;
        m_nRing = (m_nRing + 1) & (N - 1);
    }

    QByteArray m_baData;
    qint64 m_nPosition;
    quint8 m_nBitBuffer;
    qint32 m_nBitCount;
    bool m_bEof;
    bool m_bDead;

    qint32 m_nFreq[T + 1];
    qint32 m_nPrnt[T + N_CHAR];
    qint32 m_nSon[T];
    quint8 m_nDCode[256];
    quint8 m_nDLen[256];
    quint8 m_nText[N];
    qint32 m_nRing;

    QByteArray m_baPending;
    qint32 m_nPending;
};

// Sector data-block codec.  Methods 0..2; the run codings must land exactly on
// the sector size.
bool teleDeskUnpackSector(const QByteArray &baSource, quint8 nMethod, qint32 nSize, QByteArray *pbaResult)
{
    if (!pbaResult || (nSize < 0)) return false;
    pbaResult->clear();
    const qint32 nCount = baSource.size();
    const quint8 *pSource = (const quint8 *)baSource.constData();

    if (nMethod == 0) {
        if (nCount != nSize) return false;
        *pbaResult = baSource;
        return true;
    }

    if (nMethod == 1) {
        if (nCount & 3) return false;
        qint32 nLeft = nSize;
        qint32 i = 0;
        while (i < nCount) {
            const qint32 nRepeat = pSource[i] | (pSource[i + 1] << 8);
            const char nPattern0 = (char)pSource[i + 2];
            const char nPattern1 = (char)pSource[i + 3];
            i += 4;
            if (nRepeat > (nLeft / 2)) return false;
            nLeft -= nRepeat * 2;
            for (qint32 j = 0; j < nRepeat; j++) {
                pbaResult->append(nPattern0);
                pbaResult->append(nPattern1);
            }
        }
        return (nLeft == 0);
    }

    if (nMethod == 2) {
        qint32 nLeft = nSize;
        qint32 i = 0;
        qint32 nRemaining = nCount;
        while (nRemaining > 0) {
            if (nRemaining < 2) return false;
            const qint32 nCode = pSource[i];
            const qint32 nRepeat = pSource[i + 1];
            if (nCode == 0) {
                if (((nRemaining - 2) < nRepeat) || (nLeft < nRepeat)) return false;
                pbaResult->append(baSource.mid(i + 2, nRepeat));
                i += 2 + nRepeat;
                nRemaining -= 2 + nRepeat;
                nLeft -= nRepeat;
            } else {
                const qint32 nBlockLength = nCode * 2;
                if ((nRemaining - 2) < nBlockLength) return false;
                if ((nBlockLength != 0) && (nRepeat > (nLeft / nBlockLength))) return false;
                const QByteArray baBlock = baSource.mid(i + 2, nBlockLength);
                if (baBlock.size() != nBlockLength) return false;
                for (qint32 j = 0; j < nRepeat; j++) pbaResult->append(baBlock);
                nLeft -= nRepeat * nBlockLength;
                i += 2 + nBlockLength;
                nRemaining -= 2 + nBlockLength;
            }
            if (nLeft < 0) return false;
        }
        return (nLeft == 0);
    }

    return false;
}

quint16 teleDeskReadU16(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || (nOffset > (baData.size() - 2))) return 0;
    return (quint16)((quint8)baData.at(nOffset) | (((quint8)baData.at(nOffset + 1)) << 8));
}

}  // namespace

quint16 XTeleDeskDecoder::crc16(const char *pData, qint64 nSize, quint16 nInit)
{
    if (!pData || (nSize < 0)) return nInit;
    const TeleDeskCrcTable &table = teleDeskCrcTable();
    quint16 nResult = nInit;
    for (qint64 i = 0; i < nSize; i++) {
        const quint8 nByte = (quint8)pData[i];
        nResult = (quint16)(table.nTable[((nResult >> 8) ^ nByte) & 0xFF] ^ (quint16)(nResult << 8));
    }

    return nResult;
}

bool XTeleDeskDecoder::isValidHeader(const QByteArray &baHeader)
{
    if (baHeader.size() < HEADER_SIZE) return false;
    const quint8 *pHeader = (const quint8 *)baHeader.constData();
    if (!(((pHeader[0] == 'T') && (pHeader[1] == 'D')) || ((pHeader[0] == 't') && (pHeader[1] == 'd')))) return false;
    if ((pHeader[4] < 10) || (pHeader[4] > 21)) return false;
    if ((pHeader[5] & 0x7F) > 2) return false;
    if (pHeader[6] > 6) return false;
    if ((pHeader[9] != 1) && (pHeader[9] != 2)) return false;
    if (teleDeskReadU16(baHeader, 10) != crc16(baHeader.constData(), 10, 0)) return false;

    return true;
}

bool XTeleDeskDecoder::buildImage(const QByteArray &baFile, QByteArray *pbaImage, INFO *pInfo, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pInfo) return false;
    INFO info = {};
    if (pbaImage) pbaImage->clear();
    if (baFile.size() < HEADER_SIZE) return false;

    const quint8 *pHeader = (const quint8 *)baFile.constData();
    const quint8 nVersion = pHeader[4];
    if ((nVersion < 10) || (nVersion > 21)) return false;
    const bool bCompressed = ((pHeader[0] == 't') && (pHeader[1] == 'd'));
    if (!bCompressed && !((pHeader[0] == 'T') && (pHeader[1] == 'D'))) return false;
    const bool bAdvanced = bCompressed && (nVersion >= 20);

    info.nVersion = nVersion;
    info.bCompressed = bCompressed;
    info.bAdvancedCodec = bAdvanced;

    TeleDeskReader *pReader = nullptr;
    if (!bCompressed) {
        pReader = new (std::nothrow) TeleDeskRawReader(baFile, HEADER_SIZE);
    } else if (bAdvanced) {
        pReader = new (std::nothrow) TeleDeskLzhufReader(baFile, HEADER_SIZE);
    } else {
        pReader = new (std::nothrow) TeleDeskLzwReader(baFile, HEADER_SIZE);
    }
    if (!pReader) return false;

    qint64 nImageSize = 0;
    bool bComplete = false;
    QByteArray baChunk;

    // The comment block is present when bit 7 of the stepping byte is set.
    bool bRunning = true;
    if (pHeader[7] & 0x80) {
        if (!pReader->read(10, &baChunk)) {
            bRunning = false;
        } else {
            const quint16 nCommentCrc = teleDeskReadU16(baChunk, 0);
            const qint32 nCommentLength = (qint32)teleDeskReadU16(baChunk, 2);
            const quint16 nRunning = crc16(baChunk.constData() + 2, 8, 0);
            QByteArray baComment;
            if (!pReader->read(nCommentLength, &baComment)) {
                bRunning = false;
            } else if (nCommentCrc != crc16(baComment.constData(), baComment.size(), nRunning)) {
                bRunning = false;
            }
        }
    }

    qint32 nTrackCount = 0;
    QByteArray baBuffer(TELEDESK_MAX_SECTOR_SIZE, (char)0);
    while (bRunning) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
        if (nTrackCount >= TELEDESK_MAX_TRACKS) break;
        nTrackCount++;

        QByteArray baTrack;
        if (!pReader->read(4, &baTrack)) break;
        const quint8 nSectors = (quint8)baTrack.at(0);
        // The terminator is tested BEFORE the track header CRC.
        if (nSectors == 0xFF) {
            bComplete = true;
            break;
        }
        if ((quint8)(crc16(baTrack.constData(), 3, 0) & 0xFF) != (quint8)baTrack.at(3)) break;
        if (nSectors == 0) continue;

        bool bTrackOk = true;
        for (qint32 i = 0; (i < nSectors) && bTrackOk; i++) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                bTrackOk = false;
                break;
            }
            QByteArray baSector;
            if (!pReader->read(6, &baSector)) {
                bTrackOk = false;
                break;
            }
            const quint8 nSectorNumber = (quint8)baSector.at(2);
            const quint8 nSizeCode = (quint8)baSector.at(3);
            const quint8 nFlags = (quint8)baSector.at(4);
            if (nSizeCode > 7) {
                bTrackOk = false;
                break;
            }
            quint8 nCalculated = (quint8)(crc16(baSector.constData(), 5, 0) & 0xFF);
            const qint32 nSectorSize = 0x80 << nSizeCode;
            // A fresh all-zero 0x4000 scratch per sector: an unread sector is
            // written out as zeroes, and a short sector keeps zeroes in the
            // padding up to 512 bytes.
            memset(baBuffer.data(), 0, TELEDESK_MAX_SECTOR_SIZE);

            if ((nFlags & 0x30) == 0) {
                QByteArray baLength;
                if (!pReader->read(2, &baLength)) {
                    bTrackOk = false;
                    break;
                }
                qint32 nBlockLength = (qint32)teleDeskReadU16(baLength, 0);
                if (nBlockLength == 0) {
                    bTrackOk = false;
                    break;
                }
                nBlockLength--;
                QByteArray baMethod;
                if (!pReader->read(1, &baMethod)) {
                    bTrackOk = false;
                    break;
                }
                QByteArray baPayload;
                if (!pReader->read(nBlockLength, &baPayload)) {
                    bTrackOk = false;
                    break;
                }
                QByteArray baDecoded;
                if (!teleDeskUnpackSector(baPayload, (quint8)baMethod.at(0), nSectorSize, &baDecoded)) {
                    bTrackOk = false;
                    break;
                }
                if (baDecoded.size() != nSectorSize) {
                    bTrackOk = false;
                    break;
                }
                memcpy(baBuffer.data(), baDecoded.constData(), (size_t)nSectorSize);
                nCalculated = (quint8)(crc16(baDecoded.constData(), baDecoded.size(), 0) & 0xFF);
            }

            const qint32 nWriteSize = (nSectorSize >= 0x200) ? nSectorSize : 0x200;
            // Sectors go out IN FILE ORDER, padded to at least 512 bytes; the
            // one nSectors/sector/flag triple below is decoded but not written.
            if (!((nSectors == 0x13) && (nSectorNumber == 0x76) && (nFlags & 0x40))) {
                if (nImageSize > (TELEDESK_MAX_IMAGE_SIZE - nWriteSize)) {
                    bTrackOk = false;
                    break;
                }
                if (pbaImage) pbaImage->append(baBuffer.constData(), nWriteSize);
                nImageSize += nWriteSize;
            }
            if ((quint8)baSector.at(5) != nCalculated) {
                bTrackOk = false;
                break;
            }
        }
        if (!bTrackOk) break;
    }

    delete pReader;

    info.bComplete = bComplete;
    info.nImageSize = nImageSize;
    *pInfo = info;

    return true;
}

bool XTeleDeskDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || (nUncompressedSize < 0) || (nUncompressedSize > TELEDESK_MAX_IMAGE_SIZE)) return false;
    pbaResult->clear();

    INFO info = {};
    if (!buildImage(baPacked, pbaResult, &info, pPdStruct)) return false;

    return ((qint64)pbaResult->size() == nUncompressedSize);
}
