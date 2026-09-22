/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xnpack.h"

#include <QFileInfo>
#include <cstring>
#include <new>

#include "Algos/xnpackdecoder.h"

namespace {
const qint64 NPACK_HEADER_SIZE = XNPackDecoder::NPACK_MAGIC_SIZE;
// The shortest possible block is "1 1 0000000" - a bare stop code - which needs
// one byte; nothing that small can be a real member, and a two-byte payload
// floor keeps the probe from accepting a 6-byte "MSTSM"+junk blob.
const qint64 NPACK_MIN_FILE_SIZE = NPACK_HEADER_SIZE + 2;
// Containers up to this size are walked end to end by parseContext(), which is
// what makes the "ends on the stop code exactly at EOF" rule enforceable.  The
// largest corpus sample is 389149 bytes, so the partial path below is never
// taken in practice; it exists so a pathological input cannot make detection
// walk hundreds of megabytes of bits.
const qint64 NPACK_FULL_PROBE_LIMIT = 0x800000;   // 8 MiB packed
const qint64 NPACK_FULL_PROBE_OUTPUT = 0x8000000; // 128 MiB unpacked ceiling
const qint64 NPACK_PARTIAL_PROBE_INPUT = 0x100000;

const char NPACK_MAGIC[5] = {'M', 'S', 'T', 'S', 'M'};
}  // namespace

XBinary::XCONVERT _TABLE_XNPACK_STRUCTID[] = {{XNPack::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                              {XNPack::STRUCTID_NPACK_HEADER, "NPACK_HEADER", QString("NPack header")}};

XNPack::XNPack(QIODevice *pDevice) : XArchive(pDevice)
{
}

XNPack::~XNPack()
{
}

bool XNPack::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < NPACK_MIN_FILE_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, NPACK_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != NPACK_HEADER_SIZE)) return false;
    if (std::memcmp(baHeader.constData(), NPACK_MAGIC, sizeof(NPACK_MAGIC)) != 0) return false;

    context.nStreamOffset = NPACK_HEADER_SIZE;
    context.nStreamSize = context.nInputSize - NPACK_HEADER_SIZE;
    context.nUncompressedSize = -1;

    const bool bFullProbe = (context.nStreamSize <= NPACK_FULL_PROBE_LIMIT);
    const qint64 nProbeSize = bFullProbe ? context.nStreamSize : NPACK_PARTIAL_PROBE_INPUT;

    const QByteArray baPayload = read_array_process(context.nStreamOffset, nProbeSize, pPdStruct);
    if ((baPayload.size() != nProbeSize)) return false;

    XNPackDecoder::PROBE_RESULT probe = {};

    if (bFullProbe) {
        if (!XNPackDecoder::probeStream(baPayload.constData(), baPayload.size(), NPACK_FULL_PROBE_OUTPUT, &probe)) {
            return false;
        }
        // The block must own the whole file: every corpus sample terminates on
        // its stop code inside the final byte, leaving nothing behind it.
        if (!probe.bStopCode || probe.bOutputCapped) return false;
        if (probe.nConsumed != context.nStreamSize) return false;
        if (probe.nProduced <= 0) return false;

        context.nUncompressedSize = probe.nProduced;
    } else {
        // Only a prefix is available, so the stop code is out of reach.  Cap the
        // output at half the probed input: a literal costs 9 bits, so the walk
        // is guaranteed to hit that cap before it runs off the end of the
        // prefix, and "ran out of input" therefore stays a genuine error.
        if (!XNPackDecoder::probeStream(baPayload.constData(), baPayload.size(), NPACK_PARTIAL_PROBE_INPUT / 2, &probe)) {
            return false;
        }
        if (!probe.bOutputCapped) return false;
    }

    QString sName = XBinary::getDeviceFileName(guardedSource);
    if (!guardedSource) return false;
    if (!sName.isEmpty()) sName = QFileInfo(sName).fileName();
    // NPack keeps no name of its own; the installer encodes it by replacing the
    // last character of the original name with '$', and the reference extractor
    // simply reuses the container's name, so that is what is reported here.
    if (sName.isEmpty()) sName = QStringLiteral("npack_data");
    context.sFileName = sName;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XNPack::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if ((nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XNPack::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XNPack npack(pDevice);
    return npack.isValid(pPdStruct);
}

XBinary *XNPack::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XNPack(pDevice);
}

