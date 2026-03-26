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
#include "xbrotlidecoder.h"

#include "brotlideclib.cpp"

XBrotliDecoder::XBrotliDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XBrotliDecoder::decompressBlock(const quint8 *pInput, qint64 nInputSize, quint8 *pOutput, qint64 nOutputSize, qint64 *pnBytesWritten)
{
    bool bResult = false;

    size_t nDecodedSize = nOutputSize;

    BrotliDecoderResult ret = BrotliDecoderDecompress(nInputSize, pInput, &nDecodedSize, pOutput);

    if (ret == BROTLI_DECODER_RESULT_SUCCESS) {
        if (pnBytesWritten) {
            *pnBytesWritten = (qint64)nDecodedSize;
        }

        bResult = true;
    }

    return bResult;
}

bool XBrotliDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput) {
        qint32 _nBufferSize = XBinary::getBufferSize(pPdStruct);

        char *bufferIn = new char[_nBufferSize];
        char *bufferOut = new char[_nBufferSize];

        if (pDecompressState->pDeviceInput) {
            pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
        }

        if (pDecompressState->pDeviceOutput) {
            pDecompressState->pDeviceOutput->seek(0);
        }

        BrotliDecoderState *pState = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);

        if (pState) {
            BrotliDecoderResult ret = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
            bool bReadMore = true;
            size_t nAvailIn = 0;
            const uint8_t *pNextIn = nullptr;

            do {
                if (bReadMore && nAvailIn == 0) {
                    qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), _nBufferSize);

                    if (nBufferSize > 0) {
                        nAvailIn = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

                        if (nAvailIn > 0) {
                            pNextIn = (const uint8_t *)bufferIn;
                        } else {
                            bReadMore = false;
                        }
                    } else {
                        bReadMore = false;
                    }
                }

                if (nAvailIn > 0 || !bReadMore) {
                    size_t nAvailOut = _nBufferSize;
                    uint8_t *pNextOut = (uint8_t *)bufferOut;

                    ret = BrotliDecoderDecompressStream(pState, &nAvailIn, &pNextIn, &nAvailOut, &pNextOut, nullptr);

                    if (ret == BROTLI_DECODER_RESULT_ERROR) {
                        break;
                    }

                    qint32 nTemp = _nBufferSize - nAvailOut;

                    if (nTemp > 0) {
                        if (!XBinary::_writeDevice((char *)bufferOut, nTemp, pDecompressState)) {
                            break;
                        }
                    }

                    if (ret == BROTLI_DECODER_RESULT_SUCCESS) {
                        break;
                    }

                    if (ret == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT && !bReadMore) {
                        break;
                    }
                } else {
                    break;
                }

                if (XBinary::isPdStructStopped(pPdStruct)) {
                    break;
                }
            } while (ret != BROTLI_DECODER_RESULT_SUCCESS);

            BrotliDecoderDestroyInstance(pState);

            if (ret == BROTLI_DECODER_RESULT_SUCCESS) {
                bResult = true;
            }
        }

        delete[] bufferIn;
        delete[] bufferOut;
    }

    return bResult;
}

quint32 XBrotliDecoder::version()
{
    return BrotliDecoderVersion();
}

QString XBrotliDecoder::errorString(qint32 nErrorCode)
{
    return QString(BrotliDecoderErrorString((BrotliDecoderErrorCode)nErrorCode));
}
