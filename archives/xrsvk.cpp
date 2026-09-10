/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xrsvk.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 RSVK_TRAILER_SIZE = 12;
const qint64 RSVK_DIR_ENTRY_SIZE = 28;
const qint64 RSVK_FIRST_BLOCK_OFFSET = 4;
const qint32 RSVK_MAX_NAME_SIZE = 1024;
const qint32 RSVK_MAX_MEMBERS = 100000;
const qint32 RSVK_MAX_BLOCKS = 100000;
const qint64 RSVK_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB sanity cap

bool rsvkIsTag(const uchar *p, const char *pTag)
{
    return (p[0] == quint8(pTag[0])) && (p[1] == quint8(pTag[1])) &&
           (p[2] == quint8(pTag[2])) && (p[3] == quint8(pTag[3]));
}

// The stored name is a DOS path with a drive letter ("d:\aida32.da0").  Keep
// the directory structure but never publish a drive-absolute path.
QString rsvkCleanName(const QByteArray &baRaw)
{
    QString sName = QString::fromLatin1(baRaw);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if ((sName.size() >= 2) && (sName.at(1) == QLatin1Char(':'))) {
        sName = sName.mid(2);
    }
    while (sName.startsWith(QLatin1Char('/'))) sName = sName.mid(1);
    while (sName.contains(QStringLiteral("../"))) {
        sName.replace(QStringLiteral("../"), QStringLiteral(""));
    }
    return sName;
}

bool rsvkIsPrintableName(const QByteArray &baRaw)
{
    if (baRaw.isEmpty() || (baRaw.size() > RSVK_MAX_NAME_SIZE)) return false;
    for (char c : baRaw) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
    }
    return true;
}
}  // namespace

XRSVK::XRSVK(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRSVK::~XRSVK()
{
}

bool XRSVK::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XRSVK> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (RSVK_FIRST_BLOCK_OFFSET + 20 + RSVK_TRAILER_SIZE)) {
        return false;
    }

    // Header: the container tag plus the "DATA" tag of the first block, whose
    // packed size (at +0x0c) the reference implementation's detector requires to be non-negative.
    const QByteArray baHead = read_array_process(0, 16, pPdStruct);
    if (!guardedThis || !guardedSource || (baHead.size() != 16)) return false;
    const uchar *pHead = reinterpret_cast<const uchar *>(baHead.constData());
    if (!rsvkIsTag(pHead, "RSVK") && !rsvkIsTag(pHead, "DLIB")) return false;
    if (!rsvkIsTag(pHead + 4, "DATA")) return false;
    if (static_cast<qint32>(qFromLittleEndian<quint32>(pHead + 12)) < 0) {
        return false;
    }

    const QByteArray baTrailer = read_array_process(
        context.nInputSize - RSVK_TRAILER_SIZE, qint32(RSVK_TRAILER_SIZE),
        pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baTrailer.size() != RSVK_TRAILER_SIZE)) {
        return false;
    }
    const uchar *pTrailer =
        reinterpret_cast<const uchar *>(baTrailer.constData());
    if (!rsvkIsTag(pTrailer, "ECDR") && !rsvkIsTag(pTrailer, "DEND")) {
        return false;
    }
    context.nDirOffset =
        static_cast<qint64>(qFromLittleEndian<quint32>(pTrailer + 8));
    if ((context.nDirOffset < RSVK_FIRST_BLOCK_OFFSET) ||
        (context.nDirOffset >
         context.nInputSize - RSVK_TRAILER_SIZE - RSVK_DIR_ENTRY_SIZE)) {
        return false;
    }
    context.nDirSize =
        (context.nInputSize - RSVK_TRAILER_SIZE) - context.nDirOffset;

    const QByteArray baDir = read_array_process(
        context.nDirOffset, qint32(context.nDirSize), pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baDir.size() != qint32(context.nDirSize))) {
        return false;
    }

    qint64 nCursor = 0;
    while (nCursor <= context.nDirSize - RSVK_DIR_ENTRY_SIZE) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listEntries.size() > RSVK_MAX_MEMBERS) return false;
        const uchar *pEntry =
            reinterpret_cast<const uchar *>(baDir.constData()) + nCursor;
        if (!rsvkIsTag(pEntry, "CFHS") && !rsvkIsTag(pEntry, "FILE")) {
            return false;
        }
        MEMBER member = {};
        member.nDirOffset = context.nDirOffset + nCursor;
        const qint32 nPackedHint =
            static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 4));
        member.nUncompressedSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 8)));
        member.nBlockCount =
            static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 12));
        member.nDataOffset = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pEntry + 16)));
        member.nDosTime = qFromLittleEndian<quint16>(pEntry + 20);
        member.nDosDate = qFromLittleEndian<quint16>(pEntry + 22);
        member.nAttributes = qFromLittleEndian<quint32>(pEntry + 24);
        if ((nPackedHint < 0) || (member.nUncompressedSize < 0) ||
            (member.nUncompressedSize > RSVK_MAX_UNCOMPRESSED_SIZE) ||
            (member.nBlockCount <= 0) ||
            (member.nBlockCount > RSVK_MAX_BLOCKS) ||
            (member.nDataOffset < RSVK_FIRST_BLOCK_OFFSET) ||
            (member.nDataOffset >= context.nDirOffset)) {
            return false;
        }

        // Name: NUL-terminated, immediately after the fixed part.
        qint64 nNameStart = nCursor + RSVK_DIR_ENTRY_SIZE;
        qint64 nNameEnd = nNameStart;
        while ((nNameEnd < context.nDirSize) && (baDir.at(qint32(nNameEnd)) != '\0')) {
            ++nNameEnd;
        }
        if (nNameEnd >= context.nDirSize) return false;  // unterminated
        const QByteArray baName =
            baDir.mid(qint32(nNameStart), qint32(nNameEnd - nNameStart));
        if (!rsvkIsPrintableName(baName)) return false;
        member.sFileName = rsvkCleanName(baName);
        if (member.sFileName.isEmpty()) return false;

        // The entry's packed-size field does not match the real chain, so walk
        // the member's blocks: it yields the exact length and validates that
        // the declared block count really lands inside the data area.
        qint64 nChainCursor = member.nDataOffset;
        for (qint32 i = 0; i < member.nBlockCount; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (nChainCursor > context.nDirOffset - 20) return false;
            const QByteArray baBlockHeader =
                read_array_process(nChainCursor, 20, pPdStruct);
            if (!guardedThis || !guardedSource ||
                (baBlockHeader.size() != 20)) {
                return false;
            }
            const uchar *pBlock =
                reinterpret_cast<const uchar *>(baBlockHeader.constData());
            if (!rsvkIsTag(pBlock, "DATA")) return false;
            const qint64 nPacked =
                static_cast<qint64>(qFromLittleEndian<quint32>(pBlock + 8));
            if ((nPacked <= 0) ||
                (nPacked > context.nDirOffset - nChainCursor - 20)) {
                return false;
            }
            nChainCursor += 20 + nPacked;
        }
        member.nCompressedSize = nChainCursor - member.nDataOffset;

        context.listEntries.append(member);
        nCursor = nNameEnd + 1;
    }

    if (context.listEntries.isEmpty()) return false;
    if (!guardedThis || !guardedSource) return false;
    *pContext = context;
    return true;
}

