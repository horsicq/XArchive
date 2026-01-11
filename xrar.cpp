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
#include "xrar.h"
#include "Algos/xrardecoder.h"

XBinary::XCONVERT _TABLE_XRAR_STRUCTID[] = {{XRar::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                            {XRar::STRUCTID_RAR14_SIGNATURE, "RAR14_SIGNATURE", QString("RAR 1.4 signature")},
                                            {XRar::STRUCTID_RAR40_SIGNATURE, "RAR40_SIGNATURE", QString("RAR 4.0 signature")},
                                            {XRar::STRUCTID_RAR50_SIGNATURE, "RAR50_SIGNATURE", QString("RAR 5.0 signature)")},
                                            {XRar::STRUCTID_RAR14_HEADER, "RAR14_HEADER", QString("RAR 1.4 header")},
                                            {XRar::STRUCTID_RAR40_HEADER, "RAR40_HEADER", QString("RAR 4.0 header")},
                                            {XRar::STRUCTID_RAR50_HEADER, "RAR50_HEADER", QString("RAR 5.0 header")}};

XRar::XRar(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XRar::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > 20)  // TODO
    {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "'RE~^'", 0, pPdStruct) || compareSignature(&memoryMap, "'Rar!'1A07", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XRar::isValid(QIODevice *pDevice)
{
    XRar xrar(pDevice);

    return xrar.isValid();
}

QString XRar::getVersion()
{
    return getFileFormatInfo(nullptr).sVersion;
}

QString XRar::getFileFormatExt()
{
    return "rar";
}

QString XRar::getFileFormatExtsString()
{
    return "RAR (*.rar)";
}

qint64 XRar::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return getFileFormatInfo(pPdStruct).nSize;
}

QString XRar::blockType4ToString(BLOCKTYPE4 type)
{
    QString sResult;

    switch (type) {
        case BLOCKTYPE4_MARKER: sResult = QString("Marker block"); break;
        case BLOCKTYPE4_ARCHIVE: sResult = QString("Archive header"); break;
        case BLOCKTYPE4_FILE: sResult = QString("File header"); break;
        case BLOCKTYPE4_COMMENT: sResult = QString("Comment header"); break;
        case BLOCKTYPE4_EXTRA: sResult = QString("Extra information"); break;
        case BLOCKTYPE4_SUBBLOCK: sResult = QString("Subblock"); break;
        case BLOCKTYPE4_RECOVERY: sResult = QString("Recovery record"); break;
        case BLOCKTYPE4_AUTH: sResult = QString("Archive authentication"); break;
        case BLOCKTYPE4_SUBBLOCK_NEW: sResult = QString("Subblock"); break;
        case BLOCKTYPE4_END: sResult = QString("End of archive"); break;
        default: sResult = QString("Unknown (%1)").arg(type, 0, 16);
    }

    return sResult;
}

QString XRar::headerType5ToString(HEADERTYPE5 type)
{
    QString sResult;

    switch (type) {
        case HEADERTYPE5_MAIN: sResult = QString("Main archive header"); break;
        case HEADERTYPE5_FILE: sResult = QString("File header"); break;
        case HEADERTYPE5_SERVICE: sResult = QString("Service header"); break;
        case HEADERTYPE5_ENCRYPTION: sResult = QString("Archive encryption header"); break;
        case HEADERTYPE5_ENDARC: sResult = QString("End of archive header"); break;
        default: sResult = QString("Unknown (%1)").arg(type, 0, 16); break;
    }

    return sResult;
}

QString XRar::getMIMEString()
{
    return "application/x-rar-compressed";
}

QString XRar::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XRAR_STRUCTID, sizeof(_TABLE_XRAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

qint32 XRar::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords,
                          void *pUserData, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(locType)
    Q_UNUSED(nLocation)
    Q_UNUSED(dataRecordsOptions)
    Q_UNUSED(pUserData)

    qint32 nResult = 0;

    if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_RAR40_HEADER) {
        XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);

        qint64 nStartOffset = locationToOffset(dataRecordsOptions.pMemoryMap, locType, nLocation);

        quint8 nType = read_uint8(nStartOffset + 2);
        nResult = read_uint16(nStartOffset + 5);

        if (nType == BLOCKTYPE4_FILE) {
            FILEBLOCK4 fileBlock4 = readFileBlock4(nStartOffset);

            qint64 nFileSize = fileBlock4.packSize;
            nFileSize |= ((qint64)fileBlock4.highPackSize) << 32;

            nResult += nFileSize;
        }
    } else {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
    }

    return nResult;
}

