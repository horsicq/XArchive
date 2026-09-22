/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbluebytelib.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 BBLIB_DIRECTORY_POINTER_SIZE = 4;
const qint64 BBLIB_PREHEADER_OFFSET = 4;
const qint64 BBLIB_PREHEADER_SIZE = 8;
const qint64 BBLIB_DATA_OFFSET_PLAIN = 4;
const qint64 BBLIB_DATA_OFFSET_TCT = 12;
const qint64 BBLIB_ENTRY_SIZE = 12;
const qint64 BBLIB_NAME_SIZE = 8;
// The TCT pre-header stores the entry count in a 16-bit field, so a directory
// larger than that could never have been written by the producer.  It also
// caps the single directory read at ~768 KiB.
const qint32 BBLIB_MAX_ENTRIES = 65535;
const quint16 BBLIB_PREHEADER_TRAILER = 0x0049U;  // "I\0"

bool bblibRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// A member name is 8 raw bytes.  The only structural rule the producer obeys
// is that padding, once started, runs to the end of the field: a NUL followed
// by a non-NUL never occurs in the 34788 members of the reference corpus, and
// rejecting it is what keeps random binary tails from parsing as a directory.
// Deliberately NOT a printable-ASCII test: three legitimate archives
// (CHAR009/012/016.LIB) carry glyph-index names built from 0x09/0x0B/0x0F and
// high bytes, and would be lost.
bool bblibIsValidRawName(const uchar *pName)
{
    if (pName[0] == 0) return false;
    bool bPadding = false;
    for (qint64 i = 0; i < BBLIB_NAME_SIZE; i++) {
        if (pName[i] == 0) {
            bPadding = true;
        } else if (bPadding) {
            return false;
        }
    }
    return true;
}
}  // namespace

