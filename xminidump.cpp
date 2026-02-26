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
#include "xminidump.h"
#include "Algos/xstoredecoder.h"

XBinary::XCONVERT _TABLE_XMINIDUMP_STRUCTID[] = {
    {XMiniDump::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},         {XMiniDump::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XMiniDump::STRUCTID_DIRECTORY, "DIRECTORY", QString("Directory")},       {XMiniDump::STRUCTID_STREAM, "STREAM", QString("Stream")},
    {XMiniDump::STRUCTID_MODULE_LIST, "MODULE_LIST", QString("Module List")}, {XMiniDump::STRUCTID_MODULE, "MODULE", QString("Module")},
};

XMiniDump::XMiniDump(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMiniDump::~XMiniDump()
{
}

bool XMiniDump::isValid(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    qint64 nFileSize = getSize();

    if (nFileSize >= (qint64)sizeof(MINIDUMP_HEADER)) {
        MINIDUMP_HEADER header = read_MINIDUMP_HEADER();

        // Check signature 'MDMP' (0x504D444D)
        if (header.Signature == 0x504D444D) {
            // Check version (typical values are 0xA793 or similar)
            if ((header.Version & 0xFFFF) == 0xA793) {
                // Validate number of streams is reasonable (at least 1, max 10000)
                if ((header.NumberOfStreams > 0) && (header.NumberOfStreams < 10000)) {
                    // Validate stream directory RVA is within file bounds
                    qint64 nDirectoryOffset = (qint64)header.StreamDirectoryRva;
                    qint64 nDirectorySize = (qint64)header.NumberOfStreams * (qint64)sizeof(MINIDUMP_DIRECTORY);

                    if ((nDirectoryOffset > 0) && (nDirectoryOffset < nFileSize) && (nDirectorySize > 0) && (nDirectoryOffset + nDirectorySize <= nFileSize)) {
                        bResult = true;
                    }
                }
            }
        }
    }

    return bResult;
}

bool XMiniDump::isValid(QIODevice *pDevice)
{
    XMiniDump xminidump(pDevice);

    return xminidump.isValid();
}

XBinary::FT XMiniDump::getFileType()
{
    return FT_MINIDUMP;
}

QString XMiniDump::getMIMEString()
{
    return "application/x-dmp";
}

QString XMiniDump::getArch()
{
    QString sResult = tr("Unknown");

    XBinary::PDSTRUCT pdStructEmpty = {};
    PDSTRUCT *pPdStruct = &pdStructEmpty;

    // Find SystemInfoStream (stream type 7)
    MINIDUMP_DIRECTORY systemInfoDir = findStream(SystemInfoStream, pPdStruct);

    if ((systemInfoDir.StreamType == SystemInfoStream) && (systemInfoDir.DataSize >= sizeof(MINIDUMP_SYSTEM_INFO))) {
        MINIDUMP_SYSTEM_INFO systemInfo = read_MINIDUMP_SYSTEM_INFO(systemInfoDir.LocationRva);
        sResult = processorArchitectureToString(systemInfo.ProcessorArchitecture);
    }

    return sResult;
}

XBinary::MODE XMiniDump::getMode()
{
    MODE result = MODE_UNKNOWN;

    XBinary::PDSTRUCT pdStructEmpty = {};
    PDSTRUCT *pPdStruct = &pdStructEmpty;

    // Find SystemInfoStream (stream type 7)
    MINIDUMP_DIRECTORY systemInfoDir = findStream(SystemInfoStream, pPdStruct);

    if ((systemInfoDir.StreamType == SystemInfoStream) && (systemInfoDir.DataSize >= sizeof(MINIDUMP_SYSTEM_INFO))) {
        MINIDUMP_SYSTEM_INFO systemInfo = read_MINIDUMP_SYSTEM_INFO(systemInfoDir.LocationRva);

        // Determine mode based on architecture
        if ((systemInfo.ProcessorArchitecture == X_PROCESSOR_ARCHITECTURE_AMD64) || (systemInfo.ProcessorArchitecture == X_PROCESSOR_ARCHITECTURE_IA64) ||
            (systemInfo.ProcessorArchitecture == X_PROCESSOR_ARCHITECTURE_ARM64)) {
            result = MODE_64;
        } else if ((systemInfo.ProcessorArchitecture == X_PROCESSOR_ARCHITECTURE_INTEL) || (systemInfo.ProcessorArchitecture == X_PROCESSOR_ARCHITECTURE_ARM) ||
                   (systemInfo.ProcessorArchitecture == X_PROCESSOR_ARCHITECTURE_PPC) || (systemInfo.ProcessorArchitecture == X_PROCESSOR_ARCHITECTURE_MIPS)) {
            result = MODE_32;
        }
    }

    return result;
}

XBinary::ENDIAN XMiniDump::getEndian()
{
    return ENDIAN_LITTLE;  // MiniDump files are always little-endian
}

QString XMiniDump::getFileFormatExt()
{
    return "dmp";
}

QString XMiniDump::getFileFormatExtsString()
{
    return "Windows MiniDump (*.dmp)";
}

qint64 XMiniDump::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    return _calculateRawSize(pPdStruct);
}

