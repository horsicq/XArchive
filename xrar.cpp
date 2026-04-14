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
#include "Algos/xaesdecoder.h"
#include <QBuffer>

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

bool XRar::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRar xrar(pDevice);

    return xrar.isValid(pPdStruct);
}

QString XRar::getVersion()
{
    return getFileFormatInfo(nullptr).sVersion;
}

bool XRar::isCommentPresent()
{
    bool bResult = false;

    qint32 nVersion = getInternVersion(nullptr);

    if (nVersion == 4) {
        // RAR4: scan blocks for BLOCKTYPE4_COMMENT (0x75) or BLOCKTYPE4_SUBBLOCK_NEW (0x7A) with "CMT" name
        qint64 nCurrentOffset = 7;  // After signature
        qint64 nTotalSize = getSize();

        while (nCurrentOffset < nTotalSize) {
            GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

            if (genericBlock.nHeaderSize == 0 || genericBlock.nType < 0x72 || genericBlock.nType > 0x7B) {
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_COMMENT) {
                bResult = true;
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                FILEBLOCK4 fileBlock = readFileBlock4(nCurrentOffset);
                if (fileBlock.sFileName == "CMT") {
                    bResult = true;
                    break;
                }
            }

            if (genericBlock.nType == BLOCKTYPE4_END) {
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_FILE || genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                FILEBLOCK4 fileBlock = readFileBlock4(nCurrentOffset);
                qint64 nPackSize = fileBlock.packSize;
                if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                    nPackSize |= ((qint64)fileBlock.highPackSize << 32);
                }
                nCurrentOffset += genericBlock.nHeaderSize + nPackSize;
            } else {
                nCurrentOffset += genericBlock.nHeaderSize;
            }
        }
    } else if (nVersion == 5) {
        // RAR5: scan for service header (type 3) with name "CMT"
        qint64 nCurrentOffset = 8;  // After RAR5 signature
        qint64 nTotalSize = getSize();

        while (nCurrentOffset < nTotalSize) {
            GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

            if (genericHeader.nHeaderSize == 0) {
                break;
            }

            if (genericHeader.nType == HEADERTYPE5_SERVICE) {
                FILEHEADER5 fileHeader = readFileHeader5(nCurrentOffset);
                if (fileHeader.sFileName == "CMT") {
                    bResult = true;
                    break;
                }
            }

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                break;
            }

            nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
        }
    }

    return bResult;
}

QString XRar::getComment()
{
    QString sResult;

    qint32 nVersion = getInternVersion(nullptr);

    if (nVersion == 4) {
        // RAR4: find COMMENT block (0x75) or SUBBLOCK_NEW (0x7A) with "CMT" name
        qint64 nCurrentOffset = 7;
        qint64 nTotalSize = getSize();

        while (nCurrentOffset < nTotalSize) {
            GENERICBLOCK4 genericBlock = readGenericBlock4(nCurrentOffset);

            if (genericBlock.nHeaderSize == 0 || genericBlock.nType < 0x72 || genericBlock.nType > 0x7B) {
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_COMMENT) {
                // Old-style RAR4 comment block after generic header (7 bytes):
                //   2 bytes: unpacked comment size
                //   1 byte: version needed to extract
                //   1 byte: packing method
                //   2 bytes: comment CRC16
                // Remaining header data = comment (compressed or stored)
                qint64 nBodyOffset = nCurrentOffset + 7;

                quint16 nUnpSize = read_uint16(nBodyOffset);
                nBodyOffset += 2;
                quint8 nUnpVer = read_uint8(nBodyOffset);
                nBodyOffset += 1;
                quint8 nMethod = read_uint8(nBodyOffset);
                nBodyOffset += 1;

                Q_UNUSED(nUnpVer)

                if (nMethod == RAR_METHOD_STORE) {
                    nBodyOffset += 2;  // Skip CRC16
                    qint64 nCommentDataSize = genericBlock.nHeaderSize - (nBodyOffset - nCurrentOffset);
                    if (nCommentDataSize > 0 && nCommentDataSize <= nUnpSize) {
                        QByteArray baComment = read_array(nBodyOffset, nCommentDataSize);
                        sResult = QString::fromUtf8(baComment);
                    }
                }
                // Compressed comments (method != STORE) are not supported for reading yet
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                FILEBLOCK4 fileBlock = readFileBlock4(nCurrentOffset);
                if (fileBlock.sFileName == "CMT") {
                    // RAR3/4 new-style comment sub-block (same structure as FILE block)
                    // Data after header is the comment (compressed or stored)
                    qint64 nDataOffset = nCurrentOffset + genericBlock.nHeaderSize;
                    qint64 nPackSize = fileBlock.packSize;

                    if (fileBlock.method == RAR_METHOD_STORE && nPackSize > 0) {
                        QByteArray baComment = read_array(nDataOffset, nPackSize);
                        sResult = QString::fromUtf8(baComment);
                    }
                    // Compressed comments (method != STORE) require RAR decompression engine
                    break;
                }
            }

            if (genericBlock.nType == BLOCKTYPE4_END) {
                break;
            }

            if (genericBlock.nType == BLOCKTYPE4_FILE || genericBlock.nType == BLOCKTYPE4_SUBBLOCK_NEW) {
                FILEBLOCK4 fileBlock = readFileBlock4(nCurrentOffset);
                qint64 nPackSize = fileBlock.packSize;
                if (fileBlock.genericBlock4.nFlags & RAR4_FILE_LARGE) {
                    nPackSize |= ((qint64)fileBlock.highPackSize << 32);
                }
                nCurrentOffset += genericBlock.nHeaderSize + nPackSize;
            } else {
                nCurrentOffset += genericBlock.nHeaderSize;
            }
        }
    } else if (nVersion == 5) {
        // RAR5: find service header "CMT" and read comment data area
        qint64 nCurrentOffset = 8;
        qint64 nTotalSize = getSize();

        while (nCurrentOffset < nTotalSize) {
            GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

            if (genericHeader.nHeaderSize == 0) {
                break;
            }

            if (genericHeader.nType == HEADERTYPE5_SERVICE) {
                FILEHEADER5 fileHeader = readFileHeader5(nCurrentOffset);
                if (fileHeader.sFileName == "CMT" && fileHeader.nDataSize > 0) {
                    // RAR5 comment data area follows the header
                    qint64 nDataOffset = nCurrentOffset + fileHeader.nHeaderSize;
                    quint64 nMethod = fileHeader.nCompInfo & 0x003f;

                    if (nMethod == 0) {
                        // Store method (version 0) — read directly
                        QByteArray baComment = read_array(nDataOffset, (qint64)fileHeader.nDataSize);
                        sResult = QString::fromUtf8(baComment);
                    }
                    // Compressed CMT data is not supported for reading yet
                    break;
                }
            }

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                break;
            }

            nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
        }
    }

    return sResult;
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

