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
#include "xtarcompressed.h"

#include "xdecompress.h"
#include "xtar_bzip2.h"
#include "xtar_compress.h"
#include "xtar_gz.h"
#include "xtar_lzip.h"
#include "xtar_lzma.h"
#include "xtar_lzop.h"
#include "xtar_xz.h"
#include "xtar_zstd.h"

#include <QBuffer>
#include <QPointer>

namespace {

// Compressed TAR handlers materialize the complete TAR so that the streaming
// archive API can traverse it repeatedly.  Keep that unavoidable allocation
// bounded for untrusted input instead of allowing a small compressed stream to
// grow a QBuffer until the process runs out of memory.
const qint64 TARCOMPRESSED_MAX_DECOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;

class BoundedTarOutputBuffer : public QBuffer {
public:
    BoundedTarOutputBuffer(QByteArray *pData, qint64 nLimit) : QBuffer(pData), m_nLimit(nLimit), m_bLimitExceeded(false) {}

    bool seek(qint64 nPosition) override
    {
        if ((nPosition < 0) || (nPosition > m_nLimit)) {
            m_bLimitExceeded = true;
            return false;
        }

        return QBuffer::seek(nPosition);
    }

    bool isLimitExceeded() const { return m_bLimitExceeded; }

protected:
    qint64 writeData(const char *pData, qint64 nMaximumSize) override
    {
        const qint64 nPosition = pos();
        if ((nMaximumSize < 0) || ((nMaximumSize > 0) && !pData) || (nPosition < 0) ||
            (nPosition > m_nLimit) || (nMaximumSize > (m_nLimit - nPosition))) {
            m_bLimitExceeded = true;
            return -1;
        }

        return QBuffer::writeData(pData, nMaximumSize);
    }

private:
    qint64 m_nLimit;
    bool m_bLimitExceeded;
};

}  // namespace

XTARCOMPRESSED::XTARCOMPRESSED(QIODevice *pDevice) : XTAR(pDevice)
{
    m_pDecompressedData = nullptr;
    m_pOriginalDevice = nullptr;
    m_compressionType = COMPRESSION_UNKNOWN;
    m_nOuterStreamOffset = 0;
    m_nOuterStreamSize = 0;
    m_outerHandleMethod = HANDLE_METHOD_UNKNOWN;
}

XTARCOMPRESSED::~XTARCOMPRESSED()
{
    if (m_pDecompressedData) {
        delete m_pDecompressedData.data();
        m_pDecompressedData = nullptr;
    }
}

bool XTARCOMPRESSED::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XTARCOMPRESSED> guardedArchive(this);
    QPointer<QIODevice> guardedDevice(guardedArchive->getDevice());
    if (!guardedDevice || guardedDevice->isSequential() ||
        !guardedArchive || !guardedDevice) {
        return false;
    }

    const qint64 nOriginalPosition = guardedDevice->pos();
    if (!guardedArchive || !guardedDevice || (nOriginalPosition < 0)) {
        return false;
    }

    UNPACK_STATE state = {};
    const QMap<UNPACK_PROP, QVariant> mapProperties;
    bool bResult = guardedArchive->initUnpack(
        &state, mapProperties, pPdStruct);
    if (!guardedArchive) return false;
    const bool bFinished = guardedArchive->finishUnpack(
        &state, pPdStruct);
    if (!guardedArchive || !guardedDevice || !bFinished) return false;

    if (!guardedDevice->seek(nOriginalPosition) ||
        !guardedArchive || !guardedDevice) {
        bResult = false;
    }

    return bResult;
}

