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
#include "xbrotlidecoder.h"
#include "algo_utils.h"

#include <QByteArray>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>

#include "brotlideclib.cpp"

namespace {
const quint32 X_BROTLI_MT_SKIPPABLE_MAGIC = 0x184D2A50U;
const quint32 X_BROTLI_MT_HEADER_PAYLOAD_SIZE = 8U;
const quint16 X_BROTLI_MT_CODEC_MAGIC = 0x5242U;  // "BR"
const qint32 X_BROTLI_MT_HEADER_SIZE = 16;
const qint32 X_BROTLI_MT_PROBE_SIZE = 272;
const quint64 X_BROTLI_MAX_DECODER_MEMORY =
    Q_UINT64_C(512) * 1024 * 1024;

struct BrotliAllocationBudget {
    quint64 nUsed;
    quint64 nLimit;
    XBinary::UNPACK_MEMORY_RESERVATION reservation;
};

struct alignas(std::max_align_t) BrotliAllocationHeader {
    size_t nSize;
};

bool initBrotliAllocationBudget(
    const XBinary::DATAPROCESS_STATE *pState,
    BrotliAllocationBudget *pBudget)
{
    if (!pState || !pBudget) return false;

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pState->mapUnpackProperties,
                                       &nOutputLimit)) {
        return false;
    }

    pBudget->nUsed = 0;
    pBudget->nLimit = X_BROTLI_MAX_DECODER_MEMORY;
    if (nOutputLimit >= 0) {
        pBudget->nLimit = qMin(
            pBudget->nLimit, (quint64)nOutputLimit);
    }
    return pBudget->reservation.acquire(
        pState->mapUnpackProperties, 0);
}

void *brotliBoundedAlloc(void *pOpaque, size_t nSize)
{
    BrotliAllocationBudget *pBudget =
        static_cast<BrotliAllocationBudget *>(pOpaque);
    if (!pBudget) return nullptr;

    const quint64 nRequested = nSize ? (quint64)nSize : 1;
    if ((nRequested > pBudget->nLimit) ||
        (pBudget->nUsed > pBudget->nLimit - nRequested) ||
        (pBudget->nUsed + nRequested >
         (quint64)(std::numeric_limits<qint64>::max)()) ||
        (nRequested >
         (quint64)(std::numeric_limits<size_t>::max)() -
             sizeof(BrotliAllocationHeader))) {
        return nullptr;
    }

    const quint64 nOldUsed = pBudget->nUsed;
    const quint64 nNewUsed = nOldUsed + nRequested;
    if (!pBudget->reservation.resize((qint64)nNewUsed)) {
        return nullptr;
    }

    BrotliAllocationHeader *pHeader =
        static_cast<BrotliAllocationHeader *>(std::malloc(
            sizeof(BrotliAllocationHeader) + (size_t)nRequested));
    if (!pHeader) {
        pBudget->reservation.resize((qint64)nOldUsed);
        return nullptr;
    }
    pHeader->nSize = (size_t)nRequested;
    pBudget->nUsed = nNewUsed;
    return pHeader + 1;
}

void brotliBoundedFree(void *pOpaque, void *pAddress)
{
    if (!pAddress) return;
    BrotliAllocationBudget *pBudget =
        static_cast<BrotliAllocationBudget *>(pOpaque);
    BrotliAllocationHeader *pHeader =
        static_cast<BrotliAllocationHeader *>(pAddress) - 1;
    if (pBudget && ((quint64)pHeader->nSize <= pBudget->nUsed)) {
        pBudget->nUsed -= (quint64)pHeader->nSize;
        pBudget->reservation.resize((qint64)pBudget->nUsed);
    }
    std::free(pHeader);
}

quint16 readUInt16LE(const char *pData)
{
    const unsigned char *p = reinterpret_cast<const unsigned char *>(pData);
    return static_cast<quint16>(p[0]) | (static_cast<quint16>(p[1]) << 8);
}

quint32 readUInt32LE(const char *pData)
{
    const unsigned char *p = reinterpret_cast<const unsigned char *>(pData);
    return static_cast<quint32>(p[0]) | (static_cast<quint32>(p[1]) << 8) |
           (static_cast<quint32>(p[2]) << 16) | (static_cast<quint32>(p[3]) << 24);
}

