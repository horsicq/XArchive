/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbinaryii.h"

#include <QDateTime>
#include <QtEndian>

#include <cstring>
#include <new>
#include <QTimeZone>

namespace {
const qint64 BINARYII_HEADER_SIZE = 128;
const qint64 BINARYII_ALIGNMENT = 128;
// Offsets inside the 128-byte member header.  Every multi-byte field is
// little-endian, and several of them are split: the low half sits in the
// original 1986 header layout and the high half in the block that Binary II
// version 1 appended at 0x6d.
const qint32 BINARYII_OFFSET_ACCESS = 0x03;
const qint32 BINARYII_OFFSET_FILETYPE = 0x04;
const qint32 BINARYII_OFFSET_AUXTYPE = 0x05;
const qint32 BINARYII_OFFSET_STORAGETYPE = 0x07;
const qint32 BINARYII_OFFSET_BLOCKCOUNT = 0x08;
const qint32 BINARYII_OFFSET_MODDATE = 0x0a;
const qint32 BINARYII_OFFSET_MODTIME = 0x0c;
const qint32 BINARYII_OFFSET_CREATEDATE = 0x0e;
const qint32 BINARYII_OFFSET_CREATETIME = 0x10;
const qint32 BINARYII_OFFSET_ID = 0x12;
const qint32 BINARYII_OFFSET_EOF = 0x14;
const qint32 BINARYII_OFFSET_NAMESIZE = 0x17;
const qint32 BINARYII_OFFSET_NAME = 0x18;
const qint32 BINARYII_OFFSET_AUXTYPE_HIGH = 0x6d;
const qint32 BINARYII_OFFSET_ACCESS_HIGH = 0x6f;
const qint32 BINARYII_OFFSET_FILETYPE_HIGH = 0x70;
const qint32 BINARYII_OFFSET_STORAGETYPE_HIGH = 0x71;
const qint32 BINARYII_OFFSET_BLOCKCOUNT_HIGH = 0x72;
const qint32 BINARYII_OFFSET_EOF_HIGH = 0x74;
const qint32 BINARYII_OFFSET_OSTYPE = 0x79;
const qint32 BINARYII_OFFSET_NATIVETYPE = 0x7a;
const qint32 BINARYII_OFFSET_DATAFLAGS = 0x7d;
const qint32 BINARYII_OFFSET_VERSION = 0x7e;
const qint32 BINARYII_OFFSET_FILESTOFOLLOW = 0x7f;

const quint8 BINARYII_ID_BYTE = 0x02;
// ProDOS storage type $0d marks a directory.  Such a member declares a
// non-zero EOF (TIC.BNY's "TERMCAPS" declares 1024) yet stores no bytes at
// all - the next member header follows immediately.  Trusting the EOF here
// desynchronizes the whole chain and collapses a 23-member archive to one.
const quint8 BINARYII_STORAGETYPE_DIRECTORY = 0x0d;
const quint8 BINARYII_DATAFLAG_COMPRESSED = 0x80;
const qint32 BINARYII_MAX_MEMBERS = 65536;
const qint32 BINARYII_MAX_NAME_SIZE = 64;

const char BINARYII_MAGIC[3] = {'\x0a', '\x47', '\x4c'};  // "\nGL"

bool binaryiiRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

qint64 binaryiiAlignUp(qint64 nValue)
{
    if (nValue < 0) return -1;
    const qint64 nRemainder = nValue % BINARYII_ALIGNMENT;
    if (nRemainder == 0) return nValue;
    // The caller only ever aligns an offset that is already inside the file,
    // so the addition cannot approach the qint64 ceiling.
    return nValue + (BINARYII_ALIGNMENT - nRemainder);
}

quint8 binaryiiByte(const QByteArray &baData, qint32 nOffset)
{
    return static_cast<quint8>(baData.at(nOffset));
}

quint16 binaryiiWord(const QByteArray &baData, qint32 nOffset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(
        baData.constData() + nOffset));
}

