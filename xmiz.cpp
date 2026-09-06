/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xmiz.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// "DKCL" + u16 version + "SBRW" + u16 nameLen == 12 bytes, no slack.
const qint64 MIZ_MAGIC_SIZE = 12;
// dosTime + dosDate + i32 uncompressedSize + i32 compressedSize == 12 bytes.
const qint64 MIZ_RECORD_SIZE = 12;
const qint64 MIZ_FOOTER_SIZE = 4;
const quint16 MIZ_VERSION = 1;
// The writer only ever emits 8.3 names, so the field is at most "12345678.123"
// plus its NUL.  A generous ceiling is kept, but the field must still contain a
// terminator: the length is a FIELD length, not a string length.
const qint32 MIZ_MAX_NAME_FIELD = 64;
const qint64 MIZ_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB sanity cap

bool mizRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// The name field is nameLen bytes wide and holds a NUL-terminated 8.3 name.
// Read the whole FIELD; never scan one byte short.
bool mizSplitName(const QByteArray &baField, QString *pName)
{
    if (baField.isEmpty()) return false;

    const qint32 nTerminator = baField.indexOf('\0');
    if (nTerminator <= 0) return false;  // empty name, or no terminator at all

    for (qint32 i = 0; i < nTerminator; i++) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
    }
    // Everything past the terminator must be padding, otherwise this is not a
    // MIZ name field and the whole match is bogus.
    for (qint32 i = nTerminator; i < baField.size(); i++) {
        if (baField.at(i) != '\0') return false;
    }

    *pName = QString::fromLatin1(baField.constData(), nTerminator);

    return true;
}
}  // namespace

XMiz::XMiz(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMiz::~XMiz()
{
}

bool XMiz::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XMiz> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();

    // Smallest conceivable container: magic + a two-byte name field + record +
    // a bare DCL stream + footer.
    if (context.nInputSize < MIZ_MAGIC_SIZE + 2 + MIZ_RECORD_SIZE + MIZ_FOOTER_SIZE) return false;

    const QByteArray baMagic = read_array_process(0, MIZ_MAGIC_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baMagic.size() != MIZ_MAGIC_SIZE)) return false;

    const uchar *pMagic = reinterpret_cast<const uchar *>(baMagic.constData());
    if (std::memcmp(pMagic, "DKCL", 4) != 0) return false;
    if (std::memcmp(pMagic + 6, "SBRW", 4) != 0) return false;

    context.nVersion = qFromLittleEndian<quint16>(pMagic + 4);
    if (context.nVersion != MIZ_VERSION) return false;

    const qint32 nNameFieldSize = static_cast<qint32>(qFromLittleEndian<quint16>(pMagic + 10));
    if ((nNameFieldSize < 2) || (nNameFieldSize > MIZ_MAX_NAME_FIELD)) return false;

    if (!mizRangeWithin(context.nInputSize, MIZ_MAGIC_SIZE, nNameFieldSize)) return false;

    const QByteArray baName = read_array_process(MIZ_MAGIC_SIZE, nNameFieldSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baName.size() != nNameFieldSize)) return false;
    if (!mizSplitName(baName, &context.sFileName)) return false;

    const qint64 nRecordOffset = MIZ_MAGIC_SIZE + nNameFieldSize;
    if (!mizRangeWithin(context.nInputSize, nRecordOffset, MIZ_RECORD_SIZE)) return false;

    const QByteArray baRecord = read_array_process(nRecordOffset, MIZ_RECORD_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baRecord.size() != MIZ_RECORD_SIZE)) return false;

    const uchar *pRecord = reinterpret_cast<const uchar *>(baRecord.constData());
    const quint16 nDosTime = qFromLittleEndian<quint16>(pRecord + 0);
    const quint16 nDosDate = qFromLittleEndian<quint16>(pRecord + 2);
    const qint32 nUncompressedSize = static_cast<qint32>(qFromLittleEndian<quint32>(pRecord + 4));
    const qint32 nCompressedSize = static_cast<qint32>(qFromLittleEndian<quint32>(pRecord + 8));

    // U3 tests both size fields for a clear sign bit and refuses the file
    // otherwise; the sanity cap below is the same rule with a tighter bound.
    if ((nUncompressedSize < 0) || (nCompressedSize < 0)) return false;
    if ((nUncompressedSize > MIZ_MAX_UNCOMPRESSED_SIZE) || (nCompressedSize > MIZ_MAX_UNCOMPRESSED_SIZE)) return false;

    context.nHeaderSize = nRecordOffset + MIZ_RECORD_SIZE;
    context.nDataOffset = context.nHeaderSize;
    context.nCompressedSize = nCompressedSize;
    context.nUncompressedSize = nUncompressedSize;

    if (!mizRangeWithin(context.nInputSize, context.nDataOffset, context.nCompressedSize)) return false;

    context.nFooterOffset = context.nDataOffset + context.nCompressedSize;
    if (!mizRangeWithin(context.nInputSize, context.nFooterOffset, MIZ_FOOTER_SIZE)) return false;

    // "MJDK" closes every container; U3 fails the extraction when it is absent,
    // so it is treated here as part of the format gate.
    const QByteArray baFooter = read_array_process(context.nFooterOffset, MIZ_FOOTER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baFooter.size() != MIZ_FOOTER_SIZE)) return false;
    if (std::memcmp(baFooter.constData(), "MJDK", 4) != 0) return false;

    // The payload is a raw PKWARE DCL stream whose first two bytes are the
    // literal mode (0 or 1) and the dictionary exponent (4, 5 or 6).  Checking
    // them keeps a container with a foreign payload from reaching the decoder.
    if (context.nCompressedSize >= 2) {
        const QByteArray baPreamble = read_array_process(context.nDataOffset, 2, pPdStruct);
        if (!guardedThis || !guardedSource || (baPreamble.size() != 2)) return false;
        const quint8 nLiteralMode = static_cast<quint8>(baPreamble.at(0));
        const quint8 nDictExponent = static_cast<quint8>(baPreamble.at(1));
        if (nLiteralMode > 1) return false;
        if ((nDictExponent < 4) || (nDictExponent > 6)) return false;
    } else if (context.nUncompressedSize > 0) {
        // A non-empty member cannot fit in fewer than the two preamble bytes.
        return false;
    }

    const QDateTime dtModified = dosDateTimeToQDateTime(nDosDate, nDosTime);
    if (dtModified.isValid()) context.dtModified = dtModified;

    context.nArchiveSize = context.nFooterOffset + MIZ_FOOTER_SIZE;

    *pContext = context;

    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XMiz::isValid(PDSTRUCT *pPdStruct)
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

