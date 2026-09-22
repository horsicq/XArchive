/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xxpakarchive.h"
#include <new>

namespace {
const qint64 XPAK_MAGIC_OFFSET = 0;
const qint64 XPAK_MAGIC_SIZE = 4;
const qint64 XPAK_FILESIZE_OFFSET = 4;
const qint64 XPAK_NAME_OFFSET = 8;
const qint64 XPAK_NAME_SIZE = 13;
const qint64 XPAK_RAWSIZE_OFFSET = 0x15;
const qint64 XPAK_PARAMS_OFFSET = 0x19;
const qint64 XPAK_PARAMS_SIZE = 10;
const qint64 XPAK_HEADER_SIZE = 0x23;
// Header plus at least one coded byte.
const qint64 XPAK_MIN_FILE_SIZE = XPAK_HEADER_SIZE + 1;
// The largest member in the reference corpus is 530312 bytes; the ceiling only
// has to reject a field that is obviously not a length.
const qint64 XPAK_MAX_RAW_SIZE = 0x10000000;
}  // namespace

XXPAKArchive::XXPAKArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XXPAKArchive::~XXPAKArchive()
{
}

bool XXPAKArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < XPAK_MIN_FILE_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, XPAK_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != XPAK_HEADER_SIZE)) return false;

    if (baHeader.mid(XPAK_MAGIC_OFFSET, XPAK_MAGIC_SIZE) != QByteArray("XPAK", 4)) return false;

    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    context.nDeclaredSize = qint64(qFromLittleEndian<quint32>(pHeader + XPAK_FILESIZE_OFFSET));
    context.nRawSize = qint64(qFromLittleEndian<quint32>(pHeader + XPAK_RAWSIZE_OFFSET));

    // Both length fields have to look like lengths before the name is trusted.
    if ((context.nDeclaredSize < XPAK_MIN_FILE_SIZE) || (context.nRawSize <= 0) || (context.nRawSize > XPAK_MAX_RAW_SIZE)) {
        return false;
    }

    // A 13-byte NUL-padded printable field: everything up to the first NUL is
    // printable ASCII, everything after it is NUL, and the name is not empty.
    // This is what carries the detector past a chance 'XPAK' in binary data.
    qint64 nNameLength = 0;
    while ((nNameLength < XPAK_NAME_SIZE) && (pHeader[XPAK_NAME_OFFSET + nNameLength] != 0)) {
        const uchar nChar = pHeader[XPAK_NAME_OFFSET + nNameLength];
        if ((nChar < 0x20) || (nChar > 0x7e)) return false;
        ++nNameLength;
    }
    if (nNameLength == 0) return false;
    qint64 nPadIndex = nNameLength;
    while (nPadIndex < XPAK_NAME_SIZE) {
        if (pHeader[XPAK_NAME_OFFSET + nPadIndex] != 0) return false;
        ++nPadIndex;
    }
    context.sFileName = QString::fromLatin1(baHeader.constData() + XPAK_NAME_OFFSET, qint32(nNameLength));

    context.baParams = baHeader.mid(XPAK_PARAMS_OFFSET, XPAK_PARAMS_SIZE);
    if (context.baParams.size() != XPAK_PARAMS_SIZE) return false;

    // The extents come from what the device actually holds, never from the
    // header field: a truncated sample declares a size it does not have, and
    // handing that length onward would walk a reader off the end.
    context.nStreamOffset = XPAK_HEADER_SIZE;
    context.bTruncated = (context.nDeclaredSize > context.nInputSize);
    const qint64 nDeclaredStreamEnd = qMin(context.nDeclaredSize, context.nInputSize);
    context.nStreamSize = nDeclaredStreamEnd - XPAK_HEADER_SIZE;
    if (context.nStreamSize <= 0) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XXPAKArchive::isValid(PDSTRUCT *pPdStruct)
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

bool XXPAKArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XXPAKArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XXPAKArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XXPAKArchive(pDevice);
}

QList<QString> XXPAKArchive::getSearchSignatures()
{
    // 'XPAK', 21 bytes of size/name, then the parameter block that is identical
    // in every known sample. The magic alone is four bytes and would carve false
    // hits out of ordinary binary data.
    return {QStringLiteral("'XPAK'..........................................09FFFE008000040020FF")};
}

XBinary::FT XXPAKArchive::getFileType()
{
    return FT_XPAK;
}

XBinary::MODE XXPAKArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XXPAKArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XXPAKArchive::getArch()
{
    return QString();
}

QString XXPAKArchive::getFileFormatExt()
{
    return QStringLiteral("xpak");
}

QString XXPAKArchive::getFileFormatExtsString()
{
    return QStringLiteral("XPAK compressed file (*.xpak *.img)");
}

QString XXPAKArchive::getMIMEString()
{
    return QStringLiteral("application/x-xpak");
}

QString XXPAKArchive::getVersion()
{
    return QString();
}

qint64 XXPAKArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return 0;
    return context.nStreamOffset + context.nStreamSize;
}

QList<XBinary::MAPMODE> XXPAKArchive::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XXPAKArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XXPAKArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

// ISSUE-25 fixed: the console now prints FPART_PROP_INFO, so the truncation
// warning lives in that archive-level channel again and the method column is
// back to naming only the codec.

QList<XBinary::FPART> XXPAKArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = XPAK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nRawSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XPAK);
        part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, context.baParams);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("XPAK compression"));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nStreamOffset + context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QList<XBinary::FPART_PROP> XXPAKArchive::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME,  FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_COMPRESSPROPERTIES,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_STREAMOFFSET,   FPART_PROP_STREAMSIZE};
}

QMap<XBinary::UNPACK_PROP, QVariant> XXPAKArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XXPAKArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (pContext->bTruncated) {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("XPAK compressed file (truncated: the header declares %1 bytes, the file holds %2)")
                                                                 .arg(pContext->nDeclaredSize)
                                                                 .arg(pContext->nInputSize));
    } else {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("XPAK compressed file"));
    }
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nStreamSize;
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

XBinary::ARCHIVERECORD XXPAKArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nRawSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XPAK);
    // The record names a codec, so it also carries that codec's parameters:
    // a record that names a method without its profile is not self-describing,
    // and the decoder side of this tree answers a missing property with a real
    // profile rather than refusing.
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, pContext->baParams);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("XPAK compression"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XXPAKArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // Advance FIRST, then report: with a single record the caller expects
    // 0 -> 1 plus a false return, not a refusal to move.
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->nStreamOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nStreamOffset + pContext->nStreamSize;
    return false;
}

bool XXPAKArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
