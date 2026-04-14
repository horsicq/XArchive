/* Copyright (c) 2026 hors<horsicq@gmail.com>
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
#include "xarj.h"
#include "xdecompress.h"

XBinary::XCONVERT _TABLE_XARJ_STRUCTID[] = {
    {XARJ::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XARJ::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XARJ::STRUCTID_RECORD, "RECORD", QString("Record")},
};

XARJ::XARJ(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XARJ::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    // ARJ archive starts with 0x60 0xEA followed by a 2-byte basic header size
    // The basic header size must be >= 30 (minimum fixed part)
    if (getSize() >= 34) {  // 2 marker + 2 hdr_size + 30 minimum header
        quint8 nByte0 = read_uint8(0);
        quint8 nByte1 = read_uint8(1);
        quint16 nBasicHeaderSize = read_uint16(2, false);  // LE

        if ((nByte0 == 0x60) && (nByte1 == 0xEA) && (nBasicHeaderSize >= FIXED_HEADER_SIZE) && (nBasicHeaderSize <= 2600)) {
            // The first byte of the basic header is first_hdr_size (fixed part size)
            // ARJ 2.50+ uses 34; older versions use 30. Accept any value >= FIXED_HEADER_SIZE.
            quint8 nFirstHdrSize = read_uint8(4);

            if (nFirstHdrSize >= FIXED_HEADER_SIZE) {
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XARJ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARJ xarj(pDevice);

    return xarj.isValid();
}

qint64 XARJ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XARJ::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XARJ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;
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

XBinary::FT XARJ::getFileType()
{
    return FT_ARJ;
}

QString XARJ::getFileFormatExt()
{
    return "arj";
}

QString XARJ::getFileFormatExtsString()
{
    return "ARJ (*.arj)";
}

QString XARJ::getMIMEString()
{
    return "application/x-arj";
}

QString XARJ::getVersion()
{
    // Archiver version is at offset 5 (index 1 within basic header data starting at offset 4)
    if (getSize() >= 6) {
        quint8 nVersion = read_uint8(5);

        return QString::number(nVersion);
    }

    return QString();
}

QString XARJ::getArch()
{
    return QString();
}

XBinary::ENDIAN XARJ::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XARJ::getMode()
{
    return MODE_DATA;
}

bool XARJ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{

    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = nullptr;
        pState->mapUnpackProperties = mapProperties;

        // Skip main archive header first
        qint64 nOffset = 0;
        qint64 nFileSize = pState->nTotalSize;

        if (nFileSize >= 4) {
            quint8 nByte0 = read_uint8(nOffset);
            quint8 nByte1 = read_uint8(nOffset + 1);
            quint16 nBasicHeaderSize = read_uint16(nOffset + 2, false);

            if ((nByte0 == 0x60) && (nByte1 == 0xEA) && (nBasicHeaderSize > 0)) {
                qint64 nMainHeaderSize = _getEntryHeaderSize(nOffset);

                if (nMainHeaderSize > 0) {
                    nOffset += nMainHeaderSize;
                    nFileSize -= nMainHeaderSize;
                }
            }
        }

        pState->nCurrentOffset = nOffset;

        // Count file records
        qint64 nCountOffset = nOffset;

        while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nFileSize - (nCountOffset - nOffset)) < 4) {
                break;
            }

            quint8 nByte0 = read_uint8(nCountOffset);
            quint8 nByte1 = read_uint8(nCountOffset + 1);
            quint16 nBasicHeaderSize = read_uint16(nCountOffset + 2, false);

            if ((nByte0 != 0x60) || (nByte1 != 0xEA)) {
                break;
            }

            // End-of-archive marker: header size == 0
            if (nBasicHeaderSize == 0) {
                break;
            }

            qint64 nEntryHeaderSize = _getEntryHeaderSize(nCountOffset);

            if (nEntryHeaderSize <= 0) {
                break;
            }

            quint32 nCompressedSize = read_uint32(nCountOffset + 4 + 12, false);

            pState->nNumberOfRecords++;

            nCountOffset += nEntryHeaderSize + nCompressedSize;
        }

        bResult = (pState->nNumberOfRecords > 0);
    }

    return bResult;
}

XBinary::ARCHIVERECORD XARJ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        qint64 nEntryHeaderSize = _getEntryHeaderSize(pState->nCurrentOffset);
        quint32 nCompressedSize = read_uint32(pState->nCurrentOffset + 4 + 12, false);
        quint32 nOriginalSize = read_uint32(pState->nCurrentOffset + 4 + 16, false);
        quint32 nCRC32 = read_uint32(pState->nCurrentOffset + 4 + 20, false);
        quint8 nArjFlags = read_uint8(pState->nCurrentOffset + 4 + 4);
        quint8 nMethod = read_uint8(pState->nCurrentOffset + 4 + 5);
        quint8 nPasswordModifier = read_uint8(pState->nCurrentOffset + 4 + 7);
        quint32 nDosDateTime = read_uint32(pState->nCurrentOffset + 4 + 8, false);

        QString sFileName = _getFileName(pState->nCurrentOffset);
        sFileName = sFileName.replace("\\", "/");

        result.nStreamOffset = pState->nCurrentOffset + nEntryHeaderSize;
        result.nStreamSize = nCompressedSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMOFFSET, result.nStreamOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMSIZE, result.nStreamSize);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nOriginalSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)nCompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, nCRC32);
        result.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, nCRC32 != 0 ? XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF : XBinary::CRC_TYPE_UNKNOWN);
        result.mapProperties.insert(XBinary::FPART_PROP_TYPE, (quint32)nMethod);

        HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

        if ((nMethod == CMETHOD_STORED) || (nMethod == CMETHOD_NO_COMPRESSION_1) || (nMethod == CMETHOD_NO_COMPRESSION_2)) {
            compressMethod = HANDLE_METHOD_STORE;
        } else if ((nMethod == CMETHOD_COMPRESSED_MOST) || (nMethod == CMETHOD_COMPRESSED) || (nMethod == CMETHOD_COMPRESSED_FASTER)) {
            compressMethod = HANDLE_METHOD_ARJ;
        } else if (nMethod == CMETHOD_COMPRESSED_FASTEST) {
            compressMethod = HANDLE_METHOD_ARJ_FASTEST;
        }

        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        // ARJ GARBLE encryption: bit 0 of arj_flags
        if (nArjFlags & 0x01) {
            result.mapProperties.insert(XBinary::FPART_PROP_PASSWORD_MODIFIER, (quint32)nPasswordModifier);
        }

        // Convert DOS date/time
        qint32 nYear = ((nDosDateTime >> 25) & 0x7F) + 1980;
        qint32 nMonth = (nDosDateTime >> 21) & 0x0F;
        qint32 nDay = (nDosDateTime >> 16) & 0x1F;
        qint32 nHour = (nDosDateTime >> 11) & 0x1F;
        qint32 nMinute = (nDosDateTime >> 5) & 0x3F;
        qint32 nSecond = (nDosDateTime & 0x1F) * 2;

        QDateTime dtMTime(QDate(nYear, nMonth, nDay), QTime(nHour, nMinute, nSecond));

        if (dtMTime.isValid()) {
            result.mapProperties.insert(XBinary::FPART_PROP_MTIME, dtMTime);
        }
    }

    return result;
}

bool XARJ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        qint64 nEntryHeaderSize = _getEntryHeaderSize(pState->nCurrentOffset);
        quint32 nCompressedSize = read_uint32(pState->nCurrentOffset + 4 + 12, false);

        pState->nCurrentOffset += nEntryHeaderSize + nCompressedSize;
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XARJ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pState)
    Q_UNUSED(pPdStruct)

    return true;
}

QString XARJ::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XARJ_STRUCTID, sizeof(_TABLE_XARJ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XARJ::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XARJ_STRUCTID, sizeof(_TABLE_XARJ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XARJ::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XARJ_STRUCTID, sizeof(_TABLE_XARJ_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XARJ::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        qint64 nRealSize = 0;
        qint32 nCount = 0;
        qint64 nOffset = 0;
        qint64 nFileSize = getSize();

        // Skip main archive header
        if (nFileSize >= 4) {
            quint8 nByte0 = read_uint8(0);
            quint8 nByte1 = read_uint8(1);
            quint16 nHdrSize = read_uint16(2, false);

            if ((nByte0 == 0x60) && (nByte1 == 0xEA) && (nHdrSize > 0)) {
                qint64 nMainHeaderSize = _getEntryHeaderSize(0);

                if (nMainHeaderSize > 0) {
                    nOffset = nMainHeaderSize;
                }
            }
        }

        // Count file records
        qint64 nCurOffset = nOffset;

        while ((nCurOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            quint8 nByte0 = read_uint8(nCurOffset);
            quint8 nByte1 = read_uint8(nCurOffset + 1);
            quint16 nBasicHeaderSize = read_uint16(nCurOffset + 2, false);

            if ((nByte0 != 0x60) || (nByte1 != 0xEA) || (nBasicHeaderSize == 0)) {
                break;
            }

            qint64 nEntryHeaderSize = _getEntryHeaderSize(nCurOffset);

            if (nEntryHeaderSize <= 0) {
                break;
            }

            quint32 nCompressedSize = read_uint32(nCurOffset + 4 + 12, false);

            nCount++;
            nRealSize = nCurOffset + nEntryHeaderSize + nCompressedSize;

            nCurOffset += nEntryHeaderSize + nCompressedSize;
        }

        _dataHeadersOptions.nID = STRUCTID_RECORD;
        _dataHeadersOptions.nLocation = nOffset;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nCount = nCount;
        _dataHeadersOptions.nSize = nRealSize - nOffset;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_RECORD) {
                qint64 nCurrentOffset = nStartOffset;
                qint32 nCount = 0;

                while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    quint8 nByte0 = read_uint8(nCurrentOffset);
                    quint8 nByte1 = read_uint8(nCurrentOffset + 1);
                    quint16 nBasicHeaderSize = read_uint16(nCurrentOffset + 2, false);

                    if ((nByte0 != 0x60) || (nByte1 != 0xEA) || (nBasicHeaderSize == 0)) {
                        break;
                    }

                    qint64 nEntryHeaderSize = _getEntryHeaderSize(nCurrentOffset);

                    if (nEntryHeaderSize <= 0) {
                        break;
                    }

                    quint32 nCompressedSize = read_uint32(nCurrentOffset + 4 + 12, false);

                    DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
                    dataHeader.nSize = nEntryHeaderSize + nCompressedSize;

                    // Basic header fields (offsets relative to start of entry)
                    dataHeader.listRecords.append(getDataRecord(0, 2, "Marker", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(2, 2, "Basic Header Size", VT_UINT16, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(4, 1, "First Hdr Size", VT_UINT8, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(5, 1, "Archiver Version", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(6, 1, "Min Version", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(7, 1, "Host OS", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(8, 1, "ARJ Flags", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(9, 1, "Method", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(10, 1, "File Type", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(11, 1, "Reserved", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(12, 4, "Date/Time", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(16, 4, "Compressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(20, 4, "Original Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(24, 4, "CRC32", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(28, 2, "Entry Name Pos", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(30, 2, "File Access Mode", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(32, 1, "First Chapter", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(33, 1, "Last Chapter", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                    if (nBasicHeaderSize > FIXED_HEADER_SIZE) {
                        dataHeader.listRecords.append(getDataRecord(4 + FIXED_HEADER_SIZE, nBasicHeaderSize - FIXED_HEADER_SIZE, "Filename+Comment", VT_CHAR_ARRAY,
                                                                    DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    if (nCompressedSize > 0) {
                        dataHeader.listRecords.append(
                            getDataRecord(nEntryHeaderSize, nCompressedSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    listResult.append(dataHeader);

                    nCurrentOffset += nEntryHeaderSize + nCompressedSize;
                    nCount++;
                }
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XARJ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nFileSize = getSize();
    qint64 nCurrentOffset = 0;
    qint64 nMaxOffset = 0;

    // Skip main archive header
    if (nFileSize >= 4) {
        quint8 nByte0 = read_uint8(0);
        quint8 nByte1 = read_uint8(1);
        quint16 nHdrSize = read_uint16(2, false);

        if ((nByte0 == 0x60) && (nByte1 == 0xEA) && (nHdrSize > 0)) {
            qint64 nMainHeaderSize = _getEntryHeaderSize(0);

            if (nMainHeaderSize > 0) {
                nCurrentOffset = nMainHeaderSize;
            }
        }
    }

    while ((nCurrentOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint8 nByte0 = read_uint8(nCurrentOffset);
        quint8 nByte1 = read_uint8(nCurrentOffset + 1);
        quint16 nBasicHeaderSize = read_uint16(nCurrentOffset + 2, false);

        if ((nByte0 != 0x60) || (nByte1 != 0xEA) || (nBasicHeaderSize == 0)) {
            break;
        }

        qint64 nEntryHeaderSize = _getEntryHeaderSize(nCurrentOffset);

        if (nEntryHeaderSize <= 0) {
            break;
        }

        quint32 nCompressedSize = read_uint32(nCurrentOffset + 4 + 12, false);
        quint32 nOriginalSize = read_uint32(nCurrentOffset + 4 + 16, false);
        QString sFileName = _getFileName(nCurrentOffset);

        if (nFileParts & FILEPART_HEADER) {
            FPART record = {};

            record.filePart = FILEPART_HEADER;
            record.nFileOffset = nCurrentOffset;
            record.nFileSize = nEntryHeaderSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Header");

            listResult.append(record);
        }

        if (nFileParts & FILEPART_STREAM) {
            FPART record = {};

            record.filePart = FILEPART_STREAM;
            record.nFileOffset = nCurrentOffset + nEntryHeaderSize;
            record.nFileSize = nCompressedSize;
            record.nVirtualAddress = -1;
            record.sName = sFileName;
            record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nOriginalSize);

            listResult.append(record);
        }

        if (nFileParts & FILEPART_REGION) {
            FPART record = {};

            record.filePart = FILEPART_REGION;
            record.nFileOffset = nCurrentOffset;
            record.nFileSize = nEntryHeaderSize + nCompressedSize;
            record.nVirtualAddress = -1;
            record.sName = sFileName;

            listResult.append(record);
        }

        nMaxOffset = nCurrentOffset + nEntryHeaderSize + nCompressedSize;
        nCurrentOffset += nEntryHeaderSize + nCompressedSize;
    }

    // Overlay
    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < nFileSize) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nMaxOffset;
            record.nFileSize = nFileSize - nMaxOffset;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

QString XARJ::cmethodToString(CMETHOD cmethod)
{
    QString sResult;

    if (cmethod == CMETHOD_STORED) {
        sResult = "Stored";
    } else if (cmethod == CMETHOD_COMPRESSED_MOST) {
        sResult = "Compressed (most)";
    } else if (cmethod == CMETHOD_COMPRESSED) {
        sResult = "Compressed";
    } else if (cmethod == CMETHOD_COMPRESSED_FASTER) {
        sResult = "Compressed (faster)";
    } else if (cmethod == CMETHOD_COMPRESSED_FASTEST) {
        sResult = "Compressed (fastest)";
    } else if (cmethod == CMETHOD_NO_COMPRESSION_1) {
        sResult = "No compression";
    } else if (cmethod == CMETHOD_NO_COMPRESSION_2) {
        sResult = "No compression (type 2)";
    } else {
        sResult = "Unknown";
    }

    return sResult;
}

qint64 XARJ::_getEntryHeaderSize(qint64 nOffset)
{
    // Entry starts with: 2 bytes marker, 2 bytes basic_header_size
    if ((nOffset + 4) > getSize()) {
        return -1;
    }

    quint16 nBasicHeaderSize = read_uint16(nOffset + 2, false);

    if (nBasicHeaderSize == 0) {
        return 4;  // End-of-archive marker
    }

    if ((nOffset + 4 + nBasicHeaderSize + 4) > getSize()) {
        return -1;
    }

    // Skip: marker(2) + basic_hdr_size_field(2) + basic_header(nBasicHeaderSize) + CRC32(4)
    qint64 nPos = nOffset + 2 + 2 + nBasicHeaderSize + 4;

    // Skip extended headers: each is 2-byte length + data + 4-byte CRC32
    // Terminated by 0x0000
    while (true) {
        if ((nPos + 2) > getSize()) {
            break;
        }

        quint16 nExtSize = read_uint16(nPos, false);
        nPos += 2;

        if (nExtSize == 0) {
            break;
        }

        // extended header data + CRC32
        nPos += nExtSize + 4;

        if (nPos > getSize()) {
            break;
        }
    }

    return nPos - nOffset;
}

bool XARJ::_isValidEntry(qint64 nOffset)
{
    if ((nOffset + 4) > getSize()) {
        return false;
    }

    quint8 nByte0 = read_uint8(nOffset);
    quint8 nByte1 = read_uint8(nOffset + 1);

    return (nByte0 == 0x60) && (nByte1 == 0xEA);
}

QString XARJ::_getFileName(qint64 nOffset)
{
    // first_hdr_size (byte at offset+4) gives the actual fixed header size
    // (30 for old ARJ, 34 for ARJ v2.50+). Filename follows the fixed part.
    quint8 nFirstHdrSize = read_uint8(nOffset + 4);
    qint64 nNameOffset = nOffset + 4 + nFirstHdrSize;
    quint16 nBasicHeaderSize = read_uint16(nOffset + 2, false);
    qint64 nMaxNameLen = (nOffset + 4 + nBasicHeaderSize) - nNameOffset;

    if (nMaxNameLen <= 0) {
        return QString();
    }

    return read_ansiString(nNameOffset, (qint32)nMaxNameLen);
}

QList<QString> XARJ::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("60EA");

    return listResult;
}

XBinary *XARJ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XARJ(pDevice);
}

