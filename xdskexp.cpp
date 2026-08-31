/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xdskexp.h"

#include <QPointer>
#include <QtEndian>

#include <memory>
#include <new>

#include "Algos/xlzhdecoder.h"
#include "subdevice.h"

namespace {
const qint64 DSKEXP_HEADER_SIZE = 512;
const qint32 DSKEXP_MAX_TRACKS = 200;
const qint32 DSKEXP_SECTOR_SIZE = 512;

XBinary::XCONVERT g_tableDskExpStructId[] = {
    {XDskExp::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XDskExp::STRUCTID_DSKEXP_HEADER, "DSKEXP_HEADER",
     QStringLiteral("Disk eXPress header")}};

bool dskexpRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

QString dskexpMethodName(quint8 nMethod)
{
    if (nMethod == 1) return QStringLiteral("lh1");
    if (nMethod == 2) return QStringLiteral("lh5");
    return QStringLiteral("store");
}

XBinary::HANDLE_METHOD dskexpHandleMethod(quint8 nMethod)
{
    if (nMethod == 1) return XBinary::HANDLE_METHOD_LZH1;
    if (nMethod == 2) return XBinary::HANDLE_METHOD_LZH5;
    return XBinary::HANDLE_METHOD_STORE;
}
}  // namespace

static_assert(sizeof(XDskExp::DSKEXP_HEADER) == 512,
              "Disk eXPress header layout must remain byte-exact");

XDskExp::XDskExp(QIODevice *pDevice) : XArchive(pDevice)
{
}

XDskExp::~XDskExp()
{
}

QString XDskExp::decodeDescription(const QByteArray &baDescription)
{
    QStringList listLines;
    for (qint32 i = 0; i < 4; ++i) {
        QByteArray baLine = baDescription.mid(i * 50, 50);
        const qint32 nNul = baLine.indexOf('\0');
        if (nNul >= 0) baLine.truncate(nNul);
        QString sLine = QString::fromLatin1(baLine).trimmed().simplified();
        if (!sLine.isEmpty()) listLines.append(sLine);
    }
    return listLines.join(QStringLiteral(" / "));
}

