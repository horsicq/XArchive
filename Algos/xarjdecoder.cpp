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
#include "xarjdecoder.h"
#include "algo_utils.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
bool arjPrepareState(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, quint32 *pCompressedSize, quint32 *pOriginalSize)
{
    if (!pState || !pCompressedSize || !pOriginalSize) return false;

    Algo_utils::prepareState(pState);

    if (!pState->pDeviceInput || !pState->pDeviceInput->isReadable()) {
        pState->bReadError = true;
    }
    if (!pState->pDeviceOutput || !pState->pDeviceOutput->isWritable()) {
        pState->bWriteError = true;
    }
    if (pState->bReadError || pState->bWriteError) return false;

    const qint64 nMax32 = (std::numeric_limits<quint32>::max)();
    if ((pState->nInputLimit < 0) || (pState->nInputLimit > nMax32)) {
        pState->bReadError = true;
        return false;
    }

    bool bOriginalSizeValid = false;
    const qint64 nOriginalSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bOriginalSizeValid);
    if (!pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE) || !bOriginalSizeValid || (nOriginalSize < 0) ||
        (nOriginalSize > nMax32)) {
        return false;
    }

    const qint64 nMax64 = (std::numeric_limits<qint64>::max)();
    if ((pState->nProcessedOffset < 0) || (pState->nProcessedLimit < -1) ||
        ((pState->nProcessedLimit != -1) && (pState->nProcessedOffset > nMax64 - pState->nProcessedLimit))) {
        pState->bWriteError = true;
        return false;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pCompressedSize = static_cast<quint32>(pState->nInputLimit);
    *pOriginalSize = static_cast<quint32>(nOriginalSize);
    return true;
}

bool arjWriteAll(XBinary::DATAPROCESS_STATE *pState, const char *pData, qint32 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || (nSize < 0) || ((nSize > 0) && !pData) || (pState->nCountOutput < 0)) {
        if (pState) pState->bWriteError = true;
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const qint64 nChunkStart = pState->nCountOutput;
    const qint64 nWindowOffset = pState->nProcessedOffset;
    const qint64 nWindowSize = pState->nProcessedLimit;
    const qint64 nMax64 = (std::numeric_limits<qint64>::max)();
    if ((nWindowOffset < 0) || (nWindowSize < -1) || (nChunkStart > nMax64 - nSize) ||
        ((nWindowSize != -1) && (nWindowOffset > nMax64 - nWindowSize))) {
        pState->bWriteError = true;
        return false;
    }

    // This function reimplements XBinary::_writeDevice's window arithmetic and
    // is the ARJ decoder's only output path, so it must also reimplement that
    // function's output-limit gate - otherwise an ARJ member ignores a
    // correctly supplied UNPACK_PROP_MAX_OUTPUT_SIZE. The charge is against the
    // bytes produced, not the bytes that survive window clipping, and it has to
    // happen before the fully-clipped early return below so a skipped chunk
    // cannot escape it.
    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pState->mapUnpackProperties, &nOutputLimit) ||
        ((nOutputLimit >= 0) && ((nChunkStart > nOutputLimit) || ((qint64)nSize > (nOutputLimit - nChunkStart))))) {
        pState->bWriteError = true;
        return false;
    }

    const qint64 nChunkEnd = nChunkStart + nSize;
    const qint64 nWindowEnd = (nWindowSize == -1) ? nMax64 : nWindowOffset + nWindowSize;
    const qint64 nWriteStart = (std::max)(nChunkStart, nWindowOffset);
    const qint64 nWriteEnd = (std::min)(nChunkEnd, nWindowEnd);
    if (nWriteEnd <= nWriteStart) {
        pState->nCountOutput = nChunkEnd;
        return XBinary::isPdStructNotCanceled(pPdStruct);
    }

    if (!pState->pDeviceOutput || !pState->pDeviceOutput->isWritable()) {
        pState->bWriteError = true;
        return false;
    }

    const qint64 nDataOffset = nWriteStart - nChunkStart;
    const qint64 nWriteSize = nWriteEnd - nWriteStart;
    qint64 nWrittenTotal = 0;
    pState->nCountOutput = nWriteStart;

    while (nWrittenTotal < nWriteSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nWritten = pState->pDeviceOutput->write(pData + nDataOffset + nWrittenTotal, nWriteSize - nWrittenTotal);
        if ((nWritten <= 0) || (nWritten > (nWriteSize - nWrittenTotal))) {
            pState->bWriteError = true;
            return false;
        }

        nWrittenTotal += nWritten;
        pState->nCountOutput = nWriteStart + nWrittenTotal;
    }

    pState->nCountOutput = nChunkEnd;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
}  // namespace

