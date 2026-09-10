/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xagis.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// +0x00 "AGIS" | +0x04 version | +0x05 method | +0x06 CRC32(plain) |
// +0x0a usize | +0x0e total record size | +0x12 name length |
// +0x13 name[12] | +0x1f payload.  4+1+1+4+4+4+1+12 = 31 accounts for every
// header byte; the format has no timestamp, attribute or path fields.
const qint64 AGIS_HEADER_SIZE = 31;
const quint8 AGIS_VERSION_10 = 0x10U;
const quint8 AGIS_METHOD_IMPLODE_BINARY = 0x00U;
const quint8 AGIS_METHOD_IMPLODE_ASCII = 0x01U;
const quint8 AGIS_METHOD_STORED = 0xffU;
const qint32 AGIS_NAME_BUFFER_SIZE = 12;
const qint32 AGIS_MAX_MEMBERS = 100000;
const qint64 AGIS_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap
// PKWARE DCL streams carry their own two-byte prelude: literal mode then
// dictionary-size bits.  Only 4..6 (1K/2K/4K windows) are legal; AGIS always
// writes 6.
const quint8 AGIS_DCL_MIN_DICT_BITS = 4U;
const quint8 AGIS_DCL_MAX_DICT_BITS = 6U;

bool agisRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool agisIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
    }
    return true;
}

bool agisIsKnownMethod(quint8 nMethod)
{
    return (nMethod == AGIS_METHOD_IMPLODE_BINARY) ||
           (nMethod == AGIS_METHOD_IMPLODE_ASCII) ||
           (nMethod == AGIS_METHOD_STORED);
}
}  // namespace

XAGIS::XAGIS(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAGIS::~XAGIS()
{
}

bool XAGIS::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XAGIS> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // A record without at least one payload byte cannot exist: even a stored
    // zero-length member would still be rejected by the size cross-checks.
    if (context.nInputSize < AGIS_HEADER_SIZE + 1) return false;

    qint64 nOffset = 0;
    while (context.listMembers.size() < AGIS_MAX_MEMBERS &&
           isPdStructNotCanceled(pPdStruct)) {
        if (!agisRangeWithin(context.nInputSize, nOffset, AGIS_HEADER_SIZE)) {
            return false;
        }
        const QByteArray baHeader =
            read_array_process(nOffset, AGIS_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baHeader.size() != AGIS_HEADER_SIZE) {
            return false;
        }
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());

        // The magic is repeated on every record, not just the first one; that
        // is what makes a truncated or spliced chain fail closed instead of
        // being read as garbage members.
        if (std::memcmp(pHeader, "AGIS", 4) != 0) return false;
        if (pHeader[4] != AGIS_VERSION_10) return false;

        const quint8 nMethod = pHeader[5];
        if (!agisIsKnownMethod(nMethod)) return false;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + AGIS_HEADER_SIZE;
        member.nMethod = nMethod;
        member.nCRC32 = qFromLittleEndian<quint32>(pHeader + 6);
        member.nUncompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 10));
        const qint64 nRecordSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 14));
        if (nRecordSize < AGIS_HEADER_SIZE + 1 ||
            !agisRangeWithin(context.nInputSize, nOffset, nRecordSize)) {
            return false;
        }
        member.nCompressedSize = nRecordSize - AGIS_HEADER_SIZE;
        if (member.nUncompressedSize > AGIS_MAX_UNCOMPRESSED_SIZE) {
            return false;
        }

        // The 12-byte name buffer is NOT NUL-terminated and its tail holds
        // stale bytes from a previously written name ("COTTAGE.R" is stored
        // over a buffer reading "COTTAGE.R2Dt").  The length byte is the only
        // authority; never scan for a terminator and never trim.
        const qint32 nNameSize = static_cast<qint32>(pHeader[18]);
        if (nNameSize < 1 || nNameSize > AGIS_NAME_BUFFER_SIZE) return false;
        const QByteArray baName = baHeader.mid(19, nNameSize);
        if (!agisIsValidName(baName)) return false;
        member.sFileName =
            QString::fromLatin1(baName).replace(QLatin1Char('\\'),
                                                QLatin1Char('/'));

        if (nMethod == AGIS_METHOD_STORED) {
            // The stored members are the structural anchor that keeps the
            // three-value method gate honest: both size fields must agree.
            if (member.nCompressedSize != member.nUncompressedSize) {
                return false;
            }
        } else {
            // The payload is a complete PKWARE DCL Implode stream including
            // its own two-byte prelude, and the record's method byte is a
            // verbatim copy of that stream's literal-mode byte.  Cross-checking
            // the two is the strongest discriminator this format offers.
            if (member.nCompressedSize < 3) return false;
            const QByteArray baPrelude =
                read_array_process(member.nDataOffset, 2, pPdStruct);
            if (!guardedThis || !guardedSource || baPrelude.size() != 2) {
                return false;
            }
            if (static_cast<quint8>(baPrelude.at(0)) != nMethod) return false;
            const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
            if (nDictBits < AGIS_DCL_MIN_DICT_BITS ||
                nDictBits > AGIS_DCL_MAX_DICT_BITS) {
                return false;
            }
        }

        context.listMembers.append(member);
        nOffset += nRecordSize;

        // There is no terminator record: the chain ends by landing exactly on
        // EOF.  A short tail is a reject, not an overlay - no AGIS archive has
        // slack, and accepting one would turn any prefix match into a hit.
        if (nOffset == context.nInputSize) {
            context.nArchiveSize = nOffset;
            context.nFirstMemberOffset =
                context.listMembers.first().nHeaderOffset;
            *pContext = context;
            return guardedThis && guardedSource &&
                   isPdStructNotCanceled(pPdStruct);
        }
    }

    return false;
}

