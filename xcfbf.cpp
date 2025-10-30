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
    return "ole";
}

QString XCFBF::getFileFormatExtsString()
{
    return "CFBF (*.ole;*.doc;*.xls;*.ppt;*.msi)";
}

qint64 XCFBF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // CFBF files are typically complete; return full file size
    return getSize();
}

QString XCFBF::getMIMEString()
{
    return "application/x-cfbf";
}

quint64 XCFBF::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    quint64 nResult = 0;

    // CFBF header is 512 bytes minimum
    const qint64 nHeaderSize = 512;
    if (getSize() < nHeaderSize) {
        return 0;
    }

    // Read sector size from header
    quint16 nSectorShift = read_uint16(0x1E, false);
    quint32 nSectorSize = 1 << nSectorShift;
    if (nSectorSize == 0) {
        nSectorSize = 512;  // fallback
    }

    // Read directory sector start
    quint32 nDirSectorStart = read_uint32(0x30, false);

    // Read number of directory sectors (for version 4, required; for version 3, may be 0)
    quint32 nCsectDir = read_uint32(0x28, false);

    // Directory entries are 128 bytes each
    const qint32 nDirEntrySize = 128;

    // Calculate how many entries can fit
    if (nCsectDir > 0) {
        // Version 4: use csectDir field
        qint64 nDirTotalSize = (qint64)nCsectDir * nSectorSize;
        nResult = nDirTotalSize / nDirEntrySize;
    } else {
        // Version 3: count entries by reading until we hit unused entries
        qint64 nDirOffset = nHeaderSize + (qint64)nDirSectorStart * nSectorSize;
        const qint32 nMaxEntries = 4096;  // safety limit

        for (qint32 i = 0; i < nMaxEntries; i++) {
            qint64 nEntryOffset = nDirOffset + (qint64)i * nDirEntrySize;
            if (!isOffsetValid(nEntryOffset + nDirEntrySize - 1)) {
                break;
            }

            quint8 nObjectType = read_uint8(nEntryOffset + 66);
            if (nObjectType == 0) {
                // Reached unused entry
                break;
            }

            nResult++;
        }
    }

    return nResult;
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
    } else if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_HEADER | MAPMODE_STREAMS | FILEPART_OVERLAY, pPdStruct);
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
    listResult.append(MAPMODE_STREAMS);
    return listResult;
}

qint64 XCFBF::getImageSize()
{
    // CFBF is not a memory image; use total file size
    return getSize();
}

