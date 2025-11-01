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
#include "xcab.h"

static XBinary::XCONVERT _TABLE_XCAB_STRUCTID[] = {{XCab::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XCab::STRUCTID_CFHEADER, "CFHEADER", QString("CFHEADER")},
                                                   {XCab::STRUCTID_CFFOLDER, "CFFOLDER", QString("CFFOLDER")},
                                                   {XCab::STRUCTID_CFFILE, "CFFILE", QString("CFFILE")},
                                                   {XCab::STRUCTID_CFDATA, "CFDATA", QString("CFDATA")}};

XCab::XCab(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XCab::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(CFHEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "'MSCF'00000000........00000000........00000000", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XCab::isValid(QIODevice *pDevice)
{
    XCab xcab(pDevice);

    return xcab.isValid();
}

QString XCab::getVersion()
{
    return QString("%1.%2").arg(read_uint8(25)).arg(read_uint8(24), 2, 10, QChar('0'));
}

quint64 XCab::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    quint64 nResult = 0;

    nResult = read_uint16(offsetof(CFHEADER, cFiles));

    return nResult;
}

XCab::CFFILE XCab::readCFFILE(qint64 nOffset)
{
    CFFILE result = {};

    result.cbFile = read_uint32(nOffset + offsetof(CFFILE, cbFile));
    result.uoffFolderStart = read_uint32(nOffset + offsetof(CFFILE, uoffFolderStart));
    result.iFolder = read_uint16(nOffset + offsetof(CFFILE, iFolder));
    result.date = read_uint16(nOffset + offsetof(CFFILE, date));
    result.time = read_uint16(nOffset + offsetof(CFFILE, time));
    result.attribs = read_uint16(nOffset + offsetof(CFFILE, attribs));

    return result;
}

QList<XArchive::RECORD> XCab::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<XArchive::RECORD> listResult;

    CFHEADER cfHeader = readCFHeader(0);
    qint64 nFileSize = getSize();

    // Parse folders
    QList<CFFOLDER> listFolders;
    qint64 nCurrentOffset = sizeof(CFHEADER);

    for (quint16 i = 0; i < cfHeader.cFolders; i++) {
        if (nCurrentOffset + (qint64)sizeof(CFFOLDER) > nFileSize) {
            break;
        }

        CFFOLDER cfFolder = readCFFolder(nCurrentOffset);
        listFolders.append(cfFolder);

        nCurrentOffset += sizeof(CFFOLDER);
    }

    // Parse files
    qint64 nFileOffset = cfHeader.coffFiles;

    for (quint16 i = 0; i < cfHeader.cFiles; i++) {
        if (nFileOffset + (qint64)sizeof(CFFILE) > nFileSize) {
            break;
        }

        CFFILE cfFile = readCFFILE(nFileOffset);
        QString sFileName = read_ansiString(nFileOffset + sizeof(CFFILE), 256);

        if (cfFile.iFolder < (quint16)listFolders.size()) {
            CFFOLDER cfFolder = listFolders.at(cfFile.iFolder);

            XArchive::RECORD record = {};

            record.spInfo.sRecordName = sFileName;
            record.spInfo.nUncompressedSize = cfFile.cbFile;
            record.nDataOffset = cfFolder.coffCabStart;
            record.nDataSize = _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData);

            // // Set compression method
            // if (cfFolder.typeCompress == 0x0000) {
            //     record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
            // } else if (cfFolder.typeCompress == 0x0001) {
            //     record.spInfo.compressMethod = COMPRESS_METHOD_MSZIP;
            // } else if (cfFolder.typeCompress == 0x0003) {
            //     record.spInfo.compressMethod = COMPRESS_METHOD_LZX;
            // } else {
            //     record.spInfo.compressMethod = COMPRESS_METHOD_UNKNOWN;
            // }

            listResult.append(record);
        }

        nFileOffset += sizeof(CFFILE) + sFileName.length() + 1;

        if (nLimit != -1 && listResult.size() >= nLimit) {
            break;
        }
    }

    return listResult;
}

