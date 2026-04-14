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
#include "xseaarc.h"
#include "Algos/xstoredecoder.h"

XBinary::XCONVERT _TABLE_XSEAARC_STRUCTID[] = {
    {XSEAARC::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XSEAARC::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XSEAARC::STRUCTID_RECORD, "RECORD", QString("Record")},
};

XSEAARC::XSEAARC(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XSEAARC::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    // ARC archive: starts with 0x1A followed by method byte (1-9)
    // and 13-byte null-terminated filename
    if (getSize() >= 29) {  // Minimum: 1 (marker) + 1 (method) + 13 (name) + 4 (compressed size) + 2 (date) + 2 (time) + 2 (crc) + 4 (original size) = 29
        quint8 nMarker = read_uint8(0);
        quint8 nMethod = read_uint8(1);

        if ((nMarker == 0x1A) && (nMethod >= 1) && (nMethod <= 9)) {
            // Read filename (13 bytes, null-terminated ASCII)
            QByteArray baFileName = read_array(2, 13);

            // First byte of filename must be printable ASCII
            if (baFileName.size() >= 1) {
                quint8 nFirstChar = (quint8)baFileName.at(0);

                if ((nFirstChar >= 0x21) && (nFirstChar <= 0x7E)) {
                    // Verify all chars up to null are printable ASCII
                    bool bValidName = true;

                    for (qint32 i = 0; i < baFileName.size(); i++) {
                        quint8 nChar = (quint8)baFileName.at(i);

                        if (nChar == 0) {
                            break;  // Null terminator found
                        }

                        if ((nChar < 0x20) || (nChar > 0x7E)) {
                            bValidName = false;
                            break;
                        }
                    }

                    if (bValidName) {
                        bResult = true;
                    }
                }
            }
        }
    }

    return bResult;
}

bool XSEAARC::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSEAARC xseaarc(pDevice);

    return xseaarc.isValid();
}

qint64 XSEAARC::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XSEAARC::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XSEAARC::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

XBinary::FT XSEAARC::getFileType()
{
    return FT_ARC;
}

QString XSEAARC::getFileFormatExt()
{
    return "arc";
}

QString XSEAARC::getFileFormatExtsString()
{
    return "ARC (*.arc)";
}

QString XSEAARC::getMIMEString()
{
    return "application/x-arc";
}

QString XSEAARC::getVersion()
{
    // Return highest method version found in archive
    quint8 nMaxMethod = 0;
    qint64 nOffset = 0;
    qint64 nFileSize = getSize();

    while (nOffset < nFileSize) {
        quint8 nMarker = read_uint8(nOffset);
        quint8 nMethod = read_uint8(nOffset + 1);

        if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
            break;
        }

        if (!_isValidMethod(nMethod)) {
            break;
        }

        if (nMethod > nMaxMethod) {
            nMaxMethod = nMethod;
        }

        qint32 nHeaderSize = _getHeaderSize(nMethod);
        quint32 nCompressedSize = read_uint32(nOffset + 15, false);

        nOffset += nHeaderSize + nCompressedSize;
    }

    return QString::number(nMaxMethod);
}

QString XSEAARC::getArch()
{
    return QString();
}

XBinary::ENDIAN XSEAARC::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XSEAARC::getMode()
{
    return MODE_DATA;
}

bool XSEAARC::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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

        while ((nOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nFileSize - nOffset) < 2) {
                break;
            }

            quint8 nMarker = read_uint8(nOffset);
            quint8 nMethod = read_uint8(nOffset + 1);

            if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
                break;
            }

            if (!_isValidMethod(nMethod)) {
                break;
            }

            qint32 nHeaderSize = _getHeaderSize(nMethod);

            if ((nFileSize - nOffset) < nHeaderSize) {
                break;
            }

            quint32 nCompressedSize = read_uint32(nOffset + 15, false);

            pState->nNumberOfRecords++;

            nOffset += nHeaderSize + nCompressedSize;
        }

        bResult = (pState->nNumberOfRecords > 0);
    }

    return bResult;
}

