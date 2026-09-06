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
#include "xrawlzw15v.h"

#include "Algos/xrawlzw15vdecoder.h"

#include <QFileInfo>
#include <QPointer>

#include <new>

namespace {

// The stream is the whole file, from byte 0.
const qint64 RAWLZW_STREAM_OFFSET = 0;

// U3 reads 16 bytes before it will even look at a candidate; nothing smaller
// can carry a code stream plus its END code anyway.
const qint64 RAWLZW_MIN_FILE_SIZE = 16;

// Detection has to decode the WHOLE stream (there is no header to gate on), so
// the input is capped rather than the decode being sampled.  The reference
// corpus tops out at 136 KB; DOS-era install payloads never approach this.
const qint64 RAWLZW_MAX_FILE_SIZE = Q_INT64_C(16) * 1024 * 1024;

// LZW can legitimately expand a lot, but a "valid" decode that runs into the
// hundreds of megabytes is a runaway, not a member.
const qint64 RAWLZW_MAX_OUTPUT_SIZE = Q_INT64_C(64) * 1024 * 1024;

// A stream this short cannot be told apart from a coincidence.
const qint64 RAWLZW_MIN_OUTPUT_SIZE = 64;

// Enough bytes to notice that the device under a cached result changed.
const qint32 RAWLZW_CACHE_PREFIX_SIZE = 32;

const QString RAWLZW_FALLBACK_NAME = QStringLiteral("rawlzw15v.bin");

}  // namespace

XRawLzw15v::XRawLzw15v(QIODevice *pDevice)
    : XArchive(pDevice), m_bContextCached(false), m_bContextValid(false), m_context(), m_pCachedDevice(nullptr), m_nCachedSize(-1)
{
}

QString XRawLzw15v::deriveContainerName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return RAWLZW_FALLBACK_NAME;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (sDeviceName.isEmpty()) return RAWLZW_FALLBACK_NAME;

    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return RAWLZW_FALLBACK_NAME;

    return sFileName;
}

bool XRawLzw15v::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XRawLzw15v> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if ((nInputSize < RAWLZW_MIN_FILE_SIZE) || (nInputSize > RAWLZW_MAX_FILE_SIZE)) return false;

    const QByteArray baPrefix = read_array_process(0, RAWLZW_CACHE_PREFIX_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baPrefix.size() != RAWLZW_CACHE_PREFIX_SIZE)) return false;

    if (m_bContextCached && (m_pCachedDevice == guardedSource.data()) && (m_nCachedSize == nInputSize) && (m_baCachedPrefix == baPrefix)) {
        if (!m_bContextValid) return false;
        *pContext = m_context;
        return isPdStructNotCanceled(pPdStruct);
    }

    m_bContextCached = true;
    m_bContextValid = false;
    m_context = CONTEXT();
    m_pCachedDevice = guardedSource.data();
    m_nCachedSize = nInputSize;
    m_baCachedPrefix = baPrefix;

    const QByteArray baPacked = read_array_process(RAWLZW_STREAM_OFFSET, nInputSize, pPdStruct);
    if (!guardedThis || !guardedSource || (static_cast<qint64>(baPacked.size()) != nInputSize)) return false;

    // The ONLY detector this format has: decode the whole thing under the
    // strict grammar and refuse anything that does not end exactly on the END
    // code with the input fully consumed.
    qint64 nUncompressedSize = 0;
    if (!XRawLzw15vDecoder::probe(baPacked, RAWLZW_MAX_OUTPUT_SIZE, &nUncompressedSize)) return false;

    // A stream that does not expand is not a compressed member; a compressor
    // that produced one would have stored the file instead.
    if ((nUncompressedSize < RAWLZW_MIN_OUTPUT_SIZE) || (nUncompressedSize <= nInputSize)) return false;

    CONTEXT context = {};
    context.nInputSize = nInputSize;
    context.nStreamOffset = RAWLZW_STREAM_OFFSET;
    context.nStreamSize = nInputSize;
    context.nUncompressedSize = nUncompressedSize;
    context.sFileName = deriveContainerName();
    if (!guardedThis || !guardedSource) return false;
    if (context.sFileName.isEmpty()) context.sFileName = RAWLZW_FALLBACK_NAME;

    if (!isPdStructNotCanceled(pPdStruct)) return false;

    m_bContextValid = true;
    m_context = context;

    *pContext = context;

    return true;
}

bool XRawLzw15v::isValid(PDSTRUCT *pPdStruct)
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

bool XRawLzw15v::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRawLzw15v archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XRawLzw15v::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XRawLzw15v(pDevice);
}

XBinary::FT XRawLzw15v::getFileType()
{
    return FT_RAW_LZW15V;
}

XBinary::MODE XRawLzw15v::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XRawLzw15v::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XRawLzw15v::getArch()
{
    return QString();
}

qint32 XRawLzw15v::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

QString XRawLzw15v::getFileFormatExt()
{
    // These members keep the DOS "last extension character replaced by _"
    // habit (WIPEOUT.EX_, BILLBD.DL_), so there is no extension of their own.
    return QStringLiteral("_");
}

QString XRawLzw15v::getFileFormatExtsString()
{
    return QStringLiteral("Raw LZW15V compressed file (*.??_)");
}

QString XRawLzw15v::getMIMEString()
{
    return QStringLiteral("application/x-lzw15v");
}

QString XRawLzw15v::getVersion()
{
    // Nothing is stored, so the only honest "version" is the codec's code
    // width range.
    return QStringLiteral("9-15");
}

qint64 XRawLzw15v::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XRawLzw15v::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XRawLzw15v::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XRawLzw15v::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XRawLzw15v::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    // There is no header part at all: byte 0 is already payload.
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
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_RAW_LZW15V);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZW15V"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XRawLzw15v::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XRawLzw15v::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XRawLzw15v> guardedThis(this);
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Raw LZW15V stream; single member"));
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

XBinary::ARCHIVERECORD XRawLzw15v::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    // Extraction rides the generic decode chain; no unpackCurrent override.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_RAW_LZW15V);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("LZW15V"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XRawLzw15v::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XRawLzw15v::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