XCab::CFHEADER XCab::readCFHeader(qint64 nOffset)
{
    CFHEADER result = {};

    result.signature[0] = read_uint8(nOffset + 0);
    result.signature[1] = read_uint8(nOffset + 1);
    result.signature[2] = read_uint8(nOffset + 2);
    result.signature[3] = read_uint8(nOffset + 3);
    result.reserved1 = read_uint32(nOffset + offsetof(CFHEADER, reserved1));
    result.cbCabinet = read_uint32(nOffset + offsetof(CFHEADER, cbCabinet));
    result.reserved2 = read_uint32(nOffset + offsetof(CFHEADER, reserved2));
    result.coffFiles = read_uint32(nOffset + offsetof(CFHEADER, coffFiles));
    result.reserved3 = read_uint32(nOffset + offsetof(CFHEADER, reserved3));
    result.versionMinor = read_uint8(nOffset + offsetof(CFHEADER, versionMinor));
    result.versionMajor = read_uint8(nOffset + offsetof(CFHEADER, versionMajor));
    result.cFolders = read_uint16(nOffset + offsetof(CFHEADER, cFolders));
    result.cFiles = read_uint16(nOffset + offsetof(CFHEADER, cFiles));
    result.flags = read_uint16(nOffset + offsetof(CFHEADER, flags));
    result.setID = read_uint16(nOffset + offsetof(CFHEADER, setID));
    result.iCabinet = read_uint16(nOffset + offsetof(CFHEADER, iCabinet));

    // if (result.flags & 0x0004)  // TODO const
    // {
    //     result.cbCFHeader = read_uint16(offsetof(CFHEADER, cbCFHeader));
    //     result.cbCFFolder = read_uint8(offsetof(CFHEADER, cbCFFolder));
    //     result.cbCFData = read_uint8(offsetof(CFHEADER, cbCFData));
    // }

    return result;
}

XCab::CFFOLDER XCab::readCFFolder(qint64 nOffset)
{
    CFFOLDER result = {};

    result.coffCabStart = read_uint32(nOffset + offsetof(CFFOLDER, coffCabStart));
    result.cCFData = read_uint16(nOffset + offsetof(CFFOLDER, cCFData));
    result.typeCompress = read_uint16(nOffset + offsetof(CFFOLDER, typeCompress));

    return result;
}

XCab::CFFOLDER XCab::_read_CFFOLDER(qint64 nOffset)
{
    CFFOLDER result = {};

    result.coffCabStart = read_uint32(nOffset + offsetof(CFFOLDER, coffCabStart));
    result.cCFData = read_uint16(nOffset + offsetof(CFFOLDER, cCFData));
    result.typeCompress = read_uint16(nOffset + offsetof(CFFOLDER, typeCompress));

    return result;
}

XCab::CFDATA XCab::readCFData(qint64 nOffset)
{
    CFDATA result = {};

    result.csum = read_uint32(nOffset + offsetof(CFDATA, csum));
    result.cbData = read_uint16(nOffset + offsetof(CFDATA, cbData));
    result.cbUncomp = read_uint16(nOffset + offsetof(CFDATA, cbUncomp));

    return result;
}

qint64 XCab::_getStreamSize(qint64 nOffset, qint32 nCount)
{
    qint64 nResult = 0;
    qint64 nCurrentOffset = nOffset;
    qint64 nFileSize = getSize();

    for (qint32 i = 0; i < nCount; i++) {
        if ((nCurrentOffset + (qint64)sizeof(CFDATA)) > nFileSize) {
            break;
        }

        CFDATA cfData = readCFData(nCurrentOffset);
        qint64 nBlockSize = (qint64)sizeof(CFDATA) + (qint64)cfData.cbData;

        nResult += nBlockSize;
        nCurrentOffset += nBlockSize;

        if (nCurrentOffset > nFileSize) {
            break;
        }
    }

    return nResult;
}

XBinary::FT XCab::getFileType()
{
    return FT_CAB;
}

QString XCab::getFileFormatExt()
{
    return "cab";
}

QString XCab::getFileFormatExtsString()
{
    return "CAB (*.cab)";
}

qint64 XCab::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    qint64 nResult = 0;

    nResult = readCFHeader(0).cbCabinet;  // TODO check mb _getRawSize !!!
    nResult = qMin(getSize(), nResult);

    return nResult;
}

QString XCab::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

qint32 XCab::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<QVariant> *pListValues, void *pUserData,
                          PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nRow)
    Q_UNUSED(locType)
    Q_UNUSED(nLocation)
    Q_UNUSED(dataRecordsOptions)
    Q_UNUSED(pListValues)
    Q_UNUSED(pUserData)
    Q_UNUSED(pPdStruct)
    // Not implemented for CAB
    return 0;
}

QString XCab::getMIMEString()
{
    return "application/vnd.ms-cab-compressed";
}

