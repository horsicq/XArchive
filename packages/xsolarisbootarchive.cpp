/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * The block table was read out of the containers themselves: the "TL" length
 * field, the first table offset and the running sum of the packed sizes agree
 * on every one of the 194 corpus files, which is what makes the walk below
 * self-validating rather than a magic-byte guess.
 */

#include "xsolarisbootarchive.h"

#include <QFileInfo>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// The descriptor occupies a whole 512-byte sector; the group always starts on
// the next one.
const qint64 SOLARIS_BOOT_GROUP_OFFSET = 0x200;
const qint64 SOLARIS_BOOT_DESCRIPTOR_SIZE = 8;
const qint64 SOLARIS_BOOT_GROUP_FIXED_HEADER = 12;
const qint64 SOLARIS_BOOT_TABLE_ENTRY_SIZE = 12;

// 19 9E is the shared prefix; the third and fourth bytes select descriptor
// ("TL") or group ("TG").
const quint8 SOLARIS_BOOT_MAGIC_0 = 0x19U;
const quint8 SOLARIS_BOOT_MAGIC_1 = 0x9eU;

const qint32 SOLARIS_BOOT_MAX_BLOCKS = 0x10000;
// The block size is a producer constant (0x8000 everywhere in the corpus).
// The bounds only have to keep a bogus value from sizing an allocation.
const qint64 SOLARIS_BOOT_MIN_BLOCK_SIZE = 0x200;
const qint64 SOLARIS_BOOT_MAX_BLOCK_SIZE = 0x100000;
// A raw Deflate stream cannot be shorter than a stored empty final block.
const qint64 SOLARIS_BOOT_MIN_PACKED_SIZE = 3;
const qint64 SOLARIS_BOOT_MAX_GROUP_SIZE = Q_INT64_C(0x40000000);
const qint64 SOLARIS_BOOT_MAX_UNPACKED_SIZE = Q_INT64_C(0x80000000);

bool solarisBootIsMagic(const uchar *pData, char nThird, char nFourth)
{
    return (pData[0] == SOLARIS_BOOT_MAGIC_0) &&
           (pData[1] == SOLARIS_BOOT_MAGIC_1) &&
           (pData[2] == static_cast<quint8>(nThird)) &&
           (pData[3] == static_cast<quint8>(nFourth));
}
}  // namespace

XSolarisBootArchive::XSolarisBootArchive(QIODevice *pDevice)
    : XArchive(pDevice)
{
}

XSolarisBootArchive::~XSolarisBootArchive()
{
}

QString XSolarisBootArchive::buildMemberName()
{
    // The container never stores a name.  The reference unpacker publishes the
    // container's own name with its last extension removed ("mod.cpio.z" ->
    // "mod.cpio"), which is completeBaseName(); a name with no extension is
    // passed through unchanged.
    QIODevice *guardedSource = getDevice();
    QString sResult;
    if (guardedSource) {
        const QString sDeviceName =
            XBinary::getDeviceFileName(guardedSource);
        if (!sDeviceName.isEmpty()) {
            sResult = QFileInfo(sDeviceName).completeBaseName();
        }
    }
    if (sResult.isEmpty()) sResult = QStringLiteral("solaris_boot.cpio");
    return sResult;
}

