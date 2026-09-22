/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xinteduft.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 INTEDUFT_HEADER_SIZE = 6;
const qint64 INTEDUFT_MEMBER_HEADER_SIZE = 16;
const quint32 INTEDUFT_MAGIC = 0x04072E7CU;
const qint32 INTEDUFT_MAX_NAME = 12;
const quint16 INTEDUFT_METHOD_STORE = 0;
const quint16 INTEDUFT_METHOD_DEFLATE = 8;

bool inteduRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XINTEDUFT::XINTEDUFT(QIODevice *pDevice) : XArchive(pDevice)
{
}

XINTEDUFT::~XINTEDUFT()
{
}

QString XINTEDUFT::rawNameToString(const QByteArray &baRawName, qint32 nIndex)
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

bool XINTEDUFT::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < INTEDUFT_HEADER_SIZE + 6 + INTEDUFT_MEMBER_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, INTEDUFT_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != INTEDUFT_HEADER_SIZE)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    if (qFromLittleEndian<quint32>(pHeader) != INTEDUFT_MAGIC) return false;
    const qint32 nCount = static_cast<qint32>(qFromLittleEndian<quint16>(pHeader + 4));
    if (nCount < 1) return false;

    // Every index entry is 5 bytes plus at most a 12-byte name.
    const qint64 nIndexBound = qMin<qint64>(context.nInputSize - INTEDUFT_HEADER_SIZE, static_cast<qint64>(nCount) * (5 + INTEDUFT_MAX_NAME));
    if (nIndexBound < static_cast<qint64>(nCount) * 6) return false;
    const QByteArray baIndex = read_array_process(INTEDUFT_HEADER_SIZE, nIndexBound, pPdStruct);
    if ((baIndex.size() != nIndexBound)) return false;

    struct RAWENTRY {
        qint64 nOffset;
        QByteArray baName;
    };
    QList<RAWENTRY> listRaw;
    qint64 nIndexPos = 0;
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nIndexPos + 5 > nIndexBound) return false;
        const uchar *pEntry = reinterpret_cast<const uchar *>(baIndex.constData()) + nIndexPos;
        const qint32 nOffset = qFromLittleEndian<qint32>(pEntry);
        const qint32 nNameLength = static_cast<qint32>(pEntry[4]);
        if ((nOffset <= 0) || (nNameLength < 1) || (nNameLength > INTEDUFT_MAX_NAME)) return false;
        if (nIndexPos + 5 + nNameLength > nIndexBound) return false;
        RAWENTRY raw;
        raw.nOffset = nOffset;
        raw.baName = baIndex.mid(static_cast<qint32>(nIndexPos) + 5, nNameLength);
        if (static_cast<quint8>(raw.baName.at(0)) < 0x20) return false;
        listRaw.append(raw);
        nIndexPos += 5 + nNameLength;
    }
    context.nIndexSize = INTEDUFT_HEADER_SIZE + nIndexPos;

    qint64 nMaxEnd = context.nIndexSize;
    for (qint32 i = 0; i < listRaw.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const RAWENTRY &raw = listRaw.at(i);
        if (raw.nOffset < context.nIndexSize) return false;
        if (!inteduRangeWithin(context.nInputSize, raw.nOffset, INTEDUFT_MEMBER_HEADER_SIZE)) return false;

        const QByteArray baMember = read_array_process(raw.nOffset, INTEDUFT_MEMBER_HEADER_SIZE, pPdStruct);
        if ((baMember.size() != INTEDUFT_MEMBER_HEADER_SIZE)) return false;
        const uchar *pMember = reinterpret_cast<const uchar *>(baMember.constData());

        const quint16 nMethod = qFromLittleEndian<quint16>(pMember + 2);
        const quint32 nCRC32 = qFromLittleEndian<quint32>(pMember + 4);
        const qint32 nPackedSize = qFromLittleEndian<qint32>(pMember + 8);
        const qint32 nUncompressedSize = qFromLittleEndian<qint32>(pMember + 12);
        if ((nPackedSize < 0) || (nUncompressedSize < 0)) return false;
        if ((nMethod != INTEDUFT_METHOD_STORE) && (nMethod != INTEDUFT_METHOD_DEFLATE)) return false;
        if ((nMethod == INTEDUFT_METHOD_STORE) && (nPackedSize != nUncompressedSize)) return false;

        MEMBER member = {};
        member.nHeaderOffset = raw.nOffset;
        member.nDataOffset = raw.nOffset + INTEDUFT_MEMBER_HEADER_SIZE;
        member.nPackedSize = nPackedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nCRC32 = nCRC32;
        member.nMethod = nMethod;
        member.sFileName = rawNameToString(raw.baName, i);
        if (!inteduRangeWithin(context.nInputSize, member.nDataOffset, member.nPackedSize)) return false;

        const qint64 nEnd = member.nDataOffset + member.nPackedSize;
        if (nEnd > nMaxEnd) nMaxEnd = nEnd;
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nMaxEnd;
    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XINTEDUFT::isValid(PDSTRUCT *pPdStruct)
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

bool XINTEDUFT::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XINTEDUFT archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XINTEDUFT::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XINTEDUFT(pDevice);
}

QList<QString> XINTEDUFT::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("7C2E0704"));  // 0x04072E7C little endian at offset 0
    return listResult;
}

XBinary::FT XINTEDUFT::getFileType()
{
    return FT_INTEDU_FT;
}

XBinary::MODE XINTEDUFT::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XINTEDUFT::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XINTEDUFT::getArch()
{
    return QString();
}

QString XINTEDUFT::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XINTEDUFT::getFileFormatExtsString()
{
    return QStringLiteral("Resource pack (*.dat)");
}

QString XINTEDUFT::getMIMEString()
{
    return QStringLiteral("application/x-inteduft");
}

QString XINTEDUFT::getVersion()
{
    return QString();
}

qint64 XINTEDUFT::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XINTEDUFT::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XINTEDUFT::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XINTEDUFT::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XINTEDUFT::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nIndexSize;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      (member.nMethod == INTEDUFT_METHOD_DEFLATE) ? HANDLE_METHOD_DEFLATE : HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      (member.nMethod == INTEDUFT_METHOD_DEFLATE) ? QStringLiteral("Deflate") : QStringLiteral("Stored"));
            part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = INTEDUFT_MEMBER_HEADER_SIZE + member.nPackedSize;
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

QMap<XBinary::UNPACK_PROP, QVariant> XINTEDUFT::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XINTEDUFT::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Resource pack"));
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

XBinary::ARCHIVERECORD XINTEDUFT::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.nStreamSize = member.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nMethod == INTEDUFT_METHOD_DEFLATE) ? HANDLE_METHOD_DEFLATE : HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                (member.nMethod == INTEDUFT_METHOD_DEFLATE) ? QStringLiteral("Deflate") : QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XINTEDUFT::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XINTEDUFT::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
