/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xjam.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 JAM_MAGIC_SIZE = 3;
const qint64 JAM_ROOT_OFFSET = 3;
const qint64 JAM_NAME_SIZE = 15;
const qint64 JAM_FILE_ENTRY_SIZE = 23;       // 15 name + 4 offset + 4 size
const qint64 JAM_DIRECTORY_ENTRY_SIZE = 19;  // 15 name + 4 node offset
const qint64 JAM_COUNT_SIZE = 4;

// The corpus tops out at 2470 members and a two-level tree; both caps only
// bound a malformed directory.
const qint32 JAM_MAX_MEMBERS = 1000000;
const qint32 JAM_MAX_DEPTH = 32;

bool jamRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// The reference validator: every byte up to the first NUL must be > 0x20, and
// everything from the NUL to the end of the 15-byte field must be NUL.  A field
// with no NUL at all is rejected.
bool jamIsValidRawName(const char *pName)
{
    qint64 i = 0;
    while (i < JAM_NAME_SIZE) {
        const quint8 nCharacter = static_cast<quint8>(pName[i]);
        if (nCharacter == 0) break;
        if (nCharacter < 0x21) return false;
        i++;
    }
    if (i >= JAM_NAME_SIZE) return false;  // no terminator
    for (; i < JAM_NAME_SIZE; i++) {
        if (pName[i] != 0) return false;
    }
    return true;
}
}  // namespace

XJAM::XJAM(QIODevice *pDevice) : XArchive(pDevice)
{
}

XJAM::~XJAM()
{
}

// Names are DOS 8.3 identifiers throughout the corpus, but the field is raw
// bytes: path separators and Windows reserved punctuation are escaped as %XX
// rather than folded to '_', because escaping is reversible and cannot collapse
// two distinct members onto one output file.
QString XJAM::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(JAM_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;

    QString sResult;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
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

bool XJAM::walkNode(CONTEXT *pContext, qint64 nNodeOffset, const QString &sPrefix, QSet<qint64> *pSetVisited, qint32 nDepth, PDSTRUCT *pPdStruct)
{
    if (!pContext || !pSetVisited) return false;
    if (nDepth > JAM_MAX_DEPTH) return false;
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    // Node offsets are absolute, so a malformed archive could point a
    // subdirectory back at an ancestor; the reference reader guards the same way.
    if (pSetVisited->contains(nNodeOffset)) return false;
    pSetVisited->insert(nNodeOffset);

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    if (!jamRangeWithin(pContext->nInputSize, nNodeOffset, JAM_COUNT_SIZE)) return false;

    QByteArray baCount = read_array_process(nNodeOffset, JAM_COUNT_SIZE, pPdStruct);
    if ((baCount.size() != JAM_COUNT_SIZE)) return false;
    const qint64 nNumberOfFiles = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baCount.constData())));
    if (nNumberOfFiles < 0) return false;

    qint64 nOffset = nNodeOffset + JAM_COUNT_SIZE;
    const qint64 nFileTableSize = nNumberOfFiles * JAM_FILE_ENTRY_SIZE;
    if (!jamRangeWithin(pContext->nInputSize, nOffset, nFileTableSize)) return false;

    if (nNumberOfFiles > 0) {
        const QByteArray baTable = read_array_process(nOffset, nFileTableSize, pPdStruct);
        if ((static_cast<qint64>(baTable.size()) != nFileTableSize)) return false;
        for (qint64 i = 0; i < nNumberOfFiles; i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (pContext->listMembers.size() >= JAM_MAX_MEMBERS) return false;
            const char *pEntry = baTable.constData() + (i * JAM_FILE_ENTRY_SIZE);
            const uchar *pRaw = reinterpret_cast<const uchar *>(pEntry);
            if (!jamIsValidRawName(pEntry)) return false;
            const qint64 nDataOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + JAM_NAME_SIZE));
            const qint64 nSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + JAM_NAME_SIZE + 4));
            if ((nDataOffset < JAM_MAGIC_SIZE) || (nSize < 0)) return false;
            if (!jamRangeWithin(pContext->nInputSize, nDataOffset, nSize)) return false;

            MEMBER member = {};
            member.nDataOffset = nDataOffset;
            member.nSize = nSize;
            member.sFileName = sPrefix + rawNameToString(pEntry, pContext->listMembers.size());
            pContext->listMembers.append(member);
        }
        nOffset += nFileTableSize;
    }

    if (!jamRangeWithin(pContext->nInputSize, nOffset, JAM_COUNT_SIZE)) return false;
    baCount = read_array_process(nOffset, JAM_COUNT_SIZE, pPdStruct);
    if ((baCount.size() != JAM_COUNT_SIZE)) return false;
    const qint64 nNumberOfDirectories = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baCount.constData())));
    if (nNumberOfDirectories < 0) return false;
    nOffset += JAM_COUNT_SIZE;

    if (nNumberOfDirectories > 0) {
        const qint64 nDirectoryTableSize = nNumberOfDirectories * JAM_DIRECTORY_ENTRY_SIZE;
        if (!jamRangeWithin(pContext->nInputSize, nOffset, nDirectoryTableSize)) return false;
        const QByteArray baTable = read_array_process(nOffset, nDirectoryTableSize, pPdStruct);
        if ((static_cast<qint64>(baTable.size()) != nDirectoryTableSize)) return false;
        for (qint64 i = 0; i < nNumberOfDirectories; i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            const char *pEntry = baTable.constData() + (i * JAM_DIRECTORY_ENTRY_SIZE);
            const uchar *pRaw = reinterpret_cast<const uchar *>(pEntry);
            if (!jamIsValidRawName(pEntry)) return false;
            const qint64 nChildOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + JAM_NAME_SIZE));
            if (nChildOffset < JAM_MAGIC_SIZE) return false;
            const QString sChildPrefix = sPrefix + rawNameToString(pEntry, static_cast<qint32>(i)) + QLatin1Char('/');
            if (!walkNode(pContext, nChildOffset, sChildPrefix, pSetVisited, nDepth + 1, pPdStruct)) return false;
        }
    }

    return true;
}

