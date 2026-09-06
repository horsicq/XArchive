/*
 * Stac Electronics SAF archive member codec.
 *
 * Clean-room implementation derived from observed SAF streams and reference
 * decoder behavior. Copyright (c) 2026 hors<horsicq@gmail.com>.
 *
 * MIT License
 */
#include "xsafdecoder.h"

namespace {
const qint32 SAF_WINDOW = 0x800;

// MSB first reader over a 32 bit accumulator.  Running out of input simply
// stops the walk; the reference decoder ends a member on the explicit
// end-of-stream token, and a truncated member is reported as a failure by the
// size check in decode().
class SafBits {
public:
    SafBits(const uchar *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nAccumulator(0), m_nCount(0)
    {
    }

    bool read(qint32 nBits, quint32 *pnValue)
    {
        if (nBits == 0) {
            *pnValue = 0;
            return true;
        }
        while (m_nCount < nBits) {
            if (m_nPosition >= m_nSize) return false;
            m_nAccumulator = static_cast<quint32>(m_nAccumulator + (static_cast<quint32>(m_pData[m_nPosition]) << (24 - m_nCount)));
            m_nPosition++;
            m_nCount += 8;
        }
        *pnValue = m_nAccumulator >> (32 - nBits);
        m_nAccumulator = static_cast<quint32>(m_nAccumulator << nBits);
        m_nCount -= nBits;
        return true;
    }

    // 2 bits; the value 3 escapes to 2 more bits, and the value 3 there
    // escapes to a chain of 4 bit nibbles terminated by a nibble below 15.
    bool readLength(qint32 *pnLength)
    {
        quint32 nValue = 0;
        if (!read(2, &nValue)) return false;
        qint32 nLength = static_cast<qint32>(nValue);
        if (nLength == 3) {
            quint32 nExtra = 0;
            if (!read(2, &nExtra)) return false;
            nLength += static_cast<qint32>(nExtra);
            if (nExtra == 3) {
                for (;;) {
                    quint32 nNibble = 0;
                    if (!read(4, &nNibble)) return false;
                    nLength += static_cast<qint32>(nNibble);
                    if (nNibble != 0xf) break;
                    if (nLength > 0x1000000) return false;
                }
            }
        }
        *pnLength = nLength;
        return true;
    }

private:
    const uchar *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nAccumulator;
    qint32 m_nCount;
};

bool safDecodeChunk(const uchar *pData, qint64 nSize, QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    SafBits bits(pData, nSize);
    QByteArray baWindow(SAF_WINDOW, char(0));
    uchar *pWindow = reinterpret_cast<uchar *>(baWindow.data());
    qint32 nIndex = 0;

    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        quint32 nToken = 0;
        if (!bits.read(9, &nToken)) break;
        if (nToken < 0x100) {
            pWindow[nIndex] = static_cast<uchar>(nToken);
            nIndex++;
            if (nIndex == SAF_WINDOW) {
                nIndex = 0;
                pOutput->append(baWindow);
            }
            continue;
        }
        const quint32 nCode = nToken & 0xffU;
        qint32 nDistance = 0;
        if (nCode == 0x81U) {
            nDistance = 1;
        } else if (nCode < 0x80U) {
            quint32 nLow = 0;
            if (!bits.read(4, &nLow)) break;
            nDistance = static_cast<qint32>(nCode * 16U + nLow);
        } else {
            nDistance = static_cast<qint32>(nCode & 0x7fU);
            if (nDistance == 0) break;  // end of stream
        }
        qint32 nLength = 0;
        if (!bits.readLength(&nLength)) break;
        nLength += 2;
        for (qint32 i = 0; i < nLength; i++) {
            pWindow[nIndex] = pWindow[(nIndex - nDistance) & (SAF_WINDOW - 1)];
            nIndex++;
            if (nIndex == SAF_WINDOW) {
                nIndex = 0;
                pOutput->append(baWindow);
            }
        }
    }
    if (nIndex > 0) {
        pOutput->append(baWindow.constData(), nIndex);
    }
    return true;
}
}  // namespace

bool XSAFDecoder::decode(const QByteArray &packed, qint64 nRawSize, qint32 nMethod, QByteArray *pUnpacked, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pUnpacked || (nRawSize < 0)) return false;
    QByteArray baResult;
    if (nRawSize > 0) baResult.reserve(static_cast<qint32>(qMin<qint64>(nRawSize, 0x4000000)));

    const uchar *pData = reinterpret_cast<const uchar *>(packed.constData());
    const qint64 nSize = packed.size();

    if (nMethod == 3) {
        if (!safDecodeChunk(pData, nSize, &baResult, pPdStruct)) return false;
    } else {
        qint64 nPosition = 0;
        while (nPosition < nSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            if ((nSize - nPosition) < 4) return false;
            const qint32 nChunkSize = static_cast<qint32>(static_cast<quint32>(pData[nPosition]) | (static_cast<quint32>(pData[nPosition + 1]) << 8) |
                                                          (static_cast<quint32>(pData[nPosition + 2]) << 16) |
                                                          (static_cast<quint32>(pData[nPosition + 3]) << 24));
            nPosition += 4;
            if ((nChunkSize < 0) || (nChunkSize > (nSize - nPosition))) return false;
            if (!safDecodeChunk(pData + nPosition, nChunkSize, &baResult, pPdStruct)) return false;
            nPosition += nChunkSize;
        }
    }

    if (static_cast<qint64>(baResult.size()) != nRawSize) return false;
    *pUnpacked = baResult;
    return true;
}
