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
#include <QDebug>

// BCJ2 range-coder constants
static const quint32 BCJ2_RC_RANGE_MIN = 0x01000000U;
static const quint32 BCJ2_PROB_INIT = 0x400U;  // 50% = 1024 out of 2048
static const qint32 BCJ2_NUM_PROBS = 258;      // 0: JCC (0x0F 0x8x, unused); 1: E9 (JMP); 2..257: E8 keyed by prevByte

XBCJ2Decoder::XBCJ2Decoder(QObject *parent) : QObject(parent)
{
}

bool XBCJ2Decoder::_rcInit(RC_STATE *pRC)
{
    // BCJ2 range coder initialisation:
    // Read 5 bytes: byte[0] is dummy (0x00), bytes[1..4] form the initial Code value.
    char buf[5];
    qint64 nRead = pRC->pStream->read(buf, 5);

    if (nRead < 5) {
        pRC->bEof = true;
        pRC->nRange = 0;
        pRC->nCode = 0;
        return false;
    }

    pRC->bEof = false;
    pRC->nRange = 0xFFFFFFFFU;
    pRC->nCode = ((quint32)(quint8)buf[1] << 24) | ((quint32)(quint8)buf[2] << 16) | ((quint32)(quint8)buf[3] << 8) | (quint32)(quint8)buf[4];

    return true;
}

void XBCJ2Decoder::_rcNormalize(RC_STATE *pRC)
{
    while (pRC->nRange < BCJ2_RC_RANGE_MIN) {
        char b;
        if (pRC->pStream->read(&b, 1) != 1) {
            pRC->bEof = true;
            pRC->nRange <<= 8;
            pRC->nCode <<= 8;
            return;
        }
        pRC->nRange <<= 8;
        pRC->nCode = (pRC->nCode << 8) | (quint8)b;
    }
}

quint32 XBCJ2Decoder::_rcDecodeBit(RC_STATE *pRC, quint32 *pProb)
{
    _rcNormalize(pRC);

    quint32 nBound = (pRC->nRange >> 11) * (*pProb);

    if (pRC->nCode < nBound) {
        pRC->nRange = nBound;
        *pProb += (2048U - *pProb) >> 5;
        return 0U;
    } else {
        pRC->nCode -= nBound;
        pRC->nRange -= nBound;
        *pProb -= *pProb >> 5;
        return 1U;
    }
}

bool XBCJ2Decoder::decompress(QIODevice *pMainStream, QIODevice *pCallStream, QIODevice *pJmpStream, QIODevice *pRangeStream, QIODevice *pOutput, qint64 nOutputSize,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pMainStream || !pCallStream || !pJmpStream || !pRangeStream || !pOutput) {
        return false;
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

    if (!_rcInit(&rc)) {
        return false;
    }

    quint8 nPrevByte = 0;
    qint64 nOutputPos = 0;

    pOutput->seek(0);

    while ((nOutputPos < nOutputSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        char bMain;
        if (pMainStream->read(&bMain, 1) != 1) {
            qDebug() << "BCJ2Decoder: main stream EOF at outputPos=" << nOutputPos;
            break;
        }

        quint8 nByte = (quint8)bMain;
        pOutput->write(&bMain, 1);
        nOutputPos++;

        if (nByte == 0xE8 || nByte == 0xE9) {
            // 7-zip BCJ2 probability table layout (from Bcj2.c):
            //   probs[0]        : JCC (0x0F 0x8x conditional branches)
            //   probs[1]        : E9 (JMP NEAR)
            //   probs[2..257]   : E8 (CALL NEAR) keyed by previous byte
            quint32 nProbIndex = (nByte == 0xE8) ? (2U + (quint32)nPrevByte) : 1U;
            quint32 nBit = _rcDecodeBit(&rc, &probs[nProbIndex]);

            if (nBit == 1U) {
                // Real CALL/JMP: read 4-byte absolute address from call/jmp stream.
                // The address is stored big-endian (see GetBe32 in 7-zip Bcj2.c).
                QIODevice *pAddrStream = (nByte == 0xE8) ? pCallStream : pJmpStream;

                char addr[4];
                if (pAddrStream->read(addr, 4) != 4) {
                    break;
                }

                // Big-endian absolute address stored in stream
                quint32 nAbsAddr = ((quint32)(quint8)addr[0] << 24) | ((quint32)(quint8)addr[1] << 16) | ((quint32)(quint8)addr[2] << 8) | (quint32)(quint8)addr[3];

                // nOutputPos is 1 past the opcode; ip after full instruction = nOutputPos+4
                quint32 nRelAddr = nAbsAddr - (quint32)(nOutputPos + 4);

                char relAddr[4];
                relAddr[0] = (char)(nRelAddr);
                relAddr[1] = (char)(nRelAddr >> 8);
                relAddr[2] = (char)(nRelAddr >> 16);
                relAddr[3] = (char)(nRelAddr >> 24);

                pOutput->write(relAddr, 4);
                nOutputPos += 4;

                // prevByte for next E8 lookup = last byte of relative address written
                nPrevByte = (quint8)relAddr[3];
            } else {
                // Data E8/E9 (not a real instruction): prevByte = the opcode itself
                nPrevByte = nByte;
            }
        } else if (((quint8)nByte & 0xF0U) == 0x80U && nPrevByte == 0x0FU) {
            // JCC: 0x0F followed by 0x8x (conditional branch, e.g. JNE/JE/JL/...).
            // Uses probs[0] and the jmp stream (same as E9).
            quint32 nBit = _rcDecodeBit(&rc, &probs[0]);

            if (nBit == 1U) {
                char addr[4];
                if (pJmpStream->read(addr, 4) != 4) {
                    break;
                }

                quint32 nAbsAddr = ((quint32)(quint8)addr[0] << 24) | ((quint32)(quint8)addr[1] << 16) | ((quint32)(quint8)addr[2] << 8) | (quint32)(quint8)addr[3];

                // ip after full JCC instruction (0x0F + 0x8x + 4 rel bytes) = nOutputPos+4
                quint32 nRelAddr = nAbsAddr - (quint32)(nOutputPos + 4);

                char relAddr[4];
                relAddr[0] = (char)(nRelAddr);
                relAddr[1] = (char)(nRelAddr >> 8);
                relAddr[2] = (char)(nRelAddr >> 16);
                relAddr[3] = (char)(nRelAddr >> 24);

                pOutput->write(relAddr, 4);
                nOutputPos += 4;

                nPrevByte = (quint8)relAddr[3];
            } else {
                nPrevByte = nByte;
            }
        } else {
            nPrevByte = nByte;
        }
    }

    return (nOutputPos == nOutputSize);
}
