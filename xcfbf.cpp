/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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

#include <new>

#include <QtEndian>
#include <QPointer>
#include <QSet>
#include <algorithm>
#include <limits>
#include <memory>

XBinary::XCONVERT _TABLE_CFBF_STRUCTID[] = {
    {XCFBF::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XCFBF::STRUCTID_StructuredStorageHeader, "StructuredStorageHeader", QString("StructuredStorageHeader")},
    {XCFBF::STRUCTID_CFBF_DIRECTORY_ENTRY, "CFBF_DIRECTORY_ENTRY", QString("CFBF_DIRECTORY_ENTRY")},
    {XCFBF::STRUCTID_FAT, "FAT", QString("FAT")},
    {XCFBF::STRUCTID_DIFAT, "DIFAT", QString("DIFAT")},
    {XCFBF::STRUCTID_MINIFAT, "MINIFAT", QString("MINIFAT")},
};

XCFBF::XCFBF(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCFBF::~XCFBF()
{
}

bool XCFBF::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    bool bResult = false;

    const qint64 nSize = getSize();
    if (!guardedThis) return false;
    if (XBinary::isPdStructNotCanceled(pPdStruct) && (nSize >= 512)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!guardedThis) return false;

        const bool bSignature = compareSignature(
            &memoryMap,
            "D0CF11E0A1B11AE100000000000000000000000000000000",
            0, pPdStruct);
        if (!guardedThis) return false;
        if (bSignature) {
            StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);
            if (!guardedThis) return false;

            bResult = (ssh._uByteOrder == 0xFFFE) && (ssh._uMiniSectorShift == 6) && (ssh._usReserved == 0) && (ssh._ulReserved1 == 0) &&
                      (ssh._ulMiniSectorCutoff == 4096) &&
                      (((ssh._uDllVersion == 3) && (ssh._uSectorShift == 9) && (ssh._csectDir == 0)) || ((ssh._uDllVersion == 4) && (ssh._uSectorShift == 12)));
        }
    }

    return bResult;
}

bool XCFBF::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCFBF xcfbf(pDevice);

    return xcfbf.isValid(pPdStruct);
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
    return "ole";
}

QString XCFBF::getFileFormatExtsString()
{
    return "CFBF (*.ole;*.doc;*.xls;*.ppt;*.msi)";
}

qint64 XCFBF::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    // CFBF files are typically complete; return full file size
    return getSize();
}

QString XCFBF::getMIMEString()
{
    return "application/x-cfbf";
}

QString XCFBF::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_CFBF_STRUCTID, sizeof(_TABLE_CFBF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QString XCFBF::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_CFBF_STRUCTID, sizeof(_TABLE_CFBF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XCFBF::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_CFBF_STRUCTID, sizeof(_TABLE_CFBF_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::XFHEADER> XCFBF::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_StructuredStorageHeader;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_StructuredStorageHeader) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_StructuredStorageHeader);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(StructuredStorageHeader);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_StructuredStorageHeader, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_StructuredStorageHeader), xfHeader.sParentTag);
        listResult.append(xfHeader);

        if (xfStruct.bIsParent) {
            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.sParent = xfHeader.sTag;
            _xfStruct.nStructID = STRUCTID_CFBF_DIRECTORY_ENTRY;
            _xfStruct.xLoc = offsetToLoc(-1);
            _xfStruct.nCount = 0;
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));
        }
    } else if (nStructID == STRUCTID_CFBF_DIRECTORY_ENTRY) {
        StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);

        if (ssh._sectDirStart != 0xFFFFFFFF) {
            qint64 nSectorSize = (qint64)1 << ssh._uSectorShift;
            qint64 nFileSize = getSize();
            QList<quint32> listFAT = _readFAT(ssh, pPdStruct);

            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_CFBF_DIRECTORY_ENTRY);
            xfHeader.xfType = XFTYPE_TABLE;

            quint32 nSector = ssh._sectDirStart;
            qint32 nMaxChain = listFAT.size();
            qint32 nChainCount = 0;

            while ((nSector < (quint32)nMaxChain) && (nSector != 0xFFFFFFFE) && (nChainCount < nMaxChain) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                qint64 nSectorOffset = nSectorSize + (qint64)nSector * nSectorSize;

                for (qint64 nEntryOffset = 0; (nEntryOffset + 128) <= nSectorSize; nEntryOffset += 128) {
                    qint64 nRowOffset = nSectorOffset + nEntryOffset;

                    if ((nRowOffset + 128) > nFileSize) {
                        break;
                    }

                    quint8 nObjectType = read_uint8(nRowOffset + 66);

                    if (nObjectType != 0) {  // Skip unallocated entries
                        xfHeader.listRowLocations.append(nRowOffset);
                    }
                }

                nSector = listFAT.at(nSector);
                nChainCount++;
            }

            if (!xfHeader.listRowLocations.isEmpty()) {
                xfHeader.xLoc = offsetToLoc(xfHeader.listRowLocations.first());
                xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_CFBF_DIRECTORY_ENTRY, xfHeader.xLoc);
                xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_CFBF_DIRECTORY_ENTRY), xfHeader.sParentTag);
                listResult.append(xfHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XCFBF::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_StructuredStorageHeader) {
        listResult.append({"_abSig", (qint32)offsetof(StructuredStorageHeader, _abSig), 8, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"_clsid", (qint32)offsetof(StructuredStorageHeader, _clsid), 16, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"_uMinorVersion", (qint32)offsetof(StructuredStorageHeader, _uMinorVersion), 2, XFRECORD_FLAG_VERSION_MINOR, VT_UINT16});
        listResult.append({"_uDllVersion", (qint32)offsetof(StructuredStorageHeader, _uDllVersion), 2, XFRECORD_FLAG_VERSION_MAJOR, VT_UINT16});
        listResult.append({"_uByteOrder", (qint32)offsetof(StructuredStorageHeader, _uByteOrder), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"_uSectorShift", (qint32)offsetof(StructuredStorageHeader, _uSectorShift), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"_uMiniSectorShift", (qint32)offsetof(StructuredStorageHeader, _uMiniSectorShift), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"_usReserved", (qint32)offsetof(StructuredStorageHeader, _usReserved), 2, XFRECORD_FLAG_NONE, VT_UINT16});
        listResult.append({"_ulReserved1", (qint32)offsetof(StructuredStorageHeader, _ulReserved1), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"_csectDir", (qint32)offsetof(StructuredStorageHeader, _csectDir), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"_csectFat", (qint32)offsetof(StructuredStorageHeader, _csectFat), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"_sectDirStart", (qint32)offsetof(StructuredStorageHeader, _sectDirStart), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"_signature", (qint32)offsetof(StructuredStorageHeader, _signature), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"_ulMiniSectorCutoff", (qint32)offsetof(StructuredStorageHeader, _ulMiniSectorCutoff), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"_sectMiniFatStart", (qint32)offsetof(StructuredStorageHeader, _sectMiniFatStart), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"_csectMiniFat", (qint32)offsetof(StructuredStorageHeader, _csectMiniFat), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"_sectDifStart", (qint32)offsetof(StructuredStorageHeader, _sectDifStart), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"_csectDif", (qint32)offsetof(StructuredStorageHeader, _csectDif), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
    } else if (nStructID == STRUCTID_CFBF_DIRECTORY_ENTRY) {
        // On-disk layout (128 bytes); literal offsets: the in-memory struct has alignment padding
        listResult.append({"name", 0, 64, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"nameLength", 64, 2, XFRECORD_FLAG_SIZE, VT_UINT16});
        listResult.append({"objectType", 66, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"colorFlag", 67, 1, XFRECORD_FLAG_NONE, VT_UINT8});
        listResult.append({"leftSiblingID", 68, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"rightSiblingID", 72, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"childID", 76, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"clsid", 80, 16, XFRECORD_FLAG_NONE, VT_BYTE_ARRAY});
        listResult.append({"stateBits", 96, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"creationTime", 100, 8, XFRECORD_FLAG_FILETIME, VT_UINT64});
        listResult.append({"modifiedTime", 108, 8, XFRECORD_FLAG_FILETIME, VT_UINT64});
        listResult.append({"startSectorLocation", 116, 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"streamSize", 120, 8, XFRECORD_FLAG_SIZE, VT_UINT64});
    }

    return listResult;
}

// QList<XBinary::DATA_HEADER> XCFBF::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
// {
//     QList<XBinary::DATA_HEADER> listResult;

//     if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
//         DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
//         _dataHeadersOptions.bChildren = true;
//         _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
//         _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
//         _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
//         _dataHeadersOptions.nID = STRUCTID_StructuredStorageHeader;
//         _dataHeadersOptions.nLocation = 0;
//         _dataHeadersOptions.locType = XBinary::LT_OFFSET;

//         listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
//     } else {
//         qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

//         if (nStartOffset != -1) {
//             if (dataHeadersOptions.nID == STRUCTID_StructuredStorageHeader) {
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCFBF::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = sizeof(StructuredStorageHeader);

//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _abSig), 8, "_abSig", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _clsid), 16, "_clsid", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 // Note: _clsid is 16 bytes (GUID), _uSectorShift is 2 bytes, ensure correct sizes
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _uMinorVersion), 2, "_uMinorVersion", VT_UINT16, DRF_UNKNOWN,
//                     dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _uDllVersion), 2, "_uDllVersion", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _uByteOrder), 2, "_uByteOrder", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _uSectorShift), 2, "_uSectorShift", VT_UINT16, DRF_UNKNOWN,
//                     dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _uMiniSectorShift), 2, "_uMiniSectorShift", VT_UINT16, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _usReserved), 2, "_usReserved", VT_UINT16, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _ulReserved1), 4, "_ulReserved1", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _csectDir), 4, "_csectDir", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _csectFat), 4, "_csectFat", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _sectDirStart), 4, "_sectDirStart", VT_UINT32, DRF_UNKNOWN,
//                     dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _signature), 4, "_signature", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _ulMiniSectorCutoff), 4, "_ulMiniSectorCutoff", VT_UINT32, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));

