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
#include "xdsquantumdecoder.h"

#include <QVector>
#include <QtEndian>

namespace {

const qint32 QDS_POSITION_SLOTS = 42;
const qint32 QDS_NEW_LENGTH_SLOTS = 27;
const qint32 QDS_OLD_LENGTH_SLOTS = 29;
const quint32 QDS_REBUILD_LIMIT = 0xed8U;  // total frequency ceiling before a rescale
const qint32 QDS_MIN_WINDOW_BITS = 10;
const qint32 QDS_MAX_WINDOW_BITS = 21;

struct QDS_SYMBOL {
    quint16 nSymbol;
    quint16 nCumFreq;
};

struct QDS_MODEL {
    qint32 nEntries;
    qint32 nShiftsLeft;
    QVector<QDS_SYMBOL> listSymbols;  // nEntries + 1 long at construction time
};

// position_base / extra_bits are shared by both variants; the length tables are
// not (27 slots stepping by (i-2)>>2 for the new shape, 29 slots stepping by
// (i-8)>>2 for the old one).
struct QDS_TABLES {
    quint32 nPositionBase[QDS_POSITION_SLOTS];
    quint8 nPositionExtra[QDS_POSITION_SLOTS];
    quint32 nNewLengthBase[QDS_NEW_LENGTH_SLOTS];
    quint8 nNewLengthExtra[QDS_NEW_LENGTH_SLOTS];
    quint32 nOldLengthBase[QDS_OLD_LENGTH_SLOTS];
    quint8 nOldLengthExtra[QDS_OLD_LENGTH_SLOTS];

