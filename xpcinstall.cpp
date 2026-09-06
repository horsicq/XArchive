/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpcinstall.h"

#include <QPointer>
#include <QtEndian>

#include <new>

#include "Algos/xdcldecoder.h"

namespace {
const qint64 PCINSTALL_VOLUME_HEADER_SIZE = 16;
const qint64 PCINSTALL_TRAILER_SIZE = 16;
const qint64 PCINSTALL_RECORD_HEADER_SIZE = 0x114;
// A full member header is 0xe2 bytes; a continuation fragment carries only the
// common 0x3a-byte prologue and no name/size fields at all.
const qint64 PCINSTALL_MEMBER_PROLOGUE_SIZE = 0x3a;
const qint64 PCINSTALL_MEMBER_HEADER_SIZE = 0xe2;
const quint16 PCINSTALL_RECORD_FULL = 0x0074;
const quint16 PCINSTALL_RECORD_CONTINUATION = 0x0075;
const qint32 PCINSTALL_MAX_RECORDS = 100000;
const qint64 PCINSTALL_SOURCE_NAME_SIZE = 256;
const qint64 PCINSTALL_LONG_NAME_SIZE = 128;
const qint64 PCINSTALL_LINK_NAME_SIZE = 14;
// The family is a floppy disk set, so no member can plausibly be huge.  The
// caps only exist so a malformed header can never drive an allocation.
const qint64 PCINSTALL_MAX_STREAM_SIZE = Q_INT64_C(64) * 1024 * 1024;
const qint64 PCINSTALL_MAX_RAW_SIZE = Q_INT64_C(256) * 1024 * 1024;

const char PCINSTALL_TRAILER_MAGIC[8] = {'[', '2', '0', '/', '2', '0', ']',
                                         '\0'};

bool pcinstallRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// Every name field in this container is a fixed-width buffer whose tail is
// uninitialised builder heap (stale absolute paths, Win9x pointers).  Only the
// bytes before the first NUL may ever be surfaced.
QString pcinstallFixedName(const QByteArray &baField, bool *pbValid)
{
    if (pbValid) *pbValid = false;
    const qint32 nTerminator = baField.indexOf('\0');
    if (nTerminator <= 0) return QString();
    QString sResult;
    for (qint32 i = 0; i < nTerminator; ++i) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if (nCharacter < 0x20 || nCharacter > 0x7e) return QString();
        if (nCharacter == '/' || nCharacter == '\\') return QString();
        sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
    }
    if (pbValid) *pbValid = true;
    return sResult;
}
}  // namespace

