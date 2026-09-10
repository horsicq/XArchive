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
#include "xrib.h"

#include <QBuffer>
#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <limits>
#include <new>

namespace {
const qint64 RIB_HEADER_SIZE = 8;
const qint64 RIB_PREFIX_SIZE = 32;
const qint64 RIB_MAX_UNCOMPRESSED_SIZE = 512LL * 1024 * 1024;
const quint32 RIB_CANCEL_INTERVAL_MASK = 0x3fffU;

bool ribStartsWith(const QByteArray &baData, const char *pValue, qint32 nValueSize)
{
    return pValue && (nValueSize >= 0) && (baData.size() >= nValueSize) && (memcmp(baData.constData(), pValue, static_cast<size_t>(nValueSize)) == 0);
}
}  // namespace

XRIB::XRIB(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XRIB::readHeaderInfo(RIB_HEADER_INFO *pInfo, PDSTRUCT *pPdStruct)
{
    if (pInfo) *pInfo = RIB_HEADER_INFO();
    if (!pInfo || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XRIB> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!guardedThis || !guardedDevice) return false;

    const qint64 nFileSize = getSize();
    if (!guardedThis || !guardedDevice || (nFileSize < RIB_HEADER_SIZE)) {
        return false;
    }

    const qint64 nReadSize = qMin<qint64>(nFileSize, RIB_HEADER_SIZE + RIB_PREFIX_SIZE);
    const QByteArray baHeader = read_array_process(0, nReadSize, pPdStruct);
    if (!guardedThis || !guardedDevice || (baHeader.size() != nReadSize) || !ribStartsWith(baHeader, "RIB\0", 4)) {
        return false;
    }

    const quint32 nUncompressedSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData() + 4));
    const qint64 nPackedSize = nFileSize - RIB_HEADER_SIZE;

    // The codec begins with the packed stream copied into the destination and
    // only grows it backwards.  A packed stream therefore cannot be larger
    // than its declared result.  QByteArray also imposes a signed-int bound;
    // the lower hard cap keeps hostile headers from forcing an excessive
    // allocation before the caller's normal unpack policy is consulted.
    if ((nPackedSize < 0) || (nPackedSize > static_cast<qint64>(nUncompressedSize)) || (nUncompressedSize > RIB_MAX_UNCOMPRESSED_SIZE) ||
        (nPackedSize > (std::numeric_limits<qint32>::max)()) || ((nPackedSize == 0) != (nUncompressedSize == 0))) {
        return false;
    }

    pInfo->nFileSize = nFileSize;
    pInfo->nPackedSize = nPackedSize;
    pInfo->nUncompressedSize = nUncompressedSize;
    pInfo->baPayloadPrefix = baHeader.mid(RIB_HEADER_SIZE);

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XRIB::isValid(PDSTRUCT *pPdStruct)
{
    RIB_HEADER_INFO info;
    return readHeaderInfo(&info, pPdStruct);
}

bool XRIB::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRIB archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XRIB::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XRIB> guardedThis(this);
    if (!isInternalInfoHandled()) {
        RIB_HEADER_INFO info;
        if (!readHeaderInfo(&info, pPdStruct) || !guardedThis) return false;

        if (!XArchive::handleInternalInfo(pPdStruct) || !guardedThis) return false;
        XArchive::INTERNAL_INFO *pBase = static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pBase) return false;
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) = *pBase;

        m_internalInfo.nPackedSize = info.nPackedSize;
        m_internalInfo.nUncompressedSize = info.nUncompressedSize;
    }
    return true;
}

void *XRIB::getInternalInfo(PDSTRUCT *pPdStruct)
{
    return handleInternalInfo(pPdStruct) ? &m_internalInfo : nullptr;
}

void XRIB::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}

XBinary::FT XRIB::getFileType()
{
    return FT_RIB;
}

XBinary::MODE XRIB::getMode()
{
    return MODE_DATA;
}

qint32 XRIB::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XRIB::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRIB::getArch()
{
    return QString();
}

QString XRIB::getFileFormatExt()
{
    return QStringLiteral("rib");
}

QString XRIB::getFileFormatExtsString()
{
    return QStringLiteral("Parsec RIB resource (*.rib *.dat)");
}

QString XRIB::getMIMEString()
{
    return QStringLiteral("application/x-parsec-rib");
}

qint64 XRIB::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    RIB_HEADER_INFO info;
    return readHeaderInfo(&info, pPdStruct) ? info.nFileSize : 0;
}

XBinary::OSNAME XRIB::getOsName()
{
    return OSNAME_MSDOS;
}

QList<QString> XRIB::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'RIB'00");
}

