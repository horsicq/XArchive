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
#include "xhaarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 HA_HEADER_SIZE = 4;
const qint64 HA_MEMBER_HEADER = 17;
const qint32 HA_MAX_MEMBERS = 100000;
const qint32 HA_MAX_NAME_SIZE = 4096;

const quint8 HA_METHOD_STORED = 0;
const quint8 HA_METHOD_ASC = 1;
const quint8 HA_METHOD_HSC = 2;

bool haRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XHAArchive::XHAArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XHAArchive::~XHAArchive()
{
}

bool XHAArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < HA_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, HA_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != HA_HEADER_SIZE)) return false;
    if (baHeader.left(2) != QByteArray("HA")) return false;
    const qint32 nCount = (qint32)qFromLittleEndian<quint16>((const uchar *)baHeader.constData() + 2);
    if ((nCount <= 0) || (nCount > HA_MAX_MEMBERS)) return false;

    qint64 nOffset = HA_HEADER_SIZE;
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!haRangeWithin(context.nInputSize, nOffset, HA_MEMBER_HEADER)) break;
        const QByteArray baEntry = read_array_process(nOffset, HA_MEMBER_HEADER, pPdStruct);
        if ((baEntry.size() != HA_MEMBER_HEADER)) return false;
        const uchar *pEntry = (const uchar *)baEntry.constData();

        const quint8 nType = pEntry[0];
        const qint64 nPacked = (qint32)qFromLittleEndian<quint32>(pEntry + 1);
        const qint64 nOriginal = (qint32)qFromLittleEndian<quint32>(pEntry + 5);
        const quint32 nCRC = qFromLittleEndian<quint32>(pEntry + 9);
        const quint32 nTime = qFromLittleEndian<quint32>(pEntry + 13);
        if ((nPacked < 0) || (nOriginal < 0)) break;

        // two NUL-terminated strings then a machine byte, whose value is also
        // the length of the extended area that follows it
        QByteArray baPath;
        QByteArray baName;
        qint64 nCursor = nOffset + HA_MEMBER_HEADER;
        for (qint32 nWhich = 0; nWhich < 2; ++nWhich) {
            QByteArray baField;
            while (baField.size() <= HA_MAX_NAME_SIZE) {
                if (nCursor >= context.nInputSize) return false;
                const QByteArray baByte = read_array_process(nCursor, 1, pPdStruct);
                if ((baByte.size() != 1)) return false;
                ++nCursor;
                if (baByte.at(0) == (char)0) break;
                baField.append(baByte.at(0));
            }
            if (nWhich == 0) baPath = baField; else baName = baField;
        }
        if (nCursor >= context.nInputSize) return false;
        const QByteArray baMachine = read_array_process(nCursor, 1, pPdStruct);
        if ((baMachine.size() != 1)) return false;
        const qint32 nMachine = (quint8)baMachine.at(0);
        ++nCursor;

        const qint64 nHeaderSize = nMachine + baPath.size() + baName.size() + 20;
        const qint64 nDataOffset = nOffset + nHeaderSize;

        if (nType != 0xff) {
            const quint8 nMethod = (quint8)(nType & 0x0f);
            const quint8 nVersion = (quint8)(nType >> 4);
            if (nVersion != 2) break;

            // 0xFF inside the path is the directory separator, not a character
            QByteArray baFull = baPath;
            baFull.replace((char)0xff, "/");
            QString sName = QString::fromLatin1(baFull) + QString::fromLatin1(baName);

            MEMBER member = {};
            member.nHeaderOffset = nOffset;
            member.nHeaderSize = nHeaderSize;
            member.nDataOffset = nDataOffset;
            member.nCompressedSize = nPacked;
            member.nUncompressedSize = nOriginal;
            member.nCRC = nCRC;
            member.nTime = nTime;
            member.nMethod = nMethod;
            member.bIsFolder = ((nMethod == 0x0e) || (nMethod == 0x0f));
            member.sFileName = sName;
            if (!haRangeWithin(context.nInputSize, nDataOffset, nPacked)) {
                if (nDataOffset > context.nInputSize) break;
                member.nCompressedSize = context.nInputSize - nDataOffset;
            }
            context.listMembers.append(member);
        }

        nOffset = nDataOffset + nPacked;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XHAArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XHAArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHAArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHAArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHAArchive(pDevice);
}

QList<QString> XHAArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'HA'");
}

XBinary::FT XHAArchive::getFileType()
{
    return FT_HA;
}

XBinary::MODE XHAArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XHAArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XHAArchive::getArch()
{
    return QString();
}

qint32 XHAArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XHAArchive::getFileFormatExt()
{
    return QStringLiteral("ha");
}

QString XHAArchive::getFileFormatExtsString()
{
    return QStringLiteral("HA (*.ha)");
}

QString XHAArchive::getMIMEString()
{
    return QStringLiteral("application/x-ha");
}

QString XHAArchive::getVersion()
{
    return QString();
}

qint64 XHAArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XHAArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XHAArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XHAArchive::methodToString(quint8 nMethod)
{
    if (nMethod == HA_METHOD_STORED) return QStringLiteral("CPY (stored)");
    if (nMethod == HA_METHOD_ASC) return QStringLiteral("ASC");
    if (nMethod == HA_METHOD_HSC) return QStringLiteral("HSC");
    if ((nMethod == 0x0e) || (nMethod == 0x0f)) return QStringLiteral("Directory");
    return QStringLiteral("Unknown %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XHAArchive::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == HA_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == HA_METHOD_ASC) return HANDLE_METHOD_HA_ASC;
    if (nMethod == HA_METHOD_HSC) return HANDLE_METHOD_HA_HSC;
    return HANDLE_METHOD_UNKNOWN;
}

bool XHAArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XHAArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = HA_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
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

QMap<XBinary::UNPACK_PROP, QVariant> XHAArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XHAArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

XBinary::ARCHIVERECORD XHAArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));

    return result;
}

bool XHAArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XHAArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XHAArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
