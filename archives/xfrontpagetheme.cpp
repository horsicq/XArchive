/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xfrontpagetheme.h"


#include <new>

namespace {
// The marker that introduces every member payload.  Fourteen bytes, and it
// is a PREFIX, not an infix: a zero-length member (every theme has one)
// puts two of them back to back.
const char FPT_MARKER[] = "<==MS-Theme==>";
const qint64 FPT_MARKER_SIZE = 14;

// "3.0.2.1330\n" plus "1\n" plus a one-character name, a comma, a "0" and
// the newline, plus one marker.  Anything shorter cannot be a theme.
const qint64 FPT_MIN_SIZE = 24;

// The text header is read in one go; these bound how far the scan may run
// before giving up on a file that merely happens to start with digits.
const qint32 FPT_MAX_VERSION_LENGTH = 32;
const qint32 FPT_MAX_COUNT_DIGITS = 7;
const qint32 FPT_MAX_LINE_LENGTH = 300;
const qint32 FPT_MAX_NAME_LENGTH = 255;
const qint32 FPT_MAX_MEMBERS = 65536;
const qint64 FPT_MAX_MEMBER_SIZE = 0x10000000;  // 256 MB sanity cap
// The two lines in front of the directory: a version of at most 32 bytes and
// a count of at most 7 digits, each with its newline.
const qint64 FPT_PREAMBLE_MAX = 64;
// Cheapest possible member: a four-byte directory line ("a,0\n") plus the
// fourteen-byte marker.  Used to reject an inflated count before any large
// read is attempted.
const qint64 FPT_MIN_MEMBER_COST = 18;

bool fptRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// A member name is a bare 8.3-ish file name: printable ASCII, no path
// separators, no drive letters, no "..".  The corpus only ever uses
// lowercase letters, digits, '.' and '_', but the check is deliberately a
// little wider so an unusual theme is not rejected outright.
bool fptIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty() || baName.size() > FPT_MAX_NAME_LENGTH) return false;
    if ((baName == ".") || (baName == "..")) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter > 0x7e) return false;
        if ((c == '/') || (c == '\\') || (c == ':') || (c == '*') ||
            (c == '?') || (c == '"') || (c == '<') || (c == '>') ||
            (c == '|') || (c == ',')) {
            return false;
        }
    }
    return true;
}

// The version line is a dotted decimal ("3.0.2.1330", "3.0.2.926").  It is
// the only thing in front of the member count, so it has to carry the whole
// weight of the cheap first-pass rejection.
bool fptIsValidVersion(const QByteArray &baVersion)
{
    if (baVersion.isEmpty() || baVersion.size() > FPT_MAX_VERSION_LENGTH) {
        return false;
    }
    if (baVersion.at(0) < '0' || baVersion.at(0) > '9') return false;
    if (baVersion.endsWith('.')) return false;
    qint32 nDots = 0;
    char cPrevious = 0;
    for (char c : baVersion) {
        if (c == '.') {
            if (cPrevious == '.') return false;
            ++nDots;
        } else if ((c < '0') || (c > '9')) {
            return false;
        }
        cPrevious = c;
    }
    return nDots >= 1;
}

// Strict unsigned decimal: no sign, no whitespace, no leading zero (the
// generator never emits one), and it must fit the supplied cap.
bool fptParseDecimal(const QByteArray &baValue, qint32 nMaxDigits,
                     qint64 nMaxValue, qint64 *pnResult)
{
    if (baValue.isEmpty() || baValue.size() > nMaxDigits) return false;
    if ((baValue.size() > 1) && (baValue.at(0) == '0')) return false;
    qint64 nResult = 0;
    for (char c : baValue) {
        if ((c < '0') || (c > '9')) return false;
        nResult = nResult * 10 + static_cast<qint64>(c - '0');
        if (nResult > nMaxValue) return false;
    }
    *pnResult = nResult;
    return true;
}
}  // namespace

XFrontPageTheme::XFrontPageTheme(QIODevice *pDevice) : XArchive(pDevice)
{
}

XFrontPageTheme::~XFrontPageTheme()
{
}