bool isMtHeader(const char *pData, qint32 nSize)
{
    return pData && (nSize >= X_BROTLI_MT_HEADER_SIZE) &&
           (readUInt32LE(pData) == X_BROTLI_MT_SKIPPABLE_MAGIC) &&
           (readUInt32LE(pData + 4) == X_BROTLI_MT_HEADER_PAYLOAD_SIZE) &&
           (readUInt16LE(pData + 12) == X_BROTLI_MT_CODEC_MAGIC);
}

class BrotliInputBuffer {
public:
    BrotliInputBuffer(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, qint32 nChunkSize)
        : m_pState(pState), m_pPdStruct(pPdStruct), m_nChunkSize(nChunkSize), m_nPosition(0), m_bAtEnd(false)
    {
        m_baData.reserve(nChunkSize);
    }

    qint32 available() const
    {
        return m_baData.size() - m_nPosition;
    }

    const char *data() const
    {
        return m_baData.constData() + m_nPosition;
    }

    bool atEnd() const
    {
        return m_bAtEnd && (available() == 0);
    }

    void consume(qint32 nSize)
    {
        if ((nSize < 0) || (nSize > available())) {
            m_pState->bReadError = true;
            return;
        }

        m_nPosition += nSize;
        if (m_nPosition == m_baData.size()) {
            m_baData.clear();
            m_nPosition = 0;
        } else if ((m_nPosition >= m_nChunkSize) && (m_nPosition >= (m_baData.size() / 2))) {
            m_baData.remove(0, m_nPosition);
            m_nPosition = 0;
        }
    }