QString XMiniDump::getVersion()
{
    QString sResult;

    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();
    sResult = QString::number(header.Version, 16).toUpper();

    return sResult;
}

QList<XBinary::MAPMODE> XMiniDump::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);

    return listResult;
}

XBinary::_MEMORY_MAP XMiniDump::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;  // Default mode
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return result;
}

QString XMiniDump::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XMINIDUMP_STRUCTID, sizeof(_TABLE_XMINIDUMP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

QList<XBinary::DATA_HEADER> XMiniDump::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        // Initialize with default headers
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);
        _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
        _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;

        // Start with header
        _dataHeadersOptions.nID = STRUCTID_HEADER;
        _dataHeadersOptions.nLocation = 0;
        _dataHeadersOptions.locType = XBinary::LT_OFFSET;

        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_HEADER) {
                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XMiniDump::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = sizeof(MINIDUMP_HEADER);

                dataHeader.listRecords.append(
                    getDataRecord(offsetof(MINIDUMP_HEADER, Signature), sizeof(quint32), "Signature", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(MINIDUMP_HEADER, Version), sizeof(quint32), "Version", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(MINIDUMP_HEADER, NumberOfStreams), sizeof(quint32), "NumberOfStreams", VT_UINT32, DRF_COUNT,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(MINIDUMP_HEADER, StreamDirectoryRva), sizeof(quint32), "StreamDirectoryRva", VT_UINT32, DRF_OFFSET,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(MINIDUMP_HEADER, CheckSum), sizeof(quint32), "CheckSum", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(MINIDUMP_HEADER, TimeDateStamp), sizeof(quint32), "TimeDateStamp", VT_UINT32, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));  // TODO TimeDateStamp
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(MINIDUMP_HEADER, Flags), sizeof(quint64), "Flags", VT_UINT64, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();

                    // Add directory table
                    DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                    _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                    _dataHeadersOptions.nID = STRUCTID_DIRECTORY;
                    _dataHeadersOptions.nLocation = header.StreamDirectoryRva;
                    _dataHeadersOptions.nCount = header.NumberOfStreams;
                    _dataHeadersOptions.nSize = header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY);

                    listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                }
            } else if (dataHeadersOptions.nID == STRUCTID_DIRECTORY) {
                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XMiniDump::structIDToString(dataHeadersOptions.nID));
                dataHeader.nSize = sizeof(MINIDUMP_DIRECTORY);

                dataHeader.listRecords.append(getDataRecord(offsetof(MINIDUMP_DIRECTORY, StreamType), sizeof(quint32), "StreamType", VT_UINT32, DRF_UNKNOWN,
                                                            dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(MINIDUMP_DIRECTORY, DataSize), sizeof(quint32), "DataSize", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                dataHeader.listRecords.append(getDataRecord(offsetof(MINIDUMP_DIRECTORY, LocationRva), sizeof(quint32), "LocationRva", VT_UINT32, DRF_OFFSET,
                                                            dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    // Compute directory index from location
                    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();
                    qint32 nDirectoryIndex = -1;

                    if ((nStartOffset >= (qint64)header.StreamDirectoryRva) &&
                        ((nStartOffset - (qint64)header.StreamDirectoryRva) % (qint64)sizeof(MINIDUMP_DIRECTORY) == 0)) {
                        nDirectoryIndex = (nStartOffset - (qint64)header.StreamDirectoryRva) / (qint64)sizeof(MINIDUMP_DIRECTORY);
                    }

                    if (nDirectoryIndex >= 0) {
                        // Read the directory entry to check if it's a ModuleListStream
                        MINIDUMP_DIRECTORY directory = read_MINIDUMP_DIRECTORY(nDirectoryIndex);

                        if (directory.StreamType == ModuleListStream) {
                            // Add ModuleListStream child
                            DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                            _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
                            _dataHeadersOptions.nID = STRUCTID_MODULE_LIST;
                            _dataHeadersOptions.nLocation = directory.LocationRva;
                            _dataHeadersOptions.nSize = directory.DataSize;

                            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                        }
                    }
                }
            } else if (dataHeadersOptions.nID == STRUCTID_MODULE_LIST) {
                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, XMiniDump::structIDToString(dataHeadersOptions.nID));

                // Read module list header
                MINIDUMP_MODULE_LIST moduleList = read_MINIDUMP_MODULE_LIST(nStartOffset);
                dataHeader.nSize = sizeof(quint32);

                dataHeader.listRecords.append(getDataRecord(0, sizeof(quint32), "NumberOfModules", VT_UINT32, DRF_COUNT, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren && (moduleList.NumberOfModules > 0)) {
                    // Add table of modules
                    DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                    _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                    _dataHeadersOptions.nID = STRUCTID_MODULE;
                    _dataHeadersOptions.nLocation = nStartOffset + sizeof(quint32);
                    _dataHeadersOptions.nCount = moduleList.NumberOfModules;
                    qint64 nModuleSize = 8 + 4 + 4 + 4 + 4 + sizeof(VS_FIXEDFILEINFO) + sizeof(MINIDUMP_LOCATION_DESCRIPTOR) * 2 + 8 + 8;
                    _dataHeadersOptions.nSize = moduleList.NumberOfModules * nModuleSize;

                    listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                }
            } else if (dataHeadersOptions.nID == STRUCTID_MODULE) {
                // Read module to get name for display
                MINIDUMP_MODULE module = read_MINIDUMP_MODULE(nStartOffset);
                QString sModuleName = read_MINIDUMP_STRING(module.ModuleNameRva);

                QString sStructName = XMiniDump::structIDToString(dataHeadersOptions.nID);
                if (!sModuleName.isEmpty()) {
                    sStructName = sStructName + QString(" (%1)").arg(sModuleName);
                }

                DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, sStructName);
                qint64 nModuleSize = 8 + 4 + 4 + 4 + 4 + sizeof(VS_FIXEDFILEINFO) + sizeof(MINIDUMP_LOCATION_DESCRIPTOR) * 2 + 8 + 8;
                dataHeader.nSize = nModuleSize;

                qint64 nOffset = 0;

                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint64), "BaseOfImage", VT_UINT64, DRF_ADDRESS, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 8;

                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "SizeOfImage", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;

                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "CheckSum", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;

                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "TimeDateStamp", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;

                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "ModuleNameRva", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;

                // VS_FIXEDFILEINFO
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwSignature", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwStrucVersion", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileVersionMS", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileVersionLS", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwProductVersionMS", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwProductVersionLS", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileFlagsMask", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileFlags", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileOS", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileType", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileSubtype", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileDateMS", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(
                    getDataRecord(nOffset, sizeof(quint32), "VersionInfo.dwFileDateLS", VT_UINT32, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;

                // CvRecord
                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "CvRecord.DataSize", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "CvRecord.Rva", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;

                // MiscRecord
                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "MiscRecord.DataSize", VT_UINT32, DRF_SIZE, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;
                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint32), "MiscRecord.Rva", VT_UINT32, DRF_OFFSET, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 4;

                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint64), "Reserved0", VT_UINT64, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));
                nOffset += 8;

                dataHeader.listRecords.append(getDataRecord(nOffset, sizeof(quint64), "Reserved1", VT_UINT64, DRF_UNKNOWN, dataHeadersOptions.pMemoryMap->endian));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XMiniDump::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    QList<FPART> listResult;

    qint64 nFileSize = getSize();

    if (nFileSize < (qint64)sizeof(MINIDUMP_HEADER)) {
        return listResult;
    }

    // Add header
    if (nFileParts & FILEPART_HEADER) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = sizeof(MINIDUMP_HEADER);
        record.nVirtualAddress = -1;
        record.sName = tr("Header");

        listResult.append(record);
    }

    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();

    // Add directory
    if (nFileParts & FILEPART_REGION) {
        FPART record = {};

        record.filePart = FILEPART_REGION;
        record.nFileOffset = header.StreamDirectoryRva;
        record.nFileSize = header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY);
        record.nVirtualAddress = -1;
        record.sName = tr("Directory");

        listResult.append(record);
    }

    // Add streams
    QList<MINIDUMP_DIRECTORY> listDirectories = read_MINIDUMP_DIRECTORY_list(pPdStruct);

    for (qint32 i = 0; (i < listDirectories.count()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MINIDUMP_DIRECTORY directory = listDirectories.at(i);

        // Validate stream is within file bounds
        if ((qint64)directory.LocationRva + (qint64)directory.DataSize > nFileSize) {
            continue;
        }

        if (nFileParts & FILEPART_STREAM) {
            FPART record = {};

            record.filePart = FILEPART_STREAM;
            record.nFileOffset = directory.LocationRva;
            record.nFileSize = directory.DataSize;
            record.nVirtualAddress = -1;
            record.sName = streamTypeToString(directory.StreamType);

            listResult.append(record);
        }
    }

    // Check for overlay
    if (nFileParts & FILEPART_OVERLAY) {
        // Find the maximum offset + size from all streams and directory
        qint64 nMaxOffset = sizeof(MINIDUMP_HEADER);
        qint64 nDirectoryEnd = header.StreamDirectoryRva + header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY);

        if (nDirectoryEnd > nMaxOffset) {
            nMaxOffset = nDirectoryEnd;
        }

        for (qint32 i = 0; i < listDirectories.count(); i++) {
            MINIDUMP_DIRECTORY directory = listDirectories.at(i);
            qint64 nStreamEnd = (qint64)directory.LocationRva + (qint64)directory.DataSize;

            if (nStreamEnd > nMaxOffset) {
                nMaxOffset = nStreamEnd;
            }
        }

        if (nMaxOffset < nFileSize) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nMaxOffset;
            record.nFileSize = nFileSize - nMaxOffset;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

XMiniDump::MINIDUMP_HEADER XMiniDump::read_MINIDUMP_HEADER()
{
    MINIDUMP_HEADER result = {};

    if (getSize() >= (qint64)sizeof(MINIDUMP_HEADER)) {
        result.Signature = read_uint32(offsetof(MINIDUMP_HEADER, Signature));
        result.Version = read_uint32(offsetof(MINIDUMP_HEADER, Version));
        result.NumberOfStreams = read_uint32(offsetof(MINIDUMP_HEADER, NumberOfStreams));
        result.StreamDirectoryRva = read_uint32(offsetof(MINIDUMP_HEADER, StreamDirectoryRva));
        result.CheckSum = read_uint32(offsetof(MINIDUMP_HEADER, CheckSum));
        result.TimeDateStamp = read_uint32(offsetof(MINIDUMP_HEADER, TimeDateStamp));
        result.Flags = read_uint64(offsetof(MINIDUMP_HEADER, Flags));
    }

    return result;
}

XMiniDump::MINIDUMP_DIRECTORY XMiniDump::read_MINIDUMP_DIRECTORY(qint32 nIndex)
{
    MINIDUMP_DIRECTORY result = {};

    if (nIndex < 0) {
        return result;
    }

    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();

    if ((quint32)nIndex >= header.NumberOfStreams) {
        return result;
    }

    qint64 nOffset = (qint64)header.StreamDirectoryRva + (qint64)nIndex * (qint64)sizeof(MINIDUMP_DIRECTORY);
    qint64 nFileSize = getSize();

    if ((nOffset >= 0) && (nOffset + (qint64)sizeof(MINIDUMP_DIRECTORY) <= nFileSize)) {
        result.StreamType = read_uint32(nOffset + offsetof(MINIDUMP_DIRECTORY, StreamType));
        result.DataSize = read_uint32(nOffset + offsetof(MINIDUMP_DIRECTORY, DataSize));
        result.LocationRva = read_uint32(nOffset + offsetof(MINIDUMP_DIRECTORY, LocationRva));
    }

    return result;
}

QList<XMiniDump::MINIDUMP_DIRECTORY> XMiniDump::read_MINIDUMP_DIRECTORY_list(PDSTRUCT *pPdStruct)
{
    QList<MINIDUMP_DIRECTORY> listResult;

    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();
    qint32 nNumberOfStreams = header.NumberOfStreams;

    for (qint32 i = 0; (i < nNumberOfStreams) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MINIDUMP_DIRECTORY directory = read_MINIDUMP_DIRECTORY(i);
        listResult.append(directory);
    }

    return listResult;
}

QString XMiniDump::streamTypeToString(quint32 nStreamType)
{
    QString sResult;

    QMap<quint64, QString> mapStreamTypes = getStreamTypesS();

    if (mapStreamTypes.contains(nStreamType)) {
        sResult = mapStreamTypes.value(nStreamType);
    } else {
        sResult = QString("Stream_%1").arg(nStreamType);
    }

    return sResult;
}

QMap<quint64, QString> XMiniDump::getStreamTypes()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(UnusedStream, "UnusedStream");
    mapResult.insert(ThreadListStream, "ThreadListStream");
    mapResult.insert(ModuleListStream, "ModuleListStream");
    mapResult.insert(MemoryListStream, "MemoryListStream");
    mapResult.insert(ExceptionStream, "ExceptionStream");
    mapResult.insert(SystemInfoStream, "SystemInfoStream");
    mapResult.insert(ThreadExListStream, "ThreadExListStream");
    mapResult.insert(Memory64ListStream, "Memory64ListStream");
    mapResult.insert(CommentStreamA, "CommentStreamA");
    mapResult.insert(CommentStreamW, "CommentStreamW");
    mapResult.insert(HandleDataStream, "HandleDataStream");
    mapResult.insert(FunctionTableStream, "FunctionTableStream");
    mapResult.insert(UnloadedModuleListStream, "UnloadedModuleListStream");
    mapResult.insert(MiscInfoStream, "MiscInfoStream");
    mapResult.insert(MemoryInfoListStream, "MemoryInfoListStream");
    mapResult.insert(ThreadInfoListStream, "ThreadInfoListStream");
    mapResult.insert(HandleOperationListStream, "HandleOperationListStream");
    mapResult.insert(TokenStream, "TokenStream");
    mapResult.insert(JavaScriptDataStream, "JavaScriptDataStream");
    mapResult.insert(SystemMemoryInfoStream, "SystemMemoryInfoStream");
    mapResult.insert(ProcessVmCountersStream, "ProcessVmCountersStream");
    mapResult.insert(IptTraceStream, "IptTraceStream");
    mapResult.insert(ThreadNamesStream, "ThreadNamesStream");

    return mapResult;
}

