/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xqnxbase.h"

#include "Algos/xqnxbasedecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>
#include <QTimeZone>

namespace {
// The 16 constant bytes the QNX IPL puts at the very start of a boot image.
const char *QNXB_BOOT_PREFIX = "\xeb\x4c" "DDDD\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00";
const qint32 QNXB_BOOT_PREFIX_SIZE = 16;
// struct startup_header.  U3 0x00613120 only ever probes 0x400 and 0x3f8, but
// QNX 6.1 builds put it at 0x3d0 (six of the twenty reference images), so the
// header is located the way the QNX IPL itself does it: by scanning for the
// signature word whose preboot_size points back at its own offset.  The
// startup code always sits inside the first sectors, so the scan is bounded.
const qint64 QNXB_STARTUP_HEADER_SIZE = 0x34;
const qint64 QNXB_HEADER_SCAN_LIMIT = 0x10000;
const qint64 QNXB_HEADER_SCAN_STEP = 4;
const quint32 QNXB_STARTUP_SIGNATURE = 0x00ff7eebU;
// U3 0x00613120: imagefs_size is range checked, startup_size only has to be
// positive.  128 MiB is the original's own ceiling.
const qint64 QNXB_MAX_IMAGEFS_SIZE = 0x8000000;
const qint64 QNXB_MAX_COMPRESSED_SIZE = 0x8000000;
const qint32 QNXB_MAX_MEMBERS = 100000;
// image_header: "imagefs" + a flags byte that is 0 or 4 in every known build.
// 0x5c is the length of the FIXED part of struct image_header (signature, flags,
// image_size, hdr_size, dir_offset, boot_ino[4], script_ino, chain_paddr,
// spare[10], mountflags, mountpoint[]).  dir_offset legitimately points exactly
// at 0x5c - it does in all 19 reference images - so it is a lower bound that the
// directory may sit ON, not one it has to clear.
const qint64 QNXB_MIN_IMAGE_SIZE = 0x5c;
// Directory record: fixed part through mtime, then offset/size, then the path.
const qint64 QNXB_DIRENT_MIN_SIZE = 0x1c;
const qint64 QNXB_DIRENT_FILE_SIZE = 0x20;
const quint32 QNXB_MODE_TYPE_MASK = 0xf000U;
const quint32 QNXB_MODE_DIR = 0x4000U;
const quint32 QNXB_MODE_LNK = 0xa000U;
const quint32 QNXB_MODE_REG = 0x8000U;

bool qnxRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XQNXBase::XQNXBase(QIODevice *pDevice) : XArchive(pDevice)
{
}

XQNXBase::~XQNXBase()
{
}

QByteArray XQNXBase::packMemberProperty(qint64 nImageOffset, qint64 nImageFsSize)
{
    // Two little-endian u32s: where the member starts inside the decompressed
    // image filesystem, and how large that filesystem is.  The decoder needs
    // the second one as the output ceiling: a block can overshoot the member's
    // own end by up to a full 64 KiB window before the walk stops.
    QByteArray baResult(8, 0);
    qToLittleEndian<quint32>((quint32)nImageOffset, (uchar *)baResult.data());
    qToLittleEndian<quint32>((quint32)nImageFsSize, (uchar *)baResult.data() + 4);
    return baResult;
}

bool XQNXBase::readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct)
{
    if (!pHeader) return false;

    QPointer<XQNXBase> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < QNXB_BOOT_PREFIX_SIZE + QNXB_STARTUP_HEADER_SIZE) return false;

    const QByteArray baPrefix = read_array_process(0, QNXB_BOOT_PREFIX_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baPrefix.size() != QNXB_BOOT_PREFIX_SIZE)) return false;
    if (memcmp(baPrefix.constData(), QNXB_BOOT_PREFIX, QNXB_BOOT_PREFIX_SIZE) != 0) return false;

    // One read covers every place the header can be; scanning it in memory
    // keeps isValid() cheap.
    qint64 nScanSize = qMin(nInputSize, QNXB_HEADER_SCAN_LIMIT + QNXB_STARTUP_HEADER_SIZE);
    const QByteArray baScan = read_array_process(0, nScanSize, pPdStruct);
    if (!guardedThis || !guardedSource || ((qint64)baScan.size() != nScanSize)) return false;
    const uchar *pScan = (const uchar *)baScan.constData();
    const qint64 nLastCandidate = nScanSize - QNXB_STARTUP_HEADER_SIZE;

    for (qint64 nHeaderOffset = 0; nHeaderOffset <= nLastCandidate; nHeaderOffset += QNXB_HEADER_SCAN_STEP) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pHeaderData = pScan + nHeaderOffset;

        // U3 0x00613120 does not look at the signature at all - it identifies
        // the header purely by preboot_size pointing back at the header's own
        // offset, and only ever offers it 0x400 and 0x3f8 as candidates.  Both
        // conditions are required here, which is what makes the open scan safe:
        // the odds of a random dword pair satisfying them are 2^-64.
        if (qFromLittleEndian<quint32>(pHeaderData) != QNXB_STARTUP_SIGNATURE) continue;
        if (qFromLittleEndian<quint32>(pHeaderData + 0x30) != (quint32)nHeaderOffset) continue;

        const qint64 nStartupSize = (qint64)(qint32)qFromLittleEndian<quint32>(pHeaderData + 0x20);
        const qint64 nImageFsSize = (qint64)(qint32)qFromLittleEndian<quint32>(pHeaderData + 0x2c);
        if (nStartupSize <= 0) continue;
        if ((nImageFsSize < 0) || (nImageFsSize > QNXB_MAX_IMAGEFS_SIZE)) continue;

        const qint64 nCompressedOffset = nHeaderOffset + nStartupSize;
        if (!qnxRangeWithin(nInputSize, nCompressedOffset, 2)) continue;

        pHeader->nHeaderOffset = nHeaderOffset;
        pHeader->nVersion = qFromLittleEndian<quint16>(pHeaderData + 0x04);
        pHeader->nFlags1 = (quint8)pHeaderData[0x06];
        pHeader->nStartupSize = nStartupSize;
        pHeader->nStoredSize = (qint64)qFromLittleEndian<quint32>(pHeaderData + 0x24);
        pHeader->nImageFsSize = nImageFsSize;
        pHeader->nCompressedOffset = nCompressedOffset;
        return true;
    }

    return false;
}

