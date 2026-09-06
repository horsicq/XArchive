/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xexesbookbuilder.h"

#include <QBuffer>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 SBOOK_HEADER_SIZE = 13;
const qint64 SBOOK_TAIL_SIZE = 12;  // terminator record (8) + overlay pointer (4)
const qint32 SBOOK_MAX_NAME_SIZE = 0x400;
const qint32 SBOOK_MAX_MEMBERS = 100000;
const qint32 SBOOK_MAX_CHUNKS = 1048576;
// Every chunk but a member's last one inflates to exactly this much.
const qint64 SBOOK_CHUNK_RAW_SIZE = 0x4000;
const qint64 SBOOK_MAX_CHUNK_PACKED_SIZE = 0x100000;
const qint64 SBOOK_MAX_TOTAL_SIZE = 0x40000000;  // 1 GB sanity cap
// The 8 bytes after the path: a timestamp-shaped word and the codec tag.
const qint64 SBOOK_TAILWORD_SIZE = 8;
const char *SBOOK_CODEC_TAG = "EC2";

bool sbookRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// One chunk: a complete zlib stream that inflates to at most 16384 bytes.
bool sbookInflateChunk(const QByteArray &baChunk, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baChunk.size() < 6) return false;

    QByteArray baInput(baChunk);
    QByteArray baOutput;
    QBuffer inputBuffer(&baInput);
    QBuffer outputBuffer(&baOutput);
    // ReadWrite, not WriteOnly: XDeflateDecoder::decompress_zlib authenticates
    // the RFC 1950 Adler32 footer by re-READING the finished output device
    // (XBinary::getAdler32(pDeviceOutput)).  A write-only QBuffer returns -1 to
    // every read, so the checksum never matched and EVERY chunk of EVERY member
    // was reported as a broken zlib stream - which is what made parseContext()
    // fail and the whole archive refuse to open.
    if (!inputBuffer.open(QIODevice::ReadOnly) || !outputBuffer.open(QIODevice::ReadWrite)) return false;

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &inputBuffer;
    state.pDeviceOutput = &outputBuffer;
    state.nInputOffset = 0;
    state.nInputLimit = baInput.size();
    state.nProcessedOffset = 0;
    state.nProcessedLimit = SBOOK_CHUNK_RAW_SIZE;
    state.mapUnpackProperties.insert(XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE, SBOOK_CHUNK_RAW_SIZE);

    const bool bResult = XDeflateDecoder::decompress_zlib(&state, pPdStruct) && !state.bReadError && !state.bWriteError;
    inputBuffer.close();
    outputBuffer.close();
    if (!bResult) return false;
    if ((qint64)baOutput.size() > SBOOK_CHUNK_RAW_SIZE) return false;

    *pbaResult = baOutput;
    return true;
}
}  // namespace

XEXESBookBuilder::XEXESBookBuilder(QIODevice *pDevice) : XArchive(pDevice)
{
}

XEXESBookBuilder::~XEXESBookBuilder()
{
}

