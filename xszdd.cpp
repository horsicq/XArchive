/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xszdd.h"

static XBinary::XCONVERT _TABLE_XSZDD_STRUCTID[] = {
    {XSZDD::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XSZDD::STRUCTID_SZDD_HEADER, "SZDD_HEADER", QString("SZDD_HEADER")}
};

XSZDD::XSZDD(QIODevice *pDevice)
    : XArchive(pDevice)
{
}

XSZDD::~XSZDD()
{
}

bool XSZDD::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(SZDD_HEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "'SZDD'88F027'3A'", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

XSZDD::SZDD_HEADER XSZDD::_read_SZDD_HEADER(qint64 nOffset)
{
    SZDD_HEADER header = {};
    read_array(nOffset, (char *)&header, sizeof(SZDD_HEADER));
    return header;
}

XBinary::FT XSZDD::getFileType()
{
    return XBinary::FT_SZDD;
}

XBinary::MODE XSZDD::getMode()
{
    return XBinary::MODE_DATA;
}

QString XSZDD::getMIMEString()
{
    return "application/x-ms-compress";
}

qint32 XSZDD::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

XBinary::ENDIAN XSZDD::getEndian()
{
    return XBinary::ENDIAN_LITTLE;
}

QString XSZDD::getArch()
{
    return QString();
}

QString XSZDD::getFileFormatExt()
{
    return "SZDD";
}

qint64 XSZDD::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return getSize();
}

bool XSZDD::isSigned()
{
    return false;
}

XBinary::OSNAME XSZDD::getOsName()
{
    return XBinary::OSNAME_MULTIPLATFORM;
}

QString XSZDD::getOsVersion()
{
    return QString();
}

QString XSZDD::getVersion()
{
    return QString();
}

QString XSZDD::getInfo()
{
    return "";
}

bool XSZDD::isEncrypted()
{
    return false;
}

QList<XBinary::MAPMODE> XSZDD::getMapModesList()
{
    QList<MAPMODE> list;
    list.append(MAPMODE_REGIONS);
    return list;
}

XBinary::_MEMORY_MAP XSZDD::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)
    Q_UNUSED(pPdStruct)

    _MEMORY_MAP map = {};
    map.fileType = getFileType();
    map.mode = getMode();
    map.endian = getEndian();
    map.sType = typeIdToString(getType());
    map.sArch = getArch();

    qint32 nIndex = 0;

    // Add file header
    _MEMORY_RECORD recHeader = {};
    recHeader.nAddress = -1;
    recHeader.segment = ADDRESS_SEGMENT_FLAT;
    recHeader.nOffset = 0;
    recHeader.nSize = sizeof(SZDD_HEADER);
    recHeader.nIndex = nIndex++;
    recHeader.type = MMT_HEADER;
    recHeader.sName = tr("SZDD Header");
    map.listRecords.append(recHeader);

    return map;
}

QList<XBinary::HREGION> XSZDD::getHData(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return QList<HREGION>();
}

QString XSZDD::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSZDD_STRUCTID, sizeof(_TABLE_XSZDD_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XSZDD::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = DHMODE_HEADER;

        _dataHeadersOptions.nID = STRUCTID_SZDD_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else if (dataHeadersOptions.nID == STRUCTID_SZDD_HEADER) {
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
            dataHeader.nSize = sizeof(SZDD_HEADER);

            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, signature), 8, "signature", VT_HEX, DRF_UNKNOWN, ENDIAN_LITTLE));
            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, compression_mode), 1, "compression_mode", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, missing_char), 1, "missing_char", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
            dataHeader.listRecords.append(getDataRecord(offsetof(SZDD_HEADER, uncompressed_size), 4, "uncompressed_size", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));

            listResult.append(dataHeader);
        }
    }

    return listResult;
}

qint32 XSZDD::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<QVariant> *pListValues, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nRow)
    Q_UNUSED(locType)
    Q_UNUSED(nLocation)
    Q_UNUSED(dataRecordsOptions)
    Q_UNUSED(pListValues)
    Q_UNUSED(pPdStruct)
    // No tables in SZDD
    return 0;
}

quint64 XSZDD::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    return 1; // Only one file per archive
}

QList<XArchive::RECORD> XSZDD::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> records;

    if (nLimit == 0) return records;
    if (!isValid(pPdStruct)) return records;

    SZDD_HEADER header = _read_SZDD_HEADER(0);
    RECORD record = {};

    record.spInfo.sRecordName = QString(); // We don't know the original filename
    record.spInfo.nCRC32 = 0; // Not present
    record.spInfo.nUncompressedSize = header.uncompressed_size;
    record.spInfo.nWindowSize = 4096;
    record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
    record.nDataOffset = sizeof(SZDD_HEADER);
    record.nDataSize = getSize() - sizeof(SZDD_HEADER);
    record.nHeaderOffset = 0;
    record.nHeaderSize = sizeof(SZDD_HEADER);
    record.nOptHeaderOffset = 0;
    record.nOptHeaderSize = 0;
    record.sUUID = generateUUID();
    record.nLayerOffset = 0;
    record.nLayerSize = 0;
    record.layerCompressMethod = COMPRESS_METHOD_UNKNOWN;

    records.append(record);

    return records;
}
