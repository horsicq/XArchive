/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbvrppac.h"

#include <QtEndian>

#include <new>

namespace {
// The 0x80-byte container header is byte-identical over 0x00..0x5F on every
// sample of the family, so the gate can afford to be literal.
const qint64 BVRPPAC_HEADER_SIZE = 0x80;
const qint64 BVRPPAC_MEMBER_HEADER_SIZE = 32;
const qint64 BVRPPAC_MIN_FILE_SIZE =
    BVRPPAC_HEADER_SIZE + BVRPPAC_MEMBER_HEADER_SIZE;
const qint64 BVRPPAC_OFFSET_TERMINATOR = 0x4c;   // 00 0D 0A 1A
const qint64 BVRPPAC_OFFSET_SIGNATURE = 0x50;    // D6 A9
const qint64 BVRPPAC_OFFSET_VERSION = 0x52;      // 06 01 == 1.06
const qint64 BVRPPAC_OFFSET_FIRSTMEMBER = 0x5c;  // LE32
const qint64 BVRPPAC_OFFSET_MEMBERCOUNT = 0x60;  // LE16
const quint16 BVRPPAC_SIGNATURE = 0xa9d6U;
const qint32 BVRPPAC_NAME_SIZE = 12;
const quint8 BVRPPAC_METHOD_LZHUF = 0x01U;

// Member header field offsets, relative to the header start.
const qint32 BVRPPAC_MEMBER_ATTRIBUTES = 0x0c;
const qint32 BVRPPAC_MEMBER_DOSDATE = 0x0e;
const qint32 BVRPPAC_MEMBER_DOSTIME = 0x10;
const qint32 BVRPPAC_MEMBER_METHOD = 0x12;
const qint32 BVRPPAC_MEMBER_NEXTOFFSET = 0x13;
const qint32 BVRPPAC_MEMBER_UNCOMPRESSEDSIZE = 0x17;
const qint32 BVRPPAC_MEMBER_CRC = 0x1b;
const qint32 BVRPPAC_MEMBER_PAD = 0x1f;

const quint16 BVRPPAC_DOS_ATTRIBUTE_READONLY = 0x0001U;

bool bvrpRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// The 12-byte name field is a reused buffer that the writer never clears, so
// the bytes AFTER the terminating NUL are leftovers from a previously written,
// longer name ("WFCOM.HLP\0LP" is really WFCOM.HLP).  The name is therefore
// everything up to the first NUL, and all 12 bytes when there is none - which
// is the majority case.  Stripping trailing NULs (or trimmed()) corrupts most
// names in this family.
QByteArray bvrpMemberName(const QByteArray &baHeader)
{
    const QByteArray baField = baHeader.left(BVRPPAC_NAME_SIZE);
    const qint32 nTerminator = baField.indexOf('\0');
    return (nTerminator >= 0) ? baField.left(nTerminator) : baField;
}

bool bvrpIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
        // Names in this family are bare 8.3 with no path component; a
        // separator would mean the field is not a name at all.
        if (c == '/' || c == '\\' || c == ':') return false;
    }
    return true;
}
}  // namespace

