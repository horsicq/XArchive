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
#include "xdiskimagedecoder.h"

#include "include/zlib.h"

#include <QtEndian>

namespace {
const qint64 APRICOT_PREAMBLE = 0x80;
const qint64 APRICOT_CHUNK_HEADER = 16;
const qint16 APRICOT_TAG = (qint16)0xe31d;
const qint16 APRICOT_LITERAL = (qint16)0x9e90;
const qint16 APRICOT_RUN = (qint16)0x3e5a;
const qint32 APRICOT_MAX_CHUNK = 512;

const qint64 CISO_HEADER = 24;
const qint64 CLOOP_HEADER = 0x80;

// One pass over the Apricot chunk chain.  pbaOut may be null, in which case
// nothing is produced and only the length is accumulated - that is how the
// reader learns the size it has to declare.
bool walkApricot(const QByteArray &baPacked, QByteArray *pbaOut, qint64 *pnTotal, XBinary::PDSTRUCT *pPdStruct)
{
    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nSize = baPacked.size();
    qint64 nOffset = APRICOT_PREAMBLE;
    qint64 nTotal = 0;

    while ((nOffset + APRICOT_CHUNK_HEADER) <= nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const quint16 nKind = qFromLittleEndian<quint16>(pData + nOffset);
        const qint16 nTag = (qint16)qFromLittleEndian<quint16>(pData + nOffset + 2);
        const qint16 nSubType = (qint16)qFromLittleEndian<quint16>(pData + nOffset + 4);
        const quint16 nHeaderLength = qFromLittleEndian<quint16>(pData + nOffset + 6);
        const qint32 nDataLength = (qint32)qFromLittleEndian<quint32>(pData + nOffset + 8);

        if ((nTag != APRICOT_TAG) || (nKind > 3) || (nHeaderLength < APRICOT_CHUNK_HEADER) || (nDataLength < 0)) return false;

        nOffset += APRICOT_CHUNK_HEADER;
        if (nHeaderLength > APRICOT_CHUNK_HEADER) nOffset += (nHeaderLength - APRICOT_CHUNK_HEADER);
        if (nOffset > nSize) return false;

        if (nKind != 1) {
            nOffset += nDataLength;
            continue;
        }

        if (nSubType == APRICOT_LITERAL) {
            if ((nDataLength > APRICOT_MAX_CHUNK) || ((nOffset + nDataLength) > nSize)) return false;
            if (pbaOut) pbaOut->append((const char *)(pData + nOffset), nDataLength);
            nTotal += nDataLength;
            nOffset += nDataLength;
        } else if (nSubType == APRICOT_RUN) {
            if ((nDataLength != 3) || ((nOffset + 3) > nSize)) return false;
            const qint32 nCount = (qint32)qFromLittleEndian<quint16>(pData + nOffset);
            const char nByte = (char)pData[nOffset + 2];
            if (nCount > APRICOT_MAX_CHUNK) return false;
            if (pbaOut) pbaOut->append(nCount, nByte);
            nTotal += nCount;
            nOffset += 3;
        } else {
            return false;
        }
        if (nTotal > 0x7fffffff) return false;
    }

    if (pnTotal) *pnTotal = nTotal;

    return nTotal > 0;
}

// Raw deflate (CISO) or a zlib stream (cloop) into a fixed-size block.
bool inflateBlock(const char *pData, qint64 nSize, qint32 nWindowBits, QByteArray *pbaOut, qint32 nExpected)
{
    if ((nSize <= 0) || (nSize > 0x7fffffff)) return false;
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit2(&stream, nWindowBits) != Z_OK) return false;
    QByteArray baBlock(nExpected, (char)0);
    stream.next_in = (Bytef *)pData;
    stream.avail_in = (uInt)nSize;
    stream.next_out = (Bytef *)baBlock.data();
    stream.avail_out = (uInt)nExpected;
    const int nStatus = inflate(&stream, Z_FINISH);
    const qint32 nProduced = nExpected - (qint32)stream.avail_out;
    inflateEnd(&stream);
    if ((nStatus != Z_STREAM_END) && (nStatus != Z_OK) && (nStatus != Z_BUF_ERROR)) return false;
    if (nProduced <= 0) return false;
    baBlock.truncate(nProduced);
    pbaOut->append(baBlock);

