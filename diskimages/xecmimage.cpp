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
#include "xecmimage.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <memory>
#include <new>

// Format knowledge only (the public ECM stream description); EDC and P/Q
// parity are implemented from ECMA-130 Annex A: GF(2^8) with generator
// x^8+x^4+x^3+x^2+1, parity (p0, p1) appended so that sum(v_i) = 0 and
// sum(alpha^(n-1-i) * v_i) = 0 over the codeword.
namespace {
const qint64 ECM_MAGIC_SIZE = 4;
const qint64 ECM_CHECK_SIZE = 4;
const qint64 ECM_WINDOW_SIZE = 1 << 20;
const qint64 ECM_MAX_OUTPUT = Q_INT64_C(0x200000000);  // 8 GiB, larger than any CD/DVD image
const qint32 ECM_SECTOR = 2352;
const qint32 ECM_MODE2_BODY = 2336;
const quint32 ECM_END_MARKER = 0xffffffffU;
const quint32 EDC_POLY = 0xd8018001U;
const quint32 GF_POLY = 0x11dU;

struct ECC_TABLES {
    quint32 edc[256];
    quint8 gfExp[512];
    quint8 gfLog[256];
    quint8 weightP[24];  // alpha^(25 - i) for the RS(26,24) columns
    quint8 weightQ[43];  // alpha^(44 - i) for the RS(45,43) diagonals
    quint8 inv3;

    ECC_TABLES()
    {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 nValue = i;
            for (qint32 j = 0; j < 8; ++j) {
                nValue = (nValue >> 1) ^ ((nValue & 1U) ? EDC_POLY : 0U);
            }
            edc[i] = nValue;
        }
        quint32 nPower = 1;
        memset(gfLog, 0, sizeof(gfLog));
        for (qint32 i = 0; i < 255; ++i) {
            gfExp[i] = static_cast<quint8>(nPower);
            gfLog[nPower] = static_cast<quint8>(i);
            nPower <<= 1;
            if (nPower & 0x100U) nPower ^= GF_POLY;
        }
        for (qint32 i = 255; i < 512; ++i) {
            gfExp[i] = gfExp[i - 255];
        }
        for (qint32 i = 0; i < 24; ++i) weightP[i] = gfExp[(25 - i) % 255];
        for (qint32 i = 0; i < 43; ++i) weightQ[i] = gfExp[(44 - i) % 255];
        inv3 = gfExp[255 - gfLog[3]];
    }

    quint8 mul(quint8 a, quint8 b) const
    {
        if ((a == 0) || (b == 0)) return 0;
        return gfExp[gfLog[a] + gfLog[b]];
    }

    quint32 edcUpdate(quint32 nEdc, const quint8 *pData, qint64 nSize) const
    {
        for (qint64 i = 0; i < nSize; ++i) {
            nEdc = (nEdc >> 8) ^ edc[(nEdc ^ pData[i]) & 0xffU];
        }
        return nEdc;
    }

    // Parity of one codeword whose data symbols are read from pSector at
    // pPositions[0..nCount-1] with the given alpha weights.
    void parity(const quint8 *pSector, const qint32 *pPositions, const quint8 *pWeights, qint32 nCount, quint8 *pP0, quint8 *pP1) const
    {
        quint8 nS0 = 0;
        quint8 nS1 = 0;
        for (qint32 i = 0; i < nCount; ++i) {
            const quint8 nSymbol = pSector[pPositions[i]];
            nS0 ^= nSymbol;
            nS1 ^= mul(nSymbol, pWeights[i]);
        }
        const quint8 nP0 = mul(static_cast<quint8>(nS0 ^ nS1), inv3);
        *pP0 = nP0;
        *pP1 = static_cast<quint8>(nS0 ^ nP0);
    }

    // Fill bytes 2076..2351 (P then Q) of a 2352-byte sector whose bytes
    // 12..2075 are final.
    void fillEcc(quint8 *pSector) const
    {
        qint32 nPositions[43];
        for (qint32 m = 0; m < 86; ++m) {
            for (qint32 r = 0; r < 24; ++r) nPositions[r] = 12 + m + 86 * r;
            parity(pSector, nPositions, weightP, 24, pSector + 2076 + m, pSector + 2076 + 86 + m);
        }
        for (qint32 m = 0; m < 52; ++m) {
            for (qint32 i = 0; i < 43; ++i) nPositions[i] = 12 + (((m >> 1) * 86 + (m & 1) + 88 * i) % 2236);
            parity(pSector, nPositions, weightQ, 43, pSector + 2248 + m, pSector + 2248 + 52 + m);
        }
    }
};

