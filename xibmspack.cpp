/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xibmspack.h"

#include "Algos/xflsdecoder.h"
#include "subdevice.h"

#include <QFileInfo>
#include <QPointer>

#include <new>

namespace {

// The whole header.  Everything after this byte is codec payload, and
// XFLSDecoder reads the tag itself, so the member stream starts at offset 0.
const quint8 IBMS_STREAM_TAG = 0x53U;

// The shortest reference file is 101 bytes; anything below the tag plus a
// couple of code bytes cannot carry a class header and an end code.
const qint64 IBMS_MIN_FILE_SIZE = Q_INT64_C(4);

// Guards for the trial decode that stands in for a magic number.  Both are far
// above the format's real range (the 238-file reference corpus tops out at
// 448310 bytes in and 2154496 bytes out) and exist only so that a hostile file
// cannot turn detection into an unbounded decode.
const qint64 IBMS_MAX_FILE_SIZE = Q_INT64_C(64) * 1024 * 1024;
const qint64 IBMS_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(256) * 1024 * 1024;

// No code in this codec is shorter than eight bits (a two-bit class selector
// plus the six-bit narrow class) and no dictionary phrase is longer than 250
// bytes, so a stream can never expand by more than this factor.  Applying it
// keeps a crafted short file from costing a full IBMS_MAX_UNCOMPRESSED_SIZE
// decode per probe.
const qint64 IBMS_MAX_EXPANSION_RATIO = Q_INT64_C(250);

// First span tried when bracketing the member size.  It matches the decoder's
// own output block, so the usual stream is bracketed by a single probe, but
// nothing depends on the two being equal - the bracket grows until it holds.
const qint64 IBMS_PROBE_WINDOW = Q_INT64_C(0x4000);

// Hard stop for the size search.  Bracketing plus bisecting the full ceiling
// needs well under half of this; it exists so a pathological device can never
// keep the search running.
const qint32 IBMS_MAX_PROBES = 96;

// Bytes of the file kept alongside the memoised parse so a reused object with a
// different device of the same length cannot be served a stale context.
const qint64 IBMS_CACHE_PREFIX_SIZE = Q_INT64_C(16);

const QString IBMS_FALLBACK_NAME = QStringLiteral("ibm_spack.bin");
const QString IBMS_METHOD_NAME = QStringLiteral("IBM 'S' adaptive phrase");

// A write-only sink that keeps nothing.  The codec has no size field, so the
// only way to learn how long a member is, is to decode it; there is no reason
// to hold those bytes anywhere while doing so.
class IBMS_COUNTING_SINK : public QIODevice {
public:
    IBMS_COUNTING_SINK() : m_nCount(0)
    {
    }

    bool isSequential() const override
    {
        return true;
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)
        Q_UNUSED(nMaxSize)

        return -1;
    }

    qint64 writeData(const char *pData, qint64 nSize) override
    {
        Q_UNUSED(pData)

        if (nSize < 0) return -1;

        m_nCount += nSize;

        return nSize;
    }

private:
    qint64 m_nCount;
};

// A read-only view of the member that hands out its LAST BYTE on its own.
//
// XFLSDecoder reports how much input it read, never how much it consumed, and
// it refills in 0x4000-byte blocks.  Read through the archive's own device, a
// decode that stopped one code short of the end code and a decode that read the
// end code therefore report the same nCountInput - the two differ by a couple
// of bytes that live in the same refill.  With this view the decoder can only
// reach the stream size by asking for the final byte, so "nCountInput equals
// the stream size" becomes an exact statement that the end code was reached.
// That is the monotone predicate the size search below bisects on, and it can
// never be satisfied by accident: the end code is at least fourteen bits wide,
// so the code in front of it always finishes at least one byte short of the
// stream, and a decode that stops there cannot have asked for the last byte.
class IBMS_TAIL_EXACT_DEVICE : public SubDevice {
public:
    IBMS_TAIL_EXACT_DEVICE(QIODevice *pDevice, qint64 nOffset, qint64 nSize) : SubDevice(pDevice, nOffset, nSize)
    {
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        const qint64 nPosition = pos();
        const qint64 nRangeSize = size();

        if ((nMaxSize > 0) && (nPosition >= 0) && (nRangeSize > 0) && (nPosition < nRangeSize)) {
            const qint64 nLastByte = nRangeSize - 1;

            if (nPosition < nLastByte) {
                nMaxSize = qMin(nMaxSize, nLastByte - nPosition);
            } else {
                nMaxSize = qMin(nMaxSize, Q_INT64_C(1));
            }
        }

        return SubDevice::readData(pData, nMaxSize);
    }
};