XBinary *XRIB::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRIB(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XRIB::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

QString XRIB::payloadExtension(const QByteArray &baPrefix)
{
    if (baPrefix.startsWith(QByteArrayLiteral("MTCVTS PSM 2.00\0"))) return QStringLiteral("pmm");
    if (ribStartsWith(baPrefix, "SM8\0", 4)) return QStringLiteral("sm8");
    if (ribStartsWith(baPrefix, "PLX\0", 4)) return QStringLiteral("plx");
    if (ribStartsWith(baPrefix, "DTC", 3)) return QStringLiteral("dtc");
    if (ribStartsWith(baPrefix, "DMA", 3)) return QStringLiteral("dma");
    if (ribStartsWith(baPrefix, "MUS", 3)) return QStringLiteral("mus");
    if (ribStartsWith(baPrefix, "SND", 3)) return QStringLiteral("snd");
    return QStringLiteral("dat");
}

bool XRIB::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XRIB> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    RIB_UNPACK_CONTEXT *pOldContext = static_cast<RIB_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis) return false;

    RIB_HEADER_INFO info;
    if (!readHeaderInfo(&info, pPdStruct) || !guardedThis) {
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    OUTPUT_POLICY outputPolicy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &outputPolicy) || !isUnpackOutputSizeAllowed(mapProperties, info.nUncompressedSize) ||
        ((outputPolicy.nMaxMemoryOutputSize >= 0) && (info.nUncompressedSize > outputPolicy.nMaxMemoryOutputSize))) {
        XBinary::setPdStructErrorString(pPdStruct, tr("RIB output exceeds the configured in-memory limit"));
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    RIB_UNPACK_CONTEXT *pContext = new (std::nothrow) RIB_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    pContext->nFileSize = info.nFileSize;
    pContext->nPackedSize = info.nPackedSize;
    pContext->nUncompressedSize = info.nUncompressedSize;
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!guardedThis || !guardedDevice) {
        delete pContext;
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    QString sBaseName = XBinary::getDeviceFileBaseName(guardedDevice.data());
    if (!guardedThis || !guardedDevice) {
        delete pContext;
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    if (sBaseName.isEmpty()) sBaseName = QStringLiteral("data");
    pContext->sFileName = sBaseName + QLatin1Char('.') + payloadExtension(info.baPayloadPrefix);

    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = info.nFileSize;
    pState->mapUnpackProperties = mapProperties;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XRIB::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XRIB> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext) return ARCHIVERECORD();

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }
    RIB_UNPACK_CONTEXT *pContext = static_cast<RIB_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != 1) || (pState->nCurrentIndex != 0) || (pState->nTotalSize != pContext->nFileSize)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = RIB_HEADER_SIZE;
    result.nStreamSize = pContext->nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nPackedSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Parsec RIB"));
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, false);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!XBinary::markArchiveStreamRecord(&result, 0)) return ARCHIVERECORD();
    return result;
}