QList<XBinary::FPART> XCab::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    qint64 nFileSize = getSize();
    qint64 nFileFormatSize = getFileFormatSize(pPdStruct);

    CFHEADER cfHeader = readCFHeader(0);

    if (nFileParts & FILEPART_HEADER) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = qMin<qint64>(sizeof(CFHEADER), nFileSize);
        record.nVirtualAddress = -1;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_DATA) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nFileFormatSize;
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    qint64 nCurrentOffset = sizeof(CFHEADER);

    if ((nFileParts & FILEPART_HEADER) || (nFileParts & FILEPART_STREAM)) {
        // Regions: enumerate folders, files, and data blocks
        // 1) CFFOLDER area and per-folder entries (best-effort)
        if (cfHeader.cFolders) {
            for (quint32 i = 0; i < cfHeader.cFolders; ++i) {
                if ((nCurrentOffset + (qint64)sizeof(CFFOLDER)) > nFileSize) break;

                if (nFileParts & FILEPART_HEADER) {
                    FPART rec = {};
                    rec.filePart = FILEPART_HEADER;
                    rec.nFileOffset = nCurrentOffset;
                    rec.nFileSize = sizeof(CFFOLDER);
                    rec.nVirtualAddress = -1;
                    rec.sName = QString("CFFOLDER(%1)").arg(i);
                    listResult.append(rec);
                }

                if (nFileParts & FILEPART_STREAM) {
                    CFFOLDER cfFolder = readCFFolder(nCurrentOffset);

                    FPART rec = {};
                    rec.filePart = FILEPART_STREAM;
                    rec.nFileOffset = cfFolder.coffCabStart;
                    rec.nFileSize = _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData);
                    rec.nVirtualAddress = -1;
                    rec.sName = tr("Stream") + QString(" (%1)").arg(i);

                    if (cfFolder.typeCompress == 0x0000) {
                        rec.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE_CAB);
                    } else if (cfFolder.typeCompress == 0x0001) {
                        rec.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_MSZIP_CAB);
                    } else if (cfFolder.typeCompress == 0x0003) {
                        rec.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZX_CAB);
                    } else {
                        rec.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_UNKNOWN);
                    }

                    listResult.append(rec);
                }

                nCurrentOffset += sizeof(CFFOLDER);
            }
            // // Heuristic: assume folders area ends at coffFiles; derive start by subtracting cFolders*sizeof(CFFOLDER)
            // qint64 foldersEnd = qMin<qint64>(cfHeader.coffFiles, nFileSize);
            // qint64 foldersStart = qMax<qint64>(0, foldersEnd - (qint64)cfHeader.cFolders * (qint64)sizeof(CFFOLDER));

            // // Whole folders area
            // if ((foldersStart < foldersEnd) && (foldersEnd <= nFileSize)) {
            //     FPART area = {};
            //     area.filePart = FILEPART_REGION;
            //     area.nFileOffset = foldersStart;
            //     area.nFileSize = foldersEnd - foldersStart;
            //     area.nVirtualAddress = -1;
            //     area.sName = tr("CFFOLDER area");
            //     listResult.append(area);

            //     // Individual folder records (best-effort sequential)

            //     // Use folder records to enumerate CFDATA blocks
            //     for (quint32 i = 0; i < cfHeader.cFolders; ++i) {
            //         qint64 recOff = foldersStart + (qint64)i * (qint64)sizeof(CFFOLDER);
            //         if ((recOff + (qint64)sizeof(CFFOLDER)) > nFileSize) break;
            //         CFFOLDER fol = readCFFolder(recOff);

            //         qint64 dataOff = fol.coffCabStart;
            //         for (quint32 j = 0; j < fol.cCFData; ++j) {
            //             if ((dataOff + (qint64)sizeof(CFDATA)) > nFileSize) break;
            //             // CFDATA header entry
            //             FPART drec = {};
            //             drec.filePart = FILEPART_REGION;
            //             drec.nFileOffset = dataOff;
            //             drec.nFileSize = sizeof(CFDATA);
            //             drec.nVirtualAddress = -1;
            //             drec.sName = QString("%1(%2,%3)").arg("CFDATA").arg(i + 1).arg(j + 1);
            //             listResult.append(drec);

            //             // Advance to next block: header + compressed bytes
            //             CFDATA hdr = readCFData(dataOff);
            //             qint64 advance = (qint64)sizeof(CFDATA) + (qint64)hdr.cbData;
            //             if (advance <= 0) break;
            //             dataOff += advance;
            //         }
            //     }
            // }
        }

        // // 2) CFFILE table and per-file entries starting at coffFiles
        // if (cfHeader.coffFiles && cfHeader.cFiles) {
        //     // Whole files area (size unknown if names present); add per-record entries with fixed struct size
        //     for (quint32 i = 0; i < cfHeader.cFiles; ++i) {
        //         qint64 recOff = (qint64)cfHeader.coffFiles + (qint64)i * (qint64)sizeof(CFFILE);
        //         if ((recOff + (qint64)sizeof(CFFILE)) > nFileSize) break;
        //         FPART rec = {};
        //         rec.filePart = FILEPART_REGION;
        //         rec.nFileOffset = recOff;
        //         rec.nFileSize = sizeof(CFFILE);
        //         rec.nVirtualAddress = -1;
        //         rec.sName = QString("%1(%2)").arg("CFFILE").arg(i + 1);
        //         listResult.append(rec);
        //     }
        // }
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nFileFormatSize < nFileSize) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nFileFormatSize;
            record.nFileSize = nFileSize - nFileFormatSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

