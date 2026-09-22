/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xnid.h"

#include <QMap>
#include <QtEndian>

#include <new>

namespace {
const qint64 NID_HEADER_SIZE = 0x78;
const qint64 NID_ENTRY_SIZE = 29;
const qint64 NID_NAME_SIZE = 11;
const qint64 NID_BLOCK_FRAME_SIZE = 5;
// The count field is 16 bit, so 65535 members is the hard producer limit.
const qint32 NID_MAX_ENTRIES = 65535;
// Only two of the sixteen header bits are read: the payload is compressed, and
// this is the last block of the member.
const quint32 NID_BLOCK_COMPRESSED = 0x0800;
const quint32 NID_BLOCK_LAST = 0x0100;

bool nidRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XNID::XNID(QIODevice *pDevice) : XArchive(pDevice)
{
}

XNID::~XNID()
{
}

// Every name in the reference corpus is a plain DOS 8.3 identifier.  Requiring
// that of the first entry is one of the structural rules that stops random data
// from parsing as a directory.
bool XNID::isValidFcbName(const char *pRawName)
{
    bool bAny = false;
    for (qint64 i = 0; i < NID_NAME_SIZE; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        if (nCharacter <= 0x20) continue;  // the reference extractor drops these
        if (nCharacter > 0x7e) return false;
        bAny = true;
    }
    return bAny;
}

// The 11-byte FCB field is rendered exactly the way the reference extractor
// does it: bytes above 0x20 of the 8-byte stem, a dot, bytes above 0x20 of the
// 3-byte extension, and a trailing dot is dropped.  Bytes that are legal in the
// field but not in a file name are escaped as %XX: escaping is reversible and
// cannot collapse two distinct members onto one output file.
QString XNID::fcbNameToString(const char *pRawName, qint32 nIndex)
{
    QString sResult;
    for (qint32 nPart = 0; nPart < 2; nPart++) {
        const qint32 nFrom = (nPart == 0) ? 0 : 8;
        const qint32 nTo = (nPart == 0) ? 8 : 11;
        if (nPart == 1) sResult.append(QLatin1Char('.'));
        for (qint32 i = nFrom; i < nTo; i++) {
            const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
            if (nCharacter <= 0x20) continue;
            const bool bSafe = (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') && (nCharacter != ':') &&
                               (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') &&
                               (nCharacter != '|') && (nCharacter != '.');
            if (bSafe) {
                sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
            } else {
                QString sHex = QString::number(nCharacter, 16).toUpper();
                if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
                sResult.append(QLatin1Char('%'));
                sResult.append(sHex);
            }
        }
    }
    if (sResult.endsWith(QLatin1Char('.'))) sResult.chop(1);
    if (sResult.isEmpty()) sResult = QStringLiteral("record%1").arg(nIndex);
    return sResult;
}

// Walks the block chain to find where the member ends.  A chain that leaves the
// file is clamped rather than rejected: a set split across volumes has exactly
// that shape, and the reference extractor lists such members and only fails
// when it tries to expand them.
bool XNID::measureMember(MEMBER *pMember, qint64 nInputSize, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();

    qint64 nPosition = pMember->nDataOffset;
    qint32 nBlocks = 0;
    pMember->bSingleStoredBlock = false;
    bool bFirstStored = false;
    qint64 nFirstBlockSize = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        if (nPosition + NID_BLOCK_FRAME_SIZE > nInputSize) break;
        const QByteArray baFrame = read_array_process(nPosition, NID_BLOCK_FRAME_SIZE, pPdStruct);
        if ((baFrame.size() != NID_BLOCK_FRAME_SIZE)) break;
        const uchar *pFrame = reinterpret_cast<const uchar *>(baFrame.constData());
        const quint32 nFlags = qFromLittleEndian<quint16>(pFrame + 1);
        const qint64 nBlockSize = static_cast<qint64>(qFromLittleEndian<quint16>(pFrame + 3));
        if (nBlocks == 0) {
            bFirstStored = ((nFlags & NID_BLOCK_COMPRESSED) == 0);
            nFirstBlockSize = nBlockSize;
        }
        ++nBlocks;
        nPosition += NID_BLOCK_FRAME_SIZE + nBlockSize;
        if (nPosition > nInputSize) {
            nPosition = nInputSize;
            break;
        }
        if (nFlags & NID_BLOCK_LAST) {
            if ((nBlocks == 1) && bFirstStored && (nFirstBlockSize == pMember->nUncompressedSize)) {
                pMember->bSingleStoredBlock = true;
            }
            break;
        }
        // A member never runs to a million blocks; the bound stops a corrupt
        // chain from spinning.
        if (nBlocks > 1000000) break;
    }

    pMember->nCompressedSize = nPosition - pMember->nDataOffset;
    if (pMember->nCompressedSize < 0) pMember->nCompressedSize = 0;
    return guardedSource;
}

bool XNID::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < NID_HEADER_SIZE + NID_ENTRY_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, NID_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != NID_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if ((pHeader[0] != 'N') || (pHeader[1] != 'I') || (pHeader[2] != 0x15) || (pHeader[3] != 0x01)) return false;

    const qint32 nNumberOfEntries = static_cast<qint32>(qFromLittleEndian<quint16>(pHeader + 6));
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > NID_MAX_ENTRIES)) return false;
    context.nNumberOfEntries = nNumberOfEntries;
    context.nDirectoryOffset = NID_HEADER_SIZE;
    context.nDirectorySize = static_cast<qint64>(nNumberOfEntries) * NID_ENTRY_SIZE;
    if (!nidRangeWithin(context.nInputSize, context.nDirectoryOffset, context.nDirectorySize)) return false;

    const QByteArray baDirectory = read_array_process(context.nDirectoryOffset, context.nDirectorySize, pPdStruct);
    if ((baDirectory.size() != context.nDirectorySize)) return false;
    const char *pDirectory = baDirectory.constData();

    // Header and directory tile the front of the file: the first entry's data
    // offset (one based) is fixed by the entry count.  This is the check that
    // makes the four-byte magic safe to detect on.
    const qint64 nFirstOffset = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pDirectory) + 1));
    if (nFirstOffset != context.nDirectoryOffset + context.nDirectorySize + 1) return false;
    if (static_cast<quint8>(pDirectory[0]) != 1) return false;
    if (!isValidFcbName(pDirectory + 9)) return false;
    if (static_cast<qint32>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pDirectory) + 25)) < 0) return false;

    QMap<quint32, qint32> mapFolders;
    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pDirectory + (NID_ENTRY_SIZE * i);
        // The reference extractor stops the walk here rather than failing: the
        // continuation entries of the next volume carry method 2.
        if (static_cast<quint8>(pEntry[0]) != 1) break;
        const qint64 nOffset = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry) + 1));
        const qint64 nSize = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry) + 25));
        if ((nOffset < 1) || (nSize < 0)) break;

        MEMBER member = {};
        member.nRecordOffset = context.nDirectoryOffset + (NID_ENTRY_SIZE * i);
        member.nDataOffset = nOffset - 1;  // the stored offset is one based
        member.nUncompressedSize = nSize;
        member.nFolderKey = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry) + 5);
        member.nTime = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(pEntry) + 18);
        member.nDate = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(pEntry) + 20);

        if (!mapFolders.contains(member.nFolderKey)) {
            mapFolders.insert(member.nFolderKey, mapFolders.size() + 1);
        }
        member.nFolderIndex = mapFolders.value(member.nFolderKey);
        member.sFileName = QStringLiteral("Folder%1/%2").arg(member.nFolderIndex).arg(fcbNameToString(pEntry + 9, i));

        if (!measureMember(&member, context.nInputSize, pPdStruct)) return false;
        if (!guardedSource) return false;
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    qint64 nEnd = context.nDirectoryOffset + context.nDirectorySize;
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        const MEMBER &member = context.listMembers.at(i);
        const qint64 nMemberEnd = member.nDataOffset + member.nCompressedSize;
        if (nMemberEnd > nEnd) nEnd = nMemberEnd;
    }
    context.nArchiveSize = qMin(nEnd, context.nInputSize);

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XNID::isValid(PDSTRUCT *pPdStruct)
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

