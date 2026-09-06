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
#include "xirwinpacdecoder.h"

#include <new>

#include "algo_utils.h"

namespace {
const qint32 N_IRWINPAC_CHUNK_HEADER_SIZE = 10;
// Both the chunk size and the unpacked size are u16 fields, so a block can
// never be larger than this; the whole block is kept in memory because matches
// address it directly instead of a ring window.
const qint32 N_IRWINPAC_MAX_BLOCK = 65536;
const qint32 N_IRWINPAC_INPUT_BUFFER = 16384;
const qint32 N_IRWINPAC_SHORT_DIST_BITS = 7;
const qint32 N_IRWINPAC_LONG_DIST_BITS = 11;

struct IRWINPAC_READER {
    XBinary::DATAPROCESS_STATE *pState;
    char *pBuffer;
    qint32 nBufferSize;
    qint32 nBufferPos;
    quint32 nBitBuffer;
    qint32 nBitCount;
    // Bytes of the current chunk payload that the bit reader may still fetch.
    qint64 nPayloadLeft;
    bool bEndOfInput;
};

bool irwinpacReadByte(IRWINPAC_READER *pReader, quint8 *pByte)
{
    if (pReader->nBufferPos >= pReader->nBufferSize) {
        const qint32 nRead = XBinary::_readDevice(pReader->pBuffer, N_IRWINPAC_INPUT_BUFFER, pReader->pState);
        if (nRead <= 0) {
            pReader->bEndOfInput = true;
            return false;
        }
        pReader->nBufferSize = nRead;
        pReader->nBufferPos = 0;
    }

    *pByte = static_cast<quint8>(pReader->pBuffer[pReader->nBufferPos++]);

    return true;
}

// MSB-first inside each byte, and never beyond the current chunk payload: a
// truncated block must fail instead of pulling the next block's header in as
// if it were compressed data.
bool irwinpacGetBits(IRWINPAC_READER *pReader, qint32 nBits, quint32 *pValue)
{
    quint32 nResult = 0;

    while (nBits > 0) {
        if (pReader->nBitCount == 0) {
            if (pReader->nPayloadLeft <= 0) return false;
            quint8 nByte = 0;
            if (!irwinpacReadByte(pReader, &nByte)) return false;
            pReader->nPayloadLeft--;
            pReader->nBitBuffer = nByte;
            pReader->nBitCount = 8;
        }

        const qint32 nTake = (nBits < pReader->nBitCount) ? nBits : pReader->nBitCount;
        const quint32 nPart = (pReader->nBitBuffer >> (pReader->nBitCount - nTake)) & ((1u << nTake) - 1u);
        nResult = (nResult << nTake) | nPart;
        pReader->nBitCount -= nTake;
        nBits -= nTake;
    }

    *pValue = nResult;

    return true;
}

// 2 bits (2..4), then 2 bits (5..7), then a chain of nibbles where 15 escapes
// into the next 15-wide bucket.  The bucket base is capped at the largest block
// a u16 unpacked size can describe so a corrupt run of 0xF nibbles terminates.
bool irwinpacGetLength(IRWINPAC_READER *pReader, qint32 *pLength)
{
    quint32 nValue = 0;

    if (!irwinpacGetBits(pReader, 2, &nValue)) return false;
    if (nValue < 3) {
        *pLength = 2 + static_cast<qint32>(nValue);
        return true;
    }

    if (!irwinpacGetBits(pReader, 2, &nValue)) return false;
    if (nValue < 3) {
        *pLength = 5 + static_cast<qint32>(nValue);
        return true;
    }

    qint32 nBase = 8;

    while (true) {
        if (!irwinpacGetBits(pReader, 4, &nValue)) return false;
        if (nValue < 15) {
            *pLength = nBase + static_cast<qint32>(nValue);
            return true;
        }
        nBase += 15;
        if (nBase > N_IRWINPAC_MAX_BLOCK) return false;
    }
}

// Decodes one compressed block into pBlock.  The block is complete only when it
// produced exactly the declared number of bytes: a match that would overrun the
// declared size is a decode failure, not something to clamp.
bool irwinpacDecodeBlock(IRWINPAC_READER *pReader, char *pBlock, qint32 nBlockSize, XBinary::PDSTRUCT *pPdStruct)
{
    qint32 nPos = 0;

    while (nPos < nBlockSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        quint32 nFlag = 0;
        if (!irwinpacGetBits(pReader, 1, &nFlag)) return false;

        if (nFlag == 0) {
            quint32 nLiteral = 0;
            if (!irwinpacGetBits(pReader, 8, &nLiteral)) return false;
            pBlock[nPos++] = static_cast<char>(static_cast<quint8>(nLiteral));
            continue;
        }

        quint32 nShort = 0;
        if (!irwinpacGetBits(pReader, 1, &nShort)) return false;

        quint32 nDistance = 0;
        if (!irwinpacGetBits(pReader, (nShort != 0) ? N_IRWINPAC_SHORT_DIST_BITS : N_IRWINPAC_LONG_DIST_BITS, &nDistance)) {
            return false;
        }

        qint32 nLength = 0;
        if (!irwinpacGetLength(pReader, &nLength)) return false;

        if ((nDistance == 0) || (static_cast<qint32>(nDistance) > nPos)) return false;
        if (nLength <= 0) return false;
        if (nLength > (nBlockSize - nPos)) return false;

        qint32 nSource = nPos - static_cast<qint32>(nDistance);

        for (qint32 i = 0; i < nLength; i++) {
            pBlock[nPos++] = pBlock[nSource++];
        }
    }

    return true;
}
}  // namespace

