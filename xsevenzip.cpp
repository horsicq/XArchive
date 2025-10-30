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
#include "xsevenzip.h"
#include "xdecompress.h"
#include <QBuffer>

XBinary::XCONVERT _TABLE_XSevenZip_STRUCTID[] = {{XSevenZip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                 {XSevenZip::STRUCTID_SIGNATUREHEADER, "SIGNATUREHEADER", QString("SIGNATUREHEADER")},
                                                 {XSevenZip::STRUCTID_HEADER, "HEADER", QObject::tr("Header")}};

XBinary::XIDSTRING _TABLE_XSevenZip_EIdEnum[] = {
    {XSevenZip::k7zIdEnd, "End"},
    {XSevenZip::k7zIdHeader, "Header"},
    {XSevenZip::k7zIdArchiveProperties, "ArchiveProperties"},
    {XSevenZip::k7zIdAdditionalStreamsInfo, "AdditionalStreamsInfo"},
    {XSevenZip::k7zIdMainStreamsInfo, "MainStreamsInfo"},
    {XSevenZip::k7zIdFilesInfo, "FilesInfo"},
    {XSevenZip::k7zIdPackInfo, "PackInfo"},
    {XSevenZip::k7zIdUnpackInfo, "UnpackInfo"},
    {XSevenZip::k7zIdSubStreamsInfo, "SubStreamsInfo"},
    {XSevenZip::k7zIdSize, "Size"},
    {XSevenZip::k7zIdCRC, "CRC"},
    {XSevenZip::k7zIdFolder, "Folder"},
    {XSevenZip::k7zIdCodersUnpackSize, "CodersUnpackSize"},
    {XSevenZip::k7zIdNumUnpackStream, "NumUnpackStream"},
    {XSevenZip::k7zIdEmptyStream, "EmptyStream"},
    {XSevenZip::k7zIdEmptyFile, "EmptyFile"},
    {XSevenZip::k7zIdAnti, "Anti"},
    {XSevenZip::k7zIdName, "Name"},
    {XSevenZip::k7zIdCTime, "CTime"},
    {XSevenZip::k7zIdATime, "ATime"},
    {XSevenZip::k7zIdMTime, "MTime"},
    {XSevenZip::k7zIdWinAttrib, "WinAttrib"},
    {XSevenZip::k7zIdComment, "Comment"},
    {XSevenZip::k7zIdEncodedHeader, "EncodedHeader"},
    {XSevenZip::k7zIdStartPos, "StartPos"},
    {XSevenZip::k7zIdDummy, "Dummy"},
};

const QString XSevenZip::PREFIX_k7zId = "k7zId";

QMap<quint64, QString> XSevenZip::get_k7zId()
{
    return XBinary::XIDSTRING_createMapPrefix(_TABLE_XSevenZip_EIdEnum, sizeof(_TABLE_XSevenZip_EIdEnum) / sizeof(XBinary::XIDSTRING), PREFIX_k7zId);
}

QMap<quint64, QString> XSevenZip::get_k7zId_s()
{
    return XBinary::XIDSTRING_createMap(_TABLE_XSevenZip_EIdEnum, sizeof(_TABLE_XSevenZip_EIdEnum) / sizeof(XBinary::XIDSTRING));
}

XSevenZip::XSevenZip(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XSevenZip::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(SIGNATUREHEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "'7z'BCAF271C", 0, pPdStruct)) {
            // More checks
            SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
            bResult = isOffsetAndSizeValid(&memoryMap, sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset, signatureHeader.NextHeaderSize);
        }
    }

    return bResult;
}

bool XSevenZip::isValid(QIODevice *pDevice)
{
    XSevenZip xsevenzip(pDevice);

    return xsevenzip.isValid();
}

QString XSevenZip::getVersion()
{
    return QString("%1.%2").arg(read_uint8(6)).arg(read_uint8(7), 1, 10, QChar('0'));
}

quint64 XSevenZip::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    return nResult;
}

QList<XArchive::RECORD> XSevenZip::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    return listResult;
}

