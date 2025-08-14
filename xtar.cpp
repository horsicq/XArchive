/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#include "xtar.h"

XTAR::XCONVERT _TABLE_XTAR_STRUCTID[] = {{XTAR::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XTAR::STRUCTID_POSIX_HEADER, "posix_header", QString("posix_header")}};

XTAR::XTAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XTAR::isValid(PDSTRUCT *pPdStruct)
{
    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

    return _isValid(&memoryMap, 0, pPdStruct);
}

bool XTAR::_isValid(_MEMORY_MAP *pMemoryMap, qint64 nOffset, PDSTRUCT *pPdStruct)
{
    // TODO more checks
    bool bResult = false;

    if ((getSize() - nOffset) >= 0x200)  // TODO const
    {
        if (compareSignature(pMemoryMap, "00'ustar'", nOffset + 0x100, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XTAR::isValid(QIODevice *pDevice)
{
    XTAR xtar(pDevice);

    return xtar.isValid();
}

quint64 XTAR::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

    qint64 nOffset = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        nResult++;

        nOffset += (0x200);
        nOffset += align_up(QString(header.size).toULongLong(0, 8), 0x200);
    }

    return nResult;
}

QList<XArchive::RECORD> XTAR::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    qint64 nOffset = 0;

    qint32 nIndex = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        RECORD record = {};
        record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
        record.nDataOffset = nOffset + 0x200;
        record.nHeaderOffset = nOffset;
        record.nHeaderSize = 0x200;
        record.spInfo.nUncompressedSize = QString(header.size).toULongLong(0, 8);
        record.spInfo.sRecordName = header.name;
        record.nDataSize = record.spInfo.nUncompressedSize;

        listResult.append(record);

        nIndex++;

        if ((nLimit != -1) && (nIndex > nLimit)) {
            break;
        }

        nOffset += (0x200);
        nOffset += align_up(record.nDataSize, 0x200);
    }

    return listResult;
}

QString XTAR::getFileFormatExt()
{
    return "tar";
}

QString XTAR::getFileFormatExtsString()
{
    return "TAR (*.tar)";
}

QString XTAR::getMIMEString()
{
    return "application/x-tar";
}

QString XTAR::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XTAR_STRUCTID, sizeof(_TABLE_XTAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

qint32 XTAR::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords, void *pUserData, PDSTRUCT *pPdStruct)
{
    qint32 nResult = 0;

    if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_POSIX_HEADER) {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);

        qint64 nStartOffset = locationToOffset(dataRecordsOptions.pMemoryMap, locType, nLocation);

        XTAR::posix_header header = read_posix_header(nStartOffset);

        nResult = 0x200 + align_up(QString(header.size).toULongLong(0, 8), 0x200);
    } else {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
    }

    return nResult;
}

qint32 XTAR::_getNumberOf_posix_headers(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    qint32 nResult = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        nResult++;

        nOffset += (0x200);
        nOffset += align_up(QString(header.size).toULongLong(0, 8), 0x200);
    }

    return nResult;
}

QList<XBinary::DATA_HEADER> XTAR::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nCount = _getNumberOf_posix_headers(0, pPdStruct);
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nID = STRUCTID_POSIX_HEADER;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_POSIX_HEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XTAR::structIDToString(dataHeadersOptions.nID));

                dataHeader.listRecords.append(getDataRecord(0, 100, "Name", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(100, 8, "Mode", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(108, 8, "UID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(116, 8, "GID", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(124, 12, "Size", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(136, 12, "MTime", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(148, 8, "Checksum", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(156, 1, "Typeflag", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(157, 100, "Linkname", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(257, 6, "Magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(263, 2, "Version", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(265, 32, "Uname", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(297, 32, "Gname", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(329, 8, "Devmajor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(337, 8, "Devminor", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(345, 155, "Prefix", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.nSize = 500;  // TODO const

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XTAR::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    qint64 nOffset = 0;

    while (isPdStructNotCanceled(pPdStruct)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 5)) {
            break;
        }

        if (nFileParts & FILEPART_HEADER) {
            XBinary::FPART record = {};
            record.filePart = FILEPART_HEADER;
            record.nFileOffset = nOffset;
            record.nFileSize = 0x200;  // TODO const
            record.nVirtualAddress = -1;
            record.sName = tr("Header");
            listResult.append(record);
        }

        if (nFileParts & FILEPART_STREAM) {
            XBinary::FPART record = {};
            record.filePart = FILEPART_STREAM;
            record.nFileOffset = nOffset + 0x200;
            record.nFileSize = align_up(QString(header.size).toULongLong(0, 8), 0x200);  // TODO const
            record.nVirtualAddress = -1;
            record.sName = header.name;
            record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE);
            record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, record.nFileSize);
            record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, record.nFileSize);
            // TODO Checksum
            listResult.append(record);
        }

        nOffset += (0x200);
        nOffset += align_up(QString(header.size).toULongLong(0, 8), 0x200);
    }

    if (nFileParts & FILEPART_DATA) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nOffset;  // Total size of the file
        record.nVirtualAddress = -1;
        record.sName = tr("Data");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_OVERLAY) && (nOffset < getSize())) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nOffset;
        record.nFileSize = getSize() - nOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Overlay");
        listResult.append(record);
    }

    return listResult;
}

XBinary::_MEMORY_MAP XTAR::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;  // Default mode
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    } else if (mapMode == MAPMODE_DATA) {
        result = _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    }

    return result;
}

QList<XBinary::MAPMODE> XTAR::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::FT XTAR::getFileType()
{
    return FT_TAR;
}

XTAR::posix_header XTAR::read_posix_header(qint64 nOffset)
{
    posix_header record = {};

    read_array(nOffset, (char *)&record, sizeof(record));

    return record;
}
