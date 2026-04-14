/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "Algos/xdeflatedecoder.h"
#include "Algos/xlzhdecoder.h"

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

bool XCab::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCab xcab(pDevice);

    return xcab.isValid();
}

QString XCab::getVersion()
{
    return QString("%1.%2").arg(read_uint8(25)).arg(read_uint8(24), 2, 10, QChar('0'));
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

QString XCab::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XCab::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
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
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE_CAB);
                    } else if (cfFolder.typeCompress == 0x0001) {
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MSZIP_CAB);
                    } else if (cfFolder.typeCompress == 0x0003) {
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZX_CAB);
                    } else {
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
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

    qint64 nFileSize = getSize();

    if (nFileSize < (qint64)sizeof(CFHEADER)) {
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
    pContext->nCbCFHeader = 0;
    pContext->nCbCFFolder = 0;
    pContext->nCbCFData = 0;

    // Handle reserved fields (flag 0x0004 = cfhdrRESERVE_PRESENT)
    qint64 nFolderOffset = sizeof(CFHEADER);

    if (cfHeader.flags & 0x0004) {
        if ((nFolderOffset + 4) > nFileSize) {
            delete pContext;
            return false;
        }
        pContext->nCbCFHeader = read_uint16(nFolderOffset);
        pContext->nCbCFFolder = read_uint8(nFolderOffset + 2);
        pContext->nCbCFData = read_uint8(nFolderOffset + 3);
        nFolderOffset += 4 + pContext->nCbCFHeader;  // Skip cbCFHeader + cbCFFolder + cbCFData + abReserve
    }

    // Handle optional previous cabinet name (flag 0x0001 = cfhdrPREV_CABINET)
    if (cfHeader.flags & 0x0001) {
        QString sPrevCab = read_ansiString(nFolderOffset, 256);
        nFolderOffset += sPrevCab.length() + 1;
        QString sPrevDisk = read_ansiString(nFolderOffset, 256);
        nFolderOffset += sPrevDisk.length() + 1;
    }

    // Handle optional next cabinet name (flag 0x0002 = cfhdrNEXT_CABINET)
    if (cfHeader.flags & 0x0002) {
        QString sNextCab = read_ansiString(nFolderOffset, 256);
        nFolderOffset += sNextCab.length() + 1;
        QString sNextDisk = read_ansiString(nFolderOffset, 256);
        nFolderOffset += sNextDisk.length() + 1;
    }

    // Parse folders (each CFFOLDER may have per-folder reserved area)
    qint64 nFolderStructSize = (qint64)sizeof(CFFOLDER) + pContext->nCbCFFolder;

    for (quint16 i = 0; i < cfHeader.cFolders; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            delete pContext;
            return false;
        }

        if ((nFolderOffset + nFolderStructSize) > nFileSize) {
            delete pContext;
            return false;
        }

        CFFOLDER cfFolder = readCFFolder(nFolderOffset);
        pContext->listFolders.append(cfFolder);
        nFolderOffset += nFolderStructSize;
    }

    // Parse file offsets starting at coffFiles
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

        // Skip to next file entry (variable-length due to null-terminated filename)
        QString sFileName = read_ansiString(nFileOffset + sizeof(CFFILE), 256);
        nFileOffset += sizeof(CFFILE) + sFileName.length() + 1;
    }

    // Initialize state
    pState->nCurrentOffset = cfHeader.coffFiles;
    pState->nTotalSize = nFileSize;
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

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)cfFile.cbFile);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)cfFile.attribs);

    // Convert DOS date/time to QDateTime
    quint32 nDosDate = (quint32)cfFile.date;
    quint32 nDosTime = (quint32)cfFile.time;
    qint32 nYear = ((nDosDate >> 9) & 0x7F) + 1980;
    qint32 nMonth = (nDosDate >> 5) & 0x0F;
    qint32 nDay = nDosDate & 0x1F;
    qint32 nHour = (nDosTime >> 11) & 0x1F;
    qint32 nMinute = (nDosTime >> 5) & 0x3F;
    qint32 nSecond = (nDosTime & 0x1F) * 2;

    QDate date(nYear, nMonth, nDay);
    QTime time(nHour, nMinute, nSecond);

    if (date.isValid() && time.isValid()) {
        QDateTime dateTime(date, time, Qt::UTC);
        result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
    }

    if (cfFile.iFolder < (quint16)pContext->listFolders.size()) {
        CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);

        result.nStreamOffset = cfFolder.coffCabStart;
        result.nStreamSize = _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData);

        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);
        result.mapProperties.insert(FPART_PROP_TYPE, (quint32)cfFolder.typeCompress);
        result.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)cfFile.iFolder);
        result.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, (qint64)cfFile.uoffFolderStart);
        result.mapProperties.insert(FPART_PROP_OPTHEADER_OFFSET, (qint64)cfFile.uoffFolderStart);
        result.mapProperties.insert(FPART_PROP_OPTHEADER_SIZE, (qint64)pContext->nCbCFData);

        // Set compression method
        quint16 nCompressType = cfFolder.typeCompress & 0x000F;

        if (nCompressType == 0x0000) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE_CAB);
        } else if (nCompressType == 0x0001) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MSZIP_CAB);
        } else if (nCompressType == 0x0003) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZX_CAB);
        } else {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
        }
    }

    return result;
}

