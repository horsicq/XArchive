/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xriversoft.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const char RIVERSOFT_MAGIC[] = "RiverSoft Data Library\x1a";
const qint64 RIVERSOFT_MAGIC_SIZE = 23;
const qint64 RIVERSOFT_HEADER_SIZE = 0x20;
const qint32 RIVERSOFT_COUNT_OFFSET = 0x1a;
const qint32 RIVERSOFT_TOTALSIZE_OFFSET = 0x1c;
const qint64 RIVERSOFT_DIRENTRY_SIZE = 0x15;
const qint32 RIVERSOFT_NAME_FIELD_SIZE = 13;
// ShortString[12]: the reference implementation rejects any other length outright.
const qint32 RIVERSOFT_MAX_NAME_SIZE = 12;

bool riversoftIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty() || (baName.size() > RIVERSOFT_MAX_NAME_SIZE)) {
        return false;
    }
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((c == '"') || (c == '*') || (c == '<') || (c == '>') ||
            (c == '?') || (c == '|') || (c == ':') || (c == '/') ||
            (c == '\\')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XRiverSoft::XRiverSoft(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRiverSoft::~XRiverSoft()
{
}

bool XRiverSoft::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XRiverSoft> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < RIVERSOFT_HEADER_SIZE + RIVERSOFT_DIRENTRY_SIZE) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, RIVERSOFT_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baHeader.size() != RIVERSOFT_HEADER_SIZE)) {
        return false;
    }
    if (memcmp(baHeader.constData(), RIVERSOFT_MAGIC,
               static_cast<size_t>(RIVERSOFT_MAGIC_SIZE)) != 0) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    context.nEntryCount = static_cast<qint32>(
        qFromLittleEndian<quint16>(pHeader + RIVERSOFT_COUNT_OFFSET));
    context.nTotalUncompressedSize =
        qFromLittleEndian<quint32>(pHeader + RIVERSOFT_TOTALSIZE_OFFSET);
    if (context.nEntryCount <= 0) return false;

    context.nDirectorySize = context.nEntryCount * RIVERSOFT_DIRENTRY_SIZE;
    context.nDirectoryOffset = context.nInputSize - context.nDirectorySize;
    if (context.nDirectoryOffset < RIVERSOFT_HEADER_SIZE) return false;

    const QByteArray baDirectory = read_array_process(
        context.nDirectoryOffset, context.nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baDirectory.size() != context.nDirectorySize)) {
        return false;
    }

    for (qint32 i = 0; i < context.nEntryCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nRecordOffset =
            static_cast<qint32>(i * RIVERSOFT_DIRENTRY_SIZE);
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baDirectory.constData()) +
            nRecordOffset;
        const qint32 nNameSize = pRecord[0];
        if ((nNameSize <= 0) || (nNameSize > RIVERSOFT_MAX_NAME_SIZE)) {
            return false;
        }
        const QByteArray baName =
            baDirectory.mid(nRecordOffset + 1, nNameSize);
        if (!riversoftIsValidName(baName)) return false;

        MEMBER member = {};
        member.nDirOffset = context.nDirectoryOffset + nRecordOffset;
        member.sFileName = QString::fromLatin1(baName);
        member.nDataOffset = static_cast<qint64>(
            qFromLittleEndian<quint32>(pRecord + RIVERSOFT_NAME_FIELD_SIZE));
        member.nCompressedSize = static_cast<qint64>(qFromLittleEndian<quint16>(
            pRecord + RIVERSOFT_NAME_FIELD_SIZE + 4));
        member.nUncompressedSize = static_cast<qint64>(
            qFromLittleEndian<quint16>(pRecord + RIVERSOFT_NAME_FIELD_SIZE +
                                       6));
        // Members live strictly between the header and the directory.
        if ((member.nDataOffset < RIVERSOFT_HEADER_SIZE) ||
            (member.nDataOffset > context.nDirectoryOffset) ||
            (member.nCompressedSize >
             context.nDirectoryOffset - member.nDataOffset)) {
            return false;
        }
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return true;
}

void XRiverSoft::fillMemberProperties(const MEMBER &member,
                                      QMap<FPART_PROP, QVariant> *pmap)
{
    if (!pmap) return;
    pmap->insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    pmap->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    pmap->insert(FPART_PROP_HANDLEMETHOD,
                 ((member.nUncompressedSize == 0) ||
                  (member.nCompressedSize == 0))
                     ? HANDLE_METHOD_STORE
                     : HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    pmap->insert(FPART_PROP_REPORTEDMETHOD,
                 QStringLiteral("PKWARE DCL implode"));
}

bool XRiverSoft::isValid(PDSTRUCT *pPdStruct)
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

bool XRiverSoft::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRiverSoft archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRiverSoft::createInstance(QIODevice *pDevice, bool bIsImage,
                                    XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRiverSoft(pDevice);
}

QList<QString> XRiverSoft::getSearchSignatures()
{
    return {QStringLiteral("'RiverSoft Data Library'1A")};
}

XBinary::FT XRiverSoft::getFileType()
{
    return FT_RIVERSOFT;
}

XBinary::MODE XRiverSoft::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRiverSoft::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRiverSoft::getArch()
{
    return QString();
}

QString XRiverSoft::getFileFormatExt()
{
    return QStringLiteral("rdl");
}

QString XRiverSoft::getFileFormatExtsString()
{
    return QStringLiteral("RiverSoft Data Library (*.rdl)");
}

QString XRiverSoft::getMIMEString()
{
    return QStringLiteral("application/x-riversoft-rdl");
}

QString XRiverSoft::getVersion()
{
    // Header +0x17 and +0x19; both are 1 in every known archive.
    return QStringLiteral("1.1");
}

qint64 XRiverSoft::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XRiverSoft::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRiverSoft::getMemoryMap(MAPMODE mapMode,
                                              PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_FOOTER, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XRiverSoft::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XRiverSoft::getFileParts(quint32 nFileParts,
                                               qint32 nLimit,
                                               PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = RIVERSOFT_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct)) break;
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            fillMemberProperties(member, &part.mapProperties);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_FOOTER) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_FOOTER;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
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

QMap<XBinary::UNPACK_PROP, QVariant> XRiverSoft::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRiverSoft::initUnpack(UNPACK_STATE *pState,
                            const QMap<UNPACK_PROP, QVariant> &mapProperties,
                            PDSTRUCT *pPdStruct)
{
    QPointer<XRiverSoft> guardedThis(this);
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("RiverSoft Data Library; PKWARE DCL imploded members"));
    pState->nCurrentOffset = pContext->listMembers.first().nDirOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XRiverSoft::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nDirOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    fillMemberProperties(member, &result.mapProperties);
    return result;
}

bool XRiverSoft::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nDirOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XRiverSoft::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