//                 dataHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _sectMiniFatStart), 4, "_sectMiniFatStart", VT_UINT32, DRF_UNKNOWN,
//                                                             dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _csectMiniFat), 4, "_csectMiniFat", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _sectDifStart), 4, "_sectDifStart", VT_UINT32, DRF_UNKNOWN,
//                     dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _csectDif), 4, "_csectDif", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(
//                     getDataRecord(offsetof(StructuredStorageHeader, _sectFat), 436, "_sectFat", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);

//                 if (dataHeadersOptions.bChildren) {
//                     // Expand children: Directory chain, FAT sectors (first 109), MiniFAT, DIFAT
//                     StructuredStorageHeader ssh = read_StructuredStorageHeader(nStartOffset, pPdStruct);
//                     const qint64 sectorSize = (ssh._uSectorShift == 12) ? 4096 : 512;

//                     // 1) Directory chain (as a table of directory entries)
//                     if ((ssh._sectDirStart != 0xFFFFFFFF) && (ssh._csectDir)) {
//                         DATA_HEADERS_OPTIONS _dirOptions = dataHeadersOptions;
//                         _dirOptions.dsID_parent = dataHeader.dsID;
//                         _dirOptions.dhMode = XBinary::DHMODE_TABLE;
//                         _dirOptions.nID = STRUCTID_CFBF_DIRECTORY_ENTRY;
//                         _dirOptions.locType = XBinary::LT_OFFSET;
//                         _dirOptions.nLocation = sectorSize + (qint64)ssh._sectDirStart * sectorSize;
//                         _dirOptions.nSize = (qint64)ssh._csectDir * sectorSize;
//                         _dirOptions.nCount = (qint32)(_dirOptions.nSize / 128);
//                         listResult.append(getDataHeaders(_dirOptions, pPdStruct));
//                     }

//                     // // 2) FAT sectors: first 109 listed in header
//                     // {
//                     //     DATA_HEADERS_OPTIONS _fatList = dataHeadersOptions;
//                     //     _fatList.dsID_parent = dataHeader.dsID;
//                     //     _fatList.dhMode = XBinary::DHMODE_HEADER;
//                     //     _fatList.nID = STRUCTID_FAT;  // custom header name below
//                     //     _fatList.locType = XBinary::LT_OFFSET;
//                     //     _fatList.nLocation = nStartOffset + offsetof(StructuredStorageHeader, _sectFat);

//                     //     XBinary::DATA_HEADER fatHeader = _initDataHeader(_fatList, QString("FAT sectors (first 109)"));
//                     //     fatHeader.nSize = 109 * 4;

//                     //     for (int i = 0; i < 109; ++i) {
//                     //         QString sField = QString("FATSector[%1]").arg(i);
//                     //         fatHeader.listRecords.append(getDataRecord(offsetof(StructuredStorageHeader, _sectFat) + i * 4, 4, sField, VT_UINT32, DRF_UNKNOWN,
//                     //                                                    dataHeadersOptions.pMemoryMap->endian));
//                     //     }

//                     //     listResult.append(fatHeader);
//                     // }

//                     // 3) MiniFAT region (best-effort contiguous range)
//                     if ((ssh._sectMiniFatStart != 0xFFFFFFFF) && (ssh._csectMiniFat)) {
//                         qint64 miniFatOffset = sectorSize + (qint64)ssh._sectMiniFatStart * sectorSize;
//                         qint64 miniFatSize = (qint64)ssh._csectMiniFat * sectorSize;

//                         if (isOffsetValid(miniFatOffset) && miniFatSize > 0) {
//                             DATA_HEADERS_OPTIONS _miniFat = dataHeadersOptions;
//                             _miniFat.dsID_parent = dataHeader.dsID;
//                             _miniFat.dhMode = XBinary::DHMODE_HEADER;
//                             _miniFat.nID = STRUCTID_MINIFAT;
//                             _miniFat.locType = XBinary::LT_OFFSET;
//                             _miniFat.nLocation = miniFatOffset;
//                             _miniFat.nSize = miniFatSize;

//                             listResult.append(getDataHeaders(_miniFat, pPdStruct));
//                         }
//                     }

//                     // 4) DIFAT region (best-effort contiguous range)
//                     if ((ssh._sectDifStart != 0xFFFFFFFF) && (ssh._csectDif)) {
//                         qint64 difatOffset = sectorSize + (qint64)ssh._sectDifStart * sectorSize;
//                         qint64 difatSize = (qint64)ssh._csectDif * sectorSize;

//                         if (isOffsetValid(difatOffset) && difatSize > 0) {
//                             DATA_HEADERS_OPTIONS _difat = dataHeadersOptions;
//                             _difat.dsID_parent = dataHeader.dsID;
//                             _difat.dhMode = XBinary::DHMODE_HEADER;
//                             _difat.nID = STRUCTID_DIFAT;
//                             _difat.locType = XBinary::LT_OFFSET;
//                             _difat.nLocation = difatOffset;
//                             _difat.nSize = difatSize;

//                             listResult.append(getDataHeaders(_difat, pPdStruct));
//                         }
//                     }
//                 }
//             } else if (dataHeadersOptions.nID == STRUCTID_CFBF_DIRECTORY_ENTRY) {
//                 // Render a single CFBF directory entry (128 bytes)
//                 XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XCFBF::structIDToString(dataHeadersOptions.nID));

//                 dataHeader.nSize = 128;

