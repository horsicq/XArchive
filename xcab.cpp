/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "xcab.h"
#include "Algos/xdeflatedecoder.h"
#include "Algos/xlzhdecoder.h"

#include <limits>

static XBinary::XCONVERT _TABLE_XCAB_STRUCTID[] = {{XCab::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                   {XCab::STRUCTID_CFHEADER, "CFHEADER", QString("CFHEADER")},
                                                   {XCab::STRUCTID_CFFOLDER, "CFFOLDER", QString("CFFOLDER")},
                                                   {XCab::STRUCTID_CFFILE, "CFFILE", QString("CFFILE")},
                                                   {XCab::STRUCTID_CFDATA, "CFDATA", QString("CFDATA")}};
static const qint64 CAB_MAX_FOLDER_SIZE = 512LL * 1024 * 1024;

static quint32 cabDataChecksum(const char *pData, qint32 nSize, quint32 nSeed = 0)
{
    quint32 nResult = nSeed;

    while (nSize >= 4) {
        nResult ^= (quint32)(quint8)pData[0] | ((quint32)(quint8)pData[1] << 8) | ((quint32)(quint8)pData[2] << 16) |
                   ((quint32)(quint8)pData[3] << 24);
        pData += 4;
        nSize -= 4;
    }

    // CAB's partial DWORD convention places the first remaining byte in the
    // most-significant occupied position (unlike the full little-endian words).
    quint32 nTail = 0;
    if (nSize == 3) {
        nTail |= (quint32)(quint8)*pData++ << 16;
    }
    if (nSize >= 2) {
        nTail |= (quint32)(quint8)*pData++ << 8;
    }
    if (nSize >= 1) {
        nTail |= (quint32)(quint8)*pData;
    }

    return nResult ^ nTail;
}

XCab::XCab(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XCab::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(CFHEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "'MSCF'00000000........00000000........00000000", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XCab::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCab xcab(pDevice);

    return xcab.isValid();
}

QString XCab::getVersion()
{
    return QString("%1.%2").arg(read_uint8(25)).arg(read_uint8(24), 2, 10, QChar('0'));
}

XCab::CFFILE XCab::readCFFILE(qint64 nOffset)
{
    CFFILE result = {};

    result.cbFile = read_uint32(nOffset + offsetof(CFFILE, cbFile));
    result.uoffFolderStart = read_uint32(nOffset + offsetof(CFFILE, uoffFolderStart));
    result.iFolder = read_uint16(nOffset + offsetof(CFFILE, iFolder));
    result.date = read_uint16(nOffset + offsetof(CFFILE, date));
    result.time = read_uint16(nOffset + offsetof(CFFILE, time));
    result.attribs = read_uint16(nOffset + offsetof(CFFILE, attribs));

    return result;
}

XCab::CFHEADER XCab::readCFHeader(qint64 nOffset)
{
    CFHEADER result = {};

    result.signature[0] = read_uint8(nOffset + 0);
    result.signature[1] = read_uint8(nOffset + 1);
    result.signature[2] = read_uint8(nOffset + 2);
    result.signature[3] = read_uint8(nOffset + 3);
    result.reserved1 = read_uint32(nOffset + offsetof(CFHEADER, reserved1));
    result.cbCabinet = read_uint32(nOffset + offsetof(CFHEADER, cbCabinet));
    result.reserved2 = read_uint32(nOffset + offsetof(CFHEADER, reserved2));
    result.coffFiles = read_uint32(nOffset + offsetof(CFHEADER, coffFiles));
    result.reserved3 = read_uint32(nOffset + offsetof(CFHEADER, reserved3));
    result.versionMinor = read_uint8(nOffset + offsetof(CFHEADER, versionMinor));
    result.versionMajor = read_uint8(nOffset + offsetof(CFHEADER, versionMajor));
    result.cFolders = read_uint16(nOffset + offsetof(CFHEADER, cFolders));
    result.cFiles = read_uint16(nOffset + offsetof(CFHEADER, cFiles));
    result.flags = read_uint16(nOffset + offsetof(CFHEADER, flags));
    result.setID = read_uint16(nOffset + offsetof(CFHEADER, setID));
    result.iCabinet = read_uint16(nOffset + offsetof(CFHEADER, iCabinet));

    // if (result.flags & 0x0004)  // TODO const
    // {
    //     result.cbCFHeader = read_uint16(offsetof(CFHEADER, cbCFHeader));
    //     result.cbCFFolder = read_uint8(offsetof(CFHEADER, cbCFFolder));
    //     result.cbCFData = read_uint8(offsetof(CFHEADER, cbCFData));
    // }

    return result;
}

XCab::CFFOLDER XCab::readCFFolder(qint64 nOffset)
{
    CFFOLDER result = {};

    result.coffCabStart = read_uint32(nOffset + offsetof(CFFOLDER, coffCabStart));
    result.cCFData = read_uint16(nOffset + offsetof(CFFOLDER, cCFData));
    result.typeCompress = read_uint16(nOffset + offsetof(CFFOLDER, typeCompress));

    return result;
}

XCab::CFDATA XCab::readCFData(qint64 nOffset)
{
    CFDATA result = {};

    result.csum = read_uint32(nOffset + offsetof(CFDATA, csum));
    result.cbData = read_uint16(nOffset + offsetof(CFDATA, cbData));
    result.cbUncomp = read_uint16(nOffset + offsetof(CFDATA, cbUncomp));

    return result;
}

qint64 XCab::_getStreamSize(qint64 nOffset, qint32 nCount, qint32 nReservedSize, qint64 nCabinetSize, qint64 *pUncompressedSize)
{
    if ((nOffset < 0) || (nCount < 0) || (nReservedSize < 0) || (nCabinetSize < 0) || (nOffset > nCabinetSize)) {
        return -1;
    }

    qint64 nCurrentOffset = nOffset;
    qint64 nUncompressedSize = 0;

    for (qint32 i = 0; i < nCount; i++) {
        if ((qint64)sizeof(CFDATA) + nReservedSize > nCabinetSize - nCurrentOffset) {
            return -1;
        }

        CFDATA cfData = readCFData(nCurrentOffset);
        qint64 nBlockSize = (qint64)sizeof(CFDATA) + nReservedSize + (qint64)cfData.cbData;

        if ((cfData.cbData == 0) || (nBlockSize > nCabinetSize - nCurrentOffset) ||
            ((qint64)cfData.cbUncomp > (std::numeric_limits<qint64>::max)() - nUncompressedSize)) {
            return -1;
        }

        nUncompressedSize += cfData.cbUncomp;
        nCurrentOffset += nBlockSize;
    }

    if (pUncompressedSize) {
        *pUncompressedSize = nUncompressedSize;
    }

    return nCurrentOffset - nOffset;
}

XBinary::FT XCab::getFileType()
{
    return FT_CAB;
}

QString XCab::getFileFormatExt()
{
    return "cab";
}

QString XCab::getFileFormatExtsString()
{
    return "CAB (*.cab)";
}

qint64 XCab::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    qint64 nResult = 0;

    nResult = readCFHeader(0).cbCabinet;  // TODO check mb _getRawSize !!!
    nResult = qMin(getSize(), nResult);

    return nResult;
}

QString XCab::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XCab::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XCab::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XCab::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_CFHEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_CFHEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFHEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(CFHEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFHEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFHEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);

        if (xfStruct.bIsParent) {
            CFHEADER cfHeader = readCFHeader(0);

            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.sParent = xfHeader.sTag;

            _xfStruct.nStructID = STRUCTID_CFFOLDER;
            _xfStruct.xLoc = offsetToLoc(sizeof(CFHEADER));
            _xfStruct.nCount = cfHeader.cFolders;
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));

            _xfStruct.nStructID = STRUCTID_CFFILE;
            _xfStruct.xLoc = offsetToLoc(cfHeader.coffFiles);
            _xfStruct.nCount = cfHeader.cFiles;
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));
        }
    } else if (nStructID == STRUCTID_CFFOLDER) {
        qint64 nOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);
        qint32 nCount = xfStruct.nCount;
        qint64 nFileSize = getSize();

        if (nOffset == -1) {
            nOffset = sizeof(CFHEADER);
        }
        if (nCount == 0) {
            nCount = readCFHeader(0).cFolders;
        }

        if (nCount > 0) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFFOLDER);
            xfHeader.xLoc = offsetToLoc(nOffset);
            xfHeader.xfType = XFTYPE_TABLE;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFFOLDER, xfHeader.xLoc);

            qint64 nCurrentOffset = nOffset;
            for (qint32 i = 0; i < nCount; i++) {
                if ((nCurrentOffset + (qint64)sizeof(CFFOLDER)) > nFileSize) {
                    break;
                }
                xfHeader.listRowLocations.append(nCurrentOffset);
                nCurrentOffset += sizeof(CFFOLDER);
            }

            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFFOLDER), xfHeader.sParentTag);
            listResult.append(xfHeader);

            if (xfStruct.bIsParent) {
                XFHEADER xfHeaderData = {};
                xfHeaderData.sParentTag = xfHeader.sTag;
                xfHeaderData.fileType = xfStruct.fileType;
                xfHeaderData.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFDATA);
                xfHeaderData.xfType = XFTYPE_TABLE;

                nCurrentOffset = nOffset;
                for (qint32 i = 0; i < nCount; i++) {
                    if ((nCurrentOffset + (qint64)sizeof(CFFOLDER)) > nFileSize) {
                        break;
                    }
                    CFFOLDER cfFolder = readCFFolder(nCurrentOffset);
                    qint64 nDataOffset = cfFolder.coffCabStart;
                    for (quint32 j = 0; j < cfFolder.cCFData; j++) {
                        if ((nDataOffset + (qint64)sizeof(CFDATA)) > nFileSize) {
                            break;
                        }
                        xfHeaderData.listRowLocations.append(nDataOffset);
                        CFDATA cfData = readCFData(nDataOffset);
                        nDataOffset += (qint64)sizeof(CFDATA) + cfData.cbData;
                    }
                    nCurrentOffset += sizeof(CFFOLDER);
                }

                if (!xfHeaderData.listRowLocations.isEmpty()) {
                    xfHeaderData.xLoc = offsetToLoc(xfHeaderData.listRowLocations.first());
                    xfHeaderData.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFDATA, xfHeaderData.xLoc);
                    xfHeaderData.sTag = xfHeaderToTag(xfHeaderData, structIDToString(STRUCTID_CFDATA), xfHeaderData.sParentTag);
                    listResult.append(xfHeaderData);
                }
            }
        }
    } else if (nStructID == STRUCTID_CFFILE) {
        qint64 nOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);
        qint32 nCount = xfStruct.nCount;
        qint64 nFileSize = getSize();

        CFHEADER cfHeader = readCFHeader(0);

        if (nOffset == -1) {
            nOffset = cfHeader.coffFiles;
        }
        if (nCount == 0) {
            nCount = cfHeader.cFiles;
        }

        if ((nOffset > 0) && (nCount > 0)) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFFILE);
            xfHeader.xLoc = offsetToLoc(nOffset);
            xfHeader.xfType = XFTYPE_TABLE;

            qint64 nCurrentOffset = nOffset;
            for (qint32 i = 0; i < nCount; i++) {
                if ((nCurrentOffset + (qint64)sizeof(CFFILE)) > nFileSize) {
                    break;
                }
                xfHeader.listRowLocations.append(nCurrentOffset);
                QString sFileName = read_ansiString(nCurrentOffset + sizeof(CFFILE));
                nCurrentOffset += (qint64)sizeof(CFFILE) + sFileName.size() + 1;
            }

            if (!xfHeader.listRowLocations.isEmpty()) {
                xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFFILE, offsetToLoc(xfHeader.listRowLocations.first()));
                xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFFILE), xfHeader.sParentTag);
                listResult.append(xfHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XCab::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_CFHEADER) {
        listResult.append({"signature", (qint32)offsetof(CFHEADER, signature), 4, XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
        listResult.append({"reserved1", (qint32)offsetof(CFHEADER, reserved1), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"cbCabinet", (qint32)offsetof(CFHEADER, cbCabinet), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"reserved2", (qint32)offsetof(CFHEADER, reserved2), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"coffFiles", (qint32)offsetof(CFHEADER, coffFiles), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append({"reserved3", (qint32)offsetof(CFHEADER, reserved3), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"versionMinor", (qint32)offsetof(CFHEADER, versionMinor), 1, XFRECORD_FLAG_VERSION_MINOR, VT_UINT8});
        listResult.append({"versionMajor", (qint32)offsetof(CFHEADER, versionMajor), 1, XFRECORD_FLAG_VERSION_MAJOR, VT_UINT8});
        listResult.append({"cFolders", (qint32)offsetof(CFHEADER, cFolders), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"cFiles", (qint32)offsetof(CFHEADER, cFiles), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"flags", (qint32)offsetof(CFHEADER, flags), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"setID", (qint32)offsetof(CFHEADER, setID), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"iCabinet", (qint32)offsetof(CFHEADER, iCabinet), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
    } else if (nStructID == STRUCTID_CFFOLDER) {
        listResult.append({"coffCabStart", (qint32)offsetof(CFFOLDER, coffCabStart), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append({"cCFData", (qint32)offsetof(CFFOLDER, cCFData), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"typeCompress", (qint32)offsetof(CFFOLDER, typeCompress), 2, XFRECORD_FLAG_NONE, VT_UINT16});
    } else if (nStructID == STRUCTID_CFFILE) {
        listResult.append({"cbFile", (qint32)offsetof(CFFILE, cbFile), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"uoffFolderStart", (qint32)offsetof(CFFILE, uoffFolderStart), 4, XFRECORD_FLAG_RELATIVE_OFFSET, VT_UINT32});
        listResult.append({"iFolder", (qint32)offsetof(CFFILE, iFolder), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"date", (qint32)offsetof(CFFILE, date), 2, XFRECORD_FLAG_DOSDATE, VT_UINT16});
        listResult.append({"time", (qint32)offsetof(CFFILE, time), 2, XFRECORD_FLAG_DOSTIME, VT_UINT16});
        listResult.append({"attribs", (qint32)offsetof(CFFILE, attribs), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        // Variable-length fields
        QString sFileName = read_ansiString(xLoc.nLocation + sizeof(CFFILE));
        listResult.append({"szName", (qint32)sizeof(CFFILE), (qint32)(sFileName.size() + 1), XFRECORD_FLAG_NONE, VT_CHAR_ARRAY});
    } else if (nStructID == STRUCTID_CFDATA) {
        listResult.append({"csum", (qint32)offsetof(CFDATA, csum), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"cbData", (qint32)offsetof(CFDATA, cbData), 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        listResult.append({"cbUncomp", (qint32)offsetof(CFDATA, cbUncomp), 2, XFRECORD_FLAG_SIZE, VT_UINT16});
    }

    return listResult;
}

// qint32 XCab::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<QVariant> *pListValues, void *pUserData,
//                           PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(nRow)
//     Q_UNUSED(locType)
//     Q_UNUSED(nLocation)
//     Q_UNUSED(dataRecordsOptions)
//     Q_UNUSED(pListValues)
//     Q_UNUSED(pUserData)
//     Q_UNUSED(pPdStruct)
//     // Not implemented for CAB
//     return 0;
// }

QString XCab::getMIMEString()
{
    return "application/vnd.ms-cab-compressed";
}

QList<XBinary::FPART> XCab::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    qint64 nFileSize = getSize();
    qint64 nFileFormatSize = getFileFormatSize(pPdStruct);

    CFHEADER cfHeader = readCFHeader(0);

    if (nFileParts & FILEPART_HEADER) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = qMin<qint64>(sizeof(CFHEADER), nFileSize);
        record.nVirtualAddress = -1;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_DATA) {
        XBinary::FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nFileFormatSize;
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    qint64 nCurrentOffset = sizeof(CFHEADER);

    if ((nFileParts & FILEPART_HEADER) || (nFileParts & FILEPART_STREAM)) {
        // Regions: enumerate folders, files, and data blocks
        // 1) CFFOLDER area and per-folder entries (best-effort)
        if (cfHeader.cFolders) {
            for (quint32 i = 0; i < cfHeader.cFolders; ++i) {
                if ((nCurrentOffset + (qint64)sizeof(CFFOLDER)) > nFileSize) break;

                if (nFileParts & FILEPART_HEADER) {
                    FPART rec = {};
                    rec.filePart = FILEPART_HEADER;
                    rec.nFileOffset = nCurrentOffset;
                    rec.nFileSize = sizeof(CFFOLDER);
                    rec.nVirtualAddress = -1;
                    rec.sName = QString("CFFOLDER(%1)").arg(i);
                    listResult.append(rec);
                }

                if (nFileParts & FILEPART_STREAM) {
                    CFFOLDER cfFolder = readCFFolder(nCurrentOffset);

                    FPART rec = {};
                    rec.filePart = FILEPART_STREAM;
                    rec.nFileOffset = cfFolder.coffCabStart;
                    rec.nFileSize = _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData, 0, nFileFormatSize);
                    rec.nVirtualAddress = -1;
                    rec.sName = tr("Stream") + QString(" (%1)").arg(i);

                    if (cfFolder.typeCompress == 0x0000) {
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE_CAB);
                    } else if (cfFolder.typeCompress == 0x0001) {
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MSZIP_CAB);
                    } else if (cfFolder.typeCompress == 0x0003) {
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZX_CAB);
                    } else {
                        rec.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
                    }

                    listResult.append(rec);
                }

                nCurrentOffset += sizeof(CFFOLDER);
            }
            // // Heuristic: assume folders area ends at coffFiles; derive start by subtracting cFolders*sizeof(CFFOLDER)
            // qint64 foldersEnd = qMin<qint64>(cfHeader.coffFiles, nFileSize);
            // qint64 foldersStart = qMax<qint64>(0, foldersEnd - (qint64)cfHeader.cFolders * (qint64)sizeof(CFFOLDER));

            // // Whole folders area
            // if ((foldersStart < foldersEnd) && (foldersEnd <= nFileSize)) {
            //     FPART area = {};
            //     area.filePart = FILEPART_REGION;
            //     area.nFileOffset = foldersStart;
            //     area.nFileSize = foldersEnd - foldersStart;
            //     area.nVirtualAddress = -1;
            //     area.sName = tr("CFFOLDER area");
            //     listResult.append(area);

            //     // Individual folder records (best-effort sequential)

            //     // Use folder records to enumerate CFDATA blocks
            //     for (quint32 i = 0; i < cfHeader.cFolders; ++i) {
            //         qint64 recOff = foldersStart + (qint64)i * (qint64)sizeof(CFFOLDER);
            //         if ((recOff + (qint64)sizeof(CFFOLDER)) > nFileSize) break;
            //         CFFOLDER fol = readCFFolder(recOff);

            //         qint64 dataOff = fol.coffCabStart;
            //         for (quint32 j = 0; j < fol.cCFData; ++j) {
            //             if ((dataOff + (qint64)sizeof(CFDATA)) > nFileSize) break;
            //             // CFDATA header entry
            //             FPART drec = {};
            //             drec.filePart = FILEPART_REGION;
            //             drec.nFileOffset = dataOff;
            //             drec.nFileSize = sizeof(CFDATA);
            //             drec.nVirtualAddress = -1;
            //             drec.sName = QString("%1(%2,%3)").arg("CFDATA").arg(i + 1).arg(j + 1);
            //             listResult.append(drec);

            //             // Advance to next block: header + compressed bytes
            //             CFDATA hdr = readCFData(dataOff);
            //             qint64 advance = (qint64)sizeof(CFDATA) + (qint64)hdr.cbData;
            //             if (advance <= 0) break;
            //             dataOff += advance;
            //         }
            //     }
            // }
        }

        // // 2) CFFILE table and per-file entries starting at coffFiles
        // if (cfHeader.coffFiles && cfHeader.cFiles) {
        //     // Whole files area (size unknown if names present); add per-record entries with fixed struct size
        //     for (quint32 i = 0; i < cfHeader.cFiles; ++i) {
        //         qint64 recOff = (qint64)cfHeader.coffFiles + (qint64)i * (qint64)sizeof(CFFILE);
        //         if ((recOff + (qint64)sizeof(CFFILE)) > nFileSize) break;
        //         FPART rec = {};
        //         rec.filePart = FILEPART_REGION;
        //         rec.nFileOffset = recOff;
        //         rec.nFileSize = sizeof(CFFILE);
        //         rec.nVirtualAddress = -1;
        //         rec.sName = QString("%1(%2)").arg("CFFILE").arg(i + 1);
        //         listResult.append(rec);
        //     }
        // }
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nFileFormatSize < nFileSize) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nFileFormatSize;
            record.nFileSize = nFileSize - nFileFormatSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XCab::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_CFHEADER;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_CFHEADER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CFHEADER);
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, signature), 4, "signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, reserved1), 4, "reserved1", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cbCabinet), 4, "cbCabinet", VT_UINT32, DRF_SIZE,
//                 dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, reserved2), 4, "reserved2", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, coffFiles), 4, "coffFiles", VT_UINT32, DRF_OFFSET,
//                 dataHeadersOptions.pMemoryMap->endian)); dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, reserved3), 4, "reserved3", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, versionMinor), 1, "versionMinor", VT_UINT8, DRF_VERSION, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFHEADER, versionMajor), 1, "versionMajor", VT_UINT8, DRF_VERSION, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFolders), 2, "cFolders", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFiles), 2, "cFiles", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, flags), 2, "flags", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, setID), 2, "setID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, iCabinet), 2, "iCabinet", VT_UINT16, DRF_UNKNOWN,
//                 dataHeadersOptions.pMemoryMap->endian));
//                 // Optional fields not handled in this example

//                 listResult.append(dataHeader);

//                 if (dataHeadersOptions.bChildren) {
//                     CFHEADER cfHeader = readCFHeader(nStartOffset);
//                     if (cfHeader.cFolders) {
//                         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                         _dataHeadersOptions.nLocation = nStartOffset + sizeof(CFHEADER);
//                         _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//                         _dataHeadersOptions.nCount = cfHeader.cFolders;
//                         _dataHeadersOptions.nSize = sizeof(CFFOLDER) * cfHeader.cFolders;
//                         _dataHeadersOptions.nID = STRUCTID_CFFOLDER;
//                         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//                     }

//                     if (cfHeader.coffFiles) {
//                         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//                         _dataHeadersOptions.nLocation = nStartOffset + cfHeader.coffFiles;
//                         _dataHeadersOptions.dsID_parent = dataHeader.dsID;
//                         _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
//                         _dataHeadersOptions.nCount = cfHeader.cFiles;
//                         _dataHeadersOptions.nSize = sizeof(CFFILE) * cfHeader.cFiles;  // TODO Names and extra fields
//                         _dataHeadersOptions.nID = STRUCTID_CFFILE;
//                         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//                     }
//                 }
//             } else if (dataHeadersOptions.nID == STRUCTID_CFFOLDER) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CFFOLDER);
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFFOLDER, coffCabStart), 4, "coffCabStart", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFOLDER, cCFData), 2, "cCFData", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFFOLDER, typeCompress), 2, "typeCompress", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             } else if (dataHeadersOptions.nID == STRUCTID_CFFILE) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCab::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(CFFILE);
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, cbFile), 4, "cbFile", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(CFFILE, uoffFolderStart), 4, "uoffFolderStart", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, iFolder), 2, "iFolder", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, date), 2, "date", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, time), 2, "time", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(CFFILE, attribs), 2, "attribs", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 QString szName = read_ansiString(nStartOffset + sizeof(CFFILE), 256);  // Limit to 256 chars for safety)
//                 dataHeader.listRecords.append(
//                     getDataRecord(sizeof(CFFILE), szName.size() + 1, "szName", VT_CHAR_ARRAY, DRF_VOLATILE, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