const ECC_TABLES &tables()
{
    static const ECC_TABLES s_tables;
    return s_tables;
}

qint64 storedItemSize(qint32 nType)
{
    switch (nType) {
        case 0: return 1;
        case 1: return 3 + 2048;
        case 2: return 4 + 2048;
        case 3: return 4 + 2324;
        default: return 0;
    }
}

qint64 outputItemSize(qint32 nType)
{
    switch (nType) {
        case 0: return 1;
        case 1: return ECM_SECTOR;
        case 2: return ECM_MODE2_BODY;
        case 3: return ECM_MODE2_BODY;
        default: return 0;
    }
}
}  // namespace

// Buffered forward reader over the archive device.
struct XEcmImage::STREAM {
    QPointer<XEcmImage> guardedImage;
    PDSTRUCT *pPdStruct;
    qint64 nFileSize;
    qint64 nPosition;
    qint64 nWindowOffset;
    QByteArray baWindow;

    STREAM(XEcmImage *pImage, qint64 nSize, PDSTRUCT *pProgress) : guardedImage(pImage), pPdStruct(pProgress), nFileSize(nSize), nPosition(0), nWindowOffset(0)
    {
    }

    bool ensure(qint64 nCount)
    {
        if (!guardedImage || (nCount <= 0) || (nPosition < 0) || (nPosition + nCount > nFileSize)) return false;
        if ((nPosition >= nWindowOffset) && (nPosition + nCount <= nWindowOffset + baWindow.size())) return true;
        const qint64 nRead = qMin<qint64>(qMax<qint64>(ECM_WINDOW_SIZE, nCount), nFileSize - nPosition);
        const QByteArray baRead = guardedImage->read_array_process(nPosition, nRead, pPdStruct);
        if (!guardedImage || (baRead.size() != nRead) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        baWindow = baRead;
        nWindowOffset = nPosition;
        return true;
    }

    const quint8 *view(qint64 nCount)
    {
        if (!ensure(nCount)) return nullptr;
        return reinterpret_cast<const quint8 *>(baWindow.constData()) + (nPosition - nWindowOffset);
    }

    bool readByte(quint8 *pValue)
    {
        const quint8 *pData = view(1);
        if (!pData || !pValue) return false;
        *pValue = pData[0];
        ++nPosition;
        return true;
    }

    // Type-and-count varint: 2 type bits, 5 count bits, then 7-bit groups.
    bool readRecordHeader(qint32 *pType, quint32 *pStored)
    {
        quint8 nByte = 0;
        if (!readByte(&nByte) || !pType || !pStored) return false;
        *pType = nByte & 3;
        quint64 nValue = (nByte & 0x7cU) >> 2;
        qint32 nBits = 5;
        while (nByte & 0x80U) {
            if (!readByte(&nByte) || (nBits > 33)) return false;
            nValue |= static_cast<quint64>(nByte & 0x7fU) << nBits;
            nBits += 7;
        }
        if (nValue > ECM_END_MARKER) return false;
        *pStored = static_cast<quint32>(nValue);
        return true;
    }
};

XEcmImage::XEcmImage(QIODevice *pDevice) : XArchive(pDevice)
{
}

XEcmImage::~XEcmImage()
{
}

bool XEcmImage::parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XEcmImage> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pContext || !guardedThis || !guardedSource || !guardedSource->isOpen() || !guardedSource->isReadable() ||
        guardedSource->isSequential() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CONTEXT context = {};
    context.nFileSize = getSize();
    if (!guardedThis || !guardedSource || (context.nFileSize < ECM_MAGIC_SIZE + 5 + ECM_CHECK_SIZE)) return false;

    STREAM stream(this, context.nFileSize, pPdStruct);
    const quint8 *pMagic = stream.view(ECM_MAGIC_SIZE);
    if (!pMagic || !guardedThis || (memcmp(pMagic, "ECM\0", 4) != 0)) return false;
    stream.nPosition = ECM_MAGIC_SIZE;

