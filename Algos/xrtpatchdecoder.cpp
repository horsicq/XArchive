/*
 * RTPatch adaptive Huffman/LZSS decoder.
 *
 * This is a bounded C++ adaptation of the independently reverse-engineered
 * rtptool codec (src/codec.rs), commit
 * 258d1750340917bcf1361a177b90abd16de40453.  The original implementation is
 * Copyright (c) 2026 Sandy Carter and distributed under the MIT License.
 * Project adaptation Copyright (c) 2026 hors<horsicq@gmail.com>.
 *
 * See Algos/licenses/rtptool.txt for the full upstream notice and provenance.
 */
#include "xrtpatchdecoder.h"

#include <limits>

namespace {
const quint32 RTPATCH_DIFF_MAGIC = 0xb59c;
const quint32 RTPATCH_WINDOW_FLAG_8K = 8;

class RTPatchBitInput
{
public:
    explicit RTPatchBitInput(const QByteArray &data)
        : m_data(data), m_nEnd(data.size())
    {
    }

    bool readBit(quint32 *pValue)
    {
        if (!pValue || (m_nPosition < 0) ||
            (m_nPosition >= m_nEnd) || (m_nBitsLeft < 1) ||
            (m_nBitsLeft > 8)) {
            return false;
        }
        const quint8 nByte = quint8(m_data.at(qint32(m_nPosition)));
        *pValue = (nByte >> (m_nBitsLeft - 1)) & 1U;
        --m_nBitsLeft;
        if (m_nBitsLeft == 0) {
            ++m_nPosition;
            m_nBitsLeft = 8;
        }
        return true;
    }

    bool readBits(qint32 nCount, quint32 *pValue)
    {
        if (!pValue || (nCount < 0) || (nCount > 24)) return false;
        quint32 nValue = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            quint32 nBit = 0;
            if (!readBit(&nBit)) return false;
            nValue = (nValue << 1) | nBit;
        }
        *pValue = nValue;
        return true;
    }

    qint64 position() const { return m_nPosition; }
    qint64 end() const { return m_nEnd; }
    qint32 bitsLeft() const { return m_nBitsLeft; }

    bool setCursor(qint64 nPosition, qint32 nBitsLeft)
    {
        if ((nPosition < 0) || (nPosition > m_nEnd) ||
            (nBitsLeft < 1) || (nBitsLeft > 8)) {
            return false;
        }
        m_nPosition = nPosition;
        m_nBitsLeft = nBitsLeft;
        return true;
    }

private:
    const QByteArray &m_data;
    qint64 m_nPosition = 0;
    qint64 m_nEnd = 0;
    qint32 m_nBitsLeft = 8;
};

