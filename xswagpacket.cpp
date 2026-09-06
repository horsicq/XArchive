/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xswagpacket.h"

#include <QPointer>

#include <cstring>
#include <memory>
#include <new>

namespace {
// The banner is written verbatim by SWAGOLX.EXE and is the only magic the
// format has; it occupies the first 48 bytes of the first block.
const char SWAG_BANNER[] = "SWAGOLX.EXE (c) 1993 GDSOFT  ALL RIGHTS RESERVED";
const qint64 SWAG_BANNER_SIZE = 48;
const qint64 SWAG_BLOCK_SIZE = 128;

// File header block (128 bytes):
//   +0x00 banner[48] | +0x30 ' ' | +0x31 snippet count, 5 ASCII digits |
//   +0x36 nine spaces | +0x3f packet title[65]  ->  48+1+5+9+65 = 128.
const qint64 SWAG_FILE_COUNT_OFFSET = 0x31;
const qint64 SWAG_FILE_COUNT_SIZE = 5;
const qint64 SWAG_FILE_GAP_OFFSET = 0x36;
const qint64 SWAG_FILE_GAP_SIZE = 9;
const qint64 SWAG_FILE_TITLE_OFFSET = 0x3f;
const qint64 SWAG_FILE_TITLE_SIZE = 65;

// Member header block (128 bytes).  Every field is space padded ASCII; the
// widths below were measured against U3's extraction of all 227 corpus files
// and account for all 128 bytes:
//   +0x00 index[8] | +0x08 date[8] "MM-DD-YY" | +0x10 time[5] "HH:MM" |
//   +0x15 author[25] | +0x2e contributor[25] | +0x47 subject[25] |
//   +0x60 keyword[20] | +0x74 block count[7] | +0x7b opaque[4] | +0x7f ' '
const qint64 SWAG_REC_INDEX_OFFSET = 0x00;
const qint64 SWAG_REC_INDEX_SIZE = 8;
const qint64 SWAG_REC_DATE_OFFSET = 0x08;
const qint64 SWAG_REC_DATE_SIZE = 8;
const qint64 SWAG_REC_TIME_OFFSET = 0x10;
const qint64 SWAG_REC_TIME_SIZE = 5;
const qint64 SWAG_REC_AUTHOR_OFFSET = 0x15;
const qint64 SWAG_REC_AUTHOR_SIZE = 25;
const qint64 SWAG_REC_CONTRIBUTOR_OFFSET = 0x2e;
const qint64 SWAG_REC_CONTRIBUTOR_SIZE = 25;
const qint64 SWAG_REC_SUBJECT_OFFSET = 0x47;
const qint64 SWAG_REC_SUBJECT_SIZE = 25;
const qint64 SWAG_REC_KEYWORD_OFFSET = 0x60;
const qint64 SWAG_REC_KEYWORD_SIZE = 20;
const qint64 SWAG_REC_BLOCKS_OFFSET = 0x74;
const qint64 SWAG_REC_BLOCKS_SIZE = 7;
const qint64 SWAG_REC_TAIL_OFFSET = 0x7f;

// Snippet text markers.
const char SWAG_TEXT_TERMINATOR = 0x1a;
const char SWAG_TEXT_EOL = static_cast<char>(0xe3);
const char SWAG_TEXT_EOL_OUT = 0x0d;

const qint32 SWAG_MAX_MEMBERS = 99999;      // the count field is 5 digits
const qint64 SWAG_MAX_BLOCKS = 9999999;     // the block field is 7 digits
const qint64 SWAG_MAX_REGION = 0x4000000;   // 64 MB per-member sanity cap

bool swagRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

QString swagTrimField(const QByteArray &baHeader, qint64 nOffset, qint64 nSize)
{
    return QString::fromLatin1(baHeader.mid(static_cast<int>(nOffset),
                                            static_cast<int>(nSize)))
        .trimmed();
}

bool swagIsSpaces(const QByteArray &baData, qint64 nOffset, qint64 nSize)
{
    for (qint64 i = 0; i < nSize; i++) {
        if (baData.at(static_cast<int>(nOffset + i)) != ' ') return false;
    }
    return true;
}

// A space padded unsigned decimal: digits first, spaces after, nothing else.
// Returns false on an empty field so a blank block count can never be read
// as zero and stall the walk.
bool swagParseNumber(const QByteArray &baData, qint64 nOffset, qint64 nSize,
                     qint64 *pnResult)
{
    qint64 nValue = 0;
    qint32 nDigits = 0;
    bool bPadding = false;
    for (qint64 i = 0; i < nSize; i++) {
        const char cCharacter = baData.at(static_cast<int>(nOffset + i));
        if (cCharacter == ' ') {
            bPadding = true;
        } else if ((cCharacter >= '0') && (cCharacter <= '9') && !bPadding) {
            nValue = nValue * 10 + (cCharacter - '0');
            nDigits++;
            if (nValue > SWAG_MAX_BLOCKS) return false;
        } else {
            return false;
        }
    }
    if (nDigits == 0) return false;
    *pnResult = nValue;
    return true;
}

// "MM-DD-YY"; the two-digit year is a DOS-era one, so 80..99 is 19xx.
bool swagParseDate(const QString &sDate, qint32 *pnYear, qint32 *pnMonth,
                   qint32 *pnDay)
{
    if (sDate.size() != 8) return false;
    if ((sDate.at(2) != QLatin1Char('-')) ||
        (sDate.at(5) != QLatin1Char('-'))) {
        return false;
    }
    const QString sMonth = sDate.mid(0, 2);
    const QString sDay = sDate.mid(3, 2);
    const QString sYear = sDate.mid(6, 2);
    bool bMonth = false;
    bool bDay = false;
    bool bYear = false;
    const qint32 nMonth = sMonth.toInt(&bMonth);
    const qint32 nDay = sDay.toInt(&bDay);
    const qint32 nYear = sYear.toInt(&bYear);
    if (!bMonth || !bDay || !bYear) return false;
    *pnMonth = nMonth;
    *pnDay = nDay;
    *pnYear = (nYear >= 80) ? (1900 + nYear) : (2000 + nYear);
    return true;
}

bool swagParseTime(const QString &sTime, qint32 *pnHour, qint32 *pnMinute)
{
    if (sTime.size() != 5) return false;
    if (sTime.at(2) != QLatin1Char(':')) return false;
    bool bHour = false;
    bool bMinute = false;
    const qint32 nHour = sTime.mid(0, 2).toInt(&bHour);
    const qint32 nMinute = sTime.mid(3, 2).toInt(&bMinute);
    if (!bHour || !bMinute) return false;
    *pnHour = nHour;
    *pnMinute = nMinute;
    return true;
}
}  // namespace

