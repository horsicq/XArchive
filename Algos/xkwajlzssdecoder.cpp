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
#include "xkwajlzssdecoder.h"

#include "algo_utils.h"

#include <array>
#include <limits>

namespace {
const qint32 KWAJ_LZSS_WINDOW_SIZE = 4096;
const qint32 KWAJ_LZSS_WINDOW_MASK = KWAJ_LZSS_WINDOW_SIZE - 1;
const qint32 KWAJ_LZSS_INITIAL_POSITION = KWAJ_LZSS_WINDOW_SIZE - 18;
const qint32 KWAJ_LZSS_OUTPUT_BUFFER_SIZE = 0x10000;

enum class KWAJ_LZSS_READ_RESULT {
    OK,
    END,
    IO_ERROR
};

class KWAJLZSSReader {
public:
    KWAJLZSSReader(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct) : m_pState(pState), m_pPdStruct(pPdStruct)
    {
    }

    KWAJ_LZSS_READ_RESULT readByte(quint8 *pValue)
    {
        if (!m_pState || !pValue || !XBinary::isPdStructNotCanceled(m_pPdStruct)) return KWAJ_LZSS_READ_RESULT::IO_ERROR;

        if ((m_pState->nInputLimit != -1) && (m_pState->nCountInput >= m_pState->nInputLimit)) {
            return (m_pState->nCountInput == m_pState->nInputLimit) ? KWAJ_LZSS_READ_RESULT::END : KWAJ_LZSS_READ_RESULT::IO_ERROR;
        }

        char cByte = 0;
        const qint32 nRead = XBinary::_readDevice(&cByte, 1, m_pState);
        if (nRead == 1) {
            *pValue = (quint8)cByte;
            return KWAJ_LZSS_READ_RESULT::OK;
        }

        if ((nRead < 0) || m_pState->bReadError) return KWAJ_LZSS_READ_RESULT::IO_ERROR;
        return KWAJ_LZSS_READ_RESULT::END;
    }

    bool atPhysicalEnd()
    {
        if (!m_pState) return false;
        if (m_pState->nInputLimit != -1) return m_pState->nCountInput == m_pState->nInputLimit;

        quint8 nIgnored = 0;
        return readByte(&nIgnored) == KWAJ_LZSS_READ_RESULT::END;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
};

class KWAJLZSSOutput {
public:
    KWAJLZSSOutput(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, bool bHasExpectedSize, qint64 nExpectedSize, qint64 nOutputLimit)
        : m_pState(pState),
          m_pPdStruct(pPdStruct),
          m_bHasExpectedSize(bHasExpectedSize),
          m_nExpectedSize(nExpectedSize),
          m_nOutputLimit(nOutputLimit),
          m_nPosition(0),
          m_nProduced(0)
    {
    }

    bool canEmit(qint32 nSize) const
    {
        if ((nSize < 0) || (m_nProduced > ((std::numeric_limits<qint64>::max)() - nSize))) return false;
        const qint64 nEnd = m_nProduced + nSize;
        return (!m_bHasExpectedSize || (nEnd <= m_nExpectedSize)) && ((m_nOutputLimit < 0) || (nEnd <= m_nOutputLimit));
    }

    bool writeByte(quint8 nByte)
    {
        if (!canEmit(1) || !XBinary::isPdStructNotCanceled(m_pPdStruct)) return false;
        m_abOutput[m_nPosition++] = (char)nByte;
        m_nProduced++;
        return (m_nPosition < KWAJ_LZSS_OUTPUT_BUFFER_SIZE) || flush();
    }

    bool flush()
    {
        if (m_nPosition == 0) return true;
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct) || (XBinary::_writeDevice(m_abOutput.data(), m_nPosition, m_pState) != m_nPosition)) {
            return false;
        }
        m_nPosition = 0;
        return true;
    }

    qint64 produced() const
    {
        return m_nProduced;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    bool m_bHasExpectedSize;
    qint64 m_nExpectedSize;
    qint64 m_nOutputLimit;
    std::array<char, KWAJ_LZSS_OUTPUT_BUFFER_SIZE> m_abOutput;
    qint32 m_nPosition;
    qint64 m_nProduced;
};
}  // namespace

XKWAJLZSSDecoder::XKWAJLZSSDecoder(QObject *parent) : QObject(parent)
{
}

