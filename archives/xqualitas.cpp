/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xqualitas.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 QUALITAS_HEADER_SIZE = 14;
const qint64 QUALITAS_RECORD_SIZE = 26;
// The reference detector demands nNumberOfFiles * 0x1C <= nDirectorySize, i.e.
// it budgets the 26 fixed bytes plus the shortest possible name ("x" + NUL) for
// every advertised file.
const qint64 QUALITAS_MIN_RECORD_TOTAL = 28;
const quint16 QUALITAS_HEADER_TAG = 0x000EU;
// The count field is 16 bit; nothing larger can exist.
const qint32 QUALITAS_MAX_FILES = 65535;
const qint64 QUALITAS_MAX_NAME = 255;
// Payload = 4-byte CRC word + at least one implode byte.
const qint64 QUALITAS_MIN_STREAM = 5;
// Disk 1 is "the data is in this file"; the walk stops at anything else.
const quint8 QUALITAS_THIS_DISK = 1;

bool qualitasRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XQualitas::XQualitas(QIODevice *pDevice) : XArchive(pDevice)
{
}

XQualitas::~XQualitas()
{
}

// Names are plain DOS 8.3 identifiers in the whole reference corpus, but the
// field is raw bytes, so path separators and the Windows reserved punctuation
// are escaped as %XX rather than folded to '_': escaping is reversible and
// cannot collapse two distinct members onto one output file.
QString XQualitas::rawNameToString(const QByteArray &baRawName, qint32 nIndex)
{
    QString sResult;
    for (qint32 i = 0; i < baRawName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRawName.at(i));
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("record%1").arg(nIndex);
    }
    return sResult;
}

bool XQualitas::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < QUALITAS_HEADER_SIZE + QUALITAS_MIN_RECORD_TOTAL + QUALITAS_MIN_STREAM) return false;

    const QByteArray baHeader = read_array_process(0, QUALITAS_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != QUALITAS_HEADER_SIZE) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    // There is no magic.  This constant 0x000E header-size word plus the
    // directory arithmetic below is the entire gate, so every one of these
    // checks is load bearing.
    if (qFromLittleEndian<quint16>(pHeader + 4) != QUALITAS_HEADER_TAG) return false;

    context.nDirectorySize = static_cast<qint64>(qFromLittleEndian<quint16>(pHeader + 6));
    if (context.nDirectorySize == 0) return false;

    context.nNumberOfFiles = static_cast<qint32>(qFromLittleEndian<quint16>(pHeader + 10));
    if ((context.nNumberOfFiles < 1) || (context.nNumberOfFiles > QUALITAS_MAX_FILES)) return false;
    if (static_cast<qint64>(context.nNumberOfFiles) * QUALITAS_MIN_RECORD_TOTAL > context.nDirectorySize) return false;

    const qint64 nDirectoryEnd = QUALITAS_HEADER_SIZE + context.nDirectorySize;
    if (nDirectoryEnd > context.nInputSize) return false;

    const QByteArray baDirectory = read_array_process(QUALITAS_HEADER_SIZE, context.nDirectorySize, pPdStruct);
    if (baDirectory.size() != context.nDirectorySize) return false;

    qint64 nPosition = 0;
    for (qint32 i = 0; i < context.nNumberOfFiles; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nPosition + QUALITAS_RECORD_SIZE > context.nDirectorySize) break;

        const uchar *pRecord = reinterpret_cast<const uchar *>(baDirectory.constData()) + nPosition;
        const qint64 nNextRecord = static_cast<qint64>(qFromLittleEndian<qint32>(pRecord + 0));
        const qint64 nDataOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pRecord + 6));
        const quint16 nDosTime = qFromLittleEndian<quint16>(pRecord + 10);
        const quint16 nDosDate = qFromLittleEndian<quint16>(pRecord + 12);
        const qint64 nUncompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRecord + 14));
        const qint64 nCompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pRecord + 18));
        const quint16 nAttributes = qFromLittleEndian<quint16>(pRecord + 22);
        const quint8 nDiskNumber = pRecord[24];
        const quint8 nMethod = pRecord[25];

        // Both of these END the walk instead of failing the parse: a disk-1
        // image legitimately carries directory records for the members that
        // live on the later disks, and the reference extractor stops at the
        // first one exactly like this.
        if ((nDiskNumber != QUALITAS_THIS_DISK) || (nMethod > 1)) break;
        if ((nNextRecord < 0) || (nDataOffset < 0) || (nUncompressedSize < 0) || (nCompressedSize < QUALITAS_MIN_STREAM)) break;

        qint64 nNameStart = nPosition + QUALITAS_RECORD_SIZE;
        qint64 nNameEnd = nNameStart;
        while ((nNameEnd < context.nDirectorySize) && (baDirectory.at(static_cast<qint32>(nNameEnd)) != '\0')) nNameEnd++;
        if (nNameEnd >= context.nDirectorySize) break;  // unterminated: the directory is cut short
        const qint64 nNameLength = nNameEnd - nNameStart;
        if ((nNameLength == 0) || (nNameLength > QUALITAS_MAX_NAME)) break;

        nPosition = nNameEnd + 1;
        // The record carries the absolute offset of its successor; a directory
        // that does not chain to itself is not this format.
        if (nNextRecord != QUALITAS_HEADER_SIZE + nPosition) return false;

        if (!qualitasRangeWithin(context.nInputSize, nDataOffset, nCompressedSize)) break;
        if (nDataOffset < nDirectoryEnd) return false;

        MEMBER member = {};
        member.nRecordOffset = QUALITAS_HEADER_SIZE + (nNameStart - QUALITAS_RECORD_SIZE);
        member.nDataOffset = nDataOffset;
        member.nStreamOffset = nDataOffset + 4;
        member.nCompressedSize = nCompressedSize - 4;
        member.nUncompressedSize = nUncompressedSize;
        member.nDosTime = nDosTime;
        member.nDosDate = nDosDate;
        member.nAttributes = nAttributes;
        member.nMethod = nMethod;
        member.sFileName = rawNameToString(baDirectory.mid(static_cast<qint32>(nNameStart), static_cast<qint32>(nNameLength)), i);
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    // The first member must start right behind the directory: this is the
    // structural check the reference detector makes on a headerless container,
    // and it is what stops arbitrary data from parsing as an archive.
    if (context.listMembers.first().nDataOffset != nDirectoryEnd) return false;

    qint64 nArchiveEnd = nDirectoryEnd;
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        const MEMBER &member = context.listMembers.at(i);
        nArchiveEnd = qMax(nArchiveEnd, member.nDataOffset + 4 + member.nCompressedSize);
    }
    context.nArchiveSize = qMin(nArchiveEnd, context.nInputSize);

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XQualitas::isValid(PDSTRUCT *pPdStruct)
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

