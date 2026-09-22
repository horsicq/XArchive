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
#include "xteacyarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 TEACY_ENTRY_SIZE = 0x18;
const qint64 TEACY_COUNT_SIZE = 2;
const qint32 TEACY_MAX_MEMBERS = 0xffff;
const qint32 TEACY_NAME_OFFSET = 3;
const qint32 TEACY_NAME_SIZE = 13;

bool teacyRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

QString teacyName(const uchar *pRecord)
{
    QByteArray baName;
    for (qint32 i = 0; i < TEACY_NAME_SIZE; ++i) {
        const char cByte = (char)pRecord[TEACY_NAME_OFFSET + i];
        if (cByte == (char)0) break;
        baName.append(cByte);
    }

    return QString::fromLatin1(baName);
}

}  // namespace

XTeacyArchive::XTeacyArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTeacyArchive::~XTeacyArchive()
{
}

bool XTeacyArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (TEACY_COUNT_SIZE + 2 * TEACY_ENTRY_SIZE)) return false;

    const QByteArray baCount = read_array_process(0, TEACY_COUNT_SIZE, pPdStruct);
    if (!guardedSource || (baCount.size() != TEACY_COUNT_SIZE)) return false;

    const qint32 nCount = (qint32)qFromLittleEndian<quint16>((const uchar *)baCount.constData());
    if ((nCount < 2) || (nCount > TEACY_MAX_MEMBERS)) return false;

    const qint64 nDirectorySize = (qint64)nCount * TEACY_ENTRY_SIZE;
    if (!teacyRangeWithin(context.nInputSize, TEACY_COUNT_SIZE, nDirectorySize)) return false;
    const qint64 nDirectoryEnd = TEACY_COUNT_SIZE + nDirectorySize;

    const QByteArray baDirectory = read_array_process(TEACY_COUNT_SIZE, nDirectorySize, pPdStruct);
    if (!guardedSource || (baDirectory.size() != nDirectorySize)) return false;

    const uchar *pDirectory = (const uchar *)baDirectory.constData();
    qint64 nArchiveEnd = nDirectoryEnd;
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pRecord = pDirectory + (qint64)i * TEACY_ENTRY_SIZE;
        // The three leading zero bytes are the only fixed content the format
        // has; the detector uses them in place of a magic.
        if (qFromLittleEndian<quint16>(pRecord) != 0) return false;
        if (pRecord[2] != 0) return false;

        const qint64 nSize = (qint32)qFromLittleEndian<quint32>(pRecord + 0x10);
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pRecord + 0x14);
        if ((nSize < 0) || (nDataOffset < 3)) return false;
        if (!teacyRangeWithin(context.nInputSize, nDataOffset, nSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = TEACY_COUNT_SIZE + (qint64)i * TEACY_ENTRY_SIZE;
        member.nDataOffset = nDataOffset;
        member.nSize = nSize;
        member.sFileName = teacyName(pRecord);
        if (member.sFileName.isEmpty()) member.sFileName = QStringLiteral("teacy%1").arg(i);
        context.listMembers.append(member);
        nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nSize);
    }

    // The reference detector's structural invariants: the payload area opens
    // right after the directory and the members run consecutively.
    if (context.listMembers.at(0).nDataOffset != nDirectoryEnd) return false;
    if (context.listMembers.at(1).nDataOffset != (context.listMembers.at(0).nDataOffset + context.listMembers.at(0).nSize)) return false;

    context.nDirectorySize = nDirectoryEnd;
    context.nArchiveSize = qMin(nArchiveEnd, context.nInputSize);
    *pContext = context;

    return true;
}

bool XTeacyArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTeacyArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTeacyArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTeacyArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTeacyArchive(pDevice);
}

QList<QString> XTeacyArchive::getSearchSignatures()
{
    // Headerless: the format has no magic to scan for.
    return QList<QString>();
}

XBinary::FT XTeacyArchive::getFileType()
{
    return FT_TEACY;
}

XBinary::MODE XTeacyArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTeacyArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTeacyArchive::getArch()
{
    return QString();
}

qint32 XTeacyArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTeacyArchive::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XTeacyArchive::getFileFormatExtsString()
{
    return QStringLiteral("Teacy (*.dat)");
}

QString XTeacyArchive::getMIMEString()
{
    return QStringLiteral("application/x-teacy");
}

QString XTeacyArchive::getVersion()
{
    return QString();
}

qint64 XTeacyArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XTeacyArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTeacyArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XTeacyArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTeacyArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
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

QMap<XBinary::UNPACK_PROP, QVariant> XTeacyArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTeacyArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XTeacyArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTeacyArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XTeacyArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTeacyArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
