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

XBinary::XCONVERT _TABLE_XSevenZip_STRUCTID[] = {{XSevenZip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                 {XSevenZip::STRUCTID_SIGNATUREHEADER, "SIGNATUREHEADER", QString("SIGNATUREHEADER")},
                                                 {XSevenZip::STRUCTID_HEADER, "HEADER", QObject::tr("Header")}};

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

QMap<quint64, QString> XSevenZip::getEIdEnumS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0, "End");
    mapResult.insert(1, "Header");
    mapResult.insert(2, "ArchiveProperties");
    mapResult.insert(3, "AdditionalStreamsInfo");
    mapResult.insert(4, "MainStreamsInfo");
    mapResult.insert(5, "FilesInfo");
    mapResult.insert(6, "PackInfo");
    mapResult.insert(7, "UnpackInfo");
    mapResult.insert(8, "SubStreamsInfo");
    mapResult.insert(9, "Size");
    mapResult.insert(10, "CRC");
    mapResult.insert(11, "Folder");
    mapResult.insert(12, "CodersUnpackSize");
    mapResult.insert(13, "NumUnpackStream");
    mapResult.insert(14, "EmptyStream");
    mapResult.insert(15, "EmptyFile");
    mapResult.insert(16, "Anti");
    mapResult.insert(17, "Name");
    mapResult.insert(18, "CTime");
    mapResult.insert(19, "ATime");
    mapResult.insert(20, "MTime");
    mapResult.insert(21, "WinAttrib");
    mapResult.insert(22, "Comment");
    mapResult.insert(23, "EncodedHeader");
    mapResult.insert(24, "StartPos");
    mapResult.insert(25, "Dummy");

    return mapResult;
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
    if (mapMode == MAPMODE_DATA) {
        return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_REGIONS) {
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
    QString sResult = tr("Unknown");

    switch (id) {
        case k7zIdEnd: sResult = QString("k7zIdEnd"); break;
        case k7zIdHeader: sResult = QString("k7zIdHeader"); break;
        case k7zIdArchiveProperties: sResult = QString("k7zIdArchiveProperties"); break;
        case k7zIdAdditionalStreamsInfo: sResult = QString("k7zIdAdditionalStreamsInfo"); break;
        case k7zIdMainStreamsInfo: sResult = QString("k7zIdMainStreamsInfo"); break;
        case k7zIdFilesInfo: sResult = QString("k7zIdFilesInfo"); break;
        case k7zIdPackInfo: sResult = QString("k7zIdPackInfo"); break;
        case k7zIdUnpackInfo: sResult = QString("k7zIdUnpackInfo"); break;
        case k7zIdSubStreamsInfo: sResult = QString("k7zIdSubStreamsInfo"); break;
        case k7zIdSize: sResult = QString("k7zIdSize"); break;
        case k7zIdCRC: sResult = QString("k7zIdCRC"); break;
        case k7zIdFolder: sResult = QString("k7zIdFolder"); break;
        case k7zIdCodersUnpackSize: sResult = QString("k7zIdCodersUnpackSize"); break;
        case k7zIdNumUnpackStream: sResult = QString("k7zIdNumUnpackStream"); break;
        case k7zIdEmptyStream: sResult = QString("k7zIdEmptyStream"); break;
        case k7zIdEmptyFile: sResult = QString("k7zIdEmptyFile"); break;
        case k7zIdAnti: sResult = QString("k7zIdAnti"); break;
        case k7zIdName: sResult = QString("k7zIdName"); break;
        case k7zIdCTime: sResult = QString("k7zIdCTime"); break;
        case k7zIdATime: sResult = QString("k7zIdATime"); break;
        case k7zIdMTime: sResult = QString("k7zIdMTime"); break;
        case k7zIdWinAttrib: sResult = QString("k7zIdWinAttrib"); break;
        case k7zIdComment: sResult = QString("k7zIdComment"); break;
        case k7zIdEncodedHeader: sResult = QString("k7zIdEncodedHeader"); break;
        case k7zIdStartPos: sResult = QString("k7zIdStartPos"); break;
        case k7zIdDummy: sResult = QString("k7zIdDummy"); break;
    }

    return sResult;
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
                            DATA_HEADER hexStart = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("StartHeader (hex)"), dataHeader.dsID,
                                                                  XBinary::STRUCTID_HEX, startHeaderHexOff, startHeaderHexSize);
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

                QList<XSevenZip::SZRECORD> listRecords = _handleData(nStartOffset, dataHeadersOptions.nSize, pPdStruct);

                qint32 nNumberOfRecords = listRecords.count();

                for (qint32 i = 0; i < nNumberOfRecords; i++) {
                    XSevenZip::SZRECORD szRecord = listRecords.at(i);

                    DATA_RECORD dataRecord = {};
                    dataRecord.nRelOffset = szRecord.nRelOffset;
                    dataRecord.nSize = szRecord.nSize;
                    dataRecord.sName = szRecord.sName;

                    dataRecord.nFlags = DRF_UNKNOWN;  // TODO
                    dataRecord.endian = dataHeadersOptions.pMemoryMap->endian;

                    if (szRecord.srType == SRTYPE_ID) {
                        DATAVALUESET dataValueSet;
                        dataValueSet.mapValues = getEIdEnumS();
                        dataValueSet.vlType = VL_TYPE_LIST;
                        dataValueSet.nMask = 0xFFFFFFFFFFFFFFFF;
                        dataRecord.listDataValueSets.append(dataValueSet);
                        dataRecord.valType = VT_PACKEDNUMBER;  // TODO
                    } else if (szRecord.srType == SRTYPE_NUMBER) {
                        dataRecord.valType = VT_PACKEDNUMBER;  // TODO
                    } else if (szRecord.srType == SRTYPE_BYTE) {
                        dataRecord.valType = VT_UINT8;
                    } else if (szRecord.srType == SRTYPE_UINT32) {
                        dataRecord.valType = VT_UINT32;
                    } else if (szRecord.srType == SRTYPE_ARRAY) {
                        dataRecord.valType = VT_BYTE_ARRAY;
                    }

                    dataHeader.listRecords.append(dataRecord);
                }

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren && (dataHeadersOptions.nSize > 0)) {
                    // Also add hex view for this parsed header block
                    DATA_HEADER hexHdr = _dataHeaderHex(dataHeadersOptions, QString("%1").arg("Header (hex)"), dataHeader.dsID, XBinary::STRUCTID_HEX,
                                                       nStartOffset, dataHeadersOptions.nSize);
                    listResult.append(hexHdr);
                }
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XSevenZip::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    QList<FPART> listResult;

    const qint64 fileSize = getSize();
    if (fileSize < (qint64)sizeof(SIGNATUREHEADER)) return listResult;

    SIGNATUREHEADER sh = _read_SIGNATUREHEADER(0);
    const qint64 base = sizeof(SIGNATUREHEADER);
    const qint64 nextHeaderOffset = base + (qint64)sh.NextHeaderOffset;
    const qint64 nextHeaderSize = (qint64)sh.NextHeaderSize;

    if (nFileParts & FILEPART_HEADER) {
        // Signature header
        FPART hdr = {};
        hdr.filePart = FILEPART_HEADER;
        hdr.nFileOffset = 0;
        hdr.nFileSize = qMin<qint64>((qint64)sizeof(SIGNATUREHEADER), fileSize);
        hdr.nVirtualAddress = -1;
        hdr.sName = tr("Header");
        listResult.append(hdr);

        // Next header block
        if (nextHeaderSize > 0 && nextHeaderOffset >= 0 && (nextHeaderOffset + nextHeaderSize) <= fileSize) {
            FPART nh = {};
            nh.filePart = FILEPART_HEADER;
            nh.nFileOffset = nextHeaderOffset;
            nh.nFileSize = nextHeaderSize;
            nh.nVirtualAddress = -1;
            nh.sName = tr("Header");
            listResult.append(nh);
        }
    }

    if (nFileParts & FILEPART_DATA) {
        // Packed streams between signature header and next header
        qint64 dataOff = base;
        qint64 dataSize = 0;
        if (nextHeaderOffset > base) {
            dataSize = nextHeaderOffset - base;
        } else {
            // If NextHeaderOffset is zero or invalid, consider everything after header as data
            dataSize = qMax<qint64>(0, fileSize - base);
        }
        if (dataSize > 0) {
            FPART data = {};
            data.filePart = FILEPART_DATA;
            data.nFileOffset = dataOff;
            data.nFileSize = qMin<qint64>(dataSize, fileSize - dataOff);
            data.nVirtualAddress = -1;
            data.sName = tr("Data");
            listResult.append(data);
        }
    }

    if (nFileParts & FILEPART_REGION) {
        // Regions view: treat signature header and next header as regions as well
        // Signature header region
        FPART shreg = {};
        shreg.filePart = FILEPART_REGION;
        shreg.nFileOffset = 0;
        shreg.nFileSize = qMin<qint64>((qint64)sizeof(SIGNATUREHEADER), fileSize);
        shreg.nVirtualAddress = -1;
        shreg.sName = QString("%1").arg("SIGNATUREHEADER");
        listResult.append(shreg);

        // Data area region (if present)
        qint64 dataStart = base;
        qint64 dataEnd = (nextHeaderSize > 0 && nextHeaderOffset > base) ? nextHeaderOffset : fileSize;
        if (dataEnd > dataStart) {
            FPART dreg = {};
            dreg.filePart = FILEPART_REGION;
            dreg.nFileOffset = dataStart;
            dreg.nFileSize = dataEnd - dataStart;
            dreg.nVirtualAddress = -1;
            dreg.sName = QString("%1").arg("PACKED_STREAMS");
            listResult.append(dreg);
        }

        // Next header region
        if (nextHeaderSize > 0 && (nextHeaderOffset + nextHeaderSize) <= fileSize) {
            FPART nhreg = {};
            nhreg.filePart = FILEPART_REGION;
            nhreg.nFileOffset = nextHeaderOffset;
            nhreg.nFileSize = nextHeaderSize;
            nhreg.nVirtualAddress = -1;
            nhreg.sName = QString("%1").arg("NEXT_HEADER");
            listResult.append(nhreg);
        }
    }

    if (nFileParts & FILEPART_OVERLAY) {
        qint64 maxCovered = 0;
        for (const auto &p : listResult) {
            if (p.filePart != FILEPART_OVERLAY) {
                maxCovered = qMax(maxCovered, p.nFileOffset + qMax<qint64>(0, p.nFileSize));
            }
        }
        if (maxCovered < fileSize) {
            FPART ov = {};
            ov.filePart = FILEPART_OVERLAY;
            ov.nFileOffset = maxCovered;
            ov.nFileSize = fileSize - maxCovered;
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

bool XSevenZip::_handleId(QList<SZRECORD> *pListRecords, EIdEnum id, SZSTATE *pState, qint32 nCount, bool bCheck, PDSTRUCT *pPdStruct)
{
    if (isPdStructStopped(pPdStruct)) {
        return false;
    }

    if (pState->nCurrentOffset >= pState->nSize) {
        return false;
    }

    if (pState->bIsError) {
        return false;
    }

    bool bResult = false;

    XBinary::PACKED_UINT puTag = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);

    bool bProcess = false;

    if (puTag.bIsValid) {
        if (puTag.nValue == id) {
            bProcess = true;
        }
    }

    if (bProcess) {
        {
            SZRECORD record = {};
            record.nRelOffset = pState->nCurrentOffset;
            record.nSize = puTag.nByteSize;
            record.varValue = puTag.nValue;
            record.srType = SRTYPE_ID;
            record.sName = "k7zId";
            pListRecords->append(record);
        }
        pState->nCurrentOffset += puTag.nByteSize;

        if (puTag.nValue == XSevenZip::k7zIdHeader) {
            _handleId(pListRecords, XSevenZip::k7zIdMainStreamsInfo, pState, 1, true, pPdStruct);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdFilesInfo, pState, 1, true, pPdStruct);
        } else if (puTag.nValue == XSevenZip::k7zIdMainStreamsInfo) {
            _handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct);
            _handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, true, pPdStruct);
            _handleId(pListRecords, XSevenZip::k7zIdSubStreamsInfo, pState, 1, false, pPdStruct);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct);
        } else if (puTag.nValue == XSevenZip::k7zIdPackInfo) {
            _handleNumber(pListRecords, pState, pPdStruct, "PackPosition");
            quint64 nNumberOfPackStreams = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfPackStreams");
            _handleId(pListRecords, XSevenZip::k7zIdSize, pState, nNumberOfPackStreams, false, pPdStruct);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct);
        } else if (puTag.nValue == XSevenZip::k7zIdUnpackInfo) {
            _handleId(pListRecords, XSevenZip::k7zIdFolder, pState, 1, true, pPdStruct);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct);
        } else if (puTag.nValue == XSevenZip::k7zIdFolder) {
            quint64 nNumberOfFolders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFolders");
            quint8 nExt = _handleByte(pListRecords, pState, pPdStruct, "ExternalByte");

            if (nExt == 0) {
                quint64 nNumberOfCoders = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCoders");
                quint8 nFlag = _handleByte(pListRecords, pState, pPdStruct, "Flag");

                qint32 nCodecSize = nFlag & 0x0F;
                bool bIsComplex = (nFlag & 0x10) != 0;
                bool bHasAttr = (nFlag & 0x20) != 0;

                _handleArray(pListRecords, pState, nCodecSize, pPdStruct, "Coder");

                if (bIsComplex) {
                    pState->bIsError = true;
                    pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
                }

                if (bHasAttr) {
                    quint64 nPropertySize = _handleNumber(pListRecords, pState, pPdStruct, "PropertiesSize");  // PropertiesSize
                    _handleArray(pListRecords, pState, nPropertySize, pPdStruct, "Property");
                }
            } else if (nExt == 1) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("Data Stream Index"));
            } else {
                pState->bIsError = true;
                pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
            }

            _handleId(pListRecords, XSevenZip::k7zIdCodersUnpackSize, pState, nNumberOfFolders, false, pPdStruct);
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct);
            bResult = true;
        } else if (puTag.nValue == XSevenZip::k7zIdSubStreamsInfo) {
            _handleId(pListRecords, XSevenZip::k7zIdCRC, pState, 1, false, pPdStruct);
            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct);
        } else if (puTag.nValue == XSevenZip::k7zIdEncodedHeader) {
            _handleId(pListRecords, XSevenZip::k7zIdPackInfo, pState, 1, true, pPdStruct);
            _handleId(pListRecords, XSevenZip::k7zIdUnpackInfo, pState, 1, true, pPdStruct);
            _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, false, pPdStruct);
            bResult = true;
        } else if (puTag.nValue == XSevenZip::k7zIdSize) {
            for (quint64 i = 0; (i < nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("Size%1").arg(i));
            }
            bResult = true;
        } else if (puTag.nValue == XSevenZip::k7zIdCodersUnpackSize) {
            for (quint64 i = 0; (i < nCount) && isPdStructNotCanceled(pPdStruct); i++) {
                _handleNumber(pListRecords, pState, pPdStruct, QString("CodersUnpackSize%1").arg(i));
            }
            bResult = true;
        } else if (puTag.nValue == XSevenZip::k7zIdCRC) {
            quint64 nNumberOfCRCs = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfCRCs");

            for (quint64 i = 0; (i < nNumberOfCRCs) && isPdStructNotCanceled(pPdStruct); i++) {
                _handleUINT32(pListRecords, pState, pPdStruct, QString("CRC%1").arg(i));
            }
            bResult = true;
        } else if (puTag.nValue == XSevenZip::k7zIdFilesInfo) {
            quint64 nNumberOfFiles = _handleNumber(pListRecords, pState, pPdStruct, "NumberOfFiles");

            _handleId(pListRecords, XSevenZip::k7zIdDummy, pState, 1, false, pPdStruct);
            _handleId(pListRecords, XSevenZip::k7zIdName, pState, 1, true, pPdStruct);

            bResult = _handleId(pListRecords, XSevenZip::k7zIdEnd, pState, 1, true, pPdStruct);
        } else if (puTag.nValue == XSevenZip::k7zIdDummy) {
            quint32 nSize = _handleNumber(pListRecords, pState, pPdStruct, QString("DummySize"));
            _handleArray(pListRecords, pState, nSize, pPdStruct, QString("DummyArray"));
            bResult = true;
        } else if (puTag.nValue == XSevenZip::k7zIdName) {
            _handleNumber(pListRecords, pState, pPdStruct, QString("NameSize"));
        } else if (puTag.nValue == XSevenZip::k7zIdEnd) {
            bResult = true;
        }
    } else if (bCheck) {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
#ifdef QT_DEBUG
        qDebug("Invalid value: %X", puTag.nValue);
#endif
        bResult = false;
    }

    return bResult;
}