QMap<quint64, QString> XMiniDump::getStreamTypesS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(UnusedStream, "Unused");
    mapResult.insert(ThreadListStream, "Thread List");
    mapResult.insert(ModuleListStream, "Module List");
    mapResult.insert(MemoryListStream, "Memory List");
    mapResult.insert(ExceptionStream, "Exception");
    mapResult.insert(SystemInfoStream, "System Info");
    mapResult.insert(ThreadExListStream, "Thread Ex List");
    mapResult.insert(Memory64ListStream, "Memory 64 List");
    mapResult.insert(CommentStreamA, "Comment A");
    mapResult.insert(CommentStreamW, "Comment W");
    mapResult.insert(HandleDataStream, "Handle Data");
    mapResult.insert(FunctionTableStream, "Function Table");
    mapResult.insert(UnloadedModuleListStream, "Unloaded Module List");
    mapResult.insert(MiscInfoStream, "Misc Info");
    mapResult.insert(MemoryInfoListStream, "Memory Info List");
    mapResult.insert(ThreadInfoListStream, "Thread Info List");
    mapResult.insert(HandleOperationListStream, "Handle Operation List");
    mapResult.insert(TokenStream, "Token");
    mapResult.insert(JavaScriptDataStream, "JavaScript Data");
    mapResult.insert(SystemMemoryInfoStream, "System Memory Info");
    mapResult.insert(ProcessVmCountersStream, "Process VM Counters");
    mapResult.insert(IptTraceStream, "IPT Trace");
    mapResult.insert(ThreadNamesStream, "Thread Names");

    return mapResult;
}