QList<XBinary::ARCHIVERECORD> XSevenZip::getArchiveRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::ARCHIVERECORD> listResult;

    SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
    qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
    qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

    if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
        char *pData = new char[nNextHeaderSize];
        qint64 nBytesRead = read_array(nNextHeaderOffset, pData, nNextHeaderSize, pPdStruct);

        if (nBytesRead == nNextHeaderSize) {
            QList<XSevenZip::SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct, true);

            qint32 nNumberOfRecords = listRecords.count();

            if (nNumberOfRecords > 0) {
                SZRECORD firstRecord = listRecords.at(0);

                // Check if the first id is Header
                if ((firstRecord.srType == SRTYPE_ID) && (firstRecord.varValue.toULongLong() == k7zIdHeader)) {
                    // Standard header - parse file information
                    QList<QString> listFileNames;
                    QList<qint64> listFilePackedSizes;
                    QList<qint64> listFileUnpackedSizes;
                    QList<quint32> listFileAttributes;
                    QList<QDateTime> listFileTimes;

                    // Parse all records to extract file information
                    for (qint32 i = 0; (i < nNumberOfRecords) && isPdStructNotCanceled(pPdStruct); i++) {
                        SZRECORD szRecord = listRecords.at(i);

                        if (szRecord.impType == IMPTYPE_FILENAME) {
                            QString sFileName = szRecord.varValue.toString();
                            listFileNames.append(sFileName);
                        } else if (szRecord.impType == IMPTYPE_FILEPACKEDSIZE) {
                            listFilePackedSizes.append(szRecord.varValue.toLongLong());
                        } else if (szRecord.impType == IMPTYPE_FILEUNPACKEDSIZE) {
                            listFileUnpackedSizes.append(szRecord.varValue.toLongLong());
                        } else if (szRecord.impType == IMPTYPE_FILEATTRIBUTES) {
                            listFileAttributes.append(szRecord.varValue.toUInt());
                        } else if (szRecord.impType == IMPTYPE_FILETIME) {
                            // TODO: Convert file time to QDateTime
                            // listFileTimes.append(convertFileTime(szRecord.varValue));
                        }
                    }

                    qint32 nNumberOfFiles = listFileNames.count();

                    // Create archive records for each file
                    for (qint32 i = 0; (i < nNumberOfFiles) && isPdStructNotCanceled(pPdStruct); i++) {
                        XBinary::ARCHIVERECORD record = {};

                        // Set file name
                        record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, listFileNames.at(i));

                        // Set file sizes if available
                        if (i < listFileUnpackedSizes.count()) {
                            record.nDecompressedSize = listFileUnpackedSizes.at(i);
                        }

                        // For now, set basic properties
                        // TODO: Set proper stream offsets and sizes based on pack info
                        record.nStreamOffset = 0;                       // TODO: Calculate from pack info
                        record.nStreamSize = record.nDecompressedSize;  // Assume uncompressed for now

                        // Set compression method (assume STORE for now)
                        record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE);

                        listResult.append(record);
                    }
                } else if ((firstRecord.srType == SRTYPE_ID) && (firstRecord.varValue.toULongLong() == k7zIdEncodedHeader)) {
                    // Encoded header - need to decompress first
                    // This is a complex case that requires LZMA decompression
                    // For now, return empty list as this needs more implementation
                    // TODO: Implement encoded header decompression
                }
            }
        }

        delete[] pData;
    }

    return listResult;
}

qint64 XSevenZip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    qint64 nResult = 0;

    SIGNATUREHEADER signatureHeader = {};

    // TODO Check
    if (read_array(0, (char *)&signatureHeader, sizeof(SIGNATUREHEADER)) == sizeof(SIGNATUREHEADER)) {
        nResult = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset + signatureHeader.NextHeaderSize;
    }

    return nResult;
}

QString XSevenZip::getFileFormatExt()
{
    return "7z";
}

QString XSevenZip::getFileFormatExtsString()
{
    return "7-Zip (*.7z)";
}

XBinary::MODE XSevenZip::getMode()
{
    return XBinary::MODE_DATA;
}

QString XSevenZip::getMIMEString()
{
    return "application/x-7z-compressed";
}

QString XSevenZip::getArch()
{
    return QString();
}

XSevenZip::SIGNATUREHEADER XSevenZip::_read_SIGNATUREHEADER(qint64 nOffset)
{
    SIGNATUREHEADER result = {};

    read_array(nOffset, (char *)result.kSignature, 6);
    result.Major = read_uint8(nOffset + 6);
    result.Minor = read_uint8(nOffset + 7);
    result.StartHeaderCRC = read_uint32(nOffset + 8);
    result.NextHeaderOffset = read_uint64(nOffset + 12);
    result.NextHeaderSize = read_uint64(nOffset + 20);
    result.NextHeaderCRC = read_uint32(nOffset + 28);

    return result;
}

XBinary::ENDIAN XSevenZip::getEndian()
{
    return ENDIAN_LITTLE;
}

QList<XBinary::MAPMODE> XSevenZip::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_DATA);
    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XSevenZip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

XBinary::FT XSevenZip::getFileType()
{
    return FT_7Z;
}

QString XSevenZip::idToSring(XSevenZip::EIdEnum id)
{
    return XBinary::XIDSTRING_idToString((quint32)id, _TABLE_XSevenZip_EIdEnum, sizeof(_TABLE_XSevenZip_EIdEnum) / sizeof(XBinary::XIDSTRING));
}

XBinary::COMPRESS_METHOD XSevenZip::codecToCompressMethod(const QByteArray &baCodec)
{
    COMPRESS_METHOD result = COMPRESS_METHOD_UNKNOWN;

    if (baCodec.size() >= 3) {
        // 7-Zip codec IDs are typically 3+ bytes
        // Common codecs (from 7-Zip specification)
        if (baCodec.startsWith(QByteArray("\x00", 1))) {
            result = COMPRESS_METHOD_STORE;  // Copy (uncompressed)
        } else if (baCodec.startsWith(QByteArray("\x03\x01\x01", 3))) {
            result = COMPRESS_METHOD_LZMA;  // LZMA
        } else if (baCodec.startsWith(QByteArray("\x21", 1))) {
            result = COMPRESS_METHOD_LZMA2;  // LZMA2
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x08", 3))) {
            result = COMPRESS_METHOD_DEFLATE;  // Deflate
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x09", 3))) {
            result = COMPRESS_METHOD_DEFLATE64;  // Deflate64
        } else if (baCodec.startsWith(QByteArray("\x04\x02\x02", 3))) {
            result = COMPRESS_METHOD_BZIP2;  // BZip2
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01", 3))) {
            result = COMPRESS_METHOD_PPMD;  // PPMd
        }
    }

    return result;
}