XSwagPacket::XSwagPacket(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSwagPacket::~XSwagPacket()
{
}

qint64 XSwagPacket::trimmedTextSize(const QByteArray &baRegion)
{
    const int nTerminator = baRegion.indexOf(SWAG_TEXT_TERMINATOR);
    int nSize = (nTerminator >= 0) ? nTerminator : baRegion.size();
    // The writer flushes whole 128-byte blocks, so the tail of the last block
    // is space padding.  U3 drops it; the 0x1A can sit either before or after
    // that padding, so the trim has to run in both cases.
    while ((nSize > 0) && (baRegion.at(nSize - 1) == ' ')) nSize--;
    return nSize;
}

bool XSwagPacket::parseContext(CONTEXT *pContext, bool bComputeText,
                               PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSwagPacket> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // A packet needs the file header block plus at least one member block.
    if (context.nInputSize < SWAG_BLOCK_SIZE * 2) return false;

    const QByteArray baFileHeader =
        read_array_process(0, SWAG_BLOCK_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baFileHeader.size() != SWAG_BLOCK_SIZE)) {
        return false;
    }
    if (std::memcmp(baFileHeader.constData(), SWAG_BANNER,
                    static_cast<size_t>(SWAG_BANNER_SIZE)) != 0) {
        return false;
    }
    if (baFileHeader.at(static_cast<int>(SWAG_BANNER_SIZE)) != ' ') {
        return false;
    }
    if (!swagIsSpaces(baFileHeader, SWAG_FILE_GAP_OFFSET, SWAG_FILE_GAP_SIZE)) {
        return false;
    }
    qint64 nCount = 0;
    if (!swagParseNumber(baFileHeader, SWAG_FILE_COUNT_OFFSET,
                         SWAG_FILE_COUNT_SIZE, &nCount)) {
        return false;
    }
    if ((nCount < 1) || (nCount > SWAG_MAX_MEMBERS)) return false;
    context.sPacketTitle = swagTrimField(baFileHeader, SWAG_FILE_TITLE_OFFSET,
                                         SWAG_FILE_TITLE_SIZE);

    qint64 nOffset = SWAG_BLOCK_SIZE;
    for (qint64 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!swagRangeWithin(context.nInputSize, nOffset, SWAG_BLOCK_SIZE)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, SWAG_BLOCK_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baHeader.size() != SWAG_BLOCK_SIZE)) {
            return false;
        }

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + SWAG_BLOCK_SIZE;
        if (!swagParseNumber(baHeader, SWAG_REC_BLOCKS_OFFSET,
                             SWAG_REC_BLOCKS_SIZE, &member.nBlockCount)) {
            return false;
        }
        // The block count covers the member header too, so a member with no
        // payload at all would still be at least two blocks.  One block means
        // a corrupt or misidentified file, and zero would loop forever.
        if (member.nBlockCount < 2) return false;
        // The last byte of the header block is a fixed separator space; it is
        // the cheapest structural cross-check the format offers.
        if (baHeader.at(static_cast<int>(SWAG_REC_TAIL_OFFSET)) != ' ') {
            return false;
        }
        member.nRegionSize = (member.nBlockCount - 1) * SWAG_BLOCK_SIZE;
        if (member.nRegionSize > SWAG_MAX_REGION) return false;
        if (!swagRangeWithin(context.nInputSize, member.nDataOffset,
                             member.nRegionSize)) {
            return false;
        }

        member.sIndex = swagTrimField(baHeader, SWAG_REC_INDEX_OFFSET,
                                      SWAG_REC_INDEX_SIZE);
        member.sDate = QString::fromLatin1(
            baHeader.mid(static_cast<int>(SWAG_REC_DATE_OFFSET),
                         static_cast<int>(SWAG_REC_DATE_SIZE)));
        member.sTime = QString::fromLatin1(
            baHeader.mid(static_cast<int>(SWAG_REC_TIME_OFFSET),
                         static_cast<int>(SWAG_REC_TIME_SIZE)));
        member.sAuthor = swagTrimField(baHeader, SWAG_REC_AUTHOR_OFFSET,
                                       SWAG_REC_AUTHOR_SIZE);
        member.sContributor = swagTrimField(baHeader,
                                            SWAG_REC_CONTRIBUTOR_OFFSET,
                                            SWAG_REC_CONTRIBUTOR_SIZE);
        member.sSubject = swagTrimField(baHeader, SWAG_REC_SUBJECT_OFFSET,
                                        SWAG_REC_SUBJECT_SIZE);
        member.sKeyword = swagTrimField(baHeader, SWAG_REC_KEYWORD_OFFSET,
                                        SWAG_REC_KEYWORD_SIZE);
        // The index field is a plain 1-based ordinal on every member of every
        // known packet.  Requiring it turns a chance banner match inside an
        // unrelated file into a reject instead of a garbage listing.
        if (member.sIndex != QString::number(i + 1)) return false;
        qint32 nYear = 0;
        qint32 nMonth = 0;
        qint32 nDay = 0;
        qint32 nHour = 0;
        qint32 nMinute = 0;
        if (!swagParseDate(member.sDate, &nYear, &nMonth, &nDay)) return false;
        if (!swagParseTime(member.sTime, &nHour, &nMinute)) return false;

        // SWAG snippets are Pascal source and SWAGOLX stores no member name;
        // U3 numbers them, which is the only stable naming this format has.
        member.sFileName = QString("%1.pas").arg(i + 1, 4, 10,
                                                 QLatin1Char('0'));

        if (bComputeText) {
            const QByteArray baRegion = read_array_process(
                member.nDataOffset, member.nRegionSize, pPdStruct);
            if (!guardedThis || !guardedSource ||
                (baRegion.size() != member.nRegionSize)) {
                return false;
            }
            member.nTextSize = trimmedTextSize(baRegion);
        } else {
            member.nTextSize = -1;
        }

        context.listMembers.append(member);
        nOffset += member.nBlockCount * SWAG_BLOCK_SIZE;
        if (nOffset > context.nInputSize) return false;
    }

    // The declared snippet count and the block chain have to agree with the
    // file size exactly.  Every corpus packet ends on the last member's last
    // block, so a short or long tail is a reject rather than an overlay.
    if (nOffset != context.nInputSize) return false;
    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nOffset;
    context.nFirstMemberOffset = context.listMembers.first().nHeaderOffset;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