bool XTARCOMPRESSED::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) {
        return false;
    }

    if (guardedDevice->isSequential() || !guardedDevice) {
        return false;
    }

    const qint64 nOriginalPosition = guardedDevice->pos();
    if (!guardedDevice || (nOriginalPosition < 0)) {
        return false;
    }

    const COMPRESSION_TYPE compressionType = detectCompressionType(
        guardedDevice.data());
    if (!guardedDevice) return false;
    QPointer<XArchive> guardedArchive(getCompressionClassInstance(
        compressionType, guardedDevice.data()));

    if (!guardedArchive) {
        return false;
    }

    UNPACK_STATE state = {};
    const QMap<UNPACK_PROP, QVariant> mapProperties;
    bool bResult = guardedArchive->initUnpack(
        &state, mapProperties, pPdStruct);
    if (guardedArchive) {
        const bool bFinished = guardedArchive->finishUnpack(
            &state, pPdStruct);
        if (!guardedArchive || !bFinished) bResult = false;
    } else {
        bResult = false;
    }
    if (guardedArchive) delete guardedArchive.data();

    if (!guardedDevice || !guardedDevice->seek(nOriginalPosition) ||
        !guardedDevice) {
        bResult = false;
    }

    return bResult;
}

XTARCOMPRESSED::COMPRESSION_TYPE XTARCOMPRESSED::detectCompressionType(QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice || guardedDevice->isSequential() || !guardedDevice) {
        return COMPRESSION_UNKNOWN;
    }

    qint64 nOffset = guardedDevice->pos();
    if (!guardedDevice || (nOffset < 0) ||
        !guardedDevice->seek(0) || !guardedDevice) {
        return COMPRESSION_UNKNOWN;
    }

    QByteArray baMagic = guardedDevice->read(6);
    if (!guardedDevice) return COMPRESSION_UNKNOWN;
    bool bRead0 = (baMagic.size() >= 1);
    bool bRead1 = (baMagic.size() >= 2);
    bool bRead2 = (baMagic.size() >= 3);
    bool bRead3 = (baMagic.size() >= 4);
    bool bRead4 = (baMagic.size() >= 5);
    bool bRead5 = (baMagic.size() >= 6);

    quint8 nByte0 = bRead0 ? (quint8)(uchar)baMagic.at(0) : 0;
    quint8 nByte1 = bRead1 ? (quint8)(uchar)baMagic.at(1) : 0;
    quint8 nByte2 = bRead2 ? (quint8)(uchar)baMagic.at(2) : 0;
    quint8 nByte3 = bRead3 ? (quint8)(uchar)baMagic.at(3) : 0;
    quint8 nByte4 = bRead4 ? (quint8)(uchar)baMagic.at(4) : 0;
    quint8 nByte5 = bRead5 ? (quint8)(uchar)baMagic.at(5) : 0;

    COMPRESSION_TYPE result = COMPRESSION_UNKNOWN;

    if (bRead0 && bRead1) {
        if ((nByte0 == 0x1F) && (nByte1 == 0x8B)) {
            result = COMPRESSION_GZIP;
        } else if ((nByte0 == 0x42) && (nByte1 == 0x5A)) {
            result = COMPRESSION_BZIP2;
        } else if ((nByte0 == 0x1F) && (nByte1 == 0x9D)) {
            result = COMPRESSION_COMPRESS;
        } else if (bRead2 && (nByte0 == 0x89) && (nByte1 == 0x4C) && (nByte2 == 0x5A)) {
            result = COMPRESSION_LZOP;
        } else if ((nByte0 == 0x4C) && (nByte1 == 0x5A) && bRead2 && bRead3 && (nByte2 == 0x49) && (nByte3 == 0x50)) {
            result = COMPRESSION_LZIP;
        } else if (bRead3 && (nByte0 == 0x5D) && (nByte1 == 0x00) && (nByte2 == 0x00) && (nByte3 == 0x00)) {
            result = COMPRESSION_LZMA;
        } else if (bRead5 && (nByte0 == 0xFD) && (nByte1 == 0x37) && (nByte2 == 0x7A) && (nByte3 == 0x58) && (nByte4 == 0x5A) && (nByte5 == 0x00)) {
            result = COMPRESSION_XZ;
        } else if (bRead3 && (nByte0 == 0x28) && (nByte1 == 0xB5) && (nByte2 == 0x2F) && (nByte3 == 0xFD)) {
            result = COMPRESSION_ZSTD;
        }
    }

    if (!guardedDevice->seek(nOffset) || !guardedDevice) {
        return COMPRESSION_UNKNOWN;
    }

    return result;
}

