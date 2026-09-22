/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xardi2sfx.h"

#include <QDateTime>
#include <QTimeZone>
#include <QtEndian>

#include <cstring>
#include <limits>
#include <new>

namespace {
// The fixed 50-byte trailer.  Only the four year digits and the two numeric
// fields move.
const qint64 ARDI2_TRAILER_SIZE = 50;
const char ARDI2_TRAILER_HEAD[] = {'C', 'o', 'p', 'y', 'r', 'i', 'g', 'h', 't', ' ', 'D', 'a', 'n', 'i', 'e', 'l', ' ', 'F', ' ', 'V', 'a', 'l', 'o', 't', ' '};
const qint64 ARDI2_TRAILER_HEAD_SIZE = 25;
const qint64 ARDI2_TRAILER_TAIL_OFFSET = 31;
const char ARDI2_TRAILER_TAIL[] = {'T', 'S', 'H', 'T', 'S', 'H', ' ', '-', ' ', '1', '9', '9', '1', '-'};
const qint64 ARDI2_TRAILER_TAIL_SIZE = 14;

// The chain's last length word sits four bytes in front of the trailer.
const qint64 ARDI2_LASTLENGTH_BACKOFF = 54;

const quint32 ARDI2_TAG_SENTINEL = 0x98765432U;
const quint32 ARDI2_TAG_MEMBER_HEADER = 0x12345677U;
const quint32 ARDI2_TAG_MEMBER_DATA = 0x12345678U;

// The installer's destination directory, one NUL-terminated path per carrier.
// The reference implementation walks past this block; this reader reads it and
// publishes it -- see the header comment.
const quint32 ARDI2_TAG_INSTALLPATH = 0x11221122U;

// Blocks that carry only presentation text or opaque builder bytes: the
// product title, the installation blurb, the licence text and a four-byte
// record.  They are walked past.  Any OTHER tag ends the walk -- an unknown
// block cannot be skipped safely because its length word is what the walk
// depends on.
const quint32 ARDI2_SKIP_TAGS[] = {0x12121212U, 0x97979797U, 0x98989898U, 0x13131313U};
const qint32 ARDI2_SKIP_TAG_COUNT = 4;

const qint64 ARDI2_MIN_BLOCK_SIZE = 8;             // tag plus length word
const qint64 ARDI2_MIN_HEADER_BLOCK = 9;           // reference: size > 8
const qint64 ARDI2_MAX_HEADER_BLOCK = 0x1008;      // reference: size < 0x1009
const qint32 ARDI2_MAX_BLOCKS = 200000;
const qint32 ARDI2_MAX_MEMBERS = 100000;
const qint32 ARDI2_MAX_PATH_CHARS = 260;

bool ardi2IsSkipTag(quint32 nTag)
{
    for (qint32 i = 0; i < ARDI2_SKIP_TAG_COUNT; i++) {
        if (ARDI2_SKIP_TAGS[i] == nTag) return true;
    }
    return false;
}
}  // namespace

XARDI2SFX::XARDI2SFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XARDI2SFX::~XARDI2SFX()
{
}

bool XARDI2SFX::readTrailer(QString *psYear, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!psYear || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < ARDI2_LASTLENGTH_BACKOFF + ARDI2_MIN_BLOCK_SIZE) return false;

    const QByteArray baTrailer = read_array_process(nInputSize - ARDI2_TRAILER_SIZE, ARDI2_TRAILER_SIZE, pPdStruct);
    if (!guardedSource || (baTrailer.size() != ARDI2_TRAILER_SIZE)) return false;

    const char *pData = baTrailer.constData();
    if (memcmp(pData, ARDI2_TRAILER_HEAD, ARDI2_TRAILER_HEAD_SIZE) != 0) return false;
    if (memcmp(pData + ARDI2_TRAILER_TAIL_OFFSET, ARDI2_TRAILER_TAIL, ARDI2_TRAILER_TAIL_SIZE) != 0) return false;
    if (pData[ARDI2_TRAILER_SIZE - 1] != ' ') return false;

    QString sYear;
    for (qint32 i = 0; i < 4; i++) {
        const char cDigit = pData[ARDI2_TRAILER_TAIL_OFFSET + ARDI2_TRAILER_TAIL_SIZE + i];
        if ((cDigit < '0') || (cDigit > '9')) return false;
        sYear.append(QChar::fromLatin1(cDigit));
    }

    *psYear = sYear;
    return true;
}

