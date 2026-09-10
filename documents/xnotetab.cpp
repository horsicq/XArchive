/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xnotetab.h"

#include <QPointer>

#include <new>

namespace {
// Line 1 of every NoteTab document: "= V" + version digit + ' '.  The original
// tool accepts exactly two digits - 4 and 5 - through a bit test on
// (1 << (digit - '0')) & 0x30.
const char *NOTETAB_TAG = "= V";
const qint32 NOTETAB_TAG_SIZE = 5;
// The declaration line also has to carry one of these keywords; without that
// gate "= V4 " is far too weak a signature to accept.
const char *const NOTETAB_KEYWORDS[] = {"Outline", "MultiLine", "NoSorting", "TabWidth", "AutoReplace"};
const qint32 NOTETAB_KEYWORD_COUNT = 5;
// A clip is opened by this three-character tag.  Documents whose headings are
// unquoted (H=Library header) or that have no headings at all are a different,
// older NoteTab dialect: the reference tool rejects them outright, and so does
// this class rather than guessing a grammar for them.
const char *NOTETAB_HEADING_TAG = "H=\"";
const qint32 NOTETAB_HEADING_TAG_SIZE = 3;
// Hard clip on the stored heading length; the original tool truncates to 0x100
// bytes before it builds the member.
const qint32 NOTETAB_MAX_NAME = 0x100;
const qint32 NOTETAB_DECLARATION_LIMIT = 4096;
const qint32 NOTETAB_SCAN_CHUNK = 0x4000;
const qint32 NOTETAB_MAX_CLIPS = 500000;

bool notetabIsVersionDigit(char cDigit)
{
    return (cDigit == '4') || (cDigit == '5');
}
}  // namespace

XNoteTab::XNoteTab(QIODevice *pDevice) : XArchive(pDevice)
{
}

XNoteTab::~XNoteTab()
{
}

QString XNoteTab::sanitizeName(const QByteArray &baName)
{
    // The clip heading is free text: the corpus carries ':', '/', '<', '>',
    // '?', '*' and tabs inside headings ("Copy/paste", "A:link", "H? header",
    // "%-6<TAB>Braindead").  Publishing those verbatim would make the folder
    // extractor treat the member as a path - and on Windows a ':' is refused
    // outright as an alternate-data-stream attempt, which aborts the whole
    // extraction.  The reference tool folds the same set to '_' when it writes
    // the file, and doing it here reproduces its output names exactly.
    QString sResult;
    sResult.reserve(baName.size());

    for (qint32 i = 0; i < baName.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baName.at(i));
        const bool bReserved = (nCharacter < 0x20) || (nCharacter == '<') || (nCharacter == '>') || (nCharacter == ':') || (nCharacter == '"') ||
                               (nCharacter == '/') || (nCharacter == '\\') || (nCharacter == '|') || (nCharacter == '?') || (nCharacter == '*');
        sResult.append(bReserved ? QLatin1Char('_') : QLatin1Char(static_cast<char>(nCharacter)));
    }

    return sResult;
}

bool XNoteTab::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

XNoteTab::LINERESULT XNoteTab::_readLine(qint64 *pnPosition, qint64 nInputSize, qint64 *pnLineOffset, qint64 *pnLineSize, PDSTRUCT *pPdStruct)
{
    QPointer<XNoteTab> guardedThis(this);

    if (!pnPosition || !pnLineOffset || !pnLineSize) return LINERESULT_ERROR;

    const qint64 nStart = *pnPosition;

    // The reference reader publishes the cursor even when there is nothing
    // left, and the end-of-file member relies on that: the last clip's body
    // runs to this value.
    *pnLineOffset = nStart;
    *pnLineSize = 0;

    if ((nStart < 0) || (nStart >= nInputSize)) return LINERESULT_END;

    qint64 nCurrent = nStart;
    bool bTerminated = false;

    while ((nCurrent < nInputSize) && !bTerminated) {
        const qint64 nChunkSize = qMin<qint64>(NOTETAB_SCAN_CHUNK, nInputSize - nCurrent);
        const QByteArray baChunk = read_array_process(nCurrent, nChunkSize, pPdStruct);
        if (!guardedThis || (baChunk.size() != nChunkSize)) return LINERESULT_ERROR;

        for (qint32 i = 0; i < baChunk.size(); i++) {
            const char cCharacter = baChunk.at(i);

            if ((cCharacter == '\r') || (cCharacter == '\n')) {
                nCurrent += (i + 1);

                if (nCurrent < nInputSize) {
                    char cNext = 0;
                    if ((i + 1) < baChunk.size()) {
                        cNext = baChunk.at(i + 1);
                    } else {
                        const QByteArray baNext = read_array_process(nCurrent, 1, pPdStruct);
                        if (!guardedThis || (baNext.size() != 1)) return LINERESULT_ERROR;
                        cNext = baNext.at(0);
                    }

                    // CRLF and LFCR both count as one terminator; a doubled
                    // CR or LF does not.
                    if (((cCharacter == '\r') && (cNext == '\n')) || ((cCharacter == '\n') && (cNext == '\r'))) {
                        nCurrent++;
                    }
                }

                bTerminated = true;
                break;
            }
        }

        if (!bTerminated) nCurrent += nChunkSize;
        if (!isPdStructNotCanceled(pPdStruct)) return LINERESULT_ERROR;
    }

    *pnPosition = nCurrent;
    *pnLineSize = nCurrent - nStart;

    return LINERESULT_OK;
}

