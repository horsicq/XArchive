/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xit214decoder.h"
#include "algo_utils.h"

#include <algorithm>

XIT214Decoder::XIT214Decoder(QObject *parent) : QObject(parent)
{
}

bool XIT214Decoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, quint8 nBits, bool bIs215, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput ||
        ((nBits != 8) && (nBits != 16)) || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1) ||
        !pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        return false;
    }

    const qint64 nExpectedOutput =
        pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    const qint32 nBytesPerSample = nBits / 8;
    if ((nExpectedOutput < 0) || ((nExpectedOutput % nBytesPerSample) != 0)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint32 nMaxBlockSamples = (nBits == 8) ? 0x8000 : 0x4000;
    QByteArray baOutput(nMaxBlockSamples * nBytesPerSample, 0);
    bool bResult = true;

    while ((pDecompressState->nCountOutput < nExpectedOutput) && bResult &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRemainingBytes = nExpectedOutput - pDecompressState->nCountOutput;
        const qint32 nBlockSamples =
            (qint32)(std::min)((qint64)nMaxBlockSamples, nRemainingBytes / nBytesPerSample);
        if (nBlockSamples <= 0) {
            bResult = false;
            break;
        }

        STATE state = {};
        if (!readBlock(&state, pDecompressState, pPdStruct)) {
            bResult = false;
            break;
        }

        qint32 nBlockPosition = 0;
        quint8 nWidth = (nBits == 8) ? 9 : 17;

        if (nBits == 8) {
            quint8 nD1 = 0;
            quint8 nD2 = 0;

            while ((nBlockPosition < nBlockSamples) && !state.bError &&
                   XBinary::isPdStructNotCanceled(pPdStruct)) {
                quint32 nValue = readbits(&state, nWidth);
                if (state.bError) break;

                bool bWidthChange = false;
                quint32 nNewWidth = nWidth;
                if (nWidth < 7) {
                    if (nValue == (1U << (nWidth - 1))) {
                        nValue = readbits(&state, 3);
                        if (state.bError) break;
                        nValue++;
                        nNewWidth = (nValue < nWidth) ? nValue : nValue + 1;
                        bWidthChange = true;
                    }
                } else if (nWidth < 9) {
                    const quint32 nBorder = (0xFFU >> (9 - nWidth)) - 4U;
                    if ((nValue > nBorder) && (nValue <= (nBorder + 8U))) {
                        nValue -= nBorder;
                        nNewWidth = (nValue < nWidth) ? nValue : nValue + 1;
                        bWidthChange = true;
                    }
                } else if (nWidth == 9) {
                    if (nValue & 0x100U) {
                        nNewWidth = (nValue + 1U) & 0xFFU;
                        bWidthChange = true;
                    }
                } else {
                    state.bError = true;
                    break;
                }

                if (bWidthChange) {
                    if ((nNewWidth < 1) || (nNewWidth > 9)) {
                        state.bError = true;
                        break;
                    }
                    nWidth = (quint8)nNewWidth;
                    continue;
                }

                qint32 nSignedValue = (qint32)nValue;
                if (nWidth < 8) {
                    const quint32 nSignBit = 1U << (nWidth - 1);
                    if (nValue & nSignBit) nSignedValue -= (qint32)(1U << nWidth);
                } else {
                    nSignedValue = (qint8)(quint8)nValue;
                }

                nD1 = (quint8)(nD1 + (quint8)nSignedValue);
                nD2 = (quint8)(nD2 + nD1);
                baOutput[nBlockPosition++] = (char)(bIs215 ? nD2 : nD1);
            }
        } else {
            quint16 nD1 = 0;
            quint16 nD2 = 0;

            while ((nBlockPosition < nBlockSamples) && !state.bError &&
                   XBinary::isPdStructNotCanceled(pPdStruct)) {
                quint32 nValue = readbits(&state, nWidth);
                if (state.bError) break;

                bool bWidthChange = false;
                quint32 nNewWidth = nWidth;
                if (nWidth < 7) {
                    if (nValue == (1U << (nWidth - 1))) {
                        nValue = readbits(&state, 4);
                        if (state.bError) break;
                        nValue++;
                        nNewWidth = (nValue < nWidth) ? nValue : nValue + 1;
                        bWidthChange = true;
                    }
                } else if (nWidth < 17) {
                    const quint32 nBorder = (0xFFFFU >> (17 - nWidth)) - 8U;
                    if ((nValue > nBorder) && (nValue <= (nBorder + 16U))) {
                        nValue -= nBorder;
                        nNewWidth = (nValue < nWidth) ? nValue : nValue + 1;
                        bWidthChange = true;
                    }
                } else if (nWidth == 17) {
                    if (nValue & 0x10000U) {
                        nNewWidth = (nValue + 1U) & 0xFFU;
                        bWidthChange = true;
                    }
                } else {
                    state.bError = true;
                    break;
                }

                if (bWidthChange) {
                    if ((nNewWidth < 1) || (nNewWidth > 17)) {
                        state.bError = true;
                        break;
                    }
                    nWidth = (quint8)nNewWidth;
                    continue;
                }

                qint32 nSignedValue = (qint32)nValue;
                if (nWidth < 16) {
                    const quint32 nSignBit = 1U << (nWidth - 1);
                    if (nValue & nSignBit) nSignedValue -= (qint32)(1U << nWidth);
                } else {
                    nSignedValue = (qint16)(quint16)nValue;
                }

                nD1 = (quint16)(nD1 + (quint16)nSignedValue);
                nD2 = (quint16)(nD2 + nD1);
                const quint16 nSample = bIs215 ? nD2 : nD1;
                const qint32 nOutputOffset = nBlockPosition * 2;
                baOutput[nOutputOffset] = (char)(nSample & 0xFF);
                baOutput[nOutputOffset + 1] = (char)(nSample >> 8);
                nBlockPosition++;
            }
        }

        if (state.bError || (nBlockPosition != nBlockSamples) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            pDecompressState->bReadError = pDecompressState->bReadError || state.bError;
            bResult = false;
            break;
        }

        const qint32 nOutputBytes = nBlockSamples * nBytesPerSample;
        if (XBinary::_writeDevice(baOutput.constData(), nOutputBytes, pDecompressState) != nOutputBytes) {
            bResult = false;
            break;
        }
    }

    const bool bInputConsumed = (pDecompressState->nInputLimit == -1)
                                    ? pDecompressState->pDeviceInput->atEnd()
                                    : (pDecompressState->nCountInput == pDecompressState->nInputLimit);
    return bResult && bInputConsumed && (pDecompressState->nCountOutput == nExpectedOutput) &&
           !pDecompressState->bReadError && !pDecompressState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}