QList<XBinary::FPART> XRar::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    qint32 nInternVersion = getInternVersion(pPdStruct);

    if (nInternVersion == 0) {
        return listResult;
    }

    qint64 nFileHeaderSize = 0;
    if (nInternVersion == 4) {
        nFileHeaderSize = 7;
    } else if (nInternVersion == 5) {
        nFileHeaderSize = 8;
    } else if (nInternVersion == 1) {
        nFileHeaderSize = 4;
    }

    qint64 nMaxOffset = 0;

    if (nFileParts & FILEPART_SIGNATURE) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_SIGNATURE;
        record.nFileOffset = 0;
        record.nFileSize = nFileHeaderSize;
        record.nVirtualAddress = -1;
        record.sName = tr("Signature");

        listResult.append(record);
    }

    if (nFileParts & (FILEPART_HEADER | FILEPART_STREAM | FILEPART_DATA | FILEPART_OVERLAY)) {
        qint64 nCurrentOffset = nFileHeaderSize;
        qint64 nTotalSize = getSize();

        if (nInternVersion == 4) {
            while (isPdStructNotCanceled(pPdStruct) && (nLimit == -1 || listResult.size() < nLimit)) {
                if (nCurrentOffset > nTotalSize - 7) {
                    break;
                }

                GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

                if (genericBlock.nType >= 0x72 && genericBlock.nType <= 0x7B) {
                    if (nFileParts & FILEPART_HEADER) {
                        XBinary::FPART record = {};
                        record.filePart = FILEPART_HEADER;
                        record.nFileOffset = nCurrentOffset;
                        record.nFileSize = genericBlock.nHeaderSize;
                        record.nVirtualAddress = -1;
                        record.sName = blockType4ToString((BLOCKTYPE4)genericBlock.nType);

                        listResult.append(record);
                    }

                    nMaxOffset = qMax(nMaxOffset, nCurrentOffset + genericBlock.nHeaderSize);

                    if (genericBlock.nType == BLOCKTYPE4_FILE) {
                        FILEBLOCK4 fileBlock4 = readFileBlock4(nCurrentOffset);

                        // fileBlock4.packSize fileBlock4.highPackSize
                        qint64 nFileSize = fileBlock4.packSize;
                        nFileSize |= ((qint64)fileBlock4.highPackSize) << 32;

                        qint64 nUnpSize = fileBlock4.unpSize;
                        nUnpSize |= ((qint64)fileBlock4.highUnpSize) << 32;

                        if (nFileParts & FILEPART_STREAM) {
                            XBinary::FPART record = {};
                            record.filePart = FILEPART_STREAM;
                            record.nFileOffset = nCurrentOffset + fileBlock4.genericBlock4.nHeaderSize;
                            record.nFileSize = nFileSize;
                            record.nVirtualAddress = -1;
                            record.sName = "Stream";
                            record.mapProperties = _readProperties(fileBlock4);

                            listResult.append(record);
                        }

                        nCurrentOffset += fileBlock4.genericBlock4.nHeaderSize + nFileSize;
                        nMaxOffset = qMax(nMaxOffset, nCurrentOffset);
                    } else {
                        nCurrentOffset += genericBlock.nHeaderSize;
                        nMaxOffset = qMax(nMaxOffset, nCurrentOffset);
                    }
                } else {
                    break;
                }

                if (genericBlock.nType == 0x7B) {  // END
                    break;
                }
            }
        } else if (nInternVersion == 5) {
            while (isPdStructNotCanceled(pPdStruct) && (nLimit == -1 || listResult.size() < nLimit)) {
                GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

                if ((genericHeader.nType > 0) && (genericHeader.nType <= 5)) {
                    if (nFileParts & FILEPART_HEADER) {
                        XBinary::FPART record = {};
                        record.filePart = FILEPART_HEADER;
                        record.nFileOffset = nCurrentOffset;
                        record.nFileSize = genericHeader.nHeaderSize;
                        record.nVirtualAddress = -1;
                        record.sName = headerType5ToString((HEADERTYPE5)genericHeader.nType);

                        listResult.append(record);
                    }

                    nMaxOffset = qMax(nMaxOffset, (qint64)(nCurrentOffset + genericHeader.nHeaderSize));

                    if (genericHeader.nDataSize && (nFileParts & FILEPART_STREAM)) {
                        XBinary::FPART record = {};
                        record.filePart = FILEPART_STREAM;
                        record.nFileOffset = nCurrentOffset + genericHeader.nHeaderSize;
                        record.nFileSize = genericHeader.nDataSize;
                        record.nVirtualAddress = -1;
                        record.sName = "Stream";
                        if (genericHeader.nType == HEADERTYPE5_FILE) {
                            FILEHEADER5 fileHeader5 = readFileHeader5(nCurrentOffset);
                            record.mapProperties = _readProperties(fileHeader5);
                        }

                        listResult.append(record);
                    }

                    nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;

                    nMaxOffset = qMax(nMaxOffset, nCurrentOffset);
                } else {
                    break;
                }

                if (genericHeader.nType == 5) {  // END
                    break;
                }
            }
        }
    }

    nMaxOffset = qMin(nMaxOffset, getSize());

    if (nFileParts & FILEPART_DATA) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nMaxOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < getSize()) {
            XBinary::FPART record = {};
            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nMaxOffset;
            record.nFileSize = getSize() - nMaxOffset;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

QList<XBinary::DATA_HEADER> XRar::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        qint32 nVersion = getInternVersion(pPdStruct);

        if (nVersion == 1) {
            _dataHeadersOptions.nID = STRUCTID_RAR14_SIGNATURE;
            _dataHeadersOptions.nSize = 4;
        } else if (nVersion == 4) {
            _dataHeadersOptions.nID = STRUCTID_RAR40_SIGNATURE;
            _dataHeadersOptions.nSize = 7;
        } else if (nVersion == 5) {
            _dataHeadersOptions.nID = STRUCTID_RAR50_SIGNATURE;
            _dataHeadersOptions.nSize = 8;
        }

        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        if (isPdStructNotCanceled(pPdStruct)) {
            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_RAR14_SIGNATURE) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));

                dataHeader.listRecords.append(getDataRecord(0, 4, "Signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            } else if (dataHeadersOptions.nID == STRUCTID_RAR40_SIGNATURE) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));

                dataHeader.listRecords.append(getDataRecord(0, 7, "Signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    // Count RAR 4.0 blocks for table
                    qint64 nCurrentOffset = 7;
                    qint64 nTotalSize = getSize();
                    qint32 nNumberOfBlocks = 0;

                    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
                        if (nCurrentOffset >= nTotalSize - sizeof(GENERICBLOCK4)) {
                            break;
                        }

                        GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

                        if (genericBlock.nType >= 0x72 && genericBlock.nType <= 0x7B) {
                            nNumberOfBlocks++;

                            if (genericBlock.nType == BLOCKTYPE4_FILE) {
                                FILEBLOCK4 fileBlock4 = readFileBlock4(nCurrentOffset);
                                qint64 nPackSize = fileBlock4.packSize;
                                if (fileBlock4.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                                    nPackSize |= ((qint64)fileBlock4.highPackSize << 32);
                                }
                                nCurrentOffset += fileBlock4.genericBlock4.nHeaderSize + nPackSize;
                            } else {
                                nCurrentOffset += genericBlock.nHeaderSize;
                            }

                            if (genericBlock.nType == BLOCKTYPE4_END) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    // Create table of RAR 4.0 blocks
                    DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                    _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                    _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                    _dataHeadersOptions.nID = STRUCTID_RAR40_HEADER;
                    _dataHeadersOptions.nLocation += 7;
                    _dataHeadersOptions.nCount = nNumberOfBlocks;
                    _dataHeadersOptions.nSize = nCurrentOffset - 7;

                    listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                }
            } else if (dataHeadersOptions.nID == STRUCTID_RAR50_SIGNATURE) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));

                dataHeader.listRecords.append(getDataRecord(0, 8, "Signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    // Count RAR 5.0 headers for table
                    qint64 nCurrentOffset = 8;
                    qint32 nNumberOfHeaders = 0;

                    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
                        GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

                        if ((genericHeader.nType > 0) && (genericHeader.nType <= 5)) {
                            nNumberOfHeaders++;
                            nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;

                            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                                break;
                            }
                        } else {
                            break;
                        }
                    }

                    // Create table of RAR 5.0 headers
                    DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                    _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                    _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                    _dataHeadersOptions.nID = STRUCTID_RAR50_HEADER;
                    _dataHeadersOptions.nLocation += 8;
                    _dataHeadersOptions.nCount = nNumberOfHeaders;
                    _dataHeadersOptions.nSize = nCurrentOffset - 8;

                    listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                }
            } else if (dataHeadersOptions.nID == STRUCTID_RAR40_HEADER) {
                GENERICBLOCK4 genericBlock = readGenericBlock4(nStartOffset);

                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = genericBlock.nHeaderSize;

                dataHeader.listRecords.append(getDataRecord(0, 2, "CRC16", XBinary::VT_UINT16, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(2, 1, "Type", XBinary::VT_UINT8, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(3, 2, "Flags", XBinary::VT_UINT16, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(5, 2, "Header Size", XBinary::VT_UINT16, DRF_SIZE, XBinary::ENDIAN_LITTLE));

                listResult.append(dataHeader);
            } else if (dataHeadersOptions.nID == STRUCTID_RAR50_HEADER) {
                GENERICHEADER5 genericHeader = readGenericHeader5(nStartOffset);

                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XRar::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = genericHeader.nHeaderSize;

                qint64 nOffset = 0;
                dataHeader.listRecords.append(getDataRecord(nOffset, 4, "CRC32", XBinary::VT_UINT32, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
                nOffset += 4;

                // Variable-length fields (ULEB128)
                PACKED_UINT packeInt = read_uleb128(nStartOffset + nOffset, 4);
                dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Header Size", XBinary::VT_ULEB128, DRF_SIZE, XBinary::ENDIAN_LITTLE));
                nOffset += packeInt.nByteSize;

                packeInt = read_uleb128(nStartOffset + nOffset, 4);
                dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Type", XBinary::VT_ULEB128, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
                nOffset += packeInt.nByteSize;

                packeInt = read_uleb128(nStartOffset + nOffset, 4);
                dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Flags", XBinary::VT_ULEB128, DRF_UNKNOWN, XBinary::ENDIAN_LITTLE));
                nOffset += packeInt.nByteSize;

                if (genericHeader.nFlags & 0x0001) {
                    packeInt = read_uleb128(nStartOffset + nOffset, 4);
                    dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Extra Area Size", XBinary::VT_ULEB128, DRF_SIZE, XBinary::ENDIAN_LITTLE));
                    nOffset += packeInt.nByteSize;
                }

                if (genericHeader.nFlags & 0x0002) {
                    packeInt = read_uleb128(nStartOffset + nOffset, 8);
                    dataHeader.listRecords.append(getDataRecord(nOffset, packeInt.nByteSize, "Data Size", XBinary::VT_ULEB128, DRF_SIZE, XBinary::ENDIAN_LITTLE));
                    nOffset += packeInt.nByteSize;
                }

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

XBinary::FILEFORMATINFO XRar::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    FILEFORMATINFO result = {};
    result.nSize = getSize();

    qint32 nVersion = getInternVersion(pPdStruct);

    if (nVersion) {
        qint64 nCurrentOffset = 0;

        qint64 nFormatSize = 0;

        bool bFile = false;

        if (nVersion == 1) {
            result.sVersion = "1.4";
        } else if (nVersion == 4) {
            nCurrentOffset = 7;
            result.sVersion = "1.5-4.X";

            while (isPdStructNotCanceled(pPdStruct)) {
                if (nCurrentOffset >= getSize() - sizeof(GENERICBLOCK4)) {
                    break;
                }

                GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

                if (genericBlock.nType >= 0x72 && genericBlock.nType <= 0x7B) {
                    if (genericBlock.nType == BLOCKTYPE4_FILE) {
                        FILEBLOCK4 fileBlock4 = readFileBlock4(nCurrentOffset);

                        // record.sFileName = fileBlock4.sFileName;
                        // record.nCRC32 = fileBlock4.genericBlock4.nCRC16;
                        // record.nDataOffset = nCurrentOffset + fileBlock4.genericBlock4.nHeaderSize;
                        // record.nDataSize = fileBlock4.packSize;
                        // record.spInfo.nUncompressedSize = fileBlock4.unpSize;
                        // record.nHeaderOffset = nCurrentOffset;
                        // record.nHeaderSize = fileBlock4.genericBlock4.nHeaderSize;

                        // if (fileBlock4.method == RAR_METHOD_STORE) {
                        //     record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
                        // } else {
                        //     record.spInfo.compressMethod = COMPRESS_METHOD_RAR;
                        // }

                        if (!bFile) {
                            quint8 _nVer = fileBlock4.unpVer;

                            if (_nVer == 15) {
                                result.sVersion = "1.5";
                            } else if (_nVer == 20) {
                                result.sVersion = "2.X";
                            } else if (_nVer == 26) {
                                result.sVersion = "2.X";  // large files support
                            } else if (_nVer == 29) {
                                result.sVersion = "3.X";
                            }
                            // TODO
                            bFile = true;
                        }

                        nFormatSize = qMax(nFormatSize, nCurrentOffset + fileBlock4.genericBlock4.nHeaderSize + fileBlock4.packSize);

                        nCurrentOffset += fileBlock4.genericBlock4.nHeaderSize + fileBlock4.packSize;
                    } else {
                        nCurrentOffset += genericBlock.nHeaderSize;
                    }
                } else {
                    break;
                }

                nFormatSize = qMax(nFormatSize, nCurrentOffset + genericBlock.nHeaderSize);

                if (genericBlock.nType == 0x7B) {  // END
                    break;
                }
            }
        }
        if (nVersion == 5) {
            nCurrentOffset = 8;
            result.sVersion = "5.X-7.X";

            while (isPdStructNotCanceled(pPdStruct)) {
                GENERICHEADER5 genericHeader = XRar::readGenericHeader5(nCurrentOffset);

                if ((genericHeader.nType > 0) && (genericHeader.nType <= 5)) {
                    if (genericHeader.nType == HEADERTYPE5_FILE) {
                        FILEHEADER5 fileHeader5 = readFileHeader5(nCurrentOffset);

                        // record.sFileName = fileHeader5.sFileName;
                        // record.nCRC32 = fileHeader5.nCRC32;
                        // record.nDataOffset = nCurrentOffset + fileHeader5.nHeaderSize;
                        // record.nDataSize = fileHeader5.nDataSize;
                        // record.spInfo.nUncompressedSize = fileHeader5.nUnpackedSize;
                        // record.nHeaderOffset = nCurrentOffset;
                        // record.nHeaderSize = fileHeader5.nHeaderSize;
                        if (!bFile) {
                            quint8 _nVer = fileHeader5.nCompInfo & 0x003f;

                            if (_nVer == 0) {
                                result.sVersion = "5.X";  // 50
                            } else if (_nVer == 1) {
                                result.sVersion = "7.X";  // 70
                            }
                            // TODO
                            bFile = true;
                        }

                        nFormatSize = qMax(nFormatSize, nCurrentOffset + (qint64)fileHeader5.nHeaderSize + (qint64)fileHeader5.nDataSize);
                    }

                    nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
                } else {
                    break;
                }

                nFormatSize = qMax(nFormatSize, nCurrentOffset + (qint64)genericHeader.nHeaderSize);

                if (genericHeader.nType == 5) {  // END
                    break;
                }
            }
        }

        if (nFormatSize) {
            result.bIsValid = true;
        }

        result.fileType = getFileType();
        result.sExt = getFileFormatExt();
        result.sInfo = getInfo();
        result.sMIME = getMIMEString();
    }

    return result;
}

qint32 XRar::getInternVersion(PDSTRUCT *pPdStruct)
{
    qint32 nResult = 0;

    _MEMORY_MAP memoryMap = XBinary::getSimpleMemoryMap();

    // TODO more
    if (compareSignature(&memoryMap, "'RE~^'", 0, pPdStruct)) {
        nResult = 1;  // "1.4";
    } else if (compareSignature(&memoryMap, "'Rar!'1A0700", 0, pPdStruct)) {
        nResult = 4;  // "1.5-4.X";
    } else if (compareSignature(&memoryMap, "'Rar!'1A070100", 0, pPdStruct)) {
        nResult = 5;  // "5.X-7.X";
    }

    return nResult;
}

XRar::FILEHEADER5 XRar::readFileHeader5(qint64 nOffset)
{
    FILEHEADER5 result = {};

    qint64 nCurrentOffset = nOffset;
    PACKED_UINT packeInt = {};

    // Read the base header fields
    result.nCRC32 = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result._nHeaderSize = packeInt.nValue;
    result.nHeaderSize = result._nHeaderSize + packeInt.nByteSize + 4;
    nCurrentOffset += packeInt.nByteSize;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nType = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nFlags = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    // Read optional extra area size
    if (result.nFlags & 0x0001) {
        packeInt = read_uleb128(nCurrentOffset, 4);
        result.nExtraAreaSize = packeInt.nValue;
        nCurrentOffset += packeInt.nByteSize;
    }

    // Read optional data size
    if (result.nFlags & 0x0002) {
        packeInt = read_uleb128(nCurrentOffset, 8);
        result.nDataSize = packeInt.nValue;
        nCurrentOffset += packeInt.nByteSize;
    }

    // Read file header specific fields
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nFileFlags = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    // Read unpacked size
    packeInt = read_uleb128(nCurrentOffset, 8);
    result.nUnpackedSize = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    // Read attributes
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nAttributes = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    // Read mtime if present
    if (result.nFileFlags & 0x0002) {
        result.nMTime = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
    }

    // Read Data CRC32 if present
    if (result.nFileFlags & 0x0004) {
        result.nDataCRC32 = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
    }

    // Read compression information
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nCompInfo = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    // Read Host OS
    packeInt = read_uleb128(nCurrentOffset, 2);
    result.nHostOS = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    // Read Name Length
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nNameLength = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    // Read Name
    if (result.nNameLength > 0) {
        result.sFileName = decodeRarUnicodeName(read_array(nCurrentOffset, result.nNameLength));
        nCurrentOffset += result.nNameLength;
    }

    // Read Extra Area if present
    if (result.nFlags & 0x0001 && result.nExtraAreaSize > 0) {
        result.baExtraArea = read_array(nCurrentOffset, result.nExtraAreaSize);
        // nCurrentOffset += result.nExtraAreaSize;
    }

    // // Read Data Area if present
    // if (result.nFlags & 0x0002 && result.nDataSize > 0) {
    //     // Store position of data area for later use
    //     result.nDataAreaOffset = nCurrentOffset;

    //     // Optionally read the data if needed
    //     // Note: For large files, you might not want to read all data at once
    //     // result.baDataArea = read_array(nCurrentOffset, result.nDataSize);
    //     // nCurrentOffset += result.nDataSize;
    // }

    return result;
}

XRar::FILEBLOCK4 XRar::readFileBlock4(qint64 nOffset)
{
    FILEBLOCK4 result = {};

    // Bounds check - need at least 7 + 25 = 32 bytes for basic header
    if (nOffset < 0 || nOffset + 32 > getSize()) {
        return result;
    }

    qint64 nCurrentOffset = nOffset;

    // Read header fields
    result.genericBlock4 = readGenericBlock4(nCurrentOffset);
    nCurrentOffset += 7;

    // Continue reading file block specific fields
    result.packSize = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.unpSize = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.hostOS = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.fileCRC = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.fileTime = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    result.unpVer = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.method = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.nameSize = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;
    result.fileAttr = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;

    // Read high bits of pack/unpack size if large file flag is set
    if (result.genericBlock4.nFlags & RAR4_FILE_LARGE) {
        result.highPackSize = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.highUnpSize = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
    } else {
        result.highPackSize = 0;
        result.highUnpSize = 0;
    }

    // Read filename
    if (result.nameSize > 0) {
        QByteArray nameData = read_array(nCurrentOffset, result.nameSize);
        nCurrentOffset += result.nameSize;

        // Handle Unicode filenames
        if (result.genericBlock4.nFlags & RAR4_FILE_UNICODE_FILENAME) {
            // This is a simplified approach for Unicode filename handling
            // Real implementation would need more complex parsing of the RarUnicodeFileName format
            result.sFileName = decodeRarUnicodeName(nameData);
        } else {
            result.sFileName = QString::fromLatin1(nameData);
        }
    }

    return result;
}

XRar::GENERICBLOCK4 XRar::readGenericBlock4(qint64 nOffset)
{
    GENERICBLOCK4 result = {};

    // Bounds check
    if (nOffset < 0 || nOffset + 7 > getSize()) {
        result.nType = 0;
        result.nHeaderSize = 0;
        return result;
    }

    qint64 nCurrentOffset = nOffset;

    result.nCRC16 = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;
    result.nType = read_uint8(nCurrentOffset);
    nCurrentOffset++;
    result.nFlags = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;
    result.nHeaderSize = read_uint16(nCurrentOffset);
    nCurrentOffset += 2;

    // if ((result.nType != BLOCKTYPE4_END) && (result.nHeaderSize >= 11)) {
    //     result.nDataSize = read_uint32(nCurrentOffset);
    // }

    return result;
}

QString XRar::decodeRarUnicodeName(const QByteArray &nameData)
{
    // This is a complex process in RAR - simplified version here
    // Real implementation would need to follow the RarUnicodeFileName format

    // Try UTF-8 first
    QString result = QString::fromUtf8(nameData);
    if (!result.contains(QChar(0xFFFD))) {  // No replacement character
        return result;
    }

    // Fall back to system locale
    return QString::fromLocal8Bit(nameData);
}

quint16 XRar::calculateCRC16(const QByteArray &data)
{
    quint16 nCRC = 0;
    const quint16 nPolynomial = 0x1021;  // CRC16-CCITT polynomial

    for (qint32 i = 0; i < data.size(); i++) {
        nCRC ^= (quint8)data[i] << 8;

        for (qint32 j = 0; j < 8; j++) {
            if (nCRC & 0x8000) {
                nCRC = (nCRC << 1) ^ nPolynomial;
            } else {
                nCRC = nCRC << 1;
            }
        }
    }

    return nCRC;
}

QByteArray XRar::createFileBlock4(const QString &sFileName, qint64 nFileSize, quint32 nFileCRC, quint32 nFileTime, quint32 nAttributes)
{
    QByteArray baResult;
    QByteArray baHeader;
    QDataStream ds(&baHeader, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::LittleEndian);

    // Convert filename to bytes (ASCII/UTF-8)
    QByteArray baFileName = sFileName.toUtf8();
    quint16 nNameSize = baFileName.size();

    // Build header (without CRC16 at the beginning)
    ds << (quint8)BLOCKTYPE4_FILE;  // Type
    ds << (quint16)0x8000;          // Flags (0x8000 = has data)

    // Calculate header size (no high size fields for files < 4GB)
    quint16 nHeaderSize = 7 + 25 + nNameSize;  // 7 (generic) + 25 (file-specific) + name
    ds << nHeaderSize;

    // File-specific fields
    ds << (quint32)nFileSize;        // packSize (low 32 bits)
    ds << (quint32)nFileSize;        // unpSize (low 32 bits)
    ds << (quint8)RAR_OS_WIN32;      // hostOS
    ds << nFileCRC;                  // fileCRC
    ds << nFileTime;                 // fileTime (MS-DOS format)
    ds << (quint8)0x14;              // unpVer (2.0)
    ds << (quint8)RAR_METHOD_STORE;  // method (0x30 = STORE)
    ds << nNameSize;                 // nameSize
    ds << nAttributes;               // fileAttr

    // Note: For files < 4GB, we don't write highPackSize/highUnpSize fields
    // The RAR4_FILE_LARGE flag (0x0100) is not set, so reader won't expect these fields

    // Append filename
    baHeader.append(baFileName);

    // Calculate CRC16 and prepend
    quint16 nCRC16 = calculateCRC16(baHeader);
    QByteArray baCRC;
    QDataStream dsCRC(&baCRC, QIODevice::WriteOnly);
    dsCRC.setByteOrder(QDataStream::LittleEndian);
    dsCRC << nCRC16;

    baResult = baCRC + baHeader;

    return baResult;
}

bool XRar::initUnpack(XBinary::UNPACK_STATE *pUnpackState, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

    if (!pUnpackState) {
        return false;
    }

    // Initialize state
    pUnpackState->nCurrentOffset = 0;
    pUnpackState->nTotalSize = getSize();
    pUnpackState->nCurrentIndex = 0;
    pUnpackState->nNumberOfRecords = 0;

    // Create context
    RAR_UNPACK_CONTEXT *pContext = new RAR_UNPACK_CONTEXT;
    pContext->nVersion = getInternVersion(pPdStruct);

    if (pContext->nVersion == 0) {
        delete pContext;
        return false;
    }

    qint64 nFileHeaderSize = (pContext->nVersion == 4) ? 7 : 8;
    qint64 nCurrentOffset = nFileHeaderSize;

#ifdef QT_DEBUG
    qDebug() << "XRar::initUnpack() - Version:" << pContext->nVersion << "FileHeaderSize:" << nFileHeaderSize << "ArchiveSize:" << pUnpackState->nTotalSize;
#endif

    // Skip the ARCHIVE header block for RAR4
    if (pContext->nVersion == 4) {
        GENERICBLOCK4 archiveBlock = readGenericBlock4(nCurrentOffset);
#ifdef QT_DEBUG
        qDebug() << "XRar::initUnpack() - ARCHIVE Block - Type: 0x" + QString::number(archiveBlock.nType, 16) << "Size:" << archiveBlock.nHeaderSize
                 << "Flags: 0x" + QString::number(archiveBlock.nFlags, 16);
#endif

        if (archiveBlock.nType == BLOCKTYPE4_ARCHIVE && archiveBlock.nHeaderSize > 0) {
            nCurrentOffset += archiveBlock.nHeaderSize;
#ifdef QT_DEBUG
            qDebug() << "XRar::initUnpack() - ARCHIVE Block processed, next offset:" << nCurrentOffset;
#endif
        } else if (archiveBlock.nHeaderSize == 0) {
            // Failed to read - file is too small or corrupted
#ifdef QT_DEBUG
            qDebug() << "XRar::initUnpack() - ERROR: ARCHIVE Block header size is 0";
#endif
            delete pContext;
            return false;
        }
    }

    // Scan archive and collect file offsets
    if (pContext->nVersion == 4) {
        qint32 nBlockCount = 0;
        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nCurrentOffset >= pUnpackState->nTotalSize) {
#ifdef QT_DEBUG
                qDebug() << "XRar::initUnpack() - Reached end of file at offset:" << nCurrentOffset;
#endif
                break;
            }

            GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

#ifdef QT_DEBUG
            qDebug() << "XRar::initUnpack() - Block #" << nBlockCount << "at offset:" << nCurrentOffset << "Type: 0x" + QString::number(genericBlock.nType, 16)
                     << "Size:" << genericBlock.nHeaderSize << "Flags: 0x" + QString::number(genericBlock.nFlags, 16);
#endif

            // Check for read failure or invalid block
            if (genericBlock.nHeaderSize == 0 || genericBlock.nType < 0x72 || genericBlock.nType > 0x7B) {
#ifdef QT_DEBUG
                qDebug() << "XRar::initUnpack() - Invalid block: HeaderSize=" << genericBlock.nHeaderSize << "Type=0x" + QString::number(genericBlock.nType, 16);
#endif
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_FILE) {
                FILEBLOCK4 fileBlock = readFileBlock4(nCurrentOffset);

                // Verify we read a valid block
                if (fileBlock.genericBlock4.nHeaderSize == 0) {
#ifdef QT_DEBUG
                    qDebug() << "XRar::initUnpack() - Failed to read FILE block at offset:" << nCurrentOffset;
#endif
                    break;  // Failed to read file block
                }

#ifdef QT_DEBUG
                qDebug() << "XRar::initUnpack() - Found FILE: " << fileBlock.sFileName << "Size:" << fileBlock.unpSize << "Packed:" << fileBlock.packSize;
#endif

                pContext->listFileOffsets.append(nCurrentOffset);
                pContext->listFileBlocks4.append(fileBlock);
                pUnpackState->nNumberOfRecords++;

                qint64 nPackSize = fileBlock.packSize;
                if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                    nPackSize |= ((qint64)fileBlock.highPackSize << 32);
                }

                nCurrentOffset += fileBlock.genericBlock4.nHeaderSize + nPackSize;
            } else {
                nCurrentOffset += genericBlock.nHeaderSize;
            }

            if (genericBlock.nType == BLOCKTYPE4_END) {
#ifdef QT_DEBUG
                qDebug() << "XRar::initUnpack() - Found END block";
#endif
                break;
            }

            nBlockCount++;
        }
#ifdef QT_DEBUG
        qDebug() << "XRar::initUnpack() - Total blocks processed:" << nBlockCount << "Files found:" << pUnpackState->nNumberOfRecords;
#endif
    } else if (pContext->nVersion == 5) {
        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

            if (genericHeader.nType == HEADERTYPE5_FILE) {
                FILEHEADER5 fileHeader = readFileHeader5(nCurrentOffset);
                pContext->listFileOffsets.append(nCurrentOffset);
                pContext->listFileHeaders5.append(fileHeader);
                pUnpackState->nNumberOfRecords++;

                nCurrentOffset += fileHeader.nHeaderSize + fileHeader.nDataSize;
            } else {
                nCurrentOffset += genericHeader.nHeaderSize;
            }

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                break;
            }
        }
    }

    pUnpackState->pContext = pContext;
    pUnpackState->nCurrentOffset = (pUnpackState->nNumberOfRecords > 0) ? pContext->listFileOffsets.at(0) : 0;

    return true;
}

