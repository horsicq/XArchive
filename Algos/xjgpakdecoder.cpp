/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xjgpakdecoder.h"

#include <array>
#include <limits>
#include <utility>

namespace {
// LZHUF alphabet: 256 literals + lengths 3..60 folded onto 256..313.
const qint32 JGPAK_F = 60;
const qint32 JGPAK_THRESHOLD = 2;
const qint32 JGPAK_N_CHAR = 256 + JGPAK_F - JGPAK_THRESHOLD;  // 314
const qint32 JGPAK_T = JGPAK_N_CHAR * 2 - 1;                  // 627
const qint32 JGPAK_R = JGPAK_T - 1;                           // 626
const qint32 JGPAK_MAX_FREQ = 0x8000;
// The history ring is deliberately wider than the 12-bit distance range; see
// the class comment in the header.
const qint32 JGPAK_RING_SIZE = 0x2000;
const qint32 JGPAK_RING_MASK = JGPAK_RING_SIZE - 1;
// The encoder flushes a partial final byte, so a well-formed member may ask for
// a few bits that the stored payload no longer holds.  Those read as zero, but
// only for one machine word's worth: anything past that is a truncated member.
const qint64 JGPAK_BIT_SLACK = 32;

class JgpakDecoderContext {
public:
    explicit JgpakDecoderContext(const QByteArray &packed)
        : m_pInput(reinterpret_cast<const quint8 *>(packed.constData())),
          m_nBitLimit(qint64(packed.size()) * 8),
          m_nBitPosition(0),
          m_bError(false)
    {
        for (qint32 i = 0; i < JGPAK_N_CHAR; i++) {
            m_frequency[i] = 1;
            m_child[i] = i + JGPAK_T;
            m_parent[i + JGPAK_T] = i;
        }
        for (qint32 i = 0, nNode = JGPAK_N_CHAR; nNode <= JGPAK_R; i += 2, nNode++) {
            m_frequency[nNode] = m_frequency[i] + m_frequency[i + 1];
            m_child[nNode] = i;
            m_parent[i] = nNode;
            m_parent[i + 1] = nNode;
        }
        m_frequency[JGPAK_T] = 0xffff;
        m_parent[JGPAK_R] = 0;
    }

    bool isFailed() const
    {
        return m_bError;
    }

    // Returns 0..313, or -1 on a bitstream error.
    qint32 decodeCharacter()
    {
        qint32 nCurrent = m_child[JGPAK_R];
        while (nCurrent < JGPAK_T) {
            const qint32 nBit = readBit();
            if (nBit < 0) return -1;
            // Internal nodes always store an even child index, so nCurrent + 1
            // stays inside the tree; the guard is here so a future change to
            // the tree maths cannot turn into an out-of-bounds read.
            if (nCurrent + nBit >= JGPAK_T) return -1;
            nCurrent = m_child[nCurrent + nBit];
            if ((nCurrent < 0) || (nCurrent >= JGPAK_T + JGPAK_N_CHAR)) return -1;
        }
        nCurrent -= JGPAK_T;
        update(nCurrent);
        return nCurrent;
    }

    // Returns a 12-bit distance, or -1 on a bitstream error.
    qint32 decodePosition()
    {
        qint32 nLookahead = 0;
        for (qint32 i = 0; i < 8; i++) {
            const qint32 nBit = readBit();
            if (nBit < 0) return -1;
            nLookahead = (nLookahead << 1) | nBit;
        }
        const qint32 nHigh = qint32(positionCode()[nLookahead]) << 6;
        qint32 nRemaining = qint32(positionLength()[nLookahead]) - 2;
        qint32 nShifted = nLookahead;
        while (nRemaining-- > 0) {
            const qint32 nBit = readBit();
            if (nBit < 0) return -1;
            nShifted = (nShifted << 1) | nBit;
        }
        return nHigh | (nShifted & 0x3f);
    }

private:
    // Stock LZHUF p_len / p_code: 1/3/8/12/24/16 codes of length 3..8.
    static const quint8 *positionLength()
    {
        return buildTables().first.data();
    }
    static const quint8 *positionCode()
    {
        return buildTables().second.data();
    }
    static const std::pair<std::array<quint8, 256>, std::array<quint8, 256> > &buildTables()
    {
        static const std::pair<std::array<quint8, 256>, std::array<quint8, 256> > tables = []() {
            std::array<quint8, 256> lengths = {};
            std::array<quint8, 256> codes = {};
            const qint32 nPerLength[6] = {1, 3, 8, 12, 24, 16};
            qint32 nPrefix = 0;
            qint32 nSymbol = 0;
            for (qint32 nLength = 3; nLength <= 8; nLength++) {
                const qint32 nSpan = 1 << (8 - nLength);
                for (qint32 j = 0; j < nPerLength[nLength - 3]; j++) {
                    for (qint32 k = 0; k < nSpan; k++) {
                        lengths[nPrefix] = quint8(nLength);
                        codes[nPrefix] = quint8(nSymbol);
                        nPrefix++;
                    }
                    nSymbol++;
                }
            }
            return std::make_pair(lengths, codes);
        }();
        return tables;
    }