//                 // Name (UTF-16LE, 32 WCHAR -> 64 bytes)
//                 dataHeader.listRecords.append(getDataRecord(0, 64, "Name", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(64, 2, "NameLength", VT_UINT16, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(66, 1, "ObjectType", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(67, 1, "ColorFlag", VT_UINT8, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(68, 4, "LeftSiblingID", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(72, 4, "RightSiblingID", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(76, 4, "ChildID", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(80, 16, "CLSID", VT_BYTE_ARRAY, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(96, 4, "StateBits", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(100, 8, "CreationTime", VT_UINT64, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(108, 8, "ModifiedTime", VT_UINT64, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(116, 4, "StartSectorLocation", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
//                 dataHeader.listRecords.append(getDataRecord(120, 8, "StreamSize", VT_UINT64, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));

//                 listResult.append(dataHeader);
//             }
//         }
//     }

//     return listResult;
// }

XCFBF::StructuredStorageHeader XCFBF::read_StructuredStorageHeader(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    StructuredStorageHeader header = {};

    read_array_process(nOffset, (char *)&header, sizeof(header), pPdStruct);

    return header;
}

XBinary::_MEMORY_MAP XCFBF::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;  // Default similar to other archives
    if (mapMode == MAPMODE_DATA) {
        return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_REGION | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_HEADER | MAPMODE_STREAMS | FILEPART_OVERLAY, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
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
    return MODE_DATA;
}

QList<XBinary::MAPMODE> XCFBF::getMapModesList()
{
    QList<MAPMODE> listResult;
    listResult.append(MAPMODE_DATA);
    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);
    return listResult;
}

qint64 XCFBF::getImageSize()
{
    // CFBF is not a memory image; use total file size
    return getSize();
}

static bool _cfbfCanAppend(XBinary::PDSTRUCT *pPdStruct, qint32 nLimit, const QList<XBinary::FPART> *pListResult)
{
    return XBinary::isPdStructNotCanceled(pPdStruct) && ((nLimit == -1) || (pListResult->count() < nLimit));
}

static bool _cfbfLimitReached(qint32 nLimit, const QList<XBinary::FPART> *pListResult)
{
    return (nLimit > 0) && (pListResult->count() >= nLimit);
}

static bool _cfbfGetSectorOffset(quint64 nSectorSize64, qint64 nFileSize, quint32 nSector, qint64 *pnOffset)
{
    if (!pnOffset || (nSector > (((std::numeric_limits<quint64>::max)() - nSectorSize64) / nSectorSize64))) {
        return false;
    }
    const quint64 nOffset64 = nSectorSize64 + (quint64)nSector * nSectorSize64;
    if ((nOffset64 > (quint64)(std::numeric_limits<qint64>::max)()) || (nOffset64 >= (quint64)nFileSize)) {
        return false;
    }
    *pnOffset = (qint64)nOffset64;
    return true;
}

static bool _cfbfGetSectorRange(quint64 nSectorSize64, qint64 nFileSize, quint32 nSector, quint64 nCount, qint64 *pnOffset, qint64 *pnSize)
{
    qint64 nOffset = -1;
    if (!pnOffset || !pnSize || (nCount == 0) || !_cfbfGetSectorOffset(nSectorSize64, nFileSize, nSector, &nOffset) ||
        (nCount > (quint64)(std::numeric_limits<qint64>::max)() / nSectorSize64)) {
        return false;
    }
    const qint64 nSize = (qint64)(nCount * nSectorSize64);
    if ((nSize <= 0) || (nSize > nFileSize - nOffset)) {
        return false;
    }
    *pnOffset = nOffset;
    *pnSize = nSize;
    return true;
}

