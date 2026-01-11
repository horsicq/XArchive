/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xsquashfs.h"

XBinary::XCONVERT _TABLE_XSQUASHFS_STRUCTID[] = {{XSquashfs::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                 {XSquashfs::STRUCTID_HEADER, "HEADER", QString("Header")},
                                                 {XSquashfs::STRUCTID_SUPERBLOCK, "SUPERBLOCK", QString("Superblock")}};

XSquashfs::XSquashfs(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSquashfs::~XSquashfs()
{
}

bool XSquashfs::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= (qint64)sizeof(SQUASHFS_HEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "73717368", 0, pPdStruct)) {  // "sqsh"
            bResult = true;
        }
    }

    return bResult;
}

bool XSquashfs::isValid(QIODevice *pDevice)
{
    XSquashfs xsquashfs(pDevice);
    return xsquashfs.isValid();
}

XSquashfs::SQUASHFS_HEADER XSquashfs::_readHeader(qint64 nOffset)
{
    SQUASHFS_HEADER header = {};
    read_array(nOffset, (char *)&header, sizeof(SQUASHFS_HEADER));
    return header;
}

XSquashfs::SQUASHFS_COMPRESSION XSquashfs::_getCompressionMethod(quint16 nType)
{
    SQUASHFS_COMPRESSION result = COMP_UNKNOWN;

    switch (nType) {
        case 1: result = COMP_GZIP; break;
        case 2: result = COMP_LZMA; break;
        case 3: result = COMP_LZO; break;
        case 4: result = COMP_XZ; break;
        case 5: result = COMP_LZ4; break;
        case 6: result = COMP_ZSTD; break;
    }

    return result;
}

QString XSquashfs::_getCompressionMethodString(SQUASHFS_COMPRESSION comp)
{
    switch (comp) {
        case COMP_GZIP: return "GZIP";
        case COMP_LZMA: return "LZMA";
        case COMP_LZO: return "LZO";
        case COMP_XZ: return "XZ";
        case COMP_LZ4: return "LZ4";
        case COMP_ZSTD: return "ZSTD";
        default: return "Unknown";
    }
}

QString XSquashfs::getFileFormatExt()
{
    return "sqsh";
}

QString XSquashfs::getFileFormatExtsString()
{
    return "Squashfs (*.sqsh; *.squashfs)";
}

QString XSquashfs::getMIMEString()
{
    return "application/x-squashfs";
}

XBinary::FT XSquashfs::getFileType()
{
    return FT_SQUASHFS;
}

QList<XBinary::MAPMODE> XSquashfs::getMapModesList()
{
    QList<MAPMODE> listResult;
    listResult.append(MAPMODE_REGIONS);
    return listResult;
}

XBinary::_MEMORY_MAP XSquashfs::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result = _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);

    return result;
}

QString XSquashfs::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XSQUASHFS_STRUCTID, sizeof(_TABLE_XSQUASHFS_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XSquashfs::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(dataHeadersOptions)
    Q_UNUSED(pPdStruct)

    QList<DATA_HEADER> listResult;
    // TODO: Implement data header extraction
    return listResult;
}

QList<XBinary::FPART> XSquashfs::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;
    qint64 nTotalSize = getSize();

    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = sizeof(SQUASHFS_HEADER);
        record.nVirtualAddress = -1;
        record.sName = tr("Header");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_REGION) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = sizeof(SQUASHFS_HEADER);
        record.nFileSize = nTotalSize - sizeof(SQUASHFS_HEADER);
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    return listResult;
}
