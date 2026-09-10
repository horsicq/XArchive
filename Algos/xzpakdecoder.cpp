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
#include "xzpakdecoder.h"

#include "xsharedlzwdecoder.h"

#include <QtEndian>

namespace {
const quint8 N_ZPAK_CHUNK_MARKER = (quint8)XZPAKDecoder::CHUNK_MARKER;
const qint64 N_ZPAK_MAX_OUTPUT = 0x7fffffff;
}  // namespace

bool XZPAKDecoder::unchunk(const QByteArray &baPayload, QByteArray *pbaResult)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baPayload.size() < 2) return false;
    if ((quint8)baPayload.at(0) != N_ZPAK_CHUNK_MARKER) return false;
    if (baPayload.at(baPayload.size() - 1) != (char)0) return false;

    const uchar *pData = (const uchar *)baPayload.constData() + 1;
    const qint64 nSize = baPayload.size() - 2;
    QByteArray baResult;
    qint64 nPosition = 0;

    while ((nPosition + 2) <= nSize) {
        const qint64 nChunkSize = (qint64)qFromLittleEndian<quint16>(pData + nPosition);
        nPosition += 2;
        // A zero length terminates the stream early. A length that runs past
        // the framed area means a truncated archive: take what is really there
        // and stop, which is what the reference reader's slice does.
        if (nChunkSize == 0) break;
        const qint64 nAvailable = qMin(nChunkSize, nSize - nPosition);
        baResult.append((const char *)(pData + nPosition), (qint32)nAvailable);
        nPosition += nChunkSize;
    }

    if (baResult.isEmpty()) return false;
    *pbaResult = baResult;

    return true;
}

bool XZPAKDecoder::decodeLZW(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > N_ZPAK_MAX_OUTPUT)) return false;

    QByteArray baStream;
    if (!unchunk(baPacked, &baStream)) return false;

    // GIF-style, not the ULEAD dialect: see TRAP 2 in the header.
    XSharedLZWDecoder::OPTIONS options;
    options.nMaxBits = 12;
    options.bHasClearCode = true;
    options.bHasEndCode = true;
    options.bMsbFirst = false;
    options.bUnRle90 = false;
    options.bBlockPadding = false;
    options.nWidthStepBias = 0;

    if (!XSharedLZWDecoder::decode(baStream, options, nUncompressedSize, pbaResult, pPdStruct)) {
        pbaResult->clear();
        return false;
    }

    return (qint64)pbaResult->size() == nUncompressedSize;
}