XArchive *XTARCOMPRESSED::getCompressionClassInstance(COMPRESSION_TYPE compressionType, QIODevice *pDevice)
{
    XArchive *pResult = nullptr;

    if (compressionType == COMPRESSION_GZIP) {
        pResult = new XTAR_GZ(pDevice);
    } else if (compressionType == COMPRESSION_BZIP2) {
        pResult = new XTAR_BZIP2(pDevice);
    } else if (compressionType == COMPRESSION_XZ) {
        pResult = new XTAR_XZ(pDevice);
    } else if (compressionType == COMPRESSION_LZMA) {
        pResult = new XTAR_LZMA(pDevice);
    } else if (compressionType == COMPRESSION_ZSTD) {
        pResult = new XTAR_ZSTD(pDevice);
    } else if (compressionType == COMPRESSION_COMPRESS) {
        pResult = new XTAR_COMPRESS(pDevice);
    } else if (compressionType == COMPRESSION_LZIP) {
        pResult = new XTAR_LZIP(pDevice);
    } else if (compressionType == COMPRESSION_LZOP) {
        pResult = new XTAR_LZOP(pDevice);
    }

    return pResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XTARCOMPRESSED::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XTAR::getDefaultUnpackProperties();

    return result;
}

struct TARC_INITUNPACK_FAIL_CONTEXT {
    QPointer<XTARCOMPRESSED> *pGuardedArchive;
    XBinary::UNPACK_STATE *pState;
    QPointer<QIODevice> *pDecompressedData;
    QPointer<QIODevice> *pOriginalDevice;
    qint64 *pnOuterStreamOffset;
    qint64 *pnOuterStreamSize;
    XBinary::HANDLE_METHOD *pOuterHandleMethod;
};

static bool tarcFailInitUnpack(TARC_INITUNPACK_FAIL_CONTEXT *pFailContext)
{
    if (!pFailContext->pGuardedArchive->isNull()) {
        QPointer<QIODevice> guardedDecompressed(*(pFailContext->pDecompressedData));
        *(pFailContext->pDecompressedData) = nullptr;
        *(pFailContext->pOriginalDevice) = nullptr;
        *(pFailContext->pnOuterStreamOffset) = 0;
        *(pFailContext->pnOuterStreamSize) = 0;
        *(pFailContext->pOuterHandleMethod) = XBinary::HANDLE_METHOD_UNKNOWN;
        (*(pFailContext->pGuardedArchive))->releaseUnpackSource(pFailContext->pState);
        if (guardedDecompressed) delete guardedDecompressed.data();
    }
    *(pFailContext->pState) = XBinary::UNPACK_STATE();
    return false;
}