XArjDecoder::XArjDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XArjDecoder::refillInputBuffer(ArjDecodeState *pState)
{
    if (!pState || pState->bError || !pState->pProcessState || !pState->pReadBuffer || (pState->nReadBufferSize <= 0) ||
        (pState->nCompLeft == 0)) {
        if (pState) {
            pState->bError = true;
            if (pState->pProcessState) pState->pProcessState->bReadError = true;
        }
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        pState->bError = true;
        return false;
    }

    const qint32 nToRead = static_cast<qint32>((std::min)(static_cast<quint32>(pState->nReadBufferSize), pState->nCompLeft));
    const qint32 nRead = XBinary::_readDevice(reinterpret_cast<char *>(pState->pReadBuffer), nToRead, pState->pProcessState);
    if ((nRead <= 0) || (nRead > nToRead)) {
        pState->nBufAvail = 0;
        pState->bError = true;
        pState->pProcessState->bReadError = true;
        return false;
    }

    pState->pBuf = pState->pReadBuffer;
    pState->nBufAvail = nRead;
    return true;
}

bool XArjDecoder::fillBuf(ArjDecodeState *pState, qint32 nBits)
{
    if (!pState || pState->bError || (nBits < 0) || (nBits > 16)) {
        if (pState) pState->bError = true;
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
        pState->bError = true;
        return false;
    }

    pState->nBitBuf = static_cast<quint16>((static_cast<quint32>(pState->nBitBuf) << nBits) & 0xFFFFU);

    while (nBits > pState->nBitCount) {
        pState->nBitBuf |= static_cast<quint16>(static_cast<quint32>(pState->nSubBitBuf) << (nBits - pState->nBitCount));
        nBits -= pState->nBitCount;

        if (!XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
            pState->bError = true;
            return false;
        }
        // The bit reader runs a full CODE_BIT look-ahead, so the final tokens
        // of a well-formed member are decoded while the reader asks for one or
        // two bytes past the end of the packed stream.  arj's decode.c feeds
        // zero bytes there instead of failing; treating the exhausted stream
        // as a read error truncates every method 1-4 member right before its
        // last partial dictionary flush.  Real input is still bounded by
        // nCompLeft, so nCountInput can never exceed the packed size and the
        // exact-input check at the end of decompressInternal() still rejects a
        // member that did not consume its whole stream.
        if (pState->nCompLeft == 0) {
            pState->nSubBitBuf = 0;
        } else {
            if ((pState->nBufAvail <= 0) && !refillInputBuffer(pState)) return false;
            if (!pState->pBuf || (pState->nBufAvail <= 0)) {
                pState->bError = true;
                if (pState->pProcessState) pState->pProcessState->bReadError = true;
                return false;
            }

            pState->nSubBitBuf = *pState->pBuf++;
            pState->nBufAvail--;
            pState->nCompLeft--;
        }
        pState->nBitCount = 8;
    }

    pState->nBitBuf |= pState->nSubBitBuf >> (pState->nBitCount - nBits);
    pState->nBitCount -= nBits;

    return true;
}

