/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xlbrcobol.h"

#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// ---------------------------------------------------------------------------
// Layout, recovered from the Micro Focus COBOL Library handler of the reference implementation
// (class zgb, VMT 0x0066a7c8; detector 0x0066a850, worker 0x0066a8a0) and then
// confirmed byte-exact against the reference implementation's own extraction over the whole 50-file
// corpus.
//
// Fixed header, 0x100 bytes:
//   +0x00  "Micro Focus COBOL Library File" then spaces
//   +0x22  "MM/DD/YY  HH:MM:SS:hh" creation stamp, space padded
//   +0x90  u16be  block size, 0x0080 on everything seen
//   +0x92  u16be  number of directory records
// +0x94 u32le == 1 (checked by the reference implementation detector)
// +0x98 u32le == 1 (checked by the reference implementation detector)
//
// Directory record, 18 bytes, first at 0x100:
//   +0x00  u32be  file offset of the NEXT record (may point backwards)
//   +0x04  u32be  payload offset in 128-byte blocks
//   +0x08  u32be  payload length in bytes
// +0x0c u16be DOS time (the reference implementation hands this one to the TIME argument)
//   +0x0e  u16be  DOS date
//   +0x10  u16    flags; only 0x0000 and 0x0001 occur
//   +0x12  u8     name length, then that many name bytes (no terminator)
//
// The payload is stored, never compressed: the reference implementation seeks to (block << 7) and copies
// the byte count straight out.
// ---------------------------------------------------------------------------
const qint64 LBRCOBOL_HEADER_SIZE = 0x100;
const qint64 LBRCOBOL_RECORD_SIZE = 18;
const qint64 LBRCOBOL_COUNT_OFFSET = 0x92;
const qint64 LBRCOBOL_MAGIC1_OFFSET = 0x94;
const qint64 LBRCOBOL_MAGIC2_OFFSET = 0x98;
const qint64 LBRCOBOL_STAMP_OFFSET = 0x22;
const qint64 LBRCOBOL_STAMP_SIZE = 21;
const qint32 LBRCOBOL_BLOCK_SHIFT = 7;
const qint32 LBRCOBOL_MAX_MEMBERS = 0x10000;
const qint32 LBRCOBOL_MAX_NAME = 255;

bool lbrRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool lbrIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        // Names are plain uppercase/lowercase 8.3-ish COBOL module names plus
        // '-', '.' and '_'; anything outside printable ASCII means the walk
        // has run off the directory and the archive must be rejected.
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}
}  // namespace

XLbrCobol::XLbrCobol(QIODevice *pDevice) : XArchive(pDevice)
{
}

XLbrCobol::~XLbrCobol()
{
}

