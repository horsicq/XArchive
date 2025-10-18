/* Copyright (c) 2023-2025 hors<horsicq@gmail.com>
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
#include "Algos/xlzhdecoder.h"
#include "Algos/xstoredecoder.h"

XBinary::XCONVERT _TABLE_XLHA_STRUCTID[] = {
    {XLHA::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XLHA::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XLHA::STRUCTID_RECORD, "RECORD", QString("Record")},
};

XLHA::XLHA(QIODevice *pDevice) : XArchive(pDevice)
{
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

bool XLHA::isValid(QIODevice *pDevice)
{
    XLHA xhla(pDevice);

    return xhla.isValid();
}

quint64 XLHA::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return getRecords(-1, pPdStruct).count();
}

QList<XArchive::RECORD> XLHA::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

    qint64 nFileSize = getSize();

    qint64 nOffset = 0;

    qint32 nNumberOfFiles = 0;

    while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (compareSignature(&memoryMap, "....'-lh'..2d", nOffset) || compareSignature(&memoryMap, "....'-lz'..2d", nOffset) ||
            compareSignature(&memoryMap, "....'-pm'..2d", nOffset)) {
            qint64 nHeaderSize = read_uint8(nOffset) + 2;
            qint64 nCompressedSize = read_uint32(nOffset + 7);
            qint64 nUncompressedSize = read_uint32(nOffset + 11);
            QString sFileName = read_ansiString(nOffset + 22, read_uint8(nOffset + 21));
            sFileName = sFileName.replace("\\", "/");

            if (nHeaderSize < 21) {
                break;
            }

            XArchive::RECORD record = {};
            // TODO CRC
            record.spInfo.compressMethod = COMPRESS_METHOD_LZH5;

            QString sMethod = read_ansiString(nOffset + 2, 5);

            if ((sMethod == "-lh0-") || (sMethod == "-lz4-") || (sMethod == "-lhd-")) {
                record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
            } else if (sMethod == "-lh5-") {
                record.spInfo.compressMethod = COMPRESS_METHOD_LZH5;
            } else if (sMethod == "-lh6-") {
                record.spInfo.compressMethod = COMPRESS_METHOD_LZH6;
            } else if (sMethod == "-lh7-") {
                record.spInfo.compressMethod = COMPRESS_METHOD_LZH7;
            }

            record.nHeaderOffset = nOffset;
            record.nDataOffset = nOffset + nHeaderSize;
            record.spInfo.nUncompressedSize = nUncompressedSize;
            record.nHeaderSize = nHeaderSize;
            record.nDataSize = nCompressedSize;
            record.spInfo.sRecordName = sFileName;

            listResult.append(record);

            nNumberOfFiles++;

            if (nLimit != -1) {
                if (nNumberOfFiles > nLimit) {
                    break;
                }
            }

            nOffset += (nHeaderSize + nCompressedSize);
            nFileSize -= (nHeaderSize + nCompressedSize);
        } else {
            break;
        }
    }

    return listResult;
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

bool XLHA::initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

        // Count total number of records
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

        qint64 nOffset = 0;
        qint64 nFileSize = pState->nTotalSize;

        while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (compareSignature(&memoryMap, "....'-lh'..2d", nOffset) || compareSignature(&memoryMap, "....'-lz'..2d", nOffset) ||
                compareSignature(&memoryMap, "....'-pm'..2d", nOffset)) {
                qint64 nHeaderSize = read_uint8(nOffset) + 2;
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
        qint64 nHeaderSize = read_uint8(pState->nCurrentOffset) + 2;
        qint64 nCompressedSize = read_uint32(pState->nCurrentOffset + 7);
        qint64 nUncompressedSize = read_uint32(pState->nCurrentOffset + 11);
        QString sFileName = read_ansiString(pState->nCurrentOffset + 22, read_uint8(pState->nCurrentOffset + 21));
        sFileName = sFileName.replace("\\", "/");

        result.nStreamOffset = pState->nCurrentOffset + nHeaderSize;
        result.nStreamSize = nCompressedSize;
        result.nDecompressedOffset = 0;
        result.nDecompressedSize = nUncompressedSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);

        // Get compression method
        QString sMethod = read_ansiString(pState->nCurrentOffset + 2, 5);
        XBinary::COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;

        if ((sMethod == "-lh0-") || (sMethod == "-lz4-") || (sMethod == "-lhd-")) {
            compressMethod = COMPRESS_METHOD_STORE;
        } else if (sMethod == "-lh5-") {
            compressMethod = COMPRESS_METHOD_LZH5;
        } else if (sMethod == "-lh6-") {
            compressMethod = COMPRESS_METHOD_LZH6;
        } else if (sMethod == "-lh7-") {
            compressMethod = COMPRESS_METHOD_LZH7;
        }

        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
    }

    return result;
}

bool XLHA::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pState && pDevice && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        qint64 nHeaderSize = read_uint8(pState->nCurrentOffset) + 2;
        qint64 nCompressedSize = read_uint32(pState->nCurrentOffset + 7);
        qint64 nUncompressedSize = read_uint32(pState->nCurrentOffset + 11);
        qint64 nDataOffset = pState->nCurrentOffset + nHeaderSize;
        COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;

        // Create a RECORD structure for decompression


        // Get compression method
        QString sMethod = read_ansiString(pState->nCurrentOffset + 2, 5);

        if ((sMethod == "-lh0-") || (sMethod == "-lz4-") || (sMethod == "-lhd-")) {
            compressMethod = COMPRESS_METHOD_STORE;
        } else if (sMethod == "-lh5-") {
            compressMethod = COMPRESS_METHOD_LZH5;
        } else if (sMethod == "-lh6-") {
            compressMethod = COMPRESS_METHOD_LZH6;
        } else if (sMethod == "-lh7-") {
            compressMethod = COMPRESS_METHOD_LZH7;
        }

        // Decompress the record
        XBinary::DECOMPRESS_STATE state = {};
        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
        state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);

        SubDevice sd(getDevice(), nDataOffset, nCompressedSize);
        if (sd.open(QIODevice::ReadOnly)) {
            state.pDeviceInput = &sd;
            state.pDeviceOutput = pDevice;
            state.nInputOffset = 0;
            state.nInputLimit = nCompressedSize;

            if (compressMethod == COMPRESS_METHOD_STORE) {
                bResult = XStoreDecoder::decompress(&state, pPdStruct);
            } else if (compressMethod == COMPRESS_METHOD_LZH5) {
                bResult = XLZHDecoder::decompress(&state, 5, pPdStruct);
            } else if (compressMethod == COMPRESS_METHOD_LZH6) {
                bResult = XLZHDecoder::decompress(&state, 6, pPdStruct);
            } else if (compressMethod == COMPRESS_METHOD_LZH7) {
                bResult = XLZHDecoder::decompress(&state, 7, pPdStruct);
            }
            sd.close();
        }
    }

    return bResult;
}

bool XLHA::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        qint64 nHeaderSize = read_uint8(pState->nCurrentOffset) + 2;
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
            if (compareSignature(dataHeadersOptions.pMemoryMap, "....'-lh'..2d", nOffset) ||
                compareSignature(dataHeadersOptions.pMemoryMap, "....'-lz'..2d", nOffset) ||
                compareSignature(dataHeadersOptions.pMemoryMap, "....'-pm'..2d", nOffset)) {
                qint64 nHeaderSize = read_uint8(nOffset) + 2;
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
                        qint64 nHeaderSize = read_uint8(nCurrentOffset) + 2;
                        qint64 nDataSize = read_uint32(nCurrentOffset + 7);
                        QString sFileName = read_ansiString(nCurrentOffset + 22, read_uint8(nCurrentOffset + 21));

                        DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
                        dataHeader.nSize = nHeaderSize + nDataSize;

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

                        if (nDataSize > 0) {
                            dataHeader.listRecords.append(
                                getDataRecord(nHeaderSize, nDataSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                        }

                        listResult.append(dataHeader);

                        nCurrentOffset += (nHeaderSize + nDataSize);
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
            qint64 nHeaderSize = read_uint8(nCurrentOffset) + 2;
            qint64 nDataSize = read_uint32(nCurrentOffset + 7);
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
                record.nFileOffset = nCurrentOffset + nHeaderSize;
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
                record.nFileSize = nHeaderSize + nDataSize;
                record.nVirtualAddress = -1;
                record.sName = sFileName;

                listResult.append(record);
            }

            nMaxOffset = nCurrentOffset + nHeaderSize + nDataSize;
            nCurrentOffset += (nHeaderSize + nDataSize);
            nFileSize -= (nHeaderSize + nDataSize);
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

