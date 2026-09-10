/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xigf1.h"

#include <QPointer>
#include <QTimeZone>
#include <QtEndian>

#include <new>

namespace {
const qint64 IGF1_HEADER_SIZE = 0x38;
const quint16 IGF1_MAGIC = 0xecdbU;
// The name is stored NUL terminated right behind the header and padded out to
// the data offset; MS-DOS/Win16 paths never come near this ceiling.
const qint32 IGF1_MAX_NAME_SIZE = 260;
const qint64 IGF1_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB sanity cap

bool igf1IsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((c == '"') || (c == '*') || (c == '<') || (c == '>') ||
            (c == '?') || (c == '|') || (c == ':')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XIGF1::XIGF1(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIGF1::~XIGF1()
{
}

bool XIGF1::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XIGF1> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (IGF1_HEADER_SIZE + 2)) return false;

    const QByteArray baHeader =
        read_array_process(0, IGF1_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHeader.size() != IGF1_HEADER_SIZE)) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromLittleEndian<quint16>(pHeader) != IGF1_MAGIC) return false;

    RECORD record = {};
    record.nVersion = qFromLittleEndian<quint16>(pHeader + 0x02);
    record.nGroup = qFromLittleEndian<quint16>(pHeader + 0x06);
    record.nFileId = qFromLittleEndian<quint32>(pHeader + 0x0c);
    record.nPackTime = qFromLittleEndian<quint32>(pHeader + 0x10);
    record.nFileTime = qFromLittleEndian<quint32>(pHeader + 0x14);
    record.nUncompressedSize = static_cast<qint64>(
        static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 0x1c)));
    record.nPlainChecksum = qFromLittleEndian<quint32>(pHeader + 0x20);
    record.nCompressedSize = static_cast<qint64>(
        static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 0x24)));
    record.nPackedChecksum = qFromLittleEndian<quint32>(pHeader + 0x28);
    const quint32 nDataOffset = qFromLittleEndian<quint32>(pHeader + 0x2c);
    const quint32 nComplement = qFromLittleEndian<quint32>(pHeader + 0x30);
    const qint64 nSizeAgain = static_cast<qint64>(
        static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + 0x34)));

    // The reference implementation: verbatim. The one's-complement word is the real
    // signature here - a two-byte magic on its own would be far too weak.
    if ((record.nUncompressedSize < 0) || (record.nCompressedSize < 0)) {
        return false;
    }
    if ((static_cast<qint32>(nDataOffset) <= 0) ||
        (nDataOffset != ~nComplement)) {
        return false;
    }
    if ((nSizeAgain <= 0) || (nSizeAgain != record.nCompressedSize)) {
        return false;
    }
    if (record.nUncompressedSize > IGF1_MAX_UNCOMPRESSED_SIZE) return false;

    record.nDataOffset = static_cast<qint64>(nDataOffset);
    if (record.nDataOffset < (IGF1_HEADER_SIZE + 1)) return false;
    if (record.nDataOffset > context.nInputSize) return false;
    if (record.nCompressedSize > (context.nInputSize - record.nDataOffset)) {
        return false;
    }

    // The member name sits between the header and the data block.
    const qint64 nNameRoom =
        qMin<qint64>(record.nDataOffset - IGF1_HEADER_SIZE,
                     IGF1_MAX_NAME_SIZE + 1);
    const QByteArray baNameArea =
        read_array_process(IGF1_HEADER_SIZE, nNameRoom, pPdStruct);
    if (!guardedThis || !guardedSource || (baNameArea.size() != nNameRoom)) {
        return false;
    }
    const int nTerminator = baNameArea.indexOf('\0');
    if (nTerminator <= 0) return false;
    const QByteArray baName = baNameArea.left(nTerminator);
    if (!igf1IsValidName(baName)) return false;
    record.sFileName =
        QString::fromLatin1(baName).replace(QLatin1Char('\\'), QLatin1Char('/'));

    context.nArchiveSize = record.nDataOffset + record.nCompressedSize;
    context.listRecords.append(record);

    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    *pContext = context;
    return true;
}

bool XIGF1::isValid(PDSTRUCT *pPdStruct)
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

bool XIGF1::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIGF1 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIGF1::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIGF1(pDevice);
}

XBinary::FT XIGF1::getFileType()
{
    return FT_IGF1;
}

XBinary::MODE XIGF1::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIGF1::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIGF1::getArch()
{
    return QString();
}

QString XIGF1::getFileFormatExt()
{
    return QStringLiteral("ex_");
}

QString XIGF1::getFileFormatExtsString()
{
    return QStringLiteral("IGF compressed file (*.ex_ *.dl_ *.hl_ *.ic_)");
}

QString XIGF1::getMIMEString()
{
    return QStringLiteral("application/x-igf");
}

QString XIGF1::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr) || context.listRecords.isEmpty()) {
        return QString();
    }
    // +0x02 is 0x0200 in every known file.
    const quint16 nVersion = context.listRecords.first().nVersion;
    return QString("%1.%2").arg(nVersion >> 8).arg(nVersion & 0xff);
}

qint64 XIGF1::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XIGF1::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIGF1::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XIGF1::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIGF1::getFileParts(quint32 nFileParts, qint32 nLimit,
                                          PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct) || context.listRecords.isEmpty()) {
        return result;
    }
    const RECORD &record = context.listRecords.first();

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = record.nDataOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = record.nDataOffset;
        part.nFileSize = record.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = record.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  record.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  record.nUncompressedSize);
        part.mapProperties.insert(
            FPART_PROP_HANDLEMETHOD,
            (record.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                            : HANDLE_METHOD_LZH4);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  QStringLiteral("LHA -lh4-"));
        if (record.nFileTime != 0) {
            part.mapProperties.insert(
                FPART_PROP_DATETIME,
                QDateTime::fromSecsSinceEpoch(
                    static_cast<qint64>(record.nFileTime), QTimeZone::utc()));
        }
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
    if ((nFileParts & FILEPART_OVERLAY) &&
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, result.size())) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XIGF1::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIGF1::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QPointer<XIGF1> guardedThis(this);
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
        pContext->listRecords.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("IGF installer file; single LHA -lh4- member"));
    pState->nCurrentOffset = pContext->listRecords.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listRecords.size();
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

XBinary::ARCHIVERECORD XIGF1::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex >= pContext->listRecords.size())) {
        return ARCHIVERECORD();
    }
    const RECORD &record = pContext->listRecords.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != record.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = record.nDataOffset;
    result.nStreamSize = record.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                record.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                record.nUncompressedSize);
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        (record.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                        : HANDLE_METHOD_LZH4);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("LHA -lh4-"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (record.nFileTime != 0) {
        result.mapProperties.insert(
            FPART_PROP_DATETIME,
            QDateTime::fromSecsSinceEpoch(
                static_cast<qint64>(record.nFileTime), QTimeZone::utc()));
    }
    // +0x20 and +0x28 are checksums whose algorithm is not known; the reference implementation ignores
    // them, so they are deliberately not published as a CRC property.
    return result;
}

bool XIGF1::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listRecords.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listRecords.size()) {
        pState->nCurrentOffset =
            pContext->listRecords.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listRecords.size());
}

bool XIGF1::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