// bool XCab::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
// {
//     if (!pState || !pState->pContext || !pDevice) {
//         return false;
//     }

//     if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
//         return false;
//     }

//     PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
//     if (!pPdStruct) {
//         pPdStruct = &pdStructEmpty;
//     }

//     CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
//     qint64 nFileOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);

//     CFFILE cfFile = readCFFILE(nFileOffset);

//     if (cfFile.iFolder >= (quint16)pContext->listFolders.size()) {
//         return false;
//     }

//     CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);

//     // Determine compression type (lower 4 bits)
//     quint16 nCompressionType = cfFolder.typeCompress & 0x000F;
//     qint64 nTotalSize = getSize();
//     // Per-datablock reserved area size
//     qint64 nDataReservedSize = (qint64)pContext->nCbCFData;

//     if (nCompressionType == 0x0000) {
//         // STORE method (no compression) - direct copy from data blocks
//         qint64 nDataOffset = cfFolder.coffCabStart;
//         qint64 nCurrentUncompressedOffset = 0;
//         qint64 nTargetOffset = cfFile.uoffFolderStart;
//         qint64 nRemainingSize = cfFile.cbFile;
//         qint64 nWriteOffset = 0;

//         for (quint16 i = 0; (i < cfFolder.cCFData) && (nRemainingSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
//             if ((nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize) > nTotalSize) {
//                 break;
//             }

//             CFDATA cfData = readCFData(nDataOffset);
//             qint64 nPayloadOffset = nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize;

//             if (nCurrentUncompressedOffset + cfData.cbUncomp <= nTargetOffset) {
//                 // Skip this block entirely
//                 nCurrentUncompressedOffset += cfData.cbUncomp;
//                 nDataOffset = nPayloadOffset + cfData.cbData;
//                 continue;
//             }

//             // This block contains data we need
//             qint64 nBlockStart = nTargetOffset - nCurrentUncompressedOffset;
//             qint64 nCopySize = qMin(nRemainingSize, (qint64)cfData.cbUncomp - nBlockStart);

//             if (nCopySize > 0) {
//                 qint64 nSourceOffset = nPayloadOffset + nBlockStart;

//                 if ((nSourceOffset + nCopySize) > nTotalSize) {
//                     return false;
//                 }

//                 if (!copyDeviceMemory(getDevice(), nSourceOffset, pDevice, nWriteOffset, nCopySize)) {
//                     return false;
//                 }

//                 nWriteOffset += nCopySize;
//             }

//             nTargetOffset += nCopySize;
//             nRemainingSize -= nCopySize;
//             nCurrentUncompressedOffset += cfData.cbUncomp;
//             nDataOffset = nPayloadOffset + cfData.cbData;
//         }

//         return (nRemainingSize == 0);
//     } else if (nCompressionType == 0x0001) {
//         // MSZIP method (DEFLATE with 2-byte "CK" signature per block)
//         // Use folder cache to avoid re-decompressing the same folder for multiple files
//         quint16 nFolderIndex = cfFile.iFolder;