bool XTARCOMPRESSED::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTARCOMPRESSED> guardedArchive(this);
    if (!pState || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pState)) {
        return false;
    }
    if (!guardedArchive->finishUnpack(pState, nullptr) ||
        !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<QIODevice> guardedOriginal(guardedArchive->getDevice());
    if (!guardedOriginal ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bBound = guardedArchive->bindUnpackSource(
        pState, pPdStruct);
    if (!guardedArchive || !guardedOriginal || !bBound) return false;

    TARC_INITUNPACK_FAIL_CONTEXT failContext = {};
    failContext.pGuardedArchive = &guardedArchive;
    failContext.pState = pState;
    failContext.pDecompressedData = &m_pDecompressedData;
    failContext.pOriginalDevice = &m_pOriginalDevice;
    failContext.pnOuterStreamOffset = &m_nOuterStreamOffset;
    failContext.pnOuterStreamSize = &m_nOuterStreamSize;
    failContext.pOuterHandleMethod = &m_outerHandleMethod;

    pState->mapUnpackProperties = mapProperties;

    COMPRESSION_TYPE compressionType = guardedArchive->m_compressionType;
    if (compressionType == COMPRESSION_UNKNOWN) {
        compressionType = detectCompressionType(guardedOriginal.data());
        if (!guardedArchive || !guardedOriginal) {
            *pState = UNPACK_STATE();
            return false;
        }
        guardedArchive->m_compressionType = compressionType;
    }

    if (compressionType == COMPRESSION_UNKNOWN) {
        return tarcFailInitUnpack(&failContext);
    }

    guardedArchive->m_pOriginalDevice = guardedOriginal;
    QPointer<QIODevice> guardedDecompressed(
        guardedArchive->decompressData(pPdStruct));
    if (!guardedArchive) {
        if (guardedDecompressed) delete guardedDecompressed.data();
        *pState = UNPACK_STATE();
        return false;
    }
    guardedArchive->m_pDecompressedData = guardedDecompressed;

    if (!guardedDecompressed || !guardedOriginal ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return tarcFailInitUnpack(&failContext);
    }

    // Parse the materialized TAR through a separate view.  Rebinding this
    // archive, even temporarily, advances XBinary's source generation and
    // invalidates the source snapshot held by the hardened extraction path.
    XTAR materializedArchive(guardedDecompressed.data());
    UNPACK_STATE materializedState = {};
    bool bResult = materializedArchive.initUnpack(
        &materializedState, mapProperties, pPdStruct);

    if (!guardedArchive || !guardedDecompressed || !guardedOriginal) {
        materializedArchive.finishUnpack(&materializedState, nullptr);
        *pState = UNPACK_STATE();
        return false;
    }

    bResult = bResult && !materializedState.pContext &&
              (materializedState.nCurrentIndex == 0) &&
              (materializedState.nNumberOfRecords > 0) &&
              (materializedState.nCurrentIndex <
               materializedState.nNumberOfRecords) &&
              (materializedState.nCurrentOffset >= 0) &&
              (materializedState.nTotalSize >= 0) &&
              (materializedState.nCurrentOffset <=
               materializedState.nTotalSize);

    if (bResult) {
        pState->mapUnpackProperties = materializedState.mapUnpackProperties;
        pState->mapArchiveProperties = materializedState.mapArchiveProperties;
        pState->nCurrentOffset = materializedState.nCurrentOffset;
        pState->nTotalSize = materializedState.nTotalSize;
        pState->nCurrentIndex = materializedState.nCurrentIndex;
        pState->nNumberOfRecords = materializedState.nNumberOfRecords;
        pState->pContext = materializedState.pContext;
        materializedState.pContext = nullptr;
        materializedArchive.releaseUnpackSource(&materializedState);
        qint64 nOuterStreamOffset = 0;
        qint64 nOuterStreamSize = 0;
        HANDLE_METHOD outerHandleMethod = HANDLE_METHOD_UNKNOWN;
        (void)guardedArchive->getOuterStreamInfo(
            nOuterStreamOffset, nOuterStreamSize, outerHandleMethod);
        if (!guardedArchive || !guardedOriginal || !guardedDecompressed) {
            *pState = UNPACK_STATE();
            return false;
        }
        guardedArchive->m_nOuterStreamOffset = nOuterStreamOffset;
        guardedArchive->m_nOuterStreamSize = nOuterStreamSize;
        guardedArchive->m_outerHandleMethod = outerHandleMethod;
        bResult = guardedArchive->validateAndFinalizeUnpackSource(
            pState, pPdStruct);
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
    } else {
        materializedArchive.finishUnpack(&materializedState, nullptr);
    }

    return bResult ? true : tarcFailInitUnpack(&failContext);
}

XBinary::ARCHIVERECORD XTARCOMPRESSED::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XTARCOMPRESSED> guardedArchive(this);

    if (!pState ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive) {
        return XBinary::ARCHIVERECORD{};
    }
    QPointer<QIODevice> guardedDecompressed(
        guardedArchive->m_pDecompressedData);
    QPointer<QIODevice> guardedOriginal(guardedArchive->m_pOriginalDevice);
    if (!guardedDecompressed || !guardedOriginal) {
        return XBinary::ARCHIVERECORD{};
    }
    XTAR materializedArchive(guardedDecompressed.data());
    UNPACK_STATE materializedState = *pState;
    materializedState.pContext = nullptr;
    materializedState.baUnpackSourceToken.clear();
    if (!materializedArchive.bindUnpackSource(&materializedState, pPdStruct) ||
        !materializedArchive.validateAndFinalizeUnpackSource(
            &materializedState, pPdStruct) ||
        !guardedArchive || !guardedDecompressed || !guardedOriginal) {
        materializedArchive.releaseUnpackSource(&materializedState);
        return XBinary::ARCHIVERECORD{};
    }
    XBinary::ARCHIVERECORD result =
        materializedArchive.infoCurrent(&materializedState, pPdStruct);
    materializedArchive.releaseUnpackSource(&materializedState);
    if (!guardedArchive || !guardedDecompressed || !guardedOriginal) {
        return XBinary::ARCHIVERECORD{};
    }
    const qint64 nDecompressedSize = guardedDecompressed->size();
    if (!guardedArchive || !guardedDecompressed || !guardedOriginal ||
        result.mapProperties.isEmpty() || (result.nStreamOffset < 0) ||
        (result.nStreamSize < 0) || (nDecompressedSize < 0) ||
        (result.nStreamOffset > nDecompressedSize) ||
        (result.nStreamSize >
         (nDecompressedSize - result.nStreamOffset))) {
        return XBinary::ARCHIVERECORD{};
    }
    // If getOuterStreamInfo populated valid outer-stream metadata, build a solid-block
    // ARCHIVERECORD that decompressArchiveRecord can use directly with the original file.
    // FPART_PROP_SUBSTREAMOFFSET carries the TAR entry's offset in the decompressed buffer.
    if (guardedArchive->m_nOuterStreamSize > 0 &&
        guardedArchive->m_outerHandleMethod != HANDLE_METHOD_UNKNOWN) {
        result.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, result.nStreamOffset);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                    (quint32)guardedArchive->m_outerHandleMethod);
        result.mapProperties.insert(FPART_PROP_ISSOLID, true);
        result.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)0);
        result.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE,
                                    nDecompressedSize);
        result.nStreamOffset = guardedArchive->m_nOuterStreamOffset;
        result.nStreamSize = guardedArchive->m_nOuterStreamSize;
    }
    if (!guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive || !guardedDecompressed || !guardedOriginal) {
        return XBinary::ARCHIVERECORD{};
    }
    return result;
}