bool XQNXBase::parseImage(const QByteArray &baImage, QList<MEMBER> *pListMembers)
{
    if (!pListMembers) return false;
    pListMembers->clear();

    const qint64 nImageSize = baImage.size();
    if (nImageSize <= QNXB_MIN_IMAGE_SIZE) return false;
    const uchar *pImage = (const uchar *)baImage.constData();

    if (memcmp(pImage, "imagefs", 7) != 0) return false;
    if ((pImage[7] != 0) && (pImage[7] != 4)) return false;
    if ((qint64)qFromLittleEndian<quint32>(pImage + 8) != nImageSize) return false;

    const qint64 nDirOffset = (qint64)(qint32)qFromLittleEndian<quint32>(pImage + 0x10);
    // "<=" here rejected every real image: mkifs puts the first directory record
    // immediately after the fixed header, i.e. dir_offset == QNXB_MIN_IMAGE_SIZE.
    if ((nDirOffset < QNXB_MIN_IMAGE_SIZE) || (nDirOffset >= nImageSize)) return false;

    qint64 nOffset = nDirOffset;
    qint64 nRemaining = nImageSize - nDirOffset;
    bool bTerminated = false;

    // U3 0x00612d70 walks the directory with a "remaining" counter that it
    // decrements by hand; the walk stops - successfully - on the first record
    // whose size word is 0, and gives up on anything inconsistent.
    while (nRemaining > (QNXB_DIRENT_MIN_SIZE - 1)) {
        const qint64 nRecordSize = (qint64)qFromLittleEndian<quint16>(pImage + nOffset);
        if (nRecordSize == 0) {
            bTerminated = true;
            break;
        }
        if ((nRecordSize > nRemaining) || (nRecordSize < QNXB_DIRENT_MIN_SIZE)) return false;

        const quint32 nMode = (quint32)qFromLittleEndian<quint16>(pImage + nOffset + 8);
        const quint32 nType = nMode & QNXB_MODE_TYPE_MASK;
        if ((nType == QNXB_MODE_DIR) || (nType == QNXB_MODE_LNK)) {
            nOffset += nRecordSize;
            nRemaining -= nRecordSize;
            continue;
        }
        // Anything that is not a directory, a symlink or a plain file still has
        // to be long enough to carry the offset/size pair.
        if ((nType != QNXB_MODE_REG) && ((nRecordSize - QNXB_DIRENT_MIN_SIZE) < 4)) return false;
        if (nRecordSize < QNXB_DIRENT_FILE_SIZE) return false;

        const qint64 nFileOffset = (qint64)qFromLittleEndian<quint32>(pImage + nOffset + 0x18);
        const qint64 nFileSize = (qint64)qFromLittleEndian<quint32>(pImage + nOffset + 0x1c);
        if (!qnxRangeWithin(nImageSize, nFileOffset, nFileSize)) return false;

        qint64 nNameLimit = nRecordSize - QNXB_DIRENT_FILE_SIZE;
        qint64 nNameOffset = nOffset + QNXB_DIRENT_FILE_SIZE;
        qint64 nNameEnd = nNameOffset;
        while ((nNameLimit > 0) && (pImage[nNameEnd] != 0)) {
            ++nNameEnd;
            --nNameLimit;
        }

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nImageOffset = nFileOffset;
        member.nUncompressedSize = nFileSize;
        member.nMode = nMode;
        member.nMTime = qFromLittleEndian<quint32>(pImage + nOffset + 0x14);
        member.sFileName = QString::fromLatin1(baImage.constData() + nNameOffset, (qint32)(nNameEnd - nNameOffset));
        if (member.sFileName.isEmpty()) return false;
        if (pListMembers->size() >= QNXB_MAX_MEMBERS) return false;
        pListMembers->append(member);

        nRemaining -= QNXB_DIRENT_FILE_SIZE;
        nRemaining -= (nNameEnd - nNameOffset);
        nRemaining -= nNameLimit;
        nOffset = nNameEnd + nNameLimit;
    }

    // U3 0x00612d70 only reports success when it reaches the zero-length
    // record that closes the directory; running off the end of the buffer is a
    // failure there, so it is one here too.
    return bTerminated && !pListMembers->isEmpty();
}

