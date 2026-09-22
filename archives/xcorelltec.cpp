/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xcorelltec.h"

#include <QtEndian>

#include <cstring>
#include <new>

#include "Algos/xcorelltecdecoder.h"

namespace {
// "LTEC" | u32 (meaning not established) | one zero byte.
const qint64 LTEC_HEADER_SIZE = 9;
const qint64 LTEC_RECORD_FIXED_SIZE = 14;  // u16 len + three u32 fields
const qint32 LTEC_MIN_NAME_LENGTH = 1;
// The longest name in the reference corpus is twelve ("SETUPAPI.IN_"); the
// cap is loose enough to survive a longer one and tight enough that random
// bytes cannot pass as a record.
const qint32 LTEC_MAX_NAME_LENGTH = 64;
const qint32 LTEC_MAX_MEMBERS = 0x40000;
const qint64 LTEC_MAX_MEMBER_SIZE = Q_INT64_C(0x40000000);   // 1 GB
const qint64 LTEC_MAX_BLOCK_SIZE = Q_INT64_C(0x40000000);    // 1 GB
// Blocks overlap: the last symbols of a block sit in the first bytes of the
// next one (see BLOCK::nStreamSize).  Two bytes is the measured maximum; four
// is handed to the codec so a variant with a slightly deeper bit register
// still decodes, at the cost of two bytes it will simply not read.
const qint64 LTEC_BLOCK_TAIL = 4;
// isValid() trial-decodes this much of the first block.  It is enough to walk
// the 16-bit prelude, all three Huffman tables and several thousand symbols,
// and small enough to stay cheap on a file that only happens to start "LTEC".
const qint64 LTEC_PROBE_SIZE = 4096;

bool ltecRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

// Names are 8.3, upper case, and the corpus uses only A-Z 0-9 '.' '_'.
// Anything with a path separator or a control byte is rejected: the format
// stores no directories, so a separator would mean the record is not a
// record.
bool ltecIsValidName(const QByteArray &baName)
{
    if ((baName.size() < LTEC_MIN_NAME_LENGTH) ||
        (baName.size() > LTEC_MAX_NAME_LENGTH)) {
        return false;
    }
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        const char c = baName.at(i);
        if ((c == '/') || (c == '\\') || (c == ':')) return false;
    }
    return true;
}
}  // namespace

