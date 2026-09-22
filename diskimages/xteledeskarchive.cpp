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
#include "xteledeskarchive.h"

#include "Algos/xteledeskdecoder.h"

#include <QFileInfo>

#include <new>

namespace {
const qint64 TELEDESK_HEADER_SIZE = 12;
const qint64 TELEDESK_MAX_INPUT_SIZE = Q_INT64_C(64) * 1024 * 1024;
}  // namespace

XTeleDiskArchive::XTeleDiskArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTeleDiskArchive::~XTeleDiskArchive()
{
}

bool XTeleDiskArchive::parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < TELEDESK_HEADER_SIZE) || (context.nInputSize > TELEDESK_MAX_INPUT_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, TELEDESK_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != TELEDESK_HEADER_SIZE)) return false;
    if (!XTeleDeskDecoder::isValidHeader(baHeader)) return false;

    const quint8 *pHeader = (const quint8 *)baHeader.constData();
    context.nVersion = pHeader[4];
    context.bCompressed = ((pHeader[0] == 't') && (pHeader[1] == 'd'));
    context.bAdvancedCodec = context.bCompressed && (context.nVersion >= 20);
    context.nDataOffset = 0;
    context.nCompressedSize = context.nInputSize;
    context.nUncompressedSize = 0;
    context.bComplete = false;
    context.bSizeKnown = false;

    if (bMeasure) {
        const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
        if (!guardedSource || ((qint64)baFile.size() != context.nInputSize)) return false;
        XTeleDeskDecoder::INFO info = {};
        if (!XTeleDeskDecoder::buildImage(baFile, nullptr, &info, pPdStruct)) return false;
        if (!guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;
        context.nUncompressedSize = info.nImageSize;
        context.bComplete = info.bComplete;
        context.bSizeKnown = true;
    }

    // One member per archive, named after the archive itself.
    QString sName = QFileInfo(getDeviceFileName(guardedSource)).completeBaseName();
    if (!guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("teledisk");
    context.sFileName = sName + QStringLiteral(".ima");

    *pContext = context;

    return true;
}

bool XTeleDiskArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTeleDiskArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTeleDiskArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTeleDiskArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTeleDiskArchive(pDevice);
}

QList<QString> XTeleDiskArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'TD'") << QStringLiteral("'td'");
}

XBinary::FT XTeleDiskArchive::getFileType()
{
    return FT_TELEDISK;
}

XBinary::MODE XTeleDiskArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTeleDiskArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTeleDiskArchive::getArch()
{
    return QString();
}

qint32 XTeleDiskArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTeleDiskArchive::getFileFormatExt()
{
    return QStringLiteral("td0");
}

QString XTeleDiskArchive::getFileFormatExtsString()
{
    return QStringLiteral("TeleDisk (*.td0)");
}

QString XTeleDiskArchive::getMIMEString()
{
    return QStringLiteral("application/x-teledisk");
}

QString XTeleDiskArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();

    return QString("%1.%2").arg(context.nVersion / 10).arg(context.nVersion % 10);
}

qint64 XTeleDiskArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XTeleDiskArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTeleDiskArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QString XTeleDiskArchive::methodToString(const CONTEXT &context)
{
    if (!context.bCompressed) return QStringLiteral("TeleDisk stored");
    if (context.bAdvancedCodec) return QStringLiteral("TeleDisk LZHUF");

    return QStringLiteral("TeleDisk LZW");
}

bool XTeleDiskArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTeleDiskArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    const bool bNeedStream = ((nFileParts & FILEPART_STREAM) != 0);
    if (!parseContext(&context, bNeedStream, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TELEDESK_HEADER_SIZE;
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
        if (context.bSizeKnown) part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, context.bSizeKnown ? HANDLE_METHOD_TELEDISK : HANDLE_METHOD_UNKNOWN);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(context));
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

QMap<XBinary::UNPACK_PROP, QVariant> XTeleDiskArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTeleDiskArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, true, pPdStruct) || !guardedSource) {
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

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XTeleDiskArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pContext->bSizeKnown) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->bSizeKnown ? HANDLE_METHOD_TELEDISK : HANDLE_METHOD_UNKNOWN);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(*pContext));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTeleDiskArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XTeleDiskArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTeleDiskArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
