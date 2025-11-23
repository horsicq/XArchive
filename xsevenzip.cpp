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
#include "xdecompress.h"
#include "xcompress.h"
#include "Algos/xlzmadecoder.h"
#include "Algos/xaesdecoder.h"
#include "Algos/xppmd7model.h"
#include "Algos/xbzip2decoder.h"
#include "Algos/xdeflatedecoder.h"
#include <QBuffer>
#include <QFileInfo>
#include <QDir>

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
            quint64 nNumberOfPackStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfPackStreams", DRF_COUNT, IMPTYPE_NUMBEROFSTREAMS);
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
            // _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct, IMPTYPE_UNKNOWN);

            break;

        case XSevenZip::k7zIdNumUnpackStream: {
            // NumUnpackStream: one value per folder, indicating how many files are in each solid block
            // Then Size section contains unpacked sizes for all files
            quint64 nTotalSubStreams = 0;

            qDebug() << "[NumUnpackStream] Parsing" << pState->nNumberOfFolders << "folder entries";
            for (quint64 i = 0; i < pState->nNumberOfFolders && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nNumStreamsInFolder = _handleNumber(pListRecords, pState, pPdStruct, QString("NumUnpackStream%1").arg(i), DRF_COUNT, IMPTYPE_NUMUNPACKSTREAM);
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
        pContext->nSignatureSize = sizeof(SIGNATUREHEADER);

        // Debug: Show what's in mapProperties
        qDebug() << "[initUnpack] Properties count:" << mapProperties.size();
        for (auto it = mapProperties.begin(); it != mapProperties.end(); ++it) {
            qDebug() << "[initUnpack]   Property key" << it.key() << "value type:" << it.value().typeName();
        }

        // Extract password if provided
        if (mapProperties.contains(UNPACK_PROP_PASSWORD)) {
            pContext->sPassword = mapProperties.value(UNPACK_PROP_PASSWORD).toString();
            qDebug() << "[initUnpack] ✓ Password extracted:" << pContext->sPassword.length() << "characters";
        } else {
            qDebug() << "[initUnpack] ✗ No password (looking for key" << (int)UNPACK_PROP_PASSWORD << ")";
        }        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;

        QList<qint64> listSubStreamSizes;  // Moved declaration to the top of the function

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
                // NOTE: Experimental code to debug encoded header handling
                // System 7z creates solid archives even with -ms=off flag
                // The encoded header decompresses to MainStreamsInfo only (no FilesInfo)
                // FilesInfo location in solid archives needs further investigation
                /*
                QList<XSevenZip::SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct, true);
                #ifdef QT_DEBUG
                qDebug() << "XSevenZip::initUnpack: Parsed" << listRecords.count() << "records from header";
                #endif
                */

                // === USE EXISTING APPROACH ===
                // Check if this is an encoded header by looking at the first byte
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
                            QByteArray baCompressedData = read_array(sizeof(SIGNATUREHEADER) + nStreamOffset, nStreamPackedSize, pPdStruct);
                            QByteArray baUncompressedData;

                            QBuffer bufferIn;
                            bufferIn.setBuffer(&baCompressedData);

                            QBuffer bufferOut;
                            // bufferOut.setData(pUnpackedData, nStreamUnpackedSize);
                            bufferOut.setBuffer(&baUncompressedData);

                            if (bufferIn.open(QIODevice::ReadOnly) && bufferOut.open(QIODevice::WriteOnly)) {
                                DATAPROCESS_STATE decompressState = {};
                                decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
                                decompressState.pDeviceInput = &bufferIn;
                                decompressState.pDeviceOutput = &bufferOut;
                                decompressState.nInputOffset = 0;
                                decompressState.nInputLimit = nStreamPackedSize;
                                decompressState.nProcessedOffset = 0;
                                decompressState.nProcessedLimit = -1;

                                bool bDecompressResult = false;

                                // Only LZMA! in encrypted header
                                if (compressMethod == COMPRESS_METHOD_LZMA) {
                                    bDecompressResult = XLZMADecoder::decompress(&decompressState, baProperty, pPdStruct);
                                    // bDecompressResult = XLZMADecoder::decompress(&decompressState, pPdStruct);
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
                                if (bDecompressResult) {
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
                            } else {
                                state.bIsError = true;
                                state.sErrorString = tr("Failed to open buffers for decompression");
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
                        qDebug() << QString("[%1] Offset:%2 Size:%3 srType:%4 valType:%5 impType:%6 Name:%7 Value:%8")
                                        .arg(i, 4)
                                        .arg(rec.nRelOffset, 6)
                                        .arg(rec.nSize, 6)
                                        .arg(rec.srType, 2)
                                        .arg(rec.valType, 2)
                                        .arg(rec.impType, 3)
                                        .arg(rec.sName, -30)
                                        .arg(sValue);
                    }
                    qDebug() << "=== END listRecords DEBUG OUTPUT ===";
#endif

                    // Process the parsed records to extract file information
                    if (pHeaderData) {
                        QList<QString> listFileNames;
                        QList<qint64> listFilePackedSizes;
                        QList<qint64> listFileUnpackedSizes;
                        QList<qint64> listStreamOffsets;
                        QList<QByteArray> listCodecs;
                        QList<QByteArray> listCoderProperties;
                        
                        QList<XSevenZip::SZRECORD> listRecords;
                        SZSTATE state = {};
                        state.pData = pHeaderData;
                        state.nSize = nHeaderSize;
                        state.nCurrentOffset = 0;
                        state.bIsError = false;
                        state.sErrorString = QString();
                        
                        _handleId(&listRecords, XSevenZip::k7zIdHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                        qint32 nNumberOfRecords = listRecords.count();
                        
                        qDebug() << "[Record Extract] Total records after parsing:" << nNumberOfRecords;
                        qint32 nFilenameCount = 0;
                        QMap<qint32, qint32> impTypeCounts;
                        for (qint32 i = 0; i < nNumberOfRecords; i++) {
                            qint32 impType = listRecords.at(i).impType;
                            impTypeCounts[impType]++;
                            if (impType == IMPTYPE_FILENAME) nFilenameCount++;
                        }
                        qDebug() << "[Record Extract] Filename records found:" << nFilenameCount;
                        if (nFilenameCount == 0) {
                            qDebug() << "[Record Extract] ImpType distribution:" << impTypeCounts;
                        }
                        
                        // Extract file information from parsed records
                        qint32 nExtractedFilenames = 0;
                        QList<qint32> listNumUnpackStream;  // Number of files per folder
                        QList<qint64> listFolderUnpackedSizes;  // Folder sizes (STREAMUNPACKEDSIZE)
                        QList<qint64> listIndividualFileSizes;  // Individual file sizes (FILEUNPACKEDSIZE)
                        QByteArray baEmptyStreamData;  // Bitmap indicating which files are empty (0 bytes)
                        for (qint32 i = 0; i < nNumberOfRecords; i++) {
                            SZRECORD rec = listRecords.at(i);
                            if (rec.impType == IMPTYPE_FILENAME) {
                                listFileNames.append(rec.varValue.toString());
                                nExtractedFilenames++;
                            } else if (rec.impType == IMPTYPE_STREAMPACKEDSIZE) {
                                listFilePackedSizes.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                                // STREAMUNPACKEDSIZE: Folder/stream unpack size (total size for solid block or single file size for non-solid)
                                listFolderUnpackedSizes.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE) {
                                // FILEUNPACKEDSIZE: Individual file sizes from SubStreamsInfo (for solid archives, contains N-1 sizes)
                                listIndividualFileSizes.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                                listStreamOffsets.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_CODER) {
                                listCodecs.append(rec.varValue.toByteArray());
                            } else if (rec.impType == IMPTYPE_CODERPROPERTY) {
                                listCoderProperties.append(rec.varValue.toByteArray());
                            } else if (rec.impType == IMPTYPE_NUMUNPACKSTREAM) {
                                listNumUnpackStream.append(rec.varValue.toInt());
                            } else if (rec.sName == "EmptyStreamData") {
                                // EmptyStream bitmap: bit set = file is empty (0 bytes)
                                baEmptyStreamData = rec.varValue.toByteArray();
                            }
                        }
                        qDebug() << "[Extract Loop] Extracted" << nExtractedFilenames << "filenames from" << nNumberOfRecords << "records";
                        qDebug() << "[Size Lists] Folder sizes:" << listFolderUnpackedSizes.count() << "Individual file sizes:" << listIndividualFileSizes.count();
                        
                        // CRITICAL FIX: Handle archives with NumberOfFiles=0 but valid folder/stream data
                        // Some 7z archives (created by certain tools/versions) set NumberOfFiles=0 in FilesInfo
                        // but still contain valid compressed streams. Generate placeholder filenames based on streams.
                        if (listFileNames.isEmpty() && !listFilePackedSizes.isEmpty()) {
                            qWarning() << "[NumberOfFiles=0 Fix] Archive has no filenames but" << listFilePackedSizes.count() << "streams";
                            qWarning() << "[NumberOfFiles=0 Fix] Generating placeholder filenames from stream/folder data";
                            
                            // Determine number of files from NumUnpackStream or codec analysis
                            qint32 nTotalFiles = 0;
                            if (!listNumUnpackStream.isEmpty()) {
                                // Sum all NumUnpackStream values to get total file count
                                for (qint32 i = 0; i < listNumUnpackStream.count(); i++) {
                                    nTotalFiles += listNumUnpackStream.at(i);
                                }
                                qWarning() << "[NumberOfFiles=0 Fix] NumUnpackStream indicates" << nTotalFiles << "files across" << listNumUnpackStream.count() << "folders";
                            } else {
                                // Check if this is a BCJ2 archive (4 codecs with last being BCJ2)
                                // BCJ2 takes 4 input streams and produces 1 output (solid compression)
                                bool bIsBCJ2Archive = false;
                                if (listCodecs.count() == 4) {
                                    // Check if last codec is BCJ2 (0x03 0x03 0x01 0x1b)
                                    QByteArray baLastCodec = listCodecs.at(3);
                                    if (baLastCodec.size() == 4 && 
                                        (quint8)baLastCodec.at(0) == 0x03 &&
                                        (quint8)baLastCodec.at(1) == 0x03 &&
                                        (quint8)baLastCodec.at(2) == 0x01 &&
                                        (quint8)baLastCodec.at(3) == 0x1b) {
                                        bIsBCJ2Archive = true;
                                        qWarning() << "[NumberOfFiles=0 Fix] Detected BCJ2 solid archive (4 streams → 1 output)";
                                    }
                                }
                                
                                if (bIsBCJ2Archive) {
                                    // BCJ2: 4 input streams produce 1 output, assume 1 file
                                    // (without SubStreamsInfo, we cannot determine the actual file count)
                                    nTotalFiles = 1;
                                    qWarning() << "[NumberOfFiles=0 Fix] BCJ2 archive: assuming 1 file (cannot determine actual count without SubStreamsInfo)";
                                } else {
                                    // Fallback: 1 file per stream for non-BCJ2 archives
                                    nTotalFiles = listFilePackedSizes.count();
                                    qWarning() << "[NumberOfFiles=0 Fix] Assuming" << nTotalFiles << "files (1 per stream)";
                                }
                            }
                            
                            // Generate placeholder filenames
                            for (qint32 i = 0; i < nTotalFiles; i++) {
                                QString sPlaceholderName = QString("file_%1.bin").arg(i, 5, 10, QChar('0'));
                                listFileNames.append(sPlaceholderName);
                            }
                            
                            qWarning() << "[NumberOfFiles=0 Fix] Generated" << listFileNames.count() << "placeholder filenames";
                            
                            // If we have individual file sizes from SubStreamsInfo, use them
                            // Otherwise, file sizes will be determined from folder sizes later
                            if (!listIndividualFileSizes.isEmpty()) {
                                qWarning() << "[NumberOfFiles=0 Fix] Using" << listIndividualFileSizes.count() << "individual file sizes from SubStreamsInfo";
                            }
                        }
                        
                        // Parse EmptyStream bitmap to identify which files are empty (0 bytes)
                        // Bitmap format: each bit represents one file, bit=1 means empty file
                        if (!baEmptyStreamData.isEmpty()) {
                            qDebug() << "[EmptyStream Bitmap] Size:" << baEmptyStreamData.size() << "bytes";
                            QString sBitmapHex;
                            for (qint32 i = 0; i < baEmptyStreamData.size() && i < 8; i++) {
                                sBitmapHex += QString("%1 ").arg((quint8)baEmptyStreamData.at(i), 2, 16, QChar('0'));
                            }
                            qDebug() << "[EmptyStream Bitmap] Data:" << sBitmapHex;
                        }
                        
                        QList<bool> listIsEmptyFile;  // listIsEmptyFile[fileIndex] = true if file is empty
                        for (qint32 i = 0; i < listFileNames.count(); i++) {
                            bool bIsEmpty = false;
                            if (!baEmptyStreamData.isEmpty()) {
                                qint32 nByteIndex = i / 8;
                                qint32 nBitIndex = i % 8;
                                if (nByteIndex < baEmptyStreamData.size()) {
                                    quint8 nByte = (quint8)baEmptyStreamData.at(nByteIndex);
                                    // CRITICAL: 7z format EmptyStream bitmap uses MSB-first bit order!
                                    // Within each byte, bit 7 (MSB) represents the first file, bit 0 (LSB) represents the 8th file
                                    // Bit SET (1) = file has NO stream (empty, 0 bytes)
                                    // Bit CLEAR (0) = file HAS stream (non-empty, has data)
                                    // Reverse bit index: bit 7-0 → file 0-7 within byte
                                    qint32 nReversedBitIndex = 7 - nBitIndex;
                                    bIsEmpty = (nByte & (1 << nReversedBitIndex)) != 0;
                                }
                            }
                            listIsEmptyFile.append(bIsEmpty);
                        }
                        
                        qint32 nEmptyFileCount = 0;
                        QStringList listEmptyFileIndices;
                        for (qint32 i = 0; i < listIsEmptyFile.count(); i++) {
                            if (listIsEmptyFile.at(i)) {
                                nEmptyFileCount++;
                                listEmptyFileIndices.append(QString::number(i));
                            }
                        }
                        qDebug() << "[Empty Files] Detected" << nEmptyFileCount << "empty files out of" << listFileNames.count() << "total files";
                        if (!listEmptyFileIndices.isEmpty()) {
                            qDebug() << "[Empty Files] Indices:" << listEmptyFileIndices.join(", ");
                        }
                        
                        // Build file-to-folder mapping
                        // CRITICAL: listNumUnpackStream[i] = number of NON-EMPTY files in folder i
                        // Empty files (marked by EmptyStream bitmap) should be assigned folder index -1
                        // NumUnpackStream counts only files with actual data, not empty (0-byte) files
                        QList<qint32> listFileToFolderMap;  // listFileToFolderMap[fileIndex] = folderIndex (or -1 for empty files)
                        
                        if (listNumUnpackStream.isEmpty()) {
                            // Simple case: 1 file per folder
                            // CRITICAL: For encrypted COPY archives, there are multiple codecs (AES + COPY)
                            // but only 1 folder with data. Folder 0 contains the encrypted stream.
                            // Files always map to folder 0 in this case.
                            qint32 nFilesWithFolders = listCodecs.count();
                            qint32 nEmptyFilesCount = listFileNames.count() - nFilesWithFolders;
                            
                            for (qint32 i = 0; i < listFileNames.count(); i++) {
                                if (nEmptyFilesCount > 0 && i < nEmptyFilesCount) {
                                    // First N files have no folder (empty files)
                                    listFileToFolderMap.append(-1);
                                } else {
                                    // Remaining files map to folders
                                    // CRITICAL FIX: For encrypted archives with chained codecs (AES -> COPY),
                                    // all files belong to folder 0 (the first codec in the chain)
                                    qint32 nFolderIndex = (nEmptyFilesCount >= 0) ? (i - nEmptyFilesCount) : 0;
                                    listFileToFolderMap.append(nFolderIndex);
                                    qDebug() << "[Folder Map] File" << i << "mapped to folder" << nFolderIndex 
                                             << "(empty count:" << nEmptyFilesCount << "codecs:" << listCodecs.count() << ")";
                                }
                            }
                        } else {
                            // Complex case: use NumUnpackStream to map NON-EMPTY files to folders
                            // CRITICAL FIX: NumUnpackStream counts non-empty files only
                            // We must skip empty files when assigning to folders
                            qDebug() << "[NumUnpackStream Mapping] NumUnpackStream count:" << listNumUnpackStream.count();
                            for (qint32 i = 0; i < listNumUnpackStream.count() && i < 10; i++) {
                                qDebug() << "  Folder" << i << "has" << listNumUnpackStream.at(i) << "non-empty files";
                            }
                            
                            // First pass: initialize all files with folder index -1
                            for (qint32 i = 0; i < listFileNames.count(); i++) {
                                listFileToFolderMap.append(-1);
                            }
                            
                            // Second pass: assign folder indices to NON-EMPTY files only
                            qint32 nNonEmptyFilesSeen = 0;  // Counter for non-empty files processed
                            for (qint32 i = 0; i < listFileNames.count(); i++) {
                                bool bIsEmptyFile = (i < listIsEmptyFile.count()) ? listIsEmptyFile.at(i) : false;
                                
                                if (!bIsEmptyFile) {
                                    // This is a non-empty file - find which folder it belongs to
                                    qint32 nFolderIndex = 0;
                                    qint32 nFilesAccountedFor = 0;
                                    
                                    // Iterate through folders to find which folder this non-empty file belongs to
                                    for (qint32 iFolderIndex = 0; iFolderIndex < listNumUnpackStream.count(); iFolderIndex++) {
                                        qint32 nFilesInThisFolder = listNumUnpackStream.at(iFolderIndex);
                                        if (nNonEmptyFilesSeen < nFilesAccountedFor + nFilesInThisFolder) {
                                            // This non-empty file belongs to folder iFolderIndex
                                            nFolderIndex = iFolderIndex;
                                            break;
                                        }
                                        nFilesAccountedFor += nFilesInThisFolder;
                                    }
                                    
                                    listFileToFolderMap[i] = nFolderIndex;
                                    nNonEmptyFilesSeen++;
                                }
                                // else: empty file keeps folder index -1
                            }
                            
                            qDebug() << "[NumUnpackStream Mapping] Assigned" << nNonEmptyFilesSeen << "non-empty files to folders," 
                                     << (listFileNames.count() - nNonEmptyFilesSeen) << "empty files with no folder";
                        }
                        
                        qDebug() << "[File-Folder Mapping] Files:" << listFileNames.count() 
                                 << "Folders:" << listCodecs.count() 
                                 << "NumUnpackStream entries:" << listNumUnpackStream.count()
                                 << "Mapping size:" << listFileToFolderMap.count();
                        if (listFileToFolderMap.count() > 0 && listFileToFolderMap.count() <= 20) {
                            qDebug() << "[File-Folder Mapping] Map:" << listFileToFolderMap;
                        }
                        
#ifdef QT_DEBUG
                        qDebug() << "XSevenZip::initUnpack: Extracted metadata:";
                        qDebug() << "  File names:" << listFileNames.count();
                        qDebug() << "  Packed sizes:" << listFilePackedSizes.count();
                        qDebug() << "  Unpacked sizes:" << listFileUnpackedSizes.count();
                        qDebug() << "  Stream offsets:" << listStreamOffsets.count();
                        qDebug() << "  Codecs:" << listCodecs.count();
                        for (qint32 i = 0; i < listCodecs.count(); i++) {
                            QString sCodecHex;
                            for (qint32 j = 0; j < listCodecs.at(i).size(); j++) {
                                sCodecHex += QString("%1 ").arg((quint8)listCodecs.at(i)[j], 2, 16, QChar('0'));
                            }
                            qDebug() << "    Codec" << i << ":" << sCodecHex << "->" << codecToCompressMethod(listCodecs.at(i));
                        }
                        qDebug() << "  Codec properties:" << listCoderProperties.count();
                        for (qint32 i = 0; i < listCoderProperties.count(); i++) {
                            QString sPropHex;
                            for (qint32 j = 0; j < listCoderProperties.at(i).size(); j++) {
                                sPropHex += QString("%1 ").arg((quint8)listCoderProperties.at(i)[j], 2, 16, QChar('0'));
                            }
                            qDebug() << "    Property" << i << "size:" << listCoderProperties.at(i).size() << "bytes -" << sPropHex;
                        }
#endif
                        
                        // CRITICAL FIX: Apply N-1 size correction for SOLID archives
                        // 7z solid archives (LZMA/LZMA2) use N-1 format:
                        //   listFileUnpackedSizes: [FolderSize, File0Size, File1Size, ..., File(N-2)Size]
                        //   Need: [File0Size, File1Size, File2Size, ..., File(N-1)Size]
                        // Copy/non-solid archives store ALL file sizes directly, no correction needed.
                        
                        // Debug: Always log counts to diagnose issues
                        qDebug() << "[Size Fix Check] Files:" << listFileNames.count() 
                                 << "Unpacked sizes:" << listFileUnpackedSizes.count()
                                 << "Packed sizes:" << listFilePackedSizes.count()
                                 << "Codecs:" << listCodecs.count();
                        
                        // Debug: Reduced output for Copy archives
                        if (listCodecs.count() > 5) {
                            qDebug() << "[Copy Archive] Files:" << listFileNames.count() 
                                     << "Packed sizes:" << listFilePackedSizes.count()
                                     << "Unpacked sizes:" << listFileUnpackedSizes.count()
                                     << "Stream offsets:" << listStreamOffsets.count();
                            if (listFilePackedSizes.count() > 0) {
                                qDebug() << "[Copy Archive] First 5 packed sizes:"
                                         << (listFilePackedSizes.count() > 0 ? listFilePackedSizes.at(0) : -1)
                                         << (listFilePackedSizes.count() > 1 ? listFilePackedSizes.at(1) : -1)
                                         << (listFilePackedSizes.count() > 2 ? listFilePackedSizes.at(2) : -1)
                                         << (listFilePackedSizes.count() > 3 ? listFilePackedSizes.at(3) : -1)
                                         << (listFilePackedSizes.count() > 4 ? listFilePackedSizes.at(4) : -1);
                            }
                        }
                        
                        // N-1 fix will be applied later after compressMethod is determined
                        
                        // File logging for all builds (unconditional)
                        {
                            QFile logFile(QDir::tempPath() + "/xsevenzip_debug.log");
                            if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                                QTextStream log(&logFile);
                                log << "=== XSevenZip::initUnpack ===" << "\n";
                                log << "Codecs: " << listCodecs.count() << "\n";
                                for (qint32 i = 0; i < listCodecs.count(); i++) {
                                    QString sCodecHex;
                                    for (qint32 j = 0; j < listCodecs.at(i).size(); j++) {
                                        sCodecHex += QString("%1 ").arg((quint8)listCodecs.at(i)[j], 2, 16, QChar('0'));
                                    }
                                    COMPRESS_METHOD method = codecToCompressMethod(listCodecs.at(i));
                                    log << "  Codec " << i << ": " << sCodecHex << " -> " << method << "\n";
                                }
                                log << "Codec properties: " << listCoderProperties.count() << "\n";
                                for (qint32 i = 0; i < listCoderProperties.count(); i++) {
                                    QString sPropHex;
                                    for (qint32 j = 0; j < listCoderProperties.at(i).size(); j++) {
                                        sPropHex += QString("%1 ").arg((quint8)listCoderProperties.at(i)[j], 2, 16, QChar('0'));
                                    }
                                    log << "  Property " << i << " size: " << listCoderProperties.at(i).size() << " bytes - " << sPropHex << "\n";
                                }
                                logFile.close();
                            }
                        }
                        
                        // Determine compression method and check for encryption
                        COMPRESS_METHOD filterMethod = COMPRESS_METHOD_UNKNOWN;
                        COMPRESS_METHOD compressMethod = COMPRESS_METHOD_STORE;
                        bool bIsEncrypted = false;
                        qint32 nAesCodecIndex = -1;
                        qint32 nCompressCodecIndex = -1;
                        
                        for (qint32 i = 0; i < listCodecs.count(); i++) {
                            COMPRESS_METHOD method = codecToCompressMethod(listCodecs.at(i));
                            if (method == COMPRESS_METHOD_BCJ || method == COMPRESS_METHOD_BCJ2) {
                                filterMethod = method;
                            } else if (method == COMPRESS_METHOD_AES) {
                                bIsEncrypted = true;
                                nAesCodecIndex = i;
                                // AES is encryption layer, continue to find actual compression method
                            } else if (method != COMPRESS_METHOD_UNKNOWN) {
                                compressMethod = method;
                                nCompressCodecIndex = i;
                            }
                        }
                        
                        // Apply N-1 fix ONLY for LZMA/LZMA2 solid archives
                        // Condition: single codec (solid), unpacked sizes match file count, LZMA/LZMA2 codec
                        bool bApplyN1Fix = (listFileNames.count() > 1 && 
                                           listFileUnpackedSizes.count() == listFileNames.count() &&
                                           listCodecs.count() == 1 &&
                                           (compressMethod == COMPRESS_METHOD_LZMA || compressMethod == COMPRESS_METHOD_LZMA2));
                        
                        if (bApplyN1Fix) {
                            // First entry (index 0) is the total folder size (STREAMUNPACKEDSIZE)
                            // Entries 1 through N-1 are individual file sizes (FILEUNPACKEDSIZE for files 0..N-2)
                            qint64 nFolderSize = listFileUnpackedSizes.at(0);
                            
                            // Sum all file sizes (File0 through FileN-2, indices 1 through N-1)
                            qint64 nSumOfFiles = 0;
                            for (qint32 i = 1; i < listFileUnpackedSizes.count(); i++) {
                                nSumOfFiles += listFileUnpackedSizes.at(i);
                            }
                            
                            // Calculate last file size: FileLastSize = FolderSize - sum(File0...FileN-2)
                            qint64 nLastFileSize = nFolderSize - nSumOfFiles;
                            
                            // Rebuild list: [File0, File1, ..., File(N-2), CalculatedFile(N-1)]
                            QList<qint64> correctedSizes;
                            for (qint32 i = 1; i < listFileUnpackedSizes.count(); i++) {
                                correctedSizes.append(listFileUnpackedSizes.at(i));
                            }
                            correctedSizes.append(nLastFileSize);
                            listFileUnpackedSizes = correctedSizes;
                            
#ifdef QT_DEBUG
                            qDebug() << "[Size Fix] Applied N-1 size correction";
                            qDebug() << "[Size Fix]   Folder size:" << nFolderSize;
                            qDebug() << "[Size Fix]   Sum of N-1 files:" << nSumOfFiles;
                            qDebug() << "[Size Fix]   Calculated last file:" << nLastFileSize;
                            qDebug() << "[Size Fix]   Corrected sizes count:" << correctedSizes.count();
#endif
                        }
                        
                        // Detect solid compression per-folder (not archive-wide)
                        // 7z format supports folder-level solid compression:
                        // - A folder with multiple files can be solid (all files compressed into one stream)
                        // - A folder with one file is always non-solid
                        // - Different folders can have different solid settings
                        
                        qint64 nDataOffset = sizeof(SIGNATUREHEADER);
                        qint32 nNumberOfFolders = listFilePackedSizes.count();
                        
                        // Determine which folders are solid
                        QList<bool> listFolderIsSolid;
                        QList<qint32> listFolderFileCount;
                        
                        if (listNumUnpackStream.isEmpty()) {
                            // Simple case: 1 file per folder (all folders are non-solid)
                            for (qint32 i = 0; i < nNumberOfFolders; i++) {
                                listFolderIsSolid.append(false);
                                listFolderFileCount.append(1);
                            }
                        } else {
                            // Complex case: use NumUnpackStream to determine files per folder
                            for (qint32 i = 0; i < listNumUnpackStream.count(); i++) {
                                qint32 nFilesInFolder = listNumUnpackStream.at(i);
                                bool bFolderIsSolid = (nFilesInFolder > 1);  // Solid if multiple files share one stream
                                listFolderIsSolid.append(bFolderIsSolid);
                                listFolderFileCount.append(nFilesInFolder);
                                
#ifdef QT_DEBUG
                                qDebug() << "[Folder" << i << "]" 
                                         << "Files:" << nFilesInFolder 
                                         << "Solid:" << bFolderIsSolid
                                         << "Packed size:" << (i < listFilePackedSizes.count() ? listFilePackedSizes.at(i) : 0)
                                         << "Unpacked size:" << (i < listFolderUnpackedSizes.count() ? listFolderUnpackedSizes.at(i) : 0);
#endif
                            }
                        }
                        
                        // DEPRECATED: Archive-wide solid detection (kept for backward compatibility with single-folder archives)
                        bool bIsSolid = false;
                        qint64 nSolidStreamOffset = 0;
                        qint64 nSolidStreamSize = 0;
                        qint64 nSolidUnpackSize = 0;
                        
                        if (listFileNames.count() > 1 && listFilePackedSizes.count() == 1) {
                            // Legacy: Archive-wide solid (all files in one folder)
                            bIsSolid = true;
                            nSolidStreamOffset = nDataOffset;
                            if (listFilePackedSizes.count() > 0) {
                                nSolidStreamSize = listFilePackedSizes.at(0);
                            }
                            for (qint32 i = 0; i < listFolderUnpackedSizes.count(); i++) {
                                nSolidUnpackSize += listFolderUnpackedSizes.at(i);
                            }
#ifdef QT_DEBUG
                            qDebug() << "[LEGACY] Archive-wide SOLID detected:";
                            qDebug() << "  Files:" << listFileNames.count();
                            qDebug() << "  Compressed stream size:" << nSolidStreamSize;
                            qDebug() << "  Total uncompressed size:" << nSolidUnpackSize;
#endif
                        }
                        
                        // Create ARCHIVERECORD entries
                        // Track decompressed offset per-folder (for solid folders)
                        QList<qint64> listFolderDecompressedOffset;
                        for (qint32 i = 0; i < nNumberOfFolders; i++) {
                            listFolderDecompressedOffset.append(0);
                        }
                        
                        qint64 nAccumulatedOffsetSolid = 0;  // For legacy archive-wide solid
                        qint64 nAccumulatedOffsetNonSolid = 0;
                        
                        // CRITICAL: Counter for non-empty files in folders for solid archives
                        // listIndividualFileSizes contains sizes ONLY for non-empty files in folders
                        // Empty files (listIsEmptyFile[i] == true) are NOT included in listIndividualFileSizes
                        qint32 nNonEmptyFileIndexInFolder = 0;
                        
                        for (qint32 i = 0; i < listFileNames.count(); i++) {
                            ARCHIVERECORD record = {};
                            record.mapProperties.insert(FPART_PROP_ORIGINALNAME, listFileNames.at(i));
                            
                            // Get folder index for this file (-1 means no folder)
                            qint32 nFolderIndex = (i < listFileToFolderMap.count()) ? listFileToFolderMap.at(i) : -1;
                            
                            // Check if this file is empty (0 bytes) using EmptyStream bitmap
                            bool bIsEmptyFile = (i < listIsEmptyFile.count()) ? listIsEmptyFile.at(i) : false;
                            
                            // Assign decompressed size
                            // For solid archives: use individual file sizes from SubStreamsInfo
                            // For non-solid archives: use folder size (1 file per folder)
                            
                            // Check if this file's folder is solid
                            bool bFolderIsSolid = false;
                            if (nFolderIndex >= 0 && nFolderIndex < listFolderIsSolid.count()) {
                                bFolderIsSolid = listFolderIsSolid.at(nFolderIndex);
                            } else if (bIsSolid) {
                                // Fallback to legacy archive-wide solid detection
                                bFolderIsSolid = true;
                            }
                            
                            if (bIsEmptyFile) {
                                // Empty file: always 0 bytes
                                record.nDecompressedSize = 0;
                            } else if (nFolderIndex >= 0) {
                                if (bFolderIsSolid && !listIndividualFileSizes.isEmpty()) {
                                    // Solid folder: use individual file size from SubStreamsInfo
                                    // CRITICAL FIX: Use nNonEmptyFileIndexInFolder to index listIndividualFileSizes
                                    // listIndividualFileSizes[j] corresponds to the j-th NON-EMPTY file in folders
                                    // listIndividualFileSizes contains N-1 sizes (all except the last non-empty file)
                                    // Last file size = FolderSize - sum of all other non-empty files
                                    if (nNonEmptyFileIndexInFolder < listIndividualFileSizes.count()) {
                                        record.nDecompressedSize = listIndividualFileSizes.at(nNonEmptyFileIndexInFolder);
                                    } else if (nNonEmptyFileIndexInFolder == listIndividualFileSizes.count() && nFolderIndex < listFolderUnpackedSizes.count()) {
                                        // Last non-empty file: calculate size as folder size minus all other files
                                        qint64 nFolderSize = listFolderUnpackedSizes.at(nFolderIndex);
                                        qint64 nSumOtherFiles = 0;
                                        for (qint32 j = 0; j < listIndividualFileSizes.count(); j++) {
                                            nSumOtherFiles += listIndividualFileSizes.at(j);
                                        }
                                        record.nDecompressedSize = nFolderSize - nSumOtherFiles;
                                    } else {
                                        record.nDecompressedSize = 0;
                                    }
                                    nNonEmptyFileIndexInFolder++;  // Increment counter for next non-empty file
                                } else {
                                    // Non-solid archive or no individual sizes: use folder size
                                    if (nFolderIndex < listFolderUnpackedSizes.count()) {
                                        record.nDecompressedSize = listFolderUnpackedSizes.at(nFolderIndex);
                                    } else if (nFolderIndex < listFileUnpackedSizes.count()) {
                                        record.nDecompressedSize = listFileUnpackedSizes.at(nFolderIndex);
                                    } else if (nFolderIndex < listFilePackedSizes.count()) {
                                        // CRITICAL FIX: For COPY (uncompressed) archives, packed size == unpacked size
                                        // This happens with encrypted COPY archives where 7z doesn't store unpacked sizes
                                        record.nDecompressedSize = listFilePackedSizes.at(nFolderIndex);
                                        qDebug() << "[Size Fallback] File" << listFileNames.at(i) 
                                                 << "using packed size as decompressed size:" << record.nDecompressedSize;
                                    } else {
                                        record.nDecompressedSize = 0;
                                    }
                                }
                            } else {
                                record.nDecompressedSize = 0;  // Empty file
                            }
                            
                            // Calculate stream offset and decompressed offset based on folder's solid status
                            // (bFolderIsSolid already determined above)
                            
                            if (bFolderIsSolid) {
                                // SOLID FOLDER: Multiple files compressed into one stream
                                // All files in this folder share the same compressed stream
                                
                                // Calculate folder's stream offset in archive
                                qint64 nFolderStreamOffset = nDataOffset;
                                for (qint32 j = 0; j < nFolderIndex; j++) {
                                    if (j < listFilePackedSizes.count()) {
                                        nFolderStreamOffset += listFilePackedSizes.at(j);
                                    }
                                }
                                
                                // CRITICAL FIX: Empty files have no stream data, even in solid folders
                                if (bIsEmptyFile) {
                                    record.nStreamOffset = 0;  // Empty file has no stream
                                    record.nStreamSize = 0;     // Empty file has no compressed data
                                    record.nDecompressedOffset = 0;
                                } else {
                                    // Non-empty file in solid folder
                                    record.nStreamOffset = nFolderStreamOffset;
                                    record.nStreamSize = (nFolderIndex < listFilePackedSizes.count()) ? listFilePackedSizes.at(nFolderIndex) : 0;
                                    record.nDecompressedOffset = listFolderDecompressedOffset[nFolderIndex];
                                    
                                    // Accumulate offset for next file in this folder
                                    listFolderDecompressedOffset[nFolderIndex] += record.nDecompressedSize;
                                }
                                
                                // Mark file as solid
                                record.mapProperties.insert(FPART_PROP_SOLID, true);
                                
#ifdef QT_DEBUG
                                if (i < 3 || (i >= 62 && i < 65)) {  // Debug first few files and first few of second folder
                                    qDebug() << "[Solid File]" << listFileNames.at(i)
                                             << "Folder:" << nFolderIndex
                                             << "StreamOffset:" << record.nStreamOffset
                                             << "StreamSize:" << record.nStreamSize
                                             << "DecompOffset:" << record.nDecompressedOffset
                                             << "DecompSize:" << record.nDecompressedSize;
                                }
#endif
                            } else if (bIsSolid && nFolderIndex < 0) {
                                // LEGACY: Archive-wide solid (single folder with all files)
                                if (bIsEmptyFile) {
                                    record.nStreamOffset = 0;
                                    record.nStreamSize = 0;
                                } else {
                                    record.nStreamOffset = nSolidStreamOffset;
                                    record.nStreamSize = nSolidStreamSize;
                                }
                                record.nDecompressedOffset = nAccumulatedOffsetSolid;
                                nAccumulatedOffsetSolid += record.nDecompressedSize;
                                record.mapProperties.insert(FPART_PROP_SOLID, true);
                            } else {
                                // NON-SOLID: Each file has its own compressed stream
                                // If we have explicit stream offsets for all files, use them
                                // Otherwise, calculate offsets by accumulating packed sizes
                                
                                // Use folder size as stream size
                                if (nFolderIndex >= 0 && nFolderIndex < listFilePackedSizes.count()) {
                                    record.nStreamSize = listFilePackedSizes.at(nFolderIndex);
                                } else {
                                    record.nStreamSize = record.nDecompressedSize;
                                }
                                
                                if (i < listStreamOffsets.count()) {
                                    record.nStreamOffset = nDataOffset + listStreamOffsets.at(i);
                                    
                                    if (i < 3 && listCodecs.count() > 5) {
                                        qDebug() << "[Non-Solid Offset EXPLICIT]" << listFileNames.at(i) 
                                                 << "offset:" << record.nStreamOffset
                                                 << "nDataOffset:" << nDataOffset
                                                 << "streamOffset:" << listStreamOffsets.at(i)
                                                 << "size:" << record.nStreamSize;
                                    }
                                    // CRITICAL: Accumulate size for explicit offset files too!
                                    // This ensures subsequent calculated offsets start after this file
                                    nAccumulatedOffsetNonSolid += record.nStreamSize;
                                } else {
                                    // CRITICAL FIX: For Copy archives with insufficient stream offsets,
                                    // calculate offset by accumulating previous file packed sizes
                                    record.nStreamOffset = nDataOffset + nAccumulatedOffsetNonSolid;
                                    if (i < 3 && listCodecs.count() > 5) {
                                        qDebug() << "[Non-Solid Offset CALCULATED]" << listFileNames.at(i) 
                                                 << "offset:" << record.nStreamOffset
                                                 << "nDataOffset:" << nDataOffset
                                                 << "accumulated:" << nAccumulatedOffsetNonSolid
                                                 << "size:" << record.nStreamSize;
                                    }
                                    nAccumulatedOffsetNonSolid += record.nStreamSize;
                                }
                                
                                record.nDecompressedOffset = 0;  // Non-solid files don't use decompressed offset
                                record.mapProperties.insert(FPART_PROP_SOLID, false);
                            }
                            
                            record.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, compressMethod);
                            if (filterMethod != COMPRESS_METHOD_UNKNOWN) {
                                record.mapProperties.insert(FPART_PROP_FILTERMETHOD, filterMethod);
                            }
                            
                            // Check if file is encrypted (AES encryption detected in codec chain)
                            if (bIsEncrypted) {
                                record.mapProperties.insert(FPART_PROP_ENCRYPTED, true);
                                
                                // Store AES properties separately
                                qDebug() << "[AES Props Debug] nAesCodecIndex:" << nAesCodecIndex << "listCoderProperties.count():" << listCoderProperties.count();
                                if (nAesCodecIndex >= 0 && nAesCodecIndex < listCoderProperties.count()) {
                                    QByteArray baDebugProps = listCoderProperties.at(nAesCodecIndex);
                                    QString sPropsHex;
                                    for (qint32 i = 0; i < qMin(32, baDebugProps.size()); i++) {
                                        sPropsHex += QString("%1 ").arg((quint8)baDebugProps.at(i), 2, 16, QChar('0'));
                                    }
                                    qDebug() << "[AES Props Debug] Properties at index" << nAesCodecIndex << ":" << sPropsHex << "size:" << baDebugProps.size();
                                    record.mapProperties.insert(FPART_PROP_AESKEY, listCoderProperties.at(nAesCodecIndex));
                                } else {
                                    qDebug() << "[AES Props Debug] Index out of range or no properties";
                                }
                            }
                            
                            // Store compression properties if available (not AES)
                            if (nCompressCodecIndex >= 0 && nCompressCodecIndex < listCoderProperties.count()) {
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, listCoderProperties.at(nCompressCodecIndex));
                            } else if (listCoderProperties.count() > 0 && !bIsEncrypted) {
                                // Fallback for non-encrypted archives
                                record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, listCoderProperties.at(0));
                            }
                            
                            // Note: FPART_PROP_SOLID is already set above based on per-folder solid detection
                            
                            pContext->listArchiveRecords.append(record);
                            pContext->listRecordOffsets.append(record.nStreamOffset);
                        }
                        
                        // Decompress solid block if detected
                        bool bSolidDecompressSuccess = true;  // Track solid block decompression success
                        
                        if (bIsSolid && nSolidStreamSize > 0 && nSolidUnpackSize > 0) {
                            qDebug() << "[Solid Init] Processing solid block...";
                            
                            // Check if solid block is encrypted
                            if (bIsEncrypted) {
                                qDebug() << "[Solid Init] Encrypted solid archive detected";
                            }
                            
                            QString sCodecName = "UNKNOWN";
                            if (compressMethod == COMPRESS_METHOD_LZMA) sCodecName = "LZMA";
                            else if (compressMethod == COMPRESS_METHOD_LZMA2) sCodecName = "LZMA2";
                            else if (compressMethod == COMPRESS_METHOD_BZIP2) sCodecName = "BZIP2";
                            else if (compressMethod == COMPRESS_METHOD_DEFLATE) sCodecName = "DEFLATE";
                            else if (compressMethod == COMPRESS_METHOD_DEFLATE64) sCodecName = "DEFLATE64";
                            else if (compressMethod == COMPRESS_METHOD_PPMD) sCodecName = "PPMD";
                            qDebug() << "[Solid Init]   Compression codec:" << sCodecName;
                            if (bIsEncrypted) qDebug() << "[Solid Init]   Encryption: AES-256";
                            qDebug() << "[Solid Init]   Solid stream offset:" << nSolidStreamOffset;
                            qDebug() << "[Solid Init]   Solid stream size:" << nSolidStreamSize;
                            qDebug() << "[Solid Init]   Expected unpack size:" << nSolidUnpackSize;
                            
                            bSolidDecompressSuccess = false;  // Assume failure until proven otherwise
                            
                            // Step 1: Decrypt if encrypted
                            QIODevice *pDecryptedDevice = nullptr;
                            QByteArray baDecryptedData;
                            SubDevice sd(getDevice(), nSolidStreamOffset, nSolidStreamSize);
                            
                            if (!sd.open(QIODevice::ReadOnly)) {
                                qWarning() << "[Solid Init] Failed to open solid stream SubDevice";
                            } else if (bIsEncrypted) {
                                // Encrypted solid block - decrypt first
                                if (pContext->sPassword.isEmpty()) {
                                    qWarning() << "[Solid Init] Password required for encrypted archive";
                                    sd.close();
                                } else {
                                    // Get AES properties
                                    QByteArray baAesProperties;
                                    if (nAesCodecIndex >= 0 && nAesCodecIndex < listCoderProperties.count()) {
                                        baAesProperties = listCoderProperties.at(nAesCodecIndex);
                                    }
                                    
                                    if (baAesProperties.isEmpty()) {
                                        qWarning() << "[Solid Init] AES properties not found";
                                        sd.close();
                                    } else {
                                        qDebug() << "[Solid Init] Decrypting solid block with AES...";
                                        qDebug() << "[Solid Init]   AES properties size:" << baAesProperties.size();
                                        QString sAesHex;
                                        for (qint32 j = 0; j < baAesProperties.size() && j < 32; j++) {
                                            sAesHex += QString("%1 ").arg((quint8)baAesProperties[j], 2, 16, QChar('0'));
                                        }
                                        qDebug() << "[Solid Init]   AES properties hex:" << sAesHex;
                                        
                                        // In 7z format, IV is stored at the START of the encrypted stream (first 16 bytes)
                                        // Read IV from stream
                                        QByteArray baIV = sd.read(16);
                                        if (baIV.size() != 16) {
                                            qWarning() << "[Solid Init] Failed to read IV from encrypted stream (got" << baIV.size() << "bytes)";
                                            sd.close();
                                        } else {
                                        
                                        QString sIVHex;
                                        for (qint32 j = 0; j < 16; j++) {
                                            sIVHex += QString("%1 ").arg((quint8)baIV[j], 2, 16, QChar('0'));
                                        }
                                        qDebug() << "[Solid Init]   IV (from stream):" << sIVHex;
                                        
                                        // Append IV to properties: NumCyclesPower + SaltSize + Salt + IV
                                        QByteArray baFullProperties = baAesProperties + baIV;
                                        qDebug() << "[Solid Init]   Full properties size:" << baFullProperties.size();
                                        
                                        // Create temporary buffer for decrypted data
                                        QBuffer tempDecryptBuffer;
                                        tempDecryptBuffer.open(QIODevice::WriteOnly);
                                        
                                        XBinary::DATAPROCESS_STATE decryptState = {};
                                        decryptState.pDeviceInput = &sd;
                                        decryptState.pDeviceOutput = &tempDecryptBuffer;
                                        decryptState.nCountInput = 0;
                                        decryptState.nInputOffset = 0;
                                        decryptState.nInputLimit = nSolidStreamSize - 16;  // Subtract IV size
                                        decryptState.nProcessedLimit = nSolidStreamSize - 16;
                                        
                                        // Decrypt (using full properties with appended IV)
                                        qDebug() << "[Solid Init]   Password being used:" << pContext->sPassword << "(" << pContext->sPassword.length() << "chars)";
                                        XAESDecoder aesDecoder;
                                        bool bDecrypted = aesDecoder.decrypt(&decryptState, baFullProperties, pContext->sPassword, pPdStruct);
                                        
                                        sd.close();
                                        
                                        if (!bDecrypted) {
                                            qWarning() << "[Solid Init] AES decryption failed";
                                            tempDecryptBuffer.close();
                                        } else {
                                            baDecryptedData = tempDecryptBuffer.data();
                                            tempDecryptBuffer.close();
                                            
                                            qDebug() << "[Solid Init] AES decryption succeeded:" << baDecryptedData.size() << "bytes";
                                            
                                            // Debug: Show first 32 bytes of decrypted data
                                            QString sDecryptedHex;
                                            for (qint32 j = 0; j < baDecryptedData.size() && j < 32; j++) {
                                                sDecryptedHex += QString("%1 ").arg((quint8)baDecryptedData[j], 2, 16, QChar('0'));
                                            }
                                            qDebug() << "[Solid Init]   Decrypted data (first 32 bytes):" << sDecryptedHex;
                                            
                                            // Create buffer device from decrypted data for decompression
                                            pDecryptedDevice = new QBuffer(&baDecryptedData);
                                            if (!pDecryptedDevice->open(QIODevice::ReadOnly)) {
                                                qWarning() << "[Solid Init] Failed to open decrypted buffer";
                                                delete pDecryptedDevice;
                                                pDecryptedDevice = nullptr;
                                            }
                                        }
                                        }  // End else (IV read successfully)
                                    }
                                }
                            }
                            
                            // Step 2: Decompress (from encrypted or original stream)
                            QIODevice *pInputDevice = bIsEncrypted ? pDecryptedDevice : &sd;
                            qint64 nInputSize = bIsEncrypted ? baDecryptedData.size() : nSolidStreamSize;
                            
                            if (pInputDevice) {
                                // Create buffer device for decompressed data
                                QIODevice *pBufferDevice = createFileBuffer(nSolidUnpackSize, pPdStruct);
                                
                                if (pBufferDevice) {
                                    if (pBufferDevice->open(QIODevice::WriteOnly)) {
                                        bool bDecompressed = false;
                                        
                                        DATAPROCESS_STATE state = {};
                                        state.pDeviceInput = pInputDevice;
                                        state.pDeviceOutput = pBufferDevice;
                                        state.nInputOffset = 0;
                                        state.nInputLimit = nInputSize;
                                        state.nProcessedOffset = 0;
                                        state.nProcessedLimit = -1;
                                        
                                        // Get compression properties (not AES properties)
                                        qDebug() << "[Solid Init]   nCompressCodecIndex:" << nCompressCodecIndex << "listCoderProperties.count():" << listCoderProperties.count();
                                        QByteArray baCompressProperties;
                                        if (nCompressCodecIndex >= 0 && nCompressCodecIndex < listCoderProperties.count()) {
                                            baCompressProperties = listCoderProperties.at(nCompressCodecIndex);
                                            qDebug() << "[Solid Init]   Using compression properties from codec" << nCompressCodecIndex;
                                        } else if (listCoderProperties.count() > 0 && !bIsEncrypted) {
                                            baCompressProperties = listCoderProperties.at(0);
                                            qDebug() << "[Solid Init]   Using fallback properties from codec 0";
                                        } else {
                                            qWarning() << "[Solid Init]   NO COMPRESSION PROPERTIES AVAILABLE!";
                                        }
                                        
                                        qDebug() << "[Solid Init]   Compression properties size:" << baCompressProperties.size();
                                        QString sCompHex;
                                        for (qint32 j = 0; j < baCompressProperties.size() && j < 8; j++) {
                                            sCompHex += QString("%1 ").arg((quint8)baCompressProperties[j], 2, 16, QChar('0'));
                                        }
                                        qDebug() << "[Solid Init]   Compression properties hex:" << sCompHex;
                                        
                                        // Decompress based on method
                                        if (compressMethod == COMPRESS_METHOD_LZMA || compressMethod == COMPRESS_METHOD_LZMA2) {
                                            XLZMADecoder lzmaDecoder;
                                            
                                            if (compressMethod == COMPRESS_METHOD_LZMA) {
                                                bDecompressed = lzmaDecoder.decompress(&state, baCompressProperties, pPdStruct);
                                            } else {
                                                // LZMA2: Convert properties
                                                QByteArray baLzma2Prop;
                                                if (baCompressProperties.size() == 5) {
                                                    quint32 nDictSize = ((quint8)baCompressProperties[1]) |
                                                                       (((quint32)(quint8)baCompressProperties[2]) << 8) |
                                                                       (((quint32)(quint8)baCompressProperties[3]) << 16) |
                                                                       (((quint32)(quint8)baCompressProperties[4]) << 24);
                                                    
                                                    qDebug() << "[LZMA2 Props] Dict size from properties:" << nDictSize;
                                                    
                                                    quint8 nProp = 40;
                                                    for (quint8 p = 0; p <= 40; p++) {
                                                        quint64 nTestDictSize = ((quint64)(2 | (p & 1))) << ((p / 2) + 11);
                                                        if (nTestDictSize >= nDictSize) {
                                                            nProp = p;
                                                            break;
                                                        }
                                                    }
                                                    qDebug() << "[LZMA2 Props] Calculated prop byte:" << nProp;
                                                    baLzma2Prop.append((char)nProp);
                                                } else if (baCompressProperties.size() == 1) {
                                                    qDebug() << "[LZMA2 Props] Using direct property:" << (quint8)baCompressProperties[0];
                                                    baLzma2Prop = baCompressProperties;
                                                }
                                                
                                                bDecompressed = lzmaDecoder.decompressLZMA2(&state, baLzma2Prop, pPdStruct);
                                            }
                                        } else if (compressMethod == COMPRESS_METHOD_BZIP2) {
                                            bDecompressed = XBZIP2Decoder::decompress(&state, pPdStruct);
                                        } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
                                            bDecompressed = XDeflateDecoder::decompress(&state, pPdStruct);
                                        } else if (compressMethod == COMPRESS_METHOD_DEFLATE64) {
                                            bDecompressed = XDeflateDecoder::decompress64(&state, pPdStruct);
                                        } else if (compressMethod == COMPRESS_METHOD_PPMD) {
                                            qWarning() << "[Solid Init] PPMd solid block decompression not yet implemented";
                                            bDecompressed = false;
                                        }
                                        
                                        pBufferDevice->close();
                                        
                                        // Clean up decrypted device if it was created
                                        if (pDecryptedDevice) {
                                            pDecryptedDevice->close();
                                            delete pDecryptedDevice;
                                            pDecryptedDevice = nullptr;
                                        }
                                        
                                        if (bDecompressed) {
                                            // Read all data from buffer device and store in cache
                                            pBufferDevice->open(QIODevice::ReadOnly);
                                            QByteArray baDecompressed = pBufferDevice->readAll();
                                            pBufferDevice->close();
                                            
                                            qDebug() << "[Solid Result] Decompression succeeded, buffer size:" << baDecompressed.size() << "expected:" << nSolidUnpackSize;
                                            
                                            if (baDecompressed.size() == nSolidUnpackSize) {
                                                pContext->mapFolderCache.insert(0, baDecompressed);
                                                bSolidDecompressSuccess = true;
                                                qDebug() << "[Solid Result] SUCCESS - Solid block cached";
                                            } else {
                                                qWarning() << "[Solid Result] FAIL - Size mismatch: expected" << nSolidUnpackSize 
                                                          << "got" << baDecompressed.size();
                                            }
                                        } else {
                                            qWarning() << "[Solid Result] FAIL - Decompression returned false";
                                        }
                                    }
                                    
                                    // Free the buffer device
                                    freeFileBuffer(&pBufferDevice);
                                } else {
                                    qWarning() << "[Solid Init] Failed to create buffer device";
                                }
                                
                                // Close SubDevice if not encrypted (encrypted case already closed)
                                if (!bIsEncrypted && sd.isOpen()) {
                                    sd.close();
                                }
                            } else {
                                qWarning() << "[Solid Init] Input device not available";
                            }
                        } else {
#ifdef QT_DEBUG
                                qWarning() << "Failed to allocate buffer for solid block:" << nSolidUnpackSize << "bytes";
#endif
                            }
                            
                            // If solid decompression failed, keep records but don't cache solid block
                            // Individual file extraction will fail gracefully if password is needed
                            if (!bSolidDecompressSuccess) {
#ifdef QT_DEBUG
                                qWarning() << "[Solid Init] Solid block decompression skipped (password needed or error)";
                                qWarning() << "[Solid Init] Records preserved - extraction will fail at file level if password missing";
#endif
                            }
                        }
                        
