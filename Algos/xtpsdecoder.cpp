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
#include "xtpsdecoder.h"

#include "xlzhufdecoder.h"

qint64 XTPSDecoder::storedSize(qint64 nCodedSize)
{
    if (nCodedSize < 0) return -1;
    const qint64 nBlocks = (nCodedSize + 4 + (BLOCK_SIZE - 1)) / BLOCK_SIZE;

    return nCodedSize + nBlocks + 1;
}

bool XTPSDecoder::dechunk(const QByteArray &baStored, quint32 nUncompressedSize, QByteArray *pbaResult)
{
    if (!pbaResult) return false;
    pbaResult->clear();

    const qint64 nSize = baStored.size();
    const quint8 *pData = (const quint8 *)baStored.constData();
    if ((nSize < 2) || !pData) return false;

    QByteArray baOut;
    qint64 nPosition = 0;
    qint64 nInBlock = 4;  // the size dword at directory offset +0x12
    quint8 nCheck = 0;
    for (qint32 nShift = 0; nShift < 32; nShift += 8) nCheck = (quint8)(nCheck + (quint8)((nUncompressedSize >> nShift) & 0xff));

    // The coded byte count is what is left once the check bytes and the end
    // marker are taken away.  With m = nCoded + 4 and q = ceil(m / 0x4000)
    // blocks, nSize = m - 3 + q, so solve for the q that is self consistent.
    qint64 nCoded = -1;
    const qint64 nMaxBlocks = ((nSize + 3) / BLOCK_SIZE) + 2;
    for (qint64 q = 1; q <= nMaxBlocks; ++q) {
        const qint64 m = nSize + 3 - q;
        if (m < 4) break;
        if (((m + BLOCK_SIZE - 1) / BLOCK_SIZE) == q) {
            nCoded = m - 4;
            break;
        }
    }
    if ((nCoded < 0) || (storedSize(nCoded) != nSize)) return false;

    qint64 nLeft = nCoded;
    while (true) {
        while ((nLeft > 0) && (nInBlock != BLOCK_SIZE)) {
            if (nPosition >= nSize) return false;
            const quint8 nByte = (quint8)(pData[nPosition] ^ 0x80);
            ++nPosition;
            baOut.append((char)nByte);
            nCheck = (quint8)(nCheck + nByte);
            ++nInBlock;
            --nLeft;
        }
        if (nPosition >= nSize) return false;
        if (pData[nPosition] != nCheck) return false;
        ++nPosition;
        if (nLeft == 0) {
            if (nPosition >= nSize) return false;
            if (pData[nPosition] != 1) return false;
            ++nPosition;
            break;
        }
        nInBlock = 0;
        nCheck = 0;
    }

    if (nPosition != nSize) return false;
    *pbaResult = baOut;

    return true;
}

bool XTPSDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;

    QByteArray baCoded;
    if (!dechunk(baPacked, (quint32)nUncompressedSize, &baCoded)) return false;

    const XLZHUFDecoder::OPTIONS options = XLZHUFDecoder::getOptions(1, 1, 0, false, false, false);

    return XLZHUFDecoder::decode(baCoded, options, nUncompressedSize, pbaResult, pPdStruct);
}
