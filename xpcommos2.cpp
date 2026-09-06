/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 *
 * The codec was recovered by differential decoding against a working
 * extractor over the full 582-file PCOMM 4.x install-diskette corpus.  The
 * one non-obvious piece is the short control byte: C < 0x20 is a THREE-byte
 * token whose two payload bytes are an absolute little-endian offset from the
 * start of the current 16 KiB block, not a distance back.  Reading it as an
 * ordinary two-byte match token decodes the first few dozen bytes of a file
 * and then dies on an impossible distance.
 */

#include "xpcommos2.h"

#include <QPointer>

#include <limits>
#include <new>

#include "Algos/xpcommos2decoder.h"

namespace {

// The container has no magic, so every constant here is a shape constraint.
// The shortest possible file is a one-byte literal run followed by the two
// block terminators.
const qint64 PCOMM_MIN_FILE_SIZE = 4;

// This is an install-diskette payload: the largest member of the reference
// corpus is barely over one megabyte, and a floppy caps the family anyway.
// The ceiling matters because detection has to read the candidate whole -
// there is no header to reject it on - so it is what keeps a probe of a large
// unrelated file from turning into a large allocation.
const qint64 PCOMM_MAX_FILE_SIZE = Q_INT64_C(32) * 1024 * 1024;

}  // namespace

XPCommOS2::XPCommOS2(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPCommOS2::~XPCommOS2()
{
}

bool XPCommOS2::probeGate(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // Three single-byte reads, run before the candidate is pulled into memory.
    // A block always opens with a literal-run control (there is nothing to
    // match against yet), and a complete file always ends with the two block
    // terminators.  Over 18806 files of other formats this gate alone left
    // nothing for the full walk to reject.
    QPointer<XPCommOS2> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nSize = getSize();
    if (!guardedThis || !guardedSource || (nSize < PCOMM_MIN_FILE_SIZE) || (nSize > PCOMM_MAX_FILE_SIZE)) {
        return false;
    }

    const quint8 nFirst = read_uint8(0);
    if (!guardedThis || !guardedSource) return false;
    if (nFirst < 0xE1) return false;

    const quint8 nLast = read_uint8(nSize - 1);
    if (!guardedThis || !guardedSource) return false;
    const quint8 nPrevious = read_uint8(nSize - 2);
    if (!guardedThis || !guardedSource) return false;

    return (nLast == 0xE0) && (nPrevious == 0xE0);
}

bool XPCommOS2::readSource(QByteArray *pbaSource, PDSTRUCT *pPdStruct)
{
    if (!pbaSource || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPCommOS2> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nSize = getSize();
    if (!guardedThis || !guardedSource || (nSize < PCOMM_MIN_FILE_SIZE) || (nSize > PCOMM_MAX_FILE_SIZE) ||
        (nSize > static_cast<qint64>((std::numeric_limits<int>::max)()))) {
        return false;
    }

    *pbaSource = read_array_process(0, nSize, pPdStruct);

    return guardedThis && guardedSource && (static_cast<qint64>(pbaSource->size()) == nSize) && isPdStructNotCanceled(pPdStruct);
}

QString XPCommOS2::sourceMemberName()
{
    // The packer stores no name; the only name that exists is the one on the
    // device, with its mangled last character.  The missing character is not
    // recoverable from the stream, so the name is reported exactly as found
    // rather than guessed at.
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return QString();

    QString sResult = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedSource) return QString();

    const QString sSuffix = XBinary::getDeviceFileCompleteSuffix(guardedSource.data());
    if (!guardedSource) return QString();

    if (!sSuffix.isEmpty()) {
        sResult += QString(".") + sSuffix;
    }

    // A device without a backing file still has to yield a usable output name.
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("pcomm_data");
    }

    return sResult;
}