QList<XBinary::FPART> XCFBF::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    const qint64 fileSize = getSize();
    if (fileSize < 512) return listResult;

    qint64 nMaxOffset = 0;

    StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);
    const qint64 sectorSize = (ssh._uSectorShift == 12) ? 4096 : 512;
    const quint64 nSectorSize64 = (quint64)sectorSize;

    if ((nFileParts & FILEPART_HEADER) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
        FPART header = {};
        header.filePart = FILEPART_HEADER;
        header.nFileOffset = 0;
        header.nFileSize = (std::min)(sectorSize, fileSize);
        header.nVirtualAddress = XADDR_MAX;
        header.sName = tr("Header");
        listResult.append(header);
        if (_cfbfLimitReached(nLimit, &listResult)) return listResult;
    }

    nMaxOffset = (std::max)(nMaxOffset, sectorSize);

    {
        // FAT sectors: first 109 in header
        if (ssh._csectFat) {
            // Approximate: first entries are in header _sectFat array; actual FAT sectors are scattered, but we can mark their total range best-effort
            // Derive first FAT sector offset from minimum non-FFFFFFFF entry
            quint32 firstFatSect = 0xFFFFFFFF;
            for (int i = 0; i < 109; ++i) {
                if (ssh._sectFat[i] != 0xFFFFFFFF) {
                    firstFatSect = (std::min)(firstFatSect, ssh._sectFat[i]);
                }
            }
            if (firstFatSect != 0xFFFFFFFF) {
                qint64 fatOffset = -1;
                qint64 totalFat = 0;
                if (_cfbfGetSectorRange(nSectorSize64, fileSize, firstFatSect, ssh._csectFat, &fatOffset, &totalFat)) {
                    if ((nFileParts & FILEPART_REGION) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
                        _addRegion(&listResult, fileSize, fatOffset, totalFat, QString("%1").arg("FAT"));
                        if (_cfbfLimitReached(nLimit, &listResult)) return listResult;
                    }
                    nMaxOffset = (std::max)(nMaxOffset, fatOffset + totalFat);
                }
            }
        }

        // MiniFAT
        if (ssh._sectMiniFatStart != 0xFFFFFFFF && ssh._csectMiniFat) {
            qint64 miniFatOffset = -1;
            qint64 miniFatSize = 0;
            if (_cfbfGetSectorRange(nSectorSize64, fileSize, ssh._sectMiniFatStart, ssh._csectMiniFat, &miniFatOffset, &miniFatSize)) {
                if ((nFileParts & FILEPART_REGION) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
                    _addRegion(&listResult, fileSize, miniFatOffset, miniFatSize, QString("%1").arg("MiniFAT"));
                    if (_cfbfLimitReached(nLimit, &listResult)) return listResult;
                }
                nMaxOffset = (std::max)(nMaxOffset, miniFatOffset + miniFatSize);
            }
        }

        // DIFAT
        if (ssh._sectDifStart != 0xFFFFFFFF && ssh._csectDif) {
            qint64 difatOffset = -1;
            qint64 difatSize = 0;
            if (_cfbfGetSectorRange(nSectorSize64, fileSize, ssh._sectDifStart, ssh._csectDif, &difatOffset, &difatSize)) {
                if ((nFileParts & FILEPART_REGION) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
                    _addRegion(&listResult, fileSize, difatOffset, difatSize, QString("%1").arg("DIFAT"));
                    if (_cfbfLimitReached(nLimit, &listResult)) return listResult;
                }
                nMaxOffset = (std::max)(nMaxOffset, difatOffset + difatSize);
            }
        }
    }

    {
        // Best-effort: parse directory entries directly and emit stream parts.
        // Note: CFBF streams may be fragmented; we map from the first sector linearly for UI purposes.
        // Mini streams are mapped relative to the Root Storage stream start.

        // Gather Root Storage info (needed for MiniStream base)
        qint64 dirBaseOffset = -1;
        qint64 dirTotalSize = 0;
        if (ssh._sectDirStart != 0xFFFFFFFF) {
            if (ssh._csectDir) {
                if (!_cfbfGetSectorRange(nSectorSize64, fileSize, ssh._sectDirStart, ssh._csectDir, &dirBaseOffset, &dirTotalSize)) {
                    dirBaseOffset = -1;
                }
            } else if (_cfbfGetSectorOffset(nSectorSize64, fileSize, ssh._sectDirStart, &dirBaseOffset)) {
                dirTotalSize = fileSize - dirBaseOffset;
            }
        }

        if ((dirBaseOffset != -1) && (dirTotalSize >= 128)) {
            const qint32 entrySize = 128;
            const qint32 maxEntries = (qint32)qMin<qint64>(dirTotalSize / entrySize, 16384);  // sanity cap

            // First pass: locate Root Storage (ObjectType 5)
            bool bHaveRoot = false;
            quint32 nRootStartSector = 0;
            quint64 nRootStreamSize = 0;
            qint64 nFixedSize = 0;
            for (qint32 i = 0; (i < maxEntries) && isPdStructNotCanceled(pPdStruct); i++) {
                qint64 entryOffset = dirBaseOffset + (qint64)i * entrySize;
                if (!isOffsetValid(entryOffset + entrySize - 1)) {
                    break;
                }
                nFixedSize = qMin<qint64>(dirTotalSize, ((qint64)i + 1) * entrySize);
                quint8 nObjectType = read_uint8(entryOffset + 66);
                if (nObjectType == 0) {
                    continue;  // unused slot
                }
                if (nObjectType == 5) {  // Root Storage
                    nRootStartSector = read_uint32(entryOffset + 116, false);
                    nRootStreamSize = read_uint64(entryOffset + 120, false);
                    bHaveRoot = true;
                }
            }

            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                listResult.clear();
                return listResult;
            }

            nMaxOffset = (std::max)(nMaxOffset, dirBaseOffset + nFixedSize);

            if ((nFileParts & FILEPART_REGION) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
                _addRegion(&listResult, fileSize, dirBaseOffset, nFixedSize, QString("%1").arg("Directory"));
                if (_cfbfLimitReached(nLimit, &listResult)) return listResult;
            }

            // Mini stream parameters
            const qint64 miniSectorSize = (ssh._uMiniSectorShift >= 4 && ssh._uMiniSectorShift <= 12) ? ((qint64)1 << ssh._uMiniSectorShift) : 64;
            const quint64 miniCutoff = ssh._ulMiniSectorCutoff ? ssh._ulMiniSectorCutoff : 4096;
            qint64 miniBaseOffset = -1;
            if (bHaveRoot) {
                _cfbfGetSectorOffset(nSectorSize64, fileSize, nRootStartSector, &miniBaseOffset);
            }

            // Second pass: emit streams
            for (qint32 i = 0; (i < maxEntries) && isPdStructNotCanceled(pPdStruct) && _cfbfCanAppend(pPdStruct, nLimit, &listResult); i++) {
                qint64 entryOffset = dirBaseOffset + (qint64)i * entrySize;
                if (!isOffsetValid(entryOffset + entrySize - 1)) {
                    break;
                }

                // Read object type
                quint8 nObjectType = read_uint8(entryOffset + 66);
                if (!((nObjectType == 2) || (nObjectType == 1) || (nObjectType == 5))) {
                    continue;  // Streams, User Storages, Root Storage
                }

                quint32 nStartSector = read_uint32(entryOffset + 116, false);
                quint64 nStreamSize = read_uint64(entryOffset + 120, false);

                // For Root Storage, treat its stream as the MiniStream itself
                if (nObjectType == 5) {
                    nStartSector = nRootStartSector;
                    nStreamSize = nRootStreamSize;
                }

                if ((nStreamSize == 0) || (nStartSector == 0xFFFFFFFF)) {
                    continue;  // skip empty
                }

                // Only non-root entries can be mini; Root stream is stored in regular sectors
                bool bIsMini = (nObjectType != 5) && (nStreamSize < miniCutoff) && (miniBaseOffset != -1);
                qint64 streamOffset = -1;
                if (bIsMini) {
                    const quint64 nRelativeOffset = (quint64)nStartSector * (quint64)miniSectorSize;
                    if (nRelativeOffset <= (quint64)(fileSize - miniBaseOffset)) {
                        streamOffset = miniBaseOffset + (qint64)nRelativeOffset;
                    }
                } else {
                    _cfbfGetSectorOffset(nSectorSize64, fileSize, nStartSector, &streamOffset);
                }

                if (!isOffsetValid(streamOffset)) {
                    continue;
                }

                if ((nStreamSize > (quint64)(std::numeric_limits<qint64>::max)()) ||
                    (nStreamSize > (quint64)(fileSize - streamOffset))) {
                    continue;
                }
                const qint64 clampedSize = (qint64)nStreamSize;

                if ((nFileParts & FILEPART_STREAM) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
                    // Read name
                    QByteArray baName = read_array(entryOffset + 0, 64);
                    quint16 nNameLength = read_uint16(entryOffset + 64, false);
                    QString sName;
                    if ((nNameLength >= 2) && (nNameLength <= 64)) {
                        sName = QString::fromUtf16(
                            reinterpret_cast<const char16_t *>(baName.constData()),
                            (nNameLength - 2) / 2);
                    }

                    FPART part = {};
                    part.filePart = FILEPART_STREAM;
                    part.nFileOffset = streamOffset;
                    part.nFileSize = clampedSize;
                    part.nVirtualAddress = XADDR_MAX;
                    part.sName = sName;
                    listResult.append(part);
                    if (_cfbfLimitReached(nLimit, &listResult)) return listResult;
                }

                nMaxOffset = (std::max)(nMaxOffset, streamOffset + clampedSize);

            }

            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                listResult.clear();
                return listResult;
            }
        }
    }

    if (nMaxOffset <= (std::numeric_limits<qint64>::max)() - (sectorSize - 1)) {
        nMaxOffset = align_up(nMaxOffset, sectorSize);
    } else {
        nMaxOffset = fileSize;
    }

    if ((nFileParts & FILEPART_DATA) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
        const qint64 nDataEnd = (std::min)(nMaxOffset, fileSize);
        if (nDataEnd > sectorSize) {
            FPART data = {};
            data.filePart = FILEPART_DATA;
            data.nFileOffset = sectorSize;
            data.nFileSize = nDataEnd - sectorSize;
            data.nVirtualAddress = XADDR_MAX;
            data.sName = tr("Data");
            listResult.append(data);
            if (_cfbfLimitReached(nLimit, &listResult)) return listResult;
        }
    }

    if ((nFileParts & FILEPART_OVERLAY) && _cfbfCanAppend(pPdStruct, nLimit, &listResult)) {
        if (nMaxOffset < fileSize) {
            FPART overlay = {};
            overlay.filePart = FILEPART_OVERLAY;
            overlay.nFileOffset = nMaxOffset;
            overlay.nFileSize = fileSize - nMaxOffset;
            overlay.nVirtualAddress = XADDR_MAX;
            overlay.sName = tr("Overlay");
            listResult.append(overlay);
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        listResult.clear();
    }

    return listResult;
}

void XCFBF::_addRegion(QList<FPART> *pListResult, qint64 fileSize, qint64 offset, qint64 size, const QString &name)
{
    if (!pListResult) return;
    if (offset < 0) return;
    if (size <= 0) return;
    if (offset >= fileSize) return;

    if (size > (fileSize - offset)) return;

    FPART part = {};
    part.filePart = FILEPART_REGION;
    part.nFileOffset = offset;
    part.nFileSize = size;
    part.nVirtualAddress = XADDR_MAX;
    part.sName = name;
    pListResult->append(part);
}

QMap<XBinary::UNPACK_PROP, QVariant> XCFBF::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

static bool _cfbfFailUnpackSource(QPointer<XCFBF> *pGuardedThis, XBinary::UNPACK_STATE *pState)
{
    if (pGuardedThis->isNull()) return false;
    (*pGuardedThis)->releaseUnpackSource(pState);
    return false;
}

template <typename T>
static bool _cfbfFailUnpackInit(QPointer<XCFBF> *pGuardedThis, XBinary::UNPACK_STATE *pState, T *pContext)
{
    if (pGuardedThis->isNull()) return false;
    (*pGuardedThis)->releaseUnpackSource(pState);
    *pState = XBinary::UNPACK_STATE();
    delete pContext;
    return false;
}

