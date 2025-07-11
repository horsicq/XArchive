/* Copyright (c) 2023-2025 hors<horsicq@gmail.com>
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
#include "xlha.h"

XLHA::XLHA(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XLHA::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 12) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "....'-lh'..2d", 0, pPdStruct) || compareSignature(&memoryMap, "....'-lz'..2d", 0, pPdStruct) ||
            compareSignature(&memoryMap, "....'-pm'..2d", 0, pPdStruct)) {
            QString sMethod = read_ansiString(2, 5);

            if ((sMethod == "-lzs-") || (sMethod == "-lz2-") || (sMethod == "-lz3-") || (sMethod == "-lz4-") || (sMethod == "-lz5-") || (sMethod == "-lz7-") ||
                (sMethod == "-lz8-") || (sMethod == "-lh0-") || (sMethod == "-lh1-") || (sMethod == "-lh2-") || (sMethod == "-lh3-") || (sMethod == "-lh4-") ||
                (sMethod == "-lh5-") || (sMethod == "-lh6-") || (sMethod == "-lh7-") || (sMethod == "-lh8-") || (sMethod == "-lh9-") || (sMethod == "-lha-") ||
                (sMethod == "-lhb-") || (sMethod == "-lhc-") || (sMethod == "-lhe-") || (sMethod == "-lhd-") || (sMethod == "-lhx-") || (sMethod == "-pm0-") ||
                (sMethod == "-pm2-")) {
                bResult = true;
            }
            bResult = true;
        }
    }

    return bResult;
}

bool XLHA::isValid(QIODevice *pDevice)
{
    XLHA xhla(pDevice);

    return xhla.isValid();
}

quint64 XLHA::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return getRecords(-1, pPdStruct).count();
}

QList<XArchive::RECORD> XLHA::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

    qint64 nFileSize = getSize();

    qint64 nOffset = 0;

    qint32 nNumberOfFiles = 0;

    while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (compareSignature(&memoryMap, "....'-lh'..2d", nOffset) || compareSignature(&memoryMap, "....'-lz'..2d", nOffset) ||
            compareSignature(&memoryMap, "....'-pm'..2d", nOffset)) {
            qint64 nHeaderSize = read_uint8(nOffset) + 2;
            qint64 nCompressedSize = read_uint32(nOffset + 7);
            qint64 nUncompressedSize = read_uint32(nOffset + 11);
            QString sFileName = read_ansiString(nOffset + 22, read_uint8(nOffset + 21));
            sFileName = sFileName.replace("\\", "/");

            if (nHeaderSize < 21) {
                break;
            }

            XArchive::RECORD record = {};
            // TODO CRC
            record.spInfo.compressMethod = COMPRESS_METHOD_LZH5;

            QString sMethod = read_ansiString(nOffset + 2, 5);

            if ((sMethod == "-lh0-") || (sMethod == "-lz4-") || (sMethod == "-lhd-")) {
                record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
            } else if (sMethod == "-lh5-") {
                record.spInfo.compressMethod = COMPRESS_METHOD_LZH5;
            } else if (sMethod == "-lh6-") {
                record.spInfo.compressMethod = COMPRESS_METHOD_LZH6;
            } else if (sMethod == "-lh7-") {
                record.spInfo.compressMethod = COMPRESS_METHOD_LZH7;
            }

            record.nHeaderOffset = nOffset;
            record.nDataOffset = nOffset + nHeaderSize;
            record.spInfo.nUncompressedSize = nUncompressedSize;
            record.nHeaderSize = nHeaderSize;
            record.nDataSize = nCompressedSize;
            record.spInfo.sRecordName = sFileName;

            listResult.append(record);

            nNumberOfFiles++;

            if (nLimit != -1) {
                if (nNumberOfFiles > nLimit) {
                    break;
                }
            }

            nOffset += (nHeaderSize + nCompressedSize);
            nFileSize -= (nHeaderSize + nCompressedSize);
        } else {
            break;
        }
    }

    return listResult;
}

qint64 XLHA::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QList<XBinary::MAPMODE> XLHA::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XLHA::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap();

    _MEMORY_MAP result = {};

    result.nBinarySize = getSize();
    result.fileType = getFileType();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = getTypeAsString();

    qint64 nFileSize = getSize();

    qint64 nOffset = 0;

    while ((nFileSize > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (compareSignature(&memoryMap, "....'-lh'..2d", nOffset) || compareSignature(&memoryMap, "....'-lz'..2d", nOffset) ||
            compareSignature(&memoryMap, "....'-pm'..2d", nOffset)) {
            qint64 nHeaderSize = read_uint8(nOffset) + 2;
            qint64 nDataSize = read_uint32(nOffset + 7);
            QString sFileName = read_ansiString(nOffset + 22, read_uint8(nOffset + 21));

            if (nHeaderSize < 21) {
                break;
            }

            {
                _MEMORY_RECORD memoryRecord = {};

                memoryRecord.nOffset = nOffset;
                memoryRecord.nAddress = -1;
                memoryRecord.nSize = nHeaderSize;
                memoryRecord.sName = tr("Header");
                memoryRecord.filePart = FILEPART_HEADER;

                result.listRecords.append(memoryRecord);
            }

            {
                _MEMORY_RECORD memoryRecord = {};

                memoryRecord.nOffset = nOffset + nHeaderSize;
                memoryRecord.nAddress = -1;
                memoryRecord.nSize = nDataSize;
                memoryRecord.sName = sFileName;
                memoryRecord.filePart = FILEPART_REGION;

                result.listRecords.append(memoryRecord);
            }

            nOffset += (nHeaderSize + nDataSize);
            nFileSize -= (nHeaderSize + nDataSize);
        } else {
            break;
        }
    }

    _handleOverlay(&result);

    return result;
}

XBinary::FT XLHA::getFileType()
{
    return FT_LHA;
}

QString XLHA::getFileFormatExt()
{
    QString sResult = "lha";
    QString _sVersion = getVersion().left(2);

    if (_sVersion == "lh") {
        sResult = "lha";
    } else if (_sVersion == "lz") {
        sResult = "lzs";
    } else if (_sVersion == "pm") {
        sResult = "pma";
    }

    return sResult;
}

QString XLHA::getFileFormatExtsString()
{
    return "LHA(lha, lzs, pma)";
}

QString XLHA::getMIMEString()
{
    return "application/x-lzh-compressed";
}

QString XLHA::getVersion()
{
    return read_ansiString(3, 3);
}

QString XLHA::getArch()
{
    return QString();
}

XBinary::ENDIAN XLHA::getEndian()
{
    return ENDIAN_LITTLE;  // LHA is little-endian
}

XBinary::MODE XLHA::getMode()
{
    return MODE_DATA;
}
