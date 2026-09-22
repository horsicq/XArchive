/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarq.h"

#include <QtEndian>

#include <new>
#include <QTimeZone>

namespace {
const quint32 ARQ_CONTAINER_MAGIC = 0x02045767U;
const quint32 ARQ_MEMBER_MAGIC = 0x01045767U;
// Crusher! writes the member header in two shapes.  Only the low byte of the
// signature word is a constant ('0' or '1'); its high byte is a writer tag
// (0x12 or 0x31).  The method word likewise splits: the high byte selects the
// Crusher generation (0x00, 0x0a, 0x10 all observed) and only the low byte
// names the codec.  Whole-word comparisons rejected every archive that was not
// produced by the one generation the class was first written against.
const quint8 ARQ_MEMBER_SIGNATURE_LOW_A = 0x30U;
const quint8 ARQ_MEMBER_SIGNATURE_LOW_B = 0x31U;
const quint8 ARQ_METHOD_STORED = 0x01U;
const quint8 ARQ_METHOD_CRUSHED = 0x02U;
const qint64 ARQ_CONTAINER_HEADER_SIZE = 12;
const qint64 ARQ_MEMBER_PREFIX_SIZE = 8;
const qint64 ARQ_MEMBER_TRAILER_SIZE = 33;
const qint64 ARQ_TERMINATOR_SIZE =
    ARQ_MEMBER_PREFIX_SIZE + ARQ_MEMBER_TRAILER_SIZE;
const qint32 ARQ_MAX_MEMBERS = 100000;
const quint16 ARQ_MAX_NAME_SIZE = 4096;

bool arqRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool arqIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}

bool arqIsValidSignature(quint16 nSignature)
{
    const quint8 nLow = static_cast<quint8>(nSignature & 0xffU);
    return (nLow == ARQ_MEMBER_SIGNATURE_LOW_A) ||
           (nLow == ARQ_MEMBER_SIGNATURE_LOW_B);
}

quint8 arqMethodCode(quint16 nMethod)
{
    return static_cast<quint8>(nMethod & 0xffU);
}
}  // namespace

XARQ::XARQ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XARQ::~XARQ()
{
}

