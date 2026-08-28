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
#include "algo_utils.h"
#include "xalgo_local.h"

#include <QCryptographicHash>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <new>

namespace {
const qint32 N_ALGO_UTILS_BUFFER_SIZE = 65536;
ISzAlloc g_lzmaAlloc = {Algo_utils::szAlloc, Algo_utils::szFree};
ISzAlloc g_ppmdAlloc = {Algo_utils::szAlloc, Algo_utils::szFree};

void rewindUnusedInput(XBinary::DATAPROCESS_STATE *pState, qint64 nUnusedSize)
{
    if (!pState || !pState->pDeviceInput || (nUnusedSize <= 0) || (nUnusedSize > pState->nCountInput) || pState->pDeviceInput->isSequential()) {
        return;
    }

    // Look-ahead belongs to the following member/record.  Keep the physical
    // device position and nCountInput consistent when the source is seekable;
    // a sequential source cannot be unread, so its count intentionally remains
    // the number of bytes physically consumed.
    const qint64 nConsumed = pState->nCountInput - nUnusedSize;
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if ((pState->nInputOffset < 0) || (nConsumed < 0) || (pState->nInputOffset > nMax - nConsumed)) return;
    const qint64 nTarget = pState->nInputOffset + nConsumed;
    if (pState->pDeviceInput->seek(nTarget)) {
        pState->nCountInput -= nUnusedSize;
    }
}
}  // namespace

void Algo_utils::seekToStart(XBinary::DATAPROCESS_STATE *pState)
{
    if (pState->pDeviceInput) {
        pState->pDeviceInput->seek(pState->nInputOffset);
    }

    if (pState->pDeviceOutput && !pState->pDeviceOutput->isSequential()) {
        pState->pDeviceOutput->seek(0);
    }
}

void Algo_utils::prepareState(XBinary::DATAPROCESS_STATE *pState)
{
    if (!pState) {
        return;
    }

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;

    seekToStart(pState);

    if (!pState->pDeviceInput || (pState->nInputOffset < 0) || (pState->pDeviceInput->pos() != pState->nInputOffset)) {
        pState->bReadError = true;
    }
    if (!pState->pDeviceOutput || (pState->pDeviceOutput->pos() != 0)) {
        pState->bWriteError = true;
    }
}

qint32 Algo_utils::getReadChunkSize(const XBinary::DATAPROCESS_STATE *pState, qint32 nBufferSize)
{
    if (!pState || (nBufferSize <= 0) || (pState->nCountInput < 0) || (pState->nInputLimit < -1)) {
        return 0;
    }

    if (pState->nInputLimit != -1) {
        if (pState->nCountInput >= pState->nInputLimit) {
            return 0;
        }

        const qint64 nRemaining = pState->nInputLimit - pState->nCountInput;
        return (qint32)(std::min)(nRemaining, (qint64)nBufferSize);
    }

    return nBufferSize;
}

int Algo_utils::ascii85ReadByte(XBinary::DATAPROCESS_STATE *pState)
{
    if (!pState || !pState->pDeviceInput || (pState->nCountInput < 0) || (pState->nInputLimit < -1) ||
        ((pState->nInputLimit != -1) && (pState->nCountInput >= pState->nInputLimit))) {
        if (pState) pState->bReadError = true;
        return -1;
    }

    char c = 0;
    qint64 nRead = pState->pDeviceInput->read(&c, 1);
    if (nRead != 1) {
        pState->bReadError = true;
        return -1;
    }

    pState->nCountInput++;

    return (unsigned char)c;
}

bool Algo_utils::ascii85WriteBytes(XBinary::DATAPROCESS_STATE *pState, const unsigned char *pBuffer, int nSize)
{
    if (!pState || (nSize < 0) || ((nSize > 0) && !pBuffer)) return false;
    return (nSize == 0) || (XBinary::_writeDevice((char *)pBuffer, nSize, pState) == nSize);
}

void *Algo_utils::szAlloc(ISzAllocPtr pAlloc, size_t nSize)
{
    Q_UNUSED(pAlloc)

    return malloc(nSize);
}

void Algo_utils::szFree(ISzAllocPtr pAlloc, void *pAddress)
{
    Q_UNUSED(pAlloc)

    free(pAddress);
}

ISzAlloc *Algo_utils::lzmaAlloc()
{
    return &g_lzmaAlloc;
}

