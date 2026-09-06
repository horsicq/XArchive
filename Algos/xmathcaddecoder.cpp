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
#include "xmathcaddecoder.h"

#include <cstring>
#include <new>

#include "algo_utils.h"

namespace {
const qint32 N_MATHCAD_WINDOW_SIZE = 4096;    // 12-bit ring position field
const qint32 N_MATHCAD_WINDOW_START = 1;      // position 0 is reserved for EOF
const qint32 N_MATHCAD_LENGTH_BIAS = 2;       // 4-bit field encodes length - 2
const qint32 N_MATHCAD_INPUT_BUFFER = 16384;
const qint32 N_MATHCAD_OUTPUT_BUFFER = 65536;

struct MATHCAD_READER {
    XBinary::DATAPROCESS_STATE *pState;
    char *pBuffer;
    qint32 nBufferSize;
    qint32 nBufferPos;
    quint32 nBitBuffer;
    qint32 nBitCount;
};

bool mathcadReadByte(MATHCAD_READER *pReader, quint8 *pByte)
{
    if (pReader->nBufferPos >= pReader->nBufferSize) {
        const qint32 nRead = XBinary::_readDevice(pReader->pBuffer, N_MATHCAD_INPUT_BUFFER, pReader->pState);
        if (nRead <= 0) return false;
        pReader->nBufferSize = nRead;
        pReader->nBufferPos = 0;
    }

    *pByte = static_cast<quint8>(pReader->pBuffer[pReader->nBufferPos++]);

    return true;
}

// MSB-first inside each byte.  A single byte is held at a time so a token that
// straddles a byte boundary cannot over-read the bounded input.
bool mathcadGetBits(MATHCAD_READER *pReader, qint32 nBits, quint32 *pValue)
{
    quint32 nResult = 0;

    while (nBits > 0) {
        if (pReader->nBitCount == 0) {
            quint8 nByte = 0;
            if (!mathcadReadByte(pReader, &nByte)) return false;
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
}  // namespace

XMathCadDecoder::XMathCadDecoder(QObject *parent) : QObject(parent)
{
}

bool XMathCadDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError) return false;

    const qint64 nOutputLimit = pDecompressState->nProcessedLimit;

    char *pWindow = new (std::nothrow) char[N_MATHCAD_WINDOW_SIZE];
    char *pInputBuffer = new (std::nothrow) char[N_MATHCAD_INPUT_BUFFER];
    char *pOutputBuffer = new (std::nothrow) char[N_MATHCAD_OUTPUT_BUFFER];

    if (!pWindow || !pInputBuffer || !pOutputBuffer) {
        delete[] pWindow;
        delete[] pInputBuffer;
        delete[] pOutputBuffer;
        return false;
    }

    // The window content before the first wrap is never referenced by a valid
    // stream (positions are always behind the write pointer), so the fill value
    // is not observable; spaces match the classic LZSS convention.
    memset(pWindow, ' ', N_MATHCAD_WINDOW_SIZE);

    MATHCAD_READER reader = {};
    reader.pState = pDecompressState;
    reader.pBuffer = pInputBuffer;

    qint32 nWindowPos = N_MATHCAD_WINDOW_START;
    qint32 nOutputBufferPos = 0;
    qint64 nOutputCount = 0;
    bool bWindowWrapped = false;
    bool bResult = false;
    bool bDone = false;

    while (!bDone && XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint32 nFlag = 0;
        if (!mathcadGetBits(&reader, 1, &nFlag)) {
            // The stream must be terminated by the explicit end token; running
            // out of bits first means the member is truncated.
            break;
        }

        qint32 nCopyLength = 0;
        qint32 nCopyPos = 0;
        quint32 nLiteral = 0;

        if (nFlag) {
            if (!mathcadGetBits(&reader, 8, &nLiteral)) break;
            nCopyLength = 1;
        } else {
            quint32 nPosition = 0;
            if (!mathcadGetBits(&reader, 12, &nPosition)) break;

            if (nPosition == 0) {
                bResult = true;
                bDone = true;
                break;
            }

            // Until the ring wraps, every legal back-reference points strictly
            // behind the write pointer.  Enforcing that turns a corrupt or
            // misrouted stream into a failure instead of a run of window fill
            // bytes that would be published as a successful result.
            if (!bWindowWrapped && (static_cast<qint32>(nPosition) >= nWindowPos)) {
                break;
            }

            quint32 nLengthCode = 0;
            if (!mathcadGetBits(&reader, 4, &nLengthCode)) break;

            nCopyPos = static_cast<qint32>(nPosition);
            nCopyLength = static_cast<qint32>(nLengthCode) + N_MATHCAD_LENGTH_BIAS;
        }

        if ((nOutputLimit != -1) && ((nOutputCount + nCopyLength) > nOutputLimit)) {
            break;
        }

        bool bFlushFailed = false;

        for (qint32 i = 0; i < nCopyLength; i++) {
            const char cByte = nFlag ? static_cast<char>(static_cast<quint8>(nLiteral)) : pWindow[(nCopyPos + i) & (N_MATHCAD_WINDOW_SIZE - 1)];

            pOutputBuffer[nOutputBufferPos++] = cByte;
            pWindow[nWindowPos] = cByte;
            nWindowPos = (nWindowPos + 1) & (N_MATHCAD_WINDOW_SIZE - 1);
            if (nWindowPos == 0) bWindowWrapped = true;
            nOutputCount++;

            if (nOutputBufferPos >= N_MATHCAD_OUTPUT_BUFFER) {
                if (XBinary::_writeDevice(pOutputBuffer, nOutputBufferPos, pDecompressState) != nOutputBufferPos) {
                    pDecompressState->bWriteError = true;
                    bFlushFailed = true;
                    break;
                }
                nOutputBufferPos = 0;
            }
        }

        if (bFlushFailed) break;
    }

    if (nOutputBufferPos > 0) {
        if (XBinary::_writeDevice(pOutputBuffer, nOutputBufferPos, pDecompressState) != nOutputBufferPos) {
            pDecompressState->bWriteError = true;
            bResult = false;
        }
    }

    delete[] pWindow;
    delete[] pInputBuffer;
    delete[] pOutputBuffer;

    return bResult && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