bool XARQ::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <
        ARQ_CONTAINER_HEADER_SIZE + ARQ_TERMINATOR_SIZE) {
        return false;
    }

    const QByteArray baContainer =
        read_array_process(0, ARQ_CONTAINER_HEADER_SIZE, pPdStruct);
    if (baContainer.size() != ARQ_CONTAINER_HEADER_SIZE) {
        return false;
    }
    const uchar *pContainer =
        reinterpret_cast<const uchar *>(baContainer.constData());
    const quint32 nLeadingMagic = qFromLittleEndian<quint32>(pContainer);

    // Crusher! emits the 12-byte container prelude only when it wraps the
    // chain (the SFX payload does).  A bare .ARQ/.SKU/.IRD file starts
    // directly at the first member record and has no total-size field, so the
    // sum-of-members cross-check only applies to the wrapped shape.
    if (nLeadingMagic == ARQ_CONTAINER_MAGIC) {
        if (qFromLittleEndian<quint32>(pContainer + 8) != 0) return false;
        context.bHasContainerHeader = true;
        context.nDeclaredUncompressedSize =
            qFromLittleEndian<quint32>(pContainer + 4);
    } else if (nLeadingMagic == ARQ_MEMBER_MAGIC) {
        context.bHasContainerHeader = false;
        context.nDeclaredUncompressedSize = 0;
    } else {
        return false;
    }

    qint64 nOffset =
        context.bHasContainerHeader ? ARQ_CONTAINER_HEADER_SIZE : 0;
    quint64 nTotalUncompressedSize = 0;
    while (context.listMembers.size() < ARQ_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!arqRangeWithin(context.nInputSize, nOffset,
                            ARQ_MEMBER_PREFIX_SIZE)) {
            return false;
        }
        const QByteArray baPrefix = read_array_process(
            nOffset, ARQ_MEMBER_PREFIX_SIZE, pPdStruct);
        if (baPrefix.size() != ARQ_MEMBER_PREFIX_SIZE) {
            return false;
        }
        const uchar *pPrefix =
            reinterpret_cast<const uchar *>(baPrefix.constData());
        const quint32 nMagic = qFromLittleEndian<quint32>(pPrefix);
        const quint16 nSignature = qFromLittleEndian<quint16>(pPrefix + 4);
        const quint16 nNameSize = qFromLittleEndian<quint16>(pPrefix + 6);
        if (nMagic != ARQ_MEMBER_MAGIC || !arqIsValidSignature(nSignature)) {
            return false;
        }

        if (nNameSize == 0) {
            // Crusher writes a complete header-shaped index record.  Its
            // remaining fields duplicate the final member and do not describe
            // another payload, but the complete 41-byte record must be present.
            if (context.listMembers.isEmpty() ||
                !arqRangeWithin(context.nInputSize, nOffset,
                                ARQ_TERMINATOR_SIZE) ||
                (context.bHasContainerHeader &&
                 (nTotalUncompressedSize !=
                  context.nDeclaredUncompressedSize))) {
                return false;
            }
            context.nArchiveSize = nOffset + ARQ_TERMINATOR_SIZE;
            context.nFirstMemberOffset =
                context.listMembers.first().nHeaderOffset;
            *pContext = context;
            return guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }
        if (nNameSize > ARQ_MAX_NAME_SIZE) return false;

        const qint64 nHeaderSize = ARQ_MEMBER_PREFIX_SIZE +
                                   static_cast<qint64>(nNameSize) +
                                   ARQ_MEMBER_TRAILER_SIZE;
        if (!arqRangeWithin(context.nInputSize, nOffset, nHeaderSize)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, nHeaderSize, pPdStruct);
        if (baHeader.size() != nHeaderSize) {
            return false;
        }
        const QByteArray baName =
            baHeader.mid(ARQ_MEMBER_PREFIX_SIZE, nNameSize);
        if (!arqIsValidName(baName)) return false;

        const qint32 nTrailerOffset =
            static_cast<qint32>(ARQ_MEMBER_PREFIX_SIZE + nNameSize);
        const uchar *pTrailer = reinterpret_cast<const uchar *>(
            baHeader.constData() + nTrailerOffset);
        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nHeaderSize;
        member.nDataOffset = nOffset + nHeaderSize;
        member.nMTime = qFromLittleEndian<quint32>(pTrailer);
        member.nAttributes = qFromLittleEndian<quint16>(pTrailer + 4);
        const quint32 nReserved = qFromLittleEndian<quint32>(pTrailer + 6);
        member.nCompressedSize =
            qFromLittleEndian<quint32>(pTrailer + 10);
        member.nUncompressedSize =
            qFromLittleEndian<quint32>(pTrailer + 14);
        member.nPackedCRC32 = qFromLittleEndian<quint32>(pTrailer + 18);
        member.nMethod = qFromLittleEndian<quint16>(pTrailer + 22);
        member.sFileName = QString::fromLatin1(baName)
                               .replace(QLatin1Char('\\'),
                                        QLatin1Char('/'));

        const quint8 nMethodCode = arqMethodCode(member.nMethod);
        if (nMethodCode != ARQ_METHOD_STORED &&
            nMethodCode != ARQ_METHOD_CRUSHED) {
            return false;
        }
        // Byte +24 is a per-member flag (0/1/2 observed); only the eight
        // bytes after it are structurally required to be zero.
        if (nReserved != 0 ||
            baHeader.mid(nTrailerOffset + 25, 8) != QByteArray(8, '\0') ||
            !arqRangeWithin(context.nInputSize, member.nDataOffset,
                            member.nCompressedSize)) {
            return false;
        }
        // A stored member is the anchor that keeps the relaxed method gate
        // honest: its two size fields must agree exactly.
        if ((nMethodCode == ARQ_METHOD_STORED) &&
            (member.nCompressedSize != member.nUncompressedSize)) {
            return false;
        }

        // This CRC belongs to the packed bytes.  Validate it here as a strong
        // structural anchor, then deliberately omit RESULTCRC from the public
        // member record so output verification cannot apply it to plaintext.
        const quint32 nCalculatedCRC =
            _getCRC32(member.nDataOffset, member.nCompressedSize,
                      0xffffffffU, _getCRC32Table_EDB88320(), pPdStruct);
        if (!isPdStructNotCanceled(pPdStruct) ||
            nCalculatedCRC != member.nPackedCRC32) {
            return false;
        }

        nTotalUncompressedSize +=
            static_cast<quint64>(member.nUncompressedSize);
        if (context.bHasContainerHeader &&
            (nTotalUncompressedSize >
             context.nDeclaredUncompressedSize)) {
            return false;
        }
        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;
    }

    return false;
}