bool XCFBF::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();
    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || m_bUnpackOperationInProgress) {
        return false;
    }

    const bool bFinished = finishUnpack(pState, nullptr);
    if (!guardedThis || !bFinished) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    const bool bValid = isValid(pPdStruct);
    if (!guardedThis) return false;
    if (!bValid) return _cfbfFailUnpackSource(&guardedThis, pState);

    const qint64 nFileSize = getSize();
    if (!guardedThis) return false;
    const StructuredStorageHeader ssh = read_StructuredStorageHeader(0, pPdStruct);
    if (!guardedThis) return false;
    const qint64 nSectorSize = (ssh._uSectorShift == 12) ? 4096 : 512;
    const qint64 nMiniSectorSize = (qint64)1 << ssh._uMiniSectorShift;
    if ((nFileSize < nSectorSize) || (ssh._sectDirStart == 0xFFFFFFFF) || (ssh._sectDirStart == 0xFFFFFFFE)) {
        return _cfbfFailUnpackSource(&guardedThis, pState);
    }

    const quint64 nPhysicalSectors = (quint64)((nFileSize - nSectorSize) / nSectorSize);
    if ((quint64)ssh._sectDirStart >= nPhysicalSectors) {
        return _cfbfFailUnpackSource(&guardedThis, pState);
    }

    qint64 nDirectorySize = -1;
    if (ssh._uDllVersion == 4) {
        if ((ssh._csectDir == 0) || ((quint64)ssh._csectDir * (quint64)nSectorSize > (quint64)(std::numeric_limits<qint32>::max)())) {
            return _cfbfFailUnpackSource(&guardedThis, pState);
        }
        nDirectorySize = (qint64)ssh._csectDir * nSectorSize;
    }

    CFBF_UNPACK_CONTEXT *pContext = new (std::nothrow) CFBF_UNPACK_CONTEXT;
    if (!pContext) {
        return _cfbfFailUnpackSource(&guardedThis, pState);
    }
    pContext->nSectorSize = nSectorSize;
    pContext->nMiniSectorSize = nMiniSectorSize;
    pContext->nMiniCutoff = ssh._ulMiniSectorCutoff;
    pContext->nDllVersion = ssh._uDllVersion;
    pContext->nDirBaseOffset = 0;
    pContext->nRootStartSector = 0xFFFFFFFF;
    pContext->nRootStreamSize = 0;

    pState->pContext = pContext;
    if (!registerUnpackContextCleanup(
            pState, pContext,
            &deleteUnpackContext<CFBF_UNPACK_CONTEXT>)) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        delete pContext;
        return false;
    }

    const QList<quint32> listFAT = _readFAT(ssh, pPdStruct);
    if (!guardedThis) return false;
    pContext->listFAT = listFAT;
    if (pContext->listFAT.isEmpty()) {
        return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
    }

    QByteArray baDirData = _readStreamBySectorChain(pContext->listFAT, ssh._sectDirStart, nSectorSize, nDirectorySize, pPdStruct);
    if (!guardedThis) return false;
    if (baDirData.isEmpty() || (baDirData.size() % nSectorSize) || (baDirData.size() % 128)) {
        return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
    }

    // Reconstruct the exact physical directory chain.  Directory entry offsets
    // are later read from the original device, so no unresolved relative
    // offsets may survive this conversion.
    const qint32 nDirectorySectors = baDirData.size() / (qint32)nSectorSize;
    QList<qint64> listDirSectorOffsets;
    QSet<quint32> setDirSectors;
    quint32 nDirectorySector = ssh._sectDirStart;

    for (qint32 i = 0; i < nDirectorySectors; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || ((quint64)nDirectorySector >= nPhysicalSectors) ||
            (nDirectorySector >= (quint32)pContext->listFAT.size()) || setDirSectors.contains(nDirectorySector)) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }

        setDirSectors.insert(nDirectorySector);
        listDirSectorOffsets.append(nSectorSize + (qint64)nDirectorySector * nSectorSize);
        nDirectorySector = pContext->listFAT.at(nDirectorySector);
    }

    if (nDirectorySector != 0xFFFFFFFE) {
        return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
    }

    const qint32 nEntrySize = 128;
    const qint32 nMaxEntries = baDirData.size() / nEntrySize;
    qint32 nRootEntries = 0;

    for (qint32 i = 0; i < nMaxEntries; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }

        const qint64 nEntryLocalOffset = (qint64)i * nEntrySize;
        const quint8 *pEntry = reinterpret_cast<const quint8 *>(baDirData.constData()) + nEntryLocalOffset;
        const quint8 nObjectType = pEntry[66];

        if (nObjectType == 0) {
            continue;
        }
        if ((nObjectType != 1) && (nObjectType != 2) && (nObjectType != 5)) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }

        const quint16 nNameLength = qFromLittleEndian<quint16>(pEntry + 64);
        if ((nNameLength < 2) || (nNameLength > 64) || (nNameLength & 1) || (qFromLittleEndian<quint16>(pEntry + nNameLength - 2) != 0)) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }

        if (nObjectType == 5) {
            if ((i != 0) || (++nRootEntries != 1)) {
                return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
            }
            pContext->nRootStartSector = qFromLittleEndian<quint32>(pEntry + 116);
            pContext->nRootStreamSize =
                (ssh._uDllVersion == 3) ? (quint64)qFromLittleEndian<quint32>(pEntry + 120) : qFromLittleEndian<quint64>(pEntry + 120);
        } else if (nObjectType == 2) {
            const quint64 nStreamSize =
                (ssh._uDllVersion == 3) ? (quint64)qFromLittleEndian<quint32>(pEntry + 120) : qFromLittleEndian<quint64>(pEntry + 120);

            if (nStreamSize > (quint64)(std::numeric_limits<qint64>::max)()) {
                return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
            }

            const qint64 nSectorIndex = nEntryLocalOffset / nSectorSize;
            const qint64 nOffsetInSector = nEntryLocalOffset % nSectorSize;
            if ((nSectorIndex < 0) || (nSectorIndex >= listDirSectorOffsets.size())) {
                return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
            }
            pContext->listRecordOffsets.append(listDirSectorOffsets.at((qint32)nSectorIndex) + nOffsetInSector);
        }
    }

    if (nRootEntries != 1) {
        return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
    }

    // The MiniFAT declaration is all-or-nothing and its known-size chain must
    // be exact.
    if (ssh._csectMiniFat == 0) {
        if ((ssh._sectMiniFatStart != 0xFFFFFFFF) && (ssh._sectMiniFatStart != 0xFFFFFFFE)) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }
    } else {
        const quint64 nMiniFATSize64 = (quint64)ssh._csectMiniFat * (quint64)nSectorSize;
        if ((nMiniFATSize64 > (quint64)(std::numeric_limits<qint32>::max)()) || ((quint64)ssh._sectMiniFatStart >= nPhysicalSectors)) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }

        const qint64 nMiniFATSize = (qint64)nMiniFATSize64;
        QByteArray baMiniFAT = _readStreamBySectorChain(pContext->listFAT, ssh._sectMiniFatStart, nSectorSize, nMiniFATSize, pPdStruct);
        if (!guardedThis) return false;
        if (baMiniFAT.size() != nMiniFATSize) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }

        const uchar *pMiniFAT = reinterpret_cast<const uchar *>(baMiniFAT.constData());
        const qint32 nMiniFATEntries = baMiniFAT.size() / (qint32)sizeof(quint32);
        pContext->listMiniFAT.reserve(nMiniFATEntries);
        for (qint32 i = 0; i < nMiniFATEntries; i++) {
            pContext->listMiniFAT.append(qFromLittleEndian<quint32>(pMiniFAT + i * sizeof(quint32)));
        }
    }

    if (pContext->nRootStreamSize > 0) {
        if ((pContext->nRootStreamSize > (quint64)(std::numeric_limits<qint32>::max)()) ||
            ((quint64)pContext->nRootStartSector >= nPhysicalSectors)) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }
        const QByteArray baRootMiniStream = _readStreamBySectorChain(
            pContext->listFAT, pContext->nRootStartSector, nSectorSize,
            (qint64)pContext->nRootStreamSize, pPdStruct);
        if (!guardedThis) return false;
        pContext->baRootMiniStream = baRootMiniStream;
        if ((quint64)pContext->baRootMiniStream.size() != pContext->nRootStreamSize) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }
    } else if ((pContext->nRootStartSector != 0xFFFFFFFF) && (pContext->nRootStartSector != 0xFFFFFFFE)) {
        return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
    }

    for (qint64 nEntryOffset : pContext->listRecordOffsets) {
        const quint32 nStartSector = read_uint32(nEntryOffset + 116, false);
        if (!guardedThis) return false;
        quint64 nStreamSize = 0;
        if (ssh._uDllVersion == 3) {
            nStreamSize = read_uint32(nEntryOffset + 120, false);
        } else {
            nStreamSize = read_uint64(nEntryOffset + 120, false);
        }
        if (!guardedThis) return false;

        if (nStreamSize == 0) {
            if ((nStartSector != 0xFFFFFFFF) && (nStartSector != 0xFFFFFFFE)) {
                return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
            }
        } else if (nStreamSize < pContext->nMiniCutoff) {
            if ((nStartSector >= (quint32)pContext->listMiniFAT.size()) || pContext->baRootMiniStream.isEmpty()) {
                return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
            }
        } else if (((quint64)nStartSector >= nPhysicalSectors) || (nStartSector >= (quint32)pContext->listFAT.size())) {
            return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || pContext->listRecordOffsets.isEmpty()) {
        return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
    }
    pState->nNumberOfRecords = pContext->listRecordOffsets.size();
    pState->nCurrentOffset = pContext->listRecordOffsets.first();
    pState->nCurrentIndex = 0;
    pState->nTotalSize = nFileSize;
    pState->mapUnpackProperties = mapProperties;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        return _cfbfFailUnpackInit(&guardedThis, pState, pContext);
    }
    return true;
}

