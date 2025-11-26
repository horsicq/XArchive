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

XBinary::COMPRESS_METHOD XSevenZip::codecToCompressMethod(const QByteArray &baCodec)
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
                qint64 nBytesRead = read_array(nStartOffset, pData, dataHeadersOptions.nSize, pPdStruct);

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
            // Parse sections sequentially - look specifically for FilesInfo
            bool bFoundFilesInfo = false;
            while (pState->nCurrentOffset < pState->nSize && !pState->bIsError && !bFoundFilesInfo) {
                // Peek at next ID
                qint64 nPeekOffset = pState->nCurrentOffset;
                quint8 nPeekByte = pState->pData[nPeekOffset];

                XBinary::PACKED_UINT puNextTag = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
                if (!puNextTag.bIsValid) break;

                EIdEnum nextId = (EIdEnum)puNextTag.nValue;

#ifdef QT_DEBUG
                if (nextId <= 26) {  // Only log reasonable ID values
                    qDebug() << QString("_handleId: k7zIdHeader loop - offset=0x%1 peekByte=0x%2 nextId=0x%3")
                                    .arg(pState->nCurrentOffset, 0, 16)
                                    .arg(nPeekByte, 2, 16, QChar('0'))
                                    .arg((quint64)nextId, 0, 16);
                }
#endif

                if (nextId == k7zIdFilesInfo) {
                    // FilesInfo is what we really need for file listings
                    _handleId(pListRecords, k7zIdFilesInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    bFoundFilesInfo = true;
                    break;  // After FilesInfo, we're done
                } else if (nextId == k7zIdEnd && bFoundFilesInfo) {
                    // Only process End if we've already found FilesInfo
                    _handleId(pListRecords, k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    break;
                } else if (nextId == k7zIdMainStreamsInfo || nextId == k7zIdArchiveProperties || nextId == k7zIdAdditionalStreamsInfo) {
                    // These are major sections - try to parse them
                    bool bHandled = _handleId(pListRecords, nextId, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    if (!bHandled || pState->bIsError) {
                        pState->bIsError = false;
                        pState->sErrorString.clear();
                        pState->nCurrentOffset++;
                    }
                } else if (nextId == k7zIdCodersUnpackSize) {
                    // CodersUnpackSize can appear at header level in some archives
                    // Read nNumberOfFolders size values (one per folder)
                    pState->nCurrentOffset += puNextTag.nByteSize;  // Skip ID

#ifdef QT_DEBUG
                    qDebug() << "_handleId: k7zIdHeader found CodersUnpackSize, reading" << pState->nNumberOfFolders << "values";
#endif

                    // Read one CodersUnpackSize value per folder
                    for (quint64 i = 0; i < pState->nNumberOfFolders && !pState->bIsError; i++) {
                        quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CodersUnpackSize%1").arg(i), DRF_SIZE, IMPTYPE_STREAMUNPACKEDSIZE);
#ifdef QT_DEBUG
                        qDebug() << "  Folder" << i << "unpacked size:" << nSize << "bytes";
#endif
                    }

#ifdef QT_DEBUG
                    qDebug() << "  After reading CodersUnpackSize, offset:" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
                    if (pState->nCurrentOffset < pState->nSize) {
                        qDebug() << "  Next bytes:"
                                 << QByteArray((char *)(pState->pData + pState->nCurrentOffset), qMin(10, (int)(pState->nSize - pState->nCurrentOffset))).toHex(' ');
                    }
#endif
                } else if (nextId == k7zIdSubStreamsInfo) {
// SubStreamsInfo can appear at header level
#ifdef QT_DEBUG
                    qDebug() << "_handleId: k7zIdHeader found SubStreamsInfo at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif
                    bool bHandled = _handleId(pListRecords, k7zIdSubStreamsInfo, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                    if (!bHandled || pState->bIsError) {
#ifdef QT_DEBUG
                        qDebug() << "_handleId: k7zIdHeader SubStreamsInfo parsing failed or skipped";
#endif
                        pState->bIsError = false;
                        pState->sErrorString.clear();
                        pState->nCurrentOffset++;
                    }
                } else {
                    // Skip unknown/invalid bytes
                    pState->nCurrentOffset++;
                }
            }
            bResult = true;
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

            // MainStreamsInfo doesn't have its own End marker - it's terminated by the next ID
            bResult = true;
#ifdef QT_DEBUG
            qDebug() << "_handleId: MainStreamsInfo parsing complete at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif
            break;
        }

        case XSevenZip::k7zIdPackInfo: {
            _handleNumber(pListRecords, pState, pPdStruct, "PackPosition", DRF_OFFSET, IMPTYPE_STREAMOFFSET);
            quint64 nNumberOfPackStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfPackStreams", DRF_COUNT, IMPTYPE_NUMBEROFPACKSTREAMS);
            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nNumberOfPackStreams, false, pPdStruct, IMPTYPE_STREAMPACKEDSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct, IMPTYPE_STREAMCRC);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdUnpackInfo:
#ifdef QT_DEBUG
            qDebug() << "k7zIdUnpackInfo: Parsing at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif
            bResult = _handleId(pListRecords, XSevenZip::k7zIdFolder, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
#ifdef QT_DEBUG
            if (pState->bIsError) {
                qDebug() << "k7zIdUnpackInfo: Folder parsing failed:" << pState->sErrorString;
            } else {
                qDebug() << "k7zIdUnpackInfo: Folder parsing succeeded, offset now" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
            }
#endif
            // Note: k7zIdFolder already consumes the k7zIdEnd, so we don't need to read it again
            break;

        case XSevenZip::k7zIdFolder: {
            quint64 nNumberOfFolders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFolders", DRF_COUNT, IMPTYPE_UNKNOWN);
            pState->nNumberOfFolders = nNumberOfFolders;  // Store for SubStreamsInfo

#ifdef QT_DEBUG
            qDebug() << "k7zIdFolder: nNumberOfFolders=" << nNumberOfFolders;
#endif

            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                // Loop through all folders
                for (quint64 iFolderIndex = 0; iFolderIndex < nNumberOfFolders && !pState->bIsError; iFolderIndex++) {
                    quint64 nNumberOfCoders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCoders", DRF_COUNT, IMPTYPE_UNKNOWN);

#ifdef QT_DEBUG
                    qDebug() << "  Folder" << iFolderIndex << "has" << nNumberOfCoders << "coders";
#endif

                    // Loop through all coders in this folder
                    for (quint64 iCoderIndex = 0; iCoderIndex < nNumberOfCoders && !pState->bIsError; iCoderIndex++) {
                        quint8 nFlag = _handleByte(pListRecords, pState, pPdStruct, "Flag", IMPTYPE_UNKNOWN);

                        qint32 nCodecSize = nFlag & 0x0F;
                        bool bIsComplex = (nFlag & 0x10) != 0;
                        bool bHasAttr = (nFlag & 0x20) != 0;

                        _handleArray(pListRecords, pState, nCodecSize, pPdStruct, "Coder", IMPTYPE_CODER);

                        if (bIsComplex) {
                            // Complex coders have bind pairs and packed streams
                            // Read the number of input and output streams
                            quint64 nNumInStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumInStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
                            quint64 nNumOutStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumOutStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
                            Q_UNUSED(nNumInStreams)
                            Q_UNUSED(nNumOutStreams)
                        }

                        if (bHasAttr && !pState->bIsError) {
                            quint64 nPropertySize = _handleNumber(pListRecords, pState, pPdStruct, "PropertiesSize", DRF_SIZE, IMPTYPE_UNKNOWN);
                            _handleArray(pListRecords, pState, nPropertySize, pPdStruct, "Property", IMPTYPE_CODERPROPERTY);
                        }
                    }

                    // After all coders, handle bind pairs and packed streams for complex coders
                    // TODO: Implement bind pairs and packed streams parsing if needed
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

                // CodersUnpackSize is optional - check if it's present
                if (pState->nCurrentOffset < pState->nSize && pState->pData[pState->nCurrentOffset] == XSevenZip::k7zIdCodersUnpackSize) {
#ifdef QT_DEBUG
                    qDebug() << "k7zIdFolder: Found k7zIdCodersUnpackSize, parsing" << nNumberOfFolders << "values";
#endif
                    _handleId(pListRecords, XSevenZip::k7zIdCodersUnpackSize, pState, nNumberOfFolders, false, pPdStruct, IMPTYPE_STREAMUNPACKEDSIZE);
                } else {
#ifdef QT_DEBUG
                    qDebug() << "k7zIdFolder: k7zIdCodersUnpackSize not present (optional field)";
#endif
                }

                // CRC is also optional
                if (pState->nCurrentOffset < pState->nSize && pState->pData[pState->nCurrentOffset] == XSevenZip::k7zIdCRC) {
                    _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct, IMPTYPE_STREAMCRC);
                }
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
                quint64 nNumStreamsInFolder = _handleNumber(pListRecords, pState, pPdStruct, QString("NumUnpackStream%1").arg(i), DRF_COUNT, IMPTYPE_NUMBEROFUNPACKSTREAM);
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

        case XSevenZip::k7zIdSize:
#ifdef QT_DEBUG
            qDebug() << "k7zIdSize: Reading" << nCount << "size values with impType=" << impType << "(STREAMUNPACKEDSIZE=" << IMPTYPE_STREAMUNPACKEDSIZE
                     << ", STREAMPACKEDSIZE=" << IMPTYPE_STREAMPACKEDSIZE << ")";
#endif
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("Size%1").arg(i), DRF_SIZE, impType);
#ifdef QT_DEBUG
                qDebug() << "  Size" << i << "=" << nSize;
#endif
            }
            bResult = true;
            break;

        case XSevenZip::k7zIdCodersUnpackSize:
#ifdef QT_DEBUG
            qDebug() << "k7zIdCodersUnpackSize: Reading" << nCount << "values at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
#endif
            for (quint64 i = 0; (i < (quint64)nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CodersUnpackSize%1").arg(i), DRF_SIZE, impType);
#ifdef QT_DEBUG
                qDebug() << "  CodersUnpackSize[" << i << "] =" << nSize << "bytes";
#endif
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
                    _handleUINT32(pListRecords, pState, pPdStruct, QString("CRC%1").arg(i), impType);
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

            qint64 nBytesRead = read_array(nNextHeaderOffset, pData, nNextHeaderSize, pPdStruct);

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

                    {
                        qint32 nNumberOfRecords = listRecords.count();

                        COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;
                        qint64 nStreamOffset = -1;
                        qint64 nStreamPackedSize = -1;
                        qint64 nStreamUnpackedSize = -1;
                        QByteArray baProperty;
                        quint32 nStreamCRC = 0;

                        // Parse the records to extract encoded header information
                        for (qint32 i = 0; (i < nNumberOfRecords) && isPdStructNotCanceled(pPdStruct); i++) {
                            SZRECORD szRecord = listRecords.at(i);

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
                            QByteArray baUncompressedData;
                            bool bProcessed = false;

                            QBuffer bufferOut;
                            // bufferOut.setData(pUnpackedData, nStreamUnpackedSize);
                            bufferOut.setBuffer(&baUncompressedData);

                            if (bufferOut.open(QIODevice::WriteOnly)) {
                                bProcessed = _decompress(&bufferOut, compressMethod, baProperty, qint64(sizeof(SIGNATUREHEADER) + nStreamOffset), nStreamPackedSize, nStreamUnpackedSize, pPdStruct);
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
                                QByteArray ba = rec.varValue.toByteArray();
                                if (ba.size() <= 16) {
                                    sValue = ba.toHex(' ');
                                } else {
                                    sValue = QString("ByteArray[%1 bytes]: %2...").arg(ba.size()).arg(ba.left(16).toHex(' ').data());
                                }
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
                    QList<qint32> listNumberOfUnpackedStreams;
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
                        } else if (rec.impType == IMPTYPE_NUMBEROFFILES) {
                            nNumberOfFiles = rec.varValue.toLongLong();
                        } else if (rec.impType == IMPTYPE_EMPTYSTREAMDATA) {
                            baEmptyStreams = rec.varValue.toByteArray();
                        } else if (rec.impType == IMPTYPE_EMPTYFILEDATA) {
                            baEmptyFiles = rec.varValue.toByteArray();
                        } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                            nCurrentStreamPosition += rec.varValue.toLongLong();
                        } else if (rec.impType == IMPTYPE_NUMBEROFPACKSTREAMS) {
                            nNumberOfPackStreams = rec.varValue.toLongLong();
                        } else if (rec.impType == IMPTYPE_NUMBEROFUNPACKSTREAM) {
                            listNumberOfUnpackedStreams.append(rec.varValue.toLongLong());
                        } else if (rec.impType == IMPTYPE_STREAMPACKEDSIZE) {
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

                        COMPRESS_METHOD cm = codecToCompressMethod(baCoder);

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
    }  // End outer scope

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

        if (archiveRecord.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) return true; // Directory

        if (archiveRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() == 0) return true; // Empty file

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

//     if (pState && pState->pContext && pDevice) {
//         SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;
//         ARCHIVERECORD ar = pContext->listArchiveRecords.at(pState->nCurrentIndex);

// #ifdef QT_DEBUG
//         qDebug() << "_unpack: Unpacking" << ar.mapProperties.value(FPART_PROP_ORIGINALNAME).toString() << "StreamOffset:" << ar.nStreamOffset
//                  << "StreamSize:" << ar.nStreamSize << "DecompressedSize:" << ar.nDecompressedSize;
//         if (ar.mapProperties.contains(FPART_PROP_COMPRESSMETHOD)) {
//             qDebug() << "  CompressMethod:" << ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();
//         }
// #endif

//         // Check if this is a directory entry (size 0, no file extension, other files have this as path prefix)
//         QString sFilename = ar.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
//         bool bIsDirectory = false;
//         if (ar.nDecompressedSize == 0 && ar.nStreamSize == 0) {
//             // Check if this looks like a directory (no file extension, or explicit directory marker)
//             // In 7z archives, directories are stored as entries without trailing slashes
//             // but other files will have paths starting with this directory name + "/"
//             QFileInfo fi(sFilename);
//             if (fi.suffix().isEmpty()) {
//                 // No extension - likely a directory, but verify by checking if other files use this path
//                 QString sDirPrefix = sFilename + "/";
//                 for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
//                     QString sOtherFile = pContext->listArchiveRecords.at(i).mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
//                     if (sOtherFile.startsWith(sDirPrefix)) {
//                         bIsDirectory = true;
//                         break;
//                     }
//                 }
//             }
//         }

//         if (bIsDirectory) {
//             qDebug() << "[Directory] Skipping directory entry:" << sFilename;
//             // Directory entry - don't try to create as file, just succeed
//             bResult = true;
//         } else if (ar.nDecompressedSize == 0 && ar.nStreamSize == 0) {
//             qDebug() << "[Empty File] Extracting empty file:" << sFilename;
//             // Empty file - nothing to write, just return success
//             bResult = true;
//         } else {
//             // Check if this is a solid archive file
//             bool bIsSolid = ar.mapProperties.value(FPART_PROP_SOLID, false).toBool();
//             bool bIsEncrypted = ar.mapProperties.value(FPART_PROP_ENCRYPTED, false).toBool();
            
//             // Determine folder index for this file by finding other files with same stream offset
//             // Files in the same folder share the same nStreamOffset (solid folder) or have sequential offsets (non-solid)
//             qint32 nFolderIndex = 0;
//             if (bIsSolid && ar.nStreamOffset > 0) {
//                 // For solid files, folder index is determined by stream offset
//                 // Count how many unique stream offsets exist before this one
//                 QSet<qint64> uniqueOffsets;
//                 for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
//                     ARCHIVERECORD &rec = pContext->listArchiveRecords[i];
//                     bool bRecIsSolid = rec.mapProperties.value(FPART_PROP_SOLID, false).toBool();
//                     if (bRecIsSolid && rec.nStreamOffset > 0 && rec.nStreamOffset < ar.nStreamOffset) {
//                         uniqueOffsets.insert(rec.nStreamOffset);
//                     }
//                 }
//                 nFolderIndex = uniqueOffsets.count();
//             }
            
// #ifdef QT_DEBUG
//             if (bIsSolid) {
//                 qDebug() << "[Folder Index] File:" << sFilename << "Folder:" << nFolderIndex
//                          << "StreamOffset:" << ar.nStreamOffset;
//             }
// #endif
            
//             if (bIsSolid && !pContext->mapFolderCache.contains(nFolderIndex) && bIsEncrypted) {
//                 // Solid encrypted folder - need to decrypt+decompress the entire block on first access
//                 qDebug() << "[Solid Encrypted] Folder" << nFolderIndex << "first access - decrypting and decompressing entire solid block";
                
//                 // Get the solid block information
//                 // All solid files in this folder share the same nStreamOffset and nStreamSize
//                 qint64 nSolidStreamOffset = ar.nStreamOffset;
//                 qint64 nSolidStreamSize = ar.nStreamSize;
                
//                 // Calculate total uncompressed size for this folder by finding max(offset + size)
//                 qint64 nTotalUncompressedSize = 0;
//                 for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
//                     ARCHIVERECORD &rec = pContext->listArchiveRecords[i];
//                     bool bRecIsSolid = rec.mapProperties.value(FPART_PROP_SOLID, false).toBool();
//                     if (bRecIsSolid && rec.nStreamOffset == nSolidStreamOffset && rec.nDecompressedSize > 0) {
//                         qint64 nEndOffset = rec.nDecompressedOffset + rec.nDecompressedSize;
//                         if (nEndOffset > nTotalUncompressedSize) {
//                             nTotalUncompressedSize = nEndOffset;
//                         }
//                     }
//                 }
                
//                 qDebug() << "[Solid Encrypted]   Folder index:" << nFolderIndex;
//                 qDebug() << "[Solid Encrypted]   Stream offset:" << nSolidStreamOffset;
//                 qDebug() << "[Solid Encrypted]   Stream size:" << nSolidStreamSize;
//                 qDebug() << "[Solid Encrypted]   Total uncompressed size:" << nTotalUncompressedSize;
                
//                 // Get AES and compression properties
//                 QByteArray baAesProperties;
//                 if (ar.mapProperties.contains(FPART_PROP_AESKEY)) {
//                     baAesProperties = ar.mapProperties.value(FPART_PROP_AESKEY).toByteArray();
                    
//                     // Debug: Show AES properties hex dump
//                     QString sAesPropsHex;
//                     for (qint32 i = 0; i < qMin(32, baAesProperties.size()); i++) {
//                         sAesPropsHex += QString("%1 ").arg((quint8)baAesProperties.at(i), 2, 16, QChar('0'));
//                     }
//                     qDebug() << "[Solid Encrypted] AES Properties (" << baAesProperties.size() << "bytes):" << sAesPropsHex;
//                 }
                
//                 QByteArray baCompressProperties;
//                 if (ar.mapProperties.contains(FPART_PROP_COMPRESSPROPERTIES)) {
//                     baCompressProperties = ar.mapProperties.value(FPART_PROP_COMPRESSPROPERTIES).toByteArray();
//                 }
                
//                 COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();
                
//                 if (baAesProperties.isEmpty()) {
//                     qWarning() << "[Solid Encrypted] AES properties not found";
//                     bResult = false;
//                 } else if (pContext->sPassword.isEmpty()) {
//                     qWarning() << "[Solid Encrypted] Password required";
//                     bResult = false;
//                 } else {
//                     // Step 1: Decrypt the solid block
//                     SubDevice sdEncrypted(getDevice(), nSolidStreamOffset, nSolidStreamSize);
//                     if (!sdEncrypted.open(QIODevice::ReadOnly)) {
//                         qWarning() << "[Solid Encrypted] Failed to open encrypted stream";
//                         bResult = false;
//                     } else {
//                         // Check if this is NEW format (IV in properties) or OLD format (IV in stream)
//                         // According to 7-Zip SDK (7zAes.cpp):
//                         //   OLD format: (b0 & 0xC0) == 0 (neither bit 6 nor bit 7 set) - IV read from stream
//                         //   NEW format: (b0 & 0xC0) != 0 (at least one of bits 6-7 set) - IV in properties
//                         bool bNewFormat = false;
//                         QByteArray baFullAesProperties = baAesProperties;
//                         if (baAesProperties.size() >= 1) {
//                             quint8 nFirstByte = (quint8)baAesProperties[0];
//                             bNewFormat = ((nFirstByte & 0xC0) != 0);  // NEW format if ANY of bits 6-7 set
//                         }
                        
//                         if (bNewFormat) {
//                             // NEW FORMAT: IV is already in properties, don't read from stream
//                             qDebug() << "[Solid Encrypted] NEW format detected - IV in properties";
//                         } else {
//                             // OLD FORMAT: Read IV from stream (first 16 bytes) and append to properties
//                             QByteArray baIV = sdEncrypted.read(16);
//                             if (baIV.size() != 16) {
//                                 qWarning() << "[Solid Encrypted] Failed to read IV (got" << baIV.size() << "bytes)";
//                                 sdEncrypted.close();
//                                 bResult = false;
//                             } else {
//                                 qDebug() << "[Solid Encrypted] OLD format detected - IV from stream";
//                                 baFullAesProperties = baAesProperties + baIV;
//                             }
//                         }
                        
//                         if (bResult) {
                            
//                             // Create buffer for decrypted data
//                             QBuffer tempDecryptBuffer;
//                             tempDecryptBuffer.open(QIODevice::WriteOnly);
                            
//                             XBinary::DATAPROCESS_STATE decryptState = {};
//                             decryptState.pDeviceInput = &sdEncrypted;
//                             decryptState.pDeviceOutput = &tempDecryptBuffer;
//                             decryptState.nInputOffset = 0;
//                             // For OLD format, subtract 16 bytes for IV that was read from stream
//                             // For NEW format, IV is in properties so don't subtract
//                             decryptState.nInputLimit = bNewFormat ? nSolidStreamSize : (nSolidStreamSize - 16);
                            
//                             qDebug() << "[Solid Encrypted] Using password:" << pContext->sPassword << "(" << pContext->sPassword.length() << "chars)";
                            
//                             XAESDecoder aesDecoder;
//                             bool bDecrypted = aesDecoder.decrypt(&decryptState, baFullAesProperties, pContext->sPassword, pPdStruct);
//                             sdEncrypted.close();
                            
//                             if (!bDecrypted) {
//                                 qWarning() << "[Solid Encrypted] Decryption failed";
//                                 tempDecryptBuffer.close();
//                                 bResult = false;
//                             } else {
//                                 QByteArray baDecryptedData = tempDecryptBuffer.data();
//                                 tempDecryptBuffer.close();
                                
//                                 qDebug() << "[Solid Encrypted] Decrypted" << baDecryptedData.size() << "bytes";
                                
//                                 // Debug: Hex dump first 32 bytes of decrypted data
//                                 QString sDecryptedHex;
//                                 for (qint32 i = 0; i < qMin(32, baDecryptedData.size()); i++) {
//                                     sDecryptedHex += QString("%1 ").arg((quint8)baDecryptedData.at(i), 2, 16, QChar('0'));
//                                 }
//                                 qDebug() << "[Solid Encrypted] Decrypted data (first 32 bytes):" << sDecryptedHex;
                                
//                                 // Step 2: Decompress the decrypted data
//                                 QBuffer decryptedBuffer(&baDecryptedData);
//                                 if (!decryptedBuffer.open(QIODevice::ReadOnly)) {
//                                     qWarning() << "[Solid Encrypted] Failed to open decrypted buffer";
//                                     bResult = false;
//                                 } else {
//                                     QIODevice *pDecompressBuffer = createFileBuffer(nTotalUncompressedSize, pPdStruct);
//                                     if (!pDecompressBuffer || !pDecompressBuffer->open(QIODevice::WriteOnly)) {
//                                         qWarning() << "[Solid Encrypted] Failed to create decompression buffer";
//                                         decryptedBuffer.close();
//                                         if (pDecompressBuffer) freeFileBuffer(&pDecompressBuffer);
//                                         bResult = false;
//                                     } else {
//                                         XBinary::DATAPROCESS_STATE decompressState = {};
//                                         decompressState.pDeviceInput = &decryptedBuffer;
//                                         decompressState.pDeviceOutput = pDecompressBuffer;
//                                         decompressState.nInputOffset = 0;
//                                         decompressState.nInputLimit = baDecryptedData.size();
                                        
//                                         bool bDecompressed = false;
//                                         if (compressMethod == COMPRESS_METHOD_LZMA2) {
//                                             XLZMADecoder lzmaDecoder;
//                                             bDecompressed = lzmaDecoder.decompressLZMA2(&decompressState, baCompressProperties, pPdStruct);
//                                         } else if (compressMethod == COMPRESS_METHOD_LZMA) {
//                                             XLZMADecoder lzmaDecoder;
//                                             bDecompressed = lzmaDecoder.decompress(&decompressState, baCompressProperties, pPdStruct);
//                                         } else if (compressMethod == COMPRESS_METHOD_BZIP2) {
//                                             bDecompressed = XBZIP2Decoder::decompress(&decompressState, pPdStruct);
//                                         } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
//                                             bDecompressed = XDeflateDecoder::decompress(&decompressState, pPdStruct);
//                                         } else {
//                                             qWarning() << "[Solid Encrypted] Unsupported compression method:" << compressMethod;
//                                         }
                                        
//                                         decryptedBuffer.close();
//                                         pDecompressBuffer->close();
                                        
//                                         if (bDecompressed) {
//                                             // Read decompressed data and cache it
//                                             pDecompressBuffer->open(QIODevice::ReadOnly);
//                                             QByteArray baSolidData = pDecompressBuffer->readAll();
//                                             pDecompressBuffer->close();
                                            
//                                             qDebug() << "[Solid Encrypted] Decompressed" << baSolidData.size() << "bytes (expected" << nTotalUncompressedSize << ")";
                                            
//                                             if (baSolidData.size() == nTotalUncompressedSize) {
//                                                 // Cache the decompressed solid block for this folder
//                                                 pContext->mapFolderCache.insert(nFolderIndex, baSolidData);
//                                                 qDebug() << "[Solid Encrypted] Solid block for folder" << nFolderIndex << "cached successfully";
                                                
//                                                 // Now extract this file from the cached block
//                                                 qint64 nOffsetInDecompressed = ar.nDecompressedOffset;
//                                                 qint64 nSize = ar.nDecompressedSize;
                                                
//                                                 if (nOffsetInDecompressed >= 0 && nSize > 0 &&
//                                                     nOffsetInDecompressed + nSize <= baSolidData.size()) {
//                                                     qint64 nWritten = pDevice->write(baSolidData.constData() + nOffsetInDecompressed, nSize);
//                                                     bResult = (nWritten == nSize);
//                                                 } else {
//                                                     qWarning() << "[Solid Encrypted] Invalid extraction bounds";
//                                                     bResult = false;
//                                                 }
//                                             } else {
//                                                 qWarning() << "[Solid Encrypted] Decompression size mismatch";
//                                                 bResult = false;
//                                             }
//                                         } else {
//                                             qWarning() << "[Solid Encrypted] Decompression failed";
//                                             bResult = false;
//                                         }
                                        
//                                         freeFileBuffer(&pDecompressBuffer);
//                                     }
//                                 }
//                             }
//                         }
//                     }
//                 }
//             } else if (bIsSolid && pContext->mapFolderCache.contains(nFolderIndex)) {
//                 // Extract from cached solid block for this folder
//                 QByteArray baDecompressed = pContext->mapFolderCache.value(nFolderIndex);
//                 // qint64 nOffsetInDecompressed = ar.nDecompressedOffset;  // Offset in decompressed buffer
//                 // qint64 nSize = ar.nDecompressedSize;
                
//                 qDebug() << "[Solid] Extracting:" << ar.mapProperties.value(FPART_PROP_ORIGINALNAME).toString()
//                          << "Folder:" << nFolderIndex
//                          << "BufferSize:" << baDecompressed.size()
//                          << "Offset:" << nOffsetInDecompressed
//                          << "Size:" << nSize;
                
//                 if (nOffsetInDecompressed >= 0 && nSize > 0 &&
//                     nOffsetInDecompressed + nSize <= baDecompressed.size()) {
//                     // Write data from cached buffer
//                     qint64 nWritten = pDevice->write(baDecompressed.constData() + nOffsetInDecompressed, nSize);
//                     bResult = (nWritten == nSize);
                    
//                     if (!bResult) {
//                         qWarning() << "[Solid] Write FAILED: expected" << nSize << "wrote" << nWritten;
//                     }
//                 } else {
//                     qWarning() << "[Solid] Invalid bounds: offset=" << nOffsetInDecompressed
//                               << "size=" << nSize << "bufferSize=" << baDecompressed.size()
//                               << "check=" << (nOffsetInDecompressed + nSize) << "<=" << baDecompressed.size();
//                 }
//             } else if (bIsSolid && !bIsEncrypted && !pContext->mapFolderCache.contains(nFolderIndex)) {
//                 // Solid non-encrypted folder - decompress entire folder and cache it
//                 qDebug() << "[Solid Non-Encrypted] Folder" << nFolderIndex << "first access - decompressing entire solid block";
                
//                 // Calculate total uncompressed size for this folder
//                 qint64 nTotalUncompressedSize = 0;
//                 for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
//                     ARCHIVERECORD &rec = pContext->listArchiveRecords[i];
//                     bool bRecIsSolid = rec.mapProperties.value(FPART_PROP_SOLID, false).toBool();
//                     if (bRecIsSolid && rec.nStreamOffset == ar.nStreamOffset && rec.nDecompressedSize > 0) {
//                         qint64 nEndOffset = rec.nDecompressedOffset + rec.nDecompressedSize;
//                         if (nEndOffset > nTotalUncompressedSize) {
//                             nTotalUncompressedSize = nEndOffset;
//                         }
//                     }
//                 }
                
//                 qDebug() << "[Solid Non-Encrypted]   Stream offset:" << ar.nStreamOffset;
//                 qDebug() << "[Solid Non-Encrypted]   Stream size:" << ar.nStreamSize;
//                 qDebug() << "[Solid Non-Encrypted]   Expected decompressed size:" << nTotalUncompressedSize;
                
//                 // Decompress entire solid block
//                 SubDevice sd(getDevice(), ar.nStreamOffset, ar.nStreamSize);
//                 if (sd.open(QIODevice::ReadOnly)) {
//                     QBuffer *pDecompressBuffer = new QBuffer();
//                     if (pDecompressBuffer->open(QIODevice::WriteOnly)) {
//                         COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_UNKNOWN).toUInt();
//                         QByteArray baCompressProperties = ar.mapProperties.value(FPART_PROP_COMPRESSPROPERTIES, QByteArray()).toByteArray();
                        
//                         DATAPROCESS_STATE decompressState = {};
//                         decompressState.pDeviceInput = &sd;
//                         decompressState.pDeviceOutput = pDecompressBuffer;
//                         decompressState.nInputOffset = 0;
//                         decompressState.nInputLimit = ar.nStreamSize;
                        
//                         bool bDecompressed = false;
//                         if (compressMethod == COMPRESS_METHOD_LZMA2) {
//                             XLZMADecoder lzmaDecoder;
//                             bDecompressed = lzmaDecoder.decompressLZMA2(&decompressState, baCompressProperties, pPdStruct);
//                         } else if (compressMethod == COMPRESS_METHOD_LZMA) {
//                             XLZMADecoder lzmaDecoder;
//                             bDecompressed = lzmaDecoder.decompress(&decompressState, baCompressProperties, pPdStruct);
//                         } else if (compressMethod == COMPRESS_METHOD_BZIP2) {
//                             bDecompressed = XBZIP2Decoder::decompress(&decompressState, pPdStruct);
//                         } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
//                             bDecompressed = XDeflateDecoder::decompress(&decompressState, pPdStruct);
//                         } else {
//                             qWarning() << "[Solid Non-Encrypted] Unsupported compression method:" << compressMethod;
//                         }
                        
//                         pDecompressBuffer->close();
                        
//                         if (bDecompressed) {
//                             pDecompressBuffer->open(QIODevice::ReadOnly);
//                             QByteArray baSolidData = pDecompressBuffer->readAll();
//                             pDecompressBuffer->close();
                            
//                             qDebug() << "[Solid Non-Encrypted] Decompressed" << baSolidData.size() << "bytes (expected" << nTotalUncompressedSize << ")";
                            
//                             if (baSolidData.size() > 0) {
//                                 // Cache the decompressed solid block
//                                 pContext->mapFolderCache.insert(nFolderIndex, baSolidData);
//                                 qDebug() << "[Solid Non-Encrypted] Solid block for folder" << nFolderIndex << "cached successfully";
                                
//                                 // Now extract this file from the cached block
//                                 qint64 nOffsetInDecompressed = ar.nDecompressedOffset;
//                                 qint64 nSize = ar.nDecompressedSize;
                                
//                                 if (nOffsetInDecompressed >= 0 && nSize > 0 &&
//                                     nOffsetInDecompressed + nSize <= baSolidData.size()) {
//                                     qint64 nWritten = pDevice->write(baSolidData.constData() + nOffsetInDecompressed, nSize);
//                                     bResult = (nWritten == nSize);
//                                 } else {
//                                     qWarning() << "[Solid Non-Encrypted] Invalid bounds: offset=" << nOffsetInDecompressed
//                                               << "size=" << nSize << "bufferSize=" << baSolidData.size();
//                                     bResult = false;
//                                 }
//                             } else {
//                                 qWarning() << "[Solid Non-Encrypted] Decompression produced 0 bytes";
//                                 bResult = false;
//                             }
//                         } else {
//                             qWarning() << "[Solid Non-Encrypted] Decompression failed";
//                             bResult = false;
//                         }
                        
//                         delete pDecompressBuffer;
//                     }
//                     sd.close();
//                 } else {
//                     qWarning() << "[Solid Non-Encrypted] Failed to open SubDevice";
//                     bResult = false;
//                 }
//             } else if (ar.nStreamSize > 0) {
//             SubDevice sd(getDevice(), ar.nStreamOffset, ar.nStreamSize);
            
//             qDebug() << "[SubDevice] Created: offset=" << ar.nStreamOffset << "size=" << ar.nStreamSize;

//             if (sd.open(QIODevice::ReadOnly)) {
//                 bResult = true;  // Initialize to true after successful open
                
//                 // Decompress if compressed
//                 if (ar.mapProperties.contains(FPART_PROP_COMPRESSMETHOD)) {
//                     COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();

//                     DATAPROCESS_STATE state = {};
//                     state.pDeviceInput = &sd;
//                     state.pDeviceOutput = pDevice;
//                     state.nInputOffset = 0;
//                     state.nInputLimit = ar.nStreamSize;
//                     state.nProcessedOffset = 0;
//                     state.nProcessedLimit = -1;
//                     state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, ar.nDecompressedSize);
                    
// #ifdef QT_DEBUG
//                     qDebug() << "Decompression setup: InputLimit=" << state.nInputLimit << "DecompressedSize=" << ar.nDecompressedSize;
// #endif

//                     qDebug() << "[Decompression] Method:" << compressMethod
//                              << "StreamSize:" << ar.nStreamSize
//                              << "DecompressedSize:" << ar.nDecompressedSize;
                    
//                     // Handle AES encryption first (if present)
//                     QByteArray baDecryptedData;  // Store decrypted data if AES is used
//                     QBuffer *pDecryptedBuffer = nullptr;
//                     bool bNeedDecryption = ar.mapProperties.value(FPART_PROP_ENCRYPTED, false).toBool();
                    
//                     qDebug() << "[AES Check] bNeedDecryption:" << bNeedDecryption << "bResult:" << bResult;
                    
//                     if (bNeedDecryption && bResult) {
//                         if (pContext->sPassword.isEmpty()) {
//                             qWarning() << "[XSevenZip] AES-encrypted archive requires a password";
//                             bResult = false;
//                         } else {
//                             // Get AES properties
//                             QByteArray baAesProperties;
//                             if (ar.mapProperties.contains(FPART_PROP_AESKEY)) {
//                                 baAesProperties = ar.mapProperties.value(FPART_PROP_AESKEY).toByteArray();
//                             }
                            
//                             if (baAesProperties.isEmpty()) {
//                                 qWarning() << "[XSevenZip] AES properties not found";
//                                 bResult = false;
//                             } else {
//                                 // Check if this is NEW format (IV in properties) or OLD format (IV in stream)
//                                 // According to 7-Zip SDK (7zAes.cpp):
//                                 //   OLD format: (b0 & 0xC0) == 0 (neither bit 6 nor bit 7 set) - IV read from stream
//                                 //   NEW format: (b0 & 0xC0) != 0 (at least one of bits 6-7 set) - IV in properties
//                                 bool bNewFormat = false;
//                                 if (baAesProperties.size() >= 1) {
//                                     quint8 nFirstByte = (quint8)baAesProperties[0];
//                                     bNewFormat = ((nFirstByte & 0xC0) != 0);  // NEW format if ANY of bits 6-7 set
//                                 }
                                
//                                 qDebug() << "[XSevenZip] Decrypting AES-encrypted stream";
//                                 qDebug() << "  AES properties size:" << baAesProperties.size();
//                                 qDebug() << "  Encrypted stream size:" << ar.nStreamSize;
//                                 qDebug() << "  Format:" << (bNewFormat ? "NEW (IV in properties)" : "OLD (IV in stream)");
                                
//                                 // Create a temporary buffer for decrypted data
//                                 QBuffer tempDecryptBuffer;
//                                 tempDecryptBuffer.open(QIODevice::WriteOnly);
                                
//                                 XBinary::DATAPROCESS_STATE decryptState = {};
//                                 decryptState.pDeviceInput = &sd;  // Read from SubDevice, not main archive
//                                 decryptState.pDeviceOutput = &tempDecryptBuffer;
//                                 decryptState.nCountInput = 0;
//                                 decryptState.nInputOffset = 0;  // SubDevice starts at offset 0
//                                 // For OLD format: stream size includes IV (16 bytes) + encrypted data
//                                 // For NEW format: stream size is just encrypted data (IV in properties)
//                                 decryptState.nInputLimit = ar.nStreamSize;
//                                 decryptState.nProcessedLimit = ar.nStreamSize;
                                
//                                 // Decrypt using AES decoder
//                                 XAESDecoder aesDecoder;
//                                 bResult = aesDecoder.decrypt(&decryptState, baAesProperties, pContext->sPassword, pPdStruct);
                                
//                                 if (!bResult) {
//                                     qWarning() << "[XSevenZip] AES decryption failed";
//                                     tempDecryptBuffer.close();
//                                 } else {
//                                     // Get decrypted data
//                                     baDecryptedData = tempDecryptBuffer.data();
//                                     tempDecryptBuffer.close();
                                    
//                                     qDebug() << "[XSevenZip] Decrypted" << baDecryptedData.size() << "bytes";
                                    
//                                     // Now we need to decompress the decrypted data using the actual compression method
//                                     // Get compression method from record
//                                     compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE).toUInt();
                                    
//                                     // Create a buffer from decrypted data for decompression
//                                     pDecryptedBuffer = new QBuffer(&baDecryptedData);
//                                     if (!pDecryptedBuffer->open(QIODevice::ReadOnly)) {
//                                         qWarning() << "[XSevenZip] Failed to open decrypted buffer";
//                                         bResult = false;
//                                         delete pDecryptedBuffer;
//                                         pDecryptedBuffer = nullptr;
//                                     } else {
//                                         // Update state to read from decrypted buffer instead of archive file
//                                         state.pDeviceInput = pDecryptedBuffer;
//                                         state.nCountInput = 0;
//                                         state.nInputOffset = 0;
//                                         state.nInputLimit = baDecryptedData.size();
//                                         state.nProcessedLimit = baDecryptedData.size();
                                        
//                                         qDebug() << "[XSevenZip] Now decompressing" << compressMethod << "from decrypted data";
//                                     }
//                                 }
//                             }
//                         }
//                     }
                    
                    
//                     // Clean up decrypted buffer if it was created
//                     if (pDecryptedBuffer) {
//                         pDecryptedBuffer->close();
//                         delete pDecryptedBuffer;
//                         pDecryptedBuffer = nullptr;
//                     }
//                 } // End if (bResult) from AES decryption check

//                 sd.close();
//             }
// #ifdef QT_DEBUG
//             else {
//                 qDebug() << "_unpack: No compressed stream data (nStreamSize=" << ar.nStreamSize << ")";
//             }
// #endif
//             } else if (bIsSolid) {
// #ifdef QT_DEBUG
//                 qWarning() << "_unpack: Solid archive but no cached data available";
// #endif
//             }
//         }  // End of else block for non-empty files

// #ifdef QT_DEBUG
//         if (!bResult) {
//             qDebug() << "_unpack: Failed to unpack record at index" << pState->nCurrentIndex;
//         } else {
//             qint64 nActualSize = pDevice->size();
//             qDebug() << "_unpack: Successfully unpacked. Expected:" << ar.nDecompressedSize << "bytes, Actual:" << nActualSize << "bytes";
//         }
// #endif
//     }

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

bool XSevenZip::initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pDevice) {
        // Create packing context
        SEVENZ_PACK_CONTEXT *pContext = new SEVENZ_PACK_CONTEXT;
        pContext->nHeaderOffset = 0;
        pContext->compressMethod = COMPRESS_METHOD_DEFLATE;  // Default compression (LZMA2 compression not yet implemented)
        pContext->nCompressionLevel = 5;                     // Default level

        // Initialize state
        pState->pDevice = pDevice;
        pState->mapProperties = mapProperties;
        pState->nCurrentOffset = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;

        // Apply properties
        if (mapProperties.contains(PACK_PROP_COMPRESSMETHOD)) {
            QString sMethod = mapProperties.value(PACK_PROP_COMPRESSMETHOD).toString().toUpper();
            if (sMethod == "STORE" || sMethod == "COPY") {
                pContext->compressMethod = COMPRESS_METHOD_STORE;
            } else if (sMethod == "DEFLATE") {
                pContext->compressMethod = COMPRESS_METHOD_DEFLATE;
            } else if (sMethod == "BZIP2" || sMethod == "BZ2") {
                pContext->compressMethod = COMPRESS_METHOD_BZIP2;
            } else if (sMethod == "LZMA") {
                pContext->compressMethod = COMPRESS_METHOD_LZMA;
            } else if (sMethod == "LZMA2") {
                pContext->compressMethod = COMPRESS_METHOD_LZMA2;
            }
            // else keep default (DEFLATE)
        }

        if (mapProperties.contains(PACK_PROP_COMPRESSIONLEVEL)) {
            pContext->nCompressionLevel = mapProperties.value(PACK_PROP_COMPRESSIONLEVEL).toInt();
        }

        // Write signature header (placeholder, will be updated in finishPack)
        if (pDevice->isWritable()) {
            SIGNATUREHEADER signatureHeader = {};
            signatureHeader.kSignature[0] = '7';
            signatureHeader.kSignature[1] = 'z';
            signatureHeader.kSignature[2] = 0xBC;
            signatureHeader.kSignature[3] = 0xAF;
            signatureHeader.kSignature[4] = 0x27;
            signatureHeader.kSignature[5] = 0x1C;
            signatureHeader.Major = 0;
            signatureHeader.Minor = 4;
            signatureHeader.StartHeaderCRC = 0;    // Will be calculated later
            signatureHeader.NextHeaderOffset = 0;  // Will be set in finishPack
            signatureHeader.NextHeaderSize = 0;    // Will be set in finishPack
            signatureHeader.NextHeaderCRC = 0;     // Will be calculated later

            qint64 nWritten = pDevice->write((char *)&signatureHeader, sizeof(SIGNATUREHEADER));
            if (nWritten == sizeof(SIGNATUREHEADER)) {
                pState->nCurrentOffset = sizeof(SIGNATUREHEADER);
                pContext->nHeaderOffset = sizeof(SIGNATUREHEADER);
                bResult = true;
            }
        }
    }

    return bResult;
}

bool XSevenZip::addDevice(PACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext && pDevice) {
        SEVENZ_PACK_CONTEXT *pContext = (SEVENZ_PACK_CONTEXT *)pState->pContext;

        // Create archive record
        ARCHIVERECORD record = {};
        // record.nDecompressedSize = pDevice->size();
        record.mapProperties[FPART_PROP_COMPRESSMETHOD] = pContext->compressMethod;

        // Compress the data
        if (pDevice->open(QIODevice::ReadOnly)) {
            QByteArray baData = pDevice->readAll();
            pDevice->close();

            QByteArray baCompressed;
            QBuffer bufferCompressed(&baCompressed);

            if (bufferCompressed.open(QIODevice::WriteOnly)) {
                // Compress the data using XArchive::_compress

                QBuffer bufferInput(&baData);
                if (bufferInput.open(QIODevice::ReadOnly)) {
                    COMPRESS_RESULT result = _compress(pContext->compressMethod, &bufferInput, &bufferCompressed);
                    bufferInput.close();

                    if (result == COMPRESS_RESULT_OK) {
                        bufferCompressed.close();

                        record.nStreamSize = baCompressed.size();

                        // Calculate CRC32
                        quint32 *pCRCTable = _getCRC32Table_EDB88320();
                        quint32 nCRC = _getCRC32(baData, 0xFFFFFFFF, pCRCTable);
                        nCRC = ~nCRC;

                        // Store compressed data and metadata
                        pContext->listCompressedData.append(baCompressed);
                        pContext->listCRCs.append(nCRC);
                        pContext->listArchiveRecords.append(record);

                        pState->nNumberOfRecords++;
                        bResult = true;
                    }
                }
            }
        }
    }

    return bResult;
}

bool XSevenZip::addFile(PACK_STATE *pState, const QString &sFileName, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        SEVENZ_PACK_CONTEXT *pContext = (SEVENZ_PACK_CONTEXT *)pState->pContext;

        QFile file(sFileName);
        if (file.exists()) {
            // Create archive record with file metadata
            ARCHIVERECORD record = {};
            // record.nDecompressedSize = file.size();
            record.mapProperties[FPART_PROP_ORIGINALNAME] = QFileInfo(sFileName).fileName();
            record.mapProperties[FPART_PROP_COMPRESSMETHOD] = pContext->compressMethod;

            // Get file attributes
            QFileInfo fileInfo(sFileName);
            if (fileInfo.exists()) {
                record.mapProperties[FPART_PROP_DATETIME] = fileInfo.lastModified();
            }

            // Compress and add the file
            if (file.open(QIODevice::ReadOnly)) {
                QByteArray baData = file.readAll();
                file.close();

                QByteArray baCompressed;
                QBuffer bufferCompressed(&baCompressed);

                if (bufferCompressed.open(QIODevice::WriteOnly)) {
                    // Compress the data using XArchive::_compress
                    QBuffer bufferInput(&baData);
                    if (bufferInput.open(QIODevice::ReadOnly)) {
                        COMPRESS_RESULT result = _compress(pContext->compressMethod, &bufferInput, &bufferCompressed);
                        bufferInput.close();

                        if (result == COMPRESS_RESULT_OK) {
                            bufferCompressed.close();

                            record.nStreamSize = baCompressed.size();

                            // Calculate CRC32
                            quint32 *pCRCTable = _getCRC32Table_EDB88320();
                            quint32 nCRC = _getCRC32(baData, 0xFFFFFFFF, pCRCTable);
                            nCRC = ~nCRC;

                            // Store compressed data and metadata
                            pContext->listCompressedData.append(baCompressed);
                            pContext->listCRCs.append(nCRC);
                            pContext->listArchiveRecords.append(record);

                            pState->nNumberOfRecords++;
                            bResult = true;
                        }
                    }
                }
            }
        }
    }

    return bResult;
}

bool XSevenZip::addFolder(PACK_STATE *pState, const QString &sDirectoryPath, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext) {
        QDir directory(sDirectoryPath);
        if (directory.exists()) {
            // Get all files recursively
            QFileInfoList listFiles = directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::AllDirs);

            qint32 nAdded = 0;

            for (qint32 i = 0; i < listFiles.count(); i++) {
                QFileInfo fileInfo = listFiles.at(i);

                if (fileInfo.isDir()) {
                    // Recursively add subdirectory
                    if (addFolder(pState, fileInfo.absoluteFilePath(), pPdStruct)) {
                        nAdded++;
                    }
                } else if (fileInfo.isFile()) {
                    // Add file
                    if (addFile(pState, fileInfo.absoluteFilePath(), pPdStruct)) {
                        nAdded++;
                    }
                }
            }

            bResult = (nAdded > 0);
        }
    }

    return bResult;
}

