/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xxz.h"

#include <limits>
#include <memory>
#include <new>
#include <QTemporaryFile>

XBinary::XCONVERT _TABLE_XXZ_STRUCTID[] = {{XXZ::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                           {XXZ::STRUCTID_STREAM_HEADER, "STREAM_HEADER", QString("Stream Header")},
                                           {XXZ::STRUCTID_BLOCK_HEADER, "BLOCK_HEADER", QString("Block Header")},
                                           {XXZ::STRUCTID_INDEX, "INDEX", QString("Index")},
                                           {XXZ::STRUCTID_STREAM_FOOTER, "STREAM_FOOTER", QString("Stream Footer")},
                                           {XXZ::STRUCTID_RECORD, "RECORD", QString("Record")}};

static bool xzIsSupportedFlags(const char *pFlags)
{
    if (!pFlags || ((quint8)pFlags[0] != 0) || (((quint8)pFlags[1] & 0xF0) != 0)) return false;
    const quint8 nCheckType = (quint8)pFlags[1] & 0x0F;
    return (nCheckType == 0) || (nCheckType == 1) || (nCheckType == 4) || (nCheckType == 10);
}

static quint32 xzReadLE32(const char *pData)
{
    return (quint32)(quint8)pData[0] | ((quint32)(quint8)pData[1] << 8) |
           ((quint32)(quint8)pData[2] << 16) | ((quint32)(quint8)pData[3] << 24);
}

static quint32 xzCrc32(const char *pData, qint32 nDataSize)
{
    return XBinary::_getCRC32(pData, nDataSize, 0xFFFFFFFF,
                              XBinary::_getCRC32Table_EDB88320()) ^ 0xFFFFFFFF;
}

XXZ::XXZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XXZ::~XXZ()
{
}

bool XXZ::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    // The smallest XZ Stream is a 12-byte Stream Header, an 8-byte empty
    // Index, and a 12-byte Stream Footer. Streams and Stream Padding are
    // always four-byte aligned; a magic-only prefix is not an XZ container.
    const qint64 nSize = getSize();
    if ((nSize < 32) || (nSize & 3)) return false;

    const QByteArray baHeader = read_array_process(0, 12, pPdStruct);
    static const quint8 XZ_MAGIC[6] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
    if ((baHeader.size() != 12) ||
        (memcmp(baHeader.constData(), XZ_MAGIC, sizeof(XZ_MAGIC)) != 0)) {
        return false;
    }

    if (!xzIsSupportedFlags(baHeader.constData() + 6) ||
        (xzCrc32(baHeader.constData() + 6, 2) != xzReadLE32(baHeader.constData() + 8))) {
        return false;
    }

    // Locate the last Footer while accepting only complete zero Padding
    // groups. Full Block/Index authentication remains the decoder's job.
    qint64 nStreamEnd = nSize;
    while (nStreamEnd >= 4) {
        const QByteArray baGroup = read_array_process(nStreamEnd - 4, 4, pPdStruct);
        if (baGroup.size() != 4) return false;
        if ((baGroup.at(0) != 0) || (baGroup.at(1) != 0) ||
            (baGroup.at(2) != 0) || (baGroup.at(3) != 0)) {
            break;
        }
        nStreamEnd -= 4;
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    }
    if (nStreamEnd < 32) return false;

    const qint64 nFooterOffset = nStreamEnd - 12;
    const QByteArray baFooter = read_array_process(nFooterOffset, 12, pPdStruct);
    if ((baFooter.size() != 12) || (baFooter.at(10) != 'Y') || (baFooter.at(11) != 'Z') ||
        !xzIsSupportedFlags(baFooter.constData() + 8) ||
        (xzCrc32(baFooter.constData() + 4, 6) != xzReadLE32(baFooter.constData()))) {
        return false;
    }

    const quint64 nIndexSize = ((quint64)xzReadLE32(baFooter.constData() + 4) + 1) * 4;
    if ((nIndexSize < 8) || (nIndexSize > (quint64)(nFooterOffset - 12))) return false;

    const QByteArray baIndexIndicator = read_array_process(nFooterOffset - (qint64)nIndexSize, 1, pPdStruct);
    if ((baIndexIndicator.size() != 1) || (baIndexIndicator.at(0) != 0)) return false;

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XXZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice || pDevice->isSequential()) return false;

    const qint64 nCurrentPos = pDevice->pos();
    if (nCurrentPos < 0) return false;

    XXZ xz(pDevice);
    const bool bResult = xz.isValid(pPdStruct);
    if (!pDevice->seek(nCurrentPos)) return false;
    return bResult;
}