//         if (!pContext->mapFolderCache.contains(nFolderIndex)) {
//             // Decompress entire folder into cache
//             QByteArray baFolderData;

//             qint64 nDataOffset = cfFolder.coffCabStart;

//             for (quint16 i = 0; (i < cfFolder.cCFData) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
//                 if ((nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize) > nTotalSize) {
//                     break;
//                 }

//                 CFDATA cfData = readCFData(nDataOffset);
//                 qint64 nPayloadOffset = nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize;

//                 if (cfData.cbData < 2) {
//                     return false;  // Invalid block (needs at least 2-byte "CK" signature)
//                 }

//                 // Verify MSZIP signature "CK" (0x43 0x4B)
//                 quint8 nSig0 = read_uint8(nPayloadOffset);
//                 quint8 nSig1 = read_uint8(nPayloadOffset + 1);

//                 if (nSig0 != 0x43 || nSig1 != 0x4B) {
//                     return false;  // Invalid MSZIP block signature
//                 }

//                 qint64 nCompressedDataOffset = nPayloadOffset + 2;
//                 qint64 nCompressedDataSize = cfData.cbData - 2;

//                 if (nCompressedDataSize <= 0) {
//                     return false;
//                 }

//                 if ((nCompressedDataOffset + nCompressedDataSize) > nTotalSize) {
//                     return false;
//                 }

//                 QByteArray baCompressedData = read_array(nCompressedDataOffset, nCompressedDataSize);

//                 if (baCompressedData.size() != nCompressedDataSize) {
//                     return false;
//                 }

//                 QBuffer bufferCompressed(&baCompressedData);
//                 if (!bufferCompressed.open(QIODevice::ReadOnly)) {
//                     return false;
//                 }

//                 QByteArray baUncompressedBlock;
//                 QBuffer bufferUncompressed(&baUncompressedBlock);
//                 if (!bufferUncompressed.open(QIODevice::WriteOnly)) {
//                     bufferCompressed.close();
//                     return false;
//                 }

//                 DATAPROCESS_STATE decompressState = {};
//                 decompressState.pDeviceInput = &bufferCompressed;
//                 decompressState.pDeviceOutput = &bufferUncompressed;
//                 decompressState.nInputOffset = 0;
//                 decompressState.nInputLimit = nCompressedDataSize;
//                 decompressState.nProcessedOffset = 0;
//                 decompressState.nProcessedLimit = cfData.cbUncomp;

//                 bool bDecompressResult = XDeflateDecoder::decompress(&decompressState, pPdStruct);

//                 bufferCompressed.close();
//                 bufferUncompressed.close();

//                 if (!bDecompressResult) {
//                     return false;
//                 }

//                 if (baUncompressedBlock.size() != (qint32)cfData.cbUncomp) {
//                     return false;
//                 }

//                 baFolderData.append(baUncompressedBlock);
//                 nDataOffset = nPayloadOffset + cfData.cbData;
//             }

//             pContext->mapFolderCache.insert(nFolderIndex, baFolderData);
//         }

//         // Extract file data from the cached folder
//         const QByteArray &baFolderData = pContext->mapFolderCache.value(nFolderIndex);

//         if (baFolderData.size() < (qint64)cfFile.uoffFolderStart + (qint64)cfFile.cbFile) {
//             return false;
//         }

//         qint64 nBytesWritten = pDevice->write(baFolderData.constData() + cfFile.uoffFolderStart, cfFile.cbFile);
//         return (nBytesWritten == (qint64)cfFile.cbFile);
//     } else if (nCompressionType == 0x0003) {
//         // LZX method - not yet implemented (requires specialized LZX decoder)
//         // LZX window size is encoded in upper bits: (typeCompress >> 8) & 0x1F
//         return false;
//     }

//     return false;  // Unsupported compression type
// }

bool XCab::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;

        pState->nCurrentIndex++;

        if (pState->nCurrentIndex < pState->nNumberOfRecords) {
            pState->nCurrentOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);
            bResult = true;
        }
    }

    return bResult;
}

bool XCab::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
        pContext->mapFolderCache.clear();
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}

QList<QString> XCab::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append("'MSCF'");
    return listResult;
}

XBinary *XCab::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCab(pDevice);
}

