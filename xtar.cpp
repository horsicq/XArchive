/* Copyright (c) 2017-2024 hors<horsicq@gmail.com>
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
#include "xtar.h"

XTAR::XTAR(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XTAR::isValid(PDSTRUCT *pPdStruct)
{
    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

    return _isValid(&memoryMap, 0, pPdStruct);
}

bool XTAR::_isValid(_MEMORY_MAP *pMemoryMap, qint64 nOffset, PDSTRUCT *pPdStruct)
{
    // TODO more checks
    bool bResult = false;

    if ((getSize() - nOffset) >= 0x200)  // TODO const
    {
        if (compareSignature(pMemoryMap, "00'ustar'", nOffset + 0x100, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XTAR::isValid(QIODevice *pDevice)
{
    XTAR xtar(pDevice);

    return xtar.isValid();
}

quint64 XTAR::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

    qint64 nOffset = 0;

    while (!(pPdStruct->bIsStop)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 6)) {
            break;
        }

        nResult++;

        nOffset += (0x200);
        nOffset += align_up(QString(header.size).toULongLong(0, 8), 0x200);
    }

    return nResult;
}

QList<XArchive::RECORD> XTAR::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    qint64 nOffset = 0;

    while (!(pPdStruct->bIsStop)) {
        XTAR::posix_header header = read_posix_header(nOffset);

        if (!compareMemory(header.magic, "ustar", 6)) {
            break;
        }

        RECORD record = {};
        record.compressMethod = COMPRESS_METHOD_STORE;
        record.nDataOffset = nOffset + 0x200;
        record.nHeaderOffset = nOffset;
        record.nHeaderSize = 0x200;
        record.nUncompressedSize = QString(header.size).toULongLong(0, 8);
        record.sFileName = header.name;
        record.nCompressedSize = record.nUncompressedSize;

        listResult.append(record);

        nOffset += (0x200);
        nOffset += align_up(record.nCompressedSize, 0x200);
    }

    return listResult;
}

QString XTAR::getFileFormatExt()
{
    return "tar";
}

QList<XBinary::MAPMODE> XTAR::getMapModesList(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::FT XTAR::getFileType()
{
    return FT_TAR;
}

XTAR::posix_header XTAR::read_posix_header(qint64 nOffset)
{
    posix_header record = {};

    read_array(nOffset, (char *)&record, sizeof(record));

    return record;
}
