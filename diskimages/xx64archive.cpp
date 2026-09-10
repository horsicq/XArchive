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
#include "xx64archive.h"

#include <QBuffer>
#include <QPointer>
#include <QSet>
#include <QtEndian>

#include <memory>
#include <new>

namespace {
const qint64 X64_HEADER_SIZE = 64;
const qint64 X64_SECTOR_SIZE = 0x100;
const qint32 X64_DIRECTORY_TRACK = 18;
const qint32 X64_DIRECTORY_SECTOR = 1;
const qint32 X64_ENTRIES_PER_SECTOR = 8;
const qint32 X64_ENTRY_SIZE = 0x20;
const qint32 X64_NAME_SIZE = 16;
const qint32 X64_MAX_TRACK = 40;
const qint32 X64_MAX_MEMBERS = 4096;
const qint32 X64_MAX_CHAIN_SECTORS = 100000;
const quint8 X64_NAME_PAD = 0xa0;

bool x64RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

quint32 x64ChainKey(quint8 nTrack, quint8 nSector)
{
    return ((quint32)nTrack << 8) | (quint32)nSector;
}

}  // namespace

XX64Archive::XX64Archive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XX64Archive::~XX64Archive()
{
}

qint32 XX64Archive::sectorsPerTrack(qint32 nTrack)
{
    if (nTrack <= 17) return 21;
    if (nTrack <= 24) return 19;
    if (nTrack <= 30) return 18;
    return 17;
}

qint64 XX64Archive::sectorOffset(qint32 nTrack, qint32 nSector)
{
    if ((nTrack < 1) || (nSector < 0)) return -1;
    qint64 nOffset = 0;
    for (qint32 i = 1; i < nTrack; ++i) nOffset += (qint64)sectorsPerTrack(i) * X64_SECTOR_SIZE;

    return nOffset + (qint64)nSector * X64_SECTOR_SIZE;
}

bool XX64Archive::collectChain(CONTEXT *pContext, quint8 nTrack, quint8 nSector, MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pContext || !pMember) return false;

    QPointer<XX64Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    QSet<quint32> setVisited;
    quint8 nCurrentTrack = nTrack;
    quint8 nCurrentSector = nSector;

    while (nCurrentTrack != 0) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (pMember->listExtents.size() >= X64_MAX_CHAIN_SECTORS) break;
        const quint32 nKey = x64ChainKey(nCurrentTrack, nCurrentSector);
        if (setVisited.contains(nKey)) break;
        setVisited.insert(nKey);

        if ((qint32)nCurrentTrack > X64_MAX_TRACK) break;
        const qint64 nRelative = sectorOffset((qint32)nCurrentTrack, (qint32)nCurrentSector);
        if (nRelative < 0) break;
        const qint64 nOffset = pContext->nImageOffset + nRelative;
        if (!x64RangeWithin(pContext->nInputSize, nOffset, X64_SECTOR_SIZE)) break;

        const QByteArray baLink = read_array_process(nOffset, 2, pPdStruct);
        if (!guardedThis || !guardedSource || (baLink.size() != 2)) return false;
        const quint8 nNextTrack = (quint8)baLink.at(0);
        const quint8 nNextSector = (quint8)baLink.at(1);

        EXTENT extent = {};
        extent.nOffset = nOffset + 2;
        if (nNextTrack == 0) {
            // The last sector: the link's second byte is the index of the last
            // used byte, so the payload is bytes 2..that index inclusive.
            extent.nSize = ((qint32)nNextSector >= 2) ? ((qint64)nNextSector - 1) : 0;
            if (extent.nSize > 0) {
                pMember->listExtents.append(extent);
                pMember->nSize += extent.nSize;
            }
            break;
        }

        extent.nSize = X64_SECTOR_SIZE - 2;
        pMember->listExtents.append(extent);
        pMember->nSize += extent.nSize;

        nCurrentTrack = nNextTrack;
        nCurrentSector = nNextSector;
    }

    return true;
}

