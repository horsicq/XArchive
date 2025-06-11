/* Copyright (c) 2022-2025 hors<horsicq@gmail.com>
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
#include "xgzip.h"

XGzip::XGzip(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XGzip::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(GZIP_HEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "1F8B08", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XGzip::isValid(QIODevice *pDevice)
{
    XGzip xgzip(pDevice);

    return xgzip.isValid();
}

quint64 XGzip::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    return 1;  // Always 1
}

QList<XArchive::RECORD> XGzip::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)  // Always 1

    QList<RECORD> listResult;

    RECORD record = {};

    qint64 nOffset = 0;

    GZIP_HEADER gzipHeader = {};

    read_array(nOffset, (char *)&gzipHeader, sizeof(GZIP_HEADER));

    if (gzipHeader.nCompressionMethod == 8)  // TODO consts
    {
        record.spInfo.compressMethod = COMPRESS_METHOD_DEFLATE;  // TODO more
    }

    nOffset += sizeof(GZIP_HEADER);

    if (gzipHeader.nFileFlags & 8)  // File name
    {
        record.spInfo.sRecordName = read_ansiString(nOffset);
        nOffset += record.spInfo.sRecordName.size() + 1;
    }

    SubDevice sd(getDevice(), nOffset, -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XArchive::DECOMPRESSSTRUCT decompressStruct = {};
        decompressStruct.spInfo.compressMethod = record.spInfo.compressMethod;
        decompressStruct.pSourceDevice = &sd;
        decompressStruct.pDestDevice = 0;

        XArchive::COMPRESS_RESULT cr = _decompress(&decompressStruct, pPdStruct);

        Q_UNUSED(cr)

        record.nHeaderOffset = 0;
        record.nHeaderSize = nOffset;
        record.nDataOffset = nOffset;
        record.nDataSize = decompressStruct.nInSize;
        record.spInfo.nUncompressedSize = decompressStruct.nOutSize;
        record.spInfo.sRecordName = XBinary::getDeviceFileBaseName(getDevice()); // TODO, use from header

        sd.close();
    }

    // TODO

    listResult.append(record);

    return listResult;
}

qint64 XGzip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XGzip::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XGzip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};

    result.fileType = getFileType();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = getTypeAsString();
    result.nBinarySize = getSize();

    _MEMORY_RECORD memoryRecordHeader = {};
    _MEMORY_RECORD memoryRecord = {};
    _MEMORY_RECORD memoryRecordFooter = {};

    qint64 nOffset = 0;

    GZIP_HEADER gzipHeader = {};

    read_array(nOffset, (char *)&gzipHeader, sizeof(GZIP_HEADER));

    COMPRESS_METHOD cm = COMPRESS_METHOD_DEFLATE;

    if (gzipHeader.nCompressionMethod == 8)  // TODO consts
    {
        cm = COMPRESS_METHOD_DEFLATE;  // TODO more
    }

    nOffset += sizeof(GZIP_HEADER);

    if (gzipHeader.nFileFlags & 8)  // File name
    {
        QString sFileName = read_ansiString(nOffset);
        nOffset += sFileName.size() + 1;
    }

    memoryRecordHeader.nOffset = 0;
    memoryRecordHeader.nAddress = -1;
    memoryRecordHeader.nSize = nOffset;
    memoryRecordHeader.sName = tr("Header");
    memoryRecordHeader.type = MMT_HEADER;

    result.listRecords.append(memoryRecordHeader);

    SubDevice sd(getDevice(), nOffset, -1);

    if (sd.open(QIODevice::ReadOnly)) {
        XArchive::DECOMPRESSSTRUCT decompressStruct = {};
        decompressStruct.spInfo.compressMethod = cm;
        decompressStruct.pSourceDevice = &sd;
        decompressStruct.pDestDevice = 0;

        XArchive::COMPRESS_RESULT cr = _decompress(&decompressStruct, pPdStruct);

        Q_UNUSED(cr)

        memoryRecord.nOffset = nOffset;
        memoryRecord.nAddress = -1;
        memoryRecord.nSize = decompressStruct.nInSize;
        memoryRecord.type = MMT_FILESEGMENT;

        sd.close();
    }

    // TODO

    result.listRecords.append(memoryRecord);

    memoryRecordFooter.nOffset = memoryRecord.nOffset + memoryRecord.nSize;
    memoryRecordFooter.nAddress = -1;
    memoryRecordFooter.nSize = 8;
    memoryRecordFooter.sName = tr("Footer");
    memoryRecordFooter.type = MMT_FOOTER;

    result.listRecords.append(memoryRecordFooter);

    _handleOverlay(&result);

    return result;
}

QString XGzip::getFileFormatExt()
{
    return "gz";
}

QString XGzip::getFileFormatExtsString()
{
    return "GZIP (*.gz)";
}

XBinary::FT XGzip::getFileType()
{
    return FT_GZIP;
}
