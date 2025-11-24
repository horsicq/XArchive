/* Copyright (c) 2025 hors<horsicq@gmail.com>
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

XBinary::XCONVERT _TABLE_XXZ_STRUCTID[] = {{XXZ::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                           {XXZ::STRUCTID_STREAM_HEADER, "STREAM_HEADER", QString("Stream Header")},
                                           {XXZ::STRUCTID_BLOCK_HEADER, "BLOCK_HEADER", QString("Block Header")},
                                           {XXZ::STRUCTID_INDEX, "INDEX", QString("Index")},
                                           {XXZ::STRUCTID_STREAM_FOOTER, "STREAM_FOOTER", QString("Stream Footer")},
                                           {XXZ::STRUCTID_RECORD, "RECORD", QString("Record")}};

XXZ::XXZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XXZ::~XXZ()
{
}

bool XXZ::isValid(PDSTRUCT *pPdStruct)
{
    // Check magic bytes
    qint64 nSize = getSize();
    if (nSize < 6) return false;

    QByteArray baMagic = read_array(0, 6, pPdStruct);
    static const quint8 XZ_MAGIC[6] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
    if (memcmp(baMagic.constData(), XZ_MAGIC, 6) != 0) return false;

    return true;
}

bool XXZ::isValid(QIODevice *pDevice)
{
    bool bResult = false;

    if (pDevice) {
        qint64 nCurrentPos = pDevice->pos();

        pDevice->seek(0);

        quint8 baSignature[6] = {0};
        if (pDevice->read((char *)baSignature, sizeof(baSignature)) == sizeof(baSignature)) {
            static const quint8 XZ_MAGIC[6] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
            if (memcmp(baSignature, XZ_MAGIC, sizeof(baSignature)) == 0) {
                bResult = true;
            }
        }

        pDevice->seek(nCurrentPos);
    }

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
    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.nBinarySize = getSize();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());

    qint32 nIndex = 0;

    // Header
    _MEMORY_RECORD recordHeader = {};
    recordHeader.nAddress = -1;
    recordHeader.nOffset = 0;
    recordHeader.nSize = 12;  // Stream header size
    recordHeader.nIndex = nIndex++;
    recordHeader.filePart = FILEPART_HEADER;
    recordHeader.sName = tr("Stream Header");
    result.listRecords.append(recordHeader);

    // Footer
    _MEMORY_RECORD recordFooter = {};
    recordFooter.nAddress = -1;
    recordFooter.nOffset = getSize() - 12;
    recordFooter.nSize = 12;  // Stream footer size
    recordFooter.nIndex = nIndex++;
    recordFooter.filePart = FILEPART_FOOTER;
    recordFooter.sName = tr("Stream Footer");
    result.listRecords.append(recordFooter);

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
            recordOverlay.nAddress = -1;
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

QList<XBinary::DATA_HEADER> XXZ::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;

        _dataHeadersOptions.nID = STRUCTID_STREAM_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            DATA_HEADER dataHeader = {};
            dataHeader.dsID_parent = dataHeadersOptions.dsID_parent;
            dataHeader.dsID.sGUID = generateUUID();
            dataHeader.dsID.fileType = dataHeadersOptions.pMemoryMap->fileType;
            dataHeader.dsID.nID = dataHeadersOptions.nID;
            dataHeader.locType = dataHeadersOptions.locType;
            dataHeader.nLocation = dataHeadersOptions.nLocation;
            dataHeader.sName = structIDToString(dataHeadersOptions.nID);
            dataHeader.dhMode = dataHeadersOptions.dhMode;

            if (dataHeadersOptions.nID == STRUCTID_STREAM_HEADER) {
                dataHeader.nSize = 12;
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(STREAM_HEADER, header_magic), 6, "header_magic", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(STREAM_HEADER, stream_flags), 2, "stream_flags", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(STREAM_HEADER, crc32), 4, "crc32", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
            } else if (dataHeadersOptions.nID == STRUCTID_STREAM_FOOTER) {
                dataHeader.nSize = 12;
                dataHeader.listRecords.append(getDataRecord(offsetof(STREAM_FOOTER, crc32), 4, "crc32", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(STREAM_FOOTER, backward_size), 4, "backward_size", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(STREAM_FOOTER, stream_flags), 2, "stream_flags", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(STREAM_FOOTER, footer_magic), 2, "footer_magic", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
            }
            // TODO: Block header, Index, etc.
            listResult.append(dataHeader);
        }
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
    Q_UNUSED(pPdStruct)
    return 1;
}

QList<XArchive::RECORD> XXZ::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> list;
    if (nLimit == 0) return list;

    if (isValid(pPdStruct)) {
        RECORD record = {};
        record.spInfo.sRecordName = "stream";
        record.nDataOffset = 0;
        record.nDataSize = getSize();
        // TODO: parse uncompressed size, CRC, etc.
        list.append(record);
    }
    return list;
}

bool XXZ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Validate XZ file
        if (!isValid(pPdStruct)) {
            return false;
        }

        // Create and initialize context
        XXZ_UNPACK_CONTEXT *pContext = new XXZ_UNPACK_CONTEXT;

        // XZ format: 6-byte header + stream flags (2 bytes) + CRC32 (4 bytes) + compressed blocks + index + stream footer
        // For now, we treat the entire file as one compressed stream
        qint64 nOffset = 0;
        qint64 nFileSize = getSize();

        // Header size is always 12 bytes (6-byte magic + 2-byte flags + 4-byte CRC32)
        pContext->nHeaderSize = 12;
        nOffset = pContext->nHeaderSize;

        // Get filename from device
        pContext->sFileName = XBinary::getDeviceFileBaseName(getDevice());

        // Compressed data size: total size - header (12) - footer (12)
        qint64 nCompressedDataSize = nFileSize - 12 - 12;
        if (nCompressedDataSize < 0) {
            nCompressedDataSize = 0;
        }

        pContext->nCompressedSize = nCompressedDataSize;
        pContext->nUncompressedSize = 0;  // XZ uncompressed size would need to be parsed from blocks

        // Read footer CRC32 if available
        if (nFileSize >= 12) {
            qint64 nFooterOffset = nFileSize - 12;
            pContext->nCRC32 = read_uint32(nFooterOffset, false);  // Little-endian
        } else {
            pContext->nCRC32 = 0;
        }

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 1;  // XZ contains single compressed stream
        pState->pContext = pContext;

        bResult = true;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XXZ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    XXZ_UNPACK_CONTEXT *pContext = (XXZ_UNPACK_CONTEXT *)pState->pContext;

    // Fill ARCHIVERECORD
    result.nStreamOffset = pContext->nHeaderSize;
    result.nStreamSize = pContext->nCompressedSize;
    // result.nDecompressedOffset = 0;
    // result.nDecompressedSize = pContext->nUncompressedSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZMA2);

    if (pContext->nCRC32 != 0) {
        result.mapProperties.insert(FPART_PROP_CRC_VALUE, pContext->nCRC32);
        result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_EDB88320);
    }

    return result;
}

bool XXZ::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    XXZ_UNPACK_CONTEXT *pContext = (XXZ_UNPACK_CONTEXT *)pState->pContext;

    // Create a sub-device for the compressed stream (skipping header and footer)
    SubDevice sd(getDevice(), pContext->nHeaderSize, pContext->nCompressedSize);

    if (sd.open(QIODevice::ReadOnly)) {
        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZMA2);
        state.pDeviceInput = &sd;
        state.pDeviceOutput = pDevice;
        state.nInputOffset = 0;
        state.nInputLimit = sd.size();
        state.nProcessedOffset = 0;
        state.nProcessedLimit = -1;

        // Use XLZMADecoder to decompress LZMA2 data (used by XZ format)
        bResult = XLZMADecoder::decompressLZMA2(&state, pPdStruct);

        sd.close();
    }

    return bResult;
}

bool XXZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    // Move to next record
    pState->nCurrentIndex++;

    // XZ has only one record, so moving to next always returns false
    // This indicates end of archive
    bResult = false;

    return bResult;
}

bool XXZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        delete (XXZ_UNPACK_CONTEXT *)pState->pContext;
        pState->pContext = nullptr;
        bResult = true;
    }

    return bResult;
}