QString XSevenZip::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSevenZip_STRUCTID, sizeof(_TABLE_XSevenZip_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XSevenZip::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        _dataHeadersOptions.nID = STRUCTID_SIGNATUREHEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_SIGNATUREHEADER) {
                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XSevenZip::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = sizeof(SIGNATUREHEADER);

                dataHeader.listRecords.append(
                    getDataRecord(offsetof(SIGNATUREHEADER, kSignature), 6, "kSignature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(SIGNATUREHEADER, Major), 1, "Major", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(SIGNATUREHEADER, Minor), 1, "Minor", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(SIGNATUREHEADER, StartHeaderCRC), 4, "StartHeaderCRC", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(SIGNATUREHEADER, NextHeaderOffset), 8, "NextHeaderOffset", VT_UINT64, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(SIGNATUREHEADER, NextHeaderSize), 8, "NextHeaderSize", VT_UINT64, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(SIGNATUREHEADER, NextHeaderCRC), 4, "NextHeaderCRC", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    qint64 nNextHeaderOffset = read_uint64(nStartOffset + offsetof(SIGNATUREHEADER, NextHeaderOffset));
                    qint64 nNextHeaderSize = read_uint64(nStartOffset + offsetof(SIGNATUREHEADER, NextHeaderSize));
                    // Add hex for StartHeader (the 3 fields after StartHeaderCRC)
                    {
                        const qint64 startHeaderHexOff = nStartOffset + 12;  // bytes 12..31
                        const qint64 startHeaderHexSize = 20;
                        if (isOffsetAndSizeValid(dataHeadersOptions.pMemoryMap, startHeaderHexOff, startHeaderHexSize)) {
                            DATA_HEADER hexStart = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("StartHeader (hex)"), dataHeader.dsID, XBinary::STRUCTID_HEX,
                                                                  startHeaderHexOff, startHeaderHexSize);
                            listResult.append(hexStart);
                        }
                    }

                    DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                    _dataHeadersOptions.nLocation += (sizeof(SIGNATUREHEADER) + nNextHeaderOffset);
                    _dataHeadersOptions.nSize = nNextHeaderSize;
                    _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                    _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
                    _dataHeadersOptions.nID = STRUCTID_HEADER;
                    listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));

                    // Add hex view for NextHeader block
                    qint64 nNextHeaderFileOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType,
                                                                    dataHeadersOptions.nLocation + sizeof(SIGNATUREHEADER) + nNextHeaderOffset);
                    if ((nNextHeaderFileOffset != -1) && isOffsetAndSizeValid(dataHeadersOptions.pMemoryMap, nNextHeaderFileOffset, nNextHeaderSize) &&
                        (nNextHeaderSize > 0)) {
                        DATA_HEADER hexNext = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("NextHeader (hex)"), dataHeader.dsID, XBinary::STRUCTID_HEX,
                                                             nNextHeaderFileOffset, nNextHeaderSize);
                        listResult.append(hexNext);
                    }
                }
            } else if (dataHeadersOptions.nID == STRUCTID_HEADER) {
                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XSevenZip::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = dataHeadersOptions.nSize;

                char *pData = new char[dataHeadersOptions.nSize];
                qint64 nBytesRead = read_array(nStartOffset, pData, dataHeadersOptions.nSize, pPdStruct);

                QList<XSevenZip::SZRECORD> listRecords;
                if (nBytesRead == dataHeadersOptions.nSize) {
                    listRecords = _handleData(pData, dataHeadersOptions.nSize, pPdStruct, false);
                }

                qint32 nNumberOfRecords = listRecords.count();

                for (qint32 i = 0; i < nNumberOfRecords; i++) {
                    XSevenZip::SZRECORD szRecord = listRecords.at(i);

                    DATA_RECORD dataRecord = {};
                    dataRecord.nRelOffset = szRecord.nRelOffset;
                    dataRecord.nSize = szRecord.nSize;
                    dataRecord.sName = szRecord.sName;
                    dataRecord.valType = szRecord.valType;
                    dataRecord.nFlags = szRecord.nFlags;
                    dataRecord.endian = dataHeadersOptions.pMemoryMap->endian;

                    if (szRecord.srType == SRTYPE_ID) {
                        DATAVALUESET dataValueSet;
                        dataValueSet.mapValues = get_k7zId_s();
                        dataValueSet.vlType = VL_TYPE_LIST;
                        dataValueSet.nMask = 0xFFFFFFFFFFFFFFFF;
                        dataRecord.listDataValueSets.append(dataValueSet);
                    }

                    dataHeader.listRecords.append(dataRecord);
                }

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren && (dataHeadersOptions.nSize > 0)) {
                    // Also add hex view for this parsed header block
                    DATA_HEADER hexHdr = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("Header (hex)"), dataHeader.dsID, XBinary::STRUCTID_HEX, nStartOffset,
                                                        dataHeadersOptions.nSize);
                    listResult.append(hexHdr);
                }

                delete[] pData;
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XSevenZip::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    QList<FPART> listResult;

    const qint64 nFileSize = getSize();
    if (nFileSize < (qint64)sizeof(SIGNATUREHEADER)) return listResult;

    SIGNATUREHEADER sh = _read_SIGNATUREHEADER(0);
    const qint64 nBase = sizeof(SIGNATUREHEADER);
    const qint64 nextHeaderOffset = nBase + (qint64)sh.NextHeaderOffset;
    const qint64 nextHeaderSize = (qint64)sh.NextHeaderSize;

    qint64 nMaxOffset = qMin<qint64>(nFileSize, nextHeaderOffset + nextHeaderSize);

    if (nFileParts & FILEPART_HEADER) {
        // Signature header
        FPART hdr = {};
        hdr.filePart = FILEPART_HEADER;
        hdr.nFileOffset = 0;
        hdr.nFileSize = qMin<qint64>((qint64)sizeof(SIGNATUREHEADER), nFileSize);
        hdr.nVirtualAddress = -1;
        hdr.sName = tr("Header");
        listResult.append(hdr);
    }

    if (nFileParts & FILEPART_REGION) {
        // Packed streams between signature header and next header
        qint64 nDataOff = nBase;
        qint64 nDataSize = 0;
        if (nextHeaderOffset > nBase) {
            nDataSize = nextHeaderOffset - nBase;

            FPART data = {};
            data.filePart = FILEPART_REGION;
            data.nFileOffset = nDataOff;
            data.nFileSize = nDataSize;
            data.nVirtualAddress = -1;
            data.sName = tr("Data");
            listResult.append(data);
        }
    }

    if (nFileParts & FILEPART_HEADER) {
        // Next header block
        if ((nextHeaderSize > 0) && (nextHeaderOffset >= 0) && (nextHeaderOffset + nextHeaderSize) <= nFileSize) {
            FPART nh = {};
            nh.filePart = FILEPART_HEADER;
            nh.nFileOffset = nextHeaderOffset;
            nh.nFileSize = nextHeaderSize;
            nh.nVirtualAddress = -1;
            nh.sName = QString("NEXT_HEADER");
            listResult.append(nh);
        }
    }

    if (nFileParts & FILEPART_DATA) {
        FPART nh = {};
        nh.filePart = FILEPART_DATA;
        nh.nFileOffset = 0;
        nh.nFileSize = nMaxOffset;
        nh.nVirtualAddress = -1;
        nh.sName = tr("Data");
        listResult.append(nh);
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < nFileSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = nMaxOffset;
            ov.nFileSize = nFileSize - nMaxOffset;
            ov.nVirtualAddress = -1;
            ov.sName = tr("Overlay");
            listResult.append(ov);
        }
    }

    return listResult;
}