class RTPatchHuffmanTree
{
public:
    bool initialize(qint32 nEscapeBits, qint32 nLevels,
                    quint32 nInitialPeriod, quint32 nUpdatePeriod)
    {
        if ((nEscapeBits < 1) || (nEscapeBits > 8) ||
            (nLevels < 2) || (nLevels > 24) ||
            (nInitialPeriod == 0) || (nUpdatePeriod == 0) ||
            (nInitialPeriod > 0xffff) || (nUpdatePeriod > 0xffff)) {
            return false;
        }
        m_bValid = true;
        m_nAlphabetSize = 1U << nEscapeBits;
        const quint32 nGroupCountOffset = 0x34;
        const quint32 nSymbolTableOffset = quint32(nLevels * 2 + 0x34);
        const quint32 nSlotOffset = quint32(nLevels * 6 + 0x38);
        const quint32 nWeightOffset =
            quint32(nLevels * 6 + 0x40) + m_nAlphabetSize * 4;
        const quint32 nLimitOffset =
            nWeightOffset + 4 + m_nAlphabetSize * 2;
        const quint32 nDataSize = nLimitOffset + 0x1c0;
        if (nDataSize > quint32((std::numeric_limits<qint32>::max)())) {
            return false;
        }
        m_data = QByteArray(qint32(nDataSize), 0);

        write16(0x32, nInitialPeriod);
        write16(0x02, nInitialPeriod);
        write16(0x00, nInitialPeriod);
        write16(0x30, nUpdatePeriod);
        write16(0x2e, nUpdatePeriod);
        write16(0x2c, nUpdatePeriod);
        write16(0x06, nEscapeBits);
        write16(0x04, nLevels);
        write32(0x0c, nGroupCountOffset);
        write32(0x20, nSymbolTableOffset);
        write32(0x1c, nSlotOffset);
        write32(0x18, nWeightOffset);
        write32(0x14, nLimitOffset);
        write32(0x10, nLimitOffset);

        write16(0x0a, 1);
        write16(0x08, 1);
        write16(nGroupCountOffset, 2);
        for (qint32 i = 1; i < nLevels; ++i) {
            write16(nGroupCountOffset + quint32(i * 2), 0);
        }
        write32(nSymbolTableOffset, nSlotOffset);
        for (qint32 i = 1; i <= nLevels; ++i) {
            write32(nSymbolTableOffset + quint32(i * 4), nSlotOffset + 8);
        }

        const quint32 nWeightBase =
            nWeightOffset + m_nAlphabetSize * 2;
        write32(nSlotOffset, nWeightBase);
        for (quint32 i = 1; i <= m_nAlphabetSize + 1; ++i) {
            write32(nSlotOffset + i * 4, nWeightBase + 2);
        }
        for (quint32 i = 0; i < m_nAlphabetSize; ++i) {
            write16(nWeightOffset + i * 2, 0x8000);
        }
        write16(nWeightOffset + m_nAlphabetSize * 2, 0);
        write16(nWeightOffset + m_nAlphabetSize * 2 + 2, 0);
        write16(0x24, m_nAlphabetSize);
        for (qint32 i = 0; i < 0x30; ++i) {
            write32(nLimitOffset + quint32(i * 4), 0);
        }
        buildLimits(0);
        return m_bValid;
    }

    bool decode(RTPatchBitInput *pInput, quint16 *pSymbol)
    {
        if (!pInput || !pSymbol || !m_bValid ||
            (pInput->position() >= pInput->end())) {
            return false;
        }
        qint32 nBitsLeft = pInput->bitsLeft();
        const quint8 nCurrent =
            quint8(m_inputByte(pInput->position(), pInput));
        if (!m_bValid) return false;
        qint32 nValue = ((1 << nBitsLeft) - 1) & nCurrent;
        qint32 nIndex = nBitsLeft - 1;
        qint32 nTotalBits = nBitsLeft;
        const quint32 nLimitOffset = read32(0x10);
        if (!m_bValid) return false;

        if (quint32(nValue) <
            read16u(nLimitOffset + quint32(nIndex * 8))) {
            do {
                if (!pInput->setCursor(pInput->position() + 1,
                                       pInput->bitsLeft()) ||
                    (pInput->position() >= pInput->end())) {
                    return false;
                }
                nIndex += 8;
                nTotalBits += 8;
                const quint8 nNext =
                    quint8(m_inputByte(pInput->position(), pInput));
                nValue = (((nValue & 0xff) << 8) | nNext) & 0xffff;
            } while (quint32(nValue) <
                     read16u(nLimitOffset + quint32(nIndex * 8)));
        }
        if (!m_bValid) return false;

        --nIndex;
        qint32 nCount = (nTotalBits - 1) & 0xff;
        qint32 nNewBitsLeft = 0;
        while (nCount != 0) {
            if (nIndex < 0) return false;
            const qint64 nThresholdOffset =
                qint64(nLimitOffset) + 2 + qint64(nIndex) * 8;
            const qint32 nThreshold = read16u(nThresholdOffset);
            if (!m_bValid || (nValue < nThreshold)) break;
            --nIndex;
            --nCount;
            nValue >>= 1;
            ++nNewBitsLeft;
        }
        if (nIndex < -1) return false;

        const quint32 nSymbolTableOffset = read32(0x20);
        const quint32 nWeightOffset = read32(0x18);
        const quint32 nLevel = quint32((nIndex + 1) & 0xffff);
        if (!m_bValid || (nLevel > read16u(0x04))) return false;
        const quint32 nSlotArray =
            read32(nSymbolTableOffset + nLevel * 4);
        const qint32 nBase = read16s(nLimitOffset + nLevel * 8);
        const quint32 nSlotIndex = quint32((nValue - nBase) & 0xffff);
        const quint32 nWeightPointer =
            read32(nSlotArray + nSlotIndex * 4);
        if (!m_bValid || (nWeightPointer < nWeightOffset) ||
            ((nWeightPointer - nWeightOffset) & 1U)) {
            return false;
        }
        const quint32 nSymbol =
            (nWeightPointer - nWeightOffset) >> 1;
        if (nSymbol > m_nAlphabetSize) return false;

        if (nNewBitsLeft == 0) {
            nNewBitsLeft = 8;
            if (!pInput->setCursor(pInput->position() + 1,
                                   nNewBitsLeft)) {
                return false;
            }
        } else if (!pInput->setCursor(pInput->position(), nNewBitsLeft)) {
            return false;
        }

        if (updateFrequency(nSymbol)) {
            rebuild();
            buildLimits(0);
        }
        if (!m_bValid) return false;

        if (nSymbol == read16u(0x24)) {
            quint32 nRawSymbol = 0;
            if (!pInput->readBits(read16u(0x06), &nRawSymbol) ||
                (nRawSymbol >= m_nAlphabetSize)) {
                return false;
            }
            const qint32 nAffectedLevel = addSymbol(nRawSymbol);
            if ((nAffectedLevel < 0) || !m_bValid) return false;
            buildLimits(nAffectedLevel);
            if (!m_bValid) return false;
            *pSymbol = quint16(nRawSymbol);
            return true;
        }

        *pSymbol = quint16(nSymbol);
        return true;
    }

private:
    bool contains(qint64 nOffset, qint64 nSize) const
    {
        return (nOffset >= 0) && (nSize >= 0) &&
               (nOffset <= m_data.size()) &&
               (nSize <= m_data.size() - nOffset);
    }

