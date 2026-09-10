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
#include "xchieflzmultiarchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 CHIEFLZM_HEADER_SIZE = 0x2b;
const qint64 CHIEFLZM_HEADER_PADDING = 0x53;
const qint64 CHIEFLZM_DIRECTORY_OFFSET = CHIEFLZM_HEADER_SIZE + CHIEFLZM_HEADER_PADDING;  // 0x7e
const qint64 CHIEFLZM_ENTRY_SIZE = 0x29;
const qint32 CHIEFLZM_MAX_MEMBERS = 0x100000;
const qint64 CHIEFLZM_MAX_NAME_BYTES = 0x1000000;
const qint32 CHIEFLZM_MAX_PATH_DEPTH = 64;

const quint8 CHIEFLZM_METHOD_STORED = 2;
const quint8 CHIEFLZM_METHOD_FUN004C3E00 = 3;
const quint8 CHIEFLZM_METHOD_CHIEFLZ = 4;

bool chieflzmRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XChiefLZMultiArchive::XChiefLZMultiArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XChiefLZMultiArchive::~XChiefLZMultiArchive()
{
}

bool XChiefLZMultiArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XChiefLZMultiArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < CHIEFLZM_DIRECTORY_OFFSET) return false;

    const QByteArray baHeader = read_array_process(0, CHIEFLZM_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != CHIEFLZM_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (pHeader[0] != 0x0c) return false;
    if (baHeader.mid(1, 12) != QByteArray("\x04\x0d" "ChfLZ_2" "\x05\x06\x04", 12)) return false;

    const qint32 nCount = (qint32)qFromLittleEndian<quint32>(pHeader + 0x17);
    const qint64 nNameBytes = (qint32)qFromLittleEndian<quint32>(pHeader + 0x23);
    if ((nCount <= 0) || (nCount >= CHIEFLZM_MAX_MEMBERS)) return false;
    if ((nNameBytes <= 0) || (nNameBytes >= CHIEFLZM_MAX_NAME_BYTES)) return false;

    const qint64 nEntriesSize = (qint64)nCount * CHIEFLZM_ENTRY_SIZE;
    const qint64 nDirectorySize = nEntriesSize + nNameBytes;
    if (!chieflzmRangeWithin(context.nInputSize, CHIEFLZM_DIRECTORY_OFFSET, nDirectorySize)) return false;

    const QByteArray baDirectory = read_array_process(CHIEFLZM_DIRECTORY_OFFSET, nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource || (baDirectory.size() != nDirectorySize)) return false;
    const uchar *pDirectory = (const uchar *)baDirectory.constData();

    QList<qint32> listParents;
    QList<QString> listNames;
    qint64 nNamePosition = nEntriesSize;
    qint64 nNameRemaining = nNameBytes;
    qint64 nDataOffset = CHIEFLZM_DIRECTORY_OFFSET + nDirectorySize;

    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pEntry = pDirectory + (qint64)i * CHIEFLZM_ENTRY_SIZE;

        const qint32 nNameLength = (qint32)pEntry[0x23];
        if (nNameLength == 0) return false;
        if (nNameLength > nNameRemaining) return false;
        const QString sName = QString::fromLatin1((const char *)(pDirectory + nNamePosition), nNameLength);
        nNamePosition += nNameLength;
        nNameRemaining -= nNameLength;

        const qint64 nPacked = (qint32)qFromLittleEndian<quint32>(pEntry + 0x0f);
        const qint64 nUnpacked = (qint32)qFromLittleEndian<quint32>(pEntry + 0x13);
        if ((nPacked < 0) || (nUnpacked < 0)) return false;
        if (!chieflzmRangeWithin(context.nInputSize, nDataOffset, nPacked)) return false;

        // kind 0 keeps the parent index at +0x01, every other kind at +0x03
        const quint16 nParentField = (pEntry[0] == 0) ? qFromLittleEndian<quint16>(pEntry + 0x01) : qFromLittleEndian<quint16>(pEntry + 0x03);
        listParents.append((nParentField == 0) ? -1 : ((qint32)nParentField - 1));
        listNames.append(sName);

        MEMBER member = {};
        member.nHeaderOffset = CHIEFLZM_DIRECTORY_OFFSET + (qint64)i * CHIEFLZM_ENTRY_SIZE;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nPacked;
        member.nUncompressedSize = nUnpacked;
        member.nDosTime = qFromLittleEndian<quint16>(pEntry + 0x17);
        member.nDosDate = qFromLittleEndian<quint16>(pEntry + 0x19);
        member.nCRC = qFromLittleEndian<quint32>(pEntry + 0x1f);
        member.nMethod = pEntry[0x28];
        member.sFileName = sName;
        context.listMembers.append(member);

        nDataOffset += nPacked;
    }

    // The walker requires the name lengths to consume the declared block exactly.
    if (nNameRemaining != 0) return false;

    // Resolve the directory prefixes now that every entry's own name is known.
    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        QString sPath = listNames.at(i);
        qint32 nParent = listParents.at(i);
        qint32 nDepth = 0;
        while ((nParent >= 0) && (nParent < listNames.size()) && (nParent != i) && (nDepth < CHIEFLZM_MAX_PATH_DEPTH)) {
            sPath = listNames.at(nParent) + QChar('/') + sPath;
            nParent = listParents.at(nParent);
            ++nDepth;
        }
        context.listMembers[i].sFileName = sPath;
    }

    context.nDirectoryEnd = CHIEFLZM_DIRECTORY_OFFSET + nDirectorySize;
    context.nArchiveSize = qMin(nDataOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XChiefLZMultiArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XChiefLZMultiArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XChiefLZMultiArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XChiefLZMultiArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XChiefLZMultiArchive(pDevice);
}