    bool bEnd = false;
    while (!bEnd) {
        if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        qint32 nType = 0;
        quint32 nStored = 0;
        if (!stream.readRecordHeader(&nType, &nStored)) return false;
        if (nStored == ECM_END_MARKER) {
            bEnd = true;
            break;
        }
        const qint64 nCount = static_cast<qint64>(nStored) + 1;
        const qint64 nInput = nCount * storedItemSize(nType);
        if ((stream.nPosition + nInput + ECM_CHECK_SIZE > context.nFileSize) || (stream.nPosition + nInput < stream.nPosition)) return false;
        context.nOutputSize += nCount * outputItemSize(nType);
        if (context.nOutputSize > ECM_MAX_OUTPUT) return false;
        if (nType == 0) {
            context.nLiteralBytes += nCount;
        } else if (nType == 1) {
            context.nMode1Sectors += nCount;
        } else if (nType == 2) {
            context.nMode2Form1Sectors += nCount;
        } else {
            context.nMode2Form2Sectors += nCount;
        }
        ++context.nRecordCount;
        stream.nPosition += nInput;
    }
    if (!bEnd || (context.nRecordCount == 0) || (context.nOutputSize == 0)) return false;
    // The check value must be the last four bytes of the file, exactly.
    if (stream.nPosition + ECM_CHECK_SIZE != context.nFileSize) return false;
    const quint8 *pCheck = stream.view(ECM_CHECK_SIZE);
    if (!pCheck || !guardedThis) return false;
    context.nStoredCheck = qFromLittleEndian<quint32>(pCheck);

    QString sName = QFileInfo(getDeviceFileName(guardedSource.data())).fileName();
    if (!guardedThis || !guardedSource) return false;
    if (sName.endsWith(QLatin1String(".ecm"), Qt::CaseInsensitive) && (sName.size() > 4)) {
        sName.chop(4);
    } else if (sName.isEmpty()) {
        sName = QStringLiteral("image.bin");
    } else {
        sName += QStringLiteral(".bin");
    }
    context.sFileName = sName;

    *pContext = context;
    return true;
}

bool XEcmImage::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseImage(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);
    return bResult;
}

bool XEcmImage::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEcmImage image(pDevice);
    return image.isValid(pPdStruct);
}

XBinary *XEcmImage::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XEcmImage(pDevice);
}

QList<QString> XEcmImage::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ECM'00");
}

XBinary::FT XEcmImage::getFileType()
{
    return FT_ECM;
}

XBinary::MODE XEcmImage::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEcmImage::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XEcmImage::getArch()
{
    return QString();
}

qint32 XEcmImage::getType()
{
    return TYPE_ARCHIVE;
}

QString XEcmImage::getFileFormatExt()
{
    return QStringLiteral("ecm");
}

QString XEcmImage::getFileFormatExtsString()
{
    return QStringLiteral("Error Code Modeler image (*.ecm)");
}

QString XEcmImage::getMIMEString()
{
    return QStringLiteral("application/x-ecm");
}

QString XEcmImage::getVersion()
{
    return QString();
}

QMap<XBinary::UNPACK_PROP, QVariant> XEcmImage::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEcmImage::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XEcmImage> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedThis || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) goto failed;
    if (!parseImage(pContext, pPdStruct) || !guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) goto failed;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->nCurrentOffset = ECM_MAGIC_SIZE;
    pState->nTotalSize = pContext->nFileSize;
    pState->pContext = pContext;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) goto failed;
    return true;

failed:
    if (guardedThis) releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return false;
}