XBinary::ARCHIVERECORD XRar::infoCurrent(XBinary::UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD record = {};

    if (!pUnpackState || !pUnpackState->pContext || pUnpackState->nCurrentIndex >= pUnpackState->nNumberOfRecords) {
        return record;
    }

    RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;
    qint32 nIndex = pUnpackState->nCurrentIndex;

    if (pContext->nVersion == 4) {
        const FILEBLOCK4 &fileBlock = pContext->listFileBlocks4.at(nIndex);

        qint64 nPackSize = fileBlock.packSize;
        qint64 nUnpSize = fileBlock.unpSize;

        if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
            nPackSize |= ((qint64)fileBlock.highPackSize << 32);
            nUnpSize |= ((qint64)fileBlock.highUnpSize << 32);
        }

        record.nStreamOffset = pContext->listFileOffsets.at(nIndex) + fileBlock.genericBlock4.nHeaderSize;
        record.nStreamSize = nPackSize;

        record.mapProperties = _readProperties(fileBlock);

    } else if (pContext->nVersion == 5) {
        const FILEHEADER5 &fileHeader = pContext->listFileHeaders5.at(nIndex);

        record.nStreamOffset = pContext->listFileOffsets.at(nIndex) + fileHeader.nHeaderSize;
        record.nStreamSize = fileHeader.nDataSize;

        record.mapProperties = _readProperties(fileHeader);
    }

    return record;
}