bool XRIB::decompress(const QByteArray &baPackedData, qint64 nUncompressedSize, QByteArray *pUncompressedData, PDSTRUCT *pPdStruct)
{
    if (pUncompressedData) pUncompressedData->clear();
    const qint64 nPackedSize = baPackedData.size();
    if (!pUncompressedData || (nUncompressedSize < 0) || (nUncompressedSize > RIB_MAX_UNCOMPRESSED_SIZE) || (nUncompressedSize > (std::numeric_limits<qint32>::max)()) ||
        (nPackedSize > nUncompressedSize) || ((nPackedSize == 0) != (nUncompressedSize == 0)) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QByteArray baOutput;
    try {
        baOutput = QByteArray(static_cast<qint32>(nUncompressedSize), '\0');
    } catch (const std::bad_alloc &) {
        return false;
    }
    if (baOutput.size() != nUncompressedSize) return false;
    if (nPackedSize) {
        memcpy(baOutput.data(), baPackedData.constData(), static_cast<size_t>(nPackedSize));
    }

    qint64 nInput = nPackedSize - 1;
    qint64 nOutput = nUncompressedSize - 1;
    quint32 nCancelCounter = 0;

    struct GET_BYTE {
        const QByteArray *pOutputData;
        qint64 nPackedSize;
        qint64 *pInputPosition;
        bool operator()(quint8 *pValue) const
        {
            if (!pValue || (*pInputPosition < 0) || (*pInputPosition >= nPackedSize)) return false;
            *pValue = static_cast<quint8>(pOutputData->at(*pInputPosition));
            --(*pInputPosition);
            return true;
        }
    };
    const GET_BYTE getByte = {&baOutput, nPackedSize, &nInput};

    struct PUT_BYTE {
        QByteArray *pOutputData;
        qint64 nUncompressedSize;
        qint64 *pOutputPosition;
        quint32 *pCancelCounter;
        XBinary::PDSTRUCT *pPdStruct;
        bool operator()(quint8 nValue) const
        {
            if ((*pOutputPosition < 0) || (*pOutputPosition >= nUncompressedSize)) return false;
            if ((((*pCancelCounter)++ & RIB_CANCEL_INTERVAL_MASK) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
            (*pOutputData)[static_cast<qint32>(*pOutputPosition)] = static_cast<char>(nValue);
            --(*pOutputPosition);
            return true;
        }
    };
    const PUT_BYTE putByte = {&baOutput, nUncompressedSize, &nOutput, &nCancelCounter, pPdStruct};

    struct REPEAT_BYTE {
        const PUT_BYTE *pPutByte;
        const qint64 *pOutputPosition;
        bool operator()(quint8 nValue, qint64 nCount) const
        {
            if ((nCount <= 0) || (nCount > (*pOutputPosition + 1))) return false;
            for (qint64 i = 0; i < nCount; ++i) {
                if (!(*pPutByte)(nValue)) return false;
            }
            return true;
        }
    };
    const REPEAT_BYTE repeatByte = {&putByte, &nOutput};

    struct LITERAL {
        const GET_BYTE *pGetByte;
        const PUT_BYTE *pPutByte;
        const qint64 *pInputPosition;
        const qint64 *pOutputPosition;
        bool operator()(qint64 nCount) const
        {
            if ((nCount <= 0) || (nCount > (*pOutputPosition + 1)) || (nCount > (*pInputPosition + 1))) {
                return false;
            }
            for (qint64 i = 0; i < nCount; ++i) {
                quint8 nValue = 0;
                if (!(*pGetByte)(&nValue) || !(*pPutByte)(nValue)) return false;
            }
            return true;
        }
    };
    const LITERAL literal = {&getByte, &putByte, &nInput, &nOutput};

    struct REFERENCE {
        const QByteArray *pOutputData;
        const PUT_BYTE *pPutByte;
        const qint64 *pOutputPosition;
        qint64 nUncompressedSize;
        bool operator()(qint64 nDistance, qint64 nCount) const
        {
            if ((nDistance <= 0) || (nCount <= 0) || (nCount > (*pOutputPosition + 1)) ||
                (nDistance > ((nUncompressedSize - 1) - *pOutputPosition))) {
                return false;
            }
            qint64 nSource = *pOutputPosition + nDistance;
            for (qint64 i = 0; i < nCount; ++i) {
                if ((nSource <= *pOutputPosition) || (nSource < 0) || (nSource >= nUncompressedSize) ||
                    !(*pPutByte)(static_cast<quint8>(pOutputData->at(nSource)))) {
                    return false;
                }
                --nSource;
            }
            return true;
        }
    };
    const REFERENCE reference = {&baOutput, &putByte, &nOutput, nUncompressedSize};

    while (nInput < nOutput) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        quint8 nToken = 0;
        if (!getByte(&nToken)) return false;
        const quint8 nHigh = nToken >> 4;
        const quint8 nLow = nToken & 0x0f;
        bool bDecoded = false;

        if (nHigh == 0) {
            quint8 nValue = 0;
            bDecoded = getByte(&nValue) && repeatByte(nValue, static_cast<qint64>(nLow) + 4);
        } else if (nHigh == 1) {
            quint8 nLength = 0;
            quint8 nValue = 0;
            bDecoded = getByte(&nLength) && getByte(&nValue) && repeatByte(nValue, (static_cast<qint64>(nLow) << 8) + nLength + 20);
        } else if (nHigh == 2) {
            bDecoded = literal(static_cast<qint64>(nLow) + 1);
        } else if (nHigh == 3) {
            quint8 nLength = 0;
            bDecoded = getByte(&nLength) && literal((static_cast<qint64>(nLow) << 8) + nLength + 17);
        } else if (nToken == 0x40) {
            quint8 nHighLength = 0;
            quint8 nLowLength = 0;
            bDecoded = getByte(&nHighLength) && getByte(&nLowLength) && literal(static_cast<qint64>(nLowLength) | (static_cast<qint64>(nHighLength) << 8));
        } else if (nToken == 0x41) {
            quint8 nDistanceHigh = 0;
            quint8 nDistanceLow = 0;
            quint8 nLengthHigh = 0;
            quint8 nLengthLow = 0;
            bDecoded = getByte(&nDistanceHigh) && getByte(&nDistanceLow) && getByte(&nLengthHigh) && getByte(&nLengthLow) &&
                       reference(static_cast<qint64>(nDistanceLow) | (static_cast<qint64>(nDistanceHigh) << 8),
                                 static_cast<qint64>(nLengthLow) | (static_cast<qint64>(nLengthHigh) << 8));
        } else if (nToken == 0x42) {
            quint8 nDistanceHigh = 0;
            quint8 nDistanceLow = 0;
            quint8 nLength = 0;
            bDecoded = getByte(&nDistanceHigh) && getByte(&nDistanceLow) && getByte(&nLength) &&
                       reference(static_cast<qint64>(nDistanceLow) | (static_cast<qint64>(nDistanceHigh) << 8), static_cast<qint64>(nLength) + 17);
        } else if (nHigh == 4) {
            quint8 nDistanceHigh = 0;
            quint8 nDistanceLow = 0;
            bDecoded = getByte(&nDistanceHigh) && getByte(&nDistanceLow) &&
                       reference(static_cast<qint64>(nDistanceLow) | (static_cast<qint64>(nDistanceHigh) << 8), static_cast<qint64>(nLow) + 1);
        } else if (nHigh == 5) {
            quint8 nDistanceLow = 0;
            quint8 nLength = 0;
            bDecoded = getByte(&nDistanceLow) && getByte(&nLength) && reference((static_cast<qint64>(nLow) << 8) + nDistanceLow + 2, static_cast<qint64>(nLength) + 14);
        } else {
            quint8 nDistanceLow = 0;
            bDecoded = getByte(&nDistanceLow) && reference((static_cast<qint64>(nLow) << 8) + nDistanceLow + 2, static_cast<qint64>(nHigh) - 2);
        }

        // An opcode may close the gap exactly, but may never consume or emit
        // past the shared untouched prefix where both cursors converge.
        if (!bDecoded || (nInput > nOutput)) return false;
    }

    if ((nInput != nOutput) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pUncompressedData = baOutput;
    return true;
}

bool XRIB::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XRIB> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedThis || !guardedOutput || !guardedSource ||
        !isUnpackOutputSupported(guardedOutput.data()) || XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    RIB_UNPACK_CONTEXT *pContext = static_cast<RIB_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nCurrentIndex != 0) || (pState->nNumberOfRecords != 1) || (pState->nTotalSize != pContext->nFileSize) ||
        !isUnpackOutputSizeAllowed(pState->mapUnpackProperties, pContext->nUncompressedSize)) {
        return false;
    }

    OUTPUT_POLICY outputPolicy = {};
    if (!resolveUnpackOutputPolicy(pState->mapUnpackProperties, &outputPolicy) ||
        ((outputPolicy.nMaxMemoryOutputSize >= 0) && (pContext->nUncompressedSize > outputPolicy.nMaxMemoryOutputSize))) {
        XBinary::setPdStructErrorString(pPdStruct, tr("RIB output exceeds the configured in-memory limit"));
        return false;
    }

    const QByteArray baPacked = read_array_process(RIB_HEADER_SIZE, pContext->nPackedSize, pPdStruct);
    if (!guardedThis || !guardedOutput || !guardedSource || (baPacked.size() != pContext->nPackedSize) || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis) {
        return false;
    }

    QByteArray baDecoded;
    if (!decompress(baPacked, pContext->nUncompressedSize, &baDecoded, pPdStruct) || !guardedThis || !guardedOutput || !guardedSource ||
        (baDecoded.size() != pContext->nUncompressedSize) || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Invalid RIB compressed stream"));
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(baDecoded.size())) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    QBuffer stage(&baDecoded);
    if (!stage.open(QIODevice::ReadOnly)) return false;
    const bool bResult = guardedThis && guardedOutput && guardedSource && isUnpackSourceCurrent(pState, pPdStruct) && guardedThis &&
                         publishUnpackOutput(&stage, guardedOutput.data(), pState, pPdStruct);
    stage.close();
    if (bResult && guardedThis) {
        pState->nCurrentOffset = pContext->nUncompressedSize;
    }
    return bResult && guardedThis && guardedOutput && guardedSource;
}

bool XRIB::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XRIB> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext) return false;
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nNumberOfRecords != 1) || (pState->nCurrentIndex != 0)) {
        return false;
    }
    pState->nCurrentIndex = 1;
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XRIB::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    RIB_UNPACK_CONTEXT *pContext = static_cast<RIB_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete pContext;
    return true;
}

QList<XBinary::FPART_PROP> XRIB::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ENCRYPTED
                               << FPART_PROP_FILEMODE;
}
