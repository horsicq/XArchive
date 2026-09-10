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
#include "xclayarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 CLAY_HEADER_SIZE = 20;
const qint32 CLAY_MAX_MEMBERS = 100000;
const qint32 CLAY_MAX_NAME_SIZE = 4096;

bool clayRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// The name is a fixed-length field, not a C string; only control bytes make it
// implausible.  Trailing NULs do occur and are trimmed.
bool clayIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = (quint8)baName.at(i);
        if ((nCharacter < 0x20) && (nCharacter != 0)) return false;
    }
    return true;
}
}  // namespace

XClayArchive::XClayArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XClayArchive::~XClayArchive()
{
}

bool XClayArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XClayArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < CLAY_HEADER_SIZE) return false;

    qint64 nOffset = 0;
    while ((context.listMembers.size() < CLAY_MAX_MEMBERS) && isPdStructNotCanceled(pPdStruct)) {
        if (!clayRangeWithin(context.nInputSize, nOffset, CLAY_HEADER_SIZE)) break;

        const QByteArray baHeader = read_array_process(nOffset, CLAY_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baHeader.size() != CLAY_HEADER_SIZE)) return false;
        const uchar *pHeader = (const uchar *)baHeader.constData();
        if (qFromLittleEndian<quint32>(pHeader) != 0x79616c43U) break;  // 'Clay'

        const qint32 nCompressedSize = (qint32)qFromLittleEndian<quint32>(pHeader + 4);
        const qint32 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pHeader + 8);
        const quint16 nDate = qFromLittleEndian<quint16>(pHeader + 12);
        const quint16 nTime = qFromLittleEndian<quint16>(pHeader + 14);
        const qint16 nNameSize = (qint16)qFromLittleEndian<quint16>(pHeader + 16);

        if ((nCompressedSize < 0) || (nUncompressedSize < 0)) break;
        if ((nNameSize <= 0) || (nNameSize > CLAY_MAX_NAME_SIZE)) break;
        if (!clayRangeWithin(context.nInputSize, nOffset + CLAY_HEADER_SIZE, nNameSize)) break;

        const QByteArray baName = read_array_process(nOffset + CLAY_HEADER_SIZE, nNameSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baName.size() != nNameSize)) return false;
        if (!clayIsValidName(baName)) break;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = CLAY_HEADER_SIZE + nNameSize;
        member.nDataOffset = nOffset + member.nHeaderSize;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nDate = nDate;
        member.nTime = nTime;
        member.sFileName = QString::fromLatin1(baName).remove(QChar('\0'));

        // A member whose data is cut off still lists; it just cannot be
        // decoded past the end of file, which the decoder reports on its own.
        if (!clayRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) {
            member.nCompressedSize = context.nInputSize - member.nDataOffset;
            context.listMembers.append(member);
            nOffset = context.nInputSize;
            break;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nOffset;
    *pContext = context;

    return true;
}

bool XClayArchive::isValid(PDSTRUCT *pPdStruct)
{
    // getRecords-style probing displaces the caller's cursor, so snapshot it.
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XClayArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XClayArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XClayArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XClayArchive(pDevice);
}

QList<QString> XClayArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'Clay'");
}

XBinary::FT XClayArchive::getFileType()
{
    return FT_CLAY;
}

XBinary::MODE XClayArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XClayArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XClayArchive::getArch()
{
    return QString();
}

qint32 XClayArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XClayArchive::getFileFormatExt()
{
    return QStringLiteral("cmz");
}

QString XClayArchive::getFileFormatExtsString()
{
    return QStringLiteral("Clay (*.cmz)");
}

QString XClayArchive::getMIMEString()
{
    return QStringLiteral("application/x-clay");
}

QString XClayArchive::getVersion()
{
    return QString();
}

qint64 XClayArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XClayArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XClayArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XClayArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XClayArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_CLAY_LZ);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Clay LZ"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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

QMap<XBinary::UNPACK_PROP, QVariant> XClayArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XClayArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XClayArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XClayArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_CLAY_LZ);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Clay LZ"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (isValidDosDateTime(member.nDate, member.nTime)) {
        const QDateTime dtModified = dosDateTimeToQDateTime(member.nDate, member.nTime);
        if (dtModified.isValid()) result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }

    return result;
}

bool XClayArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XClayArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XClayArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_MTIME;
}
