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
#include "xit214.h"

XIT214::XIT214(QObject *parent)
    : QObject(parent)
{}

bool XIT214::decompress(XBinary::DECOMPRESS_STATE *pDecompressState, quint8 nBits, bool bIs215, XBinary::PDSTRUCT *pPdStruct)
{
    quint16 blklen;		/* length of compressed data block in samples */
    quint16 blkpos;		/* position in block */
    quint8 width;		/* actual "bit width" */

    const qint32 N_BUFFER_SIZE = 0x8000;

    char bufferOut[N_BUFFER_SIZE];

    bool bProcess = true;

    while (XBinary::isPdStructNotCanceled(pPdStruct) && bProcess) {
        STATE state = {};
        memset(bufferOut, 0, N_BUFFER_SIZE);
        char *pDestpos = bufferOut;	/* position in output buffer */

        qint32 nBufferSize = N_BUFFER_SIZE;

        /* now unpack data till the dest buffer is full */
        /* read a new block of compressed data and reset variables */

        if (!readBlock(&state, pDecompressState)) {
            bProcess = false;
            break;
        }

        blkpos = 0;
        blklen = 0;

        if (nBits == 8) {
            quint16 value;		/* value read from file to be processed */
            char d1, d2;		/* integrator buffers (d2 for it2.15) */

            d1 = d2 = 0;	/* reset integrator buffers */
            blklen = (nBufferSize < 0x8000) ? nBufferSize : 0x8000;
            width = 9;	/* start with width of 9 bits */

            /* now uncompress the data block */
            while (blkpos < blklen) {
                quint8 v;

                value = readbits(&state, width);	/* read bits */

                if (width < 7) {	/* method 1 (1-6 bits) */
                    if (value == (1 << (width - 1))) {	/* check for "100..." */
                        value = readbits(&state, 3) + 1;	/* yes -> read new width; */
                        width = (value < width) ? value : value + 1;	/* and expand it */
                        continue;	/* ... next value */
                    }
                } else if (width < 9) {	/* method 2 (7-8 bits) */
                    char border = (0xFF >> (9 - width)) - 4;	/* lower border for width chg */

                    if (value > border && value <= (border + 8)) {
                        value -= border;	/* convert width to 1-8 */
                        width = (value < width) ? value : value + 1;	/* and expand it */
                        continue;	/* ... next value */
                    }
                } else if (width == 9) {	/* method 3 (9 bits) */
                    if (value & 0x100) {	/* bit 8 set? */
                        width = (value + 1) & 0xff;	/* new width... */
                        continue;	/* ... and next value */
                    }
                } else {	/* illegal width, abort */
                    bProcess = false;
                    break;
                }

                /* now expand value to signed byte */
                /*      sbyte v;  // sample value */
                if (width < 8) {
                    char shift = 8 - width;
                    v = (value << shift);
                    v >>= shift;
                } else
                    v = (quint8) value;

                /* integrate upon the sample values */
                d1 += v;
                d2 += d1;

                /* ... and store it into the buffer */
                *(char *)pDestpos = bIs215 ? d2 : d1;
                pDestpos ++;

                blkpos++;
            }
        } else if (nBits == 16) {
            quint32 value;		/* value read from file to be processed */
            qint16 d1, d2;		/* integrator buffers (d2 for it2.15) */

            d1 = d2 = 0;	/* reset integrator buffers */
            blklen = (nBufferSize < 0x4000) ? nBufferSize : 0x4000;
            width = 17;	/* start with width of 17 bits */

            while (blkpos < blklen) {
                qint16 v;

                value = readbits(&state, width);	/* read bits */

                if (width < 7) {	/* method 1 (1-6 bits) */
                    if (value == (1 << (width - 1))) {	/* check for "100..." */
                        value = readbits(&state, 4) + 1;	/* yes -> read new width; */
                        width = (value < width) ? value : value + 1;	/* and expand it */
                        continue;	/* ... next value */
                    }
                } else if (width < 17) {	/* method 2 (7-16 bits) */
                    quint16 border = (0xFFFF >> (17 - width)) - 8;	/* lower border for width chg */

                    if (value > border && value <= (border + 16)) {
                        value -= border;	/* convert width to 1-8 */
                        width = (value < width) ? value : value + 1;	/* and expand it */
                        continue;	/* ... next value */
                    }
                } else if (width == 17) {	/* method 3 (17 bits) */
                    if (value & 0x10000) {	/* bit 16 set? */
                        width = (value + 1) & 0xff;	/* new width... */
                        continue;	/* ... and next value */
                    }
                } else {	/* illegal width, abort */
                    bProcess = false;
                    break;
                }

                /* now expand value to signed word */
                /* sword v; // sample value */
                if (width < 16) {
                    quint8 shift = 16 - width;
                    v = (value << shift);
                    v >>= shift;
                } else
                    v = (qint16) value;

                /* integrate upon the sample values */
                d1 += v;
                d2 += d1;

                /* ... and store it into the buffer */
                *(qint16 *)pDestpos = bIs215 ? d2 : d1;
                pDestpos += 2;
                blkpos++;
            }
        }

        if (state.pBufferIn) {
            delete [] state.pBufferIn;
            state.pBufferIn = nullptr;
        }

        /* now subtract block lenght from total length and go on */
        nBufferSize -= blklen;

        qint32 nCount = pDestpos - bufferOut;

        if (nCount > 0) {
            XBinary::_writeDevice(bufferOut, nCount, pDecompressState);
        } else {
            bProcess = false;
        }
    }

    return true;
}

quint32 XIT214::readbits(STATE *pState, quint8 n)
{
    quint32 retval = 0;
    int offset = 0;
    while (n) {
        int m = n;

        if (!pState->bitlen) {
            return 0;
        }

        if (m > pState->bitnum)
            m = pState->bitnum;
        retval |= (*(pState->ibuf) & ((1L << m) - 1)) << offset;
        *(pState->ibuf) >>= m;
        n -= m;
        offset += m;
        if (!(pState->bitnum -= m)) {
            pState->bitlen--;
            pState->ibuf++;
            pState->bitnum = 8;
        }
    }
    return retval;
}

bool XIT214::readBlock(STATE *pState, XBinary::DECOMPRESS_STATE *pDecompressState)
{
    bool bResult = false;

    quint8 a = 0;
    quint8 b = 0;

    XBinary::_readDevice((char *)&a, 1, pDecompressState);
    XBinary::_readDevice((char *)&b, 1, pDecompressState);

    pState->nInputBufferSize = (b << 8) | a;

    if ((pState->nInputBufferSize > 0) && (pDecompressState->pDeviceInput->size() - pDecompressState->nInputOffset + pDecompressState->nCountInput >= pState->nInputBufferSize)) {
        pState->pBufferIn = new char[pState->nInputBufferSize];

        bResult = (pState->nInputBufferSize == XBinary::_readDevice(pState->pBufferIn, pState->nInputBufferSize, pDecompressState));

        pState->ibuf = pState->pBufferIn;
        pState->bitlen = pState->nInputBufferSize;
        pState->bitnum = 8;
    } else {
        bResult = false;
    }

    return bResult;
}
