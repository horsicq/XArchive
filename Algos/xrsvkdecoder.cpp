/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xrsvkdecoder.h"

#include <QtEndian>

#include <vector>

namespace {

const qint64 RSVK_BLOCK_HEADER_SIZE = 20;
// U3's own ceilings (FUN_00691770 / FUN_00691d40).  The MTF symbol buffer is
// capped at 0xc87d2 bytes and the inverse-BWT index vector at 0x643e8 entries;
// keeping both makes this decoder accept exactly the blocks U3 accepts.
const qint32 RSVK_MAX_MTF = 0xc87d2;
const qint32 RSVK_MAX_ROWS = 0x643e8;
const qint32 RSVK_SYMBOL_EOB = 0x101;
const qint32 RSVK_GROUPS = 9;

const qint32 g_nGroupLow[RSVK_GROUPS] = {0, 1, 2, 4, 8, 16, 32, 64, 128};
const qint32 g_nGroupHigh[RSVK_GROUPS] = {0, 1, 3, 7, 15, 31, 63, 127, 257};
// Entries 0 and 1 are unused: those two groups are the bare RUNA/RUNB symbols
// and get no sub-model.
const qint32 g_nGroupMaxFrequency[RSVK_GROUPS] = {
    0, 0, 0x100, 0x100, 0x80, 0x400, 0x800, 0x1000, 0x2000};

const qint32 RSVK_SELECTOR_MAX_FREQUENCY = 0x1000;
const qint32 RSVK_SELECTOR_INCREMENT = 0x20;
const qint32 RSVK_MAX_SYMBOLS = 132;  // group 8 is the widest: 130 chars + 2

quint32 rsvkCrc32(const uchar *pData, qint64 nSize)
{
    static quint32 table[256];
    static bool bReady = false;
    if (!bReady) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (qint32 k = 0; k < 8; ++k) {
                c = (c & 1) ? (0xedb88320U ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        bReady = true;
    }
    quint32 nCrc = 0xffffffffU;
    for (qint64 i = 0; i < nSize; ++i) {
        nCrc = table[(nCrc ^ pData[i]) & 0xff] ^ (nCrc >> 8);
    }
    return nCrc ^ 0xffffffffU;
}

// MSB-first bit reader; reading past the end yields zero bits, which is what
// U3's FUN_00486b90 does when the bounded sub-stream is exhausted.
class RSVKBits {
public:
    RSVKBits(const uchar *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize)
    {
    }
    qint32 readBit()
    {
        if (m_nCount == 0) {
            m_nCurrent = (m_nPosition < m_nSize) ? m_pData[m_nPosition] : 0;
            ++m_nPosition;
            m_nCount = 8;
        }
        const qint32 nBit = (m_nCurrent >> 7) & 1;
        m_nCurrent = quint8(m_nCurrent << 1);
        --m_nCount;
        return nBit;
    }

private:
    const uchar *m_pData = nullptr;
    qint64 m_nSize = 0;
    qint64 m_nPosition = 0;
    quint8 m_nCurrent = 0;
    qint32 m_nCount = 0;
};

// One adaptive frequency model, exactly the CACM 1987 "start_model" /
// "update_model" pair with a configurable increment and rescale threshold.
struct RSVKModel {
    qint32 nBase = 0;
    qint32 nChars = 0;      // number of real characters
    qint32 nSymbols = 0;    // CACM No_of_symbols == nChars + 1 (one phantom)
    qint32 nMaxFrequency = 0;
    qint32 nIncrement = 1;
    qint32 charToIndex[RSVK_MAX_SYMBOLS + 2] = {};
    qint32 indexToChar[RSVK_MAX_SYMBOLS + 2] = {};
    qint32 frequency[RSVK_MAX_SYMBOLS + 2] = {};
    qint32 cumulative[RSVK_MAX_SYMBOLS + 2] = {};

    bool init(qint32 nLow, qint32 nHigh, qint32 nMaxFreq, qint32 nInc)
    {
        nBase = nLow;
        nChars = nHigh - nLow + 1;
        nSymbols = nChars + 1;
        if ((nChars <= 0) || (nSymbols > RSVK_MAX_SYMBOLS)) return false;
        nMaxFrequency = nMaxFreq;
        nIncrement = nInc;
        for (qint32 i = 0; i < nChars; ++i) {
            charToIndex[i] = i + 1;
            indexToChar[i + 1] = i;
        }
        for (qint32 i = 0; i <= nSymbols; ++i) {
            frequency[i] = 1;
            cumulative[i] = nSymbols - i;
        }
        frequency[0] = 0;
        return true;
    }

    void update(qint32 nIndex)
    {
        if (nMaxFrequency <= cumulative[0]) {
            qint32 nRunning = 0;
            for (qint32 i = nSymbols; i >= 0; --i) {
                frequency[i] = (frequency[i] + 1) / 2;
                cumulative[i] = nRunning;
                nRunning += frequency[i];
            }
        }
        qint32 i = nIndex;
        while ((i > 0) && (frequency[i] == frequency[i - 1])) --i;
        if (i < nIndex) {
            const qint32 nCharAtI = indexToChar[i];
            const qint32 nCharAtN = indexToChar[nIndex];
            indexToChar[i] = nCharAtN;
            indexToChar[nIndex] = nCharAtI;
            charToIndex[nCharAtI] = nIndex;
            charToIndex[nCharAtN] = i;
        }
        frequency[i] += nIncrement;
        while (i > 0) {
            --i;
            cumulative[i] += nIncrement;
        }
    }
};

class RSVKArith {
public:
    explicit RSVKArith(RSVKBits *pBits) : m_pBits(pBits)
    {
        m_nValue = 0;
        for (qint32 i = 0; i < 16; ++i) {
            m_nValue = m_nValue * 2 + m_pBits->readBit();
        }
        m_nLow = 0;
        m_nHigh = 0xffff;
    }

    // Returns the decoded character (0 .. model.nChars - 1), or -1 on a stream
    // that selects the model's phantom symbol.
    qint32 decode(RSVKModel *pModel)
    {
        const qint32 nRange = m_nHigh - m_nLow + 1;
        const qint32 nTotal = pModel->cumulative[0];
        if ((nRange <= 0) || (nTotal <= 0)) return -1;
        const qint32 nTarget = qint32(
            ((qint64(m_nValue - m_nLow) + 1) * nTotal - 1) / nRange);
        qint32 nIndex = 1;
        while ((nIndex <= pModel->nSymbols) &&
               (pModel->cumulative[nIndex] > nTarget)) {
            ++nIndex;
        }
        if (nIndex > pModel->nChars) return -1;
        const qint32 nCharacter = pModel->indexToChar[nIndex];
        m_nHigh = m_nLow +
                  qint32((qint64(nRange) * pModel->cumulative[nIndex - 1]) /
                         nTotal) - 1;
        m_nLow = m_nLow + qint32((qint64(nRange) * pModel->cumulative[nIndex]) /
                                 nTotal);
        for (;;) {
            if (m_nHigh >= 0x8000) {
                if (m_nLow < 0x8000) {
                    if ((m_nLow < 0x4000) || (m_nHigh > 0xbfff)) {
                        pModel->update(nIndex);
                        return nCharacter;
                    }
                    m_nValue -= 0x4000;
                    m_nLow -= 0x4000;
                    m_nHigh -= 0x4000;
                } else {
                    m_nValue -= 0x8000;
                    m_nLow -= 0x8000;
                    m_nHigh -= 0x8000;
                }
            }
            m_nLow = m_nLow * 2;
            m_nHigh = m_nHigh * 2 + 1;
            m_nValue = m_nValue * 2 + m_pBits->readBit();
        }
    }

private:
    RSVKBits *m_pBits = nullptr;
    qint32 m_nLow = 0;
    qint32 m_nHigh = 0;
    qint32 m_nValue = 0;
};

// Decodes one block's arithmetic stream into the MTF-index sequence.
bool rsvkDecodeSymbols(const uchar *pPayload, qint64 nPayloadSize,
                       std::vector<quint8> *pMtf, XBinary::PDSTRUCT *pPdStruct)
{
    RSVKBits bits(pPayload, nPayloadSize);
    RSVKArith arith(&bits);

    RSVKModel selector;
    if (!selector.init(0, RSVK_GROUPS - 1, RSVK_SELECTOR_MAX_FREQUENCY,
                       RSVK_SELECTOR_INCREMENT)) {
        return false;
    }
    RSVKModel groups[RSVK_GROUPS];
    for (qint32 i = 2; i < RSVK_GROUPS; ++i) {
        if (!groups[i].init(g_nGroupLow[i], g_nGroupHigh[i],
                            g_nGroupMaxFrequency[i], 1)) {
            return false;
        }
    }

    pMtf->clear();
    qint32 nGuard = 0;
    qint32 nSymbol = -1;
    bool bNeedSymbol = true;
    for (;;) {
        if (bNeedSymbol) {
            const qint32 nSelector = arith.decode(&selector);
            if (nSelector < 0) return false;
            if (nSelector < 2) {
                nSymbol = nSelector;
            } else {
                const qint32 nOffset = arith.decode(&groups[nSelector]);
                if (nOffset < 0) return false;
                nSymbol = groups[nSelector].nBase + nOffset;
            }
            bNeedSymbol = false;
        }
        if (((++nGuard & 0xffff) == 0) &&
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        if (nSymbol == RSVK_SYMBOL_EOB) break;
        if (nSymbol < 2) {
            // RUNA/RUNB bijective base-2 run of MTF zeroes.
            qint64 nRun = 0;
            qint64 nWeight = 1;
            for (;;) {
                nRun += (nSymbol == 0) ? nWeight : (nWeight * 2);
                nWeight *= 2;
                if ((nWeight > RSVK_MAX_MTF) || (nRun > RSVK_MAX_MTF)) {
                    return false;
                }
                const qint32 nSelector = arith.decode(&selector);
                if (nSelector < 0) return false;
                if (nSelector < 2) {
                    nSymbol = nSelector;
                    continue;
                }
                const qint32 nOffset = arith.decode(&groups[nSelector]);
                if (nOffset < 0) return false;
                nSymbol = groups[nSelector].nBase + nOffset;
                break;
            }
            if (qint64(pMtf->size()) + nRun > RSVK_MAX_MTF) return false;
            pMtf->insert(pMtf->end(), size_t(nRun), quint8(0));
            // nSymbol already holds the symbol that ended the run.
            continue;
        }
        if (qint32(pMtf->size()) >= RSVK_MAX_MTF) return false;
        pMtf->push_back(quint8((nSymbol - 1) & 0xff));
        bNeedSymbol = true;
    }
    return true;
}

bool rsvkInverse(const std::vector<quint8> &mtf, qint32 nPrimary, qint32 nHole,
                 QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    const qint32 nLength = qint32(mtf.size());
    if ((nHole < 0) || (nHole > nLength)) return false;
    const qint32 nRows = nLength + 1;
    if ((nRows <= 0) || (nRows > RSVK_MAX_ROWS)) return false;
    if ((nPrimary < 0) || (nPrimary >= nRows)) return false;

    // Inverse move-to-front, writing the BWT column with a gap left at nHole
    // for the sentinel row.
    std::vector<quint8> column(size_t(nRows), 0);
    quint8 table[256];
    quint8 position[256];
    for (qint32 i = 0; i < 256; ++i) {
        table[i] = quint8(i);
        position[i] = quint8(i);
    }
    // counts[0] is the sentinel, counts[1 + b] the byte b.
    std::vector<qint32> counts(257, 0);
    counts[0] = 1;
    qint32 nOut = 0;
    for (qint32 nIn = 0; nIn < nLength; ++nIn) {
        if (((nIn & 0xffff) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        if (nIn >= nHole) {
            // The sentinel row is skipped, never written.
            if (nIn == nHole) ++nOut;
        }
        const quint8 nRank = mtf[size_t(nIn)];
        const quint8 nValue = table[nRank];
        const quint8 nWhere = position[nValue];
        if (nWhere != 0) {
            for (qint32 k = nWhere; k != 0; --k) {
                table[k] = table[k - 1];
                position[table[k]] = quint8(k);
            }
            table[0] = nValue;
            position[nValue] = 0;
        }
        if ((nOut < 0) || (nOut >= nRows)) return false;
        column[size_t(nOut)] = nValue;
        ++counts[1 + nValue];
        ++nOut;
    }

    // Inverse Burrows-Wheeler.  The sentinel sorts before every byte, which is
    // why its running count starts at 1 and its bucket base is 0.
    std::vector<qint32> base(257, 0);
    qint32 nRunning = 0;
    for (qint32 i = 0; i < 257; ++i) {
        base[i] = nRunning;
        nRunning += counts[i];
        counts[i] = 0;
    }
    std::vector<qint32> succ(size_t(nRows), 0);
    for (qint32 i = 0; i < nHole; ++i) {
        const quint8 b = column[size_t(i)];
        const qint32 nIndex = counts[1 + b] + base[1 + b];
        if ((nIndex < 0) || (nIndex >= nRows)) return false;
        succ[size_t(nIndex)] = i;
        ++counts[1 + b];
    }
    {
        const qint32 nIndex = counts[0] + base[0];
        if ((nIndex < 0) || (nIndex >= nRows)) return false;
        succ[size_t(nIndex)] = nHole;
    }
    for (qint32 i = nHole + 1; i < nRows; ++i) {
        const quint8 b = column[size_t(i)];
        const qint32 nIndex = counts[1 + b] + base[1 + b];
        if ((nIndex < 0) || (nIndex >= nRows)) return false;
        succ[size_t(nIndex)] = i;
        ++counts[1 + b];
    }

    // The walk produces nRows characters; the last one is the sentinel row's
    // undefined byte and is dropped.
    QByteArray baResult(nLength, 0);
    qint32 p = nPrimary;
    for (qint32 i = 0; i < nRows; ++i) {
        if (((i & 0xffff) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        if ((p < 0) || (p >= nRows)) return false;
        if (i < nLength) baResult[i] = char(column[size_t(p)]);
        p = succ[size_t(p)];
    }
    *pOutput = baResult;
    return true;
}

}  // namespace

bool XRSVKDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize,
                          QByteArray *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || (nUncompressedSize < 0)) return false;
    if (nUncompressedSize == 0) {
        *pOutput = QByteArray();
        return baPacked.isEmpty() ||
               (baPacked.size() >= RSVK_BLOCK_HEADER_SIZE);
    }

    QByteArray baResult;
    baResult.reserve(qint32(qMin<qint64>(nUncompressedSize, 0x40000000)));

    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());
    qint64 nCursor = 0;
    while (nCursor <= baPacked.size() - RSVK_BLOCK_HEADER_SIZE) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pHeader = pData + nCursor;
        if (memcmp(pHeader, "DATA", 4) != 0) break;
        const quint32 nExpectedCrc = qFromLittleEndian<quint32>(pHeader + 4);
        const qint64 nPacked =
            static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 8));
        const qint32 nPrimary =
            static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 12));
        const qint32 nHole =
            static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 16));
        if ((nPacked <= 0) ||
            (nPacked > baPacked.size() - nCursor - RSVK_BLOCK_HEADER_SIZE)) {
            return false;
        }

        std::vector<quint8> mtf;
        if (!rsvkDecodeSymbols(pHeader + RSVK_BLOCK_HEADER_SIZE, nPacked, &mtf,
                               pPdStruct)) {
            return false;
        }
        QByteArray baBlock;
        if (!rsvkInverse(mtf, nPrimary, nHole, &baBlock, pPdStruct)) {
            return false;
        }
        if (rsvkCrc32(reinterpret_cast<const uchar *>(baBlock.constData()),
                      baBlock.size()) != nExpectedCrc) {
            return false;
        }
        if (qint64(baResult.size()) + baBlock.size() > nUncompressedSize) {
            return false;
        }
        baResult.append(baBlock);
        nCursor += RSVK_BLOCK_HEADER_SIZE + nPacked;
    }

    if (qint64(baResult.size()) != nUncompressedSize) return false;
    *pOutput = baResult;
    return true;
}
