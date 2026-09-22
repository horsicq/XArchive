/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbzip1.h"
#include <new>

namespace {
const qint64 BZIP1_HEADER_SIZE = 4;
// bzip 0.21 never emitted a stream this short: the arithmetic coder alone
// flushes two bytes and every block carries an origPtr and a symbol map.  The
// floor mainly exists so the anti-false-positive probe below always has bytes
// to look at.
const qint64 BZIP1_MIN_FILE_SIZE = 16;
const qint64 BZIP1_PROBE_SIZE = 64;
const quint8 BZIP1_MAGIC_B = 0x42U;        // 'B'
const quint8 BZIP1_MAGIC_Z = 0x5aU;        // 'Z'
const quint8 BZIP1_VERSION_TAG = 0x30U;    // '0'; bzip2 writes 'h' (0x68) here
const quint8 BZIP1_BLOCKSIZE_MIN = 0x31U;  // '1'
const quint8 BZIP1_BLOCKSIZE_MAX = 0x39U;  // '9'
}  // namespace

XBinary::XCONVERT _TABLE_XBZIP1_STRUCTID[] = {
    {XBZIP1::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XBZIP1::STRUCTID_BZIP1_HEADER, "BZIP1_HEADER", QString("BZip1 header")}};

XBZIP1::XBZIP1(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBZIP1::~XBZIP1()
{
}

bool XBZIP1::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BZIP1_MIN_FILE_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, BZIP1_HEADER_SIZE, pPdStruct);
    if (!guardedSource ||
        baHeader.size() != BZIP1_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());

    // Byte 2 is the ONLY field that separates bzip 0.21 from bzip2, and the two
    // classes are mutually exclusive on it: XBZIP2 demands 'h' (0x68) plus the
    // 48-bit pi block magic at offset 4, XBZIP1 demands '0' (0x30).  There is no
    // second magic to anchor on here -- a bit-level search across the two known
    // streams found no shared run above chance, i.e. every field after byte 3 is
    // already inside the arithmetic coder -- so the gate has to be tightened by
    // the range and content checks below instead of by a second signature.
    if ((pHeader[0] != BZIP1_MAGIC_B) || (pHeader[1] != BZIP1_MAGIC_Z) ||
        (pHeader[2] != BZIP1_VERSION_TAG)) {
        return false;
    }
    if ((pHeader[3] < BZIP1_BLOCKSIZE_MIN) ||
        (pHeader[3] > BZIP1_BLOCKSIZE_MAX)) {
        return false;
    }
    context.nBlockSize100k =
        static_cast<quint32>(pHeader[3] - BZIP1_BLOCKSIZE_MIN) + 1;

    // A finished entropy coder leaves no structure behind: both known streams
    // measure ~7.995 bits/byte with all 256 values present.  A run of zeroes at
    // the payload start therefore cannot come from a real bzip 0.21 file, and
    // rejecting it is what stops a 4-byte ASCII header from claiming padded or
    // zero-filled blobs that happen to begin "BZ0" plus a digit.
    const qint64 nProbeSize =
        qMin<qint64>(BZIP1_PROBE_SIZE, context.nInputSize - BZIP1_HEADER_SIZE);
    const QByteArray baProbe =
        read_array_process(BZIP1_HEADER_SIZE, nProbeSize, pPdStruct);
    if (!guardedSource || baProbe.size() != nProbeSize) {
        return false;
    }
    if (baProbe == QByteArray(static_cast<qint32>(nProbeSize), '\0')) {
        return false;
    }

    // The stream is defined to end at EOF and its true end cannot be measured
    // without a decoder, so the whole file is the member and no overlay is ever
    // reported.  Both known samples end in 0x00 0x00, consistent with a 16-bit
    // arithmetic-coder flush, but with two samples in existence that is far too
    // thin to use as a structural terminator.
    context.nStreamOffset = 0;
    context.nStreamSize = context.nInputSize;
    context.sFileName = XBinary::getDeviceFileBaseName(guardedSource);
    if (!guardedSource) return false;

    *pContext = context;
    return isPdStructNotCanceled(pPdStruct);
}

bool XBZIP1::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XBZIP1::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBZIP1 bzip1(pDevice);
    return bzip1.isValid(pPdStruct);
}

XBinary *XBZIP1::createInstance(QIODevice *pDevice, bool bIsImage,
                                XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBZIP1(pDevice);
}

QList<QString> XBZIP1::getSearchSignatures()
{
    // Only the three-byte tag can be expressed here; the '1'..'9' block-size
    // digit and the non-zero payload check live in parseContext(), because a
    // scan signature has no way to say "one of nine bytes".
    return {QStringLiteral("'BZ0'")};
}

XBinary::FT XBZIP1::getFileType()
{
    return FT_BZIP1;
}

XBinary::MODE XBZIP1::getMode()
{
    return MODE_DATA;
}

qint32 XBZIP1::getType()
{
    return TYPE_BZ1;
}

QString XBZIP1::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_BZ1: sResult = QString("BZ1"); break;
    }

    return sResult;
}

XBinary::ENDIAN XBZIP1::getEndian()
{
    // The four header bytes are ASCII; the payload bit stream is MSB-first, but
    // that is a decoder-internal detail and not a container property.
    return ENDIAN_LITTLE;
}

QString XBZIP1::getArch()
{
    return QString();
}

XBinary::OSNAME XBZIP1::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XBZIP1::getFileFormatExt()
{
    // Deliberately "bz", not "bz2": a .bz2 heuristic must never route a bzip2
    // file into this class, whose decoder stage is a different algorithm.
    return QStringLiteral("bz");
}