qint64 XSevenZip::getImageSize()
{
    // Not an in-memory image; use file size
    return getSize();
}

bool XSevenZip::_handleId(QList<SZRECORD> *pListRecords, EIdEnum id, SZSTATE *pState, qint32 nCount, bool bCheck, PDSTRUCT *pPdStruct, IMPTYPE impType)
{
    // Early exit checks
    if (isPdStructStopped(pPdStruct) || (pState->nCurrentOffset >= pState->nSize) || pState->bIsError) {
        return false;
    }

    bool bResult = false;

    XBinary::PACKED_UINT puTag = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);

    if (!puTag.bIsValid) {
        if (bCheck) {
            pState->bIsError = true;
            pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
#ifdef QT_DEBUG
            qDebug("Invalid packed number at offset: 0x%llX", (qint64)pState->nCurrentOffset);
#endif
        }
        return false;
    }

    // Check if this tag matches the expected ID
    if (puTag.nValue != id) {
        if (bCheck) {
            pState->bIsError = true;
            pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
#ifdef QT_DEBUG
            qDebug("Invalid value: 0x%llX (expected: 0x%llX)", (quint64)puTag.nValue, (quint64)id);
#endif
        }
        return false;
    }

    // Add ID record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = puTag.nByteSize;
    record.varValue = puTag.nValue;
    record.srType = SRTYPE_ID;
    record.valType = VT_PACKEDNUMBER;
    record.sName = "k7zId";
    pListRecords->append(record);

    pState->nCurrentOffset += puTag.nByteSize;

    // Process ID-specific data
    switch (id) {
        case XSevenZip::k7zIdHeader:
            _handleId(pListRecords, XSevenZip::k7zIdMainStreamsInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdFilesInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdMainStreamsInfo:
            _handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdSubStreamsInfo, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdPackInfo: {
            _handleNumber(pListRecords, pState, pPdStruct, "PackPosition", DRF_OFFSET, IMPTYPE_STREAMOFFSET);
            quint64 nNumberOfPackStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfPackStreams", DRF_COUNT, IMPTYPE_NUMBEROFSTREAMS);
            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nNumberOfPackStreams, false, pPdStruct, IMPTYPE_STREAMPACKEDSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct, IMPTYPE_STREAMCRC);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdUnpackInfo:
            _handleId(pListRecords, XSevenZip::k7zIdFolder, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdFolder: {
            quint64 nNumberOfFolders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFolders", DRF_COUNT, IMPTYPE_UNKNOWN);
            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                quint64 nNumberOfCoders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCoders", DRF_COUNT, IMPTYPE_UNKNOWN);
                Q_UNUSED(nNumberOfCoders)

                quint8 nFlag = _handleByte(pListRecords, pState, pPdStruct, "Flag", IMPTYPE_UNKNOWN);

                qint32 nCodecSize = nFlag & 0x0F;
                bool bIsComplex = (nFlag & 0x10) != 0;
                bool bHasAttr = (nFlag & 0x20) != 0;

                _handleArray(pListRecords, pState, nCodecSize, pPdStruct, "Coder", IMPTYPE_CODER);

                if (bIsComplex) {
                    pState->bIsError = true;
                    pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
                }

                if (bHasAttr && !pState->bIsError) {
                    quint64 nPropertySize = _handleNumber(pListRecords, pState, pPdStruct, "PropertiesSize", DRF_SIZE, IMPTYPE_UNKNOWN);
                    _handleArray(pListRecords, pState, nPropertySize, pPdStruct, "Property", IMPTYPE_CODERPROPERTY);
                }
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("Data Stream Index"), DRF_COUNT, IMPTYPE_UNKNOWN);
            } else {
                pState->bIsError = true;
                pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
            }

            if (!pState->bIsError) {
                _handleId(pListRecords, XSevenZip::k7zIdCodersUnpackSize, pState, nNumberOfFolders, false, pPdStruct, IMPTYPE_STREAMUNPACKEDSIZE);
                _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct, IMPTYPE_STREAMCRC);
            }
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdSubStreamsInfo:
            _handleId(pListRecords, XSevenZip::k7zIdNumUnpackStream, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdNumUnpackStream: {
            quint64 nNumberOfSubStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfSubStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nNumberOfSubStreams, false, pPdStruct, IMPTYPE_STREAMUNPACKEDSIZE);
        }

            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = true;
            break;

        case XSevenZip::k7zIdEncodedHeader:
            _handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdSize:
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("Size%1").arg(i), DRF_SIZE, impType);
            }
            bResult = true;
            break;

        case XSevenZip::k7zIdCodersUnpackSize:
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("CodersUnpackSize%1").arg(i), DRF_SIZE, impType);
            }
            bResult = true;
            break;

        case XSevenZip::k7zIdCRC: {
            quint64 nNumberOfCRCs = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCRCs", DRF_COUNT, IMPTYPE_UNKNOWN);
            for (quint64 i = 0; (i < nNumberOfCRCs) && isPdStructNotCanceled(pPdStruct); i++) {
                _handleUINT32(pListRecords, pState, pPdStruct, QString("CRC%1").arg(i), impType);
            }
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdFilesInfo: {
            quint64 nNumberOfFiles = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFiles", DRF_COUNT, IMPTYPE_NUMBEROFFILES);
            Q_UNUSED(nNumberOfFiles)

            // Process optional property IDs
            _handleId(pListRecords, XSevenZip::k7zIdDummy, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdEmptyStream, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdEmptyFile, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdName, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdMTime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdCTime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdATime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdWinAttrib, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdDummy: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("DummySize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("DummyArray"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdName: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("NameSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                _handleArray(pListRecords, pState, nSize - 1, pPdStruct, QString("FileName"), IMPTYPE_FILENAME);
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("DataIndex"), DRF_COUNT, IMPTYPE_UNKNOWN);
            }

            bResult = _handleId(pListRecords, XSevenZip::k7zIdDummy, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdEmptyStream: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("EmptyStreamSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyStreamData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEmptyFile: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("EmptyFileSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyFileData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdAnti: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("AntiSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("AntiData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdCTime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CTimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("CTimeData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdATime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("ATimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("ATimeData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdMTime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("MTimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("MTimeData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdWinAttrib: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("WinAttribSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("WinAttribData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdComment: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CommentSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("CommentData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdStartPos: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("StartPosSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("StartPosData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEnd: bResult = true; break;

        default:
            // Unhandled ID type
            bResult = false;
            break;
    }

    return bResult;
}

quint64 XSevenZip::_handleNumber(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, quint32 nFlags, IMPTYPE impType)
{
    // Early exit checks
    if (isPdStructStopped(pPdStruct) || (pState->nCurrentOffset >= pState->nSize) || pState->bIsError) {
        return 0;
    }

    XBinary::PACKED_UINT puNumber = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);

    if (!puNumber.bIsValid) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3)").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"), sCaption);
#ifdef QT_DEBUG
        qDebug("Invalid packed number for '%s' at offset: 0x%llX", qPrintable(sCaption), (qint64)pState->nCurrentOffset);
#endif
        return 0;
    }

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = puNumber.nByteSize;
    record.varValue = puNumber.nValue;
    record.srType = SRTYPE_NUMBER;
    record.valType = VT_PACKEDNUMBER;
    record.nFlags = nFlags;
    record.impType = impType;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset += puNumber.nByteSize;

    return puNumber.nValue;
}

quint8 XSevenZip::_handleByte(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType)
{
    // Early exit checks
    if (isPdStructStopped(pPdStruct) || (pState->nCurrentOffset >= pState->nSize) || pState->bIsError) {
        return 0;
    }

    quint8 nResult = _read_uint8(pState->pData + pState->nCurrentOffset);

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = 1;
    record.varValue = nResult;
    record.srType = SRTYPE_BYTE;
    record.valType = VT_BYTE;
    record.impType = impType;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset++;

    return nResult;
}

quint32 XSevenZip::_handleUINT32(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType)
{
    // Early exit checks
    if (isPdStructStopped(pPdStruct) || pState->bIsError) {
        return 0;
    }

    // Check if we have enough bytes for a UINT32
    if (pState->nCurrentOffset > (pState->nSize - 4)) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3)").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"), sCaption);
#ifdef QT_DEBUG
        qDebug("Not enough bytes for UINT32 '%s' at offset: 0x%llX (need 4, have %lld)", qPrintable(sCaption), (qint64)pState->nCurrentOffset,
               pState->nSize - pState->nCurrentOffset);
#endif
        return 0;
    }

    quint32 nResult = _read_uint32(pState->pData + pState->nCurrentOffset);

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = 4;
    record.varValue = nResult;
    record.srType = SRTYPE_UINT32;
    record.valType = VT_UINT32;
    record.impType = impType;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset += 4;

    return nResult;
}