bool XFrontPageTheme::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < FPT_MIN_SIZE) return false;

    // Read the preamble first so the member count is known before anything
    // bigger is pulled in - the directory read is then sized from the count
    // rather than from a fixed guess.
    const qint64 nPreambleSize =
        qMin<qint64>(context.nInputSize, FPT_PREAMBLE_MAX);
    const QByteArray baPreamble =
        read_array_process(0, nPreambleSize, pPdStruct);
    if (baPreamble.size() != nPreambleSize) {
        return false;
    }

    qint32 nPosition = baPreamble.indexOf('\n');
    if ((nPosition < 0) || (nPosition > FPT_MAX_VERSION_LENGTH)) return false;
    const QByteArray baVersion = baPreamble.left(nPosition);
    if (!fptIsValidVersion(baVersion)) return false;
    ++nPosition;

    qint32 nLineEnd = baPreamble.indexOf('\n', nPosition);
    if ((nLineEnd < 0) || (nLineEnd - nPosition > FPT_MAX_COUNT_DIGITS)) {
        return false;
    }
    qint64 nCount = 0;
    if (!fptParseDecimal(baPreamble.mid(nPosition, nLineEnd - nPosition),
                         FPT_MAX_COUNT_DIGITS, FPT_MAX_MEMBERS, &nCount)) {
        return false;
    }
    if (nCount < 1) return false;
    nPosition = nLineEnd + 1;
    // Every member costs at least a directory line and a marker, so a count
    // the file cannot possibly hold is rejected here, before the read.
    if (nCount > (context.nInputSize - nPosition) / FPT_MIN_MEMBER_COST) {
        return false;
    }

    const qint64 nProbeSize = qMin<qint64>(
        context.nInputSize,
        static_cast<qint64>(nPosition) +
            nCount * static_cast<qint64>(FPT_MAX_LINE_LENGTH + 1));
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (baProbe.size() != nProbeSize) {
        return false;
    }

    context.sVersion = QString::fromLatin1(baVersion);
    context.nDirectoryOffset = 0;

    // Pass 1: the directory.  Names and sizes only; the payload offsets are
    // computed afterwards because each one depends on every size before it.
    QList<MEMBER> listMembers;
    listMembers.reserve(static_cast<qint32>(nCount));
    qint64 nTotalPayload = 0;

    for (qint64 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        nLineEnd = baProbe.indexOf('\n', nPosition);
        if (nLineEnd < 0) return false;
        if (nLineEnd - nPosition > FPT_MAX_LINE_LENGTH) return false;
        const QByteArray baLine = baProbe.mid(nPosition, nLineEnd - nPosition);
        nPosition = nLineEnd + 1;

        // The size always follows the LAST comma, so a name may not contain
        // one; fptIsValidName() enforces that, which keeps the split
        // unambiguous in both directions.
        const qint32 nComma = baLine.lastIndexOf(',');
        if (nComma <= 0) return false;
        const QByteArray baName = baLine.left(nComma);
        if (!fptIsValidName(baName)) return false;
        qint64 nSize = 0;
        if (!fptParseDecimal(baLine.mid(nComma + 1), 10, FPT_MAX_MEMBER_SIZE,
                             &nSize)) {
            return false;
        }

        MEMBER member = {};
        member.sFileName = QString::fromLatin1(baName);
        member.nSize = nSize;
        listMembers.append(member);

        nTotalPayload += nSize + FPT_MARKER_SIZE;
        if (nTotalPayload > context.nInputSize) return false;
    }

    context.nDirectorySize = nPosition;
    if (!fptRangeWithin(context.nInputSize, 0, context.nDirectorySize)) {
        return false;
    }

    // Pass 2: walk the payload chain and confirm the marker sits in front of
    // every member.  This is the real gate - a text file that happens to
    // open with a version line and a number will not survive it.
    qint64 nOffset = context.nDirectorySize;
    for (qint32 i = 0; i < listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!fptRangeWithin(context.nInputSize, nOffset, FPT_MARKER_SIZE)) {
            return false;
        }
        const QByteArray baMarker =
            read_array_process(nOffset, FPT_MARKER_SIZE, pPdStruct);
        if (baMarker.size() != FPT_MARKER_SIZE) {
            return false;
        }
        if (baMarker != QByteArray::fromRawData(FPT_MARKER, FPT_MARKER_SIZE)) {
            return false;
        }

        MEMBER &member = listMembers[i];
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nOffset + FPT_MARKER_SIZE;
        if (!fptRangeWithin(context.nInputSize, member.nDataOffset,
                            member.nSize)) {
            return false;
        }
        nOffset = member.nDataOffset + member.nSize;
    }

    // The chain carries no terminator; it ends by landing exactly on EOF.
    if (nOffset != context.nInputSize) return false;

    context.listMembers = listMembers;
    context.nArchiveSize = nOffset;
    *pContext = context;
    return guardedSource;
}

bool XFrontPageTheme::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XFrontPageTheme::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFrontPageTheme archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XFrontPageTheme::createInstance(QIODevice *pDevice, bool bIsImage,
                                         XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFrontPageTheme(pDevice);
}

QList<QString> XFrontPageTheme::getSearchSignatures()
{
    // There is no binary magic at offset 0, so the marker is the only stable
    // byte string in the format.  It is unanchored on purpose - the scanner
    // uses it as a prefilter and isValid() does the real work.
    return {QStringLiteral("'<==MS-Theme==>'")};
}

XBinary::FT XFrontPageTheme::getFileType()
{
    return FT_FRONTPAGE_THEME;
}

XBinary::MODE XFrontPageTheme::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XFrontPageTheme::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XFrontPageTheme::getArch()
{
    return QString();
}

QString XFrontPageTheme::getFileFormatExt()
{
    return QStringLiteral("elm");
}

QString XFrontPageTheme::getFileFormatExtsString()
{
    return QStringLiteral("FrontPage theme package (*.elm)");
}

QString XFrontPageTheme::getMIMEString()
{
    return QStringLiteral("application/x-frontpage-theme");
}

QString XFrontPageTheme::getVersion()
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, nullptr);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult ? context.sVersion : QString();
}

qint64 XFrontPageTheme::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XFrontPageTheme::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XFrontPageTheme::getMemoryMap(MAPMODE mapMode,
                                                   PDSTRUCT *pPdStruct)
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

bool XFrontPageTheme::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XFrontPageTheme::getFileParts(quint32 nFileParts,
                                                    qint32 nLimit,
                                                    PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    const qint32 nNumberOfMembers = context.listMembers.size();
    for (qint32 i = 0; i < nNumberOfMembers; ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = FPT_MARKER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Store"));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = FPT_MARKER_SIZE + member.nSize;
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
        // cannot fire today; it is kept so the part list stays correct if
        // the acceptance rule is ever relaxed.
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

QMap<XBinary::UNPACK_PROP, QVariant> XFrontPageTheme::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XFrontPageTheme::initUnpack(UNPACK_STATE *pState,
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
    if (!parseContext(pContext, pPdStruct) ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("FrontPage theme package; stored members"));
    pState->nCurrentOffset = pContext->listMembers.first().nHeaderOffset;
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

XBinary::ARCHIVERECORD XFrontPageTheme::infoCurrent(UNPACK_STATE *pState,
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
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Store"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XFrontPageTheme::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XFrontPageTheme::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