quint16 XArjDecoder::getBits(ArjDecodeState *pState, qint32 nBits)
{
    if (!pState || pState->bError || (nBits < 0) || (nBits > 16)) {
        if (pState) pState->bError = true;
        return 0;
    }

    const quint16 nResult = (nBits == 0) ? 0 : static_cast<quint16>(pState->nBitBuf >> (16 - nBits));
    if (!fillBuf(pState, nBits)) return 0;
    return nResult;
}

bool XArjDecoder::initGetBits(ArjDecodeState *pState)
{
    pState->nBitBuf = 0;
    pState->nSubBitBuf = 0;
    pState->nBitCount = 0;
    return fillBuf(pState, 16);
}

bool XArjDecoder::makeTable(ArjDecodeState *pState, qint32 nChar, quint8 *pBitLen, qint32 nTableBits, quint16 *pTable, qint32 nTableSize)
{
    quint16 arrCount[17];
    quint16 arrWeight[17];
    quint16 arrStart[18];

    for (qint32 i = 1; i <= 16; i++) {
        arrCount[i] = 0;
    }

    for (qint32 i = 0; i < nChar; i++) {
        if (pBitLen[i] >= 17) {
            pState->bError = true;
            return false;
        }

        arrCount[pBitLen[i]]++;
    }

    arrStart[1] = 0;

    for (qint32 i = 1; i <= 16; i++) {
        arrStart[i + 1] = arrStart[i] + (arrCount[i] << (16 - i));
    }

    if (arrStart[17] != 0) {
        pState->bError = true;
        return false;
    }

    qint32 nJutBits = 16 - nTableBits;

    if (nTableBits >= 17) {
        pState->bError = true;
        return false;
    }

    for (qint32 i = 1; i <= nTableBits; i++) {
        arrStart[i] >>= nJutBits;
        arrWeight[i] = 1 << (nTableBits - i);
    }

    for (qint32 i = nTableBits + 1; i <= 16; i++) {
        arrWeight[i] = 1 << (16 - i);
    }

    quint32 nPos = arrStart[nTableBits + 1] >> nJutBits;

    // arrStart stores 16-bit canonical-code positions, so the complete-tree
    // sentinel (1 << 16) is represented by zero after truncation.
    if (nPos != 0U) {
        quint32 nEnd = 1U << nTableBits;

        while (nPos < nEnd) {
            if ((qint32)nPos >= nTableSize) {
                pState->bError = true;
                return false;
            }

            pTable[nPos++] = 0;
        }
    }

    quint32 nAvail = nChar;
    quint16 nMask = 1 << (15 - nTableBits);

    for (qint32 nCh = 0; nCh < nChar; nCh++) {
        qint32 nLen = pBitLen[nCh];

        if (nLen == 0) {
            continue;
        }

        if (nLen >= 17) {
            pState->bError = true;
            return false;
        }

        quint32 nK = arrStart[nLen];
        quint32 nNextCode = nK + arrWeight[nLen];

        if (nLen <= nTableBits) {
            if (nNextCode > (quint32)nTableSize) {
                pState->bError = true;
                return false;
            }

            for (quint32 i = nK; i < nNextCode; i++) {
                pTable[i] = (quint16)nCh;
            }
        } else {
            quint16 *pSlot = &pTable[nK >> nJutBits];
            qint32 nRemaining = nLen - nTableBits;

            while (nRemaining > 0) {
                if (*pSlot == 0) {
                    if (nAvail >= (quint32)(2 * NC - 1)) {
                        pState->bError = true;
                        return false;
                    }

                    pState->arrLeft[nAvail] = 0;
                    pState->arrRight[nAvail] = 0;
                    *pSlot = (quint16)nAvail++;
                }

                if (*pSlot >= (2 * NC - 1)) {
                    pState->bError = true;
                    return false;
                }

                if (nK & nMask) {
                    pSlot = &pState->arrRight[*pSlot];
                } else {
                    pSlot = &pState->arrLeft[*pSlot];
                }

                nK <<= 1;
                nRemaining--;
            }

            *pSlot = (quint16)nCh;
        }

        arrStart[nLen] = (quint16)nNextCode;
    }

    return true;
}

