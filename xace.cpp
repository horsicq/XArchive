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
#include "xace.h"
#include "xdecompress.h"

// ACE magic: "**ACE**" at offset 7 of the archive header block
const quint8 XACE::MAGIC[7] = {0x2A, 0x2A, 0x41, 0x43, 0x45, 0x2A, 0x2A};

XBinary::XCONVERT _TABLE_XACE_STRUCTID[] = {
    {XACE::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XACE::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XACE::STRUCTID_RECORD, "RECORD", QString("Record")},
};

XACE::XACE(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XACE::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    // ACE archive: minimum 14 bytes needed (4 common hdr + 10 bytes into archive block)
    // The archive header block (head_type=0) must start at offset 0.
    // Layout: [0-1]=head_crc, [2-3]=head_size, [4]=head_type(0), [5-6]=head_flags
    //         [7-13]="**ACE**" magic
    // head_size counts bytes from head_type onwards, so minimum head_size = 10 (for magic check at +3..+9 of head_size data)
    if (getSize() >= 14) {
        quint8 nHeadType = read_uint8(4);
        quint16 nHeadSize = read_uint16(2, false);

        if ((nHeadType == HEADTYPE_ARCHIVE) && (nHeadSize >= 10)) {
            // Check magic bytes at offset 7
            QByteArray baHeader = read_array(7, 7);

            if (baHeader.size() == 7) {
                bResult = (memcmp(baHeader.constData(), MAGIC, 7) == 0);
            }
        }
    }

    return bResult;
}

bool XACE::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XACE xace(pDevice);

    return xace.isValid();
}

qint64 XACE::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XACE::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XACE::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::FT XACE::getFileType()
{
    return FT_ACE;
}

QString XACE::getFileFormatExt()
{
    return "ace";
}

QString XACE::getFileFormatExtsString()
{
    return "ACE (*.ace)";
}

QString XACE::getMIMEString()
{
    return "application/x-ace";
}

QString XACE::getVersion()
{
    // ver_created is at offset 15 within file (byte 11 within archive header block data, i.e. head_type + 11)
    // Archive block: [4]=head_type, [5-6]=head_flags, [7-13]="**ACE**", [14]=ver_extract, [15]=ver_created
    if (getSize() >= 16) {
        quint8 nVerCreated = read_uint8(15);

        return QString::number(nVerCreated);
    }

    return QString();
}

QString XACE::getArch()
{
    return QString();
}

XBinary::ENDIAN XACE::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XACE::getMode()
{
    return MODE_DATA;
}

bool XACE::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

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

        qint64 nOffset = 0;
        qint64 nFileSize = pState->nTotalSize;

        // Skip the archive header block (head_type == 0)
        if (nFileSize >= 5) {
            quint8 nHeadType = read_uint8(nOffset + 4);

            if (nHeadType == HEADTYPE_ARCHIVE) {
                qint64 nArchHdrSize = _getBlockHeaderSize(nOffset);

                if (nArchHdrSize > 0) {
                    nOffset += nArchHdrSize;
                }
            }
        }

        pState->nCurrentOffset = nOffset;

        // Count file records (head_type == 1)
        qint64 nCountOffset = nOffset;

        while (XBinary::isPdStructNotCanceled(pPdStruct) && (nCountOffset < nFileSize)) {
            if ((nFileSize - nCountOffset) < 5) {
                break;
            }

            quint8 nHeadType = read_uint8(nCountOffset + 4);
            quint16 nHeadSize = read_uint16(nCountOffset + 2, false);

            // End-of-archive block
            if ((nHeadType == HEADTYPE_EOF) || (nHeadSize == 0)) {
                break;
            }

            qint64 nBlockHdrSize = _getBlockHeaderSize(nCountOffset);

            if (nBlockHdrSize <= 0) {
                break;
            }

            if (nHeadType == HEADTYPE_FILE) {
                pState->nNumberOfRecords++;

                quint32 nCompSize = _getCompressedSize(nCountOffset);
                nCountOffset += nBlockHdrSize + nCompSize;
            } else {
                // Skip non-file blocks (recovery records, etc.)
                quint32 nAddSize = 0;
                quint16 nHeadFlags = read_uint16(nCountOffset + 5, false);

                if (nHeadFlags & FILEFLAG_ADDSIZE) {
                    // Additional size field is at same position as compsize in file blocks
                    // For non-file blocks, it's a generic 4-byte add_size after head_flags
                    // In ACE, for non-file blocks with ADDSIZE, the 4 bytes immediately after head_flags carry add_size
                    // head_type at +4, head_flags at +5..+6, add_size at +7 for non-file blocks
                    nAddSize = read_uint32(nCountOffset + 7, false);
                }

                nCountOffset += nBlockHdrSize + nAddSize;
            }
        }

        bResult = (pState->nNumberOfRecords > 0);
    }

    return bResult;
}

