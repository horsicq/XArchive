/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xslsdecoder.h"

#include <QVector>

namespace {
const qint32 SLS_N = 8192;
const qint32 SLS_N_MASK = SLS_N - 1;
const qint32 SLS_F = 90;
const qint32 SLS_THRESHOLD = 2;
const qint32 SLS_N_CHAR = 256 - SLS_THRESHOLD + SLS_F;  // 344
const qint32 SLS_T = SLS_N_CHAR * 2 - 1;                // 687
const qint32 SLS_R = SLS_T - 1;                         // 686
const quint32 SLS_MAX_FREQ = 0x8000U;
const qint64 SLS_MAX_OUTPUT = 0x10000000;  // 256 MiB sanity cap

// Stock Okumura position tables; byte-identical to the reference implementation's copies.
const quint8 SLS_D_CODE[256] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0a,
    0x0a, 0x0a, 0x0a, 0x0a, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
    0x0c, 0x0c, 0x0c, 0x0c, 0x0d, 0x0d, 0x0d, 0x0d, 0x0e, 0x0e, 0x0e, 0x0e,
    0x0f, 0x0f, 0x0f, 0x0f, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11, 0x11, 0x11,
    0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14, 0x14,
    0x15, 0x15, 0x15, 0x15, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17, 0x17, 0x17,
    0x18, 0x18, 0x19, 0x19, 0x1a, 0x1a, 0x1b, 0x1b, 0x1c, 0x1c, 0x1d, 0x1d,
    0x1e, 0x1e, 0x1f, 0x1f, 0x20, 0x20, 0x21, 0x21, 0x22, 0x22, 0x23, 0x23,
    0x24, 0x24, 0x25, 0x25, 0x26, 0x26, 0x27, 0x27, 0x28, 0x28, 0x29, 0x29,
    0x2a, 0x2a, 0x2b, 0x2b, 0x2c, 0x2c, 0x2d, 0x2d, 0x2e, 0x2e, 0x2f, 0x2f,
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b,
    0x3c, 0x3d, 0x3e, 0x3f};

const quint8 SLS_D_LEN[16] = {3, 3, 4, 4, 4, 5, 5, 5,
                              5, 6, 6, 6, 7, 7, 7, 8};

class SLSLzhuf {
public:
    SLSLzhuf(const quint8 *pData, qint64 nSize)
        : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nBuffer(0), m_nBits(0),
          m_bEof(false)
    {
        m_vectFreq.resize(SLS_T + 1);
        // One slot past T: a corrupt tree can make the walk in decodeChar()
        // index son[T], which a well-formed tree never does.
        m_vectSon.resize(SLS_T + 1);
        m_vectPrnt.resize(SLS_T + SLS_N_CHAR);
        m_vectFreq.fill(0);
        m_vectSon.fill(0);
        m_vectPrnt.fill(0);

        for (qint32 i = 0; i < SLS_N_CHAR; ++i) {
            m_vectFreq[i] = 1;
            m_vectSon[i] = i + SLS_T;
            m_vectPrnt[i + SLS_T] = i;
        }
        qint32 i = 0;
        qint32 j = SLS_N_CHAR;
        while (j <= SLS_R) {
            m_vectFreq[j] = m_vectFreq[i] + m_vectFreq[i + 1];
            m_vectSon[j] = i;
            m_vectPrnt[i] = j;
            m_vectPrnt[i + 1] = j;
            i += 2;
            ++j;
        }
        m_vectFreq[SLS_T] = 0xffffU;
        m_vectPrnt[SLS_R] = 0;
    }

    bool isEof() const
    {
        return m_bEof;
    }

    qint32 decodeChar()
    {
        qint32 nCode = m_vectSon[SLS_R];
        qint32 nDepth = 0;
        while (nCode < SLS_T) {
            if ((nCode < 0) || (++nDepth > SLS_T)) return -1;
            nCode = m_vectSon[nCode + getBit()];
            if (m_bEof) return -1;
            if ((nCode < 0) || (nCode >= SLS_T + SLS_N_CHAR)) return -1;
        }
        nCode -= SLS_T;
        update(nCode);
        return nCode;
    }

    qint32 decodePosition()
    {
        qint32 nByte = getByte();
        if (m_bEof) return -1;
        const qint32 nHigh =
            static_cast<qint32>(SLS_D_CODE[nByte & 0xff]) << 7;
        qint32 nExtra = static_cast<qint32>(SLS_D_LEN[(nByte >> 4) & 0x0f]) - 1;
        while (nExtra > 0) {
            --nExtra;
            nByte = ((nByte << 1) + getBit()) & 0xffff;
            if (m_bEof) return -1;
        }
        return nHigh | (nByte & 0x7f);
    }

private:
    void fill()
    {
        quint32 nByte = 0;
        if (m_nPos < m_nSize) {
            nByte = m_pData[m_nPos];
            ++m_nPos;
        } else {
            m_bEof = true;
        }
        m_nBuffer = (m_nBuffer | (nByte << (8 - m_nBits))) & 0xffffffffU;
        m_nBits += 8;
    }