quint64 XSevenZip::_handleNumber(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption)
{
    if (isPdStructStopped(pPdStruct)) {
        return 0;
    }

    if (pState->nCurrentOffset >= pState->nSize) {
        return 0;
    }

    if (pState->bIsError) {
        return 0;
    }

    quint64 nResult = 0;

    XBinary::PACKED_UINT puNumber = XBinary::_read_packedNumber(pState->pData + pState->nCurrentOffset, pState->nSize - pState->nCurrentOffset);

    if (puNumber.bIsValid) {
        nResult = puNumber.nValue;

        {
            SZRECORD record = {};
            record.nRelOffset = pState->nCurrentOffset;
            record.nSize = puNumber.nByteSize;
            record.varValue = puNumber.nValue;
            record.srType = SRTYPE_NUMBER;
            record.sName = sCaption;
            pListRecords->append(record);
        }
        pState->nCurrentOffset += puNumber.nByteSize;
    } else {
        pState->bIsError = true;
        pState->sErrorString = QString("%1: %2").arg(XBinary::valueToHexEx(pState->nCurrentOffset), tr("Invalid data"));
    }

    return nResult;
}

quint8 XSevenZip::_handleByte(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption)
{
    if (isPdStructStopped(pPdStruct)) {
        return 0;
    }

    if (pState->nCurrentOffset >= pState->nSize) {
        return 0;
    }

    if (pState->bIsError) {
        return 0;
    }

    quint8 nResult = _read_uint8(pState->pData + pState->nCurrentOffset);

    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = 1;
    record.varValue = nResult;
    record.srType = SRTYPE_BYTE;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset++;

    return nResult;
}

