/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xqip1.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 QIP1_RECORD_SIZE = 32;
const quint16 QIP1_MAGIC = 0x4451U;  // 'Q','D'
const qint32 QIP1_NAME_SIZE = 13;
const qint32 QIP1_MAX_MEMBERS = 100000;
const qint64 QIP1_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB sanity cap

bool qip1IsValidName(const QByteArray &baName)
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

XQIP1::XQIP1(QIODevice *pDevice) : XArchive(pDevice)
{
}

XQIP1::~XQIP1()
{
}

bool XQIP1::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XQIP1> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (QIP1_RECORD_SIZE + 2)) return false;

    qint64 nOffset = 0;
    while (nOffset < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= QIP1_MAX_MEMBERS) return false;
        // A partial record at the end means the chain does not tile the file,
        // which for a container with a 2-byte magic is reason enough to say no.
        if ((context.nInputSize - nOffset) < QIP1_RECORD_SIZE) return false;

        const QByteArray baRecord =
            read_array_process(nOffset, QIP1_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baRecord.size() != QIP1_RECORD_SIZE)) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());

        if (qFromLittleEndian<quint16>(pRecord) != QIP1_MAGIC) return false;
        if (qFromLittleEndian<quint16>(pRecord + 2) != 0) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nCompressedSize =
            qint64(qint32(qFromLittleEndian<quint32>(pRecord + 4)));
        member.nSequence = qFromLittleEndian<quint16>(pRecord + 8);
        member.nFlags = quint8(pRecord[10]);
        member.nDosTime = qFromLittleEndian<quint16>(pRecord + 11);
        member.nDosDate = qFromLittleEndian<quint16>(pRecord + 13);
        member.nUncompressedSize =
            qint64(qint32(qFromLittleEndian<quint32>(pRecord + 15)));

        if ((member.nCompressedSize < 0) || (member.nUncompressedSize < 0) ||
            (member.nUncompressedSize > QIP1_MAX_UNCOMPRESSED_SIZE)) {
            return false;
        }
        // Only the first record's sequence number is fixed by the format; the
        // rest simply count up, and nothing downstream depends on them.
        if (context.listMembers.isEmpty() && (member.nSequence != 1)) {
            return false;
        }
        if (member.nSequence == 0) return false;

        QByteArray baName(reinterpret_cast<const char *>(pRecord + 19),
                          QIP1_NAME_SIZE);
        baName[QIP1_NAME_SIZE - 1] = char(0);
        const qint32 nNul = baName.indexOf(char(0));
        if (nNul >= 0) baName = baName.left(nNul);
        if (!qip1IsValidName(baName)) return false;
        member.sFileName = QString::fromLatin1(baName);

        member.nDataOffset = nOffset + QIP1_RECORD_SIZE;
        if (member.nCompressedSize >
            (context.nInputSize - member.nDataOffset)) {
            return false;
        }
        // Both sizes are stated, so an empty member would mean an empty DCL
        // stream, which cannot exist (the 2-byte codec header alone is bigger).
        if (member.nCompressedSize < 2) return false;

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;
    if (nOffset != context.nInputSize) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    context.nArchiveSize = nOffset;
    *pContext = context;
    return true;
}

bool XQIP1::isValid(PDSTRUCT *pPdStruct)
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

bool XQIP1::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XQIP1 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XQIP1::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XQIP1(pDevice);
}

XBinary::FT XQIP1::getFileType()
{
    return FT_QIP1;
}

XBinary::MODE XQIP1::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XQIP1::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XQIP1::getArch()
{
    return QString();
}

QString XQIP1::getFileFormatExt()
{
    return QStringLiteral("qip");
}

QString XQIP1::getFileFormatExtsString()
{
    return QStringLiteral("Quarterdeck install archive (*.qip)");
}

QString XQIP1::getMIMEString()
{
    return QStringLiteral("application/x-qip");
}

QString XQIP1::getVersion()
{
    // The u16 at +2 is 0 in every record; the sequence number at +8 is the only
    // other fixed field, so there is no version to report beyond the variant.
    return QStringLiteral("1");
}

qint64 XQIP1::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XQIP1::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XQIP1::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XQIP1::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XQIP1::getFileParts(quint32 nFileParts, qint32 nLimit,
                                          PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = QIP1_RECORD_SIZE;
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
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(
                FPART_PROP_HANDLEMETHOD,
                (member.nUncompressedSize == 0)
                    ? HANDLE_METHOD_STORE
                    : HANDLE_METHOD_PKWARE_DCL_IMPLODE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("PKWARE DCL implode"));
            const QDateTime dtMTime =
                dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
            if (dtMTime.isValid()) {
                part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = QIP1_RECORD_SIZE + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
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
        // parseContext() requires the record chain to end exactly at the end of
        // the file, so this cannot fire today; it is kept so the part list stays
        // correct if that rule is ever relaxed.
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

QMap<XBinary::UNPACK_PROP, QVariant> XQIP1::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XQIP1::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QPointer<XQIP1> guardedThis(this);
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
        tr("Quarterdeck QIP install archive; PKWARE DCL imploded members"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
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

XBinary::ARCHIVERECORD XQIP1::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

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
                                    : HANDLE_METHOD_PKWARE_DCL_IMPLODE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("PKWARE DCL implode"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime =
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    // No CRC exists anywhere in the record.
    return result;
}

bool XQIP1::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XQIP1::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
