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
#include "xztcarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 ZTC_HEADER_SIZE = 10;
const qint64 ZTC_RECORD_SIZE = 0x12;
const qint64 ZTC_RECORD_OVERHEAD = 0x16;  // record + trailing checksum
const quint32 ZTC_MAGIC = 0x01abbbd6;
const qint32 ZTC_STAMP = 0x00003bc4;
const qint32 ZTC_MAX_MEMBERS = 100000;
const qint64 ZTC_MAX_NAME_SIZE = 4096;

bool ztcRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

QString ztcNormalizeName(const QByteArray &baName)
{
    QByteArray baTrimmed = baName;
    const qint32 nZero = baTrimmed.indexOf((char)0);
    if (nZero >= 0) baTrimmed.truncate(nZero);
    QString sResult = QString::fromLatin1(baTrimmed);
    sResult.replace(QChar('\\'), QChar('/'));

    return sResult;
}

}  // namespace

XZTCArchive::XZTCArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZTCArchive::~XZTCArchive()
{
}

bool XZTCArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (ZTC_HEADER_SIZE + ZTC_RECORD_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, ZTC_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != ZTC_HEADER_SIZE)) return false;
    if (qFromLittleEndian<quint32>((const uchar *)baHeader.constData()) != ZTC_MAGIC) return false;
    context.nVolume = qFromLittleEndian<quint16>((const uchar *)baHeader.constData() + 8);

    qint64 nOffset = ZTC_HEADER_SIZE;

    while ((nOffset + ZTC_RECORD_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= ZTC_MAX_MEMBERS) return false;

        const QByteArray baRecord = read_array_process(nOffset, ZTC_RECORD_SIZE, pPdStruct);
        if (!guardedSource || (baRecord.size() != ZTC_RECORD_SIZE)) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();

        const qint64 nUncompressed = (qint32)qFromLittleEndian<quint32>(pRecord);
        const qint32 nStamp = (qint32)qFromLittleEndian<quint32>(pRecord + 4);
        const qint64 nTotalSize = (qint32)qFromLittleEndian<quint32>(pRecord + 8);
        const quint32 nTime = qFromLittleEndian<quint32>(pRecord + 0x0c);
        const qint64 nNameSize = (qint64)qFromLittleEndian<quint16>(pRecord + 0x10);

        if ((nUncompressed < 0) || (nNameSize == 0) || (nNameSize > ZTC_MAX_NAME_SIZE)) return false;
        if (nTotalSize < (nNameSize + ZTC_RECORD_OVERHEAD)) return false;
        if (context.listMembers.isEmpty() && (nStamp != ZTC_STAMP)) return false;
        if (!ztcRangeWithin(context.nInputSize, nOffset, nNameSize + ZTC_RECORD_OVERHEAD)) return false;

        const QByteArray baName = read_array_process(nOffset + ZTC_RECORD_SIZE, nNameSize, pPdStruct);
        if (!guardedSource || (baName.size() != nNameSize)) return false;

        const QByteArray baCheck = read_array_process(nOffset + ZTC_RECORD_SIZE + nNameSize, 4, pPdStruct);
        if (!guardedSource || (baCheck.size() != 4)) return false;

        quint32 nWanted = (quint32)pRecord[0] + (quint32)pRecord[0x0c];
        for (qint32 i = 0; i < baName.size(); ++i) {
            nWanted += (quint32)(quint8)baName.at(i);
        }
        if (qFromLittleEndian<quint32>((const uchar *)baCheck.constData()) != nWanted) return false;

        const qint64 nDataOffset = nOffset + ZTC_RECORD_OVERHEAD + nNameSize;
        qint64 nCompressedSize = nTotalSize - nNameSize - ZTC_RECORD_OVERHEAD;
        if (!ztcRangeWithin(context.nInputSize, nDataOffset, nCompressedSize)) {
            if (nDataOffset > context.nInputSize) return false;
            nCompressedSize = context.nInputSize - nDataOffset;
        }

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = ZTC_RECORD_OVERHEAD + nNameSize;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressed;
        member.nTime = nTime;
        member.sFileName = ztcNormalizeName(baName);
        context.listMembers.append(member);

        nOffset += nTotalSize;
        // The archive closes with a two byte FF FF trailer.
        if ((nOffset + 2) == context.nInputSize) break;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XZTCArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZTCArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZTCArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZTCArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZTCArchive(pDevice);
}

QList<QString> XZTCArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("d6bbab01");
}

XBinary::FT XZTCArchive::getFileType()
{
    return FT_ZTC;
}

XBinary::MODE XZTCArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZTCArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZTCArchive::getArch()
{
    return QString();
}

qint32 XZTCArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZTCArchive::getFileFormatExt()
{
    return QStringLiteral("ztc");
}

QString XZTCArchive::getFileFormatExtsString()
{
    return QStringLiteral("ZTC (*.ztc)");
}

QString XZTCArchive::getMIMEString()
{
    return QStringLiteral("application/x-ztc");
}

QString XZTCArchive::getVersion()
{
    return QString();
}

qint64 XZTCArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XZTCArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZTCArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XZTCArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZTCArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ZTC_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZTC);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("ZTC LZHUF"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XZTCArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZTCArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XZTCArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZTC);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("ZTC LZHUF"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZTCArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XZTCArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZTCArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
