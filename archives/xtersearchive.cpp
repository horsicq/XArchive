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
#include "xtersearchive.h"

#include "Algos/xtersedecoder.h"

#include <QFileInfo>
#include <QPointer>

#include <new>

namespace {
// The detector needs 12 header bytes plus the eight the code probe reads.
const qint64 TERSE_PROBE_SIZE = 20;
const qint64 TERSE_MIN_SIZE = 5;
const qint64 TERSE_MAX_INPUT_SIZE = 0x20000000;
}  // namespace

XTERSEArchive::XTERSEArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTERSEArchive::~XTERSEArchive()
{
}

bool XTERSEArchive::parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XTERSEArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < TERSE_MIN_SIZE) || (context.nInputSize > TERSE_MAX_INPUT_SIZE)) return false;

    const qint64 nProbeSize = qMin<qint64>(context.nInputSize, TERSE_PROBE_SIZE);
    const QByteArray baHeader = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != nProbeSize)) return false;

    const qint32 nHeaderSize = XTERSEDecoder::detect(baHeader);
    if (nHeaderSize < 0) return false;
    if (context.nInputSize <= nHeaderSize) return false;

    context.nHeaderSize = nHeaderSize;
    // The whole file goes to the decoder: the header size is a detection
    // result, not a fixed skip, so the record cannot start past it.
    context.nDataOffset = 0;
    context.nCompressedSize = context.nInputSize;
    context.nUncompressedSize = 0;
    context.bUncompressedSizeKnown = false;

    if (bMeasure) {
        const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baFile.size() != context.nInputSize)) return false;
        qint64 nUncompressedSize = 0;
        if (XTERSEDecoder::measure(baFile, &nUncompressedSize, pPdStruct)) {
            context.nUncompressedSize = nUncompressedSize;
            context.bUncompressedSizeKnown = true;
        }
    }

    QString sName = QFileInfo(getDeviceFileName(guardedSource.data())).fileName();
    if (!guardedThis || !guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("terse");
    context.sFileName = sName;

    *pContext = context;

    return true;
}

bool XTERSEArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTERSEArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTERSEArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTERSEArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTERSEArchive(pDevice);
}

QList<QString> XTERSEArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("018969a5");
}

XBinary::FT XTERSEArchive::getFileType()
{
    return FT_TERSE;
}

XBinary::MODE XTERSEArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTERSEArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XTERSEArchive::getArch()
{
    return QString();
}

qint32 XTERSEArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTERSEArchive::getFileFormatExt()
{
    return QStringLiteral("trs");
}

QString XTERSEArchive::getFileFormatExtsString()
{
    return QStringLiteral("IBM TERSE (*.trs *.ter)");
}

QString XTERSEArchive::getMIMEString()
{
    return QStringLiteral("application/x-terse");
}

QString XTERSEArchive::getVersion()
{
    return QString();
}

qint64 XTERSEArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XTERSEArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTERSEArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XTERSEArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTERSEArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nCompressedSize);
        if (context.bUncompressedSizeKnown) part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TERSE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("TERSE"));
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XTERSEArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTERSEArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTERSEArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
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

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XTERSEArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    if (pContext->bUncompressedSizeKnown) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TERSE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("TERSE"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTERSEArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XTERSEArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XTERSEArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