bool XArjDecoder::readPtLen(ArjDecodeState *pState, qint32 nCount, qint32 nBitWidth, qint32 nSpecial)
{
    qint32 nN = getBits(pState, nBitWidth);

    if (pState->bError) {
        return false;
    }

    if (nN == 0) {
        qint16 nC = getBits(pState, nBitWidth);

        if (pState->bError || (nC < 0) || (nC >= nCount) || (nC >= NPT)) {
            pState->bError = true;
            return false;
        }

        for (qint32 i = 0; i < nCount && i < NPT; i++) {
            pState->arrPtLen[i] = 0;
        }

        for (qint32 i = 0; i < PTABLESIZE; i++) {
            pState->arrPtTable[i] = nC;
        }
    } else {
        if ((nN > nCount) || (nN > NPT)) {
            pState->bError = true;
            return false;
        }

        qint32 i = 0;

        while ((i < nN) && (i < NPT)) {
            qint32 nC = pState->nBitBuf >> 13;

            if (nC == 7) {
                quint16 nTestMask = 1 << 12;

                while (nTestMask & pState->nBitBuf) {
                    nTestMask >>= 1;
                    nC++;
                }
            }

            fillBuf(pState, (nC < 7) ? 3 : (nC - 3));

            if (pState->bError) {
                return false;
            }

            pState->arrPtLen[i++] = static_cast<quint8>(nC);

            if (i == nSpecial) {
                qint32 nSkip = getBits(pState, 2);

                if (pState->bError) {
                    return false;
                }

                while ((--nSkip >= 0) && (i < NPT)) {
                    pState->arrPtLen[i++] = 0;
                }
            }
        }

        while ((i < nCount) && (i < NPT)) {
            pState->arrPtLen[i++] = 0;
        }

        if (!makeTable(pState, nCount, pState->arrPtLen, 8, pState->arrPtTable, PTABLESIZE)) {
            return false;
        }
    }

    return true;
}

bool XArjDecoder::readCLen(ArjDecodeState *pState)
{
    qint16 nN = getBits(pState, CBIT);

    if (pState->bError) {
        return false;
    }

    if (nN == 0) {
        qint16 nC = getBits(pState, CBIT);

        if (pState->bError || (nC < 0) || (nC >= NC)) {
            pState->bError = true;
            return false;
        }

        for (qint32 i = 0; i < NC; i++) {
            pState->arrCLen[i] = 0;
        }

        for (qint32 i = 0; i < CTABLESIZE; i++) {
            pState->arrCTable[i] = nC;
        }
    } else {
        if (nN > NC) {
            pState->bError = true;
            return false;
        }

        qint32 i = 0;

        while (i < nN) {
            qint16 nC = pState->arrPtTable[pState->nBitBuf >> 8];

            if (nC >= NT) {
                quint16 nTestMask = 1 << 7;

                do {
                    if (nC >= (2 * NC - 1)) {
                        pState->bError = true;
                        return false;
                    }

                    if (pState->nBitBuf & nTestMask) {
                        nC = pState->arrRight[nC];
                    } else {
                        nC = pState->arrLeft[nC];
                    }

                    nTestMask >>= 1;
                } while (nC >= NT);
            }

            if (nC >= NPT) {
                pState->bError = true;
                return false;
            }

            fillBuf(pState, pState->arrPtLen[nC]);

            if (pState->bError) {
                return false;
            }

            if (nC <= 2) {
                if (nC == 0) {
                    nC = 1;
                } else if (nC == 1) {
                    nC = getBits(pState, 4) + 3;
                } else {
                    nC = getBits(pState, CBIT) + 20;
                }

                if (pState->bError) {
                    return false;
                }

                while (--nC >= 0) {
                    if (i >= NC) {
                        pState->bError = true;
                        return false;
                    }

                    pState->arrCLen[i++] = 0;
                }
            } else {
                if (i >= NC) {
                    pState->bError = true;
                    return false;
                }

                pState->arrCLen[i++] = static_cast<quint8>(nC - 2);
            }
        }

        while (i < NC) {
            pState->arrCLen[i++] = 0;
        }

        if (!makeTable(pState, NC, pState->arrCLen, 12, pState->arrCTable, CTABLESIZE)) {
            return false;
        }
    }

    return true;
}

