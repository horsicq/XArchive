/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xgtu.h"

#include <QtEndian>

#include <new>

namespace {
// Index record: u32 dataOffset, u16 tag(=1), u16 kind(0 or 1), u16 zero,
// u16 nameLength.
const qint64 GTU_RECORD_SIZE = 12;
const quint16 GTU_RECORD_TAG = 1U;
const quint16 GTU_MAX_RECORD_KIND = 1U;
// Information block that follows the repeated header + name in the data area.
const qint64 GTU_INFO_SIZE = 28;
// Compression frame prelude: i32 rawSize, i32 packedSize.
const qint64 GTU_FRAME_SIZE = 8;
// The writer never emits a longer name than a full OS/2 path; the field is a
// u16, so the ceiling here is a sanity cap, not a format rule.
const qint32 GTU_MAX_NAME_SIZE = 1024;
const qint32 GTU_MAX_MEMBERS = 100000;
const qint32 GTU_MAX_FRAMES = 1048576;
const qint64 GTU_MAX_UNCOMPRESSED_SIZE = 0x40000000;  // 1 GB sanity cap
// Every frame but a member's last one carries exactly 64 KiB of plaintext.
const qint64 GTU_MAX_FRAME_RAW_SIZE = 0x10000;

bool gtuRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

bool gtuIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        // OS/2 kit names are plain 8.3 uppercase ASCII; anything outside the
        // printable range means the running-difference cipher was applied to
        // something that is not a name, i.e. this is not a GTU archive.
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        if ((c == '"') || (c == '*') || (c == '<') || (c == '>') ||
            (c == '?') || (c == '|') || (c == ':')) {
            return false;
        }
    }
    return true;
}
}  // namespace

XGTU::XGTU(QIODevice *pDevice) : XArchive(pDevice)
{
}

XGTU::~XGTU()
{
}

QByteArray XGTU::decodeName(const QByteArray &baEncoded)
{
    // The reference implementation: the key starts at the stored name length and is then
    // replaced by each plaintext byte in turn.  The seed is the low byte of the
    // length, so a 256-byte name would decode with key 0 - matching the 8-bit
    // subtraction the original performs.
    QByteArray baResult;
    baResult.resize(baEncoded.size());
    quint8 nKey = static_cast<quint8>(baEncoded.size() & 0xff);
    for (qint32 i = 0; i < baEncoded.size(); ++i) {
        const quint8 nPlain =
            static_cast<quint8>(static_cast<quint8>(baEncoded.at(i)) - nKey);
        baResult[i] = static_cast<char>(nPlain);
        nKey = nPlain;
    }
    return baResult;
}

bool XGTU::measureFrames(MEMBER *pMember, qint64 nInputSize,
                         PDSTRUCT *pPdStruct)
{
    if (!pMember) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    qint64 nOffset = pMember->nInfoOffset + GTU_INFO_SIZE;

    // Phase 1: frames whose plaintext belongs to a previous volume. The reference implementation walks
    // them without decoding, and the running total has to land exactly on the
    // skip value or the frame chain is not really a frame chain.
    qint64 nRemaining = pMember->nSkipSize;
    qint32 nFrameCount = 0;
    while (nRemaining > 0) {
        if ((++nFrameCount > GTU_MAX_FRAMES) ||
            !isPdStructNotCanceled(pPdStruct) ||
            !gtuRangeWithin(nInputSize, nOffset, GTU_FRAME_SIZE)) {
            return false;
        }
        const QByteArray baFrame =
            read_array_process(nOffset, GTU_FRAME_SIZE, pPdStruct);
        if ((baFrame.size() != GTU_FRAME_SIZE)) {
            return false;
        }
        const uchar *pFrame =
            reinterpret_cast<const uchar *>(baFrame.constData());
        const qint32 nRawSize =
            static_cast<qint32>(qFromLittleEndian<quint32>(pFrame));
        const qint32 nPackedSize =
            static_cast<qint32>(qFromLittleEndian<quint32>(pFrame + 4));
        if ((nRawSize <= 0) || (nRawSize > GTU_MAX_FRAME_RAW_SIZE) ||
            (nPackedSize < 0)) {
            return false;
        }
        nOffset += GTU_FRAME_SIZE;
        if (!gtuRangeWithin(nInputSize, nOffset, nPackedSize)) return false;
        nOffset += nPackedSize;
        nRemaining -= nRawSize;
    }
    if (nRemaining != 0) return false;

    // Phase 2: the frames this member actually contributes.
    pMember->nDataOffset = nOffset;
    nRemaining = pMember->nUncompressedSize;
    while (nRemaining > 0) {
        if ((++nFrameCount > GTU_MAX_FRAMES) ||
            !isPdStructNotCanceled(pPdStruct) ||
            !gtuRangeWithin(nInputSize, nOffset, GTU_FRAME_SIZE)) {
            return false;
        }
        const QByteArray baFrame =
            read_array_process(nOffset, GTU_FRAME_SIZE, pPdStruct);
        if ((baFrame.size() != GTU_FRAME_SIZE)) {
            return false;
        }
        const uchar *pFrame =
            reinterpret_cast<const uchar *>(baFrame.constData());
        const qint32 nRawSize =
            static_cast<qint32>(qFromLittleEndian<quint32>(pFrame));
        const qint32 nPackedSize =
            static_cast<qint32>(qFromLittleEndian<quint32>(pFrame + 4));
        if ((nRawSize <= 0) || (nRawSize > GTU_MAX_FRAME_RAW_SIZE) ||
            (nPackedSize <= 0) || (nRawSize > nRemaining)) {
            return false;
        }
        nOffset += GTU_FRAME_SIZE;
        if (!gtuRangeWithin(nInputSize, nOffset, nPackedSize)) return false;
        nOffset += nPackedSize;
        nRemaining -= nRawSize;
    }
    if (nRemaining != 0) return false;

    pMember->nCompressedSize = nOffset - pMember->nDataOffset;
    return true;
}