XMiniDump::MINIDUMP_SYSTEM_INFO XMiniDump::read_MINIDUMP_SYSTEM_INFO(qint64 nOffset)
{
    MINIDUMP_SYSTEM_INFO result = {};

    qint64 nFileSize = getSize();

    if ((nOffset >= 0) && (nOffset < nFileSize) && (nOffset + (qint64)sizeof(MINIDUMP_SYSTEM_INFO) <= nFileSize)) {
        result.ProcessorArchitecture = read_uint16(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, ProcessorArchitecture));
        result.ProcessorLevel = read_uint16(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, ProcessorLevel));
        result.ProcessorRevision = read_uint16(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, ProcessorRevision));
        result.NumberOfProcessors = read_uint8(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, NumberOfProcessors));
        result.ProductType = read_uint8(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, ProductType));
        result.MajorVersion = read_uint32(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, MajorVersion));
        result.MinorVersion = read_uint32(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, MinorVersion));
        result.BuildNumber = read_uint32(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, BuildNumber));
        result.PlatformId = read_uint32(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, PlatformId));
        result.CSDVersionRva = read_uint32(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, CSDVersionRva));
        result.SuiteMask = read_uint16(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, SuiteMask));
        result.Reserved2 = read_uint16(nOffset + offsetof(MINIDUMP_SYSTEM_INFO, Reserved2));
    }

    return result;
}