bool XLbrCobol::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < LBRCOBOL_HEADER_SIZE + LBRCOBOL_RECORD_SIZE + 2) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, LBRCOBOL_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != LBRCOBOL_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    // Exactly the reference implementation detector's five tests: the two halves of the magic, the
    // tail of the padded title, and the two little-endian ones at +0x94/+0x98.
    if (std::memcmp(pHeader, "Micro Focus COBOL Library File", 30) != 0) {
        return false;
    }
    if (std::memcmp(pHeader + 0x1e, "    ", 4) != 0) return false;
    if (qFromLittleEndian<quint32>(pHeader + LBRCOBOL_MAGIC1_OFFSET) != 1) {
        return false;
    }
    if (qFromLittleEndian<quint32>(pHeader + LBRCOBOL_MAGIC2_OFFSET) != 1) {
        return false;
    }

    const qint32 nCount = static_cast<qint32>(
        qFromBigEndian<quint16>(pHeader + LBRCOBOL_COUNT_OFFSET));
    if (nCount <= 0 || nCount > LBRCOBOL_MAX_MEMBERS) return false;

    context.sCreated = QString::fromLatin1(
                           baHeader.mid(static_cast<int>(LBRCOBOL_STAMP_OFFSET),
                                        static_cast<int>(LBRCOBOL_STAMP_SIZE)))
                           .trimmed();

    qint64 nArchiveEnd = LBRCOBOL_HEADER_SIZE;
    qint64 nOffset = LBRCOBOL_HEADER_SIZE;

    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        // The link field is free to point backwards, so there is no ordering
        // invariant to lean on; the header record count is the only bound and
        // every hop is re-validated against the file size.
        if (!lbrRangeWithin(context.nInputSize, nOffset,
                            LBRCOBOL_RECORD_SIZE + 1)) {
            return false;
        }
        const QByteArray baRecord = read_array_process(
            nOffset, LBRCOBOL_RECORD_SIZE + 1, pPdStruct);
        if (baRecord.size() != LBRCOBOL_RECORD_SIZE + 1) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());

        const qint64 nNext =
            static_cast<qint64>(qFromBigEndian<quint32>(pRecord + 0));
        const qint64 nBlock =
            static_cast<qint64>(qFromBigEndian<quint32>(pRecord + 4));
        const qint64 nSize =
            static_cast<qint64>(qFromBigEndian<quint32>(pRecord + 8));

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        // The reference implementation passes the +0x0e field as the DATE argument and the +0x0c field
        // as the TIME argument; the order is the
        // reverse of the obvious one and getting it backwards silently yields
        // nonsense timestamps.
        member.nDosTime = qFromBigEndian<quint16>(pRecord + 12);
        member.nDosDate = qFromBigEndian<quint16>(pRecord + 14);
        member.nFlags = qFromLittleEndian<quint16>(pRecord + 16);
        member.nDataOffset = nBlock << LBRCOBOL_BLOCK_SHIFT;
        member.nDataSize = nSize;
        if (!lbrRangeWithin(context.nInputSize, member.nDataOffset,
                            member.nDataSize)) {
            return false;
        }
        // The payload can never overlap the fixed header; that is the cheapest
        // structural test that keeps a random "Micro Focus"-prefixed blob from
        // parsing as a directory.
        if (member.nDataSize > 0 &&
            member.nDataOffset < LBRCOBOL_HEADER_SIZE) {
            return false;
        }

        // Pascal name: the length byte is the sole authority (the buffer is not
        // NUL-terminated and is space padded out to the next record).
        const qint32 nNameSize =
            static_cast<qint32>(pRecord[LBRCOBOL_RECORD_SIZE]);
        if (nNameSize < 1 || nNameSize > LBRCOBOL_MAX_NAME) return false;
        const qint64 nNameOffset = nOffset + LBRCOBOL_RECORD_SIZE + 1;
        if (!lbrRangeWithin(context.nInputSize, nNameOffset, nNameSize)) {
            return false;
        }
        const QByteArray baName =
            read_array_process(nNameOffset, nNameSize, pPdStruct);
        if (baName.size() != nNameSize) {
            return false;
        }
        if (!lbrIsValidName(baName)) return false;

        member.nRecordSize = LBRCOBOL_RECORD_SIZE + 1 + nNameSize;
        member.sFileName = QString::fromLatin1(baName)
                               .replace(QLatin1Char('\\'), QLatin1Char('/'));

        nArchiveEnd = qMax(nArchiveEnd, member.nRecordOffset + member.nRecordSize);
        nArchiveEnd = qMax(nArchiveEnd, member.nDataOffset + member.nDataSize);
        context.listMembers.append(member);

        nOffset = nNext;
    }

    context.nArchiveSize = qMin(nArchiveEnd, context.nInputSize);
    context.nFirstMemberOffset = context.listMembers.first().nRecordOffset;
    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XLbrCobol::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XLbrCobol::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLbrCobol archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLbrCobol::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLbrCobol(pDevice);
}

QList<QString> XLbrCobol::getSearchSignatures()
{
    return {QStringLiteral("'Micro Focus COBOL Library File'")};
}

XBinary::FT XLbrCobol::getFileType()
{
    return FT_LBR_COBOL;
}

XBinary::MODE XLbrCobol::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XLbrCobol::getEndian()
{
    // Every numeric directory field is big-endian, which is what you would
    // expect from a COBOL runtime that started life on IBM iron.
    return ENDIAN_BIG;
}

QString XLbrCobol::getArch()
{
    return QString();
}

QString XLbrCobol::getFileFormatExt()
{
    return QStringLiteral("lbr");
}

QString XLbrCobol::getFileFormatExtsString()
{
    return QStringLiteral(
        "Micro Focus COBOL Library File (*.lbr *.obr *.16 *.32)");
}

QString XLbrCobol::getMIMEString()
{
    return QStringLiteral("application/x-mf-cobol-library");
}

QString XLbrCobol::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return context.sCreated;
}

qint64 XLbrCobol::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XLbrCobol::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XLbrCobol::getMemoryMap(MAPMODE mapMode,
                                             PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XLbrCobol::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XLbrCobol::getFileParts(quint32 nFileParts, qint32 nLimit,
                                              PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LBRCOBOL_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = member.nRecordSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Directory record");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            part.mapProperties.insert(FPART_PROP_DATETIME,
                                      dosDateTimeToQDateTime(member.nDosDate,
                                                             member.nDosTime));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) &&
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, result.size())) {
        // The last 128-byte block is padded, so a few slack bytes after the
        // final member are normal and are reported rather than rejected.
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

QMap<XBinary::UNPACK_PROP, QVariant> XLbrCobol::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XLbrCobol::initUnpack(UNPACK_STATE *pState,
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
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Micro Focus COBOL Library File; stored members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XLbrCobol::infoCurrent(UNPACK_STATE *pState,
                                              PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nFlags));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(
        FPART_PROP_DATETIME,
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
    // No checksum field exists anywhere in the record; nothing to publish.
    return result;
}

bool XLbrCobol::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
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

bool XLbrCobol::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
