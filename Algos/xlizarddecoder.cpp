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
#include "xlizarddecoder.h"
#include "algo_utils.h"

#include "lz5lizarddeclib.h"

#include <QByteArray>
#include <algorithm>
#include <limits>

namespace {
const quint32 LIZARD_FRAME_MAGIC = 0x184D2206U;
const quint32 LIZARD_SKIPPABLE_MAGIC_START = 0x184D2A50U;
const quint32 LIZARD_SKIPPABLE_MAGIC_MASK = 0xFFFFFFF0U;

quint32 readUInt32LE(const char *pData)
{
    const unsigned char *p = reinterpret_cast<const unsigned char *>(pData);
    return static_cast<quint32>(p[0]) | (static_cast<quint32>(p[1]) << 8) | (static_cast<quint32>(p[2]) << 16) | (static_cast<quint32>(p[3]) << 24);
}

bool isSkippableMagic(quint32 nMagic)
{
    return (nMagic & LIZARD_SKIPPABLE_MAGIC_MASK) == LIZARD_SKIPPABLE_MAGIC_START;
}

class LizardInputBuffer {
public:
    LizardInputBuffer(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, qint32 nChunkSize)
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

    bool consume(qint32 nSize)
    {
        if ((nSize < 0) || (nSize > available())) {
            m_pState->bReadError = true;
            return false;
        }

        m_nPosition += nSize;
        if (m_nPosition == m_baData.size()) {
            m_baData.clear();
            m_nPosition = 0;
        } else if ((m_nPosition >= m_nChunkSize) && (m_nPosition >= (m_baData.size() / 2))) {
            m_baData.remove(0, m_nPosition);
            m_nPosition = 0;
        }
        return true;
    }

    bool ensure(qint32 nMinimum)
    {
        if ((nMinimum < 0) || (nMinimum > m_nChunkSize) || !m_pState) return false;

        while ((available() < nMinimum) && !m_bAtEnd && XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            if (m_nPosition > 0) {
                m_baData.remove(0, m_nPosition);
                m_nPosition = 0;
            }

            const qint32 nNeeded = nMinimum - available();
            const qint32 nWanted = (std::max)(m_nChunkSize, nNeeded);
            const qint32 nRequested = Algo_utils::getReadChunkSize(m_pState, nWanted);
            if (nRequested <= 0) {
                m_bAtEnd = true;
                break;
            }

            const qint32 nOldSize = m_baData.size();
            const qint32 nCapacity = m_nChunkSize - nOldSize;
            if (nCapacity <= 0) return false;
            const qint32 nRequest = (std::min)(nRequested, nCapacity);
            m_baData.resize(nOldSize + nRequest);
            const qint32 nRead = XBinary::_readDevice(m_baData.data() + nOldSize, nRequest, m_pState);
            if ((nRead < 0) || (nRead > nRequest)) {
                m_baData.resize(nOldSize);
                if (nRead > nRequest) m_pState->bReadError = true;
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

bool getFrameBufferRequirement(LizardInputBuffer *pInput, quint64 *pnBufferRequirement)
{
    if (!pInput || !pnBufferRequirement || !pInput->ensure(6)) return false;

    const unsigned char *pData = reinterpret_cast<const unsigned char *>(pInput->data());
    const unsigned char nFlags = pData[4];
    const unsigned char nBlockDescriptor = pData[5];
    const unsigned nVersion = (nFlags >> 6) & 0x03U;
    const unsigned nBlockSizeId = (nBlockDescriptor >> 4) & 0x07U;
    if ((nVersion != 1U) || ((nFlags & 0x13U) != 0) || ((nBlockDescriptor & 0x8FU) != 0) || (nBlockSizeId < 1U) || (nBlockSizeId > 7U)) {
        return false;
    }

    static const quint64 kBlockSizes[7] = {128ULL * 1024ULL,          256ULL * 1024ULL,          1024ULL * 1024ULL,         4ULL * 1024ULL * 1024ULL,
                                           16ULL * 1024ULL * 1024ULL, 64ULL * 1024ULL * 1024ULL, 256ULL * 1024ULL * 1024ULL};
    const quint64 nMaxBlockSize = kBlockSizes[nBlockSizeId - 1U];
    const bool bBlocksLinked = (nFlags & 0x20U) == 0;
    // The vendored Lizard frame decoder reserves two 16 MiB dictionary
    // regions in addition to the advertised maximum block for linked frames.
    *pnBufferRequirement = nMaxBlockSize + (bBlocksLinked ? 32ULL * 1024ULL * 1024ULL : 0ULL);
    return true;
}

bool writeOutput(const QByteArray *pOutput, size_t nOutputSize, qint64 nExpectedOutput, XBinary::DATAPROCESS_STATE *pState)
{
    if (nOutputSize == 0) return true;
    if (nOutputSize > static_cast<size_t>((std::numeric_limits<qint32>::max)())) return false;
    const qint32 nSize = static_cast<qint32>(nOutputSize);
    if ((nExpectedOutput != -1) && ((pState->nCountOutput > nExpectedOutput) || (nSize > nExpectedOutput - pState->nCountOutput))) {
        return false;
    }
    return XBinary::_writeDevice(pOutput->constData(), nSize, pState) == nSize;
}

bool decodeFrame(LizardInputBuffer *pInput, QByteArray *pOutput, qint64 nExpectedOutput, XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    LizardF_decompressionContext_t pContext = nullptr;
    const LizardF_errorCode_t nCreateResult = LizardF_createDecompressionContext(&pContext, LIZARDF_VERSION);
    if (LizardF_isError(nCreateResult) || !pContext) return false;

    bool bFinished = false;
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((pInput->available() == 0) && !pInput->ensure(1)) break;

        const qint32 nAvailableBefore = pInput->available();
        size_t nInputSize = static_cast<size_t>(nAvailableBefore);
        size_t nOutputSize = static_cast<size_t>(pOutput->size());
        const size_t nResult = LizardF_decompress(pContext, pOutput->data(), &nOutputSize, pInput->data(), &nInputSize, nullptr);
        if (LizardF_isError(nResult) || (nInputSize > static_cast<size_t>(nAvailableBefore)) || (nOutputSize > static_cast<size_t>(pOutput->size())) ||
            !pInput->consume(static_cast<qint32>(nInputSize)) || !writeOutput(pOutput, nOutputSize, nExpectedOutput, pState)) {
            break;
        }

        if (nResult == 0) {
            bFinished = true;
            break;
        }

        if ((nInputSize == 0) && (nOutputSize == 0)) {
            const qint32 nAvailable = pInput->available();
            if ((nAvailable == (std::numeric_limits<qint32>::max)()) || !pInput->ensure(nAvailable + 1)) break;
        }
    }

    const LizardF_errorCode_t nFreeResult = LizardF_freeDecompressionContext(pContext);
    return bFinished && (nFreeResult == 0) && !pState->bReadError && !pState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
}  // namespace

XLizardDecoder::XLizardDecoder(QObject *parent) : QObject(parent)
{
}

bool XLizardDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1) || XBinary::isPdStructStopped(pPdStruct)) {
        return false;
    }

