/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xmva.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 MVA_CONTAINER_HEADER_SIZE = 8;
const qint64 MVA_MEMBER_HEADER_SIZE = 0x15a;
const qint64 MVA_NAME_OFFSET = 0x10;
const qint64 MVA_NAME_SIZE = 260;
const qint64 MVA_SIZES_OFFSET = 0x114;
const quint16 MVA_MEMBER_VERSION = 1U;
const quint32 MVA_CONTAINER_VERSION = 1U;
const qint32 MVA_MAX_MEMBERS = 100000;
const qint64 MVA_MAX_MEMBER_SIZE = 0x40000000;  // 1 GB sanity cap

bool mvaRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// The path field is a fixed 260-byte buffer, NUL padded.  Read the whole FIELD
// and stop at the first NUL; the writer leaves stale bytes after it in some
// archives, so trailing garbage must not leak into the name.
QString mvaFieldToString(const QByteArray &baField)
{
    qint32 nLength = baField.size();
    for (qint32 i = 0; i < baField.size(); ++i) {
        if (baField.at(i) == '\0') {
            nLength = i;
            break;
        }
    }
    return QString::fromLatin1(baField.constData(), nLength);
}

bool mvaIsPrintablePath(const QString &sPath)
{
    if (sPath.isEmpty()) return false;
    for (qint32 i = 0; i < sPath.size(); ++i) {
        const ushort nCharacter = sPath.at(i).unicode();
        if ((nCharacter < 0x20) || (nCharacter > 0xff)) return false;
        if ((nCharacter == '<') || (nCharacter == '>') || (nCharacter == '"') || (nCharacter == '|') || (nCharacter == '*') || (nCharacter == '?')) {
            return false;
        }
    }
    return true;
}

// U3 reports members by base name only (its worker calls ExtractFileName on the
// stored path), and the stored paths are absolute build-machine paths such as
// "C:\Build Win95 drv\input\...\m_qdesk.dll" that must never be written as-is.
QString mvaBaseName(const QString &sPath)
{
    QString sNormalized = sPath;
    sNormalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const qint32 nIndex = sNormalized.lastIndexOf(QLatin1Char('/'));
    if (nIndex >= 0) sNormalized = sNormalized.mid(nIndex + 1);
    const qint32 nColon = sNormalized.lastIndexOf(QLatin1Char(':'));
    if (nColon >= 0) sNormalized = sNormalized.mid(nColon + 1);
    return sNormalized;
}
}  // namespace

XMVA::XMVA(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMVA::~XMVA()
{
}

bool XMVA::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XMVA> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < MVA_CONTAINER_HEADER_SIZE + MVA_MEMBER_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, MVA_CONTAINER_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != MVA_CONTAINER_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("mflh", 4)) return false;
    // A .MVB continuation volume repeats the "mflh" tag but carries the tail of
    // the previous volume's stream here instead of 1.
    if (qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHeader.constData()) + 4) != MVA_CONTAINER_VERSION) {
        return false;
    }

    qint64 nOffset = MVA_CONTAINER_HEADER_SIZE;
    while (isPdStructNotCanceled(pPdStruct)) {
        if (nOffset >= context.nInputSize) break;
        if (context.listMembers.size() >= MVA_MAX_MEMBERS) return false;
        if (!mvaRangeWithin(context.nInputSize, nOffset, MVA_MEMBER_HEADER_SIZE)) {
            // A truncated trailing header means the volume was cut; everything
            // parsed so far is still valid, so stop rather than fail.
            break;
        }
        const QByteArray baMember = read_array_process(nOffset, MVA_MEMBER_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baMember.size() != MVA_MEMBER_HEADER_SIZE)) return false;
        const uchar *pMember = reinterpret_cast<const uchar *>(baMember.constData());
        if (baMember.left(4) != QByteArray("mfen", 4)) break;
        if (qFromLittleEndian<quint16>(pMember + 4) != MVA_MEMBER_VERSION) break;
        if (static_cast<qint64>(qFromLittleEndian<quint16>(pMember + 6)) != MVA_MEMBER_HEADER_SIZE) break;

        const qint64 nUncompressedSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(pMember + MVA_SIZES_OFFSET)));
        const qint64 nCompressedSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(pMember + MVA_SIZES_OFFSET + 4)));
        if ((nUncompressedSize < 0) || (nCompressedSize < 0)) break;
        if ((nUncompressedSize > MVA_MAX_MEMBER_SIZE) || (nCompressedSize > MVA_MAX_MEMBER_SIZE)) break;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + MVA_MEMBER_HEADER_SIZE;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nModificationTime = qFromLittleEndian<quint32>(pMember + 8);
        member.nCreationTime = qFromLittleEndian<quint32>(pMember + 0x0c);
        member.nChecksum1 = qFromLittleEndian<quint32>(pMember + MVA_SIZES_OFFSET + 8);
        member.nChecksum2 = qFromLittleEndian<quint32>(pMember + MVA_SIZES_OFFSET + 12);
        member.nAttributes = qFromLittleEndian<quint32>(pMember + MVA_SIZES_OFFSET + 16);
        member.bStored = (nCompressedSize == nUncompressedSize);
        member.sFullPath = mvaFieldToString(baMember.mid(qint32(MVA_NAME_OFFSET), qint32(MVA_NAME_SIZE)));
        if (!mvaIsPrintablePath(member.sFullPath)) break;
        member.sFileName = mvaBaseName(member.sFullPath);
        if (member.sFileName.isEmpty()) break;

        // The last member of a volume that spills into a .MVB is truncated
        // here; publish what this volume actually holds instead of dropping it.
        if (!mvaRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) {
            member.nCompressedSize = context.nInputSize - member.nDataOffset;
            member.bStored = false;
            context.listMembers.append(member);
            nOffset = context.nInputSize;
            break;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;
    if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    context.nArchiveSize = nOffset;
    *pContext = context;
    return true;
}

bool XMVA::isValid(PDSTRUCT *pPdStruct)
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

bool XMVA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMVA archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMVA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMVA(pDevice);
}

XBinary::FT XMVA::getFileType()
{
    return FT_MVA;
}

XBinary::MODE XMVA::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMVA::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMVA::getArch()
{
    return QString();
}

QString XMVA::getFileFormatExt()
{
    return QStringLiteral("mva");
}

QString XMVA::getFileFormatExtsString()
{
    return QStringLiteral("MVA installer archive (*.mva *.mvb)");
}

QString XMVA::getMIMEString()
{
    return QStringLiteral("application/x-mva");
}

QString XMVA::getVersion()
{
    return QStringLiteral("1");
}

qint64 XMVA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XMVA::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XMVA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XMVA::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XMVA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = MVA_CONTAINER_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_ZLIB);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bStored ? QStringLiteral("Store") : QStringLiteral("Deflate (zlib)"));
            const QDateTime dtMTime = valueToTime(member.nModificationTime, DT_TYPE_UNIXTIME);
            if (dtMTime.isValid()) part.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = MVA_MEMBER_HEADER_SIZE + member.nCompressedSize;
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
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, result.size())) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XMVA::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMVA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XMVA> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("MVA installer archive; zlib-compressed and stored members"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
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

XBinary::ARCHIVERECORD XMVA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, member.bStored ? HANDLE_METHOD_STORE : HANDLE_METHOD_ZLIB);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.bStored ? QStringLiteral("Store") : QStringLiteral("Deflate (zlib)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime = valueToTime(member.nModificationTime, DT_TYPE_UNIXTIME);
    if (dtMTime.isValid()) result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    return result;
}

bool XMVA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XMVA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
