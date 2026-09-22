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
#include "xcruarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 CRU_HEADER_SIZE = 0x1a;
const qint64 CRU_ENTRY_SIZE = 0x18;
const qint64 CRU_NAME_SIZE = 12;
const qint32 CRU_MAX_MEMBERS = 100000;
const qint32 CRU_MAX_PATH_SIZE = 4096;

bool cruRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XCRUArchive::XCRUArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCRUArchive::~XCRUArchive()
{
}

bool XCRUArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < CRU_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, CRU_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != CRU_HEADER_SIZE)) return false;
    if (baHeader.left(8) != QByteArray("CRUSH v1", 8)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    if ((pHeader[8] != '.') || (pHeader[10] != 0x0a) || (pHeader[11] != 0x1a) || (pHeader[12] != 0)) return false;

    const qint32 nNumberOfDirectories = (qint32)qFromLittleEndian<quint16>(pHeader + 0x10);
    const qint32 nNumberOfMembers = (qint32)qFromLittleEndian<quint16>(pHeader + 0x12);
    const quint16 nZero = qFromLittleEndian<quint16>(pHeader + 0x14);
    const qint64 nTableOffset = (qint32)qFromLittleEndian<quint32>(pHeader + 0x16);
    if ((nNumberOfDirectories >= 0x100) || (nNumberOfMembers <= 1) || (nNumberOfMembers > CRU_MAX_MEMBERS) || (nZero != 0)) return false;
    if (nTableOffset <= (CRU_HEADER_SIZE - 1)) return false;

    context.nMinorVersion = pHeader[9];

    const qint64 nTableSize = (qint64)nNumberOfMembers * CRU_ENTRY_SIZE;
    if (!cruRangeWithin(context.nInputSize, nTableOffset, nTableSize)) return false;

    const QByteArray baTable = read_array_process(nTableOffset, nTableSize, pPdStruct);
    if ((baTable.size() != nTableSize)) return false;
    const quint8 *pTable = (const quint8 *)baTable.constData();

    // The directory names follow the entry table, back to back and NUL
    // terminated.
    QList<QString> listDirectories;
    qint64 nPosition = nTableOffset + nTableSize;
    for (qint32 i = 0; i < nNumberOfDirectories; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        QByteArray baPath;
        for (;;) {
            if (nPosition >= context.nInputSize) return false;
            if (baPath.size() > CRU_MAX_PATH_SIZE) return false;
            const QByteArray baByte = read_array_process(nPosition, 1, pPdStruct);
            if ((baByte.size() != 1)) return false;
            ++nPosition;
            if (baByte.at(0) == (char)0) break;
            baPath.append(baByte.at(0));
        }
        QString sPath = QString::fromLatin1(baPath);
        // "X:\..." loses its drive and root separator; the reference drops four
        // characters flat rather than parsing the path.
        if ((sPath.size() >= 3) && (sPath.at(1) == QChar(':'))) sPath = sPath.mid(4);
        listDirectories.append(sPath);
    }

    context.nArchiveSize = qMin(nPosition, context.nInputSize);

    qint64 nDataOffset = CRU_HEADER_SIZE;
    for (qint32 i = 0; i < nNumberOfMembers; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const quint8 *pEntry = pTable + ((qint64)i * CRU_ENTRY_SIZE);
        const qint32 nDirectoryIndex = pEntry[0];
        const qint64 nSize = (qint32)qFromLittleEndian<quint32>(pEntry + 6);
        if (nSize < 0) return false;
        // Offsets do not exist here, so a member that does not fit means every
        // later member is wrong as well; there is nothing to salvage.
        if (!cruRangeWithin(context.nInputSize, nDataOffset, nSize)) return false;

        qint32 nNameLength = 0;
        while ((nNameLength < CRU_NAME_SIZE) && (pEntry[0x0a + nNameLength] != 0)) ++nNameLength;

        QString sPrefix;
        if ((nDirectoryIndex > 0) && ((nDirectoryIndex - 1) < listDirectories.size())) sPrefix = listDirectories.at(nDirectoryIndex - 1);

        MEMBER member = {};
        member.nHeaderOffset = nTableOffset + ((qint64)i * CRU_ENTRY_SIZE);
        member.nDataOffset = nDataOffset;
        member.nSize = nSize;
        member.nDosTime = qFromLittleEndian<quint16>(pEntry + 2);
        member.nDosDate = qFromLittleEndian<quint16>(pEntry + 4);
        // The stored path already carries its own separator.
        member.sFileName = sPrefix + QString::fromLatin1((const char *)(pEntry + 0x0a), nNameLength);
        context.listMembers.append(member);

        nDataOffset += nSize;
    }

    if (context.listMembers.isEmpty()) return false;
    if (nDataOffset > context.nArchiveSize) context.nArchiveSize = qMin(nDataOffset, context.nInputSize);

    *pContext = context;

    return true;
}

bool XCRUArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XCRUArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCRUArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XCRUArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCRUArchive(pDevice);
}

QList<QString> XCRUArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'CRUSH v1.'");
}

XBinary::FT XCRUArchive::getFileType()
{
    return FT_CRU;
}

XBinary::MODE XCRUArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XCRUArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCRUArchive::getArch()
{
    return QString();
}

qint32 XCRUArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XCRUArchive::getFileFormatExt()
{
    return QStringLiteral("cru");
}

QString XCRUArchive::getFileFormatExtsString()
{
    return QStringLiteral("CRUSH (*.cru)");
}

QString XCRUArchive::getMIMEString()
{
    return QStringLiteral("application/x-crush");
}

QString XCRUArchive::getVersion()
{
    return QStringLiteral("1");
}

qint64 XCRUArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XCRUArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XCRUArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XCRUArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XCRUArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = CRU_HEADER_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XCRUArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCRUArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XCRUArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XCRUArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XCRUArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XCRUArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
