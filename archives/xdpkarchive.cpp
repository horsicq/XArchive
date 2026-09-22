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
#include "xdpkarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 DPK_HEADER_SIZE = 0x10;
const qint64 DPK_ENTRY_MIN_SIZE = 0x10;
const qint64 DPK_ENTRY_STRIDE = 0x14;
const qint32 DPK_MAX_MEMBERS = 100000;
const qint64 DPK_MAX_DIRECTORY_SIZE = 0x1000000;

bool dpkRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XDPKArchive::XDPKArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDPKArchive::~XDPKArchive()
{
}

bool XDPKArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < DPK_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, DPK_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != DPK_HEADER_SIZE) return false;
    if (baHeader.left(4) != QByteArray("DPK4", 4)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    const qint64 nTotalSize = (qint32)qFromLittleEndian<quint32>(pHeader + 4);
    const qint64 nDirectorySize = (qint32)qFromLittleEndian<quint32>(pHeader + 8);
    const qint32 nNumberOfMembers = (qint32)qFromLittleEndian<quint32>(pHeader + 0x0c);
    // The header carries the file's own length; a mismatch means this is not a
    // DPK4 at all, and it is the format's only self-check.
    if (nTotalSize != context.nInputSize) return false;
    if ((nDirectorySize <= 0) || (nDirectorySize > DPK_MAX_DIRECTORY_SIZE)) return false;
    if ((nNumberOfMembers <= 0) || (nNumberOfMembers > DPK_MAX_MEMBERS)) return false;
    if ((qint64)nNumberOfMembers > (nDirectorySize / DPK_ENTRY_STRIDE)) return false;
    if (!dpkRangeWithin(context.nInputSize, DPK_HEADER_SIZE, nDirectorySize)) return false;

    QByteArray baDirectory = read_array_process(DPK_HEADER_SIZE, nDirectorySize, pPdStruct);
    if (baDirectory.size() != nDirectorySize) return false;
    const quint8 *pDirectory = (const quint8 *)baDirectory.constData();

    context.nArchiveSize = DPK_HEADER_SIZE + nDirectorySize;

    qint64 nPosition = 0;
    qint64 nLeft = nDirectorySize;

    for (qint32 i = 0; i < nNumberOfMembers; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nLeft < DPK_ENTRY_MIN_SIZE) || ((nPosition + DPK_ENTRY_MIN_SIZE) > nDirectorySize)) return false;

        const quint8 *pEntry = pDirectory + nPosition;
        const qint64 nRecordSize = (qint32)qFromLittleEndian<quint32>(pEntry);
        if ((nRecordSize < DPK_ENTRY_MIN_SIZE) || (nRecordSize > nLeft)) return false;

        const qint64 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 4);
        const qint64 nCompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 8);
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pEntry + 0x0c);
        if ((nUncompressedSize < 0) || (nCompressedSize < 0) || (nDataOffset < 0)) return false;

        MEMBER member = {};
        member.nHeaderOffset = DPK_HEADER_SIZE + nPosition;
        member.nHeaderSize = nRecordSize;
        member.nDataOffset = nDataOffset;
        member.nUncompressedSize = nUncompressedSize;
        member.bStored = (nCompressedSize == nUncompressedSize);
        member.nCompressedSize = nCompressedSize;
        if (!dpkRangeWithin(context.nInputSize, nDataOffset, member.nCompressedSize)) return false;

        qint64 nNameSize = nRecordSize - DPK_ENTRY_MIN_SIZE;
        while ((nNameSize > 0) && (pEntry[DPK_ENTRY_MIN_SIZE + nNameSize - 1] == 0)) --nNameSize;
        member.sFileName = QString::fromLatin1((const char *)(pEntry + DPK_ENTRY_MIN_SIZE), (qint32)nNameSize);

        if ((nDataOffset + member.nCompressedSize) > context.nArchiveSize) context.nArchiveSize = nDataOffset + member.nCompressedSize;

        context.listMembers.append(member);

        nLeft -= nRecordSize;
        nPosition += nRecordSize;
    }

    if (context.listMembers.isEmpty()) return false;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;

    return true;
}

bool XDPKArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XDPKArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDPKArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XDPKArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDPKArchive(pDevice);
}

QList<QString> XDPKArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'DPK4'");
}

XBinary::FT XDPKArchive::getFileType()
{
    return FT_DPK;
}

XBinary::MODE XDPKArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XDPKArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XDPKArchive::getArch()
{
    return QString();
}

qint32 XDPKArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XDPKArchive::getFileFormatExt()
{
    return QStringLiteral("dpk");
}

QString XDPKArchive::getFileFormatExtsString()
{
    return QStringLiteral("DPK (*.dpk)");
}

QString XDPKArchive::getMIMEString()
{
    return QStringLiteral("application/x-dpk");
}

QString XDPKArchive::getVersion()
{
    return QStringLiteral("4");
}

qint64 XDPKArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XDPKArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XDPKArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XDPKArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XDPKArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = DPK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_ZLIB);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bStored ? QStringLiteral("Stored") : QStringLiteral("Deflate"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XDPKArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XDPKArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XDPKArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_ZLIB);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bStored ? QStringLiteral("Stored") : QStringLiteral("Deflate"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XDPKArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XDPKArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XDPKArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