QByteArray XSwagPacket::decodeMember(const MEMBER &member,
                                     PDSTRUCT *pPdStruct)
{
    QPointer<XSwagPacket> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || (member.nRegionSize <= 0)) return QByteArray();
    QByteArray baRegion =
        read_array_process(member.nDataOffset, member.nRegionSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baRegion.size() != member.nRegionSize)) {
        return QByteArray();
    }
    baRegion.resize(static_cast<int>(trimmedTextSize(baRegion)));
    // Length preserving: 0xE3 is the SWAG reader's on-screen line break and
    // becomes CR, exactly one output byte per input byte.
    baRegion.replace(SWAG_TEXT_EOL, SWAG_TEXT_EOL_OUT);
    return baRegion;
}

QDateTime XSwagPacket::memberDateTime(const MEMBER &member)
{
    qint32 nYear = 0;
    qint32 nMonth = 0;
    qint32 nDay = 0;
    qint32 nHour = 0;
    qint32 nMinute = 0;
    if (!swagParseDate(member.sDate, &nYear, &nMonth, &nDay)) {
        return QDateTime();
    }
    if (!swagParseTime(member.sTime, &nHour, &nMinute)) return QDateTime();
    const QDate date(nYear, nMonth, nDay);
    const QTime time(nHour, nMinute, 0);
    if (!date.isValid() || !time.isValid()) return QDateTime();
    return QDateTime(date, time);
}

