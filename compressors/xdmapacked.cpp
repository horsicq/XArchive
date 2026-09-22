/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xdmapacked.h"

#include "Algos/xdcldecoder.h"
#include <QtEndian>

#include <new>

namespace {
// 'd' 'm' 10 11 | char[14] name | qint32 rawSize | u16 date | u16 time |
// "PAKPAK" | u16 0x2A00
const qint64 DMAPACKED_HEADER_SIZE = 0x22;
const qint64 DMAPACKED_NAME_OFFSET = 0x04;
const qint64 DMAPACKED_NAME_SIZE = 14;
const qint64 DMAPACKED_RAWSIZE_OFFSET = 0x12;
const qint64 DMAPACKED_DOSDATE_OFFSET = 0x16;
const qint64 DMAPACKED_DOSTIME_OFFSET = 0x18;
const qint64 DMAPACKED_TAG_OFFSET = 0x1A;
const qint64 DMAPACKED_TAG_SIZE = 6;
const qint64 DMAPACKED_VERSION_OFFSET = 0x20;
const quint32 DMAPACKED_MAGIC = 0x11106D64U;
const quint16 DMAPACKED_VERSION = 0x2A00U;
// A DCL stream cannot be shorter than its own two prelude bytes plus one byte
// holding the start of the end-of-stream code.
const qint64 DMAPACKED_MIN_PACKED_SIZE = 3;
// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext
// length comes out of the file, so the decoder must stay bounded.
const qint64 DMAPACKED_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// Decompression-bomb guard for the trial decode in isValid().  The DCL format
// tops out near 259 plaintext bytes per encoded byte; the reference corpus
// peaks at 8, so this leaves two orders of magnitude of headroom and still
// refuses a header that claims a gigabyte behind a few hundred bytes.
const qint64 DMAPACKED_MAX_RATIO = 1024;
const qint64 DMAPACKED_RATIO_SLACK = 4096;
// PKWARE DCL prelude: literal mode (0 binary / 1 Huffman) then dictionary bits.
// Only 4..6 (1K/2K/4K) are legal.  Every file of the reference corpus writes
// 0/6, but the gate accepts the whole legal range because the format does.
const quint8 DMAPACKED_DCL_MAX_LITERAL_MODE = 1U;
const quint8 DMAPACKED_DCL_MIN_DICT_BITS = 4U;
const quint8 DMAPACKED_DCL_MAX_DICT_BITS = 6U;

bool dmaPackedIsHeader(const QByteArray &baHeader)
{
    if (baHeader.size() < DMAPACKED_HEADER_SIZE) return false;

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromLittleEndian<quint32>(pHeader) != DMAPACKED_MAGIC) return false;
    if (qFromLittleEndian<quint16>(pHeader + DMAPACKED_VERSION_OFFSET) !=
        DMAPACKED_VERSION) {
        return false;
    }
    // "PAKPAK" is not part of the reference tool's gate, but it is present in
    // every file of the corpus and costs nothing to require.
    if (baHeader.mid(qint32(DMAPACKED_TAG_OFFSET), qint32(DMAPACKED_TAG_SIZE)) !=
        QByteArray("PAKPAK", 6)) {
        return false;
    }

    return true;
}

bool dmaPackedIsDclPrelude(const QByteArray &baPayload)
{
    if (baPayload.size() < 2) return false;
    const quint8 nLiteralMode = static_cast<quint8>(baPayload.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPayload.at(1));
    return (nLiteralMode <= DMAPACKED_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= DMAPACKED_DCL_MIN_DICT_BITS) &&
           (nDictBits <= DMAPACKED_DCL_MAX_DICT_BITS);
}

// The name field is a fixed 14 bytes but only the run up to the first NUL is
// the name: the bytes behind it are uninitialised writer memory (AW.MSG is
// followed by B3 00 00 00 00 00, AWH.MSG by B6 ...), not padding.
QString dmaPackedReadName(const QByteArray &baHeader)
{
    const QByteArray baField =
        baHeader.mid(qint32(DMAPACKED_NAME_OFFSET), qint32(DMAPACKED_NAME_SIZE));
    const qint32 nEnd = baField.indexOf('\0');
    if (nEnd <= 0) return QString();

    const QByteArray baName = baField.left(nEnd);
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        // A DOS 8.3 name, so anything a path cannot carry is a reject rather
        // than something to sanitise.
        if ((nCharacter < 0x20U) || (nCharacter > 0x7EU)) return QString();
        const char cCharacter = static_cast<char>(nCharacter);
        if ((cCharacter == '\\') || (cCharacter == '/') ||
            (cCharacter == ':') || (cCharacter == '*') ||
            (cCharacter == '?') || (cCharacter == '"') ||
            (cCharacter == '<') || (cCharacter == '>') ||
            (cCharacter == '|')) {
            return QString();
        }
    }

    return QString::fromLatin1(baName);
}
}  // namespace