QList<XBinary::DATA_HEADER> XCab::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_CFHEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_CFHEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = sizeof(CFHEADER);
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFHEADER, signature), 4, "signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFHEADER, reserved1), 4, "reserved1", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cbCabinet), 4, "cbCabinet", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFHEADER, reserved2), 4, "reserved2", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, coffFiles), 4, "coffFiles", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFHEADER, reserved3), 4, "reserved3", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFHEADER, versionMinor), 1, "versionMinor", VT_UINT8, DRF_VERSION, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFHEADER, versionMajor), 1, "versionMajor", VT_UINT8, DRF_VERSION, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFolders), 2, "cFolders", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFiles), 2, "cFiles", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, flags), 2, "flags", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, setID), 2, "setID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, iCabinet), 2, "iCabinet", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                // Optional fields not handled in this example

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    CFHEADER cfHeader = readCFHeader(nStartOffset);
                    if (cfHeader.cFolders) {
                        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                        _dataHeadersOptions.nLocation = nStartOffset + sizeof(CFHEADER);
                        _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                        _dataHeadersOptions.nCount = cfHeader.cFolders;
                        _dataHeadersOptions.nSize = sizeof(CFFOLDER) * cfHeader.cFolders;
                        _dataHeadersOptions.nID = STRUCTID_CFFOLDER;
                        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                    }

                    if (cfHeader.coffFiles) {
                        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                        _dataHeadersOptions.nLocation = nStartOffset + cfHeader.coffFiles;
                        _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                        _dataHeadersOptions.nCount = cfHeader.cFiles;
                        _dataHeadersOptions.nSize = sizeof(CFFILE) * cfHeader.cFiles;  // TODO Names and extra fields
                        _dataHeadersOptions.nID = STRUCTID_CFFILE;
                        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                    }
                }
            } else if (dataHeadersOptions.nID == STRUCTID_CFFOLDER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = sizeof(CFFOLDER);
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFFOLDER, coffCabStart), 4, "coffCabStart", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFFOLDER, cCFData), 2, "cCFData", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFFOLDER, typeCompress), 2, "typeCompress", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            } else if (dataHeadersOptions.nID == STRUCTID_CFFILE) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

                dataHeader.nSize = sizeof(CFFILE);
                dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, cbFile), 4, "cbFile", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CFFILE, uoffFolderStart), 4, "uoffFolderStart", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, iFolder), 2, "iFolder", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, date), 2, "date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, time), 2, "time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, attribs), 2, "attribs", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                QString szName = read_ansiString(nStartOffset + sizeof(CFFILE), 256);  // Limit to 256 chars for safety)
                dataHeader.listRecords.append(
                    getDataRecord(sizeof(CFFILE), szName.size() + 1, "szName", VT_CHAR_ARRAY, DRF_VOLATILE, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::MAPMODE> XCab::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XCab::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

qint64 XCab::getImageSize()
{
    return (qint64)readCFHeader(0).cbCabinet;
}

XBinary::MODE XCab::getMode()
{
    return MODE_DATA;
}

QString XCab::getArch()
{
    return QString("Generic");
}

XBinary::ENDIAN XCab::getEndian()
{
    return ENDIAN_LITTLE;
}

// QList<XBinary::ARCHIVERECORD> XCab::getArchiveRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(nLimit)

//     QList<XBinary::ARCHIVERECORD> listResult;

//     CFHEADER cfHeader = readCFHeader(0);
//     qint64 nFileSize = getSize();

//     // Parse folders
//     qint64 nFolderOffset = sizeof(CFHEADER);
//     QList<CFFOLDER> listFolders;

//     for (quint16 i = 0; i < cfHeader.cFolders; i++) {
//         if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
//             break;
//         }

//         if ((nFolderOffset + (qint64)sizeof(CFFOLDER)) > nFileSize) {
//             break;
//         }

//         CFFOLDER cfFolder = readCFFolder(nFolderOffset);
//         listFolders.append(cfFolder);

//         nFolderOffset += sizeof(CFFOLDER);
//     }

//     // Parse files
//     qint64 nFileOffset = cfHeader.coffFiles;

//     for (quint16 i = 0; i < cfHeader.cFiles; i++) {
//         if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
//             break;
//         }

//         if ((nFileOffset + (qint64)sizeof(CFFILE)) > nFileSize) {
//             break;
//         }

//         CFFILE cfFile = readCFFILE(nFileOffset);
//         QString sFileName = read_ansiString(nFileOffset + sizeof(CFFILE), 256);

//         if (cfFile.iFolder < (quint16)listFolders.size()) {
//             CFFOLDER cfFolder = listFolders.at(cfFile.iFolder);

//             XBinary::ARCHIVERECORD record = {};

//             record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
//             record.nStreamOffset = cfFolder.coffCabStart;
//             record.nStreamSize = _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData);
//             record.nDecompressedSize = cfFile.cbFile;
//             record.nDecompressedOffset = cfFile.uoffFolderStart;

//             // Set compression method
//             if (cfFolder.typeCompress == 0x0000) {
//                 record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE_CAB);
//             } else if (cfFolder.typeCompress == 0x0001) {
//                 record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_MSZIP_CAB);
//             } else if (cfFolder.typeCompress == 0x0003) {
//                 record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_LZX_CAB);
//             } else {
//                 record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_UNKNOWN);
//             }

//             listResult.append(record);
//         }

//         nFileOffset += sizeof(CFFILE) + sFileName.length() + 1;
//     }

//     return listResult;
// }

// Helper methods for writing CAB structures
bool XCab::writeCFHeader(QIODevice *pDevice, const CFHEADER &cfHeader)
{
    if (!pDevice || !pDevice->isWritable()) {
        return false;
    }

    // Write signature
    if (pDevice->write((const char *)&cfHeader.signature, 4) != 4) return false;

    // Write reserved fields
    quint32 reserved = 0;
    if (pDevice->write((const char *)&reserved, 4) != 4) return false;  // reserved1
    if (pDevice->write((const char *)&cfHeader.cbCabinet, 4) != 4) return false;
    if (pDevice->write((const char *)&reserved, 4) != 4) return false;  // reserved2
    if (pDevice->write((const char *)&cfHeader.coffFiles, 4) != 4) return false;
    if (pDevice->write((const char *)&reserved, 4) != 4) return false;  // reserved3

    // Write version and counts
    if (pDevice->write((const char *)&cfHeader.versionMinor, 1) != 1) return false;
    if (pDevice->write((const char *)&cfHeader.versionMajor, 1) != 1) return false;
    if (pDevice->write((const char *)&cfHeader.cFolders, 2) != 2) return false;
    if (pDevice->write((const char *)&cfHeader.cFiles, 2) != 2) return false;
    if (pDevice->write((const char *)&cfHeader.flags, 2) != 2) return false;
    if (pDevice->write((const char *)&cfHeader.setID, 2) != 2) return false;
    if (pDevice->write((const char *)&cfHeader.iCabinet, 2) != 2) return false;

    return true;
}

bool XCab::writeCFFolder(QIODevice *pDevice, const CFFOLDER &cfFolder)
{
    if (!pDevice || !pDevice->isWritable()) {
        return false;
    }

    if (pDevice->write((const char *)&cfFolder.coffCabStart, 4) != 4) return false;
    if (pDevice->write((const char *)&cfFolder.cCFData, 2) != 2) return false;
    if (pDevice->write((const char *)&cfFolder.typeCompress, 2) != 2) return false;

    return true;
}

bool XCab::writeCFFILE(QIODevice *pDevice, const CFFILE &cfFile, const QString &sFileName)
{
    if (!pDevice || !pDevice->isWritable()) {
        return false;
    }

    if (pDevice->write((const char *)&cfFile.cbFile, 4) != 4) return false;
    if (pDevice->write((const char *)&cfFile.uoffFolderStart, 4) != 4) return false;
    if (pDevice->write((const char *)&cfFile.iFolder, 2) != 2) return false;
    if (pDevice->write((const char *)&cfFile.date, 2) != 2) return false;
    if (pDevice->write((const char *)&cfFile.time, 2) != 2) return false;
    if (pDevice->write((const char *)&cfFile.attribs, 2) != 2) return false;

    // Write filename as ANSI string
    QByteArray baFileName = sFileName.toLocal8Bit();
    if (pDevice->write(baFileName.constData(), baFileName.size() + 1) != baFileName.size() + 1) return false;

    return true;
}

bool XCab::writeCFData(QIODevice *pDevice, const CFDATA &cfData, const QByteArray &baData)
{
    if (!pDevice || !pDevice->isWritable()) {
        return false;
    }

    if (pDevice->write((const char *)&cfData.csum, 4) != 4) return false;
    if (pDevice->write((const char *)&cfData.cbData, 2) != 2) return false;
    if (pDevice->write((const char *)&cfData.cbUncomp, 2) != 2) return false;
    if (pDevice->write(baData.constData(), baData.size()) != baData.size()) return false;

    return true;
}

// Streaming unpacking API implementation
bool XCab::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)
    
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState) {
        return false;
    }

    // Read CAB header
    CFHEADER cfHeader = readCFHeader(0);
    if (cfHeader.signature[0] != 'M' || cfHeader.signature[1] != 'S' || cfHeader.signature[2] != 'C' || cfHeader.signature[3] != 'F') {
        return false;  // Invalid CAB signature
    }

    // Create unpack context
    CAB_UNPACK_CONTEXT *pContext = new CAB_UNPACK_CONTEXT;
    pContext->nCurrentFileIndex = 0;

    // Parse folders
    qint64 nFolderOffset = sizeof(CFHEADER);
    qint64 nFileSize = getSize();

    for (quint16 i = 0; i < cfHeader.cFolders; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            delete pContext;
            return false;
        }

        if ((nFolderOffset + (qint64)sizeof(CFFOLDER)) > nFileSize) {
            delete pContext;
            return false;
        }

        CFFOLDER cfFolder = readCFFolder(nFolderOffset);
        pContext->listFolders.append(cfFolder);
        nFolderOffset += sizeof(CFFOLDER);
    }

    // Parse file offsets
    qint64 nFileOffset = cfHeader.coffFiles;

    for (quint16 i = 0; i < cfHeader.cFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            delete pContext;
            return false;
        }

        if ((nFileOffset + (qint64)sizeof(CFFILE)) > nFileSize) {
            delete pContext;
            return false;
        }

        pContext->listFileOffsets.append(nFileOffset);

        // Skip to next file entry
        CFFILE cfFile = readCFFILE(nFileOffset);
        QString sFileName = read_ansiString(nFileOffset + sizeof(CFFILE), 256);
        nFileOffset += sizeof(CFFILE) + sFileName.length() + 1;
    }

    // Initialize state
    pState->nCurrentOffset = cfHeader.coffFiles;  // Start at first file
    pState->nTotalSize = getSize();
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = cfHeader.cFiles;
    pState->pContext = pContext;

    return true;
}

