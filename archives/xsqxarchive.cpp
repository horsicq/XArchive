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
#include "xsqxarchive.h"

#include "Algos/xsqxdecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 SQX_MIN_SIZE = 25;
const qint64 SQX_FIRST_HEADER = 25;
const qint64 SQX_HEADER_SIZE = 7;
const qint64 SQX_BODY_MIN = 26;
const qint32 SQX_MAX_MEMBERS = 200000;
const qint32 SQX_MAX_NAME_SIZE = 4096;

const quint16 SQX_MAIN_FLAG_ENCRYPTED = 0x10;
const quint16 SQX_FLAG_ENCRYPTED = 0x0008;
const quint16 SQX_FLAG_LARGE = 0x0080;
const quint16 SQX_FLAG_EXTENSION = 0x8000;

const quint8 SQX_TYPE_FILE = 0x44;  // 'D'
const quint8 SQX_TYPE_END_A = 0x41;
const quint8 SQX_TYPE_END_S = 0x53;
const quint8 SQX_TYPE_END_X = 0x58;

bool sqxIsEndType(quint8 nType)
{
    return (nType == SQX_TYPE_END_A) || (nType == SQX_TYPE_END_S) || (nType == SQX_TYPE_END_X);
}

bool sqxRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XSQXArchive::XSQXArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSQXArchive::~XSQXArchive()
{
}

