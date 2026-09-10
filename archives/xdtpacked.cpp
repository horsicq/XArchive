/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xdtpacked.h"

#include "Algos/xdcldecoder.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// "DT" | quint16 version | quint16 flags | junk[23] | quint32 rawSize |
// junk[4] | quint16 dosDate | quint16 dosTime
const qint64 DTPACK_HEADER_SIZE = 41;
const qint64 DTPACK_RAWSIZE_OFFSET = 29;
const qint64 DTPACK_DOSDATE_OFFSET = 37;
const qint64 DTPACK_DOSTIME_OFFSET = 39;
const quint16 DTPACK_VERSION = 2U;
const quint16 DTPACK_FLAGS = 1U;
// A DCL stream cannot be shorter than its own two prelude bytes plus one byte
// holding the start of the end-of-stream code.
const qint64 DTPACK_MIN_PACKED_SIZE = 3;
// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext length
// comes out of the file, so the decoder must stay bounded.
const qint64 DTPACK_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// Decompression-bomb guard for the trial decode in isValid().  The DCL format
// tops out near 259 plaintext bytes per encoded byte; the reference corpus peaks
// at 68, so this leaves an order of magnitude of headroom and still refuses a
// header that claims a gigabyte behind a few hundred bytes of payload.
const qint64 DTPACK_MAX_RATIO = 1024;
const qint64 DTPACK_RATIO_SLACK = 4096;
// PKWARE DCL prelude: literal mode (0 binary / 1 Huffman) then dictionary bits.
// Only 4..6 (1K/2K/4K) are legal.  Every file of the reference corpus writes
// 0/6, but the gate accepts the whole legal range because the format does.
const quint8 DTPACK_DCL_MAX_LITERAL_MODE = 1U;
const quint8 DTPACK_DCL_MIN_DICT_BITS = 4U;
const quint8 DTPACK_DCL_MAX_DICT_BITS = 6U;

bool dtPackIsMagic(const QByteArray &baHeader)
{
    if (baHeader.size() < DTPACK_HEADER_SIZE) return false;
    if (!baHeader.startsWith("DT")) return false;

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromLittleEndian<quint16>(pHeader + 2) != DTPACK_VERSION) return false;
    if (qFromLittleEndian<quint16>(pHeader + 4) != DTPACK_FLAGS) return false;

    return true;
}

bool dtPackIsDclPrelude(const QByteArray &baPayload)
{
    if (baPayload.size() < 2) return false;
    const quint8 nLiteralMode = static_cast<quint8>(baPayload.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPayload.at(1));
    return (nLiteralMode <= DTPACK_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= DTPACK_DCL_MIN_DICT_BITS) &&
           (nDictBits <= DTPACK_DCL_MAX_DICT_BITS);
}
}  // namespace