bool ibmsRunCodec(QIODevice *pDevice, qint64 nOffset, qint64 nSize, qint64 nDeclaredSize, XBinary::PDSTRUCT *pPdStruct, bool *pbExact, qint64 *pnProduced,
                  qint64 *pnConsumed)
{
    if (!pbExact || !pnProduced || !pnConsumed) return false;

    *pbExact = false;
    *pnProduced = 0;
    *pnConsumed = 0;

    IBMS_COUNTING_SINK sink;
    if (!sink.open(QIODevice::WriteOnly)) return false;

    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nDeclaredSize);
    state.pDeviceInput = pDevice;
    state.pDeviceOutput = &sink;
    state.nInputOffset = nOffset;
    state.nInputLimit = nSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    QPointer<QIODevice> guardedInput(pDevice);

    *pbExact = XFLSDecoder::decompress(&state, pPdStruct);

    if (!guardedInput) return false;
    if (state.bReadError || state.bWriteError) return false;

    *pnProduced = state.nCountOutput;
    *pnConsumed = state.nCountInput;

    return true;
}

}  // namespace

XIBMSPack::XIBMSPack(QIODevice *pDevice)
    : XArchive(pDevice), m_bContextCached(false), m_contextCache(), m_pContextDevice(nullptr), m_nContextSize(-1)
{
}

QString XIBMSPack::deriveContainerName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return IBMS_FALLBACK_NAME;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (sDeviceName.isEmpty()) return IBMS_FALLBACK_NAME;

    // The packed name is the original one with the last extension character
    // replaced by '#'; the original character is not stored anywhere, so the
    // container's own name is the only truthful thing to publish.  U3 does the
    // same for this format.
    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return IBMS_FALLBACK_NAME;

    return sFileName;
}

