/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xborlandpack.h"

#include <QPointer>

#include <new>

namespace {
// "This is a packed file." + 0x1A.  The DOS EOF is part of the magic: it is
// what makes TYPE stop after the notice, and it is why the archive cannot be
// confused with SEA/PKWARE ARC, whose gate requires 0x1A at offset 0.
const char BORLANDPACK_MAGIC[] = "This is a packed file.\x1a";
const qint64 BORLANDPACK_MAGIC_SIZE = 23;
// magic(23) + ":CM " + 2 digit method + ' ' + 3 digit version + ':' + CRLF
const qint64 BORLANDPACK_PREAMBLE_SIZE = 36;
// origSize(11) ' ' cksum(4) ' ' dosDate(4) ' ' dosTime(4)
const qint64 BORLANDPACK_TAIL_SIZE = 26;
const qint64 BORLANDPACK_ORIGSIZE_FIELD_SIZE = 11;
const qint64 BORLANDPACK_PACKSIZE_FIELD_SIZE = 10;
const qint64 BORLANDPACK_HEX_FIELD_SIZE = 4;
// The second header line is a fixed-width size field plus CRLF, always.
const qint64 BORLANDPACK_SIZELINE_SIZE = BORLANDPACK_PACKSIZE_FIELD_SIZE + 2;
const qint64 BORLANDPACK_MAX_NAME_SIZE = 255;
// '!' + one name byte + tail + CRLF + size line.  The payload is deliberately
// not counted: a zero-length payload is not attested but is not structurally
// forbidden either, and rejecting the whole archive for it would be a guess.
const qint64 BORLANDPACK_MIN_MEMBER_SIZE =
    1 + 1 + BORLANDPACK_TAIL_SIZE + 2 + BORLANDPACK_SIZELINE_SIZE;
// One bounded read per member covers both header lines at their maximum size.
const qint64 BORLANDPACK_HEADER_WINDOW =
    1 + BORLANDPACK_MAX_NAME_SIZE + BORLANDPACK_TAIL_SIZE + 2 +
    BORLANDPACK_SIZELINE_SIZE;
const qint32 BORLANDPACK_MAX_MEMBERS = 100000;
// The declared original size is an 11-digit text field, so it can name a
// member far larger than any buffer the decoder can hold.  Fail the parse
// instead of letting a crafted header drive an allocation.
const qint64 BORLANDPACK_MAX_UNCOMPRESSED = 0x7fffffff;
const quint32 BORLANDPACK_METHOD_LZW = 1;

bool borlandRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool borlandIsDigit(char cCharacter)
{
    return (cCharacter >= '0') && (cCharacter <= '9');
}

// Turbo Pascal write(x:width) output: right-justified decimal, blank padded.
bool borlandParseSizeField(const char *pField, qint64 nFieldSize, qint64 nMax,
                           qint64 *pnValue)
{
    if (!pField || !pnValue || (nFieldSize <= 0)) return false;
    qint64 i = 0;
    while ((i < nFieldSize) && (pField[i] == ' ')) ++i;
    if (i == nFieldSize) return false;  // Blank field: not a number.
    qint64 nValue = 0;
    for (; i < nFieldSize; ++i) {
        if (!borlandIsDigit(pField[i])) return false;
        nValue = (nValue * 10) + (pField[i] - '0');
        if (nValue > nMax) return false;
    }
    *pnValue = nValue;
    return true;
}

// The three tail fields are uppercase hex only; lower case never occurs and
// accepting it would widen the detection gate for no gain.
bool borlandParseHexField(const char *pField, quint16 *pnValue)
{
    if (!pField || !pnValue) return false;
    quint32 nValue = 0;
    for (qint64 i = 0; i < BORLANDPACK_HEX_FIELD_SIZE; ++i) {
        const char cCharacter = pField[i];
        quint32 nDigit = 0;
        if (borlandIsDigit(cCharacter)) {
            nDigit = static_cast<quint32>(cCharacter - '0');
        } else if ((cCharacter >= 'A') && (cCharacter <= 'F')) {
            nDigit = static_cast<quint32>(cCharacter - 'A') + 10;
        } else {
            return false;
        }
        nValue = (nValue << 4) | nDigit;
    }
    *pnValue = static_cast<quint16>(nValue);
    return true;
}

// The writer blank-pads short names out to the field it uses; only the
// trailing padding is decoration, everything else must be a real name.
QByteArray borlandTrimTrailingSpaces(const QByteArray &baName)
{
    qint32 nEnd = baName.size();
    while ((nEnd > 0) && (baName.at(nEnd - 1) == ' ')) --nEnd;
    return baName.left(nEnd);
}

bool borlandIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty() || (baName.size() > BORLANDPACK_MAX_NAME_SIZE)) {
        return false;
    }
    if ((baName == QByteArray(".")) || (baName == QByteArray(".."))) {
        return false;
    }
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return false;
        // The container stores a bare DOS 8.3 name; a separator here would be
        // a path smuggled through a format that has no directory concept.
        if ((c == '/') || (c == '\\') || (c == ':')) return false;
    }
    return true;
}