QString XBZIP1::getFileFormatExtsString()
{
    return QStringLiteral("bzip 0.21 (*.bz)");
}

QString XBZIP1::getMIMEString()
{
    return QStringLiteral("application/x-bzip");
}

QString XBZIP1::getVersion()
{
    // Byte 2 is the format version tag and it is the literal '0' in every
    // bzip 0.21 stream, so report the tag rather than the writer release: the
    // file does not record which 0.2x build produced it.
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QStringLiteral("0");
}

qint64 XBZIP1::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    // Not _calculateRawSize(): measuring means decoding the whole stream, and a
    // format-size query must not do that.  The stream is defined to run to EOF.
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nStreamSize : 0;
}

QList<XBinary::MAPMODE> XBZIP1::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBZIP1::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QString XBZIP1::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(
        nID, _TABLE_XBZIP1_STRUCTID,
        sizeof(_TABLE_XBZIP1_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XBZIP1::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(
        nID, _TABLE_XBZIP1_STRUCTID,
        sizeof(_TABLE_XBZIP1_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XBZIP1::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(
        sFtString, _TABLE_XBZIP1_STRUCTID,
        sizeof(_TABLE_XBZIP1_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XBZIP1::getXFHeaders(const XFSTRUCT &xfStruct,
                                              PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;
    const quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_BZIP1_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_BZIP1_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) headerLoc = offsetToLoc(0);

        const qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);
        if ((nHeaderOffset != -1) &&
            isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset,
                                 sizeof(BZIP1_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID =
                static_cast<XBinary::STRUCTID>(STRUCTID_BZIP1_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(BZIP1_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(
                xfStruct.fileType, STRUCTID_BZIP1_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(
                xfHeader, structIDToString(STRUCTID_BZIP1_HEADER),
                xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XBZIP1::getXFRecords(FT fileType, quint32 nStructID,
                                               const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_BZIP1_HEADER) {
        listResult.append({"magic",
                           static_cast<qint32>(offsetof(BZIP1_HEADER, magic)),
                           3, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append(
            {"blockSize",
             static_cast<qint32>(offsetof(BZIP1_HEADER, blockSize)), 1,
             XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    }

    return listResult;
}

QString XBZIP1::methodToString(quint32 nBlockSize100k)
{
    // The block size is the one thing the header does say, and it is worth
    // showing: it is the only visible difference between two bzip 0.21 streams.
    return tr("bzip 0.21 (BWT + MTF + adaptive arithmetic coding, %1 KB blocks)")
        .arg(nBlockSize100k * 100);
}

bool XBZIP1::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBZIP1::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    if ((nLimit < -1) || (nLimit == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    // Emitted unconditionally from the header.  XBZIP2 measures its stream first
    // and returns an empty list when measuring fails; copying that shape here
    // would make every map mode come back empty, because measuring is exactly
    // what this format cannot do.
    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, listResult.size())) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = BZIP1_HEADER_SIZE;
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        canAppendPart(nLimit, listResult.size())) {
        FPART stream = {};
        stream.filePart = FILEPART_STREAM;
        stream.nFileOffset = context.nStreamOffset;
        stream.nFileSize = context.nStreamSize;
        stream.nVirtualAddress = XADDR_MAX;
        stream.sName = context.sFileName.isEmpty() ? tr("Stream")
                                                   : context.sFileName;
        stream.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                    context.nStreamSize);
        // Still no FPART_PROP_UNCOMPRESSEDSIZE.  There is now a decoder, but the
        // size is only knowable by running it to completion, and mapping a file
        // must not silently decompress the whole stream; callers read this
        // property with a -1 default and already treat "absent" as unknown.
        stream.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                    HANDLE_METHOD_BZIP1);
        stream.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                    methodToString(context.nBlockSize100k));
        listResult.append(stream);
    }

    if ((nFileParts & FILEPART_DATA) &&
        canAppendPart(nLimit, listResult.size())) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = context.nStreamSize;
        data.nVirtualAddress = XADDR_MAX;
        data.sName = tr("Data");
        listResult.append(data);
    }

    // No FILEPART_OVERLAY arm at all.  The stream end is unmeasurable without a
    // decoder, so any overlay this class reported would be a guess.
    return listResult;
}

QList<XBinary::FPART_PROP> XBZIP1::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE,
            FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD,
            FPART_PROP_STREAMOFFSET, FPART_PROP_STREAMSIZE};
}

QMap<XBinary::UNPACK_PROP, QVariant> XBZIP1::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBZIP1::initUnpack(UNPACK_STATE *pState,
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
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    // Header-only: no measure step.  XBZIP2 refuses to init when its stream
    // cannot be measured; the equivalent here would decode the entire payload
    // just to open a listing, and a corrupt tail would then hide the member
    // rather than fail at extraction time, where the CRC report belongs.
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("bzip 0.21 stream ('BZ0'): Burrows-Wheeler + move-to-front + "
           "adaptive arithmetic coding"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
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

XBinary::ARCHIVERECORD XBZIP1::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nStreamSize);
    // FPART_PROP_UNCOMPRESSEDSIZE is deliberately absent: it cannot be known
    // without decoding, and XArchive::unpackCurrent already handles a record
    // with no declared size by staging it in a private temporary file and
    // publishing only after the decoder reports success - which for BZIP1 means
    // after the stream CRC has been verified.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_BZIP1);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(pContext->nBlockSize100k));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XBZIP1::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // Advance FIRST, then report.  With one record the correct behaviour is
    // 0 -> 1 plus a false return; returning false without advancing makes the
    // whole listing come back empty.
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->nStreamOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nStreamOffset + pContext->nStreamSize;
    return false;
}

bool XBZIP1::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