QList<XBinary::MAPMODE> XCab::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    listResult.append(MAPMODE_DATA);

    return listResult;
}

XBinary::_MEMORY_MAP XCab::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

qint64 XCab::getImageSize()
{
    return (qint64)readCFHeader(0).cbCabinet;
}

XBinary::MODE XCab::getMode()
{
    return MODE_DATA;
}

QString XCab::getArch()
{
    return QString("Generic");
}

XBinary::ENDIAN XCab::getEndian()
{
    return ENDIAN_LITTLE;
}

// Streaming unpacking API implementation
QMap<XBinary::UNPACK_PROP, QVariant> XCab::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

bool XCab::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState) {
        return false;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = getSize();
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = nullptr;
    pState->mapUnpackProperties = mapProperties;

    qint64 nFileSize = getSize();

    if (nFileSize < (qint64)sizeof(CFHEADER)) {
        return false;
    }

    // Read CAB header
    CFHEADER cfHeader = readCFHeader(0);
    if (cfHeader.signature[0] != 'M' || cfHeader.signature[1] != 'S' || cfHeader.signature[2] != 'C' || cfHeader.signature[3] != 'F') {
        return false;  // Invalid CAB signature
    }

    // cbCabinet is the authoritative end of the cabinet.  A PE resource may
    // contain alignment padding after it, but no CAB structure may reference
    // that padding.
    qint64 nCabinetSize = cfHeader.cbCabinet;
    if ((cfHeader.reserved1 != 0) || (cfHeader.reserved2 != 0) || (cfHeader.reserved3 != 0) || (cfHeader.flags & ~0x0007) ||
        (nCabinetSize < (qint64)sizeof(CFHEADER)) || (nCabinetSize > nFileSize)) {
        return false;
    }

    // Create unpack context
    CAB_UNPACK_CONTEXT *pContext = new CAB_UNPACK_CONTEXT;
    pContext->nCurrentFileIndex = 0;
    pContext->nCbCFHeader = 0;
    pContext->nCbCFFolder = 0;
    pContext->nCbCFData = 0;

    auto fail = [&]() -> bool {
        delete pContext;
        return false;
    };

    // CAB strings are byte-counted by their terminating NUL, not by the
    // decoded QString length.  Requiring the terminator also prevents a
    // malformed field from walking into the folder/file tables.
    auto readCabString = [&](qint64 *pOffset, QByteArray *pBytes) -> bool {
        if (!pOffset || !pBytes || (*pOffset < 0) || (*pOffset >= nCabinetSize)) {
            return false;
        }

        qint32 nMaximum = (qint32)qMin<qint64>(256, nCabinetSize - *pOffset);
        QByteArray baValue = read_array_process(*pOffset, nMaximum, pPdStruct);
        if ((baValue.size() != nMaximum) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        qint32 nTerminator = baValue.indexOf('\0');
        if (nTerminator < 0) {
            return false;
        }

        *pBytes = baValue.left(nTerminator);
        *pOffset += (qint64)nTerminator + 1;
        return true;
    };

    // Handle reserved fields (flag 0x0004 = cfhdrRESERVE_PRESENT)
    qint64 nFolderOffset = sizeof(CFHEADER);

    if (cfHeader.flags & 0x0004) {
        if (4 > nCabinetSize - nFolderOffset) {
            return fail();
        }
        pContext->nCbCFHeader = read_uint16(nFolderOffset);
        pContext->nCbCFFolder = read_uint8(nFolderOffset + 2);
        pContext->nCbCFData = read_uint8(nFolderOffset + 3);
        nFolderOffset += 4;

        if ((qint64)pContext->nCbCFHeader > nCabinetSize - nFolderOffset) {
            return fail();
        }

        nFolderOffset += pContext->nCbCFHeader;
    }

    // Handle optional previous cabinet name (flag 0x0001 = cfhdrPREV_CABINET)
    if (cfHeader.flags & 0x0001) {
        QByteArray baPreviousCabinet;
        QByteArray baPreviousDisk;
        if (!readCabString(&nFolderOffset, &baPreviousCabinet) || !readCabString(&nFolderOffset, &baPreviousDisk)) {
            return fail();
        }
    }

    // Handle optional next cabinet name (flag 0x0002 = cfhdrNEXT_CABINET)
    if (cfHeader.flags & 0x0002) {
        QByteArray baNextCabinet;
        QByteArray baNextDisk;
        if (!readCabString(&nFolderOffset, &baNextCabinet) || !readCabString(&nFolderOffset, &baNextDisk)) {
            return fail();
        }
    }

    // Parse folders (each CFFOLDER may have per-folder reserved area)
    qint64 nFolderStructSize = (qint64)sizeof(CFFOLDER) + pContext->nCbCFFolder;
    if ((qint64)cfHeader.cFolders * nFolderStructSize > nCabinetSize - nFolderOffset) {
        return fail();
    }

    for (quint16 i = 0; i < cfHeader.cFolders; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return fail();
        }

        CFFOLDER cfFolder = readCFFolder(nFolderOffset);
        quint16 nCompressionType = cfFolder.typeCompress & 0x000F;
        if (nCompressionType > 3) {
            return fail();
        }
        if (nCompressionType == 3) {
            qint32 nWindowBits = (cfFolder.typeCompress >> 8) & 0x1F;
            if ((nWindowBits < 15) || (nWindowBits > 21)) {
                return fail();
            }
        }

        qint64 nFolderDataSize = 0;
        qint64 nStreamSize =
            _getStreamSize(cfFolder.coffCabStart, cfFolder.cCFData, pContext->nCbCFData, nCabinetSize, &nFolderDataSize);
        if ((nStreamSize < 0) || (nFolderDataSize > CAB_MAX_FOLDER_SIZE)) {
            return fail();
        }

        // STORE blocks are byte-for-byte and every CAB data block represents
        // at most 32 KiB of uncompressed folder data.
        qint64 nBlockOffset = cfFolder.coffCabStart;
        for (quint16 nBlock = 0; nBlock < cfFolder.cCFData; nBlock++) {
            CFDATA cfData = readCFData(nBlockOffset);
            if ((cfData.cbUncomp > 32768) || ((nCompressionType == 0) && (cfData.cbData != cfData.cbUncomp)) ||
                ((nCompressionType == 1) && (cfData.cbData < 2))) {
                return fail();
            }

            if (cfData.csum != 0) {
                QByteArray baHeaderAndReserve = read_array_process(nBlockOffset + 4, 4 + pContext->nCbCFData, pPdStruct);
                QByteArray baPayload =
                    read_array_process(nBlockOffset + (qint64)sizeof(CFDATA) + pContext->nCbCFData, cfData.cbData, pPdStruct);
                if ((baHeaderAndReserve.size() != 4 + pContext->nCbCFData) || (baPayload.size() != cfData.cbData) ||
                    !XBinary::isPdStructNotCanceled(pPdStruct)) {
                    return fail();
                }

                quint32 nCalculatedChecksum = cabDataChecksum(baPayload.constData(), baPayload.size());
                nCalculatedChecksum = cabDataChecksum(baHeaderAndReserve.constData(), baHeaderAndReserve.size(), nCalculatedChecksum);
                if (nCalculatedChecksum != cfData.csum) {
                    return fail();
                }
            }

            nBlockOffset += (qint64)sizeof(CFDATA) + pContext->nCbCFData + cfData.cbData;
        }

        pContext->listFolders.append(cfFolder);
        pContext->mapFolderStreamSizes.insert(i, nStreamSize);
        pContext->mapFolderDataSizes.insert(i, nFolderDataSize);
        nFolderOffset += nFolderStructSize;
    }

    // Parse file offsets starting at coffFiles
    qint64 nFileOffset = cfHeader.coffFiles;
    if ((cfHeader.cFiles > 0) && ((nFileOffset < nFolderOffset) || (nFileOffset >= nCabinetSize))) {
        return fail();
    }

    for (quint16 i = 0; i < cfHeader.cFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return fail();
        }

        if ((qint64)sizeof(CFFILE) > nCabinetSize - nFileOffset) {
            return fail();
        }

        CFFILE cfFile = readCFFILE(nFileOffset);
        qint64 nNameOffset = nFileOffset + sizeof(CFFILE);
        QByteArray baFileName;
        if (!readCabString(&nNameOffset, &baFileName) || baFileName.isEmpty()) {
            return fail();
        }

        QString sFileName;
        if (cfFile.attribs & 0x0080) {
            sFileName = QString::fromUtf8(baFileName);
            if (sFileName.toUtf8() != baFileName) {
                return fail();
            }
        } else {
            sFileName = QString::fromLatin1(baFileName);
        }

        if (cfFile.iFolder < (quint16)pContext->listFolders.size()) {
            qint64 nFileEnd = (qint64)cfFile.uoffFolderStart + (qint64)cfFile.cbFile;
            if (nFileEnd > pContext->mapFolderDataSizes.value(cfFile.iFolder, -1)) {
                return fail();
            }
            pContext->mapFolderUncompressedSizes[cfFile.iFolder] = qMax(pContext->mapFolderUncompressedSizes.value(cfFile.iFolder, 0), nFileEnd);
        } else if ((cfFile.iFolder != 0xFFFD) && (cfFile.iFolder != 0xFFFE) && (cfFile.iFolder != 0xFFFF)) {
            return fail();
        }

        pContext->listFileOffsets.append(nFileOffset);
        pContext->listFileNames.append(sFileName);
        nFileOffset = nNameOffset;
    }

    // The variable-length file table must end before the first folder data
    // stream.  This catches unterminated/overlapping file entries even when a
    // forged offset happens to remain inside the cabinet.
    for (const CFFOLDER &cfFolder : pContext->listFolders) {
        if ((qint64)cfFolder.coffCabStart < nFileOffset) {
            return fail();
        }
    }

    // Initialize state
    pState->nCurrentOffset = cfHeader.coffFiles;
    pState->nTotalSize = nCabinetSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = cfHeader.cFiles;
    pState->pContext = pContext;

    return true;
}