quint16 XArjDecoder::decodeC(ArjDecodeState *pState)
{
    if (pState->nBlockSize == 0) {
        pState->nBlockSize = getBits(pState, 16);
        if (pState->bError || (pState->nBlockSize == 0) || !readPtLen(pState, NT, TBIT, 3) || !readCLen(pState) ||
            !readPtLen(pState, NP, PBIT, -1)) {
            pState->bError = true;
            return 0;
        }
    }

    if (pState->bError) {
        return 0;
    }

    pState->nBlockSize--;

    quint16 nJ = pState->arrCTable[pState->nBitBuf >> 4];

    if (nJ >= NC) {
        quint16 nTestMask = 1 << 3;

        do {
            if (nJ >= (quint16)(2 * NC - 1)) {
                pState->bError = true;
                return 0;
            }

            if (pState->nBitBuf & nTestMask) {
                nJ = pState->arrRight[nJ];
            } else {
                nJ = pState->arrLeft[nJ];
            }

            nTestMask >>= 1;
        } while (nJ >= NC);
    }

    fillBuf(pState, pState->arrCLen[nJ]);

    return nJ;
}

quint16 XArjDecoder::decodeP(ArjDecodeState *pState)
{
    quint16 nJ = pState->arrPtTable[pState->nBitBuf >> 8];

    if (nJ >= NP) {
        quint16 nTestMask = 1 << 7;

        do {
            if (nJ >= (quint16)(2 * NC - 1)) {
                pState->bError = true;
                return 0;
            }

            if (pState->nBitBuf & nTestMask) {
                nJ = pState->arrRight[nJ];
            } else {
                nJ = pState->arrLeft[nJ];
            }

            nTestMask >>= 1;
        } while (nJ >= NP);
    }

    fillBuf(pState, pState->arrPtLen[nJ]);

    if (pState->bError) return 0;

    if (nJ != 0) {
        quint16 nBitsToRead = nJ - 1;
        nJ = (1 << nBitsToRead) + getBits(pState, nBitsToRead);
    }

    return nJ;
}

quint16 XArjDecoder::decodeLen(ArjDecodeState *pState)
{
    if (!pState || pState->bError || (pState->nGetLen < 0) || (pState->nGetLen > CODE_BIT)) {
        if (pState) pState->bError = true;
        return 0;
    }

    quint16 nC = 0;
    quint16 nWidth = 0;
    quint16 nPlus = 0;
    quint16 nPwr = 1 << STRTL;

    for (nWidth = STRTL; nWidth < STOPL; nWidth++) {
        // getbit
        if (pState->nGetLen <= 0) {
            pState->nGetBuf |= pState->nBitBuf;
            if (!fillBuf(pState, CODE_BIT)) return 0;
            pState->nGetLen = CODE_BIT;
        }

        nC = (pState->nGetBuf & 0x8000) != 0 ? 1 : 0;
        pState->nGetBuf = static_cast<quint16>(pState->nGetBuf * 2);
        pState->nGetLen--;

        if (pState->bError) {
            return 0;
        }

        if (nC == 0) {
            break;
        }

        nPlus += nPwr;
        nPwr <<= 1;
    }

    if (nWidth != 0) {
        // getbits
        if (pState->nGetLen < nWidth) {
            if ((pState->nGetLen < 0) || (pState->nGetLen > CODE_BIT)) {
                pState->bError = true;
                return 0;
            }
            pState->nGetBuf |= pState->nBitBuf >> pState->nGetLen;
            if (!fillBuf(pState, CODE_BIT - pState->nGetLen)) return 0;
            pState->nGetLen = CODE_BIT;
        }

        nC = (quint16)pState->nGetBuf >> (CODE_BIT - nWidth);

        for (qint32 i = 0; i < nWidth; i++) {
            pState->nGetBuf = static_cast<quint16>(pState->nGetBuf * 2);
        }

        pState->nGetLen -= nWidth;
    }

    nC += nPlus;

    return nC;
}