XBinary::ARCHIVERECORD XACE::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        qint64 nBase = pState->nCurrentOffset;

        qint64 nBlockHdrSize = _getBlockHeaderSize(nBase);
        quint32 nCompSize = _getCompressedSize(nBase);

        // File header fields (all relative to start of block at nBase)
        // [7]  uint32 pack_size (compressed size)
        // [11] uint32 orig_size (uncompressed size)
        // [15] uint32 ftime (DOS date-time)
        // [19] uint32 attr (file attributes)
        // [23] uint32 crc32
        // [27] uint16 tech_info (low byte = method: 0=stored, 1=compressed; high byte = level)
        // [29] uint16 req_ver
        // [31] uint16 ???
        // [33] uint16 fname_size
        // [35] char[] fname

        quint32 nDateTime = read_uint32(nBase + 15, false);
        quint32 nAttr = read_uint32(nBase + 19, false);
        quint32 nUncompSize = read_uint32(nBase + 11, false);
        quint32 nCRC32 = read_uint32(nBase + 23, false);
        quint16 nTechInfo = read_uint16(nBase + 27, false);
        quint16 nHeadFlags = read_uint16(nBase + 5, false);

        quint8 nCompType = (quint8)(nTechInfo & 0xFF);

        QString sFileName = _getFileName(nBase);
        sFileName = sFileName.replace("\\", "/");

        result.nStreamOffset = nBase + nBlockHdrSize;
        result.nStreamSize = nCompSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMOFFSET, result.nStreamOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMSIZE, result.nStreamSize);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUncompSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)nCompSize);
        result.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, nCRC32 ^ 0xFFFFFFFF);
        result.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, nCRC32 != 0 ? XBinary::CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF : XBinary::CRC_TYPE_UNKNOWN);
        result.mapProperties.insert(XBinary::FPART_PROP_TYPE, (quint32)nCompType);

        // Encrypted flag
        if (nHeadFlags & FILEFLAG_ENCRYPT) {
            result.mapProperties.insert(XBinary::FPART_PROP_ENCRYPTED, true);
        }

        // Folder check: DOS archive attribute bit 4 (0x10)
        if (nAttr & 0x10) {
            result.mapProperties.insert(XBinary::FPART_PROP_ISFOLDER, true);
        }

        HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

        if (nCompType == CTYPE_STORED) {
            compressMethod = HANDLE_METHOD_STORE;
        } else if (nCompType == CTYPE_LZ_HUFFMAN) {
            compressMethod = HANDLE_METHOD_ACE;
        } else if (nCompType == CTYPE_LZ_HUFFMAN_DELTA) {
            compressMethod = HANDLE_METHOD_ACE_DELTA;
        }

        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        // Convert DOS date/time: same encoding as ARJ
        // DOS time: [4:0]=seconds/2, [10:5]=minutes, [15:11]=hours
        // DOS date: [4:0]=day, [8:5]=month, [15:9]=year-1980
        quint16 nDosTime = (quint16)(nDateTime & 0xFFFF);
        quint16 nDosDate = (quint16)(nDateTime >> 16);

        qint32 nSecond = (nDosTime & 0x1F) * 2;
        qint32 nMinute = (nDosTime >> 5) & 0x3F;
        qint32 nHour   = (nDosTime >> 11) & 0x1F;
        qint32 nDay    = nDosDate & 0x1F;
        qint32 nMonth  = (nDosDate >> 5) & 0x0F;
        qint32 nYear   = ((nDosDate >> 9) & 0x7F) + 1980;

        QDateTime dtMTime(QDate(nYear, nMonth, nDay), QTime(nHour, nMinute, nSecond));

        if (dtMTime.isValid()) {
            result.mapProperties.insert(XBinary::FPART_PROP_MTIME, dtMTime);
        }
    }

    return result;
}

bool XACE::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        qint64 nBlockHdrSize = _getBlockHeaderSize(pState->nCurrentOffset);
        quint32 nCompSize = _getCompressedSize(pState->nCurrentOffset);

        pState->nCurrentOffset += nBlockHdrSize + nCompSize;
        pState->nCurrentIndex++;

        // Skip non-file blocks if any
        while (pState->nCurrentIndex < pState->nNumberOfRecords) {
            if ((pState->nCurrentOffset + 5) > pState->nTotalSize) {
                break;
            }

            quint8 nHeadType = read_uint8(pState->nCurrentOffset + 4);

            if (nHeadType == HEADTYPE_FILE) {
                break;
            }

            if ((nHeadType == HEADTYPE_EOF) || (read_uint16(pState->nCurrentOffset + 2, false) == 0)) {
                break;
            }

            qint64 nSkipHdrSize = _getBlockHeaderSize(pState->nCurrentOffset);

            if (nSkipHdrSize <= 0) {
                break;
            }

            quint16 nHeadFlags = read_uint16(pState->nCurrentOffset + 5, false);
            quint32 nAddSize = 0;

            if (nHeadFlags & FILEFLAG_ADDSIZE) {
                nAddSize = read_uint32(pState->nCurrentOffset + 7, false);
            }

            pState->nCurrentOffset += nSkipHdrSize + nAddSize;
        }

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XACE::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pState)
    Q_UNUSED(pPdStruct)

    return true;
}

