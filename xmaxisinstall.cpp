/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xmaxisinstall.h"

#include <QPointer>
#include <QtEndian>

#include <algorithm>
#include <new>

#include "Algos/xlzhdecoder.h"

namespace {
// +0x00 u32 record size | +0x04 name[13] -> 17 bytes.  The record size counts
// everything after those 17 bytes, and the first 4 bytes of that region are
// the u32 original size that LZHUF's encoder emits in front of the coded bits.
const qint64 MAXIS_HEADER_SIZE = 17;
const qint32 MAXIS_NAME_FIELD_SIZE = 13;
const qint64 MAXIS_SIZE_PREFIX_SIZE = 4;
const qint32 MAXIS_MAX_MEMBERS = 100000;
// Matches LH1_MAX_UNPACKED_BUFFER_SIZE in the -lh1- decoder; a member larger
// than that could never be unpacked anyway.
const qint64 MAXIS_MAX_UNCOMPRESSED_SIZE = 256LL * 1024 * 1024;
// LZHUF cannot emit more than F=60 bytes for the cheapest possible token
// (1 bit for the length symbol plus a 9-bit minimum position code), so 48x is
// the hard ceiling; 64x leaves margin and still rejects the "1 packed byte
// claims 66536 raw bytes" shape that unrelated files produce.
const qint64 MAXIS_MAX_RATIO = 64;
// Trial-decode budget.  A probe MUST decode a whole member: the -lh1- decoder
// refuses a stream whose declared output count falls inside a match, so
// stopping early would reject genuine archives roughly half the time.  Cost is
// bounded instead by probing the SMALLEST members, at most this many of them,
// and by skipping any member too large to be worth decompressing on a probe.
const qint32 MAXIS_PROBE_MEMBERS = 3;
const qint64 MAXIS_PROBE_BUDGET = 8LL * 1024 * 1024;
const qint64 MAXIS_PROBE_MEMBER_LIMIT = 32LL * 1024 * 1024;
const qint64 MAXIS_PROBE_PACKED_LIMIT = 64LL * 1024 * 1024;

bool maxisRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool maxisIsNameCharacter(quint8 nCharacter)
{
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;
    // The DOS-legal punctuation set, minus the path and wildcard characters.
    static const char *pExtra = "!#$%&'()-@^_`{}~";
    for (const char *p = pExtra; *p; ++p) {
        if (static_cast<quint8>(*p) == nCharacter) return true;
    }
    return false;
}

// Every one of the 831 distinct member names in the reference corpus is a
// strict DOS 8.3 name.  Enforcing that is what stops a 13-byte window of
// arbitrary data from being read as a member header.
bool maxisIsValidName(const QByteArray &baName)
{
    const qint32 nSize = baName.size();
    if ((nSize < 1) || (nSize > 12)) return false;

    qint32 nDotPosition = -1;
    for (qint32 i = 0; i < nSize; ++i) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        if (nCharacter == '.') {
            if (nDotPosition >= 0) return false;  // only one dot
            nDotPosition = i;
            continue;
        }
        if (!maxisIsNameCharacter(nCharacter)) return false;
    }

    const qint32 nBaseSize = (nDotPosition < 0) ? nSize : nDotPosition;
    const qint32 nExtSize = (nDotPosition < 0) ? 0 : (nSize - nDotPosition - 1);
    if ((nBaseSize < 1) || (nBaseSize > 8)) return false;
    if (nDotPosition >= 0) {
        if ((nExtSize < 1) || (nExtSize > 3)) return false;
    }
    return true;
}
}  // namespace