XBinary::ARCHIVERECORD XCab::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
    qint64 nFileOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);

    CFFILE cfFile = readCFFILE(nFileOffset);
    QString sFileName = read_ansiString(nFileOffset + sizeof(CFFILE), 256);

    if (cfFile.iFolder < (quint16)pContext->listFolders.size()) {
        CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);

        result.nStreamOffset = cfFolder.coffCabStart;
        result.nStreamSize = _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData);
        result.nDecompressedOffset = cfFile.uoffFolderStart;
        result.nDecompressedSize = cfFile.cbFile;

        result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)cfFile.cbFile);

        // Set compression method
        if (cfFolder.typeCompress == 0x0000) {
            result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE_CAB);
        } else if (cfFolder.typeCompress == 0x0001) {
            result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_MSZIP_CAB);
        } else if (cfFolder.typeCompress == 0x0003) {
            result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_LZX_CAB);
        } else {
            result.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_UNKNOWN);
        }

        // Convert DOS date/time to QDateTime
        // QDateTime dateTime = XBinary::dosDateTimeToQt((quint32)cfFile.date | ((quint32)cfFile.time << 16));
        // result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);

        result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)cfFile.attribs);
    }

    return result;
}

bool XCab::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
    qint64 nFileOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);

    CFFILE cfFile = readCFFILE(nFileOffset);

    if (cfFile.iFolder >= (quint16)pContext->listFolders.size()) {
        return false;
    }

    CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);

    // For now, only support STORE method (no compression)
    if (cfFolder.typeCompress != 0x0000) {
        return false;  // Compressed CAB files not yet supported
    }

    // Calculate offset within the compressed stream
    qint64 nDataOffset = cfFolder.coffCabStart;
    qint64 nCurrentUncompressedOffset = 0;
    qint64 nTargetOffset = cfFile.uoffFolderStart;
    qint64 nRemainingSize = cfFile.cbFile;

    // Find the correct data block
    for (quint16 i = 0; i < cfFolder.cCFData && nRemainingSize > 0; i++) {
        if ((nDataOffset + (qint64)sizeof(CFDATA)) > getSize()) {
            break;
        }

        CFDATA cfData = readCFData(nDataOffset);

        if (nCurrentUncompressedOffset + cfData.cbUncomp <= nTargetOffset) {
            // Skip this block entirely
            nCurrentUncompressedOffset += cfData.cbUncomp;
            nDataOffset += sizeof(CFDATA) + cfData.cbData;
            continue;
        }

        // This block contains data we need
        qint64 nBlockStart = nTargetOffset - nCurrentUncompressedOffset;
        qint64 nCopySize = qMin(nRemainingSize, (qint64)cfData.cbUncomp - nBlockStart);

        if (nCopySize > 0) {
            qint64 nSourceOffset = nDataOffset + sizeof(CFDATA) + nBlockStart;
            if (!copyDeviceMemory(getDevice(), nSourceOffset, pDevice, 0, nCopySize)) {
                return false;
            }
        }

        nTargetOffset += nCopySize;
        nRemainingSize -= nCopySize;
        nCurrentUncompressedOffset += cfData.cbUncomp;
        nDataOffset += sizeof(CFDATA) + cfData.cbData;
    }

    return nRemainingSize == 0;
}