XCorelLtec::XCorelLtec(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCorelLtec::~XCorelLtec()
{
}

bool XCorelLtec::parseContext(CONTEXT *pContext, bool bProbeStream,
                              PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < LTEC_HEADER_SIZE + LTEC_RECORD_FIXED_SIZE + 2) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, LTEC_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != LTEC_HEADER_SIZE)) {
        return false;
    }
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
    if (std::memcmp(pHeader, "LTEC", 4) != 0) return false;
    // The ninth byte is zero in every known sample and is part of the gate:
    // without it "LTEC" plus four arbitrary bytes is a two-in-a-billion match
    // away from being walked as a directory.
    if (pHeader[8] != 0) return false;
    context.nHeaderValue = qFromLittleEndian<quint32>(pHeader + 4);

    context.nDirectoryOffset = LTEC_HEADER_SIZE;

    // The directory has no terminator and no count: it ends where the record
    // chain stops being well formed.  Every field is cross-checked against
    // the previous record, so the stop is decided by the format's own
    // redundancy rather than by a heuristic.
    qint64 nOffset = LTEC_HEADER_SIZE;
    qint64 nCurrentBlockOffset = -1;
    qint64 nExpectedInBlock = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        if (!ltecRangeWithin(context.nInputSize, nOffset,
                             LTEC_RECORD_FIXED_SIZE)) {
            break;
        }
        const QByteArray baRecord =
            read_array_process(nOffset, LTEC_RECORD_FIXED_SIZE, pPdStruct);
        if ((baRecord.size() != LTEC_RECORD_FIXED_SIZE)) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());
        const qint64 nRecordSize =
            static_cast<qint64>(qFromLittleEndian<quint16>(pRecord));
        const qint64 nBlockOffset =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 2));
        const qint64 nOffsetInBlock =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 6));
        const qint64 nMemberSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 10));

        const qint64 nNameSize = nRecordSize - LTEC_RECORD_FIXED_SIZE;
        if ((nNameSize < LTEC_MIN_NAME_LENGTH + 1) ||
            (nNameSize > LTEC_MAX_NAME_LENGTH + 1)) {
            break;
        }
        if (!ltecRangeWithin(context.nInputSize,
                             nOffset + LTEC_RECORD_FIXED_SIZE, nNameSize)) {
            break;
        }
        const QByteArray baNameField = read_array_process(
            nOffset + LTEC_RECORD_FIXED_SIZE, nNameSize, pPdStruct);
        if ((baNameField.size() != nNameSize)) {
            return false;
        }
        // Fixed relationship, not a search: the terminator is the LAST byte
        // of the record, so a NUL anywhere else means this is not a record.
        if (baNameField.at(static_cast<qint32>(nNameSize) - 1) != '\0') break;
        const QByteArray baName =
            baNameField.left(static_cast<qint32>(nNameSize) - 1);
        if (!ltecIsValidName(baName)) break;

        if ((nMemberSize <= 0) || (nMemberSize > LTEC_MAX_MEMBER_SIZE)) break;
        if (nOffsetInBlock > LTEC_MAX_BLOCK_SIZE - nMemberSize) break;

        if (nBlockOffset != nCurrentBlockOffset) {
            // A new block: its offset must move forward and its first member
            // must restart the in-block cursor at zero.
            if (nBlockOffset <= nCurrentBlockOffset) break;
            if (nOffsetInBlock != 0) break;
            if (context.listBlocks.size() == 0) {
                // The directory ends exactly where the payload begins, and
                // the payload begins with block offset 0.
                if (nBlockOffset != 0) break;
            }
            BLOCK block = {};
            block.nFileOffset = nBlockOffset;  // rebased once the size is known
            block.nCompressedSize = 0;
            block.nUncompressedSize = 0;
            context.listBlocks.append(block);
            nCurrentBlockOffset = nBlockOffset;
            nExpectedInBlock = 0;
        }
        // Members of one block tile its plaintext without gaps or overlap.
        if (nOffsetInBlock != nExpectedInBlock) break;
        nExpectedInBlock = nOffsetInBlock + nMemberSize;

        if (context.listMembers.size() >= LTEC_MAX_MEMBERS) break;

        MEMBER member = {};
        member.nRecordOffset = nOffset;
        member.nBlockIndex = context.listBlocks.size() - 1;
        member.nOffsetInBlock = nOffsetInBlock;
        member.nSize = nMemberSize;
        member.sFileName = QString::fromLatin1(baName);
        context.listMembers.append(member);

        BLOCK &block = context.listBlocks[context.listBlocks.size() - 1];
        block.nUncompressedSize = nExpectedInBlock;
        if (block.nUncompressedSize > LTEC_MAX_BLOCK_SIZE) return false;

        nOffset += nRecordSize;
    }

    if (!isPdStructNotCanceled(pPdStruct)) return false;
    if (context.listMembers.isEmpty() || context.listBlocks.isEmpty()) {
        return false;
    }

    context.nDirectorySize = nOffset - context.nDirectoryOffset;
    const qint64 nPayloadOffset = nOffset;
    if (nPayloadOffset >= context.nInputSize) return false;
    const qint64 nPayloadSize = context.nInputSize - nPayloadOffset;

    // Block offsets are relative to the first payload byte; sizes come from
    // the gap to the next block, and the last block runs to end of file.
    for (qint32 i = 0; i < context.listBlocks.size(); ++i) {
        BLOCK &block = context.listBlocks[i];
        const qint64 nRelative = block.nFileOffset;
        if (nRelative >= nPayloadSize) return false;
        const qint64 nNextRelative = (i + 1 < context.listBlocks.size())
                                         ? context.listBlocks.at(i + 1).nFileOffset
                                         : nPayloadSize;
        if (nNextRelative <= nRelative) return false;
        block.nFileOffset = nPayloadOffset + nRelative;
        block.nCompressedSize = nNextRelative - nRelative;
        block.nStreamSize =
            qMin<qint64>(block.nCompressedSize + LTEC_BLOCK_TAIL,
                         context.nInputSize - block.nFileOffset);
        if (block.nUncompressedSize <= 0) return false;
    }

    if (bProbeStream) {
        // Bounded trial decode of the first block.  The directory arithmetic
        // above is already strong, but a container that parses and then emits
        // garbage at exit 0 is worse than no support, so the codec has to
        // agree before this file is claimed.
        const BLOCK &block = context.listBlocks.first();
        const qint64 nProbeRead = qMin<qint64>(block.nStreamSize, 0x20000);
        const QByteArray baPacked =
            read_array_process(block.nFileOffset, nProbeRead, pPdStruct);
        if ((baPacked.size() != nProbeRead)) {
            return false;
        }
        const qint64 nProbeSize =
            qMin<qint64>(block.nUncompressedSize, LTEC_PROBE_SIZE);
        if (!XCorelLtecDecoder::probe(baPacked, block.nUncompressedSize,
                                      nProbeSize, pPdStruct)) {
            return false;
        }
    }

    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XCorelLtec::isValid(PDSTRUCT *pPdStruct)
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