XBinary::FT XXZ::getFileType()
{
    return XBinary::FT_XZ;  // Replace with FT_XZ if defined
}

XBinary::MODE XXZ::getMode()
{
    return XBinary::MODE_DATA;
}

QString XXZ::getMIMEString()
{
    return "application/x-xz";
}

qint32 XXZ::getType()
{
    return TYPE_ARCHIVE;
}

QString XXZ::typeIdToString(qint32 nType)
{
    if (nType == TYPE_ARCHIVE) return "Archive";
    return QString::number(nType);
}

XBinary::ENDIAN XXZ::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XXZ::getArch()
{
    return QString();
}

QString XXZ::getFileFormatExt()
{
    return "xz";
}

QString XXZ::getFileFormatExtsString()
{
    return "xz";
}

qint64 XXZ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return getSize();
}

bool XXZ::isSigned()
{
    return false;
}

XBinary::OSNAME XXZ::getOsName()
{
    return XBinary::OSNAME_UNKNOWN;
}

QString XXZ::getOsVersion()
{
    return QString();
}

QString XXZ::getVersion()
{
    return QString();
}

bool XXZ::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XXZ::getMapModesList()
{
    QList<MAPMODE> list;
    list.append(MAPMODE_REGIONS);
    return list;
}

XBinary::_MEMORY_MAP XXZ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.nBinarySize = getSize();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());

    qint32 nIndex = 0;

    const qint64 nSize = getSize();

    if (nSize >= 12) {
        _MEMORY_RECORD recordHeader = {};
        recordHeader.nAddress = XADDR_MAX;
        recordHeader.nOffset = 0;
        recordHeader.nSize = 12;
        recordHeader.nIndex = nIndex++;
        recordHeader.filePart = FILEPART_HEADER;
        recordHeader.sName = tr("Stream Header");
        result.listRecords.append(recordHeader);
    }

    // A valid XZ file may end in complete four-byte Stream Padding groups.
    // Never publish a negative footer offset for a truncated input.
    qint64 nStreamEnd = nSize;
    while ((nStreamEnd >= 4) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const QByteArray baGroup = read_array_process(nStreamEnd - 4, 4, pPdStruct);
        if ((baGroup.size() != 4) || (baGroup != QByteArray(4, 0))) break;
        nStreamEnd -= 4;
    }
    if (nStreamEnd >= 12) {
        const QByteArray baFooter = read_array_process(nStreamEnd - 12, 12, pPdStruct);
        if ((baFooter.size() == 12) && (baFooter.at(10) == 'Y') && (baFooter.at(11) == 'Z')) {
            _MEMORY_RECORD recordFooter = {};
            recordFooter.nAddress = XADDR_MAX;
            recordFooter.nOffset = nStreamEnd - 12;
            recordFooter.nSize = 12;
            recordFooter.nIndex = nIndex++;
            recordFooter.filePart = FILEPART_FOOTER;
            recordFooter.sName = tr("Stream Footer");
            result.listRecords.append(recordFooter);
        }
    }

    // TODO: Parse and add block and index records

    // Overlay (if any)
    qint64 nMaxOffset = getSize();
    if (!result.listRecords.isEmpty()) {
        qint64 nMaxRecordOffset = 0;
        qint32 nNumberOfRecords = result.listRecords.size();
        for (qint32 i = 0; i < nNumberOfRecords; i++) {
            qint64 nEnd = result.listRecords.at(i).nOffset + result.listRecords.at(i).nSize;
            if (nEnd > nMaxRecordOffset) {
                nMaxRecordOffset = nEnd;
            }
        }
        if (nMaxRecordOffset < nMaxOffset) {
            _MEMORY_RECORD recordOverlay = {};
            recordOverlay.nAddress = XADDR_MAX;
            recordOverlay.nOffset = nMaxRecordOffset;
            recordOverlay.nSize = nMaxOffset - nMaxRecordOffset;
            recordOverlay.nIndex = nIndex++;
            recordOverlay.filePart = FILEPART_OVERLAY;
            recordOverlay.sName = tr("Overlay");
            result.listRecords.append(recordOverlay);
        }
    }

    return result;
}