XPCInstall::XPCInstall(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPCInstall::~XPCInstall()
{
}

bool XPCInstall::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPCInstall> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PCINSTALL_VOLUME_HEADER_SIZE +
                                 PCINSTALL_RECORD_HEADER_SIZE +
                                 PCINSTALL_MEMBER_PROLOGUE_SIZE +
                                 PCINSTALL_TRAILER_SIZE) {
        return false;
    }

    const qint64 nTrailerOffset =
        context.nInputSize - PCINSTALL_TRAILER_SIZE;
    const QByteArray baTrailer =
        read_array_process(nTrailerOffset, PCINSTALL_TRAILER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baTrailer.size() != PCINSTALL_TRAILER_SIZE ||
        baTrailer.left(8) !=
            QByteArray(PCINSTALL_TRAILER_MAGIC,
                       static_cast<qint32>(sizeof(PCINSTALL_TRAILER_MAGIC)))) {
        return false;
    }
    const uchar *pTrailer =
        reinterpret_cast<const uchar *>(baTrailer.constData());
    context.nFirstRecordOffset = qFromLittleEndian<quint32>(pTrailer + 8);
    context.nLastRecordOffset = qFromLittleEndian<quint32>(pTrailer + 12);
    if (context.nFirstRecordOffset != PCINSTALL_VOLUME_HEADER_SIZE ||
        context.nLastRecordOffset < PCINSTALL_VOLUME_HEADER_SIZE ||
        context.nLastRecordOffset >
            nTrailerOffset - PCINSTALL_RECORD_HEADER_SIZE) {
        return false;
    }

    const QByteArray baVolumeHeader =
        read_array_process(0, PCINSTALL_VOLUME_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baVolumeHeader.size() != PCINSTALL_VOLUME_HEADER_SIZE) {
        return false;
    }
    // Plain .BND volumes zero this block.  The self-extracting shape that
    // Detect It Easy matches in a PE overlay repeats the tag here instead, so
    // both are accepted rather than hard-coding "sixteen zero bytes".
    if ((baVolumeHeader !=
         QByteArray(static_cast<qint32>(PCINSTALL_VOLUME_HEADER_SIZE), '\0')) &&
        (baVolumeHeader.left(8) !=
         QByteArray(PCINSTALL_TRAILER_MAGIC,
                    static_cast<qint32>(sizeof(PCINSTALL_TRAILER_MAGIC))))) {
        return false;
    }

    qint64 nOffset = context.nFirstRecordOffset;
    while (context.listMembers.size() < PCINSTALL_MAX_RECORDS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (nOffset > nTrailerOffset - PCINSTALL_RECORD_HEADER_SIZE) {
            return false;
        }
        const QByteArray baRecord = read_array_process(
            nOffset, PCINSTALL_RECORD_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baRecord.size() != PCINSTALL_RECORD_HEADER_SIZE) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());
        const qint64 nNextOffset = qFromLittleEndian<quint32>(pRecord);
        const qint64 nStoredSize = qFromLittleEndian<quint32>(pRecord + 0x10);
        const qint64 nMemberOffset = nOffset + PCINSTALL_RECORD_HEADER_SIZE;
        if (!pcinstallRangeWithin(nTrailerOffset, nMemberOffset,
                                  nStoredSize) ||
            nStoredSize < PCINSTALL_MEMBER_PROLOGUE_SIZE) {
            return false;
        }
        bool bSourceValid = false;
        const QString sSourceName = pcinstallFixedName(
            baRecord.mid(0x14, static_cast<qint32>(PCINSTALL_SOURCE_NAME_SIZE)),
            &bSourceValid);
        if (!bSourceValid) return false;

        const QByteArray baPrologue = read_array_process(
            nMemberOffset, PCINSTALL_MEMBER_PROLOGUE_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baPrologue.size() != PCINSTALL_MEMBER_PROLOGUE_SIZE) {
            return false;
        }
        const uchar *pPrologue =
            reinterpret_cast<const uchar *>(baPrologue.constData());
        const quint16 nRecordType =
            qFromLittleEndian<quint16>(pPrologue + 0x12);
        if ((qFromLittleEndian<quint16>(pPrologue + 0x0e) != 0x0074) ||
            (qFromLittleEndian<quint16>(pPrologue + 0x10) != 0x0001) ||
            (qFromLittleEndian<quint16>(pPrologue + 0x14) != 0x0005) ||
            ((nRecordType != PCINSTALL_RECORD_FULL) &&
             (nRecordType != PCINSTALL_RECORD_CONTINUATION)) ||
            (baPrologue.mid(0x16) !=
             QByteArray(static_cast<qint32>(PCINSTALL_MEMBER_PROLOGUE_SIZE -
                                            0x16),
                        '\0'))) {
            return false;
        }

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nMemberHeaderOffset = nMemberOffset;
        member.nRecordType = nRecordType;
        member.nAttributes = qFromLittleEndian<quint32>(pRecord + 4);
        member.sSourceName = sSourceName;
        member.nDeclaredCompressedSize = -1;

        // The char[14] at member+0 names the fragment this record joins across
        // the volume boundary.  It is zero-filled on an ordinary member.
        if (pPrologue[0]) {
            bool bLinkValid = false;
            member.sVolumeLinkName = pcinstallFixedName(
                baPrologue.left(static_cast<qint32>(PCINSTALL_LINK_NAME_SIZE)),
                &bLinkValid);
            if (!bLinkValid) return false;
        }

        if (nRecordType == PCINSTALL_RECORD_FULL) {
            if (nStoredSize < PCINSTALL_MEMBER_HEADER_SIZE) return false;
            const QByteArray baHeader = read_array_process(
                nMemberOffset, PCINSTALL_MEMBER_HEADER_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource ||
                baHeader.size() != PCINSTALL_MEMBER_HEADER_SIZE) {
                return false;
            }
            const uchar *pHeader =
                reinterpret_cast<const uchar *>(baHeader.constData());
            bool bNameValid = false;
            member.sFileName = pcinstallFixedName(
                baHeader.mid(0x3a,
                             static_cast<qint32>(PCINSTALL_LONG_NAME_SIZE)),
                &bNameValid);
            if (!bNameValid) return false;
            member.nAttributes = qFromLittleEndian<quint32>(pHeader + 0xba);
            if (qFromLittleEndian<quint32>(pHeader + 0xbe) != 0) return false;
            member.nDeclaredCompressedSize =
                qFromLittleEndian<quint32>(pHeader + 0xc2);
            member.nDosDate = static_cast<quint16>(
                qFromLittleEndian<quint32>(pHeader + 0xc6) & 0xffffU);
            member.nDosTime = static_cast<quint16>(
                qFromLittleEndian<quint32>(pHeader + 0xca) & 0xffffU);
            member.nMemberHeaderSize = PCINSTALL_MEMBER_HEADER_SIZE;
            member.nDataOffset = nMemberOffset + PCINSTALL_MEMBER_HEADER_SIZE;
            member.nDataSize = nStoredSize - PCINSTALL_MEMBER_HEADER_SIZE;
            // The record may hold less than the whole stream: the last member
            // of a DISK1 continues on the next volume.  It may never hold more.
            if (member.nDataSize > member.nDeclaredCompressedSize) return false;
            member.bComplete =
                (member.nDataSize == member.nDeclaredCompressedSize);
            if (member.nDataSize >= 2) {
                // Raw PKWARE DCL header: literal mode 0/1, dictionary bits 4-6.
                const QByteArray baStreamHeader =
                    read_array_process(member.nDataOffset, 2, pPdStruct);
                if (!guardedThis || !guardedSource ||
                    baStreamHeader.size() != 2) {
                    return false;
                }
                const quint8 nLiteralMode =
                    static_cast<quint8>(baStreamHeader.at(0));
                const quint8 nDictionaryBits =
                    static_cast<quint8>(baStreamHeader.at(1));
                if (nLiteralMode > 1 || nDictionaryBits < 4 ||
                    nDictionaryBits > 6) {
                    return false;
                }
            }
        } else {
            // A continuation fragment has no name and no size fields; reading
            // +0x3a/+0xba/+0xc2 here lands inside compressed data and yields
            // plausible-looking garbage, so those offsets are never touched.
            if (member.sVolumeLinkName.isEmpty()) return false;
            member.sFileName = member.sVolumeLinkName;
            member.nMemberHeaderSize = PCINSTALL_MEMBER_PROLOGUE_SIZE;
            member.nDataOffset = nMemberOffset + PCINSTALL_MEMBER_PROLOGUE_SIZE;
            member.nDataSize = nStoredSize - PCINSTALL_MEMBER_PROLOGUE_SIZE;
            member.bComplete = false;
        }

        context.listMembers.append(member);

        if (nNextOffset == 0) {
            if ((nOffset != context.nLastRecordOffset) ||
                (nMemberOffset + nStoredSize != nTrailerOffset)) {
                return false;
            }
            context.nArchiveSize = context.nInputSize;
            *pContext = context;
            return guardedThis && guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }
        if ((nNextOffset <= nOffset) ||
            (nNextOffset != nMemberOffset + nStoredSize)) {
            return false;
        }
        nOffset = nNextOffset;
    }

    return false;
}

