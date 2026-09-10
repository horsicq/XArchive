/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xkrml.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 KRML_HEADER_SIZE = 6;
const qint64 KRML_RECORD_SIZE = 21;
const qint32 KRML_NAME_SIZE = 13;
const qint32 KRML_MAX_MEMBERS = 65535;

bool krmlRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

bool krmlIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = quint8(baName.at(i));
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        const char c = baName.at(i);
        if ((c == '"') || (c == '*') || (c == '<') || (c == '>') ||
            (c == '?') || (c == '|') || (c == ':') || (c == '\\') ||
            (c == '/')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XKRML::XKRML(QIODevice *pDevice) : XArchive(pDevice)
{
}

XKRML::~XKRML()
{
}

bool XKRML::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XKRML> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (KRML_HEADER_SIZE + KRML_RECORD_SIZE)) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, KRML_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHeader.size() != KRML_HEADER_SIZE)) {
        return false;
    }
    if (baHeader.left(4) != QByteArray("KRML", 4)) return false;
    const qint32 nMemberCount = qint32(qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(baHeader.constData()) + 4));
    if ((nMemberCount <= 0) || (nMemberCount > KRML_MAX_MEMBERS)) return false;

    const qint64 nDirectorySize = qint64(nMemberCount) * KRML_RECORD_SIZE;
    if (!krmlRangeWithin(context.nInputSize, KRML_HEADER_SIZE,
                         nDirectorySize)) {
        return false;
    }
    context.nDirectorySize = nDirectorySize;

    const QByteArray baDirectory =
        read_array_process(KRML_HEADER_SIZE, nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baDirectory.size() != nDirectorySize)) {
        return false;
    }

    const qint64 nDataOffset = KRML_HEADER_SIZE + nDirectorySize;
    for (qint32 i = 0; i < nMemberCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecordOffset = qint64(i) * KRML_RECORD_SIZE;
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baDirectory.constData()) +
            nRecordOffset;

        MEMBER member = {};
        member.nIndexOffset = KRML_HEADER_SIZE + nRecordOffset;
        member.nDataOffset =
            qint64(qint32(qFromLittleEndian<quint32>(pRecord + 13)));
        member.nSize = qint64(qint32(qFromLittleEndian<quint32>(pRecord + 17)));
        if ((member.nDataOffset < 0) || (member.nSize < 0)) return false;

        // The name field is 13 bytes wide; the original forces the last byte to
        // NUL, so a full 13-character name is not representable and 12 is the
        // real ceiling.
        QByteArray baName(reinterpret_cast<const char *>(pRecord),
                          KRML_NAME_SIZE);
        baName[KRML_NAME_SIZE - 1] = char(0);
        const qint32 nNul = baName.indexOf(char(0));
        if (nNul >= 0) baName = baName.left(nNul);
        if (!krmlIsValidName(baName)) return false;
        member.sFileName = QString::fromLatin1(baName);

        // The data area starts where the directory ends, and the first record
        // must point exactly there - this is the original's own check and the
        // only thing that makes a bare 4-byte magic safe to trust.
        if (i == 0) {
            if (member.nDataOffset != nDataOffset) return false;
        } else if (member.nDataOffset < nDataOffset) {
            return false;
        }
        if (!krmlRangeWithin(context.nInputSize, member.nDataOffset,
                             member.nSize)) {
            return false;
        }

        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nArchiveSize = nDataOffset;
    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        const MEMBER &member = context.listMembers.at(i);
        nArchiveSize = qMax(nArchiveSize, member.nDataOffset + member.nSize);
    }
    context.nArchiveSize = nArchiveSize;

    *pContext = context;
    return true;
}

bool XKRML::isValid(PDSTRUCT *pPdStruct)
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

bool XKRML::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XKRML archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XKRML::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XKRML(pDevice);
}

XBinary::FT XKRML::getFileType()
{
    return FT_KRML;
}

XBinary::MODE XKRML::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XKRML::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XKRML::getArch()
{
    return QString();
}

QString XKRML::getFileFormatExt()
{
    return QString();
}

QString XKRML::getFileFormatExtsString()
{
    return QStringLiteral("KRML resource archive (*.eng *.rus)");
}

QString XKRML::getMIMEString()
{
    return QStringLiteral("application/x-krml");
}

QString XKRML::getVersion()
{
    return QStringLiteral("1");
}

qint64 XKRML::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XKRML::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XKRML::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XKRML::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XKRML::getFileParts(quint32 nFileParts, qint32 nLimit,
                                          PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = KRML_HEADER_SIZE + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            result.append(part);
        }
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

QMap<XBinary::UNPACK_PROP, QVariant> XKRML::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XKRML::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QPointer<XKRML> guardedThis(this);
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
        pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO, tr("KRML game resource archive; stored members"));
    pState->nCurrentOffset = pContext->listMembers.first().nIndexOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XKRML::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nIndexOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The directory has no timestamp and no checksum field.
    return result;
}

bool XKRML::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nIndexOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XKRML::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
