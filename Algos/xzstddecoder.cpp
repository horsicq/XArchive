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
#include "xzstddecoder.h"
#include "algo_utils.h"

#include <QByteArray>
#include <algorithm>
#include <limits>
#include <new>

extern "C" {
#include "zstdlegacydeclib.h"
}

namespace {
const quint32 X_ZSTD_FRAME_MAGIC = 0xFD2FB528U;
const quint32 X_ZSTD_LEGACY_MAGIC_V04 = 0xFD2FB524U;
const quint32 X_ZSTD_LEGACY_MAGIC_V07 = 0xFD2FB527U;
const quint32 X_ZSTD_SKIPPABLE_MAGIC_START = 0x184D2A50U;
const quint32 X_ZSTD_SKIPPABLE_MAGIC_MASK = 0xFFFFFFF0U;

quint32 readUInt32LE(const char *pData)
{
    const unsigned char *p = reinterpret_cast<const unsigned char *>(pData);
    return static_cast<quint32>(p[0]) | (static_cast<quint32>(p[1]) << 8) |
           (static_cast<quint32>(p[2]) << 16) | (static_cast<quint32>(p[3]) << 24);
}

bool isSkippableMagic(quint32 nMagic)
{
    return (nMagic & X_ZSTD_SKIPPABLE_MAGIC_MASK) == X_ZSTD_SKIPPABLE_MAGIC_START;
}

unsigned getLegacyVersion(quint32 nMagic)
{
    if ((nMagic < X_ZSTD_LEGACY_MAGIC_V04) || (nMagic > X_ZSTD_LEGACY_MAGIC_V07)) return 0;
    return 4U + static_cast<unsigned>(nMagic - X_ZSTD_LEGACY_MAGIC_V04);
}

class ZstdLegacyStream {
public:
    ZstdLegacyStream() : m_pContext(nullptr), m_nVersion(0) {}
    ~ZstdLegacyStream() { close(); }

    void reset() { close(); }

    bool initialize(unsigned nVersion)
    {
        close();
        m_nVersion = nVersion;

        size_t nResult = static_cast<size_t>(-1);
        switch (m_nVersion) {
            case 4:
                m_pContext = ZBUFFv04_createDCtx();
                if (m_pContext) nResult = ZBUFFv04_decompressInit(static_cast<ZBUFFv04_DCtx *>(m_pContext));
                break;
            case 5:
                m_pContext = ZBUFFv05_createDCtx();
                if (m_pContext) nResult = ZBUFFv05_decompressInit(static_cast<ZBUFFv05_DCtx *>(m_pContext));
                break;
            case 6:
                m_pContext = ZBUFFv06_createDCtx();
                if (m_pContext) nResult = ZBUFFv06_decompressInit(static_cast<ZBUFFv06_DCtx *>(m_pContext));
                break;
            case 7:
                m_pContext = ZBUFFv07_createDCtx();
                if (m_pContext) nResult = ZBUFFv07_decompressInit(static_cast<ZBUFFv07_DCtx *>(m_pContext));
                break;
            default: break;
        }

        if (!m_pContext || isError(nResult)) {
            close();
            return false;
        }
        return true;
    }

    bool decompress(void *pOutput, size_t *pnOutputSize, const void *pInput, size_t *pnInputSize,
                    bool *pbFrameFinished)
    {
        if (!m_pContext || !pnOutputSize || !pnInputSize || !pbFrameFinished) return false;

        size_t nResult = static_cast<size_t>(-1);
        switch (m_nVersion) {
            case 4:
                nResult = ZBUFFv04_decompressContinue(static_cast<ZBUFFv04_DCtx *>(m_pContext), pOutput,
                                                       pnOutputSize, pInput, pnInputSize);
                break;
            case 5:
                nResult = ZBUFFv05_decompressContinue(static_cast<ZBUFFv05_DCtx *>(m_pContext), pOutput,
                                                       pnOutputSize, pInput, pnInputSize);
                break;
            case 6:
                nResult = ZBUFFv06_decompressContinue(static_cast<ZBUFFv06_DCtx *>(m_pContext), pOutput,
                                                       pnOutputSize, pInput, pnInputSize);
                break;
            case 7:
                nResult = ZBUFFv07_decompressContinue(static_cast<ZBUFFv07_DCtx *>(m_pContext), pOutput,
                                                       pnOutputSize, pInput, pnInputSize);
                break;
            default: return false;
        }

        if (isError(nResult)) return false;
        *pbFrameFinished = (nResult == 0);
        return true;
    }

private:
    bool isError(size_t nResult) const
    {
        switch (m_nVersion) {
            case 4: return ZBUFFv04_isError(nResult) != 0;
            case 5: return ZBUFFv05_isError(nResult) != 0;
            case 6: return ZBUFFv06_isError(nResult) != 0;
            case 7: return ZBUFFv07_isError(nResult) != 0;
            default: return true;
        }
    }

    void close()
    {
        if (!m_pContext) {
            m_nVersion = 0;
            return;
        }

        switch (m_nVersion) {
            case 4: ZBUFFv04_freeDCtx(static_cast<ZBUFFv04_DCtx *>(m_pContext)); break;
            case 5: ZBUFFv05_freeDCtx(static_cast<ZBUFFv05_DCtx *>(m_pContext)); break;
            case 6: ZBUFFv06_freeDCtx(static_cast<ZBUFFv06_DCtx *>(m_pContext)); break;
            case 7: ZBUFFv07_freeDCtx(static_cast<ZBUFFv07_DCtx *>(m_pContext)); break;
            default: break;
        }
        m_pContext = nullptr;
        m_nVersion = 0;
    }