bool XDskExp::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XDskExp> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    const qint64 nFileSize = guardedSource->size();
    if (nFileSize < DSKEXP_HEADER_SIZE + 2) return false;

    CONTEXT context = {};
    context.nInputSize = nFileSize;
    const QByteArray baSignature = read_array_process(0, 2, pPdStruct);
    if (!guardedThis || !guardedSource || baSignature.size() != 2) return false;

    if (baSignature == QByteArrayLiteral("MZ")) {
        if (nFileSize < 64) return false;
        const QByteArray baMZ = read_array_process(0, 6, pPdStruct);
        if (!guardedThis || !guardedSource || baMZ.size() != 6) return false;
        const uchar *pMZ =
            reinterpret_cast<const uchar *>(baMZ.constData());
        const quint16 nLastPageBytes = qFromLittleEndian<quint16>(pMZ + 2);
        const quint16 nPages = qFromLittleEndian<quint16>(pMZ + 4);
        if (!nPages || nLastPageBytes > 511) return false;
        context.nStubEnd = (nLastPageBytes == 0)
                               ? qint64(nPages) * 512
                               : qint64(nPages - 1) * 512 + nLastPageBytes;
        if (context.nStubEnd < 6 || context.nStubEnd > nFileSize - 4) {
            return false;
        }
        context.bExecutable = true;
        context.nHeaderOffset = context.nStubEnd + 4;
    } else if (baSignature == QByteArrayLiteral("AS")) {
        context.bExecutable = false;
        context.nStubEnd = 0;
        context.nHeaderOffset = 0;
    } else {
        return false;
    }

    if (!dskexpRangeWithin(nFileSize, context.nHeaderOffset,
                           DSKEXP_HEADER_SIZE) ||
        context.nHeaderOffset > nFileSize - DSKEXP_HEADER_SIZE - 2) {
        return false;
    }

    const QByteArray baHeader = read_array_process(
        context.nHeaderOffset, DSKEXP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baHeader.size() != DSKEXP_HEADER_SIZE ||
        baHeader.left(2) != QByteArrayLiteral("AS")) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    context.nMajorVersion = pHeader[2];
    context.nMinorVersion = pHeader[3];
    const quint8 nRelease = pHeader[4];
    context.nDiskType = pHeader[5];
    context.nDataCRC = qFromLittleEndian<quint32>(pHeader + 6);
    context.nCompressionMethod = pHeader[10];
    const quint8 nLastCylinder = pHeader[11];
    const quint8 nLastHead = pHeader[12];
    context.nFlags = pHeader[14];

    const bool bVersionValid =
        (context.nMajorVersion == 1 &&
         (context.nMinorVersion == 1 || context.nMinorVersion == 4)) ||
        (context.nMajorVersion == 2 &&
         (context.nMinorVersion == 0 || context.nMinorVersion == 30));
    if (!bVersionValid ||
        (nRelease != 0x20 && nRelease != 'A' && nRelease != 'a') ||
        context.nDiskType < 3 || context.nDiskType > 7 ||
        (context.nCompressionMethod != 0 &&
         context.nCompressionMethod != context.nMajorVersion) ||
        (context.nFlags & 0xfe) != 0) {
        return false;
    }

    switch (context.nDiskType) {
        case 3:
            context.nCylinders = 40;
            context.nSectorsPerTrack = 9;
            break;
        case 4:
            context.nCylinders = 80;
            context.nSectorsPerTrack = 9;
            break;
        case 5:
            context.nCylinders = 80;
            context.nSectorsPerTrack = 15;
            break;
        case 6:
            context.nCylinders = 80;
            context.nSectorsPerTrack = 18;
            break;
        case 7:
            context.nCylinders = 80;
            context.nSectorsPerTrack = 36;
            break;
        default:
            return false;
    }

    context.nTrackSize = context.nSectorsPerTrack * DSKEXP_SECTOR_SIZE;
    context.nNumberOfTracks = qint32(nLastCylinder) * 2 +
                              qMin<qint32>(nLastHead, 1) + 1;
    if (context.nNumberOfTracks < 1 ||
        context.nNumberOfTracks > DSKEXP_MAX_TRACKS ||
        context.nTrackSize <= 0) {
        return false;
    }
    context.nUncompressedSize =
        qint64(context.nNumberOfTracks) * context.nTrackSize;
    context.nDataOffset = context.nHeaderOffset + DSKEXP_HEADER_SIZE;
    context.sDescription =
        decodeDescription(baHeader.mid(0x134, 200));

    if (context.nCompressionMethod == 0) {
        if (!dskexpRangeWithin(nFileSize, context.nDataOffset,
                               context.nUncompressedSize) ||
            context.nDataOffset + context.nUncompressedSize != nFileSize) {
            return false;
        }
        context.nCompressedSize = context.nUncompressedSize;
    } else {
        qint64 nOffset = context.nDataOffset;
        qint64 nCompressedSize = 0;
        context.listTracks.reserve(context.nNumberOfTracks);
        for (qint32 i = 0; i < context.nNumberOfTracks; ++i) {
            if (!guardedThis || !guardedSource ||
                !isPdStructNotCanceled(pPdStruct) || nOffset < 0 ||
                nOffset > nFileSize - 2) {
                return false;
            }
            const QByteArray baSize =
                read_array_process(nOffset, 2, pPdStruct);
            if (!guardedThis || !guardedSource || baSize.size() != 2) {
                return false;
            }
            const qint32 nChunkSize = qFromLittleEndian<quint16>(
                reinterpret_cast<const uchar *>(baSize.constData()));
            nOffset += 2;
            if (nChunkSize <= 0 ||
                !dskexpRangeWithin(nFileSize, nOffset, nChunkSize)) {
                return false;
            }
            TRACK_RECORD record = {};
            record.nOffset = nOffset;
            record.nSize = nChunkSize;
            context.listTracks.append(record);
            nCompressedSize += nChunkSize;
            nOffset += nChunkSize;
        }
        if (nOffset != nFileSize) return false;
        context.nCompressedSize = nCompressedSize;
    }

    if (!guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    *pContext = context;
    return true;
}

