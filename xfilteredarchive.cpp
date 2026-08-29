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
#include "xfilteredarchive.h"

#include "xdecompress.h"
#include "../Formats/xformats.h"

#include <QBuffer>
#include <QPointer>
#include <QTemporaryFile>

#include <memory>
#include <new>

namespace {

const qint64 FILTERED_MAX_DECODED_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint32 FILTERED_MAX_DEPTH = 4;
const char FILTERED_RECURSION_PROPERTY[] = "_xfileunpacker_filtered_archive_skip";
// Shared with XUU so mixed transport/compression stacks observe one limit.
const char FILTERED_DEPTH_PROPERTY[] = "_xfileunpacker_filter_depth";

// Detection can refine a decoded compression layer directly to a typed TAR
// wrapper (or npm) before this adapter gets a chance to unwrap that layer.
// The final archive state must refer to the final materialized buffer, so do
// not embed an XTARCOMPRESSED reader whose public state describes its private
// decoded TAR rather than the buffer that still contains gzip/bzip2/etc.
XBinary::FT filterEnvelopeType(XBinary::FT fileType)
{
    switch (fileType) {
        case XBinary::FT_TAR_GZ:
        case XBinary::FT_NPM: return XBinary::FT_GZIP;
        case XBinary::FT_TAR_BZIP2: return XBinary::FT_BZIP2;
        case XBinary::FT_TAR_LZIP: return XBinary::FT_LZIP;
        case XBinary::FT_TAR_LZMA: return XBinary::FT_LZMA;
        case XBinary::FT_TAR_LZOP: return XBinary::FT_LZO;
        case XBinary::FT_TAR_XZ: return XBinary::FT_XZ;
        case XBinary::FT_TAR_Z: return XBinary::FT_COMPRESS;
        case XBinary::FT_TAR_ZSTD: return XBinary::FT_ZSTD;
        case XBinary::FT_TAR_LZ4: return XBinary::FT_LZ4;
        default: return fileType;
    }
}

class DevicePropertyOverride {
public:
    DevicePropertyOverride(QIODevice *pDevice, const char *pName, const QVariant &value)
        : m_pDevice(pDevice), m_name(pName), m_oldValue(), m_bHadProperty(false), m_bApplied(false)
    {
        if (!m_pDevice || m_name.isEmpty()) return;
        m_bHadProperty = m_pDevice->dynamicPropertyNames().contains(m_name);
        m_oldValue = m_pDevice->property(m_name.constData());
        m_pDevice->setProperty(m_name.constData(), value);
        m_bApplied = m_pDevice && (m_pDevice->property(m_name.constData()) == value);
    }

    ~DevicePropertyOverride()
    {
        if (!m_pDevice || m_name.isEmpty()) return;
        m_pDevice->setProperty(m_name.constData(), m_bHadProperty ? m_oldValue : QVariant());
    }

    bool isApplied() const
    {
        return m_bApplied && m_pDevice;
    }

private:
    QPointer<QIODevice> m_pDevice;
    QByteArray m_name;
    QVariant m_oldValue;
    bool m_bHadProperty;
    bool m_bApplied;
};

class BoundedFilteredOutputBuffer : public QBuffer {
public:
    explicit BoundedFilteredOutputBuffer(qint64 nLimit) : QBuffer(), m_nLimit(nLimit), m_bLimitExceeded(false)
    {
    }

    bool seek(qint64 nPosition) override
    {
        if ((nPosition < 0) || (nPosition > m_nLimit)) {
            m_bLimitExceeded = true;
            return false;
        }
        return QBuffer::seek(nPosition);
    }

    bool isLimitExceeded() const
    {
        return m_bLimitExceeded;
    }

protected:
    qint64 writeData(const char *pData, qint64 nMaximumSize) override
    {
        const qint64 nPosition = pos();
        if ((nMaximumSize < 0) || ((nMaximumSize > 0) && !pData) || (nPosition < 0) || (nPosition > m_nLimit) || (nMaximumSize > (m_nLimit - nPosition))) {
            m_bLimitExceeded = true;
            return -1;
        }
        return QBuffer::writeData(pData, nMaximumSize);
    }

private:
    qint64 m_nLimit;
    bool m_bLimitExceeded;
};

class NativeArchiveSession {
public:
    explicit NativeArchiveSession(XArchive *pArchive) : m_pArchive(pArchive), m_state(), m_bClosed(false), m_bFinishResult(false)
    {
    }

    ~NativeArchiveSession()
    {
        if (!m_bClosed && m_pArchive) {
            m_pArchive->finishUnpack(&m_state, nullptr);
        }
        if (m_pArchive) delete m_pArchive.data();
    }

    XArchive *archive() const
    {
        return m_pArchive.data();
    }
    XBinary::UNPACK_STATE *state()
    {
        return &m_state;
    }