QList<QString> XNPack::getSearchSignatures()
{
    // Five ASCII bytes is all the container gives; the structural walk in
    // parseContext() is what actually decides.
    return {QStringLiteral("'MSTSM'")};
}

XBinary::FT XNPack::getFileType()
{
    return FT_NPACK;
}

XBinary::MODE XNPack::getMode()
{
    return MODE_DATA;
}

qint32 XNPack::getType()
{
    return TYPE_NPACK;
}

QString XNPack::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_NPACK: sResult = QString("NPACK"); break;
    }

    return sResult;
}

XBinary::ENDIAN XNPack::getEndian()
{
    // The magic is ASCII and the payload has no multi-byte scalar fields at all;
    // its bit stream is MSB-first, which is a decoder detail, not a container
    // property.
    return ENDIAN_LITTLE;
}

QString XNPack::getArch()
{
    return QString();
}

XBinary::OSNAME XNPack::getOsName()
{
    return OSNAME_WINDOWS;
}

QString XNPack::getFileFormatExt()
{
    // There is no canonical extension: the installer mangles the ORIGINAL
    // extension's last character to '$' (.WB$, .NS$, .38$), so the only stable
    // thing to report is the trailing '$'.
    return QStringLiteral("$");
}

QString XNPack::getFileFormatExtsString()
{
    return QStringLiteral("Symantec NPack (*.$)");
}

QString XNPack::getMIMEString()
{
    return QStringLiteral("application/x-npack");
}

QString XNPack::getVersion()
{
    // The container has no version field, and the payload grammar is identical
    // across every sample; reporting a made-up number would be worse than
    // reporting none.
    return QString();
}

qint64 XNPack::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XNPack::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XNPack::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XNPack::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XNPACK_STRUCTID, sizeof(_TABLE_XNPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XNPack::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XNPACK_STRUCTID, sizeof(_TABLE_XNPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XNPack::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XNPACK_STRUCTID, sizeof(_TABLE_XNPACK_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XNPack::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;
    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_NPACK_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_NPACK_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) headerLoc = offsetToLoc(0);

        const qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);
        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(NPACK_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_NPACK_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(NPACK_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_NPACK_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_NPACK_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XNPack::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_NPACK_HEADER) {
        listResult.append({"magic", static_cast<qint32>(offsetof(NPACK_HEADER, magic)), 5, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    }

    return listResult;
}

QString XNPack::methodToString()
{
    return tr("Stac LZS (2048-byte window)");
}

bool XNPack::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XNPack::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    if ((nLimit < -1) || (nLimit == 0)) return listResult;
    if (nFileParts == 0) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = NPACK_HEADER_SIZE;
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART stream = {};
        stream.filePart = FILEPART_STREAM;
        stream.nFileOffset = context.nStreamOffset;
        stream.nFileSize = context.nStreamSize;
        stream.nVirtualAddress = XADDR_MAX;
        stream.sName = context.sFileName;
        stream.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        stream.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        // Present only when parseContext() walked the whole block; it is the
        // measured output length, not a stored field, because the container
        // stores none.
        if (context.nUncompressedSize >= 0) {
            stream.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        }
        stream.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NPACK);
        stream.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString());
        listResult.append(stream);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = context.nInputSize;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    // No FILEPART_OVERLAY arm: parseContext() only accepts a block that ends
    // exactly at EOF, so there is never anything after it.
    return listResult;
}

QList<XBinary::FPART_PROP> XNPack::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_STREAMOFFSET,  FPART_PROP_STREAMSIZE};
}

QMap<XBinary::UNPACK_PROP, QVariant> XNPack::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XNPack::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Symantec NPack container ('MSTSM'): one Stac LZS block"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XNPack::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pContext->nUncompressedSize >= 0) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    }
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_NPACK);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString());
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XNPack::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // Advance FIRST, then report.  With a single record the correct behaviour is
    // 0 -> 1 plus a false return true; refusing to advance would make the listing
    // come back empty.
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->nStreamOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nStreamOffset + pContext->nStreamSize;
    return false;
}

bool XNPack::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