bool XSQXArchive::parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSQXArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SQX_MIN_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, SQX_FIRST_HEADER, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SQX_FIRST_HEADER)) return false;
    const quint8 *pHeader = (const quint8 *)baHeader.constData();

    if (pHeader[2] != (quint8)'R') return false;
    if (qFromLittleEndian<quint16>(pHeader + 5) != 0x19) return false;
    if (memcmp(pHeader + 7, "-sqx-", 5) != 0) return false;

    const quint16 nMainFlags = qFromLittleEndian<quint16>(pHeader + 3);
    // Encrypted headers: nothing at all can be listed.
    if (nMainFlags & SQX_MAIN_FLAG_ENCRYPTED) return false;

    context.nArchiveSize = context.nInputSize;

    if (!bFull) {
        *pContext = context;
        return true;
    }

    qint64 nOffset = SQX_FIRST_HEADER;
    const qint64 nSize = context.nInputSize;
    context.baMemberTable = XSQXDecoder::startTable();

    while ((nOffset + SQX_HEADER_SIZE) <= nSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= SQX_MAX_MEMBERS) break;

        const QByteArray baRecord = read_array_process(nOffset, SQX_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != SQX_HEADER_SIZE)) return false;
        const quint8 *pRecord = (const quint8 *)baRecord.constData();
        const quint8 nType = pRecord[2];
        const quint16 nFlags = qFromLittleEndian<quint16>(pRecord + 3);
        const qint64 nHeaderSize = (qint64)qFromLittleEndian<quint16>(pRecord + 5);

        if (sqxIsEndType(nType)) break;
        if ((nHeaderSize < SQX_HEADER_SIZE) || ((nOffset + nHeaderSize) > nSize)) break;

        if (nType != SQX_TYPE_FILE) {
            nOffset += nHeaderSize;
            quint16 nChainFlags = nFlags;
            while (nChainFlags & SQX_FLAG_EXTENSION) {
                if ((nOffset + SQX_HEADER_SIZE) > nSize) break;
                const QByteArray baChain = read_array_process(nOffset, SQX_HEADER_SIZE, pPdStruct);
                if (!guardedThis || !guardedSource || (baChain.size() != SQX_HEADER_SIZE)) return false;
                const quint8 *pChain = (const quint8 *)baChain.constData();
                const qint64 nChainSize = (qint64)qFromLittleEndian<quint16>(pChain + 5);
                if (nChainSize < SQX_HEADER_SIZE) break;
                nOffset += nChainSize;
                nChainFlags = qFromLittleEndian<quint16>(pChain + 3);
            }
            continue;
        }

        if (nFlags & SQX_FLAG_ENCRYPTED) break;

        const qint64 nBodySize = nHeaderSize - SQX_HEADER_SIZE;
        if (nBodySize < SQX_BODY_MIN) break;
        const QByteArray baBody = read_array_process(nOffset + SQX_HEADER_SIZE, nBodySize, pPdStruct);
        if (!guardedThis || !guardedSource || (baBody.size() != nBodySize)) return false;
        const quint8 *pBody = (const quint8 *)baBody.constData();

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nHeaderSize;
        member.nFlags = nFlags;
        member.nFilter = pBody[0];
        member.nMethod = pBody[5];
        member.nCRC = qFromLittleEndian<quint32>(pBody + 6);
        member.nAttributes = qFromLittleEndian<quint32>(pBody + 10);
        member.nTime = qFromLittleEndian<quint32>(pBody + 14);
        qint64 nPacked = (qint64)qFromLittleEndian<quint32>(pBody + 18);
        qint64 nUnpacked = (qint64)qFromLittleEndian<quint32>(pBody + 22);

        qint64 nCursor = 26;
        if (nFlags & SQX_FLAG_LARGE) {
            if ((nCursor + 8) > nBodySize) break;
            nPacked |= ((qint64)qFromLittleEndian<quint32>(pBody + 26)) << 32;
            nUnpacked |= ((qint64)qFromLittleEndian<quint32>(pBody + 30)) << 32;
            nCursor = 34;
        }
        if ((nCursor + 2) > nBodySize) break;
        const qint32 nNameSize = (qint32)qFromLittleEndian<quint16>(pBody + nCursor);
        nCursor += 2;
        qint32 nNameAvailable = (qint32)qMin<qint64>(nNameSize, nBodySize - nCursor);
        if (nNameAvailable < 0) nNameAvailable = 0;
        if (nNameAvailable > SQX_MAX_NAME_SIZE) nNameAvailable = SQX_MAX_NAME_SIZE;
        QString sName = QString::fromLatin1(baBody.constData() + nCursor, nNameAvailable);
        sName.replace(QChar('\\'), QChar('/'));
        if (sName.isEmpty()) sName = QStringLiteral("MEMBER%1").arg(context.listMembers.size());

        nOffset += nHeaderSize;
        quint16 nChainFlags = nFlags;
        while (nChainFlags & SQX_FLAG_EXTENSION) {
            if ((nOffset + SQX_HEADER_SIZE) > nSize) break;
            const QByteArray baChain = read_array_process(nOffset, SQX_HEADER_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource || (baChain.size() != SQX_HEADER_SIZE)) return false;
            const quint8 *pChain = (const quint8 *)baChain.constData();
            const qint64 nChainSize = (qint64)qFromLittleEndian<quint16>(pChain + 5);
            if (nChainSize < SQX_HEADER_SIZE) break;
            nOffset += nChainSize;
            nChainFlags = qFromLittleEndian<quint16>(pChain + 3);
        }

        if ((nPacked < 0) || (nUnpacked < 0)) break;
        member.nDataOffset = nOffset;
        member.nCompressedSize = nPacked;
        member.nUncompressedSize = nUnpacked;
        member.bIsFolder = ((member.nAttributes & 0x10) != 0);
        member.sFileName = sName;
        member.nTableIndex = -1;
        if (!sqxRangeWithin(nSize, nOffset, nPacked)) {
            if (nOffset > nSize) break;
            member.nCompressedSize = nSize - nOffset;
        }
        // Only coded members enter the replay table; a stored member never
        // touches the LZ window.
        if ((!member.bIsFolder) && (member.nMethod >= 1) && (member.nMethod <= 4)) {
            member.nTableIndex = (qint32)((context.baMemberTable.size() - XSQXDecoder::TABLE_HEADER_SIZE) / XSQXDecoder::ENTRY_SIZE);
            if (!XSQXDecoder::appendEntry(&context.baMemberTable, member.nDataOffset, member.nCompressedSize, member.nUncompressedSize, member.nFlags,
                                          member.nFilter, member.nMethod)) {
                member.nTableIndex = -1;
            }
        }
        context.listMembers.append(member);
        nOffset += member.nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin<qint64>(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XSQXArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XSQXArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSQXArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSQXArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSQXArchive(pDevice);
}

