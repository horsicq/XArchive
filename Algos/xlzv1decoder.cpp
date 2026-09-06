/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xlzv1decoder.h"

#include "algo_utils.h"

#include <cstring>
#include <vector>

namespace {
const quint8 LZV1_FIXED[6] = {0x5DU, 0x19U, 0x01U, 0xADU, 0x00U, 0x00U};
const qint32 LZV1_ROOT_CODES = 0x100;
const qint32 LZV1_FLUSH_SIZE = 0x10000;

// MSB-first bit reader over whole bytes; a read past the end is the normal end
// of stream, not an error.
class Lzv1BitReader {
public:
    Lzv1BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPos(0), m_nCurrent(0), m_nLeft(0)
    {
    }

    qint32 getBit()
    {
        if (m_nLeft == 0) {
            if (m_nPos >= m_nSize) return -1;
            m_nCurrent = m_pData[m_nPos];
            m_nPos++;
            m_nLeft = 8;
        }
        const qint32 nBit = (m_nCurrent >> 7) & 1;
        m_nCurrent = (quint8)((m_nCurrent << 1) & 0xffU);
        m_nLeft--;
        return nBit;
    }

    bool getBits(qint32 nCount, qint32 *pnValue)
    {
        qint32 nValue = 0;
        for (qint32 i = 0; i < nCount; i++) {
            const qint32 nBit = getBit();
            if (nBit < 0) return false;
            nValue = (nValue * 2) + nBit;
        }
        *pnValue = nValue;
        return true;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint8 m_nCurrent;
    qint32 m_nLeft;
};
}  // namespace

bool XLZV1Decoder::checkHeader(const char *pHeader, qint64 nHeaderSize, qint32 *pnMaxCodes)
{
    if (!pHeader || (nHeaderSize < LZV1_HEADER_SIZE)) return false;
    const quint8 *pBytes = (const quint8 *)pHeader;
    if (memcmp(pBytes, "LZV1", 4) != 0) return false;
    if (memcmp(pBytes + 4, LZV1_FIXED, sizeof(LZV1_FIXED)) != 0) return false;

    const qint32 nMaxCodes = ((qint32)pBytes[10] * 0x100) + (qint32)pBytes[11];
    if ((nMaxCodes <= LZV1_ROOT_CODES) || (nMaxCodes >= LZV1_MAX_CODES_LIMIT)) return false;

    if (pnMaxCodes) *pnMaxCodes = nMaxCodes;
    return true;
}

bool XLZV1Decoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

    QByteArray baInput;
    if (!Algo_utils::readInputData(pDecompressState, &baInput, pPdStruct)) {
        pDecompressState->bReadError = true;
        return false;
    }
    pDecompressState->nCountInput = baInput.size();

