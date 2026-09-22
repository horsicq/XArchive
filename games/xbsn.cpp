/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbsn.h"

#include <QtEndian>

#include <new>

#include "xbsndecoder.h"

namespace {
// 0xFF "BSG" opens the archive, 0xFF "BSA" opens every member.  BSA is the
// archiver's own name; the container extension is .BSN.
const quint32 BSN_ARCHIVE_MAGIC = 0xff425347U;
const quint32 BSN_MEMBER_MAGIC = 0xff425341U;
// Only 0x0000 and 0x0001 exist in the reference corpus (0x0001 marks the
// solid variant).  A low ceiling keeps a random 0xFF"BSG" prefix carrying a
// wild version word from reaching the more expensive checks below.
const quint16 BSN_MAX_VERSION = 0x000fU;
const qint64 BSN_ARCHIVE_HEADER_SIZE = 6;
const qint64 BSN_MEMBER_PREFIX_SIZE = 6;
const qint64 BSN_TRAILER_SIZE = 2;
// Header body = 4 attrs + 4 timestamp + name + NUL + 4 + 4 + 4.
const quint16 BSN_HEADER_BODY_OVERHEAD = 21;
const quint16 BSN_NAME_OFFSET = 8;
const quint16 BSN_MAX_NAME_SIZE = 260;
const qint32 BSN_MAX_MEMBERS = 100000;
const quint32 BSN_ATTR_DIRECTORY = 0x02000000U;
const quint32 BSN_ATTR_STORED = 0x08000000U;
const quint32 BSN_ATTR_READONLY = 0x00000001U;
// Replaying the solid window must not be turned into an allocation primitive
// by a hostile header.  Nothing in the format needs a single member larger
// than this; a member that claims more simply stops the replay.
const qint64 BSN_MAX_REPLAY_MEMBER_SIZE = Q_INT64_C(256) * 1024 * 1024;

bool bsnRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// Names already use '/' as the separator, so no '\\' rewrite belongs here.
// Only control bytes are rejected: the high half is legitimate cp866 text.
bool bsnIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        if (static_cast<quint8>(c) < 0x20) return false;
    }
    return true;
}
}  // namespace