XMiniDump::MINIDUMP_DIRECTORY XMiniDump::findStream(quint32 nStreamType, PDSTRUCT *pPdStruct)
{
    MINIDUMP_DIRECTORY result = {};

    QList<MINIDUMP_DIRECTORY> listDirectories = read_MINIDUMP_DIRECTORY_list(pPdStruct);

    for (qint32 i = 0; (i < listDirectories.count()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        if (listDirectories.at(i).StreamType == nStreamType) {
            result = listDirectories.at(i);
            break;
        }
    }

    return result;
}

QString XMiniDump::processorArchitectureToString(quint16 nArchitecture)
{
    QString sResult;

    QMap<quint64, QString> mapArchitectures = getProcessorArchitectures();

    if (mapArchitectures.contains(nArchitecture)) {
        sResult = mapArchitectures.value(nArchitecture);
    } else {
        sResult = QString("ARCH_%1").arg(nArchitecture);
    }

    return sResult;
}

QMap<quint64, QString> XMiniDump::getProcessorArchitectures()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(X_PROCESSOR_ARCHITECTURE_INTEL, "x86");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_MIPS, "MIPS");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_ALPHA, "Alpha");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_PPC, "PowerPC");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_SHX, "SHx");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_ARM, "ARM");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_IA64, "IA-64");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_ALPHA64, "Alpha64");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_MSIL, "MSIL");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_AMD64, "x64");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_IA32_ON_WIN64, "IA32 on Win64");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_NEUTRAL, "Neutral");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_ARM64, "ARM64");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64, "ARM32 on Win64");
    mapResult.insert(X_PROCESSOR_ARCHITECTURE_IA32_ON_ARM64, "IA32 on ARM64");

    return mapResult;
}

