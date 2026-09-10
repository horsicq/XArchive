/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpcinstallsfx.h"

#include <QPointer>
#include <QtEndian>

#include <new>

#include "Algos/xdcldecoder.h"

namespace {
const qint64 PCINSTALLSFX_HEAD_SIZE = 8;
const qint64 PCINSTALLSFX_TRAILER_SIZE = 16;
const qint64 PCINSTALLSFX_RECORD_SIZE = 0x114;
const qint64 PCINSTALLSFX_RECORD_NAME_OFFSET = 0x14;
const qint64 PCINSTALLSFX_RECORD_NAME_SIZE = 256;
const qint64 PCINSTALLSFX_PROLOGUE_SIZE = 0x3a;
const qint64 PCINSTALLSFX_INFO_SIZE = 0xa8;
const qint64 PCINSTALLSFX_INFO_NAME_SIZE = 128;
const quint16 PCINSTALLSFX_GROUP_TAG = 0x0074;
const quint16 PCINSTALLSFX_RECORD_FULL = 0x0074;
const quint16 PCINSTALLSFX_RECORD_CONTINUATION = 0x0075;
const qint32 PCINSTALLSFX_MAX_RECORDS = 100000;
const qint32 PCINSTALLSFX_MAX_GROUP_MEMBERS = 4096;
const qint32 PCINSTALLSFX_MAX_CANDIDATES = 64;
// The family is a floppy/CD setup builder, so no single stream can plausibly
// be huge.  The caps only exist so a malformed header can never drive an
// allocation.
const qint64 PCINSTALLSFX_MAX_STREAM_SIZE = Q_INT64_C(64) * 1024 * 1024;
const qint64 PCINSTALLSFX_MAX_RAW_SIZE = Q_INT64_C(256) * 1024 * 1024;

const char PCINSTALLSFX_TAG[8] = {'[', '2', '0', '/', '2', '0', ']', '\0'};

// Every name field in this container is a fixed-width buffer whose tail is
// uninitialised builder heap (stale names of previously packed files).  Only
// the bytes before the first NUL may ever be surfaced, and a separator would
// let a member escape the output directory.
QString pcinstallSfxFixedName(const QByteArray &baField, bool *pbValid)
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

XPCInstallSFX::XPCInstallSFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPCInstallSFX::~XPCInstallSFX()
{
}

bool XPCInstallSFX::isHeadTagAt(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    if (nOffset < 0) return false;
    QPointer<XPCInstallSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const QByteArray baTag =
        read_array_process(nOffset, PCINSTALLSFX_HEAD_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baTag.size() != PCINSTALLSFX_HEAD_SIZE)) {
        return false;
    }
    return baTag == QByteArray(PCINSTALLSFX_TAG,
                               static_cast<qint32>(sizeof(PCINSTALLSFX_TAG)));
}

