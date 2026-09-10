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
#include "xdsl2archive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 DSL2_HEADER_SIZE = 0x1c;
const qint64 DSL2_ENTRY_MIN_SIZE = 0x14;
const qint32 DSL2_MAX_MEMBERS = 100000;
const qint64 DSL2_MAX_DIRECTORY_SIZE = 0x1000000;
const quint8 DSL2_OBFUSCATION_BIAS = 0x33;
const quint16 DSL2_FLAG_COMPRESSED = 0x10;

bool dsl2RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XDSL2Archive::XDSL2Archive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDSL2Archive::~XDSL2Archive()
{
}

XBinary::HANDLE_METHOD XDSL2Archive::flagsToHandleMethod(quint16 nFlags)
{
    return (nFlags & DSL2_FLAG_COMPRESSED) ? HANDLE_METHOD_PKWARE_DCL_IMPLODE : HANDLE_METHOD_STORE;
}

QString XDSL2Archive::flagsToString(quint16 nFlags)
{
    return (nFlags & DSL2_FLAG_COMPRESSED) ? QStringLiteral("PKWARE DCL implode") : QStringLiteral("Stored");
}

bool XDSL2Archive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XDSL2Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < DSL2_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, DSL2_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != DSL2_HEADER_SIZE)) return false;
    if (baHeader.left(16) != QByteArray("DS'L install 2.0", 16)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (qFromLittleEndian<quint16>(pHeader + 0x10) != 0) return false;
    const qint32 nNumberOfMembers = (qint32)qFromLittleEndian<quint16>(pHeader + 0x12);
    const qint64 nDirectoryEnd = (qint32)qFromLittleEndian<quint32>(pHeader + 0x18);
    if ((nNumberOfMembers < 1) || (nNumberOfMembers > DSL2_MAX_MEMBERS)) return false;
    if ((nDirectoryEnd <= 0) || (nDirectoryEnd > context.nInputSize)) return false;

    // Two length-prefixed install paths sit between the header and the
    // directory; they are stored in the clear.
    qint64 nDirectoryStart = DSL2_HEADER_SIZE;
    for (qint32 i = 0; i < 2; ++i) {
        if (!dsl2RangeWithin(context.nInputSize, nDirectoryStart, 2)) return false;
        const QByteArray baLength = read_array_process(nDirectoryStart, 2, pPdStruct);
        if (!guardedThis || !guardedSource || (baLength.size() != 2)) return false;
        const qint64 nLength = (qint64)qFromLittleEndian<quint16>((const uchar *)baLength.constData());
        nDirectoryStart += 2 + nLength;
    }
    if ((nDirectoryStart < 0) || (nDirectoryStart > nDirectoryEnd)) return false;

    const qint64 nDirectorySize = nDirectoryEnd - nDirectoryStart;
    if (nDirectorySize > DSL2_MAX_DIRECTORY_SIZE) return false;

    QByteArray baDirectory = read_array_process(nDirectoryStart, nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource || (baDirectory.size() != nDirectorySize)) return false;
    quint8 *pDirectory = (quint8 *)baDirectory.data();
    for (qint64 i = 0; i < nDirectorySize; ++i) pDirectory[i] = (quint8)(pDirectory[i] - DSL2_OBFUSCATION_BIAS);

    context.nArchiveSize = nDirectoryEnd;

    qint64 nPosition = 0;
    qint64 nLeft = nDirectorySize;

    for (qint32 i = 0; i < nNumberOfMembers; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nLeft < DSL2_ENTRY_MIN_SIZE) || ((nPosition + DSL2_ENTRY_MIN_SIZE) > nDirectorySize)) return false;

        const quint8 *pEntry = pDirectory + nPosition;
        const qint64 nRecordSize = (qint64)qFromLittleEndian<quint16>(pEntry);
        if ((nRecordSize < DSL2_ENTRY_MIN_SIZE) || (nRecordSize > nLeft)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nDirectoryStart + nPosition;
        member.nHeaderSize = nRecordSize;
        member.nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 0x02);
        member.nCompressedSize = (qint32)qFromLittleEndian<quint32>(pEntry + 0x06);
        member.nDataOffset = (qint32)qFromLittleEndian<quint32>(pEntry + 0x0a);
        member.nDosTime = qFromLittleEndian<quint16>(pEntry + 0x0e);
        member.nDosDate = qFromLittleEndian<quint16>(pEntry + 0x10);
        member.nFlags = qFromLittleEndian<quint16>(pEntry + 0x12);
        if ((member.nUncompressedSize < 0) || (member.nCompressedSize < 0) || (member.nDataOffset < 0)) return false;

        // The name length counts its own terminating NUL, and the name may run
        // past the record: the reference reads it from the directory block, not
        // from the record, so the only bound that applies is the block's end.
        const qint64 nNameOffset = nPosition + 0x16;
        if (nNameOffset <= nDirectorySize) {
            const qint64 nNameLength = (qint64)qFromLittleEndian<quint16>(pEntry + 0x14);
            qint64 nNameSize = (nNameLength > 0) ? (nNameLength - 1) : 0;
            if (nNameSize > (nDirectorySize - nNameOffset)) nNameSize = nDirectorySize - nNameOffset;
            member.sFileName = QString::fromLatin1((const char *)(pDirectory + nNameOffset), (qint32)nNameSize);
        }

        if (member.nFlags & DSL2_FLAG_COMPRESSED) {
            if (!dsl2RangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;
        } else {
            // A stored member has no second size; the two fields must agree or
            // the record does not describe what is on disk.
            if (member.nCompressedSize != member.nUncompressedSize) return false;
            if (!dsl2RangeWithin(context.nInputSize, member.nDataOffset, member.nUncompressedSize)) return false;
        }
        if ((member.nDataOffset + member.nCompressedSize) > context.nArchiveSize) context.nArchiveSize = member.nDataOffset + member.nCompressedSize;

        context.listMembers.append(member);

        nLeft -= nRecordSize;
        nPosition += nRecordSize;
    }

    if (context.listMembers.isEmpty()) return false;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;

    return true;
}