bool XTARCOMPRESSED::getOuterStreamInfo(qint64 &nOuterStreamOffset, qint64 &nOuterStreamSize, HANDLE_METHOD &handleMethod)
{
    Q_UNUSED(nOuterStreamOffset)
    Q_UNUSED(nOuterStreamSize)
    Q_UNUSED(handleMethod)
    return false;
}

bool XTARCOMPRESSED::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XTARCOMPRESSED> guardedArchive(this);

    if (!pState ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive) {
        return false;
    }
    QPointer<QIODevice> guardedDecompressed(
        guardedArchive->m_pDecompressedData);
    QPointer<QIODevice> guardedOriginal(guardedArchive->m_pOriginalDevice);
    if (!guardedDecompressed || !guardedOriginal) return false;
    XTAR materializedArchive(guardedDecompressed.data());
    UNPACK_STATE materializedState = *pState;
    materializedState.pContext = nullptr;
    materializedState.baUnpackSourceToken.clear();
    if (!materializedArchive.bindUnpackSource(&materializedState, pPdStruct) ||
        !materializedArchive.validateAndFinalizeUnpackSource(
            &materializedState, pPdStruct) ||
        !guardedArchive || !guardedDecompressed || !guardedOriginal) {
        materializedArchive.releaseUnpackSource(&materializedState);
        return false;
    }
    const bool bResult = materializedArchive.moveToNext(
        &materializedState, pPdStruct);
    materializedArchive.releaseUnpackSource(&materializedState);
    if (!guardedArchive || !guardedDecompressed || !guardedOriginal ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive || (materializedState.nCurrentIndex < 0) ||
        (materializedState.nNumberOfRecords <= 0) ||
        (materializedState.nCurrentIndex >
         materializedState.nNumberOfRecords) ||
        (materializedState.nCurrentOffset < 0) ||
        (materializedState.nTotalSize < 0) ||
        (materializedState.nCurrentOffset >
         materializedState.nTotalSize)) return false;
    pState->nCurrentOffset = materializedState.nCurrentOffset;
    pState->nCurrentIndex = materializedState.nCurrentIndex;
    return bResult;
}

