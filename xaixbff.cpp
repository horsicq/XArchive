/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xaixbff.h"

#include <QPointer>
#include <QtEndian>

#include <limits>
#include <new>
#include <QTimeZone>

namespace {
// 09 00 6B EA on disk.  The 0x09 is the volume header size in 8-byte words
// (0x48 / 8), so the magic doubles as the "first record starts at 0x48" rule.
const quint32 BFF_VOLUME_MAGIC = 0xea6b0009U;
const qint64 BFF_VOLUME_HEADER_SIZE = 0x48;
const qint64 BFF_FIXED_HEADER_SIZE = 0x40;
// Every record header is followed by a duplicated 40-byte attribute block that
// nothing needs to read.  Skipping it is not optional: without it every single
// payload is located 40 bytes early, which still "works" for a couple of
// records before the walk derails into noise.
const qint64 BFF_TRAILER_SIZE = 40;
const qint64 BFF_MIN_SIZE =
    BFF_VOLUME_HEADER_SIZE + BFF_FIXED_HEADER_SIZE + 8;
const quint16 BFF_MAGIC_STORED = 0xea6bU;
const quint16 BFF_MAGIC_PACKED = 0xea6cU;
const quint8 BFF_RECORD_MEMBER = 0x0bU;
const quint8 BFF_RECORD_TERMINATOR = 0x07U;
const quint8 BFF_MIN_HEADER_WORDS = 9;  // 0x40 fixed part + one 8-byte name word
const quint32 BFF_MODE_FORMAT_MASK = 0xf000U;
const quint32 BFF_MODE_DIRECTORY = 0x4000U;
const quint32 BFF_MODE_REGULAR = 0x8000U;
const qint32 BFF_MAX_MEMBERS = 200000;
const qint64 BFF_TAPE_BLOCK_SIZE = 1024;

bool bffRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

qint64 bffAlignUp(qint64 nValue, qint64 nAlignment)
{
    if (nValue < 0 || nAlignment <= 0) return -1;
    const qint64 nRemainder = nValue % nAlignment;
    if (nRemainder == 0) return nValue;
    if (nValue > (std::numeric_limits<qint64>::max)() -
                     (nAlignment - nRemainder)) {
        return -1;
    }
    return nValue + (nAlignment - nRemainder);
}

bool bffIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}

// The bytes between the name's NUL and the end of the name area are stale
// leftovers from a previously written, longer path (roughly half the corpus's
// records carry such junk), so the name must be cut at the first NUL and the
// padding must never be validated.
QByteArray bffExtractName(const QByteArray &baHeader, qint64 nNameAreaSize)
{
    if (baHeader.size() < nNameAreaSize ||
        nNameAreaSize <= BFF_FIXED_HEADER_SIZE) {
        return QByteArray();
    }
    const qint32 nStart = static_cast<qint32>(BFF_FIXED_HEADER_SIZE);
    const qint32 nLimit = static_cast<qint32>(nNameAreaSize);
    for (qint32 i = nStart; i < nLimit; ++i) {
        if (baHeader.at(i) == '\0') {
            return baHeader.mid(nStart, i - nStart);
        }
    }
    return QByteArray();
}

QString bffLabel(const QByteArray &baField)
{
    QByteArray baResult = baField;
    const int nZero = baResult.indexOf('\0');
    if (nZero >= 0) baResult.truncate(nZero);
    if (!bffIsValidName(baResult)) return QString();
    return QString::fromLatin1(baResult).trimmed();
}
}  // namespace

XAIXBFF::XAIXBFF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAIXBFF::~XAIXBFF()
{
}

