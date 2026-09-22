/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xjgpak.h"

#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 JGPAK_SIGNATURE_SIZE = 7;
const qint64 JGPAK_TAIL_SIZE = 24;  // fixed part of a directory record
// The member count is a signed 32-bit field, but a directory of more than this
// many records cannot be produced by the writer: every member costs at least 26
// directory bytes plus one payload byte.
const qint32 JGPAK_MAX_MEMBERS = 65535;
// Both leading strings are short product/version banners in every known
// archive; the writer never emits anything approaching this.
const qint32 JGPAK_MAX_STRING = 4096;

bool jgpakRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// Member names are bare DOS/Windows file names - no directory component ever
// appears - so anything that could escape the output directory, or any control
// byte, means this is not a directory record.
bool jgpakIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if (nCharacter < 0x20) return false;
        if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') || (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
            (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|')) {
            return false;
        }
    }
    if ((baName == QByteArrayLiteral(".")) || (baName == QByteArrayLiteral(".."))) return false;
    return true;
}
}  // namespace

XJGPAK::XJGPAK(QIODevice *pDevice) : XArchive(pDevice)
{
}

XJGPAK::~XJGPAK()
{
}

bool XJGPAK::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Signature, two empty strings, the flag byte and the member count.
    if (context.nInputSize < JGPAK_SIGNATURE_SIZE + 4 + 4 + 1 + 4) return false;

    const QByteArray baSignature = read_array_process(0, JGPAK_SIGNATURE_SIZE, pPdStruct);
    if ((baSignature.size() != JGPAK_SIGNATURE_SIZE)) return false;
    {
        static const char pSignature[JGPAK_SIGNATURE_SIZE] = {'J', 'G', 'P', 'A', 'K', '\x00', '\x01'};
        if (memcmp(baSignature.constData(), pSignature, JGPAK_SIGNATURE_SIZE) != 0) return false;
    }

    qint64 nOffset = JGPAK_SIGNATURE_SIZE;
    QString sBanner[2];
    for (qint32 i = 0; i < 2; i++) {
        const QByteArray baLength = read_array_process(nOffset, 4, pPdStruct);
        if ((baLength.size() != 4)) return false;
        const qint32 nLength = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baLength.constData()));
        if ((nLength < 0) || (nLength > JGPAK_MAX_STRING)) return false;
        nOffset += 4;
        if (!jgpakRangeWithin(context.nInputSize, nOffset, nLength)) return false;
        const QByteArray baText = read_array_process(nOffset, nLength, pPdStruct);
        if ((baText.size() != nLength)) return false;
        // The banners are plain 8-bit text; a control byte here means the file
        // only happens to start with the signature.
        for (qint32 j = 0; j < baText.size(); j++) {
            const quint8 nCharacter = static_cast<quint8>(baText.at(j));
            if ((nCharacter < 0x20) && (nCharacter != '\t')) return false;
        }
        sBanner[i] = QString::fromLatin1(baText);
        nOffset += nLength;
    }
    context.sDescription = sBanner[0];
    context.sVersion = sBanner[1];

    const QByteArray baCount = read_array_process(nOffset, 5, pPdStruct);
    if ((baCount.size() != 5)) return false;
    const qint32 nNumberOfMembers = qFromLittleEndian<qint32>(reinterpret_cast<const uchar *>(baCount.constData()) + 1);
    if ((nNumberOfMembers < 0) || (nNumberOfMembers > JGPAK_MAX_MEMBERS)) return false;
    nOffset += 5;
    context.nDirectoryOffset = nOffset;

    // An empty archive carries no payload at all, so the whole file is header.
    if (nNumberOfMembers == 0) {
        context.nDirectoryEnd = nOffset;
        context.nArchiveSize = nOffset;
        *pContext = context;
        return isPdStructNotCanceled(pPdStruct);
    }

    // Upper bound for the directory: 1 length byte + a 255-byte name + the
    // 24-byte tail per member.  Reading it in one go keeps the parse to two
    // device reads even for the largest producer-reachable directory.
    const qint64 nDirectoryBudget = qMin<qint64>(context.nInputSize - nOffset, qint64(nNumberOfMembers) * (1 + 255 + JGPAK_TAIL_SIZE));
    if (nDirectoryBudget < qint64(nNumberOfMembers) * (1 + 1 + JGPAK_TAIL_SIZE)) return false;
    const QByteArray baDirectory = read_array_process(nOffset, nDirectoryBudget, pPdStruct);
    if ((baDirectory.size() != nDirectoryBudget)) return false;

    qint64 nCursor = 0;
    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nCursor + 1 > nDirectoryBudget) return false;
        const qint32 nNameLength = static_cast<quint8>(baDirectory.at(qint32(nCursor)));
        if (nNameLength == 0) return false;
        if (nCursor + 1 + nNameLength + JGPAK_TAIL_SIZE > nDirectoryBudget) return false;

        MEMBER member = {};
        member.nRecordOffset = nOffset + nCursor;
        const QByteArray baName = baDirectory.mid(qint32(nCursor) + 1, nNameLength);
        if (!jgpakIsValidName(baName)) return false;
        member.sFileName = QString::fromLatin1(baName);

        const uchar *pTail = reinterpret_cast<const uchar *>(baDirectory.constData()) + nCursor + 1 + nNameLength;
        member.nDosTime = qFromLittleEndian<quint16>(pTail);
        member.nDosDate = qFromLittleEndian<quint16>(pTail + 2);
        member.nUncompressedSize = qFromLittleEndian<qint32>(pTail + 4);
        member.nDataOffset = qFromLittleEndian<qint32>(pTail + 8);
        member.nCompressedSize = qFromLittleEndian<qint32>(pTail + 12);
        member.nCrc32 = qFromLittleEndian<quint32>(pTail + 16);
        // The reference reader refuses any of the three negative.
        if ((member.nUncompressedSize < 0) || (member.nDataOffset < 0) || (member.nCompressedSize < 0)) return false;
        if (!jgpakRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;
        context.listMembers.append(member);

        nCursor += 1 + nNameLength + JGPAK_TAIL_SIZE;
    }
    context.nDirectoryEnd = nOffset + nCursor;

    // The payload area tiles exactly: the first member starts at the byte after
    // the directory and each following one starts where the previous ended.
    // This is the structural check that makes the short signature safe.
    qint64 nExpected = context.nDirectoryEnd;
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        const MEMBER &member = context.listMembers.at(i);
        if (member.nDataOffset != nExpected) return false;
        if (member.nCompressedSize == 0) return false;
        nExpected = member.nDataOffset + member.nCompressedSize;
    }
    if (nExpected > context.nInputSize) return false;
    context.nArchiveSize = nExpected;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XJGPAK::isValid(PDSTRUCT *pPdStruct)
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

bool XJGPAK::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XJGPAK archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XJGPAK::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XJGPAK(pDevice);
}

QList<QString> XJGPAK::getSearchSignatures()
{
    return {QStringLiteral("'JGPAK'0001")};
}

XBinary::FT XJGPAK::getFileType()
{
    return FT_JGPAK;
}

XBinary::MODE XJGPAK::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XJGPAK::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XJGPAK::getArch()
{
    return QString();
}

QString XJGPAK::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XJGPAK::getFileFormatExtsString()
{
    return QStringLiteral("JGPAK archive (*.pak)");
}

QString XJGPAK::getMIMEString()
{
    return QStringLiteral("application/x-jgpak");
}

QString XJGPAK::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return context.sVersion;
}

qint64 XJGPAK::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XJGPAK::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XJGPAK::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XJGPAK::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XJGPAK::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDirectoryEnd;
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
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZH1);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZHUF"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XJGPAK::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XJGPAK::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("JGPAK archive"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
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

XBinary::ARCHIVERECORD XJGPAK::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZH1);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZHUF"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The directory stores the finished CRC-32, not the running register.
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCrc32);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    const QDateTime dtMTime = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    return result;
}

bool XJGPAK::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XJGPAK::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
