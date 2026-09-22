/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xepsfsfx.h"

#include <QtEndian>

#include <new>

#include "subdevice.h"
#include "xarcv4.h"
#include "xpe.h"

namespace {
const qint64 EPSF_HEADER_SIZE = 18;
const quint16 EPSF_VERSION = 3U;
// SETUPMN.DLL is 512-528 KiB on every known build.  The ceiling only exists so
// a corrupt header cannot ask XDecompress for an unbounded allocation; it is
// deliberately far above anything the tool ever shipped.
const qint64 EPSF_MAX_RUNTIME_SIZE = 0x4000000;
// "ARCV" plus the 0x0400 version word.  The tag alone also matches ARCV 1.10
// and ARCV 2.00, neither of which can follow an EPSF header.
const char EPSF_ARCV4_TAG[] = {'A', 'R', 'C', 'V', '\x00', '\x04'};
const qint64 EPSF_ARCV4_TAG_SIZE = 6;
// The setup script sits between the runtime and the archive and is a few
// kilobytes on every carrier.  A tag that turns up inside its compressed bytes
// is rejected by XARCV4's own validation, so the only cost of one is a retry;
// the cap keeps a hostile file from turning that into a long scan.
const qint32 EPSF_MAX_ARCHIVE_CANDIDATES = 16;
const qint32 EPSF_MAX_RECORDS = 100000;
}  // namespace

XEPSFSFX::XEPSFSFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XEPSFSFX::~XEPSFSFX()
{
}

// The header is only ever accepted AT the PE overlay offset.  Nothing else
// separates these eighteen bytes from an incidental "EPSF" inside an unrelated
// executable's data, and over the 73,838-file reference corpus this predicate
// fires on the 17 Eschalon carriers and on nothing else.
bool XEPSFSFX::readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pHeader || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < EPSF_HEADER_SIZE) return false;

    XPE pe(getDevice());
    if (!pe.isValid(pPdStruct) || !guardedSource) return false;
    const qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if (!guardedSource) return false;
    if ((nOverlayOffset <= 0) || (nOverlayOffset + EPSF_HEADER_SIZE > nInputSize)) return false;

    const QByteArray baHeader = read_array_process(nOverlayOffset, EPSF_HEADER_SIZE, pPdStruct);
    if (!guardedSource || (baHeader.size() != EPSF_HEADER_SIZE)) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baHeader.constData());
    if (memcmp(pData, "EPSF", 4) != 0) return false;
    if (qFromLittleEndian<quint16>(pData + 4) != EPSF_VERSION) return false;

    // The two sizes are read UNALIGNED: the version word is two bytes wide and
    // the writer packs the header with no padding at all.
    const quint32 nRuntimeSize = qFromLittleEndian<quint32>(pData + 6);
    const quint32 nRuntimePackedSize = qFromLittleEndian<quint32>(pData + 10);
    if ((nRuntimeSize == 0) || (nRuntimePackedSize == 0)) return false;
    if (static_cast<qint64>(nRuntimeSize) > EPSF_MAX_RUNTIME_SIZE) return false;
    if (static_cast<qint64>(nRuntimePackedSize) > nInputSize - (nOverlayOffset + EPSF_HEADER_SIZE)) return false;

    HEADER header = {};
    header.nHeaderOffset = nOverlayOffset;
    header.nVersion = EPSF_VERSION;
    header.nRuntimeSize = static_cast<qint64>(nRuntimeSize);
    header.nRuntimePackedSize = static_cast<qint64>(nRuntimePackedSize);
    header.nRuntimeCheckSum = qFromLittleEndian<quint32>(pData + 14);

    *pHeader = header;
    return true;
}

