/* Copyright (c) 2023-2026 hors<horsicq@gmail.com>
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
#include "xlha.h"
#include "xdecompress.h"

XBinary::XCONVERT _TABLE_XLHA_STRUCTID[] = {
    {XLHA::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XLHA::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XLHA::STRUCTID_RECORD, "RECORD", QString("Record")},
};

XLHA::XLHA(QIODevice *pDevice) : XArchive(pDevice)
{
}

// Level 1 archives store skip_sz at bytes 7-10 which equals ext_headers + csz.
// The extended header chain starts at nOffset+nBaseHeaderSize and each entry
// ends with a LE16 "next entry size" (0 = end of chain).
// Returns the total byte count occupied by extended headers.
qint64 XLHA::_getLevel1ExtHeadersSize(qint64 nOffset, qint64 nBaseHeaderSize)
{
    qint64 nExtTotal = 0;
    qint64 nFnLen = (qint64)read_uint8(nOffset + 21);
    qint64 nNextHdrSize = (qint64)read_uint16(nOffset + 25 + nFnLen, false);
    qint64 nExtOffset = nOffset + nBaseHeaderSize;
    while (nNextHdrSize > 0) {
        nExtTotal += nNextHdrSize;
        nExtOffset += nNextHdrSize;
        nNextHdrSize = (qint64)read_uint16(nExtOffset - 2, false);
    }
    return nExtTotal;
}

bool XLHA::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 12) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "....'-lh'..2d", 0, pPdStruct) || compareSignature(&memoryMap, "....'-lz'..2d", 0, pPdStruct) ||
            compareSignature(&memoryMap, "....'-pm'..2d", 0, pPdStruct)) {
            QString sMethod = read_ansiString(2, 5);

            if ((sMethod == "-lzs-") || (sMethod == "-lz2-") || (sMethod == "-lz3-") || (sMethod == "-lz4-") || (sMethod == "-lz5-") || (sMethod == "-lz7-") ||
                (sMethod == "-lz8-") || (sMethod == "-lh0-") || (sMethod == "-lh1-") || (sMethod == "-lh2-") || (sMethod == "-lh3-") || (sMethod == "-lh4-") ||
                (sMethod == "-lh5-") || (sMethod == "-lh6-") || (sMethod == "-lh7-") || (sMethod == "-lh8-") || (sMethod == "-lh9-") || (sMethod == "-lha-") ||
                (sMethod == "-lhb-") || (sMethod == "-lhc-") || (sMethod == "-lhe-") || (sMethod == "-lhd-") || (sMethod == "-lhx-") || (sMethod == "-pm0-") ||
                (sMethod == "-pm2-")) {
                bResult = true;
            }
            bResult = true;
        }
    }

    return bResult;
}

bool XLHA::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLHA xhla(pDevice);

    return xhla.isValid();
}

qint64 XLHA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XLHA::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XLHA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::FT XLHA::getFileType()
{
    return FT_LHA;
}

QString XLHA::getFileFormatExt()
{
    QString sResult = "lha";
    QString _sVersion = getVersion().left(2);

    if (_sVersion == "lh") {
        sResult = "lha";
    } else if (_sVersion == "lz") {
        sResult = "lzs";
    } else if (_sVersion == "pm") {
        sResult = "pma";
    }

    return sResult;
}

QString XLHA::getFileFormatExtsString()
{
    return "LHA(lha, lzs, pma)";
}

QString XLHA::getMIMEString()
{
    return "application/x-lzh-compressed";
}

QString XLHA::getVersion()
{
    return read_ansiString(3, 3);
}

QString XLHA::getArch()
{
    return QString();
}

XBinary::ENDIAN XLHA::getEndian()
{
    return ENDIAN_LITTLE;  // LHA is little-endian
}

XBinary::MODE XLHA::getMode()
{
    return MODE_DATA;
}

bool XLHA::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

        // Count total number of records
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

        qint64 nOffset = 0;
        qint64 nFileSize = pState->nTotalSize;

        while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (compareSignature(&memoryMap, "....'-lh'..2d", nOffset) || compareSignature(&memoryMap, "....'-lz'..2d", nOffset) ||
                compareSignature(&memoryMap, "....'-pm'..2d", nOffset)) {
                quint8 nLevel = read_uint8(nOffset + 20);
                qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nOffset) : (qint64)(read_uint8(nOffset) + 2);
                qint64 nCompressedSize = read_uint32(nOffset + 7);

                if (nHeaderSize < 21) {
                    break;
                }

                pState->nNumberOfRecords++;

                nOffset += (nHeaderSize + nCompressedSize);
                nFileSize -= (nHeaderSize + nCompressedSize);
            } else {
                break;
            }
        }

        bResult = (pState->nNumberOfRecords > 0);
    }

    return bResult;
}

XBinary::ARCHIVERECORD XLHA::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        quint8 nLevel = read_uint8(pState->nCurrentOffset + 20);
        qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(pState->nCurrentOffset) : (qint64)(read_uint8(pState->nCurrentOffset) + 2);
        qint64 nSkipSize = (qint64)(quint32)read_uint32(pState->nCurrentOffset + 7);
        qint64 nUncompressedSize = read_uint32(pState->nCurrentOffset + 11);
        QString sFileName = read_ansiString(pState->nCurrentOffset + 22, read_uint8(pState->nCurrentOffset + 21));
        sFileName = sFileName.replace("\\", "/");

        // For Level 1: skip_sz = ext_headers + compressed_data; resolve actual offset/size.
        qint64 nExtSize = (nLevel == 1) ? _getLevel1ExtHeadersSize(pState->nCurrentOffset, nHeaderSize) : 0;
        qint64 nCompressedSize = nSkipSize - nExtSize;

        result.nStreamOffset = pState->nCurrentOffset + nHeaderSize + nExtSize;
        result.nStreamSize = nCompressedSize;
        // result.nDecompressedOffset = 0;
        // result.nDecompressedSize = nUncompressedSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);

        // Get compression method
        QString sMethod = read_ansiString(pState->nCurrentOffset + 2, 5);
        XBinary::HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

        if ((sMethod == "-lh0-") || (sMethod == "-lz4-") || (sMethod == "-lhd-")) {
            compressMethod = HANDLE_METHOD_STORE;
        } else if (sMethod == "-lh5-") {
            compressMethod = HANDLE_METHOD_LZH5;
        } else if (sMethod == "-lh6-") {
            compressMethod = HANDLE_METHOD_LZH6;
        } else if (sMethod == "-lh7-") {
            compressMethod = HANDLE_METHOD_LZH7;
        }

        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
    }

    return result;
}

bool XLHA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        quint8 nLevel = read_uint8(pState->nCurrentOffset + 20);
        qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(pState->nCurrentOffset) : (qint64)(read_uint8(pState->nCurrentOffset) + 2);
        qint64 nCompressedSize = read_uint32(pState->nCurrentOffset + 7);

        pState->nCurrentOffset += (nHeaderSize + nCompressedSize);
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XLHA::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pState)
    Q_UNUSED(pPdStruct)

    return true;
}

QString XLHA::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XLHA::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XLHA::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XLHA_STRUCTID, sizeof(_TABLE_XLHA_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XLHA::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        // Count records for table
        qint64 nRealSize = 0;
        qint32 nCount = 0;

        qint64 nFileSize = getSize();
        qint64 nOffset = 0;

        while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (compareSignature(dataHeadersOptions.pMemoryMap, "....'-lh'..2d", nOffset) || compareSignature(dataHeadersOptions.pMemoryMap, "....'-lz'..2d", nOffset) ||
                compareSignature(dataHeadersOptions.pMemoryMap, "....'-pm'..2d", nOffset)) {
                quint8 nLevel = read_uint8(nOffset + 20);
                qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nOffset) : (qint64)(read_uint8(nOffset) + 2);
                qint64 nDataSize = read_uint32(nOffset + 7);

                if (nHeaderSize < 21) {
                    break;
                }

                nCount++;
                nRealSize = nOffset + nHeaderSize + nDataSize;

                nOffset += (nHeaderSize + nDataSize);
                nFileSize -= (nHeaderSize + nDataSize);
            } else {
                break;
            }
        }

        _dataHeadersOptions.nID = STRUCTID_RECORD;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;
        _dataHeadersOptions.nCount = nCount;
        _dataHeadersOptions.nSize = nRealSize;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_RECORD) {
                // Table of records
                qint64 nCurrentOffset = nStartOffset;
                qint32 nCount = 0;

                while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    if (compareSignature(dataHeadersOptions.pMemoryMap, "....'-lh'..2d", nCurrentOffset) ||
                        compareSignature(dataHeadersOptions.pMemoryMap, "....'-lz'..2d", nCurrentOffset) ||
                        compareSignature(dataHeadersOptions.pMemoryMap, "....'-pm'..2d", nCurrentOffset)) {
                        quint8 nLevel = read_uint8(nCurrentOffset + 20);
                        qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nCurrentOffset) : (qint64)(read_uint8(nCurrentOffset) + 2);
                        qint64 nSkipSize = (qint64)(quint32)read_uint32(nCurrentOffset + 7);
                        qint64 nExtSize = (nLevel == 1) ? _getLevel1ExtHeadersSize(nCurrentOffset, nHeaderSize) : 0;
                        qint64 nDataSize = nSkipSize - nExtSize;
                        QString sFileName = read_ansiString(nCurrentOffset + 22, read_uint8(nCurrentOffset + 21));

                        DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
                        dataHeader.nSize = nHeaderSize + nSkipSize;

                        // Record header fields
                        dataHeader.listRecords.append(getDataRecord(0, 1, "Header Size", VT_UINT8, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(1, 1, "Header CRC", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(2, 5, "Compression Method", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(7, 4, "Compressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(11, 4, "Uncompressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(15, 2, "Last Mod Time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(17, 2, "Last Mod Date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(19, 1, "File Attribute", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(getDataRecord(20, 1, "Name Length", VT_UINT8, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
                        dataHeader.listRecords.append(
                            getDataRecord(21, read_uint8(nCurrentOffset + 21), "File Name", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                        if (nHeaderSize > 22 + read_uint8(nCurrentOffset + 21)) {
                            dataHeader.listRecords.append(getDataRecord(22 + read_uint8(nCurrentOffset + 21), nHeaderSize - (22 + read_uint8(nCurrentOffset + 21)),
                                                                        "Extended Header", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        }

                        if (nExtSize > 0) {
                            dataHeader.listRecords.append(
                                getDataRecord(nHeaderSize, nExtSize, "Extended Headers", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        }

                        if (nDataSize > 0) {
                            dataHeader.listRecords.append(
                                getDataRecord(nHeaderSize + nExtSize, nDataSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        }

                        listResult.append(dataHeader);

                        nCurrentOffset += (nHeaderSize + nSkipSize);
                        nCount++;
                    } else {
                        break;
                    }
                }
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XLHA::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nFileSize = getSize();
    qint64 nCurrentOffset = 0;
    qint64 nMaxOffset = 0;
    _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

    // Iterate through all records and create file parts
    while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (compareSignature(&memoryMap, "....'-lh'..2d", nCurrentOffset) || compareSignature(&memoryMap, "....'-lz'..2d", nCurrentOffset) ||
            compareSignature(&memoryMap, "....'-pm'..2d", nCurrentOffset)) {
            quint8 nLevel = read_uint8(nCurrentOffset + 20);
            qint64 nHeaderSize = (nLevel == 2) ? (qint64)read_uint16(nCurrentOffset) : (qint64)(read_uint8(nCurrentOffset) + 2);
            qint64 nSkipSize = (qint64)(quint32)read_uint32(nCurrentOffset + 7);
            qint64 nExtSize = (nLevel == 1) ? _getLevel1ExtHeadersSize(nCurrentOffset, nHeaderSize) : 0;
            qint64 nDataSize = nSkipSize - nExtSize;
            QString sFileName = read_ansiString(nCurrentOffset + 22, read_uint8(nCurrentOffset + 21));

            if (nHeaderSize < 21) {
                break;
            }

            // Header part
            if (nFileParts & FILEPART_HEADER) {
                FPART record = {};

                record.filePart = FILEPART_HEADER;
                record.nFileOffset = nCurrentOffset;
                record.nFileSize = nHeaderSize;
                record.nVirtualAddress = -1;
                record.sName = tr("Header");

                listResult.append(record);
            }

            // Data/Stream part
            if (nFileParts & FILEPART_STREAM) {
                FPART record = {};

                record.filePart = FILEPART_STREAM;
                record.nFileOffset = nCurrentOffset + nHeaderSize + nExtSize;
                record.nFileSize = nDataSize;
                record.nVirtualAddress = -1;
                record.sName = sFileName;
                record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, read_uint32(nCurrentOffset + 11));

                listResult.append(record);
            }

            // Region part (header + data)
            if (nFileParts & FILEPART_REGION) {
                FPART record = {};

                record.filePart = FILEPART_REGION;
                record.nFileOffset = nCurrentOffset;
                record.nFileSize = nHeaderSize + nSkipSize;
                record.nVirtualAddress = -1;
                record.sName = sFileName;

                listResult.append(record);
            }

            nMaxOffset = nCurrentOffset + nHeaderSize + nSkipSize;
            nCurrentOffset += (nHeaderSize + nSkipSize);
            nFileSize -= (nHeaderSize + nSkipSize);
        } else {
            break;
        }
    }

    // Data part (all archive data)
    if (nFileParts & FILEPART_DATA) {
        FPART record = {};

        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nMaxOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    // Overlay part (any trailing data)
    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < getSize()) {
            FPART record = {};

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

QList<QString> XLHA::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("....'-lh'..2d");
    listResult.append("....'-lz'..2d");
    listResult.append("....'-pm'..2d");

    return listResult;
}

XBinary *XLHA::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XLHA(pDevice);
}

