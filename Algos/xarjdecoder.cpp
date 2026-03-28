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

#include <cstring>

XArjDecoder::XArjDecoder(QObject *pParent) : QObject(pParent)
{
}

void XArjDecoder::refillInputBuffer(ArjDecodeState *pState)
{
    if (pState->pInput) {
        qint32 nToRead = pState->nReadBufferSize;
        qint32 nRead = pState->pInput->read((char *)pState->pReadBuffer, nToRead);

        if (nRead > 0) {
            pState->pBuf = pState->pReadBuffer;
            pState->nBufAvail = nRead;
            pState->nInputBytesRead += nRead;
        } else {
            pState->nBufAvail = 0;
        }
    }
}

bool XArjDecoder::fillBuf(ArjDecodeState *pState, qint32 nBits)
{
    if (pState->bError) {
        return false;
    }

    pState->nBitBuf = (pState->nBitBuf << nBits) & 0xFFFF;

    while (nBits > pState->nBitCount) {
        pState->nBitBuf |= (quint16)(pState->nSubBitBuf << (nBits - pState->nBitCount));
        nBits -= pState->nBitCount;

        if (pState->nCompLeft > 0) {
            pState->nCompLeft--;

            if (pState->nBufAvail <= 0) {
                refillInputBuffer(pState);
            }

            if (pState->nBufAvail > 0) {
                pState->nSubBitBuf = *pState->pBuf++;
                pState->nBufAvail--;
            } else {
                pState->nSubBitBuf = 0;
            }
        } else {
            pState->nSubBitBuf = 0;
        }

        pState->nBitCount = 8;
    }

    pState->nBitBuf |= pState->nSubBitBuf >> (pState->nBitCount - nBits);
    pState->nBitCount -= nBits;

    return true;
}

quint16 XArjDecoder::getBits(ArjDecodeState *pState, qint32 nBits)
{
    quint16 nResult = pState->nBitBuf >> (16 - nBits);
    fillBuf(pState, nBits);
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

    if (nPos != (quint16)(1 << 16) >> nJutBits) {
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

        if (pState->bError) {
            return false;
        }

        for (qint32 i = 0; i < nCount && i < NPT; i++) {
            pState->arrPtLen[i] = 0;
        }

        for (qint32 i = 0; i < PTABLESIZE; i++) {
            pState->arrPtTable[i] = nC;
        }
    } else {
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

            pState->arrPtLen[i++] = (quint8)nC;

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

        if (pState->bError) {
            return false;
        }

        for (qint32 i = 0; i < NC; i++) {
            pState->arrCLen[i] = 0;
        }

        for (qint32 i = 0; i < CTABLESIZE; i++) {
            pState->arrCTable[i] = nC;
        }
    } else {
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

                pState->arrCLen[i++] = (quint8)(nC - 2);
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
        readPtLen(pState, NT, TBIT, 3);
        readCLen(pState);
        readPtLen(pState, NP, PBIT, -1);
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

    if (nJ != 0) {
        quint16 nBitsToRead = nJ - 1;
        nJ = (1 << nBitsToRead) + getBits(pState, nBitsToRead);
    }

    return nJ;
}

quint16 XArjDecoder::decodeLen(ArjDecodeState *pState)
{
    quint16 nC = 0;
    quint16 nWidth = 0;
    quint16 nPlus = 0;
    quint16 nPwr = 1 << STRTL;

    for (nWidth = STRTL; nWidth < STOPL; nWidth++) {
        // getbit
        if (pState->nGetLen <= 0) {
            pState->nGetBuf |= pState->nBitBuf >> pState->nGetLen;
            fillBuf(pState, CODE_BIT - pState->nGetLen);
            pState->nGetLen = CODE_BIT;
        }

        nC = (pState->nGetBuf & 0x8000) != 0 ? 1 : 0;
        pState->nGetBuf = (quint16)(pState->nGetBuf * 2);
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
            pState->nGetBuf |= pState->nBitBuf >> pState->nGetLen;
            fillBuf(pState, CODE_BIT - pState->nGetLen);
            pState->nGetLen = CODE_BIT;
        }

        nC = (quint16)pState->nGetBuf >> (CODE_BIT - nWidth);

        for (qint32 i = 0; i < nWidth; i++) {
            pState->nGetBuf = (quint16)(pState->nGetBuf * 2);
        }

        pState->nGetLen -= nWidth;
    }

    nC += nPlus;

    return nC;
}

