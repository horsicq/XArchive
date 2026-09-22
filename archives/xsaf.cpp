/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsaf.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 SAF_ENTRY_SIZE = 0x23;
const qint64 SAF_NAME_SIZE = 14;
const qint64 SAF_SHORT_BANNER = 0x1d;
const qint64 SAF_LONG_BANNER = 0x2f;
const qint32 SAF_MAX_MEMBERS = 65536;

// DOS 8.3 names, NUL terminated inside a 14 byte field.  Anything that is not
// printable ASCII, and anything that could redirect the output path, is
// rejected: a false positive here would let a non-SAF file list members.
bool safIsValidRawName(const char *pName)
{
    if (pName[0] == 0) return false;
    bool bTerminated = false;
    for (qint64 i = 0; i < SAF_NAME_SIZE; i++) {
        const quint8 nCharacter = static_cast<quint8>(pName[i]);
        if (nCharacter == 0) {
            bTerminated = true;
            continue;
        }
        if (bTerminated) continue;  // filler bytes after the terminator
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }
    return bTerminated;
}

QString safRawNameToString(const char *pName)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(SAF_NAME_SIZE)) && (pName[nLength] != '\0')) nLength++;
    return QString::fromLatin1(pName, nLength);
}
}  // namespace

XSAF::XSAF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSAF::~XSAF()
{
}

QByteArray XSAF::methodProperty(quint32 nMethod)
{
    QByteArray baResult;
    baResult.append(char(static_cast<quint8>(nMethod)));
    return baResult;
}

bool XSAF::parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SAF_LONG_BANNER + SAF_ENTRY_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, SAF_LONG_BANNER, pPdStruct);
    if (!guardedSource || (baHeader.size() != SAF_LONG_BANNER)) return false;
    const char *pHeader = baHeader.constData();

    if (memcmp(pHeader, "SAF, (c)", 8) != 0) return false;

    // Both banner lengths are fixed; the 1A 00 pair is the banner terminator.
    if ((static_cast<quint8>(pHeader[0x1b]) == 0x1a) && (static_cast<quint8>(pHeader[0x1c]) == 0x00)) {
        context.nFirstMemberOffset = SAF_SHORT_BANNER;
    } else if ((static_cast<quint8>(pHeader[0x2d]) == 0x1a) && (static_cast<quint8>(pHeader[0x2e]) == 0x00)) {
        context.nFirstMemberOffset = SAF_LONG_BANNER;
    } else {
        return false;
    }
    context.sBanner = QString::fromLatin1(pHeader, static_cast<int>(context.nFirstMemberOffset - 2)).trimmed();
    context.nArchiveSize = context.nInputSize;

    // The first member header is part of what makes the banner safe to detect
    // on, so it is always validated even in the header-only pass.
    const QByteArray baFirst = read_array_process(context.nFirstMemberOffset, SAF_ENTRY_SIZE, pPdStruct);
    if (!guardedSource || (baFirst.size() != SAF_ENTRY_SIZE)) return false;
    {
        const uchar *pEntry = reinterpret_cast<const uchar *>(baFirst.constData());
        if (!safIsValidRawName(baFirst.constData())) return false;
        const qint32 nRawSize = qFromLittleEndian<qint32>(pEntry + 0x0e);
        const qint32 nPackedSize = qFromLittleEndian<qint32>(pEntry + 0x12);
        if ((nRawSize < 0) || (nPackedSize < 0)) return false;
        if (static_cast<qint64>(nPackedSize) > (context.nInputSize - context.nFirstMemberOffset - SAF_ENTRY_SIZE)) return false;
    }

    if (bHeaderOnly) {
        *pContext = context;
        return true;
    }

    qint64 nPosition = context.nFirstMemberOffset;
    while ((nPosition + SAF_ENTRY_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const QByteArray baEntry = read_array_process(nPosition, SAF_ENTRY_SIZE, pPdStruct);
        if (!guardedSource || (baEntry.size() != SAF_ENTRY_SIZE)) return false;
        const uchar *pEntry = reinterpret_cast<const uchar *>(baEntry.constData());
        if (!safIsValidRawName(baEntry.constData())) return false;

        const qint32 nRawSize = qFromLittleEndian<qint32>(pEntry + 0x0e);
        const qint32 nPackedSize = qFromLittleEndian<qint32>(pEntry + 0x12);
        if ((nRawSize < 0) || (nPackedSize < 0)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nPosition;
        member.nDataOffset = nPosition + SAF_ENTRY_SIZE;
        member.nRawSize = nRawSize;
        member.nPackedSize = nPackedSize;
        member.nMethod = pEntry[0x1b];
        // +0x16 is the DOS date and +0x18 the DOS time, not the other way
        // round: the reference extractor stamps SETUP1.COM of
        // 97_axywiqrldiwclzlw_SETUP.SAF as 1992-09-24 03:00:00, which is
        // exactly date 0x1938 / time 0x1800.  Reading them the other way round
        // yields month 0 day 0 and an invalid QDateTime.
        member.nDosDate = qFromLittleEndian<quint16>(pEntry + 0x16);
        member.nDosTime = qFromLittleEndian<quint16>(pEntry + 0x18);
        member.sFileName = safRawNameToString(baEntry.constData());
        if (member.nDataOffset + member.nPackedSize > context.nInputSize) return false;
        context.listMembers.append(member);
        if (context.listMembers.size() > SAF_MAX_MEMBERS) return false;

        nPosition = member.nDataOffset + member.nPackedSize;
    }
    if (context.listMembers.isEmpty()) return false;

    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSAF::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSAF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSAF archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSAF::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSAF(pDevice);
}

QList<QString> XSAF::getSearchSignatures()
{
    return {QStringLiteral("'SAF, (c)'")};
}

XBinary::FT XSAF::getFileType()
{
    return FT_SAF;
}

XBinary::MODE XSAF::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSAF::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSAF::getArch()
{
    return QString();
}

QString XSAF::getFileFormatExt()
{
    return QStringLiteral("saf");
}

QString XSAF::getFileFormatExtsString()
{
    return QStringLiteral("Stac SAF archive (*.saf)");
}

QString XSAF::getMIMEString()
{
    return QStringLiteral("application/x-stac-saf");
}

QString XSAF::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, true, nullptr)) return QString();
    const qint32 nIndex = context.sBanner.lastIndexOf(QStringLiteral("Version "));
    if (nIndex >= 0) return context.sBanner.mid(nIndex + 8).trimmed();
    return QString();
}

qint64 XSAF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, true, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSAF::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSAF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSAF::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSAF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, false, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nFirstMemberOffset;
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
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nRawSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SAF);
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, methodProperty(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("SAF LZ77"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = SAF_ENTRY_SIZE + member.nPackedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XSAF::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSAF::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, false, pPdStruct) || !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Stac SAF archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
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

XBinary::ARCHIVERECORD XSAF::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nRawSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SAF);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, methodProperty(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("SAF LZ77"));
    if (member.nDosDate != 0) {
        result.mapProperties.insert(FPART_PROP_DATETIME, XBinary::dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
    }
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XSAF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nInputSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSAF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