bool XPCInstallSFX::parseGroup(qint64 nGroupOffset, qint64 nGroupSize,
                               qint64 nRecordOffset,
                               const QString &sSourceName,
                               QList<ENTRY> *pListEntries, PDSTRUCT *pPdStruct)
{
    if (!pListEntries) return false;
    if (nGroupSize < PCINSTALLSFX_PROLOGUE_SIZE + PCINSTALLSFX_INFO_SIZE) {
        return false;
    }

    QPointer<XPCInstallSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baPrologue = read_array_process(
        nGroupOffset, PCINSTALLSFX_PROLOGUE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baPrologue.size() != PCINSTALLSFX_PROLOGUE_SIZE)) {
        return false;
    }
    // The first fourteen bytes are the cross-volume link name, which is always
    // zero-filled inside a self-extractor: there is no second volume to join.
    if (baPrologue.left(0x0e) != QByteArray(0x0e, '\0')) return false;

    const uchar *pPrologue =
        reinterpret_cast<const uchar *>(baPrologue.constData());
    if (qFromLittleEndian<quint16>(pPrologue + 0x0e) !=
        PCINSTALLSFX_GROUP_TAG) {
        return false;
    }
    const quint16 nRecordType = qFromLittleEndian<quint16>(pPrologue + 0x12);
    if ((nRecordType != PCINSTALLSFX_RECORD_FULL) &&
        (nRecordType != PCINSTALLSFX_RECORD_CONTINUATION)) {
        return false;
    }
    const qint32 nMemberCount =
        static_cast<qint32>(qFromLittleEndian<quint16>(pPrologue + 0x10));
    if ((nMemberCount < 1) || (nMemberCount > PCINSTALLSFX_MAX_GROUP_MEMBERS)) {
        return false;
    }

    QList<ENTRY> listGroup;
    qint64 nPosition = PCINSTALLSFX_PROLOGUE_SIZE;
    for (qint32 i = 0; i < nMemberCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nPosition > nGroupSize - PCINSTALLSFX_INFO_SIZE) return false;
        const QByteArray baInfo = read_array_process(
            nGroupOffset + nPosition, PCINSTALLSFX_INFO_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baInfo.size() != PCINSTALLSFX_INFO_SIZE)) {
            return false;
        }
        const uchar *pInfo =
            reinterpret_cast<const uchar *>(baInfo.constData());
        bool bNameValid = false;
        const QString sFileName = pcinstallSfxFixedName(
            baInfo.left(static_cast<qint32>(PCINSTALLSFX_INFO_NAME_SIZE)),
            &bNameValid);
        if (!bNameValid) return false;
        if (qFromLittleEndian<quint32>(pInfo + 0x84) != 0) return false;
        const qint64 nCompressedSize =
            qFromLittleEndian<quint32>(pInfo + 0x88);
        if ((nCompressedSize < 2) ||
            (nCompressedSize >
             nGroupSize - nPosition - PCINSTALLSFX_INFO_SIZE)) {
            return false;
        }

        ENTRY entry = {};
        entry.nRecordOffset = nRecordOffset;
        entry.nGroupOffset = nGroupOffset;
        entry.nGroupSize = nGroupSize;
        entry.nInfoOffset = nGroupOffset + nPosition;
        entry.nDataOffset =
            nGroupOffset + nPosition + PCINSTALLSFX_INFO_SIZE;
        entry.nDataSize = nCompressedSize;
        entry.nDeclaredRawSize = qFromLittleEndian<quint32>(pInfo + 0x9c);
        entry.nAttributes = qFromLittleEndian<quint32>(pInfo + 0x80);
        entry.nDosDate = static_cast<quint16>(
            qFromLittleEndian<quint32>(pInfo + 0x8c) & 0xffffU);
        entry.nDosTime = static_cast<quint16>(
            qFromLittleEndian<quint32>(pInfo + 0x90) & 0xffffU);
        entry.bStored = false;
        entry.sFileName = sFileName;
        entry.sSourceName = sSourceName;

        // Raw PKWARE DCL prelude: literal mode 0/1, dictionary bits 4-6.  It
        // is the only per-member check that separates a real group from a
        // staging blob whose first bytes happen to look like a prologue.
        const QByteArray baPrelude =
            read_array_process(entry.nDataOffset, 2, pPdStruct);
        if (!guardedThis || !guardedSource || (baPrelude.size() != 2)) {
            return false;
        }
        const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
        const quint8 nDictionaryBits = static_cast<quint8>(baPrelude.at(1));
        if ((nLiteralMode > 1) || (nDictionaryBits < 4) ||
            (nDictionaryBits > 6)) {
            return false;
        }

        listGroup.append(entry);
        nPosition = nPosition + PCINSTALLSFX_INFO_SIZE + nCompressedSize;
    }
    // The declared member count must account for the record's payload exactly.
    // A staging blob that survived every field check still fails here.
    if (nPosition != nGroupSize) return false;

    for (qint32 i = 0; i < listGroup.size(); ++i) {
        pListEntries->append(listGroup.at(i));
    }
    return true;
}

