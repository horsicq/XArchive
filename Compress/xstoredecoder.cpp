/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xstoredecoder.h"

const qint32 N_BUFFER_SIZE = 65536;

XStoreDecoder::XStoreDecoder(QObject *parent) : QObject(parent)
{
}

bool XStoreDecoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput) {
        // Initialize error states
        pDecompressState->bReadError = false;
        pDecompressState->bWriteError = false;
        pDecompressState->nCountInput = 0;
        pDecompressState->nCountOutput = 0;

        // Set input device position
        if (pDecompressState->nInputOffset > 0) {
            pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
        }

        // Set output device position
        if (pDecompressState->pDeviceOutput) {
            pDecompressState->pDeviceOutput->seek(0);
        }

        // Allocate buffer for copying data
        char bufferIn[N_BUFFER_SIZE];

        // Copy data from input to output
        for (qint64 nOffset = 0; (nOffset < pDecompressState->nInputLimit) && XBinary::isPdStructNotCanceled(pPdStruct);) {
            qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - nOffset), N_BUFFER_SIZE);

            qint32 nRead = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

            if (nRead > 0) {
                XBinary::_writeDevice(bufferIn, nRead, pDecompressState);
            } else {
                break;
            }

            if (pDecompressState->bReadError || pDecompressState->bWriteError) {
                break;
            }

            nOffset += nRead;
        }

        // Success if no errors occurred
        bResult = !pDecompressState->bReadError && !pDecompressState->bWriteError;
    }

    return bResult;
}