bool XDSL2Archive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XDSL2Archive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDSL2Archive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XDSL2Archive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDSL2Archive(pDevice);
}

QList<QString> XDSL2Archive::getSearchSignatures()
{
    // Written as hex: the magic contains an apostrophe, which is the quoted
    // form's own delimiter.
    return QList<QString>() << QStringLiteral("4453274C20696E7374616C6C20322E30");
}

XBinary::FT XDSL2Archive::getFileType()
{
    return FT_DSL2;
}

XBinary::MODE XDSL2Archive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XDSL2Archive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XDSL2Archive::getArch()
{
    return QString();
}

qint32 XDSL2Archive::getType()
{
    return TYPE_ARCHIVE;
}

QString XDSL2Archive::getFileFormatExt()
{
    return QStringLiteral("d00");
}

QString XDSL2Archive::getFileFormatExtsString()
{
    return QStringLiteral("DS'L install (*.d00)");
}

QString XDSL2Archive::getMIMEString()
{
    return QStringLiteral("application/x-dsl-install");
}

QString XDSL2Archive::getVersion()
{
    return QStringLiteral("2.0");
}

qint64 XDSL2Archive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XDSL2Archive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XDSL2Archive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XDSL2Archive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XDSL2Archive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = DSL2_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, flagsToHandleMethod(member.nFlags));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, flagsToString(member.nFlags));
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

QMap<XBinary::UNPACK_PROP, QVariant> XDSL2Archive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XDSL2Archive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XDSL2Archive> guardedThis(this);
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

XBinary::ARCHIVERECORD XDSL2Archive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, flagsToHandleMethod(member.nFlags));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, flagsToString(member.nFlags));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XDSL2Archive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XDSL2Archive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XDSL2Archive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