bool XX64Archive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XX64Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nImageOffset = X64_HEADER_SIZE;
    if (context.nInputSize < (X64_HEADER_SIZE + X64_SECTOR_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, 8, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != 8)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if ((pHeader[0] != 0x43) || (pHeader[1] != 0x15) || (pHeader[2] != 0x41) || (pHeader[3] != 0x64)) return false;
    if (pHeader[4] != 1) return false;

    context.nImageSize = context.nInputSize - X64_HEADER_SIZE;

    QSet<quint32> setVisitedDirectory;
    quint8 nTrack = (quint8)X64_DIRECTORY_TRACK;
    quint8 nSector = (quint8)X64_DIRECTORY_SECTOR;

    while (nTrack != 0) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const quint32 nKey = x64ChainKey(nTrack, nSector);
        if (setVisitedDirectory.contains(nKey)) break;
        setVisitedDirectory.insert(nKey);
        if ((qint32)nTrack > X64_MAX_TRACK) break;

        const qint64 nRelative = sectorOffset((qint32)nTrack, (qint32)nSector);
        if (nRelative < 0) break;
        const qint64 nOffset = context.nImageOffset + nRelative;
        if (!x64RangeWithin(context.nInputSize, nOffset, X64_SECTOR_SIZE)) break;

        const QByteArray baSector = read_array_process(nOffset, X64_SECTOR_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baSector.size() != X64_SECTOR_SIZE)) return false;
        const uchar *pSector = (const uchar *)baSector.constData();

        for (qint32 i = 0; i < X64_ENTRIES_PER_SECTOR; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (context.listMembers.size() >= X64_MAX_MEMBERS) break;
            const uchar *pEntry = pSector + (qint64)i * X64_ENTRY_SIZE;
            if (pEntry[2] == 0) continue;  // deleted or empty

            MEMBER member = {};
            member.nEntryOffset = nOffset + (qint64)i * X64_ENTRY_SIZE;
            member.nFileType = pEntry[2];
            member.nBlocks = (qint32)qFromLittleEndian<quint16>(pEntry + 0x1e);
            member.nSize = 0;

            QByteArray baName;
            for (qint32 j = 0; j < X64_NAME_SIZE; ++j) {
                const quint8 nByte = pEntry[5 + j];
                if (nByte == X64_NAME_PAD) break;
                baName.append((char)nByte);
            }
            member.sFileName = QString::fromLatin1(baName).trimmed();
            if (member.sFileName.isEmpty()) member.sFileName = QStringLiteral("file%1").arg(context.listMembers.size() + 1);

            if (!collectChain(&context, pEntry[3], pEntry[4], &member, pPdStruct)) return false;
            if (!guardedThis || !guardedSource) return false;

            context.listMembers.append(member);
        }

        nTrack = pSector[0];
        nSector = pSector[1];
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;

    return true;
}

bool XX64Archive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XX64Archive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XX64Archive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XX64Archive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XX64Archive(pDevice);
}

QList<QString> XX64Archive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("4315416401");
}

XBinary::FT XX64Archive::getFileType()
{
    return FT_X64;
}

XBinary::MODE XX64Archive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XX64Archive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XX64Archive::getArch()
{
    return QString();
}

qint32 XX64Archive::getType()
{
    return TYPE_ARCHIVE;
}

QString XX64Archive::getFileFormatExt()
{
    return QStringLiteral("x64");
}

QString XX64Archive::getFileFormatExtsString()
{
    return QStringLiteral("VICE C64 disk image (*.x64)");
}

QString XX64Archive::getMIMEString()
{
    return QStringLiteral("application/x-x64-disk-image");
}

QString XX64Archive::getVersion()
{
    return QString();
}

qint64 XX64Archive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XX64Archive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XX64Archive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_DATA, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XX64Archive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XX64Archive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    // A member is a chain of scattered 254-byte fragments, so it cannot be
    // published as an FPART stream; only the container's own regions are.
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = X64_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = context.nImageOffset;
        part.nFileSize = context.nImageSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XX64Archive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XX64Archive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XX64Archive> guardedThis(this);
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
    pState->nCurrentOffset = pContext->listMembers.at(0).nEntryOffset;
    pState->nTotalSize = pContext->nInputSize;
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

XBinary::ARCHIVERECORD XX64Archive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nEntryOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    // Only used to derive the record's identity token; markArchiveStreamRecord()
    // strips the extent, which this member does not have.
    result.nStreamOffset = member.nEntryOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("CBM DOS"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();

    return result;
}

bool XX64Archive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XX64Archive> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedOutput || !guardedSource || !isUnpackOutputSupported(guardedOutput.data()) ||
        devicesAlias(guardedSource.data(), guardedOutput.data()) || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    if ((member.nSize < 0) || !isUnpackOutputSizeAllowed(pState->mapUnpackProperties, member.nSize)) {
        setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, member.sFileName) && pState->spOutputBudget->isEnforcing()) return false;
        if (!pState->spOutputBudget->debit(member.nSize) && pState->spOutputBudget->isEnforcing()) return false;
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(member.nSize, pPdStruct));
    if (!pStage || !pStage->seek(0) || !guardedThis || !guardedOutput || !guardedSource) return false;

    qint64 nWritten = 0;
    for (qint32 i = 0; i < member.listExtents.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const EXTENT &extent = member.listExtents.at(i);
        if ((extent.nSize < 0) || (extent.nSize > (member.nSize - nWritten))) return false;
        if (!x64RangeWithin(pContext->nInputSize, extent.nOffset, extent.nSize)) return false;
        if (extent.nSize == 0) continue;

        const QByteArray baChunk = read_array_process(extent.nOffset, extent.nSize, pPdStruct);
        if (!guardedThis || !guardedOutput || !guardedSource || (baChunk.size() != extent.nSize) || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
        if (pStage->write(baChunk) != extent.nSize) return false;
        nWritten += extent.nSize;
    }
    if (nWritten != member.nSize) return false;
    if (!pStage->seek(0) || !guardedThis || !guardedOutput) return false;

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);

    return bResult && guardedThis;
}

bool XX64Archive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nEntryOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XX64Archive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XX64Archive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_REPORTEDMETHOD
                               << FPART_PROP_ISFOLDER;
}
