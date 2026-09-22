/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xrid.h"

#include "Algos/xriddecoder.h"

#include <QtEndian>

#include <new>

namespace {
// 2 + 19 + 1 + 2 + 2 + 4 + 13 = 43; every byte of the header is accounted for.
const qint64 RID_HEADER_SIZE = 0x2b;
const qint64 RID_RESERVED_OFFSET = 0x02;
const qint64 RID_RESERVED_SIZE = 0x13;
const qint64 RID_ATTRIBUTES_OFFSET = 0x15;
const qint64 RID_DOSTIME_OFFSET = 0x16;
const qint64 RID_DOSDATE_OFFSET = 0x18;
const qint64 RID_SIZE_OFFSET = 0x1a;
const qint64 RID_NAME_OFFSET = 0x1e;
const qint32 RID_NAME_FIELD_SIZE = 13;
// DOS 8.3, so 8 + '.' + 3 = 12 characters plus the NUL exactly fill the field.
const qint32 RID_MAX_BASE_CHARS = 8;
const qint32 RID_MAX_EXT_CHARS = 3;
// The only attribute bit the writer ever sets is the archive bit; a member with
// any other attribute is a mis-parse, not a supported variation.
const quint8 RID_ATTRIBUTE_NONE = 0x00U;
const quint8 RID_ATTRIBUTE_ARCHIVE = 0x20U;
// The size field is a quint32 the writer never fills past 24 bits.
const qint64 RID_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(0x00ffffff);
const qint32 RID_MAX_MEMBERS = 65536;
// Detection trial-decodes the first member only, and stops once this much
// plaintext has come out: enough to prove the bitstream is real, cheap enough
// to run on every probe.
const qint64 RID_PROBE_OUTPUT_LIMIT = Q_INT64_C(256) * 1024;

bool ridRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

bool ridIsNameCharacter(quint8 nCharacter)
{
    if ((nCharacter < 0x20U) || (nCharacter > 0x7eU)) return false;
    // Everything DOS itself refuses inside a directory entry.  '.' is handled
    // by the caller because its position carries meaning.
    if ((nCharacter == '/') || (nCharacter == '\\') || (nCharacter == ':') ||
        (nCharacter == '*') || (nCharacter == '?') || (nCharacter == '"') ||
        (nCharacter == '<') || (nCharacter == '>') || (nCharacter == '|') ||
        (nCharacter == ' ')) {
        return false;
    }
    return true;
}
}  // namespace

XRID::XRID(QIODevice *pDevice) : XArchive(pDevice)
{
}

XRID::~XRID()
{
}

bool XRID::isValidNameField(const QByteArray &baField, QString *pName)
{
    if (baField.size() != RID_NAME_FIELD_SIZE) return false;

    // The field is a fixed writer buffer that is NOT cleared between members:
    // "ZIP.DLL\0EXE\0\0" and "PM.EXE\0V.DLL\0" both occur.  The NUL is the only
    // authority; the bytes behind it must be ignored, never validated.
    qint32 nLength = 0;
    while ((nLength < RID_NAME_FIELD_SIZE) &&
           (baField.at(nLength) != '\0')) {
        nLength++;
    }
    if (nLength <= 0) return false;
    // The name has to be terminated INSIDE the field; a full 13 printable
    // bytes means this is not a RID name field.
    if (nLength >= RID_NAME_FIELD_SIZE) return false;

    qint32 nDot = -1;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if (nCharacter == '.') {
            // Exactly one dot, never leading, never trailing.
            if ((nDot >= 0) || (i == 0) || (i == nLength - 1)) return false;
            nDot = i;
            continue;
        }
        if (!ridIsNameCharacter(nCharacter)) return false;
    }

    const qint32 nBaseChars = (nDot >= 0) ? nDot : nLength;
    const qint32 nExtChars = (nDot >= 0) ? (nLength - nDot - 1) : 0;
    if ((nBaseChars < 1) || (nBaseChars > RID_MAX_BASE_CHARS)) return false;
    if (nExtChars > RID_MAX_EXT_CHARS) return false;

    if (pName) *pName = QString::fromLatin1(baField.constData(), nLength);
    return true;
}