bool XMiz::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMiz archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMiz::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMiz(pDevice);
}

QList<QString> XMiz::getSearchSignatures()
{
    // The two magics plus the version word are contiguous; matching only "DKCL"
    // would be far too short.
    return {QStringLiteral("'DKCL'0100'SBRW'")};
}

XBinary::FT XMiz::getFileType()
{
    return FT_MIZ;
}

XBinary::MODE XMiz::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMiz::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMiz::getArch()
{
    return QString();
}

QString XMiz::getFileFormatExt()
{
    return QStringLiteral("miz");
}

QString XMiz::getFileFormatExtsString()
{
    return QStringLiteral("MIZ compressed file (*.miz)");
}

QString XMiz::getMIMEString()
{
    return QStringLiteral("application/x-miz");
}

QString XMiz::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XMiz::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XMiz::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XMiz::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_FOOTER | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XMiz::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XMiz::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return result;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
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
        part.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
        if (context.dtModified.isValid()) {
            part.mapProperties.insert(FPART_PROP_DATETIME, context.dtModified);
            part.mapProperties.insert(FPART_PROP_MTIME, context.dtModified);
        }
        result.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        result.append(part);
    }

    if ((nFileParts & FILEPART_FOOTER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_FOOTER;
        part.nFileOffset = context.nFooterOffset;
        part.nFileSize = MIZ_FOOTER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Footer");
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

QMap<XBinary::UNPACK_PROP, QVariant> XMiz::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMiz::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XMiz> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("MIZ container; a single PKWARE DCL imploded file"));
    pState->nCurrentOffset = pContext->nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XMiz::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (pContext->dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, pContext->dtModified);
        result.mapProperties.insert(FPART_PROP_MTIME, pContext->dtModified);
    }

    return result;
}

bool XMiz::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XMiz::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
