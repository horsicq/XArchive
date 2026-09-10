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
#include "xainarchive.h"

#include "Algos/xaindecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 AIN_HEADER_SIZE = 0x18;
const qint32 AIN_RECORD_SIZE = 0x1d;
const qint32 AIN_MAX_RECORD = 0x1d + 0x402;
const qint32 AIN_MAX_DIRECTORY = 16 * 1024 * 1024;
const qint32 AIN_MAX_MEMBERS = 65535;
const qint32 AIN_MAX_NAME_SIZE = 0x400;

const quint8 AIN_FLAG_NEWGROUP = 0x10;
const quint8 AIN_FLAG_ENDGROUP = 0x08;
const quint8 AIN_METHOD_STORED = 4;

bool ainRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XAINArchive::XAINArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAINArchive::~XAINArchive()
{
}

bool XAINArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XAINArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < AIN_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, AIN_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != AIN_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (pHeader[0] != '!') return false;

    const quint8 nMethod = (quint8)(pHeader[1] & 0x0f);
    const quint8 nVersion = (quint8)(pHeader[1] >> 4);
    if ((nMethod < 1) || (nMethod > 4)) return false;
    if ((nVersion < 1) || (nVersion > 3)) return false;
    const qint16 nEncrypted = (qint16)qFromLittleEndian<quint16>(pHeader + 2);
    if ((nEncrypted != 0) && (nEncrypted != (qint16)0x8000)) return false;
    const qint32 nCount = (qint32)qFromLittleEndian<quint16>(pHeader + 8);
    const qint64 nDirectoryOffset = (qint32)qFromLittleEndian<quint32>(pHeader + 0x0e);
    if (nDirectoryOffset <= 0x17) return false;
    quint32 nSum = 0;
    for (qint32 i = 0; i < 0x16; ++i) nSum += pHeader[i];
    if ((nSum & 0xffff) != (quint32)(qFromLittleEndian<quint16>(pHeader + 0x16) ^ 0x5555)) return false;
    if ((nCount <= 0) || (nCount > AIN_MAX_MEMBERS)) return false;
    if (!ainRangeWithin(context.nInputSize, nDirectoryOffset, 1)) return false;

    context.bEncrypted = (nEncrypted != 0);
    context.bStored = (nMethod == 4);
    context.nMethod = nMethod;

    // the directory has to be decoded in one go before any member is known
    const QByteArray baDirectoryStream = read_array_process(nDirectoryOffset, context.nInputSize - nDirectoryOffset, pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    // the directory ends on its own, so ask for an upper bound and accept the
    // short read that comes back
    const qint64 nBudget = qMin((qint64)nCount * AIN_MAX_RECORD + 0x1000, (qint64)AIN_MAX_DIRECTORY);
    QByteArray baDirectory;
    XAINDecoder::decode(baDirectoryStream, 0, nBudget, &baDirectory, pPdStruct);
    if (!guardedThis || !guardedSource || baDirectory.isEmpty()) return false;

    qint64 nGroupOffset = -1;
    qint64 nGroupSkip = 0;
    qint32 nGroupFirst = 0;
    qint32 nCursor = 0;
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nCursor + AIN_RECORD_SIZE) > baDirectory.size()) break;
        const uchar *pRecord = (const uchar *)baDirectory.constData() + nCursor;
        nCursor += AIN_RECORD_SIZE;

        QByteArray baName;
        while (nCursor < baDirectory.size()) {
            const char cChar = baDirectory.at(nCursor);
            ++nCursor;
            if (cChar == (char)0) break;
            baName.append(cChar);
            if (baName.size() > AIN_MAX_NAME_SIZE) return false;
        }
        if (baName.isEmpty() || (nCursor >= baDirectory.size())) break;
        if (baDirectory.at(nCursor) != (char)0) break;  // the comment must be empty
        ++nCursor;

        const qint64 nOriginal = (qint32)qFromLittleEndian<quint32>(pRecord + 5);
        const qint64 nPacked = (qint32)qFromLittleEndian<quint32>(pRecord + 9);
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pRecord + 0x0d);
        const quint8 nFlags = pRecord[0x16];
        if ((nOriginal < 0) || (nPacked < 0) || (nDataOffset < 0)) break;
        if ((nFlags & 0xe7) != 0) break;

        if (nFlags & AIN_FLAG_NEWGROUP) {
            nGroupOffset = nDataOffset;
            nGroupSkip = 0;
            nGroupFirst = context.listMembers.size();
        }
        if ((nGroupOffset < 0) || (nGroupOffset > context.nInputSize)) break;

        MEMBER member = {};
        // the headers live inside a compressed stream, so a member is
        // identified by its slot in the directory region rather than by a real
        // header offset
        member.nHeaderOffset = nDirectoryOffset + i;
        member.nHeaderSize = 0;
        member.nDataOffset = nGroupOffset;
        member.nCompressedSize = context.nInputSize - nGroupOffset;
        member.nSkipSize = nGroupSkip;
        member.nUncompressedSize = nOriginal;
        member.nCRC = 0;
        member.nTime = qFromLittleEndian<quint16>(pRecord + 1) | ((quint32)qFromLittleEndian<quint16>(pRecord + 3) << 16);
        member.nMethod = nMethod;
        member.bIsFolder = false;
        member.sFileName = QString::fromLatin1(baName);
        if (context.bStored) {
            member.nDataOffset = nGroupOffset + nGroupSkip;
            member.nCompressedSize = nOriginal;
            member.nSkipSize = 0;
            if (!ainRangeWithin(context.nInputSize, member.nDataOffset, nOriginal)) {
                if (member.nDataOffset > context.nInputSize) break;
                member.nCompressedSize = context.nInputSize - member.nDataOffset;
            }
        }
        context.listMembers.append(member);
        nGroupSkip += nOriginal;

        // the member that closes a group is the only one that records the
        // group's compressed size, so the range is back-filled here
        if ((nFlags & AIN_FLAG_ENDGROUP) && !context.bStored) {
            qint64 nGroupSize = nPacked;
            if (!ainRangeWithin(context.nInputSize, nGroupOffset, nGroupSize)) nGroupSize = context.nInputSize - nGroupOffset;
            for (qint32 k = nGroupFirst; k < context.listMembers.size(); ++k) {
                context.listMembers[k].nCompressedSize = nGroupSize;
            }
            nGroupOffset = -1;
        }
    }

    qint64 nOffset = context.nInputSize;

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XAINArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XAINArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAINArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAINArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAINArchive(pDevice);
}