XDTPacked::XDTPacked(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDTPacked::~XDTPacked()
{
}

bool XDTPacked::parseContext(CONTEXT *pContext, bool bVerifyPayload,
                             PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XDTPacked> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < DTPACK_HEADER_SIZE + DTPACK_MIN_PACKED_SIZE) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, DTPACK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || !dtPackIsMagic(baHeader)) {
        return false;
    }
    context.nVersion = DTPACK_VERSION;

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());

    MEMBER member = {};
    member.nDataOffset = DTPACK_HEADER_SIZE;
    member.nCompressedSize = context.nInputSize - DTPACK_HEADER_SIZE;
    member.nUncompressedSize = static_cast<qint64>(
        qFromLittleEndian<quint32>(pHeader + DTPACK_RAWSIZE_OFFSET));
    member.nDosDate = qFromLittleEndian<quint16>(pHeader + DTPACK_DOSDATE_OFFSET);
    member.nDosTime = qFromLittleEndian<quint16>(pHeader + DTPACK_DOSTIME_OFFSET);

    // A zero plaintext length would make the DCL handler write an empty file at
    // exit 0 instead of failing, so it is a reject rather than an empty member.
    if ((member.nUncompressedSize < 1) ||
        (member.nUncompressedSize > DTPACK_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }
    if (member.nUncompressedSize >
        (member.nCompressedSize * DTPACK_MAX_RATIO) + DTPACK_RATIO_SLACK) {
        return false;
    }

    const QByteArray baPrelude =
        read_array_process(member.nDataOffset, 2, pPdStruct);
    if (!guardedThis || !guardedSource || !dtPackIsDclPrelude(baPrelude)) {
        return false;
    }

    // The container stores no name of its own; the packer overwrites the file in
    // place on the install media, so the container's own name IS the name.
    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    if (!sDeviceName.isEmpty()) {
        member.sFileName = QFileInfo(sDeviceName).fileName();
    }
    if (member.sFileName.isEmpty()) {
        member.sFileName = QStringLiteral("dt_data");
    }

    if (bVerifyPayload) {
        const QByteArray baPacked = read_array_process(
            member.nDataOffset, member.nCompressedSize, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baPacked.size() != member.nCompressedSize)) {
            return false;
        }

        qint64 nConsumed = 0;
        qint64 nRawSize = 0;
        // The declared length doubles as the decoder's ceiling, so a stream that
        // wants to produce more than the header promises stops right there.
        if (!XDclDecoder::scan(
                reinterpret_cast<const uchar *>(baPacked.constData()),
                member.nCompressedSize, member.nUncompressedSize, &nConsumed,
                &nRawSize)) {
            return false;
        }
        // VERIFIED INVARIANT over the 160-file reference corpus: the plaintext
        // the stream produces is exactly the length the header declares.  This
        // format has no checksum, so this equality is the whole gate.
        if (nRawSize != member.nUncompressedSize) return false;
        // 159 of the 160 streams end exactly at EOF; one carries 912 bytes of
        // diskette padding behind the end-of-stream code, so trailing slack is
        // tolerated but overrun is not.
        if ((nConsumed < DTPACK_MIN_PACKED_SIZE) ||
            (nConsumed > member.nCompressedSize)) {
            return false;
        }
    }

    context.listEntries.append(member);
    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XDTPacked::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // The magic is six bytes with no length or checksum behind it; only a trial
    // decode that reproduces the declared plaintext length keeps this class from
    // claiming unrelated files and writing garbage at exit 0.
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XDTPacked::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDTPacked archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XDTPacked::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDTPacked(pDevice);
}

QList<QString> XDTPacked::getSearchSignatures()
{
    // "DT" alone is two bytes of nothing; pin the version and flag words too.
    return {QStringLiteral("'DT'02000100")};
}

XBinary::FT XDTPacked::getFileType()
{
    return FT_DT_PACK;
}

XBinary::MODE XDTPacked::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XDTPacked::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XDTPacked::getArch()
{
    return QString();
}

QString XDTPacked::getFileFormatExt()
{
    // The packed file keeps the original name and extension of the file it
    // replaces (DEFAULT.WCC, SSUPLOAD.C, WFXCOMM.DRV), so there is no extension
    // of the container's own to report.
    return QString();
}

QString XDTPacked::getFileFormatExtsString()
{
    return QStringLiteral("Delrina DT packed file");
}

QString XDTPacked::getMIMEString()
{
    return QStringLiteral("application/x-delrina-dt");
}

QString XDTPacked::getVersion()
{
    return QString::number(DTPACK_VERSION);
}

qint64 XDTPacked::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XDTPacked::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XDTPacked::getMemoryMap(MAPMODE mapMode,
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

bool XDTPacked::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XDTPacked::getFileParts(quint32 nFileParts, qint32 nLimit,
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
        part.nFileSize = DTPACK_HEADER_SIZE;
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
                                  QStringLiteral("PKWARE DCL Implode/TTCOMP"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XDTPacked::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XDTPacked::initUnpack(UNPACK_STATE *pState,
                           const QMap<UNPACK_PROP, QVariant> &mapProperties,
                           PDSTRUCT *pPdStruct)
{
    QPointer<XDTPacked> guardedThis(this);
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
    // bVerifyPayload = true: the DCL handler takes the plaintext length as an
    // INPUT, so a length the decoder cannot reproduce would silently truncate.
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listEntries.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Delrina DT packed file; PKWARE DCL Implode payload"));
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

XBinary::ARCHIVERECORD XDTPacked::infoCurrent(UNPACK_STATE *pState,
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
                                QStringLiteral("PKWARE DCL Implode/TTCOMP"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        const QDateTime dtModified =
            dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        }
    }
    // No checksum property: the container carries none, anywhere.
    return result;
}

bool XDTPacked::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XDTPacked::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
