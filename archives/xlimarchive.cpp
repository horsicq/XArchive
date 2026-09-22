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
#include "xlimarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 LIM_HEADER_SIZE = 8;
const qint64 LIM_RECORD_SIZE = 0x15;
const qint32 LIM_MAX_MEMBERS = 100000;
const qint32 LIM_MAX_NAME_SIZE = 4096;

const quint16 LIM_TAG_DIRECTORY = 0xd180;
const quint16 LIM_TAG_FILE = 0xf123;
const quint8 LIM_METHOD_STORED = 0;
const quint8 LIM_METHOD_PACKED = 1;

bool limRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XLIMArchive::XLIMArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLIMArchive::~XLIMArchive()
{
}

bool XLIMArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < LIM_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, LIM_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != LIM_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("LM\x1a\x08", 4)) return false;
    if (baHeader.at(4) != (char)0) return false;
    if (qFromLittleEndian<quint16>((const uchar *)baHeader.constData() + 6) != 0x0010) return false;

    QString sDirectory;
    qint64 nOffset = LIM_HEADER_SIZE;
    while ((nOffset + 4) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= LIM_MAX_MEMBERS) break;
        const QByteArray baChunk = read_array_process(nOffset, 4, pPdStruct);
        if ((baChunk.size() != 4)) return false;
        const quint16 nTag = qFromLittleEndian<quint16>((const uchar *)baChunk.constData());
        const qint16 nChunkSize = (qint16)qFromLittleEndian<quint16>((const uchar *)baChunk.constData() + 2);
        if (nChunkSize < 4) break;
        nOffset += 4;

        if (nTag == LIM_TAG_DIRECTORY) {
            QByteArray baName;
            while (baName.size() <= LIM_MAX_NAME_SIZE) {
                if (nOffset >= context.nInputSize) return false;
                const QByteArray baByte = read_array_process(nOffset, 1, pPdStruct);
                if ((baByte.size() != 1)) return false;
                ++nOffset;
                if (baByte.at(0) == (char)0) break;
                baName.append(baByte.at(0));
            }
            sDirectory = QString::fromLatin1(baName);
            continue;
        }

        if (nTag != LIM_TAG_FILE) break;

        const qint64 nRecordOffset = nOffset;
        if (!limRangeWithin(context.nInputSize, nOffset, LIM_RECORD_SIZE)) break;
        const QByteArray baRecord = read_array_process(nOffset, LIM_RECORD_SIZE, pPdStruct);
        if ((baRecord.size() != LIM_RECORD_SIZE)) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();
        nOffset += LIM_RECORD_SIZE;

        QByteArray baName;
        while (baName.size() <= LIM_MAX_NAME_SIZE) {
            if (nOffset >= context.nInputSize) return false;
            const QByteArray baByte = read_array_process(nOffset, 1, pPdStruct);
            if ((baByte.size() != 1)) return false;
            ++nOffset;
            if (baByte.at(0) == (char)0) break;
            baName.append(baByte.at(0));
        }

        const quint8 nAttributes = pRecord[6];
        const quint8 nMethod = pRecord[8];
        const qint64 nOriginal = (qint32)qFromLittleEndian<quint32>(pRecord + 9);
        const qint64 nPacked = (qint32)qFromLittleEndian<quint32>(pRecord + 13);
        const quint32 nCRC = qFromLittleEndian<quint32>(pRecord + 17);
        if ((nOriginal < 0) || (nPacked < 0)) break;

        MEMBER member = {};
        member.nHeaderOffset = nRecordOffset;
        member.nHeaderSize = nOffset - nRecordOffset;
        member.nDataOffset = nOffset;
        member.nCompressedSize = nPacked;
        member.nUncompressedSize = nOriginal;
        member.nCRC = nCRC;
        member.nTime = qFromLittleEndian<quint16>(pRecord + 2) | ((quint32)qFromLittleEndian<quint16>(pRecord + 4) << 16);
        member.nMethod = nMethod;
        member.bIsFolder = ((nAttributes & 0x10) != 0);
        member.sFileName = sDirectory.isEmpty() ? QString::fromLatin1(baName)
                                                : (sDirectory + QChar('/') + QString::fromLatin1(baName));
        if (!limRangeWithin(context.nInputSize, nOffset, nPacked)) {
            if (nOffset > context.nInputSize) break;
            member.nCompressedSize = context.nInputSize - nOffset;
        }
        context.listMembers.append(member);
        nOffset += nPacked;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XLIMArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XLIMArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLIMArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLIMArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLIMArchive(pDevice);
}

QList<QString> XLIMArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'LM'1A08");
}

XBinary::FT XLIMArchive::getFileType()
{
    return FT_LIM;
}

XBinary::MODE XLIMArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLIMArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLIMArchive::getArch()
{
    return QString();
}

qint32 XLIMArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XLIMArchive::getFileFormatExt()
{
    return QStringLiteral("lim");
}

QString XLIMArchive::getFileFormatExtsString()
{
    return QStringLiteral("LIM (*.lim)");
}

QString XLIMArchive::getMIMEString()
{
    return QStringLiteral("application/x-lim");
}

QString XLIMArchive::getVersion()
{
    return QString();
}

qint64 XLIMArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XLIMArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XLIMArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XLIMArchive::methodToString(quint8 nMethod)
{
    if (nMethod == LIM_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == LIM_METHOD_PACKED) return QStringLiteral("LIM");
    return QStringLiteral("Unknown %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XLIMArchive::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == LIM_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == LIM_METHOD_PACKED) return HANDLE_METHOD_LIM;
    return HANDLE_METHOD_UNKNOWN;
}

bool XLIMArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XLIMArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LIM_HEADER_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XLIMArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLIMArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
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
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XLIMArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XLIMArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XLIMArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XLIMArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
