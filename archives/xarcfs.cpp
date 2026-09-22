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
#include "xarcfs.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 ARCFS_HEADER_SIZE = 96;
const qint64 ARCFS_ENTRY_SIZE = 36;
const qint32 ARCFS_MAX_MEMBERS = 100000;
const qint32 ARCFS_MAX_DEPTH = 64;

const quint8 ARCFS_STATUS_END = 0x00;
const quint8 ARCFS_STATUS_DELETED = 0x01;
const quint8 ARCFS_METHOD_STORED = 0x82;
const quint8 ARCFS_METHOD_PACKED = 0x83;
const quint8 ARCFS_METHOD_CRUNCHED = 0x88;
const quint8 ARCFS_METHOD_COMPRESSED = 0xff;

bool arcfsRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

QString arcfsName(const QByteArray &baRaw)
{
    QByteArray baName = baRaw;
    const qint32 nZero = baName.indexOf('\0');
    if (nZero >= 0) baName.truncate(nZero);
    QString sResult;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = (quint8)baName.at(i);
        sResult.append((nCharacter < 0x20) ? QChar('_') : QChar((ushort)nCharacter));
    }
    return sResult;
}
}  // namespace

XArcFS::XArcFS(QIODevice *pDevice) : XArchive(pDevice)
{
}

XArcFS::~XArcFS()
{
}

bool XArcFS::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ARCFS_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, ARCFS_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != ARCFS_HEADER_SIZE)) return false;
    if (baHeader.left(8) != QByteArray("Archive\0", 8)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    const qint64 nDirectorySize = (qint32)qFromLittleEndian<quint32>(pHeader + 8);
    const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pHeader + 12);
    if ((nDirectorySize <= 0) || ((nDirectorySize % ARCFS_ENTRY_SIZE) != 0)) return false;
    if ((nDataOffset < ARCFS_HEADER_SIZE) || (nDataOffset > context.nInputSize)) return false;
    if (!arcfsRangeWithin(context.nInputSize, ARCFS_HEADER_SIZE, nDirectorySize)) return false;

    const qint64 nEntryCount = nDirectorySize / ARCFS_ENTRY_SIZE;
    if (nEntryCount > ARCFS_MAX_MEMBERS) return false;

    QStringList listPath;
    qint64 nArchiveEnd = nDataOffset;

    for (qint64 i = 0; i < nEntryCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nEntryOffset = ARCFS_HEADER_SIZE + (i * ARCFS_ENTRY_SIZE);
        const QByteArray baEntry = read_array_process(nEntryOffset, ARCFS_ENTRY_SIZE, pPdStruct);
        if ((baEntry.size() != ARCFS_ENTRY_SIZE)) return false;
        const uchar *pEntry = (const uchar *)baEntry.constData();

        const quint8 nStatus = pEntry[0];
        if (nStatus == ARCFS_STATUS_DELETED) continue;
        if (nStatus == ARCFS_STATUS_END) {
            if (listPath.isEmpty()) break;
            listPath.removeLast();
            continue;
        }

        const qint64 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 12);
        const quint32 nLoadAddress = qFromLittleEndian<quint32>(pEntry + 16);
        const quint32 nExecAddress = qFromLittleEndian<quint32>(pEntry + 20);
        const quint32 nAttributes = qFromLittleEndian<quint32>(pEntry + 24);
        const qint64 nCompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 28);
        const quint32 nRawOffset = qFromLittleEndian<quint32>(pEntry + 32);

        const QString sName = arcfsName(baEntry.mid(1, 11));
        if (sName.isEmpty()) break;

        MEMBER member = {};
        member.nHeaderOffset = nEntryOffset;
        member.nMethod = nStatus;
        member.nMaxBits = (quint8)((nAttributes >> 8) & 0xffU);
        member.nLoadAddress = nLoadAddress;
        member.nExecAddress = nExecAddress;
        member.sFileName = listPath.isEmpty() ? sName : (listPath.join(QChar('/')) + QChar('/') + sName);

        if (nRawOffset & 0x80000000U) {
            // A directory. The reference implementation additionally insists
            // both size fields are -1 and gives up on the whole archive when
            // they are not; several archives store an empty directory with
            // zeroes there, so only the flag is trusted here.
            member.bIsFolder = true;
            member.nDataOffset = -1;
            member.nCompressedSize = 0;
            member.nUncompressedSize = 0;
            context.listMembers.append(member);
            if (listPath.size() >= ARCFS_MAX_DEPTH) break;
            listPath.append(sName);
            continue;
        }

        if ((nUncompressedSize < 0) || (nCompressedSize < 0)) break;

        member.bIsFolder = false;
        member.nDataOffset = nDataOffset + (qint64)(nRawOffset & 0x7fffffffU);
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;

        // Truncated archives do occur; list the member but clamp its stream.
        if (!arcfsRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) {
            if ((member.nDataOffset < 0) || (member.nDataOffset > context.nInputSize)) continue;
            member.nCompressedSize = context.nInputSize - member.nDataOffset;
        }

        if ((member.nDataOffset + member.nCompressedSize) > nArchiveEnd) nArchiveEnd = member.nDataOffset + member.nCompressedSize;

        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nArchiveEnd;
    *pContext = context;

    return true;
}

bool XArcFS::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XArcFS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XArcFS archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XArcFS::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XArcFS(pDevice);
}

QList<QString> XArcFS::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'Archive'00");
}

XBinary::FT XArcFS::getFileType()
{
    return FT_ARCFS;
}

XBinary::MODE XArcFS::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XArcFS::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XArcFS::getArch()
{
    return QString();
}

qint32 XArcFS::getType()
{
    return TYPE_ARCHIVE;
}

QString XArcFS::getFileFormatExt()
{
    return QStringLiteral("arc");
}

QString XArcFS::getFileFormatExtsString()
{
    return QStringLiteral("ArcFS (*.arc)");
}

QString XArcFS::getMIMEString()
{
    return QStringLiteral("application/x-arcfs");
}

QString XArcFS::getVersion()
{
    return QString();
}

qint64 XArcFS::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XArcFS::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XArcFS::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XArcFS::methodToString(quint8 nMethod)
{
    if (nMethod == ARCFS_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == ARCFS_METHOD_PACKED) return QStringLiteral("Packed (RLE90)");
    if (nMethod == ARCFS_METHOD_CRUNCHED) return QStringLiteral("Crunched (LZW+RLE90)");
    if (nMethod == ARCFS_METHOD_COMPRESSED) return QStringLiteral("Compressed (LZW)");
    return QStringLiteral("Unknown 0x%1").arg(nMethod, 2, 16, QLatin1Char('0'));
}

XBinary::HANDLE_METHOD XArcFS::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == ARCFS_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == ARCFS_METHOD_PACKED) return HANDLE_METHOD_ARCFS_PACKED;
    if (nMethod == ARCFS_METHOD_CRUNCHED) return HANDLE_METHOD_ARCFS_CRUNCHED;
    if (nMethod == ARCFS_METHOD_COMPRESSED) return HANDLE_METHOD_ARCFS_COMPRESSED;
    return HANDLE_METHOD_UNKNOWN;
}

bool XArcFS::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XArcFS::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ARCFS_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)member.nMaxBits);
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

QMap<XBinary::UNPACK_PROP, QVariant> XArcFS::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XArcFS::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XArcFS::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    // The LZW code width travels as the window size; the decoder reads it back
    // from there because the whole-buffer dispatch has no other channel.
    result.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)member.nMaxBits);

    return result;
}

bool XArcFS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XArcFS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XArcFS::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_WINDOWSIZE;
}