    bool finish()
    {
        if (m_bClosed) return m_bFinishResult;
        m_bClosed = true;
        if (!m_pArchive) return false;
        m_bFinishResult = m_pArchive->finishUnpack(&m_state, nullptr) && m_pArchive;
        return m_bFinishResult;
    }

private:
    QPointer<XArchive> m_pArchive;
    XBinary::UNPACK_STATE m_state;
    bool m_bClosed;
    bool m_bFinishResult;
};

bool isRecordRangeValid(const XBinary::ARCHIVERECORD &record, qint64 nDeviceSize)
{
    return !record.mapProperties.isEmpty() && (record.nStreamOffset >= 0) && (record.nStreamSize > 0) && (nDeviceSize >= 0) && (record.nStreamOffset <= nDeviceSize) &&
           (record.nStreamSize <= (nDeviceSize - record.nStreamOffset));
}

bool isArchiveRecordRangeValid(const XBinary::ARCHIVERECORD &record, qint64 nDeviceSize)
{
    return !record.mapProperties.isEmpty() && (record.nStreamOffset >= 0) && (record.nStreamSize >= 0) && (nDeviceSize >= 0) && (record.nStreamOffset <= nDeviceSize) &&
           (record.nStreamSize <= (nDeviceSize - record.nStreamOffset));
}

bool isOuterEnvelopeFullyConsumed(XBinary::FT fileType, XBinary::HANDLE_METHOD handleMethod, const XBinary::ARCHIVERECORD &record, qint64 nDeviceSize)
{
    if (!isRecordRangeValid(record, nDeviceSize)) return false;

    const qint64 nStreamEnd = record.nStreamOffset + record.nStreamSize;
    // Native gzip presents only the raw DEFLATE stream.  Its authenticated
    // CRC/ISIZE trailer is the final eight bytes of the enclosing file.  RPM
    // uses the same representation when its payload compressor is gzip.
    if ((fileType == XBinary::FT_GZIP) || ((fileType == XBinary::FT_RPM) && (handleMethod == XBinary::HANDLE_METHOD_DEFLATE))) {
        return (nStreamEnd <= (nDeviceSize - 8)) && (nStreamEnd + 8 == nDeviceSize);
    }

    // All other native single-record filters either expose their complete
    // envelope (including concatenated streams where supported, such as LZ4)
    // or are rejected when bytes remain after the decoded stream.
    return nStreamEnd == nDeviceSize;
}

bool isReadableSeekableDevice(const QPointer<QIODevice> &guardedDevice)
{
    if (!guardedDevice) return false;
    const bool bSequential = guardedDevice->isSequential();
    if (!guardedDevice || bSequential) return false;
    const bool bOpen = guardedDevice->isOpen();
    if (!guardedDevice || !bOpen) return false;
    const bool bReadable = guardedDevice->isReadable();
    return guardedDevice && bReadable;
}

bool getDevicePosition(const QPointer<QIODevice> &guardedDevice, qint64 *pnPosition)
{
    if (pnPosition) *pnPosition = -1;
    if (!guardedDevice || !pnPosition) return false;
    const qint64 nPosition = guardedDevice->pos();
    if (!guardedDevice || (nPosition < 0)) return false;
    *pnPosition = nPosition;
    return true;
}

bool getDeviceSize(const QPointer<QIODevice> &guardedDevice, qint64 *pnSize)
{
    if (pnSize) *pnSize = -1;
    if (!guardedDevice || !pnSize) return false;
    const qint64 nSize = guardedDevice->size();
    if (!guardedDevice || (nSize < 0)) return false;
    *pnSize = nSize;
    return true;
}

}  // namespace

XFilteredArchive::FILTERED_UNPACK_CONTEXT::FILTERED_UNPACK_CONTEXT()
    : pDecodedDevice(nullptr),
      pInnerArchive(nullptr),
      innerState(),
      outerFileType(FT_UNKNOWN),
      innerFileType(FT_UNKNOWN),
      nFilterDepth(0),
      nOuterSize(-1),
      bInnerInitialized(false)
{
}

XFilteredArchive::FILTERED_UNPACK_CONTEXT::~FILTERED_UNPACK_CONTEXT()
{
    finishInner(nullptr);
    delete pInnerArchive;
    pInnerArchive = nullptr;
    delete pDecodedDevice;
    pDecodedDevice = nullptr;
}

bool XFilteredArchive::FILTERED_UNPACK_CONTEXT::finishInner(PDSTRUCT *pPdStruct)
{
    if (!bInnerInitialized) return true;
    if (!pInnerArchive) {
        bInnerInitialized = false;
        innerState = UNPACK_STATE();
        return false;
    }

    QPointer<XArchive> guardedInner(pInnerArchive);
    const bool bResult = guardedInner->finishUnpack(&innerState, pPdStruct) && guardedInner;
    if (!guardedInner) pInnerArchive = nullptr;
    bInnerInitialized = false;
    innerState = UNPACK_STATE();
    return bResult;
}

XFilteredArchive::XFilteredArchive(QIODevice *pDevice, FT outerFileTypeHint) : XArchive(pDevice), m_outerFileTypeHint(outerFileTypeHint)
{
}