bool XTARCOMPRESSED::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XTARCOMPRESSED> guardedArchive(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !guardedOutput ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedArchive) {
        return false;
    }
    QPointer<QIODevice> guardedOriginal(guardedArchive->m_pOriginalDevice);
    QPointer<QIODevice> guardedDecompressed(
        guardedArchive->m_pDecompressedData);
    if (!guardedOriginal || !guardedDecompressed ||
        !guardedOutput->isOpen() || !guardedArchive || !guardedOutput ||
        !guardedOutput->isWritable() || !guardedArchive || !guardedOutput ||
        guardedOutput->isSequential() || !guardedArchive || !guardedOutput) {
        return false;
    }
    const QIODevice::OpenMode outputMode = guardedOutput->openMode();
    if (!guardedArchive || !guardedOutput ||
        (outputMode & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedOriginal || !guardedDecompressed) return false;
    const bool bAliases = XBinary::devicesAlias(
        guardedOriginal.data(), guardedOutput.data());
    if (!guardedArchive || !guardedOutput || !guardedOriginal ||
        !guardedDecompressed || bAliases) return false;

    const bool bHasOuterStream =
        (guardedArchive->m_nOuterStreamSize > 0) &&
        (guardedArchive->m_outerHandleMethod != HANDLE_METHOD_UNKNOWN);
    bool bResult = false;

    if (bHasOuterStream) {
        // infoCurrent() parses the materialized TAR through an independent
        // view.  Enter the generic archive path with the original compressed
        // source so its snapshot, alias check and decoder input all refer to
        // one stable device.
        if (guardedArchive->getDevice() != guardedOriginal.data()) return false;
        bResult = guardedArchive->XTAR::unpackCurrent(
            pState, guardedOutput.data(), pPdStruct);
        if (!guardedArchive || !guardedOutput || !guardedOriginal ||
            !guardedDecompressed) return false;
    } else {
        // The local TAR view owns its own source token.  Keeping the outer
        // archive bound to the compressed source avoids generation changes
        // and lets reentrant finish/setDevice attempts fail at this guard.
        UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
        if (!operationGuard.isAcquired() ||
            !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) ||
            !guardedArchive || !guardedDecompressed || !guardedOriginal) {
            return false;
        }

        XTAR materializedArchive(guardedDecompressed.data());
        UNPACK_STATE materializedState = *pState;
        materializedState.pContext = nullptr;
        materializedState.baUnpackSourceToken.clear();
        if (!materializedArchive.bindUnpackSource(&materializedState,
                                                  pPdStruct) ||
            !materializedArchive.validateAndFinalizeUnpackSource(
                &materializedState, pPdStruct) ||
            !guardedArchive || !guardedOutput || !guardedOriginal ||
            !guardedDecompressed) {
            materializedArchive.releaseUnpackSource(&materializedState);
            return false;
        }
        bResult = materializedArchive.unpackCurrent(
            &materializedState, guardedOutput.data(), pPdStruct);
        materializedArchive.releaseUnpackSource(&materializedState);

        if (guardedArchive && guardedOutput && guardedOriginal &&
            guardedDecompressed && bResult &&
            guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) &&
            guardedArchive) {
            pState->nCurrentOffset = materializedState.nCurrentOffset;
        } else {
            if (guardedOutput) {
                XBinary::resize(guardedOutput.data(), 0);
                if (guardedOutput) guardedOutput->seek(0);
            }
            bResult = false;
        }
    }

    return bResult && guardedArchive && guardedOutput && guardedOriginal &&
           guardedDecompressed &&
           guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) &&
           guardedArchive && guardedOutput && guardedOriginal &&
           guardedDecompressed;
}

