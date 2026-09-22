/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xbwfarchive.h"

#include "Algos/xdcldecoder.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 BWF_RECORD_HEADER_SIZE = 0x16;
const qint64 BWF_NAME_OFFSET = 0x01;
// The field is 13 bytes wide but the reference reader NULs its last byte before
// reading it, so only 12 of them can ever reach a name.
const qint32 BWF_MAX_NAME_SIZE = 12;
const qint32 BWF_MAX_STEM_SIZE = 8;
const qint32 BWF_MAX_EXT_SIZE = 3;
const qint64 BWF_DATETIME_OFFSET = 0x0e;
const qint64 BWF_PACKEDSIZE_OFFSET = 0x12;
const quint8 BWF_RECORD_TAG = 0x01;
// A DCL stream cannot be shorter than its two-byte prelude plus one coded byte.
const qint64 BWF_MIN_PACKED_SIZE = 3;
const qint64 BWF_MIN_ARCHIVE_SIZE = BWF_RECORD_HEADER_SIZE + BWF_MIN_PACKED_SIZE;
const qint32 BWF_MAX_MEMBERS = 100000;
// Matches MAX_LEGACY_STORE_SIZE in xlegacystorearchive.cpp: the plaintext length
// is driven by the bitstream, so the decoder must stay bounded.
const qint64 BWF_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// PKWARE DCL prelude: literal mode (0 binary / 1 Huffman) then dictionary bits,
// of which only 4..6 (1K/2K/4K) are legal.
const quint8 BWF_DCL_MAX_LITERAL_MODE = 1U;
const quint8 BWF_DCL_MIN_DICT_BITS = 4U;
const quint8 BWF_DCL_MAX_DICT_BITS = 6U;

bool bwfRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool bwfIsNameCharacter(char cCharacter)
{
    const quint8 nCharacter = static_cast<quint8>(cCharacter);
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;

    // The rest of the DOS 8.3 charset.  '&' is in it because the corpus really
    // does ship AT&T.COM and AT&T_LP.COM.  Path separators and spaces are
    // excluded on purpose: this format has no directories, so a name carrying
    // one is a mis-parse rather than a subfolder.
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

bool bwfIsValidName(const QByteArray &baName)
{
    const qint32 nSize = static_cast<qint32>(baName.size());
    if ((nSize < 1) || (nSize > BWF_MAX_NAME_SIZE)) return false;

    qint32 nStemLength = 0;
    qint32 nExtLength = 0;
    bool bHasDot = false;

    for (qint32 i = 0; i < nSize; i++) {
        const char cCharacter = baName.at(i);
        if (cCharacter == '.') {
            // A leading dot, or a second one, cannot occur in a DOS 8.3 name.
            if (bHasDot || (i == 0)) return false;
            bHasDot = true;
            continue;
        }
        if (!bwfIsNameCharacter(cCharacter)) return false;
        if (bHasDot) {
            nExtLength++;
        } else {
            nStemLength++;
        }
    }

    if ((nStemLength < 1) || (nStemLength > BWF_MAX_STEM_SIZE)) return false;
    if (nExtLength > BWF_MAX_EXT_SIZE) return false;
    if (bHasDot && (nExtLength < 1)) return false;

    return true;
}

// The name is the NUL-terminated string in +0x01..+0x0C; byte +0x0D is the
// terminator slot (the reference reader NULs it before reading the field, and it
// is 0 in all 174 corpus records whose name is a full 12 characters).  Stale
// bytes follow the terminator at any position - 496 of 1288 records carry one
// inside the 12-byte window, 350 at +0x0D - so this scan MUST stop at the NUL.
// Reading the field fixed-width, at either 12 or 13 bytes, is what would embed
// junk in a name.
QByteArray bwfReadName(const QByteArray &baHeader)
{
    const qint32 nAvailable = static_cast<qint32>(baHeader.size()) - static_cast<qint32>(BWF_NAME_OFFSET);
    if (nAvailable < BWF_MAX_NAME_SIZE) return QByteArray();

    const char *pName = baHeader.constData() + BWF_NAME_OFFSET;
    qint32 nLength = 0;
    while ((nLength < BWF_MAX_NAME_SIZE) && (pName[nLength] != '\0')) nLength++;

    return QByteArray(pName, nLength);
}

bool bwfIsDclPrelude(const QByteArray &baPrelude)
{
    if (baPrelude.size() < 2) return false;
    const quint8 nLiteralMode = static_cast<quint8>(baPrelude.at(0));
    const quint8 nDictBits = static_cast<quint8>(baPrelude.at(1));

    return (nLiteralMode <= BWF_DCL_MAX_LITERAL_MODE) && (nDictBits >= BWF_DCL_MIN_DICT_BITS) && (nDictBits <= BWF_DCL_MAX_DICT_BITS);
}
}  // namespace

