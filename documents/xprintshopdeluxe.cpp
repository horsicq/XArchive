/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xprintshopdeluxe.h"

#include "Algos/xdcldecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
// char name[76] | quint32 totalSize
const qint64 PSD_NAME_SIZE = 76;
const qint64 PSD_HEADER_SIZE = PSD_NAME_SIZE + 4;
// A stream cannot be shorter than its own two prelude bytes plus a single byte
// holding the start of the end-of-stream code.
const qint64 PSD_MIN_PACKED_SIZE = 3;
// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext length
// is controlled by the bitstream, so the decoder must stay bounded.
const qint64 PSD_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// PKWARE DCL prelude: literal mode (0 binary / 1 Huffman) then dictionary bits.
// Only 4..6 (1K/2K/4K) are legal.  Every member of the reference corpus writes
// 0/6, but the gate accepts the whole legal range because the stream format does.
const quint8 PSD_DCL_MAX_LITERAL_MODE = 1U;
const quint8 PSD_DCL_MIN_DICT_BITS = 4U;
const quint8 PSD_DCL_MAX_DICT_BITS = 6U;
// The writer is a DOS / Windows 3.1 installer, so the stored name is a bare DOS
// 8.3 name - never a path.  Twelve characters is therefore a hard ceiling.
const int PSD_MAX_NAME_LENGTH = 12;
const int PSD_MAX_BASE_LENGTH = 8;
const int PSD_MAX_EXT_LENGTH = 3;

bool printShopIsDclPrelude(const QByteArray &baPrelude)
{
    if (baPrelude.size() < 2) return false;
    const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));
    return (nLiteralMode <= PSD_DCL_MAX_LITERAL_MODE) &&
           (nDictBits >= PSD_DCL_MIN_DICT_BITS) &&
           (nDictBits <= PSD_DCL_MAX_DICT_BITS);
}

bool printShopIsNameCharacter(quint8 nCharacter)
{
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    // The remaining characters DOS allows inside an 8.3 name.  Everything else -
    // separators, wildcards, spaces, high bytes - means this is not the header.
    switch (nCharacter) {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '-':
        case '@':
        case '^':
        case '_':
        case '`':
        case '{':
        case '}':
        case '~': return true;
        default: return false;
    }
}

// There is no magic number in this container, so the 76-byte name field carries
// most of the weight of isValid().  Accept only what the writer can actually
// emit: a non-empty DOS 8.3 name, then nothing but NULs to the end of the field.
bool printShopIsNameField(const QByteArray &baField, QString *pName)
{
    if (baField.size() != PSD_NAME_SIZE) return false;

    const int nTerminator = baField.indexOf('\0');
    if ((nTerminator <= 0) || (nTerminator > PSD_MAX_NAME_LENGTH)) return false;
    for (int i = nTerminator; i < baField.size(); i++) {
        // A single stale byte behind the terminator would mean the 76-byte field
        // is not the fixed, zero-filled buffer this parser assumes.
        if (baField.at(i) != '\0') return false;
    }

    int nDotPosition = -1;
    for (int i = 0; i < nTerminator; i++) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if (nCharacter == '.') {
            // A DOS name carries at most one separator, and never leads with it.
            if ((nDotPosition >= 0) || (i == 0)) return false;
            nDotPosition = i;
            continue;
        }
        if (!printShopIsNameCharacter(nCharacter)) return false;
    }

    const int nBaseLength = (nDotPosition >= 0) ? nDotPosition : nTerminator;
    const int nExtLength =
        (nDotPosition >= 0) ? (nTerminator - nDotPosition - 1) : 0;
    if ((nBaseLength < 1) || (nBaseLength > PSD_MAX_BASE_LENGTH)) return false;
    if (nExtLength > PSD_MAX_EXT_LENGTH) return false;
    // "NAME." with an empty extension is not something the writer produces.
    if ((nDotPosition >= 0) && (nExtLength < 1)) return false;

    if (pName) *pName = QString::fromLatin1(baField.constData(), nTerminator);
    return true;
}
}  // namespace

XPrintShopDeluxe::XPrintShopDeluxe(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPrintShopDeluxe::~XPrintShopDeluxe()
{
}

bool XPrintShopDeluxe::scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPrintShopDeluxe> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const QByteArray baPacked = read_array_process(
        pMember->nDataOffset, pMember->nCompressedSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baPacked.size() != pMember->nCompressedSize) {
        return false;
    }

    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()),
            pMember->nCompressedSize, PSD_MAX_UNCOMPRESSED_SIZE, &nConsumed,
            &nRawSize)) {
        return false;
    }
    // VERIFIED INVARIANT over the 88-file reference corpus: the DCL stream ends
    // exactly at end of file, with zero trailing slack.  A mismatch means the
    // 0x4C total-size field and the payload disagree, so the recovered plaintext
    // length cannot be trusted for extraction.
    if (nConsumed != pMember->nCompressedSize) return false;
    if ((nRawSize < 0) || (nRawSize > PSD_MAX_UNCOMPRESSED_SIZE)) return false;

    pMember->nUncompressedSize = nRawSize;
    pMember->bUncompressedSizeKnown = true;
    return true;
}

