/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsos.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 SOS_SECTOR_SIZE = 512;
const qint64 SOS_BOOTBLOCK_SIZE = 0x14;
const qint64 SOS_RECORD_SIZE = 32;
const qint32 SOS_NAME_SIZE = 24;
// The reference implementation probes sectors 1..30 inclusive for the "loader" record.
const qint32 SOS_FIRST_PROBE_SECTOR = 1;
const qint32 SOS_LAST_PROBE_SECTOR = 0x1e;
const char *SOS_ANCHOR_NAME = "loader";
// The largest AmigaDOS filesystem flag the bootblock check accepts.
const quint8 SOS_MAX_DOS_FLAG = 5;
const qint32 SOS_MAX_MEMBERS = 100000;

bool sosRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

QString sosRecordName(const uchar *pRecord, qint32 *pnLength)
{
    qint32 nLength = 0;
    while ((nLength < SOS_NAME_SIZE) && (pRecord[8 + nLength] != 0)) ++nLength;
    if (pnLength) *pnLength = nLength;
    return QString::fromLatin1((const char *)pRecord + 8, nLength);
}
}  // namespace

XSOS::XSOS(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSOS::~XSOS()
{
}

bool XSOS::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // The whole image has to be a whole number of sectors, and it has to be
    // long enough for the directory probe to have somewhere to look.
    if (context.nInputSize < (qint64)(SOS_LAST_PROBE_SECTOR + 1) * SOS_SECTOR_SIZE) return false;
    if ((context.nInputSize % SOS_SECTOR_SIZE) != 0) return false;

    const QByteArray baBoot = read_array_process(0, SOS_BOOTBLOCK_SIZE, pPdStruct);
    if (!guardedSource || (baBoot.size() != SOS_BOOTBLOCK_SIZE)) return false;
    const uchar *pBoot = (const uchar *)baBoot.constData();

    if (memcmp(pBoot, "DOS", 3) != 0) return false;
    if (pBoot[3] > SOS_MAX_DOS_FLAG) return false;
    if (memcmp(pBoot + 0x10, "SOS1", 4) != 0) return false;
    context.nDosFlag = pBoot[3];

    // Find the directory: the record named "loader" opens it, and it always
    // sits at the very start of one of the first thirty sectors.
    qint64 nDirectoryOffset = -1;
    for (qint32 nSector = SOS_FIRST_PROBE_SECTOR; nSector <= SOS_LAST_PROBE_SECTOR; ++nSector) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nOffset = (qint64)nSector * SOS_SECTOR_SIZE;
        if (!sosRangeWithin(context.nInputSize, nOffset, SOS_RECORD_SIZE)) break;
        const QByteArray baRecord = read_array_process(nOffset, SOS_RECORD_SIZE, pPdStruct);
        if (!guardedSource || (baRecord.size() != SOS_RECORD_SIZE)) return false;
        if (sosRecordName((const uchar *)baRecord.constData(), nullptr) == QLatin1String(SOS_ANCHOR_NAME)) {
            nDirectoryOffset = nOffset;
            break;
        }
    }
    if (nDirectoryOffset < 0) return false;

    context.nDirectoryOffset = nDirectoryOffset;
    qint64 nOffset = nDirectoryOffset;
    qint64 nArchiveEnd = 0;
    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!sosRangeWithin(context.nInputSize, nOffset, SOS_RECORD_SIZE)) return false;
        const QByteArray baRecord = read_array_process(nOffset, SOS_RECORD_SIZE, pPdStruct);
        if (!guardedSource || (baRecord.size() != SOS_RECORD_SIZE)) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();

        const qint64 nDataOffset = (qint64)(qint32)qFromBigEndian<quint32>(pRecord);
        const qint64 nSize = (qint64)(qint32)qFromBigEndian<quint32>(pRecord + 4);
        // A zero offset or an empty name ends the directory - both are a clean
        // stop in the original, not an error.
        if (nDataOffset == 0) break;
        qint32 nNameLength = 0;
        const QString sName = sosRecordName(pRecord, &nNameLength);
        if (nNameLength == 0) break;
        if ((nDataOffset < 0) || (nSize < 0)) return false;
        if (!sosRangeWithin(context.nInputSize, nDataOffset, nSize)) return false;
        if (context.listMembers.size() >= SOS_MAX_MEMBERS) return false;

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nSize = nSize;
        member.sFileName = sName;
        context.listMembers.append(member);

        nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nSize);
        nOffset += SOS_RECORD_SIZE;
    }

    if (context.listMembers.isEmpty()) return false;
    context.nDirectorySize = (nOffset + SOS_RECORD_SIZE) - context.nDirectoryOffset;
    context.nArchiveSize = qMax(nArchiveEnd, context.nDirectoryOffset + context.nDirectorySize);
    if (!guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    *pContext = context;
    return true;
}

bool XSOS::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSOS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSOS archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSOS::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSOS(pDevice);
}

XBinary::FT XSOS::getFileType()
{
    return FT_SOS;
}

XBinary::MODE XSOS::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSOS::getEndian()
{
    // The bootblock is 68000 code and the directory records are big-endian.
    return ENDIAN_BIG;
}

QString XSOS::getArch()
{
    return QString();
}

QString XSOS::getFileFormatExt()
{
    return QStringLiteral("adf");
}

QString XSOS::getFileFormatExtsString()
{
    return QStringLiteral("SOS bootable Amiga disk (*.adf)");
}

QString XSOS::getMIMEString()
{
    return QStringLiteral("application/x-sos-adf");
}

QString XSOS::getVersion()
{
    // "SOS1" at +0x10 is the loader's own version tag.
    return QStringLiteral("1");
}

qint64 XSOS::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    // The members never fill the whole floppy image, but the image IS the
    // container, so its full size is the format size.
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XSOS::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSOS::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XSOS::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSOS::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store"));
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSOS::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSOS::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("SOS bootable Amiga disk; stored members"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
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

XBinary::ARCHIVERECORD XSOS::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The directory has neither timestamps nor checksums.
    return result;
}

bool XSOS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSOS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
