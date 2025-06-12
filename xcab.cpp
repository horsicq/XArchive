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
#include "xcab.h"

static XBinary::XCONVERT _TABLE_XCAB_STRUCTID[] = {
    {XCab::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XCab::STRUCTID_CFHEADER, "CFHEADER", QString("CFHEADER")},
    {XCab::STRUCTID_CFFOLDER, "CFFOLDER", QString("CFFOLDER")},
    {XCab::STRUCTID_CFFILE, "CFFILE", QString("CFFILE")},
    {XCab::STRUCTID_CFDATA, "CFDATA", QString("CFDATA")}
};

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

bool XCab::isValid(QIODevice *pDevice)
{
    XCab xcab(pDevice);

    return xcab.isValid();
}

QString XCab::getVersion()
{
    return QString("%1.%2").arg(read_uint8(25)).arg(read_uint8(24), 2, 10, QChar('0'));
}

quint64 XCab::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    quint64 nResult = 0;

    nResult = read_uint16(offsetof(CFHEADER, cFiles));

    return nResult;
}

QList<XArchive::RECORD> XCab::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<XArchive::RECORD> listResult;

    qint64 nOffset = 0;  // TODO

    CFHEADER cfHeader = readCFHeader(0);

    nOffset += sizeof(CFHEADER) - 4;

    // if (cfHeader.flags & 0x0004)  // TODO const
    // {
    //     nOffset += 4;

    //     nOffset += cfHeader.cbCFHeader;
    // }

    // CFFOLDER cfFolder=readCFFolder(nOffset);

    // TODO

    return listResult;
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

    return nResult;
}

QString XCab::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XCAB_STRUCTID, sizeof(_TABLE_XCAB_STRUCTID) / sizeof(XBinary::XCONVERT));
}

qint32 XCab::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<QVariant> *pListValues, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nRow)
    Q_UNUSED(locType)
    Q_UNUSED(nLocation)
    Q_UNUSED(dataRecordsOptions)
    Q_UNUSED(pListValues)
    Q_UNUSED(pPdStruct)
    // Not implemented for CAB
    return 0;
}

QList<XBinary::DATA_HEADER> XCab::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;

        _dataHeadersOptions.nID = STRUCTID_CFHEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            XBinary::DATA_HEADER dataHeader = {};
            dataHeader.dsID_parent = dataHeadersOptions.dsID_parent;
            dataHeader.dsID.sGUID = generateUUID();
            dataHeader.dsID.fileType = dataHeadersOptions.pMemoryMap->fileType;
            dataHeader.dsID.nID = dataHeadersOptions.nID;
            dataHeader.locType = dataHeadersOptions.locType;
            dataHeader.nLocation = dataHeadersOptions.nLocation;
            dataHeader.sName = structIDToString(dataHeadersOptions.nID);
            dataHeader.dhMode = dataHeadersOptions.dhMode;

            if (dataHeadersOptions.nID == STRUCTID_CFHEADER) {
                dataHeader.nSize = sizeof(CFHEADER);
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, signature), 4, "signature", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, reserved1), 4, "reserved1", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cbCabinet), 4, "cbCabinet", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, reserved2), 4, "reserved2", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, coffFiles), 4, "coffFiles", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, reserved3), 4, "reserved3", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, versionMinor), 1, "versionMinor", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, versionMajor), 1, "versionMajor", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFolders), 2, "cFolders", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, cFiles), 2, "cFiles", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, flags), 2, "flags", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, setID), 2, "setID", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(CFHEADER, iCabinet), 2, "iCabinet", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                // Optional fields not handled in this example

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::MAPMODE> XCab::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XCab::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.nBinarySize = getSize();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());

    CFHEADER header = readCFHeader(0);
    qint32 nIndex = 0;

    // Calculate CAB header size
    qint64 nHeaderSize = sizeof(CFHEADER);
    if (header.flags & 0x0004) {  // TODO const
        nHeaderSize += 4;
    }

    _MEMORY_RECORD recordHeader = {};
    recordHeader.nAddress = -1;
    recordHeader.segment = ADDRESS_SEGMENT_FLAT;
    recordHeader.nOffset = 0;
    recordHeader.nSize = nHeaderSize;
    recordHeader.nIndex = nIndex++;
    recordHeader.type = MMT_HEADER;
    recordHeader.sName = tr("Cabinet Header");
    result.listRecords.append(recordHeader);

    return result;
}