    QDS_TABLES()
    {
        quint32 nBase = 0;
        for (qint32 i = 0; i < QDS_POSITION_SLOTS; i++) {
            nPositionExtra[i] = (i < 2) ? 0 : quint8((i - 2) >> 1);
            nPositionBase[i] = nBase;
            nBase += (1U << nPositionExtra[i]);
        }
        nBase = 0;
        for (qint32 i = 0; i < QDS_NEW_LENGTH_SLOTS - 1; i++) {
            nNewLengthExtra[i] = (i < 2) ? 0 : quint8((i - 2) >> 2);
            nNewLengthBase[i] = nBase;
            nBase += (1U << nNewLengthExtra[i]);
        }
        nNewLengthExtra[QDS_NEW_LENGTH_SLOTS - 1] = 0;
        nNewLengthBase[QDS_NEW_LENGTH_SLOTS - 1] = 254;
        nBase = 0;
        for (qint32 i = 0; i < QDS_OLD_LENGTH_SLOTS; i++) {
            // 0 for the first eight slots, then 1,1,1,1, 2,2,2,2 ... 5,5,5,5
            // and a final flat slot: {0 x8, 1 x4, 2 x4, 3 x4, 4 x4, 5 x4, 0}.
            nOldLengthExtra[i] = (i < 8) ? 0 : quint8(((i - 8) >> 2) + 1);
            if (i == QDS_OLD_LENGTH_SLOTS - 1) nOldLengthExtra[i] = 0;
            nOldLengthBase[i] = nBase;
            nBase += (1U << nOldLengthExtra[i]);
        }
    }
};

void modelInit(QDS_MODEL *pModel, qint32 nStart, qint32 nCount)
{
    pModel->nEntries = nCount;
    pModel->nShiftsLeft = 4;
    pModel->listSymbols.resize(nCount + 1);
    for (qint32 i = 0; i <= nCount; i++) {
        pModel->listSymbols[i].nSymbol = quint16(nStart + i);
        pModel->listSymbols[i].nCumFreq = quint16(nCount - i);
    }
}

// The old variant seeds four of its models with a decaying weight (8, 5, 3, 2,
// 2, ...) instead of the uniform "entries - i" ramp.
void modelWeightInit(QDS_MODEL *pModel)
{
    const qint32 nCount = pModel->nEntries;
    if ((nCount < 0) || (nCount >= pModel->listSymbols.size())) return;
    pModel->listSymbols[nCount].nCumFreq = 0;
    qint32 nWeight = 8;
    for (qint32 i = nCount - 1; i >= 0; i--) {
        pModel->listSymbols[i].nCumFreq = quint16(pModel->listSymbols[i + 1].nCumFreq + nWeight);
        nWeight = nWeight / 2 + 1;
    }
}

class QDSDecoder {
public:
    QDSDecoder(const QByteArray &baPacked, qint32 nWindowBits, bool bOldVariant)
        : m_pData(reinterpret_cast<const quint8 *>(baPacked.constData())), m_nDataSize(baPacked.size()), m_nDataPos(0), m_nBitBuffer(0), m_nBitCount(0),
          m_nLow(0), m_nHigh(0), m_nCode(0), m_bOld(bOldVariant), m_bStarted(false), m_bError(false), m_nWindowMask((1 << nWindowBits) - 1), m_nWindowPos(0)
    {
        // QByteArray::fill() takes qsizetype on Qt6, and widening the shift expression
        // itself is what MSVC flags as C4334; size it as an int first.
        const qint32 nWindowSize = 1 << nWindowBits;
        m_baWindow.fill(0, nWindowSize);

        const qint32 nSlots = nWindowBits * 2;
        modelInit(&m_model[0], 0x00, 0x40);
        modelInit(&m_model[1], 0x40, 0x40);
        modelInit(&m_model[2], 0x80, 0x40);
        modelInit(&m_model[3], 0xc0, 0x40);
        modelInit(&m_modelPos3, 0, qMin(nSlots, 24));
        modelInit(&m_modelPos4, 0, qMin(nSlots, 36));
        modelInit(&m_modelPosN, 0, nSlots);

        if (m_bOld) {
            modelInit(&m_modelLength, 0, QDS_OLD_LENGTH_SLOTS);
            modelInit(&m_modelSelector, 0, 5);
            // The weighted seeding runs against the INITIAL slot counts; the
            // entry counts below are narrowed afterwards, which is what the
            // reference does and what keeps the unused tail slots holding
            // probability mass.
            modelWeightInit(&m_modelLength);
            modelWeightInit(&m_modelPos3);
            modelWeightInit(&m_modelPos4);
            modelWeightInit(&m_modelPosN);

            qint32 nPos3 = 0;
            qint32 nPos4 = 0;
            qint32 nPosN = 0;
            for (qint32 i = 0; i < QDS_POSITION_SLOTS; i++) {
                if (m_tables.nPositionBase[i] < (1U << nWindowBits)) {
                    nPosN = i + 1;
                    if (m_tables.nPositionBase[i] < 0x1000U) nPos3 = i + 1;
                    if (m_tables.nPositionBase[i] < 0x40000U) nPos4 = i + 1;
                }
            }
            m_modelPos3.nEntries = nPos3;
            m_modelPos4.nEntries = nPos4;
            m_modelPosN.nEntries = nPosN;
        } else {
            modelInit(&m_modelLength, 0, QDS_NEW_LENGTH_SLOTS);
            modelInit(&m_modelSelector, 0, 7);
        }
    }

    bool hasError() const
    {
        return m_bError;
    }

    // Produces the next nCount bytes of the solid stream.  When pbaOut is null
    // the bytes are still decoded (the window and every model must advance) but
    // not collected - that is how the members ahead of the wanted one are
    // replayed cheaply.
    bool decodeMember(qint64 nCount, QByteArray *pbaOut, XBinary::PDSTRUCT *pPdStruct)
    {
        if (nCount < 0) return false;
        if (!m_bStarted) {
            // Flush to a byte boundary and prime the coder with 16 MSB-first bits.
            m_nBitBuffer <<= (m_nBitCount & 31);
            m_nBitCount = 0;
            m_nHigh = 0xffff;
            m_nLow = 0;
            m_nCode = quint16(readBits(16));
            m_bStarted = true;
            if (m_bError) return false;
        }
        if (pbaOut) pbaOut->reserve(qint32(qMin<qint64>(nCount, 0x7fffffff)));

        qint64 nDone = 0;
        qint32 nCheck = 0;
        while (nDone < nCount) {
            if ((++nCheck & 0xffff) == 0) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            }
            if (m_bError) return false;
            if (m_bOld) {
                if (!stepOld(&nDone, nCount, pbaOut)) return false;
            } else {
                if (!stepNew(&nDone, nCount, pbaOut)) return false;
            }
        }
        if (!m_bOld) {
            // Per-member trailer: 16 raw bits of running checksum.  Its value is
            // not verified here, but it has to leave the bit buffer or the next
            // member starts one word off.
            readBits(16);
        }
        return !m_bError;
    }

private:
    void fillBitBuffer()
    {
        if (m_nDataPos >= m_nDataSize) {
            m_bError = true;
            return;
        }
        m_nBitBuffer |= (quint32(m_pData[m_nDataPos]) << (24 - m_nBitCount));
        m_nDataPos++;
        m_nBitCount += 8;
    }

