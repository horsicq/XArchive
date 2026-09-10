/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpmdiskcopy.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const char PMD_MAGIC[11] = {'P', 'M', ' ', 'D', 'i', 's', 'k', 'c', 'o', 'p', 'y'};
const qint64 PMD_MAGIC_SIZE = 11;
const qint64 PMD_HEADER_SIZE = 47;
const qint32 PMD_BPB_OFFSET = 0x0b;

bool pmdIsSectorSizeValid(quint32 nBytesPerSector)
{
    return (nBytesPerSector == 512) || (nBytesPerSector == 1024) ||
           (nBytesPerSector == 2048) || (nBytesPerSector == 4096);
}

// The reference implementation tests bit (nValue + 0x20) of the bitmap at 0x425ba0, which accepts
// 1..16, 32, 64 and 128 and nothing else.
bool pmdIsSectorsPerClusterValid(quint32 nSectorsPerCluster)
{
    if ((nSectorsPerCluster >= 1) && (nSectorsPerCluster <= 16)) return true;
    return (nSectorsPerCluster == 32) || (nSectorsPerCluster == 64) ||
           (nSectorsPerCluster == 128);
}
}  // namespace

XPMDiskcopy::XPMDiskcopy(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPMDiskcopy::~XPMDiskcopy()
{
}

QString XPMDiskcopy::memberName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    QString sBaseName;
    if (guardedSource) {
        sBaseName = XBinary::fixFileName(
            XBinary::getDeviceFileBaseName(guardedSource.data()));
    }
    if (sBaseName.isEmpty()) sBaseName = QStringLiteral("disk-image");
    return sBaseName + QStringLiteral(".img");
}

bool XPMDiskcopy::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPMDiskcopy> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <= PMD_HEADER_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, PMD_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHeader.size() != PMD_HEADER_SIZE)) {
        return false;
    }
    if (std::memcmp(baHeader.constData(), PMD_MAGIC, PMD_MAGIC_SIZE) != 0) {
        return false;
    }

    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    const quint32 nBytesPerSector =
        qFromLittleEndian<quint16>(pHeader + PMD_BPB_OFFSET + 0);
    const quint32 nSectorsPerCluster = pHeader[PMD_BPB_OFFSET + 2];
    const quint32 nNumberOfFATs = pHeader[PMD_BPB_OFFSET + 5];
    const quint32 nMediaDescriptor = pHeader[PMD_BPB_OFFSET + 10];

    if (!pmdIsSectorSizeValid(nBytesPerSector)) return false;
    if ((nMediaDescriptor & 0xf0U) != 0xf0U) return false;
    if (!pmdIsSectorsPerClusterValid(nSectorsPerCluster)) return false;
    if ((nNumberOfFATs != 1) && (nNumberOfFATs != 2)) return false;

    context.nDataOffset = PMD_HEADER_SIZE;
    context.nDataSize = context.nInputSize - PMD_HEADER_SIZE;
    // The image is truncated after the last used sector, but never inside one.
    if ((context.nDataSize % static_cast<qint64>(nBytesPerSector)) != 0) {
        return false;
    }
    context.nBytesPerSector = nBytesPerSector;
    context.nSectorsPerCluster = nSectorsPerCluster;
    context.nNumberOfFATs = nNumberOfFATs;
    context.nMediaDescriptor = nMediaDescriptor;
    context.sFileName = memberName();
    if (!guardedThis || !guardedSource) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XPMDiskcopy::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XPMDiskcopy::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPMDiskcopy archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPMDiskcopy::createInstance(QIODevice *pDevice, bool bIsImage,
                                     XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPMDiskcopy(pDevice);
}

QList<QString> XPMDiskcopy::getSearchSignatures()
{
    return {QStringLiteral("'PM Diskcopy'")};
}

XBinary::FT XPMDiskcopy::getFileType()
{
    return FT_PM_DISKCOPY;
}

XBinary::MODE XPMDiskcopy::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPMDiskcopy::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPMDiskcopy::getArch()
{
    return QString();
}

QString XPMDiskcopy::getFileFormatExt()
{
    return QStringLiteral("img");
}

QString XPMDiskcopy::getFileFormatExtsString()
{
    return QStringLiteral("PM Diskcopy floppy image (*.img)");
}

QString XPMDiskcopy::getMIMEString()
{
    return QStringLiteral("application/x-pm-diskcopy");
}

QString XPMDiskcopy::getVersion()
{
    return QString();
}

qint64 XPMDiskcopy::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XPMDiskcopy::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPMDiskcopy::getMemoryMap(MAPMODE mapMode,
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

bool XPMDiskcopy::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XPMDiskcopy::getFileParts(quint32 nFileParts,
                                                qint32 nLimit,
                                                PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PMD_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nDataSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nDataSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nDataSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("Stored"));
        result.append(part);
    }
    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nDataSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Disk image");
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

QMap<XBinary::UNPACK_PROP, QVariant> XPMDiskcopy::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPMDiskcopy::initUnpack(UNPACK_STATE *pState,
                             const QMap<UNPACK_PROP, QVariant> &mapProperties,
                             PDSTRUCT *pPdStruct)
{
    QPointer<XPMDiskcopy> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("PM Diskcopy floppy image; 47-byte header plus raw sectors"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XPMDiskcopy::infoCurrent(UNPACK_STATE *pState,
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
    result.nStreamSize = pContext->nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XPMDiskcopy::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XPMDiskcopy::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