quint32 XSevenZip::_handleUINT32(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption)
{
    if (isPdStructStopped(pPdStruct)) {
        return 0;
    }

    if (pState->nCurrentOffset >= (pState->nSize - 3)) {
        return 9;
    }

    if (pState->bIsError) {
        return 0;
    }

    quint32 nResult = _read_uint32(pState->pData + pState->nCurrentOffset);

    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = 4;
    record.varValue = nResult;
    record.srType = SRTYPE_UINT32;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset += 4;

    return nResult;
}

void XSevenZip::_handleArray(QList<SZRECORD> *pListRecords, SZSTATE *pState, qint64 nSize, PDSTRUCT *pPdStruct, const QString &sCaption)
{
    if (isPdStructStopped(pPdStruct)) {
        return;
    }

    if (pState->nCurrentOffset >= pState->nSize) {
        return;
    }

    if (pState->bIsError) {
        return;
    }

    SZRECORD record = {};
    record.nRelOffset = pState->nCurrentOffset;
    record.nSize = nSize;
    // record.varValue = nResult;
    record.srType = SRTYPE_ARRAY;
    record.sName = sCaption;
    pListRecords->append(record);

    pState->nCurrentOffset += nSize;
}

QList<XSevenZip::SZRECORD> XSevenZip::_handleData(qint64 nOffset, qint64 nSize, PDSTRUCT *pPdStruct)
{
    QList<XSevenZip::SZRECORD> listResult;

    SZSTATE state = {};
    state.pData = new char[nSize];
    state.nSize = nSize;
    state.nCurrentOffset = 0;
    state.bIsError = false;

    read_array(nOffset, state.pData, state.nSize, pPdStruct);

    if (!_handleId(&listResult, XSevenZip::k7zIdHeader, &state, 1, false, pPdStruct)) {
        _handleId(&listResult, XSevenZip::k7zIdEncodedHeader, &state, 1, true, pPdStruct);
    }

    delete[] state.pData;

    return listResult;
}