void XSevenZip::_handleArray(QList<SZRECORD> *pListRecords, SZSTATE *pState, qint64 nSize, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType)
{
    // Early exit checks
    if (isPdStructStopped(pPdStruct) || (pState->nCurrentOffset >= pState->nSize) || pState->bIsError) {
        return;
    }

    // Validate array size
    if ((nSize < 0) || (pState->nCurrentOffset > (pState->nSize - nSize))) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3, size: %4)").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"), sCaption).arg(nSize);
#ifdef QT_DEBUG
        qDebug("Invalid array size for '%s' at offset: 0x%llX (size: %lld, available: %lld)", qPrintable(sCaption), (qint64)pState->nCurrentOffset, nSize,
               pState->nSize - pState->nCurrentOffset);
#endif
        return;
    }

    // Add record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = nSize;
    record.srType = SRTYPE_ARRAY;
    record.valType = VT_BYTE_ARRAY;
    record.impType = impType;
    record.sName = sCaption;
    record.varValue = QByteArray(pState->pData + pState->nCurrentOffset, nSize);
    pListRecords->append(record);

    pState->nCurrentOffset += nSize;
}

// bool XSevenZip::packFolderToDevice(const QString &sFolderName, QIODevice *pDevice, void *pOptions, PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(sFolderName)
//     Q_UNUSED(pDevice)
//     Q_UNUSED(pOptions)
//     Q_UNUSED(pPdStruct)