ISzAlloc *Algo_utils::ppmdAlloc()
{
    return &g_ppmdAlloc;
}

bool Algo_utils::decompressLZMA(CLzmaDec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, qint32 nBufferSize,
                                XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    const qint64 nExpectedOutput = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();
    if ((nBufferSize < 0x1000) || (nBufferSize > 0x100000) || (nExpectedOutput < -1)) {
        return false;
    }

    QByteArray baPending;
    baPending.reserve(nBufferSize);
    QByteArray baOutput(nBufferSize, 0);
    qint64 nTotalOutput = 0;
    bool bInputExhausted = false;
    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        while (!bInputExhausted && (baPending.size() < nBufferSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nRequest = getReadChunkSize(pDecompressState, nBufferSize - baPending.size());
            if (nRequest <= 0) {
                bInputExhausted = true;
                break;
            }

            const qint32 nOldSize = baPending.size();
            baPending.resize(nOldSize + nRequest);
            const qint32 nRead = XBinary::_readDevice(baPending.data() + nOldSize, nRequest, pDecompressState);
            if (nRead <= 0) {
                baPending.resize(nOldSize);
                bInputExhausted = true;
                break;
            }
            baPending.resize(nOldSize + nRead);

            if ((pDecompressState->nInputLimit != -1) && (pDecompressState->nCountInput == pDecompressState->nInputLimit)) {
                bInputExhausted = true;
            }
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        SizeT inProcessed = (SizeT)baPending.size();
        SizeT outProcessed = (SizeT)nBufferSize;
        ELzmaFinishMode finishMode = LZMA_FINISH_ANY;

        if (nExpectedOutput >= 0) {
            const qint64 nRemainingOutput = nExpectedOutput - nTotalOutput;
            if (nRemainingOutput < 0) {
                return false;
            }
            if (outProcessed >= (SizeT)nRemainingOutput) {
                outProcessed = (SizeT)nRemainingOutput;
                finishMode = LZMA_FINISH_END;
            }
        }

        ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
        const SRes ret = X_LzmaDec_DecodeToBuf(pState, (Byte *)baOutput.data(), &outProcessed, (const Byte *)baPending.constData(), &inProcessed,
                                               finishMode, &status);
        if ((ret != 0) || (inProcessed > (SizeT)baPending.size()) || (outProcessed > (SizeT)nBufferSize)) {
            return false;
        }

        if (inProcessed > 0) {
            baPending.remove(0, (qint32)inProcessed);
        }
        if (outProcessed > 0) {
            if (XBinary::_writeDevice(baOutput.constData(), (qint32)outProcessed, pDecompressState) != (qint32)outProcessed) {
                return false;
            }
            nTotalOutput += (qint64)outProcessed;
        }

        lastStatus = status;
        const bool bBoundedInputConsumed = (pDecompressState->nInputLimit != -1) && bInputExhausted && baPending.isEmpty();
        const bool bFinished = (status == LZMA_STATUS_FINISHED_WITH_MARK) ||
                               ((status == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK) && (nExpectedOutput >= 0) &&
                                (nTotalOutput == nExpectedOutput) && bBoundedInputConsumed);
        if (bFinished) {
            rewindUnusedInput(pDecompressState, baPending.size());
            baPending.clear();
            break;
        }

        if ((inProcessed == 0) && (outProcessed == 0)) {
            // The decoder needs more input. Preserve the pending prefix and
            // refill it; a full buffer or actual EOF with no progress is a
            // malformed/stalled stream.
            if (bInputExhausted || (baPending.size() >= nBufferSize)) {
                return false;
            }
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || pDecompressState->bReadError || pDecompressState->bWriteError) {
        return false;
    }

    if (nExpectedOutput >= 0) {
        return (nTotalOutput == nExpectedOutput) &&
               ((lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) || (lastStatus == LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK));
    }

    return lastStatus == LZMA_STATUS_FINISHED_WITH_MARK;
}

bool Algo_utils::decompressLZMA2(CLzma2Dec *pState, XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    const qint32 nBufferSize = XBinary::getBufferSize(pPdStruct);
    const qint64 nExpectedOutput = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();
    if ((nBufferSize <= 0) || (nExpectedOutput < -1)) {
        return false;
    }

    QByteArray baPending;
    baPending.reserve(nBufferSize);
    QByteArray baOutput(nBufferSize, 0);
    qint64 nTotalOutput = 0;
    bool bInputExhausted = false;
    ELzmaStatus lastStatus = LZMA_STATUS_NOT_FINISHED;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        while (!bInputExhausted && (baPending.size() < nBufferSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint32 nRequest = getReadChunkSize(pDecompressState, nBufferSize - baPending.size());
            if (nRequest <= 0) {
                bInputExhausted = true;
                break;
            }

            const qint32 nOldSize = baPending.size();
            baPending.resize(nOldSize + nRequest);
            const qint32 nRead = XBinary::_readDevice(baPending.data() + nOldSize, nRequest, pDecompressState);
            if (nRead <= 0) {
                baPending.resize(nOldSize);
                bInputExhausted = true;
                break;
            }
            baPending.resize(nOldSize + nRead);

            if ((pDecompressState->nInputLimit != -1) && (pDecompressState->nCountInput == pDecompressState->nInputLimit)) {
                bInputExhausted = true;
            }
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        SizeT inProcessed = (SizeT)baPending.size();
        SizeT outProcessed = (SizeT)nBufferSize;
        ELzmaFinishMode finishMode = LZMA_FINISH_ANY;
        if (nExpectedOutput >= 0) {
            const qint64 nRemainingOutput = nExpectedOutput - nTotalOutput;
            if (nRemainingOutput < 0) {
                return false;
            }
            if (outProcessed >= (SizeT)nRemainingOutput) {
                outProcessed = (SizeT)nRemainingOutput;
                finishMode = LZMA_FINISH_END;
            }
        }

        ELzmaStatus status = LZMA_STATUS_NOT_SPECIFIED;
        const SRes ret = X_Lzma2Dec_DecodeToBuf(pState, (Byte *)baOutput.data(), &outProcessed, (const Byte *)baPending.constData(), &inProcessed,
                                                finishMode, &status);
        if ((ret != 0) || (inProcessed > (SizeT)baPending.size()) || (outProcessed > (SizeT)nBufferSize)) {
            return false;
        }

        if (inProcessed > 0) {
            baPending.remove(0, (qint32)inProcessed);
        }
        if (outProcessed > 0) {
            if (XBinary::_writeDevice(baOutput.constData(), (qint32)outProcessed, pDecompressState) != (qint32)outProcessed) {
                return false;
            }
            nTotalOutput += (qint64)outProcessed;
        }

        lastStatus = status;
        if (status == LZMA_STATUS_FINISHED_WITH_MARK) {
            rewindUnusedInput(pDecompressState, baPending.size());
            baPending.clear();
            break;
        }

        if ((inProcessed == 0) && (outProcessed == 0) && (bInputExhausted || (baPending.size() >= nBufferSize))) {
            return false;
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct) && !pDecompressState->bReadError && !pDecompressState->bWriteError &&
           (lastStatus == LZMA_STATUS_FINISHED_WITH_MARK) && ((nExpectedOutput == -1) || (nTotalOutput == nExpectedOutput));
}

bool Algo_utils::xzReadVarInt(const QByteArray &baData, qint32 &nPos, quint64 &nValue)
{
    nValue = 0;
    if ((nPos < 0) || (nPos > baData.size())) {
        return false;
    }

    for (qint32 nByteIndex = 0; nByteIndex < 9; nByteIndex++) {
        if (nPos >= baData.size()) {
            return false;
        }
        quint8 nByte = (quint8)baData.at(nPos++);
        const quint8 nPayload = nByte & 0x7F;
        nValue |= ((quint64)nPayload) << (nByteIndex * 7);
        if (!(nByte & 0x80)) {
            // XZ VLIs must use their shortest encoding.
            if ((nByteIndex > 0) && (nPayload == 0)) {
                return false;
            }
            return true;
        }
    }

    return false;
}

static bool bcjTest86MSByte(quint8 b)
{
    return (((quint32)b + 1) & 0xFE) == 0;  // b == 0x00 || b == 0xFF
}

void Algo_utils::applyBCJX86Decode(QByteArray &baData, quint32 nIp)
{
    // 7-Zip x86 BCJ inverse filter. Byte-exact port of the reference x86_Convert
    // (LZMA SDK Bra86.c) with encoding=0, processing the whole buffer in a single
    // pass starting from instruction pointer nIp (0 for the standard 7z/xz filter).
    //
    // The previous implementation was a naive E8/E9 scan that ignored the "mask"
    // state machine the reference uses for call/jump opcodes closer than 5 bytes
    // apart, and wrote the raw high operand byte instead of its sign-extension.
    // That diverged from 7-Zip on real x86 executables (e.g. large PE files).
    const qint64 nSize = baData.size();

    if (nSize < 5) {
        return;
    }

    unsigned char *data = reinterpret_cast<unsigned char *>(baData.data());
    const qint64 nLimit = nSize - 4;
    const quint32 ip = nIp + 5;
    qint64 pos = 0;
    quint32 mask = 0;  // state

    for (;;) {
        unsigned char *p = data + pos;
        const unsigned char *end = data + nLimit;
        for (; p < end; p++) {
            if ((*p & 0xFE) == 0xE8) {  // E8 (CALL) or E9 (JMP)
                break;
            }
        }

        {
            const qint64 d = (qint64)(p - data) - pos;
            pos = (qint64)(p - data);
            if (p >= end) {
                return;
            }
            if (d > 2) {
                mask = 0;
            } else {
                mask >>= (unsigned)d;
                if (mask != 0 && (mask > 4 || mask == 3 || bcjTest86MSByte(p[(mask >> 1) + 1]))) {
                    mask = (mask >> 1) | 4;
                    pos++;
                    continue;
                }
            }
        }

        if (bcjTest86MSByte(p[4])) {
            quint32 v = (quint32)p[1] | ((quint32)p[2] << 8) | ((quint32)p[3] << 16) | ((quint32)p[4] << 24);
            const quint32 cur = ip + (quint32)pos;
            pos += 5;
            v -= cur;  // decode: absolute -> relative
            if (mask != 0) {
                const unsigned sh = (mask & 6) << 2;
                if (bcjTest86MSByte((quint8)(v >> sh))) {
                    v ^= (((quint32)0x100 << sh) - 1);
                    v -= cur;
                }
                mask = 0;
            }
            p[1] = (unsigned char)v;
            p[2] = (unsigned char)(v >> 8);
            p[3] = (unsigned char)(v >> 16);
            p[4] = (unsigned char)(0 - ((v >> 24) & 1));  // sign-extend into the MS byte
        } else {
            mask = (mask >> 1) | 4;
            pos++;
        }
    }
}

unsigned Algo_utils::deflate64ReadFunc(void *pInDesc, unsigned char **ppBuffer)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pInDesc;

    *ppBuffer = (unsigned char *)(pDecompressState->pInputBuffer);

    qint32 nRead = XBinary::_readDevice(pDecompressState);
    if (nRead < 0) {
        pDecompressState->bReadError = true;
        return 0;
    }

    return (unsigned)nRead;
}

int Algo_utils::deflate64WriteFunc(void *pOutDesc, unsigned char *pBuffer, unsigned nSize)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pOutDesc;

    if (!pDecompressState || (nSize > (unsigned)(std::numeric_limits<qint32>::max)())) {
        if (pDecompressState) pDecompressState->bWriteError = true;
        return 1;
    }

    return (XBinary::_writeDevice((char *)pBuffer, (qint32)nSize, pDecompressState) == (qint32)nSize) &&
                   !pDecompressState->bWriteError
               ? 0
               : 1;
}

bool Algo_utils::compressDeflate(XBinary::DATAPROCESS_STATE *pCompressState, XBinary::PDSTRUCT *pPdStruct, int nCompressionLevel, int nWindowBits)
{
    if (!pCompressState || !pCompressState->pDeviceInput || !pCompressState->pDeviceOutput ||
        (pCompressState->nInputOffset < 0) || (pCompressState->nInputLimit < -1)) {
        return false;
    }

    pCompressState->bReadError = false;
    pCompressState->bWriteError = false;
    pCompressState->nCountInput = 0;
    pCompressState->nCountOutput = 0;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (!pCompressState->pDeviceInput->seek(pCompressState->nInputOffset) &&
        (pCompressState->pDeviceInput->pos() != pCompressState->nInputOffset)) {
        pCompressState->bReadError = true;
        return false;
    }

    // Compression appends at the destination's current position.  Archive
    // writers use this to place a raw DEFLATE stream after their own header.
    if (pCompressState->pDeviceOutput->pos() < 0) {
        pCompressState->bWriteError = true;
        return false;
    }

    char *pInputBuffer = new (std::nothrow) char[N_ALGO_UTILS_BUFFER_SIZE];
    if (!pInputBuffer) {
        return false;
    }

    char *pOutputBuffer = new (std::nothrow) char[N_ALGO_UTILS_BUFFER_SIZE];
    if (!pOutputBuffer) {
        delete[] pInputBuffer;
        return false;
    }

    z_stream stream = {};
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    int ret = X_deflateInit2(&stream, nCompressionLevel, Z_DEFLATED, nWindowBits, 8, Z_DEFAULT_STRATEGY);
    bool bInputFinished = false;

    if (ret == Z_OK) {
        do {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;

            const qint32 nToRead = getReadChunkSize(pCompressState, N_ALGO_UTILS_BUFFER_SIZE);
            qint32 nRead = 0;
            if (nToRead > 0) {
                nRead = XBinary::_readDevice(pInputBuffer, nToRead, pCompressState);
                if (nRead < 0) break;
            } else if ((pCompressState->nInputLimit != -1) &&
                       (pCompressState->nCountInput != pCompressState->nInputLimit)) {
                pCompressState->bReadError = true;
                break;
            }

            if (nRead == 0) {
                if ((pCompressState->nInputLimit == -1) && !pCompressState->pDeviceInput->atEnd()) {
                    pCompressState->bReadError = true;
                    break;
                }
                bInputFinished = true;
            }

            stream.avail_in = static_cast<uInt>(nRead);
            stream.next_in = reinterpret_cast<Bytef *>(pInputBuffer);
            const int nFlush = bInputFinished ? Z_FINISH : Z_NO_FLUSH;

            do {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;
                stream.avail_out = N_ALGO_UTILS_BUFFER_SIZE;
                stream.next_out = reinterpret_cast<Bytef *>(pOutputBuffer);
                ret = X_deflate(&stream, nFlush);
                if ((ret != Z_OK) && (ret != Z_STREAM_END)) break;

                const qint32 nCompressed = N_ALGO_UTILS_BUFFER_SIZE - static_cast<qint32>(stream.avail_out);
                qint64 nWrittenTotal = 0;
                while ((nWrittenTotal < nCompressed) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    const qint64 nWritten = pCompressState->pDeviceOutput->write(pOutputBuffer + nWrittenTotal,
                                                                                nCompressed - nWrittenTotal);
                    if ((nWritten <= 0) || (nWritten > (nCompressed - nWrittenTotal)) ||
                        (pCompressState->nCountOutput >
                         ((std::numeric_limits<qint64>::max)() - nWritten))) {
                        pCompressState->bWriteError = true;
                        break;
                    }
                    nWrittenTotal += nWritten;
                    pCompressState->nCountOutput += nWritten;
                }
                if (pCompressState->bWriteError || (nWrittenTotal != nCompressed)) break;
            } while ((stream.avail_out == 0) && (ret != Z_STREAM_END));

            if (pCompressState->bReadError || pCompressState->bWriteError ||
                ((ret != Z_OK) && (ret != Z_STREAM_END))) {
                break;
            }
        } while (ret != Z_STREAM_END);

        X_deflateEnd(&stream);
    }

    delete[] pInputBuffer;
    delete[] pOutputBuffer;

    return (ret == Z_STREAM_END) && bInputFinished && !pCompressState->bReadError && !pCompressState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct) &&
           ((pCompressState->nInputLimit == -1) || (pCompressState->nCountInput == pCompressState->nInputLimit));
}

bool Algo_utils::getUclMethodFromState(const XBinary::DATAPROCESS_STATE *pDecompressState, XUCLDecoder::METHOD *pMethod)
{
    QVariant vMethod = pDecompressState->mapProperties.value(XBinary::FPART_PROP_TYPE);

    if (!vMethod.isValid()) {
        vMethod = pDecompressState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES);
    }

    bool bIsValid = false;
    qint32 nMethod = vMethod.toInt(&bIsValid);

    if ((!bIsValid) || (nMethod < XUCLDecoder::METHOD_NRV2B_8) || (nMethod > XUCLDecoder::METHOD_NRV2E_LE32)) {
        return false;
    }

    *pMethod = (XUCLDecoder::METHOD)nMethod;

    return true;
}

