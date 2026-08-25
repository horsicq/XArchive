/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#include "xdecompress.h"
#include "subdevice.h"
#include "xpng.h"
#include "Algos/algo_utils.h"
#include "Algos/xkwajlzssdecoder.h"
#include "Algos/xkwajlzhdecoder.h"
#include <QCoreApplication>
#include <QPointer>
#include <algorithm>
#include <limits>
#include <memory>
#include <new>

namespace {
class DecBoundedReadDevice : public QIODevice {
public:
    DecBoundedReadDevice(QIODevice *pSource, qint64 nLimit)
        : m_pSource(pSource), m_nLimit(nLimit), m_nConsumed(0), m_bError(false)
    {
    }

    bool isSequential() const override { return true; }
    qint64 consumed() const { return m_nConsumed; }
    bool hasError() const { return m_bError; }

protected:
    qint64 readData(char *pData, qint64 nMaximumSize) override
    {
        if (!m_pSource || (nMaximumSize < 0) || ((nMaximumSize > 0) && !pData) || (m_nConsumed < 0) || (m_nConsumed > m_nLimit)) {
            m_bError = true;
            return -1;
        }

        const qint64 nRemaining = m_nLimit - m_nConsumed;
        if ((nMaximumSize == 0) || (nRemaining == 0)) {
            return 0;
        }

        const qint64 nRequest = (std::min)(nMaximumSize, nRemaining);
        const qint64 nResult = m_pSource->read(pData, nRequest);
        if (!m_pSource || (nResult < 0) || (nResult > nRequest)) {
            m_bError = true;
            return -1;
        }
        if (nResult == 0) {
            m_bError = true;
            return 0;
        }

        m_nConsumed += nResult;
        return nResult;
    }

    qint64 writeData(const char *, qint64) override { return -1; }

private:
    QPointer<QIODevice> m_pSource;
    qint64 m_nLimit;
    qint64 m_nConsumed;
    bool m_bError;
};

class DecWindowWriteDevice : public QIODevice {
public:
    explicit DecWindowWriteDevice(XBinary::DATAPROCESS_STATE *pState) : m_pState(pState), m_bError(false) {}

    bool isSequential() const override { return true; }
    bool hasError() const { return m_bError; }

protected:
    qint64 readData(char *, qint64) override { return -1; }

    qint64 writeData(const char *pData, qint64 nSize) override
    {
        if (!m_pState || (nSize < 0) || ((nSize > 0) && !pData)) {
            m_bError = true;
            return -1;
        }

        qint64 nDone = 0;
        while (nDone < nSize) {
            const qint32 nChunk = (qint32)(std::min)(nSize - nDone, (qint64)(std::numeric_limits<qint32>::max)());
            if (XBinary::_writeDevice(pData + nDone, nChunk, m_pState) != nChunk) {
                m_bError = true;
                return -1;
            }
            nDone += nChunk;
        }

        return nSize;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    bool m_bError;
};

class DecLzipCRCWindowWriteDevice : public QIODevice {
public:
    explicit DecLzipCRCWindowWriteDevice(XBinary::DATAPROCESS_STATE *pState)
        : m_pState(pState), m_nCRC32(0xFFFFFFFF), m_nProduced(0), m_bError(false)
    {
    }

    bool isSequential() const override { return true; }
    quint32 crc32() const { return m_nCRC32 ^ 0xFFFFFFFF; }
    qint64 produced() const { return m_nProduced; }
    bool hasError() const { return m_bError; }

protected:
    qint64 readData(char *, qint64) override { return -1; }

    qint64 writeData(const char *pData, qint64 nSize) override
    {
        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if (!m_pState || (nSize < 0) || ((nSize > 0) && !pData) || (m_nProduced > (nMax - nSize))) {
            m_bError = true;
            return -1;
        }

        qint64 nDone = 0;
        while (nDone < nSize) {
            const qint32 nChunk = (qint32)(std::min)(nSize - nDone, (qint64)(std::numeric_limits<qint32>::max)());
            if (XBinary::_writeDevice(pData + nDone, nChunk, m_pState) != nChunk) {
                m_bError = true;
                return -1;
            }
            m_nCRC32 = XBinary::_getCRC32(pData + nDone, nChunk, m_nCRC32, XBinary::_getCRC32Table_EDB88320());
            nDone += nChunk;
        }

        m_nProduced += nDone;
        return nSize;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    quint32 m_nCRC32;
    qint64 m_nProduced;
    bool m_bError;
};

class DecDiscardWriteDevice : public QIODevice {
public:
    bool isSequential() const override { return true; }

protected:
    qint64 readData(char *, qint64) override { return -1; }
    qint64 writeData(const char *pData, qint64 nSize) override
    {
        return ((nSize >= 0) && ((nSize == 0) || pData)) ? nSize : -1;
    }
};

struct DecNestedProgressBridge {
    XBinary::PDSTRUCT *pOriginal;
    XBinary::PDSTRUCTLIFETIME originalLifetime;
};

static void decNestedProgressCallback(void *pUserData, XBinary::PDSTRUCT *pLocalProgress)
{
    DecNestedProgressBridge *pBridge = static_cast<DecNestedProgressBridge *>(pUserData);
    if (!pBridge || !pLocalProgress) return;

    if (!XBinary::isPdStructLifetimeAlive(pBridge->originalLifetime) ||
        !XBinary::isPdStructNotCanceled(pBridge->pOriginal)) {
        XBinary::setPdStructStopped(pLocalProgress);
    }
}

static void decPrepareNestedProgress(XBinary::PDSTRUCT *pLocalProgress,
                                     XBinary::PDSTRUCT *pOriginal,
                                     DecNestedProgressBridge *pBridge)
{
    if (pLocalProgress && pOriginal && pBridge) {
        pBridge->pOriginal = pOriginal;
        pBridge->originalLifetime = XBinary::retainPdStructLifetime(pOriginal);
        XBinary::setPdStructCallback(pLocalProgress, decNestedProgressCallback, pBridge);
    }
}

enum class DecCRCResult {
    Ok,
    Invalid,
    NotReadable,
    SeekError,
    Aborted
};

static bool decProgressAlive(XBinary::PDSTRUCT *pPdStruct,
                             const XBinary::PDSTRUCTLIFETIME &lifetime)
{
    return !pPdStruct || XBinary::isPdStructLifetimeAlive(lifetime);
}

static DecCRCResult decCheckCRCValue(XBinary::CRC_TYPE crcType, const QVariant &value,
                                     QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct,
                                     const XBinary::DATAPROCESS_STATE *pState)
{
    if (crcType == XBinary::CRC_TYPE_UNKNOWN) return DecCRCResult::Ok;

    // DATAPROCESS_STATE is caller-owned.  Snapshot every value needed below
    // before a device operation or progress callback can invalidate it.
    bool bRar5HashMac = false;
    bool bHasResultCRC = false;
    QString sPassword;
    QByteArray baAESKeyProperties;
    if (pState) {
        bRar5HashMac = pState->mapProperties.value(XBinary::FPART_PROP_RAR5_HASHMAC, false).toBool();
        bHasResultCRC = pState->mapProperties.contains(XBinary::FPART_PROP_RESULTCRC);
        sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        baAESKeyProperties = pState->mapProperties.value(XBinary::FPART_PROP_AESKEY).toByteArray();
    }

    QPointer<QIODevice> guardedDevice(pDevice);
    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct) : XBinary::PDSTRUCTLIFETIME();
    const auto contextAlive = [&]() {
        return guardedDevice && decProgressAlive(pPdStruct, progressLifetime);
    };

    if (!guardedDevice) return DecCRCResult::NotReadable;
    const bool bReadable = guardedDevice->isReadable();
    if (!contextAlive()) return DecCRCResult::Aborted;
    if (!bReadable) return DecCRCResult::NotReadable;

    const bool bSeeked = guardedDevice->seek(0);
    if (!contextAlive()) return DecCRCResult::Aborted;
    if (!bSeeked) return DecCRCResult::SeekError;

    bool bResult = false;
    if (bRar5HashMac) {
        if ((crcType == XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF) &&
            bHasResultCRC && !sPassword.isEmpty() &&
            (baAESKeyProperties.size() >= 33)) {
            const quint32 nCRC32 =
                XBinary::_getCRC32(guardedDevice.data(), 0xFFFFFFFF,
                                   XBinary::_getCRC32Table_EDB88320(), pPdStruct) ^
                0xFFFFFFFF;
            if (!contextAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return DecCRCResult::Aborted;
            }

            quint32 nMAC = 0;
            const bool bMACCalculated =
                XAESDecoder::calculateRar5CRC32MAC(sPassword, baAESKeyProperties,
                                                   nCRC32, &nMAC, pPdStruct);
            if (!contextAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return DecCRCResult::Aborted;
            }
            bResult = bMACCalculated && (nMAC == value.toUInt());
        }
    } else {
        bResult = XBinary::checkCRC(guardedDevice.data(), crcType, value, pPdStruct);
        if (!contextAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return DecCRCResult::Aborted;
        }
    }

    const bool bReset = guardedDevice->seek(0);
    if (!contextAlive()) return DecCRCResult::Aborted;
    if (!bReset) return DecCRCResult::SeekError;

    return bResult ? DecCRCResult::Ok : DecCRCResult::Invalid;
}

static QString decCRCResultMessage(DecCRCResult result)
{
    if (result == DecCRCResult::NotReadable) {
        return QCoreApplication::translate("XDecompress", "CRC check requires a readable output device");
    }
    if (result == DecCRCResult::SeekError) {
        return QCoreApplication::translate("XDecompress", "Cannot seek output for CRC check");
    }
    if (result == DecCRCResult::Invalid) {
        return QCoreApplication::translate("XDecompress", "Invalid CRC");
    }
    return QString();
}

static bool decCheckCRCQuiet(XBinary::CRC_TYPE crcType, const QVariant &value,
                             QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct,
                             const XBinary::DATAPROCESS_STATE *pState)
{
    const XBinary::PDSTRUCTLIFETIME originalLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct) : XBinary::PDSTRUCTLIFETIME();
    DecNestedProgressBridge bridge = {};
    XBinary::PDSTRUCT localProgress = XBinary::getPdStructSnapshot(pPdStruct);
    decPrepareNestedProgress(&localProgress, pPdStruct, &bridge);
    const DecCRCResult result = decCheckCRCValue(crcType, value, pDevice,
                                                 &localProgress, pState);

    if (!decProgressAlive(pPdStruct, originalLifetime) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if ((result != DecCRCResult::Ok) && (result != DecCRCResult::Aborted)) {
        XBinary::setPdStructErrorString(pPdStruct, decCRCResultMessage(result));
    }
    return result == DecCRCResult::Ok;
}

static thread_local qint32 g_nDecSignalSuppressionDepth = 0;

class DecSignalSuppressionGuard {
public:
    DecSignalSuppressionGuard() { ++g_nDecSignalSuppressionDepth; }
    ~DecSignalSuppressionGuard() { --g_nDecSignalSuppressionDepth; }
};

// Decoder backends and progress callbacks are caller-controlled re-entrancy
// points.  Never keep mutating the caller's raw DATAPROCESS_STATE across one
// of those calls.  Work on a value copy and publish only the documented result
// counters/flags while both the decoder and progress owner are still alive.
class DecProcessStateTransaction {
public:
    DecProcessStateTransaction(XDecompress *pOwner,
                               XBinary::DATAPROCESS_STATE *pCallerState,
                               XBinary::PDSTRUCT *pPdStruct)
        : m_pOwner(pOwner),
          m_pCallerState(pCallerState),
          m_state(pCallerState ? *pCallerState
                               : XBinary::DATAPROCESS_STATE()),
          m_pPdStruct(pPdStruct),
          m_progressLifetime(
              pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct)
                        : XBinary::PDSTRUCTLIFETIME())
    {
    }

    ~DecProcessStateTransaction()
    {
        if (!isAlive() || !m_pCallerState) return;
        m_pCallerState->bReadError = m_state.bReadError;
        m_pCallerState->bWriteError = m_state.bWriteError;
        m_pCallerState->nCountInput = m_state.nCountInput;
        m_pCallerState->nCountOutput = m_state.nCountOutput;
    }

    XBinary::DATAPROCESS_STATE *state() { return &m_state; }
    const XBinary::PDSTRUCTLIFETIME &progressLifetime() const
    {
        return m_progressLifetime;
    }

    bool isAlive() const
    {
        return m_pOwner &&
               (!m_pPdStruct || XBinary::isPdStructLifetimeAlive(
                                     m_progressLifetime));
    }

private:
    Q_DISABLE_COPY(DecProcessStateTransaction)
    QPointer<XDecompress> m_pOwner;
    XBinary::DATAPROCESS_STATE *m_pCallerState;
    XBinary::DATAPROCESS_STATE m_state;
    XBinary::PDSTRUCT *m_pPdStruct;
    XBinary::PDSTRUCTLIFETIME m_progressLifetime;
};

// True for the record shape XBinary::markArchiveStreamRecord() publishes: a
// member of a private decoded stream, addressable only through its owning
// archive session (XBinary::_unpackRecordByIndex) and never by coordinates.
// Both the pseudo-method and the bare presence of the logical index are
// enough, because FPART_PROP_ARCHIVE_RECORD_INDEX is written by exactly one
// function and nothing else: a caller that forges the method field but leaves
// the index in place is still holding an archive-stream record.
static bool decIsArchiveStreamProperties(
    const QMap<XBinary::FPART_PROP, QVariant> &mapProperties)
{
    if (mapProperties.contains(XBinary::FPART_PROP_ARCHIVE_RECORD_INDEX) ||
        mapProperties.contains(XBinary::FPART_PROP_ARCHIVE_RECORD_TOKEN)) {
        return true;
    }

    const XBinary::FPART_PROP arrMethodProps[] = {
        XBinary::FPART_PROP_HANDLEMETHOD, XBinary::FPART_PROP_HANDLEMETHOD2,
        XBinary::FPART_PROP_HANDLEMETHOD3, XBinary::FPART_PROP_HANDLEMETHOD4};

    for (size_t i = 0; i < (sizeof(arrMethodProps) / sizeof(arrMethodProps[0]));
         i++) {
        if (!mapProperties.contains(arrMethodProps[i])) continue;
        bool bOk = false;
        const qint64 nMethod =
            mapProperties.value(arrMethodProps[i]).toLongLong(&bOk);
        if (bOk && (nMethod == (qint64)XBinary::HANDLE_METHOD_ARCHIVE_STREAM)) {
            return true;
        }
    }

    return false;
}

static bool decPrepareBoundedInput(QIODevice *pDevice, qint64 nOffset, qint64 nLimit, qint64 *pnEffectiveLimit)
{
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if (!pDevice || !pnEffectiveLimit || (nOffset < 0) || (nLimit < -1) || ((nLimit != -1) && (nOffset > (nMax - nLimit)))) {
        return false;
    }

    QPointer<QIODevice> guardedDevice(pDevice);
    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedDevice) return false;
    if (nLimit == -1) {
        if ((nDeviceSize < 0) || (nOffset > nDeviceSize)) {
            return false;
        }
        nLimit = nDeviceSize - nOffset;
    } else {
        const bool bSequential = guardedDevice->isSequential();
        if (!guardedDevice) return false;
        if (!bSequential && (nDeviceSize >= 0) &&
            ((nOffset > nDeviceSize) ||
             (nLimit > (nDeviceSize - nOffset)))) {
            return false;
        }
    }

    const bool bSeeked = guardedDevice->seek(nOffset);
    if (!guardedDevice) return false;
    if (!bSeeked) {
        const qint64 nPosition = guardedDevice->pos();
        if (!guardedDevice || (nPosition != nOffset)) return false;
    }

    if (!guardedDevice) {
        return false;
    }

    *pnEffectiveLimit = nLimit;
    return true;
}
}  // namespace

XDecompress::XDecompress(QObject *parent) : QObject(parent)
{
    m_pCurrentSolidDevice = nullptr;
    m_pRarUnpacker = nullptr;
    m_nRarSolidIndex = 0;
}

// A decompressed size is usable as a QByteArray length only if it is non-negative
// and fits in the qint32 that QByteArray::resize takes.
static bool decIsValidBufferSize(qint64 nSize)
{
    return (nSize >= 0) && (nSize <= (std::numeric_limits<qint32>::max)());
}

static const qint64 DEC_CAB_MAX_FOLDER_SIZE = 512LL * 1024 * 1024;
static const quint16 DEC_CAB_MAX_DATA_BLOCK_SIZE = 0x9800;
static const quint16 DEC_KWAJ_MSZIP_MAX_BLOCK_SIZE = 32780;

static quint32 decCabDataChecksum(const char *pData, qint32 nSize,
                                  quint32 nSeed = 0)
{
    if ((nSize < 0) || ((nSize > 0) && !pData)) return nSeed;

    quint32 nResult = nSeed;
    while (nSize >= 4) {
        nResult ^= (quint32)(quint8)pData[0] |
                   ((quint32)(quint8)pData[1] << 8) |
                   ((quint32)(quint8)pData[2] << 16) |
                   ((quint32)(quint8)pData[3] << 24);
        pData += 4;
        nSize -= 4;
    }

    quint32 nTail = 0;
    if (nSize == 3) nTail |= (quint32)(quint8)*pData++ << 16;
    if (nSize >= 2) nTail |= (quint32)(quint8)*pData++ << 8;
    if (nSize >= 1) nTail |= (quint32)(quint8)*pData;
    return nResult ^ nTail;
}