    qint32 getBit()
    {
        while (m_nBits == 0) {
            if (m_bEof) return 0;
            fill();
        }
        const quint32 nValue = m_nBuffer;
        m_nBuffer = (m_nBuffer << 1) & 0xffffffffU;
        --m_nBits;
        return (nValue & 0x8000U) ? 1 : 0;
    }

    qint32 getByte()
    {
        while (m_nBits < 8) {
            if (m_bEof) return 0;
            fill();
        }
        const quint32 nValue = m_nBuffer;
        m_nBuffer = (m_nBuffer << 8) & 0xffffffffU;
        m_nBits -= 8;
        return static_cast<qint32>((nValue >> 8) & 0xffU);
    }

    void reconst()
    {
        qint32 j = 0;
        for (qint32 i = 0; i < SLS_T; ++i) {
            if (m_vectSon[i] >= SLS_T) {
                m_vectFreq[j] = (m_vectFreq[i] + 1) / 2;
                m_vectSon[j] = m_vectSon[i];
                ++j;
            }
        }
        qint32 i = 0;
        for (j = SLS_N_CHAR; j < SLS_T; ++j) {
            const quint32 nFreq = m_vectFreq[i] + m_vectFreq[i + 1];
            m_vectFreq[j] = nFreq;
            qint32 k = j - 1;
            while (nFreq < m_vectFreq[k]) --k;
            ++k;
            for (qint32 n = j; n > k; --n) {
                m_vectFreq[n] = m_vectFreq[n - 1];
                m_vectSon[n] = m_vectSon[n - 1];
            }
            m_vectFreq[k] = nFreq;
            m_vectSon[k] = i;
            i += 2;
        }
        for (qint32 n = 0; n < SLS_T; ++n) {
            const qint32 k = m_vectSon[n];
            if (k >= SLS_T) {
                m_vectPrnt[k] = n;
            } else {
                m_vectPrnt[k] = n;
                m_vectPrnt[k + 1] = n;
            }
        }
    }

    void update(qint32 nCode)
    {
        if (m_vectFreq[SLS_R] == SLS_MAX_FREQ) reconst();
        qint32 c = m_vectPrnt[nCode + SLS_T];
        do {
            ++m_vectFreq[c];
            const quint32 k = m_vectFreq[c];
            if (k > m_vectFreq[c + 1]) {
                qint32 l = c + 2;
                while (k > m_vectFreq[l]) ++l;
                --l;
                m_vectFreq[c] = m_vectFreq[l];
                m_vectFreq[l] = k;
                const qint32 i = m_vectSon[c];
                m_vectPrnt[i] = l;
                if (i < SLS_T) m_vectPrnt[i + 1] = l;
                const qint32 j = m_vectSon[l];
                m_vectSon[l] = i;
                m_vectPrnt[j] = c;
                if (j < SLS_T) m_vectPrnt[j + 1] = c;
                m_vectSon[c] = j;
                c = l;
            }
            c = m_vectPrnt[c];
        } while (c != 0);
    }

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nBuffer;
    qint32 m_nBits;
    bool m_bEof;
    QVector<quint32> m_vectFreq;
    QVector<qint32> m_vectSon;
    QVector<qint32> m_vectPrnt;
};
}  // namespace

bool XSLSDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                         QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;
    pOutput->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > SLS_MAX_OUTPUT)) {
        return false;
    }
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    SLSLzhuf state(reinterpret_cast<const quint8 *>(baPacked.constData()),
                   baPacked.size());

    QByteArray baRing(SLS_N + SLS_F - 1, static_cast<char>(0x20));
    QByteArray baResult;
    baResult.reserve(static_cast<qint32>(nUncompressedSize));

    qint32 nRing = 0;
    qint64 nRemaining = nUncompressedSize;
    qint32 nCounter = 0;

    while (nRemaining > 0) {
        if ((++nCounter & 0xffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
        }
        const qint32 nSymbol = state.decodeChar();
        if ((nSymbol < 0) || state.isEof()) break;

        if (nSymbol < 256) {
            baResult.append(static_cast<char>(static_cast<quint8>(nSymbol)));
            --nRemaining;
            baRing[nRing] = static_cast<char>(static_cast<quint8>(nSymbol));
            nRing = (nRing + 1) & SLS_N_MASK;
        } else {
            const qint32 nPosition = state.decodePosition();
            if ((nPosition < 0) || state.isEof()) break;
            qint32 nSource = (nRing - nPosition - 1) & SLS_N_MASK;
            qint64 nLength = nSymbol + SLS_THRESHOLD - 0xff;
            if (nLength > nRemaining) nLength = nRemaining;
            while (nLength > 0) {
                const char cByte = baRing.at(nSource & SLS_N_MASK);
                baResult.append(cByte);
                --nRemaining;
                baRing[nRing] = cByte;
                nRing = (nRing + 1) & SLS_N_MASK;
                nSource = (nSource & SLS_N_MASK) + 1;
                --nLength;
            }
        }
    }

    *pOutput = baResult;
    return (baResult.size() == nUncompressedSize);
}
