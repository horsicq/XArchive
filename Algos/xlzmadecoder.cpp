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
#include "xlzmadecoder.h"

static void *SzAlloc(ISzAllocPtr, size_t size)
{
    return malloc(size);
}

static void SzFree(ISzAllocPtr, void *address)
{
    free(address);
}

static ISzAlloc g_Alloc = {SzAlloc, SzFree};

XLZMADecoder::XLZMADecoder(QObject *parent) : QObject(parent)
{
}

bool XLZMADecoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
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

        if (pDecompressState->nInputLimit >= 4) {
            qint32 nPropSize = 0;
            char header1[4] = {};
            quint8 properties[32] = {};

            XBinary::_readDevice(header1, sizeof(header1), pDecompressState);
            // if (header1[0] != 0x5D || header1[1] != 0x00 || header1[2] != 0x00 || header1[3] != 0x00) {
            //     emit errorMessage(tr("Invalid LZMA header"));
            //     return false;
            // }
            nPropSize = header1[2];  // TODO Check

            if (nPropSize && (nPropSize < 30)) {
                XBinary::_readDevice((char *)properties, nPropSize, pDecompressState);

                CLzmaDec state = {};

                SRes ret = LzmaProps_Decode(&state.prop, (Byte *)properties, nPropSize);

                if (ret == 0)  // S_OK
                {
                    LzmaDec_Construct(&state);
                    ret = LzmaDec_Allocate(&state, (Byte *)properties, nPropSize, &g_Alloc);

                    if (ret == 0)  // S_OK
                    {
                        LzmaDec_Init(&state);
                        bResult = true;  // Assume success, set to false on error
                        ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;

                        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
                            qint32 nBufferSize = qMin((qint32)(pDecompressState->nInputLimit - pDecompressState->nCountInput), N_BUFFER_SIZE);
                            qint32 nSize = XBinary::_readDevice(bufferIn, nBufferSize, pDecompressState);

                            // Process available input
                            qint64 nPos = 0;
                            bool bContinueReading = true;

                            while (bContinueReading && nPos < nSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
                                ELzmaStatus status;
                                SizeT inProcessed = nSize - nPos;
                                SizeT outProcessed = N_BUFFER_SIZE;

                                ret = LzmaDec_DecodeToBuf(&state, (Byte *)bufferOut, &outProcessed, (Byte *)(bufferIn + nPos), &inProcessed, LZMA_FINISH_ANY, &status);

                                if (ret != 0) {  // Check for decompression error
                                    bResult = false;
                                    bContinueReading = false;
                                    break;
                                }

                                nPos += inProcessed;

                                if (outProcessed > 0) {
                                    if (!XBinary::_writeDevice((char *)bufferOut, (qint32)outProcessed, pDecompressState)) {
                                        bResult = false;
                                        bContinueReading = false;
                                        break;
                                    }
                                }

                                lastStatus = status;

                                if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
                                    // Decompression completed successfully
                                    bContinueReading = false;
                                    break;
                                }

                                // If we couldn't process any input, stop
                                if (inProcessed == 0) {
                                    break;
                                }
                            }

                            // If we got stream end mark, stop reading more
                            if (lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) {
                                break;
                            }

                            // If no data was read, we're done
                            if (nSize == 0) {
                                break;
                            }
                        }
                    }

                    LzmaDec_Free(&state, &g_Alloc);
                }
            }
        }
    }

    return bResult;
}
