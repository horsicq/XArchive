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
#include "xtwsarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 TWS_RECORD_SIZE = 25;
const qint32 TWS_MAX_MEMBERS = 100000;
const qint32 TWS_NAME_MAX = 12;

bool twsRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XTWSArchive::XTWSArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTWSArchive::~XTWSArchive()
{
}

bool XTWSArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < TWS_RECORD_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, TWS_RECORD_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != TWS_RECORD_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    const qint32 nHeaderNameLength = pHeader[0];
    if ((nHeaderNameLength < 1) || (nHeaderNameLength > TWS_NAME_MAX)) return false;
    if (qFromLittleEndian<quint16>(pHeader + 13) != 1) return false;
    if (qFromLittleEndian<quint16>(pHeader + 15) != 0) return false;
    const qint32 nCountMinusOne = (qint32)qFromLittleEndian<quint32>(pHeader + 17);
    const qint32 nPayloadTotal = (qint32)qFromLittleEndian<quint32>(pHeader + 21);
    if ((nCountMinusOne <= 0) || (nCountMinusOne > TWS_MAX_MEMBERS) || (nPayloadTotal <= 0)) return false;

    const qint32 nCount = nCountMinusOne + 1;
    if (!twsRangeWithin(context.nInputSize, 0, (qint64)(nCount + 1) * TWS_RECORD_SIZE)) return false;

    for (qint32 i = 1; i <= nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecordOffset = (qint64)i * TWS_RECORD_SIZE;
        const QByteArray baRecord = read_array_process(nRecordOffset, TWS_RECORD_SIZE, pPdStruct);
        if (!guardedSource || (baRecord.size() != TWS_RECORD_SIZE)) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();

        const qint32 nNameLength = pRecord[0];
        if ((nNameLength < 1) || (nNameLength > TWS_NAME_MAX)) return false;
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pRecord + 17);
        const qint64 nDataSize = (qint32)qFromLittleEndian<quint32>(pRecord + 21);
        if ((nDataOffset < 0) || (nDataSize < 0)) return false;
        if (!twsRangeWithin(context.nInputSize, nDataOffset, nDataSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nRecordOffset;
        member.nHeaderSize = TWS_RECORD_SIZE;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nDataSize;
        member.nUncompressedSize = nDataSize;
        member.nCRC = 0;
        member.nTime = qFromLittleEndian<quint16>(pRecord + 13) | ((quint32)qFromLittleEndian<quint16>(pRecord + 15) << 16);
        member.nMethod = 0;
        member.bIsFolder = false;
        member.sFileName = QString::fromLatin1((const char *)(pRecord + 1), nNameLength);
        context.listMembers.append(member);
    }

    qint64 nOffset = context.nInputSize;

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XTWSArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTWSArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTWSArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTWSArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTWSArchive(pDevice);
}

QList<QString> XTWSArchive::getSearchSignatures()
{
    // no signature: the header is a Turbo Pascal record like any other
    return QList<QString>();
}

XBinary::FT XTWSArchive::getFileType()
{
    return FT_TWS;
}

XBinary::MODE XTWSArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTWSArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTWSArchive::getArch()
{
    return QString();
}

qint32 XTWSArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTWSArchive::getFileFormatExt()
{
    return QStringLiteral("tws");
}

QString XTWSArchive::getFileFormatExtsString()
{
    return QStringLiteral("TWS archive");
}

QString XTWSArchive::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

QString XTWSArchive::getVersion()
{
    return QString();
}

qint64 XTWSArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XTWSArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTWSArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XTWSArchive::methodToString(quint8 nMethod)
{
    Q_UNUSED(nMethod)
    return QStringLiteral("Stored");
}

XBinary::HANDLE_METHOD XTWSArchive::methodToHandleMethod(quint8 nMethod)
{
    Q_UNUSED(nMethod)
    return HANDLE_METHOD_STORE;
}

bool XTWSArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTWSArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TWS_RECORD_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (member.bIsFolder) continue;

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XTWSArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTWSArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XTWSArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);

    if (member.bIsFolder) {
        result.nStreamOffset = 0;
        result.nStreamSize = 0;
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Directory"));
        return result;
    }

    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));

    return result;
}

bool XTWSArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XTWSArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTWSArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
