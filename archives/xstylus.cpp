/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xstylus.h"

#include "Algos/xstylusdecoder.h"

#include <QFileInfo>
#include <QtEndian>

#include <new>

namespace {
const qint64 STYLUS_HEADER_SIZE = 0x10;
const qint32 STYLUS_CRC32_OFFSET = 0x0c;
// Above this the stream is published without a measured plaintext size instead
// of buffering the whole member just to count its output.
const qint64 STYLUS_MAX_MEASURE_SIZE = 0x4000000;  // 64 MB

quint8 stylusToLower(quint8 nCharacter)
{
    return ((nCharacter >= 'A') && (nCharacter <= 'Z'))
               ? static_cast<quint8>(nCharacter + 0x20)
               : nCharacter;
}

bool stylusCheckHeader(const QByteArray &baHeader)
{
    if (baHeader.size() != STYLUS_HEADER_SIZE) return false;
    const uchar *p = reinterpret_cast<const uchar *>(baHeader.constData());
    // "DP" 0x1A 0x07, container version 1, stream kind 3, payload tag "SDC".
    // The reference implementation compares the tag case-insensitively, so the same is done here.
    if ((p[0] != 'D') || (p[1] != 'P') || (p[2] != 0x1a) || (p[3] != 0x07)) {
        return false;
    }
    if (qFromLittleEndian<quint16>(p + 4) != 1) return false;
    if (p[6] != 3) return false;
    if ((stylusToLower(p[7]) != 's') || (stylusToLower(p[8]) != 'd') ||
        (stylusToLower(p[9]) != 'c')) {
        return false;
    }
    return true;
}
}  // namespace

XStylus::XStylus(QIODevice *pDevice) : XArchive(pDevice)
{
}

XStylus::~XStylus()
{
}

bool XStylus::parseContext(CONTEXT *pContext, bool bMeasure,
                           PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <= STYLUS_HEADER_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, STYLUS_HEADER_SIZE, pPdStruct);
    if (!guardedSource ||
        (baHeader.size() != STYLUS_HEADER_SIZE)) {
        return false;
    }
    if (!stylusCheckHeader(baHeader)) return false;

    context.nCrc32 = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baHeader.constData()) +
        STYLUS_CRC32_OFFSET);
    context.nArchiveSize = context.nInputSize;
    context.nCompressedSize = context.nInputSize - STYLUS_HEADER_SIZE;
    context.nUncompressedSize = -1;

    context.sFileName = XBinary::getDeviceFileBaseName(guardedSource);
    if (!guardedSource) return false;
    if (context.sFileName.isEmpty()) {
        context.sFileName = QStringLiteral("stylus_data");
    }
    context.sFileName += QStringLiteral(".sdc");

    if (bMeasure && (context.nCompressedSize <= STYLUS_MAX_MEASURE_SIZE)) {
        const QByteArray baPacked = read_array_process(
            STYLUS_HEADER_SIZE, context.nCompressedSize, pPdStruct);
        if (!guardedSource ||
            (baPacked.size() != context.nCompressedSize)) {
            return false;
        }
        const qint64 nMeasured = XStylusDecoder::measure(
            reinterpret_cast<const quint8 *>(baPacked.constData()),
            baPacked.size(), pPdStruct);
        if (nMeasured >= 0) context.nUncompressedSize = nMeasured;
    }

    if (!isPdStructNotCanceled(pPdStruct)) return false;
    *pContext = context;
    return true;
}

bool XStylus::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XStylus::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XStylus archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XStylus::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XStylus(pDevice);
}

QList<QString> XStylus::getSearchSignatures()
{
    return {QStringLiteral("'DP'1A07010003'SDC'")};
}

XBinary::FT XStylus::getFileType()
{
    return FT_STYLUS;
}

XBinary::MODE XStylus::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XStylus::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XStylus::getArch()
{
    return QString();
}

QString XStylus::getFileFormatExt()
{
    return QStringLiteral("#sd");
}

QString XStylus::getFileFormatExtsString()
{
    return QStringLiteral("Stylus dictionary (*.#sd *.sdc)");
}

QString XStylus::getMIMEString()
{
    return QStringLiteral("application/x-stylus-dictionary");
}

QString XStylus::getVersion()
{
    // +0x04 container version and +0x06 stream kind; both are fixed by the
    // header check, so this is a constant rather than a read.
    return QStringLiteral("1.3");
}

qint64 XStylus::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XStylus::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XStylus::getMemoryMap(MAPMODE mapMode,
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

bool XStylus::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XStylus::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    const bool bNeedSize =
        (nFileParts & (FILEPART_STREAM | FILEPART_REGION)) != 0;
    if (!parseContext(&context, bNeedSize, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = STYLUS_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = STYLUS_HEADER_SIZE;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nCompressedSize);
        if (context.nUncompressedSize >= 0) {
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      context.nUncompressedSize);
        }
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  HANDLE_METHOD_STYLUS);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("SDC"));
        part.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                  CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, context.nCrc32);
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

QMap<XBinary::UNPACK_PROP, QVariant> XStylus::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XStylus::initUnpack(UNPACK_STATE *pState,
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, true, pPdStruct) ||
        !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO, tr("Stylus dictionary; XOR + LZSS (SDC) stream"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XStylus::infoCurrent(UNPACK_STATE *pState,
                                            PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex != 0) || (pState->nCurrentOffset != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = STYLUS_HEADER_SIZE;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nCompressedSize);
    if (pContext->nUncompressedSize >= 0) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                    pContext->nUncompressedSize);
    }
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_STYLUS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("SDC"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, pContext->nCrc32);
    return result;
}

bool XStylus::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nArchiveSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XStylus::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
