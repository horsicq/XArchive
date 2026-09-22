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
#include "xzpakarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 ZPAK_HEADER_SIZE = 6;
const qint64 ZPAK_RECORD_SIZE = 33;
const qint64 ZPAK_NAME_SIZE = 12;
const qint64 ZPAK_MIN_SIZE = 0x27;
const qint32 ZPAK_MAX_MEMBERS = 65535;

bool zpakRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XZPAKArchive::XZPAKArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZPAKArchive::~XZPAKArchive()
{
}

bool XZPAKArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ZPAK_MIN_SIZE) return false;

    const QByteArray baProbe = read_array_process(0, ZPAK_MIN_SIZE, pPdStruct);
    if (!guardedSource || (baProbe.size() != ZPAK_MIN_SIZE)) return false;

    const QByteArray baMagic = baProbe.left(4);
    const bool bIsV1 = (baMagic == QByteArray("zpak", 4));
    const bool bIsV2 = (baMagic == QByteArray("zpk2", 4));
    if (!bIsV1 && !bIsV2) return false;

    const uchar *pProbe = (const uchar *)baProbe.constData();
    const qint32 nCount = (qint32)qFromLittleEndian<quint16>(pProbe + 4);
    if ((nCount == 0) || (nCount > ZPAK_MAX_MEMBERS)) return false;
    // The reference detector's own three checks on the first directory entry.
    if ((qint32)qFromLittleEndian<quint32>(pProbe + 0x13) <= 0x26) return false;
    if ((qint32)qFromLittleEndian<quint32>(pProbe + 0x17) < 0) return false;
    if ((qint32)qFromLittleEndian<quint32>(pProbe + 0x23) < 0) return false;

    const qint64 nDirectorySize = (qint64)nCount * ZPAK_RECORD_SIZE;
    if (!zpakRangeWithin(context.nInputSize, ZPAK_HEADER_SIZE, nDirectorySize)) return false;

    const QByteArray baDirectory = read_array_process(ZPAK_HEADER_SIZE, nDirectorySize, pPdStruct);
    if (!guardedSource || (baDirectory.size() != nDirectorySize)) return false;

    context.bIsV2 = bIsV2;
    const uchar *pDirectory = (const uchar *)baDirectory.constData();
    qint64 nArchiveEnd = ZPAK_HEADER_SIZE + nDirectorySize;

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecord = (qint64)i * ZPAK_RECORD_SIZE;

        // Only twelve name characters are significant; byte 12 is padding.
        QByteArray baName = baDirectory.mid((qint32)nRecord, (qint32)ZPAK_NAME_SIZE);
        const qint32 nZero = baName.indexOf((char)0);
        if (nZero >= 0) baName.truncate(nZero);

        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pDirectory + nRecord + 0x0d);
        const qint64 nCompressedSize = (qint32)qFromLittleEndian<quint32>(pDirectory + nRecord + 0x11);
        const quint16 nDate = qFromLittleEndian<quint16>(pDirectory + nRecord + 0x15);
        const quint16 nTime = qFromLittleEndian<quint16>(pDirectory + nRecord + 0x17);
        const quint16 nCRCLow = qFromLittleEndian<quint16>(pDirectory + nRecord + 0x19);
        const quint16 nCRCHigh = qFromLittleEndian<quint16>(pDirectory + nRecord + 0x1b);
        const qint64 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pDirectory + nRecord + 0x1d);

        if ((nDataOffset < 0) || (nCompressedSize < 0) || (nUncompressedSize < 0)) return false;
        if (!zpakRangeWithin(context.nInputSize, nDataOffset, nCompressedSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = ZPAK_HEADER_SIZE + nRecord;
        member.nHeaderSize = ZPAK_RECORD_SIZE;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nCRC = ((quint32)nCRCHigh << 16) | (quint32)nCRCLow;
        // A zero high word means the reference reader does not check the CRC.
        member.bHasCRC = (nCRCHigh != 0);
        member.nDate = nDate;
        member.nTime = nTime;
        member.sFileName = QString::fromLatin1(baName);
        context.listMembers.append(member);

        if ((nDataOffset + nCompressedSize) > nArchiveEnd) nArchiveEnd = nDataOffset + nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nArchiveEnd, context.nInputSize);
    *pContext = context;

    return true;
}

bool XZPAKArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZPAKArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZPAKArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZPAKArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZPAKArchive(pDevice);
}

QList<QString> XZPAKArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'zpak'") << QStringLiteral("'zpk2'");
}

XBinary::FT XZPAKArchive::getFileType()
{
    return FT_ZPAK;
}

XBinary::MODE XZPAKArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZPAKArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZPAKArchive::getArch()
{
    return QString();
}

qint32 XZPAKArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZPAKArchive::getFileFormatExt()
{
    return QStringLiteral("zpk");
}

QString XZPAKArchive::getFileFormatExtsString()
{
    return QStringLiteral("ZPAK (*.zpk)");
}

QString XZPAKArchive::getMIMEString()
{
    return QStringLiteral("application/x-zpak");
}

QString XZPAKArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();

    return context.bIsV2 ? QStringLiteral("2") : QStringLiteral("1");
}

qint64 XZPAKArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XZPAKArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZPAKArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XZPAKArchive::methodToString(bool bIsV2)
{
    return bIsV2 ? QStringLiteral("PKWARE DCL") : QStringLiteral("ZPAK LZW");
}

XBinary::HANDLE_METHOD XZPAKArchive::methodToHandleMethod(bool bIsV2)
{
    // zpk2 is plain PKWARE DCL implode, so it goes through the method the
    // tree already dispatches rather than a ZPAK-specific one - a private
    // enumerator here has no arm in xdecompress and silently drops every
    // member of every v2 archive.
    return bIsV2 ? HANDLE_METHOD_PKWARE_DCL_IMPLODE : HANDLE_METHOD_ZPAK_LZW;
}

bool XZPAKArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZPAKArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ZPAK_HEADER_SIZE + ((qint64)context.listMembers.size() * ZPAK_RECORD_SIZE);
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
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(context.bIsV2));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(context.bIsV2));
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

QMap<XBinary::UNPACK_PROP, QVariant> XZPAKArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZPAKArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XZPAKArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(pContext->bIsV2));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(pContext->bIsV2));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZPAKArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XZPAKArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZPAKArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
