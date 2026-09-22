/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsls.h"

#include <QFileInfo>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const char SLS_MAGIC[9] = {'\x1f', 'S', '/', 'L', '?', 'S', 'O', 'A', '_'};
const qint64 SLS_MAGIC_SIZE = 9;
const qint64 SLS_HEADER_SIZE = 13;
const qint64 SLS_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap
}  // namespace

XSLS::XSLS(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSLS::~XSLS()
{
}

QString XSLS::memberName()
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return QStringLiteral("sls_data");

    // The reference implementation names the single member after the archive file itself, extension
    // included; there is nothing else in the container to name it with.
    const QString sPath = XBinary::getDeviceFileName(guardedSource);
    QString sResult;
    if (!sPath.isEmpty()) {
        sResult = QFileInfo(sPath).fileName();
    }
    if (sResult.isEmpty()) sResult = QStringLiteral("sls_data");
    return sResult;
}

bool XSLS::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // A container with no payload byte at all cannot decode to anything.
    if (context.nInputSize < SLS_HEADER_SIZE + 1) return false;

    const QByteArray baHeader =
        read_array_process(0, SLS_HEADER_SIZE, pPdStruct);
    if (!guardedSource ||
        (baHeader.size() != SLS_HEADER_SIZE)) {
        return false;
    }
    if (std::memcmp(baHeader.constData(), SLS_MAGIC, SLS_MAGIC_SIZE) != 0) {
        return false;
    }

    // The size word is read SIGNED, exactly as the reference implementation does; negative is a reject.
    const qint32 nUncompressed = static_cast<qint32>(qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baHeader.constData()) +
        SLS_MAGIC_SIZE));
    if (nUncompressed <= 0) return false;
    if (nUncompressed > SLS_MAX_UNCOMPRESSED_SIZE) return false;

    context.nDataOffset = SLS_HEADER_SIZE;
    context.nCompressedSize = context.nInputSize - SLS_HEADER_SIZE;
    context.nUncompressedSize = nUncompressed;
    context.sFileName = memberName();
    if (!guardedSource) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XSLS::isValid(PDSTRUCT *pPdStruct)
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

bool XSLS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSLS archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSLS::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSLS(pDevice);
}

QList<QString> XSLS::getSearchSignatures()
{
    return {QStringLiteral("1F'S/L?SOA_'")};
}

XBinary::FT XSLS::getFileType()
{
    return FT_SLS;
}

XBinary::MODE XSLS::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSLS::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSLS::getArch()
{
    return QString();
}

QString XSLS::getFileFormatExt()
{
    return QStringLiteral("sl$");
}

QString XSLS::getFileFormatExtsString()
{
    return QStringLiteral("WinSense compressed file (*.sl$ *.dat *.hlp *.grp)");
}

QString XSLS::getMIMEString()
{
    return QStringLiteral("application/x-sls");
}

QString XSLS::getVersion()
{
    return QString();
}

qint64 XSLS::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XSLS::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSLS::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSLS::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XSLS::getFileParts(quint32 nFileParts, qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SLS_HEADER_SIZE;
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
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SLS);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("LZHUF (8K window, F=90)"));
        result.append(part);
    }
    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Compressed Data");
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

QMap<XBinary::UNPACK_PROP, QVariant> XSLS::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSLS::initUnpack(UNPACK_STATE *pState,
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("WinSense compressed file; single LZHUF-compressed member"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XSLS::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SLS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("LZHUF (8K window, F=90)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XSLS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    pState->nCurrentOffset = pContext->nInputSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XSLS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