bool XCab::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
        return false;
    }

    if (pState->nCurrentIndex + 1 >= pState->nNumberOfRecords) {
        return false;  // No more records
    }

    CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);
        return true;
    }

    return false;
}

bool XCab::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}

// Streaming packing API implementation
bool XCab::initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState) {
        return false;
    }

    pState->pDevice = pDevice;
    pState->mapProperties = mapProperties;

    // Create pack context
    CAB_PACK_CONTEXT *pContext = new CAB_PACK_CONTEXT;
    pContext->nCurrentOffset = 0;
    pContext->nCompressionType = 0x0000;  // STORE method by default

    // Write CAB signature
    QByteArray baSignature("MSCF");
    baSignature.append(4, '\0');     // reserved1
    baSignature.append(4, '\0');     // cbCabinet (will be updated later)
    baSignature.append(4, '\0');     // reserved2
    baSignature.append(4, '\0');     // coffFiles (will be updated later)
    baSignature.append(4, '\0');     // reserved3
    baSignature.append((char)0x03);  // versionMinor
    baSignature.append((char)0x01);  // versionMajor
    baSignature.append(2, '\0');     // cFolders (will be updated later)
    baSignature.append(2, '\0');     // cFiles (will be updated later)
    baSignature.append(2, '\0');     // flags
    baSignature.append(2, '\0');     // setID
    baSignature.append(2, '\0');     // iCabinet

    if (pState->pDevice->write(baSignature) != baSignature.size()) {
        delete pContext;
        return false;
    }

    pContext->nCurrentOffset = baSignature.size();

    // Initialize state
    pState->nCurrentOffset = pContext->nCurrentOffset;
    pState->nNumberOfRecords = 0;
    pState->pContext = pContext;

    return true;
}