quint32 XIT214Decoder::readbits(STATE *pState, quint8 n)
{
    if (!pState || (n == 0) || (n > 32)) {
        if (pState) pState->bError = true;
        return 0;
    }

    const quint64 nTotalBits = (quint64)pState->baInput.size() * 8;
    if ((pState->nBitPosition > nTotalBits) || ((quint64)n > (nTotalBits - pState->nBitPosition))) {
        pState->bError = true;
        return 0;
    }

    quint32 nResult = 0;
    quint8 nOutputShift = 0;
    quint8 nRemaining = n;
    while (nRemaining > 0) {
        const quint64 nByteIndex = pState->nBitPosition / 8;
        const quint8 nBitOffset = (quint8)(pState->nBitPosition % 8);
        const quint8 nTake = (std::min)((quint8)(8 - nBitOffset), nRemaining);
        const quint32 nMask = (1U << nTake) - 1U;
        const quint32 nByte = (quint8)pState->baInput.at((qint32)nByteIndex);
        nResult |= ((nByte >> nBitOffset) & nMask) << nOutputShift;
        pState->nBitPosition += nTake;
        nOutputShift += nTake;
        nRemaining -= nTake;
    }

    return nResult;
}

bool XIT214Decoder::readBlock(STATE *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pDecompressState || !pDecompressState->pDeviceInput) return false;

    char header[2] = {};
    qint32 nHeaderRead = 0;
    while ((nHeaderRead < (qint32)sizeof(header)) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRequest = Algo_utils::getReadChunkSize(pDecompressState, (qint32)sizeof(header) - nHeaderRead);
        if (nRequest <= 0) break;
        const qint32 nRead = XBinary::_readDevice(header + nHeaderRead, nRequest, pDecompressState);
        if (nRead <= 0) break;
        nHeaderRead += nRead;
    }

    if ((nHeaderRead != (qint32)sizeof(header)) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pDecompressState->bReadError = true;
        return false;
    }

    const qint32 nInputBufferSize = (quint8)header[0] | ((qint32)(quint8)header[1] << 8);
    if ((nInputBufferSize <= 0) ||
        ((pDecompressState->nInputLimit != -1) &&
         (nInputBufferSize > (pDecompressState->nInputLimit - pDecompressState->nCountInput)))) {
        pDecompressState->bReadError = true;
        return false;
    }

    pState->baInput.resize(nInputBufferSize);
    qint32 nPayloadRead = 0;
    while ((nPayloadRead < nInputBufferSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRequest = Algo_utils::getReadChunkSize(pDecompressState, nInputBufferSize - nPayloadRead);
        if (nRequest <= 0) break;
        const qint32 nRead = XBinary::_readDevice(pState->baInput.data() + nPayloadRead, nRequest, pDecompressState);
        if (nRead <= 0) break;
        nPayloadRead += nRead;
    }

    if ((nPayloadRead != nInputBufferSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        pDecompressState->bReadError = true;
        pState->baInput.clear();
        return false;
    }

    pState->nBitPosition = 0;
    pState->bError = false;
    return true;
}