XBvrpPac::XBvrpPac(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBvrpPac::~XBvrpPac()
{
}

bool XBvrpPac::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BVRPPAC_MIN_FILE_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, BVRPPAC_HEADER_SIZE, pPdStruct);
    if (!guardedSource ||
        baHeader.size() != BVRPPAC_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());

    // The banner is not a flat space-pad: a CR/LF pair sits at 0x20..0x21, so
    // only the leading literal and the vendor substring may be compared.
    if (!baHeader.startsWith("PAC - ") ||
        baHeader.mid(10, 13) != QByteArray("BVRP Software")) {
        return false;
    }
    if (baHeader.mid(BVRPPAC_OFFSET_TERMINATOR, 4) !=
        QByteArray("\x00\x0d\x0a\x1a", 4)) {
        return false;
    }
    if (qFromLittleEndian<quint16>(pHeader + BVRPPAC_OFFSET_SIGNATURE) !=
        BVRPPAC_SIGNATURE) {
        return false;
    }
    // 0x52 is the writer version (0x0106 on every known sample).  It is
    // deliberately not constrained, so a later revision of the same container
    // still parses.
    context.nVersion =
        qFromLittleEndian<quint16>(pHeader + BVRPPAC_OFFSET_VERSION);

    const quint32 nFirstMemberOffset =
        qFromLittleEndian<quint32>(pHeader + BVRPPAC_OFFSET_FIRSTMEMBER);
    context.nDeclaredMemberCount =
        qFromLittleEndian<quint16>(pHeader + BVRPPAC_OFFSET_MEMBERCOUNT);
    // The first member must start after the member-count field and its whole
    // header must be present; anything else is a truncated or forged file.
    if (nFirstMemberOffset < (BVRPPAC_OFFSET_MEMBERCOUNT + 2) ||
        !bvrpRangeWithin(context.nInputSize, nFirstMemberOffset,
                         BVRPPAC_MEMBER_HEADER_SIZE) ||
        (context.nDeclaredMemberCount == 0)) {
        return false;
    }
    context.nFirstMemberOffset = nFirstMemberOffset;

    qint64 nOffset = context.nFirstMemberOffset;
    for (quint16 nIndex = 0; nIndex < context.nDeclaredMemberCount; nIndex++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!bvrpRangeWithin(context.nInputSize, nOffset,
                             BVRPPAC_MEMBER_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baMember = read_array_process(
            nOffset, BVRPPAC_MEMBER_HEADER_SIZE, pPdStruct);
        if (!guardedSource ||
            baMember.size() != BVRPPAC_MEMBER_HEADER_SIZE) {
            return false;
        }
        const uchar *pMember =
            reinterpret_cast<const uchar *>(baMember.constData());

        const QByteArray baName = bvrpMemberName(baMember);
        if (!bvrpIsValidName(baName)) return false;
        if (static_cast<quint8>(baMember.at(BVRPPAC_MEMBER_PAD)) != 0) {
            return false;
        }

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + BVRPPAC_MEMBER_HEADER_SIZE;
        member.nAttributes =
            qFromLittleEndian<quint16>(pMember + BVRPPAC_MEMBER_ATTRIBUTES);
        // The DWORD at +0x0E is (time << 16) | date - the reverse of the usual
        // DOS convention.  Reading it the other way round yields invalid
        // calendar dates on two thirds of the members.
        member.nDosDate =
            qFromLittleEndian<quint16>(pMember + BVRPPAC_MEMBER_DOSDATE);
        member.nDosTime =
            qFromLittleEndian<quint16>(pMember + BVRPPAC_MEMBER_DOSTIME);
        member.nMethod =
            static_cast<quint8>(baMember.at(BVRPPAC_MEMBER_METHOD));
        member.nUncompressedSize = qFromLittleEndian<quint32>(
            pMember + BVRPPAC_MEMBER_UNCOMPRESSEDSIZE);
        // Only the low 16 bits are ever populated; the field carries a
        // CRC-16/ARC of the UNPACKED bytes.
        member.nCRC16 =
            qFromLittleEndian<quint32>(pMember + BVRPPAC_MEMBER_CRC) & 0xffffU;
        member.sFileName = QString::fromLatin1(baName);

        // +0x13 is the ABSOLUTE offset of the NEXT header, not a compressed
        // size.  Requiring strict forward progress both derives the payload
        // slice and guarantees the walk terminates.
        const quint32 nNextOffset =
            qFromLittleEndian<quint32>(pMember + BVRPPAC_MEMBER_NEXTOFFSET);
        if ((static_cast<qint64>(nNextOffset) < member.nDataOffset) ||
            (static_cast<qint64>(nNextOffset) > context.nInputSize)) {
            return false;
        }
        member.nCompressedSize =
            static_cast<qint64>(nNextOffset) - member.nDataOffset;
        if (!bvrpRangeWithin(context.nInputSize, member.nDataOffset,
                             member.nCompressedSize)) {
            return false;
        }

        context.listMembers.append(member);
        nOffset = nNextOffset;
    }

    // Walk exactly the declared number of links and stop: five of the eleven
    // known archives carry 3..4280 bytes of trailing junk, and a loop that ran
    // to EOF would parse that as a twelfth header and invent a member.
    context.nArchiveSize = nOffset;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XBvrpPac::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XBvrpPac::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBvrpPac archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBvrpPac::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBvrpPac(pDevice);
}