QList<XBinary::FPART> XCFBF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    const qint64 fileSize = getSize();
    if (fileSize < 512) return listResult;

    qint64 nMaxOffset = 0;

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

    nMaxOffset = qMax(nMaxOffset, sectorSize);

    {
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
                if (nFileParts & FILEPART_REGION) {
                    _addRegion(&listResult, fileSize, fatOffset, totalFat, QString("%1").arg("FAT"));
                }
                nMaxOffset = qMax(nMaxOffset, fatOffset + totalFat);
            }
        }

        // MiniFAT
        if (ssh._sectMiniFatStart != 0xFFFFFFFF && ssh._csectMiniFat) {
            qint64 miniFatOffset = sectorSize + (qint64)ssh._sectMiniFatStart * sectorSize;
            qint64 miniFatSize = (qint64)ssh._csectMiniFat * sectorSize;

            if (nFileParts & FILEPART_REGION) {
                _addRegion(&listResult, fileSize, miniFatOffset, miniFatSize, QString("%1").arg("MiniFAT"));
            }
            nMaxOffset = qMax(nMaxOffset, miniFatOffset + miniFatSize);
        }

        // DIFAT
        if (ssh._sectDifStart != 0xFFFFFFFF && ssh._csectDif) {
            qint64 difatOffset = sectorSize + (qint64)ssh._sectDifStart * sectorSize;
            qint64 difatSize = (qint64)ssh._csectDif * sectorSize;

            if (nFileParts & FILEPART_REGION) {
                _addRegion(&listResult, fileSize, difatOffset, difatSize, QString("%1").arg("DIFAT"));
            }
            nMaxOffset = qMax(nMaxOffset, difatOffset + difatSize);
        }
    }

    {
        // Best-effort: parse directory entries directly and emit stream parts.
        // Note: CFBF streams may be fragmented; we map from the first sector linearly for UI purposes.
        // Mini streams are mapped relative to the Root Storage stream start.

        // Gather Root Storage info (needed for MiniStream base)
        qint64 dirBaseOffset = -1;
        qint64 dirTotalSize = 0;
        if (ssh._sectDirStart != 0xFFFFFFFF) {
            dirBaseOffset = sectorSize + (qint64)ssh._sectDirStart * sectorSize;

            if (ssh._csectDir) {
                dirTotalSize = (qint64)ssh._csectDir * sectorSize;
            } else {
                dirTotalSize = fileSize - dirBaseOffset;
            }
        }

        if ((dirBaseOffset != -1) && (dirTotalSize >= 128)) {
            const qint32 entrySize = 128;
            qint32 maxEntries = (qint32)(dirTotalSize / entrySize);
            if (maxEntries > 16384) {
                maxEntries = 16384;  // sanity cap
            }

            // First pass: locate Root Storage (ObjectType 5)
            bool bHaveRoot = false;
            quint32 nRootStartSector = 0;
            quint64 nRootStreamSize = 0;
            qint64 nFixedSize = 0;
            for (qint32 i = 0; (i < maxEntries) && isPdStructNotCanceled(pPdStruct); i++) {
                qint64 entryOffset = dirBaseOffset + (qint64)i * entrySize;
                if (!isOffsetValid(entryOffset + entrySize - 1)) {
                    break;
                }
                quint8 nObjectType = read_uint8(entryOffset + 66);
                if (nObjectType == 0) {
                    continue;  // unused slot
                }
                if (nObjectType == 5) {  // Root Storage
                    nRootStartSector = read_uint32(entryOffset + 116, false);
                    nRootStreamSize = read_uint64(entryOffset + 120, false);
                    bHaveRoot = true;
                }

                nFixedSize += entrySize;
            }

            nMaxOffset = qMax(nMaxOffset, dirBaseOffset + nFixedSize);

            if (nFileParts & FILEPART_REGION) {
                _addRegion(&listResult, fileSize, dirBaseOffset, nFixedSize, QString("%1").arg("Directory"));
            }

            // Mini stream parameters
            const qint64 miniSectorSize = (ssh._uMiniSectorShift >= 4 && ssh._uMiniSectorShift <= 12) ? ((qint64)1 << ssh._uMiniSectorShift) : 64;
            const quint64 miniCutoff = ssh._ulMiniSectorCutoff ? ssh._ulMiniSectorCutoff : 4096;
            qint64 miniBaseOffset = -1;
            if (bHaveRoot) {
                miniBaseOffset = sectorSize + (qint64)nRootStartSector * sectorSize;
            }

            // Second pass: emit streams
            qint32 nEmitted = 0;
            for (qint32 i = 0; (i < maxEntries) && isPdStructNotCanceled(pPdStruct); i++) {
                if ((nLimit > 0) && (nEmitted >= nLimit)) {
                    break;
                }

                qint64 entryOffset = dirBaseOffset + (qint64)i * entrySize;
                if (!isOffsetValid(entryOffset + entrySize - 1)) {
                    break;
                }

                // Read object type
                quint8 nObjectType = read_uint8(entryOffset + 66);
                if (!((nObjectType == 2) || (nObjectType == 1) || (nObjectType == 5))) {
                    continue;  // Streams, User Storages, Root Storage
                }

                quint32 nStartSector = read_uint32(entryOffset + 116, false);
                quint64 nStreamSize = read_uint64(entryOffset + 120, false);

                // For Root Storage, treat its stream as the MiniStream itself
                if (nObjectType == 5) {
                    nStartSector = nRootStartSector;
                    nStreamSize = nRootStreamSize;
                }

                if ((nStreamSize == 0) || (nStartSector == 0xFFFFFFFF)) {
                    continue;  // skip empty
                }

                // Only non-root entries can be mini; Root stream is stored in regular sectors
                bool bIsMini = (nObjectType != 5) && (nStreamSize < miniCutoff) && (miniBaseOffset != -1);
                qint64 streamOffset = -1;
                if (bIsMini) {
                    streamOffset = miniBaseOffset + (qint64)nStartSector * miniSectorSize;
                } else {
                    streamOffset = sectorSize + (qint64)nStartSector * sectorSize;
                }

                if (!isOffsetValid(streamOffset)) {
                    continue;
                }

                qint64 clampedSize = (qint64)nStreamSize;
                if (clampedSize > (fileSize - streamOffset)) {
                    clampedSize = fileSize - streamOffset;
                }
                if (clampedSize <= 0) {
                    continue;
                }

                if (nFileParts & FILEPART_STREAM) {
                    // Read name
                    QByteArray baName = read_array(entryOffset + 0, 64);
                    quint16 nNameLength = read_uint16(entryOffset + 64, false);
                    QString sName;
                    if ((nNameLength >= 2) && (nNameLength <= 64)) {
                        sName = QString::fromUtf16((ushort *)baName.constData(), (nNameLength - 2) / 2);
                    }

                    FPART part = {};
                    part.filePart = FILEPART_STREAM;
                    part.nFileOffset = streamOffset;
                    part.nFileSize = clampedSize;
                    part.nVirtualAddress = -1;
                    part.sName = sName;
                    listResult.append(part);
                }

                nMaxOffset = qMax(nMaxOffset, streamOffset + clampedSize);

                nEmitted++;
            }
        }
    }

    nMaxOffset = align_up(nMaxOffset, sectorSize);

    if (nFileParts & FILEPART_DATA) {
        FPART data = {};
        data.filePart = FILEPART_DATA;
        data.nFileOffset = sectorSize;
        data.nFileSize = qMin(nMaxOffset, fileSize);
        data.nVirtualAddress = -1;
        data.sName = tr("Data");
        listResult.append(data);
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < fileSize) {
            FPART overlay = {};
            overlay.filePart = FILEPART_OVERLAY;
            overlay.nFileOffset = nMaxOffset;
            overlay.nFileSize = fileSize - nMaxOffset;
            overlay.nVirtualAddress = -1;
            overlay.sName = tr("Overlay");
            listResult.append(overlay);
        }
    }

    return listResult;
}