XBinary::ARCHIVERECORD XCFBF::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    XBinary::ARCHIVERECORD result = {};

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState ||
        !pState->pContext) {
        return result;
    }
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    CFBF_UNPACK_CONTEXT *pContext = (CFBF_UNPACK_CONTEXT *)pState->pContext;
    qint64 nEntryOffset = pState->nCurrentOffset;
    const qint64 nFileSize = getSize();
    if (!guardedThis || (nEntryOffset < pContext->nSectorSize) ||
        (nEntryOffset > (nFileSize - 128))) {
        return result;
    }

    // Read directory entry fields from the file
    const QByteArray baEntry = read_array(nEntryOffset, 128);
    if (!guardedThis || (baEntry.size() != 128)) return result;
    const uchar *pEntry =
        reinterpret_cast<const uchar *>(baEntry.constData());
    const QByteArray baName = baEntry.left(64);
    const quint16 nNameLength = qFromLittleEndian<quint16>(pEntry + 64);
    const quint8 nObjectType = pEntry[66];
    const quint32 nStartSector = qFromLittleEndian<quint32>(pEntry + 116);
    const quint64 nStreamSize = (pContext->nDllVersion == 3)
                                    ? (quint64)qFromLittleEndian<quint32>(pEntry + 120)
                                    : qFromLittleEndian<quint64>(pEntry + 120);
    const quint64 nCreationTime = qFromLittleEndian<quint64>(pEntry + 100);
    const quint64 nModifiedTime = qFromLittleEndian<quint64>(pEntry + 108);

    // Parse name (UTF-16LE)
    QString sName;
    if ((baName.size() != 64) || (nObjectType != 2) || (nNameLength < 2) || (nNameLength > 64) || (nNameLength & 1) ||
        (qFromLittleEndian<quint16>(pEntry + nNameLength - 2) != 0) ||
        (nStreamSize > (quint64)(std::numeric_limits<qint64>::max)())) {
        return result;
    }

    for (qint32 i = 0; i < (nNameLength - 2); i += 2) {
        sName.append(QChar((quint8)baName.at(i) | ((quint16)(quint8)baName.at(i + 1) << 8)));
    }

    // Determine if mini-stream
    bool bIsMini = (nStreamSize < pContext->nMiniCutoff) && (pContext->nRootStartSector != 0xFFFFFFFF) && (nObjectType != 5);

    // Fill ARCHIVERECORD
    result.nStreamOffset = nStartSector;  // Store sector ID for unpackCurrent
    result.nStreamSize = (qint64)nStreamSize;

    // Set properties
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)nStreamSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);

    const QByteArray baClsid = baEntry.mid(80, 16);
    if ((baClsid.size() == 16) && (baClsid != QByteArray(16, '\0'))) {
        result.mapProperties.insert(FPART_PROP_UUID, read_UUID(nEntryOffset + 80));
    }

    // Store mini-stream flag as type property
    if (bIsMini) {
        result.mapProperties.insert(FPART_PROP_TYPE, QString("MiniStream"));
    } else {
        result.mapProperties.insert(FPART_PROP_TYPE, QString("Stream"));
    }

    // Convert FILETIME values to QDateTime (if non-zero).
    if (nCreationTime != 0) {
        const QDateTime dateTime = winFileTimeToQDateTime(nCreationTime);
        if (dateTime.isValid()) {
            result.mapProperties.insert(FPART_PROP_CTIME, dateTime);
        }
    }

    if (nModifiedTime != 0) {
        const QDateTime dateTime = winFileTimeToQDateTime(nModifiedTime);
        if (dateTime.isValid()) {
            result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
        }
    }

    const bool bFinalSourceCurrent =
        isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bFinalSourceCurrent) {
        return XBinary::ARCHIVERECORD();
    }
    return result;
}

static bool _cfbfStageWriteAll(QPointer<QIODevice> *pGuardedStage, qint64 *pnStaged, const char *pData, qint64 nSize)
{
    qint64 nWritten = 0;
    while (nWritten < nSize) {
        if (pGuardedStage->isNull() || !(*pGuardedStage)->seek(*pnStaged + nWritten)) return false;
        qint64 nResult = (*pGuardedStage)->write(
            pData + nWritten, nSize - nWritten);
        if (pGuardedStage->isNull() || nResult <= 0 || nResult > (nSize - nWritten)) {
            return false;
        }
        nWritten += nResult;
    }
    *pnStaged += nWritten;
    return true;
}