bool XDskExp::isValid(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct);
}

bool XDskExp::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XDskExp archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XDskExp::getFileType()
{
    return FT_DSKEXP;
}

XBinary::MODE XDskExp::getMode()
{
    return MODE_DATA;
}

QString XDskExp::getMIMEString()
{
    return QStringLiteral("application/x-disk-express");
}

qint32 XDskExp::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XDskExp::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XDskExp::getArch()
{
    return QString();
}

QString XDskExp::getFileFormatExt()
{
    return QStringLiteral("DXP");
}

QString XDskExp::getFileFormatExtsString()
{
    return QStringLiteral("Disk eXPress image (*.dxp;*.exe)");
}

qint64 XDskExp::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XDskExp::isSigned()
{
    return false;
}

XBinary::OSNAME XDskExp::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XDskExp::getOsVersion()
{
    return QString();
}

QString XDskExp::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QStringLiteral("%1.%2")
        .arg(context.nMajorVersion)
        .arg(context.nMinorVersion, 2, 10, QLatin1Char('0'));
}

bool XDskExp::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XDskExp::getMapModesList()
{
    return {MAPMODE_REGIONS};
}

XBinary::_MEMORY_MAP XDskExp::getMemoryMap(MAPMODE mapMode,
                                            PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)
    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.mode = getMode();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());
    result.sArch = getArch();
    result.nBinarySize = getSize();

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    _MEMORY_RECORD headerRecord = {};
    headerRecord.nAddress = XADDR_MAX;
    headerRecord.nOffset = 0;
    headerRecord.nSize = context.nDataOffset;
    headerRecord.nIndex = 0;
    headerRecord.filePart = FILEPART_HEADER;
    headerRecord.sName = context.bExecutable
                             ? tr("Executable stub and Disk eXPress header")
                             : tr("Disk eXPress header");
    result.listRecords.append(headerRecord);

    _MEMORY_RECORD dataRecord = {};
    dataRecord.nAddress = XADDR_MAX;
    dataRecord.nOffset = context.nDataOffset;
    dataRecord.nSize = context.nInputSize - context.nDataOffset;
    dataRecord.nIndex = 1;
    dataRecord.filePart = FILEPART_REGION;
    dataRecord.sName = tr("Track data");
    result.listRecords.append(dataRecord);
    return result;
}

QList<QString> XDskExp::getSearchSignatures()
{
    // "AS" alone is only two bytes and is far too broad for embedded archive
    // scanning. Top-level detection performs the complete bounded header and
    // chunk-chain validation instead.
    return {};
}

XBinary *XDskExp::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDskExp(pDevice);
}

QString XDskExp::structIDToString(quint32 nID)
{
    return XCONVERT_idToTransString(
        nID, g_tableDskExpStructId,
        sizeof(g_tableDskExpStructId) / sizeof(XCONVERT));
}

QString XDskExp::structIDToFtString(quint32 nID)
{
    return XCONVERT_idToFtString(
        nID, g_tableDskExpStructId,
        sizeof(g_tableDskExpStructId) / sizeof(XCONVERT));
}

quint32 XDskExp::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(
        sFtString, g_tableDskExpStructId,
        sizeof(g_tableDskExpStructId) / sizeof(XCONVERT));
}

QList<XBinary::XFHEADER> XDskExp::getXFHeaders(const XFSTRUCT &xfStruct,
                                                PDSTRUCT *pPdStruct)
{
    QList<XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;
    if (nStructID == STRUCTID_UNKNOWN) {
        CONTEXT context = {};
        if (!parseContext(&context, pPdStruct)) return listResult;
        XFSTRUCT child = xfStruct;
        child.nStructID = STRUCTID_DSKEXP_HEADER;
        child.xLoc = offsetToLoc(context.nHeaderOffset);
        return getXFHeaders(child, pPdStruct);
    }
    if (nStructID != STRUCTID_DSKEXP_HEADER) return listResult;

    XLOC headerLoc = xfStruct.xLoc;
    if (headerLoc.locType == LT_UNKNOWN) {
        CONTEXT context = {};
        if (!parseContext(&context, pPdStruct)) return listResult;
        headerLoc = offsetToLoc(context.nHeaderOffset);
    }
    XFHEADER header = {};
    header.sParentTag = xfStruct.sParent;
    header.fileType = xfStruct.fileType;
    header.structID = static_cast<XBinary::STRUCTID>(
        STRUCTID_DSKEXP_HEADER);
    header.xLoc = headerLoc;
    header.nSize = sizeof(DSKEXP_HEADER);
    header.xfType = XFTYPE_HEADER;
    header.listFields = getXFRecords(
        xfStruct.fileType, STRUCTID_DSKEXP_HEADER, headerLoc);
    header.sTag = xfHeaderToTag(
        header, structIDToString(STRUCTID_DSKEXP_HEADER),
        header.sParentTag);
    listResult.append(header);
    return listResult;
}

