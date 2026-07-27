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
#include "algo_utils.h"
#include <string.h>

// Unix compress (.Z) LZW decompression
// Format: magic(2: 0x1F 0x9D) + flags(1) + LZW data
// Flags byte: bits 0-4 = max code bits (9..16), bit 7 = block_compress mode
// Codes are packed LSB-first (least significant bit first)
// Initial code size is 9 bits, grows up to maxbits
// Code 256 = CLEAR (reset table) when block_compress is set

#define COMPRESS_MAGIC_0 0x1F
#define COMPRESS_MAGIC_1 0x9D
#define COMPRESS_CLEAR 256
#define COMPRESS_FIRST 257
#define COMPRESS_MINBITS 9
#define COMPRESS_MAXBITS 16
#define COMPRESS_TABLESIZE (1 << COMPRESS_MAXBITS)

XCompressDecoder::XCompressDecoder(QObject *parent) : QObject(parent)
{
}

// Streaming LSB-first bit reader for the Unix compress (LZW) code stream.
struct COMPRESS_BITREADER {
    QIODevice *pInput;
    qint64 nInputLimit;   // 0 = unbounded
    quint8 readBuf[4096];
    qint32 nReadPos;
    qint32 nReadLen;
    quint64 nBitBuf;
    qint32 nBitsInBuf;
    qint64 nBitsRead;     // code-aligned bits consumed (for boundary tracking)
    qint64 nTotalInput;
};

// Read one nCodeBits-wide code from the stream, or -1 at end of input.
static qint32 compressReadCode(COMPRESS_BITREADER *br, qint32 nCodeBits)
{
    while (br->nBitsInBuf < nCodeBits) {
        if (br->nReadPos >= br->nReadLen) {
            qint64 nToRead = (qint64)sizeof(br->readBuf);
            if (br->nInputLimit > 0) {
                qint64 nRemaining = br->nInputLimit - br->nTotalInput;
                if (nRemaining <= 0) return -1;
                if (nToRead > nRemaining) nToRead = nRemaining;
            }
            br->nReadLen = (qint32)br->pInput->read((char *)br->readBuf, nToRead);
            if (br->nReadLen <= 0) return -1;
            br->nTotalInput += br->nReadLen;
            br->nReadPos = 0;
        }
        br->nBitBuf |= ((quint64)br->readBuf[br->nReadPos++]) << br->nBitsInBuf;
        br->nBitsInBuf += 8;
    }

    qint32 nCode = (qint32)(br->nBitBuf & ((1 << nCodeBits) - 1));
    br->nBitBuf >>= nCodeBits;
    br->nBitsInBuf -= nCodeBits;
    br->nBitsRead += nCodeBits;

    return nCode;
}

// Flush the output buffer to the device, resetting *pnOutPos. Returns false on short write.
static bool compressFlushOutput(QIODevice *pOutput, quint8 *outBuf, qint32 *pnOutPos)
{
    if (*pnOutPos > 0) {
        qint64 nWritten = pOutput->write((char *)outBuf, *pnOutPos);
        if (nWritten != *pnOutPos) return false;
        *pnOutPos = 0;
    }
    return true;
}

bool XCompressDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pInput = pDecompressState->pDeviceInput;
    QIODevice *pOutput = pDecompressState->pDeviceOutput;

    Algo_utils::seekToStart(pDecompressState);

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

    // Streaming bit-read state for the variable-width LZW codes (LSB-first).
    COMPRESS_BITREADER br;
    br.pInput = pInput;
    br.nInputLimit = pDecompressState->nInputLimit;
    br.nReadPos = 0;
    br.nReadLen = 0;
    br.nBitBuf = 0;
    br.nBitsInBuf = 0;
    br.nBitsRead = 0;
    br.nTotalInput = 3;  // header already read

    // Output buffer
    const qint32 OUTBUF_SIZE = 4096;
    quint8 outBuf[4096];
    qint32 nOutPos = 0;

    qint64 nTotalOutput = 0;
    bool bResult = true;

    // Read first code (must be a literal 0..255)
    qint32 nOldCode = compressReadCode(&br, nCodeBits);
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

        qint32 nCode = compressReadCode(&br, nCodeBits);
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
            br.nBitBuf = 0;
            br.nBitsInBuf = 0;

            nCode = compressReadCode(&br, nCodeBits);
            if (nCode < 0) {
                break;
            }

            nOldCode = nCode;
            nFinChar = (quint8)nCode;
            outBuf[nOutPos++] = nFinChar;
            nTotalOutput++;
            if (nOutPos >= OUTBUF_SIZE) {
                if (!compressFlushOutput(pOutput, outBuf, &nOutPos)) {
                    bResult = false;
                    break;
                }
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
                if (!compressFlushOutput(pOutput, outBuf, &nOutPos)) {
                    bResult = false;
                    break;
                }
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
        bResult = compressFlushOutput(pOutput, outBuf, &nOutPos);
    }

    pDecompressState->nCountInput = br.nTotalInput;
    pDecompressState->nCountOutput = nTotalOutput;

    delete[] pPrefix;
    delete[] pSuffix;
    delete[] pStack;

    return bResult;
}
