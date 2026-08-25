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
#include "xkwajlzhdecoder.h"

#include "algo_utils.h"

#include <array>
#include <limits>

namespace {
const qint32 KWAJ_INPUT_BUFFER_SIZE = 0x1000;
const qint32 KWAJ_OUTPUT_BUFFER_SIZE = 0x10000;
const qint32 KWAJ_WINDOW_SIZE = 4096;
const qint32 KWAJ_WINDOW_MASK = KWAJ_WINDOW_SIZE - 1;
const quint8 KWAJ_WINDOW_FILL = 0x20;
const qint32 KWAJ_MAX_HUFF_BITS = 16;
const qint32 KWAJ_MAX_SYMBOLS = 256;

enum class KWAJ_RESULT {
    OK,
    END,
    IO_ERROR,
    DATA_ERROR
};

class KWAJBitReader {
public:
    KWAJBitReader(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
        : m_pState(pState),
          m_pPdStruct(pPdStruct),
          m_nInputPosition(0),
          m_nInputSize(0),
          m_nBitBuffer(0),
          m_nBits(0),
          m_bCleanEnd(false)
    {
    }

    KWAJ_RESULT readBits(qint32 nBits, quint32 *pValue)
    {
        if (!pValue || (nBits <= 0) || (nBits > 24)) return KWAJ_RESULT::DATA_ERROR;
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct)) return KWAJ_RESULT::IO_ERROR;

        while (m_nBits < nBits) {
            quint8 nByte = 0;
            const KWAJ_RESULT result = readByte(&nByte);
            if (result != KWAJ_RESULT::OK) return result;
            m_nBitBuffer = (m_nBitBuffer << 8) | nByte;
            m_nBits += 8;
        }

        const qint32 nRemainingBits = m_nBits - nBits;
        *pValue = (m_nBitBuffer >> nRemainingBits) & ((((quint32)1) << nBits) - 1);
        m_nBits = nRemainingBits;
        if (m_nBits == 0) {
            m_nBitBuffer = 0;
        } else {
            m_nBitBuffer &= ((((quint32)1) << m_nBits) - 1);
        }
        return KWAJ_RESULT::OK;
    }

    bool isCleanEnd() const { return m_bCleanEnd; }

private:
    KWAJ_RESULT readByte(quint8 *pValue)
    {
        if (!m_pState || !pValue || !XBinary::isPdStructNotCanceled(m_pPdStruct)) return KWAJ_RESULT::IO_ERROR;

        if (m_nInputPosition >= m_nInputSize) {
            if ((m_pState->nInputLimit != -1) && (m_pState->nCountInput >= m_pState->nInputLimit)) {
                m_bCleanEnd = true;
                return KWAJ_RESULT::END;
            }

            qint32 nRequest = KWAJ_INPUT_BUFFER_SIZE;
            if (m_pState->nInputLimit != -1) {
                const qint64 nRemaining = m_pState->nInputLimit - m_pState->nCountInput;
                if (nRemaining <= 0) {
                    m_bCleanEnd = true;
                    return KWAJ_RESULT::END;
                }
                if (nRemaining < nRequest) nRequest = (qint32)nRemaining;
            }

            const qint32 nRead = XBinary::_readDevice(m_abInput.data(), nRequest, m_pState);
            if (nRead <= 0) {
                if ((nRead == 0) && !m_pState->bReadError) {
                    m_bCleanEnd = true;
                    return KWAJ_RESULT::END;
                }
                return KWAJ_RESULT::IO_ERROR;
            }

            m_nInputPosition = 0;
            m_nInputSize = nRead;
        }

        *pValue = (quint8)m_abInput[m_nInputPosition++];
        return KWAJ_RESULT::OK;
    }

    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    std::array<char, KWAJ_INPUT_BUFFER_SIZE> m_abInput;
    qint32 m_nInputPosition;
    qint32 m_nInputSize;
    quint32 m_nBitBuffer;
    qint32 m_nBits;
    bool m_bCleanEnd;
};