    quint32 readBits(qint32 nBits)
    {
        quint32 nResult = 0;
        while (nBits > 0) {
            if (m_nBitCount == 0) {
                fillBitBuffer();
                if (m_bError) return 0;
            }
            const qint32 nTake = qMin(nBits, m_nBitCount);
            nResult = (nResult << nTake) | (m_nBitBuffer >> (32 - nTake));
            m_nBitBuffer <<= nTake;
            m_nBitCount -= nTake;
            nBits -= nTake;
        }
        return nResult;
    }

    quint32 readBit()
    {
        if (m_nBitCount == 0) {
            fillBitBuffer();
            if (m_bError) return 0;
        }
        const quint32 nResult = m_nBitBuffer >> 31;
        m_nBitBuffer <<= 1;
        m_nBitCount--;
        return nResult;
    }

    quint32 getFrequency(quint32 nTotal) const
    {
        const quint64 nSpan = quint64(quint32((m_nCode - m_nLow) & 0xffffU) + 1);
        const quint64 nRange = quint64(quint32((m_nHigh - m_nLow) & 0xffffU) + 1);
        if (nTotal == 0) return 0;
        return quint32(((nSpan * nTotal - 1) / nRange) & 0xffffU);
    }

    void updateRange(quint32 nLow, quint32 nHigh, quint32 nTotal)
    {
        if (nTotal == 0) {
            m_bError = true;
            return;
        }
        const quint32 nRange = quint32((m_nHigh - m_nLow) & 0xffffU) + 1;
        m_nHigh = quint16(m_nLow + (nHigh * nRange) / nTotal - 1);
        m_nLow = quint16(m_nLow + (nLow * nRange) / nTotal);
        while (true) {
            if ((m_nLow ^ m_nHigh) & 0x8000U) {
                if (((m_nLow & 0x4000U) == 0) || ((m_nHigh & 0x4000U) != 0)) return;
                m_nCode ^= 0x4000U;
                m_nLow &= 0x3fffU;
                m_nHigh |= 0x4000U;
            }
            m_nLow = quint16(m_nLow << 1);
            m_nHigh = quint16((m_nHigh << 1) | 1);
            const quint32 nBit = readBit();
            if (m_bError) return;
            m_nCode = quint16((m_nCode << 1) | nBit);
        }
    }

    qint32 getSymbol(QDS_MODEL *pModel)
    {
        const qint32 nAllocated = pModel->listSymbols.size() - 1;
        if (nAllocated < 1) {
            m_bError = true;
            return -1;
        }
        const quint32 nTotal = pModel->listSymbols.at(0).nCumFreq;
        const quint32 nFrequency = getFrequency(nTotal);
        qint32 i = 0;
        // The new shape stops at the declared entry count; the old one walks the
        // whole allocated table, whose last cumfreq is 0 and terminates it.
        const qint32 nLimit = m_bOld ? nAllocated : pModel->nEntries;
        while ((i + 1 < nLimit) && (nFrequency < pModel->listSymbols.at(i + 1).nCumFreq)) i++;

        const qint32 nSymbol = pModel->listSymbols.at(i).nSymbol;
        updateRange(pModel->listSymbols.at(i + 1).nCumFreq, pModel->listSymbols.at(i).nCumFreq, nTotal);
        if (m_bError) return -1;

        for (qint32 j = 0; j <= i; j++) {
            pModel->listSymbols[j].nCumFreq = quint16(pModel->listSymbols.at(j).nCumFreq + 8);
        }
        if (pModel->listSymbols.at(0).nCumFreq > QDS_REBUILD_LIMIT) updateModel(pModel);
        return nSymbol;
    }

