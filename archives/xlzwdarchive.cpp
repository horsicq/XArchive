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
#include "xlzwdarchive.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 LZWD_HEADER_SIZE = 11;
const qint64 LZWD_PROBE_SIZE = 12;  // the header plus the first stream byte
const quint32 LZWD_MAGIC = 0x575a4cfcU;
const quint16 LZWD_VERSION = 1;
const quint8 LZWD_FLAGS = 0;
const quint8 LZWD_FIRST_STREAM_BYTE = 0x80;  // a nine-bit MSB-first clear code
}  // namespace

XLZWDArchive::XLZWDArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLZWDArchive::~XLZWDArchive()
{
}

bool XLZWDArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XLZWDArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <= LZWD_PROBE_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, LZWD_PROBE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != LZWD_PROBE_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (qFromLittleEndian<quint32>(pHeader + 4) != LZWD_MAGIC) return false;
    const qint32 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pHeader);
    if (nUncompressedSize < 0) return false;
    const quint16 nVersion = qFromLittleEndian<quint16>(pHeader + 8);
    if (nVersion != LZWD_VERSION) return false;
    if (pHeader[10] != LZWD_FLAGS) return false;
    if (pHeader[11] != LZWD_FIRST_STREAM_BYTE) return false;

    context.nVersion = nVersion;
    context.nDataOffset = LZWD_HEADER_SIZE;
    context.nCompressedSize = context.nInputSize - LZWD_HEADER_SIZE;
    context.nUncompressedSize = nUncompressedSize;

    QString sName = QFileInfo(getDeviceFileName(guardedSource.data())).fileName();
    if (!guardedThis || !guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("data");
    context.sFileName = sName;

    *pContext = context;

    return true;
}

bool XLZWDArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XLZWDArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZWDArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLZWDArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLZWDArchive(pDevice);
}

QList<QString> XLZWDArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("........FC4C5A5701000080");
}

XBinary::FT XLZWDArchive::getFileType()
{
    return FT_LZWD;
}

XBinary::MODE XLZWDArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLZWDArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLZWDArchive::getArch()
{
    return QString();
}

qint32 XLZWDArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XLZWDArchive::getFileFormatExt()
{
    return QStringLiteral("lzw");
}

QString XLZWDArchive::getFileFormatExtsString()
{
    return QStringLiteral("LZWD (*)");
}

QString XLZWDArchive::getMIMEString()
{
    return QStringLiteral("application/x-newwave-lzw");
}

QString XLZWDArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();

    return QString::number(context.nVersion);
}

qint64 XLZWDArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XLZWDArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XLZWDArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XLZWDArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XLZWDArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LZWD_HEADER_SIZE;
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
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZWD_LZW);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("NewWave LZW"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XLZWDArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLZWDArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XLZWDArchive> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
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

XBinary::ARCHIVERECORD XLZWDArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZWD_LZW);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("NewWave LZW"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XLZWDArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XLZWDArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XLZWDArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