bool XRar::unpackCurrent(XBinary::UNPACK_STATE *pUnpackState, QIODevice *pOutputDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pUnpackState || !pUnpackState->pContext || !pOutputDevice) {
        return false;
    }

    if (pUnpackState->nCurrentIndex >= pUnpackState->nNumberOfRecords) {
        return false;
    }

    ARCHIVERECORD record = infoCurrent(pUnpackState, pPdStruct);
    COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)record.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();

    RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;

    bool bResult = false;
    // For solid archives: first file is not solid (bIsSolid=false), subsequent files are solid (bIsSolid=true)
    bool bIsSolid = (pUnpackState->nCurrentIndex > 0);

    SubDevice sd(getDevice(), record.nStreamOffset, record.nStreamSize);

    if (sd.open(QIODevice::ReadOnly)) {
        if (compressMethod == COMPRESS_METHOD_STORE) {
            qint64 nDataOffset = record.nStreamOffset;
            qint64 nDataSize = record.nStreamSize;

            bResult = XBinary::copyDeviceMemory(getDevice(), nDataOffset, pOutputDevice, 0, nDataSize);  // TODO
        } else if ((compressMethod == COMPRESS_METHOD_RAR_15) || (compressMethod == COMPRESS_METHOD_RAR_20) || (compressMethod == COMPRESS_METHOD_RAR_29) ||
                   (compressMethod == COMPRESS_METHOD_RAR_50) || (compressMethod == COMPRESS_METHOD_RAR_70)) {
            qint32 nWindowSize = record.mapProperties.value(FPART_PROP_WINDOWSIZE).toInt();

            pContext->rarUnpacker.setDevices(&sd, pOutputDevice);
            qint32 nInit = pContext->rarUnpacker.Init(nWindowSize, bIsSolid);

            if (nInit > 0) {
                if (compressMethod == COMPRESS_METHOD_RAR_15) {
                    pContext->rarUnpacker.Unpack15(bIsSolid, pPdStruct);
                    bResult = true;
                } else if (compressMethod == COMPRESS_METHOD_RAR_20) {
                    pContext->rarUnpacker.Unpack20(bIsSolid, pPdStruct);
                    bResult = true;
                } else if (compressMethod == COMPRESS_METHOD_RAR_29) {
                    pContext->rarUnpacker.Unpack29(bIsSolid, pPdStruct);
                    bResult = true;
                } else if ((compressMethod == COMPRESS_METHOD_RAR_50) || (compressMethod == COMPRESS_METHOD_RAR_70)) {
                    pContext->rarUnpacker.Unpack5(bIsSolid, pPdStruct);
                    bResult = true;
                }
            }
        }

        sd.close();
    }

    return bResult;
}