class KWAJHuffmanTree {
public:
    KWAJHuffmanTree() : m_nSymbolCount(0)
    {
        m_anCounts.fill(0);
        m_anFirstCode.fill(0);
        m_anFirstSymbol.fill(0);
        m_anSymbols.fill(0);
    }

    bool build(const std::array<quint8, KWAJ_MAX_SYMBOLS> &anLengths, qint32 nSymbolCount)
    {
        if ((nSymbolCount <= 0) || (nSymbolCount > KWAJ_MAX_SYMBOLS)) return false;

        m_anCounts.fill(0);
        m_anFirstCode.fill(0);
        m_anFirstSymbol.fill(0);
        m_anSymbols.fill(0);

        for (qint32 i = 0; i < nSymbolCount; i++) {
            const qint32 nLength = anLengths[i];
            if (nLength > KWAJ_MAX_HUFF_BITS) return false;
            if (nLength != 0) m_anCounts[nLength]++;
        }

        // A KWAJ tree is a complete canonical tree.  Validating the full Kraft
        // sum also rejects extra long codes hidden behind an already-full root.
        qint32 nCodesLeft = 1;
        for (qint32 nLength = 1; nLength <= KWAJ_MAX_HUFF_BITS; nLength++) {
            nCodesLeft = (nCodesLeft << 1) - m_anCounts[nLength];
            if (nCodesLeft < 0) return false;
        }
        if (nCodesLeft != 0) return false;

        quint32 nCode = 0;
        quint32 nSymbolIndex = 0;
        for (qint32 nLength = 1; nLength <= KWAJ_MAX_HUFF_BITS; nLength++) {
            nCode = (nCode + m_anCounts[nLength - 1]) << 1;
            m_anFirstCode[nLength] = nCode;
            m_anFirstSymbol[nLength] = (quint16)nSymbolIndex;

            for (qint32 nSymbol = 0; nSymbol < nSymbolCount; nSymbol++) {
                if (anLengths[nSymbol] == nLength) m_anSymbols[nSymbolIndex++] = (quint16)nSymbol;
            }
        }

        m_nSymbolCount = nSymbolCount;
        return nSymbolIndex <= (quint32)nSymbolCount;
    }

    KWAJ_RESULT decode(KWAJBitReader *pReader, quint32 *pSymbol) const
    {
        if (!pReader || !pSymbol || (m_nSymbolCount <= 0)) return KWAJ_RESULT::DATA_ERROR;

        quint32 nCode = 0;
        for (qint32 nLength = 1; nLength <= KWAJ_MAX_HUFF_BITS; nLength++) {
            quint32 nBit = 0;
            const KWAJ_RESULT result = pReader->readBits(1, &nBit);
            if (result != KWAJ_RESULT::OK) return result;
            nCode = (nCode << 1) | nBit;

            const quint32 nFirstCode = m_anFirstCode[nLength];
            const quint32 nCount = m_anCounts[nLength];
            if ((nCount != 0) && (nCode >= nFirstCode) && ((nCode - nFirstCode) < nCount)) {
                const quint32 nIndex = m_anFirstSymbol[nLength] + (nCode - nFirstCode);
                if (nIndex >= (quint32)m_nSymbolCount) return KWAJ_RESULT::DATA_ERROR;
                *pSymbol = m_anSymbols[nIndex];
                return KWAJ_RESULT::OK;
            }
        }

        return KWAJ_RESULT::DATA_ERROR;
    }

private:
    qint32 m_nSymbolCount;
    std::array<quint16, KWAJ_MAX_HUFF_BITS + 1> m_anCounts;
    std::array<quint32, KWAJ_MAX_HUFF_BITS + 1> m_anFirstCode;
    std::array<quint16, KWAJ_MAX_HUFF_BITS + 1> m_anFirstSymbol;
    std::array<quint16, KWAJ_MAX_SYMBOLS> m_anSymbols;
};