bool XJAM::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = getSize();
    if (!guardedSource) return false;
    // Magic, the root file count, and at least one complete file record.
    if (context.nInputSize < JAM_ROOT_OFFSET + JAM_COUNT_SIZE + JAM_FILE_ENTRY_SIZE + JAM_COUNT_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, JAM_ROOT_OFFSET + JAM_COUNT_SIZE + JAM_FILE_ENTRY_SIZE, pPdStruct);
    if ((baHeader.size() != JAM_ROOT_OFFSET + JAM_COUNT_SIZE + JAM_FILE_ENTRY_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if ((pHeader[0] != 'J') || (pHeader[1] != 'A') || (pHeader[2] != 'M')) return false;
    if (qFromLittleEndian<qint32>(pHeader + 3) <= 0) return false;
    // The first member's offset and size, checked exactly as the reference
    // detector does: a real archive always starts its data area on a 256-byte
    // boundary, which is what makes a three-byte magic safe to gate on.
    const quint32 nFirstOffset = qFromLittleEndian<quint32>(pHeader + 0x16);
    const qint32 nFirstSize = qFromLittleEndian<qint32>(pHeader + 0x1a);
    if ((nFirstOffset == 0) || (nFirstOffset > 0x7fffffffU)) return false;
    if ((nFirstOffset & 0xffU) != 0) return false;
    if (nFirstSize <= 0) return false;
    if (!jamIsValidRawName(baHeader.constData() + 7)) return false;

    QSet<qint64> setVisited;
    if (!walkNode(&context, JAM_ROOT_OFFSET, QString(), &setVisited, 0, pPdStruct)) return false;
    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XJAM::isValid(PDSTRUCT *pPdStruct)
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

bool XJAM::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XJAM archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XJAM::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XJAM(pDevice);
}

QList<QString> XJAM::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'JAM'"));
    return listResult;
}

XBinary::FT XJAM::getFileType()
{
    return FT_JAM;
}

XBinary::MODE XJAM::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XJAM::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XJAM::getArch()
{
    return QString();
}

QString XJAM::getFileFormatExt()
{
    return QStringLiteral("jam");
}

QString XJAM::getFileFormatExtsString()
{
    return QStringLiteral("JAM resource archive (*.jam)");
}

QString XJAM::getMIMEString()
{
    return QStringLiteral("application/x-jam");
}

QString XJAM::getVersion()
{
    return QString();
}

qint64 XJAM::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XJAM::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XJAM::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XJAM::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XJAM::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = JAM_MAGIC_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XJAM::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XJAM::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("JAM resource archive"));
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

XBinary::ARCHIVERECORD XJAM::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    return result;
}

bool XJAM::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XJAM::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