XMiniDump::MINIDUMP_MODULE_LIST XMiniDump::read_MINIDUMP_MODULE_LIST(qint64 nOffset)
{
    MINIDUMP_MODULE_LIST result = {};

    qint64 nFileSize = getSize();

    if ((nOffset >= 0) && (nOffset < nFileSize) && (nOffset + (qint64)sizeof(quint32) <= nFileSize)) {
        result.NumberOfModules = read_uint32(nOffset);
    }

    return result;
}

XMiniDump::MINIDUMP_MODULE XMiniDump::read_MINIDUMP_MODULE(qint64 nOffset)
{
    MINIDUMP_MODULE result = {};

    qint64 nFileSize = getSize();
    qint64 nStructSize = 8 + 4 + 4 + 4 + 4 + sizeof(VS_FIXEDFILEINFO) + sizeof(MINIDUMP_LOCATION_DESCRIPTOR) * 2 + 8 + 8;  // 108 bytes

    if ((nOffset >= 0) && (nOffset < nFileSize) && (nOffset + nStructSize <= nFileSize)) {
        qint64 nCurrentOffset = nOffset;

        result.BaseOfImage = read_uint64(nCurrentOffset);
        nCurrentOffset += 8;

        result.SizeOfImage = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;

        result.CheckSum = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;

        result.TimeDateStamp = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;

        result.ModuleNameRva = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;

        // Read VS_FIXEDFILEINFO
        result.VersionInfo.dwSignature = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwStrucVersion = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileVersionMS = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileVersionLS = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwProductVersionMS = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwProductVersionLS = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileFlagsMask = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileFlags = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileOS = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileType = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileSubtype = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileDateMS = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.VersionInfo.dwFileDateLS = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;

        // Read CvRecord
        result.CvRecord.DataSize = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.CvRecord.Rva = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;

        // Read MiscRecord
        result.MiscRecord.DataSize = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;
        result.MiscRecord.Rva = read_uint32(nCurrentOffset);
        nCurrentOffset += 4;

        result.Reserved0 = read_uint64(nCurrentOffset);
        nCurrentOffset += 8;

        result.Reserved1 = read_uint64(nCurrentOffset);
    }

    return result;
}

