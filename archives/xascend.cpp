/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xascend.h"

#include <QFileInfo>
#include <QtEndian>

#include <new>

#include "Algos/xdcldecoder.h"

namespace {
// Six little-endian words of date/time, then the PKWARE DCL stream.  Bytes
// 0x0C/0x0D are NOT container fields: they are the DCL stream's own header
// (literal mode, dictionary bits).  Starting the decoder at 0x0E instead of
// 0x0C throws that header away and every stream fails to decode.
const qint64 ASCEND_HEADER_SIZE = 12;
// blast rejects anything shorter than its own two header bytes plus one bit
// of stream, so a container below this size cannot hold a member at all.
const qint64 ASCEND_MIN_STREAM_SIZE = 3;
// The format has no magic, so validation ends in a full trial decode.  Cap the
// container at a size that still covers the floppy-era originals (the largest
// member in the reference corpus is 329235 bytes packed) but keeps a probe of
// an unrelated multi-gigabyte file from reading it all into memory.
const qint64 ASCEND_MAX_ARCHIVE_SIZE = Q_INT64_C(64) * 1024 * 1024;
const qint64 ASCEND_MAX_UNPACKED_SIZE = Q_INT64_C(512) * 1024 * 1024;
const quint16 ASCEND_MIN_YEAR = 1980;
const quint16 ASCEND_MAX_YEAR = 2100;
const quint8 ASCEND_MAX_LITERAL_MODE = 1;
const quint8 ASCEND_MIN_DICTIONARY_BITS = 4;
const quint8 ASCEND_MAX_DICTIONARY_BITS = 6;

bool ascendRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// The installer truncates the final character of the original extension and
// writes '!' in its place, exactly as Microsoft's COMPRESS.EXE writes '_'
// (PLANNER.EXE -> PLANNER.EX!).  The dropped character is genuinely absent
// from the container, so this map is a convention-based reconstruction, not
// recovered data; anything outside the map is left verbatim rather than
// guessed at.
QString ascendRestoreSuffix(const QString &sName)
{
    if (!sName.endsWith(QLatin1Char('!')) || sName.size() < 4) return sName;

    struct SUFFIXMAP {
        const char *pszMangled;
        const char *pszRestored;
    };
    static const SUFFIXMAP records[] = {
        {".IN!", ".INI"}, {".EX!", ".EXE"}, {".DL!", ".DLL"},
        {".HL!", ".HLP"}, {".TX!", ".TXT"}, {".DO!", ".DOC"},
        {".SM!", ".SMM"}};

    const QString sTail = sName.right(4);
    for (const SUFFIXMAP &record : records) {
        const QString sMangled = QString::fromLatin1(record.pszMangled);
        if (QString::compare(sTail, sMangled, Qt::CaseInsensitive) != 0) {
            continue;
        }
        QString sRestored = QString::fromLatin1(record.pszRestored);
        // Preserve the case the archive actually used: only the final
        // character is synthesised.
        if (sTail.at(1).isLower()) sRestored = sRestored.toLower();
        return sName.left(sName.size() - 4) + sRestored;
    }
    return sName;
}
}  // namespace