bool XNoteTab::_isHeadingLine(qint64 nLineOffset, qint64 nLineSize, PDSTRUCT *pPdStruct)
{
    QPointer<XNoteTab> guardedThis(this);

    if (nLineSize < NOTETAB_HEADING_TAG_SIZE) return false;

    const QByteArray baTag = read_array_process(nLineOffset, NOTETAB_HEADING_TAG_SIZE, pPdStruct);
    if (!guardedThis || (baTag.size() != NOTETAB_HEADING_TAG_SIZE)) return false;

    return (baTag == QByteArray(NOTETAB_HEADING_TAG, NOTETAB_HEADING_TAG_SIZE));
}

bool XNoteTab::checkDeclaration(QString *psDeclaration, PDSTRUCT *pPdStruct)
{
    QPointer<XNoteTab> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < NOTETAB_TAG_SIZE) return false;

    const QByteArray baTag = read_array_process(0, NOTETAB_TAG_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baTag.size() != NOTETAB_TAG_SIZE)) return false;

    if (!baTag.startsWith(QByteArray(NOTETAB_TAG, 3))) return false;
    if (!notetabIsVersionDigit(baTag.at(3))) return false;
    if (baTag.at(4) != ' ') return false;

    const qint64 nProbeSize = qMin<qint64>(NOTETAB_DECLARATION_LIMIT, nInputSize);
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baProbe.size() != nProbeSize)) return false;

    qint32 nLineEnd = baProbe.indexOf('\r');
    const qint32 nLineFeed = baProbe.indexOf('\n');
    if ((nLineEnd < 0) || ((nLineFeed >= 0) && (nLineFeed < nLineEnd))) nLineEnd = nLineFeed;
    if (nLineEnd < 0) nLineEnd = baProbe.size();

    const QByteArray baDeclaration = baProbe.left(nLineEnd);

    bool bKeyword = false;
    for (qint32 i = 0; (i < NOTETAB_KEYWORD_COUNT) && !bKeyword; i++) {
        if (baDeclaration.contains(NOTETAB_KEYWORDS[i])) bKeyword = true;
    }

    if (!bKeyword) return false;

    if (psDeclaration) *psDeclaration = QString::fromLatin1(baDeclaration);

    return true;
}