bool XCab::addFile(PACK_STATE *pState, const QString &sFileName, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice) {
        return false;
    }

    CAB_PACK_CONTEXT *pContext = (CAB_PACK_CONTEXT *)pState->pContext;

    // Open file
    QFile file(sFileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    // Get file info
    QFileInfo fileInfo(sFileName);
    qint64 nFileSize = fileInfo.size();
    QString sBaseName = fileInfo.fileName();

    // Read file data
    QByteArray baFileData = file.readAll();
    file.close();

    if (baFileData.size() != nFileSize) {
        return false;
    }

    // Create file entry
    CFFILE cfFile = {};
    cfFile.cbFile = nFileSize;
    cfFile.uoffFolderStart = 0;  // Will be updated when folder is finalized
    cfFile.iFolder = 0;          // Single folder for simplicity
    cfFile.attribs = 0x20;       // Archive attribute

    // Convert file time to DOS format
    QDateTime dateTime = fileInfo.lastModified();
    // quint32 nDosTime = XBinary::qtToDosDateTime(dateTime);
    // cfFile.date = nDosTime & 0xFFFF;
    // cfFile.time = (nDosTime >> 16) & 0xFFFF;

    // For STORE method, create a single data block
    CFDATA cfData = {};
    cfData.csum = 0;  // TODO: Calculate checksum
    cfData.cbData = nFileSize;
    cfData.cbUncomp = nFileSize;

    // Write data block
    qint64 nDataOffset = pState->nCurrentOffset;
    if (!writeCFData(pState->pDevice, cfData, baFileData)) {
        return false;
    }

    pState->nCurrentOffset += sizeof(CFDATA) + nFileSize;

    // Add to context
    pContext->listFiles.append(cfFile);
    pContext->listDataBlocks.append(baFileData);

    // Update folder info
    if (pContext->listFolders.isEmpty()) {
        CFFOLDER cfFolder = {};
        cfFolder.coffCabStart = nDataOffset;
        cfFolder.cCFData = 1;
        cfFolder.typeCompress = pContext->nCompressionType;
        pContext->listFolders.append(cfFolder);
    } else {
        pContext->listFolders[0].cCFData++;
    }

    pState->nNumberOfRecords++;

    return true;
}

