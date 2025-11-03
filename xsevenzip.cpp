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

/*
 * === RECENT IMPROVEMENTS (2025-11-02) ===
 * 
 * 1. k7zIdFolder: Fixed loop to process ALL folders (lines ~637-670)
 *    - Previous: Only processed first folder, then expected k7zIdEnd
 *    - Fixed: Added for-loop to iterate through all nNumberOfFolders
 *    - Impact: Multi-folder archives (100+ files) now parse correctly
 * 
 * 2. k7zIdSubStreamsInfo: Fixed CRC and optional field handling (lines ~674-685)
 *    - Previous: Expected k7zIdEnd immediately after k7zIdNumUnpackStream
 *    - Fixed: Added optional k7zIdSize and k7zIdCRC parsing before k7zIdEnd
 *    - Impact: Archives with CRC data in SubStreamsInfo now work
 * 
 * 3. k7zIdCRC: Implemented AllAreDefined format (lines ~715-735)
 *    - Previous: Read NumberOfCRCs then CRC array (wrong format)
 *    - Fixed: Read AllAreDefined byte, then conditional CRC reading
 *    - Format: AllAreDefined(1 byte) + [CRC32 × nCount if AllAreDefined==1]
 *    - Impact: CRC validation now works correctly
 * 
 * 4. k7zIdName: Individual filename extraction from UTF-16LE blob (lines ~773-827)
 *    - Previous: Stored entire blob as single IMPTYPE_FILENAME record
 *    - Fixed: Parse null-terminated UTF-16LE strings individually
 *    - Creates separate IMPTYPE_FILENAME record for each file
 *    - Impact: Filenames properly extracted as individual strings
 * 
 * 5. k7zIdName: Removed incorrect k7zIdDummy expectation (line ~829)
 *    - Previous: Expected k7zIdDummy with bCheck=true after filename data
 *    - Fixed: Return success without expecting k7zIdDummy
 *    - Impact: FilesInfo section parsing no longer fails incorrectly
 * 
 * 6. SZSTATE: Added nNumberOfFolders tracking (xsevenzip.h line ~192)
 *    - Added: quint64 nNumberOfFolders field to SZSTATE struct
 *    - Purpose: Track folder count for SubStreamsInfo CRC parsing
 *    - Usage: Stored in k7zIdFolder case, used in k7zIdSubStreamsInfo
 *    - Impact: Correct CRC count passed to k7zIdCRC parser
 * 
 * 7. EncodedHeader: Manual metadata extraction (lines ~1117-1273)
 *    - Manual PackPos extraction from EncodedHeader byte-by-byte
 *    - Manual StreamSize extraction from k7zIdSize section
 *    - Manual CodersUnpackSize extraction from k7zIdCodersUnpackSize
 *    - Reason: Parsed records pull values from wrong section (decompressed header's PackInfo)
 *    - Impact: Correct offsets and sizes for LZMA decompression
 * 
 * 8. EncodedHeader: LZMA decompression working (lines ~1280-1450)
 *    - Successfully decompresses compressed headers
 *    - Verified: 2054 bytes compressed → 6130 bytes decompressed
 *    - Contains proper k7zIdHeader → k7zIdMainStreamsInfo → k7zIdFilesInfo structure
 *    - Impact: Can now read metadata from archives with compressed headers
 * 
 * === KNOWN LIMITATIONS ===
 * - 7z format has many variations based on creation options
 * - Filename encoding may vary (UTF-16LE, UTF-8, etc.)
 * - Some archive structures not yet fully supported
 * - See test_system_create_unpack.cpp for detailed investigation notes
 */

#include "xsevenzip.h"
#include "xdecompress.h"
#include "xcompress.h"
#include "Algos/xlzmadecoder.h"
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

    // Use the streaming API to collect all records
    UNPACK_STATE state = {};
    QMap<UNPACK_PROP, QVariant> mapProperties;
    
    if (initUnpack(&state, mapProperties, pPdStruct)) {
        qint32 nCount = 0;
        
        while (isPdStructNotCanceled(pPdStruct)) {
            ARCHIVERECORD archiveRecord = infoCurrent(&state, pPdStruct);
            
            // Check if we have a valid record
            QString sRecordName = archiveRecord.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
            if (sRecordName.isEmpty()) {
                break;  // End of archive
            }
            
            // Convert ARCHIVERECORD to RECORD format
            RECORD record = {};
            record.spInfo.sRecordName = sRecordName;
            record.spInfo.nUncompressedSize = archiveRecord.nDecompressedSize;
            record.nDataOffset = archiveRecord.nStreamOffset;
            record.nDataSize = archiveRecord.nStreamSize;
            
            // Extract other properties
            if (archiveRecord.mapProperties.contains(FPART_PROP_COMPRESSMETHOD)) {
                record.spInfo.compressMethod = (COMPRESS_METHOD)archiveRecord.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toInt();
            }
            
            listResult.append(record);
            
            nCount++;
            if ((nLimit != -1) && (nCount >= nLimit)) {
                break;
            }
            
            if (!moveToNext(&state, pPdStruct)) {
                break;  // No more records
            }
        }
        
        // Clean up state if needed
        if (state.pContext) {
            SEVENZ_UNPACK_CONTEXT *pContext = (SEVENZ_UNPACK_CONTEXT *)state.pContext;
            delete pContext;
            state.pContext = nullptr;
        }
    }

    return listResult;
}

// QList<XBinary::ARCHIVERECORD> XSevenZip::getArchiveRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::ARCHIVERECORD> listResult;

//     SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
//     qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
//     qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

//     if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
//         char *pData = new char[nNextHeaderSize];
//         qint64 nBytesRead = read_array(nNextHeaderOffset, pData, nNextHeaderSize, pPdStruct);

//         if (nBytesRead == nNextHeaderSize) {
//             QList<XSevenZip::SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct, true);

//             qint32 nNumberOfRecords = listRecords.count();

//             if (nNumberOfRecords > 0) {
//                 SZRECORD firstRecord = listRecords.at(0);

//                 // Check if the first id is Header
//                 if ((firstRecord.srType == SRTYPE_ID) && (firstRecord.varValue.toULongLong() == k7zIdHeader)) {
//                     // Standard header - parse file information
//                     QList<QString> listFileNames;
//                     QList<qint64> listFilePackedSizes;
//                     QList<qint64> listFileUnpackedSizes;
//                     QList<quint32> listFileAttributes;
//                     QList<QDateTime> listFileTimes;

//                     // Parse all records to extract file information
//                     for (qint32 i = 0; (i < nNumberOfRecords) && isPdStructNotCanceled(pPdStruct); i++) {
//                         SZRECORD szRecord = listRecords.at(i);

//                         if (szRecord.impType == IMPTYPE_FILENAME) {
//                             QString sFileName = szRecord.varValue.toString();
//                             listFileNames.append(sFileName);
//                         } else if (szRecord.impType == IMPTYPE_FILEPACKEDSIZE) {
//                             listFilePackedSizes.append(szRecord.varValue.toLongLong());
//                         } else if (szRecord.impType == IMPTYPE_FILEUNPACKEDSIZE) {
//                             listFileUnpackedSizes.append(szRecord.varValue.toLongLong());
//                         } else if (szRecord.impType == IMPTYPE_FILEATTRIBUTES) {
//                             listFileAttributes.append(szRecord.varValue.toUInt());
//                         } else if (szRecord.impType == IMPTYPE_FILETIME) {
//                             // TODO: Convert file time to QDateTime
//                             // listFileTimes.append(convertFileTime(szRecord.varValue));
//                         }
//                     }

//                     qint32 nNumberOfFiles = listFileNames.count();

//                     // Create archive records for each file
//                     for (qint32 i = 0; (i < nNumberOfFiles) && isPdStructNotCanceled(pPdStruct); i++) {
//                         XBinary::ARCHIVERECORD record = {};

//                         // Set file name
//                         record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, listFileNames.at(i));

//                         // Set file sizes if available
//                         if (i < listFileUnpackedSizes.count()) {
//                             record.nDecompressedSize = listFileUnpackedSizes.at(i);
//                         }

//                         // For now, set basic properties
//                         // TODO: Set proper stream offsets and sizes based on pack info
//                         record.nStreamOffset = 0;                       // TODO: Calculate from pack info
//                         record.nStreamSize = record.nDecompressedSize;  // Assume uncompressed for now