// Helper function to write a packed number in 7z format
QByteArray XSevenZip::_writePackedNumber(quint64 nValue)
{
    QByteArray baResult;

    if (nValue < 0x80) {
        // 0xxxxxxx - 7 bits
        baResult.append((char)(nValue & 0x7F));
    } else if (nValue < 0x4000) {
        // 10xxxxxx xxxxxxxx - 14 bits
        baResult.append((char)(0x80 | ((nValue >> 8) & 0x3F)));
        baResult.append((char)(nValue & 0xFF));
    } else if (nValue < 0x200000) {
        // 110xxxxx xxxxxxxx xxxxxxxx - 21 bits
        baResult.append((char)(0xC0 | ((nValue >> 16) & 0x1F)));
        baResult.append((char)((nValue >> 8) & 0xFF));
        baResult.append((char)(nValue & 0xFF));
    } else if (nValue < 0x10000000) {
        // 1110xxxx xxxxxxxx xxxxxxxx xxxxxxxx - 28 bits
        baResult.append((char)(0xE0 | ((nValue >> 24) & 0x0F)));
        baResult.append((char)((nValue >> 16) & 0xFF));
        baResult.append((char)((nValue >> 8) & 0xFF));
        baResult.append((char)(nValue & 0xFF));
    } else {
        // For larger values, use extended format
        // 11111111 + 8 bytes
        baResult.append((char)0xFF);
        for (qint32 i = 0; i < 8; i++) {
            baResult.append((char)((nValue >> (i * 8)) & 0xFF));
        }
    }

    return baResult;
}

