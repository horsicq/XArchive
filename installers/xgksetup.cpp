/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xgksetup.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const char GKSETUP_BANNER[] = "This is a binary data file. Keep out !\x1a";
const qint64 GKSETUP_BANNER_SIZE = 39;  // 38 text bytes + the 0x1a terminator
const qint64 GKSETUP_HEADER_SIZE = 0x600;
const qint64 GKSETUP_SIGNATURE_OFFSET = 0x50;
const qint64 GKSETUP_DATAOFFSET_OFFSET = 0x128;
const qint64 GKSETUP_RECORD_SIZE = 0x124;
const qint64 GKSETUP_NAME_FIELD_SIZE = 0x104;
const qint64 GKSETUP_ATTRIBUTES_OFFSET = 0x104;
const qint64 GKSETUP_SIZE_OFFSET = 0x120;
const quint32 GKSETUP_ATTRIBUTE_DIRECTORY = 0x10;
// Two chain layouts exist and the value is the offset of the first record.
const qint64 GKSETUP_DATAOFFSET_A = 0x600;
const qint64 GKSETUP_DATAOFFSET_B = 0x700;
// The chain has no count field, so the walk is bounded only by the file.  A
// record costs 0x124 bytes, so this ceiling is far above any real producer.
const qint32 GKSETUP_MAX_RECORDS = 1000000;

bool gksetupRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XGkSetup::XGkSetup(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGkSetup::~XGkSetup()
{
}

// Names are ANSI 8.3 or long Windows names and may carry '\' path segments.
// The separator is normalized to '/', everything the host filesystem cannot
// represent is escaped as %XX: escaping is reversible and, unlike folding to
// '_', cannot collapse two distinct members onto one output file.
QString XGkSetup::sanitizeName(const QString &sRaw)
{
    QString sResult;
    for (qint32 i = 0; i < sRaw.size(); i++) {
        const quint16 nCharacter = sRaw.at(i).unicode();
        if ((nCharacter == '\\') || (nCharacter == '/')) {
            sResult.append(QLatin1Char('/'));
            continue;
        }
        const bool bSafe = (nCharacter >= 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != ':') && (nCharacter != '*') &&
                           (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QChar(nCharacter));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            while (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    return sResult;
}

bool XGkSetup::readRecord(qint64 nOffset, QString *pName, quint32 *pAttributes, qint64 *pSize, PDSTRUCT *pPdStruct)
{
    QPointer<XGkSetup> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pName || !pAttributes || !pSize || !guardedSource) return false;

    const QByteArray baRecord = read_array_process(nOffset, GKSETUP_RECORD_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baRecord.size() != GKSETUP_RECORD_SIZE)) return false;
    const char *pRecord = baRecord.constData();

    qint64 nNameLength = 0;
    // The reference extractor forces a NUL into the last byte of the field, so
    // the usable name is at most GKSETUP_NAME_FIELD_SIZE - 1 characters.
    while ((nNameLength < GKSETUP_NAME_FIELD_SIZE - 1) && (pRecord[nNameLength] != '\0')) nNameLength++;

    QString sName;
    for (qint64 i = 0; i < nNameLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(pRecord[i]);
        // Control bytes never occur in a real descriptor and are the cheapest
        // structural rule that stops payload bytes from parsing as a record.
        if (nCharacter < 0x20) return false;
        sName.append(QChar(static_cast<ushort>(nCharacter)));
    }

    *pName = sName;
    *pAttributes = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pRecord) + GKSETUP_ATTRIBUTES_OFFSET);
    *pSize = static_cast<qint64>(qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(pRecord) + GKSETUP_SIZE_OFFSET));
    return (*pSize >= 0);
}

// Nothing announces the extra all-zero quint32 between a descriptor and its
// payload.  The reference extractor decides it by walking the first two records
// under the padded interpretation and requiring both padding words to be zero
// and both extents to fit; that is reproduced verbatim here.
bool XGkSetup::probePadding(qint64 nOffset, qint64 nInputSize, bool *pbHasPadding, PDSTRUCT *pPdStruct)
{
    QPointer<XGkSetup> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pbHasPadding || !guardedSource) return false;

    *pbHasPadding = false;
    qint64 nCurrent = nOffset;
    for (qint32 i = 0; i < 2; i++) {
        QString sName;
        quint32 nAttributes = 0;
        qint64 nSize = 0;
        if (!readRecord(nCurrent, &sName, &nAttributes, &nSize, pPdStruct) || !guardedThis || !guardedSource) return true;
        const qint64 nAfterRecord = nCurrent + GKSETUP_RECORD_SIZE;
        if (!gksetupRangeWithin(nInputSize, nAfterRecord, nSize)) return true;
        if (nAfterRecord + 4 > nInputSize) return true;

        const QByteArray baPadding = read_array_process(nAfterRecord, 4, pPdStruct);
        if (!guardedThis || !guardedSource || (baPadding.size() != 4)) return true;
        if (qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baPadding.constData())) != 0) return true;

        nCurrent = nAfterRecord + 4 + nSize;
    }

    *pbHasPadding = true;
    return true;
}

