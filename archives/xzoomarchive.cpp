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
#include "xzoomarchive.h"

#include "Algos/xzoomdecoder.h"

#include <QFileInfo>
#include <QtEndian>

#include <new>

namespace {
const qint64 ZOOM_HEADER_SIZE = XZoomDecoder::HEADER_SIZE;
const qint64 ZOOM_CHUNK_HEADER_SIZE = XZoomDecoder::CHUNK_HEADER_SIZE;
const qint64 ZOOM_CYLINDER_SIZE = XZoomDecoder::CYLINDER_SIZE;
const qint64 ZOOM_MAX_CYLINDERS = 256;

}  // namespace

XZoomArchive::XZoomArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZoomArchive::~XZoomArchive()
{
}

bool XZoomArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (ZOOM_HEADER_SIZE + ZOOM_CHUNK_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, ZOOM_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != ZOOM_HEADER_SIZE)) return false;

    // The note length has to be bounded against the real file, which the
    // decoder's own header walk cannot do from a 0x4C byte slice - so the
    // container repeats the parse here against the device size instead of
    // handing that slice to XZoomDecoder.
    if (baHeader.left(4) != QByteArray("ZOM5", 4)) return false;
    if ((quint8)baHeader.at(6) != 5) return false;

    const quint8 nFirst = (quint8)baHeader.at(4);
    const quint8 nLast = (quint8)baHeader.at(5);
    if (nLast < nFirst) return false;

    const qint64 nNoteSize = (qint64)qFromBigEndian<quint32>((const uchar *)baHeader.constData() + 0x1c);
    if (nNoteSize < 0) return false;
    qint64 nChunksOffset = ZOOM_HEADER_SIZE;
    if (nNoteSize != 0) {
        if (nNoteSize > (context.nInputSize - ZOOM_HEADER_SIZE - 4)) return false;
        nChunksOffset += nNoteSize + 4;
    }

    const qint64 nCylinders = (qint64)nLast - (qint64)nFirst + 1;
    if ((nCylinders <= 0) || (nCylinders > ZOOM_MAX_CYLINDERS)) return false;

    context.nChunksOffset = nChunksOffset;
    context.nFirstCylinder = nFirst;
    context.nLastCylinder = nLast;
    context.bProtected = ((quint8)baHeader.at(0x24) != 0);
    context.nUncompressedSize = nCylinders * ZOOM_CYLINDER_SIZE;

    QString sName = QFileInfo(getDeviceFileName(guardedSource)).fileName();
    if (!guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("zoom");
    context.sFileName = sName;

    *pContext = context;

    return true;
}

bool XZoomArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZoomArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZoomArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZoomArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZoomArchive(pDevice);
}

QList<QString> XZoomArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ZOM5'");
}

XBinary::FT XZoomArchive::getFileType()
{
    return FT_ZOOM;
}

XBinary::MODE XZoomArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZoomArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XZoomArchive::getArch()
{
    return QString();
}

qint32 XZoomArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZoomArchive::getFileFormatExt()
{
    return QStringLiteral("zom");
}

QString XZoomArchive::getFileFormatExtsString()
{
    return QStringLiteral("Zoom disk image (*.zom)");
}

QString XZoomArchive::getMIMEString()
{
    return QStringLiteral("application/x-zoom-adf");
}

QString XZoomArchive::getVersion()
{
    return QStringLiteral("5");
}

qint64 XZoomArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XZoomArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZoomArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XZoomArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZoomArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nChunksOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        // The chunk index is spread across the records themselves, so the
        // decoder is handed the whole container.
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nInputSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZOOM);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Zoom LZHUF+RLE"));
        if (context.bProtected) part.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
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

QMap<XBinary::UNPACK_PROP, QVariant> XZoomArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZoomArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XZoomArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != 0) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nInputSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nInputSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZOOM);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Zoom LZHUF+RLE"));
    if (pContext->bProtected) result.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZoomArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XZoomArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZoomArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ENCRYPTED << FPART_PROP_ISFOLDER;
}