void XSevenZip::_writeId(QIODevice *pDevice, quint8 nId)
{
    pDevice->write((const char *)&nId, 1);
}

void XSevenZip::_writeNumber(QIODevice *pDevice, quint64 nValue)
{
    QByteArray baNumber = _writePackedNumber(nValue);
    pDevice->write(baNumber);
}

void XSevenZip::_writeByte(QIODevice *pDevice, quint8 nByte)
{
    pDevice->write((const char *)&nByte, 1);
}

bool XSevenZip::_decompress(QIODevice *pDevice, COMPRESS_METHOD compressMethod, const QByteArray &baProperty, qint64 nOffset, qint64 nSize, qint64 nUncompressedSize, PDSTRUCT *pPdStruct)
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

bool XSevenZip::finishPack(PACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext && pState->pDevice) {
        SEVENZ_PACK_CONTEXT *pContext = (SEVENZ_PACK_CONTEXT *)pState->pContext;
        QIODevice *pDevice = pState->pDevice;

        // Write all compressed data streams
        qint64 nDataOffset = pState->nCurrentOffset;
        QList<qint64> listPackSizes;

        for (qint32 i = 0; i < pContext->listCompressedData.count(); i++) {
            QByteArray baData = pContext->listCompressedData.at(i);
            qint64 nWritten = pDevice->write(baData);

            if (nWritten != baData.size()) {
                delete pContext;
                pState->pContext = nullptr;
                return false;
            }

            listPackSizes.append(baData.size());
            pState->nCurrentOffset += nWritten;
        }

        // Build the header in a buffer
        QByteArray baHeader;
        QBuffer bufferHeader(&baHeader);
        bufferHeader.open(QIODevice::WriteOnly);

        // k7zIdHeader
        _writeId(&bufferHeader, k7zIdHeader);

        // k7zIdMainStreamsInfo
        _writeId(&bufferHeader, k7zIdMainStreamsInfo);

        // k7zIdPackInfo
        _writeId(&bufferHeader, k7zIdPackInfo);
        _writeNumber(&bufferHeader, 0);                      // PackPos (offset from end of signature header to packed data)
        _writeNumber(&bufferHeader, listPackSizes.count());  // NumberOfPackStreams

        // k7zIdSize - sizes of packed streams
        _writeId(&bufferHeader, k7zIdSize);
        for (qint32 i = 0; i < listPackSizes.count(); i++) {
            _writeNumber(&bufferHeader, listPackSizes.at(i));
        }

        _writeId(&bufferHeader, k7zIdEnd);  // End of PackInfo

        // k7zIdUnpackInfo
        _writeId(&bufferHeader, k7zIdUnpackInfo);

        // k7zIdFolder - write one folder per file (not solid)
        _writeId(&bufferHeader, k7zIdFolder);
        _writeNumber(&bufferHeader, pContext->listArchiveRecords.count());  // NumberOfFolders = number of files
        _writeByte(&bufferHeader, 0);                                       // External = 0 (folder info follows)

        // Write folder definition for each file
        for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
            _writeNumber(&bufferHeader, 1);  // NumberOfCoders (each folder has 1 coder)

            // Coder info - write codec ID based on compression method
            QByteArray baCodecId;
            if (pContext->compressMethod == COMPRESS_METHOD_STORE) {
                baCodecId = QByteArray("\x00", 1);  // COPY/STORE
            } else if (pContext->compressMethod == COMPRESS_METHOD_DEFLATE) {
                baCodecId = QByteArray("\x04\x01\x08", 3);  // DEFLATE
            } else if (pContext->compressMethod == COMPRESS_METHOD_BZIP2) {
                baCodecId = QByteArray("\x04\x02\x02", 3);  // BZIP2
            } else if (pContext->compressMethod == COMPRESS_METHOD_LZMA) {
                baCodecId = QByteArray("\x03\x01\x01", 3);  // LZMA
            } else if (pContext->compressMethod == COMPRESS_METHOD_LZMA2) {
                baCodecId = QByteArray("\x21", 1);  // LZMA2
            } else {
                baCodecId = QByteArray("\x00", 1);  // Default to COPY
            }

            quint8 nCodecIdSize = baCodecId.size();
            quint8 nFlag = nCodecIdSize;  // Low 4 bits = codec ID size
            _writeByte(&bufferHeader, nFlag);

            // Write codec ID
            bufferHeader.write(baCodecId.data(), baCodecId.size());
        }

        // k7zIdCodersUnpackSize - unpack sizes for each coder
        _writeId(&bufferHeader, k7zIdCodersUnpackSize);
        for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
            _writeNumber(&bufferHeader, pContext->listArchiveRecords.at(i).mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong());
        }

        // k7zIdCRC (optional - folder CRCs)
        // Skip for now

        _writeId(&bufferHeader, k7zIdEnd);  // End of UnpackInfo

        // k7zIdSubStreamsInfo
        // Since we have 1 file per folder (not solid), SubStreamsInfo only needs CRCs
        _writeId(&bufferHeader, k7zIdSubStreamsInfo);

        // k7zIdCRC - CRCs for each file
        _writeId(&bufferHeader, k7zIdCRC);
        _writeNumber(&bufferHeader, pContext->listCRCs.count());  // NumberOfCRCs
        for (qint32 i = 0; i < pContext->listCRCs.count(); i++) {
            quint32 nCRC = pContext->listCRCs.at(i);
            bufferHeader.write((const char *)&nCRC, 4);
        }

        _writeId(&bufferHeader, k7zIdEnd);  // End of SubStreamsInfo

        _writeId(&bufferHeader, k7zIdEnd);  // End of MainStreamsInfo

        // k7zIdFilesInfo
        _writeId(&bufferHeader, k7zIdFilesInfo);
        _writeNumber(&bufferHeader, pContext->listArchiveRecords.count());  // NumberOfFiles

        // k7zIdEmptyStream (if any files are empty)
        // Skip for now - assume no empty files

        // k7zIdName - file names
        if (pContext->listArchiveRecords.count() > 0) {
            _writeId(&bufferHeader, k7zIdName);

            // Calculate total size needed for names
            qint64 nNamesSize = 1;  // 1 byte for "AllAreDefined" flag
            for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
                QString sName = pContext->listArchiveRecords.at(i).mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
                if (sName.isEmpty()) {
                    sName = QString("file%1").arg(i);
                }
                nNamesSize += (sName.length() + 1) * 2;  // UTF-16LE + null terminator
            }

            _writeNumber(&bufferHeader, nNamesSize);
            _writeByte(&bufferHeader, 0);  // AllAreDefined = 0 (external data follows)

            // Write names in UTF-16LE
            for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
                QString sName = pContext->listArchiveRecords.at(i).mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
                if (sName.isEmpty()) {
                    sName = QString("file%1").arg(i);
                }

                // Properly encode to UTF-16LE
                for (qint32 j = 0; j < sName.length(); j++) {
                    quint16 nChar = sName.at(j).unicode();
                    bufferHeader.write((const char *)&nChar, 2);  // Write as little-endian uint16
                }
                quint16 nNull = 0;
                bufferHeader.write((const char *)&nNull, 2);  // Null terminator
            }
        }

        _writeId(&bufferHeader, k7zIdEnd);  // End of FilesInfo

        _writeId(&bufferHeader, k7zIdEnd);  // End of Header

        bufferHeader.close();

        // Write header to archive
        qint64 nHeaderOffset = pState->nCurrentOffset - pContext->nHeaderOffset;
        qint64 nHeaderSize = baHeader.size();
        qint64 nWritten = pDevice->write(baHeader);

        if (nWritten != nHeaderSize) {
            delete pContext;
            pState->pContext = nullptr;
            return false;
        }

        // Calculate header CRC
        quint32 *pCRCTable = _getCRC32Table_EDB88320();
        quint32 nHeaderCRC = _getCRC32(baHeader, 0xFFFFFFFF, pCRCTable);
        nHeaderCRC = ~nHeaderCRC;

        // Update signature header with correct offsets
        SIGNATUREHEADER signatureHeader = {};
        signatureHeader.kSignature[0] = '7';
        signatureHeader.kSignature[1] = 'z';
        signatureHeader.kSignature[2] = 0xBC;
        signatureHeader.kSignature[3] = 0xAF;
        signatureHeader.kSignature[4] = 0x27;
        signatureHeader.kSignature[5] = 0x1C;
        signatureHeader.Major = 0;
        signatureHeader.Minor = 4;
        signatureHeader.NextHeaderOffset = nHeaderOffset;
        signatureHeader.NextHeaderSize = nHeaderSize;
        signatureHeader.NextHeaderCRC = nHeaderCRC;

        // Calculate StartHeaderCRC (CRC of NextHeaderOffset, NextHeaderSize, NextHeaderCRC)
        QByteArray baHeaderInfo;
        baHeaderInfo.append((char *)&signatureHeader.NextHeaderOffset, 8);
        baHeaderInfo.append((char *)&signatureHeader.NextHeaderSize, 8);
        baHeaderInfo.append((char *)&signatureHeader.NextHeaderCRC, 4);

        quint32 nCRC = _getCRC32(baHeaderInfo, 0xFFFFFFFF, pCRCTable);
        signatureHeader.StartHeaderCRC = ~nCRC;

        // Seek back and rewrite signature header
        if (pDevice->seek(0)) {
            nWritten = pDevice->write((char *)&signatureHeader, sizeof(SIGNATUREHEADER));
            bResult = (nWritten == sizeof(SIGNATUREHEADER));
        }

        // Clean up context
        delete pContext;
        pState->pContext = nullptr;
    }

    return bResult;
}
