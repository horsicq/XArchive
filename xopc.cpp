/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xopc.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 OPC_BANNER_SIZE = 0x50;
const qint64 OPC_TERMINATOR_OFFSET = 0x4F;
const quint8 OPC_TERMINATOR = 0x1A;
const quint8 OPC_SHIFT = 0x67;
const qint64 OPC_EOCD_SIZE = 22;
const qint64 OPC_CDENTRY_SIZE = 46;
const qint64 OPC_LOCAL_SIZE = 30;
// A ZIP end-of-central-directory record may be trailed by a comment of at most
// 65535 bytes, so the record itself starts no earlier than this from the end.
const qint64 OPC_EOCD_SEARCH = 65535 + OPC_EOCD_SIZE;
const qint32 OPC_MAX_ENTRIES = 200000;
const qint64 OPC_MAX_NAME = 4096;

bool opcRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XOPC::XOPC(QIODevice *pDevice) : XArchive(pDevice)
{
}

XOPC::~XOPC()
{
}

QByteArray XOPC::readDeobfuscated(qint64 nZipOffset, qint64 nSize, PDSTRUCT *pPdStruct)
{
    QPointer<XOPC> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || (nZipOffset < 0) || (nSize < 0)) return QByteArray();

    QByteArray baResult = read_array_process(OPC_BANNER_SIZE + nZipOffset, nSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baResult.size() != nSize)) return QByteArray();

    char *pData = baResult.data();
    for (qint64 i = 0; i < nSize; i++) {
        pData[i] = static_cast<char>(static_cast<quint8>(static_cast<quint8>(pData[i]) - OPC_SHIFT));
    }
    return baResult;
}

// ZIP member names are arbitrary bytes.  '/' stays a path separator, everything
// the host filesystem cannot represent is escaped as %XX so the mapping stays
// reversible and cannot collapse two members onto one output file.
QString XOPC::sanitizeName(const QByteArray &baRawName)
{
    QString sResult;
    for (qint32 i = 0; i < baRawName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRawName.at(i));
        if ((nCharacter == '/') || (nCharacter == '\\')) {
            sResult.append(QLatin1Char('/'));
            continue;
        }
        const bool bSafe = (nCharacter >= 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != ':') && (nCharacter != '*') &&
                           (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    return sResult;
}

QByteArray XOPC::methodProperty(quint16 nZipMethod)
{
    QByteArray baResult(1, 0);
    baResult[0] = static_cast<char>(static_cast<quint8>(nZipMethod & 0xFF));
    return baResult;
}

QString XOPC::reportedMethod(quint16 nZipMethod)
{
    if (nZipMethod == 0) return QStringLiteral("Stored (OS2Point)");
    if (nZipMethod == 8) return QStringLiteral("Deflate (OS2Point)");
    return QStringLiteral("ZIP method %1 (OS2Point)").arg(nZipMethod);
}

bool XOPC::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XOPC> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < OPC_BANNER_SIZE + OPC_EOCD_SIZE + OPC_LOCAL_SIZE) return false;

    const QByteArray baBanner = read_array_process(0, OPC_BANNER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baBanner.size() != OPC_BANNER_SIZE)) return false;
    if (memcmp(baBanner.constData(), "OS2POINT", 8) != 0) return false;
    if (static_cast<quint8>(baBanner.at(OPC_TERMINATOR_OFFSET)) != OPC_TERMINATOR) return false;

    {
        QByteArray baText = baBanner.left(static_cast<qint32>(OPC_TERMINATOR_OFFSET));
        while (baText.endsWith('\0') || baText.endsWith(' ')) baText.chop(1);
        context.sBanner = QString::fromLatin1(baText);
    }

    context.nZipSize = context.nInputSize - OPC_BANNER_SIZE;

    // The de-obfuscated payload has to be a real ZIP: locate its end-of-central
    // directory record.  This, together with the local header signature check
    // below, is what makes the eight-byte banner safe to detect on.
    const qint64 nTailSize = qMin(context.nZipSize, OPC_EOCD_SEARCH);
    const QByteArray baTail = readDeobfuscated(context.nZipSize - nTailSize, nTailSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baTail.size() != nTailSize)) return false;

    qint64 nEocdOffset = -1;
    for (qint64 i = nTailSize - OPC_EOCD_SIZE; i >= 0; i--) {
        const uchar *p = reinterpret_cast<const uchar *>(baTail.constData()) + i;
        if ((p[0] == 'P') && (p[1] == 'K') && (p[2] == 5) && (p[3] == 6)) {
            const qint64 nCommentSize = static_cast<qint64>(qFromLittleEndian<quint16>(p + 20));
            if (i + OPC_EOCD_SIZE + nCommentSize <= nTailSize) {
                nEocdOffset = (context.nZipSize - nTailSize) + i;
                break;
            }
        }
    }
    if (nEocdOffset < 0) return false;

    const QByteArray baEocd = readDeobfuscated(nEocdOffset, OPC_EOCD_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baEocd.size() != OPC_EOCD_SIZE)) return false;
    const uchar *pEocd = reinterpret_cast<const uchar *>(baEocd.constData());

    const qint32 nNumberOfEntries = static_cast<qint32>(qFromLittleEndian<quint16>(pEocd + 10));
    const qint64 nDirectorySize = static_cast<qint64>(qFromLittleEndian<quint32>(pEocd + 12));
    const qint64 nDirectoryOffset = static_cast<qint64>(qFromLittleEndian<quint32>(pEocd + 16));
    if ((nNumberOfEntries < 1) || (nNumberOfEntries > OPC_MAX_ENTRIES)) return false;
    if (!opcRangeWithin(context.nZipSize, nDirectoryOffset, nDirectorySize)) return false;
    if (nDirectorySize < static_cast<qint64>(nNumberOfEntries) * OPC_CDENTRY_SIZE) return false;
    if (nDirectoryOffset + nDirectorySize > nEocdOffset) return false;

    const QByteArray baDirectory = readDeobfuscated(nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource || (baDirectory.size() != nDirectorySize)) return false;

    qint64 nCursor = 0;
    for (qint32 i = 0; i < nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nCursor + OPC_CDENTRY_SIZE > nDirectorySize) return false;
        const uchar *pEntry = reinterpret_cast<const uchar *>(baDirectory.constData()) + nCursor;
        if ((pEntry[0] != 'P') || (pEntry[1] != 'K') || (pEntry[2] != 1) || (pEntry[3] != 2)) return false;

        const quint16 nFlags = qFromLittleEndian<quint16>(pEntry + 8);
        const quint16 nMethod = qFromLittleEndian<quint16>(pEntry + 10);
        const qint64 nCompressedSize = static_cast<qint64>(qFromLittleEndian<quint32>(pEntry + 20));
        const qint64 nUncompressedSize = static_cast<qint64>(qFromLittleEndian<quint32>(pEntry + 24));
        const qint64 nNameSize = static_cast<qint64>(qFromLittleEndian<quint16>(pEntry + 28));
        const qint64 nExtraSize = static_cast<qint64>(qFromLittleEndian<quint16>(pEntry + 30));
        const qint64 nCommentSize = static_cast<qint64>(qFromLittleEndian<quint16>(pEntry + 32));
        const qint64 nLocalOffset = static_cast<qint64>(qFromLittleEndian<quint32>(pEntry + 42));

        const qint64 nEntrySize = OPC_CDENTRY_SIZE + nNameSize + nExtraSize + nCommentSize;
        if (nCursor + nEntrySize > nDirectorySize) return false;
        if ((nNameSize < 1) || (nNameSize > OPC_MAX_NAME)) return false;
        // Encrypted members carry no key here and would silently produce noise.
        if (nFlags & 0x0001) return false;
        if (!opcRangeWithin(context.nZipSize, nLocalOffset, OPC_LOCAL_SIZE)) return false;

        const QByteArray baRawName = baDirectory.mid(static_cast<qint32>(nCursor + OPC_CDENTRY_SIZE), static_cast<qint32>(nNameSize));
        if (baRawName.size() != nNameSize) return false;
        nCursor += nEntrySize;

        const QByteArray baLocal = readDeobfuscated(nLocalOffset, OPC_LOCAL_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baLocal.size() != OPC_LOCAL_SIZE)) return false;
        const uchar *pLocal = reinterpret_cast<const uchar *>(baLocal.constData());
        if ((pLocal[0] != 'P') || (pLocal[1] != 'K') || (pLocal[2] != 3) || (pLocal[3] != 4)) return false;

        const qint64 nLocalName = static_cast<qint64>(qFromLittleEndian<quint16>(pLocal + 26));
        const qint64 nLocalExtra = static_cast<qint64>(qFromLittleEndian<quint16>(pLocal + 28));
        const qint64 nDataOffset = nLocalOffset + OPC_LOCAL_SIZE + nLocalName + nLocalExtra;
        if (!opcRangeWithin(context.nZipSize, nDataOffset, nCompressedSize)) return false;

        // Directory entries carry no payload; the extraction chain recreates
        // the folders from the member paths.
        if (baRawName.endsWith('/') || baRawName.endsWith('\\')) continue;

        MEMBER member = {};
        member.nDataOffset = OPC_BANNER_SIZE + nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nZipMethod = nMethod;
        member.sFileName = sanitizeName(baRawName);
        if (member.sFileName.isEmpty()) return false;
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;
    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XOPC::isValid(PDSTRUCT *pPdStruct)
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