bool XSolarisBootArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nGroupOffset = SOLARIS_BOOT_GROUP_OFFSET;
    if (context.nInputSize <
        (SOLARIS_BOOT_GROUP_OFFSET + SOLARIS_BOOT_GROUP_FIXED_HEADER +
         SOLARIS_BOOT_TABLE_ENTRY_SIZE + SOLARIS_BOOT_MIN_PACKED_SIZE)) {
        return false;
    }

    const QByteArray baDescriptor =
        read_array_process(0, SOLARIS_BOOT_DESCRIPTOR_SIZE, pPdStruct);
    if (!guardedSource ||
        (baDescriptor.size() != SOLARIS_BOOT_DESCRIPTOR_SIZE)) {
        return false;
    }
    const uchar *pDescriptor =
        reinterpret_cast<const uchar *>(baDescriptor.constData());
    if (!solarisBootIsMagic(pDescriptor, 'T', 'L')) return false;

    context.nGroupSize =
        static_cast<qint64>(qFromLittleEndian<quint32>(pDescriptor + 4));
    if ((context.nGroupSize < (SOLARIS_BOOT_GROUP_FIXED_HEADER +
                               SOLARIS_BOOT_TABLE_ENTRY_SIZE +
                               SOLARIS_BOOT_MIN_PACKED_SIZE)) ||
        (context.nGroupSize > SOLARIS_BOOT_MAX_GROUP_SIZE) ||
        (context.nGroupSize >
         (context.nInputSize - SOLARIS_BOOT_GROUP_OFFSET))) {
        return false;
    }

    const QByteArray baGroupHeader =
        read_array_process(SOLARIS_BOOT_GROUP_OFFSET,
                           SOLARIS_BOOT_GROUP_FIXED_HEADER, pPdStruct);
    if (!guardedSource ||
        (baGroupHeader.size() != SOLARIS_BOOT_GROUP_FIXED_HEADER)) {
        return false;
    }
    const uchar *pGroupHeader =
        reinterpret_cast<const uchar *>(baGroupHeader.constData());
    // The group magic is a second, independent 32-bit constant at a fixed
    // offset.  Together with the table cross-checks below it is what keeps a
    // random 1.44 MB disk image from being read as this format.
    if (!solarisBootIsMagic(pGroupHeader, 'T', 'G')) return false;

    const quint32 nRawBlockCount = qFromLittleEndian<quint32>(pGroupHeader + 4);
    if ((nRawBlockCount == 0) ||
        (nRawBlockCount > static_cast<quint32>(SOLARIS_BOOT_MAX_BLOCKS))) {
        return false;
    }
    const qint32 nNumberOfBlocks = static_cast<qint32>(nRawBlockCount);
    context.nBlockSizeMax =
        static_cast<qint64>(qFromLittleEndian<quint32>(pGroupHeader + 8));
    if ((context.nBlockSizeMax < SOLARIS_BOOT_MIN_BLOCK_SIZE) ||
        (context.nBlockSizeMax > SOLARIS_BOOT_MAX_BLOCK_SIZE)) {
        return false;
    }

    const qint64 nTableSize =
        static_cast<qint64>(nNumberOfBlocks) * SOLARIS_BOOT_TABLE_ENTRY_SIZE;
    const qint64 nGroupHeaderSize =
        SOLARIS_BOOT_GROUP_FIXED_HEADER + nTableSize;
    if (nGroupHeaderSize >
        (context.nGroupSize - SOLARIS_BOOT_MIN_PACKED_SIZE)) {
        return false;
    }

    const QByteArray baTable =
        read_array_process(SOLARIS_BOOT_GROUP_OFFSET +
                               SOLARIS_BOOT_GROUP_FIXED_HEADER,
                           nTableSize, pPdStruct);
    if (!guardedSource || (baTable.size() != nTableSize)) {
        return false;
    }
    const uchar *pTable = reinterpret_cast<const uchar *>(baTable.constData());

    qint64 nExpectedOffset = nGroupHeaderSize;
    for (qint32 i = 0; i < nNumberOfBlocks; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const uchar *pEntry = pTable + (static_cast<qint64>(i) *
                                        SOLARIS_BOOT_TABLE_ENTRY_SIZE);
        const qint64 nUncompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pEntry + 0));
        const qint64 nCompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pEntry + 4));
        const qint64 nRelativeOffset =
            static_cast<qint64>(qFromLittleEndian<quint32>(pEntry + 8));

        // The table is contiguous and starts immediately after itself.  Any
        // gap, overlap or reordering is a reject: this is the check that makes
        // the walk self-validating.
        if (nRelativeOffset != nExpectedOffset) return false;
        if ((nCompressedSize < SOLARIS_BOOT_MIN_PACKED_SIZE) ||
            (nCompressedSize > (context.nGroupSize - nRelativeOffset))) {
            return false;
        }
        // Every block but the last is produced by filling the encoder's window
        // completely, so a short block anywhere else means the table is not
        // this format's.
        if (nUncompressedSize <= 0) return false;
        if (i < (nNumberOfBlocks - 1)) {
            if (nUncompressedSize != context.nBlockSizeMax) return false;
        } else if (nUncompressedSize > context.nBlockSizeMax) {
            return false;
        }

        BLOCK block = {};
        block.nCompressedOffset = SOLARIS_BOOT_GROUP_OFFSET + nRelativeOffset;
        block.nCompressedSize = nCompressedSize;
        block.nUncompressedSize = nUncompressedSize;
        context.listBlocks.append(block);

        context.nUncompressedSize += nUncompressedSize;
        if (context.nUncompressedSize > SOLARIS_BOOT_MAX_UNPACKED_SIZE) {
            return false;
        }
        nExpectedOffset = nRelativeOffset + nCompressedSize;
    }

    // The descriptor's length field and the end of the last block must agree
    // exactly.  Two independent encodings of the same number matching is the
    // single strongest structural test this container offers.
    if (nExpectedOffset != context.nGroupSize) return false;

    context.nArchiveSize = SOLARIS_BOOT_GROUP_OFFSET + context.nGroupSize;
    context.sFileName = buildMemberName();
    if (!guardedSource) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XSolarisBootArchive::isValid(PDSTRUCT *pPdStruct)
{
    // Detection runs on a device the caller still owns, so the probe has to
    // leave the cursor exactly where it found it.
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XSolarisBootArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSolarisBootArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSolarisBootArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                             XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSolarisBootArchive(pDevice);
}

QList<QString> XSolarisBootArchive::getSearchSignatures()
{
    // 19 9E 'TL' at offset 0, with the group magic pinned 0x200 bytes later.
    return {QStringLiteral("199E'TL'")};
}