QList<QString> XAINArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'!'");
}

XBinary::FT XAINArchive::getFileType()
{
    return FT_AIN;
}

XBinary::MODE XAINArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAINArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XAINArchive::getArch()
{
    return QString();
}

qint32 XAINArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XAINArchive::getFileFormatExt()
{
    return QStringLiteral("ain");
}

QString XAINArchive::getFileFormatExtsString()
{
    return QStringLiteral("AIN (*.ain)");
}

QString XAINArchive::getMIMEString()
{
    return QStringLiteral("application/x-ain");
}

QString XAINArchive::getVersion()
{
    return QString();
}

qint64 XAINArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAINArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XAINArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XAINArchive::methodToString(quint8 nMethod)
{
    if (nMethod == AIN_METHOD_STORED) return QStringLiteral("Stored");
    return QStringLiteral("AIN %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XAINArchive::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == AIN_METHOD_STORED) return HANDLE_METHOD_STORE;
    return HANDLE_METHOD_AIN;
}

// the leading amount to discard, which is what makes a solid group work
// through the whole-buffer codec path
QByteArray XAINArchive::skipToProperty(qint64 nSkipSize)
{
    QByteArray baResult(8, (char)0);
    qToLittleEndian<quint64>((quint64)nSkipSize, (uchar *)baResult.data());
    return baResult;
}

bool XAINArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XAINArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = AIN_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (member.bIsFolder) continue;

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, context.bEncrypted ? HANDLE_METHOD_UNKNOWN : methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, skipToProperty(member.nSkipSize));
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

QMap<XBinary::UNPACK_PROP, QVariant> XAINArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAINArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XAINArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XAINArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);

    if (member.bIsFolder) {
        result.nStreamOffset = 0;
        result.nStreamSize = 0;
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Directory"));
        return result;
    }

    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->bEncrypted ? HANDLE_METHOD_UNKNOWN : methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, skipToProperty(member.nSkipSize));

    return result;
}

bool XAINArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XAINArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XAINArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
