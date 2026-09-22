/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xmarc.h"

#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 MARC_HEADER_SIZE = 12;
const qint64 MARC_ENTRY_SIZE = 0x44;
const qint64 MARC_NAME_SIZE = 56;
// The reference extractor overwrites the last byte of the name field with a
// NUL before reading it, so a member name can never be longer than 55 bytes.
const qint64 MARC_NAME_MAX = 55;
const quint32 MARC_VERSION = 3;
// 68 bytes per entry; a directory of a million entries is already 68 MiB and
// nothing in the wild comes close.  The cap only guards the allocation.
const qint32 MARC_MAX_ENTRIES = 1000000;

bool marcRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XMARC::XMARC(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMARC::~XMARC()
{
}

// Names are relative paths with '/' separators ("art/logo.gif").  The separator
// is therefore kept, but every character the host filesystem would reject is
// escaped as %XX rather than folded to '_': escaping is reversible and cannot
// collapse two distinct members onto one output file.
QString XMARC::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(MARC_NAME_MAX)) && (pRawName[nLength] != '\0')) nLength++;

    QString sResult;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        if (nCharacter == '\\') {
            sResult.append(QLatin1Char('/'));
            continue;
        }
        const bool bSafe = (nCharacter >= 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != ':') && (nCharacter != '*') &&
                           (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("record%1").arg(nIndex);
    }
    return sResult;
}

bool XMARC::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < MARC_HEADER_SIZE + MARC_ENTRY_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, MARC_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != MARC_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (memcmp(pHeader, "MARC", 4) != 0) return false;
    if (qFromLittleEndian<quint32>(pHeader + 4) != MARC_VERSION) return false;

    const qint32 nNumberOfEntries = qFromLittleEndian<qint32>(pHeader + 8);
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > MARC_MAX_ENTRIES)) return false;
    context.nNumberOfEntries = nNumberOfEntries;
    context.nDirectoryOffset = MARC_HEADER_SIZE;
    context.nDirectorySize = static_cast<qint64>(nNumberOfEntries) * MARC_ENTRY_SIZE;
    if (MARC_HEADER_SIZE + context.nDirectorySize > context.nInputSize) return false;

    const QByteArray baDirectory = read_array_process(context.nDirectoryOffset, context.nDirectorySize, pPdStruct);
    if ((baDirectory.size() != context.nDirectorySize)) return false;
    const char *pDirectory = baDirectory.constData();

    const qint64 nDataStart = MARC_HEADER_SIZE + context.nDirectorySize;
    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pDirectory + (static_cast<qint64>(i) * MARC_ENTRY_SIZE);
        const uchar *pRaw = reinterpret_cast<const uchar *>(pEntry);

        // A member name is never empty in the format, and the field must be
        // NUL terminated inside its own 56 bytes.  Both rules together are what
        // keeps a stray "MARC" + 3 from parsing as a directory.
        if (pEntry[0] == '\0') return false;
        bool bTerminated = false;
        for (qint64 j = 0; j < MARC_NAME_SIZE; j++) {
            if (pEntry[j] == '\0') {
                bTerminated = true;
                break;
            }
        }
        if (!bTerminated) return false;

        const qint64 nSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x38));
        const qint64 nOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x40));
        if ((nSize < 0) || (nOffset < nDataStart)) return false;
        if (!marcRangeWithin(context.nInputSize, nOffset, nSize)) return false;

        MEMBER member = {};
        member.nDataOffset = nOffset;
        member.nSize = nSize;
        // Plain CRC-32 of the member payload (the reference reader compares it
        // against the inverted running register, which is the same thing).
        member.nCRC32 = qFromLittleEndian<quint32>(pRaw + 0x3c);
        member.sFileName = rawNameToString(pEntry, i);
        context.listMembers.append(member);
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XMARC::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XMARC::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMARC archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMARC::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMARC(pDevice);
}

QList<QString> XMARC::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'MARC'03000000"));
    return listResult;
}

XBinary::FT XMARC::getFileType()
{
    return FT_MARC;
}

XBinary::MODE XMARC::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMARC::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMARC::getArch()
{
    return QString();
}

QString XMARC::getFileFormatExt()
{
    return QStringLiteral("mar");
}

QString XMARC::getFileFormatExtsString()
{
    return QStringLiteral("MARC resource archive (*.mar)");
}

QString XMARC::getMIMEString()
{
    return QStringLiteral("application/x-marc-archive");
}

QString XMARC::getVersion()
{
    return QString::number(MARC_VERSION);
}

qint64 XMARC::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XMARC::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XMARC::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XMARC::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XMARC::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = MARC_HEADER_SIZE + context.nDirectorySize;
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
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XMARC::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMARC::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("MARC resource archive"));
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

XBinary::ARCHIVERECORD XMARC::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    return result;
}

bool XMARC::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->nInputSize;
    }

    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XMARC::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