//     // TODO: Implement 7z packing
//     // 7z format is very complex with multiple compression methods, solid blocks, etc.
//     // For now, return false. A proper implementation would require:
//     // 1. Write signature header
//     // 2. Write empty/placeholder next header
//     // 3. Enumerate files and write them with STORE method
//     // 4. Build archive properties structure
//     // 5. Write next header with file metadata
//     // 6. Update signature header with next header offset/size/CRC

//     return false;
// }

qint64 XSevenZip::getNumberOfArchiveRecords(PDSTRUCT *pPdStruct)
{
    // Try to quickly get number of records from parsed header data
    qint64 nResult = 0;

    SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
    qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
    qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

    if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
        char *pData = new char[nNextHeaderSize];
        qint64 nBytesRead = read_array(nNextHeaderOffset, pData, nNextHeaderSize, pPdStruct);

        if (nBytesRead == nNextHeaderSize) {
            QList<XSevenZip::SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct, true);

            qint32 nNumberOfRecords = listRecords.count();

            // Look for NumberOfFiles in the parsed records
            for (qint32 i = 0; (i < nNumberOfRecords) && isPdStructNotCanceled(pPdStruct); i++) {
                SZRECORD szRecord = listRecords.at(i);

                if (szRecord.impType == IMPTYPE_NUMBEROFFILES) {
                    nResult = szRecord.varValue.toLongLong();
                    break;
                }
            }
        }

        delete[] pData;
    }

    return nResult;
}

bool XSevenZip::initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Create context
        SEVENZ_UNPACK_CONTEXT *pContext = new SEVENZ_UNPACK_CONTEXT;  // mb create finishUnpack
        pContext->nSignatureSize = sizeof(SIGNATUREHEADER);

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;

        // Get archive records using existing getArchiveRecords method
        QList<ARCHIVERECORD> listArchiveRecords = getArchiveRecords(-1, pPdStruct);

        pContext->listArchiveRecords = listArchiveRecords;
        pState->nNumberOfRecords = listArchiveRecords.count();

        // Generate offsets for each record (for streaming access)
        for (qint32 i = 0; i < listArchiveRecords.count(); i++) {
            pContext->listRecordOffsets.append(listArchiveRecords.at(i).nStreamOffset);
        }

        bResult = (pState->nNumberOfRecords > 0);

        if (!bResult) {
            // No records found, clean up context
            delete pContext;
            pState->pContext = nullptr;
        } else {
            // Set current offset to first record
            if (pContext->listRecordOffsets.count() > 0) {
                pState->nCurrentOffset = pContext->listRecordOffsets.at(0);
            }
        }
    }

    return bResult;
}

XBinary::ARCHIVERECORD XSevenZip::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

    // Return pre-parsed archive record
    if (pState->nCurrentIndex < pContext->listArchiveRecords.count()) {
        result = pContext->listArchiveRecords.at(pState->nCurrentIndex);
    }

    return result;
}

bool XSevenZip::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }

    SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

    // Get current record info
    if (pState->nCurrentIndex < pContext->listArchiveRecords.count()) {
        ARCHIVERECORD record = pContext->listArchiveRecords.at(pState->nCurrentIndex);

        // Get compression method
        COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;
        if (record.mapProperties.contains(FPART_PROP_COMPRESSMETHOD)) {
            compressMethod = (COMPRESS_METHOD)record.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();
        }

        // Check if we can extract
        if (compressMethod == COMPRESS_METHOD_STORE) {
            // No compression - direct copy
            bResult = copyDeviceMemory(getDevice(), record.nStreamOffset, pDevice, 0, record.nStreamSize);
        } else {
            // TODO: Implement decompression for other methods
            // For now, return false for compressed files
            // A full implementation would use XDecompress or similar
            bResult = false;
        }
    }

    return bResult;
}