bool XQNXBase::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XQNXBase> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (!readHeader(&context.header, pPdStruct) || !guardedThis || !guardedSource) return false;

    context.nCompressedOffset = context.header.nCompressedOffset;
    context.nImageFsSize = context.header.nImageFsSize;
    if (context.nImageFsSize <= 0) return false;

    // Measure the block chain without decoding it: every block is a big-endian
    // u16 length followed by that many bytes, and a zero length ends the chain.
    qint64 nOffset = context.nCompressedOffset;
    qint32 nBlockCount = 0;
    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!qnxRangeWithin(context.nInputSize, nOffset, 2)) return false;
        const QByteArray baLength = read_array_process(nOffset, 2, pPdStruct);
        if (!guardedThis || !guardedSource || (baLength.size() != 2)) return false;
        const qint64 nBlockSize = ((qint64)(quint8)baLength.at(0) << 8) | (qint64)(quint8)baLength.at(1);
        nOffset += 2;
        if (nBlockSize == 0) break;
        if (!qnxRangeWithin(context.nInputSize, nOffset, nBlockSize)) return false;
        nOffset += nBlockSize;
        if (++nBlockCount > 0x100000) return false;
    }
    if (nBlockCount == 0) return false;

    context.nCompressedSize = nOffset - context.nCompressedOffset;
    if ((context.nCompressedSize <= 0) || (context.nCompressedSize > QNXB_MAX_COMPRESSED_SIZE)) return false;
    context.nArchiveSize = nOffset;

    const QByteArray baPacked = read_array_process(context.nCompressedOffset, context.nCompressedSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baPacked.size() != context.nCompressedSize)) return false;

    QByteArray baImage;
    if (!XQNXBaseDecoder::decodeImage(baPacked, context.nImageFsSize, -1, &baImage)) return false;
    // U3 0x00612cd0 insists the decompressed length is exactly imagefs_size.
    if ((qint64)baImage.size() != context.nImageFsSize) return false;

    if (!parseImage(baImage, &context.listMembers)) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    *pContext = context;
    return true;
}