bool XPrintShopDeluxe::parseContext(CONTEXT *pContext, bool bScanSize,
                                    PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPrintShopDeluxe> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PSD_HEADER_SIZE + PSD_MIN_PACKED_SIZE) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, PSD_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != PSD_HEADER_SIZE)) {
        return false;
    }

    MEMBER member = {};
    // Fixed-width name field: hand the checker all 76 bytes.  Reading one byte
    // short would reject a name that fills the buffer to its terminator.
    if (!printShopIsNameField(baHeader.left(static_cast<int>(PSD_NAME_SIZE)),
                              &member.sFileName)) {
        return false;
    }

    const qint64 nTotalSize = static_cast<qint64>(qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baHeader.constData()) + PSD_NAME_SIZE));
    // The 0x4C field counts THIS file, prologue included.  It matching the real
    // device size is the container's only self-check, and this class treats a
    // mismatch as a reject: without it a printable-then-NUL run anywhere would
    // pass the gate.
    if (nTotalSize != context.nInputSize) return false;

    member.nDataOffset = PSD_HEADER_SIZE;
    member.nCompressedSize = context.nInputSize - PSD_HEADER_SIZE;
    member.nUncompressedSize = 0;
    member.bUncompressedSizeKnown = false;
    if (member.nCompressedSize < PSD_MIN_PACKED_SIZE) return false;

    const QByteArray baPrelude =
        read_array_process(member.nDataOffset, 2, pPdStruct);
    if (!guardedThis || !guardedSource || (baPrelude.size() != 2) ||
        !printShopIsDclPrelude(baPrelude)) {
        return false;
    }

    context.listMembers.append(member);

    if (bScanSize) {
        // A member the decoder cannot measure stays in the listing with its size
        // unknown; methodToHandleMethod() then reports it as UNKNOWN so
        // extraction refuses it instead of writing a truncated or empty file.
        scanMemberSize(&context.listMembers[0], pPdStruct);
        if (!guardedThis || !guardedSource) return false;
    }

    context.nArchiveSize = context.nInputSize;
    context.nFirstMemberOffset = context.listMembers.first().nDataOffset;
    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XPrintShopDeluxe::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XPrintShopDeluxe::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPrintShopDeluxe archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPrintShopDeluxe::createInstance(QIODevice *pDevice, bool bIsImage,
                                          XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPrintShopDeluxe(pDevice);
}

XBinary::FT XPrintShopDeluxe::getFileType()
{
    return FT_PRINTSHOP_DELUXE;
}

XBinary::MODE XPrintShopDeluxe::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPrintShopDeluxe::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPrintShopDeluxe::getArch()
{
    return QString();
}

QString XPrintShopDeluxe::getFileFormatExt()
{
    // The packed files keep the DOS convention of replacing the last extension
    // character (TRIBUNE.TT_, PSDWIN.HL$), so the container has no stable
    // extension of its own to report.
    return QString();
}

QString XPrintShopDeluxe::getFileFormatExtsString()
{
    return QStringLiteral("Print Shop Deluxe install file");
}

QString XPrintShopDeluxe::getMIMEString()
{
    return QStringLiteral("application/x-printshop-deluxe");
}

QString XPrintShopDeluxe::getVersion()
{
    return QString();
}

qint64 XPrintShopDeluxe::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPrintShopDeluxe::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XPrintShopDeluxe::getMemoryMap(MAPMODE mapMode,
                                                    PDSTRUCT *pPdStruct)
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

QString XPrintShopDeluxe::methodToString(const MEMBER &member)
{
    if (!member.bUncompressedSizeKnown) {
        return QStringLiteral("PKWARE DCL Implode/TTCOMP (undecodable)");
    }
    return QStringLiteral("PKWARE DCL Implode/TTCOMP");
}

XBinary::HANDLE_METHOD XPrintShopDeluxe::methodToHandleMethod(
    const MEMBER &member)
{
    // decPkwareDcl() consumes the packed buffer with its two-byte prelude still
    // attached, but it takes the plaintext length as an INPUT.  Without a
    // measured length the only honest answer is UNKNOWN.
    if (!member.bUncompressedSizeKnown) return HANDLE_METHOD_UNKNOWN;
    return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
}

bool XPrintShopDeluxe::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPrintShopDeluxe::getFileParts(quint32 nFileParts,
                                                     qint32 nLimit,
                                                     PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PSD_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    const qint32 nCount = static_cast<qint32>(context.listMembers.size());
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
            break;
        }
        const MEMBER &member = context.listMembers.at(i);
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      methodToHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member));
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
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

QMap<XBinary::UNPACK_PROP, QVariant>
XPrintShopDeluxe::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPrintShopDeluxe::initUnpack(
    UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XPrintShopDeluxe> guardedThis(this);
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
    // bScanSize = true: the extraction path needs the plaintext length, and the
    // container does not store it anywhere.
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis ||
        !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Print Shop Deluxe install file; PKWARE DCL Implode stream"));
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

XBinary::ARCHIVERECORD XPrintShopDeluxe::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nDataOffset) {
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
                                methodToHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No timestamp and no checksum properties: the container carries neither.
    return result;
}

bool XPrintShopDeluxe::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XPrintShopDeluxe::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