bool XRID::parseContext(CONTEXT *pContext, bool bDeepCheck,
                        PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // The shortest legal archive is one empty member: header plus a bare
    // terminator frame.
    if (context.nInputSize <
        RID_HEADER_SIZE + XRidDecoder::BLOCK_FRAME_SIZE) {
        return false;
    }

    qint64 nOffset = 0;
    while ((context.listMembers.size() < RID_MAX_MEMBERS) &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!ridRangeWithin(context.nInputSize, nOffset, RID_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, RID_HEADER_SIZE, pPdStruct);
        if (baHeader.size() != RID_HEADER_SIZE) {
            return false;
        }
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());

        MEMBER member = {};
        member.nTag = qFromLittleEndian<quint16>(pHeader);
        // There is no magic anywhere in this format; the reserved run is what
        // takes its place, and it is repeated on EVERY member, which is what
        // makes a spliced or truncated chain fail closed.
        if (member.nTag == 0U) return false;
        for (qint64 i = 0; i < RID_RESERVED_SIZE; i++) {
            if (pHeader[RID_RESERVED_OFFSET + i] != 0U) return false;
        }

        member.nAttributes = pHeader[RID_ATTRIBUTES_OFFSET];
        if ((member.nAttributes != RID_ATTRIBUTE_NONE) &&
            (member.nAttributes != RID_ATTRIBUTE_ARCHIVE)) {
            return false;
        }

        member.nDosTime =
            qFromLittleEndian<quint16>(pHeader + RID_DOSTIME_OFFSET);
        member.nDosDate =
            qFromLittleEndian<quint16>(pHeader + RID_DOSDATE_OFFSET);
        if (member.nDosDate == 0U) return false;

        const quint32 nSize =
            qFromLittleEndian<quint32>(pHeader + RID_SIZE_OFFSET);
        if (nSize > static_cast<quint32>(RID_MAX_UNCOMPRESSED_SIZE)) {
            return false;
        }
        member.nUncompressedSize = static_cast<qint64>(nSize);

        if (!isValidNameField(
                baHeader.mid(static_cast<int>(RID_NAME_OFFSET),
                             RID_NAME_FIELD_SIZE),
                &member.sFileName)) {
            return false;
        }

        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + RID_HEADER_SIZE;

        // Walk the block chain frame by frame.  This reads three bytes per
        // frame and never touches a payload, so measuring a member costs the
        // same whether it is stored or packed.
        qint64 nChainOffset = member.nDataOffset;
        qint32 nBlocks = 0;
        bool bTerminated = false;
        while (nBlocks <= RID_MAX_MEMBERS) {
            if (!ridRangeWithin(context.nInputSize, nChainOffset,
                                XRidDecoder::BLOCK_FRAME_SIZE)) {
                return false;
            }
            const QByteArray baFrame = read_array_process(
                nChainOffset, XRidDecoder::BLOCK_FRAME_SIZE, pPdStruct);
            if (baFrame.size() != XRidDecoder::BLOCK_FRAME_SIZE) {
                return false;
            }
            const uchar *pFrame =
                reinterpret_cast<const uchar *>(baFrame.constData());
            const qint64 nBlockSize =
                static_cast<qint64>(qFromLittleEndian<quint16>(pFrame));
            const quint8 nBlockType = pFrame[2];
            nChainOffset += XRidDecoder::BLOCK_FRAME_SIZE;

            if (nBlockType == XRidDecoder::BLOCK_TYPE_END) {
                if (nBlockSize != 0) return false;
                bTerminated = true;
                break;
            }
            if ((nBlockType != XRidDecoder::BLOCK_TYPE_STORED) &&
                (nBlockType != XRidDecoder::BLOCK_TYPE_PACKED)) {
                return false;
            }
            if (nBlockSize <= 0) return false;
            if (!ridRangeWithin(context.nInputSize, nChainOffset,
                                nBlockSize)) {
                return false;
            }
            nChainOffset += nBlockSize;
            nBlocks++;
        }
        if (!bTerminated) return false;

        member.nNumberOfBlocks = nBlocks;
        member.nCompressedSize = nChainOffset - member.nDataOffset;
        // A non-empty member cannot be carried by an empty chain, and an empty
        // member cannot carry blocks.
        if ((member.nUncompressedSize == 0) && (nBlocks != 0)) return false;
        if ((member.nUncompressedSize > 0) && (nBlocks == 0)) return false;

        context.listMembers.append(member);
        nOffset = nChainOffset;

        if (nOffset == context.nInputSize) {
            // There is no end-of-archive record: the chain has to land exactly
            // on EOF.  Accepting a short tail would turn any prefix match into
            // a hit on a format that has no magic at all.
            context.nArchiveSize = nOffset;
            context.nFirstMemberOffset =
                context.listMembers.first().nHeaderOffset;

            if (bDeepCheck) {
                // Headerless format: the structure alone is not proof.  Decode
                // the first member's payload for real, bounded, and - when the
                // whole member fits inside the budget - check the plaintext
                // length against the header's own field.
                const MEMBER &first = context.listMembers.first();
                if (first.nCompressedSize > 0) {
                    const QByteArray baChain = read_array_process(
                        first.nDataOffset, first.nCompressedSize, pPdStruct);
                    if (baChain.size() != first.nCompressedSize) {
                        return false;
                    }
                    bool bComplete = false;
                    qint64 nRawSize = -1;
                    if (!XRidDecoder::probe(baChain, RID_PROBE_OUTPUT_LIMIT,
                                            &bComplete, &nRawSize)) {
                        return false;
                    }
                    if (bComplete && (nRawSize != first.nUncompressedSize)) {
                        return false;
                    }
                }
            }

            *pContext = context;
            return isPdStructNotCanceled(pPdStruct);
        }
    }

    return false;
}