bool XCorelLtec::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCorelLtec archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XCorelLtec::createInstance(QIODevice *pDevice, bool bIsImage,
                                    XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCorelLtec(pDevice);
}

QList<QString> XCorelLtec::getSearchSignatures()
{
    // "LTEC", four bytes whose meaning is unknown, then the zero byte the
    // header always carries.
    return {QStringLiteral("'LTEC'........00")};
}

XBinary::FT XCorelLtec::getFileType()
{
    return FT_COREL_LTEC;
}

XBinary::MODE XCorelLtec::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XCorelLtec::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCorelLtec::getArch()
{
    return QString();
}

QString XCorelLtec::getFileFormatExt()
{
    return QStringLiteral("lta");
}

QString XCorelLtec::getFileFormatExtsString()
{
    return QStringLiteral("Corel LTEC install archive (*.lta)");
}

QString XCorelLtec::getMIMEString()
{
    return QStringLiteral("application/x-corel-lta");
}

QString XCorelLtec::getVersion()
{
    // The header carries no version field.  The u32 at +4 varies from sample
    // to sample and matches no size in the container (not the file size, not
    // the sum of the members, not any block size), so it is parsed and kept
    // in the context but deliberately not dressed up as a version.
    return QString();
}

qint64 XCorelLtec::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XCorelLtec::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XCorelLtec::getMemoryMap(MAPMODE mapMode,
                                              PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XCorelLtec::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XCorelLtec::getFileParts(quint32 nFileParts,
                                               qint32 nLimit,
                                               PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, false, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = LTEC_HEADER_SIZE + context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        const MEMBER &member = context.listMembers.at(i);
        const BLOCK &block = context.listBlocks.at(member.nBlockIndex);

        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            // Solid: the stream a member needs is its whole block.
            part.nFileOffset = block.nFileOffset;
            part.nFileSize = block.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      block.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_COREL_LTEC);
            part.mapProperties.insert(
                FPART_PROP_REPORTEDMETHOD,
                QStringLiteral("LTEC solid LZH (256 KB window)"));
            part.mapProperties.insert(
                FPART_PROP_COMPRESSPROPERTIES,
                XCorelLtecDecoder::packProperties(block.nUncompressedSize,
                                                  member.nOffsetInBlock));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = LTEC_RECORD_FIXED_SIZE +
                             member.sFileName.toLatin1().size() + 1;
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
        // The last block runs to end of file, so there is never an overlay
        // today; the branch is kept so the part list stays correct if the
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

QMap<XBinary::UNPACK_PROP, QVariant> XCorelLtec::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCorelLtec::initUnpack(UNPACK_STATE *pState,
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
    if (!parseContext(pContext, false, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Corel/LEAD LTEC install archive; solid LZH blocks"));
    pState->nCurrentOffset = pContext->listMembers.first().nRecordOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XCorelLtec::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nRecordOffset) return ARCHIVERECORD();
    if ((member.nBlockIndex < 0) ||
        (member.nBlockIndex >= pContext->listBlocks.size())) {
        return ARCHIVERECORD();
    }
    const BLOCK &block = pContext->listBlocks.at(member.nBlockIndex);

    ARCHIVERECORD result = {};
    // Solid container: the record's stream is the whole block, and the
    // properties below say which slice of its plaintext the member owns.
    result.nStreamOffset = block.nFileOffset;
    result.nStreamSize = block.nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                block.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_COREL_LTEC);
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        QStringLiteral("LTEC solid LZH (256 KB window)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(
        FPART_PROP_COMPRESSPROPERTIES,
        XCorelLtecDecoder::packProperties(block.nUncompressedSize,
                                          member.nOffsetInBlock));
    return result;
}

bool XCorelLtec::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XCorelLtec::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