bool XPCInstall::isValid(PDSTRUCT *pPdStruct)
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

bool XPCInstall::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPCInstall archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPCInstall::createInstance(QIODevice *pDevice, bool bIsImage,
                                    XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPCInstall(pDevice);
}

QList<QString> XPCInstall::getSearchSignatures()
{
    // The tag lives at EOF-16, not at offset 0.  getSearchSignatures() drives a
    // whole-device search rather than a head compare, so a trailer magic is a
    // legitimate answer here.
    return {QStringLiteral("'[20/20]'00")};
}

XBinary::FT XPCInstall::getFileType()
{
    return FT_PCINSTALL;
}

XBinary::MODE XPCInstall::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPCInstall::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPCInstall::getArch()
{
    return QString();
}

QString XPCInstall::getFileFormatExt()
{
    return QStringLiteral("bnd");
}

QString XPCInstall::getFileFormatExtsString()
{
    return QStringLiteral("PC-Install disk set (*.bnd)");
}

QString XPCInstall::getMIMEString()
{
    return QStringLiteral("application/x-pcinstall");
}

QString XPCInstall::getVersion()
{
    return QString();
}

qint64 XPCInstall::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPCInstall::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPCInstall::getMemoryMap(MAPMODE mapMode,
                                              PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

qint64 XPCInstall::resolveUncompressedSize(const MEMBER &member,
                                           PDSTRUCT *pPdStruct)
{
    if ((member.nRecordType != PCINSTALL_RECORD_FULL) || !member.bComplete ||
        (member.nDataSize <= 0) ||
        (member.nDataSize > PCINSTALL_MAX_STREAM_SIZE)) {
        return -1;
    }
    const QHash<qint64, qint64>::const_iterator itCached =
        m_mapUncompressedSizes.constFind(member.nDataOffset);
    if (itCached != m_mapUncompressedSizes.constEnd()) return itCached.value();

    QPointer<XPCInstall> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return -1;
    const QByteArray baPacked =
        read_array_process(member.nDataOffset, member.nDataSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baPacked.size() != member.nDataSize)) {
        return -1;
    }
    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()),
            baPacked.size(), PCINSTALL_MAX_RAW_SIZE, &nConsumed, &nRawSize) ||
        !guardedThis || !guardedSource) {
        return -1;
    }
    // The container stores no raw size and no checksum, so exact input
    // consumption is the only cross-check available.  A stream that ends early
    // means the record was mis-parsed; publishing its size would make the
    // dispatcher emit bytes nobody validated.
    if ((nConsumed != member.nDataSize) || (nRawSize < 0)) return -1;
    m_mapUncompressedSizes.insert(member.nDataOffset, nRawSize);
    return nRawSize;
}

