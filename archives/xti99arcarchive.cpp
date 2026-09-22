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
#include "xti99arcarchive.h"

#include "Algos/xti99arcdecoder.h"

#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 TI99_WRAPPER_SIZE = 0x80;
const qint64 TI99_SECTOR_SIZE = 0x100;
const qint64 TI99_ENTRY_SIZE = 0x12;
const qint32 TI99_ENTRIES_PER_SECTOR = 14;
const qint64 TI99_TAIL_OFFSET = 0xfc;
const qint32 TI99_MAX_SECTORS = 0x400;
const qint64 TI99_MAX_FILE_SIZE = 0x1000000;  // 16 MiB - a BE16 sector count cannot exceed it
const quint8 TI99_FLAG_COMPRESSED = 0x02;

bool ti99RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool ti99IsTiFiles(const QByteArray &baHeader, qint64 nFileSize)
{
    if (baHeader.size() < TI99_WRAPPER_SIZE) return false;
    if ((quint8)baHeader.at(0) != 0x07) return false;
    if (baHeader.mid(1, 7) != QByteArray("TIFILES", 7)) return false;
    const qint64 nSectors = (qint64)qFromBigEndian<quint16>((const uchar *)baHeader.constData() + 8);

    return (nFileSize == ((nSectors * TI99_SECTOR_SIZE) + TI99_WRAPPER_SIZE)) || (nFileSize == ((nSectors + 1) * TI99_SECTOR_SIZE));
}

bool ti99IsFiad(const QByteArray &baHeader, qint64 nFileSize)
{
    if (baHeader.size() < TI99_WRAPPER_SIZE) return false;
    if (qFromLittleEndian<quint16>((const uchar *)baHeader.constData() + 10) != 0) return false;
    if (baHeader.mid(0x1c, 100) != QByteArray(100, (char)0)) return false;
    const qint64 nSectors = (qint64)qFromBigEndian<quint16>((const uchar *)baHeader.constData() + 0x0e);

    return (nSectors != 0) && (nFileSize == ((nSectors * TI99_SECTOR_SIZE) + TI99_WRAPPER_SIZE));
}

QString ti99Name(const QByteArray &baEntry)
{
    qint32 nEnd = 10;
    while ((nEnd > 0) && (((quint8)baEntry.at(nEnd - 1) == 0x20) || ((quint8)baEntry.at(nEnd - 1) == 0))) --nEnd;

    return QString::fromLatin1(baEntry.left(nEnd));
}

}  // namespace

XTI99ARCArchive::XTI99ARCArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XTI99ARCArchive::~XTI99ARCArchive()
{
}

// The 0x80-byte member header.  The two-byte gap at 0x0a is deliberate: the
// entry's tail is written at 0x0c, not at 0x0a.
QByteArray XTI99ARCArchive::buildTiFilesPrefix(const QByteArray &baEntry)
{
    QByteArray baPrefix(0x80, (char)0);
    if (baEntry.size() < TI99_ENTRY_SIZE) return baPrefix;

    char *pPrefix = baPrefix.data();
    memcpy(pPrefix, baEntry.constData(), 0x0a);
    memcpy(pPrefix + 0x0c, baEntry.constData() + 0x0a, 0x08);

    return baPrefix;
}