// The payload archive is a complete FT_ARCV4 container, so its records are
// taken FROM that reader rather than re-derived: a SubDevice is pinned at the
// candidate offset, XARCV4 walks it, and each record comes back with its
// stream offset moved into carrier coordinates.  A record carrying coordinates
// this shift cannot express is refused outright instead of being republished
// with an offset that means something else.
bool XEPSFSFX::collectArchiveRecords(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext || !guardedSource) return false;

    qint64 nSearchOffset = pContext->nScriptOffset;
    qint32 nAttempt = 0;

    while ((nAttempt < EPSF_MAX_ARCHIVE_CANDIDATES) && (nSearchOffset + EPSF_ARCV4_TAG_SIZE <= pContext->nInputSize) && isPdStructNotCanceled(pPdStruct)) {
        const qint64 nFound = find_array(nSearchOffset, pContext->nInputSize - nSearchOffset, EPSF_ARCV4_TAG, EPSF_ARCV4_TAG_SIZE, pPdStruct);
        if (!guardedSource) return false;
        if (nFound < 0) return false;

        nAttempt++;
        nSearchOffset = nFound + 1;
        pContext->bArchiveRejected = true;

        SubDevice subDevice(guardedSource, nFound, pContext->nInputSize - nFound);
        if (!subDevice.open(QIODevice::ReadOnly)) continue;

        XARCV4 archive(&subDevice);
        UNPACK_STATE innerState = UNPACK_STATE();
        if (!archive.initUnpack(&innerState, archive.getDefaultUnpackProperties(), pPdStruct)) {
            subDevice.close();
            continue;
        }

        const qint64 nArchiveSize = innerState.nTotalSize;
        QList<ARCHIVERECORD> listRecords;
        bool bOK = (nArchiveSize > 0) && (nArchiveSize <= pContext->nInputSize - nFound);

        while (bOK && isPdStructNotCanceled(pPdStruct)) {
            ARCHIVERECORD record = archive.infoCurrent(&innerState, pPdStruct);
            if (!record.mapProperties.contains(FPART_PROP_ORIGINALNAME)) {
                bOK = false;
                break;
            }
            // Only a flat "offset plus size inside this container" record can
            // be moved.  Anything addressing a second stream, a substream or a
            // solid block would keep pointing at the SubDevice's coordinates.
            if (record.mapProperties.contains(FPART_PROP_STREAMOFFSET2) || record.mapProperties.contains(FPART_PROP_STREAMOFFSET3) ||
                record.mapProperties.contains(FPART_PROP_STREAMOFFSET4) || record.mapProperties.contains(FPART_PROP_SUBSTREAMOFFSET) ||
                record.mapProperties.contains(FPART_PROP_SOLIDFOLDERINDEX) || record.mapProperties.value(FPART_PROP_ISSOLID).toBool()) {
                bOK = false;
                break;
            }
            if ((record.nStreamOffset < 0) || (record.nStreamSize < 0) || (record.nStreamOffset > nArchiveSize) ||
                (record.nStreamSize > nArchiveSize - record.nStreamOffset)) {
                bOK = false;
                break;
            }

            record.nStreamOffset += nFound;
            if (record.mapProperties.contains(FPART_PROP_STREAMOFFSET)) {
                record.mapProperties.insert(FPART_PROP_STREAMOFFSET, record.nStreamOffset);
            }
            listRecords.append(record);

            if (listRecords.size() > EPSF_MAX_RECORDS) {
                bOK = false;
                break;
            }
            if (!archive.moveToNext(&innerState, pPdStruct)) break;
        }

        archive.finishUnpack(&innerState, pPdStruct);
        subDevice.close();
        if (!guardedSource) return false;

        if (bOK && !listRecords.isEmpty()) {
            pContext->nArchiveOffset = nFound;
            pContext->nArchiveSize = nArchiveSize;
            pContext->nScriptSize = nFound - pContext->nScriptOffset;
            pContext->listRecords = listRecords;
            pContext->bArchiveRejected = false;
            return true;
        }
    }

    return false;
}

bool XEPSFSFX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nArchiveOffset = -1;
    if (!readHeader(&context.header, pPdStruct) || !guardedSource) return false;

    context.nRuntimeOffset = context.header.nHeaderOffset + EPSF_HEADER_SIZE;
    context.nScriptOffset = context.nRuntimeOffset + context.header.nRuntimePackedSize;
    if (context.nScriptOffset > context.nInputSize) return false;
    // With no payload archive the script simply runs to end of file; that is
    // what the runtime-only carriers look like and they are not an error.
    context.nScriptSize = context.nInputSize - context.nScriptOffset;

    collectArchiveRecords(&context, pPdStruct);
    if (!guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    context.nTotalSize = (context.nArchiveOffset >= 0) ? (context.nArchiveOffset + context.nArchiveSize) : context.nInputSize;

    *pContext = context;
    return true;
}

XBinary::ARCHIVERECORD XEPSFSFX::runtimeRecord(const CONTEXT &context)
{
    ARCHIVERECORD result = {};
    result.nStreamOffset = context.nRuntimeOffset;
    result.nStreamSize = context.header.nRuntimePackedSize;
    // The container stores no name for this member.  SETUPMN.DLL is the name
    // the runtime carries in its own VS_VERSIONINFO OriginalFilename, which is
    // the name it is installed under.
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, QStringLiteral("SETUPMN.DLL"));
    result.mapProperties.insert(FPART_PROP_STREAMOFFSET, context.nRuntimeOffset);
    result.mapProperties.insert(FPART_PROP_STREAMSIZE, context.header.nRuntimePackedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.header.nRuntimePackedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.header.nRuntimeSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ARCV4_M2);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("ARCV4 2 Adaptive Huffman + LZ77"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No FPART_PROP_RESULTCRC: the header's check value is a 32-bit sum of the
    // unpacked bytes and no CRC_TYPE denotes that function.  Publishing it
    // under a CRC identity would make XDecompress verify the wrong thing and
    // fail every member.
    return result;
}