bool XNoteTab::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XNoteTab> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    CONTEXT context = {};
    context.nInputSize = 0;

    if (!checkDeclaration(&context.sDeclaration, pPdStruct) || !guardedThis || !guardedSource) return false;

    context.nInputSize = guardedSource->size();

    qint64 nPosition = 0;
    qint64 nLineOffset = 0;
    qint64 nLineSize = 0;

    // Line 1 is the declaration, line 2 is skipped unconditionally - it is
    // blank in every known document but the reader never inspects it.
    if (_readLine(&nPosition, context.nInputSize, &nLineOffset, &nLineSize, pPdStruct) != LINERESULT_OK) return false;
    if (!guardedThis || !guardedSource) return false;
    if (_readLine(&nPosition, context.nInputSize, &nLineOffset, &nLineSize, pPdStruct) != LINERESULT_OK) return false;
    if (!guardedThis || !guardedSource) return false;

    context.nHeaderSize = nPosition;

    qint64 nNameOffset = -1;
    qint32 nNameSize = 0;
    qint64 nBodyOffset = -1;

    while (isPdStructNotCanceled(pPdStruct)) {
        const LINERESULT lineResult = _readLine(&nPosition, context.nInputSize, &nLineOffset, &nLineSize, pPdStruct);
        if (!guardedThis || !guardedSource || (lineResult == LINERESULT_ERROR)) return false;
        if (lineResult == LINERESULT_END) break;

        const bool bHeading = _isHeadingLine(nLineOffset, nLineSize, pPdStruct);
        if (!guardedThis || !guardedSource) return false;

        if (bHeading) {
            // A heading closes the clip that was open; the body of that clip
            // stops at the first byte of this line.
            if ((nNameOffset >= 0) && (nBodyOffset >= 0)) {
                if (context.listClips.size() >= NOTETAB_MAX_CLIPS) return false;

                CLIP clip = {};
                clip.nNameOffset = nNameOffset;
                clip.nNameSize = nNameSize;
                clip.nBodyOffset = nBodyOffset;
                clip.nBodySize = nLineOffset - nBodyOffset;

                const QByteArray baName = (nNameSize > 0) ? read_array_process(clip.nNameOffset, nNameSize, pPdStruct) : QByteArray();
                if (!guardedThis || !guardedSource || (baName.size() != nNameSize)) return false;

                clip.baPrefix = baName;
                clip.baPrefix.append("\r\n\r\n", 4);
                clip.sFileName = sanitizeName(baName) + QStringLiteral(".txt");

                context.listClips.append(clip);
            }

            nNameOffset = -1;
            nBodyOffset = -1;
        }

        if (nNameOffset < 0) {
            // Outside a clip, only a heading line is legal.  Anything else is
            // a dialect this reader does not implement, and it fails the whole
            // document instead of skipping.
            if (!bHeading) return false;

            const qint64 nAvailable = nLineSize - NOTETAB_HEADING_TAG_SIZE;
            const qint64 nProbeSize = qMin<qint64>(nAvailable, NOTETAB_MAX_NAME + 1);

            nNameOffset = nLineOffset + NOTETAB_HEADING_TAG_SIZE;

            const QByteArray baProbe = (nProbeSize > 0) ? read_array_process(nNameOffset, nProbeSize, pPdStruct) : QByteArray();
            if (!guardedThis || !guardedSource || (baProbe.size() != nProbeSize)) return false;

            const qint32 nQuote = baProbe.indexOf('"');
            if (nQuote >= 0) {
                nNameSize = nQuote;
            } else {
                // No closing quote inside the probe window: the real heading
                // runs to the end of the line (terminator included, exactly as
                // the reference reader counts it) and is then truncated.
                nNameSize = static_cast<qint32>(qMin<qint64>(nAvailable, NOTETAB_MAX_NAME));
            }

            if (nNameSize > NOTETAB_MAX_NAME) nNameSize = NOTETAB_MAX_NAME;
            if (nNameSize < 0) nNameSize = 0;
        } else if (nBodyOffset < 0) {
            nBodyOffset = nLineOffset;
        }
    }

    if (!isPdStructNotCanceled(pPdStruct)) return false;

    // The document ends without a closing heading, so the final clip's body
    // runs to end of file.
    if ((nNameOffset >= 0) && (nBodyOffset >= 0)) {
        if (context.listClips.size() >= NOTETAB_MAX_CLIPS) return false;

        CLIP clip = {};
        clip.nNameOffset = nNameOffset;
        clip.nNameSize = nNameSize;
        clip.nBodyOffset = nBodyOffset;
        clip.nBodySize = context.nInputSize - nBodyOffset;

        const QByteArray baName = (nNameSize > 0) ? read_array_process(clip.nNameOffset, nNameSize, pPdStruct) : QByteArray();
        if (!guardedThis || !guardedSource || (baName.size() != nNameSize)) return false;

        clip.baPrefix = baName;
        clip.baPrefix.append("\r\n\r\n", 4);
        clip.sFileName = sanitizeName(baName) + QStringLiteral(".txt");

        context.listClips.append(clip);
    }

    if (context.listClips.isEmpty()) return false;

    *pContext = context;

    return guardedThis && guardedSource;
}