bool XPCInstallSFX::walkRecords(CONTEXT *pContext, qint64 nStartOffset,
                                PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    QPointer<XPCInstallSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint64 nTrailerOffset = pContext->nTrailerOffset;
    if ((nStartOffset < PCINSTALLSFX_HEAD_SIZE) ||
        (nStartOffset > nTrailerOffset - PCINSTALLSFX_RECORD_SIZE)) {
        return false;
    }
    if (!isHeadTagAt(nStartOffset - PCINSTALLSFX_HEAD_SIZE, pPdStruct)) {
        return false;
    }

    QList<ENTRY> listEntries;
    qint64 nOffset = nStartOffset;
    qint32 nRecordCount = 0;
    while (isPdStructNotCanceled(pPdStruct)) {
        if ((nOffset < PCINSTALLSFX_HEAD_SIZE) ||
            (nOffset > nTrailerOffset - PCINSTALLSFX_RECORD_SIZE)) {
            return false;
        }
        const QByteArray baRecord = read_array_process(
            nOffset, PCINSTALLSFX_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baRecord.size() != PCINSTALLSFX_RECORD_SIZE)) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());
        const qint64 nNextOffset = qFromLittleEndian<quint32>(pRecord);
        const qint64 nStoredSize = qFromLittleEndian<quint32>(pRecord + 0x10);
        const qint64 nPayloadOffset = nOffset + PCINSTALLSFX_RECORD_SIZE;
        if (nStoredSize > nTrailerOffset - nPayloadOffset) return false;

        bool bSourceValid = false;
        const QString sSourceName = pcinstallSfxFixedName(
            baRecord.mid(static_cast<qint32>(PCINSTALLSFX_RECORD_NAME_OFFSET),
                         static_cast<qint32>(PCINSTALLSFX_RECORD_NAME_SIZE)),
            &bSourceValid);
        if (!bSourceValid) return false;

        if (!parseGroup(nPayloadOffset, nStoredSize, nOffset, sSourceName,
                        &listEntries, pPdStruct)) {
            if (!guardedThis || !guardedSource) return false;
            // Not a member group: the setup engine consumes this record's
            // payload verbatim (SETUP.CFG, a .PIF shortcut, a spawn helper).
            if (nStoredSize > 0) {
                ENTRY entry = {};
                entry.nRecordOffset = nOffset;
                entry.nGroupOffset = nPayloadOffset;
                entry.nGroupSize = nStoredSize;
                entry.nInfoOffset = -1;
                entry.nDataOffset = nPayloadOffset;
                entry.nDataSize = nStoredSize;
                entry.nDeclaredRawSize = nStoredSize;
                entry.nAttributes = qFromLittleEndian<quint32>(pRecord + 4) &
                                    0x0000ffffU;
                entry.nDosTime = qFromLittleEndian<quint16>(pRecord + 8);
                entry.nDosDate = qFromLittleEndian<quint16>(pRecord + 0x0c);
                entry.bStored = true;
                entry.sFileName = sSourceName;
                entry.sSourceName = sSourceName;
                listEntries.append(entry);
            }
        }

        ++nRecordCount;
        if (nRecordCount > PCINSTALLSFX_MAX_RECORDS) return false;

        if (nNextOffset == 0) {
            // The chain must tile the whole payload region: the last record
            // ends exactly where the trailer begins.
            if (nPayloadOffset + nStoredSize != nTrailerOffset) return false;
            if (listEntries.isEmpty()) return false;
            pContext->nHeadOffset = nStartOffset - PCINSTALLSFX_HEAD_SIZE;
            pContext->nFirstRecordOffset = nStartOffset;
            pContext->listEntries = listEntries;
            return true;
        }
        if (nNextOffset != nPayloadOffset + nStoredSize) return false;
        nOffset = nNextOffset;
    }

    return false;
}

