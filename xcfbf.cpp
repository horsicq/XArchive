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
#include "xcfbf.h"

XBinary::XCONVERT _TABLE_CFBF_STRUCTID[] = {
    {XCFBF::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XCFBF::STRUCTID_StructuredStorageHeader, "StructuredStorageHeader", QString("StructuredStorageHeader")},
    {XCFBF::STRUCTID_CFBF_DIRECTORY_ENTRY, "CFBF_DIRECTORY_ENTRY", QString("CFBF_DIRECTORY_ENTRY")},
    {XCFBF::STRUCTID_FAT, "FAT", QString("FAT")},
    {XCFBF::STRUCTID_DIFAT, "DIFAT", QString("DIFAT")},
    {XCFBF::STRUCTID_MINIFAT, "MINIFAT", QString("MINIFAT")},
};

XCFBF::XCFBF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCFBF::~XCFBF()
{
}

bool XCFBF::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= 512) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "D0CF11E0A1B11AE100000000000000000000000000000000", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XCFBF::isValid(QIODevice *pDevice)
{
    XCFBF xcfbf(pDevice);

    return xcfbf.isValid();
}

QString XCFBF::getArch()
{
    return "";  // TODO
}

QString XCFBF::getVersion()
{
    QString sResult;

    quint16 uMinorVersion = read_uint16(0x18);
    quint16 uDllVersion = read_uint16(0x1A);

    sResult = QString("%1.%2").arg(uDllVersion).arg(uMinorVersion);

    return sResult;
}

QString XCFBF::getFileFormatExt()
{
    return "";
}

QString XCFBF::getMIMEString()
{
    return "application/x-cfbf";
}

quint64 XCFBF::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return 0;  // TODO
}