    qint32 nMaxCodes = 0;
    if (!checkHeader(baInput.constData(), baInput.size(), &nMaxCodes)) return false;

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nOutputLimit)) return false;

    const quint8 *pStream = (const quint8 *)baInput.constData() + LZV1_HEADER_SIZE;
    const qint64 nStreamSize = (qint64)baInput.size() - LZV1_HEADER_SIZE;
    Lzv1BitReader reader(pStream, nStreamSize);

    std::vector<quint16> vecPrevious((size_t)nMaxCodes, 0);
    std::vector<quint8> vecLastChar((size_t)nMaxCodes, 0);
    std::vector<quint8> vecFirstChar((size_t)nMaxCodes, 0);
    std::vector<quint8> vecStack((size_t)nMaxCodes + 1, 0);

    QByteArray baOutput;
    baOutput.reserve(LZV1_FLUSH_SIZE + 1);
    qint64 nProduced = 0;
    bool bFailed = false;
    bool bFinished = false;

    while (!bFailed && !bFinished) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        // Dictionary (re)start: 8-bit root alphabet, no clear code in the
        // stream - the coder just begins again once it is full.
        qint32 nBits = 8;
        qint32 nLimit = LZV1_ROOT_CODES;
        qint32 nNext = LZV1_ROOT_CODES;
        for (qint32 i = 0; i < LZV1_ROOT_CODES; i++) {
            vecPrevious[(size_t)i] = 0xffffU;
            vecLastChar[(size_t)i] = (quint8)i;
            vecFirstChar[(size_t)i] = (quint8)i;
        }

        qint32 nCode = 0;
        if (!reader.getBits(8, &nCode)) break;  // clean end of stream
        qint32 nCurrent = nCode;
        qint32 nPrevious = nCode;

        while (true) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

            qint32 nStackTop = 0;
            qint32 nWalk = nCurrent;
            while (true) {
                if ((nWalk < 0) || (nWalk >= nMaxCodes) || (nStackTop > nMaxCodes)) {
                    bFailed = true;
                    break;
                }
                vecStack[(size_t)nStackTop] = vecLastChar[(size_t)nWalk];
                nStackTop++;
                const qint32 nParent = (qint32)vecPrevious[(size_t)nWalk];
                const bool bContinue = (nParent < nWalk);
                nWalk = nParent;
                if (!bContinue) break;
            }
            if (bFailed) break;

            while (nStackTop > 0) {
                nStackTop--;
                baOutput.append((char)vecStack[(size_t)nStackTop]);
                nProduced++;
                if ((nProduced > LZV1_MAX_OUTPUT_SIZE) || ((nOutputLimit >= 0) && (nProduced > nOutputLimit))) {
                    bFailed = true;
                    break;
                }
                if (baOutput.size() >= LZV1_FLUSH_SIZE) {
                    if (XBinary::_writeDevice(baOutput.constData(), baOutput.size(), pDecompressState) != baOutput.size()) {
                        pDecompressState->bWriteError = true;
                        bFailed = true;
                        break;
                    }
                    baOutput.resize(0);
                }
            }
            if (bFailed) break;

            if (nNext + 1 == nMaxCodes) break;  // dictionary full: restart outside

            if (nLimit * 2 <= nNext + 1) {
                nBits++;
                nLimit *= 2;
            }

            qint32 nRaw = 0;
            if (!reader.getBits(nBits, &nRaw)) {
                bFinished = true;
                break;
            }
            qint32 nValue = nRaw + nLimit - nNext - 1;
            if (nValue < 0) {
                // Phased-in binary: the short codes are used up, this one needs
                // one more bit.
                const qint32 nBase = nRaw + nLimit;
                const qint32 nBit = reader.getBit();
                if (nBit < 0) {
                    bFinished = true;
                    break;
                }
                nValue = (nBase * 2) - nNext - 1 + nBit;
            }
            if ((nValue < 0) || (nValue > nNext) || (nNext >= nMaxCodes)) {
                bFailed = true;
                break;
            }

            // KwKwK: a code that names the entry being defined right now.
            const qint32 nSource = (nValue == nNext) ? nPrevious : nValue;
            vecPrevious[(size_t)nNext] = (quint16)nPrevious;
            vecLastChar[(size_t)nNext] = vecFirstChar[(size_t)nSource];
            vecFirstChar[(size_t)nNext] = vecFirstChar[(size_t)nPrevious];
            nNext++;
            nPrevious = nValue;
            nCurrent = nValue;
        }
    }

    if (bFailed) return false;

    if (!baOutput.isEmpty()) {
        if (XBinary::_writeDevice(baOutput.constData(), baOutput.size(), pDecompressState) != baOutput.size()) {
            pDecompressState->bWriteError = true;
            return false;
        }
        baOutput.resize(0);
    }

    if (nProduced == 0) return false;

    if (pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bConverted = false;
        const qint64 nDeclared = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
        if (!bConverted || (nDeclared != pDecompressState->nCountOutput)) return false;
    }

    return !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