    char m_inputByte(qint64 nOffset, RTPatchBitInput *pInput)
    {
        Q_UNUSED(pInput)
        // The bit reader owns the compressed input, not this model buffer.
        // This placeholder is never reached directly; decodeInputByte below
        // is selected through the bound callback state.
        if (!m_pPacked || (nOffset < 0) || (nOffset >= m_pPacked->size())) {
            m_bValid = false;
            return 0;
        }
        return m_pPacked->at(qint32(nOffset));
    }

public:
    void bindInput(const QByteArray *pPacked) { m_pPacked = pPacked; }

private:
    quint16 read16u(qint64 nOffset)
    {
        if (!contains(nOffset, 2)) {
            m_bValid = false;
            return 0;
        }
        const uchar *p = reinterpret_cast<const uchar *>(m_data.constData());
        return quint16(p[nOffset]) | (quint16(p[nOffset + 1]) << 8);
    }

    qint16 read16s(qint64 nOffset)
    {
        return qint16(read16u(nOffset));
    }

    quint32 read32(qint64 nOffset)
    {
        if (!contains(nOffset, 4)) {
            m_bValid = false;
            return 0;
        }
        const uchar *p = reinterpret_cast<const uchar *>(m_data.constData());
        return quint32(p[nOffset]) |
               (quint32(p[nOffset + 1]) << 8) |
               (quint32(p[nOffset + 2]) << 16) |
               (quint32(p[nOffset + 3]) << 24);
    }

    void write16(qint64 nOffset, qint64 nValue)
    {
        if (!contains(nOffset, 2)) {
            m_bValid = false;
            return;
        }
        const quint16 nWord = quint16(nValue & 0xffff);
        m_data[qint32(nOffset)] = char(nWord & 0xff);
        m_data[qint32(nOffset + 1)] = char(nWord >> 8);
    }

    void write32(qint64 nOffset, quint32 nValue)
    {
        if (!contains(nOffset, 4)) {
            m_bValid = false;
            return;
        }
        m_data[qint32(nOffset)] = char(nValue & 0xff);
        m_data[qint32(nOffset + 1)] = char((nValue >> 8) & 0xff);
        m_data[qint32(nOffset + 2)] = char((nValue >> 16) & 0xff);
        m_data[qint32(nOffset + 3)] = char(nValue >> 24);
    }

