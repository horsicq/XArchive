/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xgob2.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 GOB2_HEADER_SIZE = 0x10;
const qint64 GOB2_ENTRY_SIZE = 0x88;
const qint64 GOB2_NAME_FIELD_SIZE = 0x80;
const qint64 GOB2_NAME_FIELD_OFFSET = 8;
const quint32 GOB2_VERSION = 20;
const quint32 GOB2_INDEX_OFFSET = 0x0C;
// A directory entry costs 0x88 bytes, so this ceiling is far above any real
// producer and still keeps a corrupt count from allocating wildly.
const qint32 GOB2_MAX_ENTRIES = 1000000;

bool gob2RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XGOB2::XGOB2(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGOB2::~XGOB2()
{
}

// Names are relative DOS-style paths ("3do\00crte.3do").  The separator is
// normalized to '/', everything the host filesystem cannot represent is escaped
// as %XX: escaping is reversible and, unlike folding to '_', cannot collapse
// two distinct members onto one output file.
QString XGOB2::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(GOB2_NAME_FIELD_SIZE)) && (pRawName[nLength] != '\0')) nLength++;

    QString sResult;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRawName[i]);
        if ((nCharacter == '\\') || (nCharacter == '/')) {
            sResult.append(QLatin1Char('/'));
            continue;
        }
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != ':') && (nCharacter != '*') &&
                           (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else if (nCharacter == 0x20) {
            sResult.append(QLatin1Char(' '));
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

bool XGOB2::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < GOB2_HEADER_SIZE + GOB2_ENTRY_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, GOB2_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != GOB2_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (memcmp(baHeader.constData(), "GOB ", 4) != 0) return false;
    const quint32 nVersion = qFromLittleEndian<quint32>(pHeader + 4);
    // Version 20 is the only shape this layout describes, and the index offset
    // is what places the entry count at 0x0C; the reference extractor assumes
    // both unconditionally, so anything else has to be refused rather than
    // silently misread.
    if (nVersion != GOB2_VERSION) return false;
    if (qFromLittleEndian<quint32>(pHeader + 8) != GOB2_INDEX_OFFSET) return false;
    context.nVersion = static_cast<qint32>(nVersion);

    const qint32 nNumberOfEntries = qFromLittleEndian<qint32>(pHeader + GOB2_INDEX_OFFSET);
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > GOB2_MAX_ENTRIES)) return false;

    const qint64 nDirectorySize = static_cast<qint64>(nNumberOfEntries) * GOB2_ENTRY_SIZE;
    if (nDirectorySize > context.nInputSize - GOB2_HEADER_SIZE) return false;

    const QByteArray baDirectory = read_array_process(GOB2_HEADER_SIZE, nDirectorySize, pPdStruct);
    if (!guardedSource || (baDirectory.size() != nDirectorySize)) return false;
    const char *pDirectory = baDirectory.constData();

    qint64 nEnd = GOB2_HEADER_SIZE + nDirectorySize;
    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pDirectory + (GOB2_ENTRY_SIZE * i);
        const qint64 nOffset = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry)));
        const qint64 nSize = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pEntry) + 4));
        // Members live behind the directory and must fit inside the file; this
        // is what makes a four-byte magic safe to detect on.
        if ((nOffset < GOB2_HEADER_SIZE + nDirectorySize) || (nSize < 0)) return false;
        if (!gob2RangeWithin(context.nInputSize, nOffset, nSize)) return false;
        if (pEntry[GOB2_NAME_FIELD_OFFSET] == '\0') return false;

        MEMBER member = {};
        member.nDataOffset = nOffset;
        member.nSize = nSize;
        member.sFileName = rawNameToString(pEntry + GOB2_NAME_FIELD_OFFSET, i);
        context.listMembers.append(member);
        if (nOffset + nSize > nEnd) nEnd = nOffset + nSize;
    }

    context.nArchiveSize = nEnd;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XGOB2::isValid(PDSTRUCT *pPdStruct)
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

bool XGOB2::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGOB2 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGOB2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGOB2(pDevice);
}

QList<QString> XGOB2::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'GOB '14000000"));
    return listResult;
}

XBinary::FT XGOB2::getFileType()
{
    return FT_GOB2;
}

XBinary::MODE XGOB2::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGOB2::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGOB2::getArch()
{
    return QString();
}

QString XGOB2::getFileFormatExt()
{
    return QStringLiteral("gob");
}

QString XGOB2::getFileFormatExtsString()
{
    return QStringLiteral("LucasArts GOB archive (*.gob)");
}

QString XGOB2::getMIMEString()
{
    return QStringLiteral("application/x-gob");
}

QString XGOB2::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XGOB2::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGOB2::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XGOB2::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XGOB2::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XGOB2::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = GOB2_HEADER_SIZE + static_cast<qint64>(context.listMembers.size()) * GOB2_ENTRY_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XGOB2::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGOB2::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("LucasArts GOB archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
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

XBinary::ARCHIVERECORD XGOB2::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XGOB2::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XGOB2::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