static bool decReadExactAt(QIODevice *pDevice, qint64 nOffset, char *pData,
                           qint64 nSize,
                           XBinary::DATAPROCESS_STATE *pState,
                           XBinary::PDSTRUCT *pPdStruct,
                           qint64 *pnConsumed = nullptr)
{
    if (!pDevice || !pState || (nOffset < 0) || (nSize < 0) ||
        ((nSize > 0) && !pData)) {
        if (pState) pState->bReadError = true;
        return false;
    }

    QPointer<QIODevice> guardedDevice(pDevice);
    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct)
                  : XBinary::PDSTRUCTLIFETIME();
    const auto isProgressAlive = [&]() -> bool {
        return !pPdStruct ||
               XBinary::isPdStructLifetimeAlive(progressLifetime);
    };
    const bool bSeeked = guardedDevice->seek(nOffset);
    if (!guardedDevice || !isProgressAlive()) return false;
    if (!bSeeked) {
        const qint64 nPosition = guardedDevice->pos();
        if (!guardedDevice || !isProgressAlive() ||
            (nPosition != nOffset)) {
            pState->bReadError = true;
            return false;
        }
    }

    const auto addConsumed = [pnConsumed, pState](qint64 nAmount) -> bool {
        if (!pnConsumed) return true;
        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if ((*pnConsumed < 0) || (nAmount < 0) ||
            (nAmount > nMax - *pnConsumed)) {
            pState->bReadError = true;
            return false;
        }
        *pnConsumed += nAmount;
        return true;
    };

    qint64 nReadTotal = 0;
    while ((nReadTotal < nSize) && isProgressAlive() &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRead = guardedDevice->read(
            pData + nReadTotal, nSize - nReadTotal);
        if (!guardedDevice || !isProgressAlive()) return false;
        if ((nRead <= 0) || (nRead > nSize - nReadTotal)) {
            pState->bReadError = true;
            addConsumed(nReadTotal);
            return false;
        }
        nReadTotal += nRead;
    }

    if (!isProgressAlive()) return false;
    if ((nReadTotal != nSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        addConsumed(nReadTotal);
        return false;
    }
    return addConsumed(nReadTotal);
}

static bool decEmitByteArray(const QByteArray &baData, qint64 nDataOffset,
                             qint64 nDataSize,
                             XBinary::DATAPROCESS_STATE *pState,
                             XBinary::PDSTRUCT *pPdStruct)
{
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if (!pState || !pState->pDeviceOutput || (nDataOffset < 0) ||
        (nDataSize < 0) || (nDataOffset > baData.size()) ||
        (nDataSize > (qint64)baData.size() - nDataOffset) ||
        (pState->nProcessedOffset < 0) ||
        (pState->nProcessedLimit < -1) ||
        ((pState->nProcessedLimit != -1) &&
         (pState->nProcessedOffset > nMax - pState->nProcessedLimit))) {
        if (pState) pState->bWriteError = true;
        return false;
    }

    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct)
                  : XBinary::PDSTRUCTLIFETIME();
    const auto isProgressAlive = [&]() -> bool {
        return !pPdStruct ||
               XBinary::isPdStructLifetimeAlive(progressLifetime);
    };

    qint64 nOffset = 0;
    while ((nOffset < nDataSize) &&
           isProgressAlive() &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = (qint32)(std::min)(
            nDataSize - nOffset, (qint64)0x10000);
        const qint32 nWritten = XBinary::_writeDevice(
            baData.constData() + nDataOffset + nOffset, nChunk, pState);
        if (!isProgressAlive() || (nWritten != nChunk)) {
            return false;
        }
        nOffset += nChunk;
    }

    return (nOffset == nDataSize) &&
           isProgressAlive() &&
           XBinary::isPdStructNotCanceled(pPdStruct) &&
           !pState->bWriteError;
}

static bool decGetBranchStartOffset(const QByteArray &baProperty, quint32 *pnStartOffset)
{
    if (!pnStartOffset) return false;
    *pnStartOffset = 0;
    if (baProperty.isEmpty()) return true;
    if (baProperty.size() != 4) return false;

    *pnStartOffset = (quint32)(quint8)baProperty.at(0) | ((quint32)(quint8)baProperty.at(1) << 8) |
                     ((quint32)(quint8)baProperty.at(2) << 16) | ((quint32)(quint8)baProperty.at(3) << 24);
    return true;
}

static bool decReadInputToByteArray(
    XBinary::DATAPROCESS_STATE *pState, QByteArray *pData,
    XBinary::UNPACK_MEMORY_RESERVATION *pReservation)
{
    if (!pState || !pState->pDeviceInput || !pData || !pReservation) {
        return false;
    }
    QPointer<QIODevice> guardedInput(pState->pDeviceInput);
    if (!guardedInput) return false;

    qint64 nSize = pState->nInputLimit;
    if (nSize == -1) {
        const qint64 nDeviceSize = guardedInput->size();
        if (!guardedInput) return false;
        if ((nDeviceSize < 0) || (pState->nInputOffset < 0) || (pState->nInputOffset > nDeviceSize)) {
            pState->bReadError = true;
            return false;
        }
        nSize = nDeviceSize - pState->nInputOffset;
    }
    if (!decIsValidBufferSize(nSize)) {
        pState->bReadError = true;
        return false;
    }
    if (!XBinary::isUnpackOutputSizeAllowed(
            pState->mapUnpackProperties, nSize)) {
        return false;
    }
    if (!pReservation->acquire(pState->mapUnpackProperties, nSize)) {
        return false;
    }

    pData->resize((qint32)nSize);
    qint64 nReadTotal = 0;
    while (nReadTotal < nSize) {
        const qint64 nRead = guardedInput->read(
            pData->data() + nReadTotal, nSize - nReadTotal);
        if (!guardedInput || (nRead <= 0) ||
            (nRead > (nSize - nReadTotal))) {
            pState->bReadError = true;
            pData->clear();
            return false;
        }
        nReadTotal += nRead;
    }

    pState->nCountInput = nReadTotal;
    return true;
}

// Decoders normally seek to offset zero before writing, but seeking alone does
// not remove stale bytes when the new result is empty.  Clear random-access
// devices explicitly; a sequential device is usable only if no bytes have
// already been written to it.
static bool decClearOutputDevice(QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return false;

    const bool bSequential = guardedDevice->isSequential();
    if (!guardedDevice) return false;
    if (bSequential) {
        const qint64 nPosition = guardedDevice->pos();
        return guardedDevice && (nPosition == 0);
    }

    const bool bSeeked = guardedDevice->seek(0);
    if (!guardedDevice || !bSeeked) return false;

    const qint64 nSize = guardedDevice->size();
    if (!guardedDevice) return false;
    return (nSize == 0) ||
           (XBinary::resize(guardedDevice.data(), 0) && guardedDevice);
}

// Copy a complete logical result through XBinary's processed-output window.
// The source is always consumed in full so nCountOutput continues to describe
// the complete decoded stream, while only the requested slice reaches the
// caller's device.
static bool decEmitDevice(QIODevice *pSource, qint64 nOffset, qint64 nSize, XBinary::DATAPROCESS_STATE *pState,
                          XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSource || !pState || (nOffset < 0) || (nSize < 0) || !pState->pDeviceOutput) {
        if (pState) pState->bWriteError = true;
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<QIODevice> guardedSource(pSource);
    QPointer<QIODevice> guardedOutput(pState->pDeviceOutput);
    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct)
                  : XBinary::PDSTRUCTLIFETIME();
    const auto isProgressAlive = [&]() -> bool {
        return !pPdStruct ||
               XBinary::isPdStructLifetimeAlive(progressLifetime);
    };
    if (!guardedSource || !guardedOutput || !isProgressAlive()) return false;

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountOutput = 0;

    const bool bOutputCleared = decClearOutputDevice(guardedOutput.data());
    if (!guardedOutput || !guardedSource || !isProgressAlive()) return false;
    if (!bOutputCleared) {
        pState->bWriteError = true;
        return false;
    }
    const bool bSourceSeeked = guardedSource->seek(nOffset);
    if (!guardedSource || !guardedOutput || !isProgressAlive()) return false;
    if (!bSourceSeeked) {
        pState->bReadError = true;
        return false;
    }

    static const qint32 COPY_BUFFER_SIZE = 0x10000;
    std::unique_ptr<char[]> pBuffer(new (std::nothrow) char[COPY_BUFFER_SIZE]);
    if (!pBuffer) {
        pState->bWriteError = true;
        return false;
    }

    qint64 nReadTotal = 0;
    while ((nReadTotal < nSize) && guardedSource && guardedOutput &&
           isProgressAlive() && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint32 nRequest = (qint32)(std::min)(nSize - nReadTotal, (qint64)COPY_BUFFER_SIZE);
        const qint64 nRead = guardedSource->read(pBuffer.get(), nRequest);
        if (!guardedSource || !guardedOutput || !isProgressAlive())
            return false;
        if ((nRead <= 0) || (nRead > nRequest)) {
            pState->bReadError = true;
            break;
        }
        const qint32 nWritten =
            XBinary::_writeDevice(pBuffer.get(), (qint32)nRead, pState);
        if (!guardedSource || !guardedOutput || !isProgressAlive() ||
            (nWritten != (qint32)nRead)) {
            break;
        }
        nReadTotal += nRead;
    }

    return guardedSource && guardedOutput && isProgressAlive() &&
           (nReadTotal == nSize) && (pState->nCountOutput == nSize) &&
           !pState->bReadError && !pState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}

class DecInputStateGuard {
public:
    explicit DecInputStateGuard(XBinary::DATAPROCESS_STATE *pState)
        : m_pState(pState), m_pDevice(pState ? pState->pDeviceInput : nullptr), m_nOffset(pState ? pState->nInputOffset : 0),
          m_nLimit(pState ? pState->nInputLimit : 0)
    {
    }

    ~DecInputStateGuard()
    {
        if (m_pState) {
            m_pState->pDeviceInput = m_pDevice;
            m_pState->nInputOffset = m_nOffset;
            m_pState->nInputLimit = m_nLimit;
        }
    }

    void dismiss() { m_pState = nullptr; }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    QIODevice *m_pDevice;
    qint64 m_nOffset;
    qint64 m_nLimit;
};

// Each MSZIP block is a fresh raw-DEFLATE stream, but blocks after the first
// inherit the previous 32 KiB as their dictionary.  The bundled inflater has
// no inflateSetDictionary entry point, so feed that history through a non-final
// stored block and strip it from the decoded result.  CAB supplies an exact
// block output size; KWAJ passes -1 and validates the actual 1..32768 result.
static bool decInflateMSZIPBlock(const QByteArray &baPayload, const QByteArray &baHistory, qint32 nExpectedSize, QByteArray *pbaResult,
                                 XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || (baPayload.size() < 2) || (baPayload.at(0) != 'C') || (baPayload.at(1) != 'K') || (nExpectedSize < -1) ||
        (nExpectedSize > 32768)) {
        return false;
    }

    qint32 nDictionarySize = qMin(32768, baHistory.size());
    QByteArray baInput;

    if (nDictionarySize > 0) {
        baInput.resize(5 + nDictionarySize);
        quint16 nLength = (quint16)nDictionarySize;
        quint16 nInverseLength = (quint16)~nLength;
        baInput[0] = 0;  // BFINAL=0, BTYPE=stored, then byte alignment.
        baInput[1] = (char)(nLength & 0xFF);
        baInput[2] = (char)((nLength >> 8) & 0xFF);
        baInput[3] = (char)(nInverseLength & 0xFF);
        baInput[4] = (char)((nInverseLength >> 8) & 0xFF);
        memcpy(baInput.data() + 5, baHistory.constData() + baHistory.size() - nDictionarySize, nDictionarySize);
    }

    baInput.append(baPayload.constData() + 2, baPayload.size() - 2);

    QBuffer inputBuffer(&baInput);
    QByteArray baDecoded;
    QBuffer outputBuffer(&baDecoded);
    if (!inputBuffer.open(QIODevice::ReadOnly) || !outputBuffer.open(QIODevice::WriteOnly)) {
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &inputBuffer;
    state.pDeviceOutput = &outputBuffer;
    state.nInputOffset = 0;
    state.nInputLimit = baInput.size();
    state.nProcessedOffset = 0;
    const qint32 nMaximumBlockOutput =
        (nExpectedSize == -1) ? 32768 : nExpectedSize;
    const qint64 nMaximumDecodedSize =
        (qint64)nDictionarySize + nMaximumBlockOutput;
    state.nProcessedLimit = nMaximumDecodedSize;
    state.mapUnpackProperties.insert(
        XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE, nMaximumDecodedSize);

    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct)
                  : XBinary::PDSTRUCTLIFETIME();
    bool bResult = XDeflateDecoder::decompress(&state, pPdStruct);
    inputBuffer.close();
    outputBuffer.close();

    const qint64 nDecodedBlockSize =
        state.nCountOutput - nDictionarySize;
    const bool bOutputSizeValid =
        (nExpectedSize == -1)
            ? ((nDecodedBlockSize >= 0) && (nDecodedBlockSize <= 32768))
            : (nDecodedBlockSize == nExpectedSize);
    if ((pPdStruct &&
         !XBinary::isPdStructLifetimeAlive(progressLifetime)) ||
        !bResult || state.bReadError || state.bWriteError ||
        (state.nCountInput != baInput.size()) ||
        !bOutputSizeValid || (state.nCountOutput < nDictionarySize) ||
        (baDecoded.size() != state.nCountOutput) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pbaResult = baDecoded.mid(nDictionarySize);
    return (nExpectedSize == -1)
               ? (pbaResult->size() == nDecodedBlockSize)
               : (pbaResult->size() == nExpectedSize);
}

XDecompress::~XDecompress()
{
    clearSolidCache();
}

bool XDecompress::decompressFPART(const XBinary::FPART &fPart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput, XBinary::PDSTRUCT *pPdStruct)
{
    return decompressFPART(fPart, pDeviceInput, pDeviceOutput,
                           QMap<XBinary::UNPACK_PROP, QVariant>(), pPdStruct);
}