XBinary::ARCHIVERECORD XCab::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }

    CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
    qint64 nFileOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);

    CFFILE cfFile = readCFFILE(nFileOffset);
    QString sFileName = pContext->listFileNames.value(pState->nCurrentIndex);

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)cfFile.cbFile);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)cfFile.attribs);

    // Convert DOS date/time to QDateTime
    quint32 nDosDate = (quint32)cfFile.date;
    quint32 nDosTime = (quint32)cfFile.time;
    qint32 nYear = ((nDosDate >> 9) & 0x7F) + 1980;
    qint32 nMonth = (nDosDate >> 5) & 0x0F;
    qint32 nDay = nDosDate & 0x1F;
    qint32 nHour = (nDosTime >> 11) & 0x1F;
    qint32 nMinute = (nDosTime >> 5) & 0x3F;
    qint32 nSecond = (nDosTime & 0x1F) * 2;

    QDate date(nYear, nMonth, nDay);
    QTime time(nHour, nMinute, nSecond);

    if (date.isValid() && time.isValid()) {
        QDateTime dateTime(date, time, Qt::UTC);
        result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
    }

    if (cfFile.iFolder < (quint16)pContext->listFolders.size()) {
        CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);

        result.nStreamOffset = cfFolder.coffCabStart;
        result.nStreamSize = pContext->mapFolderStreamSizes.value(cfFile.iFolder, -1);

        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);
        result.mapProperties.insert(FPART_PROP_TYPE, (quint32)cfFolder.typeCompress);
        result.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)cfFile.iFolder);
        result.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, (qint64)cfFile.uoffFolderStart);
        result.mapProperties.insert(FPART_PROP_OPTHEADER_OFFSET, (qint64)cfFile.uoffFolderStart);
        result.mapProperties.insert(FPART_PROP_OPTHEADER_SIZE, (qint64)pContext->nCbCFData);
        result.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, pContext->mapFolderDataSizes.value(
                                                                               cfFile.iFolder, (qint64)cfFile.uoffFolderStart + cfFile.cbFile));

        // Set compression method
        quint16 nCompressType = cfFolder.typeCompress & 0x000F;

        if (nCompressType == 0x0000) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE_CAB);
        } else if (nCompressType == 0x0001) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_MSZIP_CAB);
        } else if (nCompressType == 0x0003) {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_LZX_CAB);
            // LZX window size is stored in the upper bits of typeCompress: (typeCompress >> 8) & 0x1F
            result.mapProperties.insert(FPART_PROP_WINDOWSIZE, (qint64)((cfFolder.typeCompress >> 8) & 0x1F));
        } else {
            result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN);
        }
    }

    return result;
}