XMaxisInstall::XMaxisInstall(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMaxisInstall::~XMaxisInstall()
{
}

// Trial decode of one whole member.  The container carries no magic and no
// checksum, so the only proof that a chain really is Maxis LZHUF is that its
// bitstream decodes to exactly the declared byte count.  The exact-fit rule on
// top of that is what makes the probe decisive: the writer flushes at most
// seven padding bits, so a genuine stream consumes every byte of its declared
// payload and not one more.
bool XMaxisInstall::probeMember(const MEMBER &member, PDSTRUCT *pPdStruct)
{
    if (member.nUncompressedSize <= 0) return member.nCompressedSize == 0;
    if ((member.nCompressedSize <= 0) ||
        (member.nCompressedSize > MAXIS_PROBE_PACKED_LIMIT)) {
        return false;
    }

    QPointer<XMaxisInstall> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baPacked = read_array_process(
        member.nDataOffset, member.nCompressedSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baPacked.size() != member.nCompressedSize) {
        return false;
    }

    const qint64 nConsumed = XLZHDecoder::lh1MeasureStream(
        reinterpret_cast<const quint8 *>(baPacked.constData()),
        baPacked.size(), member.nUncompressedSize, pPdStruct);

    return nConsumed == member.nCompressedSize;
}

bool XMaxisInstall::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XMaxisInstall> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // The smallest legal archive is one empty member: 17 header bytes plus the
    // 4-byte size prefix.
    if (context.nInputSize < MAXIS_HEADER_SIZE + MAXIS_SIZE_PREFIX_SIZE) {
        return false;
    }

    qint64 nOffset = 0;
    bool bComplete = false;
    while (context.listMembers.size() < MAXIS_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!maxisRangeWithin(context.nInputSize, nOffset,
                              MAXIS_HEADER_SIZE + MAXIS_SIZE_PREFIX_SIZE)) {
            return false;
        }
        const QByteArray baHeader = read_array_process(
            nOffset, MAXIS_HEADER_SIZE + MAXIS_SIZE_PREFIX_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baHeader.size() != MAXIS_HEADER_SIZE + MAXIS_SIZE_PREFIX_SIZE) {
            return false;
        }
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());

        const qint64 nRecordSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pHeader));
        if (nRecordSize < MAXIS_SIZE_PREFIX_SIZE ||
            !maxisRangeWithin(context.nInputSize, nOffset,
                              MAXIS_HEADER_SIZE + nRecordSize)) {
            return false;
        }

        // The name buffer is read at its FULL 13-byte width: a 12-character
        // name puts its terminator in the last byte, and reading one short
        // would reject exactly those records.  Everything after the terminator
        // is writer stack junk and is deliberately not inspected.
        const QByteArray baNameField = baHeader.mid(4, MAXIS_NAME_FIELD_SIZE);
        const int nTerminator = baNameField.indexOf('\0');
        if (nTerminator < 0) return false;
        const QByteArray baName = baNameField.left(nTerminator);
        if (!maxisIsValidName(baName)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset =
            nOffset + MAXIS_HEADER_SIZE + MAXIS_SIZE_PREFIX_SIZE;
        member.nCompressedSize = nRecordSize - MAXIS_SIZE_PREFIX_SIZE;
        member.nUncompressedSize = static_cast<qint64>(
            qFromLittleEndian<quint32>(pHeader + MAXIS_HEADER_SIZE));
        member.sFileName = QString::fromLatin1(baName);

        if (member.nUncompressedSize > MAXIS_MAX_UNCOMPRESSED_SIZE) {
            return false;
        }
        if (member.nUncompressedSize == 0) {
            // An empty member is stored as a bare size prefix; anything else
            // would mean packed bytes that decode to nothing.
            if (member.nCompressedSize != 0) return false;
        } else {
            if (member.nCompressedSize < 1) return false;
            if (member.nUncompressedSize >
                member.nCompressedSize * MAXIS_MAX_RATIO + 256) {
                return false;
            }
        }

        context.listMembers.append(member);
        nOffset += MAXIS_HEADER_SIZE + nRecordSize;

        // There is no terminator record and no slack: the chain ends by
        // landing exactly on EOF.  A short tail is a reject, otherwise every
        // prefix match would be accepted as an archive.
        if (nOffset == context.nInputSize) {
            bComplete = true;
            break;
        }
    }

    if (!bComplete || context.listMembers.isEmpty()) return false;

    // Probe the smallest members first: a whole member has to be decoded for
    // the probe to mean anything, so the cheapest ones carry the proof.  An
    // archive made only of empty members is rejected - there would be no
    // bitstream to check and the header shape alone is far too weak.
    QList<QPair<qint64, qint32> > listOrder;
    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        const MEMBER &member = context.listMembers.at(i);
        if (member.nUncompressedSize > 0) {
            listOrder.append(qMakePair(member.nUncompressedSize, i));
        }
    }
    if (listOrder.isEmpty()) return false;
    std::sort(listOrder.begin(), listOrder.end());

    qint32 nProbed = 0;
    qint64 nProbedBytes = 0;
    for (qint32 i = 0;
         (i < listOrder.size()) && (nProbed < MAXIS_PROBE_MEMBERS); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const MEMBER &member = context.listMembers.at(listOrder.at(i).second);
        // The first member is always probed, however large: without it the
        // codec is never exercised at all.  Later ones stop at the budget.
        if (nProbed > 0) {
            if (member.nUncompressedSize > MAXIS_PROBE_MEMBER_LIMIT) break;
            if (nProbedBytes + member.nUncompressedSize > MAXIS_PROBE_BUDGET) {
                break;
            }
        } else if (member.nUncompressedSize > MAXIS_PROBE_MEMBER_LIMIT) {
            return false;
        }
        if (!probeMember(member, pPdStruct) || !guardedThis ||
            !guardedSource) {
            return false;
        }
        nProbedBytes += member.nUncompressedSize;
        ++nProbed;
    }
    if (nProbed < 1) return false;

    context.nArchiveSize = nOffset;
    context.nFirstMemberOffset = context.listMembers.first().nHeaderOffset;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XMaxisInstall::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XMaxisInstall::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMaxisInstall archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMaxisInstall::createInstance(QIODevice *pDevice, bool bIsImage,
                                       XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMaxisInstall(pDevice);
}

