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
#include "xadcdecoder.h"
#include "algo_utils.h"

namespace {
const qint32 ADC_WINDOW_SIZE = 0x10000;  // the long chunk's 16-bit distance field reaches 65536 bytes back
const qint32 ADC_WINDOW_MASK = ADC_WINDOW_SIZE - 1;
const qint32 ADC_INPUT_BUFFER_SIZE = 0x10000;
const qint32 ADC_OUTPUT_BUFFER_SIZE = 0x10000;

// Bounded, refilling view over pState->pDeviceInput.  XBinary::_readDevice
// already honours nInputLimit; nConsumed counts only bytes the grammar used so
// the caller can tell "decoded the whole stripe" from "stopped early".
struct ADC_INPUT {
    XBinary::DATAPROCESS_STATE *pState;
    char *pBuffer;
    qint32 nAvailable;
    qint32 nPosition;
    qint64 nConsumed;
    bool bExhausted;
};

bool adcReadByte(ADC_INPUT *pInput, quint8 *pValue)
{
    if (pInput->nPosition >= pInput->nAvailable) {
        if (pInput->bExhausted) return false;
        const qint32 nRead = XBinary::_readDevice(pInput->pBuffer, ADC_INPUT_BUFFER_SIZE, pInput->pState);
        if ((nRead <= 0) || pInput->pState->bReadError) {
            pInput->bExhausted = true;
            return false;
        }
        pInput->nAvailable = nRead;
        pInput->nPosition = 0;
    }
    *pValue = (quint8)pInput->pBuffer[pInput->nPosition];
    pInput->nPosition++;
    pInput->nConsumed++;
    return true;
}

struct ADC_OUTPUT {
    XBinary::DATAPROCESS_STATE *pState;
    char *pWindow;
    char *pBuffer;
    qint32 nPending;
    qint64 nCount;
};

bool adcFlush(ADC_OUTPUT *pOutput)
{
    if (pOutput->nPending == 0) return true;
    const qint32 nWritten = XBinary::_writeDevice(pOutput->pBuffer, pOutput->nPending, pOutput->pState);
    if (nWritten != pOutput->nPending) {
        pOutput->pState->bWriteError = true;
        return false;
    }
    pOutput->nPending = 0;
    return true;
}

bool adcEmit(ADC_OUTPUT *pOutput, char cValue)
{
    pOutput->pBuffer[pOutput->nPending] = cValue;
    pOutput->nPending++;
    pOutput->pWindow[(qint32)(pOutput->nCount & ADC_WINDOW_MASK)] = cValue;
    pOutput->nCount++;
    return (pOutput->nPending < ADC_OUTPUT_BUFFER_SIZE) || adcFlush(pOutput);
}
}  // namespace

XADCDecoder::XADCDecoder(QObject *parent) : QObject(parent)
{
}

bool XADCDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) return false;

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

    const qint64 nOutputLimit = pDecompressState->nProcessedLimit;  // -1: no limit, decode the whole bounded input
    if (nOutputLimit < -1) return false;

    QByteArray baInput(ADC_INPUT_BUFFER_SIZE, 0);
    QByteArray baWindow(ADC_WINDOW_SIZE, 0);
    QByteArray baOutput(ADC_OUTPUT_BUFFER_SIZE, 0);

    ADC_INPUT input = {pDecompressState, baInput.data(), 0, 0, 0, false};
    ADC_OUTPUT output = {pDecompressState, baWindow.data(), baOutput.data(), 0, 0};

    bool bResult = true;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nOutputLimit != -1) && (output.nCount >= nOutputLimit)) break;

        quint8 nHead = 0;
        if (!adcReadByte(&input, &nHead)) {
            // Input ended on a chunk boundary: fine only when nobody asked for
            // a specific output size (a limited decode must reach its limit).
            bResult = (nOutputLimit == -1) && !pDecompressState->bReadError;
            break;
        }

        if (nHead & 0x80) {
            const qint32 nLength = (qint32)(nHead & 0x7F) + 1;
            if ((nOutputLimit != -1) && ((qint64)nLength > (nOutputLimit - output.nCount))) {
                bResult = false;
                break;
            }
            for (qint32 i = 0; i < nLength; i++) {
                quint8 nByte = 0;
                if (!adcReadByte(&input, &nByte) || !adcEmit(&output, (char)nByte)) {
                    bResult = false;
                    break;
                }
            }
            if (!bResult) break;
        } else {
            qint32 nLength = 0;
            qint32 nDistance = 0;
            if (nHead & 0x40) {
                quint8 nHigh = 0;
                quint8 nLow = 0;
                if (!adcReadByte(&input, &nHigh) || !adcReadByte(&input, &nLow)) {
                    bResult = false;
                    break;
                }
                nLength = (qint32)(nHead & 0x3F) + 4;
                nDistance = (((qint32)nHigh << 8) | (qint32)nLow) + 1;
            } else {
                quint8 nLow = 0;
                if (!adcReadByte(&input, &nLow)) {
                    bResult = false;
                    break;
                }
                nLength = (qint32)((nHead >> 2) & 0x0F) + 3;
                nDistance = ((((qint32)nHead & 0x03) << 8) | (qint32)nLow) + 1;
            }

            if ((qint64)nDistance > output.nCount) {
                bResult = false;  // reference before the first output byte
                break;
            }
            if ((nOutputLimit != -1) && ((qint64)nLength > (nOutputLimit - output.nCount))) {
                bResult = false;
                break;
            }
            for (qint32 i = 0; i < nLength; i++) {
                // Read before write: at distance 65536 the source slot is the
                // one this byte is about to occupy.
                const char cByte = output.pWindow[(qint32)((output.nCount - (qint64)nDistance) & ADC_WINDOW_MASK)];
                if (!adcEmit(&output, cByte)) {
                    bResult = false;
                    break;
                }
            }
            if (!bResult) break;
        }
    }

    if (!adcFlush(&output)) bResult = false;

    pDecompressState->nCountInput = input.nConsumed;

    bResult = bResult && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct) &&
              ((nOutputLimit == -1) || (output.nCount == nOutputLimit)) && (pDecompressState->nCountOutput == output.nCount);

    return bResult;
}
