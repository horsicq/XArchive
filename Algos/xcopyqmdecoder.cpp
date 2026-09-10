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
#include "xcopyqmdecoder.h"

namespace {
qint32 readToken(const quint8 *pData, qint64 nOffset)
{
    return (qint32)(qint16)((quint16)pData[nOffset] | ((quint16)pData[nOffset + 1] << 8));
}
}  // namespace

bool XCopyQMDecoder::measure(const QByteArray &baPacked, qint64 *pnUncompressedSize)
{
    if (!pnUncompressedSize) return false;
    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nSize = baPacked.size();
    qint64 nOffset = 0;
    qint64 nTotal = 0;

    while ((nOffset + 2) <= nSize) {
        const qint32 nToken = readToken(pData, nOffset);
        nOffset += 2;
        if (nToken == 0) break;
        if (nToken < 0) {
            if (nOffset >= nSize) break;
            ++nOffset;
            nTotal += -(qint64)nToken;
        } else {
            const qint64 nRun = qMin((qint64)nToken, nSize - nOffset);
            nOffset += nRun;
            nTotal += nRun;
            if (nRun < nToken) break;
        }
        if (nTotal > 0x7fffffff) return false;
    }

    *pnUncompressedSize = nTotal;

    return nTotal > 0;
}

bool XCopyQMDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nSize = baPacked.size();
    qint64 nOffset = 0;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    while ((nOffset + 2) <= nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nToken = readToken(pData, nOffset);
        nOffset += 2;
        if (nToken == 0) break;
        if (nToken < 0) {
            if (nOffset >= nSize) break;
            const char nByte = (char)pData[nOffset];
            ++nOffset;
            baOut.append(-nToken, nByte);
        } else {
            const qint64 nRun = qMin((qint64)nToken, nSize - nOffset);
            baOut.append((const char *)(pData + nOffset), (qint32)nRun);
            nOffset += nRun;
            if (nRun < nToken) break;
        }
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