#ifdef QT_DEBUG
                        qDebug() << "XSevenZip::initUnpack: Created" << pContext->listArchiveRecords.count() << "archive records";
#endif
                    }
                }

                //                 if (bIsEncodedHeader) {
                // // === ENCODED HEADER PATH ===
                // // TODO: Full EncodedHeader support is partially implemented
                // // The current implementation can parse EncodedHeader metadata but may fail to locate
                // // and decompress the actual header data for certain archive configurations.
                // // Issues identified:
                // // 1. Archives created with -mx=0 (store mode) use non-standard header storage
                // // 2. PackOffset in EncodedHeader may point to file data instead of compressed header
                // // 3. Compressed header location calculation formula unclear for all cases
                // // Reference: 7-Zip SDK CPP/7zip/Archive/7z/7zIn.cpp - CInArchive::ReadAndDecodePackedStreams()
                // // Status: Works for some archives, fails for system 7z with -mx=0

                // #ifdef QT_DEBUG
                //                     qDebug() << "XSevenZip::initUnpack: Processing encoded header (experimental support)...";
                // #endif
                // #ifdef QT_DEBUG
                //                     qDebug() << "  Warning: EncodedHeader decompression may fail for archives created with -mx=0";
                // #endif

                //                     // Parse encoded header metadata using SZSTATE to get compressed data offset
                //                     SZSTATE szState = {};
                //                     szState.pData = pData;
                //                     szState.nSize = nNextHeaderSize;
                //                     szState.nCurrentOffset = 0;
                //                     szState.bIsError = false;
                //                     szState.sErrorString = QString();

                //                     // Skip the k7zIdEncodedHeader byte (0x17)
                //                     quint8 nEncodedId = (quint8)pData[0];
                //                     szState.nCurrentOffset = 1;
                // #ifdef QT_DEBUG
                //                     qDebug() << "  Read EncodedHeader ID:" << QString::number(nEncodedId, 16);
                // #endif

                //                     // The encoded header contains MainStreamsInfo (NOT a full Header structure)
                //                     // MainStreamsInfo structure: PackInfo + UnpackInfo + SubStreamsInfo
                //                     // Parse this to find where the compressed LZMA data starts
                //                     QList<SZRECORD> listMetadataParse;
                //                     _handleId(&listMetadataParse, k7zIdMainStreamsInfo, &szState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);

                // #ifdef QT_DEBUG
                //                     qDebug() << "  After parsing MainStreamsInfo, offset=" << QString::number(szState.nCurrentOffset, 16)
                //                              << "(" << szState.nCurrentOffset << " bytes)";
                // #endif

                // // CRITICAL: Extract PackPos directly from EncodedHeader byte-by-byte
                // // We MUST do this BEFORE parsing records, because parsed STREAMOFFSET records
                // // may contain offsets from the decompressed header's PackInfo (file data locations),
                // // not the compressed header location!
                // //
                // // EncodedHeader structure: k7zIdEncodedHeader (0x17) + MainStreamsInfo
                // // MainStreamsInfo starts with: k7zIdPackInfo (0x06) + PackPos (encoded uint64)

                // #ifdef QT_DEBUG
                //                     qDebug() << "  Extracting PackPos directly from EncodedHeader bytes...";
                // #endif

                //                     qint64 nPackPos = 0;
                //                     qint32 nPos = 1;  // Start after 0x17 (k7zIdEncodedHeader)
                //                     qint32 nPackPosBytesRead = 0;  // Track how many bytes PackPos used

                //                     if (nPos < nNextHeaderSize && (quint8)pData[nPos] == 0x06) {  // k7zIdPackInfo
                // #ifdef QT_DEBUG
                //                         qDebug() << "  Found k7zIdPackInfo at byte" << nPos;
                // #endif
                //                         nPos++;

                //                         // Debug: print next 10 bytes to see the encoded number
                //                         QString sDebugBytes;
                //                         for (qint32 j = 0; j < qMin((qint64)10, nNextHeaderSize - nPos); j++) {
                //                             sDebugBytes += QString("%1 ").arg((quint8)pData[nPos + j], 2, 16, QChar('0'));
                //                         }
                // #ifdef QT_DEBUG
                //                         qDebug() << "  Next bytes after PackInfo:" << sDebugBytes;
                // #endif

                //                         // Read PackPos as encoded uint64
                //                         quint8 nFirstByte = (quint8)pData[nPos];
                //                         quint8 nMask = 0x80;

                // #ifdef QT_DEBUG
                //                         qDebug() << "  First byte:" << QString("0x%1").arg(nFirstByte, 2, 16, QChar('0'));
                // #endif

                //                         for (qint32 i = 0; i < 8; i++) {
                //                             if (nFirstByte & nMask) {
                //                                 if (nPos + nPackPosBytesRead + 1 < nNextHeaderSize) {
                //                                     quint64 nByte = (quint8)pData[nPos + nPackPosBytesRead + 1];
                // #ifdef QT_DEBUG
                //                                     qDebug() << "    Bit" << i << "set, reading byte:" << QString("0x%1").arg(nByte, 2, 16, QChar('0'))
                //                                              << "shift" << (8*i);
                // #endif
                //                                     nPackPos |= (nByte << (8 * i));
                //                                     nPackPosBytesRead++;
                //                                 }
                //                             } else {
                //                                 quint64 nValue = (nFirstByte & (nMask - 1));
                // #ifdef QT_DEBUG
                //                                 qDebug() << "    Bit" << i << "NOT set, using remaining bits:" << QString("0x%1").arg(nValue, 2, 16, QChar('0'))
                //                                          << "shift" << (8*i);
                // #endif
                //                                 nPackPos |= (nValue << (8 * i));
                //                                 break;
                //                             }
                //                             nMask >>= 1;
                //                         }

                // #ifdef QT_DEBUG
                //                         qDebug() << "  ✓ PackPos extracted:" << nPackPos << "using:" << (nPackPosBytesRead + 1) << "bytes";
                // #endif
                // #ifdef QT_DEBUG
                //                         qDebug() << "  ✓ Absolute file offset:" << (sizeof(SIGNATUREHEADER) + nPackPos);
                // #endif
                //                     } else {
                // #ifdef QT_DEBUG
                //                         qWarning() << "  ERROR: EncodedHeader doesn't start with PackInfo!";
                // #endif
                // #ifdef QT_DEBUG
                //                         qWarning() << "  Byte 1 = 0x" << QString::number((quint8)pData[nPos], 16);
                // #endif
                //                     }

                //                     // Continue parsing to extract StreamSize and CodersUnpackSize manually
                //                     // These values are CRITICAL and must come from the EncodedHeader's PackInfo/UnpackInfo,
                //                     // NOT from parsed records which may contain wrong values from other sections!

                //                     qint64 nPackedSize = 0;
                //                     qint64 nUnpackedSize = 0;

                //                     if (nPackPos > 0) {
                //                         // Parse NumPackStreams - nPos should already be after the PackPos encoding
                //                         nPos += nPackPosBytesRead + 1;  // Skip the bytes we already read for PackPos

                //                         quint8 nNumStreams = (quint8)pData[nPos];
                //                         nPos++;

                // #ifdef QT_DEBUG
                //                         qDebug() << "  NumPackStreams:" << nNumStreams << "at position" << nPos;
                // #endif

                //                         // Look for k7zIdSize (0x09)
                //                         if (nPos < nNextHeaderSize && (quint8)pData[nPos] == 0x09) {
                //                             nPos++;  // Skip k7zIdSize

                //                             // Read stream size (encoded number)
                //                             quint8 nFirstByte = (quint8)pData[nPos];
                //                             if (nFirstByte < 0x80) {
                //                                 // Single byte
                //                                 nPackedSize = nFirstByte;
                //                                 nPos++;
                //                             } else {
                //                                 // Multi-byte encoding
                //                                 quint8 nMask = 0x80;
                //                                 qint32 nBytesRead = 0;

                //                                 for (qint32 i = 0; i < 8; i++) {
                //                                     if (nFirstByte & nMask) {
                //                                         if (nPos + nBytesRead + 1 < nNextHeaderSize) {
                //                                             nPackedSize |= ((quint64)(quint8)pData[nPos + nBytesRead + 1] << (8 * i));
                //                                             nBytesRead++;
                //                                         }
                //                                     } else {
                //                                         nPackedSize |= ((quint64)(nFirstByte & (nMask - 1)) << (8 * i));
                //                                         break;
                //                                     }
                //                                     nMask >>= 1;
                //                                 }
                //                                 nPos += nBytesRead + 1;
                //                             }

                // #ifdef QT_DEBUG
                //                             qDebug() << "  ✓ StreamSize (compressed) extracted:" << nPackedSize;
                // #endif
                //                         }

                //                         // Now find k7zIdCodersUnpackSize (0x0c) to get the decompressed size
                //                         // Skip forward to find it
                //                         while (nPos < nNextHeaderSize - 1 && (quint8)pData[nPos] != 0x0c) {
                //                             nPos++;
                //                         }

                //                         if (nPos < nNextHeaderSize && (quint8)pData[nPos] == 0x0c) {
                //                             nPos++;  // Skip k7zIdCodersUnpackSize

                //                             // Read unpack size (encoded number)
                //                             quint8 nFirstByte = (quint8)pData[nPos];
                //                             if (nFirstByte < 0x80) {
                //                                 // Single byte
                //                                 nUnpackedSize = nFirstByte;
                //                             } else {
                //                                 // Multi-byte encoding
                //                                 quint8 nMask = 0x80;
                //                                 qint32 nBytesRead = 0;

                //                                 for (qint32 i = 0; i < 8; i++) {
                //                                     if (nFirstByte & nMask) {
                //                                         if (nPos + nBytesRead + 1 < nNextHeaderSize) {
                //                                             nUnpackedSize |= ((quint64)(quint8)pData[nPos + nBytesRead + 1] << (8 * i));
                //                                             nBytesRead++;
                //                                         }
                //                                     } else {
                //                                         nUnpackedSize |= ((quint64)(nFirstByte & (nMask - 1)) << (8 * i));
                //                                         break;
                //                                     }
                //                                     nMask >>= 1;
                //                                 }
                //                             }

                // #ifdef QT_DEBUG
                //                             qDebug() << "  ✓ CodersUnpackSize (decompressed) extracted:" << nUnpackedSize;
                // #endif
                //                         }
                //                     }

                //                     // Now parse the full EncodedHeader structure to get codec info
                //                     QList<SZRECORD> listEncodedRecords = _handleData(pData, nNextHeaderSize, pPdStruct);
                // #ifdef QT_DEBUG
                //                     qDebug() << "  Encoded header metadata records:" << listEncodedRecords.count();
                // #endif

                //                     // Debug: print first few records
                //                     for (qint32 i = 0; i < qMin(listEncodedRecords.count(), 10); i++) {
                //                         const SZRECORD &rec = listEncodedRecords.at(i);
                // #ifdef QT_DEBUG
                //                         qDebug() << "    Record" << i << "- srType:" << rec.srType << "impType:" << rec.impType
                //                                  << "value:" << rec.varValue;
                // #endif
                //                     }

                //                     qint64 nPackOffset = sizeof(SIGNATUREHEADER) + nPackPos;  // Use manually extracted PackPos!
                //                     QByteArray baCodec;
                //                     QByteArray baProperty;

                //                     // Extract codec information from parsed records
                //                     for (qint32 i = 0; i < listEncodedRecords.count(); i++) {
                //                         SZRECORD rec = listEncodedRecords.at(i);
                //                         if (rec.impType == IMPTYPE_CODER) {
                //                             baCodec = rec.varValue.toByteArray();
                //                         } else if (rec.impType == IMPTYPE_CODERPROPERTY) {
                //                             baProperty = rec.varValue.toByteArray();
                //                         }
                //                     }

                // #ifdef QT_DEBUG
                //                     qDebug() << "  PackOffset (absolute):" << nPackOffset << "PackedSize:" << nPackedSize
                //                              << "UnpackedSize:" << nUnpackedSize << "Codec size:" << baCodec.size() << "Property size:" << baProperty.size();
                // #endif

                // // The compressed header stream is located at nPackOffset with size nPackedSize
                // // It decompresses to nUnpackedSize bytes
                // #ifdef QT_DEBUG
                //                     qDebug() << "  Compressed header location: offset" << nPackOffset << "size" << nPackedSize;
                // #endif
                // #ifdef QT_DEBUG
                //                     qDebug() << "  Will decompress to" << nUnpackedSize << "bytes";
                // #endif

                //                     // Debug: Print codec bytes
                //                     if (!baCodec.isEmpty()) {
                //                         QString sCodecHex;
                //                         for (qint32 j = 0; j < baCodec.size(); j++) {
                //                             sCodecHex += QString("%1 ").arg((unsigned char)baCodec.at(j), 2, 16, QChar('0'));
                //                         }
                // #ifdef QT_DEBUG
                //                         qDebug() << "  Codec bytes:" << sCodecHex;
                // #endif
                // #ifdef QT_DEBUG
                //                         qDebug() << "  Codec method:" << codecToCompressMethod(baCodec);
                // #endif
                //                     }

                //                     // Debug: Print coder property bytes in detail
                //                     if (!baProperty.isEmpty()) {
                //                         QString sPropHex;
                //                         for (qint32 j = 0; j < baProperty.size(); j++) {
                //                             sPropHex += QString("%1 ").arg((unsigned char)baProperty.at(j), 2, 16, QChar('0'));
                //                         }
                // #ifdef QT_DEBUG
                //                         qDebug() << "  Coder property bytes:" << sPropHex;
                // #endif

                //                         // Parse LZMA properties
                //                         if (baProperty.size() == 5) {
                //                             quint8 propByte = (quint8)baProperty[0];
                //                             quint32 dictSize = ((quint8)baProperty[1]) |
                //                                                ((quint8)baProperty[2] << 8) |
                //                                                ((quint8)baProperty[3] << 16) |
                //                                                ((quint8)baProperty[4] << 24);
                // #ifdef QT_DEBUG
                //                             qDebug() << "    Properties byte:" << QString::number(propByte, 16)
                //                                      << "Dictionary size:" << dictSize << "bytes (" << (dictSize/1024) << "KB)";
                // #endif
                //                         }
                //                     }

                // #ifdef QT_DEBUG
                //                     qDebug() << "  About to decompress header from offset" << nPackOffset << "size" << nPackedSize << "expected unpacked size" <<
                //                     nUnpackedSize;
                // #endif

                //                     // Validate we have the necessary data
                //                     if (nPackOffset <= 0 || nPackedSize <= 0 || nUnpackedSize <= 0 || baProperty.isEmpty()) {
                // #ifdef QT_DEBUG
                //                         qDebug() << "  ERROR: Missing required decompression parameters";
                // #endif
                //                         bResult = false;
                //                     } else {
                //                         // Debug: Read and print first 32 bytes at compressed header offset to verify it's LZMA data
                //                         QByteArray baTestRead = read_array(nPackOffset, qMin((qint64)32, nPackedSize));
                //                         QString sTestHex;
                //                         for (qint32 j = 0; j < baTestRead.size(); j++) {
                //                             sTestHex += QString("%1 ").arg((unsigned char)baTestRead.at(j), 2, 16, QChar('0'));
                //                         }
                // #ifdef QT_DEBUG
                //                         qDebug() << "  First bytes at compressed header offset:" << sTestHex;
                // #endif

                //                         // Use SubDevice to create a view of the compressed data
                //                         SubDevice subDevice(getDevice(), nPackOffset, nPackedSize);
                //                         if (subDevice.open(QIODevice::ReadOnly)) {
                // #ifdef QT_DEBUG
                //                             qDebug() << "  SubDevice opened successfully at offset" << nPackOffset;
                // #endif

                //                             QByteArray baDecompressed;
                //                             QBuffer bufferDecompressed(&baDecompressed);
                //                             bufferDecompressed.open(QIODevice::WriteOnly);

                //                             // Decompress using XLZMADecoder with properties from EncodedHeader
                //                             XLZMADecoder lzmaDecoder;

                //                             DATAPROCESS_STATE state = {};
                //                             state.pDeviceInput = &subDevice;
                //                             state.pDeviceOutput = &bufferDecompressed;
                //                             state.nInputOffset = 0;  // Relative to SubDevice
                //                             state.nInputLimit = nPackedSize;
                //                             state.nProcessedOffset = 0;
                //                             state.nProcessedLimit = -1;  // Decompress until end of stream

                // #ifdef QT_DEBUG
                //                             qDebug() << "  Calling LZMA decoder with properties size" << baProperty.size();
                // #endif
                // #ifdef QT_DEBUG
                //                             qDebug() << "  Expected unpacked size:" << nUnpackedSize << "Packed size:" << nPackedSize;
                // #endif

                //                             bool bDecompressed = lzmaDecoder.decompress(&state, baProperty, pPdStruct);

                //                             subDevice.close();
                //                             bufferDecompressed.close();

                // #ifdef QT_DEBUG
                //                             qDebug() << "  Decompression result:" << bDecompressed << "Size:" << baDecompressed.size();
                // #endif
                // #ifdef QT_DEBUG
                //                             qDebug() << "  Input consumed:" << state.nCountInput << "Output produced:" << state.nCountOutput;
                // #endif
                //                             if (baDecompressed.size() != nUnpackedSize) {
                // #ifdef QT_DEBUG
                //                                 qDebug() << "  WARNING: Size mismatch! Expected:" << nUnpackedSize << "got:" << baDecompressed.size();
                // #endif
                //                             }

                //                             // FALLBACK: If LZMA decompression failed, try reading UNCOMPRESSED header
                //                             // This handles archives created with -mx=0 where file data is stored but header might not be compressed
                //                             if (!bDecompressed || baDecompressed.isEmpty()) {
                // #ifdef QT_DEBUG
                //                                 qDebug() << "  LZMA decompression failed, trying UNCOMPRESSED header...";
                // #endif

                //                                 // Try reading UnpackedSize bytes before NextHeader
                //                                 // Distance observed: approximately 198 bytes before NextHeader start
                //                                 qint64 nNextHeaderAbsolute = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
                //                                 qint64 nUncompressedHeaderOffset = nNextHeaderAbsolute - nUnpackedSize - signatureHeader.NextHeaderSize;

                // #ifdef QT_DEBUG
                //                                 qDebug() << "  Trying uncompressed header at offset" << nUncompressedHeaderOffset;
                // #endif
                // #ifdef QT_DEBUG
                //                                 qDebug() << "  Calculation: NextHeader(" << nNextHeaderAbsolute << ") - UnpackedSize(" << nUnpackedSize << ") -
                //                                 NextHeaderSize(" << signatureHeader.NextHeaderSize << ")";
                // #endif

                //                                 // Try multiple offsets around the calculated position
                //                                 QList<qint64> listOffsetsToTry;
                //                                 listOffsetsToTry << nUncompressedHeaderOffset;
                //                                 listOffsetsToTry << (nNextHeaderAbsolute - nUnpackedSize - 20);
                //                                 listOffsetsToTry << (nNextHeaderAbsolute - nUnpackedSize - 15);
                //                                 listOffsetsToTry << (nNextHeaderAbsolute - nUnpackedSize);
                //                                 listOffsetsToTry << (nNextHeaderAbsolute - 198);  // Observed distance

                //                                 for (qint32 iTry = 0; iTry < listOffsetsToTry.count(); iTry++) {
                //                                     qint64 nTryOffset = listOffsetsToTry.at(iTry);
                //                                     if (nTryOffset < 0 || nTryOffset + nUnpackedSize > getSize()) {
                //                                         continue;
                //                                     }

                //                                     QByteArray baUncompressed = read_array(nTryOffset, nUnpackedSize);
                //                                     if (baUncompressed.size() == nUnpackedSize && !baUncompressed.isEmpty()) {
                //                                         // Check if it starts with k7zIdHeader (0x01)
                //                                         if ((quint8)baUncompressed.at(0) == 0x01) {
                // #ifdef QT_DEBUG
                //                                             qDebug() << "  ✓ Found valid uncompressed header at offset" << nTryOffset << "(attempt" << iTry << ")";
                // #endif
                // #ifdef QT_DEBUG
                //                                             qDebug() << "  First 32 bytes:" << baUncompressed.left(32).toHex(' ');
                // #endif
                //                                             baDecompressed = baUncompressed;
                //                                             bDecompressed = true;
                //                                             break;
                //                                         }
                //                                     }
                //                                 }

                //                                 if (!bDecompressed) {
                // #ifdef QT_DEBUG
                //                                     qDebug() << "  ✗ Could not find valid uncompressed header at any tried offset";
                // #endif
                //                                 }
                //                             }

                //                             // Debug: Print first 32 bytes of decompressed header
                //                             if (!baDecompressed.isEmpty()) {
                //                                 QString sDecompressedHex;
                //                                 qint32 nBytesToShow = qMin(32, baDecompressed.size());
                //                                 for (qint32 j = 0; j < nBytesToShow; j++) {
                //                                     sDecompressedHex += QString("%1 ").arg((unsigned char)baDecompressed.at(j), 2, 16, QChar('0'));
                //                                 }
                // #ifdef QT_DEBUG
                //                                 qDebug() << "  First" << nBytesToShow << "bytes of decompressed header:" << sDecompressedHex;
                // #endif

                //                                 // Print ALL bytes for analysis
                //                                 QString sFullHex;
                //                                 for (qint32 j = 0; j < baDecompressed.size(); j++) {
                //                                     if (j > 0 && j % 32 == 0) sFullHex += "\n      ";
                //                                     sFullHex += QString("%1 ").arg((unsigned char)baDecompressed.at(j), 2, 16, QChar('0'));
                //                                 }
                // #ifdef QT_DEBUG
                //                                 qDebug() << "  FULL decompressed header (" << baDecompressed.size() << "bytes):\n     " << sFullHex;
                // #endif
                //                             }

                //                             QList<SZRECORD> listHeaderRecords;

                //                             if (bDecompressed && !baDecompressed.isEmpty()) {
                // #ifdef QT_DEBUG
                //                                 qDebug() << "  Parsing decompressed header: size=" << baDecompressed.size();
                //                                 qDebug() << "  First bytes:" << QString("0x%1 0x%2 0x%3 0x%4").arg((quint8)baDecompressed[0], 2, 16,
                //                                 QChar('0')).arg((quint8)baDecompressed[1], 2, 16, QChar('0')).arg((quint8)baDecompressed[2], 2, 16,
                //                                 QChar('0')).arg((quint8)baDecompressed[3], 2, 16, QChar('0'));
                // #endif

                //                                 SZSTATE stateDecompressed = {};
                //                                 stateDecompressed.pData = baDecompressed.data();
                //                                 stateDecompressed.nSize = baDecompressed.size();
                //                                 stateDecompressed.nCurrentOffset = 0;
                //                                 stateDecompressed.bIsError = false;
                //                                 stateDecompressed.sErrorString = QString();

                //                                 _handleId(&listHeaderRecords, XSevenZip::k7zIdHeader, &stateDecompressed, 1, false, pPdStruct, IMPTYPE_UNKNOWN);

                // #ifdef QT_DEBUG
                //                                 qDebug() << "  Decompressed header records:" << listHeaderRecords.count();
                //                                 qDebug() << "  Full decompressed header hex (first 100 bytes):" << baDecompressed.left(100).toHex(' ');

                //                                 // Debug: Count CodersUnpackSize records
                //                                 qint32 nCodersUnpackSizeCount = 0;
                //                                 for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                //                                     if (listHeaderRecords.at(i).sName.contains("CodersUnpackSize")) {
                //                                         nCodersUnpackSizeCount++;
                //                                         qDebug() << "    Record" << i << ":" << listHeaderRecords.at(i).sName << "=" <<
                //                                         listHeaderRecords.at(i).varValue.toLongLong();
                //                                     }
                //                                 }
                //                                 qDebug() << "  Total CodersUnpackSize records:" << nCodersUnpackSizeCount;
                // #endif

                //                                 // Debug: Print all record types
                //                                 QMap<qint32, qint32> mapRecordTypeCounts;
                //                                 for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                //                                     mapRecordTypeCounts[listHeaderRecords.at(i).impType]++;
                //                                 }
                // #ifdef QT_DEBUG
                //                                 qDebug() << "  Record type distribution:";
                // #endif
                //                                 QMapIterator<qint32, qint32> it(mapRecordTypeCounts);
                //                                 while (it.hasNext()) {
                //                                     it.next();
                // #ifdef QT_DEBUG
                //                                     qDebug() << "    Type" << it.key() << "=" << it.value() << "occurrences";
                // #endif
                //                                 }

                //                                 // Extract file information from decompressed header
                //                                 QList<QString> listFileNames;
                //                                 QList<qint64> listFilePackedSizes;
                //                                 QList<qint64> listFolderUnpackedSizes;  // Folder sizes from CodersUnpackSize
                //                                 QList<qint64> listStreamOffsets;
                //                                 QList<QByteArray> listCodecs;
                //                                 qint32 nNumberOfFolders = 0;  // Track number of folders
                //                                 QList<qint32> listFilesPerFolder;  // How many files in each folder (from NumUnpackStream)

                //                                 // First pass: collect folder info and per-folder file counts
                //                                 for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                //                                     SZRECORD rec = listHeaderRecords.at(i);

                //                                     if (rec.sName == "NumberOfFolders") {
                //                                         nNumberOfFolders = rec.varValue.toInt();
                //                                     } else if (rec.sName.startsWith("NumUnpackStream")) {
                //                                         listFilesPerFolder.append(rec.varValue.toInt());
                //                                     }
                //                                 }

                // // Second pass: collect all data
                // #ifdef QT_DEBUG
                //                                 qDebug() << "Collection pass: Processing" << listHeaderRecords.count() << "records";
                // #endif
                //                                 for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                //                                     SZRECORD rec = listHeaderRecords.at(i);

                // #ifdef QT_DEBUG
                //                                     if (rec.sName.startsWith("Size")) {
                //                                         qDebug() << "  DEBUG: Found record" << rec.sName << "with impType=" << rec.impType
                //                                                  << "(STREAMUNPACKEDSIZE=" << IMPTYPE_STREAMUNPACKEDSIZE
                //                                                  << ", STREAMPACKEDSIZE=" << IMPTYPE_STREAMPACKEDSIZE << ")"
                //                                                  << "value=" << rec.varValue.toLongLong();
                //                                     }
                // #endif

                //                                     if (rec.impType == IMPTYPE_FILENAME) {
                //                                         listFileNames.append(rec.varValue.toString());
                //                                     } else if (rec.impType == IMPTYPE_FILEPACKEDSIZE) {
                //                                         listFilePackedSizes.append(rec.varValue.toLongLong());
                //                                     } else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE || rec.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                //                                         // File sizes can come from either FILEUNPACKEDSIZE or STREAMUNPACKEDSIZE (SubStreamsInfo)
                //                                         listSubStreamSizes.append(rec.varValue.toLongLong());
                //                                     } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                //                                         listStreamOffsets.append(rec.varValue.toLongLong());
                //                                     } else if (rec.impType == IMPTYPE_CODER) {
                //                                         listCodecs.append(rec.varValue.toByteArray());
                //                                     }
                //                                 }

                //                                 // Calculate actual file sizes from folder sizes and SubStreamsInfo
                //                                 QList<qint64> listFileUnpackedSizes;
                //                                 qint32 nSubStreamIndex = 0;

                // #ifdef QT_DEBUG
                //                                 qDebug() << "Starting file size calculation:";
                //                                 qDebug() << "  nNumberOfFolders:" << nNumberOfFolders;
                //                                 qDebug() << "  listFilesPerFolder:" << listFilesPerFolder;
                //                                 qDebug() << "  listFolderUnpackedSizes:" << listFolderUnpackedSizes;
                //                                 qDebug() << "  listSubStreamSizes count:" << listSubStreamSizes.count();
                // #endif

                //                                 for (qint32 iFolderIdx = 0; iFolderIdx < nNumberOfFolders; iFolderIdx++) {
                //                                     qint32 nFilesInFolder = (iFolderIdx < listFilesPerFolder.count()) ? listFilesPerFolder.at(iFolderIdx) : 1;
                //                                     qint64 nFolderSize = (iFolderIdx < listFolderUnpackedSizes.count()) ? listFolderUnpackedSizes.at(iFolderIdx) : 0;

                // #ifdef QT_DEBUG
                //                                     qDebug() << "  Folder" << iFolderIdx << ": nFilesInFolder=" << nFilesInFolder << ", nFolderSize=" << nFolderSize;
                // #endif

                //                                     if (nFilesInFolder == 1) {
                //                                         // Simple case: folder has only one file, use folder size
                //                                         listFileUnpackedSizes.append(nFolderSize);
                // #ifdef QT_DEBUG
                //                                         qDebug() << "    Single file folder, size:" << nFolderSize;
                // #endif
                //                                     } else if (nFilesInFolder > 1) {
                //                                         // Solid block: multiple files in folder
                //                                         // SubStreamsInfo Size values are the unpacked sizes of the FIRST (N-1) files
                //                                         // The LAST file size = folder size - sum(first N-1 files)
                //                                         qint64 nFirstFilesTotal = 0;

                // #ifdef QT_DEBUG
                //                                         qDebug() << "    Multi-file folder, reading" << (nFilesInFolder - 1) << "sizes from SubStreamsInfo";
                // #endif

                //                                         // Add sizes for first (N-1) files from SubStreamsInfo
                //                                         for (qint32 j = 0; j < nFilesInFolder - 1; j++) {
                //                                             if (nSubStreamIndex < listSubStreamSizes.count()) {
                //                                                 qint64 nFileSize = listSubStreamSizes.at(nSubStreamIndex);
                //                                                 listFileUnpackedSizes.append(nFileSize);
                //                                                 nFirstFilesTotal += nFileSize;
                // #ifdef QT_DEBUG
                //                                                 qDebug() << "      File" << j << "size:" << nFileSize << "(from SubStreamSizes[" << nSubStreamIndex <<
                //                                                 "])";
                // #endif
                //                                                 nSubStreamIndex++;
                //                                             } else {
                // #ifdef QT_DEBUG
                //                                                 qDebug() << "      WARNING: Not enough SubStreamSizes! Expected index" << nSubStreamIndex << "but count
                //                                                 is" << listSubStreamSizes.count();
                // #endif
                //                                             }
                //                                         }

                //                                         // Calculate last file size as remainder
                //                                         qint64 nLastFileSize = nFolderSize - nFirstFilesTotal;
                //                                         listFileUnpackedSizes.append(nLastFileSize);
                // #ifdef QT_DEBUG
                //                                         qDebug() << "      File" << (nFilesInFolder - 1) << "size:" << nLastFileSize << "(calculated as" << nFolderSize
                //                                         << "-" << nFirstFilesTotal << ")";
                // #endif
                //                                     }
                //                                 }

                // #ifdef QT_DEBUG
                //                                 qDebug() << "  Files found:" << listFileNames.count();
                //                                 qDebug() << "  nNumberOfFolders:" << nNumberOfFolders;
                //                                 qDebug() << "  listFilesPerFolder:" << listFilesPerFolder;
                //                                 qDebug() << "  listFolderUnpackedSizes:" << listFolderUnpackedSizes;
                //                                 qDebug() << "  listSubStreamSizes:" << listSubStreamSizes;
                //                                 qDebug() << "  Calculated listFileUnpackedSizes:" << listFileUnpackedSizes;
                //                                 qDebug() << "  listFilePackedSizes:" << listFilePackedSizes;
                //                                 qDebug() << "  listStreamOffsets:" << listStreamOffsets;

                //                                 // Validation: Check if file count matches calculated size count
                //                                 if (listFileUnpackedSizes.count() != listFileNames.count()) {
                //                                     qDebug() << "  WARNING: Size count mismatch! Files:" << listFileNames.count()
                //                                     << "Calculated sizes:" << listFileUnpackedSizes.count();
                //                                 }
                // #endif

                //                                 // Analyze codec chain to separate filters from compressors
                //                                 // In 7z, filters (like BCJ) come before compressors (like LZMA2)
                //                                 COMPRESS_METHOD filterMethod = COMPRESS_METHOD_UNKNOWN;
                //                                 COMPRESS_METHOD compressMethod = COMPRESS_METHOD_STORE;

                //                                 for (qint32 i = 0; i < listCodecs.count(); i++) {
                //                                     COMPRESS_METHOD method = codecToCompressMethod(listCodecs.at(i));

                //                                     // BCJ and BCJ2 are filters/preprocessors, not compressors
                //                                     if (method == COMPRESS_METHOD_BCJ || method == COMPRESS_METHOD_BCJ2) {
                //                                         filterMethod = method;
                //                                     } else if (method != COMPRESS_METHOD_UNKNOWN) {
                //                                         // This is the actual compressor (LZMA, LZMA2, etc.)
                //                                         compressMethod = method;
                //                                     }
                //                                 }

                //                                 // Create ARCHIVERECORD entries
                //                                 qint64 nDataOffset = sizeof(SIGNATUREHEADER);
                //                                 for (qint32 i = 0; i < listFileNames.count(); i++) {
                //                                     ARCHIVERECORD record = {};
                //                                     record.mapProperties.insert(FPART_PROP_ORIGINALNAME, listFileNames.at(i));

                //                                     if (i < listFileUnpackedSizes.count()) {
                //                                         record.nDecompressedSize = listFileUnpackedSizes.at(i);
                //                                     }
                //                                     if (i < listFilePackedSizes.count()) {
                //                                         record.nStreamSize = listFilePackedSizes.at(i);
                //                                     } else {
                //                                         record.nStreamSize = record.nDecompressedSize;
                //                                     }
                //                                     if (i < listStreamOffsets.count()) {
                //                                         record.nStreamOffset = nDataOffset + listStreamOffsets.at(i);
                //                                     } else {
                //                                         record.nStreamOffset = nDataOffset;
                //                                     }

                //                                     // Store both compressor and filter (if present)
                //                                     record.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, compressMethod);
                //                                     if (filterMethod != COMPRESS_METHOD_UNKNOWN) {
                //                                         record.mapProperties.insert(FPART_PROP_FILTERMETHOD, filterMethod);
                //                                     }

                //                                     pContext->listArchiveRecords.append(record);
                //                                     pContext->listRecordOffsets.append(record.nStreamOffset);
                //                                 }

                // #ifdef QT_DEBUG
                //                                 qDebug() << "  Created" << pContext->listArchiveRecords.count() << "archive records";
                // #endif

                //                                 if (pContext->listArchiveRecords.isEmpty()) {
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  ======================================";
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  ENCODED HEADER ANALYSIS:";
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  Decompressed header contains only MainStreamsInfo (stream metadata)";
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  FilesInfo section NOT found in decompressed header";
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  This occurs with solid 7z archives where file metadata";
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  may be stored in a separate location or format";
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  Decompressed size:" << baDecompressed.size() << "bytes";
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  Records parsed:" << listHeaderRecords.count();
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  Files found:" << listFileNames.count();
                // #endif
                // #ifdef QT_DEBUG
                //                                     qWarning() << "  ======================================";
                // #endif
                //                                 }
                //                             } else {
                // #ifdef QT_DEBUG
                //                                 qWarning() << "  Failed to decompress header!";
                // #endif
                //                             }
                //                         } else {
                // #ifdef QT_DEBUG
                //                             qWarning() << "  Failed to open SubDevice!";
                // #endif
                //                         }
                //                     }
                //                 } else {
                // // === STANDARD HEADER PATH ===
                // #ifdef QT_DEBUG
                //                     qDebug() << "XSevenZip::initUnpack: Processing standard header...";
                // #endif

                //                     QList<SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct);
                //                     qint32 nNumberOfRecords = listRecords.count();
                // #ifdef QT_DEBUG
                //                     qDebug() << "  Header records:" << nNumberOfRecords;
                // #endif

                //                     if (nNumberOfRecords > 0) {
                //                         SZRECORD firstRecord = listRecords.at(0);
                //                         if ((firstRecord.srType == SRTYPE_ID) && (firstRecord.varValue.toULongLong() == k7zIdHeader)) {
                //                             // Extract file information
                //                             QList<QString> listFileNames;
                //                             QList<qint64> listFilePackedSizes;
                //                             QList<qint64> listFileUnpackedSizes;
                //                             QList<qint64> listStreamOffsets;
                //                             QList<QByteArray> listCodecs;

                //                             for (qint32 i = 0; i < nNumberOfRecords; i++) {
                //                                 SZRECORD rec = listRecords.at(i);
                //                                 if (rec.impType == IMPTYPE_FILENAME) {
                //                                     listFileNames.append(rec.varValue.toString());
                //                                 } else if (rec.impType == IMPTYPE_FILEPACKEDSIZE) {
                //                                     listFilePackedSizes.append(rec.varValue.toLongLong());
                //                                 } else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE || rec.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                //                                     // File sizes can come from either FILEUNPACKEDSIZE or STREAMUNPACKEDSIZE (SubStreamsInfo)
                //                                     listFileUnpackedSizes.append(rec.varValue.toLongLong());
                //                                 } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                //                                     listStreamOffsets.append(rec.varValue.toLongLong());
                //                                 } else if (rec.impType == IMPTYPE_CODER) {
                //                                     listCodecs.append(rec.varValue.toByteArray());
                //                                 }
                //                             }

                //                             // Analyze codec chain to separate filters from compressors
                //                             // In 7z, filters (like BCJ) come before compressors (like LZMA2)
                //                             COMPRESS_METHOD filterMethod = COMPRESS_METHOD_UNKNOWN;
                //                             COMPRESS_METHOD compressMethod = COMPRESS_METHOD_STORE;

                //                             for (qint32 i = 0; i < listCodecs.count(); i++) {
                //                                 COMPRESS_METHOD method = codecToCompressMethod(listCodecs.at(i));

                //                                 // BCJ and BCJ2 are filters/preprocessors, not compressors
                //                                 if (method == COMPRESS_METHOD_BCJ || method == COMPRESS_METHOD_BCJ2) {
                //                                     filterMethod = method;
                //                                 } else if (method != COMPRESS_METHOD_UNKNOWN) {
                //                                     // This is the actual compressor (LZMA, LZMA2, etc.)
                //                                     compressMethod = method;
                //                                 }
                //                             }

                //                             // Create ARCHIVERECORD entries
                //                             qint64 nDataOffset = sizeof(SIGNATUREHEADER);
                //                             for (qint32 i = 0; i < listFileNames.count(); i++) {
                //                                 ARCHIVERECORD record = {};
                //                                 record.mapProperties.insert(FPART_PROP_ORIGINALNAME, listFileNames.at(i));

                //                                 if (i < listFileUnpackedSizes.count()) {
                //                                     record.nDecompressedSize = listFileUnpackedSizes.at(i);
                //                                 }
                //                                 if (i < listFilePackedSizes.count()) {
                //                                     record.nStreamSize = listFilePackedSizes.at(i);
                //                                 } else {
                //                                     record.nStreamSize = record.nDecompressedSize;
                //                                 }
                //                                 if (i < listStreamOffsets.count()) {
                //                                     record.nStreamOffset = nDataOffset + listStreamOffsets.at(i);
                //                                 } else {
                //                                     record.nStreamOffset = nDataOffset;
                //                                 }

                //                                 // Store both compressor and filter (if present)
                //                                 record.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, compressMethod);
                //                                 if (filterMethod != COMPRESS_METHOD_UNKNOWN) {
                //                                     record.mapProperties.insert(FPART_PROP_FILTERMETHOD, filterMethod);
                //                                 }

                //                                 pContext->listArchiveRecords.append(record);
                //                                 pContext->listRecordOffsets.append(record.nStreamOffset);
                //                             }

                // #ifdef QT_DEBUG
                //                             qDebug() << "  Created" << pContext->listArchiveRecords.count() << "archive records";
                // #endif
                //                         }
                //                     }
                //                 }

        pState->nNumberOfRecords = pContext->listArchiveRecords.count();
        bResult = (pState->nNumberOfRecords > 0);

