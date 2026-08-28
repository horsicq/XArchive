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
#include "xwavpackdecoder.h"

#include "wavpack.h"

#include <cstdio>
#include <cstring>

namespace {

const qint32 WZWV_SAMPLE_CHUNK = 4096;
const qint32 WZWV_MAX_CHANNELS = 256;

// WavpackStreamReader64 backed by the record's extent on a QIODevice: the
// reader sees a virtual file of nLimit bytes starting at nBase.
struct WVREADER_CONTEXT {
    QIODevice *pDevice;
    qint64 nBase;
    qint64 nLength;  // resolved extent length
    qint64 nPosition;
    qint64 nCountInput;  // high-water mark of consumed bytes
    bool bReadError;
    qint32 nPushBack;  // -1 = none
};

static int32_t wvReadBytes(void *pId, void *pData, int32_t nCount)
{
    WVREADER_CONTEXT *pContext = (WVREADER_CONTEXT *)pId;
    if (nCount <= 0) return 0;

    quint8 *pOut = (quint8 *)pData;
    int32_t nDone = 0;

    if (pContext->nPushBack >= 0) {
        pOut[0] = (quint8)pContext->nPushBack;
        pContext->nPushBack = -1;
        pContext->nPosition++;
        nDone++;
    }

    while (nDone < nCount) {
        qint64 nRemaining = pContext->nLength - pContext->nPosition;
        if (nRemaining <= 0) break;
        qint64 nRequest = nCount - nDone;
        if (nRequest > nRemaining) nRequest = nRemaining;

        if (!pContext->pDevice->seek(pContext->nBase + pContext->nPosition)) {
            pContext->bReadError = true;
            break;
        }
        const qint64 nRead = pContext->pDevice->read((char *)(pOut + nDone), nRequest);
        if (nRead <= 0) {
            pContext->bReadError = true;
            break;
        }
        pContext->nPosition += nRead;
        nDone += (int32_t)nRead;
    }

    if (pContext->nPosition > pContext->nCountInput) pContext->nCountInput = pContext->nPosition;

    return nDone;
}

static int32_t wvWriteBytes(void *pId, void *pData, int32_t nCount)
{
    Q_UNUSED(pId)
    Q_UNUSED(pData)
    Q_UNUSED(nCount)
    return 0;  // read-only source
}

static int64_t wvGetPos(void *pId)
{
    WVREADER_CONTEXT *pContext = (WVREADER_CONTEXT *)pId;
    return pContext->nPosition;
}

static int wvSetPosAbs(void *pId, int64_t nPosition)
{
    WVREADER_CONTEXT *pContext = (WVREADER_CONTEXT *)pId;
    if ((nPosition < 0) || (nPosition > pContext->nLength)) return -1;
    pContext->nPosition = nPosition;
    pContext->nPushBack = -1;
    return 0;
}

static int wvSetPosRel(void *pId, int64_t nDelta, int nMode)
{
    WVREADER_CONTEXT *pContext = (WVREADER_CONTEXT *)pId;
    qint64 nNewPosition = 0;
    if (nMode == SEEK_SET) nNewPosition = nDelta;
    else if (nMode == SEEK_CUR) nNewPosition = pContext->nPosition + nDelta;
    else if (nMode == SEEK_END) nNewPosition = pContext->nLength + nDelta;
    else return -1;
    if ((nNewPosition < 0) || (nNewPosition > pContext->nLength)) return -1;
    pContext->nPosition = nNewPosition;
    pContext->nPushBack = -1;
    return 0;
}

static int wvPushBackByte(void *pId, int nChar)
{
    WVREADER_CONTEXT *pContext = (WVREADER_CONTEXT *)pId;
    if ((pContext->nPushBack >= 0) || (pContext->nPosition <= 0)) return EOF;
    pContext->nPosition--;
    pContext->nPushBack = nChar & 0xff;
    return nChar & 0xff;
}

static int64_t wvGetLength(void *pId)
{
    WVREADER_CONTEXT *pContext = (WVREADER_CONTEXT *)pId;
    return pContext->nLength;
}

static int wvCanSeek(void *pId)
{
    Q_UNUSED(pId)
    return 1;
}

static int wvTruncateHere(void *pId)
{
    Q_UNUSED(pId)
    return -1;  // read-only source
}

static int wvClose(void *pId)
{
    Q_UNUSED(pId)
    return 0;  // caller owns the device
}

static WavpackStreamReader64 g_wvReader = {wvReadBytes, wvWriteBytes, wvGetPos, wvSetPosAbs, wvSetPosRel, wvPushBackByte, wvGetLength, wvCanSeek, wvTruncateHere, wvClose};

static bool wvWriteAll(XBinary::DATAPROCESS_STATE *pState, const quint8 *pData, qint64 nSize, qint64 *pnTotalWritten)
{
    if (nSize <= 0) return true;
    qint64 nOffset = 0;
    while (nOffset < nSize) {
        qint32 nChunk = 0x10000;
        if ((qint64)nChunk > (nSize - nOffset)) nChunk = (qint32)(nSize - nOffset);
        if (XBinary::_writeDevice((const char *)(pData + nOffset), nChunk, pState) != nChunk) return false;
        nOffset += nChunk;
    }
    *pnTotalWritten += nSize;
    return true;
}

// Re-format decoded samples to their original little-endian byte layout, as
// stored in the source file (APPNOTE 5.9.3: the full container sample width
// is always encoded, so this restores the exact original bytes).
static void wvStoreSamples(const qint32 *pSamples, qint64 nValueCount, qint32 nBytesPerSample, quint8 *pOut)
{
    if (nBytesPerSample == 1) {
        for (qint64 i = 0; i < nValueCount; i++) pOut[i] = (quint8)(pSamples[i] + 128);
    } else if (nBytesPerSample == 2) {
        for (qint64 i = 0; i < nValueCount; i++) {
            const qint32 nValue = pSamples[i];
            pOut[i * 2 + 0] = (quint8)nValue;
            pOut[i * 2 + 1] = (quint8)(nValue >> 8);
        }
    } else if (nBytesPerSample == 3) {
        for (qint64 i = 0; i < nValueCount; i++) {
            const qint32 nValue = pSamples[i];
            pOut[i * 3 + 0] = (quint8)nValue;
            pOut[i * 3 + 1] = (quint8)(nValue >> 8);
            pOut[i * 3 + 2] = (quint8)(nValue >> 16);
        }
    } else {
        for (qint64 i = 0; i < nValueCount; i++) {
            const qint32 nValue = pSamples[i];
            pOut[i * 4 + 0] = (quint8)nValue;
            pOut[i * 4 + 1] = (quint8)(nValue >> 8);
            pOut[i * 4 + 2] = (quint8)(nValue >> 16);
            pOut[i * 4 + 3] = (quint8)(nValue >> 24);
        }
    }
}

}  // namespace