bool XEXESBookBuilder::checkOverlayTag(qint64 nOffset, qint64 nInputSize, char *pcTagFirst, PDSTRUCT *pPdStruct)
{
    if (!sbookRangeWithin(nInputSize, nOffset, SBOOK_HEADER_SIZE)) return false;

    QPointer<XEXESBookBuilder> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const QByteArray baHeader = read_array_process(nOffset, SBOOK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SBOOK_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    // U3 0x00778e70: a length-prefixed 5-character tag, "Sbook" or "Ebook".
    if (qFromLittleEndian<quint32>(pHeader) != 5) return false;
    if ((pHeader[4] != 'S') && (pHeader[4] != 'E')) return false;
    if (memcmp(pHeader + 5, "book", 4) != 0) return false;
    if (pcTagFirst) *pcTagFirst = (char)pHeader[4];
    return true;
}

qint64 XEXESBookBuilder::sectionTableOverlay(qint64 nInputSize, PDSTRUCT *pPdStruct)
{
    QPointer<XEXESBookBuilder> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    const QByteArray baDos = read_array_process(0, 0x40, pPdStruct);
    if (!guardedThis || !guardedSource || (baDos.size() != 0x40)) return -1;
    if (memcmp(baDos.constData(), "MZ", 2) != 0) return -1;
    const qint64 nNtOffset = (qint64)qFromLittleEndian<quint32>((const uchar *)baDos.constData() + 0x3c);
    if (!sbookRangeWithin(nInputSize, nNtOffset, 24)) return -1;

    const QByteArray baNt = read_array_process(nNtOffset, 24, pPdStruct);
    if (!guardedThis || !guardedSource || (baNt.size() != 24)) return -1;
    const uchar *pNt = (const uchar *)baNt.constData();
    if (memcmp(pNt, "PE\x00\x00", 4) != 0) return -1;
    const qint32 nNumberOfSections = (qint32)qFromLittleEndian<quint16>(pNt + 6);
    const qint64 nOptionalSize = (qint64)qFromLittleEndian<quint16>(pNt + 20);
    if ((nNumberOfSections <= 0) || (nNumberOfSections > 96)) return -1;

    const qint64 nTableOffset = nNtOffset + 24 + nOptionalSize;
    const qint64 nTableSize = (qint64)nNumberOfSections * 40;
    if (!sbookRangeWithin(nInputSize, nTableOffset, nTableSize)) return -1;

    const QByteArray baTable = read_array_process(nTableOffset, nTableSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baTable.size() != nTableSize)) return -1;

    qint64 nEnd = 0;
    for (qint32 i = 0; i < nNumberOfSections; ++i) {
        const uchar *pSection = (const uchar *)baTable.constData() + (qint64)i * 40;
        const qint64 nRawSize = (qint64)qFromLittleEndian<quint32>(pSection + 16);
        const qint64 nRawOffset = (qint64)qFromLittleEndian<quint32>(pSection + 20);
        if (nRawSize == 0) continue;
        if (!sbookRangeWithin(nInputSize, nRawOffset, nRawSize)) continue;
        nEnd = qMax(nEnd, nRawOffset + nRawSize);
    }
    return (nEnd > 0) ? nEnd : -1;
}

qint64 XEXESBookBuilder::findOverlay(qint64 nInputSize, PDSTRUCT *pPdStruct)
{
    QPointer<XEXESBookBuilder> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (nInputSize < SBOOK_HEADER_SIZE + SBOOK_TAIL_SIZE) return -1;

    // The stub stores the overlay's own offset in the last four bytes of the
    // file so it can seek to its data at run time.
    const QByteArray baPointer = read_array_process(nInputSize - 4, 4, pPdStruct);
    if (!guardedThis || !guardedSource || (baPointer.size() != 4)) return -1;
    const qint64 nPointed = (qint64)qFromLittleEndian<quint32>((const uchar *)baPointer.constData());
    char cTag = 0;
    if (checkOverlayTag(nPointed, nInputSize, &cTag, pPdStruct)) return nPointed;
    if (!guardedThis || !guardedSource) return -1;

    const qint64 nSectionEnd = sectionTableOverlay(nInputSize, pPdStruct);
    if (!guardedThis || !guardedSource) return -1;
    if ((nSectionEnd > 0) && checkOverlayTag(nSectionEnd, nInputSize, &cTag, pPdStruct)) return nSectionEnd;

    return -1;
}

bool XEXESBookBuilder::measureChain(qint64 nOffset, qint64 nInputSize, qint64 *pnCompressedSize, qint64 *pnUncompressedSize, PDSTRUCT *pPdStruct)
{
    if (!pnCompressedSize || !pnUncompressedSize) return false;

    QPointer<XEXESBookBuilder> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    const qint64 nStart = nOffset;
    qint64 nProduced = 0;
    qint32 nChunks = 0;
    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (++nChunks > SBOOK_MAX_CHUNKS) return false;
        if (!sbookRangeWithin(nInputSize, nOffset, 4)) return false;
        const QByteArray baLength = read_array_process(nOffset, 4, pPdStruct);
        if (!guardedThis || !guardedSource || (baLength.size() != 4)) return false;
        const qint64 nPacked = (qint64)(qint32)qFromLittleEndian<quint32>((const uchar *)baLength.constData());
        nOffset += 4;
        if ((nPacked <= 0) || (nPacked > SBOOK_MAX_CHUNK_PACKED_SIZE)) return false;
        if (!sbookRangeWithin(nInputSize, nOffset, nPacked)) return false;

        const QByteArray baChunk = read_array_process(nOffset, nPacked, pPdStruct);
        if (!guardedThis || !guardedSource || (baChunk.size() != nPacked)) return false;
        QByteArray baRaw;
        if (!sbookInflateChunk(baChunk, &baRaw, pPdStruct)) return false;
        if (!guardedThis || !guardedSource) return false;

        nOffset += nPacked;
        nProduced += baRaw.size();
        if (nProduced > SBOOK_MAX_TOTAL_SIZE) return false;
        // A short chunk is the only end-of-member signal the format has.
        if ((qint64)baRaw.size() != SBOOK_CHUNK_RAW_SIZE) break;
    }

    *pnCompressedSize = nOffset - nStart;
    *pnUncompressedSize = nProduced;
    return true;
}

