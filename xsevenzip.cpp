/* Copyright (c) 2017-2025 hors<horsic@gmail.com>
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
        bool bSignatureValid = compareSignature(&memoryMap, "'7z'BCAF271C", 0, pPdStruct);
#ifdef QT_DEBUG
        qDebug("XSevenZip::isValid: compareSignature result: %d", bSignatureValid);
#endif

        if (bSignatureValid) {
            // More checks
            SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
#ifdef QT_DEBUG
            qDebug("XSevenZip::isValid: NextHeaderOffset = %lld, NextHeaderSize = %lld", signatureHeader.NextHeaderOffset, signatureHeader.NextHeaderSize);
#endif

            qint64 nOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
            qint64 nSize = signatureHeader.NextHeaderSize;

            bResult = isOffsetAndSizeValid(&memoryMap, nOffset, nSize);
#ifdef QT_DEBUG
            qDebug("XSevenZip::isValid: isOffsetAndSizeValid(offset=%lld, size=%lld) result: %d", nOffset, nSize, bResult);
#endif
        }
    } else {
#ifdef QT_DEBUG
        qDebug("XSevenZip::isValid: File size is too small (%lld bytes)", getSize());
#endif
    }

#ifdef QT_DEBUG
    qDebug("XSevenZip::isValid: Final result: %d", bResult);
#endif
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

XBinary::COMPRESS_METHOD XSevenZip::coderToCompressMethod(const QByteArray &baCodec)
{
    COMPRESS_METHOD result = COMPRESS_METHOD_UNKNOWN;

    if (baCodec.isEmpty()) {
        return result;
    }

    // Check 1-byte codecs first
    if (baCodec.size() == 1) {
        if (baCodec[0] == '\x00') {
            result = COMPRESS_METHOD_STORE;  // Copy (uncompressed)
        } else if (baCodec[0] == '\x21') {
            result = COMPRESS_METHOD_LZMA2;  // LZMA2
        }
    } else if (baCodec.size() >= 3) {
        // 7-Zip codec IDs are typically 3+ bytes
        // Common codecs (from 7-Zip specification)
        if (baCodec.startsWith(QByteArray("\x00", 1))) {
            result = COMPRESS_METHOD_STORE;  // Copy (uncompressed)
        } else if (baCodec.startsWith(QByteArray("\x03\x01\x01", 3))) {
            result = COMPRESS_METHOD_LZMA;  // LZMA
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x08", 3))) {
            result = COMPRESS_METHOD_DEFLATE;  // Deflate
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x09", 3))) {
            result = COMPRESS_METHOD_DEFLATE64;  // Deflate64
        } else if (baCodec.startsWith(QByteArray("\x04\x02\x02", 3))) {
            result = COMPRESS_METHOD_BZIP2;  // BZip2
        } else if (baCodec.startsWith(QByteArray("\x03\x04\x01", 3))) {
            result = COMPRESS_METHOD_PPMD;  // PPMd (actual codec from 7z)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01", 3))) {
            // Check if it's BCJ (03 03 01 03) or PPMd (03 03 01 01)
            if (baCodec.size() >= 4 && baCodec.at(3) == '\x03') {
                result = COMPRESS_METHOD_BCJ;  // BCJ (x86 filter)
            } else {
                result = COMPRESS_METHOD_PPMD;  // PPMd (alternative codec)
            }
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x02\x05", 4))) {
            result = COMPRESS_METHOD_BCJ2;  // BCJ2 (x86 advanced filter)
        } else if (baCodec.startsWith(QByteArray("\x06\xF1\x07\x01", 4))) {
            result = COMPRESS_METHOD_AES;  // AES encryption
        }
    }

    return result;
}

void XSevenZip::_applyBCJFilter(QByteArray &baData, qint32 nOffset)
{
    // BCJ (Branch/Call/Jump) x86 filter
    // Converts relative addresses to absolute for better compression
    // Must be reversed after decompression

    if (baData.isEmpty()) {
        return;
    }

    const qint32 nSize = baData.size();
    unsigned char *pData = reinterpret_cast<unsigned char *>(baData.data());
    qint32 nPos = 0;

    // Process the data looking for x86 call/jump instructions
    while (nPos + 5 <= nSize) {
        unsigned char b = pData[nPos];

        // Check for CALL (0xE8) or JMP (0xE9) instructions
        if (b == 0xE8 || b == 0xE9) {
            // Read the 32-bit relative offset (little-endian)
            qint32 nSrc = static_cast<qint32>(pData[nPos + 1] | (pData[nPos + 2] << 8) | (pData[nPos + 3] << 16) | (pData[nPos + 4] << 24));

            // Convert absolute address back to relative
            qint32 nDest = nSrc - (nOffset + nPos + 5);

            // Write back the relative offset (little-endian)
            pData[nPos + 1] = static_cast<unsigned char>(nDest);
            pData[nPos + 2] = static_cast<unsigned char>(nDest >> 8);
            pData[nPos + 3] = static_cast<unsigned char>(nDest >> 16);
            pData[nPos + 4] = static_cast<unsigned char>(nDest >> 24);

            nPos += 5;
        } else {
            nPos++;
        }
    }
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
                qint64 nBytesRead = read_array_process(nStartOffset, pData, dataHeadersOptions.nSize, pPdStruct);

                QList<XSevenZip::SZRECORD> listRecords;
                if (nBytesRead == dataHeadersOptions.nSize) {
                    listRecords = _handleData(pData, dataHeadersOptions.nSize, pPdStruct);
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
            qDebug("Invalid value: 0x%llX (expected: 0x%llX) at offset 0x%llX", (quint64)puTag.nValue, (quint64)id, pState->nCurrentOffset);
#endif
        }
        return false;
    }

#ifdef QT_DEBUG
    if (id == XSevenZip::k7zIdSubStreamsInfo) {
        qDebug() << "DEBUG: _handleId called for k7zIdSubStreamsInfo at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
    }
#endif

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
        case XSevenZip::k7zIdHeader: {
#ifdef QT_DEBUG
            qDebug() << "_handleId: Parsing k7zIdHeader at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
            qDebug() << QString("  pData ptr: %1, nSize: %2").arg((quint64)pState->pData, 0, 16).arg(pState->nSize);
#endif
//             // Parse sections sequentially - look specifically for FilesInfo
//             bool bFoundFilesInfo = false;
//             while (pState->nCurrentOffset < pState->nSize && !pState->bIsError && !bFoundFilesInfo) {
//                 // Peek at next ID
//                 qint64 nPeekOffset = pState->nCurrentOffset;
//                 quint8 nPeekByte = pState->pData[nPeekOffset];

//                 XBinary::PACKED_UINT puNextTag = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
//                 if (!puNextTag.bIsValid) break;

//                 EIdEnum nextId = (EIdEnum)puNextTag.nValue;

// #ifdef QT_DEBUG
//                 if (nextId <= 26) {  // Only log reasonable ID values
//                     qDebug() << QString("_handleId: k7zIdHeader loop - offset=0x%1 peekByte=0x%2 nextId=0x%3")
//                                     .arg(pState->nCurrentOffset, 0, 16)
//                                     .arg(nPeekByte, 2, 16, QChar('0'))
//                                     .arg((quint64)nextId, 0, 16);
//                 }
// #endif

//                 if (nextId == k7zIdFilesInfo) {
//                     // FilesInfo is what we really need for file listings
//                     _handleId(pListRecords, k7zIdFilesInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
//                     bFoundFilesInfo = true;
//                     break;  // After FilesInfo, we're done
//                 } else if (nextId == k7zIdEnd && bFoundFilesInfo) {
//                     // Only process End if we've already found FilesInfo
//                     _handleId(pListRecords, k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
//                     break;
//                 } else if (nextId == k7zIdMainStreamsInfo || nextId == k7zIdArchiveProperties || nextId == k7zIdAdditionalStreamsInfo) {
//                     // These are major sections - try to parse them
//                     bool bHandled = _handleId(pListRecords, nextId, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
//                     if (!bHandled || pState->bIsError) {
//                         pState->bIsError = false;
//                         pState->sErrorString.clear();
//                         pState->nCurrentOffset++;
//                     }
//                 } else if (nextId == k7zIdCodersUnpackSize) {
//                     // CodersUnpackSize can appear at header level in some archives
//                     // Read nNumberOfFolders size values (one per folder)
//                     pState->nCurrentOffset += puNextTag.nByteSize;  // Skip ID

// #ifdef QT_DEBUG
//                     qDebug() << "_handleId: k7zIdHeader found CodersUnpackSize, reading" << pState->nNumberOfFolders << "values";
// #endif

//                     // Read one CodersUnpackSize value per folder
//                     for (quint64 i = 0; i < pState->nNumberOfFolders && !pState->bIsError; i++) {
//                         quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CodersUnpackSize%1").arg(i), DRF_SIZE, IMPTYPE_STREAMUNPACKEDSIZE);
// #ifdef QT_DEBUG
//                         qDebug() << "  Folder" << i << "unpacked size:" << nSize << "bytes";
// #endif
//                     }
//                 } else if (nextId == k7zIdSubStreamsInfo) {
// // SubStreamsInfo can appear at header level
// #ifdef QT_DEBUG
//                     qDebug() << "_handleId: k7zIdHeader found SubStreamsInfo at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
// #endif
//                     bool bHandled = _handleId(pListRecords, k7zIdSubStreamsInfo, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
//                     if (!bHandled || pState->bIsError) {
// #ifdef QT_DEBUG
//                         qDebug() << "_handleId: k7zIdHeader SubStreamsInfo parsing failed or skipped";
// #endif
//                         pState->bIsError = false;
//                         pState->sErrorString.clear();
//                         pState->nCurrentOffset++;
//                     }
//                 } else {
//                     // Logging unknown IDs at this level
// #ifdef QT_DEBUG
//                     qDebug() << "_handleId: k7zIdHeader found unknown ID"
//                     << QString("0x%1").arg((quint64)nextId, 0, 16)
//                              << "at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
// #endif
//                     // Skip unknown/invalid bytes
//                     pState->nCurrentOffset++;
//                 }
//             }
            _handleId(pListRecords, XSevenZip::k7zIdMainStreamsInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdFilesInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            break;
        }

        case XSevenZip::k7zIdMainStreamsInfo: {
#ifdef QT_DEBUG
            qDebug() << "_handleId: Parsing k7zIdMainStreamsInfo at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif
            _handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
#ifdef QT_DEBUG
            qDebug() << "_handleId: After PackInfo, offset=" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16) << "bIsError=" << pState->bIsError;
#endif

            // UnpackInfo is optional - clear error state if it fails
            bool bErrorBeforeUnpackInfo = pState->bIsError;
            _handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
#ifdef QT_DEBUG
            qDebug() << "_handleId: After UnpackInfo, offset=" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16) << "bIsError=" << pState->bIsError;
#endif

            // If UnpackInfo failed and it was optional, restore state
            if (pState->bIsError && !bErrorBeforeUnpackInfo) {
#ifdef QT_DEBUG
                qDebug() << "_handleId: UnpackInfo failed but is optional, clearing error";
#endif
                pState->bIsError = false;
                pState->sErrorString.clear();
            }

            // SubStreamsInfo is optional - clear error state if it fails
            bool bErrorBeforeSubStreams = pState->bIsError;
            _handleId(pListRecords, XSevenZip::k7zIdSubStreamsInfo, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
#ifdef QT_DEBUG
            qDebug() << "_handleId: After SubStreamsInfo, offset=" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16) << "bIsError=" << pState->bIsError;
#endif

            // If SubStreamsInfo failed and it was optional, restore state
            if (pState->bIsError && !bErrorBeforeSubStreams) {
#ifdef QT_DEBUG
                qDebug() << "_handleId: SubStreamsInfo failed but is optional, clearing error";
#endif
                pState->bIsError = false;
                pState->sErrorString.clear();
            }

            _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            // MainStreamsInfo doesn't have its own End marker - it's terminated by the next ID
            bResult = true;
#ifdef QT_DEBUG
            qDebug() << "_handleId: MainStreamsInfo parsing complete at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif
            break;
        }

        case XSevenZip::k7zIdPackInfo: {
            pState->nStreamsBegin = _handleNumber(pListRecords, pState, pPdStruct, "PackPosition", DRF_OFFSET, IMPTYPE_STREAMOFFSET);
            quint64 nNumberOfPackStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfPackStreams", DRF_COUNT, IMPTYPE_NUMBEROFPACKSTREAMS);

            for (int i = 0; i < nNumberOfPackStreams; i++) {
                SZSTREAM szStream = {};
                pState->listStreams.append(szStream);
            }

            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nNumberOfPackStreams, false, pPdStruct, IMPTYPE_STREAMSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, nNumberOfPackStreams, false, pPdStruct, IMPTYPE_STREAMCRC);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdUnpackInfo:
            _handleId(pListRecords, XSevenZip::k7zIdFolder, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdCodersUnpackSize, pState, pState->nNumberOfOutStreams, false, pPdStruct, IMPTYPE_STREAMUNPACKEDSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, pState->nNumberOfOutStreams, false, pPdStruct, IMPTYPE_STREAMUNPACKEDCRC);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdFolder: {
            quint64 nNumberOfFolders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFolders", DRF_COUNT, IMPTYPE_NUMBEROFFOLDERS);
            pState->nNumberOfFolders = nNumberOfFolders;  // Store for SubStreamsInfo
            pState->nNumberOfOutStreams = 0;

            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                // Loop through all folders
                for (quint64 iFolderIndex = 0; iFolderIndex < nNumberOfFolders && !pState->bIsError; iFolderIndex++) {
                    qint32 nNumberOfInStreams = 0;
                    qint32 nNumberOfPackStreams = 0;
                    qint32 nIndexOfMainStream = 0;
                    qint32 nNumberOfCoderStreams = 1;
                    quint64 nNumberOfCoders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCoders", DRF_COUNT, IMPTYPE_NUMBEROFCODERS);

                    // Loop through all coders in this folder
                    for (quint64 iCoderIndex = 0; iCoderIndex < nNumberOfCoders && !pState->bIsError; iCoderIndex++) {
                        SZCODER coder = {};

                        quint8 nFlag = _handleByte(pListRecords, pState, pPdStruct, "Flag", IMPTYPE_UNKNOWN);

                        qint32 nCodecSize = nFlag & 0x0F;
                        bool bIsComplex = (nFlag & 0x10) != 0;
                        bool bHasAttr = (nFlag & 0x20) != 0;

                        coder.baCoder = _handleArray(pListRecords, pState, nCodecSize, pPdStruct, "Coder", IMPTYPE_CODER);

                        if (bIsComplex) {
                            // Complex coders have bind pairs and packed streams
                            // Read the number of input and output streams
                            nNumberOfCoderStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCoderStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
                            // quint64 nNumOutStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumOutStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
                            // Q_UNUSED(nNumInStreams)
                            // Q_UNUSED(nNumOutStreams)
                        }

                        nNumberOfInStreams += nNumberOfCoderStreams;

                        if (bHasAttr && !pState->bIsError) {
                            quint64 nPropertySize = _handleNumber(pListRecords, pState, pPdStruct, "PropertiesSize", DRF_SIZE, IMPTYPE_UNKNOWN);
                            coder.baProperty = _handleArray(pListRecords, pState, nPropertySize, pPdStruct, "Property", IMPTYPE_CODERPROPERTY);
                        }

                        if (iFolderIndex < pState->listStreams.count()) {
                            pState->listStreams[iFolderIndex].listCoders.append(coder);
                        }
                    }

                    if ((nNumberOfCoders == 1) && (nNumberOfInStreams == 1)) {
                        nIndexOfMainStream = 0;
                        nNumberOfPackStreams = 1;
                    } else {
                        qint32 nNubmerOfBonds = nNumberOfCoders - 1;

                        for (qint32 iBonds = 0; iBonds < nNubmerOfBonds; iBonds++) {
                            quint32 nStreamIndex = _handleNumber(pListRecords, pState, pPdStruct, "StreamIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);
                            quint32 nCoderIndex = _handleNumber(pListRecords, pState, pPdStruct, "CoderIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);
                        }

                        nNumberOfPackStreams = nNumberOfInStreams - nNubmerOfBonds;

                        if (nNumberOfPackStreams != 1) {
                            for (qint32 iPacks = 0; iPacks < nNumberOfPackStreams; iPacks++) {
                                quint32 nStreamIndex = _handleNumber(pListRecords, pState, pPdStruct, "StreamIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);
                            }
                        }
                    }

                    pState->nNumberOfOutStreams += nNumberOfCoders;
                }
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("Data Stream Index"), DRF_COUNT, IMPTYPE_UNKNOWN);
            } else {
                pState->bIsError = true;
                pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
            }

            if (!pState->bIsError) {
#ifdef QT_DEBUG
                qDebug() << "k7zIdFolder: After folder parsing, offset:" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
                if (pState->nCurrentOffset < pState->nSize) {
                    qDebug() << "  Next byte:" << QString("0x%1").arg((quint8)pState->pData[pState->nCurrentOffset], 2, 16, QChar('0'));
                }
#endif
            } else {
#ifdef QT_DEBUG
                qDebug() << "k7zIdFolder: ERROR - skipping optional fields. Error:" << pState->sErrorString;
#endif
            }
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdSubStreamsInfo:
#ifdef QT_DEBUG
            qDebug() << QString("_handleId: Parsing k7zIdSubStreamsInfo at offset 0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif

            // SubStreamsInfo structure (all fields are optional):
            // - k7zIdNumUnpackStream (optional): present if any folder has >1 file
            // - k7zIdSize (optional): unpacked sizes for files
            // - k7zIdCRC (optional): CRCs for files

            _handleId(pListRecords, XSevenZip::k7zIdNumUnpackStream, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            // _handleId(pListRecords, XSevenZip::k7zIdSize, pState, pState->nNumberOfFolders - 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, pState->nNumberOfFolders, false, pPdStruct, IMPTYPE_STREAMUNPACKEDCRC);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            break;

        case XSevenZip::k7zIdNumUnpackStream: {
            // NumUnpackStream: one value per folder, indicating how many files are in each solid block
            // Then Size section contains unpacked sizes for all files
            quint64 nTotalSubStreams = 0;

            qDebug() << "[NumUnpackStream] Parsing" << pState->nNumberOfFolders << "folder entries";
            for (quint64 i = 0; i < pState->nNumberOfFolders && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nNumStreamsInFolder =
                    _handleNumber(pListRecords, pState, pPdStruct, QString("NumUnpackStream%1").arg(i), DRF_COUNT, IMPTYPE_NUMBEROFUNPACKSTREAM);
                qDebug() << "[NumUnpackStream] Folder" << i << "has" << nNumStreamsInFolder << "file(s)";
                nTotalSubStreams += nNumStreamsInFolder;
            }
            qDebug() << "[NumUnpackStream] Total files in all folders:" << nTotalSubStreams;

            // SubStreamsInfo Size section contains (N-1) sizes for each folder with N>1 files
            // For folders with only 1 file, no size is listed (use folder size)
            // Total Size values = TotalFiles - NumberOfFolders
            quint64 nSizeCount = (nTotalSubStreams > pState->nNumberOfFolders) ? (nTotalSubStreams - pState->nNumberOfFolders) : 0;
            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nSizeCount, false, pPdStruct, IMPTYPE_FILEUNPACKEDSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, nTotalSubStreams, false, pPdStruct, IMPTYPE_FILECRC);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEncodedHeader:
            _handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdSize: {
            qint64 nCurrentOffset = 0;
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("Size%1").arg(i), DRF_SIZE, impType);

                if (impType == IMPTYPE_STREAMSIZE) {
                    if (i < pState->listStreams.count()) {
                        pState->listStreams[i].nStreamOffset = nCurrentOffset;
                        pState->listStreams[i].nStreamSize = nSize;
                    }
                }

                nCurrentOffset += nSize;
            }
        }

            bResult = true;
            break;

        case XSevenZip::k7zIdCodersUnpackSize: {
            qint64 nCurrentOffset = 0;
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CodersUnpackSize%1").arg(i), DRF_SIZE, impType);

                if (i < pState->listStreams.count()) {
                    pState->listStreams[i].nStreamUnpackedSize = nSize;
                }

                nCurrentOffset += nSize;
            }
        }

            bResult = true;
            break;

        case XSevenZip::k7zIdCRC: {
            // CRC format: AllAreDefined byte + CRC data
            // If AllAreDefined == 1: nCount CRC32 values
            // If AllAreDefined == 0: bitmask + CRC32 values for set bits
            quint8 nAllAreDefined = _handleByte(pListRecords, pState, pPdStruct, "AllAreDefined", IMPTYPE_UNKNOWN);

            if (nAllAreDefined == 1) {
                // All CRCs are defined - read nCount CRC32 values
                for (quint64 i = 0; (i < nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                    quint32 nCRC = _handleUINT32(pListRecords, pState, pPdStruct, QString("CRC%1").arg(i), impType);

                    if (impType == IMPTYPE_STREAMCRC) {
                        if (i < pState->listStreams.count()) {
                            pState->listStreams[i].nStreamCRC = nCRC;
                        }
                    } else if (impType == IMPTYPE_STREAMUNPACKEDCRC) {
                        if (i < pState->listStreams.count()) {
                            pState->listStreams[i].nStreamUnpackedCRC = nCRC;
                        }
                    }
                }
            } else {
// Bitmask format - read bitmask then CRCs for set bits
// For now, skip this case as it's more complex
#ifdef QT_DEBUG
                qDebug() << "k7zIdCRC: AllAreDefined==0 (bitmask format) not fully implemented";
#endif
            }
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdFilesInfo: {
            quint64 nNumberOfFiles = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFiles", DRF_COUNT, IMPTYPE_NUMBEROFFILES);
            qDebug() << "[k7zIdFilesInfo] Processing" << nNumberOfFiles << "files";

            // Store file count in state for later use
            pState->nNumberOfFiles = nNumberOfFiles;
            Q_UNUSED(nNumberOfFiles)

            // Loop through property IDs until we hit End marker
            bool bFoundEnd = false;
            while (!pState->bIsError && !bFoundEnd && isPdStructNotCanceled(pPdStruct)) {
                // Peek at next ID
                if (pState->nCurrentOffset >= pState->nSize) {
                    pState->bIsError = true;
                    pState->sErrorString = tr("Unexpected end of data");
                    break;
                }

                quint8 nNextByte = pState->pData[pState->nCurrentOffset];

#ifdef QT_DEBUG
                qDebug() << "k7zIdFilesInfo: Checking property ID at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16)
                         << "byte=" << QString("0x%1").arg(nNextByte, 0, 16);
#endif

                // Check for End marker
                if (nNextByte == XSevenZip::k7zIdEnd) {
                    _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    bFoundEnd = true;
                    break;
                }

                // Try to handle known property IDs
                bool bHandled = false;
                if (nNextByte == XSevenZip::k7zIdDummy) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdDummy, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdEmptyStream) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdEmptyStream, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdEmptyFile) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdEmptyFile, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdAnti) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdAnti, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdName) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdName, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdMTime) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdMTime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdCTime) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdCTime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdATime) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdATime, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdWinAttrib) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdWinAttrib, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else if (nNextByte == XSevenZip::k7zIdComment) {
                    bHandled = _handleId(pListRecords, XSevenZip::k7zIdComment, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                } else {
#ifdef QT_DEBUG
                    qDebug() << "k7zIdFilesInfo: Unknown property ID" << QString("0x%1").arg(nNextByte, 0, 16) << "at offset"
                             << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif
                    // Unknown property - skip the ID byte, then read and skip the size field and data
                    qint64 nPropertyStartOffset = pState->nCurrentOffset;
                    pState->nCurrentOffset++;  // Skip the ID byte

                    // Try to read the size field (most properties have this)
                    if (pState->nCurrentOffset < pState->nSize) {
                        XBinary::PACKED_UINT puSize = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
                        if (puSize.bIsValid) {
                            pState->nCurrentOffset += puSize.nByteSize;
                            quint64 nDataSize = puSize.nValue;
#ifdef QT_DEBUG
                            qDebug() << "k7zIdFilesInfo: Unknown property has size" << nDataSize << "- skipping";

                            // Print hex dump of the data for debugging
                            if (nDataSize > 0 && nDataSize <= 64 && pState->nCurrentOffset + nDataSize <= pState->nSize) {
                                QString sHexData;
                                for (quint64 i = 0; i < nDataSize && i < 64; i++) {
                                    sHexData += QString("%1 ").arg((quint8)pState->pData[pState->nCurrentOffset + i], 2, 16, QChar('0'));
                                }
                                qDebug() << "k7zIdFilesInfo: Unknown property data:" << sHexData.trimmed();

                                // If this is property 0x04 and size is 1, the byte might be the file count!
                                if (nNextByte == 0x04 && nDataSize == 1 && pState->nNumberOfFiles == 0) {
                                    quint8 nPossibleFileCount = (quint8)pState->pData[pState->nCurrentOffset];
                                    qDebug() << "k7zIdFilesInfo: Property 0x04 might indicate file count:" << nPossibleFileCount;
                                    if (nPossibleFileCount > 0 && nPossibleFileCount <= 100) {
                                        pState->nNumberOfFiles = nPossibleFileCount;
                                        qDebug() << "k7zIdFilesInfo: Updated file count to" << nPossibleFileCount;
                                    }
                                }
                            }
#endif
                            // Skip the data
                            if (pState->nCurrentOffset + nDataSize <= pState->nSize) {
                                pState->nCurrentOffset += nDataSize;
                                bHandled = true;  // Successfully skipped
                            } else {
#ifdef QT_DEBUG
                                qDebug() << "k7zIdFilesInfo: Data size exceeds available bytes - treating as end";
#endif
                                bFoundEnd = true;
                                break;
                            }
                        } else {
// If we can't read a valid size, treat this as the end
#ifdef QT_DEBUG
                            qDebug() << "k7zIdFilesInfo: Cannot read size for unknown property - treating as end";
#endif
                            bFoundEnd = true;
                            break;
                        }
                    } else {
                        bFoundEnd = true;
                        break;
                    }
                }
            }

            bResult = bFoundEnd && !pState->bIsError;
            break;
        }

        case XSevenZip::k7zIdDummy: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("DummySize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("DummyArray"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdName: {
            qDebug() << "[k7zIdName] Processing file names...";
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("NameSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);
            qDebug() << "[k7zIdName] NameSize:" << nSize << "Ext:" << nExt;

            if (nExt == 0) {
                // The data is a single block of null-terminated UTF-16LE strings.
                // The total size of this block is (nSize - 1) bytes.
                qint64 nNamesDataOffset = pState->nCurrentOffset;
                qint64 nNamesDataSize = nSize - 1;  // -1 for the ExternalByte

                if ((nNamesDataSize > 0) && ((pState->nCurrentOffset + nNamesDataSize) <= pState->nSize)) {
                    qint32 nFileIndex = 0;
                    qint64 nRelativeOffset = 0;

                    while (nRelativeOffset < nNamesDataSize) {
                        qint64 nNameStartOffset = nNamesDataOffset + nRelativeOffset;

                        // Find the null terminator (2 bytes of 0)
                        qint64 nMaxLen = nNamesDataSize - nRelativeOffset;
                        qint64 nNameLenBytes = -1;

                        for (qint64 i = 0; i < nMaxLen; i += 2) {
                            if ((nNameStartOffset + i + 1) < (pState->nCurrentOffset + pState->nSize)) {
                                if (pState->pData[nNameStartOffset + i] == 0 && pState->pData[nNameStartOffset + i + 1] == 0) {
                                    nNameLenBytes = i;
                                    break;
                                }
                            }
                        }

                        if (nNameLenBytes == -1) {  // Not found, maybe the last name without terminator
                            if (nMaxLen > 0 && (nMaxLen % 2 == 0)) {
                                nNameLenBytes = nMaxLen;
                            } else {
                                break;  // Cannot determine length, stop.
                            }
                        }

                        if (nNameLenBytes > 0) {
                            QString sFilename = QString::fromUtf16(reinterpret_cast<const ushort *>(pState->pData + nNameStartOffset), nNameLenBytes / 2);

                            SZRECORD record = {};
                            record.nRelOffset = nNameStartOffset;
                            record.nSize = nNameLenBytes;
                            record.varValue = sFilename;
                            record.srType = SRTYPE_ARRAY;
                            record.valType = VT_STRING;
                            record.impType = IMPTYPE_FILENAME;
                            record.sName = QString("FileName[%1]").arg(nFileIndex);
                            pListRecords->append(record);

                            qDebug() << "[k7zIdName] Added filename" << nFileIndex << ":" << sFilename;
                            nFileIndex++;

                            nRelativeOffset += (nNameLenBytes + 2);  // Move to the start of the next name (+2 for the null terminator)
                        } else {
                            // If we read a zero-length name, it might be the end of the list.
                            // We advance by 2 to skip the null terminator and continue.
                            nRelativeOffset += 2;
                        }
                    }
                    pState->nCurrentOffset += nNamesDataSize;
                }
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("DataIndex"), DRF_COUNT, IMPTYPE_UNKNOWN);
            }

            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEmptyStream: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("EmptyStreamSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyStreamData"), IMPTYPE_EMPTYSTREAMDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEmptyFile: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("EmptyFileSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyFileData"), IMPTYPE_EMPTYFILEDATA);
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
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("CTimeData"), IMPTYPE_CTIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdATime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("ATimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("ATimeData"), IMPTYPE_ATIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdMTime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("MTimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("MTimeData"), IMPTYPE_MTIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdWinAttrib: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("WinAttribSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("WinAttribData"), IMPTYPE_WINATTRIBDATA);
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

QByteArray XSevenZip::_handleArray(QList<SZRECORD> *pListRecords, SZSTATE *pState, qint64 nSize, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType)
{
    QByteArray baResult;
    // Early exit checks
    if (isPdStructStopped(pPdStruct) || (pState->nCurrentOffset >= pState->nSize) || pState->bIsError) {
        return baResult;
    }

    if ((pState->nSize >= 0) && (pState->nCurrentOffset <= (pState->nSize - nSize))) {
        baResult = QByteArray(pState->pData + pState->nCurrentOffset, nSize);

        // Add record
        SZRECORD record = {};
        record.nRelOffset = pState->nCurrentOffset;
        record.nSize = nSize;
        record.srType = SRTYPE_ARRAY;
        record.valType = VT_BYTE_ARRAY;
        record.impType = impType;
        record.sName = sCaption;
        record.varValue = baResult;
        pListRecords->append(record);

        pState->nCurrentOffset += nSize;
    } else {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2 (%3, size: %4)").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"), sCaption).arg(nSize);
    }

    return baResult;
}

bool XSevenZip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Create context
        SEVENZ_UNPACK_CONTEXT *pContext = new SEVENZ_UNPACK_CONTEXT;
        // pContext->nSignatureSize = sizeof(SIGNATUREHEADER);

        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;

        // Parse archive structure directly using streaming approach
        SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
        qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
        qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

        if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
            char *pData = new char[nNextHeaderSize];
            char *pUnpackedData = nullptr;
            char *pHeaderData = nullptr;
            qint64 nHeaderSize = nNextHeaderSize;

            qint64 nBytesRead = read_array_process(nNextHeaderOffset, pData, nNextHeaderSize, pPdStruct);

            if (nBytesRead == nNextHeaderSize) {
                bool bIsEncodedHeader = false;
                if (nBytesRead > 0) {
                    quint8 nFirstByte = (quint8)pData[0];
                    bIsEncodedHeader = (nFirstByte == (quint8)k7zIdEncodedHeader);
                }

                if (bIsEncodedHeader) {
                    QList<XSevenZip::SZRECORD> listRecords;

                    SZSTATE state = {};
                    state.pData = pData;
                    state.nSize = nNextHeaderSize;
                    state.nCurrentOffset = 0;
                    state.bIsError = false;
                    state.sErrorString = QString();

                    _handleId(&listRecords, XSevenZip::k7zIdEncodedHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

                    if (state.listStreams.count() == 1) {
                        COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;
                        QByteArray baProperty;
                        qint64 nStreamOffset = qint64(sizeof(SIGNATUREHEADER) + state.nStreamsBegin + state.listStreams.at(0).nStreamOffset);
                        qint64 nStreamPackedSize = state.listStreams.at(0).nStreamSize;
                        qint64 nStreamUnpackedSize = state.listStreams.at(0).nStreamUnpackedSize;
                        quint32 nStreamCRC = state.listStreams.at(0).nStreamUnpackedCRC;

                        if (state.listStreams.at(0).listCoders.count() > 0) {
                            compressMethod = coderToCompressMethod(state.listStreams.at(0).listCoders.at(0).baCoder);
                            baProperty = state.listStreams.at(0).listCoders.at(0).baProperty;
                        }

                        if (compressMethod != COMPRESS_METHOD_UNKNOWN) {
                            QByteArray baUncompressedData;
                            bool bProcessed = false;

                            QBuffer bufferOut;
                            // bufferOut.setData(pUnpackedData, nStreamUnpackedSize);
                            bufferOut.setBuffer(&baUncompressedData);

                            if (bufferOut.open(QIODevice::WriteOnly)) {
                                bProcessed = _decompress(&bufferOut, compressMethod, baProperty, nStreamOffset, nStreamPackedSize, nStreamUnpackedSize, pPdStruct);
                                bufferOut.close();
                            }

                            // Process decompressed data if decompression was successful
                            if (bProcessed) {
                                // Verify CRC if available
                                quint32 nCalculatedCRC =
                                    XBinary::_getCRC32(baUncompressedData.constData(), baUncompressedData.size(), 0xFFFFFFFF, XBinary::_getCRC32Table_EDB88320());
                                nCalculatedCRC ^= 0xFFFFFFFF;  // Finalize the CRC
                                if (nCalculatedCRC == nStreamCRC) {
                                    pUnpackedData = new char[baUncompressedData.size()];
                                    _copyMemory(pUnpackedData, baUncompressedData.constData(), baUncompressedData.size());
                                    pHeaderData = pUnpackedData;
                                    nHeaderSize = baUncompressedData.size();
                                } else {
#ifdef QT_DEBUG
                                    qDebug("Decompression CRC check failed. Expected: 0x%08X, Got: 0x%08X", nStreamCRC, nCalculatedCRC);
#endif
                                }
                            }
                        }
                    }
                } else {
                    pHeaderData = pData;
                }

                if (pHeaderData) {
                    QList<XSevenZip::SZRECORD> listRecords;

                    SZSTATE state = {};
                    state.pData = pHeaderData;
                    state.nSize = nHeaderSize;
                    state.nCurrentOffset = 0;
                    state.bIsError = false;
                    state.sErrorString = QString();

                    _handleId(&listRecords, XSevenZip::k7zIdHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

                    qint32 nNumberOfRecords = listRecords.count();

#ifdef QT_DEBUG
                    {
                        qDebug() << "=== BEGIN listRecords DEBUG OUTPUT ===";
                        qDebug() << "Total records:" << nNumberOfRecords;
                        for (qint32 i = 0; i < nNumberOfRecords; i++) {
                            const SZRECORD &rec = listRecords.at(i);
                            QString sValue;
                            if (rec.sName == "k7zId") {
                                sValue = idToSring((XSevenZip::EIdEnum)rec.varValue.toUInt());
                            } else if (rec.varValue.type() == QVariant::ByteArray) {
                                // QByteArray ba = rec.varValue.toByteArray();
                                // if (ba.size() <= 16) {
                                //     sValue = ba.toHex(' ');
                                // } else {
                                //     sValue = QString("ByteArray[%1 bytes]: %2...").arg(ba.size()).arg(ba.left(16).toHex(' ').data());
                                // }
                            } else {
                                sValue = rec.varValue.toString();
                            }
                            qDebug() << QString("[%1] Offset:%2/%3 Size:%4 srType:%5 valType:%6 impType:%7 Name:%8 Value:%9")
                                            .arg(i, 4)
                                            .arg(rec.nRelOffset, 6)
                                            .arg(nHeaderSize, 6)
                                            .arg(rec.nSize, 6)
                                            .arg(rec.srType, 2)
                                            .arg(rec.valType, 2)
                                            .arg(rec.impType, 3)
                                            .arg(rec.sName, -30)
                                            .arg(sValue);

                            if (i != (nNumberOfRecords - 1)) {
                                if ((rec.nRelOffset + rec.nSize) != listRecords.at(i + 1).nRelOffset) {
                                    qDebug() << QString("CHECK!!!");
                                }
                            }
                        }
                        qDebug() << "=== END listRecords DEBUG OUTPUT ===";
                    }
#endif
                    pContext->listArchiveRecords.clear();
                    qint32 nNumberOfFiles = 0;
                    QByteArray baEmptyStreams;
                    QByteArray baEmptyFiles;
                    // QList<SEVENZ_STREAM> listStreams;
                    // qint32 nIndex = 0;
                    qint64 nCurrentStreamPosition = 0;
                    qint64 nNumberOfPackStreams = 0;
                    qint64 nNumberOfFolders = 0;
                    QList<qint32> listNumberOfUnpackedStreams;
                    QList<qint32> listNumberOfCoders;
                    QList<qint64> listPackedStreamSizes;
                    QList<qint64> listUnpackedStreamSizes;
                    QList<qint32> listUnpackedStreamCRC;
                    QList<QByteArray> listCoders;
                    QList<QByteArray> listCoderProperties;
                    QList<quint32> listFileCRC;
                    QList<qint64> listFileSizes;

                    for (qint32 i = 0; i < nNumberOfRecords; i++) {
                        SZRECORD rec = listRecords.at(i);

                        if (rec.impType == IMPTYPE_FILENAME) {
                            ARCHIVERECORD record = {};
                            record.mapProperties.insert(FPART_PROP_ORIGINALNAME, rec.varValue.toString());
                            pContext->listArchiveRecords.append(record);
                        } else if (rec.impType == IMPTYPE_NUMBEROFFOLDERS) {
                            nNumberOfFolders = rec.varValue.toLongLong();
                        } else if (rec.impType == IMPTYPE_NUMBEROFFILES) {
                            nNumberOfFiles = rec.varValue.toLongLong();
                        } else if (rec.impType == IMPTYPE_EMPTYSTREAMDATA) {
                            baEmptyStreams = rec.varValue.toByteArray();
                        } else if (rec.impType == IMPTYPE_EMPTYFILEDATA) {
                            baEmptyFiles = rec.varValue.toByteArray();
                        } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                            nCurrentStreamPosition = rec.varValue.toLongLong();
                        } else if (rec.impType == IMPTYPE_NUMBEROFPACKSTREAMS) {
                            nNumberOfPackStreams = rec.varValue.toLongLong();
                        } else if (rec.impType == IMPTYPE_NUMBEROFCODERS) {
                            listNumberOfCoders.append(rec.varValue.toLongLong());
                        } else if (rec.impType == IMPTYPE_NUMBEROFUNPACKSTREAM) {
                            listNumberOfUnpackedStreams.append(rec.varValue.toLongLong());
                        } else if (rec.impType == IMPTYPE_STREAMSIZE) {
                            listPackedStreamSizes.append(rec.varValue.toLongLong());
                        } else if (rec.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                            listUnpackedStreamSizes.append(rec.varValue.toLongLong());
                        } else if (rec.impType == IMPTYPE_STREAMUNPACKEDCRC) {
                            listUnpackedStreamCRC.append(rec.varValue.toLongLong());
                        } else if (rec.impType == IMPTYPE_CODER) {
                            listCoders.append(rec.varValue.toByteArray());
                        } else if (rec.impType == IMPTYPE_CODERPROPERTY) {
                            listCoderProperties.append(rec.varValue.toByteArray());
                        } else if (rec.impType == IMPTYPE_FILECRC) {
                            listFileCRC.append(rec.varValue.toUInt());
                        } else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE) {
                            listFileSizes.append(rec.varValue.toLongLong());
                        }
                    }

                    //                     if (listPackedStreamSizes.count() != nNumberOfPackedStreams) {
                    // #ifdef QT_DEBUG
                    //                         qDebug("CHECK!!!");
                    // #endif
                    //                     }

                    if (pContext->listArchiveRecords.count() != nNumberOfFiles) {
#ifdef QT_DEBUG
                        qDebug("CHECK!!!");
#endif
                    }

                    qint32 nNumberOfEmptyStreams = XBinary::_getBitCount_safe(baEmptyStreams.data(), baEmptyStreams.size());
                    bool bEmptyFilesPresent = (baEmptyFiles.size() > 0);

                    if (nNumberOfEmptyStreams <= nNumberOfFiles) {
                        for (qint32 i = 0; i < nNumberOfEmptyStreams; i++) {
                            bool bIsEmpty = XBinary::_read_bool_safe_rev(baEmptyStreams.data(), baEmptyStreams.size(), i);
                            bool bIsFile = false;

                            if (bIsEmpty) {
                                if (bEmptyFilesPresent) {
                                    if (XBinary::_read_bool_safe_rev(baEmptyFiles.data(), baEmptyFiles.size(), i)) {
                                        bIsFile = true;
                                    }
                                }

                                pContext->listArchiveRecords[i].mapProperties.insert(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE);
                                pContext->listArchiveRecords[i].mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, 0);
                                pContext->listArchiveRecords[i].mapProperties.insert(FPART_PROP_ISFOLDER, !bIsFile);
                                pContext->listArchiveRecords[i].mapProperties.insert(FPART_PROP_ISSOLID, false);
                            }
                        }
                    }

                    qint32 nSubStreamFileIndex = 0;
                    qint32 nSubStreamCRCIndex = 0;
                    qint64 nStreamOffset = sizeof(SIGNATUREHEADER) + nCurrentStreamPosition;
                    qint32 nRecordIndex = nNumberOfEmptyStreams;

                    for (qint32 i = 0; i < nNumberOfPackStreams; i++) {
                        if (nRecordIndex >= pContext->listArchiveRecords.count()) {
                            break;
                        }

                        qint64 nPackedStreamSize = 0;
                        qint64 nUnpackedStreamSize = 0;
                        qint32 nNumberOfUnpackedStreams = 0;
                        QByteArray baCoder;
                        QByteArray baCoderProperty;
                        quint32 nUnpackedStreamCRC = 0;

                        if (i < listPackedStreamSizes.count()) {
                            nPackedStreamSize = listPackedStreamSizes.at(i);
                        }

                        if (i < listUnpackedStreamSizes.count()) {
                            nUnpackedStreamSize = listUnpackedStreamSizes.at(i);
                        }

                        if (i < listCoders.count()) {
                            baCoder = listCoders.at(i);
                        }

                        if (i < listCoderProperties.count()) {
                            baCoderProperty = listCoderProperties.at(i);
                        }

                        if (i < listNumberOfUnpackedStreams.count()) {
                            nNumberOfUnpackedStreams = listNumberOfUnpackedStreams.at(i);
                        }

                        if (i < listUnpackedStreamCRC.count()) {
                            nUnpackedStreamCRC = listUnpackedStreamCRC.at(i);
                        }

                        COMPRESS_METHOD cm = coderToCompressMethod(baCoder);

                        if (nNumberOfUnpackedStreams > 0) {
                            qint64 nCurrentRelOffset = 0;

                            for (qint32 j = 0; j < nNumberOfUnpackedStreams; j++) {
                                qint64 nFileSize = 0;
                                quint32 nCRC = 0;

                                if (nSubStreamCRCIndex < listFileCRC.count()) {
                                    nCRC = listFileCRC.at(nSubStreamCRCIndex);
                                    nSubStreamCRCIndex++;
                                }

                                if (j != (nNumberOfUnpackedStreams - 1)) {
                                    if (nSubStreamFileIndex < listFileSizes.count()) {
                                        nFileSize = listFileSizes.at(nSubStreamFileIndex);
                                    }

                                    nSubStreamFileIndex++;
                                } else {
                                    nFileSize = nUnpackedStreamSize - nCurrentRelOffset;
                                }

                                pContext->listArchiveRecords[nRecordIndex].nStreamOffset = nStreamOffset;
                                pContext->listArchiveRecords[nRecordIndex].nStreamSize = nPackedStreamSize;
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, nUnpackedStreamSize);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_COMPRESSMETHOD, cm);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, baCoderProperty);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, nCurrentRelOffset);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nFileSize);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_EDB88320);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_CRC_VALUE, nCRC);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_ISFOLDER, false);
                                pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_ISSOLID, true);

                                nCurrentRelOffset += nFileSize;
                                nRecordIndex++;
                            }
                        } else {
                            pContext->listArchiveRecords[nRecordIndex].nStreamOffset = nStreamOffset;
                            pContext->listArchiveRecords[nRecordIndex].nStreamSize = nPackedStreamSize;
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, nUnpackedStreamSize);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_COMPRESSMETHOD, cm);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, baCoderProperty);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, 0);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nUnpackedStreamSize);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_EDB88320);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_CRC_VALUE, nUnpackedStreamCRC);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_ISFOLDER, false);
                            pContext->listArchiveRecords[nRecordIndex].mapProperties.insert(FPART_PROP_ISSOLID, false);

                            nRecordIndex++;
                        }

                        nStreamOffset += nPackedStreamSize;
                    }
                }
            }

            pState->nNumberOfRecords = pContext->listArchiveRecords.count();
            bResult = (pState->nNumberOfRecords > 0);

            if (!bResult) {
                delete pContext;
                pState->pContext = nullptr;
            }
        }  // End if (pState)
    }      // End outer scope

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
    bool bResult = false;

    if (pState && pState->pContext && pDevice) {
        SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;
        ARCHIVERECORD archiveRecord = pContext->listArchiveRecords.at(pState->nCurrentIndex);

        if (archiveRecord.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) return true;  // Directory

        if (archiveRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() == 0) return true;  // Empty file

        bool bIsSolid = archiveRecord.mapProperties.value(FPART_PROP_ISSOLID).toBool();
        COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)(archiveRecord.mapProperties.value(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_UNKNOWN).toInt());
        QByteArray baProperty = archiveRecord.mapProperties.value(FPART_PROP_COMPRESSPROPERTIES, COMPRESS_METHOD_UNKNOWN).toByteArray();
        qint64 nStreamOffset = archiveRecord.nStreamOffset;
        qint64 nStreamSize = archiveRecord.nStreamSize;

        if (bIsSolid) {
            QString sRecordName = QString("%1_%2").arg(QString::number(nStreamOffset), QString::number(nStreamSize));

            QIODevice *pSolidDevice = nullptr;

            if (pContext->mapDevices.contains(sRecordName)) {
                pSolidDevice = pContext->mapDevices.value(sRecordName);
            } else {
                qint64 nStreamUnpackedSize = archiveRecord.mapProperties.value(FPART_PROP_STREAMUNPACKEDSIZE).toLongLong();
                pSolidDevice = XBinary::createFileBuffer(nStreamUnpackedSize, pPdStruct);

                bool bProcessed = _decompress(pSolidDevice, compressMethod, baProperty, nStreamOffset, nStreamSize, nStreamUnpackedSize, pPdStruct);

                // Process decompressed data if decompression was successful
                if (bProcessed) {
                    pContext->mapDevices.insert(sRecordName, pSolidDevice);
                } else {
                    XBinary::freeFileBuffer(&pSolidDevice);
                    pSolidDevice = nullptr;
                }
            }

            if (pSolidDevice) {
                qint64 nSubstreamOffset = archiveRecord.mapProperties.value(FPART_PROP_SUBSTREAMOFFSET).toLongLong();
                qint64 nUncompressedSize = archiveRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

                XBinary::DATAPROCESS_STATE decompressState = {};
                decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE);
                decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
                decompressState.pDeviceInput = pSolidDevice;
                decompressState.pDeviceOutput = pDevice;
                decompressState.nInputOffset = nSubstreamOffset;
                decompressState.nInputLimit = nUncompressedSize;
                decompressState.nProcessedOffset = 0;
                decompressState.nProcessedLimit = -1;

                bResult = XStoreDecoder::decompress(&decompressState, pPdStruct);
            }
        } else {
            qint64 nUncompressedSize = archiveRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
            bResult = _decompress(pDevice, compressMethod, baProperty, nStreamOffset, nStreamSize, nUncompressedSize, pPdStruct);
        }
    }

    return bResult;
}

bool XSevenZip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = true;

    if (pState && pState->pContext) {
        SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

        QMapIterator<QString, QIODevice *> iterator(pContext->mapDevices);
        while (iterator.hasNext() && XBinary::isPdStructNotCanceled(pPdStruct)) {
            iterator.next();

            QIODevice *pDevice = iterator.value();

            XBinary::freeFileBuffer(&pDevice);
        }

        // Delete context
        delete pContext;
        pState->pContext = nullptr;
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

    // SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

    // Move to next record
    pState->nCurrentIndex++;

    // Check if more records available
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        bResult = true;
    }

    return bResult;
}

QList<XSevenZip::SZRECORD> XSevenZip::_handleData(char *pData, qint64 nSize, PDSTRUCT *pPdStruct)
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

    // Check if the first byte indicates an encoded header
    bool bIsEncodedHeader = (nSize > 0 && pData[0] == XSevenZip::k7zIdEncodedHeader);

    if (bIsEncodedHeader) {
        _handleId(&listResult, XSevenZip::k7zIdEncodedHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
    } else {
        _handleId(&listResult, XSevenZip::k7zIdHeader, &state, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
    }

    return listResult;
}

bool XSevenZip::_decompress(QIODevice *pDevice, COMPRESS_METHOD compressMethod, const QByteArray &baProperty, qint64 nOffset, qint64 nSize, qint64 nUncompressedSize,
                            PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    SubDevice sdCompressed(getDevice(), nOffset, nSize);

    if (sdCompressed.open(QIODevice::ReadOnly)) {
        DATAPROCESS_STATE decompressState = {};
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
        decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSPROPERTIES, baProperty);
        if (nUncompressedSize > 0) {
            decompressState.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);
        }
        decompressState.pDeviceInput = &sdCompressed;
        decompressState.pDeviceOutput = pDevice;
        decompressState.nInputOffset = 0;
        decompressState.nInputLimit = nSize;
        decompressState.nProcessedOffset = 0;
        decompressState.nProcessedLimit = -1;

        if (compressMethod == COMPRESS_METHOD_LZMA) {
            bResult = XLZMADecoder::decompress(&decompressState, baProperty, pPdStruct);
            // bDecompressResult = XLZMADecoder::decompress(&decompressState, pPdStruct);
        } else if (compressMethod == COMPRESS_METHOD_LZMA2) {
            bResult = XLZMADecoder::decompressLZMA2(&decompressState, baProperty, pPdStruct);
        } else if (compressMethod == COMPRESS_METHOD_PPMD) {
            bResult = XPPMdDecoder::decompressPPMdH(&decompressState, baProperty, pPdStruct);
        } else if (compressMethod == COMPRESS_METHOD_BZIP2) {
            bResult = XBZIP2Decoder::decompress(&decompressState, pPdStruct);
        } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
            bResult = XDeflateDecoder::decompress(&decompressState, pPdStruct);
        } else if (compressMethod == COMPRESS_METHOD_DEFLATE64) {
            bResult = XDeflateDecoder::decompress64(&decompressState, pPdStruct);
        } else if (compressMethod == COMPRESS_METHOD_STORE) {
            bResult = XStoreDecoder::decompress(&decompressState, pPdStruct);
        } else {
#ifdef QT_DEBUG
            qDebug("Unsupported compression method: %d", compressMethod);
#endif
        }

        sdCompressed.close();
    }

    return bResult;
}
