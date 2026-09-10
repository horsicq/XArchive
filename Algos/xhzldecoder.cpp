/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhzldecoder.h"

#include <array>

namespace {

const qint32 HZL_N = 8192;            // ring size, mask 0x1fff
const qint32 HZL_F = 60;              // longest match
const qint32 HZL_THRESHOLD = 2;       // shortest match is THRESHOLD + 1
const qint32 HZL_N_CHAR = 256 + HZL_F - HZL_THRESHOLD;  // 314, no stop code
const qint32 HZL_T = HZL_N_CHAR * 2 - 1;                // 627
const qint32 HZL_R = HZL_T - 1;                         // 626
const qint32 HZL_MAX_FREQ = 0x8000;
// The bit reader keeps a 16-bit look-ahead window, so a well-formed stream can
// legitimately pull a couple of bytes past its declared end.  Anything beyond
// this slack means the stream did not describe the requested plaintext.
const qint64 HZL_TAIL_SLACK = 8;

class HZLBitReader {
public:
    HZLBitReader(const uchar *pData, qint64 nSize)
        : m_pData(pData), m_nSize(nSize)
    {
    }

    bool isOverrun() const { return m_nPosition > (m_nSize + HZL_TAIL_SLACK); }

    qint32 readBit()
    {
        fill();
        const quint32 nValue = m_nBuffer;
        m_nBuffer = (m_nBuffer << 1) & 0xffffffffU;
        --m_nCount;
        return qint32((nValue >> 15) & 1U);
    }

    qint32 readByte()
    {
        fill();
        const quint32 nValue = m_nBuffer;
        m_nBuffer = (m_nBuffer << 8) & 0xffffffffU;
        m_nCount -= 8;
        return qint32((nValue >> 8) & 0xffU);
    }

private:
    void fill()
    {
        while (m_nCount <= 8) {
            quint32 nByte = 0;
            if (m_nPosition < m_nSize) {
                nByte = m_pData[m_nPosition];
            }
            // The counter keeps advancing past the end so isOverrun() can see
            // how far a broken stream tried to read.
            ++m_nPosition;
            m_nBuffer |= nByte << (8 - m_nCount);
            m_nBuffer &= 0xffffffffU;
            m_nCount += 8;
        }
    }

    const uchar *m_pData = nullptr;
    qint64 m_nSize = 0;
    qint64 m_nPosition = 0;
    quint32 m_nBuffer = 0;
    qint32 m_nCount = 0;
};

class HZLHuffman {
public:
    HZLHuffman()
    {
        for (qint32 i = 0; i < HZL_N_CHAR; ++i) {
            m_frequency[i] = 1;
            m_child[i] = i + HZL_T;
            m_parent[i + HZL_T] = i;
        }
        qint32 i = 0;
        qint32 j = HZL_N_CHAR;
        while (j <= HZL_R) {
            m_frequency[j] = m_frequency[i] + m_frequency[i + 1];
            m_child[j] = i;
            m_parent[i] = j;
            m_parent[i + 1] = j;
            i += 2;
            ++j;
        }
        m_frequency[HZL_T] = 0xffff;
        m_parent[HZL_R] = 0;
    }

    qint32 decodeCharacter(HZLBitReader *pReader)
    {
        qint32 nCode = m_child[HZL_R];
        qint32 nGuard = 0;
        while (nCode < HZL_T) {
            if (++nGuard > (HZL_T * 2)) return -1;
            nCode += pReader->readBit();
            if ((nCode < 0) || (nCode >= HZL_T)) return -1;
            nCode = m_child[nCode];
        }
        nCode -= HZL_T;
        if ((nCode < 0) || (nCode >= HZL_N_CHAR)) return -1;
        update(nCode);
        return nCode;
    }

private:
    void reconstruct()
    {
        qint32 j = 0;
        for (qint32 i = 0; i < HZL_T; ++i) {
            if (m_child[i] >= HZL_T) {
                m_frequency[j] = (m_frequency[i] + 1) / 2;
                m_child[j] = m_child[i];
                ++j;
            }
        }
        qint32 i = 0;
        j = HZL_N_CHAR;
        for (; j < HZL_T; i += 2, ++j) {
            const qint32 nSum = m_frequency[i] + m_frequency[i + 1];
            qint32 k = j - 1;
            while ((k > 0) && (nSum < m_frequency[k])) --k;
            if (nSum >= m_frequency[k]) ++k;
            for (qint32 m = j; m > k; --m) {
                m_frequency[m] = m_frequency[m - 1];
                m_child[m] = m_child[m - 1];
            }
            m_frequency[k] = nSum;
            m_child[k] = i;
        }
        for (qint32 n = 0; n < HZL_T; ++n) {
            const qint32 k = m_child[n];
            m_parent[k] = n;
            if (k < HZL_T) m_parent[k + 1] = n;
        }
    }

