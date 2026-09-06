/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xfiz.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// See the layout comment in xfiz.h; 4+1+1+2+4+4+2+2 = 20 bytes, no slack.
const qint64 FIZ_HEADER_SIZE = 20;
const quint8 FIZ_METHOD_STORED = 0x00U;
const quint8 FIZ_METHOD_LH5 = 0x01U;
// The writer only ever emits 8.3 names, and U3's own detector rejects a length
// byte outside 1..12 (it tests bit nameLen of the mask 0x1ffe), so the same
// bound is used here.
const qint32 FIZ_MAX_NAME_SIZE = 12;
const qint32 FIZ_MAX_MEMBERS = 100000;
const qint64 FIZ_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap

bool fizRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 && nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool fizIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}
}  // namespace

XFiz::XFiz(QIODevice *pDevice) : XArchive(pDevice)
{
}

XFiz::~XFiz()
{
}

bool XFiz::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XFiz> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // Smallest conceivable member: header + a one-character name + one payload
    // byte.  Anything shorter cannot be a chain.
    if (context.nInputSize < FIZ_HEADER_SIZE + 2) return false;

    qint64 nOffset = 0;
    while ((context.listMembers.size() < FIZ_MAX_MEMBERS) && isPdStructNotCanceled(pPdStruct)) {
        if (!fizRangeWithin(context.nInputSize, nOffset, FIZ_HEADER_SIZE)) return false;

        const QByteArray baHeader = read_array_process(nOffset, FIZ_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baHeader.size() != FIZ_HEADER_SIZE)) return false;

        const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

        // The magic sits on every member, not only the first one; that is what
        // makes a truncated or spliced chain fail closed.
        if (std::memcmp(pHeader, "FIZ\x1a", 4) != 0) return false;

        const quint8 nMethod = pHeader[4];
        if ((nMethod != FIZ_METHOD_STORED) && (nMethod != FIZ_METHOD_LH5)) return false;

        const qint32 nNameSize = static_cast<qint32>(pHeader[5]);
        if ((nNameSize < 1) || (nNameSize > FIZ_MAX_NAME_SIZE)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nMethod = nMethod;
        member.nCRC16 = qFromLittleEndian<quint16>(pHeader + 6);
        member.nUncompressedSize = static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 8));
        member.nCompressedSize = static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 12));

        // U3 rejects a member whose size fields have the sign bit set; the same
        // limit falls out of the 256 MB cap used here.
        if ((member.nUncompressedSize > FIZ_MAX_UNCOMPRESSED_SIZE) || (member.nCompressedSize > FIZ_MAX_UNCOMPRESSED_SIZE)) return false;

        const quint16 nDosTime = qFromLittleEndian<quint16>(pHeader + 16);
        const quint16 nDosDate = qFromLittleEndian<quint16>(pHeader + 18);
        const QDateTime dtModified = dosDateTimeToQDateTime(nDosDate, nDosTime);
        if (dtModified.isValid()) member.dtModified = dtModified;

        if (!fizRangeWithin(context.nInputSize, nOffset + FIZ_HEADER_SIZE, nNameSize)) return false;

        // The name field is exactly nameLen bytes and is NOT NUL-terminated;
        // read the field, never scan for a terminator.
        const QByteArray baName = read_array_process(nOffset + FIZ_HEADER_SIZE, nNameSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baName.size() != nNameSize)) return false;
        if (!fizIsValidName(baName)) return false;
        member.sFileName = QString::fromLatin1(baName);

        member.nDataOffset = nOffset + FIZ_HEADER_SIZE + nNameSize;
        if (!fizRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;

        if (nMethod == FIZ_METHOD_STORED) {
            // The stored members are the structural anchor that keeps the
            // two-value method gate honest: both size fields must agree.
            if (member.nCompressedSize != member.nUncompressedSize) return false;
        } else {
            // An -lh5- stream always carries at least a 16-bit block count, so
            // a non-empty member cannot have an empty payload.
            if ((member.nUncompressedSize > 0) && (member.nCompressedSize < 2)) return false;
            if ((member.nUncompressedSize == 0) && (member.nCompressedSize != 0)) return false;
        }

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;

        // There is no terminator record: the chain ends by landing exactly on
        // EOF.  A short tail is a reject, not an overlay - no FIZ archive in
        // the corpus has slack, and accepting one would turn any prefix match
        // into a hit.
        if (nOffset == context.nInputSize) {
            context.nArchiveSize = nOffset;
            context.nFirstMemberOffset = context.listMembers.first().nHeaderOffset;
            *pContext = context;
            return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
        }
    }

    return false;
}

