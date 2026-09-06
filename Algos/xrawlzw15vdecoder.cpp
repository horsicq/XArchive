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
#include "xrawlzw15vdecoder.h"

#include <QVector>

namespace {

// MSB-first bit pump.  A whole byte is taken from the input and drained from
// bit 7 down, so the position only advances on a byte boundary - which is what
// makes "every input byte consumed" a meaningful end-of-stream test.
class RawLzw15vBitReader {
public:
    RawLzw15vBitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nCurrent(0), m_nBitsLeft(0)
    {
    }

    bool read(qint32 nBits, quint32 *pnValue)
    {
        quint32 nValue = 0;

        for (qint32 i = 0; i < nBits; i++) {
            if (m_nBitsLeft == 0) {
                if (m_nPosition >= m_nSize) return false;
                m_nCurrent = m_pData[m_nPosition];
                m_nPosition++;
                m_nBitsLeft = 8;
            }

            nValue = (nValue << 1) | static_cast<quint32>((m_nCurrent >> 7) & 0x01U);
            m_nCurrent = static_cast<quint8>((m_nCurrent << 1) & 0xFFU);
            m_nBitsLeft--;
        }

        *pnValue = nValue;

        return true;
    }

    qint64 position() const
    {
        return m_nPosition;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint8 m_nCurrent;
    qint32 m_nBitsLeft;
};

// The single implementation behind decode() and probe().  bStrict adds the
// grammar checks that detection needs and that the original decoder does not
// perform; nMaxOutput caps the produced bytes either way.
bool rawLzw15vRun(const QByteArray &baPacked, bool bStrict, qint64 nMaxOutput, QByteArray *pbaUnpacked, qint64 *pnOutputSize, bool *pbFullyConsumed)
{
    if (pnOutputSize) *pnOutputSize = 0;
    if (pbFullyConsumed) *pbFullyConsumed = false;

    const qint64 nPackedSize = baPacked.size();
    if ((nPackedSize <= 0) || (nMaxOutput <= 0)) return false;

    RawLzw15vBitReader bitReader(reinterpret_cast<const quint8 *>(baPacked.constData()), nPackedSize);

    QVector<quint16> vPrefix(static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES), 0);
    QVector<quint8> vAppend(static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES), 0);
    QVector<quint8> vStack(static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES), 0);

    for (qint32 i = 0; i < 256; i++) {
        vAppend[i] = static_cast<quint8>(i);
    }

    QByteArray baResult;
    qint64 nProduced = 0;
    bool bEndSeen = false;

    while (!bEndSeen) {
        // Stream opening, and the state a CLEAR returns to: a bare 9-bit code
        // that is emitted as a literal and seeds the previous-code chain.
        quint32 nCode = 0;
        if (!bitReader.read(XRawLzw15vDecoder::MIN_CODE_BITS, &nCode)) return false;
        if (nCode == static_cast<quint32>(XRawLzw15vDecoder::CODE_END)) {
            bEndSeen = true;
            break;
        }
        if (bStrict && (nCode > 0xFFU)) return false;

        if (nProduced >= nMaxOutput) return false;
        if (pbaUnpacked) baResult.append(static_cast<char>(nCode & 0xFFU));
        nProduced++;

        quint32 nNextCode = static_cast<quint32>(XRawLzw15vDecoder::FIRST_CODE);
        qint32 nCodeBits = XRawLzw15vDecoder::MIN_CODE_BITS;
        quint32 nPreviousCode = nCode;
        quint32 nCharacter = nCode;
        bool bClear = false;

        while (!bClear) {
            quint32 nNew = 0;
            if (!bitReader.read(nCodeBits, &nNew)) return false;

            if (nNew == static_cast<quint32>(XRawLzw15vDecoder::CODE_END)) {
                bEndSeen = true;
                break;
            }

            if (nNew == static_cast<quint32>(XRawLzw15vDecoder::CODE_BUMP)) {
                // The width is driven ONLY by this code; a real stream never
                // emits it once the cap is reached.
                if (bStrict && (nCodeBits >= XRawLzw15vDecoder::MAX_CODE_BITS)) return false;
                if (nCodeBits < XRawLzw15vDecoder::MAX_CODE_BITS) nCodeBits++;
                continue;
            }

            if (nNew == static_cast<quint32>(XRawLzw15vDecoder::CODE_CLEAR)) {
                bClear = true;
                break;
            }

            if (bStrict) {
                if (nNew > nNextCode) return false;
                if (nNextCode > (1U << nCodeBits)) return false;
            }

            qint32 nStackSize = 0;
            quint32 nCurrent = 0;

            if (nNew >= nNextCode) {
                // KwKwK: the code that is about to be defined is being used.
                vStack[nStackSize] = static_cast<quint8>(nCharacter & 0xFFU);
                nStackSize++;
                nCurrent = nPreviousCode;
            } else {
                nCurrent = nNew;
            }

            while (nCurrent > 0xFFU) {
                if (nCurrent >= static_cast<quint32>(XRawLzw15vDecoder::MAX_CODES)) return false;
                if (nStackSize >= static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES)) return false;
                vStack[nStackSize] = vAppend[static_cast<qint32>(nCurrent)];
                nStackSize++;
                nCurrent = vPrefix[static_cast<qint32>(nCurrent)];
            }

            if (nStackSize >= static_cast<qint32>(XRawLzw15vDecoder::MAX_CODES)) return false;
            vStack[nStackSize] = static_cast<quint8>(nCurrent & 0xFFU);
            nStackSize++;

            nCharacter = nCurrent;

            if (nProduced > (nMaxOutput - static_cast<qint64>(nStackSize))) return false;

            while (nStackSize > 0) {
                nStackSize--;
                if (pbaUnpacked) baResult.append(static_cast<char>(vStack[nStackSize]));
                nProduced++;
            }

            if (nNextCode < static_cast<quint32>(XRawLzw15vDecoder::MAX_CODES)) {
                vPrefix[static_cast<qint32>(nNextCode)] = static_cast<quint16>(nPreviousCode);
                vAppend[static_cast<qint32>(nNextCode)] = static_cast<quint8>(nCharacter & 0xFFU);
                nNextCode++;
            }

            nPreviousCode = nNew;
        }
    }

    if (!bEndSeen) return false;

    if (pnOutputSize) *pnOutputSize = nProduced;
    if (pbFullyConsumed) *pbFullyConsumed = (bitReader.position() == nPackedSize);
    if (pbaUnpacked) *pbaUnpacked = baResult;

    return true;
}

}  // namespace

bool XRawLzw15vDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked || (nUncompressedSize <= 0)) return false;

    QByteArray baUnpacked;
    qint64 nOutputSize = 0;

    if (!rawLzw15vRun(baPacked, false, nUncompressedSize, &baUnpacked, &nOutputSize, nullptr)) return false;
    if (static_cast<qint64>(baUnpacked.size()) != nUncompressedSize) return false;

    *pbaUnpacked = baUnpacked;

    return true;
}

bool XRawLzw15vDecoder::probe(const QByteArray &baPacked, qint64 nMaxOutput, qint64 *pnOutputSize)
{
    qint64 nOutputSize = 0;
    bool bFullyConsumed = false;

    if (!rawLzw15vRun(baPacked, true, nMaxOutput, nullptr, &nOutputSize, &bFullyConsumed)) return false;

    // A headerless format has nothing else to check against: the stream must
    // end on its own END code with no trailing byte left over.
    if (!bFullyConsumed) return false;

    if (pnOutputSize) *pnOutputSize = nOutputSize;

    return true;
}