#ifdef QT_DEBUG
        qDebug() << "XSevenZip::initUnpack: Final result - Records=" << pState->nNumberOfRecords << "Success=" << bResult;
#endif

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
        }  // End if (pState)
    }  // End outer scope

    return bResult;
}

bool XSevenZip::initUnpack2(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Create context
        SEVENZ_UNPACK_CONTEXT *pContext = new SEVENZ_UNPACK_CONTEXT;
        pContext->nSignatureSize = sizeof(SIGNATUREHEADER);

        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;

        QList<qint64> listSubStreamSizes;  // Moved declaration to the top of the function

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
                            QByteArray baCompressedData = read_array(sizeof(SIGNATUREHEADER) + nStreamOffset, nStreamPackedSize, pPdStruct);
                            QByteArray baUncompressedData;

                            QBuffer bufferIn;
                            bufferIn.setBuffer(&baCompressedData);

                            QBuffer bufferOut;
                            // bufferOut.setData(pUnpackedData, nStreamUnpackedSize);
                            bufferOut.setBuffer(&baUncompressedData);

                            if (bufferIn.open(QIODevice::ReadOnly) && bufferOut.open(QIODevice::WriteOnly)) {
                                DATAPROCESS_STATE decompressState = {};
                                decompressState.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
                                decompressState.pDeviceInput = &bufferIn;
                                decompressState.pDeviceOutput = &bufferOut;
                                decompressState.nInputOffset = 0;
                                decompressState.nInputLimit = nStreamPackedSize;
                                decompressState.nProcessedOffset = 0;
                                decompressState.nProcessedLimit = -1;

                                bool bDecompressResult = false;

                                // Only LZMA! in encrypted header
                                if (compressMethod == COMPRESS_METHOD_LZMA) {
                                    bDecompressResult = XLZMADecoder::decompress(&decompressState, baProperty, pPdStruct);
                                    // bDecompressResult = XLZMADecoder::decompress(&decompressState, pPdStruct);
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
                                if (bDecompressResult) {
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
                            } else {
                                state.bIsError = true;
                                state.sErrorString = tr("Failed to open buffers for decompression");
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

                    // Process the parsed records to extract file information
                    if (pHeaderData) {
                        QList<QString> listFileNames;
                        QList<qint64> listFilePackedSizes;
                        QList<qint64> listFileUnpackedSizes;
                        QList<qint64> listStreamOffsets;
                        QList<QByteArray> listCodecs;
                        QList<QByteArray> listCoderProperties;

                        QList<XSevenZip::SZRECORD> listRecords;
                        SZSTATE state = {};
                        state.pData = pHeaderData;
                        state.nSize = nHeaderSize;
                        state.nCurrentOffset = 0;
                        state.bIsError = false;
                        state.sErrorString = QString();

                        _handleId(&listRecords, XSevenZip::k7zIdHeader, &state, 1, true, pPdStruct, IMPTYPE_UNKNOWN);
                        qint32 nNumberOfRecords = listRecords.count();

                        // Extract file information from parsed records
                        qint32 nExtractedFilenames = 0;
                        QList<qint32> listNumUnpackStream;  // Number of files per folder
                        QList<qint64> listFolderUnpackedSizes;  // Folder sizes (STREAMUNPACKEDSIZE)
                        QList<qint64> listIndividualFileSizes;  // Individual file sizes (FILEUNPACKEDSIZE)
                        QByteArray baEmptyStreamData;  // Bitmap indicating which files are empty (0 bytes)

                        for (qint32 i = 0; i < nNumberOfRecords; i++) {
                            SZRECORD rec = listRecords.at(i);
                            if (rec.impType == IMPTYPE_FILENAME) {
                                listFileNames.append(rec.varValue.toString());
                                nExtractedFilenames++;
                            } else if (rec.impType == IMPTYPE_STREAMPACKEDSIZE) {
                                listFilePackedSizes.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                                // STREAMUNPACKEDSIZE: Folder/stream unpack size (total size for solid block or single file size for non-solid)
                                listFolderUnpackedSizes.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE) {
                                // FILEUNPACKEDSIZE: Individual file sizes from SubStreamsInfo (for solid archives, contains N-1 sizes)
                                listIndividualFileSizes.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                                listStreamOffsets.append(rec.varValue.toLongLong());
                            } else if (rec.impType == IMPTYPE_CODER) {
                                listCodecs.append(rec.varValue.toByteArray());
                            } else if (rec.impType == IMPTYPE_CODERPROPERTY) {
                                listCoderProperties.append(rec.varValue.toByteArray());
                            } else if (rec.impType == IMPTYPE_NUMUNPACKSTREAM) {
                                listNumUnpackStream.append(rec.varValue.toInt());
                            } else if (rec.sName == "EmptyStreamData") {
                                // EmptyStream bitmap: bit set = file is empty (0 bytes)
                                baEmptyStreamData = rec.varValue.toByteArray();
                            }
                        }
                    }
#ifdef QT_DEBUG
                    qDebug() << "XSevenZip::initUnpack: Created" << pContext->listArchiveRecords.count() << "archive records";
#endif
                }
            }

            pState->nNumberOfRecords = pContext->listArchiveRecords.count();
            bResult = (pState->nNumberOfRecords > 0);

            if (!bResult) {
                delete pContext;
                pState->pContext = nullptr;
            } else {
                if (pContext->listRecordOffsets.count() > 0) {
                    pState->nCurrentOffset = pContext->listRecordOffsets.at(0);
                }
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
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext && pDevice) {
        SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;
        ARCHIVERECORD ar = pContext->listArchiveRecords.at(pState->nCurrentIndex);

#ifdef QT_DEBUG
        qDebug() << "_unpack: Unpacking" << ar.mapProperties.value(FPART_PROP_ORIGINALNAME).toString() << "StreamOffset:" << ar.nStreamOffset
                 << "StreamSize:" << ar.nStreamSize << "DecompressedSize:" << ar.nDecompressedSize;
        if (ar.mapProperties.contains(FPART_PROP_COMPRESSMETHOD)) {
            qDebug() << "  CompressMethod:" << ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();
        }
#endif

        // Check if this is a directory entry (size 0, no file extension, other files have this as path prefix)
        QString sFilename = ar.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
        bool bIsDirectory = false;
        if (ar.nDecompressedSize == 0 && ar.nStreamSize == 0) {
            // Check if this looks like a directory (no file extension, or explicit directory marker)
            // In 7z archives, directories are stored as entries without trailing slashes
            // but other files will have paths starting with this directory name + "/"
            QFileInfo fi(sFilename);
            if (fi.suffix().isEmpty()) {
                // No extension - likely a directory, but verify by checking if other files use this path
                QString sDirPrefix = sFilename + "/";
                for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
                    QString sOtherFile = pContext->listArchiveRecords.at(i).mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
                    if (sOtherFile.startsWith(sDirPrefix)) {
                        bIsDirectory = true;
                        break;
                    }
                }
            }
        }

        if (bIsDirectory) {
            qDebug() << "[Directory] Skipping directory entry:" << sFilename;
            // Directory entry - don't try to create as file, just succeed
            bResult = true;
        } else if (ar.nDecompressedSize == 0 && ar.nStreamSize == 0) {
            qDebug() << "[Empty File] Extracting empty file:" << sFilename;
            // Empty file - nothing to write, just return success
            bResult = true;
        } else {
            // Check if this is a solid archive file
            bool bIsSolid = ar.mapProperties.value(FPART_PROP_SOLID, false).toBool();
            bool bIsEncrypted = ar.mapProperties.value(FPART_PROP_ENCRYPTED, false).toBool();
            
            // Determine folder index for this file by finding other files with same stream offset
            // Files in the same folder share the same nStreamOffset (solid folder) or have sequential offsets (non-solid)
            qint32 nFolderIndex = 0;
            if (bIsSolid && ar.nStreamOffset > 0) {
                // For solid files, folder index is determined by stream offset
                // Count how many unique stream offsets exist before this one
                QSet<qint64> uniqueOffsets;
                for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
                    ARCHIVERECORD &rec = pContext->listArchiveRecords[i];
                    bool bRecIsSolid = rec.mapProperties.value(FPART_PROP_SOLID, false).toBool();
                    if (bRecIsSolid && rec.nStreamOffset > 0 && rec.nStreamOffset < ar.nStreamOffset) {
                        uniqueOffsets.insert(rec.nStreamOffset);
                    }
                }
                nFolderIndex = uniqueOffsets.count();
            }
            
#ifdef QT_DEBUG
            if (bIsSolid) {
                qDebug() << "[Folder Index] File:" << sFilename << "Folder:" << nFolderIndex 
                         << "StreamOffset:" << ar.nStreamOffset;
            }
#endif
            
            if (bIsSolid && !pContext->mapFolderCache.contains(nFolderIndex) && bIsEncrypted) {
                // Solid encrypted folder - need to decrypt+decompress the entire block on first access
                qDebug() << "[Solid Encrypted] Folder" << nFolderIndex << "first access - decrypting and decompressing entire solid block";
                
                // Get the solid block information
                // All solid files in this folder share the same nStreamOffset and nStreamSize
                qint64 nSolidStreamOffset = ar.nStreamOffset;
                qint64 nSolidStreamSize = ar.nStreamSize;
                
                // Calculate total uncompressed size for this folder by finding max(offset + size)
                qint64 nTotalUncompressedSize = 0;
                for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
                    ARCHIVERECORD &rec = pContext->listArchiveRecords[i];
                    bool bRecIsSolid = rec.mapProperties.value(FPART_PROP_SOLID, false).toBool();
                    if (bRecIsSolid && rec.nStreamOffset == nSolidStreamOffset && rec.nDecompressedSize > 0) {
                        qint64 nEndOffset = rec.nDecompressedOffset + rec.nDecompressedSize;
                        if (nEndOffset > nTotalUncompressedSize) {
                            nTotalUncompressedSize = nEndOffset;
                        }
                    }
                }
                
                qDebug() << "[Solid Encrypted]   Folder index:" << nFolderIndex;
                qDebug() << "[Solid Encrypted]   Stream offset:" << nSolidStreamOffset;
                qDebug() << "[Solid Encrypted]   Stream size:" << nSolidStreamSize;
                qDebug() << "[Solid Encrypted]   Total uncompressed size:" << nTotalUncompressedSize;
                
                // Get AES and compression properties
                QByteArray baAesProperties;
                if (ar.mapProperties.contains(FPART_PROP_AESKEY)) {
                    baAesProperties = ar.mapProperties.value(FPART_PROP_AESKEY).toByteArray();
                    
                    // Debug: Show AES properties hex dump
                    QString sAesPropsHex;
                    for (qint32 i = 0; i < qMin(32, baAesProperties.size()); i++) {
                        sAesPropsHex += QString("%1 ").arg((quint8)baAesProperties.at(i), 2, 16, QChar('0'));
                    }
                    qDebug() << "[Solid Encrypted] AES Properties (" << baAesProperties.size() << "bytes):" << sAesPropsHex;
                }
                
                QByteArray baCompressProperties;
                if (ar.mapProperties.contains(FPART_PROP_COMPRESSPROPERTIES)) {
                    baCompressProperties = ar.mapProperties.value(FPART_PROP_COMPRESSPROPERTIES).toByteArray();
                }
                
                COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();
                
                if (baAesProperties.isEmpty()) {
                    qWarning() << "[Solid Encrypted] AES properties not found";
                    bResult = false;
                } else if (pContext->sPassword.isEmpty()) {
                    qWarning() << "[Solid Encrypted] Password required";
                    bResult = false;
                } else {
                    // Step 1: Decrypt the solid block
                    SubDevice sdEncrypted(getDevice(), nSolidStreamOffset, nSolidStreamSize);
                    if (!sdEncrypted.open(QIODevice::ReadOnly)) {
                        qWarning() << "[Solid Encrypted] Failed to open encrypted stream";
                        bResult = false;
                    } else {
                        // Check if this is NEW format (IV in properties) or OLD format (IV in stream)
                        // According to 7-Zip SDK (7zAes.cpp):
                        //   OLD format: (b0 & 0xC0) == 0 (neither bit 6 nor bit 7 set) - IV read from stream
                        //   NEW format: (b0 & 0xC0) != 0 (at least one of bits 6-7 set) - IV in properties
                        bool bNewFormat = false;
                        QByteArray baFullAesProperties = baAesProperties;
                        if (baAesProperties.size() >= 1) {
                            quint8 nFirstByte = (quint8)baAesProperties[0];
                            bNewFormat = ((nFirstByte & 0xC0) != 0);  // NEW format if ANY of bits 6-7 set
                        }
                        
                        if (bNewFormat) {
                            // NEW FORMAT: IV is already in properties, don't read from stream
                            qDebug() << "[Solid Encrypted] NEW format detected - IV in properties";
                        } else {
                            // OLD FORMAT: Read IV from stream (first 16 bytes) and append to properties
                            QByteArray baIV = sdEncrypted.read(16);
                            if (baIV.size() != 16) {
                                qWarning() << "[Solid Encrypted] Failed to read IV (got" << baIV.size() << "bytes)";
                                sdEncrypted.close();
                                bResult = false;
                            } else {
                                qDebug() << "[Solid Encrypted] OLD format detected - IV from stream";
                                baFullAesProperties = baAesProperties + baIV;
                            }
                        }
                        
                        if (bResult) {
                            
                            // Create buffer for decrypted data
                            QBuffer tempDecryptBuffer;
                            tempDecryptBuffer.open(QIODevice::WriteOnly);
                            
                            XBinary::DATAPROCESS_STATE decryptState = {};
                            decryptState.pDeviceInput = &sdEncrypted;
                            decryptState.pDeviceOutput = &tempDecryptBuffer;
                            decryptState.nInputOffset = 0;
                            // For OLD format, subtract 16 bytes for IV that was read from stream
                            // For NEW format, IV is in properties so don't subtract
                            decryptState.nInputLimit = bNewFormat ? nSolidStreamSize : (nSolidStreamSize - 16);
                            
                            qDebug() << "[Solid Encrypted] Using password:" << pContext->sPassword << "(" << pContext->sPassword.length() << "chars)";
                            
                            XAESDecoder aesDecoder;
                            bool bDecrypted = aesDecoder.decrypt(&decryptState, baFullAesProperties, pContext->sPassword, pPdStruct);
                            sdEncrypted.close();
                            
                            if (!bDecrypted) {
                                qWarning() << "[Solid Encrypted] Decryption failed";
                                tempDecryptBuffer.close();
                                bResult = false;
                            } else {
                                QByteArray baDecryptedData = tempDecryptBuffer.data();
                                tempDecryptBuffer.close();
                                
                                qDebug() << "[Solid Encrypted] Decrypted" << baDecryptedData.size() << "bytes";
                                
                                // Debug: Hex dump first 32 bytes of decrypted data
                                QString sDecryptedHex;
                                for (qint32 i = 0; i < qMin(32, baDecryptedData.size()); i++) {
                                    sDecryptedHex += QString("%1 ").arg((quint8)baDecryptedData.at(i), 2, 16, QChar('0'));
                                }
                                qDebug() << "[Solid Encrypted] Decrypted data (first 32 bytes):" << sDecryptedHex;
                                
                                // Step 2: Decompress the decrypted data
                                QBuffer decryptedBuffer(&baDecryptedData);
                                if (!decryptedBuffer.open(QIODevice::ReadOnly)) {
                                    qWarning() << "[Solid Encrypted] Failed to open decrypted buffer";
                                    bResult = false;
                                } else {
                                    QIODevice *pDecompressBuffer = createFileBuffer(nTotalUncompressedSize, pPdStruct);
                                    if (!pDecompressBuffer || !pDecompressBuffer->open(QIODevice::WriteOnly)) {
                                        qWarning() << "[Solid Encrypted] Failed to create decompression buffer";
                                        decryptedBuffer.close();
                                        if (pDecompressBuffer) freeFileBuffer(&pDecompressBuffer);
                                        bResult = false;
                                    } else {
                                        XBinary::DATAPROCESS_STATE decompressState = {};
                                        decompressState.pDeviceInput = &decryptedBuffer;
                                        decompressState.pDeviceOutput = pDecompressBuffer;
                                        decompressState.nInputOffset = 0;
                                        decompressState.nInputLimit = baDecryptedData.size();
                                        
                                        bool bDecompressed = false;
                                        if (compressMethod == COMPRESS_METHOD_LZMA2) {
                                            XLZMADecoder lzmaDecoder;
                                            bDecompressed = lzmaDecoder.decompressLZMA2(&decompressState, baCompressProperties, pPdStruct);
                                        } else if (compressMethod == COMPRESS_METHOD_LZMA) {
                                            XLZMADecoder lzmaDecoder;
                                            bDecompressed = lzmaDecoder.decompress(&decompressState, baCompressProperties, pPdStruct);
                                        } else if (compressMethod == COMPRESS_METHOD_BZIP2) {
                                            bDecompressed = XBZIP2Decoder::decompress(&decompressState, pPdStruct);
                                        } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
                                            bDecompressed = XDeflateDecoder::decompress(&decompressState, pPdStruct);
                                        } else {
                                            qWarning() << "[Solid Encrypted] Unsupported compression method:" << compressMethod;
                                        }
                                        
                                        decryptedBuffer.close();
                                        pDecompressBuffer->close();
                                        
                                        if (bDecompressed) {
                                            // Read decompressed data and cache it
                                            pDecompressBuffer->open(QIODevice::ReadOnly);
                                            QByteArray baSolidData = pDecompressBuffer->readAll();
                                            pDecompressBuffer->close();
                                            
                                            qDebug() << "[Solid Encrypted] Decompressed" << baSolidData.size() << "bytes (expected" << nTotalUncompressedSize << ")";
                                            
                                            if (baSolidData.size() == nTotalUncompressedSize) {
                                                // Cache the decompressed solid block for this folder
                                                pContext->mapFolderCache.insert(nFolderIndex, baSolidData);
                                                qDebug() << "[Solid Encrypted] Solid block for folder" << nFolderIndex << "cached successfully";
                                                
                                                // Now extract this file from the cached block
                                                qint64 nOffsetInDecompressed = ar.nDecompressedOffset;
                                                qint64 nSize = ar.nDecompressedSize;
                                                
                                                if (nOffsetInDecompressed >= 0 && nSize > 0 && 
                                                    nOffsetInDecompressed + nSize <= baSolidData.size()) {
                                                    qint64 nWritten = pDevice->write(baSolidData.constData() + nOffsetInDecompressed, nSize);
                                                    bResult = (nWritten == nSize);
                                                } else {
                                                    qWarning() << "[Solid Encrypted] Invalid extraction bounds";
                                                    bResult = false;
                                                }
                                            } else {
                                                qWarning() << "[Solid Encrypted] Decompression size mismatch";
                                                bResult = false;
                                            }
                                        } else {
                                            qWarning() << "[Solid Encrypted] Decompression failed";
                                            bResult = false;
                                        }
                                        
                                        freeFileBuffer(&pDecompressBuffer);
                                    }
                                }
                            }
                        }
                    }
                }
            } else if (bIsSolid && pContext->mapFolderCache.contains(nFolderIndex)) {
                // Extract from cached solid block for this folder
                QByteArray baDecompressed = pContext->mapFolderCache.value(nFolderIndex);
                qint64 nOffsetInDecompressed = ar.nDecompressedOffset;  // Offset in decompressed buffer
                qint64 nSize = ar.nDecompressedSize;
                
                qDebug() << "[Solid] Extracting:" << ar.mapProperties.value(FPART_PROP_ORIGINALNAME).toString()
                         << "Folder:" << nFolderIndex
                         << "BufferSize:" << baDecompressed.size() 
                         << "Offset:" << nOffsetInDecompressed 
                         << "Size:" << nSize;
                
                if (nOffsetInDecompressed >= 0 && nSize > 0 && 
                    nOffsetInDecompressed + nSize <= baDecompressed.size()) {
                    // Write data from cached buffer
                    qint64 nWritten = pDevice->write(baDecompressed.constData() + nOffsetInDecompressed, nSize);
                    bResult = (nWritten == nSize);
                    
                    if (!bResult) {
                        qWarning() << "[Solid] Write FAILED: expected" << nSize << "wrote" << nWritten;
                    }
                } else {
                    qWarning() << "[Solid] Invalid bounds: offset=" << nOffsetInDecompressed 
                              << "size=" << nSize << "bufferSize=" << baDecompressed.size() 
                              << "check=" << (nOffsetInDecompressed + nSize) << "<=" << baDecompressed.size();
                }
            } else if (bIsSolid && !bIsEncrypted && !pContext->mapFolderCache.contains(nFolderIndex)) {
                // Solid non-encrypted folder - decompress entire folder and cache it
                qDebug() << "[Solid Non-Encrypted] Folder" << nFolderIndex << "first access - decompressing entire solid block";
                
                // Calculate total uncompressed size for this folder
                qint64 nTotalUncompressedSize = 0;
                for (qint32 i = 0; i < pContext->listArchiveRecords.count(); i++) {
                    ARCHIVERECORD &rec = pContext->listArchiveRecords[i];
                    bool bRecIsSolid = rec.mapProperties.value(FPART_PROP_SOLID, false).toBool();
                    if (bRecIsSolid && rec.nStreamOffset == ar.nStreamOffset && rec.nDecompressedSize > 0) {
                        qint64 nEndOffset = rec.nDecompressedOffset + rec.nDecompressedSize;
                        if (nEndOffset > nTotalUncompressedSize) {
                            nTotalUncompressedSize = nEndOffset;
                        }
                    }
                }
                
                qDebug() << "[Solid Non-Encrypted]   Stream offset:" << ar.nStreamOffset;
                qDebug() << "[Solid Non-Encrypted]   Stream size:" << ar.nStreamSize;
                qDebug() << "[Solid Non-Encrypted]   Expected decompressed size:" << nTotalUncompressedSize;
                
                // Decompress entire solid block
                SubDevice sd(getDevice(), ar.nStreamOffset, ar.nStreamSize);
                if (sd.open(QIODevice::ReadOnly)) {
                    QBuffer *pDecompressBuffer = new QBuffer();
                    if (pDecompressBuffer->open(QIODevice::WriteOnly)) {
                        COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_UNKNOWN).toUInt();
                        QByteArray baCompressProperties = ar.mapProperties.value(FPART_PROP_COMPRESSPROPERTIES, QByteArray()).toByteArray();
                        
                        DATAPROCESS_STATE decompressState = {};
                        decompressState.pDeviceInput = &sd;
                        decompressState.pDeviceOutput = pDecompressBuffer;
                        decompressState.nInputOffset = 0;
                        decompressState.nInputLimit = ar.nStreamSize;
                        
                        bool bDecompressed = false;
                        if (compressMethod == COMPRESS_METHOD_LZMA2) {
                            XLZMADecoder lzmaDecoder;
                            bDecompressed = lzmaDecoder.decompressLZMA2(&decompressState, baCompressProperties, pPdStruct);
                        } else if (compressMethod == COMPRESS_METHOD_LZMA) {
                            XLZMADecoder lzmaDecoder;
                            bDecompressed = lzmaDecoder.decompress(&decompressState, baCompressProperties, pPdStruct);
                        } else if (compressMethod == COMPRESS_METHOD_BZIP2) {
                            bDecompressed = XBZIP2Decoder::decompress(&decompressState, pPdStruct);
                        } else if (compressMethod == COMPRESS_METHOD_DEFLATE) {
                            bDecompressed = XDeflateDecoder::decompress(&decompressState, pPdStruct);
                        } else {
                            qWarning() << "[Solid Non-Encrypted] Unsupported compression method:" << compressMethod;
                        }
                        
                        pDecompressBuffer->close();
                        
                        if (bDecompressed) {
                            pDecompressBuffer->open(QIODevice::ReadOnly);
                            QByteArray baSolidData = pDecompressBuffer->readAll();
                            pDecompressBuffer->close();
                            
                            qDebug() << "[Solid Non-Encrypted] Decompressed" << baSolidData.size() << "bytes (expected" << nTotalUncompressedSize << ")";
                            
                            if (baSolidData.size() > 0) {
                                // Cache the decompressed solid block
                                pContext->mapFolderCache.insert(nFolderIndex, baSolidData);
                                qDebug() << "[Solid Non-Encrypted] Solid block for folder" << nFolderIndex << "cached successfully";
                                
                                // Now extract this file from the cached block
                                qint64 nOffsetInDecompressed = ar.nDecompressedOffset;
                                qint64 nSize = ar.nDecompressedSize;
                                
                                if (nOffsetInDecompressed >= 0 && nSize > 0 && 
                                    nOffsetInDecompressed + nSize <= baSolidData.size()) {
                                    qint64 nWritten = pDevice->write(baSolidData.constData() + nOffsetInDecompressed, nSize);
                                    bResult = (nWritten == nSize);
                                } else {
                                    qWarning() << "[Solid Non-Encrypted] Invalid bounds: offset=" << nOffsetInDecompressed 
                                              << "size=" << nSize << "bufferSize=" << baSolidData.size();
                                    bResult = false;
                                }
                            } else {
                                qWarning() << "[Solid Non-Encrypted] Decompression produced 0 bytes";
                                bResult = false;
                            }
                        } else {
                            qWarning() << "[Solid Non-Encrypted] Decompression failed";
                            bResult = false;
                        }
                        
                        delete pDecompressBuffer;
                    }
                    sd.close();
                } else {
                    qWarning() << "[Solid Non-Encrypted] Failed to open SubDevice";
                    bResult = false;
                }
            } else if (ar.nStreamSize > 0) {
            SubDevice sd(getDevice(), ar.nStreamOffset, ar.nStreamSize);
            
            qDebug() << "[SubDevice] Created: offset=" << ar.nStreamOffset << "size=" << ar.nStreamSize;

            if (sd.open(QIODevice::ReadOnly)) {
                bResult = true;  // Initialize to true after successful open
                
                // Decompress if compressed
                if (ar.mapProperties.contains(FPART_PROP_COMPRESSMETHOD)) {
                    COMPRESS_METHOD compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();

                    DATAPROCESS_STATE state = {};
                    state.pDeviceInput = &sd;
                    state.pDeviceOutput = pDevice;
                    state.nInputOffset = 0;
                    state.nInputLimit = ar.nStreamSize;
                    state.nProcessedOffset = 0;
                    state.nProcessedLimit = -1;
                    state.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, ar.nDecompressedSize);
                    
#ifdef QT_DEBUG
                    qDebug() << "Decompression setup: InputLimit=" << state.nInputLimit << "DecompressedSize=" << ar.nDecompressedSize;
#endif

                    qDebug() << "[Decompression] Method:" << compressMethod 
                             << "StreamSize:" << ar.nStreamSize 
                             << "DecompressedSize:" << ar.nDecompressedSize;
                    
                    // Handle AES encryption first (if present)
                    QByteArray baDecryptedData;  // Store decrypted data if AES is used
                    QBuffer *pDecryptedBuffer = nullptr;
                    bool bNeedDecryption = ar.mapProperties.value(FPART_PROP_ENCRYPTED, false).toBool();
                    
                    qDebug() << "[AES Check] bNeedDecryption:" << bNeedDecryption << "bResult:" << bResult;
                    
                    if (bNeedDecryption && bResult) {
                        if (pContext->sPassword.isEmpty()) {
                            qWarning() << "[XSevenZip] AES-encrypted archive requires a password";
                            bResult = false;
                        } else {
                            // Get AES properties
                            QByteArray baAesProperties;
                            if (ar.mapProperties.contains(FPART_PROP_AESKEY)) {
                                baAesProperties = ar.mapProperties.value(FPART_PROP_AESKEY).toByteArray();
                            }
                            
                            if (baAesProperties.isEmpty()) {
                                qWarning() << "[XSevenZip] AES properties not found";
                                bResult = false;
                            } else {
                                // Check if this is NEW format (IV in properties) or OLD format (IV in stream)
                                // According to 7-Zip SDK (7zAes.cpp):
                                //   OLD format: (b0 & 0xC0) == 0 (neither bit 6 nor bit 7 set) - IV read from stream
                                //   NEW format: (b0 & 0xC0) != 0 (at least one of bits 6-7 set) - IV in properties
                                bool bNewFormat = false;
                                if (baAesProperties.size() >= 1) {
                                    quint8 nFirstByte = (quint8)baAesProperties[0];
                                    bNewFormat = ((nFirstByte & 0xC0) != 0);  // NEW format if ANY of bits 6-7 set
                                }
                                
                                qDebug() << "[XSevenZip] Decrypting AES-encrypted stream";
                                qDebug() << "  AES properties size:" << baAesProperties.size();
                                qDebug() << "  Encrypted stream size:" << ar.nStreamSize;
                                qDebug() << "  Format:" << (bNewFormat ? "NEW (IV in properties)" : "OLD (IV in stream)");
                                
                                // Create a temporary buffer for decrypted data
                                QBuffer tempDecryptBuffer;
                                tempDecryptBuffer.open(QIODevice::WriteOnly);
                                
                                XBinary::DATAPROCESS_STATE decryptState = {};
                                decryptState.pDeviceInput = &sd;  // Read from SubDevice, not main archive
                                decryptState.pDeviceOutput = &tempDecryptBuffer;
                                decryptState.nCountInput = 0;
                                decryptState.nInputOffset = 0;  // SubDevice starts at offset 0
                                // For OLD format: stream size includes IV (16 bytes) + encrypted data
                                // For NEW format: stream size is just encrypted data (IV in properties)
                                decryptState.nInputLimit = ar.nStreamSize;
                                decryptState.nProcessedLimit = ar.nStreamSize;
                                
                                // Decrypt using AES decoder
                                XAESDecoder aesDecoder;
                                bResult = aesDecoder.decrypt(&decryptState, baAesProperties, pContext->sPassword, pPdStruct);
                                
                                if (!bResult) {
                                    qWarning() << "[XSevenZip] AES decryption failed";
                                    tempDecryptBuffer.close();
                                } else {
                                    // Get decrypted data
                                    baDecryptedData = tempDecryptBuffer.data();
                                    tempDecryptBuffer.close();
                                    
                                    qDebug() << "[XSevenZip] Decrypted" << baDecryptedData.size() << "bytes";
                                    
                                    // Now we need to decompress the decrypted data using the actual compression method
                                    // Get compression method from record
                                    compressMethod = (COMPRESS_METHOD)ar.mapProperties.value(FPART_PROP_COMPRESSMETHOD, COMPRESS_METHOD_STORE).toUInt();
                                    
                                    // Create a buffer from decrypted data for decompression
                                    pDecryptedBuffer = new QBuffer(&baDecryptedData);
                                    if (!pDecryptedBuffer->open(QIODevice::ReadOnly)) {
                                        qWarning() << "[XSevenZip] Failed to open decrypted buffer";
                                        bResult = false;
                                        delete pDecryptedBuffer;
                                        pDecryptedBuffer = nullptr;
                                    } else {
                                        // Update state to read from decrypted buffer instead of archive file
                                        state.pDeviceInput = pDecryptedBuffer;
                                        state.nCountInput = 0;
                                        state.nInputOffset = 0;
                                        state.nInputLimit = baDecryptedData.size();
                                        state.nProcessedLimit = baDecryptedData.size();
                                        
                                        qDebug() << "[XSevenZip] Now decompressing" << compressMethod << "from decrypted data";
                                    }
                                }
                            }
                        }
                    }
                    
                    // Now handle decompression (or direct copy if STORE) - only if previous steps succeeded
                    if (bResult) {
                        switch (compressMethod) {
                        case COMPRESS_METHOD_LZMA:
                        case COMPRESS_METHOD_LZMA2: {
                            XLZMADecoder lzmaDecoder;
                            // Get compression properties from archive record
                            QByteArray baProperties;
                            if (ar.mapProperties.contains(FPART_PROP_COMPRESSPROPERTIES)) {
                                baProperties = ar.mapProperties.value(FPART_PROP_COMPRESSPROPERTIES).toByteArray();
                            }
                            
#ifdef QT_DEBUG
                            qDebug() << "Decompressing with" << (compressMethod == COMPRESS_METHOD_LZMA ? "LZMA" : "LZMA2");
                            qDebug() << "  Properties size:" << baProperties.size() << "bytes";
                            if (baProperties.size() > 0) {
                                QString sHex;
                                for (qint32 i = 0; i < baProperties.size(); i++) {
                                    sHex += QString("%1 ").arg((quint8)baProperties[i], 2, 16, QChar('0'));
                                }
                                qDebug() << "  Properties hex:" << sHex;
                            }
#endif
                            
                            if (compressMethod == COMPRESS_METHOD_LZMA) {
                                bResult = lzmaDecoder.decompress(&state, baProperties, pPdStruct);
                            } else {
                                // LZMA2: Convert 5-byte LZMA properties to 1-byte LZMA2 property
                                // 7z format stores full LZMA properties even for LZMA2
                                QByteArray baLzma2Prop;
                                if (baProperties.size() == 5) {
                                    // Extract dictionary size from bytes 1-4 (little-endian)
                                    quint32 nDictSize = ((quint8)baProperties[1]) |
                                                       (((quint32)(quint8)baProperties[2]) << 8) |
                                                       (((quint32)(quint8)baProperties[3]) << 16) |
                                                       (((quint32)(quint8)baProperties[4]) << 24);
                                    
                                    // Convert dictionary size to LZMA2 1-byte property
                                    // Formula: dicSize = ((2 | (prop & 1)) << (prop / 2 + 11))
                                    // Inverse: Find prop such that result >= nDictSize (round up to next valid size)
                                    quint8 nProp = 40; // Maximum prop value
                                    for (quint8 p = 0; p <= 40; p++) {
                                        quint64 nTestDictSize = ((quint64)(2 | (p & 1))) << ((p / 2) + 11);
                                        if (nTestDictSize >= nDictSize) {
                                            nProp = p;
                                            break;
                                        }
                                    }
                                    
                                    baLzma2Prop.append((char)nProp);
                                    
                                    // Log conversion
                                    QFile logFile(QDir::tempPath() + "/xsevenzip_debug.log");
                                    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                                        QTextStream log(&logFile);
                                        log << "=== LZMA2 Property Conversion ===" << "\n";
                                        log << "  Input (5 bytes LZMA): " << QString("%1 %2 %3 %4 %5").arg((quint8)baProperties[0], 2, 16, QChar('0'))
                                            .arg((quint8)baProperties[1], 2, 16, QChar('0')).arg((quint8)baProperties[2], 2, 16, QChar('0'))
                                            .arg((quint8)baProperties[3], 2, 16, QChar('0')).arg((quint8)baProperties[4], 2, 16, QChar('0')) << "\n";
                                        log << "  Extracted dict size: " << nDictSize << " bytes (" << (nDictSize / 1024) << " KB)" << "\n";
                                        log << "  Output (1 byte LZMA2): " << QString("%1").arg((quint8)baLzma2Prop[0], 2, 16, QChar('0')) << "\n";
                                        quint64 nResultDictSize = ((quint64)(2 | (nProp & 1))) << ((nProp / 2) + 11);
                                        log << "  Resulting dict size: " << nResultDictSize << " bytes (" << (nResultDictSize / 1024) << " KB)" << "\n";
                                        logFile.close();
                                    }
                                } else if (baProperties.size() == 1) {
                                    // Already LZMA2 format
                                    baLzma2Prop = baProperties;
                                }
                                
                                bResult = lzmaDecoder.decompressLZMA2(&state, baLzma2Prop, pPdStruct);
                            }
                            
                            // Log LZMA/LZMA2 decompression result
                            {
                                QFile logFile(QDir::tempPath() + "/xsevenzip_debug.log");
                                if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                                    QTextStream log(&logFile);
                                    log << "=== " << (compressMethod == COMPRESS_METHOD_LZMA ? "LZMA" : "LZMA2") << " Decompression Result ===" << "\n";
                                    log << "  Success: " << (bResult ? "true" : "false") << "\n";
                                    log << "  Expected size: " << ar.nDecompressedSize << " bytes" << "\n";
                                    log << "  Actual size: " << pDevice->size() << " bytes" << "\n";
                                    logFile.close();
                                }
                            }
                            break;
                        }
                        case COMPRESS_METHOD_STORE: {
                            // No compression, just copy
                            // CRITICAL: If AES decryption was performed, copy from decrypted buffer
                            if (pDecryptedBuffer && bNeedDecryption) {
                                qDebug() << "[COPY] Copying from decrypted buffer:" << baDecryptedData.size() << "bytes";
                                // IMPORTANT: AES decrypts in 16-byte blocks, but the actual file might be smaller.
                                // Use ar.nDecompressedSize to get the correct file size (without padding).
                                qint64 nCopySize = qMin((qint64)baDecryptedData.size(), ar.nDecompressedSize);
                                qDebug() << "[COPY] Original file size:" << ar.nDecompressedSize << "bytes, copying:" << nCopySize << "bytes";
                                bResult = XBinary::copyDeviceMemory(pDecryptedBuffer, 0, pDevice, 0, nCopySize);
                            } else {
                                bResult = XBinary::copyDeviceMemory(&sd, 0, pDevice, 0, ar.nStreamSize);
                            }
                            break;
                        }
                        case COMPRESS_METHOD_PPMD: {
                            // Get compression properties from archive record
                            QByteArray baProperties;
                            if (ar.mapProperties.contains(FPART_PROP_COMPRESSPROPERTIES)) {
                                baProperties = ar.mapProperties.value(FPART_PROP_COMPRESSPROPERTIES).toByteArray();
                            }
                            
#ifdef QT_DEBUG
                            qDebug() << "Decompressing with PPMd";
                            qDebug() << "  Properties size:" << baProperties.size() << "bytes";
                            if (baProperties.size() > 0) {
                                QString sHex;
                                for (qint32 i = 0; i < baProperties.size(); i++) {
                                    sHex += QString("%1 ").arg((quint8)baProperties[i], 2, 16, QChar('0'));
                                }
                                qDebug() << "  Properties hex:" << sHex;
                            }
#endif
                            
                            // 7z PPMd format: 5 bytes (order, mem0-3 little-endian)
                            if (baProperties.size() == 5) {
                                // Extract PPMd parameters
                                quint8 nOrder = (quint8)baProperties[0];
                                quint32 nMemSize = ((quint8)baProperties[1]) |
                                                  (((quint32)(quint8)baProperties[2]) << 8) |
                                                  (((quint32)(quint8)baProperties[3]) << 16) |
                                                  (((quint32)(quint8)baProperties[4]) << 24);
                                
                                // Initialize Ppmd7 model (PPMdH variant used by 7z)
                                XPPMd7Model model;
                                
                                if (!model.allocate(nMemSize)) {
                                    bResult = false;
                                    break;
                                }
                                
                                // Set input stream to compressed data
                                model.setInputStream(&sd);
                                model.init(nOrder);
                                
                                // Decompress symbol by symbol
                                const qint32 N_BUFFER_SIZE = 0x4000;
                                char sBufferOut[N_BUFFER_SIZE];
                                
                                qint64 nDecompressed = 0;
                                bResult = true;
                                
                                while (XBinary::isPdStructNotCanceled(pPdStruct)) {
                                    qint32 nActual = 0;
                                    
                                    // Decode buffer
                                    for (qint32 i = 0; i < N_BUFFER_SIZE && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                                        if (nDecompressed >= ar.nDecompressedSize) {
                                            break; // Reached expected size
                                        }
                                        
                                        qint32 nSymbol = model.decodeSymbol();
                                        
                                        if (nSymbol < 0) {
                                            // End of stream or error
                                            if (nDecompressed < ar.nDecompressedSize) {
                                                bResult = false; // Unexpected end
                                            }
                                            break;
                                        }
                                        
                                        sBufferOut[nActual++] = (char)nSymbol;
                                        nDecompressed++;
                                    }
                                    
                                    // Write decoded data
                                    if (nActual > 0) {
                                        qint64 nWritten = pDevice->write(sBufferOut, nActual);
                                        if (nWritten != nActual) {
                                            bResult = false;
                                            break;
                                        }
                                    } else {
                                        // No more data
                                        break;
                                    }
                                }
                                
                                model.free();
                                
                                // Verify size
                                if (bResult && nDecompressed != ar.nDecompressedSize) {
                                    bResult = false;
                                }
                            } else {
                                bResult = false;
                            }
                            
                            // Log PPMd decompression result
                            {
                                QFile logFile(QDir::tempPath() + "/xsevenzip_debug.log");
                                if (logFile.open(QIODevice::Append | QIODevice::Text)) {
                                    QTextStream log(&logFile);
                                    log << "=== PPMd Decompression Result ===" << "\n";
                                    log << "  Success: " << (bResult ? "true" : "false") << "\n";
                                    log << "  Expected size: " << ar.nDecompressedSize << " bytes" << "\n";
                                    log << "  Actual size: " << pDevice->size() << " bytes" << "\n";
                                    logFile.close();
                                }
                            }
                            break;
                        }
                        case COMPRESS_METHOD_BZIP2: {
                            bResult = XBZIP2Decoder::decompress(&state, pPdStruct);
                            break;
                        }
                        case COMPRESS_METHOD_DEFLATE:
                        case COMPRESS_METHOD_DEFLATE64: {
                            if (compressMethod == COMPRESS_METHOD_DEFLATE) {
                                bResult = XDeflateDecoder::decompress(&state, pPdStruct);
                            } else {
                                bResult = XDeflateDecoder::decompress64(&state, pPdStruct);
                            }
                            break;
                        }
                            default: bResult = false; break;
                        }
                    } else {
                        // No compression method specified, assume STORE
                        bResult = XBinary::copyDeviceMemory(&sd, 0, pDevice, 0, ar.nStreamSize);
                    }
                    
                    // Clean up decrypted buffer if it was created
                    if (pDecryptedBuffer) {
                        pDecryptedBuffer->close();
                        delete pDecryptedBuffer;
                        pDecryptedBuffer = nullptr;
                    }
                } // End if (bResult) from AES decryption check

                sd.close();
            }