// bool XCab::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
// {
//     if (!pState || !pState->pContext || !pDevice) {
//         return false;
//     }

//     if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
//         return false;
//     }

//     PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
//     if (!pPdStruct) {
//         pPdStruct = &pdStructEmpty;
//     }

//     CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
//     qint64 nFileOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);

//     CFFILE cfFile = readCFFILE(nFileOffset);

//     if (cfFile.iFolder >= (quint16)pContext->listFolders.size()) {
//         return false;
//     }

//     CFFOLDER cfFolder = pContext->listFolders.at(cfFile.iFolder);

//     // Determine compression type (lower 4 bits)
//     quint16 nCompressionType = cfFolder.typeCompress & 0x000F;
//     qint64 nTotalSize = getSize();
//     // Per-datablock reserved area size
//     qint64 nDataReservedSize = (qint64)pContext->nCbCFData;

//     if (nCompressionType == 0x0000) {
//         // STORE method (no compression) - direct copy from data blocks
//         qint64 nDataOffset = cfFolder.coffCabStart;
//         qint64 nCurrentUncompressedOffset = 0;
//         qint64 nTargetOffset = cfFile.uoffFolderStart;
//         qint64 nRemainingSize = cfFile.cbFile;
//         qint64 nWriteOffset = 0;

//         for (quint16 i = 0; (i < cfFolder.cCFData) && (nRemainingSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
//             if ((nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize) > nTotalSize) {
//                 break;
//             }