QString XCFBF::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_CFBF_STRUCTID, sizeof(_TABLE_CFBF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XCFBF::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_StructuredStorageHeader;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_StructuredStorageHeader) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCFBF::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = sizeof(StructuredStorageHeader);

                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _abSig), 8, "_abSig", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _clsid), 16, "_clsid", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                // Note: _clsid is 16 bytes (GUID), _uSectorShift is 2 bytes, ensure correct sizes
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _uMinorVersion), 2, "_uMinorVersion", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _uDllVersion), 2, "_uDllVersion", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _uByteOrder), 2, "_uByteOrder", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _uSectorShift), 2, "_uSectorShift", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _uMiniSectorShift), 2, "_uMiniSectorShift", VT_UINT16, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _usReserved), 2, "_usReserved", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _ulReserved1), 4, "_ulReserved1", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _csectDir), 4, "_csectDir", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _csectFat), 4, "_csectFat", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _sectDirStart), 4, "_sectDirStart", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _signature), 4, "_signature", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _ulMiniSectorCutoff), 4, "_ulMiniSectorCutoff", VT_UINT32, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));

                dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _sectMiniFatStart), 4, "_sectMiniFatStart", VT_UINT32, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _csectMiniFat), 4, "_csectMiniFat", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _sectDifStart), 4, "_sectDifStart", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _csectDif), 4, "_csectDif", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _sectFat), 436, "_sectFat", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    // Expand children: Directory chain, FAT sectors (first 109), MiniFAT, DIFAT
                    StructuredStorageHeader ssh = read_StructuredStorageHeader(nStartOffset, pPdStruct);
                    const qint64 sectorSize = (ssh._uSectorShift == 12) ? 4096 : 512;

                    // 1) Directory chain (as a table of directory entries)
                    if ((ssh._sectDirStart != 0xFFFFFFFF) && (ssh._csectDir)) {
                        DATA_HEADERS_OPTIONS _dirOptions = dataHeadersOptions;
                        _dirOptions.dsID_parent = dataHeader.dsID;
                        _dirOptions.dhMode = XBinary::DHMODE_TABLE;
                        _dirOptions.nID = STRUCTID_CFBF_DIRECTORY_ENTRY;
                        _dirOptions.locType = XBinary::LT_OFFSET;
                        _dirOptions.nLocation = sectorSize + (qint64)ssh._sectDirStart * sectorSize;
                        _dirOptions.nSize = (qint64)ssh._csectDir * sectorSize;
                        _dirOptions.nCount = (qint32)(_dirOptions.nSize / 128);
                        listResult.append(getDataHeaders(_dirOptions, pPdStruct));
                    }

                    // // 2) FAT sectors: first 109 listed in header
                    // {
                    //     DATA_HEADERS_OPTIONS _fatList = dataHeadersOptions;
                    //     _fatList.dsID_parent = dataHeader.dsID;
                    //     _fatList.dhMode = XBinary::DHMODE_HEADER;
                    //     _fatList.nID = STRUCTID_FAT;  // custom header name below
                    //     _fatList.locType = XBinary::LT_OFFSET;
                    //     _fatList.nLocation = nStartOffset + offsetof(StructuredStorageHeader, _sectFat);

                    //     XBinary::DATA_HEADER fatHeader = _initDataHeader(_fatList, QString("FAT sectors (first 109)"));
                    //     fatHeader.nSize = 109 * 4;

                    //     for (int i = 0; i < 109; ++i) {
                    //         QString sField = QString("FATSector[%1]").arg(i);
                    //         fatHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _sectFat) + i * 4, 4, sField, VT_UINT32, DRF_UNKNOWN,
                    //                                                    dataHeadersOptions.pMemoryMap->endian));
                    //     }

                    //     listResult.append(fatHeader);
                    // }

                    // 3) MiniFAT region (best-effort contiguous range)
                    if ((ssh._sectMiniFatStart != 0xFFFFFFFF) && (ssh._csectMiniFat)) {
                        qint64 miniFatOffset = sectorSize + (qint64)ssh._sectMiniFatStart * sectorSize;
                        qint64 miniFatSize = (qint64)ssh._csectMiniFat * sectorSize;

                        if (isOffsetValid(miniFatOffset) && miniFatSize > 0) {
                            DATA_HEADERS_OPTIONS _miniFat = dataHeadersOptions;
                            _miniFat.dsID_parent = dataHeader.dsID;
                            _miniFat.dhMode = XBinary::DHMODE_HEADER;
                            _miniFat.nID = STRUCTID_MINIFAT;
                            _miniFat.locType = XBinary::LT_OFFSET;
                            _miniFat.nLocation = miniFatOffset;
                            _miniFat.nSize = miniFatSize;

                            listResult.append(getDataHeaders(_miniFat, pPdStruct));
                        }
                    }

                    // 4) DIFAT region (best-effort contiguous range)
                    if ((ssh._sectDifStart != 0xFFFFFFFF) && (ssh._csectDif)) {
                        qint64 difatOffset = sectorSize + (qint64)ssh._sectDifStart * sectorSize;
                        qint64 difatSize = (qint64)ssh._csectDif * sectorSize;

                        if (isOffsetValid(difatOffset) && difatSize > 0) {
                            DATA_HEADERS_OPTIONS _difat = dataHeadersOptions;
                            _difat.dsID_parent = dataHeader.dsID;
                            _difat.dhMode = XBinary::DHMODE_HEADER;
                            _difat.nID = STRUCTID_DIFAT;
                            _difat.locType = XBinary::LT_OFFSET;
                            _difat.nLocation = difatOffset;
                            _difat.nSize = difatSize;

                            listResult.append(getDataHeaders(_difat, pPdStruct));
                        }
                    }
                }
            } else if (dataHeadersOptions.nID == STRUCTID_CFBF_DIRECTORY_ENTRY) {
                // Render a single CFBF directory entry (128 bytes)
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCFBF::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = 128;

                // Name (UTF-16LE, 32 WCHAR -> 64 bytes)
                dataHeader.listRecords.append(getDataRecord(0, 64, "Name", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(64, 2, "NameLength", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(66, 1, "ObjectType", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(67, 1, "ColorFlag", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(68, 4, "LeftSiblingID", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(72, 4, "RightSiblingID", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(76, 4, "ChildID", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(80, 16, "CLSID", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(96, 4, "StateBits", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(100, 8, "CreationTime", VT_UINT64, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(108, 8, "ModifiedTime", VT_UINT64, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(116, 4, "StartSectorLocation", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(120, 8, "StreamSize", VT_UINT64, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }

        }
    }

    return listResult;
}

QList<XArchive::RECORD> XCFBF::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listRecords;

    // CFBF header is 512 bytes
    const qint64 nHeaderSize = 512;
    if (getSize() < nHeaderSize) {
        return listRecords;
    }

    // Directory sector start (offset in sectors)
    quint32 nDirSectorStart = read_uint32(0x30, false);
    quint32 nSectorSize = 1 << read_uint16(0x1E, false);
    if (nSectorSize == 0) {
        nSectorSize = 512;  // fallback
    }

    // Directory entries are 128 bytes each
    const qint32 nDirEntrySize = 128;

    // Calculate first directory sector offset
    qint64 nDirOffset = nHeaderSize + (qint64)nDirSectorStart * nSectorSize;

    // Prevent infinite loop in case of corruption
    const qint32 nMaxEntries = 4096;
    qint32 nEntriesRead = 0;

    // Read directory entries
    while (isOffsetValid(nDirOffset + nDirEntrySize * nEntriesRead) && isPdStructNotCanceled(pPdStruct)) {
        if ((nLimit > 0) && (listRecords.size() >= nLimit)) {
            break;
        }
        if (nEntriesRead >= nMaxEntries) {
            break;
        }

        qint64 nEntryOffset = nDirOffset + nDirEntrySize * nEntriesRead;

        // Name (Unicode UTF-16LE, max 32 wchar = 64 bytes, null terminated)
        QByteArray baName = read_array(nEntryOffset, 64);
        quint16 nNameLength = read_uint16(nEntryOffset + 64, false);  // in bytes
        QString sName;
        if (nNameLength >= 2 && nNameLength <= 64) {
            sName = QString::fromUtf16((ushort *)baName.constData(), (nNameLength - 2) / 2);
        }

        quint8 nObjectType = read_uint8(nEntryOffset + 66);
        // 0 = unknown, 1 = storage, 2 = stream, 5 = root storage

        if (nObjectType == 0) {
            // Reached unused entries
            break;
        }

        quint32 nStartSector = read_uint32(nEntryOffset + 116, false);
        quint64 nStreamSize = read_uint64(nEntryOffset + 120, false);

        XArchive::RECORD record = {};
        record.spInfo.sRecordName = sName;
        record.spInfo.nUncompressedSize = nStreamSize;
        record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
        record.nDataOffset = nHeaderSize + ((qint64)nStartSector * nSectorSize);
        record.nDataSize = nStreamSize;
        record.nHeaderOffset = nEntryOffset;
        record.nHeaderSize = nDirEntrySize;
        record.sUUID = "";  // CFBF entries don’t have a UUID

        listRecords.append(record);

        nEntriesRead++;
    }

    return listRecords;
}

XCFBF::StructuredStorageHeader XCFBF::read_StructuredStorageHeader(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    StructuredStorageHeader header = {};

    read_array(nOffset, (char *)&header, sizeof(header), pPdStruct);

    return header;
}

XBinary::_MEMORY_MAP XCFBF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;  // Default similar to other archives
    if (mapMode == MAPMODE_DATA) {
        return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

XBinary::FT XCFBF::getFileType()
{
    return FT_CFBF;
}

XBinary::ENDIAN XCFBF::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XCFBF::getMode()
{
    return MODE_DATA;
}

QList<XBinary::MAPMODE> XCFBF::getMapModesList()
{
    QList<MAPMODE> listResult;
    listResult.append(MAPMODE_DATA);
    listResult.append(MAPMODE_REGIONS);
    return listResult;
}

qint64 XCFBF::getImageSize()
{
    // CFBF is not a memory image; use total file size
    return getSize();
}

QList<XBinary::FPART> XCFBF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    QList<FPART> listResult;

    const qint64 fileSize = getSize();
    if (fileSize < 512) return listResult;

    StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);
    const qint64 sectorSize = (ssh._uSectorShift == 12) ? 4096 : 512;

    if (nFileParts & FILEPART_HEADER) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = qMin<qint64>(sectorSize, fileSize);
        header.nVirtualAddress = -1;
        header.sName = tr("Header");
        listResult.append(header);
    }

    if (nFileParts & FILEPART_DATA) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = 0;
        data.nFileSize = fileSize;
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if (nFileParts & FILEPART_REGION) {
        // Best-effort logical regions: FAT sectors, MiniFAT sectors, DIFAT sectors, Directory sectors
        auto addRegion = [&](qint64 offset, qint64 size, const QString &name) {
            if (offset < 0 || size <= 0) return;
            if (offset >= fileSize) return;
            size = qMin<qint64>(size, fileSize - offset);
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = offset;
            part.nFileSize = size;
            part.nVirtualAddress = -1;
            part.sName = name;
            listResult.append(part);
        };

        // Directory chain
        if (ssh._sectDirStart != 0xFFFFFFFF && ssh._csectDir) {
            qint64 dirOffset = sectorSize + (qint64)ssh._sectDirStart * sectorSize;
            addRegion(dirOffset, (qint64)ssh._csectDir * sectorSize, QString("%1").arg("Directory"));
        }

        // FAT sectors: first 109 in header
        if (ssh._csectFat) {
            qint64 totalFat = (qint64)ssh._csectFat * sectorSize;
            // Approximate: first entries are in header _sectFat array; actual FAT sectors are scattered, but we can mark their total range best-effort
            // Derive first FAT sector offset from minimum non-FFFFFFFF entry
            quint32 firstFatSect = 0xFFFFFFFF;
            for (int i = 0; i < 109; ++i) {
                if (ssh._sectFat[i] != 0xFFFFFFFF) {
                    firstFatSect = qMin(firstFatSect, ssh._sectFat[i]);
                }
            }
            if (firstFatSect != 0xFFFFFFFF) {
                qint64 fatOffset = sectorSize + (qint64)firstFatSect * sectorSize;
                addRegion(fatOffset, totalFat, QString("%1").arg("FAT"));
            }
        }

        // MiniFAT
        if (ssh._sectMiniFatStart != 0xFFFFFFFF && ssh._csectMiniFat) {
            qint64 miniFatOffset = sectorSize + (qint64)ssh._sectMiniFatStart * sectorSize;
            addRegion(miniFatOffset, (qint64)ssh._csectMiniFat * sectorSize, QString("%1").arg("MiniFAT"));
        }

        // DIFAT
        if (ssh._sectDifStart != 0xFFFFFFFF && ssh._csectDif) {
            qint64 difatOffset = sectorSize + (qint64)ssh._sectDifStart * sectorSize;
            addRegion(difatOffset, (qint64)ssh._csectDif * sectorSize, QString("%1").arg("DIFAT"));
        }
    }

    return listResult;
}
