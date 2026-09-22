/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xgamos.h"

#include <QtEndian>

#include <new>

namespace {
const char *GAMOS_MAGIC = "\x1a" "GAMOS PACKED FILE";
const qint32 GAMOS_MAGIC_SIZE = 18;
const qint64 GAMOS_HEADER_SIZE = 0x21;
const qint64 GAMOS_RECORD_SIZE = 22;
const qint32 GAMOS_NAME_SIZE = 13;
const quint8 GAMOS_METHOD_LZSS = 1;
const quint8 GAMOS_METHOD_STORE = 2;
// The member count is a u16, so this is the format's own ceiling.
const qint32 GAMOS_MAX_MEMBERS = 0xffff;

bool gamosRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XGamos::XGamos(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGamos::~XGamos()
{
}

XBinary::HANDLE_METHOD XGamos::methodOf(const MEMBER &member)
{
    if (member.nMethod == GAMOS_METHOD_STORE) return HANDLE_METHOD_STORE;
    if (member.nMethod == GAMOS_METHOD_LZSS) {
        return (member.nUncompressedSize > 0) ? HANDLE_METHOD_GAMOS : HANDLE_METHOD_STORE;
    }
    // The reference implementation returns code 3 for every other method byte, i.e. it lists
    // the member but produces nothing for it.  No such member exists in the
    // reference corpus; the record is published without a decoder rather than
    // being decoded with the wrong one.
    return HANDLE_METHOD_UNKNOWN;
}

QString XGamos::reportedMethodOf(const MEMBER &member)
{
    if (member.nMethod == GAMOS_METHOD_STORE) return QStringLiteral("Store");
    if (member.nMethod == GAMOS_METHOD_LZSS) return QStringLiteral("LZSS");
    return QStringLiteral("Unknown (%1)").arg(member.nMethod);
}

bool XGamos::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < GAMOS_HEADER_SIZE + GAMOS_RECORD_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, GAMOS_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != GAMOS_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (memcmp(pHeader, GAMOS_MAGIC, GAMOS_MAGIC_SIZE) != 0) return false;
    if ((pHeader[0x12] != 0) || (pHeader[0x13] != 1) || (pHeader[0x16] != 1)) return false;
    if (qFromLittleEndian<quint16>(pHeader + 0x14) != 0) return false;
    const qint32 nCount = (qint32)qFromLittleEndian<quint16>(pHeader + 0x17);
    if ((nCount <= 0) || (nCount > GAMOS_MAX_MEMBERS)) return false;

    context.nDirectoryOffset = GAMOS_HEADER_SIZE;
    context.nDirectorySize = (qint64)nCount * GAMOS_RECORD_SIZE;
    if (!gamosRangeWithin(context.nInputSize, context.nDirectoryOffset, context.nDirectorySize)) return false;

    const QByteArray baDirectory = read_array_process(context.nDirectoryOffset, context.nDirectorySize, pPdStruct);
    if ((baDirectory.size() != context.nDirectorySize)) return false;

    qint64 nArchiveEnd = context.nDirectoryOffset + context.nDirectorySize;
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecordOffset = context.nDirectoryOffset + (qint64)i * GAMOS_RECORD_SIZE;
        const uchar *pRecord = (const uchar *)baDirectory.constData() + (qint64)i * GAMOS_RECORD_SIZE;

        MEMBER member = {};
        member.nRecordOffset = nRecordOffset;
        member.nMethod = pRecord[0x0d];
        member.nDataOffset = (qint64)(qint32)qFromLittleEndian<quint32>(pRecord + 0x0e);
        member.nCompressedSize = (qint64)qFromLittleEndian<quint16>(pRecord + 0x12);
        member.nUncompressedSize = (qint64)qFromLittleEndian<quint16>(pRecord + 0x14);
        // The reference implementation abandons the archive on a non-positive data offset.
        if (member.nDataOffset < 1) return false;
        if (!gamosRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;

        qint32 nNameLength = 0;
        while ((nNameLength < GAMOS_NAME_SIZE) && (pRecord[nNameLength] != 0)) ++nNameLength;
        if (nNameLength == 0) return false;
        for (qint32 k = 0; k < nNameLength; ++k) {
            // 8.3 names written by a DOS tool; a control byte here means the
            // directory is not a directory.
            if (pRecord[k] < 0x20) return false;
        }
        member.sFileName = QString::fromLatin1((const char *)pRecord, nNameLength);

        // The stored size is the size on disk for both methods; only method 1
        // has a second, larger size.
        if (member.nMethod == GAMOS_METHOD_STORE) {
            member.nUncompressedSize = member.nCompressedSize;
        }

        nArchiveEnd = qMax(nArchiveEnd, member.nDataOffset + member.nCompressedSize);
        context.listMembers.append(member);
    }

    // The directory is immediately followed by the payload, so the first
    // member's data offset has to land exactly on the end of the directory.
    if (context.listMembers.first().nDataOffset != (context.nDirectoryOffset + context.nDirectorySize)) return false;

    context.nArchiveSize = nArchiveEnd;
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    *pContext = context;
    return true;
}

bool XGamos::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XGamos::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGamos archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGamos::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGamos(pDevice);
}

XBinary::FT XGamos::getFileType()
{
    return FT_GAMOS;
}

XBinary::MODE XGamos::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGamos::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGamos::getArch()
{
    return QString();
}

QString XGamos::getFileFormatExt()
{
    return QStringLiteral("gpf");
}

QString XGamos::getFileFormatExtsString()
{
    return QStringLiteral("Gamos packed file (*.gpf *.ega *.vga *.snd)");
}

QString XGamos::getMIMEString()
{
    return QStringLiteral("application/x-gamos");
}

QString XGamos::getVersion()
{
    // Bytes +0x13 and +0x16 are both 1 in every known container and are the
    // only version-shaped fields the header has.
    return QStringLiteral("1");
}

qint64 XGamos::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGamos::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XGamos::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XGamos::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XGamos::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDirectoryOffset + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodOf(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethodOf(member));
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

QMap<XBinary::UNPACK_PROP, QVariant> XGamos::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGamos::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("GAMOS packed file"));
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

XBinary::ARCHIVERECORD XGamos::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodOf(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethodOf(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The directory carries no timestamps and no checksums.
    return result;
}

bool XGamos::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XGamos::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