qint64 borlandFindCRLF(const QByteArray &baWindow, qint64 nFrom)
{
    for (qint64 i = nFrom; i + 1 < baWindow.size(); ++i) {
        if ((baWindow.at(static_cast<qint32>(i)) == '\r') &&
            (baWindow.at(static_cast<qint32>(i + 1)) == '\n')) {
            return i;
        }
    }
    return -1;
}
}  // namespace

XBorlandPack::XBorlandPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBorlandPack::~XBorlandPack()
{
}

bool XBorlandPack::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XBorlandPack> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <
        BORLANDPACK_PREAMBLE_SIZE + BORLANDPACK_MIN_MEMBER_SIZE) {
        return false;
    }

    const QByteArray baPreamble =
        read_array_process(0, BORLANDPACK_PREAMBLE_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baPreamble.size() != BORLANDPACK_PREAMBLE_SIZE)) {
        return false;
    }
    const char *pPreamble = baPreamble.constData();
    if (memcmp(pPreamble, BORLANDPACK_MAGIC,
               static_cast<size_t>(BORLANDPACK_MAGIC_SIZE)) != 0) {
        return false;
    }
    // ":CM " <2 digit method> ' ' <3 digit version> ':' CRLF - the method is
    // archive-wide, there is no per-member method field anywhere.
    if ((memcmp(pPreamble + 23, ":CM ", 4) != 0) ||
        !borlandIsDigit(pPreamble[27]) || !borlandIsDigit(pPreamble[28]) ||
        (pPreamble[29] != ' ') || !borlandIsDigit(pPreamble[30]) ||
        !borlandIsDigit(pPreamble[31]) || !borlandIsDigit(pPreamble[32]) ||
        (pPreamble[33] != ':') || (pPreamble[34] != '\r') ||
        (pPreamble[35] != '\n')) {
        return false;
    }
    context.nMethod =
        static_cast<quint32>((pPreamble[27] - '0') * 10 + (pPreamble[28] - '0'));
    context.nVersion = static_cast<quint32>((pPreamble[30] - '0') * 100 +
                                            (pPreamble[31] - '0') * 10 +
                                            (pPreamble[32] - '0'));

    qint64 nOffset = BORLANDPACK_PREAMBLE_SIZE;
    while ((nOffset < context.nInputSize) &&
           (context.listMembers.size() < BORLANDPACK_MAX_MEMBERS) &&
           isPdStructNotCanceled(pPdStruct)) {
        const qint64 nWindowSize =
            qMin(BORLANDPACK_HEADER_WINDOW, context.nInputSize - nOffset);
        if (nWindowSize < BORLANDPACK_MIN_MEMBER_SIZE) return false;
        const QByteArray baWindow =
            read_array_process(nOffset, nWindowSize, pPdStruct);
        if (!guardedThis || !guardedSource ||
            (baWindow.size() != nWindowSize)) {
            return false;
        }
        if (baWindow.at(0) != '!') return false;

        const qint64 nLineEnd = borlandFindCRLF(baWindow, 1);
        // '!' + at least one name byte + the fixed 26-byte tail.
        if ((nLineEnd < 1 + 1 + BORLANDPACK_TAIL_SIZE) ||
            (nLineEnd + 2 + BORLANDPACK_SIZELINE_SIZE > nWindowSize)) {
            return false;
        }

        // The name is variable width; the tail is what is fixed.  Parsing from
        // the left with an assumed name width breaks on every name that is not
        // exactly as long as the first one.
        const char *pTail =
            baWindow.constData() + (nLineEnd - BORLANDPACK_TAIL_SIZE);
        if ((pTail[11] != ' ') || (pTail[16] != ' ') || (pTail[21] != ' ')) {
            return false;
        }
        MEMBER member = {};
        qint64 nUncompressedSize = 0;
        if (!borlandParseSizeField(pTail, BORLANDPACK_ORIGSIZE_FIELD_SIZE,
                                   BORLANDPACK_MAX_UNCOMPRESSED,
                                   &nUncompressedSize) ||
            !borlandParseHexField(pTail + 12, &member.nChecksum) ||
            !borlandParseHexField(pTail + 17, &member.nDosDate) ||
            !borlandParseHexField(pTail + 22, &member.nDosTime)) {
            return false;
        }

        const QByteArray baName =
            baWindow.mid(1,
                         static_cast<qint32>(nLineEnd -
                                             BORLANDPACK_TAIL_SIZE - 1));
        const QByteArray baTrimmedName = borlandTrimTrailingSpaces(baName);
        if (!borlandIsValidName(baTrimmedName)) return false;

        const char *pSizeLine = baWindow.constData() + nLineEnd + 2;
        if ((pSizeLine[BORLANDPACK_PACKSIZE_FIELD_SIZE] != '\r') ||
            (pSizeLine[BORLANDPACK_PACKSIZE_FIELD_SIZE + 1] != '\n')) {
            return false;
        }
        const qint64 nHeaderSize =
            nLineEnd + 2 + BORLANDPACK_SIZELINE_SIZE;
        const qint64 nDataOffset = nOffset + nHeaderSize;
        qint64 nCompressedSize = 0;
        if (!borlandParseSizeField(pSizeLine, BORLANDPACK_PACKSIZE_FIELD_SIZE,
                                   context.nInputSize - nDataOffset,
                                   &nCompressedSize) ||
            !borlandRangeWithin(context.nInputSize, nDataOffset,
                                nCompressedSize)) {
            return false;
        }

        member.nHeaderOffset = nOffset;
        member.nHeaderSize = nHeaderSize;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.sFileName = QString::fromLatin1(baTrimmedName);
        context.listMembers.append(member);

        nOffset = nDataOffset + nCompressedSize;
    }

    // There is no trailer, no central directory and no member count: the only
    // end-of-archive signal is landing exactly on EOF.  A leftover byte or an
    // overrunning payload means truncation, and emitting the members parsed so
    // far would advertise a partial listing as a complete one.
    if ((nOffset != context.nInputSize) || context.listMembers.isEmpty() ||
        !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    context.nArchiveSize = nOffset;
    context.nFirstMemberOffset = context.listMembers.first().nHeaderOffset;
    *pContext = context;
    return true;
}

