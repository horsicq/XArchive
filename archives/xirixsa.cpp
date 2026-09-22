/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xirixsa.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 IRIXSA_BLOCK_SIZE = 512;
const qint64 IRIXSA_HEADER_SIZE = 512;
const qint64 IRIXSA_DIRECTORY_OFFSET = 0x20;
const qint32 IRIXSA_NUMBER_OF_SLOTS = 20;
const qint64 IRIXSA_ENTRY_SIZE = 24;
const qint64 IRIXSA_NAME_SIZE = 16;
const quint32 IRIXSA_MAGIC = 0xACED1234U;

bool irixsaRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// The 16-byte name field is NUL padded.  A NUL followed by a non-NUL never
// happens in a real directory slot, and refusing that shape is one of the
// structural rules that stops random data from parsing as a directory.
bool irixsaIsValidRawName(const char *pName)
{
    if (pName[0] == 0) return false;
    bool bPadding = false;
    for (qint64 i = 0; i < IRIXSA_NAME_SIZE; i++) {
        const quint8 nCharacter = static_cast<quint8>(pName[i]);
        if (nCharacter == 0) {
            bPadding = true;
        } else if (bPadding) {
            return false;
        } else if ((nCharacter < 0x20) || (nCharacter > 0x7e)) {
            return false;
        }
    }
    return true;
}

bool irixsaIsEmptySlot(const char *pEntry)
{
    for (qint64 i = 0; i < IRIXSA_ENTRY_SIZE; i++) {
        if (pEntry[i] != 0) return false;
    }
    return true;
}
}  // namespace

XIRIXSA::XIRIXSA(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIRIXSA::~XIRIXSA()
{
}

// Names are plain IRIX identifiers ("sash.IP22", "fx.ARCS") in the whole
// reference corpus, but the field is raw bytes, so path separators and the
// Windows reserved punctuation are escaped as %XX rather than folded to '_':
// escaping is reversible and cannot collapse two distinct members onto one
// output file.
QString XIRIXSA::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(IRIXSA_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;
    while ((nLength > 0) && (pRawName[nLength - 1] == ' ')) nLength--;

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

bool XIRIXSA::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // The header block plus at least one block of payload.
    if (context.nInputSize < IRIXSA_HEADER_SIZE + IRIXSA_BLOCK_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, IRIXSA_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != IRIXSA_HEADER_SIZE)) return false;
    const char *pHeader = baHeader.constData();
    const uchar *pRaw = reinterpret_cast<const uchar *>(pHeader);

    if (qFromBigEndian<quint32>(pRaw) != IRIXSA_MAGIC) return false;

    // The reference tool refuses the file unless the big-endian u32 sum of the
    // whole 512-byte header block is zero; the u32 at +4 is the value that
    // makes that true.  This is what makes the 4-byte magic safe to detect on.
    quint32 nSum = 0;
    for (qint64 i = 0; i < IRIXSA_HEADER_SIZE; i += 4) {
        nSum += qFromBigEndian<quint32>(pRaw + i);
    }
    if (nSum != 0) return false;

    // The first slot always starts at block 1, right behind the header.
    if (qFromBigEndian<quint32>(pRaw + IRIXSA_DIRECTORY_OFFSET + IRIXSA_NAME_SIZE) != 1U) return false;

    qint64 nEndOfData = IRIXSA_HEADER_SIZE;
    for (qint32 i = 0; i < IRIXSA_NUMBER_OF_SLOTS; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pHeader + IRIXSA_DIRECTORY_OFFSET + (IRIXSA_ENTRY_SIZE * i);
        // Unused slots are zeroed and simply skipped; used slots may follow
        // them, so the walk never stops early.
        if (irixsaIsEmptySlot(pEntry)) continue;
        if (!irixsaIsValidRawName(pEntry)) return false;

        const quint32 nBlock = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pEntry) + IRIXSA_NAME_SIZE);
        const quint32 nSize = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(pEntry) + IRIXSA_NAME_SIZE + 4);
        if (nBlock == 0) return false;
        if (nSize == 0) return false;

        MEMBER member = {};
        member.nBlock = nBlock;
        member.nDataOffset = static_cast<qint64>(nBlock) * IRIXSA_BLOCK_SIZE;
        member.nSize = static_cast<qint64>(nSize);
        if (!irixsaRangeWithin(context.nInputSize, member.nDataOffset, member.nSize)) return false;
        member.sFileName = rawNameToString(pEntry, i);
        context.listMembers.append(member);

        const qint64 nEnd = member.nDataOffset + member.nSize;
        if (nEnd > nEndOfData) nEndOfData = nEnd;
    }

    if (context.listMembers.isEmpty()) return false;

    // The volume is padded out to a whole block.
    context.nArchiveSize = ((nEndOfData + IRIXSA_BLOCK_SIZE - 1) / IRIXSA_BLOCK_SIZE) * IRIXSA_BLOCK_SIZE;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XIRIXSA::isValid(PDSTRUCT *pPdStruct)
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

bool XIRIXSA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIRIXSA archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIRIXSA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIRIXSA(pDevice);
}

QList<QString> XIRIXSA::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("ACED1234"));
    return listResult;
}

XBinary::FT XIRIXSA::getFileType()
{
    return FT_IRIX_SA;
}

XBinary::MODE XIRIXSA::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIRIXSA::getEndian()
{
    return ENDIAN_BIG;
}

QString XIRIXSA::getArch()
{
    return QString();
}

QString XIRIXSA::getFileFormatExt()
{
    return QString();
}

QString XIRIXSA::getFileFormatExtsString()
{
    return QStringLiteral("IRIX standalone tools volume (sa)");
}

QString XIRIXSA::getMIMEString()
{
    return QStringLiteral("application/x-irix-sa");
}

QString XIRIXSA::getVersion()
{
    return QString();
}

qint64 XIRIXSA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XIRIXSA::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIRIXSA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XIRIXSA::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIRIXSA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = IRIXSA_HEADER_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XIRIXSA::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIRIXSA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("IRIX standalone tools volume"));
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

XBinary::ARCHIVERECORD XIRIXSA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    // Members are raw ECOFF MIPS images; the volume never compresses.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XIRIXSA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XIRIXSA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
