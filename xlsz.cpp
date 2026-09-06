/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlsz.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 LSZ_HEADER_SIZE = 6;
const qint64 LSZ_RECORD_SIZE = 51;
const qint64 LSZ_NAME_SIZE = 13;
// 37 F0 FF FF | 00 03
const quint32 LSZ_MAGIC = 0xFFFFF037U;
const quint16 LSZ_VERSION = 0x0300U;
// The per-record tag repeats the container magic in one writer and is left zero
// in the other; both spellings appear inside a single corpus and U3 accepts
// exactly these two.
const quint32 LSZ_RECORD_TAG_A = 0xFFFF037FU;
const quint32 LSZ_RECORD_TAG_B = 0x00000000U;

const quint16 LSZ_METHOD_STORED = 1U;
const quint16 LSZ_METHOD_IMPLODE = 2U;
const quint16 LSZ_METHOD_EMPTY = 6U;

// A PKWARE DCL stream cannot be shorter than its two prelude bytes plus one
// byte holding the start of the end-of-stream code.
const qint64 LSZ_MIN_DCL_SIZE = 3;
const quint8 LSZ_DCL_MAX_LITERAL_MODE = 1U;
const quint8 LSZ_DCL_MIN_DICT_BITS = 4U;
const quint8 LSZ_DCL_MAX_DICT_BITS = 6U;

// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext of a
// DCL member is driven by the bitstream, so the decoder must stay bounded.
const qint64 LSZ_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// No member count is stored, so this is a runaway guard, not a format limit.
// The largest reference archive holds 67 members.
const qint32 LSZ_MAX_MEMBERS = 65535;

bool lszIsHeader(const QByteArray &baHeader)
{
    if (baHeader.size() != LSZ_HEADER_SIZE) return false;
    const uchar *pData = reinterpret_cast<const uchar *>(baHeader.constData());
    return (qFromLittleEndian<quint32>(pData) == LSZ_MAGIC) &&
           (qFromLittleEndian<quint16>(pData + 4) == LSZ_VERSION);
}

// The 13-byte field is a fixed, NUL-padded 8.3 buffer.  Read the FULL field and
// stop at the first NUL; a stale byte behind the terminator would mean this is
// not the buffer the parser assumes, so reject rather than guess.
bool lszIsNameField(const QByteArray &baField, QString *pName)
{
    if (baField.size() != LSZ_NAME_SIZE) return false;

    const int nTerminator = baField.indexOf('\0');
    if (nTerminator <= 0) return false;
    for (int i = nTerminator; i < baField.size(); i++) {
        if (baField.at(i) != '\0') return false;
    }
    for (int i = 0; i < nTerminator; i++) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if ((nCharacter < 0x20U) || (nCharacter > 0x7eU)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') ||
            (nCharacter == ':') || (nCharacter == '*') ||
            (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') ||
            (nCharacter == '|')) {
            return false;
        }
    }

    const QString sName =
        QString::fromLatin1(baField.constData(), nTerminator).trimmed();
    if (sName.isEmpty() || (sName == QLatin1String(".")) ||
        (sName == QLatin1String(".."))) {
        return false;
    }

    if (pName) *pName = sName;
    return true;
}

bool lszIsDclPrelude(const QByteArray &baPrelude)
{
    if (baPrelude.size() < 2) return false;
    const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
    return (nLiteralMode <= LSZ_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= LSZ_DCL_MIN_DICT_BITS) &&
           (nDictBits <= LSZ_DCL_MAX_DICT_BITS);
}
}  // namespace

XLSZ::XLSZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLSZ::~XLSZ()
{
}

bool XLSZ::parseRecord(const QByteArray &baRecord, MEMBER *pMember)
{
    if (!pMember || (baRecord.size() != LSZ_RECORD_SIZE)) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baRecord.constData());

    const quint32 nTag = qFromLittleEndian<quint32>(pData);
    if ((nTag != LSZ_RECORD_TAG_A) && (nTag != LSZ_RECORD_TAG_B)) return false;

    MEMBER member = {};
    if (!lszIsNameField(baRecord.mid(4, static_cast<int>(LSZ_NAME_SIZE)),
                        &member.sFileName)) {
        return false;
    }
    // Reserved fields are zero in every member of the reference corpus; they
    // are the cheapest way to keep a random 37 F0 FF FF 00 03 hit out.
    if (qFromLittleEndian<quint16>(pData + 0x11) != 0U) return false;
    for (int i = 0x27; i < LSZ_RECORD_SIZE; i++) {
        if (baRecord.at(i) != '\0') return false;
    }

    const qint32 nUncompressedSize = qFromLittleEndian<qint32>(pData + 0x13);
    const qint32 nCompressedSize = qFromLittleEndian<qint32>(pData + 0x17);
    if ((nUncompressedSize < 0) || (nCompressedSize < 0)) return false;

    member.nUncompressedSize = nUncompressedSize;
    member.nCompressedSize = nCompressedSize;
    member.nChecksum = qFromLittleEndian<quint32>(pData + 0x1b);
    member.nDosTime = qFromLittleEndian<quint16>(pData + 0x1f);
    member.nDosDate = qFromLittleEndian<quint16>(pData + 0x21);
    member.nAttributes = qFromLittleEndian<quint16>(pData + 0x23);
    member.nMethod = qFromLittleEndian<quint16>(pData + 0x25);

    if (member.nMethod == LSZ_METHOD_STORED) {
        if (member.nCompressedSize != member.nUncompressedSize) return false;
    } else if (member.nMethod == LSZ_METHOD_IMPLODE) {
        if ((member.nCompressedSize < LSZ_MIN_DCL_SIZE) ||
            (member.nUncompressedSize < 1) ||
            (member.nUncompressedSize > LSZ_MAX_UNCOMPRESSED_SIZE)) {
            return false;
        }
    } else if (member.nMethod == LSZ_METHOD_EMPTY) {
        // Directory-only archives are built entirely out of these; both sizes
        // must be zero or this is not a method 6 record.
        if ((member.nCompressedSize != 0) || (member.nUncompressedSize != 0)) {
            return false;
        }
    } else {
        return false;
    }

    *pMember = member;
    return true;
}

