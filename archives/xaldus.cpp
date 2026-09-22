/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xaldus.h"

#include <QtEndian>

#include <new>
#include <QTimeZone>

namespace {
const qint64 ALDUS_MAGIC_SIZE = 16;
const qint64 ALDUS_NAME_OFFSET = 0x12;
const qint64 ALDUS_NAME_SIZE = 32;
const qint64 ALDUS_UNCOMPRESSEDSIZE_OFFSET = 0x32;
const qint64 ALDUS_TIMESTAMP_OFFSET = 0x36;
const qint64 ALDUS_FILETIME_OFFSET = 0x3a;
const qint64 ALDUS_MIN_HEADER_SIZE = 0x40;
const qint64 ALDUS_SUBHEADER_SIZE = 22;
const qint64 ALDUS_TRAILER_SIZE = 18;
const qint64 ALDUS_HEADER_SIZE_V1 = 100;
const qint64 ALDUS_HEADER_SIZE_V2 = 200;
const qint64 ALDUS_BLOCK_SIZE_V1 = 20000;
const qint64 ALDUS_BLOCK_SIZE_V2 = 16384;
const qint64 ALDUS_MAX_BLOCK_COUNT = 0x100000;
// Seconds between 1900-01-01 and 1970-01-01.  Gen1 counts from 1970 and gen2
// from 1900; the two ranges do not overlap for any plausible file date, so the
// magnitude selects the epoch and a mislabelled generation cannot shift a
// timestamp by seventy years.
const quint32 ALDUS_EPOCH_1900_DELTA = 2208988800U;
const quint32 ALDUS_EPOCH_SPLIT = 0x80000000U;
// Seconds between 1601-01-01 (Win32 FILETIME epoch) and 1970-01-01.
const qint64 ALDUS_FILETIME_DELTA = Q_INT64_C(11644473600);
const qint64 ALDUS_FILETIME_PER_SECOND = Q_INT64_C(10000000);

bool aldusRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// The stored member name is a plain DOS name.  The container file on disk
// carries a truncated extension ("FOO.EX_"); the real name lives here and its
// case is meaningful, so it is never folded.
bool aldusIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
        if (c == '\\' || c == '/' || c == ':') return false;
    }
    return true;
}

// The stored stamps are local wall-clock values.  Rebuilding the same wall
// clock in the local spec reproduces what the original installer wrote,
// instead of shifting it by the reader's time zone.
QDateTime aldusWallClock(const QDateTime &dtUtc)
{
    if (!dtUtc.isValid()) return QDateTime();
    return QDateTime(dtUtc.date(), dtUtc.time());
}
}  // namespace