// Walks the block chain from the tail to the sentinel.  Every step is bounded
// by the file and by a strictly decreasing position, so a length word that
// points forward or nowhere ends the walk instead of looping.
//
// pPathBlock is optional; when supplied it receives the first 0x11221122 block
// the walk meets, which is the installer's destination directory.
bool XARDI2SFX::walkChain(QList<BLOCK> *pListBlocks, BLOCK *pPathBlock, qint64 *pnSentinelOffset, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pListBlocks || !pnSentinelOffset || !guardedSource) return false;

    const qint64 nInputSize = guardedSource->size();
    qint64 nPosition = nInputSize - ARDI2_LASTLENGTH_BACKOFF;
    if (nPosition < 0) return false;

    QList<BLOCK> listBlocks;
    BLOCK pathBlock = {};

    for (qint32 i = 0; i < ARDI2_MAX_BLOCKS; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nPosition < 0) || (nPosition > nInputSize - 4)) return false;

        const QByteArray baLength = read_array_process(nPosition, 4, pPdStruct);
        if (!guardedSource || (baLength.size() != 4)) return false;
        const quint32 nLength = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baLength.constData()));

        if (nLength == ARDI2_TAG_SENTINEL) {
            if (listBlocks.isEmpty()) return false;
            *pnSentinelOffset = nPosition;
            *pListBlocks = listBlocks;
            if (pPathBlock) *pPathBlock = pathBlock;
            return true;
        }

        if ((nLength < ARDI2_MIN_BLOCK_SIZE) || (nLength > 0x7FFFFFFFU)) return false;
        const qint64 nTagOffset = nPosition + 4 - static_cast<qint64>(nLength);
        if ((nTagOffset < 0) || (nTagOffset > nPosition - 4)) return false;

        const QByteArray baTag = read_array_process(nTagOffset, 4, pPdStruct);
        if (!guardedSource || (baTag.size() != 4)) return false;
        const quint32 nTag = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTag.constData()));

        if ((nTag == ARDI2_TAG_MEMBER_HEADER) || (nTag == ARDI2_TAG_MEMBER_DATA)) {
            BLOCK block = {};
            block.nOffset = nTagOffset;
            block.nSize = static_cast<qint64>(nLength) - 4;
            block.nTag = nTag;
            listBlocks.append(block);
        } else if (nTag == ARDI2_TAG_INSTALLPATH) {
            // The container's own destination directory.  It is recorded here
            // and decoded by parseContext; the walk still steps over it, since
            // it is not a member.
            if (pathBlock.nSize == 0) {
                pathBlock.nOffset = nTagOffset;
                pathBlock.nSize = static_cast<qint64>(nLength) - 4;
                pathBlock.nTag = nTag;
            }
        } else if (!ardi2IsSkipTag(nTag)) {
            return false;
        }

        nPosition = nTagOffset - 4;
    }

    return false;
}

// The 0x11221122 payload is a single NUL-terminated path.  It is read as
// stored: no separator is rewritten, nothing is stripped, and a payload that
// is not a plain path is discarded rather than repaired.
bool XARDI2SFX::readInstallPath(const BLOCK &block, QString *psPath, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!psPath || !guardedSource) return false;
    if ((block.nTag != ARDI2_TAG_INSTALLPATH) || (block.nSize <= 4) || (block.nSize > ARDI2_MAX_HEADER_BLOCK)) return false;

    const QByteArray baBlock = read_array_process(block.nOffset + 4, block.nSize - 4, pPdStruct);
    if (!guardedSource || (baBlock.size() != block.nSize - 4)) return false;

    const qint32 nZero = baBlock.indexOf('\0');
    const QByteArray baText = (nZero >= 0) ? baBlock.left(nZero) : baBlock;
    if (baText.isEmpty() || (baText.size() > ARDI2_MAX_PATH_CHARS)) return false;

    for (qint32 i = 0; i < baText.size(); i++) {
        const uchar cByte = static_cast<uchar>(baText.at(i));
        if (cByte < 0x20) return false;
    }

    *psPath = QString::fromLatin1(baText.constData(), baText.size());
    return true;
}