QString XPCInstall::methodToString(const MEMBER &member)
{
    if (member.nRecordType == PCINSTALL_RECORD_CONTINUATION) {
        return QStringLiteral(
            "PKWARE DCL implode (continuation fragment, incomplete)");
    }
    if (!member.bComplete) {
        return QStringLiteral(
            "PKWARE DCL implode (split across volumes, incomplete)");
    }
    return QStringLiteral("PKWARE DCL implode");
}

void XPCInstall::fillMemberProperties(
    const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties,
    PDSTRUCT *pPdStruct)
{
    if (!pMapProperties) return;

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    // The record's 8.3 field is the disk staging name (DEMINDEX.000), not the
    // installed name; keep it visible but never use it as the output name.
    if (!member.sSourceName.isEmpty()) {
        pMapProperties->insert(FPART_PROP_PREFIX, member.sSourceName);
    }
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, methodToString(member));
    pMapProperties->insert(FPART_PROP_TYPE,
                           static_cast<quint32>(member.nRecordType));
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
    pMapProperties->insert(FPART_PROP_ISREADONLY,
                           (member.nAttributes & 0x00000001U) != 0);
    pMapProperties->insert(FPART_PROP_ISHIDDEN,
                           (member.nAttributes & 0x00000002U) != 0);
    pMapProperties->insert(FPART_PROP_ISSYSTEM,
                           (member.nAttributes & 0x00000004U) != 0);
    pMapProperties->insert(FPART_PROP_ISARCHIVE,
                           (member.nAttributes & 0x00000020U) != 0);
    const QDateTime dtModified =
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtModified.isValid()) {
        pMapProperties->insert(FPART_PROP_MTIME, dtModified);
        pMapProperties->insert(FPART_PROP_DATETIME, dtModified);
    }

    const qint64 nUncompressedSize = resolveUncompressedSize(member, pPdStruct);
    if (nUncompressedSize >= 0) {
        pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD,
                               HANDLE_METHOD_PKWARE_DCL_IMPLODE);
        return;
    }

    // Nothing below here is extractable from this volume alone.  The DCL
    // dispatcher hard-requires an exact output size, so a member whose size
    // could not be recovered must be listed and refused, never emitted short.
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
    if (member.nRecordType == PCINSTALL_RECORD_CONTINUATION) {
        pMapProperties->insert(
            FPART_PROP_INFO,
            tr("Continuation of %1 from the previous volume; %2 bytes present")
                .arg(member.sVolumeLinkName)
                .arg(member.nDataSize));
    } else if (!member.bComplete) {
        pMapProperties->insert(
            FPART_PROP_INFO,
            tr("Continues as %1 on the next volume; %2 of %3 stream bytes "
               "present")
                .arg(member.sVolumeLinkName)
                .arg(member.nDataSize)
                .arg(member.nDeclaredCompressedSize));
    } else {
        pMapProperties->insert(FPART_PROP_INFO,
                               tr("Unrecognized compressed stream"));
    }
}

bool XPCInstall::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XPCInstall::getFileParts(quint32 nFileParts,
                                               qint32 nLimit,
                                               PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PCINSTALL_VOLUME_HEADER_SIZE;
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
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize =
                PCINSTALL_RECORD_HEADER_SIZE + member.nMemberHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
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
            fillMemberProperties(member, &part.mapProperties, pPdStruct);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = PCINSTALL_RECORD_HEADER_SIZE +
                             member.nMemberHeaderSize + member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nArchiveSize - PCINSTALL_TRAILER_SIZE;
        part.nFileSize = PCINSTALL_TRAILER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Trailer");
        result.append(part);
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
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XPCInstall::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPCInstall::initUnpack(UNPACK_STATE *pState,
                            const QMap<UNPACK_PROP, QVariant> &mapProperties,
                            PDSTRUCT *pPdStruct)
{
    QPointer<XPCInstall> guardedThis(this);
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
        tr("PC-Install disk set; PKWARE DCL implode members"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
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

XBinary::ARCHIVERECORD XPCInstall::infoCurrent(UNPACK_STATE *pState,
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
    fillMemberProperties(member, &result.mapProperties, pPdStruct);
    return result;
}

bool XPCInstall::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XPCInstall::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