bool XDecompress::decompressFPART(const XBinary::FPART &fPart, QIODevice *pDeviceInput, QIODevice *pDeviceOutput,
                                  const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct)
{
    // Same refusal as decompressArchiveRecord(): a part carrying the
    // archive-stream contract has no coordinates that mean anything here, and
    // a negative extent is not a disarmed extent - decPrepareBoundedInput()
    // reads a -1 limit as "to the end of the device".
    if (decIsArchiveStreamProperties(fPart.mapProperties) ||
        (fPart.nFileOffset < 0) || (fPart.nFileSize < 0)) {
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties = fPart.mapProperties;
    state.mapUnpackProperties = mapUnpackProperties;
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = fPart.nFileOffset;
    state.nInputLimit = fPart.nFileSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    return multiDecompress(&state, pPdStruct);
}

bool XDecompress::decompressArchiveRecord(const XBinary::ARCHIVERECORD &archiveRecord, QIODevice *pDeviceInput, QIODevice *pDeviceOutput,
                                          const QMap<XBinary::UNPACK_PROP, QVariant> &mapUnpackProperties, XBinary::PDSTRUCT *pPdStruct)
{
    // This is the ARCHIVERECORD-native decode entry point, and it is reachable
    // from shipping callers (XFormats::extractArchiveRecordsToFolder,
    // XArchive::unpackCurrent).  An index-paired archive-stream record must be
    // refused here exactly as it is on the legacy RECORD route: its member is
    // only reachable through its owning archive session
    // (XArchive::unpackArchiveStreamRecord), and any coordinates that reach
    // this function address the raw container instead of the member.
    qint32 nArchiveStreamIndex = -1;
    if (XBinary::getArchiveStreamRecordIndex(archiveRecord,
                                             &nArchiveStreamIndex) ||
        decIsArchiveStreamProperties(archiveRecord.mapProperties)) {
        return false;
    }

    // ARCHIVE_STREAM_NO_EXTENT is -1 on both axes, and -1 is not a disarmed
    // value: decPrepareBoundedInput() reads a -1 limit as "to the end of the
    // device", which is how a no-extent record once leaked a whole decoded
    // archive.  Neither axis may be negative here.
    if ((archiveRecord.nStreamOffset < 0) || (archiveRecord.nStreamSize < 0)) {
        return false;
    }

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(mapUnpackProperties, &nOutputLimit)) {
        XBinary::setPdStructErrorString(pPdStruct,
                                       tr("Invalid unpacked-output limit"));
        return false;
    }
    if (archiveRecord.mapProperties.contains(
            XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        const qint64 nDeclaredSize = archiveRecord.mapProperties
            .value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
        if (!XBinary::isUnpackOutputSizeAllowed(mapUnpackProperties,
                                                nDeclaredSize)) {
            XBinary::setPdStructErrorString(
                pPdStruct,
                tr("Unpacked output exceeds the configured limit"));
            return false;
        }
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties = archiveRecord.mapProperties;
    state.mapUnpackProperties = mapUnpackProperties;
    state.pDeviceInput = pDeviceInput;
    state.pDeviceOutput = pDeviceOutput;
    state.nInputOffset = archiveRecord.nStreamOffset;
    state.nInputLimit = archiveRecord.nStreamSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = nOutputLimit;

    return multiDecompress(&state, pPdStruct);
}

void XDecompress::clearSolidCache()
{
    QList<QString> listKeys = m_mapSolidCache.keys();
    for (qint32 i = 0; i < listKeys.count(); i++) {
        QIODevice *pDevice = m_mapSolidCache.value(listKeys.at(i));
        XBinary::freeFileBuffer(&pDevice);
    }
    m_mapSolidCache.clear();

    delete m_pRarUnpacker;
    m_pRarUnpacker = nullptr;
    m_nRarSolidIndex = 0;
    m_pCurrentSolidDevice = nullptr;
    m_sCurrentArchiveIdentity.clear();
}

bool XDecompress::decompressRarSolid(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    DecProcessStateTransaction stateTransaction(this, pState, pPdStruct);
    pState = stateTransaction.state();
    bool bResult = false;

    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput) {
        return false;
    }

    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct) : XBinary::PDSTRUCTLIFETIME();
    QPointer<QIODevice> guardedInput(pState->pDeviceInput);
    QPointer<QIODevice> guardedOutput(pState->pDeviceOutput);
    if (!guardedInput || !guardedOutput || !stateTransaction.isAlive())
        return false;

    qint64 nSolidFolderIndex = pState->mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)-1).toLongLong();
    // Names are not record identities: a valid archive may contain duplicate
    // names.  Include the solid folder and exact packed-stream region so a
    // duplicate name cannot return another record's cached bytes or skip the
    // decoder step required to advance solid state.
    QString sCacheKey = QString("rar_%1_%2_%3_%4")
                            .arg(QString::number((qulonglong)(quintptr)pState->pDeviceInput, 16))
                            .arg(nSolidFolderIndex)
                            .arg(pState->nInputOffset)
                            .arg(pState->nInputLimit);

    // If the requested file is not yet cached, decompress it using a persistent rar_Unpack
    // instance that maintains decoder dictionary state across sequential solid files.
    if (!m_mapSolidCache.contains(sCacheKey)) {
        bool bCacheCreated = false;
        bool bRarDecodeAttempted = false;
        qint64 nConsumedInput = 0;
        XBinary::HANDLE_METHOD compressMethod =
            (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
        qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();
        qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
        qint64 nConfiguredOutputLimit = -1;
        if (!XBinary::getUnpackOutputLimit(
                pState->mapUnpackProperties, &nConfiguredOutputLimit) ||
            !XBinary::isUnpackOutputSizeAllowed(
                pState->mapUnpackProperties, nUncompressedSize) ||
            ((nConfiguredOutputLimit >= 0) && (nWindowSize > 0) &&
             (nWindowSize > nConfiguredOutputLimit))) {
            return false;
        }

        // For encrypted RAR5: decrypt first, then use the inner compression method
        QIODevice *pDecryptedDevice = nullptr;
        QIODevice *pInputDevice = pState->pDeviceInput;
        qint64 nInputOffset = pState->nInputOffset;
        qint64 nInputLimit = pState->nInputLimit;
        bool bInputReady = true;

        XBinary::HANDLE_METHOD outerMethod =
            (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD2, XBinary::HANDLE_METHOD_UNKNOWN).toUInt();
        if (outerMethod == XBinary::HANDLE_METHOD_RAR5_AES) {
            bInputReady = false;
            // Decrypt the encrypted data into a temporary buffer
            QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
            qint64 nEncryptedSize = pState->nInputLimit;

            // Align to AES block size
            if (nEncryptedSize > 0 &&
                (nEncryptedSize % AES_BLOCK_SIZE) == 0 &&
                XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nEncryptedSize)) {
                pDecryptedDevice = XBinary::createUnpackFileBuffer(
                    nEncryptedSize, pState->mapUnpackProperties, pPdStruct);
                if (!stateTransaction.isAlive() || !guardedInput ||
                    !guardedOutput) {
                    XBinary::freeFileBuffer(&pDecryptedDevice);
                    return false;
                }
                if (pDecryptedDevice) {
                    // Seek to the encrypted data offset before reading
                    const bool bEncryptedInputSeeked =
                        guardedInput->seek(pState->nInputOffset);
                    if (!stateTransaction.isAlive() || !guardedInput ||
                        !guardedOutput || !bEncryptedInputSeeked) {
                        XBinary::freeFileBuffer(&pDecryptedDevice);
                        return false;
                    }

                    XBinary::DATAPROCESS_STATE decryptState = *pState;
                    decryptState.pDeviceOutput = pDecryptedDevice;
                    decryptState.nCountInput = 0;
                    decryptState.nCountOutput = 0;
                    decryptState.nProcessedOffset = 0;
                    decryptState.nProcessedLimit = -1;

                    const bool bDecrypted = XAESDecoder::decryptRar5(
                        &decryptState, sPassword, pPdStruct);
                    if (!stateTransaction.isAlive() || !guardedInput ||
                        !guardedOutput) {
                        XBinary::freeFileBuffer(&pDecryptedDevice);
                        return false;
                    }
                    if (bDecrypted) {
                        pInputDevice = pDecryptedDevice;
                        nInputOffset = 0;
                        nInputLimit = decryptState.nCountOutput;
                        bInputReady = true;
                    } else {
                        XBinary::freeFileBuffer(&pDecryptedDevice);
                        pDecryptedDevice = nullptr;
                    }
                }
            }
        }

        // For solid archives: first file is not solid (bIsSolid=false), subsequent files are solid (bIsSolid=true)
        bool bIsSolid = (m_nRarSolidIndex > 0);

        if (bInputReady && (nUncompressedSize >= 0)) {
            QIODevice *pBuffer = XBinary::createUnpackFileBuffer(
                nUncompressedSize, pState->mapUnpackProperties, pPdStruct);
            if (!stateTransaction.isAlive() || !guardedInput ||
                !guardedOutput) {
                XBinary::freeFileBuffer(&pBuffer);
                XBinary::freeFileBuffer(&pDecryptedDevice);
                return false;
            }

            if (pBuffer) {
                bool bDecompressOk = false;

                if (compressMethod == XBinary::HANDLE_METHOD_STORE) {
                    // STORE: copy data directly, decoder state is unaffected
                    qint64 nStoreSize = qMin(qMax((qint64)0, nInputLimit), nUncompressedSize);
                    DecNestedProgressBridge storeBridge = {};
                    XBinary::PDSTRUCT storeProgress = XBinary::getPdStructSnapshot(pPdStruct);
                    decPrepareNestedProgress(&storeProgress, pPdStruct, &storeBridge);
                    bDecompressOk = (nStoreSize == nUncompressedSize) &&
                                    XBinary::copyDeviceMemory(pInputDevice, nInputOffset, pBuffer, 0, nStoreSize, &storeProgress) &&
                                    decProgressAlive(pPdStruct, progressLifetime) &&
                                    XBinary::isPdStructNotCanceled(pPdStruct);
                    if (bDecompressOk) nConsumedInput = nStoreSize;
                } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_15) || (compressMethod == XBinary::HANDLE_METHOD_RAR_20) ||
                           (compressMethod == XBinary::HANDLE_METHOD_RAR_29) || (compressMethod == XBinary::HANDLE_METHOD_RAR_50) ||
                           (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                    if (!m_pRarUnpacker) {
                        m_pRarUnpacker = new rar_Unpack();
                    }

                    qint64 nEffectiveInputLimit = 0;
                    if (m_pRarUnpacker && (nWindowSize >= 0) &&
                        decPrepareBoundedInput(pInputDevice, nInputOffset, nInputLimit, &nEffectiveInputLimit) && decClearOutputDevice(pBuffer)) {
                        DecBoundedReadDevice inputDevice(pInputDevice, nEffectiveInputLimit);
                        XBinary::DATAPROCESS_STATE cacheOutputState = {};
                        cacheOutputState.pDeviceOutput = pBuffer;
                        cacheOutputState.nProcessedOffset = 0;
                        cacheOutputState.nProcessedLimit = -1;
                        cacheOutputState.mapUnpackProperties =
                            pState->mapUnpackProperties;
                        DecWindowWriteDevice outputDevice(&cacheOutputState);

                        if (inputDevice.open(QIODevice::ReadOnly) && outputDevice.open(QIODevice::WriteOnly)) {
                            bRarDecodeAttempted = true;
                            m_pRarUnpacker->setDevices(&inputDevice, &outputDevice);
                            qint32 nInit = m_pRarUnpacker->Init(nWindowSize, bIsSolid);

                            if (nInit > 0) {
                                m_pRarUnpacker->SetDestSize(nUncompressedSize);
                                DecNestedProgressBridge rarBridge = {};
                                XBinary::PDSTRUCT rarProgress =
                                    XBinary::getPdStructSnapshot(pPdStruct);
                                decPrepareNestedProgress(&rarProgress,
                                                         pPdStruct,
                                                         &rarBridge);

                                if (compressMethod == XBinary::HANDLE_METHOD_RAR_15) {
                                    m_pRarUnpacker->Unpack15(bIsSolid,
                                                             &rarProgress);
                                } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_20) {
                                    m_pRarUnpacker->Unpack20(bIsSolid,
                                                             &rarProgress);
                                } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_29) {
                                    m_pRarUnpacker->Unpack29(bIsSolid,
                                                             &rarProgress);
                                } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_50) || (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                                    m_pRarUnpacker->Unpack5(bIsSolid,
                                                            &rarProgress);
                                }

                                if (!stateTransaction.isAlive() ||
                                    !guardedInput || !guardedOutput) {
                                    outputDevice.close();
                                    inputDevice.close();
                                    XBinary::freeFileBuffer(&pBuffer);
                                    XBinary::freeFileBuffer(
                                        &pDecryptedDevice);
                                    return false;
                                }

                                bDecompressOk = m_pRarUnpacker->IsFileExtracted() && XBinary::isPdStructNotCanceled(pPdStruct) &&
                                                !inputDevice.hasError() && !outputDevice.hasError() && !cacheOutputState.bWriteError &&
                                                (cacheOutputState.nCountOutput == nUncompressedSize);
                                if (bDecompressOk) nConsumedInput = inputDevice.consumed();
                            }

                            outputDevice.close();
                            inputDevice.close();
                        } else {
                            outputDevice.close();
                            inputDevice.close();
                        }
                    }
                }

                if (bDecompressOk) {
                    pBuffer->setProperty("RAR_INPUT_CONSUMED", nConsumedInput);
                    m_mapSolidCache.insert(sCacheKey, pBuffer);
                    bCacheCreated = true;
                } else {
                    XBinary::freeFileBuffer(&pBuffer);
                }
            }
        }

        // Clean up decrypted device if we created one
        XBinary::freeFileBuffer(&pDecryptedDevice);

        if (!bCacheCreated) {
            if (bRarDecodeAttempted) {
                // A failed solid decode can leave the persistent dictionary in
                // an indeterminate state.  Drop it and its dependent cache.
                clearSolidCache();
            }
            return false;
        }

        m_nRarSolidIndex++;
    }

    // Retrieve the requested file from cache
    if (m_mapSolidCache.contains(sCacheKey)) {
        QIODevice *pCachedDevice = m_mapSolidCache.value(sCacheKey);
        qint64 nDecompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();

        pState->bReadError = false;
        pState->bWriteError = false;
        pState->nCountInput = 0;
        pState->nCountOutput = 0;

        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if (!pCachedDevice || (nDecompressedSize < 0) || (pCachedDevice->size() != nDecompressedSize) || (pState->nProcessedOffset < 0) ||
            (pState->nProcessedLimit < -1) ||
            ((pState->nProcessedLimit != -1) && (pState->nProcessedOffset > (nMax - pState->nProcessedLimit)))) {
            return false;
        }

        // Validate the full cached file before applying an output window; a CRC
        // over only the requested slice would reject a valid record.
        XBinary::CRC_TYPE crcType =
            (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
        if (XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType)) {
            QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
            if (!decCheckCRCQuiet(crcType, varCRC, pCachedDevice, pPdStruct, pState)) {
                return false;
            }
        }

        bResult = decEmitDevice(pCachedDevice, 0, nDecompressedSize, pState, pPdStruct);
        if (!stateTransaction.isAlive() || !guardedInput ||
            !guardedOutput) return false;
        if (bResult) {
            pState->nCountInput = pCachedDevice->property("RAR_INPUT_CONSUMED").toLongLong();
        }
    }

    return bResult;
}

bool XDecompress::checkCRC(XBinary::CRC_TYPE crcType, QVariant value, QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct,
                           const XBinary::DATAPROCESS_STATE *pState)
{
    QPointer<XDecompress> guardedThis(this);
    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct) : XBinary::PDSTRUCTLIFETIME();
    const DecCRCResult result = decCheckCRCValue(crcType, value, pDevice,
                                                 pPdStruct, pState);

    // checkCRC() can invoke the caller's progress callback.  It may destroy
    // any caller-owned argument (or this object), so validate the retained
    // identities before touching them again.
    if (!guardedThis || !decProgressAlive(pPdStruct, progressLifetime)) {
        return false;
    }
    if (result == DecCRCResult::Ok) return true;
    if (result == DecCRCResult::Aborted) return false;

    const QString sMessage = decCRCResultMessage(result);
    XBinary::setPdStructErrorString(pPdStruct, sMessage);
    if (!guardedThis || !decProgressAlive(pPdStruct, progressLifetime)) {
        return false;
    }
    Q_EMIT guardedThis->warningMessage(sMessage);
    return false;
}