bool XSevenZip::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

    // Move to next record
    pState->nCurrentIndex++;

    // Check if more records available
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        // Update current offset from pre-computed list
        if (pState->nCurrentIndex < pContext->listRecordOffsets.count()) {
            pState->nCurrentOffset = pContext->listRecordOffsets.at(pState->nCurrentIndex);
        }
        bResult = true;
    }

    return bResult;
}

QList<XSevenZip::SZRECORD> XSevenZip::_handleData(char *pData, qint64 nSize, PDSTRUCT *pPdStruct, bool bUnpack)
{
    QList<XSevenZip::SZRECORD> listResult;

    // Validate input parameters
    if ((nSize <= 0) || (pData == nullptr) || isPdStructStopped(pPdStruct)) {
        return listResult;
    }

    // Initialize state
    SZSTATE state = {};
    state.pData = pData;
    state.nSize = nSize;
    state.nCurrentOffset = 0;
    state.bIsError = false;
    state.sErrorString = QString();

    // Try to parse as standard header first, then try encoded header
    if (!_handleId(&listResult, XSevenZip::k7zIdHeader, &state, 1, false, pPdStruct, IMPTYPE_UNKNOWN)) {
        // Reset state for second attempt
        state.nCurrentOffset = 0;
        state.bIsError = false;
        state.sErrorString = QString();

        QList<XSevenZip::SZRECORD> _listResult;
        _handleId(&_listResult, XSevenZip::k7zIdEncodedHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

        if (bUnpack) {
            qint32 nNumberOfRecords = _listResult.count();

            COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;
            qint64 nStreamOffset = 0;
            qint64 nStreamPackedSize = 0;
            qint64 nStreamUnpackedSize = 0;
            QByteArray baProperty;
            quint32 nStreamCRC = 0;

            // Parse the records to extract encoded header information
            for (qint32 i = 0; (i < nNumberOfRecords) && isPdStructNotCanceled(pPdStruct); i++) {
                SZRECORD szRecord = _listResult.at(i);

                if (szRecord.impType == IMPTYPE_STREAMOFFSET) {
                    nStreamOffset = szRecord.varValue.toLongLong();
                } else if (szRecord.impType == IMPTYPE_STREAMPACKEDSIZE) {
                    nStreamPackedSize = szRecord.varValue.toLongLong();
                } else if (szRecord.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                    nStreamUnpackedSize = szRecord.varValue.toLongLong();
                } else if (szRecord.impType == IMPTYPE_CODER) {
                    compressMethod = codecToCompressMethod(szRecord.varValue.toByteArray());
                } else if (szRecord.impType == IMPTYPE_CODERPROPERTY) {
                    baProperty = szRecord.varValue.toByteArray();
                } else if (szRecord.impType == IMPTYPE_STREAMCRC) {
                    nStreamCRC = szRecord.varValue.toUInt();
                }
            }

            if (compressMethod != COMPRESS_METHOD_UNKNOWN) {
                QByteArray baCompressedData = read_array(sizeof(SIGNATUREHEADER) + nStreamOffset, nStreamPackedSize, pPdStruct);
                QByteArray baDecompressedData;

                QBuffer bufferIn;
                bufferIn.setBuffer(&baCompressedData);

                QBuffer bufferOut;
                bufferOut.setBuffer(&baDecompressedData);

                if (bufferIn.open(QIODevice::ReadOnly) && bufferOut.open(QIODevice::WriteOnly)) {
                    DATAPROCESS_STATE decompressState = {};
                    decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
                    decompressState.pDeviceInput = &bufferIn;
                    decompressState.pDeviceOutput = &bufferOut;
                    decompressState.nInputOffset = 0;
                    decompressState.nInputLimit = nStreamPackedSize;
                    decompressState.nProcessedOffset = 0;
                    decompressState.nProcessedLimit = -1;
                    decompressState.nCountInput = 0;
                    decompressState.nCountOutput = 0;

                    bool bDecompressResult = false;

                    if (compressMethod == COMPRESS_METHOD_LZMA) {
                        bDecompressResult = XLZMADecoder::decompress(&decompressState, baProperty, pPdStruct);
                    } else if (compressMethod == COMPRESS_METHOD_LZMA2) {
                        bDecompressResult = XLZMADecoder::decompressLZMA2(&decompressState, baProperty, pPdStruct);
                    } else {
#ifdef QT_DEBUG
                        qDebug("Unsupported compression method for encoded header: %d", compressMethod);
#endif
                    }

                    bufferIn.close();
                    bufferOut.close();

                    // Process decompressed data if decompression was successful
                    if (bDecompressResult && baDecompressedData.size() > 0) {
                        // Verify CRC if available
                        quint32 nCalculatedCRC = XBinary::_getCRC32(baDecompressedData, 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320());
                        nCalculatedCRC ^= 0xFFFFFFFF;  // Finalize the CRC
                        if ((nStreamCRC != 0) && (nCalculatedCRC != nStreamCRC)) {
                            state.bIsError = true;
                            state.sErrorString = tr("CRC mismatch for decompressed header data");
#ifdef QT_DEBUG
                            qDebug("Decompression CRC check failed. Expected: 0x%08X, Got: 0x%08X", nStreamCRC, nCalculatedCRC);
#endif
                        } else {
                            // Parse the decompressed header data
                            XSevenZip::SZSTATE stateDecompressed = {};
                            stateDecompressed.pData = baDecompressedData.data();
                            stateDecompressed.nSize = baDecompressedData.size();
                            stateDecompressed.nCurrentOffset = 0;
                            stateDecompressed.bIsError = false;
                            stateDecompressed.sErrorString = QString();

#ifdef QT_DEBUG
                            qDebug("Decompressed %lld bytes successfully. Parsing as 7z header...", baDecompressedData.size());
#endif

                            _handleId(&listResult, XSevenZip::k7zIdHeader, &stateDecompressed, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                        }
                    } else {
                        state.bIsError = true;
                        if (!bDecompressResult) {
                            state.sErrorString = tr("Failed to decompress encoded header: LZMA decompression failed");
#ifdef QT_DEBUG
                            qDebug("XLZMADecoder::decompress() failed. Compressed size: %lld, Method: %d, Property size: %d", nStreamPackedSize, compressMethod,
                                   baProperty.size());
#endif
                        } else {
                            state.sErrorString = tr("Decompressed data is empty");
#ifdef QT_DEBUG
                            qDebug("Decompression succeeded but output is empty");
#endif
                        }
                    }
                } else {
                    state.bIsError = true;
                    state.sErrorString = tr("Failed to open buffers for decompression");
                }
            }
        } else {
            listResult = _listResult;
        }

        // Log error if parsing failed
#ifdef QT_DEBUG
        if (state.bIsError && !state.sErrorString.isEmpty()) {
            qDebug("Error parsing 7z header data: %s", qPrintable(state.sErrorString));
        }

        // Debug log all SZRECORD entries
        qint32 nTotalRecords = _listResult.count();
        qDebug("=== 7z Header Parse Results ===");
        qDebug("Total Records: %d", nTotalRecords);
        qDebug("Data Size: 0x%llX bytes", (qint64)nSize);
        qDebug("Parse Error: %s", state.bIsError ? "Yes" : "No");
        if (state.bIsError && !state.sErrorString.isEmpty()) {
            qDebug("Error Details: %s", qPrintable(state.sErrorString));
        }
        qDebug("================================");

        for (qint32 i = 0; i < nTotalRecords; i++) {
            const SZRECORD &record = _listResult.at(i);

            QString sRecordType;
            switch (record.srType) {
                case SRTYPE_UNKNOWN: sRecordType = "UNKNOWN"; break;
                case SRTYPE_ID: sRecordType = "ID"; break;
                case SRTYPE_NUMBER: sRecordType = "NUMBER"; break;
                case SRTYPE_BYTE: sRecordType = "BYTE"; break;
                case SRTYPE_UINT32: sRecordType = "UINT32"; break;
                case SRTYPE_ARRAY: sRecordType = "ARRAY"; break;
                default: sRecordType = QString("UNKNOWN(%1)").arg(record.srType); break;
            }

            QString sImpType;
            switch (record.impType) {
                case IMPTYPE_UNKNOWN: sImpType = "UNKNOWN"; break;
                case IMPTYPE_NUMBEROFFILES: sImpType = "NUMBEROFFILES"; break;
                case IMPTYPE_STREAMCRC: sImpType = "STREAMCRC"; break;
                case IMPTYPE_STREAMOFFSET: sImpType = "STREAMOFFSET"; break;
                case IMPTYPE_STREAMPACKEDSIZE: sImpType = "STREAMPACKEDSIZE"; break;
                case IMPTYPE_STREAMUNPACKEDSIZE: sImpType = "STREAMUNPACKEDSIZE"; break;
                case IMPTYPE_NUMBEROFSTREAMS: sImpType = "NUMBEROFSTREAMS"; break;
                case IMPTYPE_CODER: sImpType = "CODER"; break;
                case IMPTYPE_CODERPROPERTY: sImpType = "CODERPROPERTY"; break;
                case IMPTYPE_FILENAME: sImpType = "FILENAME"; break;
                case IMPTYPE_FILEATTRIBUTES: sImpType = "FILEATTRIBUTES"; break;
                case IMPTYPE_FILETIME: sImpType = "FILETIME"; break;
                case IMPTYPE_FILEPACKEDSIZE: sImpType = "FILEPACKEDSIZE"; break;
                case IMPTYPE_FILEUNPACKEDSIZE: sImpType = "FILEUNPACKEDSIZE"; break;
                default: sImpType = QString("UNKNOWN(%1)").arg(record.impType); break;
            }

            QString sValue;
            if (record.srType == SRTYPE_ARRAY) {
                sValue = QString("[Array, size=0x%1]").arg(record.nSize, 0, 16);
            } else if (record.valType == VT_PACKEDNUMBER || record.valType == VT_UINT32 || record.valType == VT_UINT8) {
                sValue = QString("0x%1 (%2)").arg(record.varValue.toULongLong(), 0, 16).arg(record.varValue.toULongLong());
            } else if (record.valType == VT_BYTE_ARRAY) {
                QByteArray ba = record.varValue.toByteArray();
                sValue = QString("[ByteArray, size=%1, hex:%2...]").arg(ba.size()).arg(QString(ba.left(8).toHex()).toUpper());
            } else if (record.valType == VT_CHAR_ARRAY) {
                sValue = record.varValue.toString();
            } else {
                sValue = record.varValue.toString();
            }

            qDebug("[%03d] Offset: 0x%04X, Size: %3d bytes, Type: %-10s, ImpType: %-20s, Name: %-30s, Value: %s", i, record.nRelOffset, record.nSize,
                   qPrintable(sRecordType), qPrintable(sImpType), qPrintable(record.sName), qPrintable(sValue));
        }
        qDebug("================================");
#endif
    }

    return listResult;
}
