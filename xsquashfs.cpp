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

static XBinary::XCONVERT _TABLE_XSQUASHFS_STRUCTID[] = {{XSquashfs::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
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

bool XSquashfs::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
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

QString XSquashfs::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XSQUASHFS_STRUCTID, sizeof(_TABLE_XSQUASHFS_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XSquashfs::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XSQUASHFS_STRUCTID, sizeof(_TABLE_XSQUASHFS_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XSquashfs::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_SUPERBLOCK;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if ((nStructID == STRUCTID_HEADER) || (nStructID == STRUCTID_SUPERBLOCK)) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(nStructID);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(SQUASHFS_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, nStructID, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(nStructID), xfHeader.sParentTag);
        listResult.append(xfHeader);
    }

    return listResult;
}

QList<XBinary::XFRECORD> XSquashfs::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if ((nStructID == STRUCTID_HEADER) || (nStructID == STRUCTID_SUPERBLOCK)) {
        listResult.append({"nMagic", (qint32)offsetof(SQUASHFS_HEADER, nMagic), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"nInodesCount", (qint32)offsetof(SQUASHFS_HEADER, nInodesCount), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"nCreationTime", (qint32)offsetof(SQUASHFS_HEADER, nCreationTime), 4, XFRECORD_FLAG_UNIXTIME, VT_UINT32});
        listResult.append({"nBlockSize", (qint32)offsetof(SQUASHFS_HEADER, nBlockSize), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"nFragmentsCount", (qint32)offsetof(SQUASHFS_HEADER, nFragmentsCount), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"nCompressionType", (qint32)offsetof(SQUASHFS_HEADER, nCompressionType), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"nBlockLog", (qint32)offsetof(SQUASHFS_HEADER, nBlockLog), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"nFlags", (qint32)offsetof(SQUASHFS_HEADER, nFlags), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"nNoIds", (qint32)offsetof(SQUASHFS_HEADER, nNoIds), 2, XFRECORD_FLAG_COUNT, VT_UINT16});
        listResult.append({"nVersionMajor", (qint32)offsetof(SQUASHFS_HEADER, nVersionMajor), 4, XFRECORD_FLAG_VERSION_MAJOR, VT_UINT32});
        listResult.append({"nVersionMinor", (qint32)offsetof(SQUASHFS_HEADER, nVersionMinor), 4, XFRECORD_FLAG_VERSION_MINOR, VT_UINT32});
        listResult.append({"nRootInodeRef", (qint32)offsetof(SQUASHFS_HEADER, nRootInodeRef), 8, XFRECORD_FLAG_NONE, VT_UINT64});
        listResult.append({"nBytesUsed", (qint32)offsetof(SQUASHFS_HEADER, nBytesUsed), 8, XFRECORD_FLAG_SIZE, VT_UINT64});
        listResult.append({"nIdTableStart", (qint32)offsetof(SQUASHFS_HEADER, nIdTableStart), 8, XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nXattrTableStart", (qint32)offsetof(SQUASHFS_HEADER, nXattrTableStart), 8, XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nInodeTableStart", (qint32)offsetof(SQUASHFS_HEADER, nInodeTableStart), 8, XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nDirectoryTableStart", (qint32)offsetof(SQUASHFS_HEADER, nDirectoryTableStart), 8, XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nFragmentTableStart", (qint32)offsetof(SQUASHFS_HEADER, nFragmentTableStart), 8, XFRECORD_FLAG_OFFSET, VT_UINT64});
        listResult.append({"nExportTableStart", (qint32)offsetof(SQUASHFS_HEADER, nExportTableStart), 8, XFRECORD_FLAG_OFFSET, VT_UINT64});
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XSquashfs::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     Q_UNUSED(dataHeadersOptions)
//     Q_UNUSED(pPdStruct)

//     QList<DATA_HEADER> listResult;
//     // TODO: Implement data header extraction
//     return listResult;
// }

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

QList<QString> XSquashfs::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("'sqsh'");

    return listResult;
}

XBinary *XSquashfs::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSquashfs(pDevice);
}

bool XSquashfs::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = XArchive::handleInternalInfo(pPdStruct);
        static_cast<XArchive::INTERNAL_INFO &>(m_internalInfo) =
            *static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
    }

    return bResult;
}

void *XSquashfs::getInternalInfo(PDSTRUCT *pPdStruct)
{
    handleInternalInfo(pPdStruct);

    return &m_internalInfo;
}

void XSquashfs::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
