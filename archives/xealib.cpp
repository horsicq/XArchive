/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xealib.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 EALIB_HEADER_SIZE = 7;
const qint64 EALIB_ENTRY_SIZE = 18;
const qint64 EALIB_NAME_SIZE = 13;
const qint64 EALIB_METHOD_OFFSET = 13;
const qint64 EALIB_OFFSET_OFFSET = 14;
// The count field is 16 bit, so 65535 members plus the sentinel is the hard
// producer limit.
const qint32 EALIB_MAX_ENTRIES = 65535;

bool ealibRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// The 13-byte name field is NUL padded and byte 12 is always the terminator.
// Every one of the 5907 members of the reference corpus is plain printable
// ASCII with clean padding; requiring that is one of the structural rules that
// stops random data from parsing as a directory.
bool ealibIsValidRawName(const char *pName)
{
    if (pName[0] == 0) return false;
    bool bPadding = false;
    for (qint64 i = 0; i < EALIB_NAME_SIZE; i++) {
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
}  // namespace

XEALIB::XEALIB(QIODevice *pDevice) : XArchive(pDevice)
{
}

XEALIB::~XEALIB()
{
}

// Names are DOS 8.3 identifiers throughout the reference corpus, but the field
// is raw bytes, so path separators and the Windows reserved punctuation are
// escaped as %XX rather than folded to '_': escaping is reversible and cannot
// collapse two distinct members onto one output file.
QString XEALIB::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < static_cast<qint32>(EALIB_NAME_SIZE)) && (pRawName[nLength] != '\0')) nLength++;
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

XBinary::HANDLE_METHOD XEALIB::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == 1) return HANDLE_METHOD_EALIB;
    if (nMethod == 4) return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    return HANDLE_METHOD_STORE;  // 0 and 3
}

QString XEALIB::methodToString(quint8 nMethod)
{
    if (nMethod == 1) return QStringLiteral("LZSS");
    if (nMethod == 4) return QStringLiteral("PKWARE DCL implode");
    return QStringLiteral("Stored");
}

bool XEALIB::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Header + one member entry + the sentinel entry.
    if (context.nInputSize < EALIB_HEADER_SIZE + 2 * EALIB_ENTRY_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, EALIB_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != EALIB_HEADER_SIZE)) return false;
    if (memcmp(baHeader.constData(), "EALIB", 5) != 0) return false;

    const qint32 nNumberOfEntries = static_cast<qint32>(qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baHeader.constData()) + 5));
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > EALIB_MAX_ENTRIES)) return false;
    context.nNumberOfEntries = nNumberOfEntries;

    // The directory carries one sentinel entry behind the last member; it only
    // exists to supply the end offset of that member.
    context.nDirectoryOffset = EALIB_HEADER_SIZE;
    context.nDirectorySize = static_cast<qint64>(nNumberOfEntries + 1) * EALIB_ENTRY_SIZE;
    if (!ealibRangeWithin(context.nInputSize, context.nDirectoryOffset, context.nDirectorySize)) return false;

    const QByteArray baDirectory = read_array_process(context.nDirectoryOffset, context.nDirectorySize, pPdStruct);
    if ((baDirectory.size() != context.nDirectorySize)) return false;
    const char *pDirectory = baDirectory.constData();

    QList<qint64> listOffsets;
    QList<quint8> listMethods;
    for (qint32 i = 0; i <= nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pDirectory + (EALIB_ENTRY_SIZE * i);
        // The reference detector requires the terminator byte of the name field
        // and treats the offset as a signed 32 bit quantity.
        if (pEntry[EALIB_NAME_SIZE - 1] != 0) return false;
        const qint32 nOffset = static_cast<qint32>(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pEntry) + EALIB_OFFSET_OFFSET));
        if (nOffset < 0) return false;
        listOffsets.append(static_cast<qint64>(nOffset));
        listMethods.append(static_cast<quint8>(pEntry[EALIB_METHOD_OFFSET]));
    }

    // Header, directory and payload tile the file exactly: the first entry
    // starts right behind the directory and the sentinel marks the end of file.
    // This is what makes the five-byte magic safe to detect on.
    if (listOffsets.at(0) != context.nDirectoryOffset + context.nDirectorySize) return false;
    if (listOffsets.at(nNumberOfEntries) != context.nInputSize) return false;

    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const char *pEntry = pDirectory + (EALIB_ENTRY_SIZE * i);
        const qint64 nOffset = listOffsets.at(i);
        const qint64 nSize = listOffsets.at(i + 1) - nOffset;
        if (nSize < 0) return false;
        if (!ealibRangeWithin(context.nInputSize, nOffset, nSize)) return false;
        if (!ealibIsValidRawName(pEntry)) return false;

        MEMBER member = {};
        member.nRecordOffset = context.nDirectoryOffset + (EALIB_ENTRY_SIZE * i);
        member.nMethod = listMethods.at(i);
        member.sFileName = rawNameToString(pEntry, i);

        if ((member.nMethod == 0) || (member.nMethod == 3)) {
            member.nDataOffset = nOffset;
            member.nCompressedSize = nSize;
            member.nUncompressedSize = nSize;
        } else if ((member.nMethod == 1) || (member.nMethod == 4)) {
            // A compressed member starts with a little endian quint32
            // uncompressed size; the codec stream begins behind it.
            if (nSize < 4) return false;
            const QByteArray baSize = read_array_process(nOffset, 4, pPdStruct);
            if ((baSize.size() != 4)) return false;
            const qint32 nUncompressedSize = static_cast<qint32>(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baSize.constData())));
            if (nUncompressedSize < 0) return false;
            member.nDataOffset = nOffset + 4;
            member.nCompressedSize = nSize - 4;
            member.nUncompressedSize = static_cast<qint64>(nUncompressedSize);
        } else {
            // Method 2 is rejected by the reference extractor as well; refusing
            // the whole archive is better than emitting garbage for one member.
            return false;
        }
        context.listMembers.append(member);
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XEALIB::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    guardedSource->seek(nSavedPosition);
    return bResult;
}

bool XEALIB::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEALIB archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XEALIB::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XEALIB(pDevice);
}

QList<QString> XEALIB::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'EALIB'"));
    return listResult;
}

XBinary::FT XEALIB::getFileType()
{
    return FT_EALIB;
}

XBinary::MODE XEALIB::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEALIB::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XEALIB::getArch()
{
    return QString();
}

QString XEALIB::getFileFormatExt()
{
    return QStringLiteral("lib");
}

QString XEALIB::getFileFormatExtsString()
{
    return QStringLiteral("EALIB library (*.lib)");
}

QString XEALIB::getMIMEString()
{
    return QStringLiteral("application/x-ealib");
}

QString XEALIB::getVersion()
{
    return QString();
}

qint64 XEALIB::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XEALIB::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XEALIB::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XEALIB::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XEALIB::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = EALIB_HEADER_SIZE + context.nDirectorySize;
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
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
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

QMap<XBinary::UNPACK_PROP, QVariant> XEALIB::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEALIB::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("EALIB library"));
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

XBinary::ARCHIVERECORD XEALIB::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The container carries no CRC of any kind; the only integrity check it
    // offers is the offset arithmetic parseContext() already did.
    return result;
}

bool XEALIB::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XEALIB::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