XBinary::ARCHIVERECORD XSEAARC::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        quint8 nMethod = read_uint8(pState->nCurrentOffset + 1);
        qint32 nHeaderSize = _getHeaderSize(nMethod);
        quint32 nCompressedSize = read_uint32(pState->nCurrentOffset + 15, false);
        quint32 nUncompressedSize = nCompressedSize;  // Default for method 1

        if (nMethod >= CMETHOD_STORE) {
            nUncompressedSize = read_uint32(pState->nCurrentOffset + 25, false);
        }

        // Read filename (13 bytes null-terminated)
        QString sFileName = read_ansiString(pState->nCurrentOffset + 2, 13);

        quint16 nCRC16 = read_uint16(pState->nCurrentOffset + 23, false);
        quint16 nDate = read_uint16(pState->nCurrentOffset + 19, false);
        quint16 nTime = read_uint16(pState->nCurrentOffset + 21, false);

        result.nStreamOffset = pState->nCurrentOffset + nHeaderSize;
        result.nStreamSize = nCompressedSize;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUncompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, (qint64)nCompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_RESULTCRC, (quint32)nCRC16);
        result.mapProperties.insert(XBinary::FPART_PROP_TYPE, (quint32)nMethod);

        // Determine handle method
        XBinary::HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;

        if ((nMethod == CMETHOD_STORE_OLD) || (nMethod == CMETHOD_STORE)) {
            compressMethod = HANDLE_METHOD_STORE;
        }
        // Methods 3-9 are various LZW/RLE/Huffman variants - marked as unknown for now

        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, compressMethod);

        // Convert DOS date/time to QDateTime
        // DOS date: bits 15-9=year(from 1980), bits 8-5=month, bits 4-0=day
        // DOS time: bits 15-11=hour, bits 10-5=minute, bits 4-0=second/2
        qint32 nYear = ((nDate >> 9) & 0x7F) + 1980;
        qint32 nMonth = (nDate >> 5) & 0x0F;
        qint32 nDay = nDate & 0x1F;
        qint32 nHour = (nTime >> 11) & 0x1F;
        qint32 nMinute = (nTime >> 5) & 0x3F;
        qint32 nSecond = (nTime & 0x1F) * 2;

        QDateTime dtMTime(QDate(nYear, nMonth, nDay), QTime(nHour, nMinute, nSecond));

        if (dtMTime.isValid()) {
            result.mapProperties.insert(XBinary::FPART_PROP_MTIME, dtMTime);
        }
    }

    return result;
}

bool XSEAARC::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        quint8 nMethod = read_uint8(pState->nCurrentOffset + 1);
        qint32 nHeaderSize = _getHeaderSize(nMethod);
        quint32 nCompressedSize = read_uint32(pState->nCurrentOffset + 15, false);

        pState->nCurrentOffset += (nHeaderSize + nCompressedSize);
        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XSEAARC::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pState)
    Q_UNUSED(pPdStruct)

    return true;
}