bool XDecompress::multiDecompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;
    QPointer<XDecompress> guardedThis(this);

    if (!pState) {
        return false;
    }

    // An index-paired archive-stream record is not decodable from coordinates.
    // Its member lives inside a private decoded stream this decoder cannot
    // reach, so whatever (offset,size) pair arrives here addresses some OTHER
    // data - in practice the raw container.  Refuse that shape at the one place
    // every decode funnels through, rather than at each entry point that
    // happens to remember to ask.
    if (decIsArchiveStreamProperties(pState->mapProperties)) {
        return false;
    }

    DecProcessStateTransaction stateTransaction(this, pState, pPdStruct);
    pState = stateTransaction.state();

    qint64 nConfiguredOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pState->mapUnpackProperties,
                                       &nConfiguredOutputLimit)) {
        XBinary::setPdStructErrorString(pPdStruct,
                                       tr("Invalid unpacked-output limit"));
        return false;
    }
    if (pState->mapProperties.contains(
            XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        const qint64 nDeclaredOutputSize =
            pState->mapProperties
                .value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)
                .toLongLong();
        if ((nDeclaredOutputSize < 0) ||
            ((nConfiguredOutputLimit >= 0) &&
             (nDeclaredOutputSize > nConfiguredOutputLimit))) {
            XBinary::setPdStructErrorString(
                pPdStruct,
                tr("Unpacked output exceeds the configured limit"));
            return false;
        }
    }

    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct) : XBinary::PDSTRUCTLIFETIME();
    const auto isContextAlive = [&]() -> bool {
        return guardedThis && stateTransaction.isAlive();
    };

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;

    if (!pState->pDeviceOutput) {
        return false;
    }

    // Destination reset is destructive.  Reject every known view of the
    // source first (including nested SubDevices, shared QBuffer storage and
    // QFile aliases/hard links) so an in-place request leaves the archive
    // byte-for-byte intact.
    QPointer<QIODevice> guardedOutput(pState->pDeviceOutput);
    QPointer<QIODevice> guardedInput(pState->pDeviceInput);
    if (!guardedOutput || !isContextAlive()) return false;
    const bool bDevicesAlias = guardedInput &&
        XBinary::devicesAlias(guardedInput.data(), guardedOutput.data());
    if (!isContextAlive() || !guardedOutput ||
        (pState->pDeviceInput && !guardedInput)) {
        return false;
    }
    if (bDevicesAlias) {
        return false;
    }

    // Extraction has exact-replacement semantics.  Clearing up front also
    // guarantees that cancellation, CRC failure, or an unsupported method
    // cannot leave bytes from an earlier use of the destination behind.
    const bool bOutputCleared = decClearOutputDevice(guardedOutput.data());
    if (!isContextAlive() || !guardedOutput ||
        (pState->pDeviceInput && !guardedInput)) {
        return false;
    }
    if (!bOutputCleared) {
        pState->bWriteError = true;
        return false;
    }
    // Solid-cache identity fallback hashes the input before the per-codec
    // validator runs, so reject a missing input here after rolling output back.
    if (!pState->pDeviceInput) {
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bRar5HashMac = pState->mapProperties.value(XBinary::FPART_PROP_RAR5_HASHMAC, false).toBool();
    if (bRar5HashMac &&
        ((!pState->mapProperties.contains(XBinary::FPART_PROP_RESULTCRC)) ||
         ((XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt() !=
          XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF) ||
         (pState->mapProperties.value(XBinary::FPART_PROP_AESKEY).toByteArray().size() < 33) ||
         pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString().isEmpty())) {
        return false;
    }

    bool bIsSolid = pState->mapProperties.value(XBinary::FPART_PROP_ISSOLID, false).toBool();

    QString sArchiveIdentity = pState->mapProperties.value(XBinary::FPART_PROP_FILEMD5).toString().trimmed().toLower();
    if (bIsSolid) {
        // A device pointer and compressed extents do not identify mutable
        // QIODevice contents.  RAR also needs the cache and decoder dictionary
        // to survive successive records, so an invocation-local cache is not
        // viable.  When the parser did not provide FILEMD5, derive a stable
        // content identity and compare it on every solid call.  This preserves
        // a live solid sequence while detecting a QBuffer (or other reusable
        // device object) whose bytes were replaced between sequences.
        if (sArchiveIdentity.isEmpty()) {
            DecNestedProgressBridge hashBridge = {};
            XBinary::PDSTRUCT hashProgress = XBinary::getPdStructSnapshot(pPdStruct);
            decPrepareNestedProgress(&hashProgress, pPdStruct, &hashBridge);
            sArchiveIdentity = XBinary::getHash(XBinary::HASH_SHA256,
                                                guardedInput.data(), &hashProgress);
            if (!guardedThis || !guardedInput ||
                !decProgressAlive(pPdStruct, progressLifetime)) {
                return false;
            }
            if (sArchiveIdentity.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                guardedThis->clearSolidCache();
                return false;
            }
            sArchiveIdentity.prepend(QStringLiteral("sha256:"));
        } else {
            sArchiveIdentity.prepend(QStringLiteral("md5:"));
        }

        const bool bResetCache = (sArchiveIdentity != m_sCurrentArchiveIdentity) ||
                                 (m_pCurrentSolidDevice != pState->pDeviceInput);
        if (bResetCache) {
            clearSolidCache();
            m_sCurrentArchiveIdentity = sArchiveIdentity;
            m_pCurrentSolidDevice = pState->pDeviceInput;
        }
    }

    qint32 nNumberOfMethods = 1;

    XBinary::HANDLE_METHOD topMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
    // BCJ2 handles its own 4 sub-streams internally in decompress() — never treat it as multi-method
    if (topMethod != XBinary::HANDLE_METHOD_BCJ2) {
        if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD3)) {
            nNumberOfMethods = 3;
        } else if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2)) {
            nNumberOfMethods = 2;
        }
    }

    if ((nNumberOfMethods == 1) && (!bIsSolid)) {
        const XBinary::CRC_TYPE crcType =
            (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
        const bool bCheckCRC = (crcType != XBinary::CRC_TYPE_UNKNOWN) &&
                               pState->mapProperties.contains(XBinary::FPART_PROP_RESULTCRC) &&
                               XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType);
        const bool bWindowed = (pState->nProcessedOffset != 0) || (pState->nProcessedLimit != -1);

        if (bCheckCRC && bWindowed) {
            // A record CRC covers the complete decoded record, never a caller's
            // output slice.  Decode and authenticate the full record first,
            // then apply the requested processed-output window.
            const qint64 nExpectedSize =
                qMax<qint64>(0, pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong());
            QIODevice *pFullDevice =
                XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nExpectedSize)
                    ? XBinary::createUnpackFileBuffer(
                          nExpectedSize, pState->mapUnpackProperties,
                          pPdStruct)
                    : nullptr;
            if (!isContextAlive() || !guardedInput || !guardedOutput) {
                XBinary::freeFileBuffer(&pFullDevice);
                return false;
            }
            if (pFullDevice) {
                XBinary::DATAPROCESS_STATE fullState = *pState;
                fullState.pDeviceOutput = pFullDevice;
                fullState.nProcessedOffset = 0;
                fullState.nProcessedLimit = -1;
                fullState.bReadError = false;
                fullState.bWriteError = false;
                fullState.nCountInput = 0;
                fullState.nCountOutput = 0;

                {
                    DecSignalSuppressionGuard signalGuard;
                    bResult = decompress(&fullState, pPdStruct);
                }
                if (!isContextAlive() || !guardedInput ||
                    !guardedOutput) {
                    XBinary::freeFileBuffer(&pFullDevice);
                    return false;
                }
                pState->nCountInput = fullState.nCountInput;
                pState->bReadError = fullState.bReadError;
                pState->bWriteError = fullState.bWriteError;

                const qint64 nFullSize = fullState.nCountOutput;
                bResult = bResult && (nFullSize >= 0) && (pFullDevice->size() == nFullSize);
                if (bResult) {
                    const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                    bResult = decCheckCRCQuiet(crcType, varCRC, pFullDevice, pPdStruct, pState);
                }
                if (bResult) {
                    bResult = decEmitDevice(pFullDevice, 0, nFullSize, pState, pPdStruct);
                }
                XBinary::freeFileBuffer(&pFullDevice);
            }
        } else {
            // Full-output or unchecked single-method extraction can stream
            // directly to the caller.
            {
                DecSignalSuppressionGuard signalGuard;
                bResult = decompress(pState, pPdStruct);
            }
            if (!isContextAlive() || !guardedInput || !guardedOutput)
                return false;
            if (bResult && bCheckCRC && pState->pDeviceOutput) {
                const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                bResult = decCheckCRCQuiet(crcType, varCRC, pState->pDeviceOutput, pPdStruct, pState);
            }
        }
    } else if (bIsSolid) {
        // Check if this is a RAR solid archive — RAR solid requires sequential decompression
        // with persistent decoder state, unlike 7z solid which uses a single compressed block.
        bool bIsRarSolid = (topMethod == XBinary::HANDLE_METHOD_RAR_15) || (topMethod == XBinary::HANDLE_METHOD_RAR_20) || (topMethod == XBinary::HANDLE_METHOD_RAR_29) ||
                           (topMethod == XBinary::HANDLE_METHOD_RAR_50) || (topMethod == XBinary::HANDLE_METHOD_RAR_70);

        // STORE files inside a RAR solid archive must also use decompressRarSolid() to keep
        // the solid index counter in sync. RAR records have SOLIDFOLDERINDEX but no
        // SUBSTREAMOFFSET (unlike 7z/CAB), which distinguishes them.
        if (!bIsRarSolid && (topMethod == XBinary::HANDLE_METHOD_STORE) && pState->mapProperties.contains(XBinary::FPART_PROP_SOLIDFOLDERINDEX) &&
            !pState->mapProperties.contains(XBinary::FPART_PROP_SUBSTREAMOFFSET)) {
            bIsRarSolid = true;
        }

        if (bIsRarSolid) {
            // RAR solid: use XRar streaming API to decompress all files with proper decoder state,
            // cache each file's output, and return the requested file from cache.
            bResult = decompressRarSolid(pState, pPdStruct);
            if (!isContextAlive() || !guardedInput || !guardedOutput)
                return false;
        } else {
            // Non-RAR solid (e.g., 7z): decompress the entire folder block once, cache it,
            // then extract this file's sub-stream.
            // Prefer the explicit solid-folder ID (set by archive parsers such as XSevenZip
            // via FPART_PROP_SOLIDFOLDERINDEX); fall back to offset_size when absent.
            QString sCacheKey;
            qint64 nSolidFolderIndex = pState->mapProperties.value(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)-1).toLongLong();
            const QString sDeviceKey = QString::number((qulonglong)(quintptr)pState->pDeviceInput, 16);
            if (nSolidFolderIndex >= 0) {
                sCacheKey = QString("%1_f%2_%3_%4").arg(sDeviceKey).arg(nSolidFolderIndex).arg(pState->nInputOffset).arg(pState->nInputLimit);
            } else {
                sCacheKey = QString("%1_%2_%3").arg(sDeviceKey).arg(pState->nInputOffset).arg(pState->nInputLimit);
            }

            if (!m_mapSolidCache.contains(sCacheKey)) {
                qint64 nStreamUnpackedSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE, (qint64)0).toLongLong();

                // Build a block-level state: same source, ISSOLID=false, full block uncompressed size.
                // The recursive call goes to single-method or multi-method non-solid branch.
                XBinary::DATAPROCESS_STATE blockState = *pState;
                blockState.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, false);
                blockState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nStreamUnpackedSize);
                blockState.nProcessedOffset = 0;
                blockState.nProcessedLimit = -1;
                blockState.bReadError = false;
                blockState.bWriteError = false;
                blockState.nCountInput = 0;
                blockState.nCountOutput = 0;
                // A solid folder CRC covers the complete decompressed block. If
                // it is unavailable, remove the per-file CRC before decoding the block.
                if (blockState.mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDCRC)) {
                    blockState.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
                    blockState.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, blockState.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDCRC));
                } else {
                    blockState.mapProperties.remove(XBinary::FPART_PROP_CRC_TYPE);
                    blockState.mapProperties.remove(XBinary::FPART_PROP_RESULTCRC);
                }

                QIODevice *pSolidDevice =
                    XBinary::isUnpackOutputSizeAllowed(
                        pState->mapUnpackProperties,
                        nStreamUnpackedSize)
                        ? XBinary::createUnpackFileBuffer(
                              nStreamUnpackedSize,
                              pState->mapUnpackProperties, pPdStruct)
                        : nullptr;
                if (!isContextAlive() || !guardedInput || !guardedOutput) {
                    XBinary::freeFileBuffer(&pSolidDevice);
                    return false;
                }
                blockState.pDeviceOutput = pSolidDevice;

                bool bBlockResult = pSolidDevice &&
                    (nStreamUnpackedSize >= 0) &&
                    multiDecompress(&blockState, pPdStruct);
                if (!isContextAlive() || !guardedInput ||
                    !guardedOutput) {
                    XBinary::freeFileBuffer(&pSolidDevice);
                    return false;
                }
                if (pSolidDevice && bBlockResult && (blockState.nCountOutput == nStreamUnpackedSize) &&
                    (pSolidDevice->size() == nStreamUnpackedSize)) {
                    pSolidDevice->setProperty("SOLID_INPUT_CONSUMED", blockState.nCountInput);
                    m_mapSolidCache.insert(sCacheKey, pSolidDevice);
                } else {
                    XBinary::freeFileBuffer(&pSolidDevice);
                }
            }

            if (m_mapSolidCache.contains(sCacheKey)) {
                const qint64 nSubstreamOffset = pState->mapProperties.value(XBinary::FPART_PROP_SUBSTREAMOFFSET, (qint64)0).toLongLong();
                const qint64 nDecompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong();
                QIODevice *pSolidDevice = m_mapSolidCache.value(sCacheKey);
                if (pState->pDeviceOutput && pSolidDevice && (nSubstreamOffset >= 0) && (nDecompressedSize >= 0) &&
                    (nSubstreamOffset <= pSolidDevice->size()) && (nDecompressedSize <= (pSolidDevice->size() - nSubstreamOffset))) {
                    bResult = true;

                    // Authenticate the complete logical file before applying a
                    // partial output window.
                    const XBinary::CRC_TYPE crcType =
                        (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
                    if (XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType)) {
                        SubDevice crcDevice(pSolidDevice, nSubstreamOffset, nDecompressedSize);
                        if (!crcDevice.open(QIODevice::ReadOnly)) {
                            bResult = false;
                        } else {
                            const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                            bResult = decCheckCRCQuiet(crcType, varCRC, &crcDevice, pPdStruct, pState);
                            crcDevice.close();
                        }
                    }

                    if (bResult) {
                        pState->nCountInput = pSolidDevice->property("SOLID_INPUT_CONSUMED").toLongLong();
                        bResult = decEmitDevice(pSolidDevice, nSubstreamOffset, nDecompressedSize, pState, pPdStruct);
                    }
                }
            }
        }
    } else {
        // Multi-method, non-solid: every layer is decoded in full into its own
        // temporary device.  Processed-output windows belong only to the final
        // logical record; inheriting them in an intermediate layer discards
        // bytes required by the next filter.
        QIODevice *pIntermediateDevice = nullptr;
        qint64 nIntermediateSize = 0;
        qint64 nSourceCount = 0;

        for (qint32 i = nNumberOfMethods - 1; i >= 0; i--) {
            XBinary::DATAPROCESS_STATE state = *pState;

            XBinary::FPART_PROP fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD;
            XBinary::FPART_PROP fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES;
            XBinary::FPART_PROP fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE;
            XBinary::FPART_PROP fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE;

            if (i == 2) {
                if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD3)) fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSPROPERTIES3)) fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE3)) fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE3;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE3)) fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE3;
            } else if (i == 1) {
                if (pState->mapProperties.contains(XBinary::FPART_PROP_HANDLEMETHOD2)) fpHandleMethod = XBinary::FPART_PROP_HANDLEMETHOD2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSPROPERTIES2)) fpCompressProperties = XBinary::FPART_PROP_COMPRESSPROPERTIES2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_COMPRESSEDSIZE2)) fpCompressedSize = XBinary::FPART_PROP_COMPRESSEDSIZE2;
                if (pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE2)) fpUncompressedSize = XBinary::FPART_PROP_UNCOMPRESSEDSIZE2;
            }

            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, pState->mapProperties.value(fpHandleMethod));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, pState->mapProperties.value(fpCompressProperties));
            state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, pState->mapProperties.value(fpCompressedSize));
            state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, pState->mapProperties.value(fpUncompressedSize));
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;
            state.bReadError = false;
            state.bWriteError = false;
            state.nCountInput = 0;
            state.nCountOutput = 0;

            if (i == nNumberOfMethods - 1) {
                state.pDeviceInput = pState->pDeviceInput;
            } else {
                state.pDeviceInput = pIntermediateDevice;
                state.nInputOffset = 0;
                state.nInputLimit = nIntermediateSize;
            }

            const qint64 nExpectedSize =
                qMax<qint64>(0, state.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0).toLongLong());
            QIODevice *pStageOutput =
                XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nExpectedSize)
                    ? XBinary::createUnpackFileBuffer(
                          nExpectedSize, pState->mapUnpackProperties,
                          pPdStruct)
                    : nullptr;
            if (!isContextAlive() || !guardedInput || !guardedOutput) {
                XBinary::freeFileBuffer(&pStageOutput);
                XBinary::freeFileBuffer(&pIntermediateDevice);
                return false;
            }
            state.pDeviceOutput = pStageOutput;

            if (pStageOutput) {
                DecSignalSuppressionGuard signalGuard;
                bResult = decompress(&state, pPdStruct);
            } else {
                bResult = false;
            }
            if (!isContextAlive() || !guardedInput || !guardedOutput) {
                XBinary::freeFileBuffer(&pStageOutput);
                XBinary::freeFileBuffer(&pIntermediateDevice);
                return false;
            }
            nIntermediateSize = state.nCountOutput;
            if (i == nNumberOfMethods - 1) nSourceCount = state.nCountInput;
            pState->bReadError = pState->bReadError || state.bReadError;
            pState->bWriteError = pState->bWriteError || state.bWriteError;

            if (pIntermediateDevice) XBinary::freeFileBuffer(&pIntermediateDevice);

            if (!bResult || (nIntermediateSize < 0) || !pStageOutput || (pStageOutput->size() != nIntermediateSize)) {
                XBinary::freeFileBuffer(&pStageOutput);
                bResult = false;
                break;
            }

            pIntermediateDevice = pStageOutput;
        }

        if (bResult && pIntermediateDevice) {
            const XBinary::CRC_TYPE crcType =
                (XBinary::CRC_TYPE)pState->mapProperties.value(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_UNKNOWN).toUInt();
            if (XBinary::isUnpackCRCEnabled(pState->mapUnpackProperties, crcType)) {
                const QVariant varCRC = pState->mapProperties.value(XBinary::FPART_PROP_RESULTCRC, 0);
                bResult = decCheckCRCQuiet(crcType, varCRC, pIntermediateDevice, pPdStruct, pState);
            }
            pState->nCountInput = nSourceCount;
            if (bResult) bResult = decEmitDevice(pIntermediateDevice, 0, nIntermediateSize, pState, pPdStruct);
        }

        if (pIntermediateDevice) XBinary::freeFileBuffer(&pIntermediateDevice);
    }

    if (!bResult && guardedOutput && isContextAlive()) {
        const bool bSequential = guardedOutput->isSequential();
        if (guardedOutput && isContextAlive() && !bSequential) {
            decClearOutputDevice(guardedOutput.data());
        }
    }

    return bResult && isContextAlive();
}

static bool decLzipReadExactAt(XBinary::DATAPROCESS_STATE *pState, qint64 nOffset, char *pData, qint32 nSize)
{
    if (!pState || !pState->pDeviceInput || !pData || (nOffset < 0) || (nSize <= 0) ||
        !pState->pDeviceInput->seek(nOffset)) {
        if (pState) pState->bReadError = true;
        return false;
    }

    qint32 nReadTotal = 0;
    while (nReadTotal < nSize) {
        const qint64 nRead = pState->pDeviceInput->read(pData + nReadTotal, nSize - nReadTotal);
        if ((nRead <= 0) || (nRead > (nSize - nReadTotal))) {
            pState->bReadError = true;
            return false;
        }
        nReadTotal += (qint32)nRead;
    }
    return true;
}

static quint32 decLzipReadLE32(const char *pData)
{
    return (quint32)(quint8)pData[0] | ((quint32)(quint8)pData[1] << 8) |
           ((quint32)(quint8)pData[2] << 16) | ((quint32)(quint8)pData[3] << 24);
}

static quint64 decLzipReadLE64(const char *pData)
{
    quint64 nValue = 0;
    for (qint32 i = 0; i < 8; i++) nValue |= ((quint64)(quint8)pData[i] << (i * 8));
    return nValue;
}