QString XXZ::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XXZ_STRUCTID, sizeof(_TABLE_XXZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XXZ::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XXZ_STRUCTID, sizeof(_TABLE_XXZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XXZ::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XXZ_STRUCTID, sizeof(_TABLE_XXZ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

// QList<XBinary::DATA_HEADER> XXZ::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;

//         _dataHeadersOptions.nID = STRUCTID_STREAM_HEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             DATA_HEADER dataHeader = {};
//             dataHeader.dsID_parent = dataHeadersOptions.dsID_parent;
//             dataHeader.dsID.sGUID = generateUUID();
//             dataHeader.dsID.fileType = dataHeadersOptions.pMemoryMap->fileType;
//             dataHeader.dsID.nID = dataHeadersOptions.nID;
//             dataHeader.locType = dataHeadersOptions.locType;
//             dataHeader.nLocation = dataHeadersOptions.nLocation;
//             dataHeader.sName = structIDToString(dataHeadersOptions.nID);
//             dataHeader.dhMode = dataHeadersOptions.dhMode;

//             if (dataHeadersOptions.nID == STRUCTID_STREAM_HEADER) {
//                 dataHeader.nSize = 12;
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(STREAM_HEADER, header_magic), 6, "header_magic", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(STREAM_HEADER, stream_flags), 2, "stream_flags", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(STREAM_HEADER, crc32), 4, "crc32", VT_UINT32, DRF_UNKNOWN,
//                 dataHeadersOptions.pMemoryMap->endian));
//             } else if (dataHeadersOptions.nID == STRUCTID_STREAM_FOOTER) {
//                 dataHeader.nSize = 12;
//                 dataHeader.listRecords.append(getDataRecord(offsetof(STREAM_FOOTER, crc32), 4, "crc32", VT_UINT32, DRF_UNKNOWN,
//                 dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(
//                     getDataRecord(offsetof(STREAM_FOOTER, backward_size), 4, "backward_size", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(STREAM_FOOTER, stream_flags), 2, "stream_flags", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(STREAM_FOOTER, footer_magic), 2, "footer_magic", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//             }
//             // TODO: Block header, Index, etc.
//             listResult.append(dataHeader);
//         }
//     }

//     return listResult;
// }

QList<XBinary::XFHEADER> XXZ::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XBinary::XFHEADER> listResult;
    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_STREAM_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_STREAM_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        qint64 nHeaderOffset = locToOffset(xfStruct.pMemoryMap, headerLoc);

        if ((nHeaderOffset != -1) && isOffsetAndSizeValid(xfStruct.pMemoryMap, nHeaderOffset, sizeof(STREAM_HEADER))) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_STREAM_HEADER);
            xfHeader.xLoc = headerLoc;
            xfHeader.nSize = sizeof(STREAM_HEADER);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_STREAM_HEADER, headerLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_STREAM_HEADER), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XXZ::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_STREAM_HEADER) {
        listResult.append({"header_magic", (qint32)offsetof(STREAM_HEADER, header_magic), 6, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"stream_flags", (qint32)offsetof(STREAM_HEADER, stream_flags), 2, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"crc32", (qint32)offsetof(STREAM_HEADER, crc32), 4, XFRECORD_FLAG_NONE, VT_UINT32});
    }

    return listResult;
}