QString XSEAARC::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSEAARC_STRUCTID, sizeof(_TABLE_XSEAARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XSEAARC::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XSEAARC_STRUCTID, sizeof(_TABLE_XSEAARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XSEAARC::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XSEAARC_STRUCTID, sizeof(_TABLE_XSEAARC_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XSEAARC::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
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

        while ((nOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nFileSize - nOffset) < 2) {
                break;
            }

            quint8 nMarker = read_uint8(nOffset);
            quint8 nMethod = read_uint8(nOffset + 1);

            if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
                break;
            }

            if (!_isValidMethod(nMethod)) {
                break;
            }

            qint32 nHeaderSize = _getHeaderSize(nMethod);
            quint32 nCompressedSize = read_uint32(nOffset + 15, false);

            nCount++;
            nRealSize = nOffset + nHeaderSize + nCompressedSize;

            nOffset += (nHeaderSize + nCompressedSize);
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
                qint64 nCurrentOffset = nStartOffset;
                qint32 nCount = 0;

                while ((nCount < dataHeadersOptions.nCount) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                    quint8 nMarker = read_uint8(nCurrentOffset);
                    quint8 nMethod = read_uint8(nCurrentOffset + 1);

                    if ((nMarker != 0x1A) || (nMethod == CMETHOD_END) || !_isValidMethod(nMethod)) {
                        break;
                    }

                    qint32 nHeaderSize = _getHeaderSize(nMethod);
                    quint32 nCompressedSize = read_uint32(nCurrentOffset + 15, false);

                    DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(STRUCTID_RECORD));
                    dataHeader.nSize = nHeaderSize + nCompressedSize;

                    dataHeader.listRecords.append(getDataRecord(0, 1, "Marker", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(1, 1, "Method", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(2, 13, "File Name", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(15, 4, "Compressed Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(19, 2, "Date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(21, 2, "Time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    dataHeader.listRecords.append(getDataRecord(23, 2, "CRC16", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                    if (nMethod >= CMETHOD_STORE) {
                        dataHeader.listRecords.append(getDataRecord(25, 4, "Original Size", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                    }

                    if (nCompressedSize > 0) {
                        dataHeader.listRecords.append(
                            getDataRecord(nHeaderSize, nCompressedSize, "Compressed Data", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                    }

                    listResult.append(dataHeader);

                    nCurrentOffset += (nHeaderSize + nCompressedSize);
                    nCount++;
                }
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XSEAARC::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nFileSize = getSize();
    qint64 nCurrentOffset = 0;
    qint64 nMaxOffset = 0;

    while ((nCurrentOffset < nFileSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nFileSize - nCurrentOffset) < 2) {
            break;
        }

        quint8 nMarker = read_uint8(nCurrentOffset);
        quint8 nMethod = read_uint8(nCurrentOffset + 1);

        if ((nMarker != 0x1A) || (nMethod == CMETHOD_END)) {
            break;
        }

        if (!_isValidMethod(nMethod)) {
            break;
        }

        qint32 nHeaderSize = _getHeaderSize(nMethod);
        quint32 nCompressedSize = read_uint32(nCurrentOffset + 15, false);
        quint32 nUncompressedSize = nCompressedSize;

        if (nMethod >= CMETHOD_STORE) {
            nUncompressedSize = read_uint32(nCurrentOffset + 25, false);
        }

        QString sFileName = read_ansiString(nCurrentOffset + 2, 13);

        if (nFileParts & FILEPART_HEADER) {
            FPART record = {};

            record.filePart = FILEPART_HEADER;
            record.nFileOffset = nCurrentOffset;
            record.nFileSize = nHeaderSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Header");

            listResult.append(record);
        }

        if (nFileParts & FILEPART_STREAM) {
            FPART record = {};

            record.filePart = FILEPART_STREAM;
            record.nFileOffset = nCurrentOffset + nHeaderSize;
            record.nFileSize = nCompressedSize;
            record.nVirtualAddress = -1;
            record.sName = sFileName;
            record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nUncompressedSize);

            listResult.append(record);
        }

        if (nFileParts & FILEPART_REGION) {
            FPART record = {};

            record.filePart = FILEPART_REGION;
            record.nFileOffset = nCurrentOffset;
            record.nFileSize = nHeaderSize + nCompressedSize;
            record.nVirtualAddress = -1;
            record.sName = sFileName;

            listResult.append(record);
        }

        nMaxOffset = nCurrentOffset + nHeaderSize + nCompressedSize;
        nCurrentOffset += (nHeaderSize + nCompressedSize);
    }

    // Add overlay if any
    if ((nFileParts & FILEPART_OVERLAY) && (nMaxOffset < nFileSize)) {
        FPART record = {};

        record.filePart = FILEPART_OVERLAY;
        record.nFileOffset = nMaxOffset;
        record.nFileSize = nFileSize - nMaxOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Overlay");

        listResult.append(record);
    }

    return listResult;
}

QString XSEAARC::cmethodToString(CMETHOD cmethod)
{
    QString sResult = "Unknown";

    switch (cmethod) {
        case CMETHOD_END: sResult = "End"; break;
        case CMETHOD_STORE_OLD: sResult = "Stored (old)"; break;
        case CMETHOD_STORE: sResult = "Stored"; break;
        case CMETHOD_PACKED: sResult = "Packed (RLE)"; break;
        case CMETHOD_SQUEEZED: sResult = "Squeezed (Huffman)"; break;
        case CMETHOD_CRUNCHED1: sResult = "Crunched (LZW 9-bit)"; break;
        case CMETHOD_CRUNCHED2: sResult = "Crunched (LZW 9-12 bit)"; break;
        case CMETHOD_CRUNCHED3: sResult = "Crunched with pack"; break;
        case CMETHOD_CRUNCHED4: sResult = "Crunched (LZW dynamic)"; break;
        case CMETHOD_SQUASHED: sResult = "Squashed (LZW 13-bit)"; break;
    }

    return sResult;
}

qint32 XSEAARC::_getHeaderSize(quint8 nMethod)
{
    // Method 1 (old store): no original size field = 25 bytes header
    // Methods 2-9: has original size field = 29 bytes header
    if (nMethod == CMETHOD_STORE_OLD) {
        return 25;
    }

    return 29;
}

bool XSEAARC::_isValidMethod(quint8 nMethod)
{
    return (nMethod >= CMETHOD_STORE_OLD) && (nMethod <= CMETHOD_SQUASHED);
}

QList<QString> XSEAARC::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("1A");

    return listResult;
}

XBinary *XSEAARC::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSEAARC(pDevice);
}