//             CFDATA cfData = readCFData(nDataOffset);
//             qint64 nPayloadOffset = nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize;

//             if (nCurrentUncompressedOffset + cfData.cbUncomp <= nTargetOffset) {
//                 // Skip this block entirely
//                 nCurrentUncompressedOffset += cfData.cbUncomp;
//                 nDataOffset = nPayloadOffset + cfData.cbData;
//                 continue;
//             }

//             // This block contains data we need
//             qint64 nBlockStart = nTargetOffset - nCurrentUncompressedOffset;
//             qint64 nCopySize = qMin(nRemainingSize, (qint64)cfData.cbUncomp - nBlockStart);

//             if (nCopySize > 0) {
//                 qint64 nSourceOffset = nPayloadOffset + nBlockStart;

//                 if ((nSourceOffset + nCopySize) > nTotalSize) {
//                     return false;
//                 }

//                 if (!copyDeviceMemory(getDevice(), nSourceOffset, pDevice, nWriteOffset, nCopySize)) {
//                     return false;
//                 }

//                 nWriteOffset += nCopySize;
//             }

//             nTargetOffset += nCopySize;
//             nRemainingSize -= nCopySize;
//             nCurrentUncompressedOffset += cfData.cbUncomp;
//             nDataOffset = nPayloadOffset + cfData.cbData;
//         }

