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
#include "xgeniuslibrarydecoder.h"

#include "xdcldecoder.h"

#include <QtEndian>

namespace {
// quint32 plaintext length + quint32 CRC-32 behind the last block.
const qint64 GPLDEC_TRAILER_SIZE = 8;
// The block length field in front of every block.
const qint64 GPLDEC_BLOCKLEN_SIZE = 4;
// A DCL stream cannot be shorter than its two prelude bytes plus one byte
// holding the start of the end-of-stream code.
const qint64 GPLDEC_MIN_BLOCK_SIZE = 3;
// QByteArray is indexed by int, and the reader will not publish a member larger
// than this either.
const qint64 GPLDEC_MAX_OUTPUT = Q_INT64_C(512) * 1024 * 1024;
}  // namespace

bool XGeniusLibraryDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || (nUncompressedSize < 1) || (nUncompressedSize > GPLDEC_MAX_OUTPUT)) return false;

    const qint64 nPackedSize = baPacked.size();
    if (nPackedSize < (GPLDEC_TRAILER_SIZE + GPLDEC_BLOCKLEN_SIZE + GPLDEC_MIN_BLOCK_SIZE)) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());
    const qint64 nBlocksEnd = nPackedSize - GPLDEC_TRAILER_SIZE;

    QByteArray baOutput;
    baOutput.reserve(qint32(nUncompressedSize));

    qint64 nOffset = 0;

    // Every iteration consumes at least GPLDEC_BLOCKLEN_SIZE + GPLDEC_MIN_BLOCK_SIZE
    // bytes of the member, so the walk always reaches nBlocksEnd; there is no
    // block-count cap because the writer's block size is not part of the
    // format and capping on an assumed one would reject a legal member.
    while (nOffset < nBlocksEnd) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if ((nBlocksEnd - nOffset) < GPLDEC_BLOCKLEN_SIZE) return false;

        const qint64 nBlockSize = qint64(qFromLittleEndian<quint32>(pData + nOffset));
        nOffset += GPLDEC_BLOCKLEN_SIZE;
        if ((nBlockSize < GPLDEC_MIN_BLOCK_SIZE) || (nBlockSize > (nBlocksEnd - nOffset))) return false;

        const qint64 nRemaining = nUncompressedSize - qint64(baOutput.size());
        if (nRemaining < 1) return false;

        // QByteArray::fromRawData keeps the member's buffer; XDclDecoder does
        // not hold on to it past the call.
        const QByteArray baBlock = QByteArray::fromRawData(reinterpret_cast<const char *>(pData + nOffset), qint32(nBlockSize));
        QByteArray baPlain;
        // The declared plaintext that is still outstanding doubles as this
        // block's ceiling, so a block that wants to produce more than the
        // member promises stops right there instead of growing the buffer.
        if (!XDclDecoder::decode(baBlock, &baPlain, nRemaining)) return false;
        if (baPlain.isEmpty()) return false;

        baOutput.append(baPlain);
        nOffset += nBlockSize;
    }

    // The blocks have to end exactly where the trailer begins.
    if (nOffset != nBlocksEnd) return false;
    if (qint64(baOutput.size()) != nUncompressedSize) return false;

    const qint64 nDeclaredSize = qint64(qFromLittleEndian<quint32>(pData + nBlocksEnd));
    const quint32 nStoredCrc = qFromLittleEndian<quint32>(pData + nBlocksEnd + 4);
    if (nDeclaredSize != nUncompressedSize) return false;

    // The stored value is the running EDB88320 accumulator seeded with
    // 0xFFFFFFFF, NOT the complemented result, and XBinary::_getCRC32 is that
    // same unfinalised primitive.
    const quint32 nActualCrc = XBinary::_getCRC32(baOutput, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320());
    if (nActualCrc != nStoredCrc) return false;

    *pbaResult = baOutput;

    return true;
}