bool XCab::addDevice(PACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext || !pState->pDevice || !pDevice) {
        return false;
    }

    CAB_PACK_CONTEXT *pContext = (CAB_PACK_CONTEXT *)pState->pContext;

    // Get device size
    qint64 nDeviceSize = pDevice->size();
    if (nDeviceSize < 0) {
        return false;
    }

    // Read device data
    pDevice->seek(0);
    QByteArray baDeviceData = pDevice->readAll();

    if (baDeviceData.size() != nDeviceSize) {
        return false;
    }

    // Create file entry with default name
    QString sDefaultName = QString("stream_%1.dat").arg(pState->nNumberOfRecords);

    CFFILE cfFile = {};
    cfFile.cbFile = nDeviceSize;
    cfFile.uoffFolderStart = 0;  // Will be updated
    cfFile.iFolder = 0;
    cfFile.attribs = 0x20;

    // Current time for device
    QDateTime dateTime = QDateTime::currentDateTime();
    // quint32 nDosTime = XBinary::qtToDosDateTime(dateTime);
    // cfFile.date = nDosTime & 0xFFFF;
    // cfFile.time = (nDosTime >> 16) & 0xFFFF;

    // Create data block
    CFDATA cfData = {};
    cfData.csum = 0;  // TODO: Calculate checksum
    cfData.cbData = nDeviceSize;
    cfData.cbUncomp = nDeviceSize;

    // Write data block
    qint64 nDataOffset = pState->nCurrentOffset;
    if (!writeCFData(pState->pDevice, cfData, baDeviceData)) {
        return false;
    }

    pState->nCurrentOffset += sizeof(CFDATA) + nDeviceSize;

    // Add to context
    pContext->listFiles.append(cfFile);
    pContext->listDataBlocks.append(baDeviceData);

    // Update folder
    if (pContext->listFolders.isEmpty()) {
        CFFOLDER cfFolder = {};
        cfFolder.coffCabStart = nDataOffset;
        cfFolder.cCFData = 1;
        cfFolder.typeCompress = pContext->nCompressionType;
        pContext->listFolders.append(cfFolder);
    } else {
        pContext->listFolders[0].cCFData++;
    }

    pState->nNumberOfRecords++;

    return true;
}

bool XCab::addFolder(PACK_STATE *pState, const QString &sDirectoryPath, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pContext) {
        return false;
    }

    // Check if directory exists
    if (!XBinary::isDirectoryExists(sDirectoryPath)) {
        return false;
    }

    // Enumerate all files in directory
    QList<QString> listFiles;
    XBinary::findFiles(sDirectoryPath, &listFiles, true, 0, pPdStruct);

    // Add each file
    for (qint32 i = 0; i < listFiles.count() && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        QString sFilePath = listFiles.at(i);
        QFileInfo fileInfo(sFilePath);

        // Skip directories
        if (fileInfo.isDir()) {
            continue;
        }

        // Add file to archive
        if (!addFile(pState, sFilePath, pPdStruct)) {
            return false;
        }
    }

    return true;
}

bool XCab::finishPack(PACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext || !pState->pDevice) {
        return false;
    }

    CAB_PACK_CONTEXT *pContext = (CAB_PACK_CONTEXT *)pState->pContext;

    // Write folders
    qint64 nFoldersOffset = pState->nCurrentOffset;
    for (const CFFOLDER &cfFolder : pContext->listFolders) {
        if (!writeCFFolder(pState->pDevice, cfFolder)) {
            delete pContext;
            return false;
        }
        pState->nCurrentOffset += sizeof(CFFOLDER);
    }

    // Write files
    qint64 nFilesOffset = pState->nCurrentOffset;
    for (qint32 i = 0; i < pContext->listFiles.size(); i++) {
        const CFFILE &cfFile = pContext->listFiles.at(i);
        QString sFileName = QString("file_%1.dat").arg(i);  // TODO: Store actual filenames

        if (!writeCFFILE(pState->pDevice, cfFile, sFileName)) {
            delete pContext;
            return false;
        }

        pState->nCurrentOffset += sizeof(CFFILE) + sFileName.length() + 1;
    }

    // Update header with correct offsets and counts
    pState->pDevice->seek(0);

    CFHEADER cfHeader = {};
    cfHeader.signature[0] = 'M';
    cfHeader.signature[1] = 'S';
    cfHeader.signature[2] = 'C';
    cfHeader.signature[3] = 'F';
    cfHeader.cbCabinet = pState->nCurrentOffset;
    cfHeader.coffFiles = nFilesOffset;
    cfHeader.versionMinor = 0x03;
    cfHeader.versionMajor = 0x01;
    cfHeader.cFolders = pContext->listFolders.size();
    cfHeader.cFiles = pContext->listFiles.size();
    cfHeader.flags = 0;
    cfHeader.setID = 0;
    cfHeader.iCabinet = 0;

    if (!writeCFHeader(pState->pDevice, cfHeader)) {
        delete pContext;
        return false;
    }

    // Clean up
    delete pContext;
    pState->pContext = nullptr;

    return true;
}

// bool XCab::packFolderToDevice(const QString &sFolderName, QIODevice *pDevice, void *pOptions, PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(sFolderName)
//     Q_UNUSED(pDevice)
//     Q_UNUSED(pOptions)
//     Q_UNUSED(pPdStruct)

//     // TODO: Implement CAB archive creation
//     // This is a complex task that requires:
//     // 1. Collecting all files from the folder
//     // 2. Compressing files using MSZIP compression
//     // 3. Writing CFHEADER, CFFOLDER, CFFILE, and CFDATA structures
//     // 4. Computing checksums

//     return false;
// }