quint16 XArjDecoder::decodePtr(ArjDecodeState *pState)
{
    quint16 nC = 0;
    quint16 nWidth = 0;
    quint16 nPlus = 0;
    quint16 nPwr = 1 << STRTP;

    for (nWidth = STRTP; nWidth < STOPP; nWidth++) {
        // getbit
        if (pState->nGetLen <= 0) {
            pState->nGetBuf |= pState->nBitBuf >> pState->nGetLen;
            fillBuf(pState, CODE_BIT - pState->nGetLen);
            pState->nGetLen = CODE_BIT;
        }

        nC = (pState->nGetBuf & 0x8000) != 0 ? 1 : 0;
        pState->nGetBuf = (quint16)(pState->nGetBuf * 2);
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
            pState->nGetBuf |= pState->nBitBuf >> pState->nGetLen;
            fillBuf(pState, CODE_BIT - pState->nGetLen);
            pState->nGetLen = CODE_BIT;
        }

        nC = (quint16)pState->nGetBuf >> (CODE_BIT - nWidth);

        for (qint32 i = 0; i < nWidth; i++) {
            pState->nGetBuf = (quint16)(pState->nGetBuf * 2);
        }

        pState->nGetLen -= nWidth;
    }

    nC += nPlus;

    return nC;
}

bool XArjDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    XBinary::PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    qint64 nOrigSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    ArjDecodeState state = {};
    state.bError = false;
    state.nBlockSize = 0;
    state.nReadBufferSize = 8192;
    state.pReadBuffer = new quint8[state.nReadBufferSize];
    state.pText = new quint8[DDICSIZ];

    if (!state.pReadBuffer || !state.pText) {
        delete[] state.pReadBuffer;
        delete[] state.pText;
        return false;
    }

    memset(state.pText, 0, DDICSIZ);

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    state.nCompLeft = (quint32)pDecompressState->nInputLimit;
    state.pInput = pDecompressState->pDeviceInput;
    state.nInputBytesRead = 0;
    state.pBuf = nullptr;
    state.nBufAvail = 0;

    if (!initGetBits(&state)) {
        delete[] state.pReadBuffer;
        delete[] state.pText;
        return false;
    }

    quint32 nCount = 0;
    quint32 nOutPtr = 0;
    qint64 nOutputWritten = 0;

    while ((nCount < (quint32)nOrigSize) && !state.bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint16 nChr = (qint16)decodeC(&state);

        if (state.bError) {
            break;
        }

        if (nChr <= 255) {
            state.pText[nOutPtr] = (quint8)nChr;
            nCount++;

            if (++nOutPtr >= (quint32)DDICSIZ) {
                nOutPtr = 0;
                qint64 nWritten = pDecompressState->pDeviceOutput->write((char *)state.pText, DDICSIZ);

                if (nWritten != DDICSIZ) {
                    state.bError = true;
                    break;
                }

                nOutputWritten += nWritten;
            }
        } else {
            qint32 nMatchLen = nChr - (255 + 1 - THRESHOLD);
            nCount += nMatchLen;

            qint32 nPos = decodeP(&state);

            if (state.bError) {
                break;
            }

            qint32 nSrcIdx = nOutPtr - nPos - 1;

            if (nSrcIdx < 0) {
                nSrcIdx += DDICSIZ;
            }

            if ((nSrcIdx >= DDICSIZ) || (nSrcIdx < 0)) {
                state.bError = true;
                break;
            }

            if ((quint32)nSrcIdx < nOutPtr && nOutPtr < (quint32)(DDICSIZ - MAXMATCH - 1)) {
                while ((--nMatchLen >= 0) && (nSrcIdx < DDICSIZ) && (nOutPtr < (quint32)DDICSIZ)) {
                    state.pText[nOutPtr++] = state.pText[nSrcIdx++];
                }
            } else {
                while (--nMatchLen >= 0) {
                    state.pText[nOutPtr] = state.pText[nSrcIdx];

                    if (++nOutPtr >= (quint32)DDICSIZ) {
                        nOutPtr = 0;
                        qint64 nWritten = pDecompressState->pDeviceOutput->write((char *)state.pText, DDICSIZ);

                        if (nWritten != DDICSIZ) {
                            state.bError = true;
                            break;
                        }

                        nOutputWritten += nWritten;
                    }

                    if (++nSrcIdx >= DDICSIZ) {
                        nSrcIdx = 0;
                    }
                }
            }
        }
    }

    if (!state.bError && (nOutPtr != 0)) {
        qint64 nWritten = pDecompressState->pDeviceOutput->write((char *)state.pText, nOutPtr);

        if (nWritten != (qint64)nOutPtr) {
            state.bError = true;
        } else {
            nOutputWritten += nWritten;
        }
    }

    pDecompressState->nCountInput = state.nInputBytesRead;
    pDecompressState->nCountOutput = nOutputWritten;

    delete[] state.pReadBuffer;
    delete[] state.pText;

    bResult = !state.bError;

    return bResult;
}