    void updateModel(QDS_MODEL *pModel)
    {
        const qint32 nCount = pModel->nEntries;
        if ((nCount < 1) || (nCount >= pModel->listSymbols.size())) return;
        if (m_bOld) {
            halveModel(pModel, nCount);
            return;
        }
        pModel->nShiftsLeft--;
        if (pModel->nShiftsLeft) {
            halveModel(pModel, nCount);
            return;
        }
        pModel->nShiftsLeft = 50;
        for (qint32 i = 0; i < nCount; i++) {
            pModel->listSymbols[i].nCumFreq =
                quint16((quint16(pModel->listSymbols.at(i).nCumFreq - pModel->listSymbols.at(i + 1).nCumFreq + 1)) >> 1);
        }
        for (qint32 i = 0; i < nCount - 1; i++) {
            for (qint32 j = i + 1; j < nCount; j++) {
                if (pModel->listSymbols.at(i).nCumFreq < pModel->listSymbols.at(j).nCumFreq) {
                    const QDS_SYMBOL tmp = pModel->listSymbols.at(i);
                    pModel->listSymbols[i] = pModel->listSymbols.at(j);
                    pModel->listSymbols[j] = tmp;
                }
            }
        }
        for (qint32 i = nCount - 1; i >= 0; i--) {
            pModel->listSymbols[i].nCumFreq = quint16(pModel->listSymbols.at(i).nCumFreq + pModel->listSymbols.at(i + 1).nCumFreq);
        }
    }

    static void halveModel(QDS_MODEL *pModel, qint32 nCount)
    {
        for (qint32 i = nCount - 1; i >= 0; i--) {
            pModel->listSymbols[i].nCumFreq = quint16(pModel->listSymbols.at(i).nCumFreq >> 1);
            if (pModel->listSymbols.at(i).nCumFreq <= pModel->listSymbols.at(i + 1).nCumFreq) {
                pModel->listSymbols[i].nCumFreq = quint16(pModel->listSymbols.at(i + 1).nCumFreq + 1);
            }
        }
    }

    // Old variant: the "extra" bits ride the arithmetic coder against a flat
    // model rather than being taken raw off the bit buffer.
    quint32 arithmeticBits(qint32 nBits)
    {
        quint32 nResult = 0;
        if (nBits > 11) {
            nBits -= 12;
            const quint32 nValue = getFrequency(0x1000U);
            updateRange(nValue, nValue + 1, 0x1000U);
            if (m_bError) return 0;
            nResult = nValue;
        }
        if (nBits > 0) {
            const quint32 nTotal = (1U << nBits);
            const quint32 nValue = getFrequency(nTotal);
            updateRange(nValue, nValue + 1, nTotal);
            if (m_bError) return 0;
            nResult = (nResult << nBits) + nValue;
        }
        return nResult;
    }

    void emitByte(quint8 nByte, qint64 *pnDone, QByteArray *pbaOut)
    {
        m_baWindow[m_nWindowPos] = char(nByte);
        m_nWindowPos = (m_nWindowPos + 1) & m_nWindowMask;
        if (pbaOut) pbaOut->append(char(nByte));
        (*pnDone)++;
    }

    bool copyMatch(qint32 nLength, qint32 nOffset, qint64 *pnDone, qint64 nCount, QByteArray *pbaOut)
    {
        if ((nOffset <= 0) || (nOffset > (m_nWindowMask + 1)) || (nLength <= 0)) return false;
        for (qint32 i = 0; i < nLength; i++) {
            if (*pnDone >= nCount) return false;  // a match may not run past the member end
            const quint8 nByte = quint8(m_baWindow.at((m_nWindowPos - nOffset) & m_nWindowMask));
            emitByte(nByte, pnDone, pbaOut);
        }
        return true;
    }