XFilteredArchive::~XFilteredArchive()
{
}

bool XFilteredArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XFilteredArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const bool bUsableSource = isReadableSeekableDevice(guardedSource);
    if (!guardedThis || !guardedSource || !bUsableSource) return false;
    const bool bSuppressed = isRecursionSuppressed(guardedSource.data());
    if (!guardedThis || !guardedSource || bSuppressed || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nOriginalPosition = -1;
    if (!getDevicePosition(guardedSource, &nOriginalPosition) || !guardedThis || !guardedSource) return false;

    UNPACK_STATE state = {};
    const bool bInitialized = guardedThis->initUnpack(&state, QMap<UNPACK_PROP, QVariant>(), pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    const bool bFinished = guardedThis->finishUnpack(&state, nullptr);
    if (!guardedThis || !guardedSource) return false;
    const bool bPositionRestored = guardedSource->seek(nOriginalPosition);
    return bInitialized && bFinished && bPositionRestored && guardedThis && guardedSource && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XFilteredArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFilteredArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XFilteredArchive::isValid(QIODevice *pDevice, FT outerFileTypeHint, PDSTRUCT *pPdStruct)
{
    XFilteredArchive archive(pDevice, outerFileTypeHint);
    return archive.isValid(pPdStruct);
}

const char *XFilteredArchive::recursionPropertyName()
{
    return FILTERED_RECURSION_PROPERTY;
}

const char *XFilteredArchive::depthPropertyName()
{
    return FILTERED_DEPTH_PROPERTY;
}

bool XFilteredArchive::isRecursionSuppressed(QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    return guardedDevice && guardedDevice->property(FILTERED_RECURSION_PROPERTY).toBool();
}

bool XFilteredArchive::isFilterFileType(FT fileType)
{
    switch (fileType) {
        case FT_GZIP:
        case FT_BZIP2:
        case FT_XZ:
        case FT_LZIP:
        case FT_LZMA:
        case FT_LZO:
        case FT_COMPRESS:
        case FT_BROTLI:
        case FT_ZSTD:
        case FT_LZ4:
        case FT_LZ5:
        case FT_LIZARD:
        case FT_RPM: return true;
        default: return false;
    }
}

bool XFilteredArchive::isDedicatedCompressedTarFileType(FT fileType)
{
    switch (fileType) {
        case FT_TAR_GZ:
        case FT_TAR_BZIP2:
        case FT_TAR_LZIP:
        case FT_TAR_LZMA:
        case FT_TAR_LZOP:
        case FT_TAR_XZ:
        case FT_TAR_Z:
        case FT_TAR_ZSTD:
        case FT_TAR_LZ4: return true;
        default: return false;
    }
}

XBinary::FT XFilteredArchive::getFileType()
{
    return isFilterFileType(m_outerFileTypeHint) ? m_outerFileTypeHint : FT_ARCHIVE;
}

XBinary::MODE XFilteredArchive::getMode()
{
    return MODE_DATA;
}

QString XFilteredArchive::getMIMEString()
{
    return QStringLiteral("application/x-filtered-archive");
}

qint32 XFilteredArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XFilteredArchive::getFileFormatExt()
{
    switch (m_outerFileTypeHint) {
        case FT_GZIP: return QStringLiteral("gz");
        case FT_BZIP2: return QStringLiteral("bz2");
        case FT_XZ: return QStringLiteral("xz");
        case FT_LZIP: return QStringLiteral("lz");
        case FT_LZMA: return QStringLiteral("lzma");
        case FT_LZO: return QStringLiteral("lzo");
        case FT_COMPRESS: return QStringLiteral("Z");
        case FT_BROTLI: return QStringLiteral("br");
        case FT_ZSTD: return QStringLiteral("zst");
        case FT_LZ4: return QStringLiteral("lz4");
        case FT_LZ5: return QStringLiteral("lz5");
        case FT_LIZARD: return QStringLiteral("liz");
        case FT_RPM: return QStringLiteral("rpm");
        default: return QStringLiteral("archive");
    }
}

QString XFilteredArchive::getFileFormatExtsString()
{
    return QStringLiteral("Native filtered archive");
}

XBinary *XFilteredArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFilteredArchive(pDevice, m_outerFileTypeHint);
}

QMap<XBinary::UNPACK_PROP, QVariant> XFilteredArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XFilteredArchive::readFilterDepth(QIODevice *pDevice, qint32 *pnDepth)
{
    if (pnDepth) *pnDepth = 0;
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice || !pnDepth) return false;

    const QVariant value = guardedDevice->property(FILTERED_DEPTH_PROPERTY);
    if (!guardedDevice) return false;
    if (!value.isValid()) return true;

    bool bOk = false;
    const qint32 nDepth = value.toInt(&bOk);
    if (!bOk || (nDepth < 0) || (nDepth > FILTERED_MAX_DEPTH)) {
        return false;
    }
    *pnDepth = nDepth;
    return true;
}

