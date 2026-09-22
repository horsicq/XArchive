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
#include "xzcmparchive.h"

#include <QFileInfo>
#include <QtEndian>

#include <new>

namespace {
const qint64 ZCMP_HEADER_SIZE = 40;
const qint64 ZCMP_MAX_BLOCKS = 4000000;
const qint64 ZCMP_MAX_OUTPUT = 0x7fffffff;

bool zcmpRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XZcmpArchive::XZcmpArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZcmpArchive::~XZcmpArchive()
{
}

bool XZcmpArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ZCMP_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, ZCMP_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != ZCMP_HEADER_SIZE)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (qFromBigEndian<quint32>(pHeader) != 0) return false;
    if (baHeader.mid(4, 4) != QByteArray("Zcmp", 4)) return false;
    if (qFromBigEndian<quint64>(pHeader + 8) != Q_UINT64_C(1)) return false;
    if (qFromBigEndian<quint64>(pHeader + 16) != Q_UINT64_C(1)) return false;

    const qint64 nUncompressedSize = (qint64)qFromBigEndian<quint64>(pHeader + 24);
    const qint64 nBlockSize = (qint64)qFromBigEndian<quint64>(pHeader + 32);
    if ((nUncompressedSize < 0) || (nUncompressedSize > ZCMP_MAX_OUTPUT)) return false;
    if ((nBlockSize <= 0) || (nBlockSize > ZCMP_MAX_OUTPUT)) return false;

    const qint64 nBlockCount = (nUncompressedSize + nBlockSize - 1) / nBlockSize;
    if ((nBlockCount < 0) || (nBlockCount > ZCMP_MAX_BLOCKS)) return false;

    // The seek index is skipped, never read: see the TRAP in the header.
    const qint64 nIndexSize = (nBlockCount + 1) * 8;
    if (!zcmpRangeWithin(context.nInputSize, ZCMP_HEADER_SIZE, nIndexSize)) return false;

    context.nDataOffset = ZCMP_HEADER_SIZE + nIndexSize;
    context.nCompressedSize = context.nInputSize - context.nDataOffset;
    context.nUncompressedSize = nUncompressedSize;
    context.nBlockSize = nBlockSize;
    context.nBlockCount = nBlockCount;
    if ((nBlockCount > 0) && (context.nCompressedSize <= 0)) return false;

    QString sName = QFileInfo(getDeviceFileName(guardedSource)).fileName();
    if (!guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("zcmp");
    context.sFileName = sName;

    *pContext = context;

    return true;
}

bool XZcmpArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZcmpArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZcmpArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZcmpArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZcmpArchive(pDevice);
}

QList<QString> XZcmpArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("00000000'Zcmp'");
}

XBinary::FT XZcmpArchive::getFileType()
{
    return FT_ZCMP;
}

XBinary::MODE XZcmpArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZcmpArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XZcmpArchive::getArch()
{
    return QString();
}

qint32 XZcmpArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZcmpArchive::getFileFormatExt()
{
    return QStringLiteral("z");
}

QString XZcmpArchive::getFileFormatExtsString()
{
    return QStringLiteral("Zcmp compressed file");
}

QString XZcmpArchive::getMIMEString()
{
    return QStringLiteral("application/x-zcmp");
}

QString XZcmpArchive::getVersion()
{
    return QStringLiteral("1");
}

qint64 XZcmpArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XZcmpArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZcmpArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XZcmpArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZcmpArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        // The header part covers the seek index too - it is metadata that is
        // deliberately skipped, not payload.
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDataOffset;
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
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZCMP_BLOCKS);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate blocks"));
        part.mapProperties.insert(FPART_PROP_WINDOWSIZE, context.nBlockSize);
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

QMap<XBinary::UNPACK_PROP, QVariant> XZcmpArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZcmpArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nDataOffset;
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

XBinary::ARCHIVERECORD XZcmpArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZCMP_BLOCKS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate blocks"));
    result.mapProperties.insert(FPART_PROP_WINDOWSIZE, pContext->nBlockSize);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZcmpArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XZcmpArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZcmpArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_WINDOWSIZE << FPART_PROP_ISFOLDER;
}