    qint32 readBit()
    {
        if (m_nBitPosition >= m_nBitLimit) {
            if (m_nBitPosition >= m_nBitLimit + JGPAK_BIT_SLACK) {
                m_bError = true;
                return -1;
            }
            m_nBitPosition++;
            return 0;
        }
        const qint32 nValue = (m_pInput[m_nBitPosition >> 3] >> (7 - (m_nBitPosition & 7))) & 1;
        m_nBitPosition++;
        return nValue;
    }

    void reconstruct()
    {
        qint32 nLeafCount = 0;
        for (qint32 i = 0; i < JGPAK_T; i++) {
            if (m_child[i] >= JGPAK_T) {
                m_frequency[nLeafCount] = (m_frequency[i] + 1) / 2;
                m_child[nLeafCount] = m_child[i];
                nLeafCount++;
            }
        }
        for (qint32 i = 0, nNode = JGPAK_N_CHAR; nNode < JGPAK_T; i += 2, nNode++) {
            const qint32 nSum = m_frequency[i] + m_frequency[i + 1];
            qint32 nInsertion = nNode - 1;
            while ((nInsertion >= 0) && (nSum < m_frequency[nInsertion])) nInsertion--;
            nInsertion++;
            for (qint32 nMove = nNode; nMove > nInsertion; nMove--) {
                m_frequency[nMove] = m_frequency[nMove - 1];
                m_child[nMove] = m_child[nMove - 1];
            }
            m_frequency[nInsertion] = nSum;
            m_child[nInsertion] = i;
        }
        for (qint32 i = 0; i < JGPAK_T; i++) {
            const qint32 nNode = m_child[i];
            m_parent[nNode] = i;
            if (nNode < JGPAK_T) m_parent[nNode + 1] = i;
        }
    }

    void update(qint32 nCharacter)
    {
        if (m_frequency[JGPAK_R] == JGPAK_MAX_FREQ) reconstruct();
        qint32 nCurrent = m_parent[nCharacter + JGPAK_T];
        do {
            const qint32 nUpdated = ++m_frequency[nCurrent];
            qint32 nNext = nCurrent + 1;
            if (nUpdated > m_frequency[nNext]) {
                while (nUpdated > m_frequency[nNext + 1]) nNext++;
                m_frequency[nCurrent] = m_frequency[nNext];
                m_frequency[nNext] = nUpdated;
                const qint32 nOldChild = m_child[nCurrent];
                m_parent[nOldChild] = nNext;
                if (nOldChild < JGPAK_T) m_parent[nOldChild + 1] = nNext;
                const qint32 nNewChild = m_child[nNext];
                m_child[nNext] = nOldChild;
                m_parent[nNewChild] = nCurrent;
                if (nNewChild < JGPAK_T) m_parent[nNewChild + 1] = nCurrent;
                m_child[nCurrent] = nNewChild;
                nCurrent = nNext;
            }
            nCurrent = m_parent[nCurrent];
        } while (nCurrent != 0);
    }

    const quint8 *m_pInput;
    qint64 m_nBitLimit;
    qint64 m_nBitPosition;
    bool m_bError;
    std::array<qint32, JGPAK_T + 1> m_frequency = {};
    std::array<qint32, JGPAK_T + JGPAK_N_CHAR> m_parent = {};
    std::array<qint32, JGPAK_T> m_child = {};
};
}  // namespace

bool XJGPAKDecoder::decode(const QByteArray &packed, qint64 nUncompressedSize,
                           QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > qint64((std::numeric_limits<qint32>::max)()))) return false;
    if (nUncompressedSize == 0) {
        *pOutput = QByteArray();
        return true;
    }
    if (packed.isEmpty()) return false;

    JgpakDecoderContext decoder(packed);
    QByteArray result(qint32(nUncompressedSize), 0);
    std::array<quint8, JGPAK_RING_SIZE> ring;
    ring.fill(0x20);

    qint32 nWrite = 0;
    qint32 nProduced = 0;
    const qint32 nExpected = qint32(nUncompressedSize);
    while (nProduced < nExpected) {
        if (((nProduced & 0x3fff) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nCharacter = decoder.decodeCharacter();
        if (nCharacter < 0) return false;
        if (nCharacter < 256) {
            result[nProduced++] = char(quint8(nCharacter));
            ring[nWrite] = quint8(nCharacter);
            nWrite = (nWrite + 1) & JGPAK_RING_MASK;
            continue;
        }
        const qint32 nPosition = decoder.decodePosition();
        if (nPosition < 0) return false;
        qint32 nLength = nCharacter + JGPAK_THRESHOLD - 0xff;
        if ((nLength < 3) || (nLength > JGPAK_F)) return false;
        if (nLength > nExpected - nProduced) nLength = nExpected - nProduced;
        qint32 nRead = (nWrite - nPosition - 1) & JGPAK_RING_MASK;
        for (qint32 i = 0; i < nLength; i++) {
            const quint8 nValue = ring[nRead];
            result[nProduced++] = char(nValue);
            ring[nWrite] = nValue;
            nWrite = (nWrite + 1) & JGPAK_RING_MASK;
            nRead = (nRead + 1) & JGPAK_RING_MASK;
        }
    }
    if (decoder.isFailed()) return false;

    *pOutput = result;
    return true;
}