    bool stepNew(qint64 *pnDone, qint64 nCount, QByteArray *pbaOut)
    {
        const qint32 nSelector = getSymbol(&m_modelSelector);
        if (m_bError || (nSelector < 0)) return false;
        if (nSelector < 4) {
            const qint32 nLiteral = getSymbol(&m_model[nSelector]);
            if (m_bError || (nLiteral < 0)) return false;
            emitByte(quint8(nLiteral), pnDone, pbaOut);
            return true;
        }

        qint32 nLength = 0;
        QDS_MODEL *pPositionModel = nullptr;
        if (nSelector == 4) {
            nLength = 3;
            pPositionModel = &m_modelPos3;
        } else if (nSelector == 5) {
            nLength = 4;
            pPositionModel = &m_modelPos4;
        } else if (nSelector == 6) {
            const qint32 nSlot = getSymbol(&m_modelLength);
            if (m_bError || (nSlot < 0) || (nSlot >= QDS_NEW_LENGTH_SLOTS)) return false;
            const quint32 nExtra = readBits(m_tables.nNewLengthExtra[nSlot]);
            if (m_bError) return false;
            nLength = qint32(m_tables.nNewLengthBase[nSlot] + nExtra + 5);
            pPositionModel = &m_modelPosN;
        } else {
            return false;
        }

        const qint32 nSlot = getSymbol(pPositionModel);
        if (m_bError || (nSlot < 0) || (nSlot >= QDS_POSITION_SLOTS)) return false;
        const quint32 nExtra = readBits(m_tables.nPositionExtra[nSlot]);
        if (m_bError) return false;
        const qint64 nOffset = qint64(m_tables.nPositionBase[nSlot]) + nExtra + 1;
        if (nOffset > (m_nWindowMask + 1)) return false;
        return copyMatch(nLength, qint32(nOffset), pnDone, nCount, pbaOut);
    }

    bool stepOld(qint64 *pnDone, qint64 nCount, QByteArray *pbaOut)
    {
        const qint32 nRaw = getSymbol(&m_modelSelector);
        if (m_bError || (nRaw < 0) || (nRaw > 4)) return false;
        const qint32 nSelector = 4 - nRaw;  // old models hand symbols back reversed
        if (nSelector < 4) {
            const qint32 nValue = getSymbol(&m_model[nSelector]);
            if (m_bError || (nValue < 0)) return false;
            const qint32 nLiteral = (0x3f + 0x80 * nSelector) - nValue;
            if ((nLiteral < 0) || (nLiteral > 0xff)) return false;
            emitByte(quint8(nLiteral), pnDone, pbaOut);
            return true;
        }

        const qint32 nLengthSymbol = getSymbol(&m_modelLength);
        if (m_bError || (nLengthSymbol < 0)) return false;
        const qint32 nLengthSlot = m_modelLength.nEntries - nLengthSymbol - 1;
        if ((nLengthSlot < 0) || (nLengthSlot >= QDS_OLD_LENGTH_SLOTS)) return false;
        const quint32 nLengthExtra = arithmeticBits(m_tables.nOldLengthExtra[nLengthSlot]);
        if (m_bError) return false;
        const qint64 nLength = qint64(m_tables.nOldLengthBase[nLengthSlot]) + nLengthExtra + 3;

        QDS_MODEL *pPositionModel = &m_modelPosN;
        if (nLength == 3) {
            pPositionModel = &m_modelPos3;
        } else if (nLength == 4) {
            pPositionModel = &m_modelPos4;
        }

        const qint32 nPositionSymbol = getSymbol(pPositionModel);
        if (m_bError || (nPositionSymbol < 0)) return false;
        const qint32 nSlot = pPositionModel->nEntries - nPositionSymbol - 1;
        if ((nSlot < 0) || (nSlot >= QDS_POSITION_SLOTS)) return false;
        const quint32 nPositionExtra = arithmeticBits(m_tables.nPositionExtra[nSlot]);
        if (m_bError) return false;
        const qint64 nOffset = qint64(m_tables.nPositionBase[nSlot]) + nPositionExtra + 1;
        if ((nOffset > (m_nWindowMask + 1)) || (nLength > 0x7fffffff)) return false;
        return copyMatch(qint32(nLength), qint32(nOffset), pnDone, nCount, pbaOut);
    }

