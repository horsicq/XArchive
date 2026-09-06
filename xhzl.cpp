/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhzl.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 HZL_HEADER_SIZE = 12;
// Sanity ceiling; the size field is a signed 32-bit value in the container.
const qint64 HZL_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB

bool hzlIsExtensionCharacter(char cCharacter)
{
    const quint8 nCharacter = static_cast<quint8>(cCharacter);
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;
    if ((cCharacter == '_') || (cCharacter == '-') || (cCharacter == '~')) {
        return true;
    }
    // A DOS extension can be shorter than three characters; the field is then
    // padded with spaces or NULs.
    return (cCharacter == ' ') || (cCharacter == '\0');
}
}  // namespace

XHZL::XHZL(QIODevice *pDevice) : XArchive(pDevice)
{
}

XHZL::~XHZL()
{
}

bool XHZL::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XHZL> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <= HZL_HEADER_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, HZL_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHeader.size() != HZL_HEADER_SIZE)) {
        return false;
    }
    if (baHeader.left(4) != QByteArray("!HZL", 4)) return false;

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const qint32 nRawSize =
        static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 4));
    if ((nRawSize < 0) || (qint64(nRawSize) > HZL_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }
    // U3's detector demands the dot; without it there is nothing else in the
    // header that separates "!HZL" from a random four-byte match.
    if (baHeader.at(8) != '.') return false;
    for (qint32 i = 9; i < 12; ++i) {
        if (!hzlIsExtensionCharacter(baHeader.at(i))) return false;
    }

    HEADER entry = {};
    entry.nHeaderSize = HZL_HEADER_SIZE;
    entry.nCompressedSize = context.nInputSize - HZL_HEADER_SIZE;
    entry.nUncompressedSize = nRawSize;

    QString sExtension = QString::fromLatin1(baHeader.mid(8, 4));
    while (sExtension.endsWith(QLatin1Char(' ')) ||
           sExtension.endsWith(QChar(QChar::Null))) {
        sExtension.chop(1);
    }
    entry.sExtension = sExtension;

    QString sBaseName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (sBaseName.isEmpty()) sBaseName = QStringLiteral("hzl_data");
    entry.sFileName =
        (sExtension == QStringLiteral(".")) ? sBaseName
                                            : (sBaseName + sExtension);

    context.listEntries.append(entry);
    *pContext = context;
    return true;
}

bool XHZL::isValid(PDSTRUCT *pPdStruct)
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

bool XHZL::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHZL archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHZL::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHZL(pDevice);
}

QList<QString> XHZL::getSearchSignatures()
{
    return {QStringLiteral("'!HZL'")};
}

XBinary::FT XHZL::getFileType()
{
    return FT_HZL;
}

XBinary::MODE XHZL::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XHZL::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XHZL::getArch()
{
    return QString();
}

QString XHZL::getFileFormatExt()
{
    return QStringLiteral("hzl");
}

QString XHZL::getFileFormatExtsString()
{
    return QStringLiteral("HZL compressed file (*.??0)");
}

QString XHZL::getMIMEString()
{
    return QStringLiteral("application/x-hzl");
}

QString XHZL::getVersion()
{
    // The header carries no version field at all.
    return QStringLiteral("1");
}

qint64 XHZL::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XHZL::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XHZL::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART> XHZL::getFileParts(quint32 nFileParts, qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;
    if (context.listEntries.isEmpty()) return listResult;

    const HEADER &entry = context.listEntries.first();

    if (nFileParts & FILEPART_HEADER) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = entry.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = entry.nHeaderSize;
        part.nFileSize = entry.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = entry.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  entry.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  entry.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  (entry.nUncompressedSize == 0)
                                      ? HANDLE_METHOD_STORE
                                      : HANDLE_METHOD_HZL);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("LZHUF"));
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XHZL::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XHZL::initUnpack(UNPACK_STATE *pState,
                      const QMap<UNPACK_PROP, QVariant> &mapProperties,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XHZL> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listEntries.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO, tr("HZL single-file compressor; LZHUF stream"));
    pState->nCurrentOffset = pContext->listEntries.first().nHeaderSize;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XHZL::infoCurrent(UNPACK_STATE *pState,
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
    const HEADER &entry = pContext->listEntries.at(pState->nCurrentIndex);

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nHeaderSize;
    result.nStreamSize = entry.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                entry.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                entry.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                (entry.nUncompressedSize == 0)
                                    ? HANDLE_METHOD_STORE
                                    : HANDLE_METHOD_HZL);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("LZHUF"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No CRC and no timestamp exist anywhere in this container.
    return result;
}

bool XHZL::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listEntries.at(pState->nCurrentIndex).nHeaderSize;
    } else {
        pState->nCurrentOffset = pContext->nInputSize;
    }
    return (pState->nCurrentIndex < pContext->listEntries.size());
}

bool XHZL::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
