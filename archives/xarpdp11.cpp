/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarpdp11.h"

#include <QtEndian>

#include <new>
#include <QTimeZone>

namespace {
// V7 <ar.h> ARMAG 0177545, stored as the little-endian word 65 FF.  This is the
// whole global header: the first member record starts at offset 2, not at 8 as
// in the modern "!<arch>\n" ar.
const quint16 AR_PDP11_MAGIC = 0xff65U;
const qint64 AR_PDP11_MAGIC_SIZE = 2;
// struct ar_hdr = char ar_name[14]; long ar_date; char ar_uid; char ar_gid;
// int ar_mode; long ar_size;  -- packed on a 2-byte-aligned PDP-11, so there is
// no alignment hole and no terminator characters.
const qint64 AR_PDP11_HEADER_SIZE = 26;
const qint32 AR_PDP11_NAME_SIZE = 14;
const qint32 AR_PDP11_OFFSET_DATE = 14;
const qint32 AR_PDP11_OFFSET_UID = 18;
const qint32 AR_PDP11_OFFSET_GID = 19;
const qint32 AR_PDP11_OFFSET_MODE = 20;
const qint32 AR_PDP11_OFFSET_SIZE = 22;
const quint16 AR_PDP11_MODE_IFMT = 0xf000U;
const quint16 AR_PDP11_MODE_IFREG = 0x8000U;
// The largest archive in the reference family holds 242 members; the cap only
// bounds the worst case where a hostile 26-byte-per-record file would otherwise
// make the member list grow to the size of the input.
const qint32 AR_PDP11_MAX_MEMBERS = 100000;
const qint64 AR_PDP11_ZERO_SCAN_CHUNK = 0x10000;

// PDP-11 `long`: the high 16-bit word is stored first and each word is itself
// little-endian.  Reading ar_size as plain LE or BE yields plausible values for
// small members and then desynchronises the record chain a few members in, so
// qFromLittleEndian<quint32>() must not be used on these two fields.
quint32 arPdp11ReadMiddleEndian32(const uchar *pData)
{
    return (static_cast<quint32>(pData[1]) << 24) |
           (static_cast<quint32>(pData[0]) << 16) |
           (static_cast<quint32>(pData[3]) << 8) |
           static_cast<quint32>(pData[2]);
}

bool arPdp11RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// Returns the name length, or -1 when the 14-byte field is not a clean
// NUL-padded printable name.  A genuine V7 header never leaves garbage after
// the terminator, which makes this the cheapest structural discriminator after
// the 2-byte magic.
qint32 arPdp11NameLength(const uchar *pName)
{
    qint32 nLength = AR_PDP11_NAME_SIZE;
    for (qint32 i = 0; i < AR_PDP11_NAME_SIZE; i++) {
        if (pName[i] == 0) {
            nLength = i;
            break;
        }
    }
    if (nLength < 1) return -1;
    for (qint32 i = 0; i < nLength; i++) {
        if ((pName[i] < 0x20) || (pName[i] > 0x7e)) return -1;
        // ar member names are basenames by construction; a separator means the
        // bytes are not a V7 header, and accepting one would hand a traversal
        // path straight to the file-write layer.
        if ((pName[i] == '/') || (pName[i] == '\\')) return -1;
    }
    for (qint32 i = nLength; i < AR_PDP11_NAME_SIZE; i++) {
        if (pName[i] != 0) return -1;
    }
    return nLength;
}
}  // namespace

XArPdp11::XArPdp11(QIODevice *pDevice) : XArchive(pDevice)
{
}

XArPdp11::~XArPdp11()
{
}