bool Algo_utils::readInputData(XBinary::DATAPROCESS_STATE *pDecompressState, QByteArray *pbaInput, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pDecompressState && pDecompressState->pDeviceInput && pbaInput) {
        qint32 nChunkSize = XBinary::getBufferSize(pPdStruct);
        qint64 nRemaining = pDecompressState->nInputLimit;

        pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);

        pbaInput->clear();

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint32 nReadSize = nChunkSize;

            if (nRemaining != -1) {
                if (nRemaining <= 0) {
                    bResult = true;
                    break;
                }

                nReadSize = (qint32)(std::min)(nRemaining, (qint64)nChunkSize);
            }

            QByteArray baChunk;
            baChunk.resize(nReadSize);

            qint64 nRead = pDecompressState->pDeviceInput->read(baChunk.data(), nReadSize);

            if (nRead < 0) {
                pDecompressState->bReadError = true;
                break;
            }

            baChunk.resize((qint32)nRead);

            if (baChunk.isEmpty()) {
                bResult = (nRemaining == -1) || (nRemaining == 0);

                if (!bResult) {
                    pDecompressState->bReadError = true;
                }

                break;
            }

            pbaInput->append(baChunk);
            pDecompressState->nCountInput += baChunk.size();

            if (nRemaining != -1) {
                nRemaining -= baChunk.size();

                if (nRemaining == 0) {
                    bResult = true;
                    break;
                }
            }
        }
    }

    return bResult;
}