XBinary::FT XFilteredArchive::detectNativeFileType(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    const bool bUsableDevice = isReadableSeekableDevice(guardedDevice);
    if (!guardedDevice || !bUsableDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FT_UNKNOWN;
    }

    qint64 nOriginalPosition = -1;
    if (!getDevicePosition(guardedDevice, &nOriginalPosition) || !guardedDevice) return FT_UNKNOWN;

    FT result = FT_UNKNOWN;
    {
        DevicePropertyOverride recursionGuard(guardedDevice.data(), FILTERED_RECURSION_PROPERTY, true);
        if (recursionGuard.isApplied() && guardedDevice) {
            result = XFormats::getPrefFileType(guardedDevice.data(), FT_FLAG_ARCHIVES, pPdStruct);
        }
    }

    if (!guardedDevice || !guardedDevice->seek(nOriginalPosition) || !guardedDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return FT_UNKNOWN;
    }
    return result;
}

XArchive *XFilteredArchive::createNativeArchive(FT fileType, QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return nullptr;

    XBinary *pBinary = nullptr;
    {
        DevicePropertyOverride recursionGuard(guardedDevice.data(), FILTERED_RECURSION_PROPERTY, true);
        if (!recursionGuard.isApplied() || !guardedDevice) return nullptr;
        pBinary = XFormats::createClass(fileType, guardedDevice.data());
    }

    if (!guardedDevice || !pBinary) {
        delete pBinary;
        return nullptr;
    }

    XArchive *pArchive = dynamic_cast<XArchive *>(pBinary);
    if (!pArchive || dynamic_cast<XFilteredArchive *>(pArchive)) {
        delete pBinary;
        return nullptr;
    }
    return pArchive;
}

bool XFilteredArchive::publicStateMatchesInner(const UNPACK_STATE *pState, const FILTERED_UNPACK_CONTEXT *pContext)
{
    if (!pState || !pContext || (pState->pContext != pContext) || (pContext->nOuterSize < 0) || (pContext->innerState.nNumberOfRecords < 0) ||
        (pContext->innerState.nCurrentIndex < 0) || (pContext->innerState.nCurrentIndex > pContext->innerState.nNumberOfRecords)) {
        return false;
    }

    const qint64 nExpectedOffset = (pContext->innerState.nCurrentIndex < pContext->innerState.nNumberOfRecords) ? 0 : pContext->nOuterSize;
    return pState && pContext && (pState->pContext == pContext) && (pState->nCurrentOffset == nExpectedOffset) && (pState->nTotalSize == pContext->nOuterSize) &&
           (pState->nCurrentIndex == pContext->innerState.nCurrentIndex) && (pState->nNumberOfRecords == pContext->innerState.nNumberOfRecords);
}

void XFilteredArchive::copyInnerState(UNPACK_STATE *pState, const FILTERED_UNPACK_CONTEXT *pContext)
{
    if (!pState || !pContext) return;
    pState->nCurrentOffset = (pContext->innerState.nCurrentIndex < pContext->innerState.nNumberOfRecords) ? 0 : pContext->nOuterSize;
    pState->nTotalSize = pContext->nOuterSize;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->mapUnpackProperties = pContext->innerState.mapUnpackProperties;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
}