void XCFBF::_addRegion(QList<FPART> *pListResult, qint64 fileSize, qint64 offset, qint64 size, const QString &name)
{
    if (!pListResult) return;
    if (offset < 0) return;
    if (size <= 0) return;
    if (offset >= fileSize) return;

    qint64 clampedSize = size;
    if (clampedSize > (fileSize - offset)) {
        clampedSize = fileSize - offset;
    }

    FPART part = {};
    part.filePart = FILEPART_REGION;
    part.nFileOffset = offset;
    part.nFileSize = clampedSize;
    part.nVirtualAddress = -1;
    part.sName = name;
    pListResult->append(part);
}

bool XCFBF::initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        const qint64 nHeaderSize = 512;
        const qint64 nFileSize = getSize();

        if (nFileSize < nHeaderSize) {
            return false;
        }

        // Read header
        StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);

        // Create and initialize context
        CFBF_UNPACK_CONTEXT *pContext = new CFBF_UNPACK_CONTEXT;
        pContext->nSectorSize = (ssh._uSectorShift == 12) ? 4096 : 512;
        pContext->nMiniSectorSize = (ssh._uMiniSectorShift >= 4 && ssh._uMiniSectorShift <= 12) ? ((qint64)1 << ssh._uMiniSectorShift) : 64;
        pContext->nMiniCutoff = ssh._ulMiniSectorCutoff ? ssh._ulMiniSectorCutoff : 4096;
        pContext->nRootStreamStart = 0;
        pContext->nRootStreamSize = 0;

        // Calculate directory base offset
        if (ssh._sectDirStart != 0xFFFFFFFF) {
            pContext->nDirBaseOffset = pContext->nSectorSize + (qint64)ssh._sectDirStart * pContext->nSectorSize;
        } else {
            delete pContext;
            return false;
        }

        // Calculate directory total size
        qint64 nDirTotalSize = 0;
        if (ssh._csectDir) {
            nDirTotalSize = (qint64)ssh._csectDir * pContext->nSectorSize;
        } else {
            nDirTotalSize = nFileSize - pContext->nDirBaseOffset;
        }

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = nFileSize;
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;

        // Pre-scan directory entries
        const qint32 nEntrySize = 128;
        qint32 nMaxEntries = (qint32)(nDirTotalSize / nEntrySize);
        if (nMaxEntries > 16384) {
            nMaxEntries = 16384;  // sanity cap
        }

        // First pass: locate Root Storage and count streams
        for (qint32 i = 0; (i < nMaxEntries) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            qint64 nEntryOffset = pContext->nDirBaseOffset + (qint64)i * nEntrySize;

            if (!isOffsetValid(nEntryOffset + nEntrySize - 1)) {
                break;
            }

            quint8 nObjectType = read_uint8(nEntryOffset + 66);
            if (nObjectType == 0) {
                continue;  // unused slot
            }

            if (nObjectType == 5) {  // Root Storage
                quint32 nRootStartSector = read_uint32(nEntryOffset + 116, false);
                quint64 nRootStreamSize = read_uint64(nEntryOffset + 120, false);
                pContext->nRootStreamStart = nRootStartSector;
                pContext->nRootStreamSize = nRootStreamSize;
            } else if (nObjectType == 2) {  // Stream
                // Check if stream has data
                quint32 nStartSector = read_uint32(nEntryOffset + 116, false);
                quint64 nStreamSize = read_uint64(nEntryOffset + 120, false);

                if ((nStreamSize > 0) && (nStartSector != 0xFFFFFFFF)) {
                    pContext->listRecordOffsets.append(nEntryOffset);
                    pState->nNumberOfRecords++;
                }
            }
        }

        // Set current offset to first record
        if (pState->nNumberOfRecords > 0) {
            pState->nCurrentOffset = pContext->listRecordOffsets.at(0);
            bResult = true;
        } else {
            // No records found, clean up context
            delete pContext;
            pState->pContext = nullptr;
        }
    }

    return bResult;
}