    return true;
}
}  // namespace

bool XDiskImageDecoder::measureApricot(const QByteArray &baPacked, qint64 *pnUncompressedSize)
{
    return walkApricot(baPacked, nullptr, pnUncompressedSize, nullptr);
}

bool XDiskImageDecoder::decodeApricot(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > 0x7fffffff)) return false;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);
    qint64 nTotal = 0;
    if (!walkApricot(baPacked, &baOut, &nTotal, pPdStruct)) return false;
    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XDiskImageDecoder::decodeCiso(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize <= 0) || (nUncompressedSize > 0x7fffffff)) return false;
    if (baPacked.size() < CISO_HEADER) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    if (memcmp(pData, "CISO", 4) != 0) return false;
    const qint64 nTotal = (qint64)qFromLittleEndian<quint64>(pData + 8);
    const qint64 nBlockSize = (qint32)qFromLittleEndian<quint32>(pData + 16);
    const qint32 nShift = pData[21];
    if ((nBlockSize <= 0) || (nTotal != nUncompressedSize) || ((nTotal % nBlockSize) != 0)) return false;
    if (nShift > 31) return false;

    const qint64 nBlocks = nTotal / nBlockSize;
    if ((nBlocks <= 0) || (nBlocks > 4000000)) return false;
    if ((CISO_HEADER + ((nBlocks + 1) * 4)) > baPacked.size()) return false;

    QByteArray baOut;
    baOut.reserve((qint32)nTotal);

    for (qint64 i = 0; i < nBlocks; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const quint32 nThis = qFromLittleEndian<quint32>(pData + CISO_HEADER + (i * 4));
        const quint32 nNext = qFromLittleEndian<quint32>(pData + CISO_HEADER + ((i + 1) * 4));
        const qint64 nStart = (qint64)(nThis & 0x7fffffffU) << nShift;
        const qint64 nEnd = (qint64)(nNext & 0x7fffffffU) << nShift;
        if ((nStart < 0) || (nEnd < nStart) || (nEnd > baPacked.size())) return false;
        if (nThis & 0x80000000U) {
            const qint64 nRun = qMin(nEnd - nStart, nBlockSize);
            if (nRun <= 0) return false;
            baOut.append(baPacked.constData() + nStart, (qint32)nRun);
        } else {
            // CISO blocks are raw deflate, no zlib wrapper.
            if (!inflateBlock(baPacked.constData() + nStart, nEnd - nStart, -15, &baOut, (qint32)nBlockSize)) return false;
        }
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XDiskImageDecoder::decodeCloop(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize <= 0) || (nUncompressedSize > 0x7fffffff)) return false;
    if (baPacked.size() < (CLOOP_HEADER + 8)) return false;

    const quint8 *pData = (const quint8 *)baPacked.constData();
    const qint64 nBlockSize = (qint32)qFromBigEndian<quint32>(pData + CLOOP_HEADER);
    const qint64 nBlocks = (qint32)qFromBigEndian<quint32>(pData + CLOOP_HEADER + 4);
    if ((nBlockSize <= 0) || (nBlocks <= 0) || (nBlocks > 4000000)) return false;
    if ((nBlockSize * nBlocks) != nUncompressedSize) return false;

    const qint64 nIndex = CLOOP_HEADER + 8;
    if ((nIndex + ((nBlocks + 1) * 8)) > baPacked.size()) return false;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    for (qint64 i = 0; i < nBlocks; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nStart = (qint64)qFromBigEndian<quint64>(pData + nIndex + (i * 8));
        const qint64 nEnd = (qint64)qFromBigEndian<quint64>(pData + nIndex + ((i + 1) * 8));
        if ((nStart < 0) || (nEnd < nStart) || (nEnd > baPacked.size())) return false;
        if (!inflateBlock(baPacked.constData() + nStart, nEnd - nStart, 15, &baOut, (qint32)nBlockSize)) return false;
    }

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