bool XArPdp11::isZeroTail(qint64 nOffset, qint64 nSize, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (nOffset < 0 || nSize < 0) return false;

    qint64 nCurrent = nOffset;
    const qint64 nEnd = nOffset + nSize;
    while (nCurrent < nEnd) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nPortion = qMin(AR_PDP11_ZERO_SCAN_CHUNK, nEnd - nCurrent);
        const QByteArray baChunk =
            read_array_process(nCurrent, nPortion, pPdStruct);
        if (baChunk.size() != nPortion) {
            return false;
        }
        for (qint64 i = 0; i < nPortion; i++) {
            if (baChunk.at(static_cast<qint32>(i)) != '\0') return false;
        }
        nCurrent += nPortion;
    }
    return true;
}

bool XArPdp11::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < AR_PDP11_MAGIC_SIZE + AR_PDP11_HEADER_SIZE) {
        return false;
    }

    const QByteArray baMagic =
        read_array_process(0, AR_PDP11_MAGIC_SIZE, pPdStruct);
    if (baMagic.size() != AR_PDP11_MAGIC_SIZE) {
        return false;
    }
    if (qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar *>(baMagic.constData())) !=
        AR_PDP11_MAGIC) {
        return false;
    }

    qint64 nOffset = AR_PDP11_MAGIC_SIZE;
    bool bZeroTerminated = false;
    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        // A well-formed archive lands exactly on the end of the input.
        if (nOffset == context.nInputSize) break;
        if (context.listMembers.size() >= AR_PDP11_MAX_MEMBERS) return false;
        if (!arPdp11RangeWithin(context.nInputSize, nOffset,
                                AR_PDP11_HEADER_SIZE)) {
            return false;
        }

        const QByteArray baHeader =
            read_array_process(nOffset, AR_PDP11_HEADER_SIZE, pPdStruct);
        if (baHeader.size() != AR_PDP11_HEADER_SIZE) {
            return false;
        }
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());

        if (pHeader[0] == 0) {
            // Truncated/zero-padded recovery images exist in the wild (one of
            // the 61 reference files carries 20692 zero bytes after its last
            // real member).  A zero name is end-of-archive, never another
            // record: walking the all-zero headers is what makes other tools
            // emit hundreds of phantom entries named "_" on that input.
            if (context.listMembers.isEmpty() ||
                !isZeroTail(nOffset, context.nInputSize - nOffset,
                            pPdStruct)) {
                return false;
            }
            context.nArchiveSize = nOffset;
            bZeroTerminated = true;
            break;
        }

        const qint32 nNameLength = arPdp11NameLength(pHeader);
        if (nNameLength < 0) return false;

        const quint16 nMode = qFromLittleEndian<quint16>(
            pHeader + AR_PDP11_OFFSET_MODE);
        // Every record of the reference family is S_IFREG.  Demand it on the
        // first record, where it is the only discriminator behind a two-byte
        // magic; afterwards the size chain landing exactly on EOF carries the
        // proof, so a zero mode (some ar writers left it unset) is tolerated.
        if (context.listMembers.isEmpty()) {
            if ((nMode & AR_PDP11_MODE_IFMT) != AR_PDP11_MODE_IFREG) {
                return false;
            }
        } else if (((nMode & AR_PDP11_MODE_IFMT) != AR_PDP11_MODE_IFREG) &&
                   (nMode != 0)) {
            return false;
        }

        const qint64 nSize = static_cast<qint64>(
            arPdp11ReadMiddleEndian32(pHeader + AR_PDP11_OFFSET_SIZE));
        const qint64 nDataOffset = nOffset + AR_PDP11_HEADER_SIZE;
        if (!arPdp11RangeWithin(context.nInputSize, nDataOffset, nSize)) {
            return false;
        }

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nSize = nSize;
        member.nMTime =
            arPdp11ReadMiddleEndian32(pHeader + AR_PDP11_OFFSET_DATE);
        member.nUid = pHeader[AR_PDP11_OFFSET_UID];
        member.nGid = pHeader[AR_PDP11_OFFSET_GID];
        member.nMode = nMode;
        // Reported verbatim, including names such as "con.h" that are reserved
        // devices on Windows; remapping belongs to the file-write layer, not to
        // the listing.
        member.sFileName = QString::fromLatin1(
            reinterpret_cast<const char *>(pHeader), nNameLength);
        context.listMembers.append(member);

        // Member data is padded to an even boundary.  316 members of the
        // reference family are odd-sized, and the pad byte is physically
        // present even behind the last member of the file, so it is part of the
        // normal stride rather than a tail special case.
        const qint64 nStride = AR_PDP11_HEADER_SIZE + nSize + (nSize & 1);
        if (nOffset > context.nInputSize - nStride) {
            // Tolerate the one shape a shorter file could legitimately take:
            // an odd final member whose pad byte was never written.
            if ((nSize & 1) && (nDataOffset + nSize == context.nInputSize)) {
                nOffset = context.nInputSize;
                continue;
            }
            return false;
        }
        nOffset += nStride;
    }

    if (context.listMembers.isEmpty()) return false;
    if (!bZeroTerminated) context.nArchiveSize = context.nInputSize;
    context.nFirstMemberOffset = context.listMembers.first().nHeaderOffset;

    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XArPdp11::isValid(PDSTRUCT *pPdStruct)
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

