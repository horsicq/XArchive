/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhdcopy.h"

#include <QtEndian>

#include <new>

#include "Algos/xhdcopydecoder.h"

namespace {
const qint64 HDCOPY_HEADER_SIZE = 0xb8;
const qint64 HDCOPY_MAP_OFFSET = 0x10;
const qint64 HDCOPY_MAP_SIZE = 168;
const qint64 HDCOPY_SECTOR_SIZE = 512;
const qint64 HDCOPY_LABEL_OFFSET = 3;
const qint64 HDCOPY_LABEL_SIZE = 11;
const quint8 HDCOPY_MAGIC_0 = 0xffU;
const quint8 HDCOPY_MAGIC_1 = 0x18U;
const quint8 HDCOPY_MIN_LAST_CYLINDER = 79U;
const quint8 HDCOPY_MAX_LAST_CYLINDER = 83U;

bool hdcopyIsValidSectorCount(quint8 nSectors)
{
    return (nSectors == 9U) || (nSectors == 10U) || (nSectors == 15U) || (nSectors == 17U) || (nSectors == 18U) || (nSectors == 20U) ||
           (nSectors == 21U);
}
}  // namespace

XHDCopy::XHDCopy(QIODevice *pDevice) : XArchive(pDevice)
{
}

XHDCopy::~XHDCopy()
{
}

bool XHDCopy::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <= HDCOPY_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, HDCOPY_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != HDCOPY_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if ((pHeader[0] != HDCOPY_MAGIC_0) || (pHeader[1] != HDCOPY_MAGIC_1)) return false;
    const quint8 nLabelLength = pHeader[2];
    if (nLabelLength > HDCOPY_LABEL_SIZE) return false;
    // Label padding: NUL for an empty label, spaces after a non-empty one.
    // This is what turns a two-byte magic into a usable detector.
    const char cPad = (nLabelLength == 0) ? '\0' : ' ';
    for (qint64 i = nLabelLength; i < HDCOPY_LABEL_SIZE; ++i) {
        if (pHeader[HDCOPY_LABEL_OFFSET + i] != quint8(cPad)) return false;
    }

    context.nLastCylinder = pHeader[0x0e];
    context.nSectorsPerTrack = pHeader[0x0f];
    if ((context.nLastCylinder < HDCOPY_MIN_LAST_CYLINDER) || (context.nLastCylinder > HDCOPY_MAX_LAST_CYLINDER)) return false;
    if (!hdcopyIsValidSectorCount(context.nSectorsPerTrack)) return false;

    context.nTrackCount = (qint64(context.nLastCylinder) + 1) * 2;
    if (context.nTrackCount > HDCOPY_MAP_SIZE) return false;
    context.nTrackSize = qint64(context.nSectorsPerTrack) * HDCOPY_SECTOR_SIZE;
    context.nImageSize = context.nTrackCount * context.nTrackSize;
    context.sLabel = QString::fromLatin1(baHeader.constData() + HDCOPY_LABEL_OFFSET, qint32(nLabelLength));

    // Walk the block chain without expanding it: every used track contributes
    // [u16 length][length bytes], and the chain has to end exactly at EOF.
    qint64 nOffset = HDCOPY_HEADER_SIZE;
    for (qint64 nTrack = 0; nTrack < context.nTrackCount; ++nTrack) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (pHeader[HDCOPY_MAP_OFFSET + nTrack] == 0) continue;
        ++context.nUsedTracks;
        if (nOffset + 2 > context.nInputSize) return false;
        const QByteArray baLength = read_array_process(nOffset, 2, pPdStruct);
        if (!guardedSource || (baLength.size() != 2)) return false;
        const qint64 nBlockSize = qint64(qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baLength.constData())));
        if (nBlockSize < 1) return false;
        nOffset += 2;
        if (nBlockSize > context.nInputSize - nOffset) return false;
        nOffset += nBlockSize;
    }
    if (context.nUsedTracks == 0) return false;
    if (nOffset != context.nInputSize) return false;
    if (!guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    context.nArchiveSize = nOffset;
    *pContext = context;
    return true;
}

bool XHDCopy::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XHDCopy::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHDCopy archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHDCopy::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHDCopy(pDevice);
}

XBinary::FT XHDCopy::getFileType()
{
    return FT_HDCOPY;
}

XBinary::MODE XHDCopy::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XHDCopy::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XHDCopy::getArch()
{
    return QString();
}

QString XHDCopy::getFileFormatExt()
{
    return QStringLiteral("img");
}

QString XHDCopy::getFileFormatExtsString()
{
    return QStringLiteral("HD-COPY disk image (*.img)");
}

QString XHDCopy::getMIMEString()
{
    return QStringLiteral("application/x-hdcopy-image");
}

QString XHDCopy::getVersion()
{
    // The container carries no version field; 0xff 0x18 is shared by every
    // HD-COPY release that writes this layout.
    return QString();
}

qint64 XHDCopy::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XHDCopy::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XHDCopy::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XHDCopy::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XHDCopy::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = HDCOPY_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        // The decoder needs the header (geometry plus the track map), so the
        // published stream is the whole container, not just the block area.
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = QStringLiteral("Image.img");
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nArchiveSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nImageSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_HDCOPY);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("HD-COPY RLE"));
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
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, result.size())) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XHDCopy::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XHDCopy::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("HD-COPY disk image; RLE compressed tracks"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XHDCopy::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex != 0) ||
        (pState->nNumberOfRecords != 1)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pContext->nImageSize <= 0)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nArchiveSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, QStringLiteral("Image.img"));
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nArchiveSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nImageSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_HDCOPY);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("HD-COPY RLE"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (!pContext->sLabel.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_FILECOMMENT, pContext->sLabel);
    }
    return result;
}

bool XHDCopy::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nArchiveSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XHDCopy::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
