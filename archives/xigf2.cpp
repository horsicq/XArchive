/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xigf2.h"

#include <QDateTime>
#include <QtEndian>

#include <new>
#include <QTimeZone>

namespace {
const qint64 IGF2_HEADER_SIZE = 0x28;
const qint64 IGF2_RECORD_SIZE = 0x38;
const quint16 IGF2_MAGIC = 0x1324U;
const quint16 IGF2_RECORD_MAGIC = 0xECDBU;
const quint16 IGF2_END_TAG = 0xFFFFU;
const qint64 IGF2_MAX_NAME = 1024;
const qint32 IGF2_MAX_MEMBERS = 65536;

struct IGF2_LAYOUT {
    qint32 nTimeOffset;
    qint32 nRawOffset;
    qint32 nPackedOffset;
    qint32 nNameOffset;
};
const IGF2_LAYOUT s_igf2Layouts[2] = {{0x14, 0x1c, 0x24, 0x38}, {0x10, 0x18, 0x20, 0x2c}};

// Names are absolute DOS paths ("C:\WIN95\~igf0F30.TMP").  The drive colon is
// folded to '_' so the member lands under a directory of its own, backslashes
// become '/', and anything that could climb out of the destination is neutered.
QString igf2RawNameToString(const QByteArray &baName, qint32 nIndex)
{
    QString sResult;
    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if (nCharacter == '\\') {
            sResult.append(QLatin1Char('/'));
        } else if ((nCharacter == ':') || (nCharacter < 0x20) || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') || (nCharacter == '<') ||
                   (nCharacter == '>') || (nCharacter == '|')) {
            sResult.append(QLatin1Char('_'));
        } else {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        }
    }
    while (sResult.startsWith(QLatin1Char('/'))) sResult.remove(0, 1);
    sResult.replace(QLatin1String("../"), QLatin1String("__/"));
    if (sResult.isEmpty()) sResult = QStringLiteral("record%1").arg(nIndex);
    return sResult;
}
}  // namespace

XIGF2::XIGF2(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIGF2::~XIGF2()
{
}

bool XIGF2::parseContext(CONTEXT *pContext, bool bHeaderOnly, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < IGF2_HEADER_SIZE + IGF2_RECORD_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, IGF2_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != IGF2_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (qFromLittleEndian<quint16>(pHeader) != IGF2_MAGIC) return false;
    // The stored size must be the real size: this is what makes a two byte
    // magic safe to detect on, and it also rules out an appended overlay.
    if (static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + 8)) != context.nInputSize) return false;
    if (qFromLittleEndian<qint32>(pHeader + 0x0c) <= 0) return false;

    const quint32 nDirectoryOffset = qFromLittleEndian<quint32>(pHeader + 0x20);
    const quint32 nDirectoryOffsetNot = qFromLittleEndian<quint32>(pHeader + 0x24);
    if ((nDirectoryOffset == 0) || (nDirectoryOffset > 0x7fffffffU)) return false;
    if (nDirectoryOffset != (~nDirectoryOffsetNot)) return false;
    context.nDirectoryOffset = static_cast<qint64>(nDirectoryOffset);
    if (context.nDirectoryOffset + IGF2_RECORD_SIZE > context.nInputSize) return false;
    context.nArchiveSize = context.nInputSize;

    // Validate the first record even in the header-only pass.
    const QByteArray baFirst = read_array_process(context.nDirectoryOffset, IGF2_RECORD_SIZE, pPdStruct);
    if ((baFirst.size() != IGF2_RECORD_SIZE)) return false;
    if (qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baFirst.constData())) != IGF2_RECORD_MAGIC) return false;

    if (bHeaderOnly) {
        *pContext = context;
        return true;
    }

    // Two record shapes exist.  The one the reference extractor hard codes puts
    // the size quad at +0x1c and the name at +0x38; a second, otherwise
    // identical shape drops twelve bytes of padding, moving the quad to +0x18
    // and the name to +0x2c.  Nothing in the header distinguishes them, so both
    // are walked and the one that reaches the 0xFFFF terminator with every
    // extent inside the file wins.  The reference extractor, which only knows
    // the first shape, recovers a single member from an archive of the second
    // kind and stops; this walk recovers all of them, and the member it does
    // produce is byte identical either way.
    QList<MEMBER> listA;
    QList<MEMBER> listB;
    const bool bTerminatedA = walkDirectory(&context, &s_igf2Layouts[0], &listA, pPdStruct);
    if (!guardedSource) return false;
    if (bTerminatedA) {
        context.listMembers = listA;
    } else {
        const bool bTerminatedB = walkDirectory(&context, &s_igf2Layouts[1], &listB, pPdStruct);
        if (!guardedSource) return false;
        if (bTerminatedB) {
            context.listMembers = listB;
        } else {
            // Neither chain reached a terminator: keep whichever damaged walk
            // recovered more members, preferring the reference layout on a tie.
            context.listMembers = (listB.size() > listA.size()) ? listB : listA;
        }
    }
    if (context.listMembers.isEmpty()) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

