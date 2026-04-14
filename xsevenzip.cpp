/* Copyright (c) 2017-2026 hors<horsic@gmail.com>
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

bool XSevenZip::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
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

bool XSevenZip::isCommentPresent()
{
    return !getComment().isEmpty();
}

QString XSevenZip::getComment()
{
    QString sResult;

    SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
    qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
    qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

    if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
        QByteArray baData;
        baData.resize(nNextHeaderSize);
        qint64 nHeaderSize = nNextHeaderSize;

        qint64 nBytesRead = read_array_process(nNextHeaderOffset, baData.data(), nNextHeaderSize, nullptr);

        if (nBytesRead == nNextHeaderSize) {
            bool bHeader = false;
            bool bIsEncodedHeader = false;

            if (nBytesRead > 0) {
                quint8 nFirstByte = (quint8)baData.data()[0];
                bIsEncodedHeader = (nFirstByte == (quint8)k7zIdEncodedHeader);
            }

            if (bIsEncodedHeader) {
                QList<XSevenZip::SZRECORD> listRecords;

                SZSTATE state = {};
                state.pData = baData.data();
                state.nSize = nNextHeaderSize;
                state.nCurrentOffset = 0;
                state.bIsError = false;
                state.sErrorString = QString();

                _handleId(&listRecords, XSevenZip::k7zIdEncodedHeader, &state, 1, true, nullptr, IMPTYPE_UNKNOWN);

                baData.clear();

                QBuffer bufferOut;
                bufferOut.setBuffer(&baData);

                QMap<UNPACK_PROP, QVariant> mapProperties;

                if (bufferOut.open(QIODevice::ReadWrite)) {
                    bHeader = decompressHeader(mapProperties, &bufferOut, &state, nullptr);
                    bufferOut.close();
                    nHeaderSize = baData.size();
                }
            } else {
                bHeader = true;
            }

            if (bHeader) {
                QList<XSevenZip::SZRECORD> listRecords;

                SZSTATE state = {};
                state.pData = baData.data();
                state.nSize = nHeaderSize;
                state.nCurrentOffset = 0;
                state.bIsError = false;
                state.sErrorString = QString();

                _handleId(&listRecords, XSevenZip::k7zIdHeader, &state, 1, true, nullptr, IMPTYPE_UNKNOWN);

                if (!state.baComment.isEmpty()) {
                    sResult = QString::fromUtf8(state.baComment);
                }
            }
        }
    }

    return sResult;
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

XBinary::HANDLE_METHOD XSevenZip::coderToCompressMethod(const QByteArray &baCodec)
{
    HANDLE_METHOD result = HANDLE_METHOD_UNKNOWN;

    if (baCodec.isEmpty()) {
        return result;
    }

    // Check 1-byte codecs first
    if (baCodec.size() == 1) {
        if (baCodec[0] == '\x00') {
            result = HANDLE_METHOD_STORE;  // Copy (uncompressed)
        } else if (baCodec[0] == '\x21') {
            result = HANDLE_METHOD_LZMA2;  // LZMA2
        } else if (baCodec[0] == '\x0A') {
            result = HANDLE_METHOD_ARM64_BCJ;  // ARM64 branch filter (1-byte codec ID)
        }
    } else if (baCodec.size() >= 3) {
        // 7-Zip codec IDs are typically 3+ bytes
        // Common codecs (from 7-Zip specification)
        if (baCodec.startsWith(QByteArray("\x00", 1))) {
            result = HANDLE_METHOD_STORE;  // Copy (uncompressed)
        } else if (baCodec.startsWith(QByteArray("\x03\x01\x01", 3))) {
            result = HANDLE_METHOD_LZMA;  // LZMA
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x08", 3))) {
            result = HANDLE_METHOD_DEFLATE;  // Deflate
        } else if (baCodec.startsWith(QByteArray("\x04\x01\x09", 3))) {
            result = HANDLE_METHOD_DEFLATE64;  // Deflate64
        } else if (baCodec.startsWith(QByteArray("\x04\x02\x02", 3))) {
            result = HANDLE_METHOD_BZIP2;  // BZip2
        } else if (baCodec.startsWith(QByteArray("\x03\x04\x01", 3))) {
            result = HANDLE_METHOD_PPMD7;  // PPMd (actual codec from 7z)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x03", 4))) {
            result = HANDLE_METHOD_BCJ;  // BCJ (x86 E8/E9 filter, single stream)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x1b", 4))) {
            result = HANDLE_METHOD_BCJ2;  // BCJ2 (x86 4-stream filter) - fix for issue 1
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x0A", 4))) {
            result = HANDLE_METHOD_ARM64_BCJ;  // ARM64 branch/call/jump filter (4-byte codec variant)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x01\x01", 4))) {
            result = HANDLE_METHOD_PPMD7;  // PPMd (alternative codec)
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x02\x05", 4))) {
            result = HANDLE_METHOD_UNKNOWN;  // SPARC BCJ filter (not BCJ2) - fix for issue 2
        } else if (baCodec.startsWith(QByteArray("\x06\xF1\x07\x01", 4))) {
            result = HANDLE_METHOD_7Z_AES;  // AES encryption
        } else {
#ifdef QT_DEBUG
            qDebug() << "[CODEC] Unknown codec:" << baCodec.toHex() << "size=" << baCodec.size();
#endif
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

QString XSevenZip::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XSevenZip_STRUCTID, sizeof(_TABLE_XSevenZip_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XSevenZip::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XSevenZip_STRUCTID, sizeof(_TABLE_XSevenZip_STRUCTID) / sizeof(XBinary::XCONVERT));
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

    // Add ID record
    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = puTag.nByteSize;
    record.varValue = puTag.nValue;
    record.srType = SRTYPE_ID;
    record.valType = VT_PACKEDNUMBER;
    record.sName = get_k7zId().value(puTag.nValue);
    pListRecords->append(record);

    pState->nCurrentOffset += puTag.nByteSize;

    // Process ID-specific data
    switch (id) {
        case XSevenZip::k7zIdHeader: {
            _handleId(pListRecords, XSevenZip::k7zIdMainStreamsInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdFilesInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            break;
        }

        case XSevenZip::k7zIdMainStreamsInfo: {
            _handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            // UnpackInfo is optional - clear error state if it fails
            bool bErrorBeforeUnpackInfo = pState->bIsError;
            _handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);

            // If UnpackInfo failed and it was optional, restore state
            if (pState->bIsError && !bErrorBeforeUnpackInfo) {
                pState->bIsError = false;
                pState->sErrorString.clear();
            }

            // SubStreamsInfo is optional - clear error state if it fails
            bool bErrorBeforeSubStreams = pState->bIsError;
            _handleId(pListRecords, XSevenZip::k7zIdSubStreamsInfo, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);

            // If SubStreamsInfo failed and it was optional, restore state
            if (pState->bIsError && !bErrorBeforeSubStreams) {
                pState->bIsError = false;
                pState->sErrorString.clear();
            }

            _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            // MainStreamsInfo doesn't have its own End marker - it's terminated by the next ID
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdPackInfo: {
            pState->nStreamsBegin = sizeof(SIGNATUREHEADER) + _handleNumber(pListRecords, pState, pPdStruct, "PackPosition", DRF_OFFSET, IMPTYPE_STREAMOFFSET);
            quint64 nNumberOfPackStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfPackStreams", DRF_COUNT, IMPTYPE_NUMBEROFPACKSTREAMS);

            for (int i = 0; i < nNumberOfPackStreams; i++) {
                SZINSTREAM szStream = {};
                pState->listInStreams.append(szStream);
            }

            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nNumberOfPackStreams, false, pPdStruct, IMPTYPE_STREAMSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, nNumberOfPackStreams, false, pPdStruct, IMPTYPE_STREAMCRC);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;
        }

        case XSevenZip::k7zIdUnpackInfo:
            _handleId(pListRecords, XSevenZip::k7zIdFolder, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            _handleId(pListRecords, XSevenZip::k7zIdCodersUnpackSize, pState, pState->nNumberOfCoders, false, pPdStruct, IMPTYPE_CODERUNPACKEDSIZE);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, pState->listOutStreams.count(), false, pPdStruct, IMPTYPE_STREAMUNPACKEDCRC);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
            break;

        case XSevenZip::k7zIdFolder: {
            quint64 nNumberOfFolders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFolders", DRF_COUNT, IMPTYPE_NUMBEROFFOLDERS);
            pState->nNumberOfFolders = nNumberOfFolders;  // Store for SubStreamsInfo
            pState->nNumberOfCoders = 0;

            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                // Loop through all folders
                for (quint64 iFolderIndex = 0; iFolderIndex < nNumberOfFolders && !pState->bIsError; iFolderIndex++) {
                    SZFOLDER szFolder = {};

                    qint32 nNumInStreamsTotal = 0;
                    qint32 nNumOutStreamsTotal = 0;
                    quint64 nNumberOfCoders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCoders", DRF_COUNT, IMPTYPE_NUMBEROFCODERS);

                    // Loop through all coders in this folder
                    for (quint64 iCoderIndex = 0; iCoderIndex < nNumberOfCoders && !pState->bIsError; iCoderIndex++) {
                        SZCODER coder = {};
                        coder.nNumOutStreams = 1;

                        quint8 nFlag = _handleByte(pListRecords, pState, pPdStruct, "Flag", IMPTYPE_UNKNOWN);

                        qint32 nCodecSize = nFlag & 0x0F;
                        coder.bIsComplex = (nFlag & 0x10) != 0;
                        bool bHasAttr = (nFlag & 0x20) != 0;

                        coder.baCoder = _handleArray(pListRecords, pState, nCodecSize, pPdStruct, "Coder", IMPTYPE_CODER);

                        if (coder.bIsComplex) {
                            // Complex coders have bind pairs and packed streams
                            // Read the number of input and output streams
                            coder.nNumInStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumInStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
                            _handleNumber(pListRecords, pState, pPdStruct, "NumOutStreams", DRF_COUNT, IMPTYPE_UNKNOWN);
                        } else {
                            coder.nNumInStreams = 1;
                        }

                        nNumInStreamsTotal += coder.nNumInStreams;
                        nNumOutStreamsTotal += coder.nNumOutStreams;

                        if (bHasAttr && !pState->bIsError) {
                            quint64 nPropertySize = _handleNumber(pListRecords, pState, pPdStruct, "PropertiesSize", DRF_SIZE, IMPTYPE_UNKNOWN);
                            coder.baProperty = _handleArray(pListRecords, pState, nPropertySize, pPdStruct, "Property", IMPTYPE_CODERPROPERTY);
                        }

                        szFolder.listCoders.append(coder);
                    }

                    pState->nNumberOfCoders += nNumberOfCoders;

                    if (nNumInStreamsTotal == 0) {
                        nNumInStreamsTotal = 1;
                    }

                    if (nNumOutStreamsTotal == 0) {
                        nNumOutStreamsTotal = 1;
                    }

                    qint32 nNumBindPairs = nNumberOfCoders - 1;

                    for (qint32 iBonds = 0; iBonds < nNumBindPairs; iBonds++) {
                        SZBOND szBond = {};
                        szBond.nInputIndex = _handleNumber(pListRecords, pState, pPdStruct, "InputIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);
                        szBond.nOutputIndex = _handleNumber(pListRecords, pState, pPdStruct, "OutputIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);

                        szFolder.listBonds.append(szBond);
                    }

                    qint32 nNumPackedStreams = nNumInStreamsTotal - nNumBindPairs;

                    if (nNumPackedStreams == 1) {
                        // If there's only one packed stream, it implicitly contains all unbound input streams
                        szFolder.listStreamIndexes.append(0);
                    } else {
                        for (qint32 iPacks = 0; iPacks < nNumPackedStreams; iPacks++) {
                            quint32 nStreamIndex = _handleNumber(pListRecords, pState, pPdStruct, "StreamIndex", DRF_UNKNOWN, IMPTYPE_UNKNOWN);
                            szFolder.listStreamIndexes.append(nStreamIndex);
                        }
                    }

                    pState->listFolders.append(szFolder);

                    for (qint32 i = 0; i < nNumPackedStreams; i++) {
                        SZOUTSTREAM szOutStream = {};
                        pState->listOutStreams.append(szOutStream);
                    }
                }
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("Data Stream Index"), DRF_COUNT, IMPTYPE_UNKNOWN);
            } else {
                pState->bIsError = true;
                pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
            }

            bResult = true;
            break;
        }

        case XSevenZip::k7zIdSubStreamsInfo:
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

            for (quint64 i = 0; i < pState->nNumberOfFolders && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nNumStreamsInFolder =
                    _handleNumber(pListRecords, pState, pPdStruct, QString("NumUnpackStream%1").arg(i), DRF_COUNT, IMPTYPE_NUMBEROFUNPACKSTREAM);
                pState->listNumUnpackedStreams.append(nNumStreamsInFolder);
                nTotalSubStreams += nNumStreamsInFolder;
            }

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
                    if (i < pState->listInStreams.count()) {
                        pState->listInStreams[i].nOffset = nCurrentOffset;
                        pState->listInStreams[i].nSize = nSize;
                    }
                } else if (impType == IMPTYPE_FILEUNPACKEDSIZE) {
                    pState->listFileSizes.append((qint64)nSize);
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

                pState->listCodersSizes.append(nSize);  // Store unpacked size for each coder (for debugging)

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
                        if (i < pState->listInStreams.count()) {
                            pState->listInStreams[i].nCRC = nCRC;
                        }
                    } else if (impType == IMPTYPE_STREAMUNPACKEDCRC) {
                        if (i < pState->listOutStreams.count()) {
                            pState->listOutStreams[i].nCRC = nCRC;
                        }
                    } else if (impType == IMPTYPE_FILECRC) {
                        pState->listFileCRC.append(nCRC);
                    }
                }
            } else {
                // Bitmask format - read bitmask then CRCs for set bits
                // For now, skip this case as it's more complex
            }
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdFilesInfo: {
            quint64 nNumberOfFiles = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFiles", DRF_COUNT, IMPTYPE_NUMBEROFFILES);

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
                    // Unknown property - skip the ID byte, then read and skip the size field and data
                    qint64 nPropertyStartOffset = pState->nCurrentOffset;
                    pState->nCurrentOffset++;  // Skip the ID byte

                    // Try to read the size field (most properties have this)
                    if (pState->nCurrentOffset < pState->nSize) {
                        XBinary::PACKED_UINT puSize = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
                        if (puSize.bIsValid) {
                            pState->nCurrentOffset += puSize.nByteSize;
                            quint64 nDataSize = puSize.nValue;
                            // Skip the data
                            if (pState->nCurrentOffset + (qint64)nDataSize <= pState->nSize) {
                                pState->nCurrentOffset += nDataSize;
                                bHandled = true;  // Successfully skipped
                            } else {
                                bFoundEnd = true;
                                break;
                            }
                        } else {
                            // If we can't read a valid size, treat this as the end
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
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("NameSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

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
                            record.nSize = nNameLenBytes + 2;  // NUll
                            record.varValue = sFilename;
                            record.srType = SRTYPE_ARRAY;
                            record.valType = VT_STRING;
                            record.impType = IMPTYPE_FILENAME;
                            record.sName = QString("FileName[%1]").arg(nFileIndex);
                            pListRecords->append(record);

                            pState->listFileNames.append(sFilename);  // Store filename in state for later use

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
            pState->baEmptyStreams = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyStreamData"), IMPTYPE_EMPTYSTREAMDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdEmptyFile: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("EmptyFileSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baEmptyFiles = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("EmptyFileData"), IMPTYPE_EMPTYFILEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdAnti: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("AntiSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baAnti = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("AntiData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdCTime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CTimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baCTime = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("CTimeData"), IMPTYPE_CTIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdATime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("ATimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baATime = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("ATimeData"), IMPTYPE_ATIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdMTime: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("MTimeSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baMTime = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("MTimeData"), IMPTYPE_MTIMEDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdWinAttrib: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("WinAttribSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baWinAttrib = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("WinAttribData"), IMPTYPE_WINATTRIBDATA);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdComment: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("CommentSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baComment = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("CommentData"), IMPTYPE_UNKNOWN);
            bResult = true;
            break;
        }

        case XSevenZip::k7zIdStartPos: {
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("StartPosSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            pState->baStartPos = _handleArray(pListRecords, pState, nSize, pPdStruct, QString("StartPosData"), IMPTYPE_UNKNOWN);
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

bool XSevenZip::_decode7zTimeValue(const QByteArray &baData, qint32 nNumFiles, qint32 nFileIndex, quint64 *pResult)
{
    if (baData.isEmpty() || nFileIndex < 0 || nFileIndex >= nNumFiles || pResult == nullptr) {
        return false;
    }

    const char *pData = baData.constData();
    qint32 nLen = baData.size();

    // Byte 0: AllAreDefined
    bool bAllDefined = ((quint8)pData[0] != 0);
    qint32 nBitmapBytes = bAllDefined ? 0 : ((nNumFiles + 7) / 8);

    // Byte 1 [+ optional bitmap]: External flag (0 = inline values follow)
    qint32 nExternalByteOffset = 1 + nBitmapBytes;
    if (nExternalByteOffset >= nLen) {
        return false;
    }
    bool bExternal = ((quint8)pData[nExternalByteOffset] != 0);
    if (bExternal) {
        return false;  // Values are in an external stream, cannot decode inline
    }
    qint32 nValuesOffset = nExternalByteOffset + 1;

    bool bDefined = false;
    qint32 nDefinedBefore = 0;

    if (bAllDefined) {
        bDefined = true;
        nDefinedBefore = nFileIndex;
    } else {
        const char *pBitmap = pData + 1;
        bDefined = XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, nFileIndex);

        for (qint32 k = 0; k < nFileIndex; k++) {
            if (XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, k)) {
                nDefinedBefore++;
            }
        }
    }

    if (!bDefined) {
        return false;
    }

    qint32 nOffset = nValuesOffset + nDefinedBefore * 8;
    if (nOffset + 8 > nLen) {
        return false;
    }

    quint64 nValue = 0;
    for (qint32 b = 0; b < 8; b++) {
        nValue |= ((quint64)(quint8)pData[nOffset + b]) << (b * 8);
    }

    *pResult = nValue;
    return true;
}

bool XSevenZip::_decode7zAttribValue(const QByteArray &baData, qint32 nNumFiles, qint32 nFileIndex, quint32 *pResult)
{
    if (baData.isEmpty() || nFileIndex < 0 || nFileIndex >= nNumFiles || pResult == nullptr) {
        return false;
    }

    const char *pData = baData.constData();
    qint32 nLen = baData.size();

    // Byte 0: AllAreDefined
    bool bAllDefined = ((quint8)pData[0] != 0);
    qint32 nBitmapBytes = bAllDefined ? 0 : ((nNumFiles + 7) / 8);

    // Byte 1 [+ optional bitmap]: External flag (0 = inline values follow)
    qint32 nExternalByteOffset = 1 + nBitmapBytes;
    if (nExternalByteOffset >= nLen) {
        return false;
    }
    bool bExternal = ((quint8)pData[nExternalByteOffset] != 0);
    if (bExternal) {
        return false;  // Values are in an external stream, cannot decode inline
    }
    qint32 nValuesOffset = nExternalByteOffset + 1;

    bool bDefined = false;
    qint32 nDefinedBefore = 0;

    if (bAllDefined) {
        bDefined = true;
        nDefinedBefore = nFileIndex;
    } else {
        const char *pBitmap = pData + 1;
        bDefined = XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, nFileIndex);

        for (qint32 k = 0; k < nFileIndex; k++) {
            if (XBinary::_read_bool_safe_rev(const_cast<char *>(pBitmap), nBitmapBytes, k)) {
                nDefinedBefore++;
            }
        }
    }

    if (!bDefined) {
        return false;
    }

    qint32 nOffset = nValuesOffset + nDefinedBefore * 4;
    if (nOffset + 4 > nLen) {
        return false;
    }

    *pResult = (quint32)(quint8)pData[nOffset] | ((quint32)(quint8)pData[nOffset + 1] << 8) | ((quint32)(quint8)pData[nOffset + 2] << 16) |
               ((quint32)(quint8)pData[nOffset + 3] << 24);

    return true;
}

bool XSevenZip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QString sMD5;

    if (pState) {
        // Create context
        SEVENZ_UNPACK_CONTEXT *pContext = new SEVENZ_UNPACK_CONTEXT;
        // pContext->nSignatureSize = sizeof(SIGNATUREHEADER);

        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;
        pState->mapUnpackProperties = mapProperties;  // Store unpack properties (including password) for file extraction

        // Parse archive structure directly using streaming approach
        SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
        qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
        qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

        if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
            QByteArray baData;
            baData.resize(nNextHeaderSize);
            // char *pData = new char[nNextHeaderSize];
            // char *pUnpackedData = nullptr;
            // char *pHeaderData = nullptr;
            qint64 nHeaderSize = nNextHeaderSize;

            qint64 nBytesRead = read_array_process(nNextHeaderOffset, baData.data(), nNextHeaderSize, pPdStruct);

            if (nBytesRead == nNextHeaderSize) {
                bool bHeader = false;
                bool bIsEncodedHeader = false;
                if (nBytesRead > 0) {
                    quint8 nFirstByte = (quint8)baData.data()[0];
                    bIsEncodedHeader = (nFirstByte == (quint8)k7zIdEncodedHeader);
                }

                if (bIsEncodedHeader) {
                    QList<XSevenZip::SZRECORD> listRecords;

                    SZSTATE state = {};
                    state.pData = baData.data();
                    state.nSize = nNextHeaderSize;
                    state.nCurrentOffset = 0;
                    state.bIsError = false;
                    state.sErrorString = QString();

                    _handleId(&listRecords, XSevenZip::k7zIdEncodedHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

                    baData.clear();

                    QBuffer bufferOut;
                    bufferOut.setBuffer(&baData);

                    if (bufferOut.open(QIODevice::ReadWrite)) {
                        bHeader = decompressHeader(mapProperties, &bufferOut, &state, pPdStruct);
                        bufferOut.close();
                        // Update nHeaderSize to actual decompressed size
                        nHeaderSize = baData.size();
                    }
                } else {
                    bHeader = true;
                }

                if (bHeader) {
                    QList<XSevenZip::SZRECORD> listRecords;

                    SZSTATE state = {};
                    state.pData = baData.data();
                    state.nSize = nHeaderSize;
                    state.nCurrentOffset = 0;
                    state.bIsError = false;
                    state.sErrorString = QString();

                    _handleId(&listRecords, XSevenZip::k7zIdHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                    // _printRecords(&listRecords);

                    qint32 nNumberOfFiles = state.listFileNames.count();
                    qint32 nNumberOfFolders = state.listFolders.count();
                    qint32 nNumberOfEmptyStreams = XBinary::_getBitCount_safe(state.baEmptyStreams.data(), state.baEmptyStreams.size());
                    bool bEmptyFilesPresent = (state.baEmptyFiles.size() > 0);

                    qint64 nCurrentCompressedOffset = 0;
                    Q_UNUSED(nCurrentCompressedOffset)
                    qint64 nCurrentUncompressedOffset = 0;
                    qint32 nCurrentFolder = 0;

                    // Build per-folder file counts
                    QList<qint32> listFolderFileCounts;
                    for (qint32 nFi = 0; nFi < nNumberOfFolders; nFi++) {
                        qint32 nFolderFileCount = 1;
                        if (nFi < state.listNumUnpackedStreams.count()) {
                            nFolderFileCount = (qint32)state.listNumUnpackedStreams.at(nFi);
                        }
                        listFolderFileCounts.append(nFolderFileCount);
                    }

                    // Build per-folder global pack-stream start index.
                    // listStreamIndexes stores relative indices within each folder's pack-stream
                    // allocation; folders consume pack streams sequentially.
                    QList<qint32> listFolderStreamOffset;
                    {
                        qint32 nRunningStreamOffset = 0;
                        for (qint32 nFi = 0; nFi < nNumberOfFolders; nFi++) {
                            listFolderStreamOffset.append(nRunningStreamOffset);
                            if (nFi < (qint32)state.listFolders.count()) {
                                nRunningStreamOffset += state.listFolders.at(nFi).listStreamIndexes.count();
                            } else {
                                nRunningStreamOffset++;
                            }
                        }
                    }

                    qint32 nFileSizeIndex = 0;
                    qint32 nFileCRCIndex = 0;
                    qint32 nFileIndexInCurrentFolder = 0;
                    sMD5 = XBinary::getHash(XBinary::HASH_MD5, getDevice(), pPdStruct);

                    for (qint32 nCurrentFileIndex = 0; nCurrentFileIndex < nNumberOfFiles; nCurrentFileIndex++) {
                        ARCHIVERECORD record = {};
                        record.mapProperties.insert(FPART_PROP_FILEMD5, sMD5);

                        // Determine if this file has no data stream (empty dir or empty file)
                        bool bCurrentFileIsEmpty = false;
                        if (state.baEmptyStreams.size() > 0) {
                            bCurrentFileIsEmpty = XBinary::_read_bool_safe_rev(state.baEmptyStreams.data(), state.baEmptyStreams.size(), nCurrentFileIndex);
                        }

                        if (bCurrentFileIsEmpty) {
                            bool bIsFile = false;
                            if (bEmptyFilesPresent) {
                                if (XBinary::_read_bool_safe_rev(state.baEmptyFiles.data(), state.baEmptyFiles.size(), nCurrentFileIndex)) {
                                    bIsFile = true;
                                }
                            }

                            record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)HANDLE_METHOD_STORE);
                            record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0);
                            record.mapProperties.insert(FPART_PROP_ISFOLDER, !bIsFile);
                            record.mapProperties.insert(FPART_PROP_ISSOLID, false);
                        } else if (nCurrentFolder < nNumberOfFolders) {
                            // This file has a data stream; map it to the current folder
                            qint32 nFilesInFolder = (nCurrentFolder < listFolderFileCounts.count()) ? listFolderFileCounts.at(nCurrentFolder) : 1;
                            bool bIsSolid = (nFilesInFolder > 1);

                            // Resolve pack stream index for this folder using the global offset
                            qint32 nFolderStreamBase = (nCurrentFolder < listFolderStreamOffset.count()) ? listFolderStreamOffset.at(nCurrentFolder) : nCurrentFolder;
                            qint32 nRelativeStreamIndex = 0;
                            if (nCurrentFolder < (qint32)state.listFolders.count() && !state.listFolders.at(nCurrentFolder).listStreamIndexes.isEmpty()) {
                                nRelativeStreamIndex = state.listFolders.at(nCurrentFolder).listStreamIndexes.at(0);
                            }
                            qint32 nStreamListIndex = nFolderStreamBase + nRelativeStreamIndex;

                            qint64 nStreamOffset = 0;
                            qint64 nStreamSize = 0;
                            if (nStreamListIndex < (qint32)state.listInStreams.count()) {
                                nStreamOffset = state.nStreamsBegin + state.listInStreams.at(nStreamListIndex).nOffset;
                                nStreamSize = state.listInStreams.at(nStreamListIndex).nSize;
                            }

                            // Compute this folder's total decompressed size (last coder's output)
                            qint32 nCoderSizesOffset = 0;
                            for (qint32 nFi = 0; nFi < nCurrentFolder; nFi++) {
                                if (nFi < (qint32)state.listFolders.count()) {
                                    nCoderSizesOffset += state.listFolders.at(nFi).listCoders.count();
                                } else {
                                    nCoderSizesOffset++;
                                }
                            }
                            qint32 nFolderCoderCount = 1;
                            if (nCurrentFolder < (qint32)state.listFolders.count()) {
                                nFolderCoderCount = state.listFolders.at(nCurrentFolder).listCoders.count();
                            }
                            qint64 nFolderDecompressedSize = 0;
                            if ((nCoderSizesOffset + nFolderCoderCount - 1) < (qint32)state.listCodersSizes.count()) {
                                nFolderDecompressedSize = (qint64)state.listCodersSizes.at(nCoderSizesOffset + nFolderCoderCount - 1);
                            }

                            // Resolve compression method and properties from this folder's coders
                            HANDLE_METHOD cm = HANDLE_METHOD_STORE;
                            QByteArray baCoderProperty;
                            HANDLE_METHOD cm2 = HANDLE_METHOD_STORE;
                            QByteArray baCoderProperty2;
                            HANDLE_METHOD cm3 = HANDLE_METHOD_STORE;
                            QByteArray baCoderProperty3;
                            bool bHasSecondCoder = false;
                            bool bHasThirdCoder = false;
                            qint32 nSecondCoderSizeIdx = -1;  // index into listCodersSizes for cm2's output size
                            bool bBCJ2Resolved = false;
                            qint64 nBCJ2MainOffset = 0;
                            qint64 nBCJ2MainSize = 0;
                            HANDLE_METHOD cmBCJ2Main = HANDLE_METHOD_STORE;
                            QByteArray baBCJ2MainProp;
                            qint64 nBCJ2MainUnpack = 0;
                            qint64 nBCJ2CallOffset = 0;
                            qint64 nBCJ2CallSize = 0;
                            HANDLE_METHOD cmBCJ2Call = HANDLE_METHOD_STORE;
                            QByteArray baBCJ2CallProp;
                            qint64 nBCJ2CallUnpack = 0;
                            qint64 nBCJ2JmpOffset = 0;
                            qint64 nBCJ2JmpSize = 0;
                            HANDLE_METHOD cmBCJ2Jmp = HANDLE_METHOD_STORE;
                            QByteArray baBCJ2JmpProp;
                            qint64 nBCJ2JmpUnpack = 0;
                            qint64 nBCJ2RangeOffset = 0;
                            qint64 nBCJ2RangeSize = 0;
                            qint64 nBCJ2OutputSize = 0;
                            // Per-stream AES properties for AES-encrypted BCJ2 archives
                            QByteArray baBCJ2MainAESProp;
                            qint64 nBCJ2MainAESUnpack = 0;
                            QByteArray baBCJ2CallAESProp;
                            qint64 nBCJ2CallAESUnpack = 0;
                            QByteArray baBCJ2JmpAESProp;
                            qint64 nBCJ2JmpAESUnpack = 0;
                            QByteArray baBCJ2RangeAESProp;
                            qint64 nBCJ2RangeAESUnpack = 0;
                            if (nCurrentFolder < (qint32)state.listFolders.count()) {
                                const SZFOLDER &folder = state.listFolders.at(nCurrentFolder);
                                qint32 nNumCoders = folder.listCoders.count();
                                if (nNumCoders >= 1) {
                                    qint32 nLastCoder = nNumCoders - 1;
                                    cm = coderToCompressMethod(folder.listCoders.at(nLastCoder).baCoder);
                                    baCoderProperty = folder.listCoders.at(nLastCoder).baProperty;
                                    // BCJ2 is a 4-stream filter listed as coder[0]; scan for it and
                                    // override cm so the BCJ2 path in unpackCurrent fires correctly.
                                    for (qint32 nCi = 0; nCi < nNumCoders; nCi++) {
                                        if (coderToCompressMethod(folder.listCoders.at(nCi).baCoder) == HANDLE_METHOD_BCJ2) {
                                            cm = HANDLE_METHOD_BCJ2;
                                            break;
                                        }
                                    }
                                    if (nNumCoders >= 2 && cm != HANDLE_METHOD_BCJ2) {
                                        // For N coders, decompression order is coder[N-1] → coder[N-2] → ... → coder[0]
                                        // cm  = coder[N-1] (innermost, applied last — e.g. BCJ filter)
                                        // cm2 = coder[N-2] (middle — e.g. LZMA2)
                                        // cm3 = coder[N-3] / coder[0] (outermost, applied first — e.g. AES decrypt)
                                        cm2 = coderToCompressMethod(folder.listCoders.at(nLastCoder - 1).baCoder);
                                        baCoderProperty2 = folder.listCoders.at(nLastCoder - 1).baProperty;
                                        bHasSecondCoder = true;
                                        nSecondCoderSizeIdx = nCoderSizesOffset + (nLastCoder - 1);
                                    }
                                    if (nNumCoders >= 3 && cm != HANDLE_METHOD_BCJ2) {
                                        cm3 = coderToCompressMethod(folder.listCoders.at(0).baCoder);
                                        baCoderProperty3 = folder.listCoders.at(0).baProperty;
                                        bHasThirdCoder = true;
                                    }
                                    if (cm == HANDLE_METHOD_BCJ2) {
                                        // Resolve the 4 BCJ2 stream coordinates at parse time so that
                                        // XDecompress::decompress() can handle BCJ2 as a normal single-method.
                                        // Two layouts are supported:
                                        //   Classic: 4 coders (BCJ2 + LZMA2 + LZMA + LZMA), 3 bonds, 4 pack streams
                                        //   Compact: 2 coders (BCJ2 + LZMA2), 1 bond, 4 pack streams
                                        //            calls/jumps streams stored raw (STORE) in pack data
                                        qint32 nLocalPackCount = folder.listStreamIndexes.count();
                                        qint32 nLocalBondCount = folder.listBonds.count();
                                        if (nLocalPackCount >= 4) {
                                            // Map folder InStream index → global pack-stream index
                                            QMap<qint32, qint32> mapInStreamToGlobal;
                                            for (qint32 k = 0; k < nLocalPackCount; k++) {
                                                qint32 nInStreamIdx = folder.listStreamIndexes.at(k);
                                                mapInStreamToGlobal[nInStreamIdx] = nFolderStreamBase + k;
                                            }
                                            // Cumulative InStream offsets per coder within this folder
                                            QList<qint32> listInStreamOffsets;
                                            qint32 nRunningInStream = 0;
                                            for (qint32 ci = 0; ci < nNumCoders; ci++) {
                                                listInStreamOffsets.append(nRunningInStream);
                                                nRunningInStream += folder.listCoders.at(ci).nNumInStreams;
                                            }
                                            // Find BCJ2 coder local index
                                            qint32 nBCJ2LocalIdx = -1;
                                            for (qint32 ci = 0; ci < nNumCoders; ci++) {
                                                if (coderToCompressMethod(folder.listCoders.at(ci).baCoder) == HANDLE_METHOD_BCJ2) {
                                                    nBCJ2LocalIdx = ci;
                                                    break;
                                                }
                                            }
                                            if (nBCJ2LocalIdx >= 0) {
                                                qint32 nBCJ2InStreamBase = listInStreamOffsets.at(nBCJ2LocalIdx);

                                                // Build bond map: in-stream index → source coder index
                                                QMap<qint32, qint32> mapBondInToCoderOut;
                                                for (qint32 bi = 0; bi < nLocalBondCount; bi++) {
                                                    mapBondInToCoderOut[folder.listBonds.at(bi).nInputIndex] = folder.listBonds.at(bi).nOutputIndex;
                                                }

                                                // Resolve an in-stream to its global pack-stream index.
                                                // Tries direct lookup first; if not found, follows one bond to handle
                                                // AES-encrypted sub-streams (encrypted BCJ2 layout).
                                                // If resolved via a bond, *pAESCoderIdx is set to that coder's local index.
                                                auto resolveInStream = [&](qint32 nInStream, qint32 *pAESCoderIdx) -> qint32 {
                                                    if (pAESCoderIdx) *pAESCoderIdx = -1;
                                                    if (mapInStreamToGlobal.contains(nInStream)) {
                                                        return mapInStreamToGlobal.value(nInStream);
                                                    }
                                                    if (mapBondInToCoderOut.contains(nInStream)) {
                                                        qint32 nProducerCoder = mapBondInToCoderOut.value(nInStream);
                                                        if (nProducerCoder >= 0 && nProducerCoder < nNumCoders) {
                                                            qint32 nProducerInStream = listInStreamOffsets.at(nProducerCoder);
                                                            if (mapInStreamToGlobal.contains(nProducerInStream)) {
                                                                if (pAESCoderIdx) *pAESCoderIdx = nProducerCoder;
                                                                return mapInStreamToGlobal.value(nProducerInStream);
                                                            }
                                                        }
                                                    }
                                                    return -1;
                                                };

                                                // Resolve range stream (BCJ2.in[3])
                                                qint32 nRangeAESCoderIdx = -1;
                                                qint32 nRangeGlobal = resolveInStream(nBCJ2InStreamBase + 3, &nRangeAESCoderIdx);

                                                if (nRangeGlobal >= 0 && nRangeGlobal < state.listInStreams.count()) {
                                                    nBCJ2RangeOffset = state.nStreamsBegin + state.listInStreams.at(nRangeGlobal).nOffset;
                                                    nBCJ2RangeSize = state.listInStreams.at(nRangeGlobal).nSize;
                                                    if (nRangeAESCoderIdx >= 0) {
                                                        baBCJ2RangeAESProp = folder.listCoders.at(nRangeAESCoderIdx).baProperty;
                                                        nBCJ2RangeAESUnpack = (nCoderSizesOffset + nRangeAESCoderIdx < state.listCodersSizes.count())
                                                                                  ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nRangeAESCoderIdx) : 0;
                                                    }

                                                    // Scan bonds for BCJ2 inputs 0=main, 1=calls, 2=jumps
                                                    qint32 nMainLZMALocal = -1;
                                                    qint32 nCallLZMALocal = -1;
                                                    qint32 nJmpLZMALocal = -1;
                                                    for (qint32 bi = 0; bi < nLocalBondCount; bi++) {
                                                        qint32 nInIdx = folder.listBonds.at(bi).nInputIndex;
                                                        qint32 nOutIdx = folder.listBonds.at(bi).nOutputIndex;
                                                        qint32 nBCJ2LocalInput = nInIdx - nBCJ2InStreamBase;
                                                        if (nBCJ2LocalInput == 0) {
                                                            nMainLZMALocal = nOutIdx;
                                                        } else if (nBCJ2LocalInput == 1) {
                                                            nCallLZMALocal = nOutIdx;
                                                        } else if (nBCJ2LocalInput == 2) {
                                                            nJmpLZMALocal = nOutIdx;
                                                        }
                                                    }

                                                    if (nMainLZMALocal >= 0) {
                                                        qint32 nMainAESCoderIdx = -1;
                                                        qint32 nMainGlobal = resolveInStream(listInStreamOffsets.at(nMainLZMALocal), &nMainAESCoderIdx);
                                                        if (nMainGlobal >= 0 && nMainGlobal < state.listInStreams.count()) {
                                                            nBCJ2MainOffset = state.nStreamsBegin + state.listInStreams.at(nMainGlobal).nOffset;
                                                            nBCJ2MainSize = state.listInStreams.at(nMainGlobal).nSize;
                                                            cmBCJ2Main = coderToCompressMethod(folder.listCoders.at(nMainLZMALocal).baCoder);
                                                            baBCJ2MainProp = folder.listCoders.at(nMainLZMALocal).baProperty;
                                                            nBCJ2MainUnpack = (nCoderSizesOffset + nMainLZMALocal < state.listCodersSizes.count())
                                                                                  ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nMainLZMALocal) : 0;
                                                            nBCJ2OutputSize = (nCoderSizesOffset + nBCJ2LocalIdx < state.listCodersSizes.count())
                                                                                  ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nBCJ2LocalIdx) : 0;
                                                            if (nMainAESCoderIdx >= 0) {
                                                                baBCJ2MainAESProp = folder.listCoders.at(nMainAESCoderIdx).baProperty;
                                                                nBCJ2MainAESUnpack = (nCoderSizesOffset + nMainAESCoderIdx < state.listCodersSizes.count())
                                                                                         ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nMainAESCoderIdx) : 0;
                                                            }

                                                            // Resolve calls stream
                                                            bool bCallOk = false;
                                                            if (nCallLZMALocal >= 0) {
                                                                qint32 nCallAESCoderIdx = -1;
                                                                qint32 nCallGlobal = resolveInStream(listInStreamOffsets.at(nCallLZMALocal), &nCallAESCoderIdx);
                                                                if (nCallGlobal >= 0 && nCallGlobal < state.listInStreams.count()) {
                                                                    nBCJ2CallOffset = state.nStreamsBegin + state.listInStreams.at(nCallGlobal).nOffset;
                                                                    nBCJ2CallSize = state.listInStreams.at(nCallGlobal).nSize;
                                                                    cmBCJ2Call = coderToCompressMethod(folder.listCoders.at(nCallLZMALocal).baCoder);
                                                                    baBCJ2CallProp = folder.listCoders.at(nCallLZMALocal).baProperty;
                                                                    nBCJ2CallUnpack = (nCoderSizesOffset + nCallLZMALocal < state.listCodersSizes.count())
                                                                                          ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nCallLZMALocal) : 0;
                                                                    if (nCallAESCoderIdx >= 0) {
                                                                        baBCJ2CallAESProp = folder.listCoders.at(nCallAESCoderIdx).baProperty;
                                                                        nBCJ2CallAESUnpack = (nCoderSizesOffset + nCallAESCoderIdx < state.listCodersSizes.count())
                                                                                                 ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nCallAESCoderIdx) : 0;
                                                                    }
                                                                    bCallOk = true;
                                                                }
                                                            } else {
                                                                // Compact layout: BCJ2.in[1] (calls) is a raw/encrypted direct stream
                                                                qint32 nCallAESCoderIdx = -1;
                                                                qint32 nCallGlobal = resolveInStream(nBCJ2InStreamBase + 1, &nCallAESCoderIdx);
                                                                if (nCallGlobal >= 0 && nCallGlobal < state.listInStreams.count()) {
                                                                    nBCJ2CallOffset = state.nStreamsBegin + state.listInStreams.at(nCallGlobal).nOffset;
                                                                    nBCJ2CallSize = state.listInStreams.at(nCallGlobal).nSize;
                                                                    cmBCJ2Call = HANDLE_METHOD_STORE;
                                                                    nBCJ2CallUnpack = nBCJ2CallSize;
                                                                    if (nCallAESCoderIdx >= 0) {
                                                                        baBCJ2CallAESProp = folder.listCoders.at(nCallAESCoderIdx).baProperty;
                                                                        nBCJ2CallAESUnpack = (nCoderSizesOffset + nCallAESCoderIdx < state.listCodersSizes.count())
                                                                                                 ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nCallAESCoderIdx) : 0;
                                                                    }
                                                                    bCallOk = true;
                                                                }
                                                            }

                                                            // Resolve jumps stream
                                                            bool bJmpOk = false;
                                                            if (nJmpLZMALocal >= 0) {
                                                                qint32 nJmpAESCoderIdx = -1;
                                                                qint32 nJmpGlobal = resolveInStream(listInStreamOffsets.at(nJmpLZMALocal), &nJmpAESCoderIdx);
                                                                if (nJmpGlobal >= 0 && nJmpGlobal < state.listInStreams.count()) {
                                                                    nBCJ2JmpOffset = state.nStreamsBegin + state.listInStreams.at(nJmpGlobal).nOffset;
                                                                    nBCJ2JmpSize = state.listInStreams.at(nJmpGlobal).nSize;
                                                                    cmBCJ2Jmp = coderToCompressMethod(folder.listCoders.at(nJmpLZMALocal).baCoder);
                                                                    baBCJ2JmpProp = folder.listCoders.at(nJmpLZMALocal).baProperty;
                                                                    nBCJ2JmpUnpack = (nCoderSizesOffset + nJmpLZMALocal < state.listCodersSizes.count())
                                                                                         ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nJmpLZMALocal) : 0;
                                                                    if (nJmpAESCoderIdx >= 0) {
                                                                        baBCJ2JmpAESProp = folder.listCoders.at(nJmpAESCoderIdx).baProperty;
                                                                        nBCJ2JmpAESUnpack = (nCoderSizesOffset + nJmpAESCoderIdx < state.listCodersSizes.count())
                                                                                                ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nJmpAESCoderIdx) : 0;
                                                                    }
                                                                    bJmpOk = true;
                                                                }
                                                            } else {
                                                                // Compact layout: BCJ2.in[2] (jumps) is a raw/encrypted direct stream
                                                                qint32 nJmpAESCoderIdx = -1;
                                                                qint32 nJmpGlobal = resolveInStream(nBCJ2InStreamBase + 2, &nJmpAESCoderIdx);
                                                                if (nJmpGlobal >= 0 && nJmpGlobal < state.listInStreams.count()) {
                                                                    nBCJ2JmpOffset = state.nStreamsBegin + state.listInStreams.at(nJmpGlobal).nOffset;
                                                                    nBCJ2JmpSize = state.listInStreams.at(nJmpGlobal).nSize;
                                                                    cmBCJ2Jmp = HANDLE_METHOD_STORE;
                                                                    nBCJ2JmpUnpack = nBCJ2JmpSize;
                                                                    if (nJmpAESCoderIdx >= 0) {
                                                                        baBCJ2JmpAESProp = folder.listCoders.at(nJmpAESCoderIdx).baProperty;
                                                                        nBCJ2JmpAESUnpack = (nCoderSizesOffset + nJmpAESCoderIdx < state.listCodersSizes.count())
                                                                                                ? (qint64)state.listCodersSizes.at(nCoderSizesOffset + nJmpAESCoderIdx) : 0;
                                                                    }
                                                                    bJmpOk = true;
                                                                }
                                                            }

                                                            if (bCallOk && bJmpOk) {
                                                                bBCJ2Resolved = true;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            // Determine this file's uncompressed size
                            qint64 nFileSize = 0;
                            if (bIsSolid && (nFileIndexInCurrentFolder < (nFilesInFolder - 1))) {
                                // Not the last file in this solid folder; use explicit size
                                if (nFileSizeIndex < (qint32)state.listFileSizes.count()) {
                                    nFileSize = state.listFileSizes.at(nFileSizeIndex);
                                }
                                nFileSizeIndex++;
                            } else {
                                // Last (or only) file: remainder of decompressed block
                                nFileSize = nFolderDecompressedSize - nCurrentUncompressedOffset;
                            }

                            // Get CRC for this file:
                            // If SubStreamsInfo provided per-file CRCs (solid/multi-file folders), use listFileCRC.
                            // Otherwise fall back to the per-folder CRC stored in listOutStreams.
                            quint32 nFileCRC = 0;
                            if (nFileCRCIndex < (qint32)state.listFileCRC.count()) {
                                nFileCRC = state.listFileCRC.at(nFileCRCIndex);
                                nFileCRCIndex++;
                            } else if (nCurrentFolder < (qint32)state.listOutStreams.count()) {
                                nFileCRC = state.listOutStreams.at(nCurrentFolder).nCRC;
                            }

                            record.nStreamOffset = bBCJ2Resolved ? nBCJ2MainOffset : nStreamOffset;
                            record.nStreamSize = bBCJ2Resolved ? nBCJ2MainSize : nStreamSize;
                            record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)cm);
                            record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, bBCJ2Resolved ? baBCJ2MainProp : baCoderProperty);
                            if (bHasSecondCoder) {
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)cm2);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES2, baCoderProperty2);
                                // AES output size = coder[N-2] output = intermediate size needed for proper truncation
                                if (nSecondCoderSizeIdx >= 0 && nSecondCoderSizeIdx < (qint32)state.listCodersSizes.count()) {
                                    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE2, (qint64)state.listCodersSizes.at(nSecondCoderSizeIdx));
                                }
                            }
                            if (bHasThirdCoder) {
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD3, (quint32)cm3);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES3, baCoderProperty3);
                                // Outermost coder (coder[0]) output size
                                if (nCoderSizesOffset < (qint32)state.listCodersSizes.count()) {
                                    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE3, (qint64)state.listCodersSizes.at(nCoderSizesOffset));
                                }
                            }
                            if (bBCJ2Resolved) {
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD4, (quint32)cmBCJ2Main);
                                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE4, nBCJ2MainUnpack);
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)cmBCJ2Call);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES2, baBCJ2CallProp);
                                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE2, nBCJ2CallUnpack);
                                record.mapProperties.insert(FPART_PROP_STREAMOFFSET2, nBCJ2CallOffset);
                                record.mapProperties.insert(FPART_PROP_STREAMSIZE2, nBCJ2CallSize);
                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD3, (quint32)cmBCJ2Jmp);
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES3, baBCJ2JmpProp);
                                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE3, nBCJ2JmpUnpack);
                                record.mapProperties.insert(FPART_PROP_STREAMOFFSET3, nBCJ2JmpOffset);
                                record.mapProperties.insert(FPART_PROP_STREAMSIZE3, nBCJ2JmpSize);
                                record.mapProperties.insert(FPART_PROP_STREAMOFFSET4, nBCJ2RangeOffset);
                                record.mapProperties.insert(FPART_PROP_STREAMSIZE4, nBCJ2RangeSize);
                                // AES encryption properties for BCJ2 sub-streams (encrypted BCJ2 layout)
                                if (!baBCJ2MainAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_0, baBCJ2MainAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_0, nBCJ2MainAESUnpack);
                                }
                                if (!baBCJ2CallAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_1, baBCJ2CallAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_1, nBCJ2CallAESUnpack);
                                }
                                if (!baBCJ2JmpAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_2, baBCJ2JmpAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_2, nBCJ2JmpAESUnpack);
                                }
                                if (!baBCJ2RangeAESProp.isEmpty()) {
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_PROPS_3, baBCJ2RangeAESProp);
                                    record.mapProperties.insert(FPART_PROP_BCJ2_AES_UNPACK_3, nBCJ2RangeAESUnpack);
                                }
                            }
                            record.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, nFolderDecompressedSize);
                            record.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, nCurrentUncompressedOffset);
                            record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, nFileSize);
                            record.mapProperties.insert(FPART_PROP_CRC_TYPE, (quint32)CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
                            record.mapProperties.insert(FPART_PROP_RESULTCRC, nFileCRC);
                            // For solid folders, also store the whole-folder unpack CRC for post-decompression block verification
                            if (bIsSolid && nCurrentFolder < (qint32)state.listOutStreams.count()) {
                                quint32 nFolderCRC = state.listOutStreams.at(nCurrentFolder).nCRC;
                                if (nFolderCRC != 0) {
                                    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDCRC, nFolderCRC);
                                }
                            }
                            record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
                            record.mapProperties.insert(FPART_PROP_ISSOLID, bIsSolid);
                            record.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)nCurrentFolder);

                            nCurrentUncompressedOffset += nFileSize;
                            nFileIndexInCurrentFolder++;

                            // Advance to next folder when current folder's files are exhausted
                            if (nFileIndexInCurrentFolder >= nFilesInFolder) {
                                nCurrentFolder++;
                                nFileIndexInCurrentFolder = 0;
                                nCurrentUncompressedOffset = 0;
                            }
                        }

                        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, state.listFileNames.at(nCurrentFileIndex));

                        // Decode per-file timestamps and Windows attributes
                        quint64 nWinFileTime = 0;
                        if (_decode7zTimeValue(state.baMTime, nNumberOfFiles, nCurrentFileIndex, &nWinFileTime)) {
                            QDateTime dtMTime = XBinary::winFileTimeToQDateTime(nWinFileTime);
                            if (dtMTime.isValid()) {
                                record.mapProperties.insert(FPART_PROP_MTIME, dtMTime);
                            }
                        }
                        nWinFileTime = 0;
                        if (_decode7zTimeValue(state.baCTime, nNumberOfFiles, nCurrentFileIndex, &nWinFileTime)) {
                            QDateTime dtCTime = XBinary::winFileTimeToQDateTime(nWinFileTime);
                            if (dtCTime.isValid()) {
                                record.mapProperties.insert(FPART_PROP_CTIME, dtCTime);
                            }
                        }
                        nWinFileTime = 0;
                        if (_decode7zTimeValue(state.baATime, nNumberOfFiles, nCurrentFileIndex, &nWinFileTime)) {
                            QDateTime dtATime = XBinary::winFileTimeToQDateTime(nWinFileTime);
                            if (dtATime.isValid()) {
                                record.mapProperties.insert(FPART_PROP_ATIME, dtATime);
                            }
                        }
                        quint32 nWinAttrib = 0;
                        if (_decode7zAttribValue(state.baWinAttrib, nNumberOfFiles, nCurrentFileIndex, &nWinAttrib)) {
                            record.mapProperties.insert(FPART_PROP_ISREADONLY, (nWinAttrib & 0x01) != 0);
                            record.mapProperties.insert(FPART_PROP_ISHIDDEN, (nWinAttrib & 0x02) != 0);
                            record.mapProperties.insert(FPART_PROP_ISSYSTEM, (nWinAttrib & 0x04) != 0);
                            record.mapProperties.insert(FPART_PROP_ISARCHIVE, (nWinAttrib & 0x20) != 0);
                        }

                        pContext->listArchiveRecords.append(record);
                    }

                    if (pContext->listArchiveRecords.count() != nNumberOfFiles) {
#ifdef QT_DEBUG
                        qDebug("CHECK!!!");
#endif
                    }
                } else {
                    _errorMessage(tr("Cannot unpack data"), pPdStruct);
                }
            } else {
                _errorMessage(tr("Invalid format data"), pPdStruct);
            }

            pState->nNumberOfRecords = pContext->listArchiveRecords.count();
            bResult = (pState->nNumberOfRecords > 0);

            if (!bResult) {
                delete pContext;
                pState->pContext = nullptr;
            } else {
                pState->mapArchiveProperties.insert(FPART_PROP_FILEMD5, sMD5);
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

bool XSevenZip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = true;

    if (pState && pState->pContext) {
        SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;

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

QList<XBinary::FPART_PROP> XSevenZip::getAvailableFPARTProperties()
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

void XSevenZip::_printRecords(QList<SZRECORD> *pListRecords)
{
    if (!pListRecords) {
        return;
    }

    qDebug("=== SZRECORD list: %d records ===", pListRecords->count());

    for (qint32 nI = 0; nI < pListRecords->count(); nI++) {
        const SZRECORD &rec = pListRecords->at(nI);

        QString sSrType;
        if (rec.srType == SRTYPE_UNKNOWN) sSrType = "UNKNOWN";
        else if (rec.srType == SRTYPE_ID) sSrType = "ID";
        else if (rec.srType == SRTYPE_NUMBER) sSrType = "NUMBER";
        else if (rec.srType == SRTYPE_BYTE) sSrType = "BYTE";
        else if (rec.srType == SRTYPE_UINT32) sSrType = "UINT32";
        else if (rec.srType == SRTYPE_ARRAY) sSrType = "ARRAY";
        else sSrType = QString::number((qint32)rec.srType);

        QString sValType;
        if (rec.valType == VT_UNKNOWN) sValType = "UNKNOWN";
        else if (rec.valType == VT_BYTE) sValType = "BYTE";
        else if (rec.valType == VT_UINT16) sValType = "UINT16";
        else if (rec.valType == VT_UINT32) sValType = "UINT32";
        else if (rec.valType == VT_UINT64) sValType = "UINT64";
        else if (rec.valType == VT_INT8) sValType = "INT8";
        else if (rec.valType == VT_INT16) sValType = "INT16";
        else if (rec.valType == VT_INT32) sValType = "INT32";
        else if (rec.valType == VT_INT64) sValType = "INT64";
        else if (rec.valType == VT_PACKEDNUMBER) sValType = "PACKEDNUMBER";
        else if (rec.valType == VT_STRING) sValType = "STRING";
        else if (rec.valType == VT_BYTE_ARRAY) sValType = "BYTE_ARRAY";
        else if (rec.valType == VT_CHAR_ARRAY) sValType = "CHAR_ARRAY";
        else sValType = QString::number((qint32)rec.valType);

        QString sImpType;
        if (rec.impType == IMPTYPE_UNKNOWN) sImpType = "UNKNOWN";
        else if (rec.impType == IMPTYPE_NUMBEROFFOLDERS) sImpType = "NUMBEROFFOLDERS";
        else if (rec.impType == IMPTYPE_NUMBEROFFILES) sImpType = "NUMBEROFFILES";
        else if (rec.impType == IMPTYPE_NUMBEROFCODERS) sImpType = "NUMBEROFCODERS";
        else if (rec.impType == IMPTYPE_STREAMCRC) sImpType = "STREAMCRC";
        else if (rec.impType == IMPTYPE_STREAMOFFSET) sImpType = "STREAMOFFSET";
        else if (rec.impType == IMPTYPE_STREAMSIZE) sImpType = "STREAMSIZE";
        else if (rec.impType == IMPTYPE_CODERUNPACKEDSIZE) sImpType = "CODERUNPACKEDSIZE";
        else if (rec.impType == IMPTYPE_STREAMUNPACKEDCRC) sImpType = "STREAMUNPACKEDCRC";
        else if (rec.impType == IMPTYPE_NUMBEROFPACKSTREAMS) sImpType = "NUMBEROFPACKSTREAMS";
        else if (rec.impType == IMPTYPE_CODER) sImpType = "CODER";
        else if (rec.impType == IMPTYPE_CODERPROPERTY) sImpType = "CODERPROPERTY";
        else if (rec.impType == IMPTYPE_FILENAME) sImpType = "FILENAME";
        else if (rec.impType == IMPTYPE_FILECRC) sImpType = "FILECRC";
        else if (rec.impType == IMPTYPE_FILEATTRIBUTES) sImpType = "FILEATTRIBUTES";
        else if (rec.impType == IMPTYPE_FILETIME) sImpType = "FILETIME";
        else if (rec.impType == IMPTYPE_FILEPACKEDSIZE) sImpType = "FILEPACKEDSIZE";
        else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE) sImpType = "FILEUNPACKEDSIZE";
        else if (rec.impType == IMPTYPE_NUMBEROFUNPACKSTREAM) sImpType = "NUMBEROFUNPACKSTREAM";
        else if (rec.impType == IMPTYPE_EMPTYSTREAMDATA) sImpType = "EMPTYSTREAMDATA";
        else if (rec.impType == IMPTYPE_EMPTYFILEDATA) sImpType = "EMPTYFILEDATA";
        else if (rec.impType == IMPTYPE_CTIMEDATA) sImpType = "CTIMEDATA";
        else if (rec.impType == IMPTYPE_ATIMEDATA) sImpType = "ATIMEDATA";
        else if (rec.impType == IMPTYPE_MTIMEDATA) sImpType = "MTIMEDATA";
        else if (rec.impType == IMPTYPE_WINATTRIBDATA) sImpType = "WINATTRIBDATA";
        else sImpType = QString::number((qint32)rec.impType);

        QString sValue;
        if (rec.srType == SRTYPE_ARRAY) {
            QByteArray baValue = rec.varValue.toByteArray();
            sValue = QString("(%1 bytes) %2").arg(baValue.size()).arg(QString(baValue.toHex()));
        } else {
            sValue = rec.varValue.toString();
        }

        qDebug("[%d] offset=0x%X size=0x%X name=\"%s\" srType=%s valType=%s impType=%s flags=0x%X value=%s", nI, (quint32)rec.nRelOffset, (quint32)rec.nSize,
               rec.sName.toUtf8().constData(), sSrType.toUtf8().constData(), sValType.toUtf8().constData(), sImpType.toUtf8().constData(), rec.nFlags,
               sValue.toUtf8().constData());
    }

    qDebug("=== end SZRECORD list ===");
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

bool XSevenZip::decompressHeader(const QMap<UNPACK_PROP, QVariant> &mapUnpackProperties, QIODevice *pDeviceOut, SZSTATE *pState, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XDecompress xDecompress;
    connect(&xDecompress, &XDecompress::errorMessage, this, &XBinary::errorMessage);
    connect(&xDecompress, &XDecompress::infoMessage, this, &XBinary::infoMessage);

    if (pState->listFolders.count() > 0) {
        qint32 nFolderIndex = 0;

        qint64 nStreamOffset = pState->nStreamsBegin + pState->listInStreams.at(nFolderIndex).nOffset;
        qint64 nStreamSize = pState->listInStreams.at(nFolderIndex).nSize;

        const SZFOLDER &folder = pState->listFolders.at(nFolderIndex);
        qint32 nNumCoders = folder.listCoders.count();

        // Compute the index into listCodersSizes for the first coder of this folder
        qint32 nCoderSizesOffset = 0;
        for (qint32 i = 0; i < nFolderIndex; i++) {
            nCoderSizesOffset += pState->listFolders.at(i).listCoders.count();
        }

        // Build mapProperties BEFORE assigning to state.
        // multiDecompress processes methods in reverse order:
        //   i=last uses HANDLEMETHOD2 -> reads raw stream  (= coder[0])
        //   i=0    uses HANDLEMETHOD  -> writes final output (= coder[last])
        QMap<FPART_PROP, QVariant> mapProperties;
        mapProperties.insert(FPART_PROP_STREAMOFFSET, nStreamOffset);
        mapProperties.insert(FPART_PROP_STREAMSIZE, nStreamSize);
        mapProperties.insert(FPART_PROP_CRC_TYPE, (quint32)CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
        mapProperties.insert(FPART_PROP_RESULTCRC, pState->listOutStreams.at(nFolderIndex).nCRC);

        if (nNumCoders >= 1) {
            qint32 nLastCoder = nNumCoders - 1;
            mapProperties.insert(FPART_PROP_HANDLEMETHOD, (quint32)coderToCompressMethod(folder.listCoders.at(nLastCoder).baCoder));
            mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, folder.listCoders.at(nLastCoder).baProperty);
            if ((nCoderSizesOffset + nLastCoder) < pState->listCodersSizes.count()) {
                mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)pState->listCodersSizes.at(nCoderSizesOffset + nLastCoder));
            }
        }

        if (nNumCoders >= 2) {
            mapProperties.insert(FPART_PROP_HANDLEMETHOD2, (quint32)coderToCompressMethod(folder.listCoders.at(0).baCoder));
            mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES2, folder.listCoders.at(0).baProperty);
            if (nCoderSizesOffset < pState->listCodersSizes.count()) {
                mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE2, (qint64)pState->listCodersSizes.at(nCoderSizesOffset));
            }
        }

        XBinary::DATAPROCESS_STATE state = {};
        state.mapProperties = mapProperties;
        state.mapUnpackProperties = mapUnpackProperties;
        state.pDeviceInput = getDevice();
        state.pDeviceOutput = pDeviceOut;
        state.nInputOffset = nStreamOffset;
        state.nInputLimit = nStreamSize;
        state.nProcessedOffset = 0;
        // Limit final output to the expected unpack size so each decoder is properly bounded
        if (nNumCoders >= 1) {
            qint32 nLastCoder = nNumCoders - 1;
            if ((nCoderSizesOffset + nLastCoder) < pState->listCodersSizes.count()) {
                state.nProcessedLimit = (qint64)pState->listCodersSizes.at(nCoderSizesOffset + nLastCoder);
            } else {
                state.nProcessedLimit = -1;
            }
        } else {
            state.nProcessedLimit = -1;
        }

        bResult = xDecompress.multiDecompress(&state, pPdStruct);
    }

    return bResult;
}

QList<QString> XSevenZip::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'7z'BCAF271C");

    return listResult;
}

XBinary *XSevenZip::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSevenZip(pDevice);
}