#ifdef QT_DEBUG
            else {
                qDebug() << "_unpack: No compressed stream data (nStreamSize=" << ar.nStreamSize << ")";
            }
#endif
            } else if (bIsSolid) {
#ifdef QT_DEBUG
                qWarning() << "_unpack: Solid archive but no cached data available";
#endif
            }
        }  // End of else block for non-empty files

#ifdef QT_DEBUG
        if (!bResult) {
            qDebug() << "_unpack: Failed to unpack record at index" << pState->nCurrentIndex;
        } else {
            qint64 nActualSize = pDevice->size();
            qDebug() << "_unpack: Successfully unpacked. Expected:" << ar.nDecompressedSize << "bytes, Actual:" << nActualSize << "bytes";
        }
#endif
    }

    return bResult;
}

bool XSevenZip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    
    bool bResult = true;
    
    if (pState && pState->pContext) {
        SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)pState->pContext;
        
        // Free cached solid block data if any
        if (!pContext->mapFolderCache.isEmpty()) {
#ifdef QT_DEBUG
            qDebug() << "XSevenZip::finishUnpack: Freeing" << pContext->mapFolderCache.size() << "cached solid blocks";
#endif
            pContext->mapFolderCache.clear();
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
        record.nDecompressedSize = pDevice->size();
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
            record.nDecompressedSize = file.size();
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
            _writeNumber(&bufferHeader, pContext->listArchiveRecords.at(i).nDecompressedSize);
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