KWAJ_RESULT readHuffmanTree(KWAJBitReader *pReader, quint32 nType, qint32 nSymbolCount, KWAJHuffmanTree *pTree)
{
    if (!pReader || !pTree || (nType > 3) || (nSymbolCount <= 0) || (nSymbolCount > KWAJ_MAX_SYMBOLS)) return KWAJ_RESULT::DATA_ERROR;

    std::array<quint8, KWAJ_MAX_SYMBOLS> anLengths = {};
    quint32 nCurrentLength = 0;

    if (nType == 0) {
        quint8 nFixedLength = 0;
        if (nSymbolCount == 16) nFixedLength = 4;
        else if (nSymbolCount == 32) nFixedLength = 5;
        else if (nSymbolCount == 64) nFixedLength = 6;
        else if (nSymbolCount == 256) nFixedLength = 8;
        else return KWAJ_RESULT::DATA_ERROR;

        for (qint32 i = 0; i < nSymbolCount; i++) anLengths[i] = nFixedLength;
    } else if (nType == 1) {
        KWAJ_RESULT result = pReader->readBits(4, &nCurrentLength);
        if (result != KWAJ_RESULT::OK) return result;
        anLengths[0] = (quint8)nCurrentLength;

        for (qint32 i = 1; i < nSymbolCount; i++) {
            quint32 nSelector = 0;
            result = pReader->readBits(1, &nSelector);
            if (result != KWAJ_RESULT::OK) return result;
            if (nSelector != 0) {
                result = pReader->readBits(1, &nSelector);
                if (result != KWAJ_RESULT::OK) return result;
                if (nSelector == 0) {
                    if (nCurrentLength >= KWAJ_MAX_HUFF_BITS) return KWAJ_RESULT::DATA_ERROR;
                    nCurrentLength++;
                } else {
                    result = pReader->readBits(4, &nCurrentLength);
                    if (result != KWAJ_RESULT::OK) return result;
                }
            }
            anLengths[i] = (quint8)nCurrentLength;
        }
    } else if (nType == 2) {
        KWAJ_RESULT result = pReader->readBits(4, &nCurrentLength);
        if (result != KWAJ_RESULT::OK) return result;
        anLengths[0] = (quint8)nCurrentLength;

        for (qint32 i = 1; i < nSymbolCount; i++) {
            quint32 nSelector = 0;
            result = pReader->readBits(2, &nSelector);
            if (result != KWAJ_RESULT::OK) return result;
            if (nSelector == 0) {
                if (nCurrentLength == 0) return KWAJ_RESULT::DATA_ERROR;
                nCurrentLength--;
            } else if (nSelector == 2) {
                if (nCurrentLength >= KWAJ_MAX_HUFF_BITS) return KWAJ_RESULT::DATA_ERROR;
                nCurrentLength++;
            } else if (nSelector == 3) {
                result = pReader->readBits(4, &nCurrentLength);
                if (result != KWAJ_RESULT::OK) return result;
            }
            anLengths[i] = (quint8)nCurrentLength;
        }
    } else {
        for (qint32 i = 0; i < nSymbolCount; i++) {
            quint32 nLength = 0;
            const KWAJ_RESULT result = pReader->readBits(4, &nLength);
            if (result != KWAJ_RESULT::OK) return result;
            anLengths[i] = (quint8)nLength;
        }
    }

    return pTree->build(anLengths, nSymbolCount) ? KWAJ_RESULT::OK : KWAJ_RESULT::DATA_ERROR;
}

class KWAJOutput {
public:
    KWAJOutput(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, bool bHasExpectedSize, qint64 nExpectedSize, qint64 nOutputLimit)
        : m_pState(pState),
          m_pPdStruct(pPdStruct),
          m_bHasExpectedSize(bHasExpectedSize),
          m_nExpectedSize(nExpectedSize),
          m_nOutputLimit(nOutputLimit),
          m_nPosition(0),
          m_nProduced(0)
    {
    }

    bool writeByte(quint8 nByte)
    {
        if (!m_pState || !XBinary::isPdStructNotCanceled(m_pPdStruct) ||
            (m_nProduced == (std::numeric_limits<qint64>::max)()) ||
            (m_bHasExpectedSize && (m_nProduced >= m_nExpectedSize)) ||
            ((m_nOutputLimit >= 0) && (m_nProduced >= m_nOutputLimit))) {
            return false;
        }

        m_abOutput[m_nPosition++] = (char)nByte;
        m_nProduced++;
        return (m_nPosition < KWAJ_OUTPUT_BUFFER_SIZE) || flush();
    }

