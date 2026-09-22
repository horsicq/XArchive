/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free reader for the RTA container of the Pocket Soft
 * RTPatch tooling.
 * MIT License
 */

#include "xrta.h"

#include <QDateTime>
#include <QtEndian>

#include <new>

namespace {
const qint64 RTA_MAGIC_SIZE = 4;
const qint64 RTA_FIXED_HEADER_SIZE = 13;
// One record header is a length-prefixed name, a length-prefixed second string
// and the fixed part; both prefixes are quint8, so this is the ceiling.
const qint64 RTA_MAX_RECORD_HEADER = 1 + 255 + 1 + 255 + RTA_FIXED_HEADER_SIZE;
const qint64 RTA_ATTRIBUTES_OFFSET = 0;
const qint64 RTA_DOSDATE_OFFSET = 1;
const qint64 RTA_DOSTIME_OFFSET = 3;
const qint64 RTA_UNPACKEDSIZE_OFFSET = 5;
const qint64 RTA_PACKEDSIZE_OFFSET = 9;
// A member stream always opens with the codec magic, the raw-literal flag and
// the mandatory reserved byte.  These are the decoder's own invariants, which
// is what makes them safe to require of a candidate container.
const qint64 RTA_STREAM_PREFIX_SIZE = 4;
const quint8 RTA_STREAM_MAGIC_HIGH = 0xb5;
const quint8 RTA_STREAM_MAGIC_LOW = 0x9c;
const quint8 RTA_STREAM_RESERVED = 0xff;
const qint64 RTA_MAX_MEMBERS = 100000;
// DOS attribute bits carried by the record's attribute byte.
const quint8 RTA_ATTRIBUTE_READONLY = 0x01;
const quint8 RTA_ATTRIBUTE_HIDDEN = 0x02;
const quint8 RTA_ATTRIBUTE_SYSTEM = 0x04;
const quint8 RTA_ATTRIBUTE_VOLUME = 0x08;
const quint8 RTA_ATTRIBUTE_DIRECTORY = 0x10;
const quint8 RTA_ATTRIBUTE_ARCHIVE = 0x20;

bool rtaRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XRTA::XRTA(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRTA::~XRTA()
{
}

// The name is taken verbatim; only the OS/2 - DOS separator is normalized.
// Nothing is stripped and nothing is folded onto '_', so two distinct members
// can never collapse onto one output path.  A name the host cannot represent
// rejects the container rather than being rewritten into something that might
// collide with a sibling.
bool XRTA::decodeName(const uchar *pData, qint32 nSize, QString *pName)
{
    if (!pData || !pName || (nSize <= 0)) return false;

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = pData[i];
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
    }

    QString sName = QString::fromLatin1(reinterpret_cast<const char *>(pData), nSize);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/')) || (XBinary::fixFileName(sName) != sName)) {
        return false;
    }

    const QStringList listParts = sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (qint32 i = 0; i < listParts.size(); i++) {
        const QString sPart = listParts.at(i);
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return false;
    }

    *pName = sName;
    return true;
}

// The second declared string never reaches the file name.  It is empty
// throughout the reach set and its meaning is not established, so it is kept
// verbatim as display text and only has to be printable.
bool XRTA::decodeExtra(const uchar *pData, qint32 nSize, QString *pExtra)
{
    if (!pExtra) return false;
    pExtra->clear();
    if (nSize == 0) return true;
    if (!pData || (nSize < 0)) return false;

    for (qint32 i = 0; i < nSize; i++) {
        const quint8 nCharacter = pData[i];
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
    }

    *pExtra = QString::fromLatin1(reinterpret_cast<const char *>(pData), nSize);
    return true;
}

// A member with no bytes on either side is an empty file and carries no stream
// of any kind; everything else is one complete RTPatch stream.
XBinary::HANDLE_METHOD XRTA::memberHandleMethod(const MEMBER &member)
{
    if ((member.nPackedSize == 0) && (member.nUnpackedSize == 0)) return HANDLE_METHOD_STORE;
    return HANDLE_METHOD_RTPATCH;
}

