/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xis11.h"

#include <QtEndian>

#include <new>

#include "Algos/xis11decoder.h"

namespace {
const qint64 IS11_HEADER_SIZE = 13;
const qint64 IS11_MEMBER_HEADER_SIZE = 12;
const quint32 IS11_MAGIC = 0x8C135D65U;
const quint32 IS11_FORMAT = 0x00010108U;
// Every archive in the reference corpus is a short chain (1 or 2 members); a
// four digit ceiling only exists to bound a corrupt nNextOffset ring.
const qint32 IS11_MAX_MEMBERS = 4096;
const qint64 IS11_MAX_UNCOMPRESSED = 64 * 1024 * 1024;

bool is11RangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XIS11::XIS11(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIS11::~XIS11()
{
}

// Names are DOS 8.3 identifiers, occasionally NUL terminated inside the fixed
// field.  Everything from the first NUL on is padding; the remaining bytes are
// escaped as %XX when they are not safe in a file name, which is reversible and
// cannot collapse two members onto one output file.
QString XIS11::rawNameToString(const QByteArray &baRawName, qint32 nIndex)
{
    qint32 nLength = 0;
    while ((nLength < baRawName.size()) && (baRawName.at(nLength) != '\0')) nLength++;
    while ((nLength > 0) && (baRawName.at(nLength - 1) == ' ')) nLength--;

    QString sResult;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(baRawName.at(i));
        const bool bSafe = (nCharacter > 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != '/') && (nCharacter != '\\') &&
                           (nCharacter != ':') && (nCharacter != '*') && (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') &&
                           (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("record%1").arg(nIndex);
    }
    return sResult;
}

bool XIS11::resolveSize(MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember) return false;
    if (pMember->nUncompressedSize >= 0) return true;
    if (pMember->nPackedSize <= 0) {
        pMember->nUncompressedSize = 0;
        return true;
    }
    QIODevice *guardedSource = getDevice();
    const QByteArray baPacked = read_array_process(pMember->nDataOffset, pMember->nPackedSize, pPdStruct);
    if ((baPacked.size() != pMember->nPackedSize)) return false;

    QByteArray baUnpacked;
    if (!XIS11Decoder::decodeStream(baPacked, &baUnpacked, IS11_MAX_UNCOMPRESSED, pPdStruct)) return false;
    pMember->nUncompressedSize = baUnpacked.size();
    return true;
}

bool XIS11::parseContext(CONTEXT *pContext, bool bResolveSizes, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < IS11_HEADER_SIZE + IS11_MEMBER_HEADER_SIZE + 2) return false;

    const QByteArray baHeader = read_array_process(0, IS11_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != IS11_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (qFromLittleEndian<quint32>(pHeader) != IS11_MAGIC) return false;
    if (qFromLittleEndian<quint32>(pHeader + 4) != IS11_FORMAT) return false;
    const quint8 nVariant = static_cast<quint8>(pHeader[8]);
    if ((nVariant != 1) && (nVariant != 2)) return false;
    if (qFromLittleEndian<quint32>(pHeader + 9) != 0) return false;
    context.nVariant = nVariant;

    qint64 nOffset = IS11_HEADER_SIZE;
    while (true) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= IS11_MAX_MEMBERS) return false;
        if (!is11RangeWithin(context.nInputSize, nOffset, IS11_MEMBER_HEADER_SIZE)) return false;

        const QByteArray baMember = read_array_process(nOffset, IS11_MEMBER_HEADER_SIZE, pPdStruct);
        if ((baMember.size() != IS11_MEMBER_HEADER_SIZE)) return false;
        const uchar *pMemberHeader = reinterpret_cast<const uchar *>(baMember.constData());

        const quint8 nFlags = static_cast<quint8>(pMemberHeader[0]);
        const qint32 nPackedSize = qFromLittleEndian<qint32>(pMemberHeader + 1);
        const qint32 nNextOffset = qFromLittleEndian<qint32>(pMemberHeader + 5);
        const qint32 nNameLength = static_cast<qint32>(pMemberHeader[11]);
        if ((nPackedSize < 0) || (nNextOffset < 0) || (nNameLength == 0)) return false;

        const qint64 nNameOffset = nOffset + IS11_MEMBER_HEADER_SIZE;
        // The name field is followed by one separator byte before the stream.
        if (!is11RangeWithin(context.nInputSize, nNameOffset, static_cast<qint64>(nNameLength) + 1)) return false;

        const QByteArray baName = read_array_process(nNameOffset, nNameLength, pPdStruct);
        if ((baName.size() != nNameLength)) return false;
        if (static_cast<quint8>(baName.at(0)) < 0x20) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nNameOffset + nNameLength + 1;
        member.nPackedSize = nPackedSize;
        member.nUncompressedSize = -1;
        member.nFlags = nFlags;
        member.sFileName = rawNameToString(baName, context.listMembers.size());

        if (!is11RangeWithin(context.nInputSize, member.nDataOffset, member.nPackedSize)) return false;
        const qint64 nEnd = member.nDataOffset + member.nPackedSize;

        context.listMembers.append(member);

        // The chain has to tile the file exactly.  This is what makes the
        // detector safe: a coincidental 13-byte magic match cannot survive it.
        if (nNextOffset == 0) {
            if (nEnd != context.nInputSize) return false;
            break;
        }
        if (static_cast<qint64>(nNextOffset) != nEnd) return false;
        nOffset = nEnd;
    }

    if (context.listMembers.isEmpty()) return false;

    if (bResolveSizes) {
        for (qint32 i = 0; i < context.listMembers.size(); i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (!resolveSize(&context.listMembers[i], pPdStruct)) return false;
        }
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XIS11::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XIS11::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIS11 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIS11::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIS11(pDevice);
}

QList<QString> XIS11::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("655D138C"));  // 0x8C135D65 little endian at offset 0
    return listResult;
}

XBinary::FT XIS11::getFileType()
{
    return FT_IS11;
}

XBinary::MODE XIS11::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIS11::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIS11::getArch()
{
    return QString();
}

QString XIS11::getFileFormatExt()
{
    return QStringLiteral("ex$");
}

QString XIS11::getFileFormatExtsString()
{
    return QStringLiteral("InstallShield compressed file (*.ex$ *.dl$ *.hl$ *.$$$)");
}

QString XIS11::getMIMEString()
{
    return QStringLiteral("application/x-installshield-compressed");
}

QString XIS11::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();
    return QString::number(context.nVariant);
}

qint64 XIS11::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XIS11::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIS11::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XIS11::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIS11::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = IS11_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IS11);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZW"));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = (member.nDataOffset + member.nPackedSize) - member.nHeaderOffset;
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

QMap<XBinary::UNPACK_PROP, QVariant> XIS11::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIS11::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, true, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("InstallShield compressed file"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XIS11::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (member.nUncompressedSize < 0) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IS11);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZW"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XIS11::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XIS11::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