QByteArray Algo_utils::hmacSha1(const QByteArray &baKey, const QByteArray &baMessage)
{
    const qint32 BLOCK_SIZE = 64;
    const quint8 IPAD = 0x36;
    const quint8 OPAD = 0x5c;

    QByteArray baKeyPadded;
    if (baKey.size() > BLOCK_SIZE) {
        baKeyPadded = QCryptographicHash::hash(baKey, QCryptographicHash::Sha1);
    } else {
        baKeyPadded = baKey;
    }
    while (baKeyPadded.size() < BLOCK_SIZE) {
        baKeyPadded.append((char)0);
    }

    QByteArray baInnerKey(BLOCK_SIZE, 0);
    QByteArray baOuterKey(BLOCK_SIZE, 0);
    for (qint32 i = 0; i < BLOCK_SIZE; i++) {
        baInnerKey[i] = baKeyPadded[i] ^ IPAD;
        baOuterKey[i] = baKeyPadded[i] ^ OPAD;
    }

    QCryptographicHash innerHash(QCryptographicHash::Sha1);
    innerHash.addData(baInnerKey);
    innerHash.addData(baMessage);
    QByteArray baInnerResult = innerHash.result();

    QCryptographicHash outerHash(QCryptographicHash::Sha1);
    outerHash.addData(baOuterKey);
    outerHash.addData(baInnerResult);

    return outerHash.result();
}

