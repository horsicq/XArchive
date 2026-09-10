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
#include "xzfsfarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 ZFSF_HEADER_SIZE = 0x1c;
const qint64 ZFSF_ENTRY_SIZE = 0x24;
const qint64 ZFSF_NAME_SIZE = 0x10;
const qint64 ZFSF_FIRST_GROUP = 0x1c;

bool zfsfRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

QString zfsfName(const QByteArray &baField)
{
    const qint32 nZero = baField.indexOf((char)0);
    return QString::fromLatin1((nZero >= 0) ? baField.left(nZero) : baField);
}

}  // namespace

XZFSFArchive::XZFSFArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZFSFArchive::~XZFSFArchive()
{
}

bool XZFSFArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XZFSFArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ZFSF_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, ZFSF_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != ZFSF_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("ZFSF", 4)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (qFromLittleEndian<quint32>(pHeader + 4) != 1) return false;
    if (qFromLittleEndian<quint32>(pHeader + 8) != 0x10) return false;
    const qint32 nGroupCapacity = (qint32)qFromLittleEndian<quint32>(pHeader + 12);
    const qint32 nTotalCount = (qint32)qFromLittleEndian<quint32>(pHeader + 16);
    if (qFromLittleEndian<quint32>(pHeader + 24) != (quint32)ZFSF_FIRST_GROUP) return false;
    if ((nGroupCapacity <= 0) || (nTotalCount < 0)) return false;
    // Every entry is a distinct 0x24-byte record inside the file, so a count
    // that cannot physically fit is a rejection, not a clamp.
    if ((qint64)nTotalCount > (context.nInputSize / ZFSF_ENTRY_SIZE)) return false;

    qint64 nGroupOffset = ZFSF_FIRST_GROUP;
    qint32 nRemaining = nTotalCount;

    while (nRemaining > 0) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!zfsfRangeWithin(context.nInputSize, nGroupOffset, 4)) return false;

        const QByteArray baLink = read_array_process(nGroupOffset, 4, pPdStruct);
        if (!guardedThis || !guardedSource || (baLink.size() != 4)) return false;
        const qint64 nNextGroup = (qint64)qFromLittleEndian<quint32>((const uchar *)baLink.constData());

        qint64 nEntryOffset = nGroupOffset + 4;
        // A group holds min(groupCap, remaining) entries; reading the full
        // capacity out of the last one invents members out of member data.
        qint32 nInGroup = (nGroupCapacity < nRemaining) ? nGroupCapacity : nRemaining;

        for (qint32 i = 0; i < nInGroup; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (!zfsfRangeWithin(context.nInputSize, nEntryOffset, ZFSF_ENTRY_SIZE)) return false;

            const QByteArray baEntry = read_array_process(nEntryOffset, ZFSF_ENTRY_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource || (baEntry.size() != ZFSF_ENTRY_SIZE)) return false;
            const uchar *pEntry = (const uchar *)baEntry.constData();

            const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pEntry + 0x10);
            const qint64 nDataSize = (qint32)qFromLittleEndian<quint32>(pEntry + 0x18);
            if ((nDataOffset < 0) || (nDataSize < 0)) return false;

            MEMBER member = {};
            member.nHeaderOffset = nEntryOffset;
            member.nDataOffset = nDataOffset;
            member.nDataSize = nDataSize;
            member.nMTime = qFromLittleEndian<quint32>(pEntry + 0x1c);
            member.sFileName = zfsfName(baEntry.left((qint32)ZFSF_NAME_SIZE));
            if (!zfsfRangeWithin(context.nInputSize, member.nDataOffset, member.nDataSize)) return false;

            context.listMembers.append(member);
            nEntryOffset += ZFSF_ENTRY_SIZE;
            --nRemaining;
        }

        if (nNextGroup == 0) break;
        // A link that does not move forward is how a crafted file loops.
        if (nNextGroup <= nGroupOffset) return false;
        nGroupOffset = nNextGroup;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return true;
}

bool XZFSFArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZFSFArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZFSFArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZFSFArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZFSFArchive(pDevice);
}

QList<QString> XZFSFArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ZFSF'01000000");
}

XBinary::FT XZFSFArchive::getFileType()
{
    return FT_ZFSF;
}

XBinary::MODE XZFSFArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZFSFArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZFSFArchive::getArch()
{
    return QString();
}

qint32 XZFSFArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZFSFArchive::getFileFormatExt()
{
    return QStringLiteral("zfs");
}

QString XZFSFArchive::getFileFormatExtsString()
{
    return QStringLiteral("ZFSF (*.zfs)");
}

QString XZFSFArchive::getMIMEString()
{
    return QStringLiteral("application/x-zfsf");
}

QString XZFSFArchive::getVersion()
{
    return QStringLiteral("1");
}

qint64 XZFSFArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XZFSFArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZFSFArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XZFSFArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZFSFArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ZFSF_HEADER_SIZE;
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
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
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

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XZFSFArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZFSFArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XZFSFArchive> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
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

XBinary::ARCHIVERECORD XZFSFArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZFSFArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XZFSFArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZFSFArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