XAscend::XAscend(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAscend::~XAscend()
{
}

QString XAscend::memberName(QIODevice *pDevice)
{
    // getDeviceFileBaseName() cuts at the FIRST dot, which would drop the
    // mangled extension the name reconstruction depends on, so take the full
    // file name instead.
    const QString sDeviceName = XBinary::getDeviceFileName(pDevice);
    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return QStringLiteral("ascend_data");
    return ascendRestoreSuffix(sFileName);
}

bool XAscend::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <
            (ASCEND_HEADER_SIZE + ASCEND_MIN_STREAM_SIZE) ||
        context.nInputSize > ASCEND_MAX_ARCHIVE_SIZE) {
        return false;
    }

    // Read the date header together with the two DCL header bytes so the
    // cheap structural gate runs before the payload is pulled into memory.
    const qint64 nPrefixSize = ASCEND_HEADER_SIZE + 2;
    const QByteArray baPrefix = read_array_process(0, nPrefixSize, pPdStruct);
    if (baPrefix.size() != nPrefixSize) {
        return false;
    }
    const uchar *pPrefix =
        reinterpret_cast<const uchar *>(baPrefix.constData());

    const quint16 nYear = qFromLittleEndian<quint16>(pPrefix);
    const quint16 nMonth = qFromLittleEndian<quint16>(pPrefix + 2);
    const quint16 nDay = qFromLittleEndian<quint16>(pPrefix + 4);
    const quint16 nHour = qFromLittleEndian<quint16>(pPrefix + 6);
    const quint16 nMinute = qFromLittleEndian<quint16>(pPrefix + 8);
    const quint16 nSecond = qFromLittleEndian<quint16>(pPrefix + 10);
    if (nYear < ASCEND_MIN_YEAR || nYear > ASCEND_MAX_YEAR) return false;
    const QDate date(nYear, nMonth, nDay);
    const QTime time(nHour, nMinute, nSecond);
    // QDate/QTime reject impossible calendar dates (month 0, 30 February,
    // hour 24), which is the whole strength of this magicless gate.
    if (!date.isValid() || !time.isValid()) return false;

    context.nLiteralMode = pPrefix[ASCEND_HEADER_SIZE];
    context.nDictionaryBits = pPrefix[ASCEND_HEADER_SIZE + 1];
    if (context.nLiteralMode > ASCEND_MAX_LITERAL_MODE ||
        context.nDictionaryBits < ASCEND_MIN_DICTIONARY_BITS ||
        context.nDictionaryBits > ASCEND_MAX_DICTIONARY_BITS) {
        return false;
    }

    context.nDataOffset = ASCEND_HEADER_SIZE;
    context.nCompressedSize = context.nInputSize - ASCEND_HEADER_SIZE;
    if (!ascendRangeWithin(context.nInputSize, context.nDataOffset,
                           context.nCompressedSize)) {
        return false;
    }

    const QByteArray baPacked = read_array_process(
        context.nDataOffset, context.nCompressedSize, pPdStruct);
    if (baPacked.size() != context.nCompressedSize ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // The container carries no size, no name and no checksum, so the trial
    // decode is both the only source of the unpacked size and the only real
    // structural check.  Requiring the stream to end exactly at EOF is what
    // makes the gate selective: on the reference corpus it is true for all 32
    // members and for nothing else in a 4573-file mixed-format sweep.
    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()),
            context.nCompressedSize, ASCEND_MAX_UNPACKED_SIZE, &nConsumed,
            &nRawSize)) {
        return false;
    }
    if (nConsumed != context.nCompressedSize || nRawSize < 1) return false;

    context.nUncompressedSize = nRawSize;
    context.dtModified = QDateTime(date, time);
    context.sFileName = memberName(guardedSource);
    if (!guardedSource) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XAscend::isValid(PDSTRUCT *pPdStruct)
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

bool XAscend::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAscend archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAscend::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAscend(pDevice);
}

QList<QString> XAscend::getSearchSignatures()
{
    // Deliberately empty: the first twelve bytes are a timestamp, so there is
    // no constant byte anywhere in the container.  Detection is structural
    // (plausible date + DCL header + a stream that ends exactly at EOF).
    return QList<QString>();
}

XBinary::FT XAscend::getFileType()
{
    return FT_ASCEND;
}

XBinary::MODE XAscend::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAscend::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XAscend::getArch()
{
    return QString();
}

QString XAscend::getFileFormatExt()
{
    // No single canonical extension: the installer replaces the last character
    // of the original one with '!'.  .EX!/.DL! is the dominant shape.
    return QStringLiteral("ex!");
}

QString XAscend::getFileFormatExtsString()
{
    return QStringLiteral("Ascend for Windows compressed file (*.??!)");
}

QString XAscend::getMIMEString()
{
    return QStringLiteral("application/x-ascend-compressed");
}

QString XAscend::getVersion()
{
    return QString();
}

qint64 XAscend::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    // parseContext only succeeds when the DCL stream ends exactly at EOF, so
    // the whole file is the archive and there is never an overlay.
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XAscend::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAscend::getMemoryMap(MAPMODE mapMode,
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

QString XAscend::methodToString(quint8 nLiteralMode, quint8 nDictionaryBits)
{
    const QString sLiterals = (nLiteralMode == 0)
                                  ? QStringLiteral("uncoded literals")
                                  : QStringLiteral("coded literals");
    const qint64 nDictionarySize =
        Q_INT64_C(1) << (static_cast<qint64>(nDictionaryBits) + 6);
    return QStringLiteral("PKWARE DCL implode (%1, %2 byte dictionary)")
        .arg(sLiterals)
        .arg(nDictionarySize);
}

bool XAscend::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XAscend::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ASCEND_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nCompressedSize);
        // The size is not stored anywhere; it comes from the trial decode.
        // Publishing anything else (-1, an estimate) makes the DCL dispatch
        // in xdecompress.cpp bail out and write nothing at all.
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  HANDLE_METHOD_PKWARE_DCL_IMPLODE);
        part.mapProperties.insert(
            FPART_PROP_REPORTEDMETHOD,
            methodToString(context.nLiteralMode, context.nDictionaryBits));
        result.append(part);
    }
    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        result.append(part);
    }
    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XAscend::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAscend::initUnpack(UNPACK_STATE *pState,
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
        tr("Ascend for Windows compressed file; single member, name and size "
           "are not stored in the container"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XAscend::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || pState->nCurrentOffset != 0) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        methodToString(pContext->nLiteralMode, pContext->nDictionaryBits));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The 12-byte header is the only metadata this format carries; it is the
    // ORIGINAL file's mtime, not the archive's.
    if (pContext->dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, pContext->dtModified);
    }
    return result;
}

bool XAscend::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nInputSize;
    return false;
}

bool XAscend::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