QBuffer *XFilteredArchive::materializeLayer(QIODevice *pInputDevice, FT fileType, const QMap<UNPACK_PROP, QVariant> &mapProperties, UNPACK_STATE *pOuterState,
                                            PDSTRUCT *pPdStruct)
{
    QPointer<XFilteredArchive> guardedThis(this);
    QPointer<QIODevice> guardedInput(pInputDevice);
    if (!guardedInput || !pOuterState || !isFilterFileType(fileType)) {
        return nullptr;
    }
    const bool bUsableInput = isReadableSeekableDevice(guardedInput);
    if (!guardedThis || !guardedInput || !bUsableInput || !XBinary::isPdStructNotCanceled(pPdStruct)) return nullptr;
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pOuterState, pPdStruct);
    if (!guardedThis || !guardedInput || !bSourceCurrent) return nullptr;

    qint64 nOriginalPosition = -1;
    if (!getDevicePosition(guardedInput, &nOriginalPosition) || !guardedThis || !guardedInput) return nullptr;
    qint64 nInputSize = -1;
    if (!getDeviceSize(guardedInput, &nInputSize) || !guardedThis || !guardedInput || (nInputSize <= 0)) {
        return nullptr;
    }

    NativeArchiveSession nativeSession(createNativeArchive(fileType, guardedInput.data()));
    QPointer<XArchive> guardedNative(nativeSession.archive());
    bool bResult = guardedNative && guardedThis && guardedInput;
    if (bResult) {
        bResult = guardedNative->initUnpack(nativeSession.state(), mapProperties, pPdStruct);
    }
    if (!guardedThis || !guardedInput || !guardedNative) bResult = false;
    if (bResult) {
        bResult = guardedThis->isUnpackSourceCurrent(pOuterState, pPdStruct) && guardedThis && guardedInput && guardedNative && XBinary::isPdStructNotCanceled(pPdStruct);
    }

    UNPACK_STATE *pNativeState = nativeSession.state();
    if (bResult) {
        bResult = (pNativeState->nCurrentIndex == 0) && (pNativeState->nNumberOfRecords == 1) && (pNativeState->nCurrentOffset >= 0) &&
                  (pNativeState->nTotalSize == nInputSize) && (pNativeState->nCurrentOffset <= pNativeState->nTotalSize);
    }

    ARCHIVERECORD record = {};
    if (bResult) {
        record = guardedNative->infoCurrent(pNativeState, pPdStruct);
        if (!guardedThis || !guardedInput || !guardedNative) {
            bResult = false;
        }
    }

    bool bMethodOk = false;
    const HANDLE_METHOD handleMethod = static_cast<HANDLE_METHOD>(record.mapProperties.value(FPART_PROP_HANDLEMETHOD).toUInt(&bMethodOk));
    if (bResult) {
        bResult = isOuterEnvelopeFullyConsumed(fileType, handleMethod, record, nInputSize) && bMethodOk && (handleMethod != HANDLE_METHOD_UNKNOWN) &&
                  !record.mapProperties.value(FPART_PROP_ISFOLDER, false).toBool() && (pNativeState->nCurrentIndex == 0) && (pNativeState->nNumberOfRecords == 1) &&
                  guardedThis->isUnpackSourceCurrent(pOuterState, pPdStruct) && guardedThis && guardedInput && guardedNative && XBinary::isPdStructNotCanceled(pPdStruct);
    }

    qint64 nExpectedSize = -1;
    qint64 nConfiguredLimit = -1;
    qint64 nEffectiveLimit = FILTERED_MAX_DECODED_SIZE;
    if (bResult) {
        bResult = XBinary::getUnpackOutputLimit(mapProperties, &nConfiguredLimit);
        if (bResult && (nConfiguredLimit >= 0)) {
            nEffectiveLimit = qMin(nEffectiveLimit, nConfiguredLimit);
        }
    }
    if (bResult && record.mapProperties.contains(FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bSizeOk = false;
        nExpectedSize = record.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bSizeOk);
        bResult = bSizeOk && (nExpectedSize >= 0) && (nExpectedSize <= nEffectiveLimit);
    }

    std::unique_ptr<BoundedFilteredOutputBuffer> pDecoded;
    if (bResult) {
        pDecoded.reset(new (std::nothrow) BoundedFilteredOutputBuffer(nEffectiveLimit));
        bResult = pDecoded && pDecoded->open(QIODevice::ReadWrite);
    }

    XBinary::DATAPROCESS_STATE decompressState = {};
    if (bResult) {
        decompressState.pDeviceInput = guardedInput.data();
        decompressState.pDeviceOutput = pDecoded.get();
        decompressState.nInputOffset = record.nStreamOffset;
        decompressState.nInputLimit = record.nStreamSize;
        decompressState.nProcessedOffset = 0;
        decompressState.nProcessedLimit = -1;
        decompressState.mapProperties = record.mapProperties;
        decompressState.mapUnpackProperties = pNativeState->mapUnpackProperties;

        XDecompress decompressor;
        bResult = decompressor.multiDecompress(&decompressState, pPdStruct);
        if (!guardedThis || !guardedInput || !guardedNative) {
            bResult = false;
        }
    }

    qint64 nDecodedSize = -1;
    if (bResult) {
        nDecodedSize = pDecoded->size();
        bResult = !pDecoded->isLimitExceeded() && (nDecodedSize > 0) && (nDecodedSize <= nEffectiveLimit) && (decompressState.nCountInput == record.nStreamSize) &&
                  (decompressState.nCountOutput == nDecodedSize) && ((nExpectedSize < 0) || (nDecodedSize == nExpectedSize)) &&
                  guardedThis->isUnpackSourceCurrent(pOuterState, pPdStruct) && guardedThis && guardedInput && guardedNative && XBinary::isPdStructNotCanceled(pPdStruct);
    }

    if (bResult) {
        const bool bMoved = guardedNative->moveToNext(pNativeState, pPdStruct);
        if (!guardedThis || !guardedInput || !guardedNative) {
            bResult = false;
        } else {
            bResult = !bMoved && (pNativeState->nCurrentIndex == 1) && (pNativeState->nNumberOfRecords == 1) && XBinary::isPdStructNotCanceled(pPdStruct) &&
                      guardedThis->isUnpackSourceCurrent(pOuterState, pPdStruct) && guardedThis && guardedInput && guardedNative;
        }
    }

    const bool bFinished = nativeSession.finish();
    if (!guardedThis || !guardedInput) bResult = false;
    const bool bPositionRestored = guardedInput && guardedInput->seek(nOriginalPosition);
    bResult = bResult && bFinished && bPositionRestored && guardedThis && guardedInput && XBinary::isPdStructNotCanceled(pPdStruct) &&
              guardedThis->isUnpackSourceCurrent(pOuterState, pPdStruct) && guardedThis && guardedInput;

    if (!bResult || !pDecoded) return nullptr;
    pDecoded->close();
    if (!pDecoded->open(QIODevice::ReadOnly) || !pDecoded->seek(0)) {
        return nullptr;
    }
    return pDecoded.release();
}

