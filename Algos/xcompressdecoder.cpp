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
#include "xcompressdecoder.h"
#include <string.h>

// Unix compress (.Z) LZW decompression
// Format: magic(2: 0x1F 0x9D) + flags(1) + LZW data
// Flags byte: bits 0-4 = max code bits (9..16), bit 7 = block_compress mode
// Codes are packed LSB-first (least significant bit first)
// Initial code size is 9 bits, grows up to maxbits
// Code 256 = CLEAR (reset table) when block_compress is set

#define COMPRESS_MAGIC_0    0x1F
#define COMPRESS_MAGIC_1    0x9D
#define COMPRESS_CLEAR      256
#define COMPRESS_FIRST      257
#define COMPRESS_MINBITS    9
#define COMPRESS_MAXBITS    16
#define COMPRESS_TABLESIZE  (1 << COMPRESS_MAXBITS)

XCompressDecoder::XCompressDecoder(QObject *parent) : QObject(parent)
{
}

bool XCompressDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pInput = pDecompressState->pDeviceInput;
    QIODevice *pOutput = pDecompressState->pDeviceOutput;

    pInput->seek(pDecompressState->nInputOffset);
    pOutput->seek(0);

    // Read 3-byte header: magic(2) + flags(1)
    quint8 header[3];
    if (pInput->read((char *)header, 3) != 3) {
        return false;
    }

    if (header[0] != COMPRESS_MAGIC_0 || header[1] != COMPRESS_MAGIC_1) {
        return false;
    }

    qint32 nMaxBits = header[2] & 0x1F;
    bool bBlockCompress = (header[2] & 0x80) != 0;

    if (nMaxBits < COMPRESS_MINBITS || nMaxBits > COMPRESS_MAXBITS) {
        return false;
    }

    qint32 nMaxCode = (1 << nMaxBits);

    // Allocate LZW table
    // Each entry: prefix code + suffix byte
    quint16 *pPrefix = new quint16[nMaxCode];
    quint8 *pSuffix = new quint8[nMaxCode];
    quint8 *pStack = new quint8[nMaxCode];

    // Initialize table with single-byte codes (0..255)
    for (qint32 i = 0; i < 256; i++) {
        pPrefix[i] = 0;
        pSuffix[i] = (quint8)i;
    }

    qint32 nNextCode = bBlockCompress ? COMPRESS_FIRST : 256;
    qint32 nCodeBits = COMPRESS_MINBITS;
    qint32 nMaxVal = (1 << nCodeBits);

    // Bit buffer for reading variable-width codes (LSB-first)
    quint64 nBitBuf = 0;
    qint32 nBitsInBuf = 0;

    // Read buffer
    const qint32 READBUF_SIZE = 4096;
    quint8 readBuf[4096];
    qint32 nReadPos = 0;
    qint32 nReadLen = 0;

    // Output buffer
    const qint32 OUTBUF_SIZE = 4096;
    quint8 outBuf[4096];
    qint32 nOutPos = 0;

    // Lambda to read one code
    qint64 nTotalInput = 3;  // header already read

    // nBitsRead tracks how many code-aligned bits have been consumed.
    // Unix compress pads to code-size boundaries, so after a CLEAR we must
    // skip to the next nCodeBits-aligned boundary.
    qint64 nBitsRead = 0;

    auto readCode = [&]() -> qint32 {
        while (nBitsInBuf < nCodeBits) {
            if (nReadPos >= nReadLen) {
                qint64 nToRead = READBUF_SIZE;
                if (pDecompressState->nInputLimit > 0) {
                    qint64 nRemaining = pDecompressState->nInputLimit - nTotalInput;
                    if (nRemaining <= 0) return -1;
                    if (nToRead > nRemaining) nToRead = nRemaining;
                }
                nReadLen = (qint32)pInput->read((char *)readBuf, nToRead);
                if (nReadLen <= 0) return -1;
                nTotalInput += nReadLen;
                nReadPos = 0;
            }
            nBitBuf |= ((quint64)readBuf[nReadPos++]) << nBitsInBuf;
            nBitsInBuf += 8;
        }

        qint32 nCode = (qint32)(nBitBuf & ((1 << nCodeBits) - 1));
        nBitBuf >>= nCodeBits;
        nBitsInBuf -= nCodeBits;
        nBitsRead += nCodeBits;

        return nCode;
    };

    auto flushOutput = [&]() -> bool {
        if (nOutPos > 0) {
            qint64 nWritten = pOutput->write((char *)outBuf, nOutPos);
            if (nWritten != nOutPos) return false;
            nOutPos = 0;
        }
        return true;
    };

    qint64 nTotalOutput = 0;
    bool bResult = true;

    // Read first code (must be a literal 0..255)
    qint32 nOldCode = readCode();
    if (nOldCode < 0 || nOldCode >= 256) {
        delete[] pPrefix;
        delete[] pSuffix;
        delete[] pStack;
        return false;
    }

    quint8 nFinChar = (quint8)nOldCode;
    outBuf[nOutPos++] = nFinChar;
    nTotalOutput++;

    // Main decompression loop
    while (true) {
        if (pPdStruct && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            bResult = false;
            break;
        }

        qint32 nCode = readCode();
        if (nCode < 0) {
            break;  // End of input
        }

        // Handle CLEAR code in block_compress mode
        if (bBlockCompress && nCode == COMPRESS_CLEAR) {
            // Reset table
            nNextCode = COMPRESS_FIRST;
            nCodeBits = COMPRESS_MINBITS;
            nMaxVal = (1 << nCodeBits);

            // After CLEAR, discard remaining bits up to next nCodeBits boundary
            // Unix compress aligns bit reads to code-size groups.
            // Flush the bit buffer - remaining bits in current byte group are discarded.
            nBitBuf = 0;
            nBitsInBuf = 0;

            nCode = readCode();
            if (nCode < 0) {
                break;
            }

            nOldCode = nCode;
            nFinChar = (quint8)nCode;
            outBuf[nOutPos++] = nFinChar;
            nTotalOutput++;
            if (nOutPos >= OUTBUF_SIZE) {
                if (!flushOutput()) { bResult = false; break; }
            }
            continue;
        }

        qint32 nInCode = nCode;
        qint32 nStackTop = 0;

        // If code is not yet in table, handle the special KwKwK case
        if (nCode >= nNextCode) {
            if (nCode > nNextCode) {
                // Invalid code
                bResult = false;
                break;
            }
            pStack[nStackTop++] = nFinChar;
            nCode = nOldCode;
        }

        // Chase prefix chain to build output string (in reverse)
        while (nCode >= 256) {
            if (nStackTop >= nMaxCode) {
                bResult = false;
                break;
            }
            pStack[nStackTop++] = pSuffix[nCode];
            nCode = pPrefix[nCode];
        }

        if (!bResult) break;

        nFinChar = pSuffix[nCode];
        pStack[nStackTop++] = nFinChar;

        // Output in correct order (reverse of stack)
        for (qint32 i = nStackTop - 1; i >= 0; i--) {
            outBuf[nOutPos++] = pStack[i];
            nTotalOutput++;
            if (nOutPos >= OUTBUF_SIZE) {
                if (!flushOutput()) { bResult = false; break; }
            }
        }
        if (!bResult) break;

        // Add new entry to table
        if (nNextCode < nMaxCode) {
            pPrefix[nNextCode] = (quint16)nOldCode;
            pSuffix[nNextCode] = nFinChar;
            nNextCode++;

            // Increase code size when needed
            if (nNextCode >= nMaxVal && nCodeBits < nMaxBits) {
                nCodeBits++;
                nMaxVal = (1 << nCodeBits);
            }
        }

        nOldCode = nInCode;
    }

    if (bResult) {
        bResult = flushOutput();
    }

    pDecompressState->nCountInput = nTotalInput;
    pDecompressState->nCountOutput = nTotalOutput;

    delete[] pPrefix;
    delete[] pSuffix;
    delete[] pStack;

    return bResult;
}