bool XGTU::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < 4 + GTU_RECORD_SIZE) return false;

    // The one and only global field: the u32 at +0 is the file's own size.
    const QByteArray baTotal = read_array_process(0, 4, pPdStruct);
    if ((baTotal.size() != 4)) return false;
    const qint64 nDeclaredSize = static_cast<qint64>(
        qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(baTotal.constData())));
    if (nDeclaredSize != context.nInputSize) return false;

    context.nArchiveSize = context.nInputSize;
    context.nIndexOffset = 4;

    qint64 nOffset = 4;
    bool bTerminated = false;
    while (!bTerminated && isPdStructNotCanceled(pPdStruct)) {
        if (context.listMembers.size() > GTU_MAX_MEMBERS) return false;
        if (!gtuRangeWithin(context.nInputSize, nOffset, GTU_RECORD_SIZE)) {
            return false;
        }
        const QByteArray baRecord =
            read_array_process(nOffset, GTU_RECORD_SIZE, pPdStruct);
        if ((baRecord.size() != GTU_RECORD_SIZE)) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());
        const qint64 nBlockOffset =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord));

        // The chain ends on the record that points at itself.  There is no
        // count and no sentinel name; this self-reference is the terminator.
        if (nBlockOffset == nOffset) {
            bTerminated = true;
            break;
        }

        // The reference implementation reads +0x04 as a single u32 and demands exactly 1, which makes it
        // abandon every archive that mixes in kind-1 members (5 of the 47 in
        // the reference corpus, 447 members lost).  The field is really two
        // u16s: the tag is always 1, and the second word is 0 or 1 with no
        // other effect on the layout - kind-1 members carry the same repeated
        // header, the same information block and the same LZARI frame chain,
        // and they decode to correct content (verified: text, MZ images and
        // OS/2 bitmap arrays).
        const quint16 nTag = qFromLittleEndian<quint16>(pRecord + 4);
        const quint16 nKind = qFromLittleEndian<quint16>(pRecord + 6);
        if ((nTag != GTU_RECORD_TAG) || (nKind > GTU_MAX_RECORD_KIND) ||
            (qFromLittleEndian<quint16>(pRecord + 8) != 0)) {
            return false;
        }
        const qint32 nNameSize =
            static_cast<qint32>(qFromLittleEndian<quint16>(pRecord + 10));
        if ((nNameSize <= 0) || (nNameSize > GTU_MAX_NAME_SIZE)) return false;
        if ((nBlockOffset <= 0) || (nBlockOffset >= context.nInputSize)) {
            return false;
        }

        const qint64 nNameOffset = nOffset + GTU_RECORD_SIZE;
        if (!gtuRangeWithin(context.nInputSize, nNameOffset, nNameSize)) {
            return false;
        }
        const QByteArray baEncodedName =
            read_array_process(nNameOffset, nNameSize, pPdStruct);
        if ((baEncodedName.size() != nNameSize)) {
            return false;
        }
        const QByteArray baName = decodeName(baEncodedName);
        if (!gtuIsValidName(baName)) return false;

        // The data block opens with a verbatim copy of the 12-byte record and
        // of the still-obfuscated name.  Cross-checking both is what makes a
        // format with no magic safe to detect: a random 12-byte window would
        // have to point at a copy of itself to survive this.
        if (!gtuRangeWithin(context.nInputSize, nBlockOffset,
                            GTU_RECORD_SIZE + nNameSize)) {
            return false;
        }
        const QByteArray baBlock = read_array_process(
            nBlockOffset, GTU_RECORD_SIZE + nNameSize, pPdStruct);
        if ((baBlock.size() != GTU_RECORD_SIZE + nNameSize)) {
            return false;
        }
        if (baBlock.left(qint32(GTU_RECORD_SIZE)) != baRecord) return false;
        if (baBlock.mid(qint32(GTU_RECORD_SIZE)) != baEncodedName) {
            return false;
        }

        MEMBER member = {};
        member.nIndexOffset = nOffset;
        member.nBlockOffset = nBlockOffset;
        member.nKind = nKind;
        member.nInfoOffset = nBlockOffset + GTU_RECORD_SIZE + nNameSize;
        member.sFileName = QString::fromLatin1(baName)
                               .replace(QLatin1Char('\\'), QLatin1Char('/'));

        if (!gtuRangeWithin(context.nInputSize, member.nInfoOffset,
                            GTU_INFO_SIZE)) {
            return false;
        }
        const QByteArray baInfo =
            read_array_process(member.nInfoOffset, GTU_INFO_SIZE, pPdStruct);
        if ((baInfo.size() != GTU_INFO_SIZE)) {
            return false;
        }
        const uchar *pInfo =
            reinterpret_cast<const uchar *>(baInfo.constData());
        member.nDosDate = qFromLittleEndian<quint16>(pInfo);
        member.nDosTime = qFromLittleEndian<quint16>(pInfo + 2);
        member.nUncompressedSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pInfo + 0x0c)));
        member.nAllocatedSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pInfo + 0x10)));
        member.nAttributes = qFromLittleEndian<quint32>(pInfo + 0x14);
        member.nSkipSize = static_cast<qint64>(
            static_cast<qint32>(qFromLittleEndian<quint32>(pInfo + 0x18)));
        if ((member.nUncompressedSize < 0) || (member.nAllocatedSize < 0) ||
            (member.nSkipSize < 0) ||
            (member.nUncompressedSize > GTU_MAX_UNCOMPRESSED_SIZE) ||
            (member.nSkipSize > GTU_MAX_UNCOMPRESSED_SIZE)) {
            return false;
        }

        if (!measureFrames(&member, context.nInputSize, pPdStruct)) {
            return false;
        }

        context.listMembers.append(member);
        nOffset = nNameOffset + nNameSize;
    }

    if (!bTerminated || context.listMembers.isEmpty()) return false;
    if (!isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    context.nIndexSize = (nOffset + GTU_RECORD_SIZE) - context.nIndexOffset;
    *pContext = context;
    return true;
}