bool XPCommOS2::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XPCommOS2> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    // Cheap bounded shape test first, so the whole-file read below only ever
    // runs on a genuine candidate.
    if (!probeGate(pPdStruct) || !guardedThis || !guardedSource) return false;

    QByteArray baSource;
    if (!readSource(&baSource, pPdStruct) || !guardedThis || !guardedSource) return false;

    CONTEXT context;
    context.nInputSize = static_cast<qint64>(baSource.size());

    // The structural walk is the whole detector: it produces no output, but it
    // proves the token stream is self-consistent, that no block overruns
    // 16384 bytes, that every block but the last is exactly full, and that the
    // input is consumed to the byte.  Nothing but this format survives it.
    if (!XPCommOS2Decoder::scan(baSource, &context.nUncompressedSize)) return false;
    if (context.nUncompressedSize <= 0) return false;

    context.sFileName = sourceMemberName();
    if (!guardedThis || !guardedSource || context.sFileName.isEmpty()) return false;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XPCommOS2::isValid(PDSTRUCT *pPdStruct)
{
    // Detection probes a device the caller still owns; read_array_process
    // moves its cursor, so the position has to be put back.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context;
    const bool bResult = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XPCommOS2::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPCommOS2 archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XPCommOS2::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XPCommOS2(pDevice);
}

QList<QString> XPCommOS2::getSearchSignatures()
{
    // Deliberately empty: the format has no magic at all - the first byte is
    // an ordinary literal-run control.  XBinary's scan helper returns an empty
    // result for an empty list, so this cannot degrade into a match-everything
    // scan.
    return QList<QString>();
}

XBinary::FT XPCommOS2::getFileType()
{
    return FT_PCOMM_OS2;
}

XBinary::MODE XPCommOS2::getMode()
{
    return MODE_DATA;
}

qint32 XPCommOS2::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XPCommOS2::getEndian()
{
    // The only multi-byte field in the stream is the absolute match offset of
    // the three-byte token, and it is little-endian.
    return ENDIAN_LITTLE;
}

QString XPCommOS2::getArch()
{
    return QString();
}

QString XPCommOS2::getFileFormatExt()
{
    return QStringLiteral("pcomm");
}

QString XPCommOS2::getFileFormatExtsString()
{
    return QStringLiteral("IBM PCOMM for OS/2 packed file (*.??_)");
}

QString XPCommOS2::getMIMEString()
{
    return QStringLiteral("application/octet-stream");
}

QString XPCommOS2::getVersion()
{
    return QString();
}

XBinary::OSNAME XPCommOS2::getOsName()
{
    return OSNAME_OS2;
}

qint64 XPCommOS2::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    // The token stream runs to the last byte - the trailing 0xE0 0xE0 pair is
    // part of it - so there is no trailer and no overlay to distinguish.
    return isValid(pPdStruct) ? getSize() : 0;
}

QMap<XBinary::UNPACK_PROP, QVariant> XPCommOS2::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPCommOS2::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XPCommOS2> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    // Staging the source is only half of the contract: without the matching
    // validateAndFinalizeUnpackSource() below, listing works and extraction
    // silently yields nothing.
    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis || !guardedSource) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    if (!parseContext(&pContext->context, pPdStruct) || !guardedThis || !guardedSource) {
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

XBinary::ARCHIVERECORD XPCommOS2::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XPCommOS2> guardedThis(this);

    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nNumberOfRecords != 1)) {
        return ARCHIVERECORD();
    }

    const UNPACK_CONTEXT *pContext = static_cast<const UNPACK_CONTEXT *>(pState->pContext);
    if (pContext->context.sFileName.isEmpty() || (pContext->context.nUncompressedSize <= 0) || (pContext->context.nInputSize <= 0)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    // The member is the entire device: the stream starts at byte 0 and its
    // terminator is the last byte of the file.
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->context.nInputSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->context.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->context.nInputSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->context.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_PCOMM_OS2);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("PCOMM OS/2 LZ77 (16K blocks)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XPCommOS2::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XPCommOS2> guardedThis(this);

    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    // Advance first, then report.  With a single member, returning false
    // without incrementing leaves the caller's cursor on record 0 forever and
    // the whole listing comes back empty.
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = (pState->nCurrentIndex >= pState->nNumberOfRecords) ? pState->nTotalSize : 0;

    return pState->nCurrentIndex < pState->nNumberOfRecords;
}

bool XPCommOS2::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XPCommOS2::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER};
}
