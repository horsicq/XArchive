/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xiso9660.h"

XBinary::XCONVERT _TABLE_XISO9660_STRUCTID[] = {{XISO9660::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                {XISO9660::STRUCTID_PVDESC, "PVDESC", QString("Primary Volume Descriptor")},
                                                {XISO9660::STRUCTID_DIR_RECORD, "DIR_RECORD", QString("Directory Record")}};

XISO9660::XISO9660(QIODevice *pDevice) : XArchive(pDevice)
{
}

XISO9660::~XISO9660()
{
}

bool XISO9660::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 0x8000) {  // At least PVD offset + size
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        // ISO 9660 Primary Volume Descriptor is typically at offset 0x8000 (32KB)
        // Check for "CD001" signature
        if (compareSignature(&memoryMap, "4344303031", 0x8001, pPdStruct)) {  // "CD001"
            bResult = true;
        }
    }

    return bResult;
}

bool XISO9660::isValid(QIODevice *pDevice)
{
    XISO9660 xiso(pDevice);
    return xiso.isValid();
}

XISO9660::ISO9660_PVDESC XISO9660::_readPrimaryVolumeDescriptor(qint64 nOffset)
{
    ISO9660_PVDESC desc = {};
    read_array(nOffset, (char *)&desc, sizeof(ISO9660_PVDESC));
    return desc;
}

qint32 XISO9660::_getLogicalBlockSize()
{
    ISO9660_PVDESC pvd = _readPrimaryVolumeDescriptor(0x8000);
    return qFromLittleEndian(pvd.nLogicalBlockSize);
}

qint64 XISO9660::_getPrimaryVolumeDescriptorOffset()
{
    // Primary Volume Descriptor is always at sector 16 (offset 0x8000)
    return 0x8000;
}

bool XISO9660::_isValidDescriptor(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    char szStandard[6] = {0};
    read_array(nOffset + 1, szStandard, 5);

    return QString::fromLatin1(szStandard, 5) == "CD001";
}

QString XISO9660::getFileFormatExt()
{
    return "iso";
}

QString XISO9660::getFileFormatExtsString()
{
    return "ISO 9660 (*.iso)";
}

QString XISO9660::getMIMEString()
{
    return "application/x-iso9660-image";
}

XBinary::FT XISO9660::getFileType()
{
    return FT_ISO9660;
}

QList<XBinary::MAPMODE> XISO9660::getMapModesList()
{
    QList<MAPMODE> listResult;
    listResult.append(MAPMODE_REGIONS);
    return listResult;
}

XBinary::_MEMORY_MAP XISO9660::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};
    result.fileType = getFileType();
    result = _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);

    return result;
}

QString XISO9660::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XISO9660_STRUCTID, sizeof(_TABLE_XISO9660_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XISO9660::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(dataHeadersOptions)
    Q_UNUSED(pPdStruct)

    QList<DATA_HEADER> listResult;
    // TODO: Implement data header extraction
    return listResult;
}

quint64 XISO9660::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // For ISO 9660, we return 1 as the base record count
    // In reality, we would need to parse the directory tree
    return 1;
}

QList<XArchive::RECORD> XISO9660::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<RECORD> listResult;

    ISO9660_PVDESC pvd = _readPrimaryVolumeDescriptor(_getPrimaryVolumeDescriptorOffset());

    RECORD record = {};
    record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
    record.nHeaderOffset = _getPrimaryVolumeDescriptorOffset();
    record.nHeaderSize = sizeof(ISO9660_PVDESC);
    record.spInfo.nUncompressedSize = getSize();
    record.spInfo.sRecordName = QString::fromLatin1(pvd.szVolumeId, 32).trimmed();
    record.nDataOffset = record.nHeaderSize;
    record.nDataSize = getSize() - record.nHeaderSize;

    listResult.append(record);

    return listResult;
}

QList<XBinary::FPART> XISO9660::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)
    Q_UNUSED(pPdStruct)

    QList<FPART> listResult;
    qint64 nTotalSize = getSize();

    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = _getPrimaryVolumeDescriptorOffset();
        record.nFileSize = sizeof(ISO9660_PVDESC);
        record.nVirtualAddress = -1;
        record.sName = tr("Primary Volume Descriptor");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_REGION) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = _getPrimaryVolumeDescriptorOffset() + sizeof(ISO9660_PVDESC);
        record.nFileSize = nTotalSize - (_getPrimaryVolumeDescriptorOffset() + sizeof(ISO9660_PVDESC));
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    return listResult;
}