XBinary::ARCHIVERECORD XEPSFSFX::recordAt(const CONTEXT &context, qint32 nIndex)
{
    if (nIndex == 0) return runtimeRecord(context);
    if ((nIndex < 0) || (nIndex - 1 >= context.listRecords.size())) return ARCHIVERECORD();
    return context.listRecords.at(nIndex - 1);
}

qint64 XEPSFSFX::recordOffset(const CONTEXT &context, qint32 nIndex)
{
    if (nIndex == 0) return context.nRuntimeOffset;
    if ((nIndex < 0) || (nIndex - 1 >= context.listRecords.size())) return -1;
    return context.listRecords.at(nIndex - 1).nStreamOffset;
}

qint32 XEPSFSFX::recordCount(const CONTEXT &context)
{
    return context.listRecords.size() + 1;
}

bool XEPSFSFX::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    HEADER header = {};
    const bool bResult = readHeader(&header, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XEPSFSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XEPSFSFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XEPSFSFX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XEPSFSFX(pDevice);
}

QList<QString> XEPSFSFX::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'EPSF'0300"));
    return listResult;
}

XBinary::FT XEPSFSFX::getFileType()
{
    return FT_EPSF_SFX;
}

XBinary::MODE XEPSFSFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XEPSFSFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XEPSFSFX::getArch()
{
    return QString();
}

QString XEPSFSFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XEPSFSFX::getFileFormatExtsString()
{
    return QStringLiteral("Eschalon Setup EPSF SFX (*.exe)");
}

QString XEPSFSFX::getMIMEString()
{
    return QStringLiteral("application/x-eschalon-setup");
}

QString XEPSFSFX::getVersion()
{
    return QStringLiteral("3");
}

qint64 XEPSFSFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nTotalSize : 0;
}

QList<XBinary::MAPMODE> XEPSFSFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XEPSFSFX::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XEPSFSFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XEPSFSFX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nRuntimeOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if (nFileParts & FILEPART_STREAM) {
        const qint32 nCount = recordCount(context);
        for (qint32 i = 0; i < nCount; i++) {
            if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
            const ARCHIVERECORD record = recordAt(context, i);
            if (!record.mapProperties.contains(FPART_PROP_ORIGINALNAME)) continue;
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = record.nStreamOffset;
            part.nFileSize = record.nStreamSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
            part.mapProperties = record.mapProperties;
            listResult.append(part);
        }
    }

    // The setup script is a second stream in the same codec whose unpacked
    // size nothing records, so it can be located but not decoded.  It is
    // published as a region so the map has no hole, never as a member.
    if ((nFileParts & FILEPART_REGION) && (context.nScriptSize > 0) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nScriptOffset;
        part.nFileSize = context.nScriptSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Setup script");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nTotalSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) && (context.nTotalSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nTotalSize;
        part.nFileSize = context.nInputSize - context.nTotalSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XEPSFSFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XEPSFSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    if (pContext->nArchiveOffset >= 0) {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO,
                                            tr("Eschalon Setup EPSF self-extractor; the installer runtime is followed by an ARCV 4.00 payload archive"));
    } else if (pContext->bArchiveRejected) {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Eschalon Setup EPSF self-extractor; the payload archive could not be read, only the "
                                                               "installer runtime is available"));
    } else {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Eschalon Setup EPSF self-extractor; this carrier ships the installer runtime only"));
    }
    pState->nCurrentOffset = recordOffset(*pContext, 0);
    pState->nTotalSize = pContext->nTotalSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = recordCount(*pContext);
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XEPSFSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= recordCount(*pContext))) return ARCHIVERECORD();

    const qint32 nIndex = pState->nCurrentIndex;
    if (pState->nCurrentOffset != recordOffset(*pContext, nIndex)) return ARCHIVERECORD();

    return recordAt(*pContext, nIndex);
}

bool XEPSFSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= recordCount(*pContext))) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < recordCount(*pContext)) {
        pState->nCurrentOffset = recordOffset(*pContext, pState->nCurrentIndex);
        return true;
    }
    pState->nCurrentOffset = pContext->nTotalSize;
    return false;
}

bool XEPSFSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
