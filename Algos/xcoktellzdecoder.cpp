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
#include "xcoktellzdecoder.h"
#include "algo_utils.h"

XCoktelLZDecoder::XCoktelLZDecoder(QObject *parent) : QObject(parent)
{
}

bool XCoktelLZDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput) {
        Algo_utils::prepareState(pDecompressState);

        const qint32 N_WINDOW_SIZE = 4096;    // Sliding window size
        const qint32 N_MATCH_MIN_LENGTH = 3;  // Minimum match length

        // The uncompressed size is authoritative: the LZSS stream can end mid-match and the last
        // flag byte can carry unused bits, so decoding must stop by output count, not input EOF.
        const qint64 nUncompSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();

        char *pWindowBuffer = new char[N_WINDOW_SIZE];
        if (!pWindowBuffer) {
            return false;
        }
        memset(pWindowBuffer, ' ', N_WINDOW_SIZE);
        qint32 nWindowPos = 4078;  // N - F for a 4096/18 window (classic Okumura start position)

        const qint32 N_OUTPUT_BUFFER_SIZE = 65536;
        char *pOutputBuffer = new char[N_OUTPUT_BUFFER_SIZE];
        if (!pOutputBuffer) {
            delete[] pWindowBuffer;
            return false;
        }
        qint32 nOutputBufferPos = 0;
        qint64 nTotalDecoded = 0;

        quint8 nFlagByte = 0;
        qint32 nFlagBitPos = 0;

        bResult = true;

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nUncompSize != -1) && (nTotalDecoded >= nUncompSize)) {
                break;
            }

            if (nFlagBitPos == 0) {
                if (XBinary::_readDevice((char *)&nFlagByte, 1, pDecompressState) != 1) {
                    // EOF is valid only for streams without a declared output
                    // size. A sized STK member must produce exactly that size.
                    if ((nUncompSize >= 0) && (nTotalDecoded < nUncompSize)) {
                        pDecompressState->bReadError = true;
                        bResult = false;
                    }
                    break;
                }
            }

            bool bIsLiteral = (nFlagByte & (1 << nFlagBitPos)) != 0;
            nFlagBitPos = (nFlagBitPos + 1) & 7;

            if (bIsLiteral) {
                char cByte;
                if (XBinary::_readDevice(&cByte, 1, pDecompressState) != 1) {
                    pDecompressState->bReadError = true;
                    bResult = false;
                    break;
                }

                pOutputBuffer[nOutputBufferPos++] = cByte;
                pWindowBuffer[nWindowPos] = cByte;
                nWindowPos = (nWindowPos + 1) & (N_WINDOW_SIZE - 1);
                nTotalDecoded++;

                if (nOutputBufferPos >= N_OUTPUT_BUFFER_SIZE) {
                    if (XBinary::_writeDevice(pOutputBuffer, nOutputBufferPos, pDecompressState) != nOutputBufferPos) {
                        pDecompressState->bWriteError = true;
                        bResult = false;
                        break;
                    }
                    nOutputBufferPos = 0;
                }
            } else {
                char bytePos[2];
                if (XBinary::_readDevice(bytePos, 2, pDecompressState) != 2) {
                    pDecompressState->bReadError = true;
                    bResult = false;
                    break;
                }

                const quint8 nFirstByte = static_cast<quint8>(bytePos[0]);
                const quint8 nSecondByte = static_cast<quint8>(bytePos[1]);
                const quint16 nLen = (nSecondByte & 0x0F) + N_MATCH_MIN_LENGTH;        // 4-bit length + 3
                const qint32 nPos = (((nSecondByte & 0xF0) << 4) | nFirstByte) & (N_WINDOW_SIZE - 1);  // no bias (Coktel)

                for (quint16 i = 0; i < nLen; i++) {
                    if ((nUncompSize != -1) && (nTotalDecoded >= nUncompSize)) {
                        break;  // last match may cross the uncompressed size; copy only up to it
                    }

                    char cByte = pWindowBuffer[(nPos + i) & (N_WINDOW_SIZE - 1)];
                    pOutputBuffer[nOutputBufferPos++] = cByte;
                    pWindowBuffer[nWindowPos] = cByte;
                    nWindowPos = (nWindowPos + 1) & (N_WINDOW_SIZE - 1);
                    nTotalDecoded++;

                    if (nOutputBufferPos >= N_OUTPUT_BUFFER_SIZE) {
                        if (XBinary::_writeDevice(pOutputBuffer, nOutputBufferPos, pDecompressState) != nOutputBufferPos) {
                            pDecompressState->bWriteError = true;
                            bResult = false;
                            break;
                        }
                        nOutputBufferPos = 0;
                    }
                }

                if (!bResult) {
                    break;
                }
            }
        }

        if (bResult && (nUncompSize >= 0) && (nTotalDecoded != nUncompSize)) {
            pDecompressState->bReadError = true;
            bResult = false;
        }

        if (bResult && (nOutputBufferPos > 0)) {
            if (XBinary::_writeDevice(pOutputBuffer, nOutputBufferPos, pDecompressState) != nOutputBufferPos) {
                pDecompressState->bWriteError = true;
                bResult = false;
            }
        }

        delete[] pWindowBuffer;
        delete[] pOutputBuffer;

        bResult = bResult && XBinary::isPdStructNotCanceled(pPdStruct) && !pDecompressState->bReadError && !pDecompressState->bWriteError;
    }

    return bResult;
}