bool XEXESBookBuilder::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XEXESBookBuilder> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nOverlayOffset = findOverlay(context.nInputSize, pPdStruct);
    if (!guardedThis || !guardedSource || (context.nOverlayOffset < 0)) return false;

    const QByteArray baHeader = read_array_process(context.nOverlayOffset, SBOOK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SBOOK_HEADER_SIZE)) return false;
    context.cTagFirst = baHeader.at(4);
    context.nTotalSize = (qint64)qFromLittleEndian<quint32>((const uchar *)baHeader.constData() + 9);
    if (context.nTotalSize > SBOOK_MAX_TOTAL_SIZE) return false;

    qint64 nOffset = context.nOverlayOffset + SBOOK_HEADER_SIZE;
    bool bTerminated = false;
    while (!bTerminated) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset >= context.nInputSize) {
            // U3 0x00778ec0 treats end of stream as a clean end of the chain,
            // exactly like the explicit terminator record.
            bTerminated = true;
            break;
        }
        if (!sbookRangeWithin(context.nInputSize, nOffset, 8)) return false;
        const QByteArray baRecord = read_array_process(nOffset, 8, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != 8)) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();
        const qint64 nCount = (qint64)qFromLittleEndian<quint32>(pRecord);
        const qint32 nNameSize = (qint32)qFromLittleEndian<quint32>(pRecord + 4);
        const qint64 nRecordOffset = nOffset;
        nOffset += 8;
        if ((nNameSize < 0) || (nNameSize > SBOOK_MAX_NAME_SIZE)) return false;

        if ((nCount == 0) && (nNameSize == 0)) {
            bTerminated = true;
            break;
        }
        if (context.listMembers.isEmpty()) {
            if ((nCount <= 0) || (nCount > SBOOK_MAX_MEMBERS)) return false;
            context.nDeclaredCount = (qint32)nCount;
        }
        if (nNameSize == 0) return false;

        if (!sbookRangeWithin(context.nInputSize, nOffset, nNameSize)) return false;
        const QByteArray baName = read_array_process(nOffset, nNameSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baName.size() != nNameSize)) return false;
        nOffset += nNameSize;
        for (qint32 i = 0; i < nNameSize; ++i) {
            // The stored path is the author's own file name; a control byte
            // here means this is not an Sbook record chain.
            if ((quint8)baName.at(i) < 0x20) return false;
        }

        if (!sbookRangeWithin(context.nInputSize, nOffset, SBOOK_TAILWORD_SIZE)) return false;
        const QByteArray baTail = read_array_process(nOffset, SBOOK_TAILWORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baTail.size() != SBOOK_TAILWORD_SIZE)) return false;
        // U3 reads these eight bytes without looking at them; the last four are
        // "EC2\0" in every known file and make the record chain self-checking.
        if (memcmp(baTail.constData() + 4, SBOOK_CODEC_TAG, 4) != 0) return false;
        const quint32 nStamp = qFromLittleEndian<quint32>((const uchar *)baTail.constData());
        nOffset += SBOOK_TAILWORD_SIZE;

        MEMBER member = {};
        member.nRecordOffset = nRecordOffset;
        member.nDataOffset = nOffset;
        member.nStamp = nStamp;
        member.sFileName = QString::fromLatin1(baName).replace(QLatin1Char('\\'), QLatin1Char('/'));

        if (!measureChain(nOffset, context.nInputSize, &member.nCompressedSize, &member.nUncompressedSize, pPdStruct)) return false;
        if (!guardedThis || !guardedSource) return false;
        nOffset += member.nCompressedSize;

        if (context.listMembers.size() >= SBOOK_MAX_MEMBERS) return false;
        context.listMembers.append(member);
    }

    if (!bTerminated || context.listMembers.isEmpty()) return false;
    // The record chain is followed by the four-byte pointer back to the
    // overlay, which is the last thing in the file.
    context.nArchiveSize = qMin(context.nInputSize, nOffset + 4);
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    *pContext = context;
    return true;
}