XIrwinPacDecoder::XIrwinPacDecoder(QObject *parent) : QObject(parent)
{
}

bool XIrwinPacDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError) return false;

    // nProcessedLimit counts bytes AFTER nProcessedOffset, so the point where
    // nothing more can be wanted is the sum of the two.
    qint64 nOutputEnd = -1;
    if (pDecompressState->nProcessedLimit >= 0) {
        nOutputEnd = pDecompressState->nProcessedOffset + pDecompressState->nProcessedLimit;
    }

    char *pInputBuffer = new (std::nothrow) char[N_IRWINPAC_INPUT_BUFFER];
    char *pBlock = new (std::nothrow) char[N_IRWINPAC_MAX_BLOCK];

    if (!pInputBuffer || !pBlock) {
        delete[] pInputBuffer;
        delete[] pBlock;
        return false;
    }

    IRWINPAC_READER reader = {};
    reader.pState = pDecompressState;
    reader.pBuffer = pInputBuffer;

    bool bResult = true;
    bool bDone = false;
    qint64 nOutputCount = 0;

    while (!bDone && XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint8 header[N_IRWINPAC_CHUNK_HEADER_SIZE] = {};
        qint32 nHeaderRead = 0;

        reader.nPayloadLeft = 0;
        reader.nBitCount = 0;

        // The header is read as raw bytes, outside the bit reader's payload
        // budget; the loop below is what bounds it.
        while ((nHeaderRead < N_IRWINPAC_CHUNK_HEADER_SIZE) && irwinpacReadByte(&reader, &header[nHeaderRead])) {
            nHeaderRead++;
        }

        if (nHeaderRead == 0) {
            // A clean end of the bounded input on a chunk boundary is how the
            // member finishes; there is no explicit terminator record.
            bDone = true;
            break;
        }

        if (nHeaderRead != N_IRWINPAC_CHUNK_HEADER_SIZE) {
            bResult = false;
            break;
        }

        const quint32 nFlag = static_cast<quint32>(header[0]) | (static_cast<quint32>(header[1]) << 8);
        const quint32 nChunkSize = static_cast<quint32>(header[2]) | (static_cast<quint32>(header[3]) << 8);
        const quint32 nUnpackedSize = static_cast<quint32>(header[4]) | (static_cast<quint32>(header[5]) << 8);
        // header[6..9] is encoder scratch memory; it differs between files that
        // otherwise decode identically, so it carries no information.

        if ((nFlag > 1) || (nChunkSize < static_cast<quint32>(N_IRWINPAC_CHUNK_HEADER_SIZE))) {
            bResult = false;
            break;
        }

        const qint64 nPayloadSize = static_cast<qint64>(nChunkSize) - N_IRWINPAC_CHUNK_HEADER_SIZE;
        const qint32 nBlockSize = static_cast<qint32>(nUnpackedSize);

        if (nFlag == 0) {
            if (nPayloadSize != static_cast<qint64>(nBlockSize)) {
                bResult = false;
                break;
            }
        } else if (nBlockSize <= 0) {
            bResult = false;
            break;
        }

        reader.nPayloadLeft = nPayloadSize;
        reader.nBitCount = 0;

        if (nBlockSize > 0) {
            if (nFlag == 0) {
                qint32 nStored = 0;
                while (nStored < nBlockSize) {
                    quint8 nByte = 0;
                    if ((reader.nPayloadLeft <= 0) || !irwinpacReadByte(&reader, &nByte)) break;
                    reader.nPayloadLeft--;
                    pBlock[nStored++] = static_cast<char>(nByte);
                }
                if (nStored != nBlockSize) {
                    bResult = false;
                    break;
                }
            } else if (!irwinpacDecodeBlock(&reader, pBlock, nBlockSize, pPdStruct)) {
                bResult = false;
                break;
            }

            if (XBinary::_writeDevice(pBlock, nBlockSize, pDecompressState) != nBlockSize) {
                pDecompressState->bWriteError = true;
                bResult = false;
                break;
            }

            nOutputCount += nBlockSize;
        }

        // Every block ends with a few spare bytes the encoder never uses; step
        // over whatever is left so the next chunk header is read byte-aligned.
        while (reader.nPayloadLeft > 0) {
            quint8 nSkip = 0;
            if (!irwinpacReadByte(&reader, &nSkip)) {
                bResult = false;
                break;
            }
            reader.nPayloadLeft--;
        }

        if (!bResult) break;

        if ((nOutputEnd >= 0) && (nOutputCount >= nOutputEnd)) {
            bDone = true;
        }
    }

    delete[] pInputBuffer;
    delete[] pBlock;

    return bResult && bDone && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