bool XRar::moveToNext(XBinary::UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pUnpackState || !pUnpackState->pContext) {
        return false;
    }

    pUnpackState->nCurrentIndex++;

    if (pUnpackState->nCurrentIndex < pUnpackState->nNumberOfRecords) {
        RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;
        pUnpackState->nCurrentOffset = pContext->listFileOffsets.at(pUnpackState->nCurrentIndex);

        return true;
    } else {
        return false;
    }
}

QMap<XBinary::FPART_PROP, QVariant> XRar::_readProperties(const FILEBLOCK4 &fileBlock4)
{
    QMap<XBinary::FPART_PROP, QVariant> mapResult;

    qint64 nPackSize = fileBlock4.packSize;
    qint64 nUnpSize = fileBlock4.unpSize;

    if (fileBlock4.genericBlock4.nFlags & RAR4_FILE_LARGE) {
        nPackSize |= ((qint64)fileBlock4.highPackSize << 32);
        nUnpSize |= ((qint64)fileBlock4.highUnpSize << 32);
    }

    mapResult.insert(XBinary::FPART_PROP_ORIGINALNAME, fileBlock4.sFileName);
    mapResult.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nPackSize);
    mapResult.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUnpSize);
    mapResult.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_EDB88320);
    mapResult.insert(XBinary::FPART_PROP_CRC_VALUE, fileBlock4.fileCRC);

    COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;

    if (fileBlock4.method == RAR_METHOD_STORE) {
        compressMethod = COMPRESS_METHOD_STORE;
    } else if (fileBlock4.unpVer == 15) {
        compressMethod = COMPRESS_METHOD_RAR_15;
        mapResult.insert(XBinary::FPART_PROP_WINDOWSIZE, 0x10000);
    } else if ((fileBlock4.unpVer == 20) || (fileBlock4.unpVer == 26)) {
        compressMethod = COMPRESS_METHOD_RAR_20;
    } else if (fileBlock4.unpVer == 29) {
        compressMethod = COMPRESS_METHOD_RAR_29;
    }

    mapResult.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);

    // Extract DOS date and time from 32-bit fileTime field (date in high word, time in low word)
    quint16 nDosTime = fileBlock4.fileTime & 0xFFFF;
    quint16 nDosDate = (fileBlock4.fileTime >> 16) & 0xFFFF;
    QDateTime dateTime = XBinary::dosDateTimeToQDateTime(nDosDate, nDosTime);
    mapResult.insert(XBinary::FPART_PROP_DATETIME, dateTime);

    return mapResult;
}