XBinary::ARCHIVERECORD XEcmImage::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XEcmImage> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nNumberOfRecords != 1) || (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = ECM_MAGIC_SIZE;
    result.nStreamSize = pContext->nFileSize - ECM_MAGIC_SIZE - ECM_CHECK_SIZE;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nOutputSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("ECM (Mode 1: %1, Mode 2/1: %2, Mode 2/2: %3, literal: %4 bytes)")
                                                               .arg(pContext->nMode1Sectors)
                                                               .arg(pContext->nMode2Form1Sectors)
                                                               .arg(pContext->nMode2Form2Sectors)
                                                               .arg(pContext->nLiteralBytes));
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, pContext->nStoredCheck);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XEcmImage::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XEcmImage> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedThis || !guardedOutput || !guardedSource ||
        !isUnpackOutputSupported(guardedOutput.data()) || devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nFileSize) || (pContext->nOutputSize <= 0)) return false;
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, pContext->nOutputSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
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
        if (!pState->spOutputBudget->debit(pContext->nOutputSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(pContext->nOutputSize, pPdStruct));
    if (!pStage || !pStage->seek(0) || !guardedThis || !guardedOutput || !guardedSource) return false;

    const ECC_TABLES &ecc = tables();
    STREAM stream(this, pContext->nFileSize, pPdStruct);
    stream.nPosition = ECM_MAGIC_SIZE;
    quint32 nRunningCheck = 0;
    qint64 nWritten = 0;
    quint8 nSector[ECM_SECTOR];
    bool bEnd = false;

    while (!bEnd) {
        if (!guardedThis || !guardedOutput || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || !isUnpackSourceCurrent(pState, pPdStruct)) {
            return false;
        }
        qint32 nType = 0;
        quint32 nStored = 0;
        if (!stream.readRecordHeader(&nType, &nStored)) return false;
        if (nStored == ECM_END_MARKER) {
            bEnd = true;
            break;
        }
        qint64 nCount = static_cast<qint64>(nStored) + 1;
        if (nType == 0) {
            while (nCount > 0) {
                const qint64 nChunk = qMin<qint64>(nCount, ECM_WINDOW_SIZE);
                const quint8 *pData = stream.view(nChunk);
                if (!pData || !guardedThis) return false;
                if (pStage->write(reinterpret_cast<const char *>(pData), nChunk) != nChunk) return false;
                nRunningCheck = ecc.edcUpdate(nRunningCheck, pData, nChunk);
                nWritten += nChunk;
                stream.nPosition += nChunk;
                nCount -= nChunk;
            }
            continue;
        }
        const qint64 nItemInput = storedItemSize(nType);
        const qint64 nItemOutput = outputItemSize(nType);
        for (qint64 i = 0; i < nCount; ++i) {
            if (((i & 0xff) == 0) && (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct))) return false;
            const quint8 *pIn = stream.view(nItemInput);
            if (!pIn || !guardedThis) return false;
            memset(nSector, 0, sizeof(nSector));
            const quint8 *pOut = nSector;
            if (nType == 1) {
                nSector[0] = 0x00;
                memset(nSector + 1, 0xff, 10);
                nSector[11] = 0x00;
                memcpy(nSector + 12, pIn, 3);
                nSector[15] = 0x01;
                memcpy(nSector + 16, pIn + 3, 2048);
                qToLittleEndian<quint32>(ecc.edcUpdate(0, nSector, 2064), nSector + 2064);
                ecc.fillEcc(nSector);
            } else if (nType == 2) {
                memcpy(nSector + 16, pIn, 4);
                memcpy(nSector + 20, pIn, 4);
                memcpy(nSector + 24, pIn + 4, 2048);
                qToLittleEndian<quint32>(ecc.edcUpdate(0, nSector + 16, 2056), nSector + 2072);
                ecc.fillEcc(nSector);
                pOut = nSector + 16;
            } else {
                memcpy(nSector + 16, pIn, 4);
                memcpy(nSector + 20, pIn, 4);
                memcpy(nSector + 24, pIn + 4, 2324);
                qToLittleEndian<quint32>(ecc.edcUpdate(0, nSector + 16, 2332), nSector + 2348);
                pOut = nSector + 16;
            }
            if (pStage->write(reinterpret_cast<const char *>(pOut), nItemOutput) != nItemOutput) return false;
            nRunningCheck = ecc.edcUpdate(nRunningCheck, pOut, nItemOutput);
            nWritten += nItemOutput;
            stream.nPosition += nItemInput;
        }
    }

    if (!bEnd || (nWritten != pContext->nOutputSize) || (pStage->size() != pContext->nOutputSize) || (stream.nPosition + ECM_CHECK_SIZE != pContext->nFileSize)) {
        return false;
    }
    if (nRunningCheck != pContext->nStoredCheck) {
        XBinary::setPdStructErrorString(pPdStruct, tr("ECM check value mismatch"));
        return false;
    }
    if (!pStage->seek(0) || !guardedThis || !guardedOutput || !guardedSource || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (bResult && guardedThis) pState->nCurrentOffset = pContext->nFileSize;
    return bResult && guardedThis;
}

bool XEcmImage::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    ++pState->nCurrentIndex;
    if (pContext) pState->nCurrentOffset = pContext->nFileSize;
    return false;
}

bool XEcmImage::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XEcmImage::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_REPORTEDMETHOD
                               << FPART_PROP_UNCOMPRESSEDCRC << FPART_PROP_ISFOLDER;
}