//         return (nRemainingSize == 0);
//     } else if (nCompressionType == 0x0001) {
//         // MSZIP method (DEFLATE with 2-byte "CK" signature per block)
//         // Use folder cache to avoid re-decompressing the same folder for multiple files
//         quint16 nFolderIndex = cfFile.iFolder;

//         if (!pContext->mapFolderCache.contains(nFolderIndex)) {
//             // Decompress entire folder into cache
//             QByteArray baFolderData;

//             qint64 nDataOffset = cfFolder.coffCabStart;

//             for (quint16 i = 0; (i < cfFolder.cCFData) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
//                 if ((nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize) > nTotalSize) {
//                     break;
//                 }

//                 CFDATA cfData = readCFData(nDataOffset);
//                 qint64 nPayloadOffset = nDataOffset + (qint64)sizeof(CFDATA) + nDataReservedSize;

//                 if (cfData.cbData < 2) {
//                     return false;  // Invalid block (needs at least 2-byte "CK" signature)
//                 }

//                 // Verify MSZIP signature "CK" (0x43 0x4B)
//                 quint8 nSig0 = read_uint8(nPayloadOffset);
//                 quint8 nSig1 = read_uint8(nPayloadOffset + 1);

//                 if (nSig0 != 0x43 || nSig1 != 0x4B) {
//                     return false;  // Invalid MSZIP block signature
//                 }

