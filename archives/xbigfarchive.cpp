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
#include "xbigfarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 BIGF_HEADER_SIZE = 16;
const qint64 BIGF_ENTRY_FIXED = 8;
const quint32 BIGF_SENTINEL = 0xcdcdcdcdU;
const qint32 BIGF_MAX_MEMBERS = 100000;
const qint32 BIGF_MAX_NAME_SIZE = 4096;

bool bigfRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// The name is a fixed-length field, not a C string; only control bytes make it
// implausible.  Trailing NULs do occur and are trimmed.
bool bigfIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = (quint8)baName.at(i);
        if ((nCharacter < 0x20) && (nCharacter != 0)) return false;
    }
    return true;
}
}  // namespace

XBigfArchive::XBigfArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBigfArchive::~XBigfArchive()
{
}

bool XBigfArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XBigfArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BIGF_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, BIGF_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != BIGF_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("BIGF")) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    const qint64 nCount = (qint32)qFromBigEndian<quint32>(pHeader + 8);
    if ((nCount <= 0) || (nCount > BIGF_MAX_MEMBERS)) return false;

    qint64 nOffset = BIGF_HEADER_SIZE;
    for (qint64 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!bigfRangeWithin(context.nInputSize, nOffset, BIGF_ENTRY_FIXED)) return false;
        const QByteArray baEntry = read_array_process(nOffset, BIGF_ENTRY_FIXED, pPdStruct);
        if (!guardedThis || !guardedSource || (baEntry.size() != BIGF_ENTRY_FIXED)) return false;
        const uchar *pEntry = (const uchar *)baEntry.constData();
        nOffset += BIGF_ENTRY_FIXED;

        // A trailing 0xCDCDCDCD pair stands in for the last entry in archives
        // written by some tools; it is padding, not a member.
        if ((i == (nCount - 1)) && (qFromLittleEndian<quint32>(pEntry) == BIGF_SENTINEL) &&
            (qFromLittleEndian<quint32>(pEntry + 4) == BIGF_SENTINEL)) {
            break;
        }

        const qint64 nDataOffset = (qint32)qFromBigEndian<quint32>(pEntry);
        const qint64 nDataSize = (qint32)qFromBigEndian<quint32>(pEntry + 4);
        if ((nDataOffset < 0) || (nDataSize < 0)) return false;

        QByteArray baName;
        while (baName.size() <= BIGF_MAX_NAME_SIZE) {
            if (nOffset >= context.nInputSize) return false;
            const QByteArray baByte = read_array_process(nOffset, 1, pPdStruct);
            if (!guardedThis || !guardedSource || (baByte.size() != 1)) return false;
            ++nOffset;
            if (baByte.at(0) == (char)0) break;
            baName.append(baByte.at(0));
        }
        if (!bigfIsValidName(baName)) return false;
        if (!bigfRangeWithin(context.nInputSize, nDataOffset, nDataSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = 0;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nDataSize;
        member.nUncompressedSize = nDataSize;
        member.sFileName = QString::fromLatin1(baName);
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return true;
}

bool XBigfArchive::isValid(PDSTRUCT *pPdStruct)
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

bool XBigfArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBigfArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBigfArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBigfArchive(pDevice);
}

QList<QString> XBigfArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'BIGF'");
}

XBinary::FT XBigfArchive::getFileType()
{
    return FT_EA_BIG;
}

XBinary::MODE XBigfArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBigfArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBigfArchive::getArch()
{
    return QString();
}

qint32 XBigfArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XBigfArchive::getFileFormatExt()
{
    return QStringLiteral("big");
}

QString XBigfArchive::getFileFormatExtsString()
{
    return QStringLiteral("EA BIG (*.big *.viv)");
}

QString XBigfArchive::getMIMEString()
{
    return QStringLiteral("application/x-ea-big");
}

QString XBigfArchive::getVersion()
{
    return QString();
}

qint64 XBigfArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBigfArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XBigfArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XBigfArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBigfArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

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

QMap<XBinary::UNPACK_PROP, QVariant> XBigfArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBigfArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XBigfArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XBigfArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XBigfArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XBigfArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XBigfArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