XWavPackDecoder::XWavPackDecoder(QObject *parent) : QObject(parent)
{
}

bool XWavPackDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1)) {
        return false;
    }
    if ((pDecompressState->nProcessedOffset < 0) || (pDecompressState->nProcessedLimit < -1)) {
        pDecompressState->bWriteError = true;
        return false;
    }

    WVREADER_CONTEXT readerContext = {};
    readerContext.pDevice = pDecompressState->pDeviceInput;
    readerContext.nBase = pDecompressState->nInputOffset;
    readerContext.nPushBack = -1;

    qint64 nDeviceRemaining = pDecompressState->pDeviceInput->size() - pDecompressState->nInputOffset;
    if (nDeviceRemaining < 0) nDeviceRemaining = 0;
    if (pDecompressState->nInputLimit == -1) readerContext.nLength = nDeviceRemaining;
    else readerContext.nLength = qMin(pDecompressState->nInputLimit, nDeviceRemaining);

    // OPEN_WRAPPER keeps the stored source-file header/trailer bytes;
    // normalization must stay off so float data restores bit-exactly.
    char szError[81] = {0};
    WavpackContext *pWavpack = WavpackOpenFileInputEx64(&g_wvReader, &readerContext, nullptr, szError, OPEN_WRAPPER, 0);
    if (!pWavpack) {
        if (readerContext.bReadError) pDecompressState->bReadError = true;
        pDecompressState->nCountInput = readerContext.nCountInput;
        return false;
    }

    bool bResult = true;
    qint64 nTotalWritten = 0;

    const qint32 nMode = WavpackGetMode(pWavpack);
    const qint32 nChannels = WavpackGetNumChannels(pWavpack);
    const qint32 nBytesPerSample = WavpackGetBytesPerSample(pWavpack);

    // Only complete lossless PCM streams can reproduce the original file.
    if (!(nMode & MODE_LOSSLESS)) bResult = false;
    if ((nChannels < 1) || (nChannels > WZWV_MAX_CHANNELS)) bResult = false;
    if ((nBytesPerSample < 1) || (nBytesPerSample > 4)) bResult = false;

    // Leading wrapper: the stored source header (e.g. the RIFF chunks up to
    // the data).
    if (bResult && WavpackGetWrapperBytes(pWavpack)) {
        bResult = wvWriteAll(pDecompressState, WavpackGetWrapperData(pWavpack), WavpackGetWrapperBytes(pWavpack), &nTotalWritten);
        if (!bResult) pDecompressState->bWriteError = true;
        WavpackFreeWrapper(pWavpack);
    }

    if (bResult) {
        QByteArray baSampleBuffer;
        QByteArray baByteBuffer;
        baSampleBuffer.resize(WZWV_SAMPLE_CHUNK * nChannels * (qint32)sizeof(qint32));
        baByteBuffer.resize(WZWV_SAMPLE_CHUNK * nChannels * 4);
        if ((baSampleBuffer.size() != WZWV_SAMPLE_CHUNK * nChannels * (qint32)sizeof(qint32)) || (baByteBuffer.size() != WZWV_SAMPLE_CHUNK * nChannels * 4)) {
            bResult = false;
        }

        while (bResult) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                bResult = false;
                break;
            }

            const quint32 nUnpacked = WavpackUnpackSamples(pWavpack, (int32_t *)baSampleBuffer.data(), WZWV_SAMPLE_CHUNK);
            if (nUnpacked == 0) break;

            const qint64 nValueCount = (qint64)nUnpacked * nChannels;
            wvStoreSamples((const qint32 *)baSampleBuffer.constData(), nValueCount, nBytesPerSample, (quint8 *)baByteBuffer.data());

            if (!wvWriteAll(pDecompressState, (const quint8 *)baByteBuffer.constData(), nValueCount * nBytesPerSample, &nTotalWritten)) {
                pDecompressState->bWriteError = true;
                bResult = false;
            }
        }
    }

    if (bResult && WavpackGetNumErrors(pWavpack)) bResult = false;

    // Trailing wrapper: source bytes stored after the audio data accumulate in
    // the wrapper buffer while the samples are unpacked; do not call
    // WavpackSeekTrailingWrapper here, which would collect them a second time.
    if (bResult && WavpackGetWrapperBytes(pWavpack)) {
        bResult = wvWriteAll(pDecompressState, WavpackGetWrapperData(pWavpack), WavpackGetWrapperBytes(pWavpack), &nTotalWritten);
        if (!bResult) pDecompressState->bWriteError = true;
        WavpackFreeWrapper(pWavpack);
    }

    if (bResult) {
        const bool bHasExpectedSize = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
        const qint64 nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
        if (bHasExpectedSize && (nExpectedSize >= 0) && (nTotalWritten != nExpectedSize)) bResult = false;
    }

    if (readerContext.bReadError) pDecompressState->bReadError = true;
    pDecompressState->nCountInput = readerContext.nCountInput;

    WavpackCloseFile(pWavpack);

    return bResult;
}
