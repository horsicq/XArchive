/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsci.h"

#include <QPointer>
#include <QStringList>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 SCI_HEADER_SIZE = 46;
const qint64 SCI_NAME_SIZE = 50;
const qint64 SCI_RECORD_SIZE = 1 + SCI_NAME_SIZE + 4;  // 55
// The container has no member count; this only bounds a runaway chain.
const qint32 SCI_MAX_MEMBERS = 65536;

// Path components are DOS names separated by backslashes.  The result is
// normalised to '/' with the leading separator dropped, and anything that could
// climb out of the output directory is refused outright.
bool sciNormalizeName(const QByteArray &baRaw, QString *pResult)
{
    if (baRaw.isEmpty()) return false;
    QString sName;
    for (qint32 i = 0; i < baRaw.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRaw.at(i));
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
        if ((nCharacter == '/') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') || (nCharacter == '<') ||
            (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
        sName.append(QLatin1Char(static_cast<char>(nCharacter == '\\' ? '/' : nCharacter)));
    }
    while (sName.startsWith(QLatin1Char('/'))) sName.remove(0, 1);
    if (sName.isEmpty()) return false;
    const QStringList listParts = sName.split(QLatin1Char('/'));
    for (qint32 i = 0; i < listParts.size(); i++) {
        const QString &sPart = listParts.at(i);
        if (sPart.isEmpty() || (sPart == QStringLiteral(".")) || (sPart == QStringLiteral(".."))) return false;
    }
    *pResult = sName;
    return true;
}
}  // namespace

XSCI::XSCI(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSCI::~XSCI()
{
}

bool XSCI::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSCI> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SCI_HEADER_SIZE + SCI_RECORD_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, SCI_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SCI_HEADER_SIZE)) return false;
    // Exactly the fields the reference detector pins; the banner text between
    // them differs between the 1.00 and 2.00 writers.
    const bool bTag = (memcmp(baHeader.constData(), "SCI1", 4) == 0) || (memcmp(baHeader.constData(), "SCI2", 4) == 0);
    if (!bTag) return false;
    if (memcmp(baHeader.constData() + 4, "00 -", 4) != 0) return false;
    if (memcmp(baHeader.constData() + 0x28, "nd.\r\n\x1a", 6) != 0) return false;
    context.sBanner = QString::fromLatin1(baHeader.left(43)).trimmed();

    qint64 nOffset = SCI_HEADER_SIZE;
    for (qint32 i = 0; i < SCI_MAX_MEMBERS; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset >= context.nInputSize) break;
        if (nOffset + SCI_RECORD_SIZE > context.nInputSize) {
            context.bTruncated = true;
            break;
        }
        const QByteArray baRecord = read_array_process(nOffset, SCI_RECORD_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baRecord.size() != SCI_RECORD_SIZE)) return false;

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nKind = static_cast<quint8>(baRecord.at(0));

        // Only the bytes before the first NUL are the name; the rest of the
        // 50-byte field is uninitialised writer scratch and must be ignored.
        const QByteArray baNameField = baRecord.mid(1, qint32(SCI_NAME_SIZE));
        qint32 nNameLength = baNameField.indexOf('\0');
        if (nNameLength < 0) nNameLength = qint32(SCI_NAME_SIZE);
        if (!sciNormalizeName(baNameField.left(nNameLength), &member.sFileName)) return false;

        member.nSize = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baRecord.constData()) + 1 + SCI_NAME_SIZE);
        if (member.nSize < 0) return false;
        member.nDataOffset = nOffset + SCI_RECORD_SIZE;
        if (member.nDataOffset + member.nSize > context.nInputSize) {
            // Cut off by the end of the file: publish only the intact members.
            context.bTruncated = true;
            break;
        }
        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nSize;
    }

    if (context.listMembers.isEmpty()) return false;
    context.nArchiveSize = nOffset;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XSCI::isValid(PDSTRUCT *pPdStruct)
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

bool XSCI::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSCI archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSCI::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSCI(pDevice);
}

QList<QString> XSCI::getSearchSignatures()
{
    // The version digit at +3 is '1' or '2'; everything after "00 - " differs
    // between the two writers, so the fixed tail at +0x28 is left to isValid.
    return {QStringLiteral("'SCI'..'00 -'")};
}

XBinary::FT XSCI::getFileType()
{
    return FT_SCI;
}

XBinary::MODE XSCI::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSCI::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSCI::getArch()
{
    return QString();
}

QString XSCI::getFileFormatExt()
{
    return QStringLiteral("sxd");
}

QString XSCI::getFileFormatExtsString()
{
    return QStringLiteral("SCI archive (*.sxd *.sxp)");
}

QString XSCI::getMIMEString()
{
    return QStringLiteral("application/x-sci-sixxac");
}

QString XSCI::getVersion()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return QString();
    const QByteArray baTag = read_array_process(0, 6, nullptr);
    if (baTag.size() != 6) return QString();
    if ((memcmp(baTag.constData(), "SCI1", 4) != 0) && (memcmp(baTag.constData(), "SCI2", 4) != 0)) return QString();
    return QString::fromLatin1(baTag);
}

qint64 XSCI::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSCI::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSCI::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XSCI::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSCI::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SCI_HEADER_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XSCI::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSCI::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSCI> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("SCI archive"));
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

XBinary::ARCHIVERECORD XSCI::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_TYPE, static_cast<quint32>(member.nKind));
    return result;
}

bool XSCI::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XSCI::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