    bool bExpectedOutputValid = true;
    qint64 nExpectedOutput = -1;
    qint64 nConfiguredOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nConfiguredOutputLimit)) {
        return false;
    }
    if (pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        nExpectedOutput = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bExpectedOutputValid);
        if (!bExpectedOutputValid || !XBinary::isUnpackOutputSizeAllowed(pDecompressState->mapUnpackProperties, nExpectedOutput)) return false;
    }

    const qint32 nRequestedBufferSize = XBinary::getBufferSize(pPdStruct);
    if (nRequestedBufferSize <= 0) return false;
    const qint32 nBufferSize = qBound(static_cast<qint32>(0x1000), nRequestedBufferSize, static_cast<qint32>(0x100000));

    const qint64 nBaseBufferReservation = (qint64)nBufferSize * 2;
    XBinary::UNPACK_MEMORY_RESERVATION memoryReservation;
    if (!memoryReservation.acquire(pDecompressState->mapUnpackProperties, nBaseBufferReservation)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

    LizardInputBuffer input(pDecompressState, pPdStruct, nBufferSize);
    QByteArray baOutput(nBufferSize, 0);
    bool bSawDataFrame = false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!input.ensure(4)) {
            if (input.atEnd() && bSawDataFrame) break;
            return false;
        }

        const quint32 nMagic = readUInt32LE(input.data());
        if ((nMagic != LIZARD_FRAME_MAGIC) && !isSkippableMagic(nMagic)) return false;
        if (nMagic == LIZARD_FRAME_MAGIC) {
            quint64 nFrameBufferRequirement = 0;
            if (!getFrameBufferRequirement(&input, &nFrameBufferRequirement) || (nFrameBufferRequirement > (quint64)(std::numeric_limits<qint64>::max)()) ||
                ((nConfiguredOutputLimit >= 0) && (nFrameBufferRequirement > static_cast<quint64>(nConfiguredOutputLimit))) ||
                !memoryReservation.resize(nBaseBufferReservation + (qint64)nFrameBufferRequirement)) {
                return false;
            }
        }
        if (!decodeFrame(&input, &baOutput, nExpectedOutput, pDecompressState, pPdStruct)) return false;
        if ((nMagic == LIZARD_FRAME_MAGIC) && !memoryReservation.resize(nBaseBufferReservation)) {
            return false;
        }
        if (nMagic == LIZARD_FRAME_MAGIC) bSawDataFrame = true;
    }

    const bool bExactInput = input.atEnd() && ((pDecompressState->nInputLimit == -1) || (pDecompressState->nCountInput == pDecompressState->nInputLimit));
    const bool bExactOutput = (nExpectedOutput == -1) || (pDecompressState->nCountOutput == nExpectedOutput);
    return bSawDataFrame && bExactInput && bExactOutput && !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