QList<QString> XSQXArchive::getSearchSignatures()
{
    // bytes 0..1 free, byte 2 'R', bytes 3..6 free, bytes 7..11 "-sqx-"
    return QList<QString>() << QStringLiteral("....'R'........'-sqx-'");
}

XBinary::FT XSQXArchive::getFileType()
{
    return FT_SQX;
}

XBinary::MODE XSQXArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSQXArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSQXArchive::getArch()
{
    return QString();
}

qint32 XSQXArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XSQXArchive::getFileFormatExt()
{
    return QStringLiteral("sqx");
}

QString XSQXArchive::getFileFormatExtsString()
{
    return QStringLiteral("SQX (*.sqx)");
}

QString XSQXArchive::getMIMEString()
{
    return QStringLiteral("application/x-sqx");
}

QString XSQXArchive::getVersion()
{
    return QString();
}

qint64 XSQXArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, true, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSQXArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XSQXArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XSQXArchive::methodToString(quint8 nMethod)
{
    if (nMethod == 0) return QStringLiteral("Stored");
    if ((nMethod >= 1) && (nMethod <= 4)) return QStringLiteral("SQX %1").arg(nMethod);
    return QStringLiteral("SQX %1 (unsupported)").arg(nMethod);
}

XBinary::HANDLE_METHOD XSQXArchive::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == 0) return HANDLE_METHOD_STORE;
    if ((nMethod >= 1) && (nMethod <= 4)) return HANDLE_METHOD_SQX;
    // Methods 5 and up are a different coder; fail cleanly.
    return HANDLE_METHOD_UNKNOWN;
}

bool XSQXArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSQXArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SQX_FIRST_HEADER;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (member.bIsFolder) continue;

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            const bool bCoded = (member.nTableIndex >= 0);
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            // A coded member needs the whole archive: its solid window is
            // rebuilt from the members in front of it, which are not adjacent
            // to it in the file.
            part.nFileOffset = bCoded ? 0 : member.nDataOffset;
            part.nFileSize = bCoded ? context.nArchiveSize : member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nMethod == 0) ? HANDLE_METHOD_STORE : (bCoded ? HANDLE_METHOD_SQX : HANDLE_METHOD_UNKNOWN));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
            if (bCoded) part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XSQXDecoder::finishTable(context.baMemberTable, member.nTableIndex));
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, member.nCRC);
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

QMap<XBinary::UNPACK_PROP, QVariant> XSQXArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSQXArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSQXArchive> guardedThis(this);
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
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
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

XBinary::ARCHIVERECORD XSQXArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

    const bool bCoded = (member.nTableIndex >= 0);
    // A coded member is decoded from the whole archive, because the solid
    // window in front of it lives in members that are not adjacent to it.
    result.nStreamOffset = bCoded ? 0 : member.nDataOffset;
    result.nStreamSize = bCoded ? pContext->nArchiveSize : member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nMethod == 0) ? HANDLE_METHOD_STORE : (bCoded ? HANDLE_METHOD_SQX : HANDLE_METHOD_UNKNOWN));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    if (bCoded) result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XSQXDecoder::finishTable(pContext->baMemberTable, member.nTableIndex));
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, member.nCRC);

    return result;
}

bool XSQXArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSQXArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XSQXArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_UNCOMPRESSEDCRC << FPART_PROP_ISFOLDER;
}
