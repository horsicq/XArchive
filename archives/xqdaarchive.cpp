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
#include "xqdaarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 QDA_HEADER_SIZE = 0x100;
const qint64 QDA_ENTRY_SIZE = 0x10c;
const qint64 QDA_NAME_SIZE = 0x100;
const qint32 QDA_MAX_MEMBERS = 100000;

bool qdaRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XQDAArchive::XQDAArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XQDAArchive::~XQDAArchive()
{
}

bool XQDAArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XQDAArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < QDA_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, 0x10, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != 0x10)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (baHeader.mid(4, 4) != QByteArray("QDA0", 4)) return false;

    const quint32 nFlag = qFromLittleEndian<quint32>(pHeader);
    if (nFlag > 1) return false;
    const qint32 nNumberOfMembers = (qint32)qFromLittleEndian<quint32>(pHeader + 8);
    if ((qint32)qFromLittleEndian<quint32>(pHeader + 0x0c) != 0) return false;
    // A count of zero yields nothing at all, which the reference reports as a
    // failure rather than as an empty archive.
    if ((nNumberOfMembers < 1) || (nNumberOfMembers > QDA_MAX_MEMBERS)) return false;

    context.bPacked = (nFlag == 1);
    context.nArchiveSize = QDA_HEADER_SIZE + ((qint64)nNumberOfMembers * QDA_ENTRY_SIZE);
    if (context.nArchiveSize > context.nInputSize) return false;

    for (qint32 i = 0; i < nNumberOfMembers; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nEntryOffset = QDA_HEADER_SIZE + ((qint64)i * QDA_ENTRY_SIZE);

        const QByteArray baEntry = read_array_process(nEntryOffset, QDA_ENTRY_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baEntry.size() != QDA_ENTRY_SIZE)) return false;
        const uchar *pEntry = (const uchar *)baEntry.constData();

        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pEntry);
        const qint64 nCompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 4);
        const qint64 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 8);
        if ((nDataOffset < 0) || (nCompressedSize < 0) || (nUncompressedSize < 0)) return false;

        // The stream length depends on the ARCHIVE's flag, not on the entry.
        const qint64 nStreamSize = context.bPacked ? nCompressedSize : nUncompressedSize;
        if (!qdaRangeWithin(context.nInputSize, nDataOffset, nStreamSize)) return false;

        qint32 nNameLength = 0;
        while ((nNameLength < QDA_NAME_SIZE) && (pEntry[0x0c + nNameLength] != 0)) ++nNameLength;

        MEMBER member = {};
        member.nHeaderOffset = nEntryOffset;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nStreamSize;
        member.nUncompressedSize = nUncompressedSize;
        member.sFileName = QString::fromLatin1((const char *)(pEntry + 0x0c), nNameLength);
        context.listMembers.append(member);

        if ((nDataOffset + nStreamSize) > context.nArchiveSize) context.nArchiveSize = nDataOffset + nStreamSize;
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;

    return true;
}

bool XQDAArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XQDAArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQDAArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XQDAArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQDAArchive(pDevice);
}

QList<QString> XQDAArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("........'QDA0'");
}

XBinary::FT XQDAArchive::getFileType()
{
    return FT_QDA;
}

XBinary::MODE XQDAArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XQDAArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XQDAArchive::getArch()
{
    return QString();
}

qint32 XQDAArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XQDAArchive::getFileFormatExt()
{
    return QStringLiteral("qda");
}

QString XQDAArchive::getFileFormatExtsString()
{
    return QStringLiteral("QDA (*.qda)");
}

QString XQDAArchive::getMIMEString()
{
    return QStringLiteral("application/x-qda");
}

QString XQDAArchive::getVersion()
{
    return QString();
}

qint64 XQDAArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XQDAArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XQDAArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XQDAArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XQDAArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = QDA_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, context.bPacked ? HANDLE_METHOD_QDA : HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, context.bPacked ? QStringLiteral("QDA BPE") : QStringLiteral("Stored"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XQDAArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XQDAArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XQDAArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XQDAArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->bPacked ? HANDLE_METHOD_QDA : HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, pContext->bPacked ? QStringLiteral("QDA BPE") : QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XQDAArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XQDAArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XQDAArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
