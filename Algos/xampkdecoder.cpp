/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * The two codecs below are ports of Haruhiko Okumura's 1988/1989 LZSS and
 * LZARI reference implementations ("Use, distribute, and modify this program
 * freely"), reparameterised to the constants an AMPK container actually uses.
 */
#include "xampkdecoder.h"

#include <memory>
#include <new>

namespace {
// A member never legitimately expands past a few hundred KiB in this family;
// the cap only exists so a malformed uncompressedSize field cannot turn into an
// attacker-chosen allocation.
const qint64 AMPK_MAX_UNPACKED_SIZE = 0x10000000;  // 256 MiB

// Both codecs share the 4 KiB Okumura ring buffer.
const qint32 AMPK_RING_SIZE = 4096;
const qint32 AMPK_RING_MASK = AMPK_RING_SIZE - 1;

const qint32 AMPK_LZSS_F = 18;  // longest LZSS match; also the ring cursor origin

// Okumura's arithmetic decoder keeps a 17-bit look-ahead, so it always reads
// one or two bytes past the last coded bit.  Anything beyond this budget means
// the stream is not really LZARI.
const qint64 AMPK_LZARI_MAX_OVERREAD = 8;

// The renormalisation loop terminates in ~17 steps on a well-formed stream.
// The bound turns a corrupt interval into a failure instead of a hang.
const qint32 AMPK_LZARI_RENORM_LIMIT = 256;

// MSB-first bit source.  Reads past the end return zero bits rather than
// failing: LZARI legitimately primes its register from bytes that lie outside
// the member's declared compressed size.
class AmpkBitReader {
public:
    explicit AmpkBitReader(const QByteArray &baData)
        : m_pData(reinterpret_cast<const quint8 *>(baData.constData())),
          m_nSize(baData.size()),
          m_nBitPosition(0)
    {
    }

    quint32 getBit()
    {
        const qint64 nByte = m_nBitPosition >> 3;
        const quint32 nBit =
            (nByte < m_nSize)
                ? ((m_pData[nByte] >> (7 - (m_nBitPosition & 7))) & 1U)
                : 0U;
        ++m_nBitPosition;
        return nBit;
    }

    qint64 consumedSize() const { return (m_nBitPosition + 7) / 8; }
    qint64 inputSize() const { return m_nSize; }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nBitPosition;
};

struct AmpkLzari {
    static const qint32 N = AMPK_RING_SIZE;
    static const qint32 F = 60;
    static const qint32 THRESHOLD = 2;
    static const qint32 N_CHAR = 256 - THRESHOLD + F;  // 314, no EOF symbol
    static const qint32 M = 15;
    static const quint32 Q1 = 1U << M;
    static const quint32 Q2 = 2U * Q1;
    static const quint32 Q3 = 3U * Q1;
    static const quint32 Q4 = 4U * Q1;
    static const quint32 MAX_CUM = Q1 - 1U;

    AmpkBitReader bits;
    bool bError;
    quint32 nLow;
    quint32 nHigh;
    quint32 nValue;
    quint32 symFreq[N_CHAR + 1];
    quint32 symCum[N_CHAR + 1];
    quint32 posCum[N + 1];
    qint32 charToSym[N_CHAR];
    qint32 symToChar[N_CHAR + 1];
    quint8 ring[N];

    explicit AmpkLzari(const QByteArray &baPacked)
        : bits(baPacked), bError(false), nLow(0), nHigh(Q4), nValue(0)
    {
        // Model start-up.  The sym_to_char / char_to_sym permutation is not
        // decoration: UpdateModel promotes a symbol by swapping it with the
        // first equally frequent one, so the symbol index and the character it
        // stands for drift apart after the very first update.
        symCum[N_CHAR] = 0;
        for (qint32 nSym = N_CHAR; nSym > 0; --nSym) {
            const qint32 nChar = nSym - 1;
            charToSym[nChar] = nSym;
            symToChar[nSym] = nChar;
            symFreq[nSym] = 1;
            symCum[nSym - 1] = symCum[nSym] + symFreq[nSym];
        }
        // symFreq[0] is a sentinel that stops the promotion walk in
        // updateModel(); it must stay below every real frequency.
        symFreq[0] = 0;
        symToChar[0] = 0;

        posCum[N] = 0;
        for (qint32 i = N; i > 0; --i) {
            posCum[i - 1] =
                posCum[i] + 10000U / static_cast<quint32>(i + 200);
        }

        for (qint32 i = 0; i < N; ++i) ring[i] = ' ';

        for (qint32 i = 0; i < M + 2; ++i) {
            nValue = (nValue << 1) + bits.getBit();
        }
    }

    qint32 searchSymbol(quint32 nTarget) const
    {
        qint32 i = 1;
        qint32 j = N_CHAR;
        while (i < j) {
            const qint32 k = (i + j) / 2;
            if (symCum[k] > nTarget) {
                i = k + 1;
            } else {
                j = k;
            }
        }
        return i;
    }

    qint32 searchPosition(quint32 nTarget) const
    {
        qint32 i = 1;
        qint32 j = N;
        while (i < j) {
            const qint32 k = (i + j) / 2;
            if (posCum[k] > nTarget) {
                i = k + 1;
            } else {
                j = k;
            }
        }
        return i - 1;
    }

    bool renormalize()
    {
        for (qint32 nGuard = 0; nGuard < AMPK_LZARI_RENORM_LIMIT; ++nGuard) {
            if (nLow >= Q2) {
                nValue -= Q2;
                nLow -= Q2;
                nHigh -= Q2;
            } else if ((nLow >= Q1) && (nHigh <= Q3)) {
                nValue -= Q1;
                nLow -= Q1;
                nHigh -= Q1;
            } else if (nHigh > Q2) {
                return true;
            }
            nLow += nLow;
            nHigh += nHigh;
            nValue = (nValue << 1) + bits.getBit();
        }
        return false;
    }

    void updateModel(qint32 nSym)
    {
        if (symCum[0] >= MAX_CUM) {
            quint32 nAccumulated = 0;
            for (qint32 i = N_CHAR; i > 0; --i) {
                symCum[i] = nAccumulated;
                symFreq[i] = (symFreq[i] + 1) >> 1;
                nAccumulated += symFreq[i];
            }
            symCum[0] = nAccumulated;
        }

        qint32 i = nSym;
        while (symFreq[i] == symFreq[i - 1]) --i;
        if (i < nSym) {
            const qint32 nCharAtI = symToChar[i];
            const qint32 nCharAtSym = symToChar[nSym];
            symToChar[i] = nCharAtSym;
            symToChar[nSym] = nCharAtI;
            charToSym[nCharAtI] = nSym;
            charToSym[nCharAtSym] = i;
        }
        ++symFreq[i];
        for (qint32 j = i - 1; j >= 0; --j) ++symCum[j];
    }

    qint32 decodeChar()
    {
        const quint32 nRange = nHigh - nLow;
        if ((nRange == 0) || (symCum[0] == 0)) {
            bError = true;
            return -1;
        }
        const quint64 nTarget =
            ((static_cast<quint64>(nValue - nLow + 1) * symCum[0]) - 1U) /
            nRange;
        const qint32 nSym = searchSymbol(static_cast<quint32>(nTarget));
        nHigh = nLow + static_cast<quint32>(
                           (static_cast<quint64>(nRange) * symCum[nSym - 1]) /
                           symCum[0]);
        nLow = nLow + static_cast<quint32>(
                          (static_cast<quint64>(nRange) * symCum[nSym]) /
                          symCum[0]);
        if (!renormalize()) {
            bError = true;
            return -1;
        }
        const qint32 nChar = symToChar[nSym];
        updateModel(nSym);
        return nChar;
    }

    qint32 decodePosition()
    {
        const quint32 nRange = nHigh - nLow;
        if ((nRange == 0) || (posCum[0] == 0)) {
            bError = true;
            return -1;
        }
        const quint64 nTarget =
            ((static_cast<quint64>(nValue - nLow + 1) * posCum[0]) - 1U) /
            nRange;
        const qint32 nPosition = searchPosition(static_cast<quint32>(nTarget));
        nHigh = nLow + static_cast<quint32>(
                           (static_cast<quint64>(nRange) * posCum[nPosition]) /
                           posCum[0]);
        nLow = nLow + static_cast<quint32>(
                          (static_cast<quint64>(nRange) *
                           posCum[nPosition + 1]) /
                          posCum[0]);
        if (!renormalize()) {
            bError = true;
            return -1;
        }
        return nPosition;
    }

    bool decode(qint64 nOutSize, char *pOut)
    {
        qint32 nRing = N - F;
        qint64 nOutPosition = 0;
        while (nOutPosition < nOutSize) {
            if (bits.consumedSize() >
                bits.inputSize() + AMPK_LZARI_MAX_OVERREAD) {
                return false;
            }
            const qint32 nChar = decodeChar();
            if (bError) return false;
            if (nChar < 256) {
                pOut[nOutPosition++] = static_cast<char>(nChar);
                ring[nRing] = static_cast<quint8>(nChar);
                nRing = (nRing + 1) & AMPK_RING_MASK;
            } else {
                const qint32 nPosition = decodePosition();
                if (bError) return false;
                const qint32 nLength = nChar - 255 + THRESHOLD;
                const qint32 nSource =
                    (nRing - nPosition - 1) & AMPK_RING_MASK;
                for (qint32 k = 0;
                     (k < nLength) && (nOutPosition < nOutSize); ++k) {
                    const quint8 nByte = ring[(nSource + k) & AMPK_RING_MASK];
                    pOut[nOutPosition++] = static_cast<char>(nByte);
                    ring[nRing] = nByte;
                    nRing = (nRing + 1) & AMPK_RING_MASK;
                }
            }
        }
        return true;
    }
};
}  // namespace

bool XAMPKDecoder::decodeLZSS(const QByteArray &baPacked,
                              qint64 nUncompressedSize,
                              QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked || (nUncompressedSize < 0) ||
        (nUncompressedSize > AMPK_MAX_UNPACKED_SIZE)) {
        return false;
    }
    pbaUnpacked->clear();
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    QByteArray baRing(AMPK_RING_SIZE, ' ');
    // Okumura's encoder leaves the F slots the write cursor starts on at 0x00
    // while the rest of the pre-history is ' '.  No member in the reference
    // corpus reads them before they are overwritten, but reproducing the
    // encoder's exact initial state is what keeps that true for unseen files.
    for (qint32 i = AMPK_RING_SIZE - AMPK_LZSS_F; i < AMPK_RING_SIZE; ++i) {
        baRing[i] = static_cast<char>(0);
    }

    QByteArray baOut;
    baOut.resize(static_cast<qint32>(nUncompressedSize));
    if (baOut.size() != nUncompressedSize) return false;

    quint8 *pRing = reinterpret_cast<quint8 *>(baRing.data());
    char *pOut = baOut.data();
    const quint8 *pIn = reinterpret_cast<const quint8 *>(baPacked.constData());
    const qint64 nInSize = baPacked.size();

    qint64 nInPosition = 0;
    qint64 nOutPosition = 0;
    qint32 nRing = AMPK_RING_SIZE - AMPK_LZSS_F;
    quint32 nFlags = 0;
    qint32 nFlagBits = 0;

    while (nOutPosition < nUncompressedSize) {
        if (nFlagBits == 0) {
            if (nInPosition >= nInSize) return false;
            nFlags = pIn[nInPosition++];
            nFlagBits = 8;
        }
        const bool bLiteral = (nFlags & 1U) != 0;
        nFlags >>= 1;
        --nFlagBits;

        if (bLiteral) {
            if (nInPosition >= nInSize) return false;
            const quint8 nByte = pIn[nInPosition++];
            pOut[nOutPosition++] = static_cast<char>(nByte);
            pRing[nRing] = nByte;
            nRing = (nRing + 1) & AMPK_RING_MASK;
        } else {
            if (nInPosition + 1 >= nInSize) return false;
            const quint32 nFirst = pIn[nInPosition];
            const quint32 nSecond = pIn[nInPosition + 1];
            nInPosition += 2;
            const qint32 nSource =
                static_cast<qint32>(nFirst | ((nSecond & 0xf0U) << 4));
            const qint32 nLength = static_cast<qint32>(nSecond & 0x0fU) + 3;
            for (qint32 k = 0;
                 (k < nLength) && (nOutPosition < nUncompressedSize); ++k) {
                const quint8 nByte = pRing[(nSource + k) & AMPK_RING_MASK];
                pOut[nOutPosition++] = static_cast<char>(nByte);
                pRing[nRing] = nByte;
                nRing = (nRing + 1) & AMPK_RING_MASK;
            }
        }
    }

    *pbaUnpacked = baOut;
    return true;
}

bool XAMPKDecoder::decodeLZARI(const QByteArray &baPacked,
                               qint64 nUncompressedSize,
                               QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked || (nUncompressedSize < 0) ||
        (nUncompressedSize > AMPK_MAX_UNPACKED_SIZE)) {
        return false;
    }
    pbaUnpacked->clear();
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    QByteArray baOut;
    baOut.resize(static_cast<qint32>(nUncompressedSize));
    if (baOut.size() != nUncompressedSize) return false;

    // ~25 KiB of model tables; kept off the stack so a deeply nested unpack
    // chain cannot overflow it.
    std::unique_ptr<AmpkLzari> pDecoder(new (std::nothrow) AmpkLzari(baPacked));
    if (!pDecoder || !pDecoder->decode(nUncompressedSize, baOut.data())) {
        return false;
    }

    *pbaUnpacked = baOut;
    return true;
}
