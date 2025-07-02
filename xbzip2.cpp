/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xbzip2.h"

XBinary::XCONVERT _TABLE_XBZIP2_STRUCTID[] = {{XBZIP2::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                              {XBZIP2::STRUCTID_BZIP2_HEADER, "BZIP2_HEADER", QString("BZip2 header")},
                                              {XBZIP2::STRUCTID_BLOCK_HEADER, "BLOCK_HEADER", QString("Block header")}};

XBZIP2::XBZIP2(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBZIP2::~XBZIP2()
{
}

bool XBZIP2::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 14) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "'BZh'..314159265359", 0, pPdStruct) || compareSignature(&memoryMap, "'BZh'..17724538509000000000", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

XBinary::MODE XBZIP2::getMode()
{
    return MODE_DATA;
}

qint32 XBZIP2::getType()
{
    return TYPE_BZ2;
}

XBinary::ENDIAN XBZIP2::getEndian()
{
    return ENDIAN_LITTLE;  // BZip2 is always little-endian
}

QString XBZIP2::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_BZ2: sResult = QString("BZ2"); break;
    }

    return sResult;
}

QString XBZIP2::getFileFormatExt()
{
    return "bz2";
}

XBinary::FT XBZIP2::getFileType()
{
    return FT_BZIP2;
}

QString XBZIP2::getFileFormatExtsString()
{
    return "bz2";
}

qint64 XBZIP2::getFileFormatSize(XBinary::PDSTRUCT *pPdStruct)
{
    qint64 nResult = getSize();

    return nResult;
}

QString XBZIP2::getMIMEString()
{
    return "application/x-bzip2";
}

XBinary::OSNAME XBZIP2::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QList<XBinary::MAPMODE> XBZIP2::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XBZIP2::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result.nBinarySize = getSize();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = typeIdToString(getType());

    qint32 nIndex = 0;

    SubDevice sd(getDevice(), 0, -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XArchive::DECOMPRESSSTRUCT decompressStruct = {};
        decompressStruct.spInfo.compressMethod = COMPRESS_METHOD_BZIP2;
        decompressStruct.pSourceDevice = &sd;
        decompressStruct.pDestDevice = 0;
        decompressStruct.spInfo.nUncompressedSize = 0;

        XArchive::COMPRESS_RESULT cr = _decompress(&decompressStruct, pPdStruct);

        _MEMORY_RECORD memoryRecord = {};
        memoryRecord.nOffset = 0;
        memoryRecord.nAddress = -1;
        memoryRecord.nSize = decompressStruct.nInSize;
        memoryRecord.filePart = FILEPART_REGION;
        memoryRecord.nIndex = nIndex;

        result.listRecords.append(memoryRecord);

        sd.close();
    }

    // Check for overlay
    if (isPdStructNotCanceled(pPdStruct)) {
        _handleOverlay(&result);
    }

    return result;
}

QString XBZIP2::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XBZIP2_STRUCTID, sizeof(_TABLE_XBZIP2_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XBZIP2::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_BZIP2_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        if (isPdStructNotCanceled(pPdStruct)) {
            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_BZIP2_HEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XBZIP2::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = sizeof(BZIP2_HEADER);

                dataHeader.listRecords.append(
                    getDataRecord(offsetof(BZIP2_HEADER, magic), 3, "magic", VT_CHAR_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(BZIP2_HEADER, blockSize), 1, "blockSize", VT_BYTE, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

quint64 XBZIP2::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // BZip2 typically contains a single compressed stream
    return 1;
}

QList<XArchive::RECORD> XBZIP2::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listResult;

    if (isPdStructNotCanceled(pPdStruct)) {
        SubDevice sd(getDevice(), 0, -1);

        if (sd.open(QIODevice::ReadOnly)) {
            XArchive::DECOMPRESSSTRUCT decompressStruct = {};
            decompressStruct.spInfo.compressMethod = COMPRESS_METHOD_BZIP2;
            decompressStruct.pSourceDevice = &sd;
            decompressStruct.pDestDevice = 0;
            decompressStruct.spInfo.nUncompressedSize = 0;

            XArchive::COMPRESS_RESULT cr = _decompress(&decompressStruct, pPdStruct);

            Q_UNUSED(cr)
            RECORD record = {};
            record.nHeaderOffset = 0;
            record.nHeaderSize = sizeof(BZIP2_HEADER);
            record.nDataOffset = 0;
            record.nDataSize = decompressStruct.nInSize;
            record.spInfo.nUncompressedSize = decompressStruct.nOutSize;
            record.spInfo.sRecordName = XBinary::getDeviceFileBaseName(getDevice());
            record.spInfo.compressMethod = COMPRESS_METHOD_BZIP2;
            record.sUUID = generateUUID();
            listResult.append(record);

            sd.close();
        }
    }

    Q_UNUSED(nLimit)  // We always return just one record for BZip2

    return listResult;
}

XBZIP2::BZIP2_HEADER XBZIP2::_read_BZIP2_HEADER(qint64 nOffset)
{
    BZIP2_HEADER result = {};

    // Read the magic "BZh" signature
    read_array(nOffset, (char *)&result.magic, 3);

    // Read the block size
    result.blockSize = read_uint8(nOffset + 3);

    return result;
}
