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
#include "xztcdecoder.h"

#include "xlzhufdecoder.h"

#include <QtEndian>

namespace {
const qint64 N_ZTC_PAGE_SIZE = XZTCDecoder::PAGE_SIZE;
const qint64 N_ZTC_PAGE_CHECK_SIZE = XZTCDecoder::PAGE_CHECK_SIZE;
const qint64 N_ZTC_MAX_OUTPUT = 0x7fffffff;
}  // namespace

bool XZTCDecoder::depagePayload(const QByteArray &baPaged, QByteArray *pbaResult, bool bVerifySums)
{
    if (!pbaResult) return false;
    pbaResult->clear();

    const qint64 nSize = baPaged.size();
    const uchar *pData = (const uchar *)baPaged.constData();
    QByteArray baResult;
    qint64 nRemaining = nSize;
    qint64 nPosition = 0;

    while (nRemaining >= N_ZTC_PAGE_CHECK_SIZE) {
        // The check field is charged to the budget BEFORE the page length is
        // chosen, which is what makes the last page short by exactly four.
        nRemaining -= N_ZTC_PAGE_CHECK_SIZE;
        const qint64 nPageSize = (nRemaining >= N_ZTC_PAGE_SIZE) ? N_ZTC_PAGE_SIZE : nRemaining;
        if ((nPageSize < 0) || ((nPosition + nPageSize + N_ZTC_PAGE_CHECK_SIZE) > nSize)) break;

        quint32 nSum = 0;
        for (qint64 i = 0; i < nPageSize; ++i) {
            nSum += (quint32)pData[nPosition + i];
        }

        const quint32 nStored = qFromLittleEndian<quint32>(pData + nPosition + nPageSize);
        if (bVerifySums && (nStored != nSum)) return false;

        baResult.append(baPaged.constData() + nPosition, (qint32)nPageSize);
        nPosition += nPageSize + N_ZTC_PAGE_CHECK_SIZE;
        nRemaining -= nPageSize;
    }

    *pbaResult = baResult;

    return true;
}

bool XZTCDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > N_ZTC_MAX_OUTPUT)) return false;

    QByteArray baStream;
    if (!depagePayload(baPacked, &baStream, true)) return false;

    if (!XLZHUFDecoder::decode(baStream, XLZHUFDecoder::getZTCOptions(), nUncompressedSize, pbaResult, pPdStruct)) {
        pbaResult->clear();
        return false;
    }

    return (qint64)pbaResult->size() == nUncompressedSize;
}