XBlueByteLib::XBlueByteLib(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBlueByteLib::~XBlueByteLib()
{
}

// Names are not path-safe and, in the glyph-name archives, not even printable.
// All 34788 names are unique as RAW BYTES, but a naive "non-alnum -> _"
// sanitizer downstream collapses 120 distinct names to 73 and silently
// overwrites 47 members.  Escaping every unsafe byte as %XX is reversible,
// survives a later sanitizer (it only rewrites '%', which keeps the hex digits
// distinct) and was measured to keep all names unique in all 562 archives.
QString XBlueByteLib::rawNameToString(const QByteArray &baRawName,
                                      qint32 nIndex)
{
    // Padding is NUL or space depending on the producer generation; 361 names
    // are space-padded and a NUL-only trim would leave trailing blanks on them.
    qint32 nLength = baRawName.size();
    while ((nLength > 0) && (baRawName.at(nLength - 1) == '\0')) nLength--;
    while ((nLength > 0) && (baRawName.at(nLength - 1) == ' ')) nLength--;

    QString sResult;
    for (qint32 i = 0; i < nLength; i++) {
        const quint8 nCharacter = static_cast<quint8>(baRawName.at(i));
        const bool bSafe =
            (nCharacter > 0x20) && (nCharacter < 0x7f) &&
            (nCharacter != '%') && (nCharacter != '/') &&
            (nCharacter != '\\') && (nCharacter != ':') &&
            (nCharacter != '*') && (nCharacter != '?') &&
            (nCharacter != '"') && (nCharacter != '<') &&
            (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
        } else {
            // Built by hand rather than with arg(): in a format string "%%" is
            // not an escape for QString::arg, so the obvious one-liner would
            // emit a literal double percent.
            QString sHex = QString::number(nCharacter, 16).toUpper();
            if (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("record%1").arg(nIndex);
    }
    return sResult;
}

bool XBlueByteLib::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // 4 pointer bytes + at least one payload byte + one 12-byte entry.
    if (context.nInputSize <
        BBLIB_DIRECTORY_POINTER_SIZE + 1 + BBLIB_ENTRY_SIZE) {
        return false;
    }

    const QByteArray baPreHeader = read_array_process(
        0, BBLIB_PREHEADER_OFFSET + BBLIB_PREHEADER_SIZE, pPdStruct);
    if (!guardedSource ||
        baPreHeader.size() != BBLIB_PREHEADER_OFFSET + BBLIB_PREHEADER_SIZE) {
        return false;
    }
    const uchar *pPreHeader =
        reinterpret_cast<const uchar *>(baPreHeader.constData());

    context.nDirectoryOffset =
        static_cast<qint64>(qFromLittleEndian<quint32>(pPreHeader));
    if ((context.nDirectoryOffset < BBLIB_DATA_OFFSET_PLAIN) ||
        (context.nDirectoryOffset > context.nInputSize - BBLIB_ENTRY_SIZE)) {
        return false;
    }
    const qint64 nDirectorySize =
        context.nInputSize - context.nDirectoryOffset;
    // The directory is the file's tail: it must tile it exactly, which is what
    // rules out an appended overlay as well as a coincidental pointer value.
    if ((nDirectorySize % BBLIB_ENTRY_SIZE) != 0) return false;
    const qint64 nEntryCount = nDirectorySize / BBLIB_ENTRY_SIZE;
    if ((nEntryCount < 1) || (nEntryCount > BBLIB_MAX_ENTRIES)) return false;
    context.nNumberOfEntries = static_cast<qint32>(nEntryCount);

    const QByteArray baDirectory = read_array_process(
        context.nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedSource ||
        baDirectory.size() != nDirectorySize) {
        return false;
    }
    const uchar *pDirectory =
        reinterpret_cast<const uchar *>(baDirectory.constData());

    // The first entry's offset is what tells the two sub-variants apart: 4 for
    // the plain shape and 12 when the 8-byte "TCT " block sits in between.
    context.nDataOffset = static_cast<qint64>(
        qFromLittleEndian<quint32>(pDirectory + BBLIB_NAME_SIZE));
    if ((context.nDataOffset != BBLIB_DATA_OFFSET_PLAIN) &&
        (context.nDataOffset != BBLIB_DATA_OFFSET_TCT)) {
        return false;
    }
    if (context.nDataOffset == BBLIB_DATA_OFFSET_TCT) {
        context.bHasPreHeader = true;
        // Three independent anchors, all invariant over the 284 TCT archives:
        // the tag, the literal "I\0" trailer, and a count that must agree with
        // the directory the pointer at offset 0 led us to.
        if (baPreHeader.mid(static_cast<int>(BBLIB_PREHEADER_OFFSET), 4) !=
                QByteArrayLiteral("TCT ") ||
            (qFromLittleEndian<quint16>(pPreHeader + 10) !=
             BBLIB_PREHEADER_TRAILER) ||
            (static_cast<qint32>(qFromLittleEndian<quint16>(pPreHeader + 8)) !=
             context.nNumberOfEntries)) {
            return false;
        }
    }

    qint64 nPreviousOffset = 0;
    for (qint32 i = 0; i < context.nNumberOfEntries; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pEntry = pDirectory + (BBLIB_ENTRY_SIZE * i);
        if (!bblibIsValidRawName(pEntry)) return false;
        const qint64 nOffset = static_cast<qint64>(
            qFromLittleEndian<quint32>(pEntry + BBLIB_NAME_SIZE));
        // Member sizes are implicit (next offset, or the directory offset for
        // the last one), so strictly ascending offsets are what guarantees
        // every member has a positive, non-overlapping extent.
        if (nOffset <= nPreviousOffset) return false;
        nPreviousOffset = nOffset;

        MEMBER member = {};
        member.nDataOffset = nOffset;
        member.baRawName = QByteArray(
            reinterpret_cast<const char *>(pEntry),
            static_cast<int>(BBLIB_NAME_SIZE));
        member.sFileName = rawNameToString(member.baRawName, i);
        context.listMembers.append(member);
    }
    // The last member must still end before the directory begins.
    if (nPreviousOffset >= context.nDirectoryOffset) return false;

    for (qint32 i = 0; i < context.nNumberOfEntries; i++) {
        const qint64 nEnd = (i + 1 < context.nNumberOfEntries)
                                ? context.listMembers.at(i + 1).nDataOffset
                                : context.nDirectoryOffset;
        MEMBER &member = context.listMembers[i];
        member.nSize = nEnd - member.nDataOffset;
        if ((member.nSize <= 0) ||
            !bblibRangeWithin(context.nInputSize, member.nDataOffset,
                              member.nSize)) {
            return false;
        }
    }

    // Header, payloads and directory tile the file exactly; there is never a
    // trailer, so the archive size is always the whole input.
    context.nArchiveSize = context.nInputSize;
    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XBlueByteLib::isValid(PDSTRUCT *pPdStruct)
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

bool XBlueByteLib::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBlueByteLib archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBlueByteLib::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBlueByteLib(pDevice);
}

QList<QString> XBlueByteLib::getSearchSignatures()
{
    // Intentionally empty.  There is no magic anywhere in the container: byte 0
    // is the low byte of a variable directory offset (450 distinct values over
    // the 562-file reference corpus) and the "TCT " tag is present in only half
    // the family.  Detection is the structural predicate in parseContext().
    return QList<QString>();
}

XBinary::FT XBlueByteLib::getFileType()
{
    return FT_BLUEBYTE_LIB;
}

XBinary::MODE XBlueByteLib::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBlueByteLib::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBlueByteLib::getArch()
{
    return QString();
}

QString XBlueByteLib::getFileFormatExt()
{
    return QStringLiteral("lib");
}

QString XBlueByteLib::getFileFormatExtsString()
{
    return QStringLiteral(
        "Blue Byte library (*.lib *.dat *.afx *.sfx *.cga *.ega *.tdy *.pmp)");
}

QString XBlueByteLib::getMIMEString()
{
    return QStringLiteral("application/x-bluebyte-lib");
}

QString XBlueByteLib::getVersion()
{
    return QString();
}

qint64 XBlueByteLib::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBlueByteLib::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBlueByteLib::getMemoryMap(MAPMODE mapMode,
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

bool XBlueByteLib::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XBlueByteLib::getFileParts(quint32 nFileParts,
                                                 qint32 nLimit,
                                                 PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if (nFileParts & FILEPART_HEADER) {
        if (canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = 0;
            part.nFileSize = BBLIB_DIRECTORY_POINTER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Directory pointer");
            result.append(part);
        }
        // The 4 or 12 bytes ahead of the first member are header, never a
        // member: emitting a phantom record for them would break byte equality
        // with the reference extractor.
        if (context.bHasPreHeader && canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = BBLIB_PREHEADER_OFFSET;
            part.nFileSize = BBLIB_PREHEADER_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Header");
            result.append(part);
        }
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nInputSize - context.nDirectoryOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        result.append(part);
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
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBlueByteLib::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBlueByteLib::initUnpack(UNPACK_STATE *pState,
                              const QMap<UNPACK_PROP, QVariant> &mapProperties,
                              PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource ||
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        pContext->bHasPreHeader
            ? tr("Blue Byte library (TCT)")
            : tr("Blue Byte library"));
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XBlueByteLib::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    // Every member is a verbatim byte slice.  The TPWM/ILBM/BBHDF streams that
    // many members begin with are inner payload formats and must stay packed
    // here; decoding them belongs to a nested detection pass, not to the
    // container, or the extracted bytes stop matching the reference.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XBlueByteLib::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nDirectoryOffset;
    return false;
}

bool XBlueByteLib::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
