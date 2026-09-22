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
#include "xxeditpackarchive.h"

#include "Algos/xxeditpackdecoder.h"

#include <QFileInfo>

#include <new>

namespace {
const qint64 XEP_HEADER_SIZE = 8;
const qint64 XEP_MIN_SIZE = 9;  // the header plus at least one opcode
const quint8 XEP_BLANK = 0x40;
const quint8 XEP_FORMAT_FIXED = 0xc6;     // EBCDIC 'F'
const quint8 XEP_FORMAT_VARIABLE = 0xe5;  // EBCDIC 'V'
}  // namespace

XXEditPackArchive::XXEditPackArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XXEditPackArchive::~XXEditPackArchive()
{
}

bool XXEditPackArchive::parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < XEP_MIN_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, XEP_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != XEP_HEADER_SIZE)) return false;
    const quint8 *pHeader = (const quint8 *)baHeader.constData();

    if ((pHeader[0] != 0x00) || (pHeader[1] != 0x01) || (pHeader[2] != XEP_BLANK)) return false;
    if ((pHeader[3] != XEP_FORMAT_FIXED) && (pHeader[3] != XEP_FORMAT_VARIABLE)) return false;

    context.nRecordFormat = pHeader[3];
    context.nDataOffset = XEP_HEADER_SIZE;
    context.nCompressedSize = context.nInputSize - XEP_HEADER_SIZE;
    context.nUncompressedSize = 0;
    context.bUncompressedSizeKnown = false;

    if (bMeasure) {
        if (context.nCompressedSize > XXEditPackDecoder::MAX_UNCOMPRESSED_SIZE) return false;
        const QByteArray baPacked = read_array_process(context.nDataOffset, context.nCompressedSize, pPdStruct);
        if (!guardedSource || (baPacked.size() != context.nCompressedSize)) return false;
        qint64 nUncompressedSize = 0;
        if (XXEditPackDecoder::measure(baPacked, &nUncompressedSize, pPdStruct)) {
            context.nUncompressedSize = nUncompressedSize;
            context.bUncompressedSizeKnown = true;
        }
    }

    QString sName = QFileInfo(getDeviceFileName(guardedSource)).fileName();
    if (!guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("xeditpack");
    context.sFileName = sName;

    *pContext = context;

    return true;
}

bool XXEditPackArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XXEditPackArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XXEditPackArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XXEditPackArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XXEditPackArchive(pDevice);
}

QList<QString> XXEditPackArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("000140C6") << QStringLiteral("000140E5");
}

XBinary::FT XXEditPackArchive::getFileType()
{
    return FT_XEDITPACK;
}

XBinary::MODE XXEditPackArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XXEditPackArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XXEditPackArchive::getArch()
{
    return QString();
}

qint32 XXEditPackArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XXEditPackArchive::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XXEditPackArchive::getFileFormatExtsString()
{
    return QStringLiteral("XEDIT PACK");
}

QString XXEditPackArchive::getMIMEString()
{
    return QStringLiteral("application/x-xedit-pack");
}

QString XXEditPackArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();
    if (context.nRecordFormat == XEP_FORMAT_FIXED) return QStringLiteral("F");
    return QStringLiteral("V");
}

qint64 XXEditPackArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XXEditPackArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XXEditPackArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XXEditPackArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XXEditPackArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = XEP_HEADER_SIZE;
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
        if (context.bUncompressedSizeKnown) {
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XEDITPACK);
        } else {
            // The decoder takes the output length as an input, so publishing a
            // zero there would quietly write an empty file instead of failing.
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
        }
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("XEDIT PACK"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XXEditPackArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XXEditPackArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XXEditPackArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) || (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != 0) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    if (pContext->bUncompressedSizeKnown) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XEDITPACK);
    } else {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
    }
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("XEDIT PACK"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XXEditPackArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) || (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XXEditPackArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XXEditPackArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