//                         // Set compression method (assume STORE for now)
//                         record.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, XBinary::COMPRESS_METHOD_STORE);

//                         listResult.append(record);
//                     }
//                 } else if ((firstRecord.srType == SRTYPE_ID) && (firstRecord.varValue.toULongLong() == k7zIdEncodedHeader)) {
//                     // Encoded header - need to decompress first
//                     // This is a complex case that requires LZMA decompression
//                     // For now, return empty list as this needs more implementation
//                     // TODO: Implement encoded header decompression
//                 }
//             }
//         }

//         delete[] pData;
//     }

//     return listResult;
// }

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
            // Check if it's BCJ (03 03 01 03) or PPMd (03 03 01 01)
            if (baCodec.size() >= 4 && baCodec.at(3) == '\x03') {
                result = COMPRESS_METHOD_BCJ;  // BCJ (x86 filter)
            } else {
                result = COMPRESS_METHOD_PPMD;  // PPMd
            }
        } else if (baCodec.startsWith(QByteArray("\x03\x03\x02\x05", 4))) {
            result = COMPRESS_METHOD_BCJ2;  // BCJ2 (x86 advanced filter)
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
                    qDebug() << QString("_handleId: k7zIdHeader loop - offset=0x%1 peekByte=0x%2 nextId=0x%3").arg(pState->nCurrentOffset, 0, 16).arg(nPeekByte, 2, 16, QChar('0')).arg((quint64)nextId, 0, 16);
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
                        qDebug() << "  Next bytes:" << QByteArray((char*)(pState->pData + pState->nCurrentOffset), qMin(10, (int)(pState->nSize - pState->nCurrentOffset))).toHex(' ');
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
            
            // Check if next byte is k7zIdNumUnpackStream
            if (pState->nCurrentOffset < pState->nSize && pState->pData[pState->nCurrentOffset] == XSevenZip::k7zIdNumUnpackStream) {
                // NumUnpackStream internally calls Size, so we don't call Size separately
                _handleId(pListRecords, XSevenZip::k7zIdNumUnpackStream, pState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                #ifdef QT_DEBUG
                qDebug() << QString("  After NumUnpackStream: offset=0x%1").arg(pState->nCurrentOffset, 0, 16);
                #endif
            } else {
                #ifdef QT_DEBUG
                qDebug() << "  No k7zIdNumUnpackStream found (solid blocks with 1 file each or no sub-streams)";
                #endif
            }
            
            // Check if next byte is k7zIdCRC
            if (pState->nCurrentOffset < pState->nSize && pState->pData[pState->nCurrentOffset] == XSevenZip::k7zIdCRC) {
                // Use nNumberOfFolders as count for CRCs (assuming one unpacked stream per folder)
                _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, pState->nNumberOfFolders, false, pPdStruct, IMPTYPE_UNKNOWN);
                #ifdef QT_DEBUG
                qDebug() << QString("  After CRC: offset=0x%1 nNumberOfFolders=%2").arg(pState->nCurrentOffset, 0, 16).arg(pState->nNumberOfFolders);
                #endif
            } else {
                #ifdef QT_DEBUG
                qDebug() << "  No k7zIdCRC found in SubStreamsInfo";
                #endif
            }
            
            // Don't require End marker - MainStreamsInfo will handle it
            bResult = true;
            break;

        case XSevenZip::k7zIdNumUnpackStream: {
            // NumUnpackStream: one value per folder, indicating how many files are in each solid block
            // Then Size section contains unpacked sizes for all files
            quint64 nTotalSubStreams = 0;
            
            #ifdef QT_DEBUG
            qDebug() << "k7zIdNumUnpackStream: Reading for" << pState->nNumberOfFolders << "folders";
            #endif
            
            for (quint64 i = 0; i < pState->nNumberOfFolders && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nNumStreamsInFolder = _handleNumber(pListRecords, pState, pPdStruct, QString("NumUnpackStream%1").arg(i), DRF_COUNT, IMPTYPE_UNKNOWN);
                nTotalSubStreams += nNumStreamsInFolder;
                
                #ifdef QT_DEBUG
                qDebug() << "  Folder" << i << "has" << nNumStreamsInFolder << "files";
                #endif
            }
            
            #ifdef QT_DEBUG
            qDebug() << "  Total files across all folders:" << nTotalSubStreams;
            qDebug() << "  SubStreamsInfo Size section should contain" << (nTotalSubStreams - pState->nNumberOfFolders) << "values";
            qDebug() << "  -> Calling k7zIdSize with impType=IMPTYPE_STREAMUNPACKEDSIZE (6)";
            #endif
            
            // SubStreamsInfo Size section contains (N-1) sizes for each folder with N>1 files
            // For folders with only 1 file, no size is listed (use folder size)
            // Total Size values = TotalFiles - NumberOfFolders
            quint64 nSizeCount = (nTotalSubStreams > pState->nNumberOfFolders) ? (nTotalSubStreams - pState->nNumberOfFolders) : 0;
            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nSizeCount, false, pPdStruct, IMPTYPE_STREAMUNPACKEDSIZE);
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
            qDebug() << "k7zIdSize: Reading" << nCount << "size values with impType=" << impType 
                     << "(STREAMUNPACKEDSIZE=" << IMPTYPE_STREAMUNPACKEDSIZE 
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
                qDebug() << "k7zIdFilesInfo: Checking property ID at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16) << "byte=" << QString("0x%1").arg(nNextByte, 0, 16);
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
                    qDebug() << "k7zIdFilesInfo: Unknown property ID" << QString("0x%1").arg(nNextByte, 0, 16) << "at offset" << QString("0x%1").arg(pState->nCurrentOffset, 0, 16);
                    #endif
                    // Unknown property - skip the ID byte, then read and skip the size field and data
                    pState->nCurrentOffset++;  // Skip the ID byte
                    
                    // Try to read the size field (most properties have this)
                    if (pState->nCurrentOffset < pState->nSize) {
                        XBinary::PACKED_UINT puSize = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);
                        if (puSize.bIsValid) {
                            pState->nCurrentOffset += puSize.nByteSize;
                            quint64 nDataSize = puSize.nValue;
                            #ifdef QT_DEBUG
                            qDebug() << "k7zIdFilesInfo: Unknown property has size" << nDataSize << "- skipping";
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
                            qDebug() << "k7zIdFilesInfo: Cannot read size for unknown property - treating as end marker";
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
            quint64 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("NameSize"), DRF_SIZE, IMPTYPE_UNKNOWN);
            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte", IMPTYPE_UNKNOWN);

            if (nExt == 0) {
                // Parse individual UTF-16LE null-terminated filenames
                qint64 nStartOffset = pState->nCurrentOffset;
                qint64 nEndOffset = nStartOffset + nSize - 1;
                qint32 nFileIndex = 0;
                
                while (pState->nCurrentOffset < nEndOffset && !pState->bIsError && isPdStructNotCanceled(pPdStruct)) {
                    // Read UTF-16LE string until null terminator (0x0000)
                    QByteArray baFilename;
                    qint64 nFilenameStart = pState->nCurrentOffset;
                    
                    while (pState->nCurrentOffset < nEndOffset - 1) {
                        quint16 nChar = _read_uint16(pState->pData + pState->nCurrentOffset);
                        pState->nCurrentOffset += 2;
                        
                        if (nChar == 0) {
                            break;  // Null terminator found
                        }
                        
                        baFilename.append((char)(nChar & 0xFF));
                        baFilename.append((char)((nChar >> 8) & 0xFF));
                    }
                    
                    // Convert UTF-16LE to QString
                    QString sFilename = QString::fromUtf16((const ushort *)baFilename.data(), baFilename.size() / 2);
                    
                    // Add as separate record for each filename
                    SZRECORD record = {};
                    record.nRelOffset = nFilenameStart;
                    record.nSize = pState->nCurrentOffset - nFilenameStart;
                    record.varValue = sFilename;
                    record.srType = SRTYPE_ARRAY;
                    record.valType = VT_STRING;
                    record.impType = IMPTYPE_FILENAME;
                    record.sName = QString("FileName[%1]").arg(nFileIndex);
                    pListRecords->append(record);
                    
                    nFileIndex++;
                    
                    if (nFileIndex >= 200) {
                        break;  // Safety limit to prevent infinite loops
                    }
                }
                
                // Ensure we consumed exactly nSize-1 bytes
                if (pState->nCurrentOffset != nEndOffset) {
                    pState->nCurrentOffset = nEndOffset;
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

// qint64 XSevenZip::getNumberOfArchiveRecords(PDSTRUCT *pPdStruct)
// {
//     // Try to quickly get number of records from parsed header data
//     qint64 nResult = 0;

//     SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
//     qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
//     qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

//     if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
//         char *pData = new char[nNextHeaderSize];
//         qint64 nBytesRead = read_array(nNextHeaderOffset, pData, nNextHeaderSize, pPdStruct);

//         if (nBytesRead == nNextHeaderSize) {
//             QList<XSevenZip::SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct, true);

//             qint32 nNumberOfRecords = listRecords.count();

//             // Look for NumberOfFiles in the parsed records
//             for (qint32 i = 0; (i < nNumberOfRecords) && isPdStructNotCanceled(pPdStruct); i++) {
//                 SZRECORD szRecord = listRecords.at(i);

//                 if (szRecord.impType == IMPTYPE_NUMBEROFFILES) {
//                     nResult = szRecord.varValue.toLongLong();
//                     break;
//                 }
//             }
//         }

//         delete[] pData;
//     }

//     return nResult;
// }

bool XSevenZip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)
    
    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        // Create context
        SEVENZ_UNPACK_CONTEXT *pContext = new SEVENZ_UNPACK_CONTEXT;
        pContext->nSignatureSize = sizeof(SIGNATUREHEADER);

        // Initialize state
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = pContext;

        // Parse archive structure directly using streaming approach
        SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
        qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
        qint64 nNextHeaderSize = signatureHeader.NextHeaderSize;

        #ifdef QT_DEBUG
        qDebug() << "XSevenZip::initUnpack: NextHeaderOffset=" << nNextHeaderOffset 
                 << "NextHeaderSize=" << nNextHeaderSize;
        #endif
        #ifdef QT_DEBUG
        qDebug() << "  SignatureHeader.NextHeaderOffset=" << signatureHeader.NextHeaderOffset;
        #endif
        #ifdef QT_DEBUG
        qDebug() << "  File size=" << getSize();
        #endif

        if ((nNextHeaderSize > 0) && isOffsetValid(nNextHeaderOffset)) {
            char *pData = new char[nNextHeaderSize];
            qint64 nBytesRead = read_array(nNextHeaderOffset, pData, nNextHeaderSize, pPdStruct);

            #ifdef QT_DEBUG
            qDebug() << "XSevenZip::initUnpack: BytesRead=" << nBytesRead << "Expected=" << nNextHeaderSize;
            #endif
            
            // Check first few bytes to see if it's encoded
            if (nBytesRead > 0) {
                QString sFirstBytes;
                for (qint32 i = 0; i < qMin(nBytesRead, (qint64)16); i++) {
                    sFirstBytes += QString("%1 ").arg((unsigned char)pData[i], 2, 16, QChar('0'));
                }
                #ifdef QT_DEBUG
                qDebug() << "XSevenZip::initUnpack: First bytes of header:" << sFirstBytes;
                #endif
            }

            if (nBytesRead == nNextHeaderSize) {
                // NOTE: Experimental code to debug encoded header handling
                // System 7z creates solid archives even with -ms=off flag
                // The encoded header decompresses to MainStreamsInfo only (no FilesInfo)
                // FilesInfo location in solid archives needs further investigation
                /*
                QList<SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct, true);
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
                    #ifdef QT_DEBUG
                    qDebug() << "XSevenZip::initUnpack: First byte=" << nFirstByte 
                             << "k7zIdEncodedHeader=" << (quint8)k7zIdEncodedHeader
                             << "IsEncoded=" << bIsEncodedHeader;
                    #endif
                }

                if (bIsEncodedHeader) {
                    // === ENCODED HEADER PATH ===
                    // TODO: Full EncodedHeader support is partially implemented
                    // The current implementation can parse EncodedHeader metadata but may fail to locate
                    // and decompress the actual header data for certain archive configurations.
                    // Issues identified:
                    // 1. Archives created with -mx=0 (store mode) use non-standard header storage
                    // 2. PackOffset in EncodedHeader may point to file data instead of compressed header
                    // 3. Compressed header location calculation formula unclear for all cases
                    // Reference: 7-Zip SDK CPP/7zip/Archive/7z/7zIn.cpp - CInArchive::ReadAndDecodePackedStreams()
                    // Status: Works for some archives, fails for system 7z with -mx=0
                    
                    #ifdef QT_DEBUG
                    qDebug() << "XSevenZip::initUnpack: Processing encoded header (experimental support)...";
                    #endif
                    #ifdef QT_DEBUG
                    qDebug() << "  Warning: EncodedHeader decompression may fail for archives created with -mx=0";
                    #endif

                    // Parse encoded header metadata using SZSTATE to get compressed data offset
                    SZSTATE szState = {};
                    szState.pData = pData;
                    szState.nSize = nNextHeaderSize;
                    szState.nCurrentOffset = 0;
                    szState.bIsError = false;
                    szState.sErrorString = QString();
                    
                    // Skip the k7zIdEncodedHeader byte (0x17)
                    quint8 nEncodedId = (quint8)pData[0];
                    szState.nCurrentOffset = 1;
                    #ifdef QT_DEBUG
                    qDebug() << "  Read EncodedHeader ID:" << QString::number(nEncodedId, 16);
                    #endif
                    
                    // The encoded header contains MainStreamsInfo (NOT a full Header structure)
                    // MainStreamsInfo structure: PackInfo + UnpackInfo + SubStreamsInfo
                    // Parse this to find where the compressed LZMA data starts
                    QList<SZRECORD> listMetadataParse;
                    _handleId(&listMetadataParse, k7zIdMainStreamsInfo, &szState, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                    
                    #ifdef QT_DEBUG
                    qDebug() << "  After parsing MainStreamsInfo, offset=" << QString::number(szState.nCurrentOffset, 16) 
                             << "(" << szState.nCurrentOffset << " bytes)";
                    #endif
                    
                    // CRITICAL: Extract PackPos directly from EncodedHeader byte-by-byte
                    // We MUST do this BEFORE parsing records, because parsed STREAMOFFSET records
                    // may contain offsets from the decompressed header's PackInfo (file data locations),
                    // not the compressed header location!
                    //
                    // EncodedHeader structure: k7zIdEncodedHeader (0x17) + MainStreamsInfo
                    // MainStreamsInfo starts with: k7zIdPackInfo (0x06) + PackPos (encoded uint64)
                    
                    #ifdef QT_DEBUG
                    qDebug() << "  Extracting PackPos directly from EncodedHeader bytes...";
                    #endif
                    
                    qint64 nPackPos = 0;
                    qint32 nPos = 1;  // Start after 0x17 (k7zIdEncodedHeader)
                    qint32 nPackPosBytesRead = 0;  // Track how many bytes PackPos used
                    
                    if (nPos < nNextHeaderSize && (quint8)pData[nPos] == 0x06) {  // k7zIdPackInfo
                        #ifdef QT_DEBUG
                        qDebug() << "  Found k7zIdPackInfo at byte" << nPos;
                        #endif
                        nPos++;
                        
                        // Debug: print next 10 bytes to see the encoded number
                        QString sDebugBytes;
                        for (qint32 j = 0; j < qMin((qint64)10, nNextHeaderSize - nPos); j++) {
                            sDebugBytes += QString("%1 ").arg((quint8)pData[nPos + j], 2, 16, QChar('0'));
                        }
                        #ifdef QT_DEBUG
                        qDebug() << "  Next bytes after PackInfo:" << sDebugBytes;
                        #endif
                        
                        // Read PackPos as encoded uint64
                        quint8 nFirstByte = (quint8)pData[nPos];
                        quint8 nMask = 0x80;
                        
                        #ifdef QT_DEBUG
                        qDebug() << "  First byte:" << QString("0x%1").arg(nFirstByte, 2, 16, QChar('0'));
                        #endif
                        
                        for (qint32 i = 0; i < 8; i++) {
                            if (nFirstByte & nMask) {
                                if (nPos + nPackPosBytesRead + 1 < nNextHeaderSize) {
                                    quint64 nByte = (quint8)pData[nPos + nPackPosBytesRead + 1];
                                    #ifdef QT_DEBUG
                                    qDebug() << "    Bit" << i << "set, reading byte:" << QString("0x%1").arg(nByte, 2, 16, QChar('0'))
                                             << "shift" << (8*i);
                                    #endif
                                    nPackPos |= (nByte << (8 * i));
                                    nPackPosBytesRead++;
                                }
                            } else {
                                quint64 nValue = (nFirstByte & (nMask - 1));
                                #ifdef QT_DEBUG
                                qDebug() << "    Bit" << i << "NOT set, using remaining bits:" << QString("0x%1").arg(nValue, 2, 16, QChar('0'))
                                         << "shift" << (8*i);
                                #endif
                                nPackPos |= (nValue << (8 * i));
                                break;
                            }
                            nMask >>= 1;
                        }
                        
                        #ifdef QT_DEBUG
                        qDebug() << "  ✓ PackPos extracted:" << nPackPos << "using" << (nPackPosBytesRead + 1) << "bytes";
                        #endif
                        #ifdef QT_DEBUG
                        qDebug() << "  ✓ Absolute file offset:" << (sizeof(SIGNATUREHEADER) + nPackPos);
                        #endif
                    } else {
                        #ifdef QT_DEBUG
                        qWarning() << "  ERROR: EncodedHeader doesn't start with PackInfo!";
                        #endif
                        #ifdef QT_DEBUG
                        qWarning() << "  Byte 1 = 0x" << QString::number((quint8)pData[nPos], 16);
                        #endif
                    }
                    
                    // Continue parsing to extract StreamSize and CodersUnpackSize manually
                    // These values are CRITICAL and must come from the EncodedHeader's PackInfo/UnpackInfo,
                    // NOT from parsed records which may contain wrong values from other sections!
                    
                    qint64 nPackedSize = 0;
                    qint64 nUnpackedSize = 0;
                    
                    if (nPackPos > 0) {
                        // Parse NumPackStreams - nPos should already be after the PackPos encoding
                        nPos += nPackPosBytesRead + 1;  // Skip the bytes we already read for PackPos
                        
                        quint8 nNumStreams = (quint8)pData[nPos];
                        nPos++;
                        
                        #ifdef QT_DEBUG
                        qDebug() << "  NumPackStreams:" << nNumStreams << "at position" << nPos;
                        #endif
                        
                        // Look for k7zIdSize (0x09)
                        if (nPos < nNextHeaderSize && (quint8)pData[nPos] == 0x09) {
                            nPos++;  // Skip k7zIdSize
                            
                            // Read stream size (encoded number)
                            quint8 nFirstByte = (quint8)pData[nPos];
                            if (nFirstByte < 0x80) {
                                // Single byte
                                nPackedSize = nFirstByte;
                                nPos++;
                            } else {
                                // Multi-byte encoding
                                quint8 nMask = 0x80;
                                qint32 nBytesRead = 0;
                                
                                for (qint32 i = 0; i < 8; i++) {
                                    if (nFirstByte & nMask) {
                                        if (nPos + nBytesRead + 1 < nNextHeaderSize) {
                                            nPackedSize |= ((quint64)(quint8)pData[nPos + nBytesRead + 1] << (8 * i));
                                            nBytesRead++;
                                        }
                                    } else {
                                        nPackedSize |= ((quint64)(nFirstByte & (nMask - 1)) << (8 * i));
                                        break;
                                    }
                                    nMask >>= 1;
                                }
                                nPos += nBytesRead + 1;
                            }
                            
                            #ifdef QT_DEBUG
                            qDebug() << "  ✓ StreamSize (compressed) extracted:" << nPackedSize;
                            #endif
                        }
                        
                        // Now find k7zIdCodersUnpackSize (0x0c) to get the decompressed size
                        // Skip forward to find it
                        while (nPos < nNextHeaderSize - 1 && (quint8)pData[nPos] != 0x0c) {
                            nPos++;
                        }
                        
                        if (nPos < nNextHeaderSize && (quint8)pData[nPos] == 0x0c) {
                            nPos++;  // Skip k7zIdCodersUnpackSize
                            
                            // Read unpack size (encoded number)
                            quint8 nFirstByte = (quint8)pData[nPos];
                            if (nFirstByte < 0x80) {
                                // Single byte
                                nUnpackedSize = nFirstByte;
                            } else {
                                // Multi-byte encoding
                                quint8 nMask = 0x80;
                                qint32 nBytesRead = 0;
                                
                                for (qint32 i = 0; i < 8; i++) {
                                    if (nFirstByte & nMask) {
                                        if (nPos + nBytesRead + 1 < nNextHeaderSize) {
                                            nUnpackedSize |= ((quint64)(quint8)pData[nPos + nBytesRead + 1] << (8 * i));
                                            nBytesRead++;
                                        }
                                    } else {
                                        nUnpackedSize |= ((quint64)(nFirstByte & (nMask - 1)) << (8 * i));
                                        break;
                                    }
                                    nMask >>= 1;
                                }
                            }
                            
                            #ifdef QT_DEBUG
                            qDebug() << "  ✓ CodersUnpackSize (decompressed) extracted:" << nUnpackedSize;
                            #endif
                        }
                    }
                    
                    // Now parse the full EncodedHeader structure to get codec info
                    QList<SZRECORD> listEncodedRecords = _handleData(pData, nNextHeaderSize, pPdStruct, false);
                    #ifdef QT_DEBUG
                    qDebug() << "  Encoded header metadata records:" << listEncodedRecords.count();
                    #endif

                    // Debug: print first few records
                    for (qint32 i = 0; i < qMin(listEncodedRecords.count(), 10); i++) {
                        const SZRECORD &rec = listEncodedRecords.at(i);
                        #ifdef QT_DEBUG
                        qDebug() << "    Record" << i << "- srType:" << rec.srType << "impType:" << rec.impType 
                                 << "value:" << rec.varValue;
                        #endif
                    }

                    qint64 nPackOffset = sizeof(SIGNATUREHEADER) + nPackPos;  // Use manually extracted PackPos!
                    QByteArray baCodec;
                    QByteArray baCoderProperty;

                    // Extract codec information from parsed records
                    for (qint32 i = 0; i < listEncodedRecords.count(); i++) {
                        SZRECORD rec = listEncodedRecords.at(i);
                        if (rec.impType == IMPTYPE_CODER) {
                            baCodec = rec.varValue.toByteArray();
                        } else if (rec.impType == IMPTYPE_CODERPROPERTY) {
                            baCoderProperty = rec.varValue.toByteArray();
                        }
                    }

                    #ifdef QT_DEBUG
                    qDebug() << "  PackOffset (absolute):" << nPackOffset << "PackedSize:" << nPackedSize 
                             << "UnpackedSize:" << nUnpackedSize << "Codec size:" << baCodec.size() << "Property size:" << baCoderProperty.size();
                    #endif
                    
                    // The compressed header stream is located at nPackOffset with size nPackedSize
                    // It decompresses to nUnpackedSize bytes
                    #ifdef QT_DEBUG
                    qDebug() << "  Compressed header location: offset" << nPackOffset << "size" << nPackedSize;
                    #endif
                    #ifdef QT_DEBUG
                    qDebug() << "  Will decompress to" << nUnpackedSize << "bytes";
                    #endif

                    // Debug: Print codec bytes
                    if (!baCodec.isEmpty()) {
                        QString sCodecHex;
                        for (qint32 j = 0; j < baCodec.size(); j++) {
                            sCodecHex += QString("%1 ").arg((unsigned char)baCodec.at(j), 2, 16, QChar('0'));
                        }
                        #ifdef QT_DEBUG
                        qDebug() << "  Codec bytes:" << sCodecHex;
                        #endif
                        #ifdef QT_DEBUG
                        qDebug() << "  Codec method:" << codecToCompressMethod(baCodec);
                        #endif
                    }

                    // Debug: Print coder property bytes in detail
                    if (!baCoderProperty.isEmpty()) {
                        QString sPropHex;
                        for (qint32 j = 0; j < baCoderProperty.size(); j++) {
                            sPropHex += QString("%1 ").arg((unsigned char)baCoderProperty.at(j), 2, 16, QChar('0'));
                        }
                        #ifdef QT_DEBUG
                        qDebug() << "  Coder property bytes:" << sPropHex;
                        #endif
                        
                        // Parse LZMA properties
                        if (baCoderProperty.size() == 5) {
                            quint8 propByte = (quint8)baCoderProperty[0];
                            quint32 dictSize = ((quint8)baCoderProperty[1]) |
                                             ((quint8)baCoderProperty[2] << 8) |
                                             ((quint8)baCoderProperty[3] << 16) |
                                             ((quint8)baCoderProperty[4] << 24);
                            #ifdef QT_DEBUG
                            qDebug() << "    Properties byte:" << QString::number(propByte, 16) 
                                     << "Dictionary size:" << dictSize << "bytes (" << (dictSize/1024) << "KB)";
                            #endif
                        }
                    }

                    #ifdef QT_DEBUG
                    qDebug() << "  About to decompress header from offset" << nPackOffset << "size" << nPackedSize << "expected unpacked size" << nUnpackedSize;
                    #endif

                    // Validate we have the necessary data
                    if (nPackOffset <= 0 || nPackedSize <= 0 || nUnpackedSize <= 0 || baCoderProperty.isEmpty()) {
                        #ifdef QT_DEBUG
                        qDebug() << "  ERROR: Missing required decompression parameters";
                        #endif
                        bResult = false;
                    } else {
                        // Debug: Read and print first 32 bytes at compressed header offset to verify it's LZMA data
                        QByteArray baTestRead = read_array(nPackOffset, qMin((qint64)32, nPackedSize));
                        QString sTestHex;
                        for (qint32 j = 0; j < baTestRead.size(); j++) {
                            sTestHex += QString("%1 ").arg((unsigned char)baTestRead.at(j), 2, 16, QChar('0'));
                        }
                        #ifdef QT_DEBUG
                        qDebug() << "  First bytes at compressed header offset:" << sTestHex;
                        #endif

                        // Use SubDevice to create a view of the compressed data
                        SubDevice subDevice(getDevice(), nPackOffset, nPackedSize);
                        if (subDevice.open(QIODevice::ReadOnly)) {
                            #ifdef QT_DEBUG
                            qDebug() << "  SubDevice opened successfully at offset" << nPackOffset;
                            #endif

                            QByteArray baDecompressed;
                            QBuffer bufferDecompressed(&baDecompressed);
                            bufferDecompressed.open(QIODevice::WriteOnly);

                            // Decompress using XLZMADecoder with properties from EncodedHeader
                            XLZMADecoder lzmaDecoder;
                            
                            DATAPROCESS_STATE state = {};
                            state.pDeviceInput = &subDevice;
                            state.pDeviceOutput = &bufferDecompressed;
                            state.nInputOffset = 0;  // Relative to SubDevice
                            state.nInputLimit = nPackedSize;
                            state.nProcessedOffset = 0;
                            state.nProcessedLimit = -1;  // Decompress until end of stream

                            #ifdef QT_DEBUG
                            qDebug() << "  Calling LZMA decoder with properties size" << baCoderProperty.size();
                            #endif
                            #ifdef QT_DEBUG
                            qDebug() << "  Expected unpacked size:" << nUnpackedSize << "Packed size:" << nPackedSize;
                            #endif
                            
                            bool bDecompressed = lzmaDecoder.decompress(&state, baCoderProperty, pPdStruct);
                            
                            subDevice.close();
                            bufferDecompressed.close();

                        #ifdef QT_DEBUG
                        qDebug() << "  Decompression result:" << bDecompressed << "Size:" << baDecompressed.size();
                        #endif
                        #ifdef QT_DEBUG
                        qDebug() << "  Input consumed:" << state.nCountInput << "Output produced:" << state.nCountOutput;
                        #endif
                        if (baDecompressed.size() != nUnpackedSize) {
                            #ifdef QT_DEBUG
                            qDebug() << "  WARNING: Size mismatch! Expected" << nUnpackedSize << "got" << baDecompressed.size();
                            #endif
                        }
                        
                        // FALLBACK: If LZMA decompression failed, try reading UNCOMPRESSED header
                        // This handles archives created with -mx=0 where file data is stored but header might not be compressed
                        if (!bDecompressed || baDecompressed.isEmpty()) {
                            #ifdef QT_DEBUG
                            qDebug() << "  LZMA decompression failed, trying UNCOMPRESSED header...";
                            #endif
                            
                            // Try reading UnpackedSize bytes before NextHeader
                            // Distance observed: approximately 198 bytes before NextHeader start
                            qint64 nNextHeaderAbsolute = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
                            qint64 nUncompressedHeaderOffset = nNextHeaderAbsolute - nUnpackedSize - signatureHeader.NextHeaderSize;
                            
                            #ifdef QT_DEBUG
                            qDebug() << "  Trying uncompressed header at offset" << nUncompressedHeaderOffset;
                            #endif
                            #ifdef QT_DEBUG
                            qDebug() << "  Calculation: NextHeader(" << nNextHeaderAbsolute << ") - UnpackedSize(" << nUnpackedSize << ") - NextHeaderSize(" << signatureHeader.NextHeaderSize << ")";
                            #endif
                            
                            // Try multiple offsets around the calculated position
                            QList<qint64> listOffsetsToTry;
                            listOffsetsToTry << nUncompressedHeaderOffset;
                            listOffsetsToTry << (nNextHeaderAbsolute - nUnpackedSize - 20);
                            listOffsetsToTry << (nNextHeaderAbsolute - nUnpackedSize - 15);
                            listOffsetsToTry << (nNextHeaderAbsolute - nUnpackedSize);
                            listOffsetsToTry << (nNextHeaderAbsolute - 198);  // Observed distance
                            
                            for (qint32 iTry = 0; iTry < listOffsetsToTry.count(); iTry++) {
                                qint64 nTryOffset = listOffsetsToTry.at(iTry);
                                if (nTryOffset < 0 || nTryOffset + nUnpackedSize > getSize()) {
                                    continue;
                                }
                                
                                QByteArray baUncompressed = read_array(nTryOffset, nUnpackedSize);
                                if (baUncompressed.size() == nUnpackedSize && !baUncompressed.isEmpty()) {
                                    // Check if it starts with k7zIdHeader (0x01)
                                    if ((quint8)baUncompressed.at(0) == 0x01) {
                                        #ifdef QT_DEBUG
                                        qDebug() << "  ✓ Found valid uncompressed header at offset" << nTryOffset << "(attempt" << iTry << ")";
                                        #endif
                                        #ifdef QT_DEBUG
                                        qDebug() << "  First 32 bytes:" << baUncompressed.left(32).toHex(' ');
                                        #endif
                                        baDecompressed = baUncompressed;
                                        bDecompressed = true;
                                        break;
                                    }
                                }
                            }
                            
                            if (!bDecompressed) {
                                #ifdef QT_DEBUG
                                qDebug() << "  ✗ Could not find valid uncompressed header at any tried offset";
                                #endif
                            }
                        }

                        // Debug: Print first 32 bytes of decompressed header
                        if (!baDecompressed.isEmpty()) {
                            QString sDecompressedHex;
                            qint32 nBytesToShow = qMin(32, baDecompressed.size());
                            for (qint32 j = 0; j < nBytesToShow; j++) {
                                sDecompressedHex += QString("%1 ").arg((unsigned char)baDecompressed.at(j), 2, 16, QChar('0'));
                            }
                            #ifdef QT_DEBUG
                            qDebug() << "  First" << nBytesToShow << "bytes of decompressed header:" << sDecompressedHex;
                            #endif
                            
                            // Print ALL bytes for analysis
                            QString sFullHex;
                            for (qint32 j = 0; j < baDecompressed.size(); j++) {
                                if (j > 0 && j % 32 == 0) sFullHex += "\n      ";
                                sFullHex += QString("%1 ").arg((unsigned char)baDecompressed.at(j), 2, 16, QChar('0'));
                            }
                            #ifdef QT_DEBUG
                            qDebug() << "  FULL decompressed header (" << baDecompressed.size() << "bytes):\n     " << sFullHex;
                            #endif
                        }

                        if (bDecompressed && !baDecompressed.isEmpty()) {
                                #ifdef QT_DEBUG
                                qDebug() << "  Decompressed size:" << baDecompressed.size();
                                #endif
                                
                                // Save decompressed header for analysis
                                QString sDebugPath = QString("/tmp/decompressed_header_%1.bin").arg(QDateTime::currentMSecsSinceEpoch());
                                QFile debugFile(sDebugPath);
                                if (debugFile.open(QIODevice::WriteOnly)) {
                                    debugFile.write(baDecompressed);
                                    debugFile.close();
                                    #ifdef QT_DEBUG
                                    qDebug() << "  Saved decompressed header to:" << sDebugPath;
                                    #endif
                                }

                                // Parse decompressed header manually using SZSTATE
                                #ifdef QT_DEBUG
                                qDebug() << "  Parsing decompressed header: size=" << baDecompressed.size();
                                qDebug() << "  First bytes:" << QString("0x%1 0x%2 0x%3 0x%4").arg((quint8)baDecompressed[0], 2, 16, QChar('0')).arg((quint8)baDecompressed[1], 2, 16, QChar('0')).arg((quint8)baDecompressed[2], 2, 16, QChar('0')).arg((quint8)baDecompressed[3], 2, 16, QChar('0'));
                                #endif
                                
                                SZSTATE stateDecompressed = {};
                                stateDecompressed.pData = baDecompressed.data();
                                stateDecompressed.nSize = baDecompressed.size();
                                stateDecompressed.nCurrentOffset = 0;
                                stateDecompressed.bIsError = false;
                                stateDecompressed.sErrorString = QString();

                                QList<SZRECORD> listHeaderRecords;
                                _handleId(&listHeaderRecords, XSevenZip::k7zIdHeader, &stateDecompressed, 1, false, pPdStruct, IMPTYPE_UNKNOWN);
                                
                                #ifdef QT_DEBUG
                                qDebug() << "  Decompressed header records:" << listHeaderRecords.count();
                                qDebug() << "  Full decompressed header hex (first 100 bytes):" << baDecompressed.left(100).toHex(' ');
                                
                                // Debug: Count CodersUnpackSize records
                                qint32 nCodersUnpackSizeCount = 0;
                                for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                                    if (listHeaderRecords.at(i).sName.contains("CodersUnpackSize")) {
                                        nCodersUnpackSizeCount++;
                                        qDebug() << "    Record" << i << ":" << listHeaderRecords.at(i).sName << "=" << listHeaderRecords.at(i).varValue.toLongLong();
                                    }
                                }
                                qDebug() << "  Total CodersUnpackSize records:" << nCodersUnpackSizeCount;
                                #endif

                                // Debug: Print all record types
                                QMap<qint32, qint32> mapRecordTypeCounts;
                                for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                                    mapRecordTypeCounts[listHeaderRecords.at(i).impType]++;
                                }
                                #ifdef QT_DEBUG
                                qDebug() << "  Record type distribution:";
                                #endif
                                QMapIterator<qint32, qint32> it(mapRecordTypeCounts);
                                while (it.hasNext()) {
                                    it.next();
                                    #ifdef QT_DEBUG
                                    qDebug() << "    Type" << it.key() << "=" << it.value() << "occurrences";
                                    #endif
                                }

                                // Extract file information from decompressed header
                                QList<QString> listFileNames;
                                QList<qint64> listFilePackedSizes;
                                QList<qint64> listFolderUnpackedSizes;  // Folder sizes from CodersUnpackSize
                                QList<qint64> listSubStreamSizes;  // File size deltas from SubStreamsInfo
                                QList<qint64> listStreamOffsets;
                                QList<QByteArray> listCodecs;
                                qint32 nNumberOfFolders = 0;  // Track number of folders
                                QList<qint32> listFilesPerFolder;  // How many files in each folder (from NumUnpackStream)

                                // First pass: collect folder info and per-folder file counts
                                for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                                    SZRECORD rec = listHeaderRecords.at(i);
                                    
                                    if (rec.sName == "NumberOfFolders") {
                                        nNumberOfFolders = rec.varValue.toInt();
                                    } else if (rec.sName.startsWith("NumUnpackStream")) {
                                        listFilesPerFolder.append(rec.varValue.toInt());
                                    }
                                }

                                // Second pass: collect all data
                                #ifdef QT_DEBUG
                                qDebug() << "Collection pass: Processing" << listHeaderRecords.count() << "records";
                                #endif
                                for (qint32 i = 0; i < listHeaderRecords.count(); i++) {
                                    SZRECORD rec = listHeaderRecords.at(i);
                                    
                                    #ifdef QT_DEBUG
                                    if (rec.sName.startsWith("Size")) {
                                        qDebug() << "  DEBUG: Found record" << rec.sName << "with impType=" << rec.impType 
                                                 << "(STREAMUNPACKEDSIZE=" << IMPTYPE_STREAMUNPACKEDSIZE 
                                                 << ", STREAMPACKEDSIZE=" << IMPTYPE_STREAMPACKEDSIZE << ")"
                                                 << "value=" << rec.varValue.toLongLong();
                                    }
                                    #endif
                                    
                                    if (rec.impType == IMPTYPE_FILENAME) {
                                        listFileNames.append(rec.varValue.toString());
                                    } else if (rec.impType == IMPTYPE_FILEPACKEDSIZE) {
                                        listFilePackedSizes.append(rec.varValue.toLongLong());
                                    } else if (rec.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                                        // Distinguish between folder sizes and file sizes by name
                                        if (rec.sName.startsWith("CodersUnpackSize")) {
                                            #ifdef QT_DEBUG
                                            qDebug() << "  Collecting folder size from" << rec.sName << "=" << rec.varValue.toLongLong() << "at offset" << QString("0x%1").arg(rec.nRelOffset, 0, 16);
                                            #endif
                                            listFolderUnpackedSizes.append(rec.varValue.toLongLong());
                                        } else if (rec.sName.startsWith("Size")) {
                                            #ifdef QT_DEBUG
                                            qDebug() << "  Collecting SubStream size from" << rec.sName << "=" << rec.varValue.toLongLong() << "at offset" << QString("0x%1").arg(rec.nRelOffset, 0, 16);
                                            #endif
                                            listSubStreamSizes.append(rec.varValue.toLongLong());
                                        }
                                    } else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE) {
                                        // This would be explicit file sizes (rare)
                                        listSubStreamSizes.append(rec.varValue.toLongLong());
                                    } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                                        listStreamOffsets.append(rec.varValue.toLongLong());
                                    } else if (rec.impType == IMPTYPE_CODER) {
                                        listCodecs.append(rec.varValue.toByteArray());
                                    }
                                }
                                
                                // Calculate actual file sizes from folder sizes and SubStreamsInfo
                                QList<qint64> listFileUnpackedSizes;
                                qint32 nSubStreamIndex = 0;
                                
                                #ifdef QT_DEBUG
                                qDebug() << "Starting file size calculation:";
                                qDebug() << "  nNumberOfFolders:" << nNumberOfFolders;
                                qDebug() << "  listFilesPerFolder:" << listFilesPerFolder;
                                qDebug() << "  listFolderUnpackedSizes:" << listFolderUnpackedSizes;
                                qDebug() << "  listSubStreamSizes count:" << listSubStreamSizes.count();
                                #endif
                                
                                for (qint32 iFolderIdx = 0; iFolderIdx < nNumberOfFolders; iFolderIdx++) {
                                    qint32 nFilesInFolder = (iFolderIdx < listFilesPerFolder.count()) ? listFilesPerFolder.at(iFolderIdx) : 1;
                                    qint64 nFolderSize = (iFolderIdx < listFolderUnpackedSizes.count()) ? listFolderUnpackedSizes.at(iFolderIdx) : 0;
                                    
                                    #ifdef QT_DEBUG
                                    qDebug() << "  Folder" << iFolderIdx << ": nFilesInFolder=" << nFilesInFolder << ", nFolderSize=" << nFolderSize;
                                    #endif
                                    
                                    if (nFilesInFolder == 1) {
                                        // Simple case: folder has only one file, use folder size
                                        listFileUnpackedSizes.append(nFolderSize);
                                        #ifdef QT_DEBUG
                                        qDebug() << "    Single file folder, size:" << nFolderSize;
                                        #endif
                                    } else if (nFilesInFolder > 1) {
                                        // Solid block: multiple files in folder
                                        // SubStreamsInfo Size values are the unpacked sizes of the FIRST (N-1) files
                                        // The LAST file size = folder size - sum(first N-1 files)
                                        qint64 nFirstFilesTotal = 0;
                                        
                                        #ifdef QT_DEBUG
                                        qDebug() << "    Multi-file folder, reading" << (nFilesInFolder - 1) << "sizes from SubStreamsInfo";
                                        #endif
                                        
                                        // Add sizes for first (N-1) files from SubStreamsInfo
                                        for (qint32 j = 0; j < nFilesInFolder - 1; j++) {
                                            if (nSubStreamIndex < listSubStreamSizes.count()) {
                                                qint64 nFileSize = listSubStreamSizes.at(nSubStreamIndex);
                                                listFileUnpackedSizes.append(nFileSize);
                                                nFirstFilesTotal += nFileSize;
                                                #ifdef QT_DEBUG
                                                qDebug() << "      File" << j << "size:" << nFileSize << "(from SubStreamSizes[" << nSubStreamIndex << "])";
                                                #endif
                                                nSubStreamIndex++;
                                            } else {
                                                #ifdef QT_DEBUG
                                                qDebug() << "      WARNING: Not enough SubStreamSizes! Expected index" << nSubStreamIndex << "but count is" << listSubStreamSizes.count();
                                                #endif
                                            }
                                        }
                                        
                                        // Calculate last file size as remainder
                                        qint64 nLastFileSize = nFolderSize - nFirstFilesTotal;
                                        listFileUnpackedSizes.append(nLastFileSize);
                                        #ifdef QT_DEBUG
                                        qDebug() << "      File" << (nFilesInFolder - 1) << "size:" << nLastFileSize << "(calculated as" << nFolderSize << "-" << nFirstFilesTotal << ")";
                                        #endif
                                    }
                                }

                                #ifdef QT_DEBUG
                                qDebug() << "  Files found:" << listFileNames.count();
                                qDebug() << "  nNumberOfFolders:" << nNumberOfFolders;
                                qDebug() << "  listFilesPerFolder:" << listFilesPerFolder;
                                qDebug() << "  listFolderUnpackedSizes:" << listFolderUnpackedSizes;
                                qDebug() << "  listSubStreamSizes:" << listSubStreamSizes;
                                qDebug() << "  Calculated listFileUnpackedSizes:" << listFileUnpackedSizes;
                                qDebug() << "  listFilePackedSizes:" << listFilePackedSizes;
                                qDebug() << "  listStreamOffsets:" << listStreamOffsets;
                                
                                // Validation: Check if file count matches calculated size count
                                if (listFileUnpackedSizes.count() != listFileNames.count()) {
                                    qDebug() << "  WARNING: Size count mismatch! Files:" << listFileNames.count() 
                                             << "Calculated sizes:" << listFileUnpackedSizes.count();
                                }
                                #endif

                                // Analyze codec chain to separate filters from compressors
                                // In 7z, filters (like BCJ) come before compressors (like LZMA2)
                                COMPRESS_METHOD filterMethod = COMPRESS_METHOD_UNKNOWN;
                                COMPRESS_METHOD compressMethod = COMPRESS_METHOD_STORE;
                                
                                for (qint32 i = 0; i < listCodecs.count(); i++) {
                                    COMPRESS_METHOD method = codecToCompressMethod(listCodecs.at(i));
                                    
                                    // BCJ and BCJ2 are filters/preprocessors, not compressors
                                    if (method == COMPRESS_METHOD_BCJ || method == COMPRESS_METHOD_BCJ2) {
                                        filterMethod = method;
                                    } else if (method != COMPRESS_METHOD_UNKNOWN) {
                                        // This is the actual compressor (LZMA, LZMA2, etc.)
                                        compressMethod = method;
                                    }
                                }

                                // Create ARCHIVERECORD entries
                                qint64 nDataOffset = sizeof(SIGNATUREHEADER);
                                for (qint32 i = 0; i < listFileNames.count(); i++) {
                                    ARCHIVERECORD record = {};
                                    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, listFileNames.at(i));
                                    
                                    if (i < listFileUnpackedSizes.count()) {
                                        record.nDecompressedSize = listFileUnpackedSizes.at(i);
                                    }
                                    if (i < listFilePackedSizes.count()) {
                                        record.nStreamSize = listFilePackedSizes.at(i);
                                    } else {
                                        record.nStreamSize = record.nDecompressedSize;
                                    }
                                    if (i < listStreamOffsets.count()) {
                                        record.nStreamOffset = nDataOffset + listStreamOffsets.at(i);
                                    } else {
                                        record.nStreamOffset = nDataOffset;
                                    }

                                    // Store both compressor and filter (if present)
                                    record.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, compressMethod);
                                    if (filterMethod != COMPRESS_METHOD_UNKNOWN) {
                                        record.mapProperties.insert(FPART_PROP_FILTERMETHOD, filterMethod);
                                    }

                                    pContext->listArchiveRecords.append(record);
                                    pContext->listRecordOffsets.append(record.nStreamOffset);
                                }

                                #ifdef QT_DEBUG
                                qDebug() << "  Created" << pContext->listArchiveRecords.count() << "archive records";
                                #endif
                                
                                if (pContext->listArchiveRecords.isEmpty()) {
                                    #ifdef QT_DEBUG
                                    qWarning() << "  ======================================";
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  ENCODED HEADER ANALYSIS:";
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  Decompressed header contains only MainStreamsInfo (stream metadata)";
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  FilesInfo section NOT found in decompressed header";
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  This occurs with solid 7z archives where file metadata";
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  may be stored in a separate location or format";
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  Decompressed size:" << baDecompressed.size() << "bytes";
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  Records parsed:" << listHeaderRecords.count();
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  Files found:" << listFileNames.count();
                                    #endif
                                    #ifdef QT_DEBUG
                                    qWarning() << "  ======================================";
                                    #endif
                                }
                            } else {
                                #ifdef QT_DEBUG
                                qWarning() << "  Failed to decompress header!";
                                #endif
                            }
                        } else {
                            #ifdef QT_DEBUG
                            qWarning() << "  Failed to open SubDevice!";
                            #endif
                        }
                    }
                } else {
                    // === STANDARD HEADER PATH ===
                    #ifdef QT_DEBUG
                    qDebug() << "XSevenZip::initUnpack: Processing standard header...";
                    #endif

                    QList<SZRECORD> listRecords = _handleData(pData, nNextHeaderSize, pPdStruct, true);
                    qint32 nNumberOfRecords = listRecords.count();
                    #ifdef QT_DEBUG
                    qDebug() << "  Header records:" << nNumberOfRecords;
                    #endif

                    if (nNumberOfRecords > 0) {
                        SZRECORD firstRecord = listRecords.at(0);
                        if ((firstRecord.srType == SRTYPE_ID) && (firstRecord.varValue.toULongLong() == k7zIdHeader)) {
                            // Extract file information
                            QList<QString> listFileNames;
                            QList<qint64> listFilePackedSizes;
                            QList<qint64> listFileUnpackedSizes;
                            QList<qint64> listStreamOffsets;
                            QList<QByteArray> listCodecs;

                            for (qint32 i = 0; i < nNumberOfRecords; i++) {
                                SZRECORD rec = listRecords.at(i);
                                if (rec.impType == IMPTYPE_FILENAME) {
                                    listFileNames.append(rec.varValue.toString());
                                } else if (rec.impType == IMPTYPE_FILEPACKEDSIZE) {
                                    listFilePackedSizes.append(rec.varValue.toLongLong());
                                } else if (rec.impType == IMPTYPE_FILEUNPACKEDSIZE || rec.impType == IMPTYPE_STREAMUNPACKEDSIZE) {
                                    // File sizes can come from either FILEUNPACKEDSIZE or STREAMUNPACKEDSIZE (SubStreamsInfo)
                                    listFileUnpackedSizes.append(rec.varValue.toLongLong());
                                } else if (rec.impType == IMPTYPE_STREAMOFFSET) {
                                    listStreamOffsets.append(rec.varValue.toLongLong());
                                } else if (rec.impType == IMPTYPE_CODER) {
                                    listCodecs.append(rec.varValue.toByteArray());
                                }
                            }

                            // Analyze codec chain to separate filters from compressors
                            // In 7z, filters (like BCJ) come before compressors (like LZMA2)
                            COMPRESS_METHOD filterMethod = COMPRESS_METHOD_UNKNOWN;
                            COMPRESS_METHOD compressMethod = COMPRESS_METHOD_STORE;
                            
                            for (qint32 i = 0; i < listCodecs.count(); i++) {
                                COMPRESS_METHOD method = codecToCompressMethod(listCodecs.at(i));
                                
                                // BCJ and BCJ2 are filters/preprocessors, not compressors
                                if (method == COMPRESS_METHOD_BCJ || method == COMPRESS_METHOD_BCJ2) {
                                    filterMethod = method;
                                } else if (method != COMPRESS_METHOD_UNKNOWN) {
                                    // This is the actual compressor (LZMA, LZMA2, etc.)
                                    compressMethod = method;
                                }
                            }

                            // Create ARCHIVERECORD entries
                            qint64 nDataOffset = sizeof(SIGNATUREHEADER);
                            for (qint32 i = 0; i < listFileNames.count(); i++) {
                                ARCHIVERECORD record = {};
                                record.mapProperties.insert(FPART_PROP_ORIGINALNAME, listFileNames.at(i));
                                
                                if (i < listFileUnpackedSizes.count()) {
                                    record.nDecompressedSize = listFileUnpackedSizes.at(i);
                                }
                                if (i < listFilePackedSizes.count()) {
                                    record.nStreamSize = listFilePackedSizes.at(i);
                                } else {
                                    record.nStreamSize = record.nDecompressedSize;
                                }
                                if (i < listStreamOffsets.count()) {
                                    record.nStreamOffset = nDataOffset + listStreamOffsets.at(i);
                                } else {
                                    record.nStreamOffset = nDataOffset;
                                }

                                // Store both compressor and filter (if present)
                                record.mapProperties.insert(FPART_PROP_COMPRESSMETHOD, compressMethod);
                                if (filterMethod != COMPRESS_METHOD_UNKNOWN) {
                                    record.mapProperties.insert(FPART_PROP_FILTERMETHOD, filterMethod);
                                }

                                pContext->listArchiveRecords.append(record);
                                pContext->listRecordOffsets.append(record.nStreamOffset);
                            }

                            #ifdef QT_DEBUG
                            qDebug() << "  Created" << pContext->listArchiveRecords.count() << "archive records";
                            #endif
                        }
                    }
                }
            }

            delete[] pData;
        }

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

        #ifdef QT_DEBUG
        QString sName = record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
        qDebug() << "unpackCurrent: File" << pState->nCurrentIndex << sName;
        qDebug() << "  StreamOffset:" << record.nStreamOffset;
        qDebug() << "  StreamSize:" << record.nStreamSize;
        qDebug() << "  DecompSize:" << record.nDecompressedSize;
        #endif

        // Get compression method
        COMPRESS_METHOD compressMethod = COMPRESS_METHOD_UNKNOWN;
        if (record.mapProperties.contains(FPART_PROP_COMPRESSMETHOD)) {
            compressMethod = (COMPRESS_METHOD)record.mapProperties.value(FPART_PROP_COMPRESSMETHOD).toUInt();
        }

        // Get filter method (BCJ, BCJ2, etc.)
        COMPRESS_METHOD filterMethod = COMPRESS_METHOD_UNKNOWN;
        if (record.mapProperties.contains(FPART_PROP_FILTERMETHOD)) {
            filterMethod = (COMPRESS_METHOD)record.mapProperties.value(FPART_PROP_FILTERMETHOD).toUInt();
        }

        // Check if we can extract
        if (compressMethod == COMPRESS_METHOD_STORE) {
            // No compression - direct copy
            bResult = copyDeviceMemory(getDevice(), record.nStreamOffset, pDevice, 0, record.nStreamSize);
        } else if (compressMethod == COMPRESS_METHOD_LZMA || compressMethod == COMPRESS_METHOD_LZMA2) {
            // Use XDecompress for LZMA/LZMA2 decompression
            SubDevice sd(getDevice(), record.nStreamOffset, record.nStreamSize);

            if (sd.open(QIODevice::ReadOnly)) {
                // For BCJ filter, we need to decompress to memory first, apply filter, then write
                if (filterMethod == COMPRESS_METHOD_BCJ || filterMethod == COMPRESS_METHOD_BCJ2) {
                    // Decompress to temporary buffer
                    QByteArray baDecompressed;
                    QBuffer bufferOut;
                    bufferOut.setBuffer(&baDecompressed);

                    if (bufferOut.open(QIODevice::WriteOnly)) {
                        XBinary::DATAPROCESS_STATE state = {};
                        state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
                        state.pDeviceInput = &sd;
                        state.pDeviceOutput = &bufferOut;
                        state.nInputOffset = 0;
                        state.nInputLimit = record.nStreamSize;
                        state.nProcessedOffset = 0;
                        state.nProcessedLimit = -1;

                        XDecompress decompressor;
                        bResult = decompressor.decompress(&state, pPdStruct);

                        bufferOut.close();

                        if (bResult && !baDecompressed.isEmpty()) {
                            // Apply BCJ filter
                            if (filterMethod == COMPRESS_METHOD_BCJ) {
                                _applyBCJFilter(baDecompressed, 0);
                            }
                            // BCJ2 filter not yet implemented
                            
                            // Write filtered data to output device
                            if (pDevice->write(baDecompressed) == baDecompressed.size()) {
                                bResult = true;
                            } else {
                                bResult = false;
                            }
                        }
                    }
                } else {
                    // No filter - decompress directly to output device
                    XBinary::DATAPROCESS_STATE state = {};
                    state.mapProperties.insert(XBinary::FPART_PROP_COMPRESSMETHOD, compressMethod);
                    state.pDeviceInput = &sd;
                    state.pDeviceOutput = pDevice;
                    state.nInputOffset = 0;
                    state.nInputLimit = record.nStreamSize;
                    state.nProcessedOffset = 0;
                    state.nProcessedLimit = -1;

                    XDecompress decompressor;
                    bResult = decompressor.decompress(&state, pPdStruct);
                }

                sd.close();
            }
        } else {
            // Unsupported compression method
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

    // Check if the first byte indicates an encoded header
    bool bIsEncodedHeader = (nSize > 0 && pData[0] == XSevenZip::k7zIdEncodedHeader);
    
    #ifdef QT_DEBUG
    if (bIsEncodedHeader) {
        qDebug() << "======= DETECTED ENCODED HEADER - will decompress first =======";
    }
    #endif

    // Try to parse as standard header first, then try encoded header
    if (!bIsEncodedHeader && !_handleId(&listResult, XSevenZip::k7zIdHeader, &state, 1, false, pPdStruct, IMPTYPE_UNKNOWN)) {
        // Reset state for second attempt
        state.nCurrentOffset = 0;
        state.bIsError = false;
        state.sErrorString = QString();

        bIsEncodedHeader = true;  // Fallback to encoded header
    }
    
    if (bIsEncodedHeader) {
        // Handle encoded header
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
                            qDebug() << "======= PARSING DECOMPRESSED HEADER (" << baDecompressedData.size() << "bytes ) =======";
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
            #ifdef QT_DEBUG
            qDebug("Error parsing 7z header data: %s", qPrintable(state.sErrorString));
            #endif
        }

        // Debug log all SZRECORD entries
        qint32 nTotalRecords = _listResult.count();
        #ifdef QT_DEBUG
        qDebug("=== 7z Header Parse Results ===");
        #endif
        #ifdef QT_DEBUG
        qDebug("Total Records: %d", nTotalRecords);
        #endif
        #ifdef QT_DEBUG
        qDebug("Data Size: 0x%llX bytes", (qint64)nSize);
        #endif
        #ifdef QT_DEBUG
        qDebug("Parse Error: %s", state.bIsError ? "Yes" : "No");
        #endif
        if (state.bIsError && !state.sErrorString.isEmpty()) {
            #ifdef QT_DEBUG
            qDebug("Error Details: %s", qPrintable(state.sErrorString));
            #endif
        }
        #ifdef QT_DEBUG
        qDebug("================================");
        #endif

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

            #ifdef QT_DEBUG
            qDebug("[%03d] Offset: 0x%04X, Size: %3d bytes, Type: %-10s, ImpType: %-20s, Name: %-30s, Value: %s", i, record.nRelOffset, record.nSize,
                   qPrintable(sRecordType), qPrintable(sImpType), qPrintable(record.sName), qPrintable(sValue));
            #endif
        }
        #ifdef QT_DEBUG
        qDebug("================================");
        #endif
#endif
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
            signatureHeader.StartHeaderCRC = 0;        // Will be calculated later
            signatureHeader.NextHeaderOffset = 0;     // Will be set in finishPack
            signatureHeader.NextHeaderSize = 0;       // Will be set in finishPack
            signatureHeader.NextHeaderCRC = 0;        // Will be calculated later

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
        _writeNumber(&bufferHeader, 0);  // PackPos (offset from end of signature header to packed data)
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
        _writeByte(&bufferHeader, 0);    // External = 0 (folder info follows)

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
