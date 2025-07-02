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

QString XXZ::getInfo()
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

QList<XBinary::HREGION> XXZ::getHData(PDSTRUCT *pPdStruct)
{
    // For now, return empty
    QList<HREGION> list;
    return list;
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

qint32 XXZ::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<QVariant> *pListValues, PDSTRUCT *pPdStruct)
{
    // No table rows defined for basic headers
    Q_UNUSED(nRow)
    Q_UNUSED(locType)
    Q_UNUSED(nLocation)
    Q_UNUSED(dataRecordsOptions)
    Q_UNUSED(pListValues)
    Q_UNUSED(pPdStruct)
    return 0;
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
