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
#include "xlzssdecoder.h"

XLZSSDecoder::XLZSSDecoder(QObject *parent) : QObject(parent)
{
}

bool XLZSSDecoder::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pDecompressState->pDeviceOutput) {
        // Initialize error states
        pDecompressState->bReadError = false;
        pDecompressState->bWriteError = false;
        pDecompressState->nCountInput = 0;
        pDecompressState->nCountOutput = 0;

        // Set input device position
        if (pDecompressState->pDeviceInput) {
            pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
        }

        // Set output device position
        if (pDecompressState->pDeviceOutput) {
            pDecompressState->pDeviceOutput->seek(0);
        }

        // LZSS parameters for SZDD format
        const qint32 N_WINDOW_SIZE = 4096;      // Size of sliding window
        const qint32 N_MATCH_MIN_LENGTH = 3;    // Minimum match length
        const qint32 N_MATCH_MAX_LENGTH = 18;   // Maximum match length (3 + 15)

        // Allocate sliding window buffer
        char *pWindowBuffer = new char[N_WINDOW_SIZE];
        if (!pWindowBuffer) {
            return false;
        }

        memset(pWindowBuffer, 0, N_WINDOW_SIZE);
        qint32 nWindowPos = 0;

        // Output buffer
        const qint32 N_OUTPUT_BUFFER_SIZE = 65536;
        char *pOutputBuffer = new char[N_OUTPUT_BUFFER_SIZE];
        if (!pOutputBuffer) {
            delete[] pWindowBuffer;
            return false;
        }

        qint32 nOutputBufferPos = 0;

        // Bit stream variables
        quint8 nFlagByte = 0;
        qint32 nFlagBitPos = 0;

        bResult = true;

        // Main decompression loop
        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            // Read flag byte every 8 iterations
            if (nFlagBitPos == 0) {
                if (XBinary::_readDevice((char *)&nFlagByte, 1, pDecompressState) != 1) {
                    break;  // End of input
                }
            }

            // Check flag bit (LSB is first)
            bool bIsLiteral = (nFlagByte & (1 << nFlagBitPos)) != 0;
            nFlagBitPos = (nFlagBitPos + 1) & 7;

            if (bIsLiteral) {
                // Read literal byte
                char cByte;
                if (XBinary::_readDevice(&cByte, 1, pDecompressState) != 1) {
                    break;  // End of input
                }

                // Write literal to output and window
                pOutputBuffer[nOutputBufferPos++] = cByte;
                pWindowBuffer[nWindowPos] = cByte;
                nWindowPos = (nWindowPos + 1) & (N_WINDOW_SIZE - 1);

                // Flush output buffer if needed
                if (nOutputBufferPos >= N_OUTPUT_BUFFER_SIZE) {
                    if (XBinary::_writeDevice(pOutputBuffer, nOutputBufferPos, pDecompressState) != nOutputBufferPos) {
                        pDecompressState->bWriteError = true;
                        bResult = false;
                        break;
                    }
                    nOutputBufferPos = 0;
                }
            } else {
                // Read match: 2 bytes for position and length
                char bytePos[2];
                if (XBinary::_readDevice(bytePos, 2, pDecompressState) != 2) {
                    break;  // End of input
                }

                // Decode position and length
                // Typically: position (12 bits) and length (4 bits)
                quint16 nEncoded = (quint8)bytePos[0] | ((quint8)bytePos[1] << 8);
                quint16 nPos = (nEncoded >> 4) & 0xFFF;    // 12-bit position
                quint16 nLen = (nEncoded & 0x0F) + N_MATCH_MIN_LENGTH;  // 4-bit length + 3

                // Copy match from window to output
                qint32 nSrcPos = nWindowPos - nPos;
                if (nSrcPos < 0) {
                    nSrcPos += N_WINDOW_SIZE;
                }

                for (quint16 i = 0; i < nLen; i++) {
                    char cByte = pWindowBuffer[(nSrcPos + i) & (N_WINDOW_SIZE - 1)];
                    pOutputBuffer[nOutputBufferPos++] = cByte;
                    pWindowBuffer[nWindowPos] = cByte;
                    nWindowPos = (nWindowPos + 1) & (N_WINDOW_SIZE - 1);

                    // Flush output buffer if needed
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

        // Flush remaining output buffer
        if (bResult && nOutputBufferPos > 0) {
            if (XBinary::_writeDevice(pOutputBuffer, nOutputBufferPos, pDecompressState) != nOutputBufferPos) {
                pDecompressState->bWriteError = true;
                bResult = false;
            }
        }

        // Clean up
        delete[] pWindowBuffer;
        delete[] pOutputBuffer;

        // Success if no errors occurred
        bResult = bResult && !pDecompressState->bReadError && !pDecompressState->bWriteError;
    }

    return bResult;
}
