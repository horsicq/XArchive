/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xartipack.h"

#include <QtEndian>

#include <new>

namespace {
const char ARTIPACK_MAGIC[8] = {'A', 'R', 'T', 'I', 'P', 'A', 'C', 'K'};
const qint64 ARTIPACK_HEADER_SIZE = 32;
const qint64 ARTIPACK_RECORD_SIZE = 34;
const qint64 ARTIPACK_NAME_SIZE = 13;
// LZHUF.C prefixes every stream with the original size as a dword, and the
// directory's compressed size counts it.  Nothing shorter can be a member.
const qint64 ARTIPACK_SIZE_PREFIX = 4;
// The only version word seen on the whole family.  A different one would mean
// an unproven record layout, so refuse it rather than mis-parse it.
const quint16 ARTIPACK_VERSION_1_0 = 0x0100U;
// The count field is a quint16, so this is the field's own ceiling, not a
// heuristic cap.
const qint32 ARTIPACK_MAX_MEMBERS = 0xffff;

bool artipackRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// A record name is char[13]: an 8.3 name, NUL-terminated and zero-padded out to
// the full field.  Requiring the padding to be zero is what keeps the whole
// 34-byte record grid honest on a file that merely happens to start with the
// magic.
bool artipackExtractName(const uchar *pRecord, QString *pResult)
{
    qint32 nLength = -1;
    for (qint32 i = 0; i < ARTIPACK_NAME_SIZE; i++) {
        if (pRecord[i] == 0) {
            nLength = i;
            break;
        }
    }
    if (nLength <= 0) return false;
    for (qint32 i = nLength; i < ARTIPACK_NAME_SIZE; i++) {
        if (pRecord[i] != 0) return false;
    }
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = pRecord[i];
        if (nCharacter < 0x21 || nCharacter > 0x7e) return false;
        // Members are plain 8.3 names with no path component.  Refusing
        // separators here means no later consumer has to undo a traversal.
        if (nCharacter == '/' || nCharacter == '\\' || nCharacter == ':') {
            return false;
        }
    }
    const QByteArray baName(reinterpret_cast<const char *>(pRecord), nLength);
    if (baName == QByteArray(".") || baName == QByteArray("..")) return false;
    *pResult = QString::fromLatin1(baName);
    return true;
}
}  // namespace

XArtiPack::XArtiPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

XArtiPack::~XArtiPack()
{
}

bool XArtiPack::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <
        ARTIPACK_HEADER_SIZE + ARTIPACK_RECORD_SIZE + ARTIPACK_SIZE_PREFIX) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, ARTIPACK_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != ARTIPACK_HEADER_SIZE) {
        return false;
    }
    if (memcmp(baHeader.constData(), ARTIPACK_MAGIC,
               sizeof(ARTIPACK_MAGIC)) != 0) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    context.nVersion = qFromLittleEndian<quint16>(pHeader + 8);
    if (context.nVersion != ARTIPACK_VERSION_1_0) return false;

    const qint32 nNumberOfMembers =
        static_cast<qint32>(qFromLittleEndian<quint16>(pHeader + 10));
    context.nDirectoryOffset =
        static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 12));
    if (nNumberOfMembers < 1 || nNumberOfMembers > ARTIPACK_MAX_MEMBERS ||
        context.nDirectoryOffset < ARTIPACK_HEADER_SIZE) {
        return false;
    }
    context.nDirectorySize =
        static_cast<qint64>(nNumberOfMembers) * ARTIPACK_RECORD_SIZE;
    // The directory is the file's tail and it ends exactly at EOF on every
    // known producer.  Demanding the exact equality both bounds the read below
    // by the real device size and rejects a truncated or padded container.
    if (context.nDirectoryOffset >
            context.nInputSize - context.nDirectorySize ||
        context.nDirectoryOffset + context.nDirectorySize !=
            context.nInputSize) {
        return false;
    }

    const QByteArray baDirectory = read_array_process(
        context.nDirectoryOffset, context.nDirectorySize, pPdStruct);
    if (baDirectory.size() != context.nDirectorySize) {
        return false;
    }

    // Payloads are stored back to back in directory order, starting right
    // after the header.  The running expectation is the strongest structural
    // gate the format offers: a single wrong field breaks the chain.
    qint64 nExpectedOffset = ARTIPACK_HEADER_SIZE;
    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nRecordOffset =
            context.nDirectoryOffset +
            static_cast<qint64>(i) * ARTIPACK_RECORD_SIZE;
        const uchar *pRecord = reinterpret_cast<const uchar *>(
            baDirectory.constData() +
            static_cast<qint64>(i) * ARTIPACK_RECORD_SIZE);

        MEMBER member = {};
        if (!artipackExtractName(pRecord, &member.sFileName)) return false;
        // Byte +0x0D and dword +0x1E are producer residue keyed to the record
        // INDEX, not to the member (index 0/1/2 carry the same values across
        // unrelated archives).  They are neither a method nor a CRC: never
        // gate, branch or verify on them.
        member.nRecordOffset = nRecordOffset;
        member.nDataOffset =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 14));
        member.nUncompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 18));
        member.nCompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 22));
        member.nDosDate = qFromLittleEndian<quint16>(pRecord + 26);
        member.nDosTime = qFromLittleEndian<quint16>(pRecord + 28);

        if (member.nDataOffset != nExpectedOffset ||
            member.nCompressedSize < ARTIPACK_SIZE_PREFIX ||
            !artipackRangeWithin(context.nDirectoryOffset,
                                 member.nDataOffset,
                                 member.nCompressedSize)) {
            return false;
        }

        // The payload repeats its own original size in LZHUF's leading dword.
        // Cross-checking it against the directory is what makes the container
        // self-consistent instead of merely well-shaped.
        const QByteArray baSizePrefix = read_array_process(
            member.nDataOffset, ARTIPACK_SIZE_PREFIX, pPdStruct);
        if (baSizePrefix.size() != ARTIPACK_SIZE_PREFIX) {
            return false;
        }
        const qint64 nInnerSize = static_cast<qint64>(
            qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(
                baSizePrefix.constData())));
        if (nInnerSize != member.nUncompressedSize) return false;

        member.nStreamOffset = member.nDataOffset + ARTIPACK_SIZE_PREFIX;
        member.nStreamSize = member.nCompressedSize - ARTIPACK_SIZE_PREFIX;

        nExpectedOffset = member.nDataOffset + member.nCompressedSize;
        context.listMembers.append(member);
    }

    // The last payload has to butt up against the directory; any slack means
    // this is not the layout this parser was verified against.
    if (nExpectedOffset != context.nDirectoryOffset) return false;

    context.nArchiveSize = context.nInputSize;
    context.nFirstMemberOffset = context.listMembers.first().nDataOffset;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XArtiPack::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition =
        guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XArtiPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XArtiPack archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XArtiPack::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XArtiPack(pDevice);
}