//                 qint64 nCompressedDataOffset = nPayloadOffset + 2;
//                 qint64 nCompressedDataSize = cfData.cbData - 2;

//                 if (nCompressedDataSize <= 0) {
//                     return false;
//                 }

//                 if ((nCompressedDataOffset + nCompressedDataSize) > nTotalSize) {
//                     return false;
//                 }

//                 QByteArray baCompressedData = read_array(nCompressedDataOffset, nCompressedDataSize);

//                 if (baCompressedData.size() != nCompressedDataSize) {
//                     return false;
//                 }

//                 QBuffer bufferCompressed(&baCompressedData);
//                 if (!bufferCompressed.open(QIODevice::ReadOnly)) {
//                     return false;
//                 }

//                 QByteArray baUncompressedBlock;
//                 QBuffer bufferUncompressed(&baUncompressedBlock);
//                 if (!bufferUncompressed.open(QIODevice::WriteOnly)) {
//                     bufferCompressed.close();
//                     return false;
//                 }

//                 DATAPROCESS_STATE decompressState = {};
//                 decompressState.pDeviceInput = &bufferCompressed;
//                 decompressState.pDeviceOutput = &bufferUncompressed;
//                 decompressState.nInputOffset = 0;
//                 decompressState.nInputLimit = nCompressedDataSize;
//                 decompressState.nProcessedOffset = 0;
//                 decompressState.nProcessedLimit = cfData.cbUncomp;