bool XEXESBookBuilder::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XEXESBookBuilder> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    bool bResult = false;
    if (guardedSource && !guardedSource->isSequential()) {
        const qint64 nInputSize = guardedSource->size();
        const qint64 nOverlayOffset = findOverlay(nInputSize, pPdStruct);
        if (guardedThis && (nOverlayOffset >= 0)) {
            // The tag is matched; confirm the first record looks like a record
            // rather than inflating the whole book here.
            const qint64 nRecordOffset = nOverlayOffset + SBOOK_HEADER_SIZE;
            if (sbookRangeWithin(nInputSize, nRecordOffset, 8)) {
                const QByteArray baRecord = read_array_process(nRecordOffset, 8, pPdStruct);
                if (guardedThis && (baRecord.size() == 8)) {
                    const uchar *pRecord = (const uchar *)baRecord.constData();
                    const qint64 nCount = (qint64)qFromLittleEndian<quint32>(pRecord);
                    const qint64 nNameSize = (qint64)qFromLittleEndian<quint32>(pRecord + 4);
                    bResult = (nCount > 0) && (nCount <= SBOOK_MAX_MEMBERS) && (nNameSize > 0) && (nNameSize <= SBOOK_MAX_NAME_SIZE) &&
                              sbookRangeWithin(nInputSize, nRecordOffset + 8, nNameSize + SBOOK_TAILWORD_SIZE);
                    if (bResult) {
                        const QByteArray baTail = read_array_process(nRecordOffset + 8 + nNameSize, SBOOK_TAILWORD_SIZE, pPdStruct);
                        bResult = guardedThis && (baTail.size() == SBOOK_TAILWORD_SIZE) && (memcmp(baTail.constData() + 4, SBOOK_CODEC_TAG, 4) == 0);
                    }
                }
            }
        }
    }

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XEXESBookBuilder::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEXESBookBuilder archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XEXESBookBuilder::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XEXESBookBuilder(pDevice);
}

XBinary::FT XEXESBookBuilder::getFileType()
{
    return FT_EXE_SBOOKBUILDER;
}

XBinary::MODE XEXESBookBuilder::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEXESBookBuilder::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XEXESBookBuilder::getArch()
{
    return QString();
}

QString XEXESBookBuilder::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XEXESBookBuilder::getFileFormatExtsString()
{
    return QStringLiteral("SbookBuilder self-running Sbook (*.exe)");
}

QString XEXESBookBuilder::getMIMEString()
{
    return QStringLiteral("application/x-sbookbuilder");
}

QString XEXESBookBuilder::getVersion()
{
    // "EC2" in front of every chunk chain is the only version tag the container
    // carries.
    return QStringLiteral("2");
}

qint64 XEXESBookBuilder::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XEXESBookBuilder::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XEXESBookBuilder::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XEXESBookBuilder::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XEXESBookBuilder::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        // The Delphi stub plus the overlay's own 13-byte tag.
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nOverlayOffset + SBOOK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Stub");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = (member.nUncompressedSize > 0) ? member.nCompressedSize : 0;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize > 0) ? HANDLE_METHOD_EXE_SBOOKBUILDER : HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate (EC2 chunks)"));
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
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XEXESBookBuilder::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEXESBookBuilder::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XEXESBookBuilder> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("SbookBuilder self-running Sbook; chunked zlib members"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XEXESBookBuilder::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = (member.nUncompressedSize > 0) ? member.nCompressedSize : 0;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize > 0) ? HANDLE_METHOD_EXE_SBOOKBUILDER : HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate (EC2 chunks)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The record has no CRC; each chunk carries its own zlib adler32 instead,
    // and the inflater checks it.
    return result;
}

bool XEXESBookBuilder::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XEXESBookBuilder::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
