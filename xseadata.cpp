/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xseadata.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const quint32 SEADATA_MAGIC = 0x12213443U;
const quint32 SEADATA_RECORD_TAG = 0x23324554U;  // "TE2#"
const qint64 SEADATA_MAGIC_SIZE = 4;
// tag + nNextOffset + nNameLength + the name's NUL terminator
const qint64 SEADATA_RECORD_OVERHEAD = 13;
const qint64 SEADATA_RECORD_FIXED = 12;
const qint32 SEADATA_MAX_NAME = 255;
const qint32 SEADATA_MAX_MEMBERS = 1000000;

bool seaDataRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XSeaData::XSeaData(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSeaData::~XSeaData()
{
}

// Names are plain 8.3-ish asset names in the whole reference corpus, but the
// field is raw bytes of a producer-chosen length, so anything the host
// filesystem would reject is escaped as %XX rather than folded to '_':
// escaping is reversible and cannot collapse two members onto one output file.
QString XSeaData::rawNameToString(const QByteArray &baRawName, qint32 nIndex)
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

bool XSeaData::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSeaData> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SEADATA_MAGIC_SIZE + SEADATA_RECORD_OVERHEAD) return false;

    const QByteArray baMagic = read_array_process(0, SEADATA_MAGIC_SIZE + 4, pPdStruct);
    if (!guardedThis || !guardedSource || (baMagic.size() != SEADATA_MAGIC_SIZE + 4)) return false;
    const uchar *pMagic = reinterpret_cast<const uchar *>(baMagic.constData());
    if (qFromLittleEndian<quint32>(pMagic) != SEADATA_MAGIC) return false;
    // The first record must follow the magic immediately.
    if (qFromLittleEndian<quint32>(pMagic + 4) != SEADATA_RECORD_TAG) return false;

    qint64 nOffset = SEADATA_MAGIC_SIZE;
    qint64 nPreviousEnd = SEADATA_MAGIC_SIZE;
    qint32 nIndex = 0;
    while (nOffset < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nIndex >= SEADATA_MAX_MEMBERS) return false;
        if (nOffset + 4 > context.nInputSize) return false;

        const QByteArray baTag = read_array_process(nOffset, 4, pPdStruct);
        if (!guardedThis || !guardedSource || (baTag.size() != 4)) return false;
        const quint32 nTag = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTag.constData()));
        // A zero word closes the chain; the reference reader stops there and
        // treats whatever follows as padding.
        if (nTag == 0) break;
        if (nTag != SEADATA_RECORD_TAG) return false;

        if (nOffset + SEADATA_RECORD_FIXED > context.nInputSize) return false;
        const QByteArray baFixed = read_array_process(nOffset + 4, 8, pPdStruct);
        if (!guardedThis || !guardedSource || (baFixed.size() != 8)) return false;
        const uchar *pFixed = reinterpret_cast<const uchar *>(baFixed.constData());
        const qint64 nNextOffset = static_cast<qint64>(qFromLittleEndian<qint32>(pFixed));
        const qint32 nNameLength = qFromLittleEndian<qint32>(pFixed + 4);

        if ((nNameLength < 1) || (nNameLength > SEADATA_MAX_NAME)) return false;
        if ((nNextOffset < nPreviousEnd) || (nNextOffset > context.nInputSize)) return false;
        if (nOffset + SEADATA_RECORD_FIXED + nNameLength + 1 > context.nInputSize) return false;

        const QByteArray baName = read_array_process(nOffset + SEADATA_RECORD_FIXED, nNameLength + 1, pPdStruct);
        if (!guardedThis || !guardedSource || (baName.size() != nNameLength + 1)) return false;
        if (baName.at(nNameLength) != '\0') return false;

        const qint64 nDataOffset = nOffset + SEADATA_RECORD_FIXED + nNameLength + 1;
        const qint64 nSize = (nNextOffset - nPreviousEnd) - nNameLength - SEADATA_RECORD_OVERHEAD;
        if (nSize < 0) return false;
        if (!seaDataRangeWithin(context.nInputSize, nDataOffset, nSize)) return false;
        // The record's own end must land exactly on the announced next offset;
        // that is the self-check the container is built on.
        if (nDataOffset + nSize != nNextOffset) return false;

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nSize = nSize;
        member.sFileName = rawNameToString(baName.left(nNameLength), nIndex);
        context.listMembers.append(member);

        nPreviousEnd = nNextOffset;
        nOffset = nNextOffset;
        nIndex++;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSeaData::isValid(PDSTRUCT *pPdStruct)
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

bool XSeaData::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSeaData archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSeaData::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSeaData(pDevice);
}

QList<QString> XSeaData::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("43342112'TE2#'"));
    return listResult;
}

XBinary::FT XSeaData::getFileType()
{
    return FT_SEA_DATA;
}

XBinary::MODE XSeaData::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSeaData::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSeaData::getArch()
{
    return QString();
}

QString XSeaData::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XSeaData::getFileFormatExtsString()
{
    return QStringLiteral("Sea Data asset bundle (*.dat)");
}

QString XSeaData::getMIMEString()
{
    return QStringLiteral("application/x-sea-data");
}

QString XSeaData::getVersion()
{
    return QString();
}

qint64 XSeaData::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSeaData::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSeaData::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSeaData::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSeaData::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SEADATA_MAGIC_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = member.nDataOffset - member.nRecordOffset;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XSeaData::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSeaData::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSeaData> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Sea Data asset bundle"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
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

XBinary::ARCHIVERECORD XSeaData::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSeaData::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->nInputSize;
    }

    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSeaData::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