QString XRar::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XRAR_STRUCTID, sizeof(_TABLE_XRAR_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XRar::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XRAR_STRUCTID, sizeof(_TABLE_XRAR_STRUCTID) / sizeof(XBinary::XCONVERT));
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
            qint64 nFileSize = getSize();
            while (isPdStructNotCanceled(pPdStruct) && (nLimit == -1 || listResult.size() < nLimit)) {
                if (nCurrentOffset >= nFileSize) break;

                GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

                if (genericHeader.nHeaderSize == 0) break;

                // Stop at encryption header — can't parse encrypted data without password
                if (genericHeader.nType == HEADERTYPE5_ENCRYPTION) {
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
                    break;
                }

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
                    qint64 nFileSize = getSize();

                    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
                        if (nCurrentOffset >= nFileSize) break;

                        GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

                        if (genericHeader.nHeaderSize == 0) break;

                        // Stop at encryption header
                        if (genericHeader.nType == HEADERTYPE5_ENCRYPTION) {
                            nNumberOfHeaders++;
                            nCurrentOffset += genericHeader.nHeaderSize;
                            break;
                        }

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
                        //     record.spInfo.compressMethod = HANDLE_METHOD_STORE;
                        // } else {
                        //     record.spInfo.compressMethod = HANDLE_METHOD_RAR;
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
            qint64 nFileSize = getSize();

            while (isPdStructNotCanceled(pPdStruct)) {
                if (nCurrentOffset >= nFileSize) break;

                GENERICHEADER5 genericHeader = XRar::readGenericHeader5(nCurrentOffset);

                if (genericHeader.nHeaderSize == 0) break;

                // Stop at encryption header — can't determine version from encrypted data
                if (genericHeader.nType == HEADERTYPE5_ENCRYPTION) {
                    nFormatSize = qMax(nFormatSize, nCurrentOffset + (qint64)genericHeader.nHeaderSize);
                    // Encrypted headers archive — set format size to entire file
                    nFormatSize = qMax(nFormatSize, nFileSize);
                    break;
                }

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

XRar::FILEBLOCK14 XRar::readFileBlock14(qint64 nOffset)
{
    FILEBLOCK14 result = {};

    // RAR 1.4 file block fixed part is 24 bytes.
    // Bounds check for minimum header (24 bytes).
    if (nOffset < 0 || nOffset + 24 > getSize()) {
        return result;
    }

    qint64 nCurrentOffset = nOffset;

    /* byte [0]: unknown byte (0x07 in all known samples) */
    nCurrentOffset += 1;
    /* byte [1]: unknown byte (0x00 in all known samples) */
    nCurrentOffset += 1;
    result.nFlags = read_uint8(nCurrentOffset);  // flags (bit 0x08 = solid)
    nCurrentOffset += 1;
    result.nPackSize = read_uint32(nCurrentOffset);  // packed size
    nCurrentOffset += 4;
    result.nUnpSize = read_uint32(nCurrentOffset);  // unpacked size
    nCurrentOffset += 4;
    result.nFileCRC16 = read_uint16(nCurrentOffset);  // CRC16 of unpacked data
    nCurrentOffset += 2;
    result.nFileTime = read_uint32(nCurrentOffset);  // DOS date/time
    nCurrentOffset += 4;
    /* bytes [17-18]: additional time / unknown (2 bytes) */
    nCurrentOffset += 2;
    result.nFileAttr = read_uint16(nCurrentOffset);  // file attributes
    nCurrentOffset += 2;
    /* byte [21]: unknown byte (0x02 in all known samples) */
    nCurrentOffset += 1;
    result.nNameLen = read_uint8(nCurrentOffset);  // filename length
    nCurrentOffset += 1;
    result.nMethod = read_uint8(nCurrentOffset);  // packing method (0=store, 1-5=compress)
    nCurrentOffset += 1;

    result.nHeaderSize = 24 + (qint64)result.nNameLen;

    // Bounds check for full header including filename
    if (nOffset + result.nHeaderSize > getSize()) {
        result.nHeaderSize = 0;
        return result;
    }

    if (result.nNameLen > 0) {
        QByteArray nameData = read_array(nCurrentOffset, result.nNameLen);
        result.sFileName = QString::fromLatin1(nameData);
    }

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
    if (!pUnpackState) {
        return false;
    }

    // Store unpack properties (including password for encrypted archives)
    pUnpackState->mapUnpackProperties = mapProperties;

    // Initialize state
    pUnpackState->nCurrentOffset = 0;
    pUnpackState->nTotalSize = getSize();
    pUnpackState->nCurrentIndex = 0;
    pUnpackState->nNumberOfRecords = 0;

    // Create context
    RAR_UNPACK_CONTEXT *pContext = new RAR_UNPACK_CONTEXT;
    pContext->nVersion = getInternVersion(pPdStruct);
    pContext->bArchiveIsSolid = false;
    pContext->bHeadersEncrypted = false;

    if (pContext->nVersion == 0) {
        delete pContext;
        return false;
    }
    qint64 nFileHeaderSize = (pContext->nVersion == 4) ? 7 : ((pContext->nVersion == 1) ? 4 : 8);
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
            // RAR4 archive header flag 0x0008 = solid archive
            pContext->bArchiveIsSolid = (archiveBlock.nFlags & 0x0008) != 0;
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
    if (pContext->nVersion == 1) {
        // RAR 1.4 (RE~^): no global archive header; file blocks start right after the 4-byte signature
        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nCurrentOffset >= pUnpackState->nTotalSize) {
                break;
            }

            FILEBLOCK14 fileBlock = readFileBlock14(nCurrentOffset);

            if (fileBlock.nHeaderSize == 0) {
                break;  // read error or truncated archive
            }

            pContext->listFileOffsets.append(nCurrentOffset);
            pContext->listFileBlocks14.append(fileBlock);
            pUnpackState->nNumberOfRecords++;

            nCurrentOffset += fileBlock.nHeaderSize + (qint64)fileBlock.nPackSize;
        }

        // Solid flag is archive-level in RAR 1.4 (flags bit 0x08)
        if (!pContext->listFileBlocks14.isEmpty()) {
            pContext->bArchiveIsSolid = (pContext->listFileBlocks14.at(0).nFlags & 0x08) != 0;
        }

        // Compute solid folder indices
        {
            qint32 nFolderIndex = 0;
            for (qint32 i = 0; i < pContext->listFileBlocks14.count(); i++) {
                if (i > 0) {
                    if (!pContext->bArchiveIsSolid) {
                        nFolderIndex++;
                    }
                }
                pContext->listSolidFolderIndex.append(nFolderIndex);
            }
        }
    } else if (pContext->nVersion == 4) {
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

        // Compute solid folder indices for RAR4: increment when per-file solid flag is false
        {
            qint32 nFolderIndex = 0;
            for (qint32 i = 0; i < pContext->listFileBlocks4.count(); i++) {
                bool bPerFileSolid = (pContext->listFileBlocks4.at(i).genericBlock4.nFlags & 0x0010) != 0;
                if (!bPerFileSolid && (i > 0)) {
                    nFolderIndex++;
                }
                pContext->listSolidFolderIndex.append(nFolderIndex);
            }
        }
    } else if (pContext->nVersion == 5) {
        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nCurrentOffset >= pUnpackState->nTotalSize) {
                break;
            }

            GENERICHEADER5 genericHeader = readGenericHeader5(nCurrentOffset);

            if (genericHeader.nHeaderSize == 0) {
#ifdef QT_DEBUG
                qDebug() << "XRar::initUnpack() - RAR5: Invalid header size 0 at offset:" << nCurrentOffset;
#endif
                break;
            }

            if (genericHeader.nType == HEADERTYPE5_MAIN) {
                // RAR5 main archive header: archive flags bit 0x0004 = solid
                // Read archive flags from header body (after CRC + headerSize + type + flags)
                // The GENERICHEADER5 nFlags field contains common header flags, not archive flags.
                // Archive flags are in the header body. For simplicity, scan file headers instead.
            }

            if (genericHeader.nType == HEADERTYPE5_ENCRYPTION) {
                // Archive has encrypted headers — parse encryption params from header body
                // Body layout: vint encVer, vint encFlags, byte kdfCount, 16-byte salt, [8+4 pswCheck]
                qint64 nBodyOffset = nCurrentOffset + 4;  // skip CRC32

                PACKED_UINT puBodySize = read_uleb128(nBodyOffset, 4);
                nBodyOffset += puBodySize.nByteSize;

                PACKED_UINT puType = read_uleb128(nBodyOffset, 4);  // type vint
                nBodyOffset += puType.nByteSize;

                PACKED_UINT puFlags = read_uleb128(nBodyOffset, 4);  // common flags vint
                nBodyOffset += puFlags.nByteSize;

                PACKED_UINT puEncVer = read_uleb128(nBodyOffset, 4);
                nBodyOffset += puEncVer.nByteSize;

                PACKED_UINT puEncFlags = read_uleb128(nBodyOffset, 4);
                nBodyOffset += puEncFlags.nByteSize;

                quint8 nKdfCount = read_uint8(nBodyOffset);
                nBodyOffset += 1;

                QByteArray baSalt = read_array(nBodyOffset, 16);
                nBodyOffset += 16;

#ifdef QT_DEBUG
                qDebug() << "XRar::initUnpack() - ENCRYPTION header: encVer=" << puEncVer.nValue << "encFlags=" << puEncFlags.nValue << "kdfCount=" << nKdfCount
                         << "saltSize=" << baSalt.size();
#endif

                // Derive AES key for header decryption
                QString sPassword = pUnpackState->mapUnpackProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();

                if (!sPassword.isEmpty() && puEncVer.nValue == 0) {
                    QByteArray baHeaderAesKey = XAESDecoder::deriveRar5HeaderKey(sPassword, baSalt, nKdfCount);

                    if (!baHeaderAesKey.isEmpty()) {
                        pContext->bHeadersEncrypted = true;
                        nCurrentOffset += genericHeader.nHeaderSize;

#ifdef QT_DEBUG
                        qDebug() << "XRar::initUnpack() - Decrypting encrypted headers starting at offset:" << nCurrentOffset;
#endif

                        // Read encrypted headers
                        while (XBinary::isPdStructNotCanceled(pPdStruct) && nCurrentOffset < pUnpackState->nTotalSize) {
                            qint64 nConsumed = 0;
                            QByteArray baDecHeader = decryptRar5HeaderBlock(nCurrentOffset, baHeaderAesKey, &nConsumed);

                            if (baDecHeader.isEmpty() || nConsumed == 0) {
#ifdef QT_DEBUG
                                qDebug() << "XRar::initUnpack() - Failed to decrypt header at offset:" << nCurrentOffset;
#endif
                                break;
                            }

                            // Parse decrypted header using QBuffer + temporary XRar
                            QBuffer bufHeader(&baDecHeader);
                            bufHeader.open(QIODevice::ReadOnly);
                            XRar rarTemp(&bufHeader);

                            GENERICHEADER5 decGeneric = rarTemp.readGenericHeader5(0);

#ifdef QT_DEBUG
                            qDebug() << "XRar::initUnpack() - Decrypted header type:" << decGeneric.nType << "headerSize(plain):" << decGeneric.nHeaderSize
                                     << "dataSize:" << decGeneric.nDataSize << "consumed(ondisk):" << nConsumed;
#endif

                            if (decGeneric.nType == HEADERTYPE5_FILE) {
                                FILEHEADER5 fileHeader = rarTemp.readFileHeader5(0);

                                // Override nHeaderSize to reflect the actual on-disk size (IV + encrypted)
                                fileHeader.nHeaderSize = nConsumed;

                                pContext->listFileOffsets.append(nCurrentOffset);
                                pContext->listFileHeaders5.append(fileHeader);
                                pUnpackState->nNumberOfRecords++;

#ifdef QT_DEBUG
                                qDebug() << "XRar::initUnpack() - Encrypted FILE:" << fileHeader.sFileName << "unpackedSize:" << fileHeader.nUnpackedSize
                                         << "dataSize:" << fileHeader.nDataSize;
#endif
                            }

                            // Move past encrypted header + data area
                            nCurrentOffset += nConsumed;

                            if (decGeneric.nDataSize > 0) {
                                nCurrentOffset += decGeneric.nDataSize;
                            }

                            if (decGeneric.nType == HEADERTYPE5_ENDARC) {
                                break;
                            }
                        }

                        baHeaderAesKey.fill(0);
                    }
                }
#ifdef QT_DEBUG
                else {
                    qDebug() << "XRar::initUnpack() - ENCRYPTION header found but no password provided or unsupported version";
                }
#endif

                break;  // After encryption header processing, we're done with block scanning
            }

            if (genericHeader.nType == HEADERTYPE5_FILE) {
                FILEHEADER5 fileHeader = readFileHeader5(nCurrentOffset);
                pContext->listFileOffsets.append(nCurrentOffset);
                pContext->listFileHeaders5.append(fileHeader);
                pUnpackState->nNumberOfRecords++;

                nCurrentOffset += fileHeader.nHeaderSize + fileHeader.nDataSize;
            } else {
                nCurrentOffset += genericHeader.nHeaderSize + genericHeader.nDataSize;
            }

            if (genericHeader.nType == HEADERTYPE5_ENDARC) {
                break;
            }
        }

        // Detect solid archive: if any RAR5 file has solid flag (nCompInfo bit 6),
        // the archive uses solid compression.
        for (qint32 i = 0; i < pContext->listFileHeaders5.count(); i++) {
            if ((pContext->listFileHeaders5.at(i).nCompInfo >> 6) & 1) {
                pContext->bArchiveIsSolid = true;
                break;
            }
        }

        // Compute solid folder indices for RAR5: increment when per-file solid flag is false
        {
            qint32 nFolderIndex = 0;
            for (qint32 i = 0; i < pContext->listFileHeaders5.count(); i++) {
                bool bPerFileSolid = (pContext->listFileHeaders5.at(i).nCompInfo >> 6) & 1;
                if (!bPerFileSolid && (i > 0)) {
                    nFolderIndex++;
                }
                pContext->listSolidFolderIndex.append(nFolderIndex);
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

    if (pContext->nVersion == 1) {
        const FILEBLOCK14 &fileBlock = pContext->listFileBlocks14.at(nIndex);

        record.nStreamOffset = pContext->listFileOffsets.at(nIndex) + fileBlock.nHeaderSize;
        record.nStreamSize = (qint64)fileBlock.nPackSize;

        record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, fileBlock.sFileName);
        record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)fileBlock.nPackSize);
        record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)fileBlock.nUnpSize);
        // RAR 1.4 stores CRC16, not CRC32; set to 0 to skip CRC32 verification
        record.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, (quint32)0);

        HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
        if (fileBlock.nMethod == 0) {
            compressMethod = HANDLE_METHOD_STORE;
        } else {
            // Methods 1-5 all use the same RAR 1.5 algorithm
            compressMethod = HANDLE_METHOD_RAR_15;
            record.mapProperties.insert(XBinary::FPART_PROP_WINDOWSIZE, (qint32)0x10000);
        }
        record.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        bool bIsFolder = (fileBlock.nFileAttr & 0x10) != 0;
        record.mapProperties.insert(XBinary::FPART_PROP_ISFOLDER, bIsFolder);

        if (pContext->bArchiveIsSolid) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }

        if (nIndex < pContext->listSolidFolderIndex.count()) {
            record.mapProperties.insert(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)pContext->listSolidFolderIndex.at(nIndex));
        }

    } else if (pContext->nVersion == 4) {
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

        // For solid archives, mark ALL files as solid so decompressArchiveRecord
        // routes them through the persistent decoder path
        if (pContext->bArchiveIsSolid) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }

        if (nIndex < pContext->listSolidFolderIndex.count()) {
            record.mapProperties.insert(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)pContext->listSolidFolderIndex.at(nIndex));
        }

    } else if (pContext->nVersion == 5) {
        const FILEHEADER5 &fileHeader = pContext->listFileHeaders5.at(nIndex);

        record.nStreamOffset = pContext->listFileOffsets.at(nIndex) + fileHeader.nHeaderSize;
        record.nStreamSize = fileHeader.nDataSize;

        record.mapProperties = _readProperties(fileHeader);

        // For solid archives, mark ALL files as solid so decompressArchiveRecord
        // routes them through the persistent decoder path
        if (pContext->bArchiveIsSolid) {
            record.mapProperties.insert(XBinary::FPART_PROP_ISSOLID, true);
        }

        if (nIndex < pContext->listSolidFolderIndex.count()) {
            record.mapProperties.insert(XBinary::FPART_PROP_SOLIDFOLDERINDEX, (qint64)pContext->listSolidFolderIndex.at(nIndex));
        }
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

    RAR_UNPACK_CONTEXT *pContext = (RAR_UNPACK_CONTEXT *)pUnpackState->pContext;
    ARCHIVERECORD archiveRecord = infoCurrent(pUnpackState, pPdStruct);

    if (archiveRecord.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) return true;  // Directory

    if (archiveRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() == 0) return true;  // Empty file

    bool bResult = pContext->decompress.decompressArchiveRecord(archiveRecord, getDevice(), pOutputDevice, pUnpackState->mapUnpackProperties, pPdStruct);

    // bool bResult = false;
    // // For solid archives: first file is not solid (bIsSolid=false), subsequent files are solid (bIsSolid=true)
    // bool bIsSolid = (pUnpackState->nCurrentIndex > 0);

    // SubDevice sd(getDevice(), record.nStreamOffset, record.nStreamSize);

    // if (sd.open(QIODevice::ReadOnly)) {
    //     if (compressMethod == HANDLE_METHOD_STORE) {
    //         qint64 nDataOffset = record.nStreamOffset;
    //         qint64 nDataSize = record.nStreamSize;

    //         bResult = XBinary::copyDeviceMemory(getDevice(), nDataOffset, pOutputDevice, 0, nDataSize);  // TODO
    //     } else if ((compressMethod == HANDLE_METHOD_RAR_15) || (compressMethod == HANDLE_METHOD_RAR_20) || (compressMethod == HANDLE_METHOD_RAR_29) ||
    //                (compressMethod == HANDLE_METHOD_RAR_50) || (compressMethod == HANDLE_METHOD_RAR_70)) {
    //         qint32 nWindowSize = record.mapProperties.value(FPART_PROP_WINDOWSIZE).toInt();
    //         qint64 nUncompressedSize = record.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    //         pContext->rarUnpacker.setDevices(&sd, pOutputDevice);
    //         qint32 nInit = pContext->rarUnpacker.Init(nWindowSize, bIsSolid);

    //         if (nInit > 0) {
    //             pContext->rarUnpacker.SetDestSize(nUncompressedSize);
    //             if (compressMethod == HANDLE_METHOD_RAR_15) {
    //                 pContext->rarUnpacker.Unpack15(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             } else if (compressMethod == HANDLE_METHOD_RAR_20) {
    //                 pContext->rarUnpacker.Unpack20(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             } else if (compressMethod == HANDLE_METHOD_RAR_29) {
    //                 pContext->rarUnpacker.Unpack29(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             } else if ((compressMethod == HANDLE_METHOD_RAR_50) || (compressMethod == HANDLE_METHOD_RAR_70)) {
    //                 pContext->rarUnpacker.Unpack5(bIsSolid, pPdStruct);
    //                 bResult = true;
    //             }
    //         }
    //     }

    //     sd.close();
    // }

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

QList<XBinary::FPART_PROP> XRar::getAvailableFPARTProperties()
{
    QList<XBinary::FPART_PROP> listResult;

    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_HANDLEMETHOD);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);

    return listResult;
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
    mapResult.insert(XBinary::FPART_PROP_RESULTCRC, fileBlock4.fileCRC);

    HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

    if (fileBlock4.method == RAR_METHOD_STORE) {
        compressMethod = HANDLE_METHOD_STORE;
    } else if (fileBlock4.unpVer == 15) {
        compressMethod = HANDLE_METHOD_RAR_15;
        mapResult.insert(XBinary::FPART_PROP_WINDOWSIZE, 0x10000);
    } else if ((fileBlock4.unpVer == 20) || (fileBlock4.unpVer == 26)) {
        compressMethod = HANDLE_METHOD_RAR_20;
    } else if (fileBlock4.unpVer == 29) {
        compressMethod = HANDLE_METHOD_RAR_29;
    }

    mapResult.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

    // Solid flag: RAR4 file header nFlags bit 0x0010
    bool bIsSolid = (fileBlock4.genericBlock4.nFlags & 0x0010) != 0;
    mapResult.insert(XBinary::FPART_PROP_ISSOLID, bIsSolid);

    // Directory flag: RAR4 uses dictionary size field (bits 7-5) == 7 or fileAttr & 0x10
    bool bIsFolder = ((fileBlock4.genericBlock4.nFlags & 0x00E0) == 0x00E0) || (fileBlock4.fileAttr & 0x10);
    mapResult.insert(XBinary::FPART_PROP_ISFOLDER, bIsFolder);

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
    mapResult.insert(XBinary::FPART_PROP_RESULTCRC, fileHeader5.nDataCRC32);

    quint8 nVer = fileHeader5.nCompInfo & 0x003f;
    quint8 nMethod = (fileHeader5.nCompInfo >> 7) & 7;

    HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

    if (nMethod == RAR5_METHOD_STORE) {
        compressMethod = HANDLE_METHOD_STORE;
    } else if (nVer == 0) {
        compressMethod = HANDLE_METHOD_RAR_50;
    } else if (nVer == 1) {
        compressMethod = HANDLE_METHOD_RAR_70;
    }

    mapResult.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

    // Solid flag: bit 6 of nCompInfo
    bool bIsSolid = (fileHeader5.nCompInfo >> 6) & 1;
    mapResult.insert(XBinary::FPART_PROP_ISSOLID, bIsSolid);

    // Directory flag: RAR5 nFileFlags bit 0 = directory
    bool bIsFolder = (fileHeader5.nFileFlags & 0x0001) != 0;
    mapResult.insert(XBinary::FPART_PROP_ISFOLDER, bIsFolder);

    // Calculate window (dictionary) size from compression info
    if (nVer == 0) {
        // RAR 5.0: bits 10-14 encode dictionary size as 128KB << dictBits
        quint8 nDictBits = (fileHeader5.nCompInfo >> 10) & 0x1F;
        quint64 nWindowSize = (quint64)0x20000 << nDictBits;
        mapResult.insert(XBinary::FPART_PROP_WINDOWSIZE, nWindowSize);
    } else if (nVer == 1) {
        // RAR 7.0: bits 10-14 = d, bit 15 = fraction flag
        // If fraction flag is 0: window_size = 128KB << d
        // If fraction flag is 1: window_size = 3 * (128KB << (d-1))  (1.5x rounding)
        quint8 nDictBits = (fileHeader5.nCompInfo >> 10) & 0x1F;
        bool bFraction = (fileHeader5.nCompInfo >> 15) & 1;
        quint64 nWindowSize;
        if (!bFraction) {
            nWindowSize = (quint64)0x20000 << nDictBits;
        } else {
            nWindowSize = 3 * ((quint64)0x20000 << (nDictBits > 0 ? nDictBits - 1 : 0));
        }
        mapResult.insert(XBinary::FPART_PROP_WINDOWSIZE, nWindowSize);
    }

    if (fileHeader5.nFileFlags & 0x0002) {
        QDateTime dateTime = XBinary::valueToTime(fileHeader5.nMTime, XBinary::DT_TYPE_UNIXTIME);
        mapResult.insert(XBinary::FPART_PROP_DATETIME, dateTime);
    }

    // Parse extra area for encryption record (id=1)
    if (!fileHeader5.baExtraArea.isEmpty()) {
        qint64 nExtraOffset = 0;
        qint64 nExtraSize = fileHeader5.baExtraArea.size();
        char *pExtraData = const_cast<char *>(fileHeader5.baExtraArea.constData());

        while (nExtraOffset < nExtraSize) {
            // Read record size (varint)
            PACKED_UINT puRecSize = _read_uleb128(pExtraData + nExtraOffset, qMin((qint64)10, nExtraSize - nExtraOffset));
            if (puRecSize.nByteSize == 0) break;
            nExtraOffset += puRecSize.nByteSize;
            qint64 nRecSize = puRecSize.nValue;

            if (nRecSize <= 0 || nExtraOffset + nRecSize > nExtraSize) break;

            qint64 nRecStart = nExtraOffset;

            // Read record id (varint)
            PACKED_UINT puRecId = _read_uleb128(pExtraData + nExtraOffset, qMin((qint64)10, nExtraSize - nExtraOffset));
            if (puRecId.nByteSize == 0) break;
            nExtraOffset += puRecId.nByteSize;

            if (puRecId.nValue == 1) {
                // Encryption record: [varint version=0][varint flags][byte cnt][16 salt][16 IV][opt 12 pswCheck]
                PACKED_UINT puVersion = _read_uleb128(pExtraData + nExtraOffset, qMin((qint64)10, nExtraSize - nExtraOffset));
                if (puVersion.nByteSize == 0) break;
                nExtraOffset += puVersion.nByteSize;

                if (puVersion.nValue == 0) {
                    PACKED_UINT puFlags = _read_uleb128(pExtraData + nExtraOffset, qMin((qint64)10, nExtraSize - nExtraOffset));
                    if (puFlags.nByteSize == 0) break;
                    nExtraOffset += puFlags.nByteSize;

                    quint64 nCryptoFlags = puFlags.nValue;
                    bool bHasPswCheck = (nCryptoFlags & 0x0001) != 0;  // flag bit 0 = password check present

                    if (nExtraOffset + 1 + 16 + 16 <= nExtraSize) {
                        quint8 nCnt = (quint8)pExtraData[nExtraOffset];
                        nExtraOffset += 1;

                        QByteArray baSalt(pExtraData + nExtraOffset, 16);
                        nExtraOffset += 16;

                        QByteArray baIV(pExtraData + nExtraOffset, 16);
                        nExtraOffset += 16;

                        QByteArray baPswCheck;
                        if (bHasPswCheck && nExtraOffset + 12 <= nExtraSize) {
                            baPswCheck = QByteArray(pExtraData + nExtraOffset, 12);
                            nExtraOffset += 12;
                        }

                        // Pack crypto params: [1 byte cnt][16 salt][16 IV][opt 12 pswCheck]
                        QByteArray baAESKey;
                        baAESKey.append((char)nCnt);
                        baAESKey.append(baSalt);
                        baAESKey.append(baIV);
                        if (!baPswCheck.isEmpty()) {
                            baAESKey.append(baPswCheck);
                        }

                        mapResult.insert(XBinary::FPART_PROP_ENCRYPTED, true);
                        mapResult.insert(XBinary::FPART_PROP_AESKEY, baAESKey);

                        // RAR5_AES is the outer (decryption) method; original compress method stays as HANDLEMETHOD (inner)
                        mapResult.insert(XBinary::FPART_PROP_HANDLEMETHOD2, (quint32)HANDLE_METHOD_RAR5_AES);

                        // RAR5 encrypted files store HMAC-modified CRC, not plain CRC32.
                        // Disable CRC verification since we can't compare directly.
                        mapResult.insert(XBinary::FPART_PROP_CRC_TYPE, (quint32)CRC_TYPE_UNKNOWN);
                        mapResult.remove(XBinary::FPART_PROP_RESULTCRC);
                    }
                }
            }

            // Skip to next record
            nExtraOffset = nRecStart + nRecSize;
        }
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

QByteArray XRar::decryptRar5HeaderBlock(qint64 nOffset, const QByteArray &baAesKey, qint64 *pConsumedSize)
{
    *pConsumedSize = 0;

    qint64 nTotalSize = getSize();

    if (nOffset + 32 > nTotalSize) {
        // Need at least IV(16) + one AES block(16)
        return QByteArray();
    }

    // Read 16-byte IV
    QByteArray baIV = read_array(nOffset, 16);

    if (baIV.size() != 16) {
        return QByteArray();
    }

    // Read first AES block (16 bytes) of encrypted header
    QByteArray baFirstCipher = read_array(nOffset + 16, 16);

    if (baFirstCipher.size() != 16) {
        return QByteArray();
    }

    // Decrypt first block to get CRC and headerSize
    QByteArray baFirstPlain(16, 0);

    if (!XAESDecoder::decryptAESCBC(baAesKey, baIV, (const quint8 *)baFirstCipher.constData(), (quint8 *)baFirstPlain.data(), 16)) {
        return QByteArray();
    }

    // Parse _headerSize from decrypted data: CRC32(4) + vint(_headerSize)
    PACKED_UINT puHeaderSize = _read_uleb128(baFirstPlain.data() + 4, 10);

    if (puHeaderSize.nByteSize == 0) {
        return QByteArray();
    }

    quint64 nPlainSize = 4 + puHeaderSize.nByteSize + puHeaderSize.nValue;
    quint64 nEncSize = ((nPlainSize + 15) / 16) * 16;

    if (nOffset + 16 + (qint64)nEncSize > nTotalSize) {
        return QByteArray();
    }

    if (nEncSize <= 16) {
        // First AES block was enough
        *pConsumedSize = 16 + (qint64)nEncSize;
        return baFirstPlain.left((qint32)nPlainSize);
    }

    // Need more data — read full encrypted buffer and re-decrypt from scratch
    QByteArray baAllCipher = read_array(nOffset + 16, (qint32)nEncSize);

    if ((quint64)baAllCipher.size() != nEncSize) {
        return QByteArray();
    }

    QByteArray baAllPlain((qint32)nEncSize, 0);

    if (!XAESDecoder::decryptAESCBC(baAesKey, baIV, (const quint8 *)baAllCipher.constData(), (quint8 *)baAllPlain.data(), nEncSize)) {
        return QByteArray();
    }

    *pConsumedSize = 16 + (qint64)nEncSize;
    return baAllPlain.left((qint32)nPlainSize);
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

QList<QString> XRar::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'RE~^'");
    listResult.append("'Rar!'1A07");
    listResult.append("'Rar!'1A0700");
    listResult.append("'Rar!'1A070100");

    return listResult;
}

XBinary *XRar::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XRar(pDevice);
}
