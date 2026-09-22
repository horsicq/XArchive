/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpackit.h"

#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 PACKIT_SIGNATURE_SIZE = 16;
// Tag, both size copies, date, time, CRC and the name-length byte.
const qint64 PACKIT_FIXED_HEADER_SIZE = 19;
const quint16 PACKIT_TAG_MEMBER = 0x00ffU;
const quint16 PACKIT_TAG_END = 0xffffU;
// A guard against a corrupt chain spinning forever; the largest real archive in
// the reference corpus holds 20 members.
const qint32 PACKIT_MAX_MEMBERS = 65536;

bool packitIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }
    if ((baName == QByteArrayLiteral(".")) || (baName == QByteArrayLiteral(".."))) return false;
    return true;
}
}  // namespace

XPACKIT::XPACKIT(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPACKIT::~XPACKIT()
{
}

bool XPACKIT::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PACKIT_SIGNATURE_SIZE + PACKIT_FIXED_HEADER_SIZE + 2) return false;

    const QByteArray baSignature = read_array_process(0, PACKIT_SIGNATURE_SIZE, pPdStruct);
    if (baSignature.size() != PACKIT_SIGNATURE_SIZE) return false;
    {
        static const char pSignature[PACKIT_SIGNATURE_SIZE] = {'P', 'A', 'C', 'K', 'I', 'T', ' ', 'b', 'y', ' ', 'M', 'J', 'P', '\r', '\n', '\x1a'};
        if (memcmp(baSignature.constData(), pSignature, PACKIT_SIGNATURE_SIZE) != 0) return false;
    }

    qint64 nOffset = PACKIT_SIGNATURE_SIZE;
    bool bTerminated = false;
    for (qint32 i = 0; i < PACKIT_MAX_MEMBERS; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset + 2 > context.nInputSize) break;

        const QByteArray baTag = read_array_process(nOffset, 2, pPdStruct);
        if (baTag.size() != 2) return false;
        const quint16 nTag = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baTag.constData()));
        if (nTag == PACKIT_TAG_END) {
            nOffset += 2;
            bTerminated = true;
            break;
        }
        // Any other tag is a hard structural error, not the end of the chain.
        if (nTag != PACKIT_TAG_MEMBER) return false;

        if (nOffset + PACKIT_FIXED_HEADER_SIZE > context.nInputSize) break;
        const QByteArray baHeader = read_array_process(nOffset, PACKIT_FIXED_HEADER_SIZE, pPdStruct);
        if (baHeader.size() != PACKIT_FIXED_HEADER_SIZE) return false;
        const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

        const qint32 nSize = qFromLittleEndian<qint32>(pHeader + 2);
        const qint32 nSizeCopy = qFromLittleEndian<qint32>(pHeader + 6);
        // Both copies of the size have to agree; that redundancy is the only
        // per-member integrity field the container checks up front.
        if ((nSize < 0) || (nSizeCopy < 0) || (nSize != nSizeCopy)) return false;

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nDosDate = qFromLittleEndian<quint16>(pHeader + 10);
        member.nDosTime = qFromLittleEndian<quint16>(pHeader + 12);
        member.nCrc32Register = qFromLittleEndian<quint32>(pHeader + 14);
        const qint32 nNameLength = pHeader[18];
        if (nNameLength == 0) return false;

        const qint64 nNameOffset = nOffset + PACKIT_FIXED_HEADER_SIZE;
        if (nNameOffset + nNameLength + 1 > context.nInputSize) break;
        const QByteArray baName = read_array_process(nNameOffset, nNameLength + 1, pPdStruct);
        if (baName.size() != nNameLength + 1) return false;
        // The name field is followed by an explicit NUL; the reference reader
        // refuses the archive when it is anything else.
        if (baName.at(nNameLength) != '\0') return false;
        const QByteArray baRawName = baName.left(nNameLength);
        if (!packitIsValidName(baRawName)) return false;
        member.sFileName = QString::fromLatin1(baRawName);

        member.nDataOffset = nNameOffset + nNameLength + 1;
        member.nSize = nSize;
        if (member.nDataOffset + member.nSize > context.nInputSize) {
            // A member cut off by the end of the file.  The reference reader
            // writes the bytes that survive and only then reports the archive
            // as damaged, so publish this member clamped to what is actually
            // present and stop the chain here.  Its stored CRC-32 will not
            // match, which is how a consumer learns the member is short.
            context.bTruncated = true;
            member.nSize = context.nInputSize - member.nDataOffset;
            if (member.nSize > 0) {
                context.listMembers.append(member);
                nOffset = context.nInputSize;
            }
            break;
        }
        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nSize;
    }

    if (context.listMembers.isEmpty()) return false;
    if (!bTerminated) context.bTruncated = true;
    context.nArchiveSize = nOffset;
    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XPACKIT::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XPACKIT::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPACKIT archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPACKIT::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPACKIT(pDevice);
}

QList<QString> XPACKIT::getSearchSignatures()
{
    return {QStringLiteral("'PACKIT by MJP'0D0A1A")};
}

XBinary::FT XPACKIT::getFileType()
{
    return FT_PACKIT;
}

XBinary::MODE XPACKIT::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPACKIT::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPACKIT::getArch()
{
    return QString();
}

QString XPACKIT::getFileFormatExt()
{
    return QStringLiteral("ins");
}

QString XPACKIT::getFileFormatExtsString()
{
    return QStringLiteral("PACKIT archive (*.ins *.dat)");
}

QString XPACKIT::getMIMEString()
{
    return QStringLiteral("application/x-packit");
}

QString XPACKIT::getVersion()
{
    return QString();
}

qint64 XPACKIT::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPACKIT::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPACKIT::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XPACKIT::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPACKIT::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PACKIT_SIGNATURE_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XPACKIT::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPACKIT::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("PACKIT archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XPACKIT::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    // PACKIT stores the CRC-32 register without the final complement, which is
    // exactly CRC_TYPE_FFFFFFFF_EDB88320_00000000.
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCrc32Register);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_00000000);
    const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XPACKIT::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XPACKIT::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