bool XRID::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XRID::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRID archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRID::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRID(pDevice);
}

XBinary::FT XRID::getFileType()
{
    return FT_RID;
}

XBinary::MODE XRID::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRID::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRID::getArch()
{
    return QString();
}

QString XRID::getFileFormatExt()
{
    return QStringLiteral("rid");
}

QString XRID::getFileFormatExtsString()
{
    return QStringLiteral("RID installer archive (*.rid)");
}

QString XRID::getMIMEString()
{
    return QStringLiteral("application/x-rid");
}

QString XRID::getVersion()
{
    // Nothing in the container names a version: there is no magic and no
    // format byte, only the per-member writer tag, which is not a version.
    return QString();
}

qint64 XRID::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XRID::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRID::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XRID::methodToString(const MEMBER &member)
{
    if (member.nNumberOfBlocks == 0) return QStringLiteral("Empty");
    return QStringLiteral("RID blocks (stored / PKWARE DCL Implode)");
}

bool XRID::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XRID::getFileParts(quint32 nFileParts, qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, false, pPdStruct)) return result;

    const qint32 nNumberOfMembers = context.listMembers.size();
    for (qint32 i = 0; i < nNumberOfMembers; i++) {
        const MEMBER &member = context.listMembers.at(i);
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = RID_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
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
            // The whole block chain, terminator included, is handed to the
            // decoder: the frames are part of the codec, not of the header.
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_RID);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member));
            part.mapProperties.insert(
                FPART_PROP_TYPE, static_cast<quint32>(member.nAttributes));
            if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
                const QDateTime dtModified =
                    dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
                if (dtModified.isValid()) {
                    part.mapProperties.insert(FPART_PROP_MTIME, dtModified);
                }
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = RID_HEADER_SIZE + member.nCompressedSize;
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
        // parseContext() only accepts a chain that lands on EOF, so this
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

QMap<XBinary::UNPACK_PROP, QVariant> XRID::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRID::initUnpack(UNPACK_STATE *pState,
                      const QMap<UNPACK_PROP, QVariant> &mapProperties,
                      PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) ||
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
    if (!parseContext(pContext, false, pPdStruct) ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("RID OS/2 installer archive; stored and PKWARE DCL Implode blocks"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XRID::infoCurrent(UNPACK_STATE *pState,
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_RID);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nAttributes));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        const QDateTime dtModified =
            dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        }
    }
    // No checksum property: the container carries none, anywhere.
    return result;
}

bool XRID::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XRID::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
