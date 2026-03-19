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
#include "xzstddecoder.h"

XZstdDecoder::XZstdDecoder(QObject *parent) : QObject(parent)
{
}

bool XZstdDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
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

        ZSTD_DStream *pDStream = ZSTD_createDStream();

        if (pDStream) {
            size_t nInitResult = ZSTD_initDStream(pDStream);

            if (!ZSTD_isError(nInitResult)) {
                bool bReadMore = true;
                bool bFinished = false;

                ZSTD_inBuffer input = {};
                ZSTD_outBuffer output = {};

                do {
                    if (bReadMore && input.pos >= input.size) {
                        qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), _nBufferSize);

                        if (nBufferSize > 0) {
                            qint32 nRead = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

                            if (nRead > 0) {
                                input.src = bufferIn;
                                input.size = nRead;
                                input.pos = 0;
                            } else {
                                bReadMore = false;
                            }
                        } else {
                            bReadMore = false;
                        }
                    }

                    if (input.pos < input.size) {
                        output.dst = bufferOut;
                        output.size = _nBufferSize;
                        output.pos = 0;

                        size_t nRet = ZSTD_decompressStream(pDStream, &output, &input);

                        if (ZSTD_isError(nRet)) {
                            break;
                        }

                        if (output.pos > 0) {
                            if (!XBinary::_writeDevice(bufferOut, (qint32)output.pos, pDecompressState)) {
                                break;
                            }
                        }

                        if (nRet == 0) {
                            bFinished = true;
                            break;
                        }
                    } else if (!bReadMore) {
                        break;
                    }

                    if (XBinary::isPdStructStopped(pPdStruct)) {
                        break;
                    }
                } while (true);

                if (bFinished) {
                    bResult = true;
                }
            }

            ZSTD_freeDStream(pDStream);
        }

        delete[] bufferIn;
        delete[] bufferOut;
    }

    return bResult;
}