bool XOPC::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XOPC archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XOPC::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XOPC(pDevice);
}

QList<QString> XOPC::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'OS2POINT'"));
    return listResult;
}

XBinary::FT XOPC::getFileType()
{
    return FT_OPC;
}

XBinary::MODE XOPC::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XOPC::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XOPC::getArch()
{
    return QString();
}

QString XOPC::getFileFormatExt()
{
    return QStringLiteral("opc");
}

QString XOPC::getFileFormatExtsString()
{
    return QStringLiteral("OS2Point package (*.opc)");
}

QString XOPC::getMIMEString()
{
    return QStringLiteral("application/x-os2point");
}

QString XOPC::getVersion()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential() || (guardedSource->size() < OPC_BANNER_SIZE)) return QString();
    const QByteArray baBanner = read_array_process(0, OPC_BANNER_SIZE, nullptr);
    if (baBanner.size() != OPC_BANNER_SIZE) return QString();
    if (memcmp(baBanner.constData(), "OS2POINT", 8) != 0) return QString();
    // "OS2POINT v1.2: ..." - the version token sits between the space and ':'.
    const qint32 nStart = baBanner.indexOf(" v");
    if (nStart < 0) return QString();
    const qint32 nEnd = baBanner.indexOf(':', nStart);
    if (nEnd <= nStart + 2) return QString();
    return QString::fromLatin1(baBanner.mid(nStart + 2, nEnd - nStart - 2));
}

qint64 XOPC::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XOPC::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XOPC::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XOPC::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XOPC::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = OPC_BANNER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_OPC);
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, methodProperty(member.nZipMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethod(member.nZipMethod));
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

QMap<XBinary::UNPACK_PROP, QVariant> XOPC::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XOPC::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XOPC> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, pContext->sBanner.isEmpty() ? tr("OS2Point package") : pContext->sBanner);
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

XBinary::ARCHIVERECORD XOPC::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_OPC);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, methodProperty(member.nZipMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethod(member.nZipMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XOPC::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XOPC::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
