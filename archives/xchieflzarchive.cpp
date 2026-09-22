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
#include "xchieflzarchive.h"

#include <QFileInfo>
#include <QtEndian>

#include <new>

namespace {
const qint64 CHIEFLZ_HEADER_SIZE = 0x101;
const qint32 CHIEFLZ_NAME_OFFSET = 40;
const qint32 CHIEFLZ_METHOD_OFFSET = 173;
const quint8 CHIEFLZ_METHOD = 4;
}  // namespace

XChiefLZArchive::XChiefLZArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XChiefLZArchive::~XChiefLZArchive()
{
}

bool XChiefLZArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <= CHIEFLZ_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, CHIEFLZ_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != CHIEFLZ_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (pHeader[0] != 8) return false;
    if (baHeader.mid(1, 8) != QByteArray("aChiefM#")) return false;
    if (pHeader[CHIEFLZ_METHOD_OFFSET] != CHIEFLZ_METHOD) return false;

    const qint32 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pHeader + 11);
    if (nUncompressedSize < 0) return false;

    context.nDate = qFromLittleEndian<quint16>(pHeader + 19);
    context.nTime = qFromLittleEndian<quint16>(pHeader + 21);
    context.nDataOffset = CHIEFLZ_HEADER_SIZE;
    context.nCompressedSize = context.nInputSize - CHIEFLZ_HEADER_SIZE;
    context.nUncompressedSize = nUncompressedSize;

    // the stored name is length-prefixed, not NUL terminated
    const qint32 nNameLength = pHeader[CHIEFLZ_NAME_OFFSET];
    QByteArray baName;
    if ((nNameLength > 0) && ((CHIEFLZ_NAME_OFFSET + 1 + nNameLength) <= CHIEFLZ_HEADER_SIZE)) {
        baName = baHeader.mid(CHIEFLZ_NAME_OFFSET + 1, nNameLength);
    }
    for (qint32 i = 0; i < baName.size(); ++i) {
        if ((quint8)baName.at(i) < 0x20) return false;
    }
    if (baName.isEmpty()) {
        QString sFallback = QFileInfo(getDeviceFileName(guardedSource)).fileName();
        if (!guardedSource) return false;
        context.sFileName = sFallback.isEmpty() ? QStringLiteral("data") : sFallback;
    } else {
        context.sFileName = QString::fromLatin1(baName);
    }

    *pContext = context;

    return true;
}

bool XChiefLZArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XChiefLZArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XChiefLZArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XChiefLZArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XChiefLZArchive(pDevice);
}

QList<QString> XChiefLZArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("08'aChiefM#'");
}

XBinary::FT XChiefLZArchive::getFileType()
{
    return FT_CHIEFLZ;
}

XBinary::MODE XChiefLZArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XChiefLZArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XChiefLZArchive::getArch()
{
    return QString();
}

qint32 XChiefLZArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XChiefLZArchive::getFileFormatExt()
{
    return QStringLiteral("pk_");
}

QString XChiefLZArchive::getFileFormatExtsString()
{
    return QStringLiteral("ChiefLZ (*)");
}

QString XChiefLZArchive::getMIMEString()
{
    return QStringLiteral("application/x-chieflz");
}

QString XChiefLZArchive::getVersion()
{
    return QStringLiteral("2");
}

qint64 XChiefLZArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XChiefLZArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XChiefLZArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XChiefLZArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XChiefLZArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = CHIEFLZ_HEADER_SIZE;
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
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_CHIEFLZ);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("ChiefLZ"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XChiefLZArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XChiefLZArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct)) {
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
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XChiefLZArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_CHIEFLZ);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("ChiefLZ"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (isValidDosDateTime(pContext->nDate, pContext->nTime)) {
        const QDateTime dtModified = dosDateTimeToQDateTime(pContext->nDate, pContext->nTime);
        if (dtModified.isValid()) result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }

    return result;
}

bool XChiefLZArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XChiefLZArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XChiefLZArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_MTIME;
}
