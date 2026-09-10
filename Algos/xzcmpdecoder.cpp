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
#include "xzcmpdecoder.h"

#include "include/zlib.h"

#include <string.h>

namespace {
const qint64 N_ZCMP_MAX_BLOCKS = 4000000;
const qint64 N_ZCMP_MAX_OUTPUT = 0x7fffffff;
const qint32 N_ZCMP_CHUNK_SIZE = XZcmpDecoder::CHUNK_SIZE;
}  // namespace

bool XZcmpDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > N_ZCMP_MAX_OUTPUT)) return false;
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    QByteArray baResult;
    baResult.reserve((qint32)nUncompressedSize);
    QByteArray baBuffer(N_ZCMP_CHUNK_SIZE, (char)0);

    const qint64 nSize = baPacked.size();
    qint64 nPosition = 0;
    qint64 nBlocks = 0;

    while ((baResult.size() < nUncompressedSize) && (nPosition < nSize)) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (++nBlocks > N_ZCMP_MAX_BLOCKS) return false;

        z_stream stream;
        memset(&stream, 0, sizeof(stream));
        if (inflateInit2(&stream, 15) != Z_OK) return false;

        stream.next_in = (Bytef *)(baPacked.constData() + nPosition);
        stream.avail_in = (uInt)qMin((qint64)0x7fffffff, nSize - nPosition);

        bool bBlockDone = false;
        bool bFailed = false;

        while (!bBlockDone && !bFailed) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                bFailed = true;
                break;
            }
            stream.next_out = (Bytef *)baBuffer.data();
            stream.avail_out = (uInt)N_ZCMP_CHUNK_SIZE;
            const int nStatus = inflate(&stream, Z_NO_FLUSH);
            const qint32 nProduced = N_ZCMP_CHUNK_SIZE - (qint32)stream.avail_out;

            if (nProduced > 0) {
                // Never let a stream expand past the size the header declared.
                if ((qint64)nProduced > (nUncompressedSize - baResult.size())) {
                    bFailed = true;
                    break;
                }
                baResult.append(baBuffer.constData(), nProduced);
            }

            if (nStatus == Z_STREAM_END) bBlockDone = true;
            else if (nStatus != Z_OK) bFailed = true;
            else if ((nProduced == 0) && (stream.avail_in == 0)) bFailed = true;
        }

        // total_in is the ONLY boundary marker the format has: the next stream
        // starts at exactly the byte this one stopped on.
        const qint64 nConsumed = (qint64)stream.total_in;
        inflateEnd(&stream);
        if (bFailed || (nConsumed <= 0)) return false;
        nPosition += nConsumed;
    }

    if (baResult.size() != nUncompressedSize) return false;
    *pbaResult = baResult;

    return true;
}