QMap<XBinary::FPART_PROP, QVariant> XRar::_readProperties(const FILEHEADER5 &fileHeader5)
{
    QMap<XBinary::FPART_PROP, QVariant> mapResult;

    mapResult.insert(XBinary::FPART_PROP_ORIGINALNAME, fileHeader5.sFileName);
    mapResult.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, fileHeader5.nDataSize);
    mapResult.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, fileHeader5.nUnpackedSize);
    mapResult.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_EDB88320);
    mapResult.insert(XBinary::FPART_PROP_CRC_VALUE, fileHeader5.nCRC32);

    quint8 nVer = fileHeader5.nCompInfo & 0x003f;
    quint8 nMethod = (fileHeader5.nCompInfo >> 7) & 7;

    COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;

    if (nMethod == RAR5_METHOD_STORE) {
        compressMethod = COMPRESS_METHOD_STORE;
    } else if (nVer == 0) {
        compressMethod = COMPRESS_METHOD_RAR_50;
    } else if (nVer == 1) {
        compressMethod = COMPRESS_METHOD_RAR_70;
    }

    mapResult.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);

    if (fileHeader5.nFileFlags & 0x0002) {
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
        QDateTime dateTime = QDateTime::fromSecsSinceEpoch(fileHeader5.nMTime);
#else
        QDateTime dateTime = QDateTime::fromMSecsSinceEpoch(fileHeader5.nMTime * 1000);
#endif
        mapResult.insert(XBinary::FPART_PROP_DATETIME, dateTime);
    }

    return mapResult;
}

