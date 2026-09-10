/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xexeebookcreator.h"

#include <QPointer>
#include <QtEndian>

#include <new>

#include "../Formats/exec/xpe.h"

namespace {
const qint64 EBC_HEADER_SIZE = 0x2b;
const qint64 EBC_RECORD_SIZE = 8;
const qint64 EBC_COUNT_OFFSET = 22;
// the four trailing bytes of every record belong to the NEXT MFC object,
// not to this member's zlib stream
const qint32 EBC_TAG_SIZE = 4;
// two bytes of zlib header, four of Adler-32, four of the next tag
const qint32 EBC_MIN_RECORD_STRIDE = 10;
// The count field is 16 bit, so 65535 members is the hard producer limit.
const qint32 EBC_MAX_ENTRIES = 65535;

// The fixed part of the overlay header: the two MFC CArchive class descriptors.
// 28 of the 43 bytes are constant, which is what makes detection safe.
struct EBC_FIXED_BYTE {
    qint32 nOffset;
    quint8 nValue;
};

const EBC_FIXED_BYTE EBC_FIXED[] = {
    {0x00, 0x00}, {0x01, 0x00}, {0x02, 0x02}, {0x03, 0x00}, {0x04, 0xff}, {0x05, 0xff}, {0x06, 0x01}, {0x07, 0x00},
    {0x08, 0x0a}, {0x09, 0x00}, {0x0a, 'C'},  {0x0b, 'U'},  {0x0c, 'p'},  {0x0d, 'd'},  {0x0e, 'a'},  {0x0f, 't'},
    {0x10, 'e'},  {0x11, 'D'},  {0x12, 'i'},  {0x13, 'r'},  {0x18, 0xff}, {0x19, 0xff}, {0x1a, 0x01}, {0x1b, 0x00},
    {0x1c, 0x0b}, {0x1d, 0x00}, {0x1e, 'C'},  {0x1f, 'U'},  {0x20, 'p'},  {0x21, 'd'},  {0x22, 'a'},  {0x23, 't'},
    {0x24, 'e'},  {0x25, 'E'},  {0x26, 'l'},  {0x27, 'e'},  {0x28, 'm'},  {0x29, 0x01}, {0x2a, 0x00},
};

bool ebcRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XEXEEBookCreator::XEXEEBookCreator(QIODevice *pDevice) : XArchive(pDevice)
{
}

XEXEEBookCreator::~XEXEEBookCreator()
{
}

bool XEXEEBookCreator::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XEXEEBookCreator> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < EBC_HEADER_SIZE) return false;

    // The payload is always the PE overlay; there is no second copy of the
    // header anywhere else in the image, so the offset is not searched for.
    XPE pe(getDevice());
    if (!pe.isValid(pPdStruct) || !guardedThis || !guardedSource) return false;
    context.nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    if (!ebcRangeWithin(context.nInputSize, context.nOverlayOffset, EBC_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array_process(context.nOverlayOffset, EBC_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != EBC_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    for (qint32 i = 0; i < static_cast<qint32>(sizeof(EBC_FIXED) / sizeof(EBC_FIXED[0])); i++) {
        if (pHeader[EBC_FIXED[i].nOffset] != EBC_FIXED[i].nValue) return false;
    }

    const qint32 nNumberOfEntries = static_cast<qint32>(qFromLittleEndian<quint16>(pHeader + EBC_COUNT_OFFSET));
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > EBC_MAX_ENTRIES)) return false;
    context.nNumberOfEntries = nNumberOfEntries;

    qint64 nPosition = context.nOverlayOffset + EBC_HEADER_SIZE;
    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!ebcRangeWithin(context.nInputSize, nPosition, EBC_RECORD_SIZE)) return false;
        const QByteArray baRecord = read_array_process(nPosition, EBC_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != EBC_RECORD_SIZE)) return false;
        const uchar *pRecord = reinterpret_cast<const uchar *>(baRecord.constData());
        // Careful: this is the stride to the NEXT record, which is the zlib
        // stream length plus the four bytes of the following MFC CArchive
        // object's tag. EBC_TAG_SIZE below recovers the real stream length.
        const qint32 nCompressedSize = static_cast<qint32>(qFromLittleEndian<quint32>(pRecord));
        const qint32 nUncompressedSize = static_cast<qint32>(qFromLittleEndian<quint32>(pRecord + 4));
        // Both sizes are signed in the producer and the reference extractor
        // rejects a negative one outright.
        if ((nCompressedSize < EBC_MIN_RECORD_STRIDE) || (nUncompressedSize < 0)) return false;

        MEMBER member = {};
        member.nRecordOffset = nPosition;
        member.nDataOffset = nPosition + EBC_RECORD_SIZE;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.sFileName = QString::number(i);
        if (!ebcRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;

        context.listMembers.append(member);
        nPosition = member.nDataOffset + member.nCompressedSize;
    }

    context.nArchiveSize = nPosition;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XEXEEBookCreator::isValid(PDSTRUCT *pPdStruct)
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

bool XEXEEBookCreator::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEXEEBookCreator archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XEXEEBookCreator::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XEXEEBookCreator(pDevice);
}

QList<QString> XEXEEBookCreator::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'MZ'"));
    return listResult;
}

XBinary::FT XEXEEBookCreator::getFileType()
{
    return FT_EXE_EBOOKCREATOR;
}

XBinary::MODE XEXEEBookCreator::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEXEEBookCreator::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XEXEEBookCreator::getArch()
{
    return QString();
}

QString XEXEEBookCreator::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XEXEEBookCreator::getFileFormatExtsString()
{
    return QStringLiteral("EBook self-running executable (*.exe)");
}

QString XEXEEBookCreator::getMIMEString()
{
    return QStringLiteral("application/x-msdownload");
}

QString XEXEEBookCreator::getVersion()
{
    return QString();
}

qint64 XEXEEBookCreator::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XEXEEBookCreator::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XEXEEBookCreator::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XEXEEBookCreator::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XEXEEBookCreator::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nOverlayOffset + EBC_HEADER_SIZE;
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
            part.nFileSize = member.nCompressedSize - EBC_TAG_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize - EBC_TAG_SIZE);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate (zlib)"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XEXEEBookCreator::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEXEEBookCreator::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XEXEEBookCreator> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("EBook self-running executable"));
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

XBinary::ARCHIVERECORD XEXEEBookCreator::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nCompressedSize - EBC_TAG_SIZE;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize - EBC_TAG_SIZE);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate (zlib)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The zlib stream carries its own Adler-32; the container adds no checksum.
    return result;
}

bool XEXEEBookCreator::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XEXEEBookCreator::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
