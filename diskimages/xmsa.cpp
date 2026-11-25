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
#include "xmsa.h"

#include <QtEndian>

#include <cstring>
#include <memory>
#include <new>

// The rules are stated in xmsa.h and are shared, step for step, with xxfclib
// (src/scan/xx_scan_floppy.c): change both sides together.

namespace {
const qint64 MSA_HEADER_SIZE = 10;
const quint32 MSA_SECTOR_SIZE = 512;
const quint32 MSA_MAX_SECTORS = 36;
const quint32 MSA_MAX_TRACK = 85;

quint32 msaU16(const uchar *pData)
{
    return qFromBigEndian<quint16>(pData);
}
}  // namespace

XMSA::XMSA(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMSA::~XMSA()
{
}

// The header and the chain of track length words.
bool XMSA::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext || !guardedSource || !guardedSource->isOpen() || !guardedSource->isReadable() || guardedSource->isSequential() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    CONTEXT context = {};
    context.nInputSize = getSize();
    if (context.nInputSize < MSA_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, MSA_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != MSA_HEADER_SIZE) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if ((pHeader[0] != 0x0E) || (pHeader[1] != 0x0F)) return false;

    const quint32 nSectors = msaU16(pHeader + 2);
    const quint32 nSides = msaU16(pHeader + 4);
    const quint32 nFirstTrack = msaU16(pHeader + 6);
    const quint32 nLastTrack = msaU16(pHeader + 8);
    if (!nSectors || (nSectors > MSA_MAX_SECTORS) || (nSides > 1) || (nFirstTrack > nLastTrack) || (nLastTrack > MSA_MAX_TRACK)) return false;

    context.nTrackSize = nSectors * MSA_SECTOR_SIZE;
    context.nTracks = (nLastTrack - nFirstTrack + 1) * (nSides + 1);

    qint64 nPosition = MSA_HEADER_SIZE;
    for (quint32 i = 0; i < context.nTracks; i++) {
        if (nPosition + 2 > context.nInputSize) return false;
        const QByteArray baLength = read_array_process(nPosition, 2, pPdStruct);
        if (baLength.size() != 2) return false;
        const quint32 nLength = msaU16((const uchar *)baLength.constData());
        nPosition += 2;
        if (!nLength || (nLength > context.nTrackSize) || (nPosition + (qint64)nLength > context.nInputSize)) return false;
        nPosition += nLength;
    }

    context.nPackedSize = nPosition - MSA_HEADER_SIZE;
    context.nImageSize = (qint64)context.nTracks * context.nTrackSize;
    *pContext = context;

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XMSA::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XMSA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMSA archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XMSA::getFileType()
{
    return FT_MSA;
}

XBinary::MODE XMSA::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMSA::getEndian()
{
    return ENDIAN_BIG;
}

QString XMSA::getMIMEString()
{
    return QStringLiteral("application/x-msa-disk-image");
}

QString XMSA::getFileFormatExt()
{
    return QStringLiteral("msa");
}

QString XMSA::getFileFormatExtsString()
{
    return QStringLiteral("Atari MSA disk image (*.msa)");
}

QList<QString> XMSA::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("0E0F");
}

XBinary *XMSA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMSA(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XMSA::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMSA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XMSA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nInputSize)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = MSA_HEADER_SIZE;
    result.nStreamSize = pContext->nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, QStringLiteral("msa-content"));
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nPackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nImageSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("RLE"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The decoded image has no extent in the file; only unpackCurrent() yields it.
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();

    return result;
}

bool XMSA::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedOutput || !guardedSource || !isUnpackOutputSupported(guardedOutput) ||
        devicesAlias(guardedSource, guardedOutput) || !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nInputSize) || (pState->nNumberOfRecords != 1) || (pState->nCurrentIndex != 0)) return false;

    const CONTEXT context = *pContext;

    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, context.nImageSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, QStringLiteral("msa-content"))) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(context.nImageSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(context.nImageSize, pPdStruct));
    if (!pStage || !pStage->seek(0) || !guardedOutput || !guardedSource) return false;

    QByteArray baTrack((qint32)context.nTrackSize, '\0');
    qint64 nPosition = MSA_HEADER_SIZE;

    for (quint32 i = 0; i < context.nTracks; i++) {
        if (!guardedOutput || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const QByteArray baLength = read_array_process(nPosition, 2, pPdStruct);
        if (baLength.size() != 2) return false;
        const quint32 nLength = msaU16((const uchar *)baLength.constData());
        nPosition += 2;
        if (!nLength || (nLength > context.nTrackSize)) return false;

        const QByteArray baPacked = read_array_process(nPosition, nLength, pPdStruct);
        if (!guardedOutput || !guardedSource || (baPacked.size() != (qint32)nLength) || !isUnpackSourceCurrent(pState, pPdStruct)) return false;

        const char *pOutput = baPacked.constData();

        if (nLength != context.nTrackSize) {
            const uchar *pPacked = (const uchar *)baPacked.constData();
            char *pTrack = baTrack.data();
            quint32 nRead = 0;
            quint32 nWritten = 0;

            while (nRead < nLength) {
                uchar nValue = pPacked[nRead++];
                quint32 nCount = 1;
                if (nValue == 0xE5) {
                    if (nLength - nRead < 3) return false;
                    nValue = pPacked[nRead];
                    nCount = msaU16(pPacked + nRead + 1);
                    nRead += 3;
                }
                if (!nCount || (nCount > context.nTrackSize - nWritten)) return false;
                memset(pTrack + nWritten, nValue, nCount);
                nWritten += nCount;
            }
            if (nWritten != context.nTrackSize) return false;
            pOutput = baTrack.constData();
        }

        qint64 nStaged = 0;
        while (nStaged < (qint64)context.nTrackSize) {
            const qint64 nWrite = pStage->write(pOutput + nStaged, (qint64)context.nTrackSize - nStaged);
            if ((nWrite <= 0) || (nWrite > (qint64)context.nTrackSize - nStaged)) return false;
            nStaged += nWrite;
        }

        nPosition += nLength;
    }

    if ((pStage->size() != context.nImageSize) || !pStage->seek(0) || !guardedOutput || !guardedSource || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
    if (bResult) pState->nCurrentOffset = context.nImageSize;

    return bResult;
}

bool XMSA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) || (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XMSA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XMSA::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
