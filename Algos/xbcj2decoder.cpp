/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xbcj2decoder.h"

#include <limits>

// BCJ2 range-coder constants
static const quint32 BCJ2_RC_RANGE_MIN = 0x01000000U;
static const quint32 BCJ2_PROB_INIT = 0x400U;  // 50% = 1024 out of 2048
static const qint32 BCJ2_NUM_PROBS = 258;      // 0: JCC (0x0F 0x8x, unused); 1: E9 (JMP); 2..257: E8 keyed by prevByte

// Append bytes to the output device, advancing *pnOutputPos. Positive short
// writes are legal QIODevice progress and must be drained.
static bool bcj2ReadExact(QIODevice *pInput, char *pData, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pInput || (nSize < 0) || ((nSize > 0) && !pData)) return false;

    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRead = pInput->read(pData + nDone, nSize - nDone);
        if ((nRead <= 0) || (nRead > (nSize - nDone))) return false;
        nDone += nRead;
    }

    return (nDone == nSize) && XBinary::isPdStructNotCanceled(pPdStruct);
}

static bool bcj2WriteOutput(QIODevice *pOutput, const char *pData, qint64 nSize,
                            qint64 *pnOutputPos, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pOutput || !pnOutputPos || (nSize < 0) || ((nSize > 0) && !pData)) return false;

    qint64 nDone = 0;
    while ((nDone < nSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nWritten = pOutput->write(pData + nDone, nSize - nDone);
        if ((nWritten <= 0) || (nWritten > (nSize - nDone))) return false;
        nDone += nWritten;
    }

    if ((nDone != nSize) || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (*pnOutputPos > (std::numeric_limits<qint64>::max)() - nDone)) return false;
    *pnOutputPos += nDone;
    return true;
}

static bool bcj2GetRemainingSize(QIODevice *pInput, qint64 *pnSize)
{
    if (!pInput || !pnSize || pInput->isSequential()) return false;
    const qint64 nPosition = pInput->pos();
    const qint64 nSize = pInput->size();
    if ((nPosition < 0) || (nSize < nPosition)) return false;
    *pnSize = nSize - nPosition;
    return true;
}

static bool bcj2IsExactEnd(QIODevice *pInput)
{
    if (!pInput) return false;
    if (!pInput->isSequential()) return (pInput->pos() >= 0) && (pInput->pos() == pInput->size());

    char cExtra = 0;
    const qint64 nRead = pInput->read(&cExtra, 1);
    return (nRead == 0) && pInput->atEnd();
}

static bool bcj2ResetOutput(QIODevice *pOutput)
{
    return pOutput && !pOutput->isSequential() && XBinary::isResizeEnable(pOutput) &&
           XBinary::resize(pOutput, 0) && pOutput->seek(0);
}

static bool bcj2Fail(QIODevice *pOutput)
{
    bcj2ResetOutput(pOutput);
    return false;
}

XBCJ2Decoder::XBCJ2Decoder(QObject *parent) : QObject(parent)
{
}