bool XARQ::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition =
        guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XARQ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARQ archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XARQ::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XARQ(pDevice);
}

QList<QString> XARQ::getSearchSignatures()
{
    // 67570402 is the wrapped container prelude; 67570401 is the bare member
    // chain, which is how standalone Crusher! archives begin.
    return {QStringLiteral("67570402"), QStringLiteral("67570401")};
}

XBinary::FT XARQ::getFileType()
{
    return FT_ARQ;
}

XBinary::MODE XARQ::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XARQ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XARQ::getArch()
{
    return QString();
}

QString XARQ::getFileFormatExt()
{
    return QStringLiteral("arq");
}

QString XARQ::getFileFormatExtsString()
{
    return QStringLiteral("Crusher ARQ (*.arq)");
}

QString XARQ::getMIMEString()
{
    return QStringLiteral("application/x-arq");
}

QString XARQ::getVersion()
{
    return QString();
}

qint64 XARQ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XARQ::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XARQ::getMemoryMap(MAPMODE mapMode,
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

QString XARQ::methodToString(quint16 nMethod)
{
    const quint8 nMethodCode = arqMethodCode(nMethod);
    const QString sName = (nMethodCode == ARQ_METHOD_STORED)
                              ? QStringLiteral("Stored")
                              : ((nMethodCode == ARQ_METHOD_CRUSHED)
                                     ? QStringLiteral("Crushed (LH5-compatible)")
                                     : QStringLiteral("Unknown"));
    return QStringLiteral("Crusher 0x%1 %2")
        .arg(nMethod, 4, 16, QLatin1Char('0'))
        .arg(sName);
}

XBinary::HANDLE_METHOD XARQ::methodToHandleMethod(quint16 nMethod)
{
    const quint8 nMethodCode = arqMethodCode(nMethod);
    if (nMethodCode == ARQ_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethodCode == ARQ_METHOD_CRUSHED) return HANDLE_METHOD_LZH5;
    return HANDLE_METHOD_UNKNOWN;
}

bool XARQ::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XARQ::getFileParts(quint32 nFileParts,
                                         qint32 nLimit,
                                         PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if (context.bHasContainerHeader && (nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ARQ_CONTAINER_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Container header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize;
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
                                      methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member.nMethod));
            part.mapProperties.insert(FPART_PROP_TYPE,
                                      static_cast<quint32>(member.nMethod));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) &&
        context.nArchiveSize < context.nInputSize &&
        canAppendPart(nLimit, result.size())) {
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

QMap<XBinary::UNPACK_PROP, QVariant> XARQ::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XARQ::initUnpack(
    UNPACK_STATE *pState,
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
        tr("Crusher ARQ; packed member CRCs verified"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
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

XBinary::ARCHIVERECORD XARQ::infoCurrent(UNPACK_STATE *pState,
                                          PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext ||
        pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member =
        pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_ISREADONLY,
                                (member.nAttributes & 0222U) == 0);
    const QDateTime dtModified = QDateTime::fromSecsSinceEpoch(
        member.nMTime, X_UTC_TZ);
    if (dtModified.isValid()) {
        result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
    }
    return result;
}

bool XARQ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext ||
        pState->nCurrentIndex >= pContext->listMembers.size()) {
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

bool XARQ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