XBinary::ARCHIVERECORD XCFBF::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    CFBF_UNPACK_CONTEXT *pContext = (CFBF_UNPACK_CONTEXT *)pState->pContext;
    qint64 nEntryOffset = pState->nCurrentOffset;

    // Read directory entry fields
    QByteArray baName = read_array(nEntryOffset + 0, 64);
    quint16 nNameLength = read_uint16(nEntryOffset + 64, false);
    quint32 nStartSector = read_uint32(nEntryOffset + 116, false);
    quint64 nStreamSize = read_uint64(nEntryOffset + 120, false);
    quint64 nModifiedTime = read_uint64(nEntryOffset + 108, false);

    // Parse name
    QString sName;
    if ((nNameLength >= 2) && (nNameLength <= 64)) {
        sName = QString::fromUtf16((ushort *)baName.constData(), (nNameLength - 2) / 2);
    }

    // Determine if mini-stream
    bool bIsMini = (nStreamSize < pContext->nMiniCutoff) && (pContext->nRootStreamStart != 0xFFFFFFFF);

    // Calculate stream offset
    qint64 nStreamOffset = -1;
    if (bIsMini) {
        qint64 nMiniBaseOffset = pContext->nSectorSize + (qint64)pContext->nRootStreamStart * pContext->nSectorSize;
        nStreamOffset = nMiniBaseOffset + (qint64)nStartSector * pContext->nMiniSectorSize;
    } else {
        nStreamOffset = pContext->nSectorSize + (qint64)nStartSector * pContext->nSectorSize;
    }

    // Fill ARCHIVERECORD
    result.nStreamOffset = nStreamOffset;
    result.nStreamSize = (qint64)nStreamSize;
    result.nDecompressedOffset = 0;
    result.nDecompressedSize = (qint64)nStreamSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nStreamSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE);

    // Convert FILETIME to QDateTime (if non-zero)
    if (nModifiedTime != 0) {
        // FILETIME is 100-nanosecond intervals since January 1, 1601
        // Convert to Unix timestamp (seconds since January 1, 1970)
        const quint64 nEpochDiff = 116444736000000000ULL;  // Difference between 1601 and 1970 in 100-ns intervals
        if (nModifiedTime > nEpochDiff) {
            qint64 nUnixTime = (nModifiedTime - nEpochDiff) / 10000000;
            QDateTime dateTime = QDateTime::fromSecsSinceEpoch(nUnixTime, Qt::UTC);
            result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);
        }
    }

    return result;
}

bool XCFBF::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    // Get current record info
    ARCHIVERECORD record = infoCurrent(pState, pPdStruct);

    if (record.nStreamSize <= 0) {
        return true;  // Empty file, success
    }

    // CFBF streams may be fragmented across sectors following FAT chain
    // For simplicity, we assume contiguous storage (best-effort)
    // TODO: Implement proper FAT chain following for fragmented files

    if (record.nStreamOffset >= 0) {
        // Direct copy (assumes contiguous storage)
        bResult = copyDeviceMemory(getDevice(), record.nStreamOffset, pDevice, 0, record.nStreamSize);
    }

    return bResult;
}

bool XCFBF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    CFBF_UNPACK_CONTEXT *pContext = (CFBF_UNPACK_CONTEXT *)pState->pContext;

    // Move to next record
    pState->nCurrentIndex++;

    // Check if more records available
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        // Update current offset from pre-computed list
        pState->nCurrentOffset = pContext->listRecordOffsets.at(pState->nCurrentIndex);
        bResult = true;
    }

    return bResult;
}