bool XSwagPacket::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSwagPacket::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSwagPacket archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSwagPacket::createInstance(QIODevice *pDevice, bool bIsImage,
                                     XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSwagPacket(pDevice);
}

QList<QString> XSwagPacket::getSearchSignatures()
{
    // Deliberately shorter than the 48 bytes isValid() insists on: the scan
    // only has to nominate a candidate, the structural walk decides.
    return {QStringLiteral("'SWAGOLX.EXE (c) 1993 GDSOFT'")};
}

XBinary::FT XSwagPacket::getFileType()
{
    return FT_SWAG_PACKET;
}

XBinary::MODE XSwagPacket::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSwagPacket::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSwagPacket::getArch()
{
    return QString();
}

QString XSwagPacket::getFileFormatExt()
{
    return QStringLiteral("swg");
}

QString XSwagPacket::getFileFormatExtsString()
{
    return QStringLiteral("SWAG packet (*.swg)");
}

QString XSwagPacket::getMIMEString()
{
    return QStringLiteral("application/x-swag-packet");
}

QString XSwagPacket::getVersion()
{
    // The banner carries the writer's release year and no version number.
    return QStringLiteral("1993");
}

qint64 XSwagPacket::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSwagPacket::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSwagPacket::getMemoryMap(MAPMODE mapMode,
                                               PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XSwagPacket::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSwagPacket::getFileParts(quint32 nFileParts,
                                                qint32 nLimit,
                                                PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return result;

    if (nFileParts & FILEPART_HEADER) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SWAG_BLOCK_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if ((nFileParts & FILEPART_HEADER) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = SWAG_BLOCK_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nTextSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nTextSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nTextSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nBlockCount * SWAG_BLOCK_SIZE;
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
        // parseContext() only accepts a chain that lands exactly on EOF, so
        // this branch cannot fire today; it is kept so the part list stays
        // correct if that rule is ever relaxed.
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

QMap<XBinary::UNPACK_PROP, QVariant> XSwagPacket::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSwagPacket::initUnpack(UNPACK_STATE *pState,
                             const QMap<UNPACK_PROP, QVariant> &mapProperties,
                             PDSTRUCT *pPdStruct)
{
    QPointer<XSwagPacket> guardedThis(this);
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("SWAG packet (SWAGOLX.EXE 1993); stored snippets"));
    if (!pContext->sPacketTitle.isEmpty()) {
        pState->mapArchiveProperties.insert(FPART_PROP_ORIGINALNAME,
                                            pContext->sPacketTitle);
    }
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
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

XBinary::ARCHIVERECORD XSwagPacket::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();
    if (member.nTextSize < 0) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nTextSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nTextSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nTextSize);
    // The stored bytes and the emitted bytes have the same length; only the
    // 0xE3 line separator differs, and unpackCurrent() applies that mapping.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dateTime = memberDateTime(member);
    if (dateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);
    }
    if (!member.sSubject.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_INFO, member.sSubject);
    }
    if (!member.sAuthor.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_USERNAME, member.sAuthor);
    }
    if (!member.sKeyword.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_PREFIX, member.sKeyword);
    }
    return result;
}

bool XSwagPacket::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSwagPacket::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XSwagPacket> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !guardedSource || !guardedOutput ||
        !isUnpackOutputSupported(guardedOutput.data()) ||
        devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    if ((pState->nCurrentOffset != member.nHeaderOffset) ||
        (member.nTextSize < 0)) {
        return false;
    }
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   member.nTextSize)) {
        return false;
    }

    // This route materializes its own output, so it charges the operation
    // budget itself: publishUnpackOutput never debits the copy.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                member.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(member.nTextSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    const QByteArray baDecoded = decodeMember(member, pPdStruct);
    if (!guardedThis || !guardedSource || !guardedOutput ||
        (baDecoded.size() != member.nTextSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(member.nTextSize,
                                                       pPdStruct));
    if (!pStage || !guardedThis || !guardedOutput) return false;
    if (member.nTextSize > 0) {
        if (pStage->write(baDecoded) != member.nTextSize) return false;
    }
    if (!pStage->seek(0) || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis) {
        return false;
    }
    return guardedThis->publishUnpackOutput(pStage.get(),
                                            guardedOutput.data(), pState,
                                            pPdStruct);
}

bool XSwagPacket::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