XBWFArchive::XBWFArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBWFArchive::~XBWFArchive()
{
}

bool XBWFArchive::scanMemberSize(MEMBER *pMember, PDSTRUCT *pPdStruct)
{
    if (!pMember || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    const QByteArray baPacked = read_array_process(pMember->nDataOffset, pMember->nCompressedSize, pPdStruct);
    if ((baPacked.size() != pMember->nCompressedSize)) return false;

    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(reinterpret_cast<const uchar *>(baPacked.constData()), pMember->nCompressedSize, BWF_MAX_UNCOMPRESSED_SIZE, &nConsumed, &nRawSize)) {
        return false;
    }

    // VERIFIED INVARIANT over the 146-file / 1288-member reference corpus: the
    // record's packed size is exactly the bitstream boundary the decoder stops
    // at, on every single member.  A mismatch means the chain and the payload
    // disagree, so the recovered length cannot be trusted for extraction.
    if (nConsumed != pMember->nCompressedSize) return false;
    if ((nRawSize < 0) || (nRawSize > BWF_MAX_UNCOMPRESSED_SIZE)) return false;

    pMember->nUncompressedSize = nRawSize;
    pMember->bUncompressedSizeKnown = true;

    return true;
}

bool XBWFArchive::parseContext(CONTEXT *pContext, bool bScanSizes, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BWF_MIN_ARCHIVE_SIZE) return false;

    qint64 nOffset = 0;
    bool bClosedOnEof = false;

    while ((context.listMembers.size() < BWF_MAX_MEMBERS) && isPdStructNotCanceled(pPdStruct)) {
        if (!bwfRangeWithin(context.nInputSize, nOffset, BWF_RECORD_HEADER_SIZE)) return false;

        const QByteArray baHeader = read_array_process(nOffset, BWF_RECORD_HEADER_SIZE, pPdStruct);
        if ((baHeader.size() != BWF_RECORD_HEADER_SIZE)) return false;

        const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());
        if (pHeader[0] != BWF_RECORD_TAG) return false;

        const QByteArray baName = bwfReadName(baHeader);
        if (!bwfIsValidName(baName)) return false;

        const quint32 nDateTime = qFromLittleEndian<quint32>(pHeader + BWF_DATETIME_OFFSET);

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + BWF_RECORD_HEADER_SIZE;
        member.nCompressedSize = static_cast<qint64>(static_cast<qint32>(qFromLittleEndian<quint32>(pHeader + BWF_PACKEDSIZE_OFFSET)));
        member.nDosTime = static_cast<quint16>(nDateTime & 0xffffU);
        member.nDosDate = static_cast<quint16>((nDateTime >> 16) & 0xffffU);
        member.sFileName = QString::fromLatin1(baName);
        // The container never stores the plaintext length; it is filled in below
        // only on the paths that actually asked for it.
        member.nUncompressedSize = 0;
        member.bUncompressedSizeKnown = false;

        if (member.nCompressedSize < BWF_MIN_PACKED_SIZE) return false;
        if (!bwfRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;

        const QByteArray baPrelude = read_array_process(member.nDataOffset, 2, pPdStruct);
        if ((baPrelude.size() != 2) || !bwfIsDclPrelude(baPrelude)) return false;

        context.listMembers.append(member);
        nOffset = member.nDataOffset + member.nCompressedSize;

        // There is no terminator record, no count and no global size field: the
        // chain ends by landing exactly on EOF.  Slack is a reject rather than
        // an overlay, because with no magic that exact tiling IS the signature.
        if (nOffset == context.nInputSize) {
            bClosedOnEof = true;
            break;
        }
    }

    if (!bClosedOnEof || context.listMembers.isEmpty()) return false;

    // Only now, on a file that already tiles exactly with 8.3 names and DCL
    // preludes throughout, is a real decode worth paying for.  Two archives in
    // the corpus hold a single member, so the chain alone cannot be asked to
    // prove itself twice - this decode is what carries those two.
    if (!scanMemberSize(&context.listMembers.first(), pPdStruct)) return false;

    if (bScanSizes) {
        const qint32 nCount = static_cast<qint32>(context.listMembers.size());
        for (qint32 i = 1; i < nCount; i++) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            // A member the decoder cannot measure stays in the listing with its
            // size unknown; methodToHandleMethod() then reports UNKNOWN so
            // extraction refuses it instead of writing a truncated file.
            scanMemberSize(&context.listMembers[i], pPdStruct);
            if (!guardedSource) return false;
        }
    }

    context.nArchiveSize = nOffset;
    *pContext = context;

    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XBWFArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XBWFArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBWFArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBWFArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XBWFArchive(pDevice);
}