bool XARDI2SFX::readMemberHeader(const BLOCK &block, MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pMember || !guardedSource) return false;
    if ((block.nTag != ARDI2_TAG_MEMBER_HEADER) || (block.nSize < ARDI2_MIN_HEADER_BLOCK) || (block.nSize > ARDI2_MAX_HEADER_BLOCK)) return false;

    const QByteArray baBlock = read_array_process(block.nOffset, block.nSize, pPdStruct);
    if (!guardedSource || (baBlock.size() != block.nSize)) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baBlock.constData());
    if (qFromLittleEndian<quint32>(pData) != ARDI2_TAG_MEMBER_HEADER) return false;

    const quint32 nUnixTime = qFromLittleEndian<quint32>(pData + 4);
    const qint64 nTextSize = block.nSize - 8;

    qint64 nSeparator = -1;
    for (qint64 i = 0; i < nTextSize; i++) {
        if (pData[8 + i] == '!') {
            nSeparator = i;
            break;
        }
    }
    // The reference implementation refuses a header whose '!' is missing or is
    // the very first byte; an empty name is not a member.
    if (nSeparator <= 0) return false;

    pMember->sName = QString::fromLatin1(baBlock.constData() + 8, static_cast<int>(nSeparator));
    if (pMember->sName.isEmpty()) return false;

    // Behind the separator the container stores a one-character kind code and a
    // human description.  The kind codes are not documented anywhere this
    // reader can check, so the remainder is published verbatim as information
    // and nothing is inferred from it.
    QString sRemainder = QString::fromLatin1(baBlock.constData() + 8 + nSeparator + 1, static_cast<int>(nTextSize - nSeparator - 1));
    const qint32 nZero = sRemainder.indexOf(QChar(0));
    if (nZero >= 0) sRemainder = sRemainder.left(nZero);
    pMember->sDescription = sRemainder;

    pMember->nUnixTime = nUnixTime;
    // Stamps outside the years the tool could have been used in are dropped
    // rather than published as a nonsense date.
    pMember->bHasTime = (nUnixTime >= 631152000U) && (nUnixTime <= 2147483647U);

    return true;
}

bool XARDI2SFX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nSentinelOffset = -1;

    if (!readTrailer(&context.sTrailerYear, pPdStruct) || !guardedSource) return false;

    QList<BLOCK> listBlocks;
    BLOCK pathBlock = {};
    if (!walkChain(&listBlocks, &pathBlock, &context.nSentinelOffset, pPdStruct) || !guardedSource) return false;

    // The destination directory the container states.  A carrier that does not
    // carry one is still read; the path is simply not published.
    if (pathBlock.nSize > 0) {
        QString sInstallPath;
        if (readInstallPath(pathBlock, &sInstallPath, pPdStruct) && guardedSource) {
            context.sInstallPath = sInstallPath;
        }
        if (!guardedSource) return false;
    }

    // Every member is a header block immediately followed by its data block
    // once the non-member blocks are dropped, so an odd count means a header
    // with no data.  The container never produces one, and a file that does is
    // not understood rather than half-read.
    const qint32 nBlockCount = listBlocks.size();
    if ((nBlockCount < 2) || ((nBlockCount % 2) != 0)) return false;

    // The walk collected the blocks from the tail forwards; step back through
    // it so the members come out in the order the container stores them, which
    // is the order the reference implementation extracts in.
    for (qint32 i = nBlockCount - 1; i >= 1; i -= 2) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const BLOCK &headerBlock = listBlocks.at(i);
        const BLOCK &dataBlock = listBlocks.at(i - 1);
        if ((headerBlock.nTag != ARDI2_TAG_MEMBER_HEADER) || (dataBlock.nTag != ARDI2_TAG_MEMBER_DATA)) return false;
        if (dataBlock.nSize < 4) return false;

        MEMBER member = {};
        if (!readMemberHeader(headerBlock, &member, pPdStruct) || !guardedSource) return false;

        member.nStreamOffset = dataBlock.nOffset + 4;
        member.nStreamSize = dataBlock.nSize - 4;
        if ((member.nStreamOffset < 0) || (member.nStreamSize < 0) || (member.nStreamOffset > context.nInputSize - member.nStreamSize)) return false;

        context.listMembers.append(member);
        if (context.listMembers.size() > ARDI2_MAX_MEMBERS) return false;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nTotalSize = context.nInputSize;

    *pContext = context;
    return true;
}

