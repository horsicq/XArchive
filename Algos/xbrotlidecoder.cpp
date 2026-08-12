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
#include "algo_utils.h"
#include <new>

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

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput &&
        (pDecompressState->nInputOffset >= 0) && (pDecompressState->nInputLimit >= -1) &&
        XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRequestedBufferSize = XBinary::getBufferSize(pPdStruct);
        if (nRequestedBufferSize <= 0) return false;
        const qint32 _nBufferSize = qBound((qint32)0x1000, nRequestedBufferSize, (qint32)0x100000);

        char *bufferIn = new (std::nothrow) char[_nBufferSize];
        if (!bufferIn) return false;
        char *bufferOut = new (std::nothrow) char[_nBufferSize];
        if (!bufferOut) {
            delete[] bufferIn;
            return false;
        }

        Algo_utils::prepareState(pDecompressState);
        if (pDecompressState->bReadError || pDecompressState->bWriteError) {
            delete[] bufferIn;
            delete[] bufferOut;
            return false;
        }

        BrotliDecoderState *pState = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);

        if (pState) {
            BrotliDecoderResult ret = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;
            bool bReadMore = true;
            size_t nAvailIn = 0;
            const uint8_t *pNextIn = nullptr;

            do {
                if (bReadMore && nAvailIn == 0) {
                    qint32 nBufferSize = Algo_utils::getReadChunkSize(pDecompressState, _nBufferSize);

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

                    if (nAvailOut > static_cast<size_t>(_nBufferSize)) {
                        break;
                    }

                    qint32 nTemp = _nBufferSize - static_cast<qint32>(nAvailOut);

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

            const bool bConsumedInput = (nAvailIn == 0) &&
                                        ((pDecompressState->nInputLimit == -1) ||
                                         (pDecompressState->nCountInput == pDecompressState->nInputLimit));
            const bool bExpectedOutput = !pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE) ||
                                         ((pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() >= 0) &&
                                          (pDecompressState->nCountOutput ==
                                           pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong()));
            bResult = (ret == BROTLI_DECODER_RESULT_SUCCESS) && bConsumedInput && bExpectedOutput &&
                      !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
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