QString XMiniDump::read_MINIDUMP_STRING(qint64 nOffset)
{
    QString sResult;

    qint64 nFileSize = getSize();

    if ((nOffset >= 0) && (nOffset < nFileSize) && (nOffset + 4 <= nFileSize)) {
        quint32 nLength = read_uint32(nOffset);

        // Validate length (reasonable limit: max 32KB for module name)
        if ((nLength > 0) && (nLength < 32768)) {
            qint64 nStringOffset = nOffset + 4;
            qint64 nBytesNeeded = nLength;

            if (nStringOffset + nBytesNeeded <= nFileSize) {
                // Read Unicode string (UTF-16LE)
                QByteArray baData = read_array(nStringOffset, nBytesNeeded);

                // Convert UTF-16LE to QString
                sResult = QString::fromUtf16(reinterpret_cast<const ushort *>(baData.constData()), nLength / 2);
            }
        }
    }

    return sResult;
}

QList<XMiniDump::MINIDUMP_MODULE> XMiniDump::read_MINIDUMP_MODULE_list(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    QList<MINIDUMP_MODULE> listResult;

    MINIDUMP_MODULE_LIST moduleList = read_MINIDUMP_MODULE_LIST(nOffset);
    qint32 nNumberOfModules = moduleList.NumberOfModules;

    // Validate number of modules (reasonable limit)
    if ((nNumberOfModules <= 0) || (nNumberOfModules > 10000)) {
        return listResult;
    }

    qint64 nCurrentOffset = nOffset + sizeof(quint32);                                                                     // Skip NumberOfModules field
    qint64 nModuleSize = 8 + 4 + 4 + 4 + 4 + sizeof(VS_FIXEDFILEINFO) + sizeof(MINIDUMP_LOCATION_DESCRIPTOR) * 2 + 8 + 8;  // 108 bytes

    for (qint32 i = 0; (i < nNumberOfModules) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MINIDUMP_MODULE module = read_MINIDUMP_MODULE(nCurrentOffset);
        listResult.append(module);
        nCurrentOffset += nModuleSize;
    }

    return listResult;
}