bool XTARCOMPRESSED::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XTARCOMPRESSED> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !guardedArchive->ownsUnpackSource(pState)) {
        return false;
    }
    guardedArchive->releaseUnpackSource(pState);

    if (guardedArchive->m_pDecompressedData) {
        QPointer<QIODevice> guardedDecompressed(
            guardedArchive->m_pDecompressedData);
        guardedArchive->m_pDecompressedData = nullptr;
        if (guardedDecompressed) delete guardedDecompressed.data();
        if (!guardedArchive) {
            *pState = UNPACK_STATE();
            return false;
        }
    }

    guardedArchive->m_pOriginalDevice = nullptr;
    guardedArchive->m_nOuterStreamOffset = 0;
    guardedArchive->m_nOuterStreamSize = 0;
    guardedArchive->m_outerHandleMethod = HANDLE_METHOD_UNKNOWN;

    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = nullptr;

    return true;
}

QIODevice *XTARCOMPRESSED::decompressByMethod(HANDLE_METHOD handleMethod, qint64 nOffset, qint64 nSize, PDSTRUCT *pPdStruct)
{
    QPointer<XTARCOMPRESSED> guardedArchive(this);
    QPointer<QIODevice> guardedDevice(guardedArchive->getDevice());

    if (!guardedDevice) {
        return nullptr;
    }

    qint64 nInputSize = nSize;

    if (nInputSize == -1) {
        const qint64 nSizeFromDevice = guardedDevice->size();
        if (!guardedArchive || !guardedDevice) return nullptr;
        nInputSize = nSizeFromDevice - nOffset;
    }

    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedArchive || !guardedDevice) return nullptr;
    if ((nOffset < 0) || (nInputSize <= 0) || (nDeviceSize < 0) || (nOffset > nDeviceSize) ||
        (nInputSize > (nDeviceSize - nOffset))) {
        return nullptr;
    }

    QByteArray baData;
    BoundedTarOutputBuffer output(&baData, TARCOMPRESSED_MAX_DECOMPRESSED_SIZE);
    if (!output.open(QIODevice::ReadWrite)) {
        return nullptr;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = guardedDevice.data();
    state.pDeviceOutput = &output;
    state.nInputOffset = nOffset;
    state.nInputLimit = nInputSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;
    state.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethod);

    XDecompress decompress;
    const bool bDecoded = decompress.multiDecompress(&state, pPdStruct);
    output.close();

    if (!guardedArchive || !guardedDevice) {
        baData.clear();
        return nullptr;
    }

    // decomressToByteArray() historically returned bytes even when the decoder
    // reported failure.  Do not admit a parseable TAR prefix from a truncated or
    // checksum-invalid compressed stream, and require the exact bounded source
    // extent to have been consumed.
    const bool bComplete = bDecoded && !state.bReadError && !state.bWriteError && !output.isLimitExceeded() &&
                           XBinary::isPdStructNotCanceled(pPdStruct) && (state.nCountInput == nInputSize) &&
                           (state.nCountOutput == baData.size()) && (state.nCountOutput > 0) &&
                           (state.nCountOutput <= TARCOMPRESSED_MAX_DECOMPRESSED_SIZE);
    if (!bComplete) {
        baData.clear();
        return nullptr;
    }

    return XTARCOMPRESSED::createMemoryBuffer(baData);
}

QIODevice *XTARCOMPRESSED::createMemoryBuffer(const QByteArray &baData)
{
    if (baData.isEmpty()) {
        return nullptr;
    }

    QBuffer *pBuffer = new QBuffer();
    pBuffer->setData(baData);

    if (!pBuffer->open(QIODevice::ReadOnly)) {
        delete pBuffer;
        return nullptr;
    }

    return pBuffer;
}

bool XTARCOMPRESSED::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTARCOMPRESSED> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XTAR::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XTAR::INTERNAL_INFO *pInfo =
            static_cast<XTAR::INTERNAL_INFO *>(
                guardedThis->XTAR::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XTAR::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XTARCOMPRESSED::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XTARCOMPRESSED> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XTARCOMPRESSED::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XTAR::setInternalInfo(static_cast<XTAR::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XTAR::setInternalInfo(nullptr);
    }
}