void XRTA::fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    if (!pMapProperties) return;

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUnpackedSize);
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, memberHandleMethod(member));
    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD,
                           (memberHandleMethod(member) == HANDLE_METHOD_STORE) ? QString("Stored") : QString("RTPatch"));
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
    pMapProperties->insert(FPART_PROP_HEADER_OFFSET, member.nHeaderOffset);
    pMapProperties->insert(FPART_PROP_HEADER_SIZE, member.nHeaderSize);
    pMapProperties->insert(FPART_PROP_ISREADONLY, (member.nAttributes & RTA_ATTRIBUTE_READONLY) != 0);
    pMapProperties->insert(FPART_PROP_ISHIDDEN, (member.nAttributes & RTA_ATTRIBUTE_HIDDEN) != 0);
    pMapProperties->insert(FPART_PROP_ISSYSTEM, (member.nAttributes & RTA_ATTRIBUTE_SYSTEM) != 0);
    pMapProperties->insert(FPART_PROP_ISARCHIVE, (member.nAttributes & RTA_ATTRIBUTE_ARCHIVE) != 0);
    if (!member.sExtra.isEmpty()) {
        pMapProperties->insert(FPART_PROP_INFO, member.sExtra);
    }
    if (XBinary::isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        const QDateTime dtModified = XBinary::dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            pMapProperties->insert(FPART_PROP_DATETIME, dtModified);
            pMapProperties->insert(FPART_PROP_MTIME, dtModified);
        }
    }
}