bool XIBMSPack::measureStream(QIODevice *pDevice, qint64 nOffset, qint64 nSize, qint64 *pnUncompressedSize, PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pnUncompressedSize || (nOffset < 0) || (nSize <= 0) || (nSize > IBMS_MAX_FILE_SIZE)) return false;

    // The largest member this stream could possibly carry.  Below the format's
    // own guard for any short file, so a hostile one cannot buy a long decode
    // with a few bytes.
    qint64 nCeiling = IBMS_MAX_UNCOMPRESSED_SIZE;
    if (nSize < (nCeiling / IBMS_MAX_EXPANSION_RATIO)) nCeiling = nSize * IBMS_MAX_EXPANSION_RATIO;
    if (nCeiling <= 0) return false;

    // Unbuffered: QIODevice must hand every request straight to the view, so
    // that the short read which isolates the last byte is the one the codec
    // sees.  A read buffer in between would refill itself across that boundary.
    IBMS_TAIL_EXACT_DEVICE probeDevice(pDevice, nOffset, nSize);
    if (!probeDevice.open(QIODevice::ReadOnly | QIODevice::Unbuffered)) return false;
    if (probeDevice.size() != nSize) return false;

    // Probe one runs the codec with a deliberately generous declared size so it
    // reaches its own end code.
    //
    // WHAT COMES BACK IS NOT THE MEMBER SIZE.  XFLSDecoder buffers decoded
    // bytes in 0x4000-byte blocks and writes the final partial block ONLY when
    // the whole decode succeeded - and it cannot succeed here, because a
    // declared size that is not the real one makes it return false.  So
    // nCountOutput is the flushed byte count: a floor that is short of the
    // member by anything from one byte to a full block, and plain zero for
    // every member below 0x4000 bytes.  Believing it is what made this class
    // measure 0 and then reject all 238 reference files.
    bool bExact = false;
    qint64 nFloor = 0;
    qint64 nConsumed = 0;
    if (!ibmsRunCodec(&probeDevice, 0, nSize, nCeiling, pPdStruct, &bExact, &nFloor, &nConsumed)) return false;

    // A member that stops before the end of its own extent, or that runs past
    // the probe ceiling, is not this format.  Over the tail-exact view the
    // input count is byte-precise, so this rejects a stream that merely stopped
    // near the end as firmly as one that fell over immediately.
    if ((nConsumed != nSize) || (nFloor < 0) || (nFloor > nCeiling)) return false;

    // bExact means the member happens to be exactly the ceiling, which probe
    // one has then already verified: the bracket is a single value.
    qint64 nLow = nFloor;
    qint64 nHigh = bExact ? nFloor : Q_INT64_C(-1);
    qint32 nProbes = 0;

    // Bracket the member size.  The predicate "the decode read the last byte of
    // the stream" is monotone in the declared size: below the real size the
    // codec runs out of declared room and stops on an earlier code, at or above
    // it the codec always reaches the end code.  Grow the span until it holds.
    for (qint64 nWindow = IBMS_PROBE_WINDOW; (nHigh < 0) && (nProbes < IBMS_MAX_PROBES); nWindow *= 2) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        qint64 nCandidate = ((nCeiling - nLow) < nWindow) ? nCeiling : (nLow + nWindow);

        bool bProbeExact = false;
        qint64 nProbeProduced = 0;
        qint64 nProbeConsumed = 0;
        nProbes++;
        if (!ibmsRunCodec(&probeDevice, 0, nSize, nCandidate, pPdStruct, &bProbeExact, &nProbeProduced, &nProbeConsumed)) return false;

        if (bProbeExact || (nProbeConsumed == nSize)) {
            nHigh = nCandidate;
            if (bProbeExact) nLow = nCandidate;
        } else if (nCandidate >= nCeiling) {
            return false;
        } else {
            nLow = nCandidate + 1;
        }
    }

    if ((nHigh < 0) || (nLow > nHigh)) return false;

    // Bisect the bracket down to the one declared size the codec accepts.
    while ((nLow < nHigh) && (nProbes < IBMS_MAX_PROBES)) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nMiddle = nLow + ((nHigh - nLow) / 2);

        bool bProbeExact = false;
        qint64 nProbeProduced = 0;
        qint64 nProbeConsumed = 0;
        nProbes++;
        if (!ibmsRunCodec(&probeDevice, 0, nSize, nMiddle, pPdStruct, &bProbeExact, &nProbeProduced, &nProbeConsumed)) return false;

        if (bProbeExact) {
            nLow = nMiddle;
            nHigh = nMiddle;
        } else if (nProbeConsumed == nSize) {
            nHigh = nMiddle;
        } else {
            nLow = nMiddle + 1;
        }
    }

    if ((nLow != nHigh) || (nLow <= 0) || (nLow > nCeiling)) return false;

    // The authority, and deliberately run over the archive's own device rather
    // than the probe view, so the size that is published is the one the
    // extraction path will be handed.  XFLSDecoder returns true only when the
    // stream ended on its end code, produced exactly the declared byte count
    // and consumed every input byte, so a garbage stream that merely stumbled
    // through the search with a plausible number cannot survive here.
    bool bVerified = false;
    qint64 nVerifiedProduced = 0;
    qint64 nVerifiedConsumed = 0;
    if (!ibmsRunCodec(pDevice, nOffset, nSize, nLow, pPdStruct, &bVerified, &nVerifiedProduced, &nVerifiedConsumed)) return false;

    if (!bVerified || (nVerifiedProduced != nLow) || (nVerifiedConsumed != nSize)) return false;

    *pnUncompressedSize = nLow;

    return true;
}