// ProDOS packs the date as yyyyyyym mmmddddd and the time as
// 000hhhhh 00mmmmmm, with a two-digit year: below 40 is 20xx, otherwise 19xx.
QDateTime binaryiiProDOSDateTime(quint16 nDate, quint16 nTime)
{
    if ((nDate == 0) && (nTime == 0)) return QDateTime();

    const qint32 nYear = (nDate >> 9) & 0x7f;
    const qint32 nMonth = (nDate >> 5) & 0x0f;
    const qint32 nDay = nDate & 0x1f;
    const qint32 nHour = (nTime >> 8) & 0x1f;
    const qint32 nMinute = nTime & 0x3f;

    const QDate date((nYear < 40) ? (2000 + nYear) : (1900 + nYear), nMonth,
                     nDay);
    const QTime time(nHour, nMinute);
    if (!date.isValid() || !time.isValid()) return QDateTime();
    return QDateTime(date, time, X_UTC_TZ);
}

// ProDOS names are 7-bit ASCII, but a writer may leave the Apple II high bit
// set on every character (GUADCNL.DOX stores "USERS.GUIDE.BXY" as D5 D3 C5...).
// Masking is what turns that member into a readable name instead of mojibake.
bool binaryiiDecodeName(const QByteArray &baRaw, QString *pName)
{
    if (baRaw.isEmpty() || !pName) return false;

    QString sName;
    sName.reserve(baRaw.size());
    for (char cChar : baRaw) {
        const quint8 nCharacter = static_cast<quint8>(cChar) & 0x7fU;
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        sName.append(QLatin1Char(static_cast<char>(nCharacter)));
    }

    // '/' is the ProDOS path separator and must survive into a subdirectory,
    // but an absolute or dot-relative path would escape the output folder.
    if (sName.startsWith(QLatin1Char('/')) ||
        sName.contains(QLatin1Char('\\')) ||
        sName.contains(QStringLiteral("//"))) {
        return false;
    }
    const QList<QString> listParts = sName.split(QLatin1Char('/'));
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QStringLiteral(".")) ||
            (sPart == QStringLiteral(".."))) {
            return false;
        }
    }

    *pName = sName;
    return true;
}
}  // namespace