bool XAIXBFF::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XAIXBFF> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BFF_MIN_SIZE) return false;

    const QByteArray baVolume =
        read_array_process(0, BFF_VOLUME_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baVolume.size() != BFF_VOLUME_HEADER_SIZE) {
        return false;
    }
    const uchar *pVolume =
        reinterpret_cast<const uchar *>(baVolume.constData());
    if (qFromLittleEndian<quint32>(pVolume) != BFF_VOLUME_MAGIC) return false;

    context.nBackupTime = qFromLittleEndian<quint32>(pVolume + 0x0c);
    context.sVolumeLabel = bffLabel(baVolume.mid(0x34, 16));
    context.nFirstMemberOffset = BFF_VOLUME_HEADER_SIZE;

    qint64 nOffset = BFF_VOLUME_HEADER_SIZE;
    while (context.listMembers.size() < BFF_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        // Only four bytes are read before the record type is known.  The
        // terminator record is a single 8-byte word and five of the sixty-six
        // corpus archives end within a dozen bytes of it, so demanding a full
        // 0x40-byte header up front rejects otherwise perfect files.
        if (!bffRangeWithin(context.nInputSize, nOffset, 4)) return false;
        const QByteArray baLead = read_array_process(nOffset, 4, pPdStruct);
        if (!guardedThis || !guardedSource || baLead.size() != 4) return false;
        const uchar *pLead =
            reinterpret_cast<const uchar *>(baLead.constData());
        const quint8 nHeaderWords = pLead[0];
        const quint8 nRecordType = pLead[1];
        const quint16 nMagic = qFromLittleEndian<quint16>(pLead + 2);
        if ((nMagic != BFF_MAGIC_STORED) && (nMagic != BFF_MAGIC_PACKED)) {
            return false;
        }

        if (nRecordType == BFF_RECORD_TERMINATOR) {
            // Everything past this sentinel is stale tape-buffer content that
            // does contain accidental 6B EA / 6C EA pairs at 8-aligned
            // offsets.  A scanner that resynchronises on the next magic instead
            // of honouring the sentinel invents members out of noise, so the
            // walk stops here unconditionally.
            if (context.listMembers.isEmpty()) return false;
            context.nTerminatorOffset = nOffset;
            const qint64 nPadded =
                bffAlignUp(nOffset + 4, BFF_TAPE_BLOCK_SIZE);
            context.nArchiveSize =
                (nPadded < 0) ? (nOffset + 4)
                              : qMin(context.nInputSize, nPadded);
            *pContext = context;
            return guardedThis && guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }
        if (nRecordType != BFF_RECORD_MEMBER) return false;
        if (nHeaderWords < BFF_MIN_HEADER_WORDS) return false;

        const qint64 nNameAreaSize = static_cast<qint64>(nHeaderWords) * 8;
        const qint64 nHeaderSize = nNameAreaSize + BFF_TRAILER_SIZE;
        if (!bffRangeWithin(context.nInputSize, nOffset, nHeaderSize)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, nNameAreaSize, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baHeader.size() != nNameAreaSize) {
            return false;
        }
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nHeaderSize;
        member.nDataOffset = nOffset + nHeaderSize;
        member.nMagic = nMagic;
        // st_mode is stored in a full 32-bit field and 52 directory records in
        // the corpus carry 0o240755, which loses its type nibble if the value
        // is first truncated to 16 bits.  Mask the whole word.
        member.nMode = qFromLittleEndian<quint32>(pHeader + 0x0c);
        member.nUncompressedSize = qFromLittleEndian<quint32>(pHeader + 0x18);
        member.nATime = qFromLittleEndian<quint32>(pHeader + 0x1c);
        member.nMTime = qFromLittleEndian<quint32>(pHeader + 0x20);
        member.nCTime = qFromLittleEndian<quint32>(pHeader + 0x24);
        member.nCompressedSize = qFromLittleEndian<quint32>(pHeader + 0x38);

        const quint32 nFormat = member.nMode & BFF_MODE_FORMAT_MASK;
        // Only directories and regular files occur in the "backup by name"
        // stream this class covers.  Anything else would need a payload rule
        // that has not been established on real data, so the archive is
        // refused rather than mis-walked.
        if ((nFormat != BFF_MODE_DIRECTORY) && (nFormat != BFF_MODE_REGULAR)) {
            return false;
        }
        member.bIsFolder = (nFormat == BFF_MODE_DIRECTORY);

        const QByteArray baName = bffExtractName(baHeader, nNameAreaSize);
        if (!bffIsValidName(baName)) return false;
        // The writer allocates the name area minimally, which is what keeps
        // the walk self-checking: a header whose word count does not match its
        // own path length is not a BFF record.
        const qint64 nExpectedNameArea =
            BFF_FIXED_HEADER_SIZE +
            bffAlignUp(static_cast<qint64>(baName.size()) + 1, 8);
        if (nExpectedNameArea != nNameAreaSize) return false;
        member.sFileName = QString::fromLatin1(baName);

        qint64 nPayloadSize = 0;
        if (nMagic == BFF_MAGIC_PACKED) {
            nPayloadSize = member.nCompressedSize;
        } else if (!member.bIsFolder) {
            // A stored member repeats its size in both fields; that agreement
            // is the anchor that keeps the rest of the walk honest.
            if (member.nCompressedSize != member.nUncompressedSize) {
                return false;
            }
            nPayloadSize = member.nUncompressedSize;
        } else {
            // 52 directory records in the corpus declare a compressed size of
            // 512 while carrying no payload at all.  Directory payload length
            // is decided by the mode, never by the size field.
            member.nCompressedSize = 0;
            member.nUncompressedSize = 0;
        }
        if (!bffRangeWithin(context.nInputSize, member.nDataOffset,
                            nPayloadSize)) {
            return false;
        }
        context.listMembers.append(member);

        const qint64 nNextOffset = bffAlignUp(
            nOffset + nHeaderSize + nPayloadSize, 8);
        if ((nNextOffset < 0) || (nNextOffset <= nOffset)) return false;
        nOffset = nNextOffset;
    }

    return false;
}