bool XCFBF::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext || !guardedOutput ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bOutputSupported =
        isUnpackOutputSupported(guardedOutput.data());
    if (!guardedThis || !guardedOutput || !bOutputSupported) return false;

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;
    CFBF_UNPACK_CONTEXT *pContext = static_cast<CFBF_UNPACK_CONTEXT *>(pState->pContext);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || !guardedOutput) return false;
    const bool bAliases =
        XBinary::devicesAlias(guardedSource.data(), guardedOutput.data());
    if (!guardedThis || !guardedSource || !guardedOutput || bAliases) {
        return false;
    }
    const qint64 nEntryOffset = pState->nCurrentOffset;
    const QByteArray baStreamEntry = read_array(nEntryOffset + 116, 12);
    if (!guardedThis || (baStreamEntry.size() != 12)) return false;
    const uchar *pStreamEntry =
        reinterpret_cast<const uchar *>(baStreamEntry.constData());
    const quint32 nStartSector =
        qFromLittleEndian<quint32>(pStreamEntry);
    const quint64 nStreamSize = (pContext->nDllVersion == 3)
                                    ? (quint64)qFromLittleEndian<quint32>(pStreamEntry + 4)
                                    : qFromLittleEndian<quint64>(pStreamEntry + 4);
    const bool bIsMini = (nStreamSize < pContext->nMiniCutoff) && (pContext->nRootStartSector != 0xFFFFFFFF);

    if ((nStreamSize > (quint64)(std::numeric_limits<qint64>::max)()) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            (qint64)nStreamSize)) {
        return false;
    }

    // This override bypasses the base decode chain: the sector loops below
    // produce the bytes themselves into a private stage, so account the
    // member and its exact size here (publishUnpackOutput never debits).
    if (pState->spOutputBudget) {
        const QString sRecordName = read_unicodeString(nEntryOffset, 32);
        if (!guardedThis) return false;
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                sRecordName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit((qint64)nStreamSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(
        (qint64)nStreamSize, pPdStruct));
    if (!guardedThis || !pStage || !guardedOutput || !guardedSource)
        return false;
    const bool bStageSourceCurrent =
        isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bStageSourceCurrent) return false;
    QPointer<QIODevice> guardedStage(pStage.get());
    qint64 nStaged = 0;

    if (nStreamSize == 0) {
        const bool bEmptySourceCurrent =
            isUnpackSourceCurrent(pState, pPdStruct);
        if (!guardedThis || !bEmptySourceCurrent || !guardedOutput)
            return false;
        const bool bPublished = publishUnpackOutput(
            pStage.get(), guardedOutput.data(), pState, pPdStruct);
        return guardedThis && bPublished;
    }

    quint32 nCurrentSector = nStartSector;
    quint64 nBytesRemaining = nStreamSize;
    QSet<quint32> setVisited;

    if (bIsMini) {
        const qint32 nMaxChain = pContext->listMiniFAT.size();
        const quint64 nRequiredSectors = (nStreamSize + (quint64)pContext->nMiniSectorSize - 1) / (quint64)pContext->nMiniSectorSize;
        if (nRequiredSectors > (quint64)nMaxChain) {
            return false;
        }

        while ((nBytesRemaining > 0) && (nCurrentSector < (quint32)nMaxChain) && (nCurrentSector != 0xFFFFFFFE) && (nCurrentSector != 0xFFFFFFFF) &&
               XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (setVisited.contains(nCurrentSector)) {
                return false;
            }
            setVisited.insert(nCurrentSector);

            const quint64 nOffset = (quint64)nCurrentSector * (quint64)pContext->nMiniSectorSize;
            if (nOffset >= (quint64)pContext->baRootMiniStream.size()) {
                return false;
            }

            const qint64 nChunkSize = (qint64)(std::min)((quint64)pContext->nMiniSectorSize, nBytesRemaining);
            if ((nOffset + (quint64)nChunkSize) > (quint64)pContext->baRootMiniStream.size() ||
                !_cfbfStageWriteAll(&guardedStage, &nStaged, pContext->baRootMiniStream.constData() + (qint64)nOffset, nChunkSize)) {
                return false;
            }

            nBytesRemaining -= (quint64)nChunkSize;
            nCurrentSector = pContext->listMiniFAT.at(nCurrentSector);
        }
    } else {
        const qint32 nMaxChain = pContext->listFAT.size();
        const qint64 nFileSize = getSize();
        if (!guardedThis) return false;
        const quint64 nRequiredSectors = (nStreamSize + (quint64)pContext->nSectorSize - 1) / (quint64)pContext->nSectorSize;
        if (nRequiredSectors > (quint64)nMaxChain) {
            return false;
        }

        while ((nBytesRemaining > 0) && (nCurrentSector < (quint32)nMaxChain) && (nCurrentSector != 0xFFFFFFFE) && (nCurrentSector != 0xFFFFFFFF) &&
               XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (setVisited.contains(nCurrentSector)) {
                return false;
            }
            setVisited.insert(nCurrentSector);

            const quint64 nOffset = (quint64)pContext->nSectorSize + (quint64)nCurrentSector * (quint64)pContext->nSectorSize;
            const qint64 nChunkSize = (qint64)(std::min)((quint64)pContext->nSectorSize, nBytesRemaining);

            if ((nOffset > (quint64)nFileSize) || ((quint64)nChunkSize > ((quint64)nFileSize - nOffset))) {
                return false;
            }

            QByteArray baChunk = read_array((qint64)nOffset, nChunkSize);
            if (!guardedThis || (baChunk.size() != nChunkSize) ||
                !_cfbfStageWriteAll(&guardedStage, &nStaged, baChunk.constData(), nChunkSize)) {
                return false;
            }

            nBytesRemaining -= (quint64)nChunkSize;
            nCurrentSector = pContext->listFAT.at(nCurrentSector);
        }
    }

    if ((nBytesRemaining != 0) || (nCurrentSector != 0xFFFFFFFE) ||
        (nStaged != (qint64)nStreamSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const bool bFinalSourceCurrent =
        isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bFinalSourceCurrent || !guardedOutput)
        return false;
    const bool bPublished = publishUnpackOutput(
        pStage.get(), guardedOutput.data(), pState, pPdStruct);
    return guardedThis && bPublished;
}

bool XCFBF::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    bool bResult = false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CFBF_UNPACK_CONTEXT *pContext = (CFBF_UNPACK_CONTEXT *)pState->pContext;
    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;

    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listRecordOffsets.at(pState->nCurrentIndex);
        bResult = true;
    }

    return bResult;
}

bool XCFBF::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) return false;

    CFBF_UNPACK_CONTEXT *pContext =
        static_cast<CFBF_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    delete pContext;
    Q_UNUSED(guardedThis)
    return true;
}

static bool _cfbfIsPhysicalSector(quint64 nPhysicalSectors, quint32 nSector)
{
    return (quint64)nSector < nPhysicalSectors;
}

static bool _cfbfAppendFATSector(quint64 nPhysicalSectors, quint32 nFATCount, QSet<quint32> *pSetFATSectors, const QSet<quint32> *pSetDIFATSectors,
                                 QList<quint32> *pListFATSectors, quint32 nSector)
{
    if (!_cfbfIsPhysicalSector(nPhysicalSectors, nSector) || pSetFATSectors->contains(nSector) || pSetDIFATSectors->contains(nSector) ||
        (pListFATSectors->size() >= (qint32)nFATCount)) {
        return false;
    }
    pSetFATSectors->insert(nSector);
    pListFATSectors->append(nSector);
    return true;
}