bool XRTA::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < RTA_MAGIC_SIZE + 1) return false;

    const QByteArray baMagic = read_array_process(0, RTA_MAGIC_SIZE + 1, pPdStruct);
    if (baMagic.size() != RTA_MAGIC_SIZE + 1) return false;
    if (memcmp(baMagic.constData(), "KJd\x00", 4) != 0) return false;
    // The reference refuses a container whose first record has an empty name:
    // that byte is the end marker, so such a file declares no members at all.
    if (static_cast<quint8>(baMagic.at(4)) == 0) return false;

    qint64 nCurrent = RTA_MAGIC_SIZE;
    bool bTerminated = false;

    while (!bTerminated) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() > RTA_MAX_MEMBERS) return false;
        if (nCurrent >= context.nInputSize) return false;

        const qint64 nWindow = qMin<qint64>(RTA_MAX_RECORD_HEADER, context.nInputSize - nCurrent);
        const QByteArray baRecord = read_array_process(nCurrent, nWindow, pPdStruct);
        if (baRecord.size() != nWindow) return false;
        const uchar *pRecord = reinterpret_cast<const uchar *>(baRecord.constData());

        qint64 nPosition = 0;
        const quint8 nNameLength = pRecord[nPosition++];
        if (nNameLength == 0) {
            context.nArchiveSize = nCurrent + 1;
            bTerminated = true;
            break;
        }

        if ((nPosition + nNameLength) > nWindow) return false;
        MEMBER member = {};
        if (!decodeName(pRecord + nPosition, nNameLength, &member.sFileName)) return false;
        nPosition += nNameLength;

        if (nPosition >= nWindow) return false;
        const quint8 nExtraLength = pRecord[nPosition++];
        if ((nPosition + nExtraLength) > nWindow) return false;
        if (!decodeExtra(pRecord + nPosition, nExtraLength, &member.sExtra)) return false;
        nPosition += nExtraLength;

        if ((nPosition + RTA_FIXED_HEADER_SIZE) > nWindow) return false;
        const uchar *pFixed = pRecord + nPosition;
        member.nAttributes = pFixed[RTA_ATTRIBUTES_OFFSET];
        member.nDosDate = qFromLittleEndian<quint16>(pFixed + RTA_DOSDATE_OFFSET);
        member.nDosTime = qFromLittleEndian<quint16>(pFixed + RTA_DOSTIME_OFFSET);
        member.nUnpackedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pFixed + RTA_UNPACKEDSIZE_OFFSET));
        member.nPackedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pFixed + RTA_PACKEDSIZE_OFFSET));
        nPosition += RTA_FIXED_HEADER_SIZE;

        if ((member.nUnpackedSize < 0) || (member.nPackedSize < 0)) return false;
        // A volume label or a directory entry has no stream this reader knows
        // how to place, so the container is refused rather than guessed at.
        if (member.nAttributes & (RTA_ATTRIBUTE_VOLUME | RTA_ATTRIBUTE_DIRECTORY)) return false;
        // The only record without a stream is the empty file.
        if ((member.nPackedSize == 0) && (member.nUnpackedSize != 0)) return false;

        member.nHeaderOffset = nCurrent;
        member.nHeaderSize = nPosition;
        member.nDataOffset = nCurrent + nPosition;
        if (!rtaRangeWithin(context.nInputSize, member.nDataOffset, member.nPackedSize)) return false;

        if (member.nPackedSize > 0) {
            if (member.nPackedSize < RTA_STREAM_PREFIX_SIZE) return false;
            const QByteArray baPrefix = read_array_process(member.nDataOffset, RTA_STREAM_PREFIX_SIZE, pPdStruct);
            if (baPrefix.size() != RTA_STREAM_PREFIX_SIZE) return false;
            const uchar *pPrefix = reinterpret_cast<const uchar *>(baPrefix.constData());
            if ((pPrefix[0] != RTA_STREAM_MAGIC_HIGH) || (pPrefix[1] != RTA_STREAM_MAGIC_LOW) || (pPrefix[2] > 1) ||
                (pPrefix[3] != RTA_STREAM_RESERVED)) {
                return false;
            }
        }

        context.listMembers.append(member);
        nCurrent = member.nDataOffset + member.nPackedSize;
    }

    if (!bTerminated || context.listMembers.isEmpty()) return false;
    if (!rtaRangeWithin(context.nInputSize, 0, context.nArchiveSize)) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XRTA::isValid(PDSTRUCT *pPdStruct)
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

bool XRTA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRTA archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRTA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRTA(pDevice);
}

QList<QString> XRTA::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'KJd'00"));
    return listResult;
}

XBinary::FT XRTA::getFileType()
{
    return FT_RTA;
}

XBinary::MODE XRTA::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRTA::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRTA::getArch()
{
    return QString();
}

QString XRTA::getFileFormatExt()
{
    return QStringLiteral("rta");
}

QString XRTA::getFileFormatExtsString()
{
    return QStringLiteral("RTPatch RTA archive (*.rta)");
}

QString XRTA::getMIMEString()
{
    return QStringLiteral("application/x-rta");
}

qint64 XRTA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XRTA::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRTA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XRTA::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XRTA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = RTA_MAGIC_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nPackedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            fillRecordProperties(member, &part.mapProperties);
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nPackedSize;
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

    return listResult;
}

QList<XBinary::FPART_PROP> XRTA::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER,     FPART_PROP_DATETIME,         FPART_PROP_MTIME,
            FPART_PROP_HEADER_OFFSET,  FPART_PROP_HEADER_SIZE,  FPART_PROP_ISREADONLY,       FPART_PROP_ISHIDDEN,
            FPART_PROP_ISSYSTEM,       FPART_PROP_ISARCHIVE,    FPART_PROP_INFO};
}

QMap<XBinary::UNPACK_PROP, QVariant> XRTA::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRTA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("RTA archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
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

XBinary::ARCHIVERECORD XRTA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nPackedSize;
    fillRecordProperties(member, &result.mapProperties);
    return result;
}

bool XRTA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XRTA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