bool XFiz::isValid(PDSTRUCT *pPdStruct)
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

bool XFiz::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFiz archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XFiz::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFiz(pDevice);
}

QList<QString> XFiz::getSearchSignatures()
{
    // The 0x1a terminator is part of the magic; "FIZ" alone is far too short.
    return {QStringLiteral("'FIZ'1A")};
}

XBinary::FT XFiz::getFileType()
{
    return FT_FIZ;
}

XBinary::MODE XFiz::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XFiz::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XFiz::getArch()
{
    return QString();
}

QString XFiz::getFileFormatExt()
{
    return QStringLiteral("fiz");
}

QString XFiz::getFileFormatExtsString()
{
    return QStringLiteral("Maximus FIZ archive (*.fiz)");
}

QString XFiz::getMIMEString()
{
    return QStringLiteral("application/x-fiz");
}

QString XFiz::getVersion()
{
    // Byte +4 is the per-member compression method, not a container version,
    // and there is no version field anywhere else in the file.
    return QString();
}

qint64 XFiz::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XFiz::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XFiz::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XFiz::methodToString(quint8 nMethod)
{
    if (nMethod == FIZ_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == FIZ_METHOD_LH5) return QStringLiteral("LHA -lh5-");
    return QStringLiteral("Unknown 0x%1").arg(nMethod, 2, 16, QLatin1Char('0'));
}

XBinary::HANDLE_METHOD XFiz::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == FIZ_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == FIZ_METHOD_LH5) {
        // Verified against U3's decompressor: it drives the shared LHA engine
        // with dicbit 13 / np 14 / pbit 4 and NC 510, i.e. plain -lh5-, so no
        // new HANDLE_METHOD is needed.
        return HANDLE_METHOD_LZH5;
    }
    return HANDLE_METHOD_UNKNOWN;
}

bool XFiz::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XFiz::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    const qint32 nNumberOfMembers = context.listMembers.size();
    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, result.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nDataOffset - member.nHeaderOffset;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
            part.mapProperties.insert(FPART_PROP_TYPE, static_cast<quint32>(member.nMethod));
            // The CRC covers the UNPACKED member, so it is published for the
            // unpack path.  Verifying it while parsing would decompress the
            // whole archive on every format probe.
            part.mapProperties.insert(FPART_PROP_RESULTCRC, static_cast<quint32>(member.nCRC16));
            part.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16ARC);
            if (member.dtModified.isValid()) {
                part.mapProperties.insert(FPART_PROP_DATETIME, member.dtModified);
                part.mapProperties.insert(FPART_PROP_MTIME, member.dtModified);
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = (member.nDataOffset - member.nHeaderOffset) + member.nCompressedSize;
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
        // parseContext() only accepts a chain that lands on EOF, so this branch
        // cannot fire today; it is kept so the part list stays correct if the
        // acceptance rule is ever relaxed.
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

QMap<XBinary::UNPACK_PROP, QVariant> XFiz::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XFiz::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XFiz> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Maximus FIZ archive; stored and LHA -lh5- members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
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

XBinary::ARCHIVERECORD XFiz::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
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
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_TYPE, static_cast<quint32>(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, static_cast<quint32>(member.nCRC16));
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_CRC16ARC);
    if (member.dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, member.dtModified);
        result.mapProperties.insert(FPART_PROP_MTIME, member.dtModified);
    }
    return result;
}

bool XFiz::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XFiz::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}
