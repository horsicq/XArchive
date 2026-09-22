/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xfld.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 FLD_RECORD_SIZE = 27;
const qint64 FLD_NAME_OFFSET = 1;
const qint64 FLD_NAME_SIZE = 12;
const qint64 FLD_TRAILER_SIZE = 5;

const quint8 FLD_NAME_LENGTH_BYTE = 0x0C;
const quint8 FLD_MARKER = 0x24;  // '$'

const quint8 FLD_METHOD_STORED = 0;
const quint8 FLD_METHOD_DCL_IMPLODE = 1;

// The chain is walked in full during detection, so the member count needs a
// producer-plausible ceiling; the reference corpus tops out at 261.
const qint32 FLD_MAX_MEMBERS = 100000;

// No member in the family exceeds ~1.5 MB unpacked.  The cap only exists so a
// corrupt size field cannot become an attacker-chosen allocation downstream.
const qint64 FLD_MAX_MEMBER_SIZE = Q_INT64_C(256) * 1024 * 1024;

bool fldRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// The reference implementation rejects a record whose 12-byte name field carries any control character.
// Keeping that rule is the strongest structural constraint the format offers.
bool fldIsValidRawName(const char *pName)
{
    bool bAny = false;
    for (qint64 i = 0; i < FLD_NAME_SIZE; i++) {
        const quint8 nCharacter = static_cast<quint8>(pName[i]);
        if (nCharacter < 0x20) return false;
        if (nCharacter != 0x20) bAny = true;
    }
    return bAny;
}
}  // namespace

XFLD::XFLD(QIODevice *pDevice) : XArchive(pDevice)
{
}

XFLD::~XFLD()
{
}

// The name field is raw bytes.  Path separators and the Windows reserved
// punctuation are escaped as %XX rather than folded to '_': escaping is
// reversible and cannot collapse two distinct members onto one output file.
QString XFLD::rawNameToString(const char *pRawName, qint32 nIndex)
{
    qint32 nLength = static_cast<qint32>(FLD_NAME_SIZE);
    while ((nLength > 0) && ((pRawName[nLength - 1] == ' ') || (pRawName[nLength - 1] == '\0'))) nLength--;

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

XBinary::HANDLE_METHOD XFLD::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == FLD_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == FLD_METHOD_DCL_IMPLODE) {
        // The payload keeps its own two-byte DCL prelude, which decPkwareDcl
        // reads off the front of the packed buffer itself.
        return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    }
    return HANDLE_METHOD_UNKNOWN;
}

QString XFLD::methodToString(quint8 nMethod)
{
    if (nMethod == FLD_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == FLD_METHOD_DCL_IMPLODE) return QStringLiteral("PKWARE DCL implode");
    return QStringLiteral("Unknown");
}