QList<QString> XMaxisInstall::getSearchSignatures()
{
    // Headerless: there is nothing constant at any fixed offset, so there is
    // no signature to publish.  Detection is the chained walk in isValid().
    return QList<QString>();
}

XBinary::FT XMaxisInstall::getFileType()
{
    return FT_MAXIS_MXS;
}

XBinary::MODE XMaxisInstall::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMaxisInstall::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMaxisInstall::getArch()
{
    return QString();
}

QString XMaxisInstall::getFileFormatExt()
{
    return QStringLiteral("mxs");
}

QString XMaxisInstall::getFileFormatExtsString()
{
    return QStringLiteral("Maxis install archive (*.mxs *._)");
}

QString XMaxisInstall::getMIMEString()
{
    return QStringLiteral("application/x-maxis-install");
}

QString XMaxisInstall::getVersion()
{
    // Nothing in the container is versioned.
    return QString();
}

qint64 XMaxisInstall::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XMaxisInstall::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XMaxisInstall::getMemoryMap(MAPMODE mapMode,
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

XBinary::HANDLE_METHOD XMaxisInstall::memberHandleMethod(const MEMBER &member)
{
    // Stock Yoshizaki LZHUF == LHA -lh1-; the in-tree lh1 decoder is the same
    // algorithm bit for bit, so no new method id is needed.  An empty member
    // has no bitstream at all.
    if (member.nUncompressedSize == 0) return HANDLE_METHOD_STORE;
    return HANDLE_METHOD_LZH1;
}

QString XMaxisInstall::memberMethodString(const MEMBER &member)
{
    if (member.nUncompressedSize == 0) return QStringLiteral("Stored");
    return QStringLiteral("LZHUF (LZSS 4K + adaptive Huffman)");
}

bool XMaxisInstall::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XMaxisInstall::getFileParts(quint32 nFileParts,
                                                  qint32 nLimit,
                                                  PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = MAXIS_HEADER_SIZE + MAXIS_SIZE_PREFIX_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      memberHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      memberMethodString(member));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = MAXIS_HEADER_SIZE + MAXIS_SIZE_PREFIX_SIZE +
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
        // parseContext() only accepts a chain that lands exactly on EOF, so
        // this cannot fire today; it is kept so the part list stays correct if
        // that rule is ever relaxed for split-disk parts.
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

QMap<XBinary::UNPACK_PROP, QVariant> XMaxisInstall::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMaxisInstall::initUnpack(UNPACK_STATE *pState,
                               const QMap<UNPACK_PROP, QVariant> &mapProperties,
                               PDSTRUCT *pPdStruct)
{
    QPointer<XMaxisInstall> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Maxis DOS install archive; LZHUF (-lh1-compatible) members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
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

XBinary::ARCHIVERECORD XMaxisInstall::infoCurrent(UNPACK_STATE *pState,
                                                  PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                memberHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                memberMethodString(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No CRC, timestamp or attribute properties: the 17-byte header plus the
    // 4-byte size prefix account for every byte the format stores.
    return result;
}

bool XMaxisInstall::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
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

bool XMaxisInstall::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