XBinaryII::XBinaryII(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBinaryII::~XBinaryII()
{
}

bool XBinaryII::parseHeader(const QByteArray &baHeader, qint64 nHeaderOffset,
                            MEMBER *pMember)
{
    if (!pMember || (baHeader.size() != BINARYII_HEADER_SIZE)) return false;
    if (memcmp(baHeader.constData(), BINARYII_MAGIC, sizeof(BINARYII_MAGIC)) !=
        0) {
        return false;
    }
    if (binaryiiByte(baHeader, BINARYII_OFFSET_ID) != BINARYII_ID_BYTE) {
        return false;
    }

    const qint32 nNameSize = binaryiiByte(baHeader, BINARYII_OFFSET_NAMESIZE);
    if ((nNameSize < 1) || (nNameSize > BINARYII_MAX_NAME_SIZE)) return false;

    MEMBER member = {};
    if (!binaryiiDecodeName(baHeader.mid(BINARYII_OFFSET_NAME, nNameSize),
                            &member.sFileName)) {
        return false;
    }

    member.nHeaderOffset = nHeaderOffset;
    member.nDataOffset = nHeaderOffset + BINARYII_HEADER_SIZE;
    member.nDeclaredSize =
        static_cast<qint64>(binaryiiWord(baHeader, BINARYII_OFFSET_EOF)) |
        (static_cast<qint64>(binaryiiByte(baHeader, BINARYII_OFFSET_EOF + 2))
         << 16) |
        (static_cast<qint64>(binaryiiByte(baHeader, BINARYII_OFFSET_EOF_HIGH))
         << 24);
    member.nAuxType =
        static_cast<quint32>(binaryiiWord(baHeader, BINARYII_OFFSET_AUXTYPE)) |
        (static_cast<quint32>(
             binaryiiWord(baHeader, BINARYII_OFFSET_AUXTYPE_HIGH))
         << 16);
    member.nBlockCount =
        static_cast<quint32>(
            binaryiiWord(baHeader, BINARYII_OFFSET_BLOCKCOUNT)) |
        (static_cast<quint32>(
             binaryiiWord(baHeader, BINARYII_OFFSET_BLOCKCOUNT_HIGH))
         << 16);
    member.nFileType =
        static_cast<quint16>(binaryiiByte(baHeader, BINARYII_OFFSET_FILETYPE)) |
        (static_cast<quint16>(
             binaryiiByte(baHeader, BINARYII_OFFSET_FILETYPE_HIGH))
         << 8);
    member.nStorageType =
        static_cast<quint16>(
            binaryiiByte(baHeader, BINARYII_OFFSET_STORAGETYPE)) |
        (static_cast<quint16>(
             binaryiiByte(baHeader, BINARYII_OFFSET_STORAGETYPE_HIGH))
         << 8);
    member.nAccess =
        static_cast<quint16>(binaryiiByte(baHeader, BINARYII_OFFSET_ACCESS)) |
        (static_cast<quint16>(
             binaryiiByte(baHeader, BINARYII_OFFSET_ACCESS_HIGH))
         << 8);
    member.nNativeFileType = binaryiiWord(baHeader, BINARYII_OFFSET_NATIVETYPE);
    member.nOSType = binaryiiByte(baHeader, BINARYII_OFFSET_OSTYPE);
    member.nDataFlags = binaryiiByte(baHeader, BINARYII_OFFSET_DATAFLAGS);
    member.nVersion = binaryiiByte(baHeader, BINARYII_OFFSET_VERSION);
    member.nFilesToFollow =
        binaryiiByte(baHeader, BINARYII_OFFSET_FILESTOFOLLOW);
    member.bIsFolder = ((member.nStorageType & 0xffU) ==
                        BINARYII_STORAGETYPE_DIRECTORY);
    member.nStoredSize = member.bIsFolder ? 0 : member.nDeclaredSize;
    member.dtModified = binaryiiProDOSDateTime(
        binaryiiWord(baHeader, BINARYII_OFFSET_MODDATE),
        binaryiiWord(baHeader, BINARYII_OFFSET_MODTIME));
    member.dtCreated = binaryiiProDOSDateTime(
        binaryiiWord(baHeader, BINARYII_OFFSET_CREATEDATE),
        binaryiiWord(baHeader, BINARYII_OFFSET_CREATETIME));

    *pMember = member;
    return true;
}

bool XBinaryII::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BINARYII_HEADER_SIZE) return false;

    qint64 nOffset = 0;
    bool bFollowExpected = false;
    quint8 nExpectedFilesToFollow = 0;
    while (context.listMembers.size() < BINARYII_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!binaryiiRangeWithin(context.nInputSize, nOffset,
                                 BINARYII_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, BINARYII_HEADER_SIZE, pPdStruct);
        if (!guardedSource ||
            (baHeader.size() != BINARYII_HEADER_SIZE)) {
            return false;
        }

        MEMBER member = {};
        if (!parseHeader(baHeader, nOffset, &member)) return false;
        // The countdown is the only terminator this format has.  Requiring it
        // to step by exactly one keeps a stray "\nGL" inside a payload from
        // being adopted as the next member.
        if (bFollowExpected &&
            (member.nFilesToFollow != nExpectedFilesToFollow)) {
            return false;
        }
        if (!binaryiiRangeWithin(context.nInputSize, member.nDataOffset,
                                 member.nStoredSize)) {
            return false;
        }

        context.listMembers.append(member);

        const qint64 nMemberEnd = member.nDataOffset + member.nStoredSize;
        if (member.nFilesToFollow == 0) {
            // The final payload is not always padded on disk: six of the
            // .prog samples stop exactly at nMemberEnd, so the padded end may
            // legitimately lie past the file.  Anything after the aligned end
            // is a ProDOS allocation overlay, not part of the archive.
            const qint64 nAlignedEnd = binaryiiAlignUp(nMemberEnd);
            context.nArchiveSize = qMin(nAlignedEnd, context.nInputSize);
            context.nFirstMemberOffset =
                context.listMembers.first().nHeaderOffset;
            context.nVersion = context.listMembers.first().nVersion;
            *pContext = context;
            return guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }

        nOffset = binaryiiAlignUp(nMemberEnd);
        bFollowExpected = true;
        nExpectedFilesToFollow =
            static_cast<quint8>(member.nFilesToFollow - 1);
    }

    return false;
}