bool XAIXBFF::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XAIXBFF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAIXBFF archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAIXBFF::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAIXBFF(pDevice);
}

QList<QString> XAIXBFF::getSearchSignatures()
{
    return {QStringLiteral("09006BEA")};
}

XBinary::FT XAIXBFF::getFileType()
{
    return FT_AIX_BFF;
}

XBinary::MODE XAIXBFF::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAIXBFF::getEndian()
{
    // The container fields are little-endian even though the members are
    // big-endian AIX objects; the endianness reported here is the container's.
    return ENDIAN_LITTLE;
}

QString XAIXBFF::getArch()
{
    return QString();
}

QString XAIXBFF::getFileFormatExt()
{
    return QStringLiteral("bff");
}

QString XAIXBFF::getFileFormatExtsString()
{
    return QStringLiteral("IBM AIX backup (*.bff)");
}

QString XAIXBFF::getMIMEString()
{
    return QStringLiteral("application/x-bff");
}

QString XAIXBFF::getVersion()
{
    return QString();
}

qint64 XAIXBFF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAIXBFF::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAIXBFF::getMemoryMap(MAPMODE mapMode,
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

QString XAIXBFF::methodToString(quint16 nMagic)
{
    if (nMagic == BFF_MAGIC_STORED) return QStringLiteral("Stored");
    if (nMagic == BFF_MAGIC_PACKED) {
        return QStringLiteral("Pack (SysV Huffman)");
    }
    return QStringLiteral("Unknown");
}

XBinary::HANDLE_METHOD XAIXBFF::methodToHandleMethod(quint16 nMagic)
{
    if (nMagic == BFF_MAGIC_STORED) return HANDLE_METHOD_STORE;
    // Headerless classic SysV `pack` stream: no 0x1F1E magic and no embedded
    // raw size, so the dispatch synthesises the six-byte prefix from the
    // uncompressed size the record header declares.
    if (nMagic == BFF_MAGIC_PACKED) return HANDLE_METHOD_UNIX_PACK;
    return HANDLE_METHOD_UNKNOWN;
}

bool XAIXBFF::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XAIXBFF::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = BFF_VOLUME_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Volume header");
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
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && !member.bIsFolder &&
            canAppendPart(nLimit, result.size())) {
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
                                      methodToHandleMethod(member.nMagic));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member.nMagic));
            part.mapProperties.insert(FPART_PROP_TYPE,
                                      static_cast<quint32>(member.nMagic));
            part.mapProperties.insert(
                FPART_PROP_FILEMODE,
                static_cast<quint32>(member.nMode & 0xffffU));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nCompressedSize;
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
    // The archive proper ends at the tape-block boundary that follows the
    // sentinel; anything beyond that is stale writer buffer, not payload.
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

QMap<XBinary::UNPACK_PROP, QVariant> XAIXBFF::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAIXBFF::initUnpack(UNPACK_STATE *pState,
                         const QMap<UNPACK_PROP, QVariant> &mapProperties,
                         PDSTRUCT *pPdStruct)
{
    QPointer<XAIXBFF> guardedThis(this);
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
    QString sInfo = tr("IBM AIX backup (BFF), by name");
    if (!pContext->sVolumeLabel.isEmpty()) {
        sInfo += QStringLiteral("; ") + pContext->sVolumeLabel;
    }
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, sInfo);
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
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

XBinary::ARCHIVERECORD XAIXBFF::infoCurrent(UNPACK_STATE *pState,
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
    result.nStreamOffset = member.bIsFolder ? 0 : member.nDataOffset;
    result.nStreamSize = member.bIsFolder ? 0 : member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        member.bIsFolder ? HANDLE_METHOD_STORE
                         : methodToHandleMethod(member.nMagic));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member.nMagic));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nMagic));
    result.mapProperties.insert(FPART_PROP_FILEMODE,
                                static_cast<quint32>(member.nMode & 0xffffU));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);
    // AIX stores the permission bits of the original inode; no write bit
    // anywhere is the closest equivalent of a read-only attribute.
    result.mapProperties.insert(FPART_PROP_ISREADONLY,
                                (member.nMode & 0222U) == 0);
    const QDateTime dtModified =
        QDateTime::fromSecsSinceEpoch(member.nMTime, X_UTC_TZ);
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }
    const QDateTime dtAccessed =
        QDateTime::fromSecsSinceEpoch(member.nATime, X_UTC_TZ);
    if (dtAccessed.isValid()) {
        result.mapProperties.insert(FPART_PROP_ATIME, dtAccessed);
    }
    const QDateTime dtChanged =
        QDateTime::fromSecsSinceEpoch(member.nCTime, X_UTC_TZ);
    if (dtChanged.isValid()) {
        result.mapProperties.insert(FPART_PROP_CTIME, dtChanged);
    }
    return result;
}

bool XAIXBFF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XAIXBFF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