bool XGTU::isValid(PDSTRUCT *pPdStruct)
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

bool XGTU::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGTU archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGTU::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGTU(pDevice);
}

XBinary::FT XGTU::getFileType()
{
    return FT_GTU;
}

XBinary::MODE XGTU::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XGTU::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGTU::getArch()
{
    return QString();
}

QString XGTU::getFileFormatExt()
{
    return QStringLiteral("csd");
}

QString XGTU::getFileFormatExtsString()
{
    return QStringLiteral("GTU distribution kit (*.csd *.001 *.002)");
}

QString XGTU::getMIMEString()
{
    return QStringLiteral("application/x-gtu");
}

QString XGTU::getVersion()
{
    // The tag word at record +0x04 is 1 in every known archive and is the
    // closest thing this container has to a version field.
    return QStringLiteral("1");
}

qint64 XGTU::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XGTU::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XGTU::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

bool XGTU::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XGTU::getFileParts(quint32 nFileParts, qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = 4 + context.nIndexSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Index");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
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
                (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                                : HANDLE_METHOD_GTU);
            part.mapProperties.insert(
                FPART_PROP_REPORTEDMETHOD,
                QStringLiteral("LZARI (framed)"));
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
            part.nFileOffset = member.nBlockOffset;
            part.nFileSize =
                (member.nDataOffset - member.nBlockOffset) +
                member.nCompressedSize;
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
        // parseContext() requires the declared size to equal the real file
        // size, so this cannot fire today; it is kept so the part list stays
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

QMap<XBinary::UNPACK_PROP, QVariant> XGTU::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGTU::initUnpack(UNPACK_STATE *pState,
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
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

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
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("GTU OS/2 distribution kit; framed Okumura LZARI members"));
    pState->nCurrentOffset = pContext->listMembers.first().nIndexOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XGTU::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nIndexOffset) {
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
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE
                                        : HANDLE_METHOD_GTU);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("LZARI (framed)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    const QDateTime dtMTime =
        dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dtMTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dtMTime);
    }
    // The information block has no CRC of any kind; the only integrity check
    // the container offers is the frame arithmetic parseContext() already did.
    return result;
}

bool XGTU::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nIndexOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XGTU::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
