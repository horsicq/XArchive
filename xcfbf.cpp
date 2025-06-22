/* Copyright (c) 2025 hors<horsicq@gmail.com>
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
#include "xcfbf.h"

XBinary::XCONVERT _TABLE_CFBF_STRUCTID[] = {
    {XCFBF::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XCFBF::STRUCTID_StructuredStorageHeader, "StructuredStorageHeade", QString("StructuredStorageHeade")},
    {XCFBF::STRUCTID_CFBF_DIRECTORY_ENTRY, "CFBF_DIRECTORY_ENTRY", QString("CFBF_DIRECTORY_ENTRY")},
};

XCFBF::XCFBF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCFBF::~XCFBF()
{
}

bool XCFBF::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (getSize() >= 512) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        if (compareSignature(&memoryMap, "D0CF11E0A1B11AE100000000000000000000000000000000", 0, pPdStruct)) {
            bResult = true;
        }
    }

    return bResult;
}

bool XCFBF::isValid(QIODevice *pDevice)
{
    XCFBF xcfbf(pDevice);

    return xcfbf.isValid();
}

QString XCFBF::getArch()
{
    return "";  // TODO
}

QString XCFBF::getVersion()
{
    QString sResult;

    quint16 uMinorVersion = read_uint16(0x18);
    quint16 uDllVersion = read_uint16(0x1A);

    sResult = QString("%1.%2").arg(uDllVersion).arg(uMinorVersion);

    return sResult;
}

QString XCFBF::getFileFormatExt()
{
    return "";
}

QString XCFBF::getMIMEString()
{
    return "application/x-cfbf";
}

quint64 XCFBF::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    return 0;  // TODO
}

QString XCFBF::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_CFBF_STRUCTID, sizeof(_TABLE_CFBF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XCFBF::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
        _dataHeadersOptions.nID = STRUCTID_StructuredStorageHeader;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCFBF::structIDToString(dataHeadersOptions.nID));

            if (dataHeadersOptions.nID == STRUCTID_StructuredStorageHeader) {
                dataHeader.nSize = sizeof(StructuredStorageHeader);

                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _abSig), 8, "_abSig", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _uSectorShift), 2, "_uSectorShift", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _uMiniSectorShift), 2, "_uMiniSectorShift", VT_UINT16, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _usReserved), 2, "_usReserved", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _ulReserved1), 4, "_ulReserved1", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _csectFat), 4, "_csectFat", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _sectDirStart), 4, "_sectDirStart", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _signature), 4, "_signature", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _ulMiniSectorCutoff), 4, "_ulMiniSectorCutoff", VT_UINT32, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));

                dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _sectMiniFatStart), 4, "_sectMiniFatStart", VT_UINT32, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _csectMiniFat), 4, "_csectMiniFat", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _sectDifStart), 4, "_sectDifStart", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _csectDif), 4, "_csectDif", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(StructuredStorageHeader, _sectFat), 436, "_sectFat", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
            } else if (dataHeadersOptions.nID == STRUCTID_CFBF_DIRECTORY_ENTRY) {
                // TODO
            }

            if (dataHeader.nSize) {
                listResult.append(dataHeader);
            }

            if (dataHeadersOptions.bChildren && isPdStructNotCanceled(pPdStruct)) {
                if (dataHeadersOptions.nID == STRUCTID_StructuredStorageHeader) {
                }
            }
        }
    }

    return listResult;
}

QList<XArchive::RECORD> XCFBF::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<RECORD> listRecords;

    // CFBF header is 512 bytes
    const qint64 nHeaderSize = 512;
    if (getSize() < nHeaderSize) {
        return listRecords;
    }

    // Directory sector start (offset in sectors)
    quint32 nDirSectorStart = read_uint32(0x30, false);
    quint32 nSectorSize = 1 << read_uint16(0x1E, false);
    if (nSectorSize == 0) {
        nSectorSize = 512;  // fallback
    }

    // Directory entries are 128 bytes each
    const qint32 nDirEntrySize = 128;

    // Calculate first directory sector offset
    qint64 nDirOffset = nHeaderSize + (qint64)nDirSectorStart * nSectorSize;

    // Prevent infinite loop in case of corruption
    const qint32 nMaxEntries = 4096;
    qint32 nEntriesRead = 0;

    // Read directory entries
    while (isOffsetValid(nDirOffset + nDirEntrySize * nEntriesRead) && isPdStructNotCanceled(pPdStruct)) {
        if ((nLimit > 0) && (listRecords.size() >= nLimit)) {
            break;
        }
        if (nEntriesRead >= nMaxEntries) {
            break;
        }

        qint64 nEntryOffset = nDirOffset + nDirEntrySize * nEntriesRead;

        // Name (Unicode UTF-16LE, max 32 wchar = 64 bytes, null terminated)
        QByteArray baName = read_array(nEntryOffset, 64);
        quint16 nNameLength = read_uint16(nEntryOffset + 64, false);  // in bytes
        QString sName;
        if (nNameLength >= 2 && nNameLength <= 64) {
            sName = QString::fromUtf16((ushort *)baName.constData(), (nNameLength - 2) / 2);
        }

        quint8 nObjectType = read_uint8(nEntryOffset + 66);
        // 0 = unknown, 1 = storage, 2 = stream, 5 = root storage

        if (nObjectType == 0) {
            // Reached unused entries
            break;
        }

        quint32 nStartSector = read_uint32(nEntryOffset + 116, false);
        quint64 nStreamSize = read_uint64(nEntryOffset + 120, false);

        XArchive::RECORD record = {};
        record.spInfo.sRecordName = sName;
        record.spInfo.nUncompressedSize = nStreamSize;
        record.spInfo.compressMethod = COMPRESS_METHOD_STORE;
        record.nDataOffset = nHeaderSize + ((qint64)nStartSector * nSectorSize);
        record.nDataSize = nStreamSize;
        record.nHeaderOffset = nEntryOffset;
        record.nHeaderSize = nDirEntrySize;
        record.sUUID = "";  // CFBF entries don’t have a UUID

        listRecords.append(record);

        nEntriesRead++;
    }

    return listRecords;
}

XCFBF::StructuredStorageHeader XCFBF::read_StructuredStorageHeader(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    StructuredStorageHeader header = {};

    read_array(nOffset, (char *)&header, sizeof(header), pPdStruct);

    return header;
}

XBinary::_MEMORY_MAP XCFBF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    qint64 nTotalSize = getSize();

    _MEMORY_MAP result = {};

    result.nBinarySize = nTotalSize;
    result.fileType = getFileType();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = getTypeAsString();

    StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);

    qint32 nIndex = 0;
    qint32 nSectSize = 512;
    qint32 nMiniFatSectSize = 64;

    if (ssh._uSectorShift == 9) {
        nSectSize = 512;
    } else if (ssh._uSectorShift == 12) {
        nSectSize = 4096;
    }

    qint64 nCurrentOffset = 0;

    {
        _MEMORY_RECORD recordHeader = {};
        recordHeader.nAddress = -1;
        recordHeader.segment = ADDRESS_SEGMENT_FLAT;
        recordHeader.nOffset = nCurrentOffset;
        recordHeader.nSize = nSectSize;
        recordHeader.nIndex = nIndex++;
        recordHeader.type = MMT_HEADER;
        recordHeader.sName = tr("Header");

        result.listRecords.append(recordHeader);
    }

    nCurrentOffset += nSectSize;

    // quint32 nNumberOfSects = ssh._csectFat + ssh._csectDir + ssh._csectMiniFat + ssh._csectDif;

    // for (quint32 i = 0; i < nNumberOfSects; i++) {
    //     _MEMORY_RECORD recordHeader = {};
    //     recordHeader.nAddress = -1;
    //     recordHeader.segment = ADDRESS_SEGMENT_FLAT;
    //     recordHeader.nOffset = nCurrentOffset;
    //     recordHeader.nSize = nSectSize;
    //     recordHeader.nIndex = nIndex++;
    //     recordHeader.type = MMT_FILESEGMENT;
    //     recordHeader.sName = QString("%1 %2").arg(tr("Sector"), QString::number(i));

    //     result.listRecords.append(recordHeader);

    //     nCurrentOffset += nSectSize;
    // }

    return result;
}

XBinary::FT XCFBF::getFileType()
{
    return FT_CFBF;
}

XBinary::ENDIAN XCFBF::getEndian()
{
    return ENDIAN_LITTLE;
}

XBinary::MODE XCFBF::getMode()
{
    return MODE_32;
}