quint16 XArjDecoder::decodePtr(ArjDecodeState *pState)
{
    if (!pState || pState->bError || (pState->nGetLen < 0) || (pState->nGetLen > CODE_BIT)) {
        if (pState) pState->bError = true;
        return 0;
    }

    quint16 nC = 0;
    quint16 nWidth = 0;
    quint16 nPlus = 0;
    quint16 nPwr = 1 << STRTP;

    for (nWidth = STRTP; nWidth < STOPP; nWidth++) {
        // getbit
        if (pState->nGetLen <= 0) {
            pState->nGetBuf |= pState->nBitBuf;
            if (!fillBuf(pState, CODE_BIT)) return 0;
            pState->nGetLen = CODE_BIT;
        }

        nC = (pState->nGetBuf & 0x8000) != 0 ? 1 : 0;
        pState->nGetBuf = static_cast<quint16>(pState->nGetBuf * 2);
        pState->nGetLen--;

        if (pState->bError) {
            return 0;
        }

        if (nC == 0) {
            break;
        }

        nPlus += nPwr;
        nPwr <<= 1;
    }

    if (nWidth != 0) {
        // getbits
        if (pState->nGetLen < nWidth) {
            if ((pState->nGetLen < 0) || (pState->nGetLen > CODE_BIT)) {
                pState->bError = true;
                return 0;
            }
            pState->nGetBuf |= pState->nBitBuf >> pState->nGetLen;
            if (!fillBuf(pState, CODE_BIT - pState->nGetLen)) return 0;
            pState->nGetLen = CODE_BIT;
        }

        nC = (quint16)pState->nGetBuf >> (CODE_BIT - nWidth);

        for (qint32 i = 0; i < nWidth; i++) {
            pState->nGetBuf = static_cast<quint16>(pState->nGetBuf * 2);
        }

        pState->nGetLen -= nWidth;
    }

    nC += nPlus;

    return nC;
}

bool XArjDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    return decompressInternal(pDecompressState, pPdStruct, false);
}

bool XArjDecoder::decompressFastest(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    return decompressInternal(pDecompressState, pPdStruct, true);
}