bool XNID::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNID archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XNID::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XNID(pDevice);
}

QList<QString> XNID::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'NI'1501"));
    return listResult;
}

XBinary::FT XNID::getFileType()
{
    return FT_NID;
}

XBinary::MODE XNID::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XNID::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XNID::getArch()
{
    return QString();
}

QString XNID::getFileFormatExt()
{
    return QStringLiteral("nid");
}

QString XNID::getFileFormatExtsString()
{
    return QStringLiteral("NI install set (*.nid *.dat *.pac)");
}

QString XNID::getMIMEString()
{
    return QStringLiteral("application/x-nid");
}

QString XNID::getVersion()
{
    return QStringLiteral("1.15");
}

qint64 XNID::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XNID::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XNID::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XNID::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XNID::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDirectoryOffset + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        // A member that is one stored block needs no codec: point the generic
        // chain straight at the payload behind the block frame.
        const qint64 nStreamOffset = member.bSingleStoredBlock ? (member.nDataOffset + NID_BLOCK_FRAME_SIZE) : member.nDataOffset;
        const qint64 nStreamSize = member.bSingleStoredBlock ? member.nUncompressedSize : member.nCompressedSize;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = nStreamOffset;
            part.nFileSize = nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bSingleStoredBlock ? HANDLE_METHOD_STORE : HANDLE_METHOD_NID);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bSingleStoredBlock ? QStringLiteral("Stored") : QStringLiteral("NI blocks"));
            const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDate, member.nTime);
            if (dtMTime.isValid()) part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XNID::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XNID::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("NI install set"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
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

XBinary::ARCHIVERECORD XNID::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) return ARCHIVERECORD();

    const qint64 nStreamOffset = member.bSingleStoredBlock ? (member.nDataOffset + NID_BLOCK_FRAME_SIZE) : member.nDataOffset;
    const qint64 nStreamSize = member.bSingleStoredBlock ? member.nUncompressedSize : member.nCompressedSize;

    ARCHIVERECORD result = {};
    result.nStreamOffset = nStreamOffset;
    result.nStreamSize = nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bSingleStoredBlock ? HANDLE_METHOD_STORE : HANDLE_METHOD_NID);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bSingleStoredBlock ? QStringLiteral("Stored") : QStringLiteral("NI blocks"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDate, member.nTime);
    if (dtMTime.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    return result;
}

bool XNID::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XNID::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