bool XRSVK::isValid(PDSTRUCT *pPdStruct)
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

bool XRSVK::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRSVK archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRSVK::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRSVK(pDevice);
}

QList<QString> XRSVK::getSearchSignatures()
{
    return {QStringLiteral("'RSVKDATA'"), QStringLiteral("'DLIBDATA'")};
}

XBinary::FT XRSVK::getFileType()
{
    return FT_RSVK;
}

XBinary::MODE XRSVK::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRSVK::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRSVK::getArch()
{
    return QString();
}

QString XRSVK::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XRSVK::getFileFormatExtsString()
{
    return QStringLiteral("RSVKDATA container (*.dat *.bin)");
}

QString XRSVK::getMIMEString()
{
    return QStringLiteral("application/x-rsvk");
}

QString XRSVK::getVersion()
{
    // Nothing in the container is a version field.
    return QStringLiteral("1");
}

qint64 XRSVK::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XRSVK::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRSVK::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_STREAM | FILEPART_TABLE, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QList<XBinary::FPART> XRSVK::getFileParts(quint32 nFileParts, qint32 nLimit,
                                          PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if (nFileParts & FILEPART_STREAM) {
        for (const MEMBER &member : context.listEntries) {
            if (!isPdStructNotCanceled(pPdStruct)) break;
            if ((nLimit > 0) && (listResult.size() >= nLimit)) break;
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      (member.nUncompressedSize == 0)
                                          ? HANDLE_METHOD_STORE
                                          : HANDLE_METHOD_RSVK);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("BWT + arithmetic"));
            const QDateTime dtMTime =
                dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
            if (dtMTime.isValid()) {
                part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            }
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_TABLE) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_TABLE;
        part.nFileOffset = context.nDirOffset;
        part.nFileSize = context.nDirSize + RSVK_TRAILER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) &&
        ((nLimit <= 0) || (listResult.size() < nLimit))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XRSVK::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRSVK::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QPointer<XRSVK> guardedThis(this);
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
        pContext->listEntries.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("RSVKDATA container; block-sorting (BWT) members"));
    pState->nCurrentOffset = pContext->listEntries.first().nDirOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
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

XBinary::ARCHIVERECORD XRSVK::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listEntries.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDirOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                (member.nUncompressedSize == 0)
                                    ? HANDLE_METHOD_STORE
                                    : HANDLE_METHOD_RSVK);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("BWT + arithmetic"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime =
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    // The per-block CRC-32s are checked inside the codec; there is no
    // whole-member checksum to publish.
    return result;
}

bool XRSVK::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listEntries.size()) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nDirOffset;
    } else {
        pState->nCurrentOffset = pContext->nInputSize;
    }
    return (pState->nCurrentIndex < pContext->listEntries.size());
}

bool XRSVK::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