//                 bool bDecompressResult = XDeflateDecoder::decompress(&decompressState, pPdStruct);

//                 bufferCompressed.close();
//                 bufferUncompressed.close();

//                 if (!bDecompressResult) {
//                     return false;
//                 }

//                 if (baUncompressedBlock.size() != (qint32)cfData.cbUncomp) {
//                     return false;
//                 }

//                 baFolderData.append(baUncompressedBlock);
//                 nDataOffset = nPayloadOffset + cfData.cbData;
//             }

//             pContext->mapFolderCache.insert(nFolderIndex, baFolderData);
//         }

//         // Extract file data from the cached folder
//         const QByteArray &baFolderData = pContext->mapFolderCache.value(nFolderIndex);

//         if (baFolderData.size() < (qint64)cfFile.uoffFolderStart + (qint64)cfFile.cbFile) {
//             return false;
//         }

//         qint64 nBytesWritten = pDevice->write(baFolderData.constData() + cfFile.uoffFolderStart, cfFile.cbFile);
//         return (nBytesWritten == (qint64)cfFile.cbFile);
//     } else if (nCompressionType == 0x0003) {
//         // LZX method - not yet implemented (requires specialized LZX decoder)
//         // LZX window size is encoded in upper bits: (typeCompress >> 8) & 0x1F
//         return false;
//     }

//     return false;  // Unsupported compression type
// }

bool XCab::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && pState->pContext && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;

        pState->nCurrentIndex++;

        if (pState->nCurrentIndex < pState->nNumberOfRecords) {
            pState->nCurrentOffset = pContext->listFileOffsets.at(pState->nCurrentIndex);
            bResult = true;
        }
    }

    return bResult;
}

bool XCab::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        CAB_UNPACK_CONTEXT *pContext = (CAB_UNPACK_CONTEXT *)pState->pContext;
        pContext->mapFolderCache.clear();
        pContext->mapFolderUncompressedSizes.clear();
        pContext->mapFolderStreamSizes.clear();
        pContext->mapFolderDataSizes.clear();
        delete pContext;
        pState->pContext = nullptr;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}

QList<QString> XCab::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append("'MSCF'");
    return listResult;
}

XBinary *XCab::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCab(pDevice);
}

bool XCab::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XCab::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XCab::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