bool XPCInstallSFX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPCInstallSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PCINSTALLSFX_HEAD_SIZE +
                                 PCINSTALLSFX_RECORD_SIZE +
                                 PCINSTALLSFX_TRAILER_SIZE) {
        return false;
    }
    context.nArchiveSize = context.nInputSize;
    context.nTrailerOffset = context.nInputSize - PCINSTALLSFX_TRAILER_SIZE;

    const QByteArray baTrailer = read_array_process(
        context.nTrailerOffset, PCINSTALLSFX_TRAILER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baTrailer.size() != PCINSTALLSFX_TRAILER_SIZE) ||
        (baTrailer.left(static_cast<qint32>(PCINSTALLSFX_HEAD_SIZE)) !=
         QByteArray(PCINSTALLSFX_TAG,
                    static_cast<qint32>(sizeof(PCINSTALLSFX_TAG))))) {
        return false;
    }
    const uchar *pTrailer =
        reinterpret_cast<const uchar *>(baTrailer.constData());
    const qint64 nStoredFirst = qFromLittleEndian<quint32>(pTrailer + 8);

    // The trailer's own answer first: it is right on every stub whose payload
    // was appended after the final link.
    if (walkRecords(&context, nStoredFirst, pPdStruct)) {
        *pContext = context;
        return guardedThis && guardedSource &&
               isPdStructNotCanceled(pPdStruct);
    }
    if (!guardedThis || !guardedSource) return false;

    // Otherwise the stored offsets are stale (the stub was rebuilt at a
    // different size and only the tag moved with the payload).  The head tag
    // is the real anchor, so look for it.  Bounded: only a file that already
    // carries the trailer tag ever reaches this point.
    qint64 nSearch = 0;
    qint32 nCandidates = 0;
    while (isPdStructNotCanceled(pPdStruct) &&
           (nCandidates < PCINSTALLSFX_MAX_CANDIDATES) &&
           (nSearch < context.nTrailerOffset)) {
        const qint64 nFound = find_array(
            nSearch, context.nTrailerOffset - nSearch, PCINSTALLSFX_TAG,
            PCINSTALLSFX_HEAD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (nFound < 0)) break;
        ++nCandidates;
        const qint64 nStart = nFound + PCINSTALLSFX_HEAD_SIZE;
        if ((nStart != nStoredFirst) &&
            walkRecords(&context, nStart, pPdStruct)) {
            *pContext = context;
            return guardedThis && guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }
        if (!guardedThis || !guardedSource) return false;
        nSearch = nFound + 1;
    }

    return false;
}

bool XPCInstallSFX::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XPCInstallSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPCInstallSFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPCInstallSFX::createInstance(QIODevice *pDevice, bool bIsImage,
                                       XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPCInstallSFX(pDevice);
}

XBinary::FT XPCInstallSFX::getFileType()
{
    return FT_PCINSTALL_SFX;
}

XBinary::MODE XPCInstallSFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPCInstallSFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPCInstallSFX::getArch()
{
    return QString();
}

QString XPCInstallSFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XPCInstallSFX::getFileFormatExtsString()
{
    return QStringLiteral("PC-Install self-extracting installer (*.exe)");
}

QString XPCInstallSFX::getMIMEString()
{
    return QStringLiteral("application/x-pcinstall-sfx");
}

QString XPCInstallSFX::getVersion()
{
    return QString();
}

qint64 XPCInstallSFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPCInstallSFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPCInstallSFX::getMemoryMap(MAPMODE mapMode,
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

qint64 XPCInstallSFX::resolveUncompressedSize(const ENTRY &entry,
                                              PDSTRUCT *pPdStruct)
{
    if (entry.bStored) return entry.nDataSize;
    if ((entry.nDataSize <= 0) ||
        (entry.nDataSize > PCINSTALLSFX_MAX_STREAM_SIZE)) {
        return -1;
    }
    const QHash<qint64, qint64>::const_iterator itCached =
        m_mapUncompressedSizes.constFind(entry.nDataOffset);
    if (itCached != m_mapUncompressedSizes.constEnd()) return itCached.value();

    QPointer<XPCInstallSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return -1;
    const QByteArray baPacked =
        read_array_process(entry.nDataOffset, entry.nDataSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baPacked.size() != entry.nDataSize)) {
        return -1;
    }
    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()),
            baPacked.size(), PCINSTALLSFX_MAX_RAW_SIZE, &nConsumed,
            &nRawSize) ||
        !guardedThis || !guardedSource) {
        return -1;
    }
    // The info block's raw-size field is blank on roughly half of the members,
    // so exact input consumption is the only cross-check that always exists.
    // A stream that ends early means the record was mis-parsed; publishing its
    // size would make the dispatcher emit bytes nobody validated.
    if ((nConsumed != entry.nDataSize) || (nRawSize < 0)) return -1;
    if ((entry.nDeclaredRawSize > 0) &&
        (entry.nDeclaredRawSize != nRawSize)) {
        return -1;
    }
    m_mapUncompressedSizes.insert(entry.nDataOffset, nRawSize);
    return nRawSize;
}