QList<QString> XChiefLZMultiArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("0C040D'ChfLZ_2'050604");
}

XBinary::FT XChiefLZMultiArchive::getFileType()
{
    return FT_CHIEFLZMULTI;
}

XBinary::MODE XChiefLZMultiArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XChiefLZMultiArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XChiefLZMultiArchive::getArch()
{
    return QString();
}

qint32 XChiefLZMultiArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XChiefLZMultiArchive::getFileFormatExt()
{
    return QStringLiteral("clz");
}

QString XChiefLZMultiArchive::getFileFormatExtsString()
{
    return QStringLiteral("ChiefLZ Multiple (*.clz)");
}

QString XChiefLZMultiArchive::getMIMEString()
{
    return QStringLiteral("application/x-chieflz");
}

QString XChiefLZMultiArchive::getVersion()
{
    return QStringLiteral("2");
}

qint64 XChiefLZMultiArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XChiefLZMultiArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XChiefLZMultiArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XChiefLZMultiArchive::methodToString(quint8 nMethod)
{
    if (nMethod == CHIEFLZM_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == CHIEFLZM_METHOD_FUN004C3E00) return QStringLiteral("ChiefLZ method 3");
    if (nMethod == CHIEFLZM_METHOD_CHIEFLZ) return QStringLiteral("ChiefLZ");
    return QStringLiteral("Unknown %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XChiefLZMultiArchive::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == CHIEFLZM_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == CHIEFLZM_METHOD_CHIEFLZ) return HANDLE_METHOD_CHIEFLZ;
    return HANDLE_METHOD_UNKNOWN;
}

bool XChiefLZMultiArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XChiefLZMultiArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDirectoryEnd;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
            // The header stores the COMPLEMENT of the CRC32, which is exactly
            // the "no final xor" variant.
            part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_00000000);
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

QMap<XBinary::UNPACK_PROP, QVariant> XChiefLZMultiArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XChiefLZMultiArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XChiefLZMultiArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XChiefLZMultiArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_00000000);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, member.nCRC);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dateTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dateTime.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);

    return result;
}

bool XChiefLZMultiArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XChiefLZMultiArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XChiefLZMultiArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_CRC_TYPE << FPART_PROP_UNCOMPRESSEDCRC << FPART_PROP_DATETIME
                               << FPART_PROP_ISFOLDER;
}