bool XQNXBase::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    HEADER header = {};
    // The boot prefix is 16 fixed bytes and the startup header points back at
    // its own offset - that is a strong enough signature that the (expensive)
    // whole-image decompression is left to parseContext().
    const bool bResult = readHeader(&header, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XQNXBase::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQNXBase archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XQNXBase::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQNXBase(pDevice);
}

XBinary::FT XQNXBase::getFileType()
{
    return FT_QNX_BASE;
}

XBinary::MODE XQNXBase::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XQNXBase::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XQNXBase::getArch()
{
    return QString();
}

QString XQNXBase::getFileFormatExt()
{
    return QStringLiteral("ifs");
}

QString XQNXBase::getFileFormatExtsString()
{
    return QStringLiteral("QNX Neutrino boot image (*.ifs *.boot *.altboot)");
}

QString XQNXBase::getMIMEString()
{
    return QStringLiteral("application/x-qnx-ifs");
}

QString XQNXBase::getVersion()
{
    HEADER header = {};
    if (!readHeader(&header, nullptr)) return QString();
    return QString::number(header.nVersion);
}

qint64 XQNXBase::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XQNXBase::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XQNXBase::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XQNXBase::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XQNXBase::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        // Boot record, startup header and startup code, i.e. everything that
        // is not the compressed image filesystem.
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nCompressedOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Startup");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            // Every member reads the same block chain; the member's own place
            // inside the decompressed image travels in the compress properties.
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = context.nCompressedOffset;
            part.nFileSize = (member.nUncompressedSize > 0) ? context.nCompressedSize : 0;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, part.nFileSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize > 0) ? HANDLE_METHOD_QNX_BASE : HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, packMemberProperty(member.nImageOffset, context.nImageFsSize));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("UCL NRV2B (blocked)"));
            part.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch((qint64)member.nMTime, QTimeZone::utc()));
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
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, result.size())) {
        // The block chain normally stops a handful of bytes short of the end of
        // the file - the image is padded out to a sector boundary.
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

QMap<XBinary::UNPACK_PROP, QVariant> XQNXBase::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XQNXBase::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XQNXBase> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("QNX Neutrino boot image; UCL NRV2B compressed image filesystem"));
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

XBinary::ARCHIVERECORD XQNXBase::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamOffset = pContext->nCompressedOffset;
    result.nStreamSize = (member.nUncompressedSize > 0) ? pContext->nCompressedSize : 0;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize > 0) ? HANDLE_METHOD_QNX_BASE : HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, packMemberProperty(member.nImageOffset, pContext->nImageFsSize));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("UCL NRV2B (blocked)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch((qint64)member.nMTime, QTimeZone::utc()));
    // The image filesystem carries no per-file checksum of any kind.
    return result;
}

bool XQNXBase::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XQNXBase::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