bool XFLD::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = getSize();
    if (!guardedSource) return false;
    if (context.nInputSize < FLD_RECORD_SIZE + FLD_TRAILER_SIZE) return false;

    qint64 nOffset = 0;
    while (nOffset + FLD_RECORD_SIZE <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= FLD_MAX_MEMBERS) return false;

        const QByteArray baRecord = read_array_process(nOffset, FLD_RECORD_SIZE, pPdStruct);
        if ((baRecord.size() != FLD_RECORD_SIZE)) return false;
        const char *pRecord = baRecord.constData();
        const uchar *pRaw = reinterpret_cast<const uchar *>(pRecord);

        if (pRaw[0] != FLD_NAME_LENGTH_BYTE) break;
        if (pRaw[0x1A] != FLD_MARKER) break;
        if (!fldIsValidRawName(pRecord + FLD_NAME_OFFSET)) break;

        const qint64 nCompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x0D));
        const qint64 nUncompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRaw + 0x11));
        const quint16 nDosTime = qFromLittleEndian<quint16>(pRaw + 0x15);
        const quint16 nDosDate = qFromLittleEndian<quint16>(pRaw + 0x17);
        const quint8 nMethod = pRaw[0x19];

        if ((nCompressedSize < 0) || (nUncompressedSize < 0)) break;
        if ((nCompressedSize > FLD_MAX_MEMBER_SIZE) || (nUncompressedSize > FLD_MAX_MEMBER_SIZE)) break;
        if (methodToHandleMethod(nMethod) == HANDLE_METHOD_UNKNOWN) break;
        if ((nMethod == FLD_METHOD_STORED) && (nCompressedSize != nUncompressedSize)) break;

        const qint64 nDataOffset = nOffset + FLD_RECORD_SIZE;
        if (!fldRangeWithin(context.nInputSize, nDataOffset, nCompressedSize)) break;

        if (nMethod == FLD_METHOD_DCL_IMPLODE) {
            // A DCL implode stream opens with a literal-mode byte in {0,1} and a
            // dictionary-size byte in {4,5,6}. The reference implementation gates its own detector on
            // exactly these two bytes of the first member.
            if (nCompressedSize < 3) break;
            const QByteArray baPrelude = read_array_process(nDataOffset, 2, pPdStruct);
            if ((baPrelude.size() != 2)) return false;
            const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
            const quint8 nDictionaryBits = static_cast<quint8>(baPrelude.at(1));
            if (nLiteralMode > 1) break;
            if ((nDictionaryBits < 4) || (nDictionaryBits > 6)) break;
        }

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nDosDateTime = (static_cast<quint32>(nDosDate) << 16) | static_cast<quint32>(nDosTime);
        member.nMethod = nMethod;
        member.sFileName = rawNameToString(pRecord + FLD_NAME_OFFSET, context.listMembers.size());
        context.listMembers.append(member);

        nOffset = nDataOffset + nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;

    // The chain has to tile the container exactly.  Every archive in the
    // reference corpus closes with a 5-byte trailer; nothing else is tolerated,
    // because without a magic number this is the only thing that separates a
    // real .FLD from a coincidence.
    if (nOffset == context.nInputSize) {
        context.nTrailerOffset = context.nInputSize;
    } else if (nOffset + FLD_TRAILER_SIZE == context.nInputSize) {
        context.nTrailerOffset = nOffset;
    } else {
        return false;
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XFLD::isValid(PDSTRUCT *pPdStruct)
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

bool XFLD::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFLD archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XFLD::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFLD(pDevice);
}

XBinary::FT XFLD::getFileType()
{
    return FT_FLD;
}

XBinary::MODE XFLD::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XFLD::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XFLD::getArch()
{
    return QString();
}

QString XFLD::getFileFormatExt()
{
    return QStringLiteral("fld");
}

QString XFLD::getFileFormatExtsString()
{
    return QStringLiteral("CodeBase install file group (*.fld)");
}

QString XFLD::getMIMEString()
{
    return QStringLiteral("application/x-fld");
}

QString XFLD::getVersion()
{
    return QString();
}

qint64 XFLD::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XFLD::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XFLD::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XFLD::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XFLD::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = FLD_RECORD_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
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
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = FLD_RECORD_SIZE + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_HEADER) && (context.nTrailerOffset < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nTrailerOffset;
        part.nFileSize = context.nInputSize - context.nTrailerOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Trailer");
        listResult.append(part);
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

QMap<XBinary::UNPACK_PROP, QVariant> XFLD::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XFLD::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("CodeBase file group"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
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

XBinary::ARCHIVERECORD XFLD::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    const quint16 nDosDate = static_cast<quint16>(member.nDosDateTime >> 16);
    const quint16 nDosTime = static_cast<quint16>(member.nDosDateTime & 0xFFFFU);
    const QDateTime dtMTime(QDate(((nDosDate >> 9) & 0x7F) + 1980, (nDosDate >> 5) & 0x0F, nDosDate & 0x1F),
                            QTime((nDosTime >> 11) & 0x1F, (nDosTime >> 5) & 0x3F, (nDosTime & 0x1F) * 2));
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XFLD::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->nTrailerOffset;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XFLD::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
