/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xearefpack.h"

#include <QFileInfo>

#include <new>

#include "Algos/xearefpackdecoder.h"

namespace {
// Signature word plus the shortest possible size field: nothing smaller can
// even carry a header, let alone a terminator command.
const qint64 REFPACK_MIN_SIZE = 6;
// Above this the whole container is no longer read for the validity probe;
// a prefix walk is used instead.  The probe allocates only the input buffer -
// it counts output bytes rather than producing them.
const qint64 REFPACK_FULL_PROBE_LIMIT = 0x1000000;  // 16 MiB
const qint64 REFPACK_PARTIAL_PROBE_INPUT = 0x100000;  // 1 MiB
const qint64 REFPACK_PARTIAL_PROBE_OUTPUT = 0x400000;  // 4 MiB
const QString REFPACK_FALLBACK_NAME = QStringLiteral("refpack.bin");
}  // namespace

XEARefPack::XEARefPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

QString XEARefPack::deriveMemberName()
{
    QIODevice *guardedSource = getDevice();

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource);
    if (sDeviceName.isEmpty()) return REFPACK_FALLBACK_NAME;

    // RefPack stores no name of its own.  The payload keeps the container's
    // name, extension included - that is what the reference extractor writes.
    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return REFPACK_FALLBACK_NAME;

    return sFileName;
}

bool XEARefPack::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < REFPACK_MIN_SIZE) return false;
    if (context.nInputSize > XEARefPackDecoder::REFPACK_MAX_INPUT_SIZE) return false;

    // Header first: 10 bytes covers the widest legal form (flags word plus a
    // four-byte packed size plus a four-byte unpacked size).
    const qint64 nHeaderRead = (context.nInputSize < 10) ? context.nInputSize : 10;
    const QByteArray baHeader = read_array_process(0, nHeaderRead, pPdStruct);
    if ((baHeader.size() != nHeaderRead)) return false;

    XEARefPackDecoder::HEADER header = {};
    if (!XEARefPackDecoder::readHeader(baHeader.constData(), baHeader.size(), context.nInputSize, &header)) {
        return false;
    }

    context.nHeaderSize = header.nHeaderSize;
    context.nUncompressedSize = header.nUnpackedSize;
    context.bLargeSizes = header.bLargeSizes;
    context.bHasPackedSize = header.bHasPackedSize;
    context.nStreamOffset = 0;
    context.nStreamSize = context.nInputSize;

    // The two-byte signature alone matches roughly one file in 8000 by chance,
    // so acceptance rests on the command grammar instead.
    const bool bFull = (context.nInputSize <= REFPACK_FULL_PROBE_LIMIT);
    const qint64 nSampleSize = bFull ? context.nInputSize : REFPACK_PARTIAL_PROBE_INPUT;

    const QByteArray baSample = read_array_process(0, nSampleSize, pPdStruct);
    if ((baSample.size() != nSampleSize)) return false;

    const qint64 nProduceLimit = bFull ? 0 : REFPACK_PARTIAL_PROBE_OUTPUT;
    qint64 nProduced = 0;
    qint64 nConsumed = 0;
    if (!XEARefPackDecoder::probeStream(baSample, header, bFull, nProduceLimit, &nProduced, &nConsumed)) {
        return false;
    }

    if (bFull) {
        // probeStream() already demanded the terminator and the exact declared
        // output length; all that is left is that the stream really is the
        // whole file.  Every corpus sample ends on its last byte.
        if (nConsumed != context.nInputSize) return false;
    }

    context.sFileName = deriveMemberName();

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XEARefPack::isValid(PDSTRUCT *pPdStruct)
{
    // The probe runs on a device the caller still owns: remember where its
    // cursor was and put it back, whatever the outcome.
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();

    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);

    guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XEARefPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEARefPack archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XEARefPack::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XEARefPack(pDevice);
}

QList<QString> XEARefPack::getSearchSignatures()
{
    // The plain form is the only one the corpus uses; the 0x01/0x80 flag
    // variants would need their own patterns and are not scanned for.
    return {QStringLiteral("10FB")};
}

XBinary::FT XEARefPack::getFileType()
{
    return FT_EA_REFPACK;
}

XBinary::MODE XEARefPack::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEARefPack::getEndian()
{
    // The size fields are stored most significant byte first.
    return ENDIAN_BIG;
}

QString XEARefPack::getArch()
{
    return QString();
}

qint32 XEARefPack::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

QString XEARefPack::getFileFormatExt()
{
    return QStringLiteral("qfs");
}

QString XEARefPack::getFileFormatExtsString()
{
    return QStringLiteral("EA RefPack/QFS compressed file (*.qfs *.fsh *.cfs *.ori *.iff)");
}

QString XEARefPack::getMIMEString()
{
    return QStringLiteral("application/x-ea-refpack");
}

QString XEARefPack::getVersion()
{
    // There is no version field.  What the header does vary is the width of
    // the size fields and whether a packed size is present, so report that -
    // it is the only thing that distinguishes one container form from another.
    // Like isValid(), this touches a device the caller owns; put the cursor
    // back where it was.
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();

    CONTEXT context = {};
    const bool bParsed = parseContext(&context, nullptr);

    guardedSource->seek(nSavedPosition);

    if (!bParsed) return QString();

    QString sResult = context.bLargeSizes ? QStringLiteral("32") : QStringLiteral("24");
    if (context.bHasPackedSize) sResult += QStringLiteral("+packed");

    return sResult;
}

qint64 XEARefPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XEARefPack::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XEARefPack::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XEARefPack::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XEARefPack::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = context.nHeaderSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_STREAM;
        // The decoder consumes the signature word itself: the stream part has
        // to start at offset 0, not after the header.
        record.nFileOffset = context.nStreamOffset;
        record.nFileSize = context.nStreamSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_EA_REFPACK);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("EA RefPack"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XEARefPack::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEARefPack::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();

    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) {
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

    if (!parseContext(pContext, pPdStruct)) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("EA RefPack/QFS compressed file; single member"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source; without this finalize the listing would
    // work and every extraction would silently produce nothing.
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

XBinary::ARCHIVERECORD XEARefPack::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_EA_REFPACK);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("EA RefPack"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XEARefPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XEARefPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