    bool ensure(qint32 nMinimum)
    {
        if ((nMinimum < 0) || !m_pState || (nMinimum > (std::numeric_limits<qint32>::max)())) return false;

        while ((available() < nMinimum) && !m_bAtEnd && XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            if (m_nPosition > 0) {
                m_baData.remove(0, m_nPosition);
                m_nPosition = 0;
            }

            const qint32 nNeeded = nMinimum - available();
            const qint32 nWanted = (std::max)(m_nChunkSize, nNeeded);
            const qint32 nRequest = Algo_utils::getReadChunkSize(m_pState, nWanted);
            if (nRequest <= 0) {
                m_bAtEnd = true;
                break;
            }

            const qint32 nOldSize = m_baData.size();
            if (nOldSize > (std::numeric_limits<qint32>::max)() - nRequest) return false;
            m_baData.resize(nOldSize + nRequest);
            const qint32 nRead = XBinary::_readDevice(m_baData.data() + nOldSize, nRequest, m_pState);
            if (nRead < 0) {
                m_baData.resize(nOldSize);
                return false;
            }
            m_baData.resize(nOldSize + nRead);
            if (nRead == 0) {
                m_bAtEnd = true;
                break;
            }

            if ((m_pState->nInputLimit != -1) && (m_pState->nCountInput == m_pState->nInputLimit)) {
                m_bAtEnd = true;
            }
        }

        return available() >= nMinimum;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    qint32 m_nChunkSize;
    QByteArray m_baData;
    qint32 m_nPosition;
    bool m_bAtEnd;
};

// A valid raw Brotli stream can theoretically begin with the bytes used by the
// private brotli-mt header.  Match 7-Zip-zstd's content-based selection: only
// select the framed path when the strict header is present and a bounded raw
// Brotli probe rejects that prefix.
bool isMtStream(BrotliInputBuffer *pInput,
                XBinary::DATAPROCESS_STATE *pOutputState)
{
    if (!pInput || !pOutputState ||
        !isMtHeader(pInput->data(), pInput->available())) {
        return false;
    }

    BrotliAllocationBudget allocationBudget = {};
    if (!initBrotliAllocationBudget(pOutputState, &allocationBudget)) {
        return false;
    }
    BrotliDecoderState *pState = BrotliDecoderCreateInstance(
        brotliBoundedAlloc, brotliBoundedFree, &allocationBudget);
    if (!pState) return false;
    if (!BrotliDecoderSetParameter(pState, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1)) {
        BrotliDecoderDestroyInstance(pState);
        return false;
    }

    const size_t nProbeSize = static_cast<size_t>((std::min)(pInput->available(), X_BROTLI_MT_PROBE_SIZE));
    size_t nAvailIn = nProbeSize;
    const uint8_t *pNextIn = reinterpret_cast<const uint8_t *>(pInput->data());
    qint64 nProduced = 0;
    BrotliDecoderResult ret = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;

    do {
        uint8_t output[256];
        size_t nAvailOut = sizeof(output);
        uint8_t *pNextOut = output;
        const size_t nBeforeIn = nAvailIn;

        ret = BrotliDecoderDecompressStream(pState, &nAvailIn, &pNextIn, &nAvailOut, &pNextOut, nullptr);
        nProduced += static_cast<qint64>(sizeof(output) - nAvailOut);

        if ((ret != BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) ||
            ((nBeforeIn == nAvailIn) && (nAvailOut == sizeof(output))) || (nProduced >= 0x10000)) {
            break;
        }
    } while (true);

    BrotliDecoderDestroyInstance(pState);
    return ret == BROTLI_DECODER_RESULT_ERROR;
}

bool decodeBrotliStream(BrotliInputBuffer *pInput, qint64 nCompressedSize, qint64 nOutputLimit, qint64 nExpectedTotalOutput,
                        QByteArray *pOutput, XBinary::DATAPROCESS_STATE *pState,
                        XBinary::PDSTRUCT *pPdStruct)
{
    if (!pInput || !pOutput || !pState || (nCompressedSize < -1) || (nOutputLimit < -1) ||
        (nExpectedTotalOutput < -1)) {
        return false;
    }

    BrotliAllocationBudget allocationBudget = {};
    if (!initBrotliAllocationBudget(pState, &allocationBudget)) {
        return false;
    }
    BrotliDecoderState *pDecoder = BrotliDecoderCreateInstance(
        brotliBoundedAlloc, brotliBoundedFree, &allocationBudget);
    if (!pDecoder) return false;
    if (!BrotliDecoderSetParameter(pDecoder, BROTLI_DECODER_PARAM_LARGE_WINDOW, 1)) {
        BrotliDecoderDestroyInstance(pDecoder);
        return false;
    }

    qint64 nRemaining = nCompressedSize;
    qint64 nFrameOutput = 0;
    bool bFinished = false;
    BrotliDecoderResult ret = BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nRemaining != 0) && (pInput->available() == 0) &&
            (ret != BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) && !pInput->ensure(1)) {
            break;
        }

        const qint64 nAvailable64 = (nRemaining == -1)
                                        ? static_cast<qint64>(pInput->available())
                                        : (std::min)(static_cast<qint64>(pInput->available()), nRemaining);
        if (nAvailable64 < 0) break;

        size_t nAvailIn = static_cast<size_t>(nAvailable64);
        const size_t nBeforeIn = nAvailIn;
        const uint8_t *pNextIn = nAvailIn ? reinterpret_cast<const uint8_t *>(pInput->data()) : nullptr;
        size_t nAvailOut = static_cast<size_t>(pOutput->size());
        uint8_t *pNextOut = reinterpret_cast<uint8_t *>(pOutput->data());

        ret = BrotliDecoderDecompressStream(pDecoder, &nAvailIn, &pNextIn, &nAvailOut, &pNextOut, nullptr);
        if ((ret == BROTLI_DECODER_RESULT_ERROR) || (nAvailIn > nBeforeIn) ||
            (nAvailOut > static_cast<size_t>(pOutput->size()))) {
            break;
        }

        const qint64 nConsumed = static_cast<qint64>(nBeforeIn - nAvailIn);
        const qint64 nProduced = static_cast<qint64>(pOutput->size()) - static_cast<qint64>(nAvailOut);
        pInput->consume(static_cast<qint32>(nConsumed));
        if (nRemaining != -1) nRemaining -= nConsumed;

        if ((nOutputLimit != -1) && ((nFrameOutput > nOutputLimit) || (nProduced > nOutputLimit - nFrameOutput))) {
            break;
        }
        nFrameOutput += nProduced;

        if ((nExpectedTotalOutput != -1) &&
            ((pState->nCountOutput > nExpectedTotalOutput) ||
             (nProduced > nExpectedTotalOutput - pState->nCountOutput))) {
            break;
        }
        if ((nProduced > 0) &&
            (XBinary::_writeDevice(pOutput->constData(), static_cast<qint32>(nProduced), pState) != static_cast<qint32>(nProduced))) {
            break;
        }

        if (ret == BROTLI_DECODER_RESULT_SUCCESS) {
            bFinished = (nRemaining == -1) || (nRemaining == 0);
            break;
        }

        if ((ret == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) && (nRemaining == 0)) break;

        if ((nConsumed == 0) && (nProduced == 0)) {
            if (ret != BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) break;
            const qint32 nAvailable = pInput->available();
            if ((nAvailable == (std::numeric_limits<qint32>::max)()) || !pInput->ensure(nAvailable + 1)) break;
        }
    }

    BrotliDecoderDestroyInstance(pDecoder);
    return bFinished && !pState->bReadError && !pState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool decodeMtStream(BrotliInputBuffer *pInput, qint64 nExpectedTotalOutput, QByteArray *pOutput,
                    XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bSawFrame = false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!pInput->ensure(X_BROTLI_MT_HEADER_SIZE)) {
            return bSawFrame && pInput->atEnd() && !pState->bReadError;
        }
        if (!isMtHeader(pInput->data(), pInput->available())) return false;

        const qint64 nCompressedSize = static_cast<qint64>(readUInt32LE(pInput->data() + 8));
        const qint64 nOutputLimit = static_cast<qint64>(readUInt16LE(pInput->data() + 14)) << 16;
        pInput->consume(X_BROTLI_MT_HEADER_SIZE);

        if (!decodeBrotliStream(pInput, nCompressedSize, nOutputLimit, nExpectedTotalOutput, pOutput, pState, pPdStruct)) return false;
        bSawFrame = true;
    }

    return false;
}
}  // namespace