bool XMiniDump::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapProperties)

    bool bResult = false;

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    if (!pState) {
        return false;
    }

    qint64 nFileSize = getSize();

    if (nFileSize < (qint64)sizeof(MINIDUMP_HEADER)) {
        return false;
    }

    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();

    // Validate header
    if (header.Signature != 0x504D444D) {
        return false;
    }

    // Create and initialize context
    MINIDUMP_UNPACK_CONTEXT *pContext = new MINIDUMP_UNPACK_CONTEXT;

    // Initialize state
    pState->nCurrentOffset = (qint64)sizeof(MINIDUMP_HEADER);
    pState->nTotalSize = nFileSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = pContext;

    // Read all directory entries
    pContext->listDirectories = read_MINIDUMP_DIRECTORY_list(pPdStruct);

    // Pre-compute stream offsets and count valid streams
    for (qint32 i = 0; (i < pContext->listDirectories.count()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MINIDUMP_DIRECTORY directory = pContext->listDirectories.at(i);

        // Validate stream is within file bounds
        qint64 nStreamOffset = (qint64)directory.LocationRva;
        qint64 nStreamSize = (qint64)directory.DataSize;

        if ((nStreamOffset >= 0) && (nStreamSize >= 0) && (nStreamOffset < nFileSize) && (nStreamOffset + nStreamSize <= nFileSize)) {
            pContext->listStreamOffsets.append(nStreamOffset);
            pState->nNumberOfRecords++;
        } else {
            // Invalid stream, add -1 as placeholder
            pContext->listStreamOffsets.append(-1);
        }
    }

    // Reset to first valid stream
    if (pState->nNumberOfRecords > 0) {
        // Find first valid stream
        for (qint32 i = 0; i < pContext->listStreamOffsets.count(); i++) {
            if (pContext->listStreamOffsets.at(i) >= 0) {
                pState->nCurrentIndex = i;
                pState->nCurrentOffset = pContext->listStreamOffsets.at(i);
                bResult = true;
                break;
            }
        }
    }

    // Clean up if no valid streams found
    if (!bResult) {
        delete pContext;
        pState->pContext = nullptr;
        pState->nNumberOfRecords = 0;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XMiniDump::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    XBinary::ARCHIVERECORD result = {};

    if (!pState || !pState->pContext) {
        return result;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= (qint32)pState->nNumberOfRecords)) {
        return result;
    }

    MINIDUMP_UNPACK_CONTEXT *pContext = (MINIDUMP_UNPACK_CONTEXT *)pState->pContext;

    if ((pState->nCurrentIndex >= pContext->listDirectories.count()) || (pState->nCurrentIndex >= pContext->listStreamOffsets.count())) {
        return result;
    }

    MINIDUMP_DIRECTORY directory = pContext->listDirectories.at(pState->nCurrentIndex);
    qint64 nStreamOffset = pContext->listStreamOffsets.at(pState->nCurrentIndex);

    // Skip invalid streams
    if (nStreamOffset < 0) {
        return result;
    }

    // Fill ARCHIVERECORD
    result.nStreamOffset = nStreamOffset;
    result.nStreamSize = (qint64)directory.DataSize;

    // Set properties
    QString sStreamName = streamTypeToString(directory.StreamType);
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sStreamName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)directory.DataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)directory.DataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);

    return result;
}

bool XMiniDump::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (!pState || !pState->pContext || !pDevice) {
        return false;
    }

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= (qint32)pState->nNumberOfRecords)) {
        return false;
    }

    MINIDUMP_UNPACK_CONTEXT *pContext = (MINIDUMP_UNPACK_CONTEXT *)pState->pContext;

    if ((pState->nCurrentIndex >= pContext->listDirectories.count()) || (pState->nCurrentIndex >= pContext->listStreamOffsets.count())) {
        return false;
    }

    MINIDUMP_DIRECTORY directory = pContext->listDirectories.at(pState->nCurrentIndex);
    qint64 nStreamOffset = pContext->listStreamOffsets.at(pState->nCurrentIndex);

    // Skip invalid streams
    if (nStreamOffset < 0) {
        return false;
    }

    qint64 nStreamSize = (qint64)directory.DataSize;

    // MiniDump streams are not compressed - use XStoreDecoder for direct copy
    XBinary::DATAPROCESS_STATE decompressState = {};
    decompressState.pDeviceInput = getDevice();
    decompressState.pDeviceOutput = pDevice;
    decompressState.nInputOffset = nStreamOffset;
    decompressState.nInputLimit = nStreamSize;
    decompressState.nProcessedOffset = 0;
    decompressState.nProcessedLimit = -1;

    bResult = XStoreDecoder::decompress(&decompressState, pPdStruct);

    return bResult;
}

bool XMiniDump::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (!pState || !pState->pContext) {
        return false;
    }

    MINIDUMP_UNPACK_CONTEXT *pContext = (MINIDUMP_UNPACK_CONTEXT *)pState->pContext;

    // Move to next stream (skip invalid ones)
    for (qint32 i = pState->nCurrentIndex + 1; i < pContext->listStreamOffsets.count(); i++) {
        if (pContext->listStreamOffsets.at(i) >= 0) {
            pState->nCurrentIndex = i;
            pState->nCurrentOffset = pContext->listStreamOffsets.at(i);
            bResult = true;
            break;
        }
    }

    return bResult;
}

bool XMiniDump::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    // Delete format-specific context
    if (pState->pContext) {
        MINIDUMP_UNPACK_CONTEXT *pContext = (MINIDUMP_UNPACK_CONTEXT *)pState->pContext;
        delete pContext;
        pState->pContext = nullptr;
    }

    // Reset state fields
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;

    return true;
}
