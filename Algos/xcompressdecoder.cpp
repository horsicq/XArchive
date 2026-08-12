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
#include <limits>
#include <new>
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
    XBinary::DATAPROCESS_STATE *pState;
    quint8 readBuf[4096];
    qint32 nReadPos;
    qint32 nReadLen;
    quint64 nBitBuf;
    qint32 nBitsInBuf;
    qint64 nBitsRead;     // code-aligned bits consumed (for boundary tracking)
};

// Read one nCodeBits-wide code from the stream, or -1 at end of input.
static qint32 compressReadCode(COMPRESS_BITREADER *br, qint32 nCodeBits)
{
    while (br->nBitsInBuf < nCodeBits) {
        if (br->nReadPos >= br->nReadLen) {
            const qint32 nToRead = Algo_utils::getReadChunkSize(br->pState, (qint32)sizeof(br->readBuf));
            if (nToRead <= 0) return -1;

            br->nReadLen = XBinary::_readDevice((char *)br->readBuf, nToRead, br->pState);
            if (br->nReadLen <= 0) return -1;
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

// compress writes codes in groups of eight. When the code width changes or a
// CLEAR code is emitted, the remainder of the current width-sized byte group
// is padding and the next code begins at the following group boundary.
static bool compressAlignCodeGroup(COMPRESS_BITREADER *br, qint32 nCodeBits, qint64 *pnGroupStartBits)
{
    if (!br || !pnGroupStartBits || (nCodeBits < COMPRESS_MINBITS) || (nCodeBits > COMPRESS_MAXBITS) ||
        (br->nBitsRead < *pnGroupStartBits)) {
        return false;
    }

    const qint64 nGroupBits = (qint64)nCodeBits * 8;
    const qint64 nUsedBits = br->nBitsRead - *pnGroupStartBits;
    qint64 nSkipBits = (nGroupBits - (nUsedBits % nGroupBits)) % nGroupBits;

    while (nSkipBits > 0) {
        const qint32 nChunk = (qint32)qMin(nSkipBits, (qint64)COMPRESS_MAXBITS);
        if (compressReadCode(br, nChunk) < 0) {
            return false;
        }
        nSkipBits -= nChunk;
    }

    *pnGroupStartBits = br->nBitsRead;
    return true;
}

// Flush the output buffer to the device, resetting *pnOutPos. Returns false on short write.
static bool compressFlushOutput(XBinary::DATAPROCESS_STATE *pState, quint8 *outBuf, qint32 *pnOutPos)
{
    if (*pnOutPos > 0) {
        const qint32 nWritten = XBinary::_writeDevice((char *)outBuf, *pnOutPos, pState);
        if (nWritten != *pnOutPos) return false;
        *pnOutPos = 0;
    }
    return true;
}

static bool compressAppendOutput(XBinary::DATAPROCESS_STATE *pState, quint8 *pOutBuf, qint32 nOutBufSize, qint32 *pnOutPos,
                                 qint64 *pnTotalOutput, bool bHasExpectedSize, qint64 nExpectedSize, quint8 nByte)
{
    if (!pState || !pOutBuf || !pnOutPos || !pnTotalOutput || (*pnOutPos < 0) || (*pnOutPos >= nOutBufSize) ||
        (*pnTotalOutput == (std::numeric_limits<qint64>::max)()) || (bHasExpectedSize && (*pnTotalOutput >= nExpectedSize))) {
        return false;
    }

    pOutBuf[(*pnOutPos)++] = nByte;
    (*pnTotalOutput)++;

    return (*pnOutPos < nOutBufSize) || compressFlushOutput(pState, pOutBuf, pnOutPos);
}

bool XCompressDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if ((pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1) ||
        ((pDecompressState->nInputLimit != -1) && (pDecompressState->nInputLimit < 3)) ||
        (pDecompressState->nProcessedOffset < 0) || (pDecompressState->nProcessedLimit < -1) ||
        ((pDecompressState->nProcessedLimit != -1) &&
         (pDecompressState->nProcessedOffset >
          ((std::numeric_limits<qint64>::max)() - pDecompressState->nProcessedLimit)))) {
        return false;
    }

    const bool bHasExpectedSize = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    const qint64 nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    if (bHasExpectedSize && (nExpectedSize < 0)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Read 3-byte header: magic(2) + flags(1)
    quint8 header[3];
    if (XBinary::_readDevice((char *)header, 3, pDecompressState) != 3) {
        return false;
    }

    if (header[0] != COMPRESS_MAGIC_0 || header[1] != COMPRESS_MAGIC_1) {
        return false;
    }

    // Bits 5 and 6 are reserved by the format and must be zero.
    if (header[2] & 0x60) {
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
    if ((nMaxCode < 256) || (nMaxCode > COMPRESS_TABLESIZE)) {
        return false;
    }

    quint16 *pPrefix = new (std::nothrow) quint16[nMaxCode];
    if (!pPrefix) {
        return false;
    }

    quint8 *pSuffix = new (std::nothrow) quint8[nMaxCode];
    if (!pSuffix) {
        delete[] pPrefix;
        return false;
    }

    quint8 *pStack = new (std::nothrow) quint8[nMaxCode];
    if (!pStack) {
        delete[] pPrefix;
        delete[] pSuffix;
        return false;
    }

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
    br.pState = pDecompressState;
    br.nReadPos = 0;
    br.nReadLen = 0;
    br.nBitBuf = 0;
    br.nBitsInBuf = 0;
    br.nBitsRead = 0;
    qint64 nGroupStartBits = 0;

    // Output buffer
    const qint32 OUTBUF_SIZE = 4096;
    quint8 outBuf[4096];
    qint32 nOutPos = 0;

    qint64 nTotalOutput = 0;
    bool bResult = true;

    // Read first code (must be a literal 0..255)
    qint32 nOldCode = compressReadCode(&br, nCodeBits);
    if (nOldCode < 0) {
        // A valid empty .Z stream has only the three-byte header (and may
        // contain fewer than nine zero padding bits). Nonzero residual bits
        // are a truncated first code, not an empty payload.
        const bool bZeroPadding = (br.nBitsInBuf >= 0) && (br.nBitsInBuf < nCodeBits) && (br.nBitBuf == 0);
        const bool bEmptyResult = bZeroPadding && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
                                  ((pDecompressState->nInputLimit == -1) ||
                                   (pDecompressState->nCountInput == pDecompressState->nInputLimit)) &&
                                  (!bHasExpectedSize || (nExpectedSize == 0)) && (pDecompressState->nCountOutput == 0) &&
                                  XBinary::isPdStructNotCanceled(pPdStruct);
        delete[] pPrefix;
        delete[] pSuffix;
        delete[] pStack;
        return bEmptyResult;
    }
    if (nOldCode >= 256) {
        delete[] pPrefix;
        delete[] pSuffix;
        delete[] pStack;
        return false;
    }

    quint8 nFinChar = (quint8)nOldCode;
    if (!compressAppendOutput(pDecompressState, outBuf, OUTBUF_SIZE, &nOutPos, &nTotalOutput, bHasExpectedSize, nExpectedSize, nFinChar)) {
        bResult = false;
    }

    // Main decompression loop
    while (bResult) {
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
            do {
                if (!compressAlignCodeGroup(&br, nCodeBits, &nGroupStartBits) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                    bResult = false;
                    break;
                }

                // Reset table. Some canonical encoders can emit more than one
                // CLEAR before the next literal, with every CLEAR starting a
                // newly aligned 9-bit group.
                nNextCode = COMPRESS_FIRST;
                nCodeBits = COMPRESS_MINBITS;
                nMaxVal = (1 << nCodeBits);
                nCode = compressReadCode(&br, nCodeBits);
            } while (nCode == COMPRESS_CLEAR);

            // The first code after a CLEAR starts a fresh dictionary and must
            // therefore be a literal. Accepting a forward dictionary code here
            // leaves prefix/suffix entries uninitialized for malformed input.
            if (!bResult || (nCode < 0) || (nCode >= 256)) {
                bResult = false;
                break;
            }

            nOldCode = nCode;
            nFinChar = (quint8)nCode;
            if (!compressAppendOutput(pDecompressState, outBuf, OUTBUF_SIZE, &nOutPos, &nTotalOutput, bHasExpectedSize, nExpectedSize,
                                      nFinChar)) {
                bResult = false;
                break;
            }
            continue;
        }

        qint32 nInCode = nCode;
        qint32 nStackTop = 0;

        // If code is not yet in table, handle the special KwKwK case
        if (nCode >= nNextCode) {
            if ((nCode > nNextCode) || (nCode >= nMaxCode)) {
                // Invalid code
                bResult = false;
                break;
            }
            pStack[nStackTop++] = nFinChar;
            nCode = nOldCode;
        }

        // Chase prefix chain to build output string (in reverse)
        while (nCode >= 256) {
            if ((nCode >= nNextCode) || (nCode >= nMaxCode) || (nStackTop >= nMaxCode)) {
                bResult = false;
                break;
            }
            pStack[nStackTop++] = pSuffix[nCode];
            nCode = pPrefix[nCode];
        }

        if (!bResult) break;

        if ((nCode < 0) || (nCode >= 256) || (nStackTop >= nMaxCode)) {
            bResult = false;
            break;
        }

        nFinChar = pSuffix[nCode];
        pStack[nStackTop++] = nFinChar;

        // Output in correct order (reverse of stack)
        for (qint32 i = nStackTop - 1; i >= 0; i--) {
            if (!compressAppendOutput(pDecompressState, outBuf, OUTBUF_SIZE, &nOutPos, &nTotalOutput, bHasExpectedSize, nExpectedSize,
                                      pStack[i])) {
                bResult = false;
                break;
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
                if (!compressAlignCodeGroup(&br, nCodeBits, &nGroupStartBits)) {
                    bResult = false;
                    break;
                }
                nCodeBits++;
                nMaxVal = (1 << nCodeBits);
            }
        }

        nOldCode = nInCode;
    }

    if (bResult) {
        bResult = compressFlushOutput(pDecompressState, outBuf, &nOutPos);
    }

    bResult = bResult && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
              ((pDecompressState->nInputLimit == -1) || (pDecompressState->nCountInput == pDecompressState->nInputLimit)) &&
              (!bHasExpectedSize || (nTotalOutput == nExpectedSize)) && (pDecompressState->nCountOutput == nTotalOutput) &&
              XBinary::isPdStructNotCanceled(pPdStruct);

    delete[] pPrefix;
    delete[] pSuffix;
    delete[] pStack;

    return bResult;
}