XBinary::FT XSolarisBootArchive::getFileType()
{
    return FT_SOLARIS_BOOT;
}

XBinary::MODE XSolarisBootArchive::getMode()
{
    return MODE_DATA;
}

qint32 XSolarisBootArchive::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XSolarisBootArchive::getEndian()
{
    // x86 boot path: every header field is little-endian even though the
    // payload is an SVR4 cpio image.
    return ENDIAN_LITTLE;
}

QString XSolarisBootArchive::getArch()
{
    return QString();
}

QString XSolarisBootArchive::getFileFormatExt()
{
    return QStringLiteral("z");
}

QString XSolarisBootArchive::getFileFormatExtsString()
{
    return QStringLiteral("Solaris boot compressed archive (*.z)");
}

QString XSolarisBootArchive::getMIMEString()
{
    return QStringLiteral("application/x-solaris-boot");
}

QString XSolarisBootArchive::getVersion()
{
    return QString();
}

qint64 XSolarisBootArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSolarisBootArchive::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSolarisBootArchive::getMemoryMap(MAPMODE mapMode,
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

bool XSolarisBootArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSolarisBootArchive::getFileParts(quint32 nFileParts,
                                                        qint32 nLimit,
                                                        PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, listResult.size())) {
        // Only the descriptor sector.  The group's own header and block table
        // stay inside the stream extent below, because the codec consumes
        // them: splitting them out would leave the member undecodable from
        // its published coordinates.
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nGroupOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        canAppendPart(nLimit, listResult.size())) {
        // One member: the whole group decodes to a single concatenated cpio
        // image, so the stream extent is the group and not an individual
        // Deflate block.
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nGroupOffset;
        part.nFileSize = context.nGroupSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nGroupSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  HANDLE_METHOD_SOLARIS_BOOT);
        part.mapProperties.insert(
            FPART_PROP_REPORTEDMETHOD,
            QStringLiteral("Solaris boot blocked Deflate (%1 x %2)")
                .arg(context.listBlocks.size())
                .arg(context.nBlockSizeMax));
        part.mapProperties.insert(FPART_PROP_ISFOLDER, false);
        listResult.append(part);
    }

    for (qint32 i = 0; (i < context.listBlocks.size()) &&
                       (nFileParts & FILEPART_REGION) &&
                       canAppendPart(nLimit, listResult.size());
         i++) {
        const BLOCK &block = context.listBlocks.at(i);
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = block.nCompressedOffset;
        part.nFileSize = block.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Block %1").arg(i);
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    // Containers are padded out to a 512-byte sector, and boot floppy images
    // keep the rest of the medium behind the archive; both are overlay.
    if ((nFileParts & FILEPART_OVERLAY) &&
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant>
XSolarisBootArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSolarisBootArchive::initUnpack(
    UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties,
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
    if (!parseContext(pContext, pPdStruct) ||
        !guardedSource || pContext->listBlocks.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Solaris boot compressed archive; one blocked-Deflate cpio image"));
    pState->nCurrentOffset = pContext->nGroupOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source; without this the listing still works
    // while extraction silently produces nothing.
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

XBinary::ARCHIVERECORD XSolarisBootArchive::infoCurrent(UNPACK_STATE *pState,
                                                        PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pState->nNumberOfRecords != 1)) {
        return ARCHIVERECORD();
    }
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (pContext->listBlocks.isEmpty() ||
        (pState->nCurrentOffset != pContext->nGroupOffset)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    // The member is a real byte range of this device: the whole "TG" group.
    // The dispatch for HANDLE_METHOD_SOLARIS_BOOT re-reads the block table off
    // that range, so no separate coordinate is needed.
    result.nStreamOffset = pContext->nGroupOffset;
    result.nStreamSize = pContext->nGroupSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nGroupSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_SOLARIS_BOOT);
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        QStringLiteral("Solaris boot blocked Deflate (%1 x %2)")
            .arg(pContext->listBlocks.size())
            .arg(pContext->nBlockSizeMax));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No timestamp, attribute or CRC fields exist anywhere in the container;
    // the cpio image inside carries all of that per file.
    return result;
}

bool XSolarisBootArchive::moveToNext(UNPACK_STATE *pState,
                                     PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);

    // Advance first, then report.  Guarding on nNumberOfRecords - 1 (or
    // returning without incrementing) leaves the caller's cursor parked on
    // record 0 and the whole listing comes back empty.
    ++pState->nCurrentIndex;
    pState->nCurrentOffset =
        (pState->nCurrentIndex >= pState->nNumberOfRecords)
            ? pContext->nArchiveSize
            : pContext->nGroupOffset;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XSolarisBootArchive::finishUnpack(UNPACK_STATE *pState,
                                       PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP>
XSolarisBootArchive::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME,  FPART_PROP_COMPRESSEDSIZE,
            FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER};
}