Byte Algo_utils::readFromQIODeviceStream(const IByteIn *pStream)
{
    QIODeviceByteInStream *pStreamEx = Z7_CONTAINER_FROM_VTBL(pStream, QIODeviceByteInStream, vt);

    if (pStreamEx->bError || !pStreamEx->pDevice || ((pStreamEx->nLimit >= 0) && (pStreamEx->nBytesRead >= pStreamEx->nLimit))) {
        pStreamEx->bError = true;
        return 0;
    }

    char c = 0;
    qint64 nRead = pStreamEx->pDevice->read(&c, 1);

    if (nRead != 1) {
        pStreamEx->bError = true;
        return 0;
    }

    pStreamEx->nBytesRead++;
    return (Byte)c;
}

size_t Algo_utils::readFromState(void *pState, void *pBuffer, size_t nSize)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pState;

    qint32 nRead = XBinary::_readDevice((char *)pBuffer, (qint32)nSize, pDecompressState);
    return nRead > 0 ? (size_t)nRead : 0;
}

size_t Algo_utils::writeToState(void *pState, const void *pBuffer, size_t nSize)
{
    XBinary::DATAPROCESS_STATE *pDecompressState = (XBinary::DATAPROCESS_STATE *)pState;

    qint32 nWritten = XBinary::_writeDevice((char *)pBuffer, (qint32)nSize, pDecompressState);
    return (!pDecompressState->bWriteError && (nWritten > 0)) ? (size_t)nWritten : 0;
}