bool XBorlandPack::isValid(PDSTRUCT *pPdStruct)
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

bool XBorlandPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBorlandPack archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBorlandPack::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBorlandPack(pDevice);
}

QList<QString> XBorlandPack::getSearchSignatures()
{
    return {QStringLiteral("'This is a packed file.'1A")};
}

XBinary::FT XBorlandPack::getFileType()
{
    return FT_BORLAND_PACK;
}

XBinary::MODE XBorlandPack::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBorlandPack::getEndian()
{
    // Every field in the container is ASCII text, so endianness is nominal.
    return ENDIAN_LITTLE;
}

QString XBorlandPack::getArch()
{
    return QString();
}

QString XBorlandPack::getFileFormatExt()
{
    return QStringLiteral("arc");
}

QString XBorlandPack::getFileFormatExtsString()
{
    return QStringLiteral("Borland PACK (*.arc)");
}

QString XBorlandPack::getMIMEString()
{
    return QStringLiteral("application/x-borland-pack");
}

QString XBorlandPack::versionToString(quint32 nVersion)
{
    return QStringLiteral("%1.%2")
        .arg(nVersion / 100)
        .arg(nVersion % 100, 2, 10, QLatin1Char('0'));
}

QString XBorlandPack::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return versionToString(context.nVersion);
}

qint64 XBorlandPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBorlandPack::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBorlandPack::getMemoryMap(MAPMODE mapMode,
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

QString XBorlandPack::methodToString(quint32 nMethod)
{
    if (nMethod == BORLANDPACK_METHOD_LZW) {
        return QStringLiteral("CM 01 LZW");
    }
    return QStringLiteral("CM %1").arg(nMethod, 2, 10, QLatin1Char('0'));
}

XBinary::HANDLE_METHOD XBorlandPack::methodToHandleMethod(quint32 nMethod)
{
    // Only method 01 is attested, and it is a Unix-compress LZW stream whose
    // 1F 9D magic the container does not store.  Any other token is reported
    // rather than guessed: there is no per-member method field to fall back on.
    if (nMethod == BORLANDPACK_METHOD_LZW) return HANDLE_METHOD_COMPRESS_RAW;
    return HANDLE_METHOD_UNKNOWN;
}

bool XBorlandPack::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBorlandPack::getFileParts(quint32 nFileParts,
                                                 qint32 nLimit,
                                                 PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = BORLANDPACK_PREAMBLE_SIZE;
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
                                      methodToHandleMethod(context.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(context.nMethod));
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                      member.sFileName);
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

    // The walk only succeeds when the last payload ends exactly at EOF, so
    // there is never an overlay to describe.
    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBorlandPack::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBorlandPack::initUnpack(UNPACK_STATE *pState,
                              const QMap<UNPACK_PROP, QVariant> &mapProperties,
                              PDSTRUCT *pPdStruct)
{
    QPointer<XBorlandPack> guardedThis(this);
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
        tr("Borland PACK %1; %2")
            .arg(versionToString(pContext->nVersion))
            .arg(methodToString(pContext->nMethod)));
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

XBinary::ARCHIVERECORD XBorlandPack::infoCurrent(UNPACK_STATE *pState,
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                methodToHandleMethod(pContext->nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(pContext->nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The 4-hex-digit field is carried for information only.  An exhaustive
    // CRC-16 sweep failed to identify it, so it must never gate extraction.
    result.mapProperties.insert(
        FPART_PROP_INFO,
        tr("Checksum %1 (algorithm unidentified, not verified)")
            .arg(member.nChecksum, 4, 16, QLatin1Char('0')).toUpper());
    if (isValidDosDateTime(member.nDosDate, member.nDosTime)) {
        const QDateTime dtModified =
            dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dtModified);
        }
    }
    return result;
}

bool XBorlandPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XBorlandPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