bool XAGIS::isValid(PDSTRUCT *pPdStruct)
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

bool XAGIS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAGIS archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAGIS::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAGIS(pDevice);
}

QList<QString> XAGIS::getSearchSignatures()
{
    // Include the version byte: "AGIS" alone is a common enough four-letter
    // run inside unrelated payloads to be worth pinning down.
    return {QStringLiteral("'AGIS'10")};
}

XBinary::FT XAGIS::getFileType()
{
    return FT_AGIS;
}

XBinary::MODE XAGIS::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAGIS::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XAGIS::getArch()
{
    return QString();
}

QString XAGIS::getFileFormatExt()
{
    return QStringLiteral("ags");
}

QString XAGIS::getFileFormatExtsString()
{
    return QStringLiteral("AGIS install archive (*.ags)");
}

QString XAGIS::getMIMEString()
{
    return QStringLiteral("application/x-agis");
}

QString XAGIS::getVersion()
{
    // Byte +4 is 0x10 on every record of every known archive; the gate in
    // parseContext() rejects anything else, so the version is a constant here.
    return QStringLiteral("1.0");
}

qint64 XAGIS::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAGIS::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAGIS::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XAGIS::methodToString(quint8 nMethod)
{
    if (nMethod == AGIS_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == AGIS_METHOD_IMPLODE_BINARY) {
        return QStringLiteral("PKWARE DCL Implode (binary literals)");
    }
    if (nMethod == AGIS_METHOD_IMPLODE_ASCII) {
        return QStringLiteral("PKWARE DCL Implode (ASCII literals)");
    }
    return QStringLiteral("Unknown 0x%1")
        .arg(nMethod, 2, 16, QLatin1Char('0'));
}

XBinary::HANDLE_METHOD XAGIS::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == AGIS_METHOD_STORED) return HANDLE_METHOD_STORE;
    if ((nMethod == AGIS_METHOD_IMPLODE_BINARY) ||
        (nMethod == AGIS_METHOD_IMPLODE_ASCII)) {
        // The stream is handed over from +0x1f with its own two-byte prelude
        // still attached: decPkwareDcl reads the literal-mode and dictionary
        // bits off the front of the packed buffer itself.
        return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    }
    return HANDLE_METHOD_UNKNOWN;
}

bool XAGIS::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XAGIS::getFileParts(quint32 nFileParts, qint32 nLimit,
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
            part.nFileSize = AGIS_HEADER_SIZE;
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
            // The CRC covers the UNPACKED member, so it is published for the
            // unpack path.  Verifying it in parseContext() would decompress
            // the whole archive on every format probe.
            part.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
            part.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                      CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = AGIS_HEADER_SIZE + member.nCompressedSize;
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
        context.nArchiveSize < context.nInputSize &&
        canAppendPart(nLimit, result.size())) {
        // parseContext() only accepts a chain that lands on EOF, so this
        // branch cannot fire today; it is kept so the part list stays correct
        // if the acceptance rule is ever relaxed for split-disk parts.
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

QMap<XBinary::UNPACK_PROP, QVariant> XAGIS::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAGIS::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QPointer<XAGIS> guardedThis(this);
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("AGIS 1.0 install archive; stored and PKWARE DCL Implode members"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XAGIS::infoCurrent(UNPACK_STATE *pState,
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
                                methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
    result.mapProperties.insert(FPART_PROP_CRC_TYPE,
                                CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    // No MTIME/attribute properties: the 31 header bytes are fully accounted
    // for and AGIS simply has no such fields.
    return result;
}

bool XAGIS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XAGIS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