bool XFilteredArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XFilteredArchive> guardedThis(this);
    if (!pState || m_bUnpackOperationInProgress || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState))) {
        return false;
    }
    if (!guardedThis->finishUnpack(pState, nullptr) || !guardedThis) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedThis || !guardedSource) return false;
    const bool bUsableSource = isReadableSeekableDevice(guardedSource);
    if (!guardedThis || !guardedSource || !bUsableSource) return false;
    const bool bSuppressed = isRecursionSuppressed(guardedSource.data());
    if (!guardedThis || !guardedSource || bSuppressed || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    qint64 nOriginalPosition = -1;
    if (!getDevicePosition(guardedSource, &nOriginalPosition) || !guardedThis || !guardedSource) return false;
    qint64 nOuterSize = -1;
    if (!getDeviceSize(guardedSource, &nOuterSize) || !guardedThis || !guardedSource || (nOuterSize <= 0)) {
        return false;
    }

    const bool bBound = guardedThis->bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !guardedSource || !bBound) return false;

    const auto failInitialization = [&]() -> bool {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    };

    qint32 nDepth = 0;
    if (!readFilterDepth(guardedSource.data(), &nDepth) || !guardedThis || !guardedSource) {
        return failInitialization();
    }

    FT currentType = detectNativeFileType(guardedSource.data(), pPdStruct);
    if (!guardedThis || !guardedSource || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !guardedSource ||
        ((m_outerFileTypeHint != FT_UNKNOWN) && (!isFilterFileType(m_outerFileTypeHint) || (currentType != m_outerFileTypeHint))) ||
        isDedicatedCompressedTarFileType(currentType) || !isFilterFileType(currentType)) {
        return failInitialization();
    }
    const FT outerFileType = currentType;

    QIODevice *pCurrentDevice = guardedSource.data();
    std::unique_ptr<QBuffer> pCurrentOwned;
    while (isFilterFileType(filterEnvelopeType(currentType))) {
        if ((nDepth < 0) || (nDepth >= FILTERED_MAX_DEPTH) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return failInitialization();
        }

        const FT layerType = filterEnvelopeType(currentType);

        std::unique_ptr<QBuffer> pDecoded(guardedThis->materializeLayer(pCurrentDevice, layerType, mapProperties, pState, pPdStruct));
        if (!guardedThis || !guardedSource || !pDecoded) {
            return failInitialization();
        }

        ++nDepth;
        pDecoded->setProperty(FILTERED_DEPTH_PROPERTY, nDepth);
        if (pDecoded->property(FILTERED_DEPTH_PROPERTY).toInt() != nDepth) {
            return failInitialization();
        }

        pCurrentOwned.swap(pDecoded);
        pCurrentDevice = pCurrentOwned.get();
        currentType = detectNativeFileType(pCurrentDevice, pPdStruct);
        if (!guardedThis || !guardedSource || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !guardedSource || (currentType == FT_UNKNOWN)) {
            return failInitialization();
        }
    }

    const FT innerFileType = currentType;
    if (!pCurrentOwned || !XFormats::isArchive(innerFileType) || isFilterFileType(innerFileType)) {
        return failInitialization();
    }

    std::unique_ptr<FILTERED_UNPACK_CONTEXT> pContext(new (std::nothrow) FILTERED_UNPACK_CONTEXT());
    if (!pContext) return failInitialization();
    pContext->pDecodedDevice = pCurrentOwned.release();
    pContext->outerFileType = outerFileType;
    pContext->innerFileType = innerFileType;
    pContext->nFilterDepth = nDepth;
    pContext->nOuterSize = nOuterSize;
    pContext->pInnerArchive = createNativeArchive(innerFileType, pContext->pDecodedDevice);
    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    QPointer<QIODevice> guardedDecoded(pContext->pDecodedDevice);
    if (!guardedInner || !guardedDecoded || !guardedThis || !guardedSource) {
        return failInitialization();
    }

    bool bResult = guardedInner->initUnpack(&pContext->innerState, mapProperties, pPdStruct);
    if (bResult && guardedInner) pContext->bInnerInitialized = true;
    if (!guardedThis || !guardedSource || !guardedInner || !guardedDecoded || !bResult || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !guardedSource) {
        return failInitialization();
    }

    const qint64 nDecodedSize = guardedDecoded->size();
    if (!guardedThis || !guardedSource || !guardedInner || !guardedDecoded || (nDecodedSize < 0)) {
        return failInitialization();
    }
    const qint32 nInnerRecords = pContext->innerState.nNumberOfRecords;
    bResult = (pContext->innerState.nCurrentIndex == 0) && (nInnerRecords >= 0) && (pContext->innerState.nCurrentOffset >= 0) &&
              (pContext->innerState.nTotalSize == nDecodedSize) && (pContext->innerState.nCurrentOffset <= pContext->innerState.nTotalSize);

    ARCHIVERECORD firstRecord = {};
    if (bResult && (nInnerRecords > 0)) {
        firstRecord = guardedInner->infoCurrent(&pContext->innerState, pPdStruct);
        if (!guardedThis || !guardedSource || !guardedInner || !guardedDecoded) {
            bResult = false;
        }
    }
    if (bResult && (nInnerRecords > 0)) {
        bResult = isArchiveRecordRangeValid(firstRecord, nDecodedSize) && (pContext->innerState.nCurrentIndex == 0) &&
                  (pContext->innerState.nNumberOfRecords == nInnerRecords) && XBinary::isPdStructNotCanceled(pPdStruct) &&
                  guardedThis->isUnpackSourceCurrent(pState, pPdStruct) && guardedThis && guardedSource && guardedInner && guardedDecoded;
    }
    if (!bResult) return failInitialization();

    const bool bPositionRestored = guardedSource->seek(nOriginalPosition);
    if (!guardedThis || !guardedSource || !guardedInner || !guardedDecoded || !bPositionRestored || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedSource) {
        return failInitialization();
    }

    copyInnerState(pState, pContext.get());
    FILTERED_UNPACK_CONTEXT *pRawContext = pContext.release();
    pState->pContext = pRawContext;
    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pRawContext, pPdStruct);
    if (!guardedThis) {
        *pState = UNPACK_STATE();
        return false;
    }
    if (!bFinalized) {
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pRawContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XFilteredArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return ARCHIVERECORD();
    QPointer<XFilteredArchive> guardedThis(this);
    if (!pState || !XBinary::isPdStructNotCanceled(pPdStruct) || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis) {
        return ARCHIVERECORD();
    }

    FILTERED_UNPACK_CONTEXT *pContext = static_cast<FILTERED_UNPACK_CONTEXT *>(pState->pContext);
    if (!publicStateMatchesInner(pState, pContext) || !pContext->bInnerInitialized || !pContext->pInnerArchive || !pContext->pDecodedDevice) {
        return ARCHIVERECORD();
    }

    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    QPointer<QIODevice> guardedDecoded(pContext->pDecodedDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedInner || !guardedDecoded || !guardedSource) {
        return ARCHIVERECORD();
    }

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    const qint32 nIndex = pContext->innerState.nCurrentIndex;
    const qint32 nRecords = pContext->innerState.nNumberOfRecords;
    ARCHIVERECORD result = guardedInner->infoCurrent(&pContext->innerState, pPdStruct);
    qint64 nDecodedSize = -1;
    if (guardedDecoded) nDecodedSize = guardedDecoded->size();
    if (!guardedThis || !guardedInner || !guardedDecoded || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !guardedInner || !guardedDecoded || !guardedSource ||
        !isArchiveRecordRangeValid(result, nDecodedSize) || (pContext->innerState.nCurrentIndex != nIndex) || (pContext->innerState.nNumberOfRecords != nRecords)) {
        return ARCHIVERECORD();
    }
    if (!XBinary::markArchiveStreamRecord(&result, nIndex)) {
        return ARCHIVERECORD();
    }
    return result;
}