bool XBinaryII::isValid(PDSTRUCT *pPdStruct)
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

bool XBinaryII::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBinaryII archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBinaryII::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBinaryII(pDevice);
}

QList<QString> XBinaryII::getSearchSignatures()
{
    return {QStringLiteral("0A'GL'")};
}

XBinary::FT XBinaryII::getFileType()
{
    return FT_BINARY2;
}

XBinary::MODE XBinaryII::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBinaryII::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBinaryII::getArch()
{
    return QString();
}

QString XBinaryII::getFileFormatExt()
{
    return QStringLiteral("bny");
}

QString XBinaryII::getFileFormatExtsString()
{
    return QStringLiteral("Binary II (*.bny *.bxy *.sdk)");
}

QString XBinaryII::getMIMEString()
{
    return QStringLiteral("application/x-binary2");
}

QString XBinaryII::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XBinaryII::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBinaryII::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBinaryII::getMemoryMap(MAPMODE mapMode,
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

QString XBinaryII::describeMember(const MEMBER &member)
{
    // Binary II never compresses.  The data flag only records the writer's
    // claim that the payload is itself a compressed file (a .SHK or a
    // SQueezed .QQ), so it is reported and never used to pick a decoder.
    QString sResult = QStringLiteral("Stored");
    if (member.bIsFolder) {
        sResult = QStringLiteral("Stored (ProDOS directory)");
    } else if (member.nDataFlags & BINARYII_DATAFLAG_COMPRESSED) {
        sResult = QStringLiteral("Stored (payload flagged as compressed)");
    }
    return QStringLiteral("%1, ProDOS type 0x%2")
        .arg(sResult)
        .arg(member.nFileType, 4, 16, QLatin1Char('0'));
}

bool XBinaryII::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBinaryII::getFileParts(quint32 nFileParts,
                                              qint32 nLimit,
                                              PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = BINARYII_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nStoredSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nStoredSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nStoredSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      describeMember(member));
            part.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);
            part.mapProperties.insert(
                FPART_PROP_TYPE, static_cast<quint32>(member.nFileType));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = BINARYII_HEADER_SIZE + member.nStoredSize;
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
    // A Binary II file is padded to its ProDOS block allocation, so a short
    // zero-filled tail is normal and must be reported as overlay, not treated
    // as a broken chain.
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

QMap<XBinary::UNPACK_PROP, QVariant> XBinaryII::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBinaryII::initUnpack(UNPACK_STATE *pState,
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
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
        tr("Apple II Binary II container; all members are stored"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XBinaryII::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nStoredSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStoredSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nStoredSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                describeMember(member));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nFileType));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);
    // ProDOS access bit 1 is "write enabled"; everything else is read-only.
    result.mapProperties.insert(FPART_PROP_ISREADONLY,
                                (member.nAccess & 0x02U) == 0);
    if (member.dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, member.dtModified);
        result.mapProperties.insert(FPART_PROP_MTIME, member.dtModified);
    }
    if (member.dtCreated.isValid()) {
        result.mapProperties.insert(FPART_PROP_CTIME, member.dtCreated);
    }
    return result;
}

bool XBinaryII::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XBinaryII::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