QString XACE::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XACE_STRUCTID, sizeof(_TABLE_XACE_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XACE::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XACE_STRUCTID, sizeof(_TABLE_XACE_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XACE::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XACE_STRUCTID, sizeof(_TABLE_XACE_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XACE::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
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
        qint64 nFileSize = getSize();

        // Skip archive header
        qint64 nOffset = 0;

        if (nFileSize >= 5) {
            quint8 nHeadType = read_uint8(4);

            if (nHeadType == HEADTYPE_ARCHIVE) {
                qint64 nArchHdrSize = _getBlockHeaderSize(0);

                if (nArchHdrSize > 0) {
                    nOffset = nArchHdrSize;
                }
            }
        }

        // Count file records
        qint64 nCurOffset = nOffset;

        while ((nCurOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nFileSize - nCurOffset) < 5) {
                break;
            }

            quint8 nHeadType = read_uint8(nCurOffset + 4);
            quint16 nHeadSize = read_uint16(nCurOffset + 2, false);

            if ((nHeadType == HEADTYPE_EOF) || (nHeadSize == 0)) {
                break;
            }

            qint64 nBlockHdrSize = _getBlockHeaderSize(nCurOffset);

            if (nBlockHdrSize <= 0) {
                break;
            }

            if (nHeadType == HEADTYPE_FILE) {
                quint32 nCompSize = _getCompressedSize(nCurOffset);

                nCount++;
                nRealSize = nCurOffset + nBlockHdrSize + nCompSize;
                nCurOffset += nBlockHdrSize + nCompSize;
            } else {
                quint16 nHeadFlags = read_uint16(nCurOffset + 5, false);
                quint32 nAddSize = 0;

                if (nHeadFlags & FILEFLAG_ADDSIZE) {
                    nAddSize = read_uint32(nCurOffset + 7, false);
                }

                nCurOffset += nBlockHdrSize + nAddSize;
            }
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
                qint64 nFileSize = getSize();

                while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    if ((nFileSize - nCurrentOffset) < 5) {
                        break;
                    }

                    quint8 nHeadType = read_uint8(nCurrentOffset + 4);
                    quint16 nHeadSize = read_uint16(nCurrentOffset + 2, false);

                    if ((nHeadType == HEADTYPE_EOF) || (nHeadSize == 0)) {
                        break;
                    }

                    qint64 nBlockHdrSize = _getBlockHeaderSize(nCurrentOffset);

                    if (nBlockHdrSize <= 0) {
                        break;
                    }

                    if (nHeadType != HEADTYPE_FILE) {
                        quint16 nHeadFlags = read_uint16(nCurrentOffset + 5, false);
                        quint32 nAddSize = 0;

                        if (nHeadFlags & FILEFLAG_ADDSIZE) {
                            nAddSize = read_uint32(nCurrentOffset + 7, false);
                        }

                        nCurrentOffset += nBlockHdrSize + nAddSize;
                        continue;
                    }

                    quint32 nCompSize = _getCompressedSize(nCurrentOffset);

                    DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
                    dataHeader.nSize = nBlockHdrSize + nCompSize;

                    dataHeader.listRecords.append(getDataRecord(0, 2, "Head CRC", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(2, 2, "Head Size", VT_UINT16, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(4, 1, "Head Type", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(5, 2, "Head Flags", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(7, 4, "Compressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(11, 4, "Uncompressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(15, 4, "Date/Time", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(19, 4, "Attributes", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(23, 4, "CRC32", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(27, 2, "Tech Info", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(29, 2, "Required Version", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(31, 2, "Reserved", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(33, 2, "Filename Size", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                    quint16 nFnameSize = read_uint16(nCurrentOffset + 33, false);

                    if (nFnameSize > 0) {
                        dataHeader.listRecords.append(getDataRecord(35, nFnameSize, "Filename", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    if (nCompSize > 0) {
                        dataHeader.listRecords.append(
                            getDataRecord(nBlockHdrSize, nCompSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    listResult.append(dataHeader);

                    nCurrentOffset += nBlockHdrSize + nCompSize;
                    nCount++;
                }
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XACE::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nFileSize = getSize();
    qint64 nCurrentOffset = 0;
    qint64 nMaxOffset = 0;

    // Skip archive header
    if (nFileSize >= 5) {
        quint8 nHeadType = read_uint8(4);

        if (nHeadType == HEADTYPE_ARCHIVE) {
            qint64 nArchHdrSize = _getBlockHeaderSize(0);

            if (nArchHdrSize > 0) {
                nCurrentOffset = nArchHdrSize;
            }
        }
    }

    while ((nCurrentOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nFileSize - nCurrentOffset) < 5) {
            break;
        }

        quint8 nHeadType = read_uint8(nCurrentOffset + 4);
        quint16 nHeadSize = read_uint16(nCurrentOffset + 2, false);

        if ((nHeadType == HEADTYPE_EOF) || (nHeadSize == 0)) {
            break;
        }

        qint64 nBlockHdrSize = _getBlockHeaderSize(nCurrentOffset);

        if (nBlockHdrSize <= 0) {
            break;
        }

        if (nHeadType == HEADTYPE_FILE) {
            quint32 nCompSize = _getCompressedSize(nCurrentOffset);
            quint32 nOrigSize = read_uint32(nCurrentOffset + 11, false);
            QString sFileName = _getFileName(nCurrentOffset);

            if (nFileParts & FILEPART_HEADER) {
                FPART record = {};

                record.filePart = FILEPART_HEADER;
                record.nFileOffset = nCurrentOffset;
                record.nFileSize = nBlockHdrSize;
                record.nVirtualAddress = -1;
                record.sName = tr("Header");

                listResult.append(record);
            }

            if (nFileParts & FILEPART_STREAM) {
                FPART record = {};

                record.filePart = FILEPART_STREAM;
                record.nFileOffset = nCurrentOffset + nBlockHdrSize;
                record.nFileSize = nCompSize;
                record.nVirtualAddress = -1;
                record.sName = sFileName;
                record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nOrigSize);

                listResult.append(record);
            }

            if (nFileParts & FILEPART_REGION) {
                FPART record = {};

                record.filePart = FILEPART_REGION;
                record.nFileOffset = nCurrentOffset;
                record.nFileSize = nBlockHdrSize + nCompSize;
                record.nVirtualAddress = -1;
                record.sName = sFileName;

                listResult.append(record);
            }

            nMaxOffset = nCurrentOffset + nBlockHdrSize + nCompSize;
            nCurrentOffset += nBlockHdrSize + nCompSize;
        } else {
            quint16 nHeadFlags = read_uint16(nCurrentOffset + 5, false);
            quint32 nAddSize = 0;

            if (nHeadFlags & FILEFLAG_ADDSIZE) {
                nAddSize = read_uint32(nCurrentOffset + 7, false);
            }

            nCurrentOffset += nBlockHdrSize + nAddSize;
        }
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

qint64 XACE::_getBlockHeaderSize(qint64 nOffset)
{
    // Generic ACE block: [0-1]=head_crc, [2-3]=head_size, [4]=head_type, ...
    // Total header = 4 + head_size
    if ((nOffset + 4) > getSize()) {
        return -1;
    }

    quint16 nHeadSize = read_uint16(nOffset + 2, false);

    if (nHeadSize == 0) {
        return 4;  // End-of-archive marker
    }

    if ((nOffset + 4 + nHeadSize) > getSize()) {
        return -1;
    }

    return 4 + nHeadSize;
}

bool XACE::_isFileBlock(qint64 nOffset)
{
    if ((nOffset + 5) > getSize()) {
        return false;
    }

    return read_uint8(nOffset + 4) == HEADTYPE_FILE;
}

bool XACE::_isValidBlock(qint64 nOffset)
{
    if ((nOffset + 4) > getSize()) {
        return false;
    }

    quint16 nHeadSize = read_uint16(nOffset + 2, false);

    return (nHeadSize > 0);
}

quint32 XACE::_getCompressedSize(qint64 nOffset)
{
    // File header: pack_size (compressed) at offset +7
    if ((nOffset + 11) > getSize()) {
        return 0;
    }

    if (!_isFileBlock(nOffset)) {
        return 0;
    }

    return read_uint32(nOffset + 7, false);
}

QString XACE::_getFileName(qint64 nOffset)
{
    // fname_size (uint16) at offset +33, fname at offset +35
    if ((nOffset + 35) > getSize()) {
        return QString();
    }

    quint16 nFnameSize = read_uint16(nOffset + 33, false);

    if (nFnameSize == 0) {
        return QString();
    }

    return read_ansiString(nOffset + 35, nFnameSize);
}

QList<QString> XACE::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("...............'**ACE**'");

    return listResult;
}

XBinary *XACE::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XACE(pDevice);
}