    void update(qint32 nCharacter)
    {
        if (m_frequency[HZL_R] == HZL_MAX_FREQ) reconstruct();
        qint32 c = m_parent[nCharacter + HZL_T];
        do {
            const qint32 k = ++m_frequency[c];
            qint32 l = c + 1;
            if (k > m_frequency[l]) {
                while (k > m_frequency[l]) ++l;
                --l;
                m_frequency[c] = m_frequency[l];
                m_frequency[l] = k;
                const qint32 i = m_child[c];
                m_parent[i] = l;
                if (i < HZL_T) m_parent[i + 1] = l;
                const qint32 j = m_child[l];
                m_child[l] = i;
                m_parent[j] = c;
                if (j < HZL_T) m_parent[j + 1] = c;
                m_child[c] = j;
                c = l;
            }
            c = m_parent[c];
        } while (c != 0);
    }

    std::array<qint32, HZL_T + 1> m_frequency = {};
    std::array<qint32, HZL_T + HZL_N_CHAR> m_parent = {};
    std::array<qint32, HZL_T> m_child = {};
};

struct HZLPositionTable {
    std::array<quint8, 256> code = {};
    std::array<quint8, 256> length = {};
    bool bValid = false;

    HZLPositionTable()
    {
        static const qint32 nPerLength[6] = {1, 3, 8, 12, 24, 16};
        qint32 nPrefix = 0;
        qint32 nSymbol = 0;
        for (qint32 nLength = 3; nLength <= 8; ++nLength) {
            const qint32 nSpan = 1 << (8 - nLength);
            for (qint32 j = 0; j < nPerLength[nLength - 3]; ++j) {
                for (qint32 k = 0; k < nSpan; ++k) {
                    length[nPrefix] = quint8(nLength);
                    code[nPrefix] = quint8(nSymbol);
                    ++nPrefix;
                }
                ++nSymbol;
            }
        }
        bValid = ((nPrefix == 256) && (nSymbol == 64));
    }
};

qint32 hzlDecodePosition(HZLBitReader *pReader, const HZLPositionTable &table)
{
    qint32 i = pReader->readByte();
    if ((i < 0) || (i > 255)) return -1;
    const qint32 nHigh = qint32(table.code[i]) << 6;
    qint32 j = qint32(table.length[i]) - 2;
    while (j-- > 0) {
        i = ((i << 1) + pReader->readBit()) & 0xffff;
    }
    return nHigh | (i & 0x3f);
}

}  // namespace

bool XHZLDecoder::decode(const QByteArray &baPacked, qint32 nUncompressedSize,
                         QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || (nUncompressedSize < 0)) return false;
    if (nUncompressedSize == 0) {
        *pOutput = QByteArray();
        return true;
    }
    if (baPacked.isEmpty()) return false;

    static const HZLPositionTable table;
    if (!table.bValid) return false;

    HZLBitReader reader(reinterpret_cast<const uchar *>(baPacked.constData()),
                        baPacked.size());
    HZLHuffman huffman;

    std::array<quint8, HZL_N + HZL_F> ring;
    ring.fill(0x20);

    QByteArray baResult(nUncompressedSize, 0);
    qint32 nProduced = 0;
    qint32 nRing = 0;
    const qint32 nMask = HZL_N - 1;

    while (nProduced < nUncompressedSize) {
        if (((nProduced & 0x3fff) == 0) &&
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const qint32 nCharacter = huffman.decodeCharacter(&reader);
        if ((nCharacter < 0) || reader.isOverrun()) return false;
        if (nCharacter < 256) {
            baResult[nProduced++] = char(quint8(nCharacter));
            ring[nRing] = quint8(nCharacter);
            nRing = (nRing + 1) & nMask;
            continue;
        }
        const qint32 nPosition = hzlDecodePosition(&reader, table);
        if ((nPosition < 0) || reader.isOverrun()) return false;
        qint32 nLength = nCharacter + HZL_THRESHOLD - 0xff;
        if ((nLength < 3) || (nLength > HZL_F + HZL_THRESHOLD)) return false;
        // The reference implementation clamps a final overlong match to the remaining plaintext instead
        // of rejecting the member; the stream has no stop code, so the last
        // match legitimately overshoots.
        if (nLength > (nUncompressedSize - nProduced)) {
            nLength = nUncompressedSize - nProduced;
        }
        qint32 nSource = (nRing - nPosition - 1) & nMask;
        for (qint32 i = 0; i < nLength; ++i) {
            const quint8 nValue = ring[nSource & nMask];
            baResult[nProduced++] = char(nValue);
            ring[nRing] = nValue;
            nRing = (nRing + 1) & nMask;
            nSource = (nSource & nMask) + 1;
        }
    }

    *pOutput = baResult;
    return true;
}