XBSN::XBSN(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBSN::~XBSN()
{
}

bool XBSN::isFolderMember(const MEMBER &member)
{
    return (member.nAttributes & BSN_ATTR_DIRECTORY) != 0;
}

bool XBSN::isStoredMember(const MEMBER &member)
{
    return (member.nAttributes & BSN_ATTR_STORED) != 0;
}

bool XBSN::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nDictionaryIndex = 0;
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BSN_ARCHIVE_HEADER_SIZE + BSN_TRAILER_SIZE) {
        return false;
    }

    const QByteArray baArchiveHeader =
        read_array_process(0, BSN_ARCHIVE_HEADER_SIZE, pPdStruct);
    if (!guardedSource ||
        baArchiveHeader.size() != BSN_ARCHIVE_HEADER_SIZE) {
        return false;
    }
    const uchar *pArchiveHeader =
        reinterpret_cast<const uchar *>(baArchiveHeader.constData());
    if (qFromBigEndian<quint32>(pArchiveHeader) != BSN_ARCHIVE_MAGIC) {
        return false;
    }
    context.nVersion = qFromBigEndian<quint16>(pArchiveHeader + 4);
    if (context.nVersion > BSN_MAX_VERSION) return false;

    qint64 nOffset = BSN_ARCHIVE_HEADER_SIZE;
    while (isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRemaining = context.nInputSize - nOffset;
        if (nRemaining < BSN_TRAILER_SIZE) return false;

        const QByteArray baPrefix = read_array_process(
            nOffset, qMin<qint64>(BSN_MEMBER_PREFIX_SIZE, nRemaining),
            pPdStruct);
        if (!guardedSource ||
            baPrefix.size() < BSN_TRAILER_SIZE) {
            return false;
        }
        const uchar *pPrefix =
            reinterpret_cast<const uchar *>(baPrefix.constData());

        // The archive ends on a 2-byte 00 00 trailer.  A member can never be
        // confused with it: every member starts with the 0xFF magic byte.
        if ((pPrefix[0] == 0) && (pPrefix[1] == 0)) {
            if (context.listMembers.isEmpty()) return false;
            context.nArchiveSize = nOffset + BSN_TRAILER_SIZE;
            context.nFirstMemberOffset =
                context.listMembers.first().nHeaderOffset;
            *pContext = context;
            return guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }

        if ((baPrefix.size() != BSN_MEMBER_PREFIX_SIZE) ||
            (qFromBigEndian<quint32>(pPrefix) != BSN_MEMBER_MAGIC) ||
            (context.listMembers.size() >= BSN_MAX_MEMBERS)) {
            return false;
        }

        const quint16 nHeaderBodySize = qFromBigEndian<quint16>(pPrefix + 4);
        if ((nHeaderBodySize < BSN_HEADER_BODY_OVERHEAD) ||
            (nHeaderBodySize >
             BSN_HEADER_BODY_OVERHEAD + BSN_MAX_NAME_SIZE)) {
            return false;
        }
        const quint16 nNameSize =
            static_cast<quint16>(nHeaderBodySize - BSN_HEADER_BODY_OVERHEAD);
        const qint64 nHeaderSize = BSN_MEMBER_PREFIX_SIZE +
                                   static_cast<qint64>(nHeaderBodySize) +
                                   4;  // trailing u32be header CRC
        if (!bsnRangeWithin(context.nInputSize, nOffset, nHeaderSize)) {
            return false;
        }

        const QByteArray baHeader =
            read_array_process(nOffset, nHeaderSize, pPdStruct);
        if (!guardedSource ||
            baHeader.size() != nHeaderSize) {
            return false;
        }
        const uchar *pBody = reinterpret_cast<const uchar *>(
            baHeader.constData() + BSN_MEMBER_PREFIX_SIZE);

        // The declared body length and the ASCIIZ name must agree exactly:
        // hdrlen == 21 + strlen(name) holds for every member of the corpus,
        // so a name that stops early (or not at all) is a mis-parse.
        const QByteArray baName(
            reinterpret_cast<const char *>(pBody + BSN_NAME_OFFSET),
            nNameSize);
        if ((pBody[BSN_NAME_OFFSET + nNameSize] != 0) ||
            baName.contains('\0') || !bsnIsValidName(baName)) {
            return false;
        }

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nHeaderSize;
        member.nDataOffset = nOffset + nHeaderSize;
        member.nAttributes = qFromBigEndian<quint32>(pBody);
        member.nDosDateTime = qFromBigEndian<quint32>(pBody + 4);
        member.nUncompressedSize =
            qFromBigEndian<quint32>(pBody + BSN_NAME_OFFSET + nNameSize + 1);
        member.nCompressedSize =
            qFromBigEndian<quint32>(pBody + BSN_NAME_OFFSET + nNameSize + 5);
        member.nCRC32 =
            qFromBigEndian<quint32>(pBody + BSN_NAME_OFFSET + nNameSize + 9);
        // Names are nominally cp866 and all-ASCII across the corpus.  Latin-1
        // is what the other DOS-era classes in this tree use: it round-trips a
        // high-half byte instead of collapsing it to U+FFFD.
        member.sFileName = QString::fromLatin1(baName);

        // CRC32 of the header body, stored as the u32be right after it.  This
        // is the decisive gate: it holds for 881/881 headers in the corpus, so
        // a file that reaches this point by accident is a ~2^-32 event.
        const quint32 nHeaderCRC = qFromBigEndian<quint32>(
            reinterpret_cast<const uchar *>(baHeader.constData()) +
            BSN_MEMBER_PREFIX_SIZE + nHeaderBodySize);
        const quint32 nCalculatedCRC = _getCRC32(
            baHeader.mid(static_cast<qint32>(BSN_MEMBER_PREFIX_SIZE),
                         nHeaderBodySize),
            0xffffffffU, _getCRC32Table_EDB88320());
        if ((nCalculatedCRC ^ 0xffffffffU) != nHeaderCRC) return false;

        if (!bsnRangeWithin(context.nInputSize, member.nDataOffset,
                            member.nCompressedSize)) {
            return false;
        }
        // A directory carries no payload, and a stored member's two size
        // fields must agree - that pair is what keeps the attribute bits
        // honest when a header is otherwise plausible.
        if (isFolderMember(member) &&
            ((member.nCompressedSize != 0) ||
             (member.nUncompressedSize != 0))) {
            return false;
        }
        if (!isFolderMember(member) && isStoredMember(member) &&
            (member.nCompressedSize != member.nUncompressedSize)) {
            return false;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    return false;
}

bool XBSN::ensureDictionary(CONTEXT *pContext, qint32 nIndex,
                            PDSTRUCT *pPdStruct)
{
    if (!pContext || (nIndex < 0) ||
        (nIndex > pContext->listMembers.size()) ||
        (pContext->nDictionaryIndex < 0)) {
        return false;
    }

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    // Records are consumed in index order by every extraction path, so the
    // replay is normally a single forward step.  A backwards request (a
    // second listing pass over the same session) restarts it rather than
    // reusing a window that already contains later members' bytes.
    if (pContext->nDictionaryIndex > nIndex) {
        pContext->nDictionaryIndex = 0;
        pContext->baDictionary.clear();
    }

    const qint32 nWindowSize = XBSNDecoder::windowSize();
    while (pContext->nDictionaryIndex < nIndex) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const MEMBER &member =
            pContext->listMembers.at(pContext->nDictionaryIndex);

        QByteArray baPlain;
        if (!isFolderMember(member) && (member.nUncompressedSize > 0)) {
            if ((member.nUncompressedSize > BSN_MAX_REPLAY_MEMBER_SIZE) ||
                (member.nCompressedSize > BSN_MAX_REPLAY_MEMBER_SIZE)) {
                pContext->nDictionaryIndex = -1;
                return false;
            }
            if (isStoredMember(member)) {
                // Only the tail can ever be referenced, so a 400 KiB stored
                // JPEG costs a 32 KiB read here instead of a full one.
                const qint64 nTailSize =
                    qMin<qint64>(member.nCompressedSize, nWindowSize);
                baPlain = read_array_process(
                    member.nDataOffset + member.nCompressedSize - nTailSize,
                    nTailSize, pPdStruct);
                if (!guardedSource ||
                    (baPlain.size() != nTailSize)) {
                    pContext->nDictionaryIndex = -1;
                    return false;
                }
            } else {
                const QByteArray baPacked = read_array_process(
                    member.nDataOffset, member.nCompressedSize, pPdStruct);
                if (!guardedSource ||
                    (baPacked.size() != member.nCompressedSize) ||
                    !XBSNDecoder::decode(baPacked, member.nUncompressedSize,
                                         pContext->baDictionary, &baPlain,
                                         pPdStruct)) {
                    pContext->nDictionaryIndex = -1;
                    return false;
                }
            }
        }

        if (!baPlain.isEmpty()) {
            pContext->baDictionary.append(baPlain);
            if (pContext->baDictionary.size() > nWindowSize) {
                pContext->baDictionary =
                    pContext->baDictionary.right(nWindowSize);
            }
        }
        ++pContext->nDictionaryIndex;
    }

    return guardedSource;
}

