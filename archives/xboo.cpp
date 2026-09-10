/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * The transform implemented here was read out of DEBOO.EXE, the format's own
 * reference decoder (which ships BOO-encoded in the same corpus): a name line,
 * then 6-bit characters packed four-to-three big-endian, with '~' escaping a
 * run of NUL bytes.
 */

#include "xboo.h"

#include <QPointer>

#include <limits>
#include <memory>
#include <new>

namespace {

// The container has no magic, so every constant below is a shape constraint
// rather than a field offset.
const qint64 BOO_MIN_FILE_SIZE = 96;
const qint64 BOO_MIN_BODY_SIZE = 80;
const qint64 BOO_MAX_NAME_LINE = 32;
const qint32 BOO_MAX_NAME_SIZE = 16;
const qint32 BOO_MAX_NAME_STEM = 12;
const qint32 BOO_MAX_NAME_EXT = 3;

// The gate walks a bounded window only.  It runs on every candidate the
// archive probe chain reaches, so its cost must not scale with the file.
const qint64 BOO_PROBE_WINDOW = 4096;
const qint32 BOO_MIN_PROBE_LINES = 2;
const qint64 BOO_MIN_PROBE_CHARS = 128;
const qint64 BOO_MIN_PROBE_LINE_WIDTH = 32;

const qint64 BOO_MAX_ENCODED_SIZE = Q_INT64_C(64) * 1024 * 1024;
// A '~' escape pair emits up to 78 NULs, so a crafted file expands by ~39x.
// The decode is an in-memory allocation, hence the hard ceiling on top of
// whatever the caller's output policy allows.
const qint64 BOO_MAX_DECODED_SIZE = Q_INT64_C(256) * 1024 * 1024;

const quint8 BOO_LF = 0x0aU;
const quint8 BOO_CR = 0x0dU;
const quint8 BOO_ESCAPE = 0x7eU;
const quint8 BOO_ALPHABET_FIRST = 0x30U;
const quint8 BOO_ALPHABET_LAST = 0x6fU;
// DEBOO computes the run length as (count byte - 0x30) and accepts the escape
// character itself in that slot, which is where the common "~~" pairs (78
// NULs) come from.  Anything above that is out of the printable range.
const quint8 BOO_MAX_ESCAPE_COUNT = 0x4eU;

// MAKEBOO pads the last 4-character group with NULs because the stream has no
// length field, so at most two bytes of the decoded tail are padding.  Never
// strip a whole trailing NUL run: real payloads here end in 4, 6 and several
// hundred genuine NULs.
const qint32 BOO_MAX_TRAILING_PAD = 2;

bool booIsPayloadCharacter(quint8 nCharacter)
{
    return (nCharacter >= BOO_ALPHABET_FIRST) &&
           (nCharacter <= BOO_ALPHABET_LAST);
}

bool booIsNameCharacter(quint8 nCharacter, bool bStem)
{
    if ((nCharacter >= 'A') && (nCharacter <= 'Z')) return true;
    if ((nCharacter >= 'a') && (nCharacter <= 'z')) return true;
    if ((nCharacter >= '0') && (nCharacter <= '9')) return true;
    switch (nCharacter) {
        case '_':
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '(':
        case ')':
        case '-':
        case '@':
        case '^':
        case '{':
        case '}':
            return true;
        case '\'':
        case '`':
        case '~':
            // Tolerated in the 8.3 stem only.  Keeping them out of the
            // extension is what stops ordinary prose (an apostrophe, a
            // backquoted word) from passing as a name line.
            return bStem;
        default:
            return false;
    }
}

// A DOS 8.3 shape with no separator, no space and no control byte.  The name
// comes from untrusted content and is handed to the extractor as the output
// file name, so path traversal has to die here, not downstream.
bool booIsValidName(const QByteArray &baName)
{
    const qint32 nSize = baName.size();
    if ((nSize < 1) || (nSize > BOO_MAX_NAME_SIZE)) return false;

    const qint32 nDotPosition = baName.indexOf('.');
    const qint32 nStemSize = (nDotPosition < 0) ? nSize : nDotPosition;
    if ((nStemSize < 1) || (nStemSize > BOO_MAX_NAME_STEM)) return false;
    for (qint32 i = 0; i < nStemSize; i++) {
        if (!booIsNameCharacter(static_cast<quint8>(baName.at(i)), true)) {
            return false;
        }
    }
    if (nDotPosition >= 0) {
        const qint32 nExtSize = nSize - nDotPosition - 1;
        if ((nExtSize < 1) || (nExtSize > BOO_MAX_NAME_EXT)) return false;
        for (qint32 i = nDotPosition + 1; i < nSize; i++) {
            // '.' is not in the character set, so a second dot - and with it
            // "..", "./" and every other traversal shape - is rejected here.
            if (!booIsNameCharacter(static_cast<quint8>(baName.at(i)),
                                    false)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

XBOO::XBOO(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBOO::~XBOO()
{
}

bool XBOO::parseNameLine(const QByteArray &baSource, QString *psName,
                         qint64 *pnBodyOffset)
{
    if (!psName || !pnBodyOffset) return false;

    const qint32 nScanLimit = static_cast<qint32>(
        qMin<qint64>(baSource.size(), BOO_MAX_NAME_LINE));
    qint32 nLineEnd = -1;
    for (qint32 i = 0; i < nScanLimit; i++) {
        if (static_cast<quint8>(baSource.at(i)) == BOO_LF) {
            nLineEnd = i;
            break;
        }
    }
    // An empty first line carries no name and is not a BOO header.
    if (nLineEnd < 1) return false;

    QByteArray baName = baSource.left(nLineEnd);
    if (baName.endsWith('\r')) baName.chop(1);
    if (!booIsValidName(baName)) return false;

    *psName = QString::fromLatin1(baName);
    *pnBodyOffset = static_cast<qint64>(nLineEnd) + 1;
    return true;
}

bool XBOO::probeGate(const QByteArray &baSource, qint64 nBodyOffset)
{
    const qint64 nBodySize =
        static_cast<qint64>(baSource.size()) - nBodyOffset;
    if ((nBodyOffset < 0) || (nBodySize < BOO_MIN_BODY_SIZE)) return false;

    const qint32 nWindowSize =
        static_cast<qint32>(qMin<qint64>(nBodySize, BOO_PROBE_WINDOW));
    const char *pBody = baSource.constData() + nBodyOffset;

    // The window can cut a line in half.  A truncated fragment is evidence of
    // nothing, so everything after the last line feed is dropped.
    qint32 nLastLineEnd = -1;
    for (qint32 i = nWindowSize - 1; i >= 0; i--) {
        if (static_cast<quint8>(pBody[i]) == BOO_LF) {
            nLastLineEnd = i;
            break;
        }
    }
    if (nLastLineEnd < 0) return false;

    qint32 nCompleteLines = 0;
    qint64 nTotalCharacters = 0;
    qint64 nMaxLineWidth = 0;
    qint32 nLineStart = 0;
    for (qint32 i = 0; i <= nLastLineEnd; i++) {
        if (static_cast<quint8>(pBody[i]) != BOO_LF) continue;

        qint32 nLineEnd = i;
        if ((nLineEnd > nLineStart) &&
            (static_cast<quint8>(pBody[nLineEnd - 1]) == BOO_CR)) {
            nLineEnd--;
        }
        const qint32 nLineSize = nLineEnd - nLineStart;
        const qint32 nCurrentStart = nLineStart;
        nLineStart = i + 1;
        if (nLineSize == 0) continue;

        qint64 nCharacters = 0;
        qint32 j = 0;
        while (j < nLineSize) {
            const quint8 nCharacter =
                static_cast<quint8>(pBody[nCurrentStart + j]);
            if (nCharacter == BOO_ESCAPE) {
                // The count byte always sits on the same line: DEBOO reads it
                // out of the same buffer, and a '~' at end of line would make
                // it decode the line feed as a negative run length.
                if ((j + 1) >= nLineSize) return false;
                const quint8 nCount =
                    static_cast<quint8>(pBody[nCurrentStart + j + 1]);
                if ((nCount < BOO_ALPHABET_FIRST) ||
                    (nCount > (BOO_ALPHABET_FIRST + BOO_MAX_ESCAPE_COUNT))) {
                    return false;
                }
                j += 2;
                continue;
            }
            if (!booIsPayloadCharacter(nCharacter)) return false;
            nCharacters++;
            j++;
        }
        // Every producer in the wild wraps on a whole-group boundary; this is
        // the single strongest discriminator against ordinary uppercase or
        // digit-heavy text, which is why the gate keeps it.
        if ((nCharacters % 4) != 0) return false;

        nCompleteLines++;
        nTotalCharacters += nCharacters;
        nMaxLineWidth = qMax<qint64>(nMaxLineWidth, nLineSize);
    }

    return (nCompleteLines >= BOO_MIN_PROBE_LINES) &&
           (nTotalCharacters >= BOO_MIN_PROBE_CHARS) &&
           (nMaxLineWidth >= BOO_MIN_PROBE_LINE_WIDTH);
}

bool XBOO::decodeBody(const QByteArray &baSource, qint64 nBodyOffset,
                      qint64 nDecodedLimit, QByteArray *pbaDecoded,
                      qint64 *pnRawDecodedSize, PDSTRUCT *pPdStruct)
{
    if (!pbaDecoded || !pnRawDecodedSize || (nBodyOffset < 0) ||
        (nDecodedLimit < 0) || (nBodyOffset > baSource.size())) {
        return false;
    }
    pbaDecoded->clear();
    *pnRawDecodedSize = 0;

    const qint32 nBodySize =
        baSource.size() - static_cast<qint32>(nBodyOffset);
    const char *pBody = baSource.constData() + nBodyOffset;

    quint8 nGroup[4] = {};
    qint32 nGroupCount = 0;
    bool bEscapePending = false;
    QByteArray baDecoded;

    for (qint32 i = 0; i < nBodySize; i++) {
        if ((i & 0xffff) == 0 && !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const quint8 nCharacter = static_cast<quint8>(pBody[i]);

        if ((nCharacter == BOO_LF) || (nCharacter == BOO_CR)) {
            // A pending escape at a line break is a desynchronised stream, not
            // a run that continues on the next line.  Bare CR is treated as a
            // terminator too, so a stray 0x0d can never reach the accumulator
            // and decode as a negative 6-bit value.
            if (bEscapePending) return false;
            continue;
        }

        if (bEscapePending) {
            if ((nCharacter < BOO_ALPHABET_FIRST) ||
                (nCharacter > (BOO_ALPHABET_FIRST + BOO_MAX_ESCAPE_COUNT))) {
                return false;
            }
            const qint32 nCount = nCharacter - BOO_ALPHABET_FIRST;
            if ((nCount > 0) &&
                ((baDecoded.size() + nCount) > nDecodedLimit)) {
                return false;
            }
            if (nCount > 0) baDecoded.append(nCount, '\0');
            bEscapePending = false;
            continue;
        }

        if (nCharacter == BOO_ESCAPE) {
            bEscapePending = true;
            continue;
        }

        if (!booIsPayloadCharacter(nCharacter)) return false;
        nGroup[nGroupCount++] = nCharacter - BOO_ALPHABET_FIRST;
        if (nGroupCount < 4) continue;
        nGroupCount = 0;

        if ((baDecoded.size() + 3) > nDecodedLimit) return false;
        baDecoded.append(
            static_cast<char>(((nGroup[0] << 2) + (nGroup[1] >> 4)) & 0xffU));
        baDecoded.append(
            static_cast<char>(((nGroup[1] << 4) + (nGroup[2] >> 2)) & 0xffU));
        baDecoded.append(
            static_cast<char>(((nGroup[2] << 6) + nGroup[3]) & 0xffU));
    }

    // A truncated escape or a partial 4-character group means the transport
    // was cut short; publishing what decoded so far would be a silently
    // truncated file.
    if (bEscapePending || (nGroupCount != 0) || baDecoded.isEmpty()) {
        return false;
    }

    *pnRawDecodedSize = baDecoded.size();
    for (qint32 i = 0; (i < BOO_MAX_TRAILING_PAD) && !baDecoded.isEmpty();
         i++) {
        if (baDecoded.at(baDecoded.size() - 1) != '\0') break;
        baDecoded.chop(1);
    }
    if (baDecoded.isEmpty()) return false;

    *pbaDecoded = baDecoded;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XBOO::readSource(QByteArray *pbaSource, PDSTRUCT *pPdStruct)
{
    if (!pbaSource || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XBOO> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nSize = getSize();
    if (!guardedThis || !guardedSource || (nSize < BOO_MIN_FILE_SIZE) ||
        (nSize > BOO_MAX_ENCODED_SIZE) ||
        (nSize > static_cast<qint64>((std::numeric_limits<int>::max)()))) {
        return false;
    }

    *pbaSource = read_array_process(0, nSize, pPdStruct);
    return guardedThis && guardedSource &&
           (static_cast<qint64>(pbaSource->size()) == nSize) &&
           isPdStructNotCanceled(pPdStruct);
}

bool XBOO::parseContext(CONTEXT *pContext, qint64 nDecodedLimit,
                        PDSTRUCT *pPdStruct)
{
    if (!pContext || (nDecodedLimit <= 0) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<XBOO> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QByteArray baSource;
    if (!readSource(&baSource, pPdStruct) || !guardedThis || !guardedSource) {
        return false;
    }

    CONTEXT context;
    context.nInputSize = baSource.size();
    if (!parseNameLine(baSource, &context.sFileName, &context.nBodyOffset)) {
        return false;
    }
    // Cheap bounded shape test first: it rejects everything that is not this
    // format after a few kilobytes, so the full decode below only ever runs on
    // a genuine candidate.
    if (!probeGate(baSource, context.nBodyOffset)) return false;
    if (!decodeBody(baSource, context.nBodyOffset, nDecodedLimit,
                    &context.baDecoded, &context.nRawDecodedSize,
                    pPdStruct)) {
        return false;
    }

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XBOO::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context;
    const bool bResult =
        parseContext(&context, BOO_MAX_DECODED_SIZE, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XBOO::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBOO archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBOO::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBOO(pDevice);
}

QList<QString> XBOO::getSearchSignatures()
{
    // Deliberately empty: the first line of a BOO file is the payload's own
    // name, so there is nothing to compare.  XBinary's scan helper returns an
    // empty result for an empty list, so this cannot degrade into a
    // match-everything scan.
    return QList<QString>();
}

XBinary::FT XBOO::getFileType()
{
    return FT_BOO;
}

XBinary::MODE XBOO::getMode()
{
    return MODE_DATA;
}

qint32 XBOO::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XBOO::getEndian()
{
    // The 4-to-3 packing fills the output most significant bits first.
    return ENDIAN_BIG;
}

QString XBOO::getArch()
{
    return QString();
}

QString XBOO::getFileFormatExt()
{
    return QStringLiteral("boo");
}

QString XBOO::getFileFormatExtsString()
{
    return QStringLiteral("BOO transport encoding (*.boo)");
}

QString XBOO::getMIMEString()
{
    return QStringLiteral("text/plain");
}

QString XBOO::getVersion()
{
    return QString();
}

qint64 XBOO::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    // The transport occupies the whole file; there is no trailer and no
    // overlay to distinguish.
    return isValid(pPdStruct) ? getSize() : 0;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBOO::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBOO::initUnpack(UNPACK_STATE *pState,
                      const QMap<UNPACK_PROP, QVariant> &mapProperties,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XBOO> guardedThis(this);
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

    qint64 nLegacyOutputLimit = -1;
    OUTPUT_POLICY outputPolicy = {};
    if (!getUnpackOutputLimit(mapProperties, &nLegacyOutputLimit) ||
        !resolveUnpackOutputPolicy(mapProperties, &outputPolicy)) {
        setPdStructErrorString(pPdStruct, tr("Invalid unpacked-output limit"));
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    // The decode is a single in-memory allocation driven by an unbounded
    // expansion ratio, so every configured ceiling has to be folded in before
    // the first byte is produced.
    qint64 nDecodedLimit = BOO_MAX_DECODED_SIZE;
    if (nLegacyOutputLimit >= 0) {
        nDecodedLimit = qMin(nDecodedLimit, nLegacyOutputLimit);
    }
    if (outputPolicy.nMaxEntryOutputSize >= 0) {
        nDecodedLimit = qMin(nDecodedLimit, outputPolicy.nMaxEntryOutputSize);
    }
    if (outputPolicy.nMaxMemoryOutputSize >= 0) {
        nDecodedLimit = qMin(nDecodedLimit, outputPolicy.nMaxMemoryOutputSize);
    }
    if (outputPolicy.nMaxTotalOutputSize >= 0) {
        nDecodedLimit = qMin(nDecodedLimit, outputPolicy.nMaxTotalOutputSize);
    }
    if ((nDecodedLimit <= 0) ||
        ((outputPolicy.nMaxEntryCount >= 0) &&
         (outputPolicy.nMaxEntryCount < 1))) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    if (!parseContext(&pContext->context, nDecodedLimit, pPdStruct) ||
        !guardedThis || !guardedSource) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->context.nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XBOO::infoCurrent(UNPACK_STATE *pState,
                                         PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    QPointer<XBOO> guardedThis(this);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pState->nNumberOfRecords != 1)) {
        return ARCHIVERECORD();
    }
    const UNPACK_CONTEXT *pContext =
        static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->context.sFileName.isEmpty() ||
        pContext->context.baDecoded.isEmpty()) {
        return ARCHIVERECORD();
    }
    const qint64 nDecodedSize = pContext->context.baDecoded.size();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pState->nTotalSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                pContext->context.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pState->nTotalSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nDecodedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        QStringLiteral("BOO printable transport (6-bit, '~' NUL escape)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The payload is not a byte range of this device - it only exists once
    // decoded - so the record must be published without an extent.  This also
    // replaces the descriptive HANDLE_METHOD above, which is what keeps a
    // generic consumer from resolving STORE against the encoded text.
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) {
        return ARCHIVERECORD();
    }
    return result;
}

bool XBOO::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                         PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XBOO> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !pDevice || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) ||
        (pState->nNumberOfRecords != 1) ||
        devicesAlias(getDevice(), pDevice)) {
        return false;
    }

    const UNPACK_CONTEXT *pContext =
        static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    const QByteArray &baDecoded = pContext->context.baDecoded;
    const qint64 nDecodedSize = baDecoded.size();
    if ((nDecodedSize <= 0) ||
        !isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   nDecodedSize)) {
        return false;
    }

    // This route materializes its own output, so it must charge the operation
    // budget itself: publishUnpackOutput never debits the copy.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(
                pState->nCurrentIndex, pContext->context.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(nDecodedSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(nDecodedSize,
                                                       pPdStruct));
    if (!pStage || !guardedThis || !guardedOutput ||
        (pStage->write(baDecoded) != nDecodedSize) || !pStage->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }
    const bool bResult = publishUnpackOutput(pStage.get(),
                                             guardedOutput.data(), pState,
                                             pPdStruct);
    if (!guardedThis || !bResult) return false;
    pState->nCurrentOffset = pState->nTotalSize;
    return true;
}

bool XBOO::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XBOO> guardedThis(this);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    // Advance first, then report.  With a single member, returning false
    // without incrementing leaves the caller's cursor on record 0 forever and
    // the whole listing comes back empty.
    ++pState->nCurrentIndex;
    pState->nCurrentOffset =
        (pState->nCurrentIndex >= pState->nNumberOfRecords)
            ? pState->nTotalSize
            : 0;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XBOO::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XBOO::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE,
            FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER};
}