bool XNoteTab::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XNoteTab> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    bool bResult = false;

    QString sDeclaration;
    if (checkDeclaration(&sDeclaration, pPdStruct) && guardedThis && guardedSource) {
        const qint64 nInputSize = guardedSource->size();

        qint64 nPosition = 0;
        qint64 nLineOffset = 0;
        qint64 nLineSize = 0;

        // Bounded structural probe: the declaration line, the reserved line,
        // then a quoted heading with at least one body line after it.  That is
        // the smallest shape that yields one member, and it is what separates
        // this dialect from the unquoted-heading and no-heading documents the
        // reference tool also refuses.
        if ((_readLine(&nPosition, nInputSize, &nLineOffset, &nLineSize, pPdStruct) == LINERESULT_OK) && guardedThis && guardedSource &&
            (_readLine(&nPosition, nInputSize, &nLineOffset, &nLineSize, pPdStruct) == LINERESULT_OK) && guardedThis && guardedSource &&
            (_readLine(&nPosition, nInputSize, &nLineOffset, &nLineSize, pPdStruct) == LINERESULT_OK) && guardedThis && guardedSource) {
            if (_isHeadingLine(nLineOffset, nLineSize, pPdStruct) && guardedThis && guardedSource) {
                bResult = (_readLine(&nPosition, nInputSize, &nLineOffset, &nLineSize, pPdStruct) == LINERESULT_OK) && guardedThis && guardedSource;
            }
        }
    }

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XNoteTab::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNoteTab archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XNoteTab::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XNoteTab(pDevice);
}

QList<QString> XNoteTab::getSearchSignatures()
{
    return {QStringLiteral("'= V4 '"), QStringLiteral("'= V5 '")};
}

XBinary::FT XNoteTab::getFileType()
{
    return FT_NOTETAB;
}

XBinary::MODE XNoteTab::getMode()
{
    return MODE_DATA;
}

qint32 XNoteTab::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XNoteTab::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XNoteTab::getArch()
{
    return QString();
}

QString XNoteTab::getFileFormatExt()
{
    return QStringLiteral("clb");
}

QString XNoteTab::getFileFormatExtsString()
{
    return QStringLiteral("NoteTab document (*.clb *.otl *.clh)");
}

QString XNoteTab::getMIMEString()
{
    return QStringLiteral("application/x-notetab");
}

QString XNoteTab::getVersion()
{
    QString sDeclaration;

    if (!checkDeclaration(&sDeclaration, nullptr)) return QString();
    if (sDeclaration.size() < 4) return QString();

    return QStringLiteral("V") + sDeclaration.mid(3, 1);
}

bool XNoteTab::isSigned()
{
    return false;
}

bool XNoteTab::isEncrypted()
{
    return false;
}

XBinary::OSNAME XNoteTab::getOsName()
{
    return OSNAME_WINDOWS;
}

qint64 XNoteTab::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XNoteTab::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XNoteTab::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART> XNoteTab::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    const qint32 nNumberOfClips = context.listClips.size();

    for (qint32 i = 0; (i < nNumberOfClips) && isPdStructNotCanceled(pPdStruct); i++) {
        const CLIP &clip = context.listClips.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = clip.nBodyOffset;
            part.nFileSize = clip.nBodySize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = clip.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, clip.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, clip.nBodySize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, clip.nBodySize + clip.baPrefix.size());
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NOTETAB);
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, clip.baPrefix);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
            part.mapProperties.insert(FPART_PROP_ISFOLDER, false);
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = clip.nBodyOffset;
            part.nFileSize = clip.nBodySize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = clip.sFileName;
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XNoteTab::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XNoteTab::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XNoteTab> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listClips.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("NoteTab document; every clip is stored"));
    pState->nCurrentOffset = pContext->listClips.first().nBodyOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listClips.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XNoteTab::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listClips.size())) return ARCHIVERECORD();

    const CLIP &clip = pContext->listClips.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != clip.nBodyOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = clip.nBodyOffset;
    result.nStreamSize = clip.nBodySize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, clip.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, clip.nBodySize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, clip.nBodySize + clip.baPrefix.size());
    // The extracted clip is the heading, a blank line, then the stored body.
    // Heading and body are not contiguous in the file (the closing quote and
    // the heading line's own terminator sit between them), so the heading
    // travels as the compression property and the handler concatenates.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NOTETAB);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, clip.baPrefix);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XNoteTab::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listClips.size())) return false;

    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listClips.at(pState->nCurrentIndex).nBodyOffset;
        return true;
    }

    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XNoteTab::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