bool XFilteredArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XFilteredArchive> guardedThis(this);
    if (!pState || !pDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedOutput || !guardedSource || !guardedThis->isUnpackOutputSupported(guardedOutput.data()) || !guardedThis || !guardedOutput || !guardedSource ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) || !guardedThis || !guardedOutput || !guardedSource ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !guardedOutput || !guardedSource) {
        return false;
    }

    FILTERED_UNPACK_CONTEXT *pContext = static_cast<FILTERED_UNPACK_CONTEXT *>(pState->pContext);
    if (!publicStateMatchesInner(pState, pContext) || !pContext->bInnerInitialized || !pContext->pInnerArchive || !pContext->pDecodedDevice) {
        return false;
    }

    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    QPointer<QIODevice> guardedDecoded(pContext->pDecodedDevice);
    if (!guardedInner || !guardedDecoded || XBinary::devicesAlias(guardedDecoded.data(), guardedOutput.data()) || !guardedThis || !guardedInner || !guardedDecoded ||
        !guardedOutput || !guardedSource) {
        return false;
    }

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    const qint32 nIndex = pContext->innerState.nCurrentIndex;
    const qint32 nRecords = pContext->innerState.nNumberOfRecords;
    ARCHIVERECORD record = guardedInner->infoCurrent(&pContext->innerState, pPdStruct);
    qint64 nDecodedSize = -1;
    if (guardedDecoded) nDecodedSize = guardedDecoded->size();
    if (!guardedThis || !guardedInner || !guardedDecoded || !guardedOutput || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !guardedInner || !guardedDecoded || !guardedOutput || !guardedSource ||
        !isArchiveRecordRangeValid(record, nDecodedSize) || (pContext->innerState.nCurrentIndex != nIndex) || (pContext->innerState.nNumberOfRecords != nRecords)) {
        return false;
    }

    qint64 nExpectedSize = -1;
    if (record.mapProperties.contains(FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bSizeOk = false;
        nExpectedSize = record.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bSizeOk);
        if (!bSizeOk || (nExpectedSize < 0)) return false;
    }

    QIODevice *pStage = nullptr;
    if (nExpectedSize >= 0) {
        if (!XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nExpectedSize)) return false;
        pStage = XBinary::createFileBuffer(nExpectedSize, pPdStruct);
    } else {
        QTemporaryFile *pTemporaryFile = new (std::nothrow) QTemporaryFile();
        if (pTemporaryFile && pTemporaryFile->open()) {
            pStage = pTemporaryFile;
        } else {
            delete pTemporaryFile;
        }
    }
    QPointer<QIODevice> guardedStage(pStage);
    if (!guardedThis || !guardedInner || !guardedDecoded || !guardedOutput || !guardedSource || !guardedStage || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis) {
        if (!guardedStage) pStage = nullptr;
        XBinary::freeFileBuffer(&pStage);
        return false;
    }

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    // Thread the operation budget into the inner session: the inner
    // unpackCurrent route performs its own per-entry and produced-byte
    // accounting, so no outer beginEntry/debit here.  The subsequent
    // publishUnpackOutput stage-to-caller copy is never charged.
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = guardedInner->unpackCurrent(&pContext->innerState, guardedStage.data(), pPdStruct);
    if (!guardedThis || !guardedInner || !guardedDecoded || !guardedOutput || !guardedSource || !guardedStage) {
        bResult = false;
    }
    if (bResult) {
        const qint64 nStageSize = guardedStage->size();
        bResult =
            (nStageSize >= 0) && XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nStageSize) && ((nExpectedSize < 0) || (nStageSize == nExpectedSize)) &&
            (pContext->innerState.nCurrentIndex == nIndex) && (pContext->innerState.nNumberOfRecords == nRecords) && XBinary::isPdStructNotCanceled(pPdStruct) &&
            guardedThis->isUnpackSourceCurrent(pState, pPdStruct) && guardedThis && guardedInner && guardedDecoded && guardedOutput && guardedSource && guardedStage;
    }
    if (bResult) {
        bResult = guardedThis->publishUnpackOutput(guardedStage.data(), guardedOutput.data(), pState, pPdStruct);
    }
    if (bResult && guardedThis && guardedInner && guardedDecoded && guardedOutput && guardedSource && guardedStage) {
        copyInnerState(pState, pContext);
    } else {
        bResult = false;
    }

    if (!guardedStage) pStage = nullptr;
    XBinary::freeFileBuffer(&pStage);
    return bResult;
}