    void buildLimits(qint32 nStart)
    {
        const quint32 nGroupCountOffset = read32(0x0c);
        const quint32 nLimitOffset = read32(0x10);
        const qint32 nLevels = read16u(0x04);
        if (!m_bValid || (nStart < 0) || (nStart > nLevels)) {
            m_bValid = false;
            return;
        }
        qint32 nAccumulator = (nStart == 0)
            ? 2 : qint32(read16s(nLimitOffset + (nStart - 1) * 8)) * 2;
        for (qint32 nLevel = nStart; nLevel < nLevels; ++nLevel) {
            const qint32 nValue = nAccumulator -
                read16s(nGroupCountOffset + nLevel * 2);
            write16(nLimitOffset + nLevel * 8, nValue);
            nAccumulator = nValue * 2;
            write16(nLimitOffset + nLevel * 8 + 2, nAccumulator);
            write16(nLimitOffset + nLevel * 8 + 4, nValue * 4);
            write16(nLimitOffset + nLevel * 8 + 6, nValue * 16);
        }
    }

    bool updateFrequency(quint32 nSymbol)
    {
        if (nSymbol > m_nAlphabetSize) {
            m_bValid = false;
            return false;
        }
        const quint32 nWeightOffset = read32(0x18);
        const quint32 nWeight = read16u(nWeightOffset + nSymbol * 2);
        write16(nWeightOffset + nSymbol * 2, nWeight + 1);
        const qint32 nCounter = read16s(0x00) - 1;
        write16(0x00, nCounter);
        return quint16(nCounter) == 0;
    }

    qint32 addSymbol(quint32 nNewSymbol)
    {
        if (nNewSymbol >= m_nAlphabetSize) return -1;
        const quint32 nWeightOffset = read32(0x18);
        const quint32 nSlotOffset = read32(0x1c);
        const quint32 nGroupCountOffset = read32(0x0c);
        const quint32 nSymbolTableOffset = read32(0x20);
        write16(nWeightOffset + nNewSymbol * 2, 1);
        quint32 nSlotCount = read16u(0x08);
        if (!m_bValid || (nSlotCount > m_nAlphabetSize)) return -1;
        write32(nSlotOffset + nSlotCount * 4,
                nWeightOffset + nNewSymbol * 2);
        ++nSlotCount;
        write16(0x08, nSlotCount);
        if (nSlotCount == 2) return m_bValid ? 0 : -1;

        quint32 nGroupCount = read16u(0x0a);
        const quint32 nLevels = read16u(0x04);
        if (!m_bValid || (nGroupCount == 0) ||
            (nGroupCount > nLevels)) {
            return -1;
        }
        quint32 nGroup = 0;
        if (nGroupCount < nLevels) {
            nGroup = (nGroupCount - 1) & 0xffff;
            write16(0x0a, nGroupCount + 1);
        } else {
            if (nGroupCount < 2) return -1;
            nGroup = (nGroupCount - 2) & 0xffff;
            for (quint32 i = 0;
                 (i < nLevels) &&
                 (read16s(nGroupCountOffset + nGroup * 2) == 0);
                 ++i) {
                if (nGroup == 0) return -1;
                nGroup = (nGroup - 1) & 0xffff;
            }
        }
        write16(nGroupCountOffset + nGroup * 2,
                read16s(nGroupCountOffset + nGroup * 2) - 1);
        write16(nGroupCountOffset + (nGroup + 1) * 2,
                read16s(nGroupCountOffset + (nGroup + 1) * 2) + 2);
        const quint32 nNext =
            read32(nSymbolTableOffset + (nGroup + 1) * 4);
        if (nNext < 4) return -1;
        write32(nSymbolTableOffset + (nGroup + 1) * 4, nNext - 4);
        for (quint32 i = nGroup + 2; i <= nLevels; ++i) {
            write32(nSymbolTableOffset + i * 4,
                    read32(nSymbolTableOffset + i * 4) + 4);
        }
        return m_bValid ? qint32(nGroup) : -1;
    }

