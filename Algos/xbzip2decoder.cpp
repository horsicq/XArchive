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
#include "xbzip2decoder.h"

XBZIP2Decoder::XBZIP2Decoder(QObject *parent) : QObject(parent)
{
}

bool XBZIP2Decoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    const qint32 N_BUFFER_SIZE = 0x4000;

    char bufferIn[N_BUFFER_SIZE];
    char bufferOut[N_BUFFER_SIZE];

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput) {
        if (pDecompressState->pDeviceInput) {
            pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
        }

        if (pDecompressState->pDeviceOutput) {
            pDecompressState->pDeviceOutput->seek(0);
        }

        bz_stream strm = {};
        qint32 ret = BZ_MEM_ERROR;
        bool bReadMore = true;

        qint32 rc = BZ2_bzDecompressInit(&strm, 0, 0);

        if (rc == BZ_OK) {
            do {
                // Read more data only if we consumed all input
                if (bReadMore && strm.avail_in == 0) {
                    qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), N_BUFFER_SIZE);

                    if (nBufferSize > 0) {
                        strm.avail_in = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

                        if (strm.avail_in > 0) {
                            strm.next_in = bufferIn;
                        } else {
                            // No more data available from device - signal to stop reading
                            bReadMore = false;
                        }
                    } else {
                        // nBufferSize is 0 - we've reached input limit
                        bReadMore = false;
                    }
                }

                // If we have input or previous buffered data, decompress
                if (strm.avail_in > 0 || bReadMore == false) {
                    strm.total_in_hi32 = 0;
                    strm.total_in_lo32 = 0;
                    strm.total_out_hi32 = 0;
                    strm.total_out_lo32 = 0;
                    strm.avail_out = N_BUFFER_SIZE;
                    strm.next_out = bufferOut;
                    ret = BZ2_bzDecompress(&strm);

                    if ((ret != BZ_STREAM_END) && (ret != BZ_OK)) {
                        break;
                    }

                    qint32 nTemp = N_BUFFER_SIZE - strm.avail_out;

                    if (nTemp > 0) {
                        if (!XBinary::_writeDevice((char *)bufferOut, nTemp, pDecompressState)) {
                            ret = BZ_MEM_ERROR;
                            break;
                        }
                    }
                } else {
                    // No input and nothing to read
                    ret = BZ_MEM_ERROR;
                    break;
                }

                if (XBinary::isPdStructStopped(pPdStruct)) {
                    break;
                }
            } while (ret != BZ_STREAM_END);

            BZ2_bzDecompressEnd(&strm);

            if (ret == BZ_STREAM_END) {
                bResult = true;
            }
        }
    }

    return bResult;
}