QList<XBinary::MAPMODE> XRar::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XRar::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_REGIONS;  // Default mode
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

XBinary::FT XRar::getFileType()
{
    return FT_RAR;
}

XRar::GENERICHEADER5 XRar::readGenericHeader5(qint64 nOffset)
{
    GENERICHEADER5 result = {};

    qint64 nCurrentOffset = nOffset;
    PACKED_UINT packeInt = {};

    result.nCRC32 = read_uint32(nCurrentOffset);
    nCurrentOffset += 4;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result._nHeaderSize = packeInt.nValue;
    result.nHeaderSize = result._nHeaderSize + packeInt.nByteSize + 4;
    nCurrentOffset += packeInt.nByteSize;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nType = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;
    packeInt = read_uleb128(nCurrentOffset, 4);
    result.nFlags = packeInt.nValue;
    nCurrentOffset += packeInt.nByteSize;

    if (result.nFlags & 0x0001) {
        packeInt = read_uleb128(nCurrentOffset, 4);
        result.nExtraAreaSize = packeInt.nValue;
        nCurrentOffset += packeInt.nByteSize;
    }

    if (result.nFlags & 0x0002) {
        packeInt = read_uleb128(nCurrentOffset, 8);
        result.nDataSize = packeInt.nValue;
    }

    return result;
}