XXZ::STREAM_HEADER XXZ::_read_STREAM_HEADER(qint64 nOffset)
{
    STREAM_HEADER sh = {};
    QByteArray arr = read_array(nOffset, 12);
    if (arr.size() == 12) {
        memcpy(sh.header_magic, arr.constData(), 6);
        memcpy(sh.stream_flags, arr.constData() + 6, 2);
        sh.crc32 = _read_uint32((char *)(arr.constData() + 8), false);
    }
    return sh;
}

XXZ::STREAM_FOOTER XXZ::_read_STREAM_FOOTER(qint64 nOffset)
{
    STREAM_FOOTER sf = {};
    QByteArray arr = read_array(nOffset, 12);
    if (arr.size() == 12) {
        sf.crc32 = _read_uint32((char *)(arr.constData()), false);
        sf.backward_size = _read_uint32((char *)(arr.constData() + 4), false);
        memcpy(sf.stream_flags, arr.constData() + 8, 2);
        memcpy(sf.footer_magic, arr.constData() + 10, 2);
    }
    return sf;
}

XXZ::BLOCK_HEADER XXZ::_read_BLOCK_HEADER(qint64 nOffset)
{
    BLOCK_HEADER bh = {};
    QByteArray arr = read_array(nOffset, 2);
    if (arr.size() >= 2) {
        bh.header_size = (quint8)arr[0];
        bh.flags = (quint8)arr[1];
        // TODO: parse rest of block header as needed
    }
    return bh;
}

XXZ::INDEX XXZ::_read_INDEX(qint64 nOffset)
{
    INDEX idx = {};
    QByteArray arr = read_array(nOffset, 2);  // At least indicator and start of num_records
    if (arr.size() >= 2) {
        idx.indicator = (quint8)arr[0];
        // TODO: parse variable-length num_records
    }
    return idx;
}

quint64 XXZ::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    // XZ is not a multi-file archive, only one record (the whole decompressed stream)
    return isValid(pPdStruct) ? 1 : 0;
}

QList<XArchive::RECORD> XXZ::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> list;
    if ((nLimit < -1) || (nLimit == 0)) return list;

    if (isValid(pPdStruct)) {
        RECORD record = {};
        record.spInfo.sRecordName = "stream";
        record.spInfo.compressMethod = HANDLE_METHOD_XZ;
        record.spInfo.nUncompressedSize = -1;  // Determined from the validated Index(es) while decoding.
        record.nDataOffset = 0;
        record.nDataSize = getSize();
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, record.spInfo.sRecordName);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XZ);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, record.nDataSize);
        list.append(record);
    }
    return list;
}