XAldus::XAldus(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAldus::~XAldus()
{
}

XAldus::GENERATION XAldus::magicToGeneration(const QByteArray &baMagic)
{
    // The signature is byte-exact and space-padded.  A loose "ALDUS "/"ADOBE "
    // prefix test would accept unrelated files and, worse, would not tell the
    // three incompatible codecs apart.
    if (baMagic == QByteArrayLiteral("ALDUS LZW   1.00")) return GENERATION_LZW;
    if (baMagic == QByteArrayLiteral("ALDUS PKZP  2.00")) return GENERATION_PKZP;
    if (baMagic == QByteArrayLiteral("ADOBE LZSH  3.00")) return GENERATION_LZSH;
    return GENERATION_UNKNOWN;
}

QString XAldus::generationToString(GENERATION generation)
{
    if (generation == GENERATION_LZW) {
        return QStringLiteral("Aldus LZW 1.00 (blocked TIFF-style LZW)");
    }
    if (generation == GENERATION_PKZP) {
        return QStringLiteral("Aldus PKZP 2.00 (blocked PKWARE DCL implode)");
    }
    if (generation == GENERATION_LZSH) {
        return QStringLiteral("Adobe LZSH 3.00 (blocked LHA lh5)");
    }
    return QStringLiteral("Unknown");
}

XBinary::HANDLE_METHOD XAldus::generationToHandleMethod(GENERATION generation)
{
    // Each generation needs its own method: the payload is a table of
    // independently coded blocks, so no whole-stream handle method fits, and
    // the codec is named by the file magic rather than by the payload.
    if (generation == GENERATION_LZW) return HANDLE_METHOD_ALDUS_LZW;
    if (generation == GENERATION_PKZP) return HANDLE_METHOD_ALDUS_PKZP;
    if (generation == GENERATION_LZSH) return HANDLE_METHOD_ALDUS_LZSH;
    return HANDLE_METHOD_UNKNOWN;
}

bool XAldus::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ALDUS_MIN_HEADER_SIZE + ALDUS_SUBHEADER_SIZE) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, ALDUS_MIN_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != ALDUS_MIN_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());

    context.generation = magicToGeneration(baHeader.left(ALDUS_MAGIC_SIZE));
    if (context.generation == GENERATION_UNKNOWN) return false;

    // Offset 0x10 is the header SIZE, not the version, despite matching the
    // "1.00"/"2.00" text; it is also the absolute offset of the payload.
    context.nHeaderSize = qFromBigEndian<quint16>(pHeader + ALDUS_MAGIC_SIZE);
    const qint64 nExpectedHeaderSize = (context.generation == GENERATION_LZW)
                                           ? ALDUS_HEADER_SIZE_V1
                                           : ALDUS_HEADER_SIZE_V2;
    if (context.nHeaderSize != nExpectedHeaderSize) return false;
    if (!aldusRangeWithin(context.nInputSize, context.nHeaderSize,
                          ALDUS_SUBHEADER_SIZE)) {
        return false;
    }

    const QByteArray baNameField =
        baHeader.mid(qint32(ALDUS_NAME_OFFSET), qint32(ALDUS_NAME_SIZE));
    const qint32 nNameEnd = baNameField.indexOf('\0');
    if (nNameEnd <= 0) return false;
    const QByteArray baName = baNameField.left(nNameEnd);
    if (!aldusIsValidName(baName) ||
        (baNameField.mid(nNameEnd) !=
         QByteArray(qint32(ALDUS_NAME_SIZE) - nNameEnd, '\0'))) {
        return false;
    }
    context.sFileName = QString::fromLatin1(baName);
    context.nUncompressedSize =
        qFromBigEndian<quint32>(pHeader + ALDUS_UNCOMPRESSEDSIZE_OFFSET);

    const QByteArray baSubHeader = read_array_process(
        context.nHeaderSize, ALDUS_SUBHEADER_SIZE, pPdStruct);
    if (baSubHeader.size() != ALDUS_SUBHEADER_SIZE) {
        return false;
    }
    const uchar *pSubHeader =
        reinterpret_cast<const uchar *>(baSubHeader.constData());
    const qint64 nSubHeaderSize = qFromBigEndian<quint16>(pSubHeader);
    context.nBlockSize = qFromBigEndian<quint16>(pSubHeader + 2);
    context.nLastBlockSize = qFromBigEndian<quint16>(pSubHeader + 4);
    context.nBlockCount = qFromBigEndian<quint32>(pSubHeader + 6);
    const qint64 nTableOffset = qFromBigEndian<quint32>(pSubHeader + 10);
    context.nDataOffset = qFromBigEndian<quint32>(pSubHeader + 14);
    const qint64 nDeclaredFileSize = qFromBigEndian<quint32>(pSubHeader + 18);

    const qint64 nExpectedBlockSize = (context.generation == GENERATION_LZW)
                                          ? ALDUS_BLOCK_SIZE_V1
                                          : ALDUS_BLOCK_SIZE_V2;
    if ((nSubHeaderSize != ALDUS_SUBHEADER_SIZE) ||
        (context.nBlockSize != nExpectedBlockSize) ||
        (context.nBlockCount < 1) ||
        (context.nBlockCount > ALDUS_MAX_BLOCK_COUNT) ||
        (context.nLastBlockSize < 1) ||
        (context.nLastBlockSize > context.nBlockSize)) {
        return false;
    }
    if (((context.nBlockCount - 1) * context.nBlockSize +
         context.nLastBlockSize) != context.nUncompressedSize) {
        return false;
    }

    // The two offsets in the sub-header are ABSOLUTE file offsets and are
    // redundant with the sizes around them.  Cross-checking them, together
    // with the declared total size, is what makes a false positive require a
    // fully self-consistent forgery rather than just the right 16 bytes.
    const qint64 nTableSize = 2 * context.nBlockCount;
    if ((nTableOffset != context.nHeaderSize + ALDUS_SUBHEADER_SIZE) ||
        (context.nDataOffset != nTableOffset + nTableSize) ||
        (nDeclaredFileSize != context.nInputSize) ||
        !aldusRangeWithin(context.nInputSize, nTableOffset, nTableSize)) {
        return false;
    }

    const QByteArray baTable =
        read_array_process(nTableOffset, nTableSize, pPdStruct);
    if ((baTable.size() != nTableSize)) {
        return false;
    }
    const uchar *pTable = reinterpret_cast<const uchar *>(baTable.constData());
    qint64 nTotalBlockSize = 0;
    for (qint64 i = 0; i < context.nBlockCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nBlockSize = qFromBigEndian<quint16>(pTable + 2 * i);
        // Blocks are word-padded, so a zero or odd length is not an Aldus
        // block table.
        if ((nBlockSize < 2) || (nBlockSize & 1)) return false;
        nTotalBlockSize += nBlockSize;
    }
    // The block data is followed by an all-zero 18-byte trailer that the
    // declared total size includes.  A check that expects the data to end at
    // EOF rejects every file in the family.
    if (!aldusRangeWithin(context.nInputSize, context.nDataOffset,
                          nTotalBlockSize) ||
        ((context.nDataOffset + nTotalBlockSize + ALDUS_TRAILER_SIZE) !=
         context.nInputSize)) {
        return false;
    }

    context.nStreamOffset = context.nHeaderSize;
    context.nStreamSize =
        context.nDataOffset + nTotalBlockSize - context.nHeaderSize;
    context.nArchiveSize = context.nInputSize;

    if (context.generation == GENERATION_LZSH) {
        // Gen3 replaced the seconds field with Win32 file attributes and put
        // three little-endian FILETIMEs after it; the first is the member's
        // own timestamp.
        const quint64 nFileTime =
            qFromLittleEndian<quint64>(pHeader + ALDUS_FILETIME_OFFSET);
        const qint64 nSeconds =
            qint64(nFileTime / ALDUS_FILETIME_PER_SECOND) -
            ALDUS_FILETIME_DELTA;
        const qint32 nMilliseconds = qint32(
            (nFileTime % ALDUS_FILETIME_PER_SECOND) / 10000);
        if (nFileTime > 0) {
            context.dtModified = aldusWallClock(
                QDateTime::fromMSecsSinceEpoch(
                    nSeconds * 1000 + nMilliseconds, X_UTC_TZ));
        }
    } else {
        const quint32 nStamp =
            qFromLittleEndian<quint32>(pHeader + ALDUS_TIMESTAMP_OFFSET);
        if (nStamp > 0) {
            const qint64 nSeconds =
                (nStamp >= ALDUS_EPOCH_SPLIT)
                    ? (qint64(nStamp) - qint64(ALDUS_EPOCH_1900_DELTA))
                    : qint64(nStamp);
            context.dtModified = aldusWallClock(
                QDateTime::fromSecsSinceEpoch(nSeconds, X_UTC_TZ));
        }
    }

    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XAldus::isValid(PDSTRUCT *pPdStruct)
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