bool XKWAJLZSSDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1)) {
        return false;
    }

    const bool bHasExpectedSize = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    qint64 nExpectedSize = -1;
    if (bHasExpectedSize) {
        bool bConverted = false;
        nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
        if (!bConverted || (nExpectedSize < 0)) return false;
    }

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nOutputLimit) ||
        (bHasExpectedSize && !XBinary::isUnpackOutputSizeAllowed(pDecompressState->mapUnpackProperties, nExpectedSize))) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    if ((pDecompressState->nProcessedOffset < 0) || (pDecompressState->nProcessedLimit < -1) ||
        ((pDecompressState->nProcessedLimit != -1) &&
         (pDecompressState->nProcessedOffset > ((std::numeric_limits<qint64>::max)() - pDecompressState->nProcessedLimit)))) {
        pDecompressState->bWriteError = true;
        return false;
    }

    KWAJLZSSReader reader(pDecompressState, pPdStruct);
    KWAJLZSSOutput output(pDecompressState, pPdStruct, bHasExpectedSize, nExpectedSize, nOutputLimit);
    std::array<quint8, KWAJ_LZSS_WINDOW_SIZE> abWindow;
    abWindow.fill(0x20);
    qint32 nWindowPosition = KWAJ_LZSS_INITIAL_POSITION;
    bool bCleanEnd = false;
    bool bDataError = false;
    bool bIOError = false;

    while (!bCleanEnd && !bDataError && !bIOError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (bHasExpectedSize && (output.produced() == nExpectedSize)) {
            bCleanEnd = reader.atPhysicalEnd();
            bDataError = !bCleanEnd;
            break;
        }

        quint8 nFlags = 0;
        KWAJ_LZSS_READ_RESULT readResult = reader.readByte(&nFlags);
        if (readResult == KWAJ_LZSS_READ_RESULT::END) {
            bCleanEnd = !bHasExpectedSize;
            bDataError = bHasExpectedSize;
            break;
        }
        if (readResult != KWAJ_LZSS_READ_RESULT::OK) {
            bIOError = true;
            break;
        }

        for (quint16 nMask = 1; nMask <= 0x80; nMask <<= 1) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                bIOError = true;
                break;
            }
            if (bHasExpectedSize && (output.produced() == nExpectedSize)) {
                bCleanEnd = reader.atPhysicalEnd();
                bDataError = !bCleanEnd;
                break;
            }

            if (nFlags & nMask) {
                quint8 nLiteral = 0;
                readResult = reader.readByte(&nLiteral);
                if (readResult == KWAJ_LZSS_READ_RESULT::END) {
                    // KWAJ has no end marker. Without a declared length an EOF
                    // at a token boundary is the only available terminator.
                    bCleanEnd = !bHasExpectedSize;
                    bDataError = bHasExpectedSize;
                    break;
                }
                if (readResult != KWAJ_LZSS_READ_RESULT::OK) {
                    bIOError = true;
                    break;
                }
                if (!output.canEmit(1)) {
                    bDataError = true;
                    break;
                }

                abWindow[nWindowPosition] = nLiteral;
                nWindowPosition = (nWindowPosition + 1) & KWAJ_LZSS_WINDOW_MASK;
                if (!output.writeByte(nLiteral)) {
                    bIOError = pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct);
                    bDataError = !bIOError;
                    break;
                }
            } else {
                quint8 nPositionLow = 0;
                readResult = reader.readByte(&nPositionLow);
                if (readResult == KWAJ_LZSS_READ_RESULT::END) {
                    bCleanEnd = !bHasExpectedSize;
                    bDataError = bHasExpectedSize;
                    break;
                }
                if (readResult != KWAJ_LZSS_READ_RESULT::OK) {
                    bIOError = true;
                    break;
                }

                quint8 nPositionAndLength = 0;
                readResult = reader.readByte(&nPositionAndLength);
                if (readResult == KWAJ_LZSS_READ_RESULT::END) {
                    // One byte of a two-byte match is always a truncated token.
                    bDataError = true;
                    break;
                }
                if (readResult != KWAJ_LZSS_READ_RESULT::OK) {
                    bIOError = true;
                    break;
                }

                const qint32 nMatchLength = (nPositionAndLength & 0x0F) + 3;
                if (!output.canEmit(nMatchLength)) {
                    bDataError = true;
                    break;
                }
                qint32 nMatchPosition = nPositionLow | ((nPositionAndLength & 0xF0) << 4);
                for (qint32 i = 0; i < nMatchLength; i++) {
                    const quint8 nByte = abWindow[nMatchPosition];
                    nMatchPosition = (nMatchPosition + 1) & KWAJ_LZSS_WINDOW_MASK;
                    abWindow[nWindowPosition] = nByte;
                    nWindowPosition = (nWindowPosition + 1) & KWAJ_LZSS_WINDOW_MASK;
                    if (!output.writeByte(nByte)) {
                        bIOError = pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct);
                        bDataError = !bIOError;
                        break;
                    }
                }
                if (bDataError || bIOError) break;
            }
        }
    }

    const bool bOutputComplete = !bHasExpectedSize || (output.produced() == nExpectedSize);
    bool bResult = bCleanEnd && bOutputComplete && !bDataError && !bIOError && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
                   XBinary::isPdStructNotCanceled(pPdStruct);
    if (bResult && !output.flush()) bResult = false;

    return bResult && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