QMap<XBinary::UNPACK_PROP, QVariant> XXZ::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XXZ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XXZ> guardedArchive(this);
    if (!pState || m_bUnpackOperationInProgress ||
        ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState))) return false;
    if (!guardedArchive->finishUnpack(pState, nullptr) || !guardedArchive) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = guardedArchive->bindUnpackSource(pState, pPdStruct);
    if (!guardedArchive || !bBound) return false;
    const bool bValid = guardedArchive->isValid(pPdStruct);
    if (!guardedArchive) return false;
    if (!bValid) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    XXZ_UNPACK_CONTEXT *pContext = new (std::nothrow) XXZ_UNPACK_CONTEXT;
    if (!pContext) {
        guardedArchive->releaseUnpackSource(pState);
        return false;
    }

    pContext->nHeaderSize = 0;
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedArchive || !guardedSource) {
        if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        return false;
    }
    pContext->sFileName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedArchive || !guardedSource) {
        if (guardedArchive) guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        return false;
    }
    pContext->nCompressedSize = guardedArchive->getSize();
    if (!guardedArchive) {
        delete pContext;
        return false;
    }
    pContext->nUncompressedSize = -1;
    pContext->nCRC32 = 0;

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nCompressedSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;
    if (!guardedArchive->validateAndFinalizeUnpackSource(
            pState, pContext, pPdStruct)) {
        if (!guardedArchive) return false;
        pState->pContext = nullptr;
        guardedArchive->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XXZ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();
    QPointer<XXZ> guardedArchive(this);

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    XXZ_UNPACK_CONTEXT *pContext = (XXZ_UNPACK_CONTEXT *)pState->pContext;

    // Fill ARCHIVERECORD
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nCompressedSize;
    // result.nDecompressedOffset = 0;
    // result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    // Do not publish a fake zero uncompressed size. The complete, validated
    // total is available only after walking all concatenated Stream Indexes.
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XZ);

    return result;
}

bool XXZ::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XXZ> guardedArchive(this);

    if (!pState || !pState->pContext || !pDevice) return false;
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedArchive->getDevice());
    if (!guardedOutput || !guardedSource ||
        !guardedArchive->isUnpackOutputSupported(guardedOutput.data()) || !guardedArchive ||
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !guardedArchive || !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if ((pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    XXZ_UNPACK_CONTEXT *pContext = (XXZ_UNPACK_CONTEXT *)pState->pContext;
    if (pContext->nCompressedSize < 0) return false;
    const qint64 nCompressedSize = pContext->nCompressedSize;
    // The authoritative uncompressed total lives in the validated XZ
    // Index(es), which decompressXZ walks before decoding any Block.  Stage in
    // a growable temporary file rather than rejecting the still-unknown size
    // or trusting an unauthenticated estimate.
    std::unique_ptr<QTemporaryFile> pStage(
        new (std::nothrow) QTemporaryFile());
    if (!guardedArchive || !pStage || !pStage->open() || !guardedOutput ||
        !guardedSource ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive) return false;

    // Keep the complete member so XLZMADecoder can validate container framing,
    // all Block checks and Index records, and concatenated Streams.
    SubDevice sd(guardedSource.data(), 0, nCompressedSize);

    bool bResult = false;
    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_XZ);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pStage.get();
        state.nInputOffset = 0;
        state.nInputLimit = sd.size();
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        bResult = XLZMADecoder::decompressXZ(&state, pPdStruct) &&
                  guardedArchive && guardedOutput && guardedSource &&
                  (state.nCountInput == nCompressedSize) &&
                  (state.nCountOutput >= 0) &&
                  (pStage->size() == state.nCountOutput);

        if (bResult) {
            pContext->nUncompressedSize = state.nCountOutput;
        }

        sd.close();
    }

    bResult = bResult && guardedArchive && guardedOutput && guardedSource &&
              guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) && guardedArchive &&
              guardedArchive->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState,
                                  pPdStruct);

    if (bResult && guardedArchive) pState->nCurrentOffset = nCompressedSize;

    return bResult;
}

bool XXZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XXZ> guardedArchive(this);

    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !guardedArchive->isUnpackSourceCurrent(pState, pPdStruct) || !guardedArchive ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;
    pState->nCurrentOffset = pState->nTotalSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XXZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;
    QPointer<XXZ> guardedArchive(this);

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedArchive->ownsUnpackSource(pState)) return false;

    XXZ_UNPACK_CONTEXT *pContext =
        static_cast<XXZ_UNPACK_CONTEXT *>(pState->pContext);
    guardedArchive->releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    if (!guardedArchive) return false;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    return true;
}

QList<QString> XXZ::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("FD377A585A00");

    return listResult;
}

XBinary *XXZ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XXZ(pDevice);
}

bool XXZ::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XXZ> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XXZ::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XXZ> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XXZ::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