void XPCInstallSFX::fillEntryProperties(
    const ENTRY &entry, QMap<FPART_PROP, QVariant> *pMapProperties,
    PDSTRUCT *pPdStruct)
{
    if (!pMapProperties) return;

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    // The outer record names the file as it is staged on the install media
    // (INST32.SHR, TEMP.SHR, KI.000); the installed name lives in the member
    // info block.  Keep the staging name visible but never use it as the
    // output name.
    if (!entry.sSourceName.isEmpty() &&
        (entry.sSourceName != entry.sFileName)) {
        pMapProperties->insert(FPART_PROP_PREFIX, entry.sSourceName);
    }
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, entry.nDataSize);
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);
    pMapProperties->insert(FPART_PROP_ISREADONLY,
                           (entry.nAttributes & 0x00000001U) != 0);
    pMapProperties->insert(FPART_PROP_ISHIDDEN,
                           (entry.nAttributes & 0x00000002U) != 0);
    pMapProperties->insert(FPART_PROP_ISSYSTEM,
                           (entry.nAttributes & 0x00000004U) != 0);
    pMapProperties->insert(FPART_PROP_ISARCHIVE,
                           (entry.nAttributes & 0x00000020U) != 0);
    const QDateTime dtModified =
        dosDateTimeToQDateTime(entry.nDosDate, entry.nDosTime);
    if (dtModified.isValid()) {
        pMapProperties->insert(FPART_PROP_MTIME, dtModified);
        pMapProperties->insert(FPART_PROP_DATETIME, dtModified);
    }

    if (entry.bStored) {
        pMapProperties->insert(FPART_PROP_REPORTEDMETHOD,
                               QStringLiteral("Store"));
        pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nDataSize);
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        return;
    }

    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD,
                           QStringLiteral("PKWARE DCL implode"));
    const qint64 nUncompressedSize = resolveUncompressedSize(entry, pPdStruct);
    if (nUncompressedSize >= 0) {
        pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        pMapProperties->insert(FPART_PROP_HANDLEMETHOD,
                               HANDLE_METHOD_PKWARE_DCL_IMPLODE);
        return;
    }

    // The DCL dispatcher hard-requires an exact output size, so a member whose
    // size could not be recovered must be listed and refused, never emitted
    // short.
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
    pMapProperties->insert(FPART_PROP_INFO,
                           tr("Unrecognized compressed stream"));
}

bool XPCInstallSFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPCInstallSFX::getFileParts(quint32 nFileParts,
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
        part.nFileSize = context.nHeadOffset + PCINSTALLSFX_HEAD_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Stub");
        result.append(part);
    }

    for (qint32 i = 0; i < context.listEntries.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        const ENTRY &entry = context.listEntries.at(i);
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = (entry.nInfoOffset >= 0) ? entry.nInfoOffset
                                                        : entry.nRecordOffset;
            part.nFileSize = (entry.nInfoOffset >= 0)
                                 ? PCINSTALLSFX_INFO_SIZE
                                 : PCINSTALLSFX_RECORD_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = entry.nDataOffset;
            part.nFileSize = entry.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sFileName;
            fillEntryProperties(entry, &part.mapProperties, pPdStruct);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = (entry.nInfoOffset >= 0) ? entry.nInfoOffset
                                                        : entry.nRecordOffset;
            part.nFileSize =
                entry.nDataOffset + entry.nDataSize - part.nFileOffset;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nTrailerOffset;
        part.nFileSize = PCINSTALLSFX_TRAILER_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XPCInstallSFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPCInstallSFX::initUnpack(UNPACK_STATE *pState,
                               const QMap<UNPACK_PROP, QVariant> &mapProperties,
                               PDSTRUCT *pPdStruct)
{
    QPointer<XPCInstallSFX> guardedThis(this);
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
        pContext->listEntries.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("PC-Install self-extracting installer; PKWARE DCL implode members"));
    pState->nCurrentOffset = pContext->listEntries.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
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

XBinary::ARCHIVERECORD XPCInstallSFX::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return ARCHIVERECORD();
    }
    const ENTRY &entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != entry.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nDataSize;
    fillEntryProperties(entry, &result.mapProperties, pPdStruct);
    return result;
}

bool XPCInstallSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XPCInstallSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