bool XBCJ2Decoder::_rcInit(RC_STATE *pRC, XBinary::PDSTRUCT *pPdStruct)
{
    // BCJ2 range coder initialisation:
    // Read 5 bytes: byte[0] is dummy (0x00), bytes[1..4] form the initial Code value.
    if (!pRC) return false;
    char buf[5] = {};
    if (!pRC->pStream || !bcj2ReadExact(pRC->pStream, buf, sizeof(buf), pPdStruct) ||
        ((quint8)buf[0] != 0)) {
        pRC->bEof = true;
        pRC->nRange = 0;
        pRC->nCode = 0;
        return false;
    }

    pRC->bEof = false;
    pRC->nRange = 0xFFFFFFFFU;
    pRC->nCode = ((quint32)(quint8)buf[1] << 24) | ((quint32)(quint8)buf[2] << 16) | ((quint32)(quint8)buf[3] << 8) | (quint32)(quint8)buf[4];

    if (pRC->nCode == 0xFFFFFFFFU) {
        pRC->bEof = true;
        pRC->nRange = 0;
        pRC->nCode = 0;
        return false;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XBCJ2Decoder::_rcNormalize(RC_STATE *pRC, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pRC || !pRC->pStream || pRC->bEof) return false;

    while ((pRC->nRange < BCJ2_RC_RANGE_MIN) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        char b = 0;
        if (!bcj2ReadExact(pRC->pStream, &b, 1, pPdStruct)) {
            pRC->bEof = true;
            return false;
        }
        pRC->nRange <<= 8;
        pRC->nCode = (pRC->nCode << 8) | (quint8)b;
    }

    return (pRC->nRange >= BCJ2_RC_RANGE_MIN) && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XBCJ2Decoder::_rcDecodeBit(RC_STATE *pRC, quint32 *pProb, quint32 *pBit,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pProb || !pBit || !_rcNormalize(pRC, pPdStruct)) return false;

    quint32 nBound = (pRC->nRange >> 11) * (*pProb);

    if (pRC->nCode < nBound) {
        pRC->nRange = nBound;
        *pProb += (2048U - *pProb) >> 5;
        *pBit = 0U;
    } else {
        pRC->nCode -= nBound;
        pRC->nRange -= nBound;
        *pProb -= *pProb >> 5;
        *pBit = 1U;
    }

    return true;
}

bool XBCJ2Decoder::decompress(QIODevice *pMainStream, QIODevice *pCallStream, QIODevice *pJmpStream, QIODevice *pRangeStream, QIODevice *pOutput, qint64 nOutputSize,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pMainStream || !pCallStream || !pJmpStream || !pRangeStream || !pOutput ||
        !pMainStream->isOpen() || !pMainStream->isReadable() ||
        !pCallStream->isOpen() || !pCallStream->isReadable() ||
        !pJmpStream->isOpen() || !pJmpStream->isReadable() ||
        !pRangeStream->isOpen() || !pRangeStream->isReadable() ||
        !pOutput->isOpen() || !pOutput->isWritable() || pOutput->isSequential() ||
        !XBinary::isResizeEnable(pOutput) || (nOutputSize < 0)) {
        return false;
    }

    if (!bcj2ResetOutput(pOutput)) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return bcj2Fail(pOutput);

    qint64 nMainRemaining = 0;
    qint64 nCallRemaining = 0;
    qint64 nJmpRemaining = 0;
    qint64 nRangeRemaining = 0;
    const bool bMainSizeKnown = bcj2GetRemainingSize(pMainStream, &nMainRemaining);
    const bool bCallSizeKnown = bcj2GetRemainingSize(pCallStream, &nCallRemaining);
    const bool bJmpSizeKnown = bcj2GetRemainingSize(pJmpStream, &nJmpRemaining);
    const bool bRangeSizeKnown = bcj2GetRemainingSize(pRangeStream, &nRangeRemaining);
    if ((bCallSizeKnown && ((nCallRemaining & 3) != 0)) ||
        (bJmpSizeKnown && ((nJmpRemaining & 3) != 0)) ||
        (bRangeSizeKnown && (nRangeRemaining < 5))) return bcj2Fail(pOutput);
    if (bMainSizeKnown && bCallSizeKnown && bJmpSizeKnown) {
        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if ((nMainRemaining > nMax - nCallRemaining) ||
            (nMainRemaining + nCallRemaining > nMax - nJmpRemaining) ||
            (nMainRemaining + nCallRemaining + nJmpRemaining != nOutputSize)) return bcj2Fail(pOutput);
    }

    // Initialise probability table: 256 slots for E8 (indexed by previous byte) + 1 for E9
    quint32 probs[BCJ2_NUM_PROBS];
    for (qint32 i = 0; i < BCJ2_NUM_PROBS; i++) {
        probs[i] = BCJ2_PROB_INIT;
    }

    // Initialise range coder
    RC_STATE rc;
    rc.pStream = pRangeStream;
    rc.nRange = 0;
    rc.nCode = 0;
    rc.bEof = false;

    if (!_rcInit(&rc, pPdStruct)) return bcj2Fail(pOutput);

    quint8 nPrevByte = 0;
    qint64 nOutputPos = 0;

    while ((nOutputPos < nOutputSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        char bMain = 0;
        if (!bcj2ReadExact(pMainStream, &bMain, 1, pPdStruct)) return bcj2Fail(pOutput);

        quint8 nByte = (quint8)bMain;
        if (!bcj2WriteOutput(pOutput, &bMain, 1, &nOutputPos, pPdStruct)) return bcj2Fail(pOutput);

        if (nByte == 0xE8 || nByte == 0xE9) {
            // 7-zip BCJ2 probability table layout (from Bcj2.c):
            //   probs[0]        : JCC (0x0F 0x8x conditional branches)
            //   probs[1]        : E9 (JMP NEAR)
            //   probs[2..257]   : E8 (CALL NEAR) keyed by previous byte
            quint32 nProbIndex = (nByte == 0xE8) ? (2U + (quint32)nPrevByte) : 1U;
            quint32 nBit = 0;
            if (!_rcDecodeBit(&rc, &probs[nProbIndex], &nBit, pPdStruct)) return bcj2Fail(pOutput);

            if (nBit == 1U) {
                // Real CALL/JMP: read 4-byte absolute address from call/jmp stream.
                // The address is stored big-endian (see GetBe32 in 7-zip Bcj2.c).
                QIODevice *pAddrStream = (nByte == 0xE8) ? pCallStream : pJmpStream;

                if ((nOutputSize - nOutputPos) < 4) return bcj2Fail(pOutput);
                char addr[4] = {};
                if (!bcj2ReadExact(pAddrStream, addr, sizeof(addr), pPdStruct)) return bcj2Fail(pOutput);

                // Big-endian absolute address stored in stream
                quint32 nAbsAddr = ((quint32)(quint8)addr[0] << 24) | ((quint32)(quint8)addr[1] << 16) | ((quint32)(quint8)addr[2] << 8) | (quint32)(quint8)addr[3];

                // nOutputPos is 1 past the opcode; ip after full instruction = nOutputPos+4
                quint32 nRelAddr = nAbsAddr - (quint32)(nOutputPos + 4);

                char relAddr[4];
                relAddr[0] = (char)(nRelAddr);
                relAddr[1] = (char)(nRelAddr >> 8);
                relAddr[2] = (char)(nRelAddr >> 16);
                relAddr[3] = (char)(nRelAddr >> 24);

                if (!bcj2WriteOutput(pOutput, relAddr, 4, &nOutputPos, pPdStruct)) return bcj2Fail(pOutput);

                // prevByte for next E8 lookup = last byte of relative address written
                nPrevByte = (quint8)relAddr[3];
            } else {
                // Data E8/E9 (not a real instruction): prevByte = the opcode itself
                nPrevByte = nByte;
            }
        } else if (((quint8)nByte & 0xF0U) == 0x80U && nPrevByte == 0x0FU) {
            // JCC: 0x0F followed by 0x8x (conditional branch, e.g. JNE/JE/JL/...).
            // Uses probs[0] and the jmp stream (same as E9).
            quint32 nBit = 0;
            if (!_rcDecodeBit(&rc, &probs[0], &nBit, pPdStruct)) return bcj2Fail(pOutput);

            if (nBit == 1U) {
                if ((nOutputSize - nOutputPos) < 4) return bcj2Fail(pOutput);
                char addr[4] = {};
                if (!bcj2ReadExact(pJmpStream, addr, sizeof(addr), pPdStruct)) return bcj2Fail(pOutput);

                quint32 nAbsAddr = ((quint32)(quint8)addr[0] << 24) | ((quint32)(quint8)addr[1] << 16) | ((quint32)(quint8)addr[2] << 8) | (quint32)(quint8)addr[3];

                // ip after full JCC instruction (0x0F + 0x8x + 4 rel bytes) = nOutputPos+4
                quint32 nRelAddr = nAbsAddr - (quint32)(nOutputPos + 4);

                char relAddr[4];
                relAddr[0] = (char)(nRelAddr);
                relAddr[1] = (char)(nRelAddr >> 8);
                relAddr[2] = (char)(nRelAddr >> 16);
                relAddr[3] = (char)(nRelAddr >> 24);

                if (!bcj2WriteOutput(pOutput, relAddr, 4, &nOutputPos, pPdStruct)) return bcj2Fail(pOutput);

                nPrevByte = (quint8)relAddr[3];
            } else {
                nPrevByte = nByte;
            }
        } else {
            nPrevByte = nByte;
        }
    }

    if ((nOutputPos != nOutputSize) || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !_rcNormalize(&rc, pPdStruct) || rc.bEof || (rc.nCode != 0) ||
        !bcj2IsExactEnd(pMainStream) || !bcj2IsExactEnd(pCallStream) ||
        !bcj2IsExactEnd(pJmpStream) || !bcj2IsExactEnd(pRangeStream) ||
        (pOutput->pos() != nOutputSize) || (pOutput->size() != nOutputSize)) return bcj2Fail(pOutput);

    return true;
}