    bool canEmit(qint32 nSize) const
    {
        if ((nSize < 0) || (m_nProduced > ((std::numeric_limits<qint64>::max)() - nSize))) return false;
        const qint64 nEnd = m_nProduced + nSize;
        return (!m_bHasExpectedSize || (nEnd <= m_nExpectedSize)) && ((m_nOutputLimit < 0) || (nEnd <= m_nOutputLimit));
    }

    bool flush()
    {
        if (m_nPosition == 0) return true;
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct) ||
            (XBinary::_writeDevice(m_abOutput.data(), m_nPosition, m_pState) != m_nPosition)) {
            return false;
        }
        m_nPosition = 0;
        return true;
    }

    qint64 produced() const { return m_nProduced; }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    bool m_bHasExpectedSize;
    qint64 m_nExpectedSize;
    qint64 m_nOutputLimit;
    std::array<char, KWAJ_OUTPUT_BUFFER_SIZE> m_abOutput;
    qint32 m_nPosition;
    qint64 m_nProduced;
};
}  // namespace

XKWAJLZHDecoder::XKWAJLZHDecoder(QObject *parent) : QObject(parent)
{
}

bool XKWAJLZHDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput ||
        (pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1)) {
        return false;
    }

    bool bHasExpectedSize = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    qint64 nExpectedSize = -1;
    if (bHasExpectedSize) {
        bool bConverted = false;
        nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
        if (!bConverted || (nExpectedSize < 0)) return false;
    }

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nOutputLimit) ||
        (bHasExpectedSize && (nOutputLimit >= 0) && (nExpectedSize > nOutputLimit))) {
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

    KWAJBitReader reader(pDecompressState, pPdStruct);
    std::array<quint32, 6> anTypes = {};
    for (qint32 i = 0; i < 6; i++) {
        if (reader.readBits(4, &anTypes[i]) != KWAJ_RESULT::OK) return false;
    }
    for (qint32 i = 0; i < 5; i++) {
        if (anTypes[i] > 3) return false;
    }

    KWAJHuffmanTree matchLength1Tree;
    KWAJHuffmanTree matchLength2Tree;
    KWAJHuffmanTree literalLengthTree;
    KWAJHuffmanTree offsetTree;
    KWAJHuffmanTree literalTree;

    if ((readHuffmanTree(&reader, anTypes[0], 16, &matchLength1Tree) != KWAJ_RESULT::OK) ||
        (readHuffmanTree(&reader, anTypes[1], 16, &matchLength2Tree) != KWAJ_RESULT::OK) ||
        (readHuffmanTree(&reader, anTypes[2], 32, &literalLengthTree) != KWAJ_RESULT::OK) ||
        (readHuffmanTree(&reader, anTypes[3], 64, &offsetTree) != KWAJ_RESULT::OK) ||
        (readHuffmanTree(&reader, anTypes[4], 256, &literalTree) != KWAJ_RESULT::OK)) {
        return false;
    }

    std::array<quint8, KWAJ_WINDOW_SIZE> abWindow;
    abWindow.fill(KWAJ_WINDOW_FILL);
    qint32 nWindowPosition = 0;
    bool bUseSecondMatchTree = false;
    bool bCleanEnd = false;
    bool bDataError = false;
    bool bIOError = false;
    KWAJOutput output(pDecompressState, pPdStruct, bHasExpectedSize, nExpectedSize, nOutputLimit);

    while (!bCleanEnd && !bDataError && !bIOError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint32 nLengthCode = 0;
        KWAJ_RESULT result = (bUseSecondMatchTree ? matchLength2Tree : matchLength1Tree).decode(&reader, &nLengthCode);
        if (result == KWAJ_RESULT::END) {
            bCleanEnd = true;
            break;
        }
        if (result == KWAJ_RESULT::IO_ERROR) {
            bIOError = true;
            break;
        }
        if (result != KWAJ_RESULT::OK) {
            bDataError = true;
            break;
        }

        if (nLengthCode != 0) {
            const qint32 nMatchLength = (qint32)nLengthCode + 2;
            quint32 nOffsetHigh = 0;
            result = offsetTree.decode(&reader, &nOffsetHigh);
            if (result == KWAJ_RESULT::END) {
                bDataError = true;
                break;
            }
            if (result == KWAJ_RESULT::IO_ERROR) {
                bIOError = true;
                break;
            }
            if (result != KWAJ_RESULT::OK) {
                bDataError = true;
                break;
            }

            quint32 nOffsetLow = 0;
            result = reader.readBits(6, &nOffsetLow);
            if (result == KWAJ_RESULT::END) {
                bDataError = true;
                break;
            }
            if (result == KWAJ_RESULT::IO_ERROR) {
                bIOError = true;
                break;
            }
            if (result != KWAJ_RESULT::OK) {
                bDataError = true;
                break;
            }

            if (!output.canEmit(nMatchLength)) {
                bDataError = true;
                break;
            }

            const qint32 nOffset = (qint32)((nOffsetHigh << 6) | nOffsetLow);
            for (qint32 i = 0; i < nMatchLength; i++) {
                const quint8 nByte = abWindow[(nWindowPosition + KWAJ_WINDOW_SIZE - nOffset) & KWAJ_WINDOW_MASK];
                abWindow[nWindowPosition] = nByte;
                nWindowPosition = (nWindowPosition + 1) & KWAJ_WINDOW_MASK;
                if (!output.writeByte(nByte)) {
                    bDataError = !pDecompressState->bWriteError;
                    bIOError = pDecompressState->bWriteError;
                    break;
                }
            }
            bUseSecondMatchTree = false;
        } else {
            quint32 nLiteralLengthCode = 0;
            result = literalLengthTree.decode(&reader, &nLiteralLengthCode);
            if (result == KWAJ_RESULT::END) {
                bDataError = true;
                break;
            }
            if (result == KWAJ_RESULT::IO_ERROR) {
                bIOError = true;
                break;
            }
            if (result != KWAJ_RESULT::OK) {
                bDataError = true;
                break;
            }

            const qint32 nLiteralCount = (qint32)nLiteralLengthCode + 1;
            bUseSecondMatchTree = (nLiteralCount != 32);
            for (qint32 i = 0; i < nLiteralCount; i++) {
                quint32 nLiteral = 0;
                result = literalTree.decode(&reader, &nLiteral);
                if (result == KWAJ_RESULT::END) {
                    bDataError = true;
                    break;
                }
                if (result == KWAJ_RESULT::IO_ERROR) {
                    bIOError = true;
                    break;
                }
                if (result != KWAJ_RESULT::OK) {
                    bDataError = true;
                    break;
                }

                abWindow[nWindowPosition] = (quint8)nLiteral;
                nWindowPosition = (nWindowPosition + 1) & KWAJ_WINDOW_MASK;
                if (!output.writeByte((quint8)nLiteral)) {
                    bDataError = !pDecompressState->bWriteError;
                    bIOError = pDecompressState->bWriteError;
                    break;
                }
            }
        }
    }

    const bool bInputComplete = bCleanEnd && reader.isCleanEnd() &&
                                ((pDecompressState->nInputLimit == -1) ||
                                 (pDecompressState->nCountInput == pDecompressState->nInputLimit));
    const bool bOutputComplete = !bHasExpectedSize || (output.produced() == nExpectedSize);
    bool bResult = bInputComplete && bOutputComplete && !bDataError && !bIOError &&
                   !pDecompressState->bReadError && !pDecompressState->bWriteError &&
                   XBinary::isPdStructNotCanceled(pPdStruct);

    if (bResult && !output.flush()) bResult = false;

    return bResult && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
           (pDecompressState->nCountOutput == output.produced()) && XBinary::isPdStructNotCanceled(pPdStruct);
}
