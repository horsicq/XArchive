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
#include "xiso9660.h"
#include "Algos/xstoredecoder.h"

XBinary::XCONVERT _TABLE_XISO9660_STRUCTID[] = {{XISO9660::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
                                                {XISO9660::STRUCTID_PVDESC, "PVDESC", QString("Primary Volume Descriptor")},
                                                {XISO9660::STRUCTID_DIR_RECORD, "DIR_RECORD", QString("Directory Record")}};

XISO9660::XISO9660(QIODevice *pDevice) : XArchive(pDevice)
{
    if (isValid()) {
        qint64 nPVDOffset = _getPrimaryVolumeDescriptorOffset();
        m_sSystemIdentifier            = QString::fromLatin1(read_array(nPVDOffset + 8,   32)).trimmed();
        m_sVolumeIdentifier            = QString::fromLatin1(read_array(nPVDOffset + 40,  32)).trimmed();
        m_sVolumeSetIdentifier         = QString::fromLatin1(read_array(nPVDOffset + 190, 128)).trimmed();
        m_sPublisherIdentifier         = QString::fromLatin1(read_array(nPVDOffset + 318, 128)).trimmed();
        m_sDataPreparerIdentifier      = QString::fromLatin1(read_array(nPVDOffset + 446, 128)).trimmed();
        m_sApplicationIdentifier       = QString::fromLatin1(read_array(nPVDOffset + 574, 128)).trimmed();
        m_sCopyrightFileIdentifier     = QString::fromLatin1(read_array(nPVDOffset + 702, 37)).trimmed();
        m_sAbstractFileIdentifier      = QString::fromLatin1(read_array(nPVDOffset + 739, 36)).trimmed();
        m_sBibliographicFileIdentifier = QString::fromLatin1(read_array(nPVDOffset + 775, 37)).trimmed();
        m_sCreationDateTime            = QString::fromLatin1(read_array(nPVDOffset + 812, 17)).trimmed();
        m_sModificationDateTime        = QString::fromLatin1(read_array(nPVDOffset + 829, 17)).trimmed();
        m_sExpirationDateTime          = QString::fromLatin1(read_array(nPVDOffset + 846, 17)).trimmed();
        m_sEffectiveDateTime           = QString::fromLatin1(read_array(nPVDOffset + 863, 17)).trimmed();
    }
}

XISO9660::~XISO9660()
{
}

bool XISO9660::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() >= 0x8000) {  // At least PVD offset + size
        _MEMORY_MAP memoryMap = XBinary::getSimpleMemoryMap();

        // ISO 9660 Primary Volume Descriptor is typically at offset 0x8000 (32KB)
        // Check for "CD001" signature
        if (compareSignature(&memoryMap, "4344303031", 0x8001, pPdStruct)) {  // "CD001"
            bResult = true;
        }
    }

    return bResult;
}

bool XISO9660::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
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
    // Logical block size is at offset 128 (0x80) in the PVD (little-endian 16-bit value stored as 32-bit)
    quint16 nBlockSize = read_uint16(0x8000 + 128);
    return (qint32)nBlockSize;
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

qint64 XISO9660::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    qint64 nResult = 0;

    if (isValid()) {
        // Volume Space Size is at offset 80 in the Primary Volume Descriptor (little-endian)
        // It specifies the total number of logical blocks
        qint64 nPVDOffset = _getPrimaryVolumeDescriptorOffset();
        quint32 nVolumeSpaceSize = read_uint32(nPVDOffset + 80);
        qint32 nLogicalBlockSize = _getLogicalBlockSize();

        if (nLogicalBlockSize > 0 && nVolumeSpaceSize > 0) {
            nResult = (qint64)nVolumeSpaceSize * (qint64)nLogicalBlockSize;
        }
    }

    return nResult;
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