bool XQualitas::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQualitas archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XQualitas::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQualitas(pDevice);
}

QList<QString> XQualitas::getSearchSignatures()
{
    // Headerless: the only fixed bytes are the 0x000E header-size word at
    // offset 4, which is far too weak to publish as a scan signature.  All of
    // the detection lives in parseContext().
    return QList<QString>();
}

XBinary::FT XQualitas::getFileType()
{
    return FT_QUALITAS;
}

XBinary::MODE XQualitas::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XQualitas::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XQualitas::getArch()
{
    return QString();
}

QString XQualitas::getFileFormatExt()
{
    // The extension is the disk number, so there is no single canonical one.
    return QStringLiteral("1");
}

QString XQualitas::getFileFormatExtsString()
{
    return QStringLiteral("Qualitas install disk (*.1 *.2 *.3)");
}

QString XQualitas::getMIMEString()
{
    return QStringLiteral("application/x-qualitas");
}

QString XQualitas::getVersion()
{
    return QString();
}

qint64 XQualitas::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XQualitas::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XQualitas::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XQualitas::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XQualitas::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = QUALITAS_HEADER_SIZE + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nStreamOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
            const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
            if (dtMTime.isValid()) {
                part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            }
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize + 4;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }
    return listResult;
}

QList<XBinary::FPART_PROP> XQualitas::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_DATETIME, FPART_PROP_STREAMOFFSET, FPART_PROP_STREAMSIZE};
}

QMap<XBinary::UNPACK_PROP, QVariant> XQualitas::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XQualitas::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Qualitas install disk"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XQualitas::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nStreamOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    // Both method bytes seen in the corpus (0 and 1) carry a PKWARE DCL implode
    // stream; the byte is not a codec selector, and the reference extractor
    // never branches on it either.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PKWARE DCL implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XQualitas::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XQualitas::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