// Walks the record chain under one candidate layout.  Returns true only when
// the chain ends on the 0xFFFF terminator record; whatever could be parsed is
// left in *pListMembers either way.
bool XIGF2::walkDirectory(const CONTEXT *pContext, const void *pLayoutRaw, QList<MEMBER> *pListMembers, PDSTRUCT *pPdStruct)
{
    const IGF2_LAYOUT &layout = *static_cast<const IGF2_LAYOUT *>(pLayoutRaw);
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    qint64 nPosition = pContext->nDirectoryOffset;
    while ((nPosition + 4) <= pContext->nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        // The terminator record is often truncated at EOF, so its tag is
        // checked before a whole record is required.
        const QByteArray baTag = read_array_process(nPosition, 4, pPdStruct);
        if ((baTag.size() != 4)) return false;
        if (qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baTag.constData()) + 2) == IGF2_END_TAG) return true;
        if (qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baTag.constData())) != IGF2_RECORD_MAGIC) return false;
        if ((nPosition + IGF2_RECORD_SIZE) > pContext->nInputSize) return false;

        const QByteArray baRecord = read_array_process(nPosition, IGF2_RECORD_SIZE, pPdStruct);
        if ((baRecord.size() != IGF2_RECORD_SIZE)) return false;
        const uchar *pRecord = reinterpret_cast<const uchar *>(baRecord.constData());

        const qint32 nRawSize = qFromLittleEndian<qint32>(pRecord + layout.nRawOffset);
        const qint32 nPackedSize = qFromLittleEndian<qint32>(pRecord + layout.nPackedOffset);
        if ((nRawSize < 0) || (nPackedSize < 0)) return false;

        // The name is NUL terminated, and the reader then aligns to the next
        // multiple of four measured from the record start - strictly past the
        // terminator, so an already aligned position still advances.
        const qint64 nNameOffset = nPosition + layout.nNameOffset;
        const qint64 nNameRoom = qMin<qint64>(IGF2_MAX_NAME, pContext->nInputSize - nNameOffset);
        if (nNameRoom <= 0) return false;
        const QByteArray baNameRaw = read_array_process(nNameOffset, nNameRoom, pPdStruct);
        if (baNameRaw.isEmpty()) return false;
        const qint32 nTerminator = baNameRaw.indexOf(char(0));
        if (nTerminator <= 0) return false;
        const QByteArray baName = baNameRaw.left(nTerminator);

        qint64 nRelative = (nNameOffset + nTerminator + 1) - nPosition;
        nRelative = (nRelative + 4) & ~static_cast<qint64>(3);
        const qint64 nDataOffset = nPosition + nRelative;
        if ((nDataOffset < 0) || (nDataOffset > pContext->nInputSize)) return false;
        if (nPackedSize > (pContext->nInputSize - nDataOffset)) return false;

        MEMBER member = {};
        member.nRecordOffset = nPosition;
        member.nDataOffset = nDataOffset;
        member.nPackedSize = nPackedSize;
        member.nRawSize = nRawSize;
        member.nUnixTime = qFromLittleEndian<quint32>(pRecord + layout.nTimeOffset);
        member.sFileName = igf2RawNameToString(baName, pListMembers->size());
        pListMembers->append(member);
        if (pListMembers->size() > IGF2_MAX_MEMBERS) return false;

        nPosition = nDataOffset + nPackedSize;
    }
    return false;
}

bool XIGF2::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, true, pPdStruct);
    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XIGF2::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIGF2 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIGF2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIGF2(pDevice);
}

QList<QString> XIGF2::getSearchSignatures()
{
    return {QStringLiteral("2413")};
}

XBinary::FT XIGF2::getFileType()
{
    return FT_IGF2;
}

XBinary::MODE XIGF2::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIGF2::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIGF2::getArch()
{
    return QString();
}

QString XIGF2::getFileFormatExt()
{
    return QStringLiteral("igf");
}

QString XIGF2::getFileFormatExtsString()
{
    return QStringLiteral("IGF installer container (*.igf)");
}

QString XIGF2::getMIMEString()
{
    return QStringLiteral("application/x-igf");
}

QString XIGF2::getVersion()
{
    return QString();
}

qint64 XIGF2::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, true, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XIGF2::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIGF2::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XIGF2::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIGF2::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, false, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = IGF2_HEADER_SIZE;
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
            part.nFileSize = member.nPackedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nRawSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      ((member.nRawSize == 0) && (member.nPackedSize == 0)) ? HANDLE_METHOD_STORE : HANDLE_METHOD_LZH4);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LHA -lh4-"));
            if (member.nUnixTime != 0) {
                part.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch(static_cast<qint64>(member.nUnixTime), X_UTC_TZ));
            }
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = (member.nDataOffset - member.nRecordOffset) + member.nPackedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XIGF2::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIGF2::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, false, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("IGF installer container"));
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

XBinary::ARCHIVERECORD XIGF2::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nRawSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                ((member.nRawSize == 0) && (member.nPackedSize == 0)) ? HANDLE_METHOD_STORE : HANDLE_METHOD_LZH4);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LHA -lh4-"));
    if (member.nUnixTime != 0) {
        result.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch(static_cast<qint64>(member.nUnixTime), X_UTC_TZ));
    }
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XIGF2::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XIGF2::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