XBinary::ARCHIVERECORD XARDI2SFX::recordAt(const CONTEXT &context, qint32 nIndex)
{
    if ((nIndex < 0) || (nIndex >= context.listMembers.size())) return ARCHIVERECORD();
    const MEMBER &member = context.listMembers.at(nIndex);

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nStreamOffset;
    result.nStreamSize = member.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sName);
    result.mapProperties.insert(FPART_PROP_STREAMOFFSET, member.nStreamOffset);
    result.mapProperties.insert(FPART_PROP_STREAMSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nStreamSize);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate"));
    // No FPART_PROP_UNCOMPRESSEDSIZE: the container records no inflated length
    // for a member anywhere, and a declared size here would be a guess that
    // XDeflateDecoder would then reject the stream for not matching.
    if (member.bHasTime) {
        result.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch(static_cast<qint64>(member.nUnixTime), X_UTC_TZ));
    }
    if (!member.sDescription.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_INFO, member.sDescription);
    }
    // The 0x11221122 block is the installer's destination directory: an
    // archive-level, absolute OS/2 path that the user may override at install
    // time, and not part of any member name.  It is published so that the
    // container's own path is visible, and the extracted layout stays flat --
    // which is what the stub writes and what XQuarterdeckQP does with the same
    // kind of record.
    if (!context.sInstallPath.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_PREFIX, context.sInstallPath);
    }
    return result;
}

qint64 XARDI2SFX::recordOffset(const CONTEXT &context, qint32 nIndex)
{
    if ((nIndex < 0) || (nIndex >= context.listMembers.size())) return -1;
    return context.listMembers.at(nIndex).nStreamOffset;
}

bool XARDI2SFX::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    QString sYear;
    bool bResult = readTrailer(&sYear, pPdStruct);
    if (bResult) {
        // The trailer alone is the author's copyright watermark and appears on
        // his ordinary products too, so the chain walk is part of the gate.
        QList<BLOCK> listBlocks;
        qint64 nSentinelOffset = -1;
        bResult = walkChain(&listBlocks, nullptr, &nSentinelOffset, pPdStruct);
    }
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XARDI2SFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARDI2SFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XARDI2SFX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XARDI2SFX(pDevice);
}

QList<QString> XARDI2SFX::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("3254769897979797"));
    return listResult;
}

XBinary::FT XARDI2SFX::getFileType()
{
    return FT_ARDI2_SFX;
}

XBinary::MODE XARDI2SFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XARDI2SFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XARDI2SFX::getArch()
{
    return QString();
}

QString XARDI2SFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XARDI2SFX::getFileFormatExtsString()
{
    return QStringLiteral("ARDI installer SFX (*.exe)");
}

QString XARDI2SFX::getMIMEString()
{
    return QStringLiteral("application/x-ardi-installer-sfx");
}

QString XARDI2SFX::getVersion()
{
    QString sYear;
    if (!readTrailer(&sYear, nullptr)) return QString();
    return QStringLiteral("1991-") + sYear;
}

qint64 XARDI2SFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nTotalSize : 0;
}

QList<XBinary::MAPMODE> XARDI2SFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XARDI2SFX::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_REGION, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XARDI2SFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XARDI2SFX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nSentinelOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if (nFileParts & FILEPART_STREAM) {
        const qint32 nCount = context.listMembers.size();
        for (qint32 i = 0; i < nCount; i++) {
            if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
            const ARCHIVERECORD record = recordAt(context, i);
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = record.nStreamOffset;
            part.nFileSize = record.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
            part.mapProperties = record.mapProperties;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nInputSize - ARDI2_TRAILER_SIZE;
        part.nFileSize = ARDI2_TRAILER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Trailer");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nTotalSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XARDI2SFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XARDI2SFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    // Name the stored destination rather than deny it.  The path is the
    // container's own; the members are written flat into it.
    if (pContext->sInstallPath.isEmpty()) {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("ARDI self-extracting installer; members are written flat into one destination directory and "
                                                                "this carrier stores no default for it"));
    } else {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("ARDI self-extracting installer; members are written flat into the stored destination directory "
                                                                "%1, which the user may change at install time")
                                                                 .arg(pContext->sInstallPath));
    }
    pState->nCurrentOffset = recordOffset(*pContext, 0);
    pState->nTotalSize = pContext->nTotalSize;
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

XBinary::ARCHIVERECORD XARDI2SFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const qint32 nIndex = pState->nCurrentIndex;
    if (pState->nCurrentOffset != recordOffset(*pContext, nIndex)) return ARCHIVERECORD();

    return recordAt(*pContext, nIndex);
}

bool XARDI2SFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = recordOffset(*pContext, pState->nCurrentIndex);
        return true;
    }
    pState->nCurrentOffset = pContext->nTotalSize;
    return false;
}

bool XARDI2SFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