    QDS_TABLES m_tables;
    const quint8 *m_pData;
    qint64 m_nDataSize;
    qint64 m_nDataPos;
    quint32 m_nBitBuffer;
    qint32 m_nBitCount;
    quint16 m_nLow;
    quint16 m_nHigh;
    quint16 m_nCode;
    bool m_bOld;
    bool m_bStarted;
    bool m_bError;
    QByteArray m_baWindow;
    qint32 m_nWindowMask;
    qint32 m_nWindowPos;
    QDS_MODEL m_model[4];
    QDS_MODEL m_modelPos3;
    QDS_MODEL m_modelPos4;
    QDS_MODEL m_modelPosN;
    QDS_MODEL m_modelLength;
    QDS_MODEL m_modelSelector;
};

}  // namespace

QByteArray XDSQuantumDecoder::createProperties(qint32 nWindowBits, bool bOldVariant, qint32 nMemberIndex, const QList<qint64> &listSizes)
{
    QByteArray baResult;
    if ((nWindowBits < QDS_MIN_WINDOW_BITS) || (nWindowBits > QDS_MAX_WINDOW_BITS)) return baResult;
    if ((nMemberIndex < 0) || (nMemberIndex >= listSizes.size())) return baResult;

    baResult.append(char(quint8(nWindowBits)));
    baResult.append(char(bOldVariant ? 1 : 0));
    quint32 nValue = qToLittleEndian<quint32>(quint32(nMemberIndex));
    baResult.append(reinterpret_cast<const char *>(&nValue), 4);
    nValue = qToLittleEndian<quint32>(quint32(listSizes.size()));
    baResult.append(reinterpret_cast<const char *>(&nValue), 4);
    for (qint32 i = 0; i < listSizes.size(); i++) {
        const qint64 nSize = listSizes.at(i);
        if ((nSize < 0) || (nSize > qint64(0xffffffffu))) return QByteArray();
        nValue = qToLittleEndian<quint32>(quint32(nSize));
        baResult.append(reinterpret_cast<const char *>(&nValue), 4);
    }
    return baResult;
}

bool XDSQuantumDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, const QByteArray &baProperties, QByteArray *pbaUnpacked,
                               XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUnpacked) return false;
    pbaUnpacked->clear();
    if (baProperties.size() < 10) return false;

    const quint8 *pProperties = reinterpret_cast<const quint8 *>(baProperties.constData());
    const qint32 nWindowBits = qint32(pProperties[0]);
    const bool bOldVariant = (pProperties[1] != 0);
    const qint64 nMemberIndex = qint64(qFromLittleEndian<quint32>(pProperties + 2));
    const qint64 nMemberCount = qint64(qFromLittleEndian<quint32>(pProperties + 6));

    if ((nWindowBits < QDS_MIN_WINDOW_BITS) || (nWindowBits > QDS_MAX_WINDOW_BITS)) return false;
    if ((nMemberCount <= 0) || (nMemberIndex < 0) || (nMemberIndex >= nMemberCount)) return false;
    if (baProperties.size() != (10 + nMemberCount * 4)) return false;
    if (baPacked.isEmpty()) return false;

    QList<qint64> listSizes;
    for (qint64 i = 0; i < nMemberCount; i++) {
        listSizes.append(qint64(qFromLittleEndian<quint32>(pProperties + 10 + i * 4)));
    }
    if (listSizes.at(qint32(nMemberIndex)) != nUncompressedSize) return false;

    QDSDecoder decoder(baPacked, nWindowBits, bOldVariant);
    for (qint64 i = 0; i < nMemberIndex; i++) {
        if (!decoder.decodeMember(listSizes.at(qint32(i)), nullptr, pPdStruct)) return false;
    }
    if (!decoder.decodeMember(nUncompressedSize, pbaUnpacked, pPdStruct)) {
        pbaUnpacked->clear();
        return false;
    }
    if (qint64(pbaUnpacked->size()) != nUncompressedSize) {
        pbaUnpacked->clear();
        return false;
    }
    return true;
}