bool XIBMSPack::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XIBMSPack> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if ((nInputSize < IBMS_MIN_FILE_SIZE) || (nInputSize > IBMS_MAX_FILE_SIZE)) return false;

    const QByteArray baPrefix = read_array_process(0, IBMS_CACHE_PREFIX_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || baPrefix.isEmpty()) return false;
    if (static_cast<quint8>(baPrefix.at(0)) != IBMS_STREAM_TAG) return false;

    if (m_bContextCached && (m_pContextDevice == guardedSource.data()) && (m_nContextSize == nInputSize) && (m_baContextPrefix == baPrefix)) {
        *pContext = m_contextCache;
        return true;
    }

    CONTEXT context = {};
    context.nInputSize = nInputSize;
    // The tag byte belongs to the codec stream, so the member extent is the
    // whole file; handing XFLSDecoder anything past offset 0 loses the tag it
    // insists on reading.
    context.nStreamOffset = 0;
    context.nStreamSize = nInputSize;

    if (!measureStream(guardedSource.data(), context.nStreamOffset, context.nStreamSize, &context.nUncompressedSize, pPdStruct)) return false;
    if (!guardedThis || !guardedSource) return false;

    context.sFileName = deriveContainerName();
    if (!guardedThis || !guardedSource) return false;
    if (context.sFileName.isEmpty()) context.sFileName = IBMS_FALLBACK_NAME;

    if (!isPdStructNotCanceled(pPdStruct)) return false;

    m_contextCache = context;
    m_pContextDevice = guardedSource.data();
    m_nContextSize = nInputSize;
    m_baContextPrefix = baPrefix;
    m_bContextCached = true;

    *pContext = context;

    return true;
}

bool XIBMSPack::isValid(PDSTRUCT *pPdStruct)
{
    // Detection runs on a device the caller still owns: snapshot the cursor and
    // put it back whatever the outcome.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XIBMSPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIBMSPack archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XIBMSPack::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XIBMSPack(pDevice);
}

QList<QString> XIBMSPack::getSearchSignatures()
{
    // A single 'S' is not a signature; scanning for it would flood every
    // result set.  Detection is the trial decode in isValid().
    return QList<QString>();
}

XBinary::FT XIBMSPack::getFileType()
{
    return FT_IBM_SPACK;
}

XBinary::MODE XIBMSPack::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIBMSPack::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIBMSPack::getArch()
{
    return QString();
}

qint32 XIBMSPack::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

QString XIBMSPack::getFileFormatExt()
{
    // Members carry the original extension with its last character replaced by
    // '#', so the format has no extension of its own.
    return QStringLiteral("#");
}

QString XIBMSPack::getFileFormatExtsString()
{
    return QStringLiteral("IBM install-diskette packed file (*.*#)");
}

QString XIBMSPack::getMIMEString()
{
    return QStringLiteral("application/x-ibm-spack");
}

QString XIBMSPack::getVersion()
{
    // Nothing in the one-byte header identifies a revision.
    return QString();
}

qint64 XIBMSPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XIBMSPack::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XIBMSPack::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        // No header region: the one header byte is the codec's own stream tag,
        // so it lives inside the stream part and cannot be mapped beside it.
        return _getMemoryMap(FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XIBMSPack::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIBMSPack::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    // There is deliberately no FILEPART_HEADER record.  The whole header is the
    // single 'S' byte at offset 0, XFLSDecoder reads that byte itself, and the
    // stream part therefore starts at 0 as well - publishing both would hand
    // the memory map two parts covering the same byte.

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_STREAM;
        record.nFileOffset = context.nStreamOffset;
        record.nFileSize = context.nStreamSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_FLS_LZ);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, IBMS_METHOD_NAME);
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XIBMSPack::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIBMSPack::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XIBMSPack> guardedThis(this);
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
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }

    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("IBM install-diskette packed file; single member"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source.  Without this finalize the listing works
    // and every extraction silently writes nothing.
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

XBinary::ARCHIVERECORD XIBMSPack::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    // Extraction rides the generic decode chain; the codec is the one XArchive
    // already ships for IBM SaveRam FLS members.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_FLS_LZ);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, IBMS_METHOD_NAME);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XIBMSPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // The index must move PAST the last record; stopping one short makes both
    // the GUI and the CLI list nothing at all.
    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->nStreamOffset;
        return true;
    }

    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XIBMSPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}