    void rebuild()
    {
        const quint32 nSlotOffset = read32(0x1c);
        const quint32 nSymbolTableOffset = read32(0x20);
        const quint32 nGroupCountOffset = read32(0x0c);
        const quint32 nSlotCount = read16u(0x08);
        const qint32 nUpdateCounter = read16s(0x2c);
        const quint32 nLevels = read16u(0x04);
        if (!m_bValid || (nSlotCount > m_nAlphabetSize + 1) ||
            (nLevels == 0) || (nLevels > 24)) {
            m_bValid = false;
            return;
        }
        write16(0x2c, nUpdateCounter - 1);

        quint32 nMaximumWeight = 0;
        for (quint32 i = 0; i < nSlotCount; ++i) {
            const quint32 nPointer = read32(nSlotOffset + i * 4);
            quint32 nWeight = read16u(nPointer);
            if (quint16(nUpdateCounter - 1) == 0) {
                nWeight >>= 1;
                write16(nPointer, nWeight);
            }
            nMaximumWeight = qMax(nMaximumWeight, nWeight);
        }
        if (!m_bValid) return;

        if (nMaximumWeight != 0) {
            quint32 nMask = 0x8000;
            for (qint32 guard = 0;
                 ((nMaximumWeight & nMask) == 0) && (guard < 16);
                 ++guard) {
                nMask = (nMask >> 1) | 0x8000;
            }
            quint32 nPosition = 0;
            bool bDone = false;
            while ((nPosition < nSlotCount) && !bDone && m_bValid) {
                const quint32 nCurrentPointer =
                    read32(nSlotOffset + nPosition * 4);
                if ((read16u(nCurrentPointer) & nMask) == 0) {
                    quint32 nScan = nPosition + 1;
                    quint32 nInsertion = nPosition;
                    if (nScan >= nSlotCount) break;
                    quint32 nLastInsertion = nInsertion;
                    while (nScan < nSlotCount) {
                        const quint32 nScanSlot = nSlotOffset + nScan * 4;
                        const quint32 nScanPointer = read32(nScanSlot);
                        nLastInsertion = nInsertion;
                        if ((read16u(nScanPointer) & nMask) != 0) {
                            nLastInsertion = nInsertion + 1;
                            const quint32 nSaved =
                                read32(nSlotOffset + nInsertion * 4);
                            write32(nSlotOffset + nInsertion * 4,
                                    nScanPointer);
                            write32(nScanSlot, nSaved);
                        }
                        ++nScan;
                        nInsertion = nLastInsertion;
                    }
                    if (nLastInsertion != nPosition) {
                        nPosition = nLastInsertion - 1;
                    }
                    const quint32 nNextMask = nMask >> 1;
                    nMask = nNextMask | 0x8000;
                    if (nNextMask & 1U) bDone = true;
                } else {
                    ++nPosition;
                }
            }
        }
        if (!m_bValid) return;

        quint32 nLevel = 0;
        quint32 nGroupCount = read16u(0x0a);
        if ((nGroupCount == 0) || (nGroupCount > nLevels)) {
            m_bValid = false;
            return;
        }
        quint32 nLastGroup = (nGroupCount - 1) & 0xffff;
        qint32 nMoved = 0;
        qint32 nGuard = 0;
        while ((nLevel < nGroupCount) && m_bValid) {
            if (++nGuard > 65536) {
                m_bValid = false;
                return;
            }
            const quint32 nGroupOffset = nGroupCountOffset + nLevel * 2;
            const quint32 nTableOffset = nSymbolTableOffset + nLevel * 4;
            const quint32 nGroupSize = read16u(nGroupOffset);
            if (nGroupSize == 0) {
                ++nLevel;
                continue;
            }
            const quint32 nTableEnd = read32(nTableOffset + 4);
            if (nTableEnd < 8) {
                m_bValid = false;
                return;
            }
            const quint32 nFirstSlot = read32(nTableOffset);
            const quint32 nFirstWeightPointer = read32(nFirstSlot);
            const quint32 nLastWeightPointer = read32(nTableEnd - 4);
            const quint32 nPreviousWeightPointer = read32(nTableEnd - 8);
            const quint32 nFirstWeight = read16u(nFirstWeightPointer);
            const quint32 nLastWeight = read16u(nLastWeightPointer);
            const quint32 nPreviousWeight = read16u(nPreviousWeightPointer);

            if ((nGroupSize < 3) ||
                (((nLevels - 1) & 0xffff) == nLevel) ||
                (nFirstWeight < nLastWeight + nPreviousWeight)) {
                quint32 nMovingTableOffset = nTableOffset + 4;
                bool bFound = false;
                qint32 nAccumulator = qint32(nLastWeight);
                quint32 nTargetLevel = nLevel + 2;
                while (nTargetLevel < nGroupCount) {
                    const quint32 nTargetTable =
                        nSymbolTableOffset + nTargetLevel * 4;
                    const quint32 nTargetSlot = read32(nTargetTable);
                    nAccumulator =
                        (nAccumulator - read16s(read32(nTargetSlot))) &
                        0xffff;
                    const quint32 nTargetGroupSize =
                        read16u(nGroupCountOffset + nTargetLevel * 2);
                    const quint32 nNextSlot = read32(nTargetSlot + 4);
                    const quint32 nNextWeight = read16u(nNextSlot);
                    if ((nTargetGroupSize > 1) &&
                        ((nAccumulator & 0x8000) ||
                         (quint32(nAccumulator) < nNextWeight))) {
                        bFound = true;
                        break;
                    }
                    ++nTargetLevel;
                }
                if (!bFound) {
                    ++nLevel;
                    continue;
                }

                write16(nGroupOffset, read16s(nGroupOffset) - 1);
                ++nMoved;
                ++nLevel;
                const quint32 nMovingValue = read32(nMovingTableOffset);
                if (nMovingValue < 4) {
                    m_bValid = false;
                    return;
                }
                write32(nMovingTableOffset, nMovingValue - 4);
                write16(nGroupCountOffset + nLevel * 2,
                        read16s(nGroupCountOffset + nLevel * 2) + 2);
                write32(nTableOffset + 8,
                        read32(nTableOffset + 8) + 4);
                const quint32 nBeforeTarget =
                    (nTargetLevel - 1) & 0xffff;
                if (nLevel < nBeforeTarget) {
                    const quint32 nSpan = nBeforeTarget - nLevel;
                    nLevel += nSpan;
                    for (quint32 i = 0; i < nSpan; ++i) {
                        write32(nMovingTableOffset + 8,
                                read32(nMovingTableOffset + 8) + 4);
                        nMovingTableOffset += 4;
                    }
                }
                write16(nGroupCountOffset + nLevel * 2,
                        read16s(nGroupCountOffset + nLevel * 2) + 1);
                write32(nMovingTableOffset + 4,
                        read32(nMovingTableOffset + 4) + 4);
                write16(nGroupCountOffset + (nLevel + 1) * 2,
                        read16s(nGroupCountOffset + (nLevel + 1) * 2) - 2);
                if (read16s(nGroupCountOffset + nLastGroup * 2) == 0) {
                    if (nGroupCount == 0) {
                        m_bValid = false;
                        return;
                    }
                    --nGroupCount;
                    write16(0x0a, nGroupCount);
                    nLastGroup = (nLastGroup - 1) & 0xffff;
                }
                nLevel = 0;
            } else {
                if (nLevel == 0) {
                    m_bValid = false;
                    return;
                }
                ++nMoved;
                write16(nGroupOffset - 2,
                        read16s(nGroupOffset - 2) + 1);
                write16(nGroupOffset, read16s(nGroupOffset) - 3);
                write16(nGroupOffset + 2,
                        read16s(nGroupOffset + 2) + 2);
                write32(nTableOffset, read32(nTableOffset) + 4);
                const quint32 nEnd = read32(nTableOffset + 4);
                if (nEnd < 8) {
                    m_bValid = false;
                    return;
                }
                write32(nTableOffset + 4, nEnd - 8);
                if (nLastGroup == nLevel) {
                    ++nGroupCount;
                    if (nGroupCount > nLevels) {
                        m_bValid = false;
                        return;
                    }
                    write16(0x0a, nGroupCount);
                    nLastGroup = (nLastGroup + 1) & 0xffff;
                }
                nLevel = 0;
            }
        }
        if (!m_bValid) return;

        if (nMoved < 0x10) {
            if ((nMoved < 8) && (read16u(0x2e) != 1)) {
                write16(0x02, quint32(read16u(0x02)) << 1);
                write16(0x2c, read16u(0x2c) >> 1);
                write16(0x2e, read16u(0x2e) >> 1);
            }
        } else {
            write16(0x02, read16u(0x32));
            write16(0x2e, read16u(0x30));
        }
        write16(0x00, read16u(0x02));
        if (read16u(0x2c) == 0) write16(0x2c, read16u(0x2e));
    }