bool XArjDecoder::decompressFastest(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    XBinary::PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    qint64 nOrigSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    ArjDecodeState state = {};
    state.bError = false;
    state.nGetLen = 0;
    state.nGetBuf = 0;
    state.nReadBufferSize = 8192;
    state.pReadBuffer = new quint8[state.nReadBufferSize];
    state.pText = new quint8[DDICSIZ];

    if (!state.pReadBuffer || !state.pText) {
        delete[] state.pReadBuffer;
        delete[] state.pText;
        return false;
    }

    memset(state.pText, 0, DDICSIZ);

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    state.nCompLeft = (quint32)pDecompressState->nInputLimit;
    state.pInput = pDecompressState->pDeviceInput;
    state.nInputBytesRead = 0;
    state.pBuf = nullptr;
    state.nBufAvail = 0;

    if (!initGetBits(&state)) {
        delete[] state.pReadBuffer;
        delete[] state.pText;
        return false;
    }

    quint32 nCount = 0;
    quint32 nOutPtr = 0;
    qint64 nOutputWritten = 0;

    while ((nCount < (quint32)nOrigSize) && !state.bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint16 nChr = (qint16)decodeLen(&state);

        if (state.bError) {
            break;
        }

        if (nChr == 0) {
            // Literal byte: getbits(8)
            if (state.nGetLen < 8) {
                state.nGetBuf |= state.nBitBuf >> state.nGetLen;
                fillBuf(&state, CODE_BIT - state.nGetLen);
                state.nGetLen = CODE_BIT;
            }

            nChr = (quint16)state.nGetBuf >> (CODE_BIT - 8);

            for (qint32 i = 0; i < 8; i++) {
                state.nGetBuf = (quint16)(state.nGetBuf * 2);
            }

            state.nGetLen -= 8;

            if (state.bError) {
                break;
            }

            state.pText[nOutPtr] = (quint8)nChr;
            nCount++;

            if (++nOutPtr >= (quint32)DDICSIZ) {
                nOutPtr = 0;
                qint64 nWritten = pDecompressState->pDeviceOutput->write((char *)state.pText, DDICSIZ);

                if (nWritten != DDICSIZ) {
                    state.bError = true;
                    break;
                }

                nOutputWritten += nWritten;
            }
        } else {
            qint32 nMatchLen = nChr - 1 + THRESHOLD;
            nCount += nMatchLen;

            qint32 nPos = decodePtr(&state);

            if (state.bError) {
                break;
            }

            qint32 nSrcIdx = nOutPtr - nPos - 1;

            if (nSrcIdx < 0) {
                nSrcIdx += DDICSIZ;
            }

            if ((nSrcIdx >= DDICSIZ) || (nSrcIdx < 0)) {
                state.bError = true;
                break;
            }

            while (--nMatchLen >= 0) {
                state.pText[nOutPtr] = state.pText[nSrcIdx];

                if (++nOutPtr >= (quint32)DDICSIZ) {
                    nOutPtr = 0;
                    qint64 nWritten = pDecompressState->pDeviceOutput->write((char *)state.pText, DDICSIZ);

                    if (nWritten != DDICSIZ) {
                        state.bError = true;
                        break;
                    }

                    nOutputWritten += nWritten;
                }

                if (++nSrcIdx >= DDICSIZ) {
                    nSrcIdx = 0;
                }
            }
        }
    }

    if (!state.bError && (nOutPtr != 0)) {
        qint64 nWritten = pDecompressState->pDeviceOutput->write((char *)state.pText, nOutPtr);

        if (nWritten != (qint64)nOutPtr) {
            state.bError = true;
        } else {
            nOutputWritten += nWritten;
        }
    }

    pDecompressState->nCountInput = state.nInputBytesRead;
    pDecompressState->nCountOutput = nOutputWritten;

    delete[] state.pReadBuffer;
    delete[] state.pText;

    bResult = !state.bError;

    return bResult;
}
