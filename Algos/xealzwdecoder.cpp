/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xealzwdecoder.h"

#include <QVector>

namespace {
const qint64 EA_LZW_MAX_OUTPUT = 0x10000000;  // 256 MiB sanity cap

class EABitReader {
public:
    EABitReader(const quint8 *pData, qint64 nSize)
        : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nBuffer(0), m_nBits(0)
    {
    }

    // LSB-first: bytes enter the accumulator at the current bit height and the
    // requested width is taken off the bottom.
    bool read(qint32 nWidth, quint32 *pnValue)
    {
        while (m_nBits < nWidth) {
            if (m_nPos >= m_nSize) return false;
            m_nBuffer |= static_cast<quint32>(m_pData[m_nPos]) << m_nBits;
            ++m_nPos;
            m_nBits += 8;
        }
        *pnValue = m_nBuffer & ((1U << nWidth) - 1U);
        m_nBuffer >>= nWidth;
        m_nBits -= nWidth;
        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nBuffer;
    qint32 m_nBits;
};
}  // namespace

bool XEALzwDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                           QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput) return false;
    pOutput->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > EA_LZW_MAX_OUTPUT)) {
        return false;
    }
    if (baPacked.isEmpty()) return (nUncompressedSize == 0);

    QVector<quint16> vectPrefix(EA_LZW_MAX_CODES, 0);
    QVector<quint8> vectSuffix(EA_LZW_MAX_CODES, 0);
    for (qint32 i = 0; i < 256; ++i) {
        vectSuffix[i] = static_cast<quint8>(i);
    }
    QVector<quint8> vectStack(EA_LZW_MAX_CODES + 1, 0);

    QByteArray baResult;
    baResult.reserve(static_cast<qint32>(qMin<qint64>(nUncompressedSize, EA_LZW_MAX_OUTPUT)));

    EABitReader reader(reinterpret_cast<const quint8 *>(baPacked.constData()),
                       baPacked.size());

    qint32 nWidth = 9;
    qint32 nMaxCode = 0x1ff;
    qint32 nNextFree = EA_LZW_FIRST_CODE;
    qint32 nPrevCode = 0;
    qint32 nFirstChar = 0;
    bool bStarted = false;
    bool bClean = false;
    qint32 nCounter = 0;

    while (true) {
        if ((++nCounter & 0xffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
        }
        // The width is bumped by the reader BEFORE the fetch, and only while
        // it is still below the 12-bit ceiling.
        if (nMaxCode < nNextFree) {
            ++nWidth;
            nMaxCode = (nWidth == EA_LZW_MAX_BITS) ? EA_LZW_MAX_CODES
                                                   : ((1 << nWidth) - 1);
        }
        quint32 nRaw = 0;
        if (!reader.read(nWidth, &nRaw)) break;
        const qint32 nCode = static_cast<qint32>(nRaw);

        if (nCode == EA_LZW_END) {
            bClean = true;
            break;
        }

        if (nCode == EA_LZW_CLEAR) {
            nWidth = 9;
            nMaxCode = 0x1ff;
            nNextFree = EA_LZW_FIRST_CODE;
            if (!reader.read(nWidth, &nRaw)) break;
            const qint32 nSeed = static_cast<qint32>(nRaw);
            if (nSeed == EA_LZW_END) {
                bClean = true;
                break;
            }
            if (nSeed >= 0x100) break;  // CLEAR must be followed by a literal
            nPrevCode = nSeed;
            nFirstChar = nSeed;
            baResult.append(static_cast<char>(static_cast<quint8>(nSeed)));
            if (baResult.size() > EA_LZW_MAX_OUTPUT) break;
            bStarted = true;
            continue;
        }

        // A code stream that does not open with CLEAR has no defined previous
        // code; U3 reads a stale register there, so refuse instead.
        if (!bStarted) break;

        qint32 nStackSize = 0;
        qint32 nCurrent = nCode;
        if (nCode >= nNextFree) {
            vectStack[nStackSize++] = static_cast<quint8>(nFirstChar);
            nCurrent = nPrevCode;
        }
        bool bOverflow = false;
        while (nCurrent > 0xff) {
            if (nStackSize >= EA_LZW_MAX_CODES) {
                bOverflow = true;
                break;
            }
            vectStack[nStackSize++] = vectSuffix[nCurrent];
            nCurrent = vectPrefix[nCurrent];
        }
        if (bOverflow) break;
        vectStack[nStackSize++] = static_cast<quint8>(nCurrent & 0xff);
        nFirstChar = nCurrent & 0xff;

        while (nStackSize > 0) {
            --nStackSize;
            baResult.append(static_cast<char>(vectStack[nStackSize]));
        }
        if (baResult.size() > EA_LZW_MAX_OUTPUT) break;

        if (nNextFree < EA_LZW_MAX_CODES) {
            vectPrefix[nNextFree] = static_cast<quint16>(nPrevCode);
            vectSuffix[nNextFree] = static_cast<quint8>(nFirstChar);
            ++nNextFree;
        }
        nPrevCode = nCode;
    }

    *pOutput = baResult;
    Q_UNUSED(bClean)
    return (baResult.size() == nUncompressedSize);
}