bool XAldus::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAldus archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAldus::createInstance(QIODevice *pDevice, bool bIsImage,
                                XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAldus(pDevice);
}

QList<QString> XAldus::getSearchSignatures()
{
    return {QStringLiteral("'ALDUS LZW   1.00'"),
            QStringLiteral("'ALDUS PKZP  2.00'"),
            QStringLiteral("'ADOBE LZSH  3.00'")};
}

XBinary::FT XAldus::getFileType()
{
    return FT_ALDUS;
}

XBinary::MODE XAldus::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAldus::getEndian()
{
    return ENDIAN_BIG;
}

QString XAldus::getArch()
{
    return QString();
}

QString XAldus::getFileFormatExt()
{
    return QStringLiteral("aldus");
}

QString XAldus::getFileFormatExtsString()
{
    return QStringLiteral("Aldus/Adobe Setup container (*.??_ *.??~)");
}

QString XAldus::getMIMEString()
{
    return QStringLiteral("application/x-aldus-setup");
}

QString XAldus::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    if (context.generation == GENERATION_LZW) return QStringLiteral("1.00");
    if (context.generation == GENERATION_PKZP) return QStringLiteral("2.00");
    return QStringLiteral("3.00");
}

qint64 XAldus::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAldus::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAldus::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_FOOTER, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XAldus::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XAldus::getFileParts(quint32 nFileParts, qint32 nLimit,
                                           PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        // The stream deliberately starts at the payload sub-header: the block
        // table is part of what the decoder has to read.
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nStreamSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  generationToHandleMethod(context.generation));
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  generationToString(context.generation));
        part.mapProperties.insert(FPART_PROP_TYPE,
                                  static_cast<quint32>(context.generation));
        result.append(part);
    }
    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        result.append(part);
    }
    if ((nFileParts & FILEPART_FOOTER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_FOOTER;
        part.nFileOffset = context.nStreamOffset + context.nStreamSize;
        part.nFileSize = ALDUS_TRAILER_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XAldus::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAldus::initUnpack(UNPACK_STATE *pState,
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
    if (!parseContext(pContext, pPdStruct)) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Aldus/Adobe Setup container; %1")
            .arg(generationToString(pContext->generation)));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    // A container of this family always holds exactly one member.
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XAldus::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentOffset != pContext->nStreamOffset)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                generationToHandleMethod(pContext->generation));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                generationToString(pContext->generation));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(pContext->generation));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (pContext->dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, pContext->dtModified);
    }
    return result;
}

bool XAldus::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) return true;
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XAldus::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
