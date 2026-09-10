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
#include "xtrcpakarchive.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 TRCPAK_HEADER_SIZE = 0x1c;
const qint64 TRCPAK_ENTRY_SIZE = 0x10c;
const qint64 TRCPAK_NAME_SIZE = 0x104;

bool trcRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

QString trcName(const QByteArray &baField)
{
    const qint32 nZero = baField.indexOf((char)0);
    return QString::fromLatin1((nZero >= 0) ? baField.left(nZero) : baField);
}

}  // namespace

XTRCPAKArchive::XTRCPAKArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTRCPAKArchive::~XTRCPAKArchive()
{
}

bool XTRCPAKArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XTRCPAKArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < TRCPAK_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, TRCPAK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != TRCPAK_HEADER_SIZE)) return false;
    if (baHeader.left(7) != QByteArray("TRCPAK\x00", 7)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    context.nVersionRaw = qFromLittleEndian<quint32>(pHeader + 8);
    const qint32 nCount = (qint32)qFromLittleEndian<quint32>(pHeader + 12);
    if (nCount <= 0) return false;
    if ((qint64)nCount > ((context.nInputSize - TRCPAK_HEADER_SIZE) / TRCPAK_ENTRY_SIZE)) return false;

    context.nDirectorySize = (qint64)nCount * TRCPAK_ENTRY_SIZE;
    qint64 nArchiveEnd = TRCPAK_HEADER_SIZE + context.nDirectorySize;

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nEntryOffset = TRCPAK_HEADER_SIZE + ((qint64)i * TRCPAK_ENTRY_SIZE);
        const QByteArray baEntry = read_array_process(nEntryOffset, TRCPAK_ENTRY_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baEntry.size() != TRCPAK_ENTRY_SIZE)) return false;
        const uchar *pEntry = (const uchar *)baEntry.constData();

        // Size first, offset second - see the header comment.
        const qint64 nDataSize = (qint32)qFromLittleEndian<quint32>(pEntry + 0x104);
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pEntry + 0x108);
        if ((nDataSize < 0) || (nDataOffset < 0)) return false;
        if (!trcRangeWithin(context.nInputSize, nDataOffset, nDataSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nEntryOffset;
        member.nDataOffset = nDataOffset;
        member.nDataSize = nDataSize;
        member.sFileName = trcName(baEntry.left((qint32)TRCPAK_NAME_SIZE));

        if ((nDataOffset + nDataSize) > nArchiveEnd) nArchiveEnd = nDataOffset + nDataSize;
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = (nArchiveEnd < context.nInputSize) ? nArchiveEnd : context.nInputSize;
    *pContext = context;

    return true;
}

bool XTRCPAKArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTRCPAKArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTRCPAKArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTRCPAKArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTRCPAKArchive(pDevice);
}

QList<QString> XTRCPAKArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'TRCPAK'00");
}

XBinary::FT XTRCPAKArchive::getFileType()
{
    return FT_TRCPAK;
}

XBinary::MODE XTRCPAKArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTRCPAKArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTRCPAKArchive::getArch()
{
    return QString();
}

qint32 XTRCPAKArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTRCPAKArchive::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XTRCPAKArchive::getFileFormatExtsString()
{
    return QStringLiteral("TRCPAK (*.pak)");
}

QString XTRCPAKArchive::getMIMEString()
{
    return QStringLiteral("application/x-trcpak");
}

QString XTRCPAKArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();

    // The field is an IEEE-754 float, not a packed BCD version.
    float fVersion = 0;
    memcpy(&fVersion, &context.nVersionRaw, sizeof(fVersion));
    if ((fVersion <= 0) || (fVersion > 1000)) return QString();

    return QString::number((double)fVersion, 'f', 1);
}

qint64 XTRCPAKArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XTRCPAKArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTRCPAKArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XTRCPAKArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTRCPAKArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TRCPAK_HEADER_SIZE + context.nDirectorySize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XTRCPAKArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTRCPAKArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTRCPAKArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XTRCPAKArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XTRCPAKArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XTRCPAKArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTRCPAKArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