QString XISO9660::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XISO9660_STRUCTID, sizeof(_TABLE_XISO9660_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XISO9660::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XISO9660_STRUCTID, sizeof(_TABLE_XISO9660_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XISO9660::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        _dataHeadersOptions.nID = STRUCTID_PVDESC;
        _dataHeadersOptions.nLocation = _getPrimaryVolumeDescriptorOffset();
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_PVDESC) {
                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XISO9660::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = sizeof(ISO9660_PVDESC);

                dataHeader.listRecords.append(getDataRecord(0, 1, "Descriptor Type", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(1, 5, "Standard Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(6, 1, "Version", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(7, 1, "Unused", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(8, 32, "System Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(40, 32, "Volume Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(72, 8, "Unused", VT_BYTE_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(80, 4, "Volume Space Size (LE)", VT_UINT32, DRF_SIZE, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(84, 4, "Volume Space Size (BE)", VT_UINT32, DRF_SIZE, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(88, 32, "Unused", VT_BYTE_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(120, 2, "Volume Set Size (LE)", VT_UINT16, DRF_COUNT, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(122, 2, "Volume Set Size (BE)", VT_UINT16, DRF_COUNT, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(124, 2, "Volume Sequence Number (LE)", VT_UINT16, DRF_COUNT, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(126, 2, "Volume Sequence Number (BE)", VT_UINT16, DRF_COUNT, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(128, 2, "Logical Block Size (LE)", VT_UINT16, DRF_SIZE, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(130, 2, "Logical Block Size (BE)", VT_UINT16, DRF_SIZE, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(132, 4, "Path Table Size (LE)", VT_UINT32, DRF_SIZE, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(136, 4, "Path Table Size (BE)", VT_UINT32, DRF_SIZE, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(140, 4, "L Path Table Location", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(144, 4, "Optional L Path Table Location", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(148, 4, "M Path Table Location", VT_UINT32, DRF_UNKNOWN, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(152, 4, "Optional M Path Table Location", VT_UINT32, DRF_UNKNOWN, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(156, 34, "Root Directory Record", VT_BYTE_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(190, 128, "Volume Set Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(318, 128, "Publisher Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(446, 128, "Data Preparer Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(574, 128, "Application Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(702, 37, "Copyright File Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(739, 36, "Abstract File Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(775, 37, "Bibliographic File Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(812, 17, "Creation Date/Time", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(829, 17, "Modification Date/Time", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(846, 17, "Expiration Date/Time", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(863, 17, "Effective Date/Time", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(880, 1, "File Structure Version", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    // Add root directory record as a child structure
                    DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                    _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
                    _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
                    _dataHeadersOptions.nID = STRUCTID_DIR_RECORD;
                    _dataHeadersOptions.locType = LT_OFFSET;
                    _dataHeadersOptions.nLocation = nStartOffset + 156;  // Root directory record offset within PVD
                    _dataHeadersOptions.nSize = 34;

                    listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                }
            } else if (dataHeadersOptions.nID == STRUCTID_DIR_RECORD) {
                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XISO9660::structIDToString(dataHeadersOptions.nID));

                quint8 nRecordLength = read_uint8(nStartOffset);
                dataHeader.nSize = nRecordLength;

                dataHeader.listRecords.append(getDataRecord(0, 1, "Length of Directory Record", VT_UINT8, DRF_SIZE, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(1, 1, "Extended Attribute Record Length", VT_UINT8, DRF_SIZE, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(2, 4, "Location of Extent (LE)", VT_UINT32, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(6, 4, "Location of Extent (BE)", VT_UINT32, DRF_UNKNOWN, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(10, 4, "Data Length (LE)", VT_UINT32, DRF_SIZE, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(14, 4, "Data Length (BE)", VT_UINT32, DRF_SIZE, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(18, 1, "Years since 1900", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(19, 1, "Month", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(20, 1, "Day", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(21, 1, "Hour", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(22, 1, "Minute", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(23, 1, "Second", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(24, 1, "GMT Offset", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(25, 1, "File Flags", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(26, 1, "File Unit Size", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(27, 1, "Interleave Gap Size", VT_UINT8, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(28, 2, "Volume Sequence Number (LE)", VT_UINT16, DRF_UNKNOWN, ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(30, 2, "Volume Sequence Number (BE)", VT_UINT16, DRF_UNKNOWN, ENDIAN_BIG));
                dataHeader.listRecords.append(getDataRecord(32, 1, "Length of File Identifier", VT_UINT8, DRF_SIZE, ENDIAN_LITTLE));

                // File identifier follows at offset 33
                quint8 nFileIdLength = read_uint8(nStartOffset + 32);
                if (nFileIdLength > 0 && nRecordLength >= 33 + nFileIdLength) {
                    dataHeader.listRecords.append(getDataRecord(33, nFileIdLength, "File Identifier", VT_CHAR_ARRAY, DRF_UNKNOWN, ENDIAN_LITTLE));
                }

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XISO9660::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;
    qint64 nTotalSize = getSize();
    qint64 nFormatSize = getFileFormatSize(pPdStruct);

    if (nFileParts & FILEPART_REGION) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = _getPrimaryVolumeDescriptorOffset();
        record.nVirtualAddress = -1;
        record.sName = tr("Reserved");

        listResult.append(record);
    }

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
        qint64 nDataOffset = _getPrimaryVolumeDescriptorOffset() + sizeof(ISO9660_PVDESC);
        qint64 nDataSize = (nFormatSize > 0) ? (nFormatSize - nDataOffset) : (nTotalSize - nDataOffset);

        if (nDataSize > 0) {
            FPART record = {};
            record.filePart = FILEPART_REGION;
            record.nFileOffset = nDataOffset;
            record.nFileSize = nDataSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Data");

            listResult.append(record);
        }
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nFormatSize > 0 && nTotalSize > nFormatSize) {
            FPART record = {};
            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nFormatSize;
            record.nFileSize = nTotalSize - nFormatSize;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

QString XISO9660::_cleanFileName(const QString &sFileName)
{
    QString sResult = sFileName;

    // Remove version number (e.g., ";1" at the end)
    qint32 nSemicolon = sResult.indexOf(';');
    if (nSemicolon != -1) {
        sResult = sResult.left(nSemicolon);
    }

    return sResult;
}

QList<XBinary::ARCHIVERECORD> XISO9660::_parseDirectoryEntries(qint64 nOffset, qint64 nSize, qint32 nBlockSize, const QString &sParentPath, PDSTRUCT *pPdStruct)
{
    QList<ARCHIVERECORD> listResult;

    qint64 nFileSize = getSize();
    qint64 nEndOffset = nOffset + nSize;

    if (nEndOffset > nFileSize) {
        nEndOffset = nFileSize;
    }

    qint64 nCurrentOffset = nOffset;

    while ((nCurrentOffset < nEndOffset) && isPdStructNotCanceled(pPdStruct)) {
        // Align to logical block boundaries for padding detection
        qint64 nBlockStart = nCurrentOffset;
        qint64 nNextBlockStart = ((nBlockStart / nBlockSize) + 1) * (qint64)nBlockSize;
        qint64 nBlockEnd = qMin(nNextBlockStart, nEndOffset);

        while (nCurrentOffset < nBlockEnd && isPdStructNotCanceled(pPdStruct)) {
            quint8 nRecordLength = read_uint8(nCurrentOffset);

            if (nRecordLength == 0) {
                // Zero-padding to next logical block
                nCurrentOffset = nBlockEnd;
                break;
            }

            if (nRecordLength < 34 || nCurrentOffset + nRecordLength > nBlockEnd) {
                nCurrentOffset = nBlockEnd;
                break;
            }

            quint8 nExtAttrLength = read_uint8(nCurrentOffset + 1);
            quint32 nExtentLocation = read_uint32(nCurrentOffset + 2);
            quint32 nDataLength = read_uint32(nCurrentOffset + 10);
            quint8 nFileFlags = read_uint8(nCurrentOffset + 25);
            quint8 nFileNameLength = read_uint8(nCurrentOffset + 32);

            QString sFileName;

            if (nFileNameLength > 0 && nCurrentOffset + 33 + nFileNameLength <= nBlockEnd) {
                QByteArray baFileName = read_array(nCurrentOffset + 33, nFileNameLength);
                sFileName = QString::fromLatin1(baFileName);
            }

            // Skip "." (0x00) and ".." (0x01) entries
            if (nFileNameLength == 1 && (sFileName == "\x00" || sFileName == "\x01")) {
                nCurrentOffset += nRecordLength;
                continue;
            }

            ARCHIVERECORD record = {};
            record.nStreamOffset = (qint64)nExtentLocation * nBlockSize + (qint64)nExtAttrLength * nBlockSize;
            record.nStreamSize = nDataLength;

            QString sCleanName = _cleanFileName(sFileName);
            QString sFullPath;

            if (sParentPath.isEmpty()) {
                sFullPath = sCleanName;
            } else {
                sFullPath = sParentPath + "/" + sCleanName;
            }

            bool bIsFolder = (nFileFlags & 0x02) != 0;

            record.mapProperties[FPART_PROP_ORIGINALNAME] = sFullPath;
            record.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)nDataLength;
            record.mapProperties[FPART_PROP_COMPRESSEDSIZE] = (qint64)nDataLength;
            record.mapProperties[FPART_PROP_HANDLEMETHOD] = HANDLE_METHOD_STORE;
            record.mapProperties[FPART_PROP_ISFOLDER] = bIsFolder;

            if (bIsFolder) {
                record.mapProperties[FPART_PROP_STREAMOFFSET] = (qint64)nExtentLocation * nBlockSize;
                record.mapProperties[FPART_PROP_STREAMSIZE] = (qint64)nDataLength;
            }

            // Read recording date/time from directory record
            quint8 nYear = read_uint8(nCurrentOffset + 18);
            quint8 nMonth = read_uint8(nCurrentOffset + 19);
            quint8 nDay = read_uint8(nCurrentOffset + 20);
            quint8 nHour = read_uint8(nCurrentOffset + 21);
            quint8 nMinute = read_uint8(nCurrentOffset + 22);
            quint8 nSecond = read_uint8(nCurrentOffset + 23);

            if (nYear > 0 && nMonth >= 1 && nMonth <= 12 && nDay >= 1 && nDay <= 31) {
                QDateTime dt(QDate(1900 + nYear, nMonth, nDay), QTime(nHour, nMinute, nSecond));

                if (dt.isValid()) {
                    record.mapProperties[FPART_PROP_MTIME] = dt;
                }
            }

            listResult.append(record);
            nCurrentOffset += nRecordLength;
        }
    }

    return listResult;
}

QList<XBinary::ARCHIVERECORD> XISO9660::_collectAllRecords(qint64 nRootOffset, qint64 nRootSize, qint32 nBlockSize, PDSTRUCT *pPdStruct)
{
    QList<ARCHIVERECORD> listResult;

    // BFS: queue of (dirOffset, dirSize, parentPath)
    struct DirEntry {
        qint64 nOffset;
        qint64 nSize;
        QString sPath;
    };

    QList<DirEntry> listDirQueue;
    QSet<qint64> setProcessedBlocks;

    DirEntry rootEntry;
    rootEntry.nOffset = nRootOffset;
    rootEntry.nSize = nRootSize;
    rootEntry.sPath = QString();

    listDirQueue.append(rootEntry);
    setProcessedBlocks.insert(nRootOffset / nBlockSize);

    while (!listDirQueue.isEmpty() && isPdStructNotCanceled(pPdStruct)) {
        DirEntry dirInfo = listDirQueue.takeFirst();

        QList<ARCHIVERECORD> listDirRecords = _parseDirectoryEntries(dirInfo.nOffset, dirInfo.nSize, nBlockSize, dirInfo.sPath, pPdStruct);

        for (qint32 i = 0; i < listDirRecords.count() && isPdStructNotCanceled(pPdStruct); i++) {
            ARCHIVERECORD record = listDirRecords.at(i);
            listResult.append(record);

            // If it's a directory, enqueue it for processing
            if (record.mapProperties.value(FPART_PROP_ISFOLDER).toBool()) {
                qint64 nSubDirOffset = record.mapProperties.value(FPART_PROP_STREAMOFFSET).toLongLong();
                qint64 nSubDirSize = record.mapProperties.value(FPART_PROP_STREAMSIZE).toLongLong();
                qint64 nSubDirBlock = nSubDirOffset / nBlockSize;

                if (!setProcessedBlocks.contains(nSubDirBlock) && nSubDirOffset > 0 && nSubDirSize > 0) {
                    DirEntry subEntry;
                    subEntry.nOffset = nSubDirOffset;
                    subEntry.nSize = nSubDirSize;
                    subEntry.sPath = record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();

                    listDirQueue.append(subEntry);
                    setProcessedBlocks.insert(nSubDirBlock);
                }
            }
        }
    }

    return listResult;
}

bool XISO9660::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

    if (!pState) {
        return false;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = getSize();
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    ISO9660_UNPACK_CONTEXT *pContext = new ISO9660_UNPACK_CONTEXT;
    pContext->nLogicalBlockSize = _getLogicalBlockSize();

    if (pContext->nLogicalBlockSize < 512 || pContext->nLogicalBlockSize > 8192) {
        delete pContext;
        return false;
    }

    // Read root directory record from PVD at offset 156
    qint64 nRootRecordOffset = 0x8000 + 156;

    if (nRootRecordOffset + 34 > pState->nTotalSize) {
        delete pContext;
        return false;
    }

    quint8 nRootRecordLength = read_uint8(nRootRecordOffset);

    if (nRootRecordLength < 34) {
        delete pContext;
        return false;
    }

    quint32 nRootExtentLocation = read_uint32(nRootRecordOffset + 2);
    quint32 nRootDataLength = read_uint32(nRootRecordOffset + 10);

    qint64 nRootOffset = (qint64)nRootExtentLocation * pContext->nLogicalBlockSize;
    qint64 nRootSize = (qint64)nRootDataLength;

    if (nRootOffset <= 0 || nRootSize <= 0 || nRootOffset >= pState->nTotalSize) {
        delete pContext;
        return false;
    }

    // Build flat list of all records via BFS traversal
    pContext->listAllRecords = _collectAllRecords(nRootOffset, nRootSize, pContext->nLogicalBlockSize, pPdStruct);

    pState->nNumberOfRecords = pContext->listAllRecords.count();
    pState->pContext = pContext;

    return true;
}

XBinary::ARCHIVERECORD XISO9660::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    ARCHIVERECORD record = {};

    if (!pState || !pState->pContext) {
        return record;
    }

    ISO9660_UNPACK_CONTEXT *pContext = (ISO9660_UNPACK_CONTEXT *)pState->pContext;

    if (pState->nCurrentIndex >= 0 && pState->nCurrentIndex < pContext->listAllRecords.count()) {
        record = pContext->listAllRecords.at(pState->nCurrentIndex);
    }

    return record;
}

bool XISO9660::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pContext) {
        return false;
    }

    pState->nCurrentIndex++;

    ISO9660_UNPACK_CONTEXT *pContext = (ISO9660_UNPACK_CONTEXT *)pState->pContext;

    return (pState->nCurrentIndex < pContext->listAllRecords.count());
}

bool XISO9660::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if (pState->pContext) {
        ISO9660_UNPACK_CONTEXT *pContext = (ISO9660_UNPACK_CONTEXT *)pState->pContext;

        pContext->listAllRecords.clear();

        delete pContext;
        pState->pContext = nullptr;
    }

    return true;
}

QString XISO9660::getSystemIdentifier()
{
    return m_sSystemIdentifier;
}

QString XISO9660::getVolumeIdentifier()
{
    return m_sVolumeIdentifier;
}

QString XISO9660::getVolumeSetIdentifier()
{
    return m_sVolumeSetIdentifier;
}

QString XISO9660::getPublisherIdentifier()
{
    return m_sPublisherIdentifier;
}

QString XISO9660::getDataPreparerIdentifier()
{
    return m_sDataPreparerIdentifier;
}

QString XISO9660::getApplicationIdentifier()
{
    return m_sApplicationIdentifier;
}

QString XISO9660::getCopyrightFileIdentifier()
{
    return m_sCopyrightFileIdentifier;
}

QString XISO9660::getAbstractFileIdentifier()
{
    return m_sAbstractFileIdentifier;
}

QString XISO9660::getBibliographicFileIdentifier()
{
    return m_sBibliographicFileIdentifier;
}

QString XISO9660::getCreationDateTime()
{
    return m_sCreationDateTime;
}

QString XISO9660::getModificationDateTime()
{
    return m_sModificationDateTime;
}

QString XISO9660::getExpirationDateTime()
{
    return m_sExpirationDateTime;
}

QString XISO9660::getEffectiveDateTime()
{
    return m_sEffectiveDateTime;
}

QList<QString> XISO9660::getSearchSignatures()
{
    // ISO 9660 has no fixed magic bytes at offset 0; the "CD001" signature
    // appears at offset 0x8001 (sector 16), so no start-of-file search is possible.
    return QList<QString>();
}

XBinary *XISO9660::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XISO9660(pDevice);
}