bool XFilteredArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XFilteredArchive> guardedThis(this);
    if (!pState || !XBinary::isPdStructNotCanceled(pPdStruct) || !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis) {
        return false;
    }

    FILTERED_UNPACK_CONTEXT *pContext = static_cast<FILTERED_UNPACK_CONTEXT *>(pState->pContext);
    if (!publicStateMatchesInner(pState, pContext) || !pContext->bInnerInitialized || !pContext->pInnerArchive || !pContext->pDecodedDevice) {
        return false;
    }

    QPointer<XArchive> guardedInner(pContext->pInnerArchive);
    QPointer<QIODevice> guardedDecoded(pContext->pDecodedDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());
    if (!guardedInner || !guardedDecoded || !guardedSource) return false;

    pContext->innerState.mapUnpackProperties = pState->mapUnpackProperties;
    const qint32 nPreviousIndex = pContext->innerState.nCurrentIndex;
    const qint32 nRecords = pContext->innerState.nNumberOfRecords;
    if ((nPreviousIndex < 0) || (nPreviousIndex >= nRecords)) return false;

    const bool bMoved = guardedInner->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !guardedInner || !guardedDecoded || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedThis->isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !guardedInner || !guardedDecoded || !guardedSource ||
        (pContext->innerState.nNumberOfRecords != nRecords) || (pContext->innerState.nCurrentIndex != (nPreviousIndex + 1)) ||
        (bMoved != (pContext->innerState.nCurrentIndex < nRecords))) {
        return false;
    }

    copyInnerState(pState, pContext);
    return bMoved;
}

bool XFilteredArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    QPointer<XFilteredArchive> guardedThis(this);

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState)) {
        return false;
    }

    FILTERED_UNPACK_CONTEXT *pContext = static_cast<FILTERED_UNPACK_CONTEXT *>(pState->pContext);
    guardedThis->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    const bool bInnerFinished = pContext ? pContext->finishInner(nullptr) : true;
    delete pContext;
    if (!guardedThis) return false;
    *pState = UNPACK_STATE();
    return bInnerFinished;
}