XDMAPacked::XDMAPacked(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDMAPacked::~XDMAPacked()
{
}

bool XDMAPacked::parseContext(CONTEXT *pContext, bool bVerifyPayload,
                              PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (DMAPACKED_HEADER_SIZE + DMAPACKED_MIN_PACKED_SIZE)) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, DMAPACKED_HEADER_SIZE, pPdStruct);
    if (!guardedSource || !dmaPackedIsHeader(baHeader)) {
        return false;
    }

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());

    MEMBER member = {};
    member.nDataOffset = DMAPACKED_HEADER_SIZE;
    member.nCompressedSize = context.nInputSize - DMAPACKED_HEADER_SIZE;
    member.nUncompressedSize = static_cast<qint64>(
        static_cast<qint32>(qFromLittleEndian<quint32>(
            pHeader + DMAPACKED_RAWSIZE_OFFSET)));
    member.nDosDate =
        qFromLittleEndian<quint16>(pHeader + DMAPACKED_DOSDATE_OFFSET);
    member.nDosTime =
        qFromLittleEndian<quint16>(pHeader + DMAPACKED_DOSTIME_OFFSET);
    member.sFileName = dmaPackedReadName(baHeader);

    // A zero plaintext length would make the DCL handler write an empty file at
    // exit 0 instead of failing, so it is a reject rather than an empty member.
    if ((member.nUncompressedSize < 1) ||
        (member.nUncompressedSize > DMAPACKED_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }
    if (member.nUncompressedSize >
        ((member.nCompressedSize * DMAPACKED_MAX_RATIO) + DMAPACKED_RATIO_SLACK)) {
        return false;
    }
    // The trial decode below reads the whole packed stream into memory, and the
    // stream extent is "the rest of the file" - a bound the header does not
    // state.  A packed stream larger than the largest plaintext this reader
    // accepts is not a member it could publish anyway.
    if (member.nCompressedSize > DMAPACKED_MAX_UNCOMPRESSED_SIZE) return false;
    if (member.sFileName.isEmpty()) return false;

    const QByteArray baPrelude =
        read_array_process(member.nDataOffset, 2, pPdStruct);
    if (!guardedSource || !dmaPackedIsDclPrelude(baPrelude)) {
        return false;
    }

    if (bVerifyPayload) {
        const QByteArray baPacked = read_array_process(
            member.nDataOffset, member.nCompressedSize, pPdStruct);
        if (!guardedSource ||
            (baPacked.size() != member.nCompressedSize)) {
            return false;
        }

        qint64 nConsumed = 0;
        qint64 nRawSize = 0;
        // The declared length doubles as the decoder's ceiling, so a stream
        // that wants to produce more than the header promises stops there.
        if (!XDclDecoder::scan(
                reinterpret_cast<const uchar *>(baPacked.constData()),
                member.nCompressedSize, member.nUncompressedSize, &nConsumed,
                &nRawSize)) {
            return false;
        }
        // VERIFIED INVARIANT over the 48-file reference corpus: the plaintext
        // the stream produces is exactly the length the header declares.  This
        // format has no checksum, so this equality is the whole gate.
        if (nRawSize != member.nUncompressedSize) return false;
        // Every stream of the corpus ends at the last byte of the file, but
        // trailing padding behind the end-of-stream code is tolerated; an
        // overrun is not.
        if ((nConsumed < DMAPACKED_MIN_PACKED_SIZE) ||
            (nConsumed > member.nCompressedSize)) {
            return false;
        }
    }

    context.listEntries.append(member);
    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XDMAPacked::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // The reference implementation gates on the magic, the sign of the size
    // field and the 0x2A00 word alone.  A trial decode that reproduces the
    // declared plaintext length is added here so the class cannot claim an
    // unrelated file and write garbage at exit 0.
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XDMAPacked::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDMAPacked archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XDMAPacked::createInstance(QIODevice *pDevice, bool bIsImage,
                                    XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDMAPacked(pDevice);
}

QList<QString> XDMAPacked::getSearchSignatures()
{
    // The 4-byte magic plus the inner "PAKPAK" tag and the version word, with
    // the name, size and time stamp wildcarded between them.
    return {QStringLiteral("646D1011"
                           "............................................"
                           "'PAKPAK'002A")};
}

XBinary::FT XDMAPacked::getFileType()
{
    return FT_DMA_PACK;
}

XBinary::MODE XDMAPacked::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XDMAPacked::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XDMAPacked::getArch()
{
    return QString();
}

QString XDMAPacked::getFileFormatExt()
{
    return QStringLiteral("pk$");
}

QString XDMAPacked::getFileFormatExtsString()
{
    return QStringLiteral("DMA packed file (*.pk$)");
}

QString XDMAPacked::getMIMEString()
{
    return QStringLiteral("application/x-dma-packed");
}

qint64 XDMAPacked::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XDMAPacked::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XDMAPacked::getMemoryMap(MAPMODE mapMode,
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

bool XDMAPacked::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XDMAPacked::getFileParts(quint32 nFileParts,
                                               qint32 nLimit,
                                               PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;
    if (context.listEntries.isEmpty()) return listResult;

    const MEMBER &member = context.listEntries.first();

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = DMAPACKED_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
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
                                  HANDLE_METHOD_PKWARE_DCL_IMPLODE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("PKWARE DCL Implode"));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_REGION) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = member.nDataOffset;
        part.nFileSize = member.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = member.sFileName;
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XDMAPacked::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XDMAPacked::initUnpack(UNPACK_STATE *pState,
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
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    // bVerifyPayload = true: the DCL handler takes the plaintext length as an
    // INPUT, so a length the decoder cannot reproduce would silently truncate.
    if (!parseContext(pContext, true, pPdStruct) || !guardedSource || pContext->listEntries.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("DMA packed file; PKWARE DCL Implode payload"));
    pState->nCurrentOffset = pContext->listEntries.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
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

XBinary::ARCHIVERECORD XDMAPacked::infoCurrent(UNPACK_STATE *pState,
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
    const MEMBER &member = pContext->listEntries.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) {
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
                                HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("PKWARE DCL Implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtModified =
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtModified);
    }
    // No checksum property: the container carries none.
    return result;
}

bool XDMAPacked::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pState->nCurrentIndex < pContext->listEntries.size()) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listEntries.size());
}

bool XDMAPacked::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