    void *m_pContext;
    unsigned m_nVersion;
};

class ZstdInputBuffer {
public:
    ZstdInputBuffer(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct, qint32 nChunkSize)
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
}  // namespace

XZstdDecoder::XZstdDecoder(QObject *parent) : QObject(parent)
{
}

bool XZstdDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
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

    ZSTD_DStream *pDStream = ZSTD_createDStream();
    if (!pDStream) return false;

    ZstdInputBuffer input(pDecompressState, pPdStruct, nBufferSize);
    QByteArray baOutput(nBufferSize, 0);
    ZstdLegacyStream legacyStream;
    bool bAtFrameStart = true;
    bool bCurrentFrameIsData = false;
    bool bCurrentFrameIsLegacy = false;
    bool bSawDataFrame = false;
    bool bFinished = false;
    bool bValid = true;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (bAtFrameStart) {
            if (!input.ensure(4)) {
                bFinished = input.atEnd() && bSawDataFrame;
                if (!bFinished) bValid = false;
                break;
            }

            const quint32 nMagic = readUInt32LE(input.data());
            const unsigned nLegacyVersion = getLegacyVersion(nMagic);
            bCurrentFrameIsLegacy = nLegacyVersion != 0;
            bCurrentFrameIsData = (nMagic == X_ZSTD_FRAME_MAGIC) || bCurrentFrameIsLegacy;
            if (!bCurrentFrameIsData && !isSkippableMagic(nMagic)) {
                bValid = false;
                break;
            }

            if (bCurrentFrameIsLegacy) {
                if (!legacyStream.initialize(nLegacyVersion)) {
                    bValid = false;
                    break;
                }
            } else {
                legacyStream.reset();
                const size_t nInitResult = ZSTD_initDStream(pDStream);
                if (ZSTD_isError(nInitResult)) {
                    bValid = false;
                    break;
                }
            }
            bAtFrameStart = false;
        }

        if ((input.available() == 0) && !input.ensure(1)) {
            bValid = false;
            break;
        }

        ZSTD_inBuffer zstdInput = {};
        zstdInput.src = input.data();
        zstdInput.size = static_cast<size_t>(input.available());
        zstdInput.pos = 0;

        ZSTD_outBuffer zstdOutput = {};
        zstdOutput.dst = baOutput.data();
        zstdOutput.size = static_cast<size_t>(baOutput.size());
        zstdOutput.pos = 0;

        size_t nRet = 0;
        bool bDecodeResult = true;
        if (bCurrentFrameIsLegacy) {
            size_t nLegacyInputSize = zstdInput.size;
            size_t nLegacyOutputSize = zstdOutput.size;
            bool bLegacyFrameFinished = false;
            bDecodeResult = legacyStream.decompress(zstdOutput.dst, &nLegacyOutputSize, zstdInput.src,
                                                    &nLegacyInputSize, &bLegacyFrameFinished);
            zstdInput.pos = nLegacyInputSize;
            zstdOutput.pos = nLegacyOutputSize;
            nRet = bLegacyFrameFinished ? 0 : 1;
        } else {
            nRet = ZSTD_decompressStream(pDStream, &zstdOutput, &zstdInput);
            bDecodeResult = ZSTD_isError(nRet) == 0;
        }

        if (!bDecodeResult || (zstdInput.pos > zstdInput.size) || (zstdOutput.pos > zstdOutput.size) ||
            (zstdInput.pos > static_cast<size_t>((std::numeric_limits<qint32>::max)()))) {
            bValid = false;
            break;
        }

        input.consume(static_cast<qint32>(zstdInput.pos));
        if ((nExpectedOutput != -1) &&
            ((pDecompressState->nCountOutput > nExpectedOutput) ||
             (static_cast<qint64>(zstdOutput.pos) > nExpectedOutput - pDecompressState->nCountOutput))) {
            bValid = false;
            break;
        }
        if ((zstdOutput.pos > 0) &&
            (XBinary::_writeDevice(baOutput.constData(), static_cast<qint32>(zstdOutput.pos), pDecompressState) !=
             static_cast<qint32>(zstdOutput.pos))) {
            bValid = false;
            break;
        }

        if (nRet == 0) {
            if (bCurrentFrameIsData) bSawDataFrame = true;
            bAtFrameStart = true;
            continue;
        }

        if ((zstdInput.pos == 0) && (zstdOutput.pos == 0)) {
            const qint32 nAvailable = input.available();
            if ((nAvailable == (std::numeric_limits<qint32>::max)()) || !input.ensure(nAvailable + 1)) {
                bValid = false;
                break;
            }
        }
    }

    ZSTD_freeDStream(pDStream);

    const bool bExactInput = input.atEnd() &&
                             ((pDecompressState->nInputLimit == -1) ||
                              (pDecompressState->nCountInput == pDecompressState->nInputLimit));
    const bool bExactOutput = (nExpectedOutput == -1) || (pDecompressState->nCountOutput == nExpectedOutput);
    return bValid && bFinished && bExactInput && bExactOutput && !pDecompressState->bReadError &&
           !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
