/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xmegatechvol.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// The whole header is the offset table; the smallest legal one is two entries
// (one member plus the end sentinel).
const qint64 MEGATECH_MIN_TABLE_SIZE = 8;
// The known writer emits a 0x80 or 0x100 byte table; this is only a sanity cap
// so a bogus first u32 cannot make the parser allocate wildly.
const qint64 MEGATECH_MAX_TABLE_SIZE = 0x10000;
// A .VOL smaller than this cannot hold a table plus any payload at all.
const qint64 MEGATECH_MIN_FILE_SIZE = 16;

const quint32 MEGATECH_MAGIC_VOC = 0x61657243U;  // "Crea", Creative Voice File
const quint32 MEGATECH_MAGIC_GPH = 0x1d485047U;  // "GPH\x1d", Megatech graphics

QString megatechExtension(const QByteArray &baHead)
{
    if (baHead.size() >= 4) {
        const quint32 nMagic = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(baHead.constData()));
        if (nMagic == MEGATECH_MAGIC_GPH) return QStringLiteral("gph");
        if (nMagic == MEGATECH_MAGIC_VOC) return QStringLiteral("voc");
    }
    return QStringLiteral("bin");
}

// U3's FUN_0063d7b0 / FUN_0063d8e0, verbatim: the table must be non-decreasing
// and non-negative, must reach a sentinel entry equal to the file size that is
// last or followed by a 0, and everything after that sentinel must be 0.
bool megatechTableIsConsistent(const QList<quint32> &listTable,
                               qint64 nFileSize)
{
    const qint32 nCount = listTable.size();
    qint32 i = 0;
    while (i < nCount) {
        const quint32 nEntry = listTable.at(i);
        if ((static_cast<qint64>(nEntry) == nFileSize) &&
            (((i + 1) >= nCount) || (listTable.at(i + 1) == 0))) {
            ++i;
            break;
        }
        if (nEntry & 0x80000000U) return false;  // negative when read as i32
        if (static_cast<qint64>(nEntry) > nFileSize) return false;
        if ((i + 1) >= nCount) return false;     // ran out without a sentinel
        if (listTable.at(i + 1) < nEntry) return false;
        ++i;
    }
    for (; i < nCount; ++i) {
        if (listTable.at(i) != 0) return false;
    }
    return true;
}
}  // namespace

XMegatechVOL::XMegatechVOL(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMegatechVOL::~XMegatechVOL()
{
}

bool XMegatechVOL::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XMegatechVOL> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < MEGATECH_MIN_FILE_SIZE) return false;

    const QByteArray baFirst = read_array_process(0, 4, pPdStruct);
    if (!guardedThis || !guardedSource || (baFirst.size() != 4)) return false;
    const qint64 nTableSize = static_cast<qint64>(qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baFirst.constData())));
    if ((nTableSize < MEGATECH_MIN_TABLE_SIZE) ||
        (nTableSize > MEGATECH_MAX_TABLE_SIZE) || (nTableSize & 3) ||
        (nTableSize > context.nInputSize)) {
        return false;
    }
    context.nTableSize = nTableSize;

    const QByteArray baTable = read_array_process(0, nTableSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baTable.size() != nTableSize)) {
        return false;
    }
    const qint32 nCount = static_cast<qint32>(nTableSize / 4);
    QList<quint32> listTable;
    listTable.reserve(nCount);
    const uchar *pTable = reinterpret_cast<const uchar *>(baTable.constData());
    for (qint32 i = 0; i < nCount; ++i) {
        listTable.append(qFromLittleEndian<quint32>(pTable + (i * 4)));
    }
    if (!megatechTableIsConsistent(listTable, context.nInputSize)) return false;

    // The last entry is the end sentinel, never a member start, so the walk
    // stops one short of the table's end (U3 iterates 0 .. nCount - 2).
    qint32 nNumber = 0;
    for (qint32 i = 0; (i + 1) < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (listTable.at(i + 1) == 0) break;
        const qint64 nOffset = static_cast<qint64>(listTable.at(i));
        const qint64 nSize =
            static_cast<qint64>(listTable.at(i + 1)) - nOffset;
        if (nSize == 0) continue;  // repeated offset: an empty slot, skipped
        if ((nSize < 0) || (nOffset < nTableSize) ||
            (nSize > (context.nInputSize - nOffset))) {
            return false;
        }
        const QByteArray baHead = read_array_process(
            nOffset, qMin<qint64>(4, nSize), pPdStruct);
        if (!guardedThis || !guardedSource) return false;

        MEMBER member = {};
        member.nTableIndex = i;
        member.nDataOffset = nOffset;
        member.nDataSize = nSize;
        ++nNumber;
        member.sFileName = QString::number(nNumber) + QLatin1Char('.') +
                           megatechExtension(baHead);
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    *pContext = context;
    return true;
}

bool XMegatechVOL::isValid(PDSTRUCT *pPdStruct)
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

bool XMegatechVOL::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMegatechVOL archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMegatechVOL::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMegatechVOL(pDevice);
}

XBinary::FT XMegatechVOL::getFileType()
{
    return FT_MEGATECH_VOL;
}

XBinary::MODE XMegatechVOL::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMegatechVOL::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMegatechVOL::getArch()
{
    return QString();
}

QString XMegatechVOL::getFileFormatExt()
{
    return QStringLiteral("vol");
}

QString XMegatechVOL::getFileFormatExtsString()
{
    return QStringLiteral("Megatech resource volume (*.vol)");
}

QString XMegatechVOL::getMIMEString()
{
    return QStringLiteral("application/x-megatech-vol");
}

QString XMegatechVOL::getVersion()
{
    // No version field of any kind exists in the container.
    return QString();
}

qint64 XMegatechVOL::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return 0;
    // The sentinel entry is the file size, so the container always spans the
    // whole file and there is never an overlay.
    return context.nInputSize;
}

QList<XBinary::MAPMODE> XMegatechVOL::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XMegatechVOL::getMemoryMap(MAPMODE mapMode,
                                                PDSTRUCT *pPdStruct)
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

bool XMegatechVOL::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XMegatechVOL::getFileParts(quint32 nFileParts,
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
        part.nFileSize = context.nTableSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Offset table");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Store"));
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XMegatechVOL::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMegatechVOL::initUnpack(UNPACK_STATE *pState,
                              const QMap<UNPACK_PROP, QVariant> &mapProperties,
                              PDSTRUCT *pPdStruct)
{
    QPointer<XMegatechVOL> guardedThis(this);
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
        FPART_PROP_INFO,
        tr("Megatech resource volume; stored members, offset table only"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nInputSize;
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

XBinary::ARCHIVERECORD XMegatechVOL::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Store"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No timestamps and no checksums exist anywhere in this container.
    return result;
}

bool XMegatechVOL::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nInputSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XMegatechVOL::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