QList<QString> XArtiPack::getSearchSignatures()
{
    // The version word is part of the signature: the record layout below is
    // only proven for 0x0100 (bytes 00 01).
    return {QStringLiteral("'ARTIPACK'0001")};
}

XBinary::FT XArtiPack::getFileType()
{
    return FT_ARTIPACK;
}

XBinary::MODE XArtiPack::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XArtiPack::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XArtiPack::getArch()
{
    return QString();
}

QString XArtiPack::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XArtiPack::getFileFormatExtsString()
{
    return QStringLiteral("Artisoft ARTIPACK (*.pak)");
}

QString XArtiPack::getMIMEString()
{
    return QStringLiteral("application/x-artipack");
}

QString XArtiPack::getVersion()
{
    // 0x0100 is read as major.minor; that split is an interpretation of the
    // two bytes, so nothing in the parser depends on it.
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString("%1.%2")
        .arg(context.nVersion >> 8)
        .arg(context.nVersion & 0xffU);
}

qint64 XArtiPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XArtiPack::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XArtiPack::getMemoryMap(MAPMODE mapMode,
                                             PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_TABLE | FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XArtiPack::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XArtiPack::getFileParts(quint32 nFileParts,
                                              qint32 nLimit,
                                              PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ARTIPACK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            // The 4-byte LZHUF size dword is header, not bitstream.  Handing
            // the decoder the payload offset instead of this one desynchronises
            // it on the very first symbol and silently yields nothing.
            part.nFileOffset = member.nStreamOffset;
            part.nFileSize = member.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nStreamSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_LZH1);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("LZHUF"));
            if (XBinary::isValidDosDateTime(member.nDosDate,
                                            member.nDosTime)) {
                part.mapProperties.insert(
                    FPART_PROP_MTIME,
                    XBinary::dosDateTimeToQDateTime(member.nDosDate,
                                                    member.nDosTime));
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_TABLE) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_TABLE;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        result.append(part);
    }
    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) &&
        context.nArchiveSize < context.nInputSize &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XArtiPack::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XArtiPack::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

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
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Artisoft ARTIPACK; LZHUF-compressed members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XArtiPack::infoCurrent(UNPACK_STATE *pState,
                                              PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext ||
        pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member =
        pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nStreamOffset;
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    // Every member of every known ARTIPACK uses stock LZHUF.C, which is what
    // HANDLE_METHOD_LZH1 implements (ring pre-filled with ' ', write cursor at
    // N-F).  The ARCV_LZHUF* methods look related and are not: different
    // symbol count, length bias and initial cursor.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZH1);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("LZHUF"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (XBinary::isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        result.mapProperties.insert(
            FPART_PROP_MTIME,
            XBinary::dosDateTimeToQDateTime(member.nDosDate,
                                            member.nDosTime));
    }
    return result;
}

bool XArtiPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext ||
        pState->nCurrentIndex >= pContext->listMembers.size()) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nDirectoryOffset;
    return false;
}

bool XArtiPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
