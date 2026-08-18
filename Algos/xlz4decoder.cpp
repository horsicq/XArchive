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
#include "xlz4decoder.h"
#include "algo_utils.h"

#include "lz4declib.h"

#include <QByteArray>
#include <algorithm>
#include <cstring>
#include <limits>

namespace {
const quint32 LZ4_LEGACY_MAGIC = 0x184C2102U;
const quint32 LZ4_SKIPPABLE_MASK = 0xFFFFFFF0U;
const qint32 LZ4_LEGACY_BLOCK_SIZE = 8 * 1024 * 1024;
const qint32 LZ4_LEGACY_COMPRESSED_LIMIT = LZ4_COMPRESSBOUND(LZ4_LEGACY_BLOCK_SIZE);

quint32 readUInt32LE(const char *pData)
{
    const unsigned char *p = reinterpret_cast<const unsigned char *>(pData);
    return static_cast<quint32>(p[0]) | (static_cast<quint32>(p[1]) << 8) |
           (static_cast<quint32>(p[2]) << 16) | (static_cast<quint32>(p[3]) << 24);
}

bool isSkippableMagic(quint32 nMagic)
{
    return (nMagic & LZ4_SKIPPABLE_MASK) == LZ4F_MAGIC_SKIPPABLE_START;
}

bool isMemberMagic(quint32 nMagic)
{
    return (nMagic == LZ4F_MAGICNUMBER) || (nMagic == LZ4_LEGACY_MAGIC) || isSkippableMagic(nMagic);
}

bool writeOutput(const QByteArray *pOutput, size_t nOutputSize, qint64 nExpectedOutput,
                 XBinary::DATAPROCESS_STATE *pState)
{
    if (nOutputSize == 0) return true;
    if (nOutputSize > static_cast<size_t>((std::numeric_limits<qint32>::max)())) return false;
    const qint32 nSize = static_cast<qint32>(nOutputSize);
    if ((nExpectedOutput != -1) &&
        ((pState->nCountOutput > nExpectedOutput) || (nSize > nExpectedOutput - pState->nCountOutput))) {
        return false;
    }
    return XBinary::_writeDevice(pOutput->constData(), nSize, pState) == nSize;
}

class Lz4InputBuffer {
public:
    Lz4InputBuffer(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, qint32 nChunkSize)
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

bool decodeFrame(Lz4InputBuffer *pInput, QByteArray *pOutput, qint64 nExpectedOutput,
                 XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    LZ4F_dctx *pContext = nullptr;
    const LZ4F_errorCode_t nCreateResult = LZ4F_createDecompressionContext(&pContext, LZ4F_VERSION);
    if (LZ4F_isError(nCreateResult) || !pContext) return false;

    bool bFinished = false;
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((pInput->available() == 0) && !pInput->ensure(1)) break;

        size_t nInputSize = static_cast<size_t>(pInput->available());
        size_t nOutputSize = static_cast<size_t>(pOutput->size());
        const size_t nResult = LZ4F_decompress(pContext, pOutput->data(), &nOutputSize, pInput->data(), &nInputSize, nullptr);
        if (LZ4F_isError(nResult) || (nInputSize > static_cast<size_t>(pInput->available())) ||
            (nOutputSize > static_cast<size_t>(pOutput->size()))) {
            break;
        }

        pInput->consume(static_cast<qint32>(nInputSize));
        if (!writeOutput(pOutput, nOutputSize, nExpectedOutput, pState)) break;

        if (nResult == 0) {
            bFinished = true;
            break;
        }

        if ((nInputSize == 0) && (nOutputSize == 0)) {
            const qint32 nAvailable = pInput->available();
            if (!pInput->ensure(nAvailable + 1)) break;
        }
    }

    LZ4F_freeDecompressionContext(pContext);
    return bFinished && !pState->bReadError && !pState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool decodeLegacyFrame(Lz4InputBuffer *pInput, QByteArray *pCompressed, QByteArray *pOutput,
                       qint64 nExpectedOutput, XBinary::DATAPROCESS_STATE *pState,
                       XBinary::PDSTRUCT *pPdStruct)
{
    bool bDecodedBlock = false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!pInput->ensure(4)) {
            return bDecodedBlock && pInput->atEnd() && !pState->bReadError;
        }

        const quint32 nBlockSize = readUInt32LE(pInput->data());
        if (bDecodedBlock && isMemberMagic(nBlockSize)) {
            return true;
        }
        if ((nBlockSize == 0) || (nBlockSize > static_cast<quint32>(LZ4_LEGACY_COMPRESSED_LIMIT))) return false;

        pInput->consume(4);
        if (!pInput->ensure(static_cast<qint32>(nBlockSize))) return false;

        pCompressed->resize(static_cast<qint32>(nBlockSize));
        std::memcpy(pCompressed->data(), pInput->data(), static_cast<size_t>(nBlockSize));
        pInput->consume(static_cast<qint32>(nBlockSize));

        const int nDecoded = LZ4_decompress_safe(pCompressed->constData(), pOutput->data(), static_cast<int>(nBlockSize), pOutput->size());
        if (nDecoded < 0) return false;
        if (!writeOutput(pOutput, static_cast<size_t>(nDecoded), nExpectedOutput, pState)) return false;
        bDecodedBlock = true;
    }

    return false;
}
}  // namespace

XLZ4Decoder::XLZ4Decoder(QObject *parent) : QObject(parent)
{
}

bool XLZ4Decoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
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

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError) return false;

    Lz4InputBuffer input(pDecompressState, pPdStruct, nBufferSize);
    QByteArray baOutput(nBufferSize, 0);
    QByteArray baLegacyOutput(LZ4_LEGACY_BLOCK_SIZE, 0);
    QByteArray baLegacyCompressed;
    bool bSawDataFrame = false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (!input.ensure(4)) {
            if (input.atEnd() && bSawDataFrame) break;
            return false;
        }

        const quint32 nMagic = readUInt32LE(input.data());
        if ((nMagic == LZ4F_MAGICNUMBER) || isSkippableMagic(nMagic)) {
            if (!decodeFrame(&input, &baOutput, nExpectedOutput, pDecompressState, pPdStruct)) return false;
            if (nMagic == LZ4F_MAGICNUMBER) bSawDataFrame = true;
        } else if (nMagic == LZ4_LEGACY_MAGIC) {
            input.consume(4);
            if (!decodeLegacyFrame(&input, &baLegacyCompressed, &baLegacyOutput, nExpectedOutput,
                                   pDecompressState, pPdStruct)) return false;
            bSawDataFrame = true;
        } else {
            return false;
        }
    }

    const bool bExactInput = input.atEnd() &&
                             ((pDecompressState->nInputLimit == -1) ||
                              (pDecompressState->nCountInput == pDecompressState->nInputLimit));
    const bool bExactOutput = (nExpectedOutput == -1) || (pDecompressState->nCountOutput == nExpectedOutput);
    return bSawDataFrame && bExactInput && bExactOutput && !pDecompressState->bReadError &&
           !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