bool XLSZ::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XLSZ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < LSZ_HEADER_SIZE + LSZ_RECORD_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, LSZ_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || !lszIsHeader(baHeader)) return false;

    qint64 nOffset = LSZ_HEADER_SIZE;
    while (nOffset < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= LSZ_MAX_MEMBERS) return false;
        if (LSZ_RECORD_SIZE > context.nInputSize - nOffset) return false;

        const QByteArray baRecord =
            read_array_process(nOffset, LSZ_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baRecord.size() != LSZ_RECORD_SIZE)) {
            return false;
        }

        MEMBER member = {};
        if (!parseRecord(baRecord, &member)) return false;

        member.nRecordOffset = nOffset;
        member.nDataOffset = nOffset + LSZ_RECORD_SIZE;
        if (member.nCompressedSize >
            context.nInputSize - member.nDataOffset) {
            return false;
        }

        if (member.nMethod == LSZ_METHOD_IMPLODE) {
            const QByteArray baPrelude =
                read_array_process(member.nDataOffset, 2, pPdStruct);
            if (!guardedThis || !guardedSource || (baPrelude.size() != 2) ||
                !lszIsDclPrelude(baPrelude)) {
                return false;
            }
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    // No count, no directory, no terminator: the chain landing exactly on
    // end-of-file is this format's only self-check, and slack there means the
    // records do not describe the payload.
    if (nOffset != context.nInputSize) return false;
    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    context.nFirstRecordOffset = context.listMembers.first().nRecordOffset;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XLSZ::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XLSZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLSZ archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLSZ::createInstance(QIODevice *pDevice, bool bIsImage,
                             XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLSZ(pDevice);
}

QList<QString> XLSZ::getSearchSignatures()
{
    return {QStringLiteral("37F0FFFF0003")};
}

XBinary::FT XLSZ::getFileType()
{
    return FT_LSZ;
}

XBinary::MODE XLSZ::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLSZ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XLSZ::getArch()
{
    return QString();
}

QString XLSZ::getFileFormatExt()
{
    return QStringLiteral("lsz");
}

QString XLSZ::getFileFormatExtsString()
{
    return QStringLiteral("Delrina WinFax library (*.lsz)");
}

QString XLSZ::getMIMEString()
{
    return QStringLiteral("application/x-winfax-lsz");
}

QString XLSZ::getVersion()
{
    return QStringLiteral("3.0");
}

qint64 XLSZ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XLSZ::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XLSZ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_TABLE | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QString XLSZ::methodToString(const MEMBER &member)
{
    if (member.nMethod == LSZ_METHOD_STORED) return QStringLiteral("Store");
    if (member.nMethod == LSZ_METHOD_IMPLODE) {
        return QStringLiteral("PKWARE DCL Implode");
    }
    if (member.nMethod == LSZ_METHOD_EMPTY) {
        return QStringLiteral("Store (empty)");
    }
    return QStringLiteral("Unknown");
}

XBinary::HANDLE_METHOD XLSZ::methodToHandleMethod(const MEMBER &member)
{
    if (member.nMethod == LSZ_METHOD_IMPLODE) {
        // The payload is a complete DCL stream, prelude bytes included, and
        // decPkwareDcl() takes the plaintext length as an input - which the
        // record stores, so no bitstream scan is needed here.
        return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    }
    // Method 1 and the zero-length method 6 are both plain byte copies; a
    // method 6 member is a real, empty file, not a failure.
    return HANDLE_METHOD_STORE;
}

bool XLSZ::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XLSZ::getFileParts(quint32 nFileParts, qint32 nLimit,
                                        PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LSZ_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    const qint32 nCount = static_cast<qint32>(context.listMembers.size());
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_TABLE) &&
            canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
            FPART part = {};
            part.filePart = FILEPART_TABLE;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = LSZ_RECORD_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Record");
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      methodToHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member));
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = LSZ_RECORD_SIZE + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XLSZ::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLSZ::initUnpack(UNPACK_STATE *pState,
                      const QMap<UNPACK_PROP, QVariant> &mapProperties,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XLSZ> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Delrina WinFax LSZ library; stored / PKWARE DCL Implode members"));
    pState->nCurrentOffset = pContext->nFirstRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XLSZ::infoCurrent(UNPACK_STATE *pState,
                                         PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
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
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                methodToHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        const QDateTime dtModified =
            dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        }
    }
    // The checksum word is zero for most members and the algorithm behind the
    // non-zero ones is not recoverable from the container, so it is not
    // published as a verifiable CRC.
    return result;
}

bool XLSZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XLSZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
