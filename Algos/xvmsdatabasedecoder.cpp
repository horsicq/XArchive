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
#include "xvmsdatabasedecoder.h"

#include <cstring>
#include <limits>

namespace {
const quint8 VMSDB_TAG_EOC = 0x00;
const quint8 VMSDB_TAG_OCTETSTRING = 0x04;
const quint8 VMSDB_FORM_INDEFINITE = 0x80;
const quint8 VMSDB_FORM_ONE_BYTE = 0x81;
const quint8 VMSDB_FORM_TWO_BYTES = 0x82;
}  // namespace

bool XVMSDataBaseDecoder::parseTag(const quint8 *pData, qint64 nAvailable, TAG *pTag)
{
    if (!pData || !pTag || (nAvailable < 2)) return false;

    TAG tag = {};
    tag.nTag = pData[0];
    const quint8 nForm = pData[1];

    if (nForm == VMSDB_FORM_INDEFINITE) {
        tag.bConstructed = true;
        tag.nLength = 0;
        tag.nHeaderSize = 2;
    } else if (nForm == VMSDB_FORM_ONE_BYTE) {
        if (nAvailable < 3) return false;
        const qint64 nLength = pData[2];
        // A one-byte long form below 0x80 would have been written as the short
        // form; refusing it is what keeps arbitrary data from parsing.
        if (nLength < 0x80) return false;
        tag.nLength = nLength;
        tag.nHeaderSize = 3;
    } else if (nForm == VMSDB_FORM_TWO_BYTES) {
        if (nAvailable < 4) return false;
        const qint64 nLength = ((qint64)pData[2] << 8) | (qint64)pData[3];
        if (nLength < 0x100) return false;
        tag.nLength = nLength;
        tag.nHeaderSize = 4;
    } else if (nForm >= 0x80) {
        return false;
    } else {
        tag.nLength = nForm;
        tag.nHeaderSize = 2;
    }

    *pTag = tag;

    return true;
}

bool XVMSDataBaseDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > (qint64)(std::numeric_limits<qint32>::max)())) return false;
    if (nUncompressedSize == 0) return true;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nSize = baPacked.size();
    qint64 nPosition = 0;

    QByteArray baOut((qint32)nUncompressedSize, (char)0);
    if (baOut.size() != (qint32)nUncompressedSize) return false;
    quint8 *pOut = (quint8 *)baOut.data();
    qint64 nProduced = 0;

    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        TAG tag = {};
        if (!parseTag(pData + nPosition, nSize - nPosition, &tag)) return false;
        nPosition += tag.nHeaderSize;
        if (tag.nTag == VMSDB_TAG_EOC) break;
        // A constructed element here reports a length of zero, which this test
        // rejects along with a genuinely empty chunk.
        if ((tag.nTag != VMSDB_TAG_OCTETSTRING) || (tag.nLength == 0)) return false;
        if (tag.nLength > (nSize - nPosition)) return false;

        if (nProduced < nUncompressedSize) {
            qint64 nPortion = nUncompressedSize - nProduced;
            if (nPortion > tag.nLength) nPortion = tag.nLength;
            memcpy(pOut + nProduced, pData + nPosition, (size_t)nPortion);
            nProduced += nPortion;
        }
        nPosition += tag.nLength;
    }

    if (nProduced != nUncompressedSize) return false;

    *pbaResult = baOut;

    return true;
}