QList<XBinary::XFRECORD> XDskExp::getXFRecords(FT fileType,
                                                quint32 nStructID,
                                                const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)
    QList<XFRECORD> listResult;
    if (nStructID != STRUCTID_DSKEXP_HEADER) return listResult;
    listResult.append({"signature", (qint32)offsetof(DSKEXP_HEADER, signature),
                       2, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    listResult.append({"majorVersion", (qint32)offsetof(DSKEXP_HEADER, majorVersion),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"minorVersion", (qint32)offsetof(DSKEXP_HEADER, minorVersion),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"release", (qint32)offsetof(DSKEXP_HEADER, release),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"diskType", (qint32)offsetof(DSKEXP_HEADER, diskType),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"dataCRC", (qint32)offsetof(DSKEXP_HEADER, dataCRC),
                       4, XFRECORD_FLAG_NONE, VT_UINT32});
    listResult.append({"compressionMethod",
                       (qint32)offsetof(DSKEXP_HEADER, compressionMethod),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"lastCylinder",
                       (qint32)offsetof(DSKEXP_HEADER, lastCylinder),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"lastHead", (qint32)offsetof(DSKEXP_HEADER, lastHead),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"flags", (qint32)offsetof(DSKEXP_HEADER, flags),
                       1, XFRECORD_FLAG_NONE, VT_UINT8});
    listResult.append({"headerCRC", (qint32)offsetof(DSKEXP_HEADER, headerCRC),
                       4, XFRECORD_FLAG_NONE, VT_UINT32});
    listResult.append({"description",
                       (qint32)offsetof(DSKEXP_HEADER, description),
                       200, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    listResult.append({"descriptionCRC",
                       (qint32)offsetof(DSKEXP_HEADER, descriptionCRC),
                       4, XFRECORD_FLAG_NONE, VT_UINT32});
    return listResult;
}

bool XDskExp::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit == -1 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XDskExp::getFileParts(quint32 nFileParts,
                                             qint32 nLimit,
                                             PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    if (!nFileParts || nLimit < -1 || nLimit == 0) return listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) &&
        canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDataOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_REGION) &&
        canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nInputSize - context.nDataOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Track data");
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_INFO,
                                  context.sDescription);
        listResult.append(part);
    }
    return listResult;
}

XDskExp::DSKEXP_HEADER XDskExp::readHeader(qint64 nOffset)
{
    DSKEXP_HEADER header = {};
    read_array(nOffset, reinterpret_cast<char *>(&header), sizeof(header));
    return header;
}

QMap<XBinary::UNPACK_PROP, QVariant> XDskExp::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XDskExp::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XDskExp> guardedThis(this);
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

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis ||
        !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    QString sInfo = QStringLiteral(
        "Disk eXPress %1.%2; %3 cylinders, 2 heads, %4 sectors/track; "
        "%5 stored tracks")
        .arg(pContext->nMajorVersion)
        .arg(pContext->nMinorVersion, 2, 10, QLatin1Char('0'))
        .arg(pContext->nCylinders)
        .arg(pContext->nSectorsPerTrack)
        .arg(pContext->nNumberOfTracks);
    if (!pContext->sDescription.isEmpty()) {
        sInfo += QStringLiteral("; %1").arg(pContext->sDescription);
    }
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, sInfo);
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XDskExp::infoCurrent(UNPACK_STATE *pState,
                                             PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        pState->nCurrentIndex != 0 || pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pContext->nUncompressedSize <= 0 ||
        pContext->nCompressedSize <= 0) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD record = {};
    record.nStreamOffset = pContext->nDataOffset;
    record.nStreamSize = pContext->nInputSize - pContext->nDataOffset;
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                QStringLiteral("Image.img"));
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nCompressedSize);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    record.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                dskexpHandleMethod(
                                    pContext->nCompressionMethod));
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                dskexpMethodName(
                                    pContext->nCompressionMethod));
    record.mapProperties.insert(FPART_PROP_RESULTCRC,
                                pContext->nDataCRC);
    if (!markArchiveStreamRecord(&record, 0)) return ARCHIVERECORD();
    return record;
}