QList<QString> XBvrpPac::getSearchSignatures()
{
    return {QStringLiteral("'PAC - (c) BVRP Software 1990'")};
}

XBinary::FT XBvrpPac::getFileType()
{
    return FT_BVRP_PAC;
}

XBinary::MODE XBvrpPac::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBvrpPac::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBvrpPac::getArch()
{
    return QString();
}

QString XBvrpPac::getFileFormatExt()
{
    return QStringLiteral("pac");
}

QString XBvrpPac::getFileFormatExtsString()
{
    return QStringLiteral("BVRP PAC (*.pac)");
}

QString XBvrpPac::getMIMEString()
{
    return QStringLiteral("application/x-bvrp-pac");
}

QString XBvrpPac::getVersion()
{
    // 0x52 is the minor and 0x53 the major byte of the writer version.
    const quint16 nVersion = read_uint16(BVRPPAC_OFFSET_VERSION);
    return QStringLiteral("%1.%2")
        .arg(nVersion >> 8)
        .arg(nVersion & 0xffU, 2, 10, QLatin1Char('0'));
}

qint64 XBvrpPac::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    // The end of the last payload, not the device size: the trailing bytes of
    // the five padded archives must surface as an overlay.
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBvrpPac::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBvrpPac::getMemoryMap(MAPMODE mapMode,
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

QString XBvrpPac::methodToString(quint8 nMethod)
{
    const QString sName = (nMethod == BVRPPAC_METHOD_LZHUF)
                              ? QStringLiteral("LZHUF (lh1-compatible)")
                              : QStringLiteral("Unknown");
    return QStringLiteral("BVRP %1 %2").arg(nMethod).arg(sName);
}

XBinary::HANDLE_METHOD XBvrpPac::methodToHandleMethod(quint8 nMethod)
{
    // Method 1 is plain Okumura/Yoshizaki LZHUF with the stock lh1 parameters
    // (N=4096, F=60, THRESHOLD=2, 314 symbols, no stop code) - not one of the
    // Eschalon ARCV_LZHUF variants, which use a stop code and decode to
    // garbage here.
    if (nMethod == BVRPPAC_METHOD_LZHUF) return HANDLE_METHOD_LZH1;
    return HANDLE_METHOD_UNKNOWN;
}

QDateTime XBvrpPac::dosDateTime(quint16 nDate, quint16 nTime)
{
    const QDate date(1980 + ((nDate >> 9) & 0x7f), (nDate >> 5) & 0x0f,
                     nDate & 0x1f);
    const QTime time((nTime >> 11) & 0x1f, (nTime >> 5) & 0x3f,
                     (nTime & 0x1f) * 2);
    if (!date.isValid() || !time.isValid()) return QDateTime();
    return QDateTime(date, time);
}

bool XBvrpPac::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XBvrpPac::getFileParts(quint32 nFileParts, qint32 nLimit,
                                             PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nFirstMemberOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Container header");
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
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = BVRPPAC_MEMBER_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member.nMethod));
            part.mapProperties.insert(FPART_PROP_TYPE,
                                      static_cast<quint32>(member.nMethod));
            part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC16);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16ARC);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize =
                BVRPPAC_MEMBER_HEADER_SIZE + member.nCompressedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XBvrpPac::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBvrpPac::initUnpack(UNPACK_STATE *pState,
                          const QMap<UNPACK_PROP, QVariant> &mapProperties,
                          PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource ||
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("BVRP PAC; member CRC-16/ARC over the unpacked data"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    // Binding alone only STAGES the source: without this finalize the listing
    // works while extraction silently produces nothing.
    const bool bFinalized = validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XBvrpPac::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nHeaderOffset) {
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
                                methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nMethod));
    // The stored CRC covers the UNPACKED bytes, so it is safe to hand to the
    // output verifier.
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC16);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16ARC);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(
        FPART_PROP_ISREADONLY,
        (member.nAttributes & BVRPPAC_DOS_ATTRIBUTE_READONLY) != 0);
    const QDateTime dtModified =
        dosDateTime(member.nDosDate, member.nDosTime);
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }
    return result;
}

bool XBvrpPac::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XBvrpPac::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