bool XTI99ARCArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < TI99_SECTOR_SIZE) || (context.nInputSize > TI99_MAX_FILE_SIZE)) return false;

    quint8 nFlags = 0;
    context.nPayloadOffset = 0;

    if (context.nInputSize >= TI99_WRAPPER_SIZE) {
        const QByteArray baWrapper = read_array_process(0, TI99_WRAPPER_SIZE, pPdStruct);
        if (!guardedSource || (baWrapper.size() != TI99_WRAPPER_SIZE)) return false;
        if (ti99IsTiFiles(baWrapper, context.nInputSize)) {
            nFlags = (quint8)baWrapper.at(0x0a);
            context.nPayloadOffset = TI99_WRAPPER_SIZE;
        } else if (ti99IsFiad(baWrapper, context.nInputSize)) {
            nFlags = (quint8)baWrapper.at(0x0c);
            context.nPayloadOffset = TI99_WRAPPER_SIZE;
        }
    }

    context.nPayloadSize = context.nInputSize - context.nPayloadOffset;
    if (context.nPayloadSize < TI99_SECTOR_SIZE) return false;
    context.bCompressed = ((nFlags & TI99_FLAG_COMPRESSED) != 0);

    // The catalogue chain lives at the start of the payload.  For a compressed
    // payload that means expanding it first - which is exactly why this format
    // needs a codec at all.  The probe is capped at the largest catalogue the
    // format can describe, so a file that is not this format costs one bounded
    // LZW pass and no more.
    QByteArray baCatalogue;
    if (context.bCompressed) {
        const QByteArray baPayload = read_array_process(context.nPayloadOffset, context.nPayloadSize, pPdStruct);
        if (!guardedSource || (baPayload.size() != context.nPayloadSize)) return false;
        if (!XTI99ARCDecoder::expand(baPayload, XTI99ARCDecoder::PROBE_SIZE, &baCatalogue, pPdStruct)) return false;
    } else {
        const qint64 nProbeSize = (context.nPayloadSize < XTI99ARCDecoder::PROBE_SIZE) ? context.nPayloadSize : XTI99ARCDecoder::PROBE_SIZE;
        baCatalogue = read_array_process(context.nPayloadOffset, nProbeSize, pPdStruct);
        if (!guardedSource || (baCatalogue.size() != nProbeSize)) return false;
    }

    // Walk the sector chain first; the entries are only read once the chain is
    // known to terminate, because the data cursor starts behind the last one.
    qint32 nSectorCount = 0;
    qint64 nWalk = 0;
    bool bTerminated = false;

    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nWalk + TI99_SECTOR_SIZE) > baCatalogue.size()) return false;
        const quint8 *pTail = (const quint8 *)baCatalogue.constData() + nWalk + TI99_TAIL_OFFSET;
        ++nSectorCount;
        nWalk += TI99_SECTOR_SIZE;

        if ((pTail[0] == 'E') && (pTail[1] == 'N') && (pTail[2] == 'D') && (pTail[3] == '!')) {
            bTerminated = true;
            break;
        }
        if ((pTail[0] != 0) || (pTail[1] != 0) || (pTail[2] != 0) || (pTail[3] != 0)) return false;
        if (nSectorCount > TI99_MAX_SECTORS) return false;
    }
    if (!bTerminated) return false;

    const QByteArray baFreeSlot((qint32)TI99_ENTRY_SIZE, (char)0);
    const qint64 nCatalogueSize = nWalk;
    qint64 nDataCursor = nCatalogueSize;

    for (qint32 nSector = 0; nSector < nSectorCount; ++nSector) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        for (qint32 i = 0; i < TI99_ENTRIES_PER_SECTOR; ++i) {
            const qint64 nEntryOffset = ((qint64)nSector * TI99_SECTOR_SIZE) + ((qint64)i * TI99_ENTRY_SIZE);
            const QByteArray baEntry = baCatalogue.mid((qint32)nEntryOffset, (qint32)TI99_ENTRY_SIZE);
            if (baEntry.size() != TI99_ENTRY_SIZE) return false;
            // A free slot consumes no data - advancing the cursor here shifts
            // every following member.
            if (baEntry == baFreeSlot) continue;

            const qint64 nSectors = (qint64)qFromBigEndian<quint16>((const uchar *)baEntry.constData() + 0x0c);
            const qint64 nMemberSize = nSectors * TI99_SECTOR_SIZE;

            MEMBER member = {};
            member.nMemberOffset = nDataCursor;
            member.nMemberSize = nMemberSize;
            member.nUncompressedSize = TI99_WRAPPER_SIZE + nMemberSize;
            member.bCompressed = context.bCompressed;
            member.baPrefix = buildTiFilesPrefix(baEntry);
            member.sFileName = ti99Name(baEntry);
            member.nHeaderOffset = context.nPayloadOffset + nEntryOffset;

            if (context.bCompressed) {
                // The member is a slice of the expanded stream, so the stream
                // handed to the decoder is the WHOLE payload.
                member.nStreamOffset = context.nPayloadOffset;
                member.nStreamSize = context.nPayloadSize;
            } else {
                member.nStreamOffset = context.nPayloadOffset + nDataCursor;
                member.nStreamSize = nMemberSize;
                if (!ti99RangeWithin(context.nInputSize, member.nStreamOffset, member.nStreamSize)) return false;
            }

            nDataCursor += nMemberSize;
            if (nDataCursor > XTI99ARCDecoder::MAX_PLAIN_SIZE) return false;
            context.listMembers.append(member);
        }
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;

    return true;
}

bool XTI99ARCArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTI99ARCArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTI99ARCArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTI99ARCArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTI99ARCArchive(pDevice);
}

QList<QString> XTI99ARCArchive::getSearchSignatures()
{
    // The wrapper is validated arithmetically and the raw form has no magic at
    // all, so a signature scan cannot stand in for the walk.
    return QList<QString>();
}

XBinary::FT XTI99ARCArchive::getFileType()
{
    return FT_TI99ARC;
}

XBinary::MODE XTI99ARCArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTI99ARCArchive::getEndian()
{
    return ENDIAN_BIG;
}

QString XTI99ARCArchive::getArch()
{
    return QString();
}

qint32 XTI99ARCArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTI99ARCArchive::getFileFormatExt()
{
    return QStringLiteral("ark");
}

QString XTI99ARCArchive::getFileFormatExtsString()
{
    return QStringLiteral("TI99 ARC (*.ark)");
}

QString XTI99ARCArchive::getMIMEString()
{
    return QStringLiteral("application/x-ti99-ark");
}

QString XTI99ARCArchive::getVersion()
{
    return QString();
}

qint64 XTI99ARCArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XTI99ARCArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTI99ARCArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XTI99ARCArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTI99ARCArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && (context.nPayloadOffset > 0) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nPayloadOffset;
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
            part.nFileOffset = member.nStreamOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            if (member.bCompressed) {
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TI99ARC);
                part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("TI99 LZW"));
                part.mapProperties.insert(
                    FPART_PROP_COMPRESSPROPERTIES,
                    XTI99ARCDecoder::buildProperties(member.nMemberOffset + member.nMemberSize, member.nMemberOffset, member.nMemberSize, member.baPrefix));
            } else {
                part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SCL_SECTORS);
                part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored sectors"));
                part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
            }
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XTI99ARCArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTI99ARCArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->nTotalSize = pContext->nInputSize;
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

XBinary::ARCHIVERECORD XTI99ARCArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamOffset = member.nStreamOffset;
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    if (member.bCompressed) {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_TI99ARC);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("TI99 LZW"));
        result.mapProperties.insert(
            FPART_PROP_COMPRESSPROPERTIES,
            XTI99ARCDecoder::buildProperties(member.nMemberOffset + member.nMemberSize, member.nMemberOffset, member.nMemberSize, member.baPrefix));
    } else {
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SCL_SECTORS);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored sectors"));
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baPrefix);
    }
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTI99ARCArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XTI99ARCArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XTI99ARCArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