bool XDskExp::writeRange(qint64 nOffset, qint64 nSize,
                         DATAPROCESS_STATE *pDirectState,
                         QIODevice *pOutputLifetime,
                         PDSTRUCT *pPdStruct)
{
    QPointer<XDskExp> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pOutputLifetime);
    if (!pDirectState || !guardedSource || !guardedOutput ||
        (nOffset < 0) || (nSize < 0)) {
        return false;
    }

    const qint32 nBufferSize = 0x10000;
    qint64 nDone = 0;
    while ((nDone < nSize) && guardedThis && guardedSource &&
           guardedOutput && isPdStructNotCanceled(pPdStruct)) {
        const qint32 nChunk = qint32(
            qMin<qint64>(nBufferSize, nSize - nDone));
        const QByteArray baData = read_array_process(
            nOffset + nDone, nChunk, pPdStruct);
        if (!guardedThis || !guardedSource || !guardedOutput ||
            (baData.size() != nChunk) ||
            (_writeDevice(baData.constData(), nChunk,
                          pDirectState) != nChunk)) {
            return false;
        }
        nDone += nChunk;
    }
    return nDone == nSize;
}

bool XDskExp::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                            PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QPointer<XDskExp> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!operationGuard.isAcquired() || !pState || !guardedSource ||
        !guardedOutput || pState->nCurrentIndex != 0 ||
        pState->nNumberOfRecords != 1 ||
        !isUnpackOutputSupported(guardedOutput.data()) ||
        devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pContext->nUncompressedSize <= 0 ||
        !isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   pContext->nUncompressedSize)) {
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(
                0, QStringLiteral("Image.img"))) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(
        createFileBuffer(pContext->nUncompressedSize, pPdStruct));
    if (!pStage || !guardedThis || !guardedSource || !guardedOutput) {
        return false;
    }

    DATAPROCESS_STATE directState = {};
    directState.mapUnpackProperties = pState->mapUnpackProperties;
    directState.spOutputBudget = pState->spOutputBudget;
    directState.pDeviceInput = guardedSource.data();
    directState.pDeviceOutput = pStage.get();
    directState.nInputOffset = 0;
    directState.nInputLimit = -1;
    directState.nProcessedOffset = 0;
    directState.nProcessedLimit = pContext->nUncompressedSize;

    bool bResult = true;
    if (pContext->nCompressionMethod == 0) {
        bResult = writeRange(pContext->nDataOffset,
                             pContext->nUncompressedSize, &directState,
                             guardedOutput.data(), pPdStruct);
    } else {
        if (pContext->listTracks.size() !=
            pContext->nNumberOfTracks) {
            bResult = false;
        }
        for (qint32 i = 0; bResult &&
                            i < pContext->listTracks.size(); ++i) {
            const TRACK_RECORD &track = pContext->listTracks.at(i);
            const qint64 nOutputOffset =
                qint64(i) * pContext->nTrackSize;
            if (!guardedThis || !guardedSource || !guardedOutput ||
                !isPdStructNotCanceled(pPdStruct) ||
                !pStage->seek(nOutputOffset)) {
                bResult = false;
                break;
            }
            if (track.nSize == 1) {
                const QByteArray baValue = read_array_process(
                    track.nOffset, 1, pPdStruct);
                if (!guardedThis || !guardedSource || !guardedOutput ||
                    baValue.size() != 1) {
                    bResult = false;
                    break;
                }
                const QByteArray baTrack(
                    pContext->nTrackSize, baValue.at(0));
                bResult =
                    _writeDevice(baTrack.constData(), baTrack.size(),
                                 &directState) == baTrack.size();
            } else if (track.nSize == pContext->nTrackSize) {
                bResult = writeRange(track.nOffset, track.nSize,
                                     &directState, guardedOutput.data(),
                                     pPdStruct);
            } else {
                SubDevice subDevice(guardedSource.data(), track.nOffset,
                                    track.nSize);
                SubDevice outputDevice(pStage.get(), nOutputOffset,
                                       pContext->nTrackSize);
                if (!subDevice.open(QIODevice::ReadOnly) ||
                    !outputDevice.open(QIODevice::ReadWrite)) {
                    subDevice.close();
                    outputDevice.close();
                    bResult = false;
                    break;
                }
                DATAPROCESS_STATE decodeState = {};
                decodeState.mapProperties.insert(
                    FPART_PROP_HANDLEMETHOD,
                    dskexpHandleMethod(
                        pContext->nCompressionMethod));
                decodeState.mapProperties.insert(
                    FPART_PROP_UNCOMPRESSEDSIZE,
                    pContext->nTrackSize);
                decodeState.mapUnpackProperties =
                    pState->mapUnpackProperties;
                decodeState.spOutputBudget = pState->spOutputBudget;
                decodeState.pDeviceInput = &subDevice;
                // Every track is a fresh stream. A bounded output view lets
                // the decoder seek to its required logical offset zero
                // without overwriting previously decoded tracks.
                decodeState.pDeviceOutput = &outputDevice;
                decodeState.nInputOffset = 0;
                decodeState.nInputLimit = track.nSize;
                decodeState.nProcessedOffset = 0;
                decodeState.nProcessedLimit = pContext->nTrackSize;
                bResult = XLZHDecoder::decompress(
                              &decodeState,
                              pContext->nCompressionMethod == 1 ? 1 : 5,
                              pPdStruct) &&
                          decodeState.nCountInput == track.nSize &&
                          decodeState.nCountOutput ==
                              pContext->nTrackSize &&
                          outputDevice.pos() ==
                              pContext->nTrackSize;
                subDevice.close();
                outputDevice.close();
            }
        }
    }

    if (!bResult || !guardedThis || !guardedSource || !guardedOutput ||
        pStage->pos() != pContext->nUncompressedSize ||
        !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }

    // Versions 2.x checksum the emitted stored-track region. Versions 1.x
    // checksum compressed track payloads instead, so their output is not
    // validated with this calculation.
    if (pContext->nMajorVersion >= 2 ||
        pContext->nCompressionMethod == 0) {
        const quint32 nCRC1 = _getCRC32(
            pStage.get(), 0x0000059d,
            _getCRC32Table_EDB88320(), pPdStruct);
        if (!guardedThis || !guardedSource || !guardedOutput) return false;
        quint32 nCRC2 = nCRC1;
        if (nCRC1 != pContext->nDataCRC) {
            nCRC2 = _getCRC32(
                pStage.get(), 0x0000031e,
                _getCRC32Table_EDB88320(), pPdStruct);
        }
        if (nCRC1 != pContext->nDataCRC &&
            nCRC2 != pContext->nDataCRC) {
            setPdStructErrorString(
                pPdStruct, tr("Disk eXPress data CRC mismatch"));
            return false;
        }
    }

    if (!guardedThis || !guardedSource || !guardedOutput ||
        !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }
    const bool bPublished = publishUnpackOutput(
        pStage.get(), guardedOutput.data(), pState, pPdStruct);
    if (bPublished && guardedThis) {
        pState->nCurrentOffset = pState->nTotalSize;
    }
    return bPublished && guardedThis;
}

bool XDskExp::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        pState->nCurrentIndex != 0 || pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }
    pState->nCurrentIndex = 1;
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XDskExp::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XDskExp::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XDskExp> guardedThis(this);
    bool bResult = true;
    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) = *pInfo;
    }
    return guardedThis && bResult;
}

void *XDskExp::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XDskExp> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &m_internalInfo;
}

void XDskExp::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(
            static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