    QByteArray m_data;
    const QByteArray *m_pPacked = nullptr;
    quint32 m_nAlphabetSize = 0;
    bool m_bValid = false;
};
}  // namespace

bool XRTPatchDecoder::decode(const QByteArray &packed, qint64 expectedSize,
                             QByteArray *output,
                             XBinary::PDSTRUCT *pPdStruct)
{
    if (!output || (packed.size() < 8) || (expectedSize < 0) ||
        (expectedSize > (std::numeric_limits<qint32>::max)()) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    RTPatchBitInput input(packed);
    quint32 nMagic = 0;
    quint32 nUseRawLiterals = 0;
    quint32 nReserved = 0;
    quint32 nInitialPeriod = 0;
    quint32 nUpdatePeriod = 0;
    quint32 nWindowFlag = 0;
    if (!input.readBits(16, &nMagic) || (nMagic != RTPATCH_DIFF_MAGIC) ||
        !input.readBits(8, &nUseRawLiterals) ||
        !input.readBits(8, &nReserved) ||
        !input.readBits(12, &nInitialPeriod) ||
        !input.readBits(12, &nUpdatePeriod) ||
        !input.readBits(4, &nWindowFlag) ||
        (nUseRawLiterals > 1) || (nReserved != 0xff) ||
        (nInitialPeriod == 0) || (nUpdatePeriod == 0)) {
        return false;
    }

    const qint32 nDistanceBits =
        (nWindowFlag == RTPATCH_WINDOW_FLAG_8K) ? 7 : 6;
    RTPatchHuffmanTree literalTree;
    RTPatchHuffmanTree lengthTree;
    RTPatchHuffmanTree distanceTree;
    if (((nUseRawLiterals == 0) &&
         !literalTree.initialize(8, 0x10, nInitialPeriod,
                                 nUpdatePeriod)) ||
        !lengthTree.initialize(6, 0x0c, nInitialPeriod, nUpdatePeriod) ||
        !distanceTree.initialize(6, 0x0c, nInitialPeriod,
                                 nUpdatePeriod)) {
        return false;
    }
    literalTree.bindInput(&packed);
    lengthTree.bindInput(&packed);
    distanceTree.bindInput(&packed);

    QByteArray decoded;
    decoded.reserve(qint32(expectedSize));
    while ((decoded.size() < expectedSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint32 nFlag = 0;
        if (!input.readBit(&nFlag)) return false;
        if (nFlag == 0) {
            quint32 nSymbol = 0;
            if (nUseRawLiterals == 0) {
                quint16 nDecodedSymbol = 0;
                if (!literalTree.decode(&input, &nDecodedSymbol)) return false;
                nSymbol = nDecodedSymbol;
            } else if (!input.readBits(8, &nSymbol)) {
                return false;
            }
            if (nSymbol > 0xff) return false;
            decoded.append(char(nSymbol));
        } else {
            quint32 nDistanceLow = 0;
            quint16 nDistanceHigh = 0;
            if (!input.readBits(nDistanceBits, &nDistanceLow) ||
                !distanceTree.decode(&input, &nDistanceHigh)) {
                return false;
            }
            const quint32 nDistance =
                (quint32(nDistanceHigh) << nDistanceBits) | nDistanceLow;
            if (nDistance == 0) break;
            quint16 nLengthSymbol = 0;
            if (!lengthTree.decode(&input, &nLengthSymbol)) return false;
            const qint32 nLength = nLengthSymbol & 0x7f;
            if (nLength > expectedSize - decoded.size()) return false;
            const qint32 nBack = qint32(nDistance + 1);
            for (qint32 i = 0; i < nLength; ++i) {
                const qint32 nPosition = decoded.size();
                decoded.append((nPosition >= nBack)
                                   ? decoded.at(nPosition - nBack) : '\0');
            }
        }
        if ((decoded.size() & 0xffff) == 0 &&
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
    }

    if ((decoded.size() != expectedSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    *output = decoded;
    return true;
}