XBrotliDecoder::XBrotliDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XBrotliDecoder::decompressBlock(const quint8 *pInput, qint64 nInputSize, quint8 *pOutput, qint64 nOutputSize, qint64 *pnBytesWritten)
{
    bool bResult = false;

    size_t nDecodedSize = nOutputSize;

    BrotliDecoderResult ret = BrotliDecoderDecompress(nInputSize, pInput, &nDecodedSize, pOutput);

    if (ret == BROTLI_DECODER_RESULT_SUCCESS) {
        if (pnBytesWritten) {
            *pnBytesWritten = (qint64)nDecodedSize;
        }

        bResult = true;
    }

    return bResult;
}

bool XBrotliDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput ||
        (pDecompressState->nInputOffset < 0) || (pDecompressState->nInputLimit < -1) ||
        XBinary::isPdStructStopped(pPdStruct)) {
        return false;
    }

    bool bExpectedOutputValid = true;
    qint64 nExpectedOutput = -1;
    if (pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        nExpectedOutput = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bExpectedOutputValid);
        if (!bExpectedOutputValid || (nExpectedOutput < 0)) return false;
    }

    const qint32 nRequestedBufferSize = XBinary::getBufferSize(pPdStruct);
    if (nRequestedBufferSize <= 0) return false;
    const qint32 nBufferSize = qBound(static_cast<qint32>(0x1000), nRequestedBufferSize, static_cast<qint32>(0x100000));

    XBinary::UNPACK_MEMORY_RESERVATION ioReservation;
    if (!ioReservation.acquire(pDecompressState->mapUnpackProperties,
                               (qint64)nBufferSize * 2)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

    BrotliInputBuffer input(pDecompressState, pPdStruct, nBufferSize);
    QByteArray baOutput(nBufferSize, 0);

    // Fill a bounded prefix for content-based raw-vs-mt selection.  ensure()
    // retains short inputs, so ordinary small raw Brotli streams still work.
    input.ensure(X_BROTLI_MT_PROBE_SIZE);
    bool bDecoded = false;
    if (isMtStream(&input, pDecompressState)) {
        bDecoded = decodeMtStream(&input, nExpectedOutput, &baOutput, pDecompressState, pPdStruct);
    } else {
        bDecoded = decodeBrotliStream(&input, -1, -1, nExpectedOutput, &baOutput, pDecompressState, pPdStruct);
        if (bDecoded && (input.available() == 0)) input.ensure(1);
        bDecoded = bDecoded && input.atEnd();
    }

    const bool bExactInput = input.atEnd() &&
                             ((pDecompressState->nInputLimit == -1) ||
                              (pDecompressState->nCountInput == pDecompressState->nInputLimit));
    const bool bExactOutput = (nExpectedOutput == -1) || (pDecompressState->nCountOutput == nExpectedOutput);
    return bDecoded && bExactInput && bExactOutput && !pDecompressState->bReadError &&
           !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}

quint32 XBrotliDecoder::version()
{
    return BrotliDecoderVersion();
}

QString XBrotliDecoder::errorString(qint32 nErrorCode)
{
    return QString(BrotliDecoderErrorString((BrotliDecoderErrorCode)nErrorCode));
}
