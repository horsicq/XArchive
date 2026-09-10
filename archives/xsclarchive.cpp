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
#include "xsclarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 SCL_HEADER_SIZE = 9;
const qint64 SCL_ENTRY_SIZE = 14;
const qint64 SCL_SECTOR_SIZE = 256;
const qint64 SCL_PREFIX_SIZE = 17;

bool sclRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XSCLArchive::XSCLArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSCLArchive::~XSCLArchive()
{
}

bool XSCLArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSCLArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SCL_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, SCL_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SCL_HEADER_SIZE)) return false;
    if (baHeader.left(8) != QByteArray("SINCLAIR")) return false;

    const qint32 nCount = (quint8)baHeader.at(8);
    if (nCount == 0) return false;
    const qint64 nCatalogueSize = (qint64)nCount * SCL_ENTRY_SIZE;
    if (!sclRangeWithin(context.nInputSize, SCL_HEADER_SIZE, nCatalogueSize)) return false;

    qint64 nDataOffset = SCL_HEADER_SIZE + nCatalogueSize;

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nEntryOffset = SCL_HEADER_SIZE + ((qint64)i * SCL_ENTRY_SIZE);
        const QByteArray baEntry = read_array_process(nEntryOffset, SCL_ENTRY_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baEntry.size() != SCL_ENTRY_SIZE)) return false;

        const qint32 nSectors = (quint8)baEntry.at(13);
        const qint64 nDataSize = (qint64)nSectors * SCL_SECTOR_SIZE;

        QByteArray baPrefix = baEntry.left(13);
        baPrefix.append((char)0);
        baPrefix.append((char)(quint8)nSectors);
        quint16 nChecksum = 0;
        for (qint32 k = 0; k < baPrefix.size(); ++k) nChecksum = (quint16)(nChecksum + (quint8)baPrefix.at(k));
        nChecksum = (quint16)((nChecksum * 0x101U) + 0x69U);
        baPrefix.append((char)(quint8)(nChecksum & 0xff));
        baPrefix.append((char)(quint8)((nChecksum >> 8) & 0xff));

        QString sName = QString::fromLatin1(baEntry.left(8)).trimmed();
        const quint8 nExtension = (quint8)baEntry.at(8);
        sName += QChar('.');
        sName += (nExtension < 0x20) ? QChar('_') : QChar((ushort)nExtension);

        MEMBER member = {};
        member.nHeaderOffset = nEntryOffset;
        member.nDataOffset = nDataOffset;
        member.nDataSize = nDataSize;
        member.baPrefix = baPrefix;
        member.sFileName = sName;

        // Truncated images do occur; clamp rather than reject the catalogue.
        if (!sclRangeWithin(context.nInputSize, member.nDataOffset, member.nDataSize)) {
            if (member.nDataOffset > context.nInputSize) break;
            member.nDataSize = context.nInputSize - member.nDataOffset;
        }

        context.listMembers.append(member);
        nDataOffset += nDataSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nDataOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XSCLArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XSCLArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSCLArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSCLArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSCLArchive(pDevice);
}

QList<QString> XSCLArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'SINCLAIR'");
}

XBinary::FT XSCLArchive::getFileType()
{
    return FT_SCL;
}

XBinary::MODE XSCLArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSCLArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSCLArchive::getArch()
{
    return QString();
}

qint32 XSCLArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XSCLArchive::getFileFormatExt()
{
    return QStringLiteral("scl");
}

QString XSCLArchive::getFileFormatExtsString()
{
    return QStringLiteral("SCL (*.scl)");
}

QString XSCLArchive::getMIMEString()
{
    return QStringLiteral("application/x-spectrum-scl");
}

QString XSCLArchive::getVersion()
{
    return QString();
}

qint64 XSCLArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSCLArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XSCLArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XSCLArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSCLArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SCL_HEADER_SIZE + ((qint64)context.listMembers.size() * SCL_ENTRY_SIZE);
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Catalogue");
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
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize + SCL_PREFIX_SIZE);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SCL_SECTORS);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored sectors"));
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
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

QMap<XBinary::UNPACK_PROP, QVariant> XSCLArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSCLArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSCLArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XSCLArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize + SCL_PREFIX_SIZE);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SCL_SECTORS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored sectors"));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XSCLArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSCLArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XSCLArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_COMPRESSPROPERTIES;
}