bool XArjDecoder::decompressInternal(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct, bool bFastest)
{
    XBinary::PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) pPdStruct = &pdStructEmpty;

    quint32 nCompressedSize = 0;
    quint32 nOriginalSize = 0;
    if (!arjPrepareState(pDecompressState, pPdStruct, &nCompressedSize, &nOriginalSize)) return false;

    if (nOriginalSize == 0) {
        return (nCompressedSize == 0) && (pDecompressState->nCountInput == 0) && (pDecompressState->nCountOutput == 0) &&
               !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
    }

    std::unique_ptr<quint8[]> pReadBuffer(new (std::nothrow) quint8[8192]);
    std::unique_ptr<quint8[]> pText(new (std::nothrow) quint8[DDICSIZ]);
    if (!pReadBuffer || !pText) return false;

    memset(pText.get(), 0, DDICSIZ);

    ArjDecodeState state = {};
    state.nReadBufferSize = 8192;
    state.pReadBuffer = pReadBuffer.get();
    state.pText = pText.get();
    state.nCompLeft = nCompressedSize;
    state.pProcessState = pDecompressState;
    state.pPdStruct = pPdStruct;

    if (!initGetBits(&state)) return false;

    quint32 nCount = 0;
    quint32 nOutPtr = 0;

    while ((nCount < nOriginalSize) && !state.bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nToken = bFastest ? static_cast<qint32>(decodeLen(&state)) : static_cast<qint32>(decodeC(&state));
        if (state.bError) break;

        const bool bLiteral = bFastest ? (nToken == 0) : (nToken <= 255);
        if (bLiteral) {
            quint8 nLiteral = static_cast<quint8>(nToken);
            if (bFastest) {
                if ((state.nGetLen < 0) || (state.nGetLen > CODE_BIT)) {
                    state.bError = true;
                    break;
                }
                if (state.nGetLen < 8) {
                    state.nGetBuf |= state.nBitBuf >> state.nGetLen;
                    if (!fillBuf(&state, CODE_BIT - state.nGetLen)) break;
                    state.nGetLen = CODE_BIT;
                }

                nLiteral = static_cast<quint8>(state.nGetBuf >> (CODE_BIT - 8));
                state.nGetBuf = static_cast<quint16>(state.nGetBuf << 8);
                state.nGetLen -= 8;
            }

            state.pText[nOutPtr++] = nLiteral;
            nCount++;
        } else {
            const qint32 nMatchLen = bFastest ? (nToken - 1 + THRESHOLD) : (nToken - (255 + 1 - THRESHOLD));
            if ((nMatchLen <= 0) || (nMatchLen > MAXMATCH) ||
                (static_cast<quint32>(nMatchLen) > (nOriginalSize - nCount))) {
                state.bError = true;
                break;
            }

            const qint32 nPos = bFastest ? static_cast<qint32>(decodePtr(&state)) : static_cast<qint32>(decodeP(&state));
            if (state.bError) break;
            if ((nPos < 0) || (nPos >= DDICSIZ)) {
                state.bError = true;
                break;
            }

            qint32 nSrcIdx = static_cast<qint32>(nOutPtr) - nPos - 1;
            if (nSrcIdx < 0) nSrcIdx += DDICSIZ;
            if ((nSrcIdx < 0) || (nSrcIdx >= DDICSIZ)) {
                state.bError = true;
                break;
            }

            nCount += static_cast<quint32>(nMatchLen);
            for (qint32 i = 0; (i < nMatchLen) && !state.bError; i++) {
                state.pText[nOutPtr] = state.pText[nSrcIdx];
                nOutPtr++;
                nSrcIdx++;

                if (nOutPtr == static_cast<quint32>(DDICSIZ)) {
                    if (!arjWriteAll(pDecompressState, reinterpret_cast<const char *>(state.pText), DDICSIZ, pPdStruct)) {
                        state.bError = true;
                        break;
                    }
                    nOutPtr = 0;
                }
                if (nSrcIdx == DDICSIZ) nSrcIdx = 0;
            }
        }

        if (!state.bError && (nOutPtr == static_cast<quint32>(DDICSIZ))) {
            if (!arjWriteAll(pDecompressState, reinterpret_cast<const char *>(state.pText), DDICSIZ, pPdStruct)) {
                state.bError = true;
                break;
            }
            nOutPtr = 0;
        }
    }

    const bool bExactInput = (pDecompressState->nCountInput == static_cast<qint64>(nCompressedSize)) &&
                             (state.nCompLeft == 0) && (state.nBufAvail == 0);
    const bool bCodecStateExact = bFastest || (state.nBlockSize == 0);
    const bool bDecodedExact = !state.bError && (nCount == nOriginalSize) && bExactInput && bCodecStateExact &&
                               !pDecompressState->bReadError && !pDecompressState->bWriteError &&
                               XBinary::isPdStructNotCanceled(pPdStruct);
    if (bDecodedExact && (nOutPtr != 0) &&
        !arjWriteAll(pDecompressState, reinterpret_cast<const char *>(state.pText), static_cast<qint32>(nOutPtr), pPdStruct)) {
        state.bError = true;
    }

    return !state.bError && bDecodedExact && (pDecompressState->nCountOutput == static_cast<qint64>(nOriginalSize)) &&
           !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