bool XDecompress::decompress(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState) {
        return false;
    }

    // The third public decode entry point, and it dispatches to the codecs
    // without passing through multiDecompress().  It gets the same refusal, so
    // that no entry point is the one that forgot to ask.
    if (decIsArchiveStreamProperties(pState->mapProperties)) {
        return false;
    }

    DecProcessStateTransaction stateTransaction(this, pState, pPdStruct);
    pState = stateTransaction.state();
    QPointer<XDecompress> guardedThis(this);
    const XBinary::PDSTRUCTLIFETIME progressLifetime =
        pPdStruct ? XBinary::retainPdStructLifetime(pPdStruct)
                  : XBinary::PDSTRUCTLIFETIME();
    const auto isContextAlive = [&]() -> bool {
        return guardedThis &&
               (!pPdStruct ||
                XBinary::isPdStructLifetimeAlive(progressLifetime));
    };

    DecInputStateGuard inputStateGuard(pState);
    bool bResult = false;
    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;

    // The direct public entry point must provide the same exact-replacement
    // contract as multiDecompress(): clear a usable destination even when the
    // source is missing, and never let a format branch dereference null.
    if (!pState->pDeviceOutput) {
        pState->bWriteError = true;
        return false;
    }
    QPointer<QIODevice> guardedOutput(pState->pDeviceOutput);
    QPointer<QIODevice> guardedInput(pState->pDeviceInput);
    if (!guardedOutput || !isContextAlive()) return false;
    const bool bDevicesAlias = guardedInput &&
        XBinary::devicesAlias(guardedInput.data(), guardedOutput.data());
    if (!isContextAlive() || !guardedOutput ||
        (pState->pDeviceInput && !guardedInput)) {
        return false;
    }
    if (bDevicesAlias) {
        return false;
    }
    const bool bOutputCleared = decClearOutputDevice(guardedOutput.data());
    if (!isContextAlive() || !guardedOutput ||
        (pState->pDeviceInput && !guardedInput)) {
        return false;
    }
    if (!bOutputCleared) {
        pState->bWriteError = true;
        return false;
    }
    if (!pState->pDeviceInput) {
        pState->bReadError = true;
        return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bInputSeeked = guardedInput->seek(pState->nInputOffset);
    if (!guardedInput || !guardedOutput || !isContextAlive()) return false;
    if (!bInputSeeked) {
        const qint64 nInputPosition = guardedInput->pos();
        if (!guardedInput || !guardedOutput || !isContextAlive())
            return false;
        if (nInputPosition != pState->nInputOffset) {
            pState->bReadError = true;
            return false;
        }
    }

    XBinary::HANDLE_METHOD compressMethod = (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_STORE).toUInt();
    QByteArray baProperty = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
    bool bUncompressedSizeDefined = pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
    qint64 nUncompressedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, 0).toLongLong();
    qint64 nWindowSize = pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toLongLong();
    qint64 nConfiguredOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pState->mapUnpackProperties,
                                       &nConfiguredOutputLimit) ||
        (bUncompressedSizeDefined &&
         !XBinary::isUnpackOutputSizeAllowed(
             pState->mapUnpackProperties, nUncompressedSize)) ||
        ((nConfiguredOutputLimit >= 0) && (nWindowSize > 0) &&
         (nWindowSize > nConfiguredOutputLimit))) {
        XBinary::setPdStructErrorString(
            pPdStruct,
            tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    // ARJ GARBLE pre-decryption: if PASSWORD_MODIFIER is present, XOR the compressed stream
    // with (modifier + password[i % len]) mod 256 before decompressing.
    QByteArray baArjGarbleDecrypted;
    XBinary::UNPACK_MEMORY_RESERVATION arjGarbleReservation;
    QBuffer arjGarbleBuf;
    if (pState->mapProperties.contains(XBinary::FPART_PROP_PASSWORD_MODIFIER)) {
        quint8 nModifier = (quint8)pState->mapProperties.value(XBinary::FPART_PROP_PASSWORD_MODIFIER).toUInt();
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        // An empty member (0-byte file, or a directory entry ARJ stores as a
        // zero-length record) carries no packed bytes to decrypt.  Rejecting
        // it here would fail every empty entry of an encrypted archive; leave
        // the plain input in place and let the STORE path emit nothing.
        if (!sPassword.isEmpty() && pState->pDeviceInput &&
            (pState->nInputLimit != 0)) {
            if ((pState->nInputLimit < 0) ||
                ((nConfiguredOutputLimit >= 0) &&
                 (pState->nInputLimit > nConfiguredOutputLimit)) ||
                (pState->nInputLimit >
                 (std::numeric_limits<qint32>::max)())) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    tr("Encrypted input exceeds the configured limit"));
                return false;
            }
            if (!arjGarbleReservation.acquire(
                    pState->mapUnpackProperties,
                    pState->nInputLimit)) {
                return false;
            }
            const bool bGarbleSeeked = guardedInput->seek(pState->nInputOffset);
            if (!guardedInput || !guardedOutput || !isContextAlive() ||
                !bGarbleSeeked) return false;
            baArjGarbleDecrypted.resize((qint32)pState->nInputLimit);
            const qint64 nReadResult = guardedInput->read(
                baArjGarbleDecrypted.data(), pState->nInputLimit);
            if (!guardedInput || !guardedOutput || !isContextAlive())
                return false;
            const qint32 nRead = (qint32)nReadResult;
            if (nRead == (qint32)pState->nInputLimit) {
                const qint32 nPwdLen = sPassword.length();
                for (qint32 i = 0; i < nRead; i++) {
                    quint8 k = (quint8)(nModifier + (quint8)sPassword[i % nPwdLen].toLatin1());
                    baArjGarbleDecrypted.data()[i] = (char)((quint8)baArjGarbleDecrypted[i] ^ k);
                }
                arjGarbleBuf.setBuffer(&baArjGarbleDecrypted);
                if (!arjGarbleBuf.open(QIODevice::ReadOnly)) {
                    return false;
                }
                pState->pDeviceInput = &arjGarbleBuf;
                pState->nInputOffset = 0;
            }
        }
    }

    if (compressMethod == XBinary::HANDLE_METHOD_STORE) {
        // For STORE after AES decryption, the input may include AES padding bytes.
        // Cap input to the actual uncompressed size (including zero) to avoid
        // copying padding or stale data into an empty result.
        if (bUncompressedSizeDefined && (nUncompressedSize >= 0) && (nUncompressedSize < pState->nInputLimit)) {
            pState->nInputLimit = nUncompressedSize;
        }
        bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BZIP2) {
        bResult = XBZIP2Decoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BROTLI) {
        bResult = XBrotliDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZMA) {
        if (!baProperty.isEmpty()) {
            bResult = XLZMADecoder::decompress(pState, baProperty, pPdStruct);
        } else {
            bResult = XLZMADecoder::decompress(pState, pPdStruct);
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZMA2) {
        if (!baProperty.isEmpty()) {
            bResult = XLZMADecoder::decompressLZMA2(pState, baProperty, pPdStruct);
        } else {
            bResult = XLZMADecoder::decompressLZMA2(pState, pPdStruct);
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_BCJ) {
        // x86 BCJ inverse filter — delegate to the single byte-exact reference port.
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            qint64 nFilterSize = pState->nInputLimit;
            if (nFilterSize == -1) {
                const qint64 nInputSize = guardedInput->size();
                if (!guardedInput || !guardedOutput || !isContextAlive())
                    return false;
                nFilterSize = nInputSize - pState->nInputOffset;
            }
            if (!decIsValidBufferSize(nFilterSize)) {
                pState->bReadError = true;
                return false;
            }
            if (!XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nFilterSize)) {
                return false;
            }
            XBinary::UNPACK_MEMORY_RESERVATION filterReservation;
            if (!filterReservation.acquire(
                    pState->mapUnpackProperties, nFilterSize)) {
                return false;
            }
            QByteArray baData = guardedInput->read(nFilterSize);
            if (!guardedInput || !guardedOutput || !isContextAlive())
                return false;
            pState->nCountInput = baData.size();
            if (baData.size() != nFilterSize) {
                pState->bReadError = true;
                return false;
            }

            // Optional 4-byte LE start-offset property (ip); absent/0 for standard 7z.
            quint32 nIp = 0;
            if (!decGetBranchStartOffset(baProperty, &nIp)) return false;

            Algo_utils::applyBCJX86Decode(baData, nIp);

            bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARM64_BCJ) {
        quint32 nIp = 0;
        bResult = decGetBranchStartOffset(baProperty, &nIp) &&
                  XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_ARM64, pPdStruct, nIp);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARM_BCJ) {
        quint32 nIp = 0;
        bResult = decGetBranchStartOffset(baProperty, &nIp) &&
                  XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_ARM, pPdStruct, nIp);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARMT_BCJ) {
        quint32 nIp = 0;
        bResult = decGetBranchStartOffset(baProperty, &nIp) &&
                  XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_ARMT, pPdStruct, nIp);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPC_BCJ) {
        quint32 nIp = 0;
        bResult = decGetBranchStartOffset(baProperty, &nIp) &&
                  XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_PPC, pPdStruct, nIp);
    } else if (compressMethod == XBinary::HANDLE_METHOD_SPARC_BCJ) {
        quint32 nIp = 0;
        bResult = decGetBranchStartOffset(baProperty, &nIp) &&
                  XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_SPARC, pPdStruct, nIp);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IA64_BCJ) {
        quint32 nIp = 0;
        bResult = decGetBranchStartOffset(baProperty, &nIp) &&
                  XBranchDecoder::decompressBranch(pState, XBranchDecoder::BTYPE_IA64, pPdStruct, nIp);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DELTA) {
        // Property byte holds distance - 1 (7z and XZ delta filter convention)
        qint32 nDistance = baProperty.isEmpty() ? 1 : ((qint32)(quint8)baProperty.at(0) + 1);
        bResult = XBranchDecoder::decompressDelta(pState, nDistance, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_KWAJ_XOR) {
        // KWAJ compression method 1: every byte XOR 0xFF
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            qint64 nFilterSize = pState->nInputLimit;
            if (nFilterSize == -1) {
                const qint64 nInputSize = guardedInput->size();
                if (!guardedInput || !guardedOutput || !isContextAlive())
                    return false;
                nFilterSize = nInputSize - pState->nInputOffset;
            }
            if (!decIsValidBufferSize(nFilterSize)) {
                pState->bReadError = true;
                return false;
            }
            if (!XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nFilterSize)) {
                return false;
            }
            XBinary::UNPACK_MEMORY_RESERVATION filterReservation;
            if (!filterReservation.acquire(
                    pState->mapUnpackProperties, nFilterSize)) {
                return false;
            }
            QByteArray baData = guardedInput->read(nFilterSize);
            if (!guardedInput || !guardedOutput || !isContextAlive())
                return false;
            pState->nCountInput = baData.size();
            if (baData.size() != nFilterSize) {
                pState->bReadError = true;
                return false;
            }

            for (qint32 i = 0; i < baData.size(); i++) {
                baData[i] = (char)((quint8)baData.at(i) ^ 0xFF);
            }

            bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_KWAJ_LZSS) {
        bResult = XKWAJLZSSDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_KWAJ_LZH) {
        bResult = XKWAJLZHDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_KWAJ_MSZIP) {
        // KWAJ method 4 is a sequence of uint16-sized MSZIP blocks.  The
        // length includes the two-byte CK signature but not its own uint16;
        // a zero length or clean physical EOF at a length boundary terminates
        // the stream.  Blocks share the preceding 32 KiB DEFLATE history.
        qint64 nStreamSize = 0;
        qint64 nPhysicalInputConsumed = 0;
        qint64 nOffset = 0;
        qint64 nProduced = 0;
        qint32 nPreviousBlockOutput = -1;
        QByteArray baHistory;
        bool bCleanEnd = false;

        bResult = decPrepareBoundedInput(
            pState->pDeviceInput, pState->nInputOffset,
            pState->nInputLimit, &nStreamSize);
        if (!isContextAlive() || !guardedInput || !guardedOutput) return false;

        while (bResult && !bCleanEnd &&
               XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nOffset == nStreamSize) {
                bCleanEnd = true;
                break;
            }

            char abLength[2] = {};
            const bool bLengthRead = decReadExactAt(
                pState->pDeviceInput, pState->nInputOffset + nOffset,
                abLength, sizeof(abLength), pState, pPdStruct,
                &nPhysicalInputConsumed);
            if (!isContextAlive() || !guardedInput || !guardedOutput)
                return false;
            if (!bLengthRead) {
                pState->nCountInput =
                    qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            nOffset += sizeof(abLength);
            const quint16 nBlockSize =
                (quint8)abLength[0] |
                ((quint16)(quint8)abLength[1] << 8);
            if (nBlockSize == 0) {
                // An explicit terminator owns the remainder of the bounded
                // stream.  Bytes after it are unauthenticated trailing data.
                bCleanEnd = nOffset == nStreamSize;
                bResult = bCleanEnd;
                break;
            }

            // Only the final block may expand to fewer than 32768 bytes.  A
            // following nonzero block proves the preceding one was non-final.
            if ((nPreviousBlockOutput >= 0) &&
                (nPreviousBlockOutput != 32768)) {
                bResult = false;
                break;
            }

            if ((nBlockSize < 2) ||
                (nBlockSize > DEC_KWAJ_MSZIP_MAX_BLOCK_SIZE) ||
                ((qint64)nBlockSize > nStreamSize - nOffset)) {
                if ((qint64)nBlockSize > nStreamSize - nOffset)
                    pState->bReadError = true;
                bResult = false;
                break;
            }

            QByteArray baPayload(nBlockSize, 0);
            const bool bPayloadRead = decReadExactAt(
                pState->pDeviceInput, pState->nInputOffset + nOffset,
                baPayload.data(), nBlockSize, pState, pPdStruct,
                &nPhysicalInputConsumed);
            if (!isContextAlive() || !guardedInput || !guardedOutput)
                return false;
            if (!bPayloadRead) {
                pState->nCountInput =
                    qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }
            nOffset += nBlockSize;

            QByteArray baBlock;
            const bool bInflated = decInflateMSZIPBlock(
                baPayload, baHistory, -1, &baBlock, pPdStruct);
            if (!isContextAlive() || !guardedInput || !guardedOutput)
                return false;
            if (!bInflated || baBlock.isEmpty() ||
                (baBlock.size() > 32768) ||
                (nProduced > (std::numeric_limits<qint64>::max)() -
                                 baBlock.size())) {
                bResult = false;
                break;
            }

            const qint64 nNextProduced = nProduced + baBlock.size();
            if (bUncompressedSizeDefined &&
                (nNextProduced > nUncompressedSize)) {
                bResult = false;
                break;
            }

            const qint32 nWritten = XBinary::_writeDevice(
                baBlock.constData(), baBlock.size(), pState);
            if (!isContextAlive() || !guardedInput || !guardedOutput)
                return false;
            if (nWritten != baBlock.size()) {
                bResult = false;
                break;
            }

            nProduced = nNextProduced;
            nPreviousBlockOutput = baBlock.size();
            baHistory.append(baBlock);
            if (baHistory.size() > 32768)
                baHistory = baHistory.right(32768);
        }

        if (isContextAlive() && guardedInput && guardedOutput) {
            pState->nCountInput =
                qMin(nPhysicalInputConsumed, nStreamSize);
            bResult = bResult && bCleanEnd &&
                      XBinary::isPdStructNotCanceled(pPdStruct) &&
                      (nOffset == nStreamSize) &&
                      (pState->nCountInput == nStreamSize) &&
                      (pState->nCountOutput == nProduced) &&
                      (!bUncompressedSizeDefined ||
                       (nProduced == nUncompressedSize));
        } else {
            return false;
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_XZ) {
        bResult = XLZMADecoder::decompressXZ(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPMD7) {
        bResult = XPPMdDecoder::decompressPPMD7(pState, baProperty, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_PPMD8) {
        bResult = XPPMdDecoder::decompressPPMD8(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DEFLATE) {
        bResult = XDeflateDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_DEFLATE64) {
        bResult = XDeflateDecoder::decompress64(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT214_8) {
        bResult = XIT214Decoder::decompress(pState, 8, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT214_16) {
        bResult = XIT214Decoder::decompress(pState, 16, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT215_8) {
        bResult = XIT214Decoder::decompress(pState, 8, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IT215_16) {
        bResult = XIT214Decoder::decompress(pState, 16, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_4KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_4KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, false, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_8KDICT_2TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, false, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_IMPLODED_8KDICT_3TREES) {
        bResult = XImplodeDecoder::decompress(pState, true, true, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_SHRINK) {
        bResult = XShrinkDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_1) {
        bResult = XReduceDecoder::decompress(pState, 1, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_2) {
        bResult = XReduceDecoder::decompress(pState, 2, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_3) {
        bResult = XReduceDecoder::decompress(pState, 3, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_REDUCE_4) {
        bResult = XReduceDecoder::decompress(pState, 4, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZLIB) {
        bResult = XDeflateDecoder::decompress_zlib(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZW_PDF) {
        bResult = XLZWDecoder::decompress_pdf(pState, pPdStruct);
        // bResult = XStoreDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ASCII85) {
        bResult = XASCII85Decoder::decompress_pdf(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ASCIIHEX) {
        bResult = XASCIIHexDecoder::decompress_pdf(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_RUNLENGTH) {
        bResult = XRunLengthDecoder::decompress_pdf(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH1) {
        bResult = XLZHDecoder::decompress(pState, 1, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH4) {
        bResult = XLZHDecoder::decompress(pState, 4, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH5) {
        bResult = XLZHDecoder::decompress(pState, 5, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH6) {
        bResult = XLZHDecoder::decompress(pState, 6, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZH7) {
        bResult = XLZHDecoder::decompress(pState, 7, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZOO_LZH) {
        bResult = XLZHDecoder::decompress(pState, 5, pPdStruct, XLZHDecoder::TERMINATION_ZERO_BLOCK);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZOO_LZD) {
        bResult = XLZWDecoder::decompress_zoo(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARC_PACK) {
        bResult = XArcDecoder::decompress(pState, 3, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARC_SQUEEZE) {
        bResult = XArcDecoder::decompress(pState, 4, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARC_CRUNCH_DYN) {
        bResult = XArcDecoder::decompress(pState, 8, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARC_SQUASH) {
        bResult = XArcDecoder::decompress(pState, 9, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ACE) {
        bResult = XAceDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARJ) {
        bResult = XArjDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ARJ_FASTEST) {
        bResult = XArjDecoder::decompressFastest(pState, pPdStruct);
    } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_15) || (compressMethod == XBinary::HANDLE_METHOD_RAR_20) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_29) || (compressMethod == XBinary::HANDLE_METHOD_RAR_50) ||
               (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
        pState->bReadError = false;
        pState->bWriteError = false;
        pState->nCountInput = 0;
        pState->nCountOutput = 0;

        const qint64 nMax = (std::numeric_limits<qint64>::max)();
        if (!pState->pDeviceInput || !pState->pDeviceOutput || !bUncompressedSizeDefined || (nUncompressedSize < 0) || (nWindowSize < 0) ||
            (pState->nProcessedOffset < 0) || (pState->nProcessedLimit < -1) ||
            ((pState->nProcessedLimit != -1) && (pState->nProcessedOffset > (nMax - pState->nProcessedLimit)))) {
            pState->bWriteError = true;
            return false;
        }

        qint64 nEffectiveInputLimit = 0;
        if (!decPrepareBoundedInput(pState->pDeviceInput, pState->nInputOffset, pState->nInputLimit, &nEffectiveInputLimit)) {
            pState->bReadError = true;
            return false;
        }
        if (!decClearOutputDevice(pState->pDeviceOutput)) {
            pState->bWriteError = true;
            return false;
        }

        DecBoundedReadDevice inputDevice(pState->pDeviceInput, nEffectiveInputLimit);
        DecWindowWriteDevice outputDevice(pState);
        if (!inputDevice.open(QIODevice::ReadOnly)) {
            pState->bReadError = true;
            return false;
        }
        if (!outputDevice.open(QIODevice::WriteOnly)) {
            inputDevice.close();
            pState->bWriteError = true;
            return false;
        }

        bool bIsSolid = false;
        std::unique_ptr<rar_Unpack> pRarUnpack(new (std::nothrow) rar_Unpack());
        if (!pRarUnpack) {
            outputDevice.close();
            inputDevice.close();
            pState->bWriteError = true;
            return false;
        }

        pRarUnpack->setDevices(&inputDevice, &outputDevice);
        qint32 nInit = pRarUnpack->Init(nWindowSize, bIsSolid);

        if (nInit > 0) {
            pRarUnpack->SetDestSize(nUncompressedSize);

            if (compressMethod == XBinary::HANDLE_METHOD_RAR_15) {
                pRarUnpack->Unpack15(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_20) {
                pRarUnpack->Unpack20(bIsSolid, pPdStruct);
            } else if (compressMethod == XBinary::HANDLE_METHOD_RAR_29) {
                pRarUnpack->Unpack29(bIsSolid, pPdStruct);
            } else if ((compressMethod == XBinary::HANDLE_METHOD_RAR_50) || (compressMethod == XBinary::HANDLE_METHOD_RAR_70)) {
                pRarUnpack->Unpack5(bIsSolid, pPdStruct);
            }

            bResult = pRarUnpack->IsFileExtracted() && XBinary::isPdStructNotCanceled(pPdStruct) && !inputDevice.hasError() &&
                      !outputDevice.hasError() && !pState->bReadError && !pState->bWriteError && (pState->nCountOutput == nUncompressedSize);
        } else {
            bResult = false;
        }

        pState->nCountInput = inputDevice.consumed();
        if (inputDevice.hasError()) pState->bReadError = true;
        if (outputDevice.hasError()) pState->bWriteError = true;
        outputDevice.close();
        inputDevice.close();
    } else if ((compressMethod == XBinary::HANDLE_METHOD_ZIP_AES) || (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES128) ||
               (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES192 || (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES256))) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();

        if (compressMethod == XBinary::HANDLE_METHOD_ZIP_AES) {
            compressMethod = XBinary::HANDLE_METHOD_ZIP_AES256;
        }

        bResult = XAESDecoder::decrypt(pState, sPassword, compressMethod, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZIPCRYPTO) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XZipCryptoDecoder::decrypt(pState, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_7Z_AES) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XAESDecoder::decrypt(pState, baProperty, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_RAR5_AES) {
        QString sPassword = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
        bResult = XAESDecoder::decryptRar5(pState, sPassword, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_BCJ2) {
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            XBinary::HANDLE_METHOD cmMain =
                (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD4, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropMain = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES).toByteArray();
            qint64 nMainUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE4, (qint64)0).toLongLong();

            XBinary::HANDLE_METHOD cmCall =
                (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD2, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropCall = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES2).toByteArray();
            qint64 nCallUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE2, (qint64)0).toLongLong();
            qint64 nCallOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET2, (qint64)0).toLongLong();
            qint64 nCallSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE2, (qint64)0).toLongLong();

            XBinary::HANDLE_METHOD cmJmp =
                (XBinary::HANDLE_METHOD)pState->mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD3, (quint32)XBinary::HANDLE_METHOD_LZMA).toUInt();
            QByteArray baPropJmp = pState->mapProperties.value(XBinary::FPART_PROP_COMPRESSPROPERTIES3).toByteArray();
            qint64 nJmpUnpack = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE3, (qint64)0).toLongLong();
            qint64 nJmpOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET3, (qint64)0).toLongLong();
            qint64 nJmpSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE3, (qint64)0).toLongLong();

            qint64 nRangeOffset = pState->mapProperties.value(XBinary::FPART_PROP_STREAMOFFSET4, (qint64)0).toLongLong();
            qint64 nRangeSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMSIZE4, (qint64)0).toLongLong();

            qint64 nOutputSize = nUncompressedSize;  // BCJ2 total output set by multiDecompress

            // BCJ2 AES-encrypted layout: each sub-stream separately AES-encrypted before compression
            QString sBCJ2Password = pState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
            QByteArray aBCJ2AESProps[4];
            qint64 aBCJ2AESUnpack[4] = {0, 0, 0, 0};
            aBCJ2AESProps[0] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_0).toByteArray();
            aBCJ2AESUnpack[0] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_0, (qint64)0).toLongLong();
            aBCJ2AESProps[1] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_1).toByteArray();
            aBCJ2AESUnpack[1] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_1, (qint64)0).toLongLong();
            aBCJ2AESProps[2] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_2).toByteArray();
            aBCJ2AESUnpack[2] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_2, (qint64)0).toLongLong();
            aBCJ2AESProps[3] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_PROPS_3).toByteArray();
            aBCJ2AESUnpack[3] = pState->mapProperties.value(XBinary::FPART_PROP_BCJ2_AES_UNPACK_3, (qint64)0).toLongLong();
            bool bBCJ2HasAES = !aBCJ2AESProps[0].isEmpty();

            bool bBCJ2SizesAllowed =
                XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nOutputSize) &&
                XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nMainUnpack) &&
                XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nCallUnpack) &&
                XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties, nJmpUnpack);
            for (qint32 i = 0; i < 4 && bBCJ2SizesAllowed; i++) {
                if (!aBCJ2AESProps[i].isEmpty()) {
                    bBCJ2SizesAllowed =
                        XBinary::isUnpackOutputSizeAllowed(
                            pState->mapUnpackProperties,
                            aBCJ2AESUnpack[i]);
                }
            }

            // Pre-decrypt AES-encrypted BCJ2 sub-streams into temp buffers
            QByteArray aBCJ2Decrypted[4];
            XBinary::UNPACK_MEMORY_RESERVATION aesOutputReservation;
            bool bAESDecryptOk = true;
            if (bBCJ2HasAES && bBCJ2SizesAllowed) {
                qint64 nAesOutputReservation = 0;
                const qint64 nMax =
                    (std::numeric_limits<qint64>::max)();
                for (qint32 i = 0; i < 4; i++) {
                    if (aBCJ2AESProps[i].isEmpty()) continue;
                    if ((aBCJ2AESUnpack[i] < 0) ||
                        (aBCJ2AESUnpack[i] >
                         (std::numeric_limits<qint32>::max)()) ||
                        (nAesOutputReservation >
                         nMax - aBCJ2AESUnpack[i])) {
                        bAESDecryptOk = false;
                        break;
                    }
                    nAesOutputReservation += aBCJ2AESUnpack[i];
                }
                if (bAESDecryptOk &&
                    !aesOutputReservation.acquire(
                        pState->mapUnpackProperties,
                        nAesOutputReservation)) {
                    bAESDecryptOk = false;
                }
                if (bAESDecryptOk) {
                    for (qint32 i = 0; i < 4; i++) {
                        if (!aBCJ2AESProps[i].isEmpty()) {
                            aBCJ2Decrypted[i].reserve(
                                (qint32)aBCJ2AESUnpack[i]);
                        }
                    }
                }
            }
            if (bBCJ2HasAES && bBCJ2SizesAllowed) {
                qint64 aEncOffsets[3] = {pState->nInputOffset, nCallOffset, nJmpOffset};
                qint64 aEncSizes[3] = {pState->nInputLimit, nCallSize, nJmpSize};
                for (qint32 ni = 0; ni < 3 && bAESDecryptOk; ni++) {
                    if (aBCJ2AESProps[ni].isEmpty()) continue;
                    QBuffer decBuf(&aBCJ2Decrypted[ni]);
                    if (!decBuf.open(QIODevice::WriteOnly)) {
                        bAESDecryptOk = false;
                        break;
                    }
                    XBinary::DATAPROCESS_STATE aesState = {};
                    aesState.pDeviceInput = pState->pDeviceInput;
                    aesState.pDeviceOutput = &decBuf;
                    aesState.nInputOffset = aEncOffsets[ni];
                    aesState.nInputLimit = aEncSizes[ni];
                    aesState.mapUnpackProperties =
                        pState->mapUnpackProperties;
                    aesState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, aBCJ2AESUnpack[ni]);
                    const bool bAESInputSeeked =
                        guardedInput->seek(aEncOffsets[ni]);
                    if (!guardedInput || !guardedOutput ||
                        !isContextAlive() || !bAESInputSeeked) {
                        bAESDecryptOk = false;
                        decBuf.close();
                        break;
                    }
                    bAESDecryptOk = XAESDecoder::decrypt(&aesState, aBCJ2AESProps[ni], sBCJ2Password, pPdStruct);
                    if (!guardedInput || !guardedOutput ||
                        !isContextAlive()) return false;
                    decBuf.close();
                }
                // Range stream
                if (bAESDecryptOk && !aBCJ2AESProps[3].isEmpty()) {
                    QBuffer decBuf(&aBCJ2Decrypted[3]);
                    if (decBuf.open(QIODevice::WriteOnly)) {
                        XBinary::DATAPROCESS_STATE aesState = {};
                        aesState.pDeviceInput = pState->pDeviceInput;
                        aesState.pDeviceOutput = &decBuf;
                        aesState.nInputOffset = nRangeOffset;
                        aesState.nInputLimit = nRangeSize;
                        aesState.mapUnpackProperties =
                            pState->mapUnpackProperties;
                        aesState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, aBCJ2AESUnpack[3]);
                        const bool bRangeSeeked =
                            guardedInput->seek(nRangeOffset);
                        if (!guardedInput || !guardedOutput ||
                            !isContextAlive() || !bRangeSeeked) {
                            bAESDecryptOk = false;
                            decBuf.close();
                            return false;
                        }
                        bAESDecryptOk = XAESDecoder::decrypt(&aesState, aBCJ2AESProps[3], sBCJ2Password, pPdStruct);
                        if (!guardedInput || !guardedOutput ||
                            !isContextAlive()) return false;
                        decBuf.close();
                    } else {
                        bAESDecryptOk = false;
                    }
                }
            }

            // nCallUnpack / nJmpUnpack may be 0 when the data contains no CALL/JMP instructions
            // (e.g. pure image or text files). Only require nMainUnpack > 0 and nOutputSize > 0.
            if (nMainUnpack > 0 && nOutputSize > 0 && bBCJ2SizesAllowed && bAESDecryptOk && decIsValidBufferSize(nMainUnpack) && decIsValidBufferSize(nCallUnpack) &&
                decIsValidBufferSize(nJmpUnpack)) {
                const qint64 nMax =
                    (std::numeric_limits<qint64>::max)();
                if ((nMainUnpack > nMax - nCallUnpack) ||
                    (nMainUnpack + nCallUnpack > nMax - nJmpUnpack)) {
                    return false;
                }
                const qint64 nDecodedStreamsSize =
                    nMainUnpack + nCallUnpack + nJmpUnpack;
                XBinary::UNPACK_MEMORY_RESERVATION
                    decodedStreamsReservation;
                if (!decodedStreamsReservation.acquire(
                        pState->mapUnpackProperties,
                        nDecodedStreamsSize)) {
                    return false;
                }
                QByteArray baMain, baCall, baJmp;
                baMain.resize((qint32)nMainUnpack);
                baCall.resize((qint32)nCallUnpack);
                baJmp.resize((qint32)nJmpUnpack);

                struct _BCJ2Task {
                    qint64 nOffset;
                    qint64 nSize;
                    qint64 nOutputSize;
                    XBinary::HANDLE_METHOD cm;
                    QByteArray *pOutput;
                    const QByteArray *pProperty;
                };

                _BCJ2Task tasks[3];
                tasks[0] = {pState->nInputOffset, pState->nInputLimit, nMainUnpack, cmMain, &baMain, &baPropMain};
                tasks[1] = {nCallOffset, nCallSize, nCallUnpack, cmCall, &baCall, &baPropCall};
                tasks[2] = {nJmpOffset, nJmpSize, nJmpUnpack, cmJmp, &baJmp, &baPropJmp};

                bool bLZMAOk = true;
                for (qint32 nTask = 0; nTask < 3 && bLZMAOk && XBinary::isPdStructNotCanceled(pPdStruct); nTask++) {
                    QBuffer outBuf(tasks[nTask].pOutput);
                    if (!outBuf.open(QIODevice::WriteOnly)) {
                        bLZMAOk = false;
                        break;
                    }
                    XBinary::DATAPROCESS_STATE dpState = {};
                    dpState.pDeviceOutput = &outBuf;
                    dpState.nProcessedOffset = 0;
                    dpState.nProcessedLimit = tasks[nTask].nOutputSize;
                    dpState.mapUnpackProperties =
                        pState->mapUnpackProperties;
                    dpState.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, (quint32)tasks[nTask].cm);
                    dpState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, *tasks[nTask].pProperty);
                    dpState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, tasks[nTask].nOutputSize);
                    if (bBCJ2HasAES && !aBCJ2Decrypted[nTask].isEmpty()) {
                        // Use AES-decrypted buffer as LZMA input
                        QBuffer lzmaBuf(&aBCJ2Decrypted[nTask]);
                        if (lzmaBuf.open(QIODevice::ReadOnly)) {
                            dpState.pDeviceInput = &lzmaBuf;
                            dpState.nInputOffset = 0;
                            dpState.nInputLimit = aBCJ2Decrypted[nTask].size();
                            {
                                DecSignalSuppressionGuard signalGuard;
                                bLZMAOk = decompress(&dpState, pPdStruct);
                            }
                            lzmaBuf.close();
                        } else {
                            bLZMAOk = false;
                        }
                    } else {
                        dpState.pDeviceInput = pState->pDeviceInput;
                        dpState.nInputOffset = tasks[nTask].nOffset;
                        dpState.nInputLimit = tasks[nTask].nSize;
                        {
                            DecSignalSuppressionGuard signalGuard;
                            bLZMAOk = decompress(&dpState, pPdStruct);
                        }
                    }
                    outBuf.close();
                }

                if (bLZMAOk && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    // Range coder stream: raw or AES-decrypted
                    QByteArray baRange;
                    XBinary::UNPACK_MEMORY_RESERVATION rangeReservation;
                    if (bBCJ2HasAES && !aBCJ2Decrypted[3].isEmpty()) {
                        baRange = aBCJ2Decrypted[3];
                    } else {
                        if ((nRangeSize < 0) ||
                            (nRangeSize >
                             (std::numeric_limits<qint32>::max)()) ||
                            !XBinary::isUnpackOutputSizeAllowed(
                                pState->mapUnpackProperties,
                                nRangeSize) ||
                            !rangeReservation.acquire(
                                pState->mapUnpackProperties,
                                nRangeSize)) {
                            return false;
                        }
                        const bool bRangeSeeked =
                            guardedInput->seek(nRangeOffset);
                        if (!guardedInput || !guardedOutput ||
                            !isContextAlive() || !bRangeSeeked) return false;
                        baRange = guardedInput->read(nRangeSize);
                        if (!guardedInput || !guardedOutput ||
                            !isContextAlive()) return false;
                    }
                    if (baRange.size() > 0) {
                        QBuffer mainBuf(&baMain);
                        QBuffer callBuf(&baCall);
                        QBuffer jmpBuf(&baJmp);
                        QBuffer rangeBuf(&baRange);
                        if (mainBuf.open(QIODevice::ReadOnly) && callBuf.open(QIODevice::ReadOnly) && jmpBuf.open(QIODevice::ReadOnly)) {
                            if (rangeBuf.open(QIODevice::ReadOnly)) {
                                QIODevice *pBCJ2Output =
                                    XBinary::createUnpackFileBuffer(
                                        nOutputSize,
                                        pState->mapUnpackProperties,
                                        pPdStruct);
                                if (!isContextAlive() || !guardedInput ||
                                    !guardedOutput) {
                                    XBinary::freeFileBuffer(&pBCJ2Output);
                                    return false;
                                }
                                QPointer<QIODevice> guardedBCJ2Output(pBCJ2Output);
                                const bool bBCJ2OutputCleared =
                                    guardedBCJ2Output &&
                                    decClearOutputDevice(
                                        guardedBCJ2Output.data());
                                if (!isContextAlive() || !guardedInput ||
                                    !guardedOutput || !guardedBCJ2Output) {
                                    if (!guardedBCJ2Output)
                                        pBCJ2Output = nullptr;
                                    XBinary::freeFileBuffer(&pBCJ2Output);
                                    return false;
                                }
                                if (bBCJ2OutputCleared) {
                                    const bool bDecoded =
                                        XBCJ2Decoder::decompress(
                                            &mainBuf, &callBuf, &jmpBuf,
                                            &rangeBuf,
                                            guardedBCJ2Output.data(),
                                            nOutputSize, pPdStruct);
                                    if (!isContextAlive() || !guardedInput ||
                                        !guardedOutput ||
                                        !guardedBCJ2Output) {
                                        if (!guardedBCJ2Output)
                                            pBCJ2Output = nullptr;
                                        XBinary::freeFileBuffer(
                                            &pBCJ2Output);
                                        return false;
                                    }
                                    const qint64 nBCJ2OutputSize =
                                        guardedBCJ2Output->size();
                                    if (!isContextAlive() || !guardedInput ||
                                        !guardedOutput ||
                                        !guardedBCJ2Output) {
                                        if (!guardedBCJ2Output)
                                            pBCJ2Output = nullptr;
                                        XBinary::freeFileBuffer(
                                            &pBCJ2Output);
                                        return false;
                                    }
                                    bResult = bDecoded &&
                                              (nBCJ2OutputSize ==
                                               nOutputSize);
                                    if (bResult) {
                                        const qint64 nMax = (std::numeric_limits<qint64>::max)();
                                        bResult = (pState->nInputLimit >= 0) && (nCallSize >= 0) && (nJmpSize >= 0) && (nRangeSize >= 0) &&
                                                  (pState->nInputLimit <= nMax - nCallSize) &&
                                                  (pState->nInputLimit + nCallSize <= nMax - nJmpSize) &&
                                                  (pState->nInputLimit + nCallSize + nJmpSize <= nMax - nRangeSize);
                                        if (bResult) {
                                            pState->nCountInput = pState->nInputLimit + nCallSize + nJmpSize + nRangeSize;
                                            bResult = decEmitDevice(pBCJ2Output, 0, nOutputSize, pState, pPdStruct);
                                        }
                                    }
                                }
                                if (!guardedBCJ2Output)
                                    pBCJ2Output = nullptr;
                                XBinary::freeFileBuffer(&pBCJ2Output);
                            }
                        }
                    }
                }
            } else {
                bResult = false;
            }
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_PDF_CCITTIMAGE) {
        // CCITT Fax image: wrap raw data in a TIFF container
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData;
            XBinary::UNPACK_MEMORY_RESERVATION inputReservation;
            if (!decReadInputToByteArray(pState, &baData,
                                         &inputReservation)) return false;

            qint32 nWidth = pState->mapProperties.value(XBinary::FPART_PROP_WIDTH).toInt();
            qint32 nHeight = pState->mapProperties.value(XBinary::FPART_PROP_HEIGHT).toInt();
            qint32 nCcittK = pState->mapProperties.value(XBinary::FPART_PROP_CCITTK, -1).toInt();

            // TIFF compression type from CCITT /K parameter
            quint16 nTiffCompression = 4;  // Group 4 (default for /K < 0)
            if (nCcittK == 0) {
                nTiffCompression = 3;  // Group 3 1D
            } else if (nCcittK > 0) {
                nTiffCompression = 3;  // Group 3 mixed
            }

            const qint32 nTagCount = 9;
            const qint32 nIfdSize = 2 + nTagCount * 12 + 4;
            const qint32 nStripOffset = 8 + nIfdSize;
            const qint32 nStripSize = baData.size();

            QByteArray baTiff;
            QBuffer tiffBuffer(&baTiff);
            if (!tiffBuffer.open(QIODevice::WriteOnly)) {
                return false;
            }
            QDataStream ds(&tiffBuffer);
            ds.setByteOrder(QDataStream::LittleEndian);

            // TIFF header: "II" (little-endian), magic 42, IFD offset
            ds.writeRawData("II", 2);
            ds << (quint16)42;
            ds << (quint32)8;

            // IFD
            ds << (quint16)nTagCount;
            // Tag 256 (0x0100): ImageWidth
            ds << (quint16)0x0100 << (quint16)3 << (quint32)1 << (quint16)nWidth << (quint16)0;
            // Tag 257 (0x0101): ImageLength
            ds << (quint16)0x0101 << (quint16)3 << (quint32)1 << (quint16)nHeight << (quint16)0;
            // Tag 258 (0x0102): BitsPerSample
            ds << (quint16)0x0102 << (quint16)3 << (quint32)1 << (quint16)1 << (quint16)0;
            // Tag 259 (0x0103): Compression
            ds << (quint16)0x0103 << (quint16)3 << (quint32)1 << (quint16)nTiffCompression << (quint16)0;
            // Tag 262 (0x0106): PhotometricInterpretation (0=WhiteIsZero)
            ds << (quint16)0x0106 << (quint16)3 << (quint32)1 << (quint16)0 << (quint16)0;
            // Tag 273 (0x0111): StripOffsets
            ds << (quint16)0x0111 << (quint16)4 << (quint32)1 << (quint32)nStripOffset;
            // Tag 278 (0x0116): RowsPerStrip
            ds << (quint16)0x0116 << (quint16)3 << (quint32)1 << (quint16)nHeight << (quint16)0;
            // Tag 279 (0x0117): StripByteCounts
            ds << (quint16)0x0117 << (quint16)4 << (quint32)1 << (quint32)nStripSize;
            // Tag 296 (0x0128): ResolutionUnit (1=No absolute unit)
            ds << (quint16)0x0128 << (quint16)3 << (quint32)1 << (quint16)1 << (quint16)0;

            // Next IFD offset (0 = no more IFDs)
            ds << (quint32)0;

            // Strip data
            ds.writeRawData(baData.constData(), nStripSize);

            tiffBuffer.close();

            bResult = XBinary::_writeDevice(baTiff.constData(), baTiff.size(), pState) == baTiff.size();
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_PDF_PALETTE) {
        // Palette data: build RIFF PAL container from decompressed RGB data
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData;
            XBinary::UNPACK_MEMORY_RESERVATION inputReservation;
            if (!decReadInputToByteArray(pState, &baData,
                                         &inputReservation)) return false;

            qint32 nRgbSize = baData.size();
            qint32 nColorCount = nRgbSize / 3;

            if ((nColorCount > 0) && (nColorCount <= 256)) {
                qint32 nDataChunkPayload = 4 + nColorCount * 4;
                qint32 nFileSize = 4 + 8 + nDataChunkPayload;

                QByteArray baPal;
                QBuffer palBuffer(&baPal);
                if (!palBuffer.open(QIODevice::WriteOnly)) {
                    return false;
                }
                QDataStream ds(&palBuffer);
                ds.setByteOrder(QDataStream::LittleEndian);

                // RIFF header
                ds.writeRawData("RIFF", 4);
                ds << (quint32)nFileSize;
                ds.writeRawData("PAL ", 4);

                // data chunk
                ds.writeRawData("data", 4);
                ds << (quint32)nDataChunkPayload;

                // PAL version and color count
                ds << (quint16)0x0300;
                ds << (quint16)nColorCount;

                // RGBX entries
                const quint8 *pRgb = (const quint8 *)baData.constData();

                for (qint32 i = 0; i < nColorCount; ++i) {
                    ds << pRgb[i * 3];      // R
                    ds << pRgb[i * 3 + 1];  // G
                    ds << pRgb[i * 3 + 2];  // B
                    ds << (quint8)0;        // Flags
                }

                palBuffer.close();

                bResult = XBinary::_writeDevice(baPal.constData(), baPal.size(), pState) == baPal.size();
            } else {
                // Fallback: write raw data
                bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
            }
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_PDF_IMAGEDATA) {
        // Raw pixel data image: convert decompressed data to PNG
        if (pState->pDeviceInput && pState->pDeviceOutput) {
            QByteArray baData;
            XBinary::UNPACK_MEMORY_RESERVATION inputReservation;
            if (!decReadInputToByteArray(pState, &baData,
                                         &inputReservation)) return false;

            qint32 nWidth = pState->mapProperties.value(XBinary::FPART_PROP_WIDTH).toInt();
            qint32 nHeight = pState->mapProperties.value(XBinary::FPART_PROP_HEIGHT).toInt();
            qint32 nBitsPerComponent = pState->mapProperties.value(XBinary::FPART_PROP_BITSPERCOMPONENT).toInt();
            QString sColorSpace = pState->mapProperties.value(XBinary::FPART_PROP_COLORSPACE).toString();

            bool bConverted = false;

            if ((nWidth > 0) && (nHeight > 0) && (nBitsPerComponent > 0)) {
                XPNG::COLOR_TYPE pngColorType = XPNG::COLOR_TYPE_RGB;
                qint32 nBytesPerPixel = 0;
                bool bValidFormat = false;

                if ((sColorSpace == QLatin1String("/DeviceRGB")) || sColorSpace.isEmpty()) {
                    if (nBitsPerComponent == 8) {
                        pngColorType = XPNG::COLOR_TYPE_RGB;
                        nBytesPerPixel = 3;
                        bValidFormat = true;
                    }
                } else if (sColorSpace == QLatin1String("/DeviceGray")) {
                    if (nBitsPerComponent == 8) {
                        pngColorType = XPNG::COLOR_TYPE_GRAYSCALE;
                        nBytesPerPixel = 1;
                        bValidFormat = true;
                    } else if (nBitsPerComponent == 1) {
                        pngColorType = XPNG::COLOR_TYPE_GRAYSCALE;
                        nBytesPerPixel = 0;  // 1 bit per pixel
                        bValidFormat = true;
                    }
                } else if (sColorSpace == QLatin1String("/DeviceCMYK")) {
                    if (nBitsPerComponent == 8) {
                        nBytesPerPixel = 4;  // CMYK uses 4 bytes per pixel
                    }
                } else if (sColorSpace == QLatin1String("/Indexed")) {
                    if (nBitsPerComponent == 8) {
                        nBytesPerPixel = 1;
                    }
                }

                // Indexed colorspace with palette
                if ((sColorSpace == QLatin1String("/Indexed")) && (nBitsPerComponent == 8) && (nBytesPerPixel == 1)) {
                    QByteArray baPalette = pState->mapProperties.value(XBinary::FPART_PROP_PALETTE).toByteArray();
                    QString sBaseColorSpace = pState->mapProperties.value(XBinary::FPART_PROP_BASECOLORSPACE).toString();
                    qint64 nExpectedSize = (qint64)nWidth * nHeight;

                    if ((baData.size() >= nExpectedSize) && !baPalette.isEmpty()) {
                        qint32 nColorsPerEntry = 3;  // Default: RGB

                        if (sBaseColorSpace == QLatin1String("/DeviceGray")) {
                            nColorsPerEntry = 1;
                        } else if (sBaseColorSpace == QLatin1String("/DeviceCMYK")) {
                            nColorsPerEntry = 4;
                        }

                        // Convert palette to RGB format (3 bytes per entry) for PNG PLTE chunk
                        qint32 nPalColorCount = baPalette.size() / nColorsPerEntry;
                        QByteArray baRgbPalette;
                        baRgbPalette.reserve(nPalColorCount * 3);
                        const quint8 *pPal = (const quint8 *)baPalette.constData();

                        for (qint32 i = 0; i < nPalColorCount; i++) {
                            quint8 nR = 0;
                            quint8 nG = 0;
                            quint8 nB = 0;

                            if (nColorsPerEntry == 3) {
                                nR = pPal[i * 3];
                                nG = pPal[i * 3 + 1];
                                nB = pPal[i * 3 + 2];
                            } else if (nColorsPerEntry == 1) {
                                nR = pPal[i];
                                nG = pPal[i];
                                nB = pPal[i];
                            } else if (nColorsPerEntry == 4) {
                                quint8 nC = pPal[i * 4];
                                quint8 nM = pPal[i * 4 + 1];
                                quint8 nY = pPal[i * 4 + 2];
                                quint8 nK = pPal[i * 4 + 3];
                                nR = (quint8)(255 - qMin(255, (qint32)nC + nK));
                                nG = (quint8)(255 - qMin(255, (qint32)nM + nK));
                                nB = (quint8)(255 - qMin(255, (qint32)nY + nK));
                            }

                            baRgbPalette.append((char)nR);
                            baRgbPalette.append((char)nG);
                            baRgbPalette.append((char)nB);
                        }

                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNGIndexed(&pngBuffer, nWidth, nHeight, baData.left(nExpectedSize), baRgbPalette);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }

                if (!bConverted && bValidFormat && (nBytesPerPixel > 0)) {
                    qint64 nExpectedSize = (qint64)nWidth * nHeight * nBytesPerPixel;

                    if (baData.size() >= nExpectedSize) {
                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNG(&pngBuffer, nWidth, nHeight, baData.left(nExpectedSize), pngColorType, nBitsPerComponent);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }

                if (!bConverted && (sColorSpace == QLatin1String("/DeviceCMYK")) && (nBitsPerComponent == 8)) {
                    // Manual CMYK to RGB conversion
                    qint64 nExpectedSize = (qint64)nWidth * nHeight * 4;

                    if (baData.size() >= nExpectedSize) {
                        QByteArray baRgbData;
                        baRgbData.resize(nWidth * nHeight * 3);

                        const quint8 *pSrc = (const quint8 *)baData.constData();
                        quint8 *pDst = (quint8 *)baRgbData.data();

                        for (qint32 y = 0; y < nHeight; y++) {
                            for (qint32 x = 0; x < nWidth; x++) {
                                qint32 nSrcIdx = (y * nWidth + x) * 4;
                                qint32 nDstIdx = (y * nWidth + x) * 3;
                                quint8 nC = pSrc[nSrcIdx];
                                quint8 nM = pSrc[nSrcIdx + 1];
                                quint8 nY = pSrc[nSrcIdx + 2];
                                quint8 nK = pSrc[nSrcIdx + 3];

                                pDst[nDstIdx] = (quint8)(255 - qMin(255, (qint32)nC + nK));
                                pDst[nDstIdx + 1] = (quint8)(255 - qMin(255, (qint32)nM + nK));
                                pDst[nDstIdx + 2] = (quint8)(255 - qMin(255, (qint32)nY + nK));
                            }
                        }

                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNG(&pngBuffer, nWidth, nHeight, baRgbData, XPNG::COLOR_TYPE_RGB);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }

                if (!bConverted && (nBitsPerComponent == 1)) {
                    // 1-bit monochrome
                    qint32 nBytesPerRow = (nWidth + 7) / 8;
                    qint64 nExpectedSize = (qint64)nBytesPerRow * nHeight;

                    if (baData.size() >= nExpectedSize) {
                        QBuffer pngBuffer;
                        if (pngBuffer.open(QIODevice::ReadWrite)) {
                            bConverted = XPNG::createPNG(&pngBuffer, nWidth, nHeight, baData.left(nExpectedSize), XPNG::COLOR_TYPE_GRAYSCALE, 1);
                            pngBuffer.close();
                        }

                        if (bConverted) {
                            bResult = XBinary::_writeDevice(pngBuffer.data().constData(), pngBuffer.data().size(), pState) == pngBuffer.data().size();
                        }
                    }
                }
            }

            if (!bConverted) {
                // Fallback: write raw decompressed data
                bResult = XBinary::_writeDevice(baData.constData(), baData.size(), pState) == baData.size();
            }
        }
    } else if ((compressMethod == XBinary::HANDLE_METHOD_STORE_CAB) || (compressMethod == XBinary::HANDLE_METHOD_MSZIP_CAB)) {
        // CAB archive: data is stored in CFDATA blocks with 8-byte headers
        // CFDATA: checksum(4) + cbData(2) + cbUncomp(2) + [reserved] + payload(cbData)
        qint64 nSubstreamOffset = pState->mapProperties.value(XBinary::FPART_PROP_SUBSTREAMOFFSET, 0).toLongLong();
        qint64 nDataReservedSize = pState->mapProperties.value(XBinary::FPART_PROP_OPTHEADER_SIZE, 0).toLongLong();
        qint64 nStreamSize = pState->nInputLimit;
        const qint64 nCFDataHeaderSize = 8;  // sizeof(CFDATA): checksum(4) + cbData(2) + cbUncomp(2)
        bool bTargetRangeValid = (nSubstreamOffset >= 0) && (nUncompressedSize >= 0) &&
                                 (nUncompressedSize <= (std::numeric_limits<qint64>::max)() - nSubstreamOffset);
        qint64 nMinimumFolderSize = bTargetRangeValid ? nSubstreamOffset + nUncompressedSize : -1;
        qint64 nDeclaredFolderSize = pState->mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE, nMinimumFolderSize).toLongLong();
        const qint64 nCabFolderLimit =
            (nConfiguredOutputLimit >= 0)
                ? qMin(DEC_CAB_MAX_FOLDER_SIZE, nConfiguredOutputLimit)
                : DEC_CAB_MAX_FOLDER_SIZE;

        QByteArray baFolderData;
        qint64 nOffset = 0;
        qint64 nDecodedFolderSize = 0;
        qint64 nPhysicalInputConsumed = 0;
        qint64 nEffectiveCabInputSize = 0;
        bResult = bUncompressedSizeDefined && bTargetRangeValid &&
                  (nDataReservedSize >= 0) &&
                  (nDataReservedSize <= 255) && (nStreamSize >= 0) &&
                  (pState->nInputOffset >= 0) &&
                  (nStreamSize <= (std::numeric_limits<qint64>::max)() -
                                     pState->nInputOffset) &&
                  decPrepareBoundedInput(pState->pDeviceInput,
                                         pState->nInputOffset, nStreamSize,
                                         &nEffectiveCabInputSize) &&
                  (nEffectiveCabInputSize == nStreamSize) &&
                  decIsValidBufferSize(nDeclaredFolderSize) &&
                  (nDeclaredFolderSize <= nCabFolderLimit) &&
                  (nDeclaredFolderSize >= nMinimumFolderSize);

        while (bResult && (nOffset < nStreamSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nCFDataHeaderSize + nDataReservedSize > nStreamSize - nOffset) {
                pState->bReadError = true;
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            // Read CFDATA header (little-endian)
            char header[8];
            if (!decReadExactAt(pState->pDeviceInput,
                                pState->nInputOffset + nOffset, header, 8,
                                pState, pPdStruct, &nPhysicalInputConsumed)) {
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            quint16 nCbData = (quint8)header[4] | ((quint16)(quint8)header[5] << 8);
            quint16 nCbUncomp = (quint8)header[6] | ((quint16)(quint8)header[7] << 8);
            const quint32 nDeclaredChecksum =
                (quint8)header[0] | ((quint32)(quint8)header[1] << 8) |
                ((quint32)(quint8)header[2] << 16) |
                ((quint32)(quint8)header[3] << 24);

            qint64 nPayloadOffset = nOffset + nCFDataHeaderSize + nDataReservedSize;

            if ((nCbData == 0) || (nCbUncomp == 0) ||
                (nCbData > DEC_CAB_MAX_DATA_BLOCK_SIZE) ||
                (nCbUncomp > 32768) || ((qint64)nCbData > nStreamSize - nPayloadOffset) ||
                (((qint64)nCbData < nStreamSize - nPayloadOffset) && (nCbUncomp != 32768)) ||
                ((qint64)nCbUncomp > nDeclaredFolderSize - nDecodedFolderSize)) {
                if ((qint64)nCbData > nStreamSize - nPayloadOffset) {
                    pState->bReadError = true;
                }
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            QByteArray baHeaderAndReserve(4 + (qint32)nDataReservedSize, 0);
            memcpy(baHeaderAndReserve.data(), header + 4, 4);
            if ((nDataReservedSize > 0) &&
                !decReadExactAt(pState->pDeviceInput,
                                pState->nInputOffset + nOffset +
                                    nCFDataHeaderSize,
                                baHeaderAndReserve.data() + 4,
                                nDataReservedSize, pState, pPdStruct,
                                &nPhysicalInputConsumed)) {
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            QByteArray baPayload(nCbData, 0);
            if (!decReadExactAt(pState->pDeviceInput,
                                pState->nInputOffset + nPayloadOffset,
                                baPayload.data(), nCbData, pState,
                                pPdStruct, &nPhysicalInputConsumed)) {
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            if (nDeclaredChecksum != 0) {
                quint32 nCalculatedChecksum = decCabDataChecksum(
                    baPayload.constData(), baPayload.size());
                nCalculatedChecksum = decCabDataChecksum(
                    baHeaderAndReserve.constData(), baHeaderAndReserve.size(),
                    nCalculatedChecksum);
                if (nCalculatedChecksum != nDeclaredChecksum) {
                    pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                    bResult = false;
                    break;
                }
            }

            if (compressMethod == XBinary::HANDLE_METHOD_STORE_CAB) {
                if (nCbData != nCbUncomp) {
                    bResult = false;
                    break;
                }
                baFolderData.append(baPayload);
            } else {
                QByteArray baUncompressedBlock;
                if (!decInflateMSZIPBlock(baPayload, baFolderData, nCbUncomp, &baUncompressedBlock, pPdStruct)) {
                    bResult = false;
                    break;
                }

                baFolderData.append(baUncompressedBlock);
            }

            nDecodedFolderSize += nCbUncomp;
            nOffset = nPayloadOffset + nCbData;
        }

        pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
        bResult = bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (nOffset == nStreamSize) &&
                  (nDecodedFolderSize == nDeclaredFolderSize) && (baFolderData.size() == nDeclaredFolderSize);

        if (bResult) {
            // The complete bounded source has been consumed before output is
            // committed, so retain that accounting even if the destination
            // subsequently stalls or cancellation arrives during emission.
            pState->nCountInput = nStreamSize;
            bResult = decEmitByteArray(baFolderData, nSubstreamOffset,
                                       nUncompressedSize, pState, pPdStruct);
        }
    } else if ((compressMethod == XBinary::HANDLE_METHOD_LZX_CAB) || (compressMethod == XBinary::HANDLE_METHOD_QUANTUM_CAB)) {
        // CAB LZX and Quantum share identical CFDATA framing (checksum, per-block
        // boundary, exact cbUncomp result) and both keep decoder state across
        // records; only the decoder call and the window-bits floor differ.
        const bool bQuantumCab = (compressMethod == XBinary::HANDLE_METHOD_QUANTUM_CAB);
        // CAB LZX keeps dictionary state across CFDATA records, but every
        // payload has its own compressed boundary and exact cbUncomp result.
        // The window bits come from CFFOLDER.typeCompress.
        qint64 nSubstreamOffset = pState->mapProperties.value(XBinary::FPART_PROP_SUBSTREAMOFFSET, 0).toLongLong();
        qint64 nDataReservedSize = pState->mapProperties.value(XBinary::FPART_PROP_OPTHEADER_SIZE, 0).toLongLong();
        qint32 nWindowBits = (qint32)pState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, 0).toInt();
        qint64 nStreamSize = pState->nInputLimit;
        const qint64 nCFDataHeaderSize = 8;

        QList<QByteArray> listCompressedBlocks;
        QList<qint32> listUncompressedBlockSizes;
        qint64 nCompressedFolderSize = 0;
        bool bFolderSizeValid = (nSubstreamOffset >= 0) && (nUncompressedSize >= 0) &&
                                (nUncompressedSize <= (std::numeric_limits<qint64>::max)() - nSubstreamOffset);
        qint64 nMinimumFolderSize = bFolderSizeValid ? (nSubstreamOffset + nUncompressedSize) : -1;
        qint64 nFolderUncompressed = pState->mapProperties.value(XBinary::FPART_PROP_STREAMUNPACKEDSIZE, nMinimumFolderSize).toLongLong();
        const qint64 nCabFolderLimit =
            (nConfiguredOutputLimit >= 0)
                ? qMin(DEC_CAB_MAX_FOLDER_SIZE, nConfiguredOutputLimit)
                : DEC_CAB_MAX_FOLDER_SIZE;
        const qint64 nLzxWindowSize =
            ((nWindowBits >= 0) && (nWindowBits < 63))
                ? (Q_INT64_C(1) << nWindowBits)
                : -1;
        qint64 nOffset = 0;
        qint64 nDeclaredBlockOutput = 0;
        qint64 nPhysicalInputConsumed = 0;
        qint64 nEffectiveCabInputSize = 0;
        bResult = bUncompressedSizeDefined && bFolderSizeValid &&
                  (nDataReservedSize >= 0) &&
                  (nDataReservedSize <= 255) && (nStreamSize >= 0) &&
                  (pState->nInputOffset >= 0) &&
                  (nStreamSize <= (std::numeric_limits<qint64>::max)() -
                                     pState->nInputOffset) &&
                  decPrepareBoundedInput(pState->pDeviceInput,
                                         pState->nInputOffset, nStreamSize,
                                         &nEffectiveCabInputSize) &&
                  (nEffectiveCabInputSize == nStreamSize) &&
                  (nWindowBits >= (bQuantumCab ? 10 : 15)) && (nWindowBits <= 21) &&
                  ((nConfiguredOutputLimit < 0) ||
                   ((nLzxWindowSize >= 0) &&
                    (nLzxWindowSize <= nConfiguredOutputLimit))) &&
                  decIsValidBufferSize(nFolderUncompressed) &&
                  (nFolderUncompressed <= nCabFolderLimit) &&
                  (nFolderUncompressed >= nMinimumFolderSize);

        while (bResult && (nOffset < nStreamSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nCFDataHeaderSize + nDataReservedSize > nStreamSize - nOffset) {
                pState->bReadError = true;
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            char header[8];
            if (!decReadExactAt(pState->pDeviceInput,
                                pState->nInputOffset + nOffset, header, 8,
                                pState, pPdStruct, &nPhysicalInputConsumed)) {
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            quint16 nCbData = (quint8)header[4] | ((quint16)(quint8)header[5] << 8);
            quint16 nCbUncomp = (quint8)header[6] | ((quint16)(quint8)header[7] << 8);
            const quint32 nDeclaredChecksum =
                (quint8)header[0] | ((quint32)(quint8)header[1] << 8) |
                ((quint32)(quint8)header[2] << 16) |
                ((quint32)(quint8)header[3] << 24);

            qint64 nPayloadOffset = nOffset + nCFDataHeaderSize + nDataReservedSize;

            if ((nCbData == 0) || (nCbUncomp == 0) ||
                (nCbData > DEC_CAB_MAX_DATA_BLOCK_SIZE) ||
                (nCbUncomp > 32768) || ((qint64)nCbData > nStreamSize - nPayloadOffset) ||
                (((qint64)nCbData < nStreamSize - nPayloadOffset) && (nCbUncomp != 32768)) ||
                !decIsValidBufferSize(nCompressedFolderSize + nCbData) ||
                (nCompressedFolderSize + nCbData >
                 nCabFolderLimit) ||
                ((qint64)nCbUncomp > nFolderUncompressed - nDeclaredBlockOutput)) {
                if ((qint64)nCbData > nStreamSize - nPayloadOffset) {
                    pState->bReadError = true;
                }
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            QByteArray baHeaderAndReserve(4 + (qint32)nDataReservedSize, 0);
            memcpy(baHeaderAndReserve.data(), header + 4, 4);
            if ((nDataReservedSize > 0) &&
                !decReadExactAt(pState->pDeviceInput,
                                pState->nInputOffset + nOffset +
                                    nCFDataHeaderSize,
                                baHeaderAndReserve.data() + 4,
                                nDataReservedSize, pState, pPdStruct,
                                &nPhysicalInputConsumed)) {
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            QByteArray baPayload(nCbData, 0);
            if (!decReadExactAt(pState->pDeviceInput,
                                pState->nInputOffset + nPayloadOffset,
                                baPayload.data(), nCbData, pState,
                                pPdStruct, &nPhysicalInputConsumed)) {
                pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                bResult = false;
                break;
            }

            if (nDeclaredChecksum != 0) {
                quint32 nCalculatedChecksum = decCabDataChecksum(
                    baPayload.constData(), baPayload.size());
                nCalculatedChecksum = decCabDataChecksum(
                    baHeaderAndReserve.constData(), baHeaderAndReserve.size(),
                    nCalculatedChecksum);
                if (nCalculatedChecksum != nDeclaredChecksum) {
                    pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
                    bResult = false;
                    break;
                }
            }

            listCompressedBlocks.append(baPayload);
            listUncompressedBlockSizes.append((qint32)nCbUncomp);
            nCompressedFolderSize += nCbData;
            nDeclaredBlockOutput += nCbUncomp;
            nOffset = nPayloadOffset + nCbData;
        }

        pState->nCountInput = qMin(nPhysicalInputConsumed, nStreamSize);
        bResult = bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (nOffset == nStreamSize) &&
                  (nDeclaredBlockOutput == nFolderUncompressed);

        if (bResult) pState->nCountInput = nStreamSize;
        if (bResult && (nFolderUncompressed == 0)) {
            bResult = (nUncompressedSize == 0) && (nSubstreamOffset == 0) &&
                      listCompressedBlocks.isEmpty() &&
                      decEmitByteArray(QByteArray(), 0, 0, pState,
                                       pPdStruct);
        } else if (bResult) {
            QByteArray baFolderData;
            bResult = bQuantumCab
                          ? XQuantumDecoder::decompressCABDataBlocks(
                                listCompressedBlocks, listUncompressedBlockSizes,
                                &baFolderData, nWindowBits, pPdStruct)
                          : XLZXDecoder::decompressCABDataBlocks(
                                listCompressedBlocks, listUncompressedBlockSizes,
                                &baFolderData, nWindowBits, pPdStruct);

            if (bResult && (baFolderData.size() == nFolderUncompressed)) {
                bResult = decEmitByteArray(baFolderData, nSubstreamOffset,
                                           nUncompressedSize, pState,
                                           pPdStruct);
            } else {
                bResult = false;
            }
        }
    } else if (compressMethod == XBinary::HANDLE_METHOD_ZSTD) {
        bResult = XZstdDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZ4) {
        bResult = XLZ4Decoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZ5) {
        bResult = XLZ5Decoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LIZARD) {
        bResult = XLizardDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZOP) {
        bResult = XLZODecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_COMPRESS) {
        bResult = XCompressDecoder::decompress(pState, pPdStruct);
    } else if (compressMethod == XBinary::HANDLE_METHOD_LZIP) {
        // A lzip file is a concatenation of independently checksummed members.
        // Walk trailers backwards first so framing is known before any output
        // is committed and no raw LZMA decoder can consume a following member.
        struct LzipMember {
            qint64 nOffset;
            qint64 nSize;
            qint64 nCompressedSize;
            qint64 nUncompressedSize;
            quint32 nCRC32;
            quint32 nDictionarySize;
        };

        qint64 nInputSize = 0;
        const qint64 nInputOffset = pState->nInputOffset;
        const bool bLzipSequential = guardedInput->isSequential();
        if (!guardedInput || !guardedOutput || !isContextAlive()) return false;
        bResult = !bLzipSequential &&
                  decPrepareBoundedInput(guardedInput.data(), nInputOffset, pState->nInputLimit, &nInputSize) &&
                  (nInputSize >= 36);

        const auto readExactAt = [pState, &guardedInput, &guardedOutput,
                                  &isContextAlive](qint64 nOffset,
                                                   char *pData,
                                                   qint32 nSize) -> bool {
            if (!pState || !guardedInput || !guardedOutput ||
                !isContextAlive() || !pData || (nOffset < 0) ||
                (nSize <= 0)) {
                if (pState) pState->bReadError = true;
                return false;
            }
            const bool bSeeked = guardedInput->seek(nOffset);
            if (!guardedInput || !guardedOutput || !isContextAlive() ||
                !bSeeked) return false;

            qint32 nReadTotal = 0;
            while (nReadTotal < nSize) {
                const qint64 nRead = guardedInput->read(
                    pData + nReadTotal, nSize - nReadTotal);
                if (!guardedInput || !guardedOutput ||
                    !isContextAlive()) return false;
                if ((nRead <= 0) || (nRead > (nSize - nReadTotal))) {
                    pState->bReadError = true;
                    return false;
                }
                nReadTotal += (qint32)nRead;
            }
            return true;
        };
        const auto readLE32 = [](const char *pData) -> quint32 {
            return (quint32)(quint8)pData[0] | ((quint32)(quint8)pData[1] << 8) |
                   ((quint32)(quint8)pData[2] << 16) | ((quint32)(quint8)pData[3] << 24);
        };
        const auto readLE64 = [](const char *pData) -> quint64 {
            quint64 nValue = 0;
            for (qint32 i = 0; i < 8; i++) nValue |= ((quint64)(quint8)pData[i] << (i * 8));
            return nValue;
        };

        QList<LzipMember> listMembers;
        qint64 nTotalUncompressedSize = 0;
        qint64 nMemberEnd = bResult ? nInputOffset + nInputSize : nInputOffset;
        const qint32 nMaximumMembers = 1000000;

        while (bResult && (nMemberEnd > nInputOffset)) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || ((nMemberEnd - nInputOffset) < 36) ||
                (listMembers.size() >= nMaximumMembers)) {
                bResult = false;
                break;
            }

            char trailer[20] = {};
            bResult = decLzipReadExactAt(pState, nMemberEnd - (qint64)sizeof(trailer), trailer, sizeof(trailer));
            const quint64 nMemberSize64 = bResult ? decLzipReadLE64(trailer + 12) : 0;
            if (!bResult || (nMemberSize64 < 36) || (nMemberSize64 > (quint64)(nMemberEnd - nInputOffset)) ||
                (nMemberSize64 > (quint64)(std::numeric_limits<qint64>::max)())) {
                bResult = false;
                break;
            }

            LzipMember member = {};
            member.nSize = (qint64)nMemberSize64;
            member.nOffset = nMemberEnd - member.nSize;
            member.nCompressedSize = member.nSize - 26;
            const quint64 nDataSize64 = decLzipReadLE64(trailer + 4);
            member.nCRC32 = decLzipReadLE32(trailer);
            if ((member.nCompressedSize < 10) ||
                (nDataSize64 > (quint64)(std::numeric_limits<qint64>::max)())) {
                bResult = false;
                break;
            }
            member.nUncompressedSize = (qint64)nDataSize64;
            if (!XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties,
                    member.nUncompressedSize)) {
                bResult = false;
                break;
            }

            char header[6] = {};
            if (!decLzipReadExactAt(pState, member.nOffset, header, sizeof(header))) {
                bResult = false;
                break;
            }
            const quint8 nDictionaryCode = (quint8)header[5];
            const quint8 nExponent = nDictionaryCode & 0x1F;
            const quint8 nFraction = nDictionaryCode >> 5;
            if ((memcmp(header, "LZIP", 4) != 0) || ((quint8)header[4] != 1) ||
                (nExponent < 12) || (nExponent > 29)) {
                bResult = false;
                break;
            }
            const quint32 nBaseSize = 1U << nExponent;
            member.nDictionarySize = nBaseSize - ((nBaseSize / 16) * nFraction);
            if ((member.nDictionarySize < (1U << 12)) || (member.nDictionarySize > (1U << 29)) ||
                (member.nUncompressedSize > ((std::numeric_limits<qint64>::max)() - nTotalUncompressedSize))) {
                bResult = false;
                break;
            }

            nTotalUncompressedSize += member.nUncompressedSize;
            if (!XBinary::isUnpackOutputSizeAllowed(
                    pState->mapUnpackProperties,
                    nTotalUncompressedSize)) {
                bResult = false;
                break;
            }
            listMembers.append(member);
            nMemberEnd = member.nOffset;
        }

        if (bResult) {
            bResult = !listMembers.isEmpty() && (nMemberEnd == nInputOffset);
        }
        if (bResult && pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
            const qint64 nDeclaredSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
            bResult = (nDeclaredSize >= 0) && (nDeclaredSize == nTotalUncompressedSize);
        }
        if (bResult) std::reverse(listMembers.begin(), listMembers.end());

        for (const LzipMember &member : listMembers) {
            if (!bResult || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                bResult = false;
                break;
            }

            QByteArray baLzmaProperty(5, 0);
            baLzmaProperty[0] = (char)0x5D;  // lc=3, lp=0, pb=2
            baLzmaProperty[1] = (char)(member.nDictionarySize & 0xFF);
            baLzmaProperty[2] = (char)((member.nDictionarySize >> 8) & 0xFF);
            baLzmaProperty[3] = (char)((member.nDictionarySize >> 16) & 0xFF);
            baLzmaProperty[4] = (char)((member.nDictionarySize >> 24) & 0xFF);

            DecLzipCRCWindowWriteDevice crcOutput(pState);
            if (!crcOutput.open(QIODevice::WriteOnly)) {
                pState->bWriteError = true;
                bResult = false;
                break;
            }

            XBinary::DATAPROCESS_STATE lzmaState = {};
            lzmaState.mapUnpackProperties = pState->mapUnpackProperties;
            lzmaState.pDeviceInput = pState->pDeviceInput;
            lzmaState.pDeviceOutput = &crcOutput;
            lzmaState.nInputOffset = member.nOffset + 6;
            lzmaState.nInputLimit = member.nCompressedSize;
            lzmaState.nProcessedOffset = 0;
            lzmaState.nProcessedLimit = -1;
            lzmaState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);

            bResult = XLZMADecoder::decompress(&lzmaState, baLzmaProperty, pPdStruct);
            crcOutput.close();

            pState->bReadError = pState->bReadError || lzmaState.bReadError;
            pState->bWriteError = pState->bWriteError || lzmaState.bWriteError || crcOutput.hasError();
            if (lzmaState.nCountInput == member.nCompressedSize) {
                pState->nCountInput = (member.nOffset - nInputOffset) + member.nSize;
            } else if ((lzmaState.nCountInput >= 0) && (lzmaState.nCountInput <= member.nCompressedSize)) {
                pState->nCountInput = (member.nOffset - nInputOffset) + 6 + lzmaState.nCountInput;
            }

            bResult = bResult && (lzmaState.nCountInput == member.nCompressedSize) &&
                      (lzmaState.nCountOutput == member.nUncompressedSize) &&
                      (crcOutput.produced() == member.nUncompressedSize) &&
                      (crcOutput.crc32() == member.nCRC32) && !pState->bReadError && !pState->bWriteError;
        }

        if (bResult) {
            const bool bEndSeeked = guardedInput->seek(
                nInputOffset + nInputSize);
            if (!guardedInput || !guardedOutput || !isContextAlive())
                return false;
            bResult = (pState->nCountInput == nInputSize) &&
                      (pState->nCountOutput == nTotalUncompressedSize) &&
                      bEndSeeked;
            if (!bResult) pState->bReadError = true;
        }
    } else {
#ifdef QT_DEBUG
        qDebug() << "Unknown compression method" << XBinary::handleMethodToString(compressMethod);
#endif
        // A public signal may synchronously delete both this object and the
        // caller-owned state. Nested multiDecompress() calls suppress the
        // signal; the public entry point returns immediately after reporting.
        if (g_nDecSignalSuppressionDepth == 0) {
            const QString sMessage = QString("%1: %2")
                                         .arg(tr("Unknown compression method"))
                                         .arg(XBinary::handleMethodToString(compressMethod));
            inputStateGuard.dismiss();
            if (!guardedThis || !isContextAlive()) return false;
            Q_EMIT guardedThis->errorMessage(sMessage);
            return false;
        }
        bResult = false;
    }

    bResult = bResult && isContextAlive() && !pState->bReadError &&
              !pState->bWriteError &&
              XBinary::isPdStructNotCanceled(pPdStruct);
    if (!bResult && guardedOutput && isContextAlive()) {
        const bool bSequential = guardedOutput->isSequential();
        if (guardedOutput && isContextAlive() && !bSequential) {
            decClearOutputDevice(guardedOutput.data());
        }
    }

    return bResult;
}

QByteArray XDecompress::decomressToByteArray(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    if (pDevice) {
        QBuffer buffer(&baResult);

        if (buffer.open(QIODevice::ReadWrite)) {
            XBinary::DATAPROCESS_STATE state = {};
            state.pDeviceInput = pDevice;
            state.pDeviceOutput = &buffer;
            state.nInputOffset = nOffset;
            state.nInputLimit = nSize;
            state.nProcessedOffset = 0;
            state.nProcessedLimit = -1;
            state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

            if (!multiDecompress(&state, pPdStruct)) {
                baResult.clear();
            }

            buffer.close();
        }
    }

    return baResult;
}

qint64 XDecompress::getCompressedDataSize(QIODevice *pDevice, qint64 nOffset, qint64 nSize, XBinary::HANDLE_METHOD compressMethod, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || (nOffset < 0) || (nSize < -1)) {
        return 0;
    }

    if (nSize == -1) {
        const qint64 nDeviceSize = pDevice->size();
        if ((nDeviceSize < 0) || (nOffset > nDeviceSize)) return 0;
        nSize = nDeviceSize - nOffset;
    }

    qint64 nResult = 0;
    DecDiscardWriteDevice outputDevice;
    if (outputDevice.open(QIODevice::WriteOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.pDeviceInput = pDevice;
        state.pDeviceOutput = &outputDevice;
        state.nInputOffset = nOffset;
        state.nInputLimit = nSize;
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        if (multiDecompress(&state, pPdStruct)) {
            nResult = state.nCountInput;
        }
        outputDevice.close();
    }

    return nResult;
}
