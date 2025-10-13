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
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<XArchive::RECORD> listResult;

    qint64 nOffset = 0;  // TODO

    CFHEADER cfHeader = readCFHeader(0);

    nOffset += sizeof(CFHEADER) - 4;

    // if (cfHeader.flags & 0x0004)  // TODO const
    // {
    //     nOffset += 4;

    //     nOffset += cfHeader.cbCFHeader;
    // }

    // CFFOLDER cfFolder=readCFFolder(nOffset);

    // TODO

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

qint64 XCab::getNumberOfArchiveRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    qint64 nResult = 0;

    nResult = read_uint16(offsetof(CFHEADER, cFiles));

    return nResult;
}