bool XArPdp11::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XArPdp11 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XArPdp11::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XArPdp11(pDevice);
}

QList<QString> XArPdp11::getSearchSignatures()
{
    // Deliberately empty: the whole signature is the two bytes 65 FF, which as
    // a scan pattern would flood SFX/embedded searches with candidates.  This
    // family is only ever a standalone file.
    return QList<QString>();
}

XBinary::FT XArPdp11::getFileType()
{
    return FT_AR_PDP11;
}

XBinary::MODE XArPdp11::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XArPdp11::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XArPdp11::getArch()
{
    return QString();
}

QString XArPdp11::getFileFormatExt()
{
    return QStringLiteral("a");
}

QString XArPdp11::getFileFormatExtsString()
{
    return QStringLiteral("UNIX V7 PDP-11 ar (*.a)");
}

QString XArPdp11::getMIMEString()
{
    return QStringLiteral("application/x-archive");
}

QString XArPdp11::getVersion()
{
    return QStringLiteral("V7");
}

qint64 XArPdp11::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XArPdp11::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XArPdp11::getMemoryMap(MAPMODE mapMode,
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

bool XArPdp11::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XArPdp11::getFileParts(quint32 nFileParts, qint32 nLimit,
                                             PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = AR_PDP11_MAGIC_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Archive magic");
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
            part.nFileSize = AR_PDP11_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            // The even-boundary pad byte belongs to the container; including it
            // here would append a stray byte to every odd-sized member.
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            part.mapProperties.insert(FPART_PROP_FILEMODE,
                                      static_cast<quint32>(member.nMode));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = AR_PDP11_HEADER_SIZE + member.nSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XArPdp11::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XArPdp11::initUnpack(UNPACK_STATE *pState,
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
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO, tr("UNIX V7 / PDP-11 ar; all members stored"));
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

XBinary::ARCHIVERECORD XArPdp11::infoCurrent(UNPACK_STATE *pState,
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
    // Exactly ar_size: never the padded stride (see getFileParts).
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_FILEMODE,
                                static_cast<quint32>(member.nMode));
    result.mapProperties.insert(FPART_PROP_UID,
                                static_cast<quint32>(member.nUid));
    result.mapProperties.insert(FPART_PROP_GID,
                                static_cast<quint32>(member.nGid));
    // Unix permission bits sit in the low 9 bits of the V7 mode word.
    result.mapProperties.insert(FPART_PROP_ISREADONLY,
                                (member.nMode & 0222U) == 0);
    const QDateTime dtModified =
        QDateTime::fromSecsSinceEpoch(member.nMTime, X_UTC_TZ);
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        result.mapProperties.insert(FPART_PROP_DATETIME, dtModified);
    }
    return result;
}

bool XArPdp11::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XArPdp11::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