bool XBSN::isValid(PDSTRUCT *pPdStruct)
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

bool XBSN::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBSN archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBSN::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBSN(pDevice);
}

QList<QString> XBSN::getSearchSignatures()
{
    return {QStringLiteral("FF425347")};
}

XBinary::FT XBSN::getFileType()
{
    return FT_BSN;
}

XBinary::MODE XBSN::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBSN::getEndian()
{
    return ENDIAN_BIG;
}

QString XBSN::getArch()
{
    return QString();
}

QString XBSN::getFileFormatExt()
{
    return QStringLiteral("bsn");
}

QString XBSN::getFileFormatExtsString()
{
    return QStringLiteral("PTS BSA archive (*.bsn)");
}

QString XBSN::getMIMEString()
{
    return QStringLiteral("application/x-bsn");
}

QString XBSN::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XBSN::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBSN::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBSN::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XBSN::methodToString(const MEMBER &member)
{
    if (isFolderMember(member)) return QStringLiteral("Directory");
    if (isStoredMember(member)) return QStringLiteral("Stored");
    return QStringLiteral("BSA solid -lh6-");
}

XBinary::HANDLE_METHOD XBSN::methodToHandleMethod(const MEMBER &member)
{
    if (isFolderMember(member) || isStoredMember(member)) {
        return HANDLE_METHOD_STORE;
    }
    return HANDLE_METHOD_BSN_LH6;
}

bool XBSN::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XBSN::getFileParts(quint32 nFileParts, qint32 nLimit,
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
        part.nFileSize = BSN_ARCHIVE_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Archive header");
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
        if ((nFileParts & FILEPART_STREAM) &&
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
                                      methodToHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member));
            part.mapProperties.insert(FPART_PROP_TYPE, member.nAttributes);
            part.mapProperties.insert(FPART_PROP_ISFOLDER,
                                      isFolderMember(member));
            part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                      CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            // No FPART_PROP_COMPRESSPROPERTIES here on purpose.  Publishing
            // the solid history for every part would replay the whole archive
            // for what is a map/inspection query and pin 32 KiB per member;
            // the record path (infoCurrent) is where extraction gets it, and
            // the stored CRC rejects any part decoded without it.
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

QMap<XBinary::UNPACK_PROP, QVariant> XBSN::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBSN::initUnpack(UNPACK_STATE *pState,
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
    if (!parseContext(pContext, pPdStruct) ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        (pContext->nVersion != 0)
            ? tr("PTS BSA archive (solid); member header CRCs verified")
            : tr("PTS BSA archive; member header CRCs verified"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

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

XBinary::ARCHIVERECORD XBSN::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext ||
        pState->nCurrentIndex >= pContext->listMembers.size()) {
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
                                methodToHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member));
    result.mapProperties.insert(FPART_PROP_TYPE, member.nAttributes);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, isFolderMember(member));
    result.mapProperties.insert(
        FPART_PROP_ISREADONLY,
        (member.nAttributes & BSN_ATTR_READONLY) != 0);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    // Big-endian u32 holding a DOS packed date in the high half and a DOS
    // packed time in the low half - not a unix timestamp.
    const QDateTime dtModified = dosDateTimeToQDateTime(
        static_cast<quint16>(member.nDosDateTime >> 16),
        static_cast<quint16>(member.nDosDateTime & 0xffffU));
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }

    // The compressed member needs the plaintext that precedes it in the
    // archive; without it roughly 58% of a solid archive decodes to garbage.
    // Hand the decoder that history explicitly.  When the replay cannot be
    // completed the property is simply absent and the stored CRC32 turns the
    // attempt into a reported failure rather than a wrong file on disk.
    if (!isFolderMember(member) && !isStoredMember(member) &&
        (member.nUncompressedSize > 0) &&
        ensureDictionary(pContext, pState->nCurrentIndex, pPdStruct) &&
        !pContext->baDictionary.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES,
                                    pContext->baDictionary);
    }
    return result;
}

bool XBSN::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XBSN::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