bool XGkSetup::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XGkSetup> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < GKSETUP_HEADER_SIZE + GKSETUP_RECORD_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, GKSETUP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != GKSETUP_HEADER_SIZE)) return false;
    const char *pHeader = baHeader.constData();

    if (memcmp(pHeader, GKSETUP_BANNER, static_cast<size_t>(GKSETUP_BANNER_SIZE)) != 0) return false;
    if ((pHeader[GKSETUP_SIGNATURE_OFFSET] != 'G') || (pHeader[GKSETUP_SIGNATURE_OFFSET + 1] != 'K')) return false;

    context.nDataOffset = static_cast<qint64>(qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pHeader) + GKSETUP_DATAOFFSET_OFFSET));
    // The two known chain origins.  UNINSTAL.DAT carries the same banner and
    // the same "GK" signature but a completely different body, and this is the
    // field that tells the two apart - the reference extractor refuses
    // everything else here too.
    if ((context.nDataOffset != GKSETUP_DATAOFFSET_A) && (context.nDataOffset != GKSETUP_DATAOFFSET_B)) return false;
    if (context.nDataOffset + GKSETUP_RECORD_SIZE > context.nInputSize) return false;

    if (!probePadding(context.nDataOffset, context.nInputSize, &context.bHasPadding, pPdStruct) || !guardedThis || !guardedSource) return false;
    const qint64 nPaddingSize = context.bHasPadding ? 4 : 0;

    QString sCurrentPath;
    qint64 nCurrent = context.nDataOffset;
    qint32 nRecordCount = 0;
    while (nCurrent + GKSETUP_RECORD_SIZE <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nRecordCount >= GKSETUP_MAX_RECORDS) return false;

        QString sName;
        quint32 nAttributes = 0;
        qint64 nSize = 0;
        if (!readRecord(nCurrent, &sName, &nAttributes, &nSize, pPdStruct) || !guardedThis || !guardedSource) break;
        nRecordCount++;
        qint64 nPayload = nCurrent + GKSETUP_RECORD_SIZE;
        if (nPaddingSize) {
            if (nPayload + 4 > context.nInputSize) break;
            const QByteArray baPadding = read_array_process(nPayload, 4, pPdStruct);
            if (!guardedThis || !guardedSource || (baPadding.size() != 4)) break;
            if (qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baPadding.constData())) != 0) break;
            nPayload += 4;
        }

        if (sName == QLatin1String("..")) {
            // Pop one level; the record carries no payload of its own.
            qint32 nSeparator = sCurrentPath.lastIndexOf(QLatin1Char('/'), sCurrentPath.endsWith(QLatin1Char('/')) ? -2 : -1);
            sCurrentPath = (nSeparator >= 0) ? sCurrentPath.left(nSeparator + 1) : QString();
            nCurrent = nPayload;
            continue;
        }

        const QString sSanitized = sanitizeName(sName);
        if (sSanitized.isEmpty()) break;

        if (nAttributes & GKSETUP_ATTRIBUTE_DIRECTORY) {
            if (nSize != 0) break;
            sCurrentPath = sCurrentPath + sSanitized + QLatin1Char('/');
            nCurrent = nPayload;
            continue;
        }

        if (!gksetupRangeWithin(context.nInputSize, nPayload, nSize)) break;

        MEMBER member = {};
        member.nDataOffset = nPayload;
        member.nSize = nSize;
        member.nAttributes = nAttributes;
        member.sFileName = sCurrentPath + sSanitized;
        context.listMembers.append(member);

        nCurrent = nPayload + nSize;
        context.nArchiveSize = nCurrent;
    }

    if (context.listMembers.isEmpty()) return false;
    if (context.nArchiveSize <= 0) context.nArchiveSize = context.nInputSize;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XGkSetup::isValid(PDSTRUCT *pPdStruct)
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

bool XGkSetup::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGkSetup archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGkSetup::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGkSetup(pDevice);
}

QList<QString> XGkSetup::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'This is a binary data file. Keep out !'1A"));
    return listResult;
}

XBinary::FT XGkSetup::getFileType()
{
    return FT_GKSETUP;
}

XBinary::MODE XGkSetup::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGkSetup::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGkSetup::getArch()
{
    return QString();
}

QString XGkSetup::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XGkSetup::getFileFormatExtsString()
{
    return QStringLiteral("GkSetup data (*.dat *.da_)");
}

QString XGkSetup::getMIMEString()
{
    return QStringLiteral("application/x-gksetup");
}

QString XGkSetup::getVersion()
{
    return QString();
}

qint64 XGkSetup::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGkSetup::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XGkSetup::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XGkSetup::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XGkSetup::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDataOffset;
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
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XGkSetup::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGkSetup::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XGkSetup> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("GkSetup data"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XGkSetup::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XGkSetup::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XGkSetup::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