QList<quint32> XCFBF::_readFAT(const StructuredStorageHeader &ssh, PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    QList<quint32> listResult;

    const qint64 nSectorSize = (ssh._uSectorShift == 12) ? 4096 : 512;
    const qint32 nEntriesPerSector = (qint32)(nSectorSize / 4);
    const qint32 nDIFATEntriesPerSector = nEntriesPerSector - 1;
    const qint64 nFileSize = getSize();
    if (!guardedThis) return listResult;

    if ((nFileSize < nSectorSize) || (ssh._csectFat == 0)) {
        return listResult;
    }

    const quint64 nPhysicalSectors = (quint64)((nFileSize - nSectorSize) / nSectorSize);
    if ((ssh._csectFat > nPhysicalSectors) ||
        ((quint64)ssh._csectFat * (quint64)nEntriesPerSector > (quint64)(std::numeric_limits<qint32>::max)()) ||
        ((quint64)ssh._csectFat * (quint64)nEntriesPerSector < nPhysicalSectors)) {
        return listResult;
    }

    const quint32 nRequiredDIFATSectors =
        (ssh._csectFat > 109) ? (quint32)(((quint64)ssh._csectFat - 109 + (quint64)nDIFATEntriesPerSector - 1) / (quint64)nDIFATEntriesPerSector) : 0;
    if (ssh._csectDif != nRequiredDIFATSectors) {
        return listResult;
    }

    QList<quint32> listFATSectors;
    QSet<quint32> setFATSectors;
    QSet<quint32> setDIFATSectors;

    // The header DIFAT supplies up to 109 FAT-sector IDs.
    for (qint32 i = 0; i < 109; i++) {
        quint32 nSector = ssh._sectFat[i];
        if (nSector == 0xFFFFFFFF) {
            continue;
        }
        if ((nSector == 0xFFFFFFFE) || (listFATSectors.size() >= (qint32)ssh._csectFat) || !_cfbfAppendFATSector(nPhysicalSectors, ssh._csectFat, &setFATSectors, &setDIFATSectors, &listFATSectors, nSector)) {
            return QList<quint32>();
        }
    }

    // Additional FAT-sector IDs are stored in the declared DIFAT chain.
    if (ssh._csectDif == 0) {
        if ((ssh._sectDifStart != 0xFFFFFFFE) && (ssh._sectDifStart != 0xFFFFFFFF)) {
            return QList<quint32>();
        }
    } else {
        if ((ssh._csectDif > nPhysicalSectors) || !_cfbfIsPhysicalSector(nPhysicalSectors, ssh._sectDifStart)) {
            return QList<quint32>();
        }

        quint32 nDIFATSector = ssh._sectDifStart;
        for (quint32 i = 0; i < ssh._csectDif; i++) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || !_cfbfIsPhysicalSector(nPhysicalSectors, nDIFATSector) || setDIFATSectors.contains(nDIFATSector) ||
                setFATSectors.contains(nDIFATSector)) {
                return QList<quint32>();
            }
            setDIFATSectors.insert(nDIFATSector);

            const qint64 nOffset = nSectorSize + (qint64)nDIFATSector * nSectorSize;
            QByteArray baSector = read_array(nOffset, nSectorSize);
            if (!guardedThis || (baSector.size() != nSectorSize)) {
                return QList<quint32>();
            }

            const uchar *pEntries = reinterpret_cast<const uchar *>(baSector.constData());
            for (qint32 j = 0; j < nDIFATEntriesPerSector; j++) {
                quint32 nSector = qFromLittleEndian<quint32>(pEntries + j * sizeof(quint32));
                if (nSector == 0xFFFFFFFF) {
                    continue;
                }
                if ((nSector == 0xFFFFFFFE) || (listFATSectors.size() >= (qint32)ssh._csectFat) || !_cfbfAppendFATSector(nPhysicalSectors, ssh._csectFat, &setFATSectors, &setDIFATSectors, &listFATSectors, nSector)) {
                    return QList<quint32>();
                }
            }

            quint32 nNextDIFAT = qFromLittleEndian<quint32>(pEntries + nDIFATEntriesPerSector * sizeof(quint32));
            if ((i + 1) == ssh._csectDif) {
                if (nNextDIFAT != 0xFFFFFFFE) {
                    return QList<quint32>();
                }
            } else {
                if (!_cfbfIsPhysicalSector(nPhysicalSectors, nNextDIFAT) || setDIFATSectors.contains(nNextDIFAT) || setFATSectors.contains(nNextDIFAT)) {
                    return QList<quint32>();
                }
                nDIFATSector = nNextDIFAT;
            }
        }
    }

    if ((listFATSectors.size() != (qint32)ssh._csectFat) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return QList<quint32>();
    }

    listResult.reserve((qint32)ssh._csectFat * nEntriesPerSector);
    for (quint32 nFATSector : listFATSectors) {
        const qint64 nOffset = nSectorSize + (qint64)nFATSector * nSectorSize;
        QByteArray baSector = read_array(nOffset, nSectorSize);
        if (!guardedThis || (baSector.size() != nSectorSize) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return QList<quint32>();
        }

        const uchar *pEntries = reinterpret_cast<const uchar *>(baSector.constData());
        for (qint32 i = 0; i < nEntriesPerSector; i++) {
            listResult.append(qFromLittleEndian<quint32>(pEntries + i * sizeof(quint32)));
        }
    }

    // Allocation-table sectors must identify themselves with their reserved
    // markers. This also rejects a DIFAT/FAT alias hidden in otherwise valid
    // chains.
    for (quint32 nSector : listFATSectors) {
        if ((nSector >= (quint32)listResult.size()) || (listResult.at(nSector) != 0xFFFFFFFD)) {
            return QList<quint32>();
        }
    }
    for (quint32 nSector : setDIFATSectors) {
        if ((nSector >= (quint32)listResult.size()) || (listResult.at(nSector) != 0xFFFFFFFC)) {
            return QList<quint32>();
        }
    }

    for (quint64 i = 0; i < (quint64)listResult.size(); i++) {
        quint32 nValue = listResult.at((qint32)i);
        if (i >= nPhysicalSectors) {
            if (nValue != 0xFFFFFFFF) {
                return QList<quint32>();
            }
        } else if ((nValue >= nPhysicalSectors) && (nValue != 0xFFFFFFFF) && (nValue != 0xFFFFFFFE) && (nValue != 0xFFFFFFFD) &&
                   (nValue != 0xFFFFFFFC)) {
            return QList<quint32>();
        }
    }

    return listResult;
}

QByteArray XCFBF::_readStreamBySectorChain(const QList<quint32> &listFAT, quint32 nStartSector, qint64 nSectorSize, qint64 nStreamSize, PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    QByteArray baResult;

    const qint64 nFileSize = getSize();
    if (!guardedThis) return baResult;
    const bool bKnownSize = nStreamSize >= 0;
    if (((nSectorSize != 512) && (nSectorSize != 4096)) || (nFileSize < nSectorSize) || listFAT.isEmpty() || (nStreamSize < -1) ||
        (bKnownSize && (nStreamSize > (qint64)(std::numeric_limits<qint32>::max)()))) {
        return baResult;
    }

    if (bKnownSize && (nStreamSize == 0)) {
        return baResult;
    }

    const quint64 nPhysicalSectors = (quint64)((nFileSize - nSectorSize) / nSectorSize);
    const quint64 nMaximumChain = (std::min)((quint64)listFAT.size(), nPhysicalSectors);
    if ((nMaximumChain == 0) || ((quint64)nStartSector >= nMaximumChain)) {
        return QByteArray();
    }

    if (bKnownSize) {
        const quint64 nRequiredSectors = ((quint64)nStreamSize + (quint64)nSectorSize - 1) / (quint64)nSectorSize;
        if (nRequiredSectors > nMaximumChain) {
            return QByteArray();
        }
        baResult.reserve((qint32)nStreamSize);
    }

    quint32 nCurrentSector = nStartSector;
    qint64 nBytesRemaining = bKnownSize ? nStreamSize : -1;
    QSet<quint32> setVisited;

    while (nCurrentSector != 0xFFFFFFFE) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || ((quint64)nCurrentSector >= nMaximumChain) || setVisited.contains(nCurrentSector)) {
            return QByteArray();
        }
        setVisited.insert(nCurrentSector);

        const qint64 nOffset = nSectorSize + (qint64)nCurrentSector * nSectorSize;
        QByteArray baSector = read_array(nOffset, nSectorSize);
        if (!guardedThis || (baSector.size() != nSectorSize)) {
            return QByteArray();
        }

        const qint64 nReadSize = bKnownSize ? (std::min)(nSectorSize, nBytesRemaining) : nSectorSize;
        if ((nReadSize <= 0) || (baResult.size() > (std::numeric_limits<qint32>::max)() - nReadSize)) {
            return QByteArray();
        }
        baResult.append(baSector.constData(), (qint32)nReadSize);

        quint32 nNextSector = listFAT.at(nCurrentSector);
        if (bKnownSize) {
            nBytesRemaining -= nReadSize;
            if (nBytesRemaining == 0) {
                return (nNextSector == 0xFFFFFFFE) ? baResult : QByteArray();
            }
        }

        nCurrentSector = nNextSector;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (bKnownSize && (nBytesRemaining != 0))) {
        return QByteArray();
    }

    return baResult;
}

QList<QString> XCFBF::getSearchSignatures()
{
    QList<QString> listResult;

    listResult.append("D0CF11E0A1B11AE100000000000000000000000000000000");

    return listResult;
}

XBinary *XCFBF::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XCFBF(pDevice);
}

bool XCFBF::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XCFBF::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XCFBF> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XCFBF::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