// getSearchSignatures() is deliberately NOT overridden: the format has no magic
// at all.  Byte 0 is a record tag of 0x01 and everything after it is a name, so
// any byte pattern registered here would be a lie.

XBinary::FT XBWFArchive::getFileType()
{
    return FT_BWF;
}

XBinary::MODE XBWFArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBWFArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBWFArchive::getArch()
{
    return QString();
}

qint32 XBWFArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XBWFArchive::getFileFormatExt()
{
    return QStringLiteral("bwf");
}

QString XBWFArchive::getFileFormatExtsString()
{
    return QStringLiteral("Beame & Whiteside distribution file (*.bwf)");
}

QString XBWFArchive::getMIMEString()
{
    return QStringLiteral("application/x-bwf");
}

QString XBWFArchive::getVersion()
{
    // The container carries no version field; the record tag 0x01 is a tag, not
    // a revision - every member of every corpus file has it.
    return QString();
}

qint64 XBWFArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBWFArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XBWFArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XBWFArchive::methodToString(const MEMBER &member)
{
    if (!member.bUncompressedSizeKnown) return QStringLiteral("PKWARE DCL Implode (undecodable)");

    return QStringLiteral("PKWARE DCL Implode");
}

XBinary::HANDLE_METHOD XBWFArchive::methodToHandleMethod(const MEMBER &member)
{
    // decPkwareDcl() takes the plaintext length as an INPUT, so without a
    // measured length the only honest answer is UNKNOWN.
    if (!member.bUncompressedSizeKnown) return HANDLE_METHOD_UNKNOWN;

    return HANDLE_METHOD_PKWARE_DCL_IMPLODE;
}

bool XBWFArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBWFArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    const qint32 nCount = static_cast<qint32>(context.listMembers.size());
    for (qint32 i = 0; i < nCount; i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = BWF_RECORD_HEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            listResult.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member));
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
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        // parseContext() only accepts a chain that lands exactly on EOF, so this
        // cannot fire today; it is kept so the part list stays correct if that
        // rule is ever relaxed for a split set.
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBWFArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBWFArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    // bScanSizes = true: extraction needs the plaintext length of every member
    // and the container does not store it.
    if (!parseContext(pContext, true, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Beame & Whiteside distribution file; PKWARE DCL Implode members"));
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XBWFArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        result.mapProperties.insert(FPART_PROP_MTIME, dosDateTimeToQDateTime(member.nDosDate, member.nDosTime));
    }

    return result;
}

bool XBWFArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XBWFArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XBWFArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_MTIME;
}
