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

#include <memory>
#include <new>
#include <QPointer>
#include "xdecompress.h"

XBinary::XCONVERT _TABLE_XMINIDUMP_STRUCTID[] = {
    {XMiniDump::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},         {XMiniDump::STRUCTID_HEADER, "HEADER", QString("Header")},
    {XMiniDump::STRUCTID_DIRECTORY, "DIRECTORY", QString("Directory")},       {XMiniDump::STRUCTID_STREAM, "STREAM", QString("Stream")},
    {XMiniDump::STRUCTID_MODULE_LIST, "MODULE_LIST", QString("Module List")}, {XMiniDump::STRUCTID_MODULE, "MODULE", QString("Module")},
};

XMiniDump::XMiniDump(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XMiniDump::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

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

bool XMiniDump::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMiniDump xminidump(pDevice);

    return xminidump.isValid(pPdStruct);
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
    return QString::number(read_MINIDUMP_HEADER().Version, 16).toUpper();
}

QList<XBinary::MAPMODE> XMiniDump::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS};
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

QString XMiniDump::structIDToFtString(quint32 nID)
{
    return XBinary::XCONVERT_idToFtString(nID, _TABLE_XMINIDUMP_STRUCTID, sizeof(_TABLE_XMINIDUMP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

quint32 XMiniDump::ftStringToStructID(const QString &sFtString)
{
    return XCONVERT_ftStringToId(sFtString, _TABLE_XMINIDUMP_STRUCTID, sizeof(_TABLE_XMINIDUMP_STRUCTID) / sizeof(XBinary::XCONVERT));
}

static XBinary::XIDSTRING _TABLE_XMiniDump_StreamTypes[] = {{XMiniDump::UnusedStream, "UnusedStream"},
                                                            {XMiniDump::ThreadListStream, "ThreadListStream"},
                                                            {XMiniDump::ModuleListStream, "ModuleListStream"},
                                                            {XMiniDump::MemoryListStream, "MemoryListStream"},
                                                            {XMiniDump::ExceptionStream, "ExceptionStream"},
                                                            {XMiniDump::SystemInfoStream, "SystemInfoStream"},
                                                            {XMiniDump::ThreadExListStream, "ThreadExListStream"},
                                                            {XMiniDump::Memory64ListStream, "Memory64ListStream"},
                                                            {XMiniDump::CommentStreamA, "CommentStreamA"},
                                                            {XMiniDump::CommentStreamW, "CommentStreamW"},
                                                            {XMiniDump::HandleDataStream, "HandleDataStream"},
                                                            {XMiniDump::FunctionTableStream, "FunctionTableStream"},
                                                            {XMiniDump::UnloadedModuleListStream, "UnloadedModuleListStream"},
                                                            {XMiniDump::MiscInfoStream, "MiscInfoStream"},
                                                            {XMiniDump::MemoryInfoListStream, "MemoryInfoListStream"},
                                                            {XMiniDump::ThreadInfoListStream, "ThreadInfoListStream"},
                                                            {XMiniDump::HandleOperationListStream, "HandleOperationListStream"},
                                                            {XMiniDump::TokenStream, "TokenStream"},
                                                            {XMiniDump::JavaScriptDataStream, "JavaScriptDataStream"},
                                                            {XMiniDump::SystemMemoryInfoStream, "SystemMemoryInfoStream"},
                                                            {XMiniDump::ProcessVmCountersStream, "ProcessVmCountersStream"},
                                                            {XMiniDump::IptTraceStream, "IptTraceStream"},
                                                            {XMiniDump::ThreadNamesStream, "ThreadNamesStream"}};

QList<XBinary::XFHEADER> XMiniDump::getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct)
{
    QList<XBinary::XFHEADER> listResult;

    quint32 nStructID = xfStruct.nStructID;

    if (nStructID == STRUCTID_UNKNOWN) {
        XFSTRUCT _xfStruct = xfStruct;
        _xfStruct.nStructID = STRUCTID_HEADER;
        _xfStruct.xLoc = offsetToLoc(0);
        listResult.append(getXFHeaders(_xfStruct, pPdStruct));
    } else if (nStructID == STRUCTID_HEADER) {
        XLOC headerLoc = xfStruct.xLoc;
        if (headerLoc.locType == LT_UNKNOWN) {
            headerLoc = offsetToLoc(0);
        }

        XFHEADER xfHeader = {};
        xfHeader.sParentTag = xfStruct.sParent;
        xfHeader.fileType = xfStruct.fileType;
        xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_HEADER);
        xfHeader.xLoc = headerLoc;
        xfHeader.nSize = sizeof(MINIDUMP_HEADER);
        xfHeader.xfType = XFTYPE_HEADER;
        xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_HEADER, headerLoc);
        xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_HEADER), xfHeader.sParentTag);
        listResult.append(xfHeader);

        if (xfStruct.bIsParent) {
            MINIDUMP_HEADER header = read_MINIDUMP_HEADER();

            XFSTRUCT _xfStruct = xfStruct;
            _xfStruct.sParent = xfHeader.sTag;

            _xfStruct.nStructID = STRUCTID_DIRECTORY;
            _xfStruct.xLoc = offsetToLoc(header.StreamDirectoryRva);
            _xfStruct.nCount = header.NumberOfStreams;
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));

            _xfStruct.nStructID = STRUCTID_MODULE_LIST;
            _xfStruct.xLoc = offsetToLoc(-1);
            _xfStruct.nCount = 0;
            listResult.append(getXFHeaders(_xfStruct, pPdStruct));
        }
    } else if (nStructID == STRUCTID_DIRECTORY) {
        qint64 nOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);
        qint32 nCount = xfStruct.nCount;
        qint64 nFileSize = getSize();

        if ((nOffset == -1) || (nCount == 0)) {
            MINIDUMP_HEADER header = read_MINIDUMP_HEADER();
            nOffset = header.StreamDirectoryRva;
            nCount = header.NumberOfStreams;
        }

        if ((nOffset > 0) && (nCount > 0)) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_DIRECTORY);
            xfHeader.xLoc = offsetToLoc(nOffset);
            xfHeader.xfType = XFTYPE_TABLE;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_DIRECTORY, xfHeader.xLoc);
            // Field 0 = StreamType
            xfHeader.listDataSt.append({0, 0, XFDATASTYPE_LIST, _TABLE_XMiniDump_StreamTypes, sizeof(_TABLE_XMiniDump_StreamTypes) / sizeof(XBinary::XIDSTRING)});

            qint64 nCurrentOffset = nOffset;
            for (qint32 i = 0; i < nCount; i++) {
                if ((nCurrentOffset + (qint64)sizeof(MINIDUMP_DIRECTORY)) > nFileSize) {
                    break;
                }
                xfHeader.listRowLocations.append(nCurrentOffset);
                nCurrentOffset += sizeof(MINIDUMP_DIRECTORY);
            }

            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_DIRECTORY), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    } else if (nStructID == STRUCTID_MODULE_LIST) {
        MINIDUMP_DIRECTORY directory = findStream(ModuleListStream, pPdStruct);

        if ((directory.LocationRva > 0) && (directory.DataSize >= sizeof(MINIDUMP_MODULE_LIST))) {
            XLOC listLoc = offsetToLoc(directory.LocationRva);

            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_MODULE_LIST);
            xfHeader.xLoc = listLoc;
            xfHeader.nSize = sizeof(MINIDUMP_MODULE_LIST);
            xfHeader.xfType = XFTYPE_HEADER;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_MODULE_LIST, listLoc);
            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_MODULE_LIST), xfHeader.sParentTag);
            listResult.append(xfHeader);

            if (xfStruct.bIsParent) {
                MINIDUMP_MODULE_LIST moduleList = read_MINIDUMP_MODULE_LIST(directory.LocationRva);

                XFSTRUCT _xfStruct = xfStruct;
                _xfStruct.sParent = xfHeader.sTag;
                _xfStruct.nStructID = STRUCTID_MODULE;
                _xfStruct.xLoc = offsetToLoc(directory.LocationRva + sizeof(MINIDUMP_MODULE_LIST));
                _xfStruct.nCount = moduleList.NumberOfModules;
                listResult.append(getXFHeaders(_xfStruct, pPdStruct));
            }
        }
    } else if (nStructID == STRUCTID_MODULE) {
        qint64 nOffset = locToOffset(xfStruct.pMemoryMap, xfStruct.xLoc);
        qint32 nCount = xfStruct.nCount;
        qint64 nFileSize = getSize();

        if ((nOffset == -1) || (nCount == 0)) {
            MINIDUMP_DIRECTORY directory = findStream(ModuleListStream, pPdStruct);

            if (directory.LocationRva > 0) {
                nOffset = directory.LocationRva + sizeof(MINIDUMP_MODULE_LIST);
                nCount = read_MINIDUMP_MODULE_LIST(directory.LocationRva).NumberOfModules;
            }
        }

        if ((nOffset > 0) && (nCount > 0)) {
            XFHEADER xfHeader = {};
            xfHeader.sParentTag = xfStruct.sParent;
            xfHeader.fileType = xfStruct.fileType;
            xfHeader.structID = static_cast<XBinary::STRUCTID>(STRUCTID_MODULE);
            xfHeader.xLoc = offsetToLoc(nOffset);
            xfHeader.xfType = XFTYPE_TABLE;
            xfHeader.listFields = getXFRecords(xfStruct.fileType, STRUCTID_MODULE, xfHeader.xLoc);

            qint64 nCurrentOffset = nOffset;
            for (qint32 i = 0; i < nCount; i++) {
                if ((nCurrentOffset + (qint64)sizeof(MINIDUMP_MODULE)) > nFileSize) {
                    break;
                }
                xfHeader.listRowLocations.append(nCurrentOffset);
                nCurrentOffset += sizeof(MINIDUMP_MODULE);
            }

            xfHeader.sTag = xfHeaderToTag(xfHeader, structIDToString(STRUCTID_MODULE), xfHeader.sParentTag);
            listResult.append(xfHeader);
        }
    }

    return listResult;
}

QList<XBinary::XFRECORD> XMiniDump::getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc)
{
    Q_UNUSED(fileType)
    Q_UNUSED(xLoc)

    QList<XBinary::XFRECORD> listResult;

    if (nStructID == STRUCTID_HEADER) {
        listResult.append({"Signature", (qint32)offsetof(MINIDUMP_HEADER, Signature), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"Version", (qint32)offsetof(MINIDUMP_HEADER, Version), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"NumberOfStreams", (qint32)offsetof(MINIDUMP_HEADER, NumberOfStreams), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
        listResult.append({"StreamDirectoryRva", (qint32)offsetof(MINIDUMP_HEADER, StreamDirectoryRva), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append({"CheckSum", (qint32)offsetof(MINIDUMP_HEADER, CheckSum), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"TimeDateStamp", (qint32)offsetof(MINIDUMP_HEADER, TimeDateStamp), 4, XFRECORD_FLAG_UNIXTIME, VT_UINT32});
        listResult.append({"Flags", (qint32)offsetof(MINIDUMP_HEADER, Flags), 8, XFRECORD_FLAG_NONE, VT_UINT64});
    } else if (nStructID == STRUCTID_DIRECTORY) {
        listResult.append({"StreamType", (qint32)offsetof(MINIDUMP_DIRECTORY, StreamType), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"DataSize", (qint32)offsetof(MINIDUMP_DIRECTORY, DataSize), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"LocationRva", (qint32)offsetof(MINIDUMP_DIRECTORY, LocationRva), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
    } else if (nStructID == STRUCTID_MODULE_LIST) {
        listResult.append({"NumberOfModules", (qint32)offsetof(MINIDUMP_MODULE_LIST, NumberOfModules), 4, XFRECORD_FLAG_COUNT, VT_UINT32});
    } else if (nStructID == STRUCTID_MODULE) {
        listResult.append({"BaseOfImage", (qint32)offsetof(MINIDUMP_MODULE, BaseOfImage), 8, XFRECORD_FLAG_ADDRESS, VT_UINT64});
        listResult.append({"SizeOfImage", (qint32)offsetof(MINIDUMP_MODULE, SizeOfImage), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append({"CheckSum", (qint32)offsetof(MINIDUMP_MODULE, CheckSum), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"TimeDateStamp", (qint32)offsetof(MINIDUMP_MODULE, TimeDateStamp), 4, XFRECORD_FLAG_UNIXTIME, VT_UINT32});
        listResult.append({"ModuleNameRva", (qint32)offsetof(MINIDUMP_MODULE, ModuleNameRva), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append(
            {"VersionInfo.dwSignature", (qint32)(offsetof(MINIDUMP_MODULE, VersionInfo) + offsetof(VS_FIXEDFILEINFO, dwSignature)), 4, XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"VersionInfo.dwFileVersionMS", (qint32)(offsetof(MINIDUMP_MODULE, VersionInfo) + offsetof(VS_FIXEDFILEINFO, dwFileVersionMS)), 4,
                           XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"VersionInfo.dwFileVersionLS", (qint32)(offsetof(MINIDUMP_MODULE, VersionInfo) + offsetof(VS_FIXEDFILEINFO, dwFileVersionLS)), 4,
                           XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"VersionInfo.dwProductVersionMS", (qint32)(offsetof(MINIDUMP_MODULE, VersionInfo) + offsetof(VS_FIXEDFILEINFO, dwProductVersionMS)), 4,
                           XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append({"VersionInfo.dwProductVersionLS", (qint32)(offsetof(MINIDUMP_MODULE, VersionInfo) + offsetof(VS_FIXEDFILEINFO, dwProductVersionLS)), 4,
                           XFRECORD_FLAG_NONE, VT_UINT32});
        listResult.append(
            {"CvRecord.DataSize", (qint32)(offsetof(MINIDUMP_MODULE, CvRecord) + offsetof(MINIDUMP_LOCATION_DESCRIPTOR, DataSize)), 4, XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append(
            {"CvRecord.Rva", (qint32)(offsetof(MINIDUMP_MODULE, CvRecord) + offsetof(MINIDUMP_LOCATION_DESCRIPTOR, Rva)), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
        listResult.append({"MiscRecord.DataSize", (qint32)(offsetof(MINIDUMP_MODULE, MiscRecord) + offsetof(MINIDUMP_LOCATION_DESCRIPTOR, DataSize)), 4,
                           XFRECORD_FLAG_SIZE, VT_UINT32});
        listResult.append(
            {"MiscRecord.Rva", (qint32)(offsetof(MINIDUMP_MODULE, MiscRecord) + offsetof(MINIDUMP_LOCATION_DESCRIPTOR, Rva)), 4, XFRECORD_FLAG_OFFSET, VT_UINT32});
    }

    return listResult;
}

static bool _minidumpCanAppend(qint32 nLimit, const QList<XBinary::FPART> &listResult)
{
    return (nLimit == -1) || (listResult.size() < nLimit);
}

QList<XBinary::FPART> XMiniDump::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0)) {
        return listResult;
    }

    qint64 nFileSize = getSize();

    if (nFileSize < (qint64)sizeof(MINIDUMP_HEADER)) {
        return listResult;
    }

    // Add header
    if ((nFileParts & FILEPART_HEADER) && _minidumpCanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = sizeof(MINIDUMP_HEADER);
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");

        listResult.append(record);
    }

    MINIDUMP_HEADER header = read_MINIDUMP_HEADER();

    // Add directory
    if ((nFileParts & FILEPART_REGION) && _minidumpCanAppend(nLimit, listResult)) {
        FPART record = {};

        record.filePart = FILEPART_REGION;
        record.nFileOffset = header.StreamDirectoryRva;
        record.nFileSize = header.NumberOfStreams * sizeof(MINIDUMP_DIRECTORY);
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Directory");

        listResult.append(record);
    }

    // Add streams
    QList<MINIDUMP_DIRECTORY> listDirectories = read_MINIDUMP_DIRECTORY_list(pPdStruct);

    for (qint32 i = 0; (i < listDirectories.count()) && _minidumpCanAppend(nLimit, listResult) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MINIDUMP_DIRECTORY directory = listDirectories.at(i);

        // Validate stream is within file bounds
        if ((qint64)directory.LocationRva + (qint64)directory.DataSize > nFileSize) {
            continue;
        }

        if ((nFileParts & FILEPART_STREAM) && _minidumpCanAppend(nLimit, listResult)) {
            FPART record = {};

            record.filePart = FILEPART_STREAM;
            record.nFileOffset = directory.LocationRva;
            record.nFileSize = directory.DataSize;
            record.nVirtualAddress = XADDR_MAX;
            record.sName = streamTypeToString(directory.StreamType);

            listResult.append(record);
        }
    }

    // Check for overlay
    if ((nFileParts & FILEPART_OVERLAY) && _minidumpCanAppend(nLimit, listResult)) {
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
            record.nVirtualAddress = XADDR_MAX;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
}

XMiniDump::MINIDUMP_HEADER XMiniDump::read_MINIDUMP_HEADER()
{
    QPointer<XMiniDump> guardedThis(this);
    MINIDUMP_HEADER result = {};

    if (guardedThis->getSize() >= (qint64)sizeof(MINIDUMP_HEADER)) {
        result.Signature = guardedThis->read_uint32(offsetof(MINIDUMP_HEADER, Signature));
        if (!guardedThis) return MINIDUMP_HEADER();
        result.Version = guardedThis->read_uint32(offsetof(MINIDUMP_HEADER, Version));
        if (!guardedThis) return MINIDUMP_HEADER();
        result.NumberOfStreams = guardedThis->read_uint32(offsetof(MINIDUMP_HEADER, NumberOfStreams));
        if (!guardedThis) return MINIDUMP_HEADER();
        result.StreamDirectoryRva = guardedThis->read_uint32(offsetof(MINIDUMP_HEADER, StreamDirectoryRva));
        if (!guardedThis) return MINIDUMP_HEADER();
        result.CheckSum = guardedThis->read_uint32(offsetof(MINIDUMP_HEADER, CheckSum));
        if (!guardedThis) return MINIDUMP_HEADER();
        result.TimeDateStamp = guardedThis->read_uint32(offsetof(MINIDUMP_HEADER, TimeDateStamp));
        if (!guardedThis) return MINIDUMP_HEADER();
        result.Flags = guardedThis->read_uint64(offsetof(MINIDUMP_HEADER, Flags));
        if (!guardedThis) return MINIDUMP_HEADER();
    }

    return result;
}

XMiniDump::MINIDUMP_DIRECTORY XMiniDump::read_MINIDUMP_DIRECTORY(qint32 nIndex)
{
    QPointer<XMiniDump> guardedThis(this);
    MINIDUMP_DIRECTORY result = {};

    if (nIndex < 0) {
        return result;
    }

    MINIDUMP_HEADER header = guardedThis->read_MINIDUMP_HEADER();
    if (!guardedThis) return MINIDUMP_DIRECTORY();

    if ((quint32)nIndex >= header.NumberOfStreams) {
        return result;
    }

    qint64 nOffset = (qint64)header.StreamDirectoryRva + (qint64)nIndex * (qint64)sizeof(MINIDUMP_DIRECTORY);
    qint64 nFileSize = guardedThis->getSize();

    if ((nOffset >= 0) && (nOffset + (qint64)sizeof(MINIDUMP_DIRECTORY) <= nFileSize)) {
        result.StreamType = guardedThis->read_uint32(nOffset + offsetof(MINIDUMP_DIRECTORY, StreamType));
        if (!guardedThis) return MINIDUMP_DIRECTORY();
        result.DataSize = guardedThis->read_uint32(nOffset + offsetof(MINIDUMP_DIRECTORY, DataSize));
        if (!guardedThis) return MINIDUMP_DIRECTORY();
        result.LocationRva = guardedThis->read_uint32(nOffset + offsetof(MINIDUMP_DIRECTORY, LocationRva));
        if (!guardedThis) return MINIDUMP_DIRECTORY();
    }

    return result;
}

QList<XMiniDump::MINIDUMP_DIRECTORY> XMiniDump::read_MINIDUMP_DIRECTORY_list(PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    QList<MINIDUMP_DIRECTORY> listResult;

    MINIDUMP_HEADER header = guardedThis->read_MINIDUMP_HEADER();
    if (!guardedThis) return QList<MINIDUMP_DIRECTORY>();
    qint32 nNumberOfStreams = header.NumberOfStreams;

    for (qint32 i = 0; (i < nNumberOfStreams) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MINIDUMP_DIRECTORY directory = guardedThis->read_MINIDUMP_DIRECTORY(i);
        if (!guardedThis) return QList<MINIDUMP_DIRECTORY>();
        listResult.append(directory);
    }

    return listResult;
}

QString XMiniDump::streamTypeToString(quint32 nStreamType)
{
    return getStreamTypesS().value(nStreamType, QString("Stream_%1").arg(nStreamType));
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
    return getProcessorArchitectures().value(nArchitecture, QString("ARCH_%1").arg(nArchitecture));
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
                sResult = QString::fromUtf16(reinterpret_cast<const char16_t *>(baData.constData()), nLength / 2);
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

QMap<XBinary::UNPACK_PROP, QVariant> XMiniDump::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XArchive::getDefaultUnpackProperties();

    return result;
}

static bool _minidumpFailSource(QPointer<XMiniDump> *pGuardedThis, XBinary::UNPACK_STATE *pState)
{
    if (pGuardedThis->isNull()) return false;
    (*pGuardedThis)->releaseUnpackSource(pState);
    *pState = XBinary::UNPACK_STATE();
    return false;
}

bool XMiniDump::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    bool bResult = false;

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    if (!pState || m_bUnpackOperationInProgress) {
        return false;
    }

    const bool bFinished = guardedThis->finishUnpack(pState, nullptr);
    if (!guardedThis || !bFinished) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    const bool bBound = guardedThis->bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;
    qint64 nFileSize = guardedThis->getSize();

    if (nFileSize < (qint64)sizeof(MINIDUMP_HEADER)) {
        return _minidumpFailSource(&guardedThis, pState);
    }

    MINIDUMP_HEADER header = guardedThis->read_MINIDUMP_HEADER();
    if (!guardedThis) return false;

    // Validate header
    if (header.Signature != 0x504D444D) {
        return _minidumpFailSource(&guardedThis, pState);
    }

    // Read all directory entries
    QList<MINIDUMP_DIRECTORY> listDirectories = guardedThis->read_MINIDUMP_DIRECTORY_list(pPdStruct);
    if (!guardedThis) return false;
    QList<qint64> listStreamOffsets;
    QList<qint32> listValidDirectoryIndexes;
    qint32 nNumberOfRecords = 0;
    qint64 nCurrentOffset = (qint64)sizeof(MINIDUMP_HEADER);

    // Pre-compute stream offsets and count valid streams
    for (qint32 i = 0; (i < listDirectories.count()) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        MINIDUMP_DIRECTORY directory = listDirectories.at(i);

        // Validate stream is within file bounds
        qint64 nStreamOffset = (qint64)directory.LocationRva;
        qint64 nStreamSize = (qint64)directory.DataSize;

        if ((nStreamOffset >= 0) && (nStreamSize >= 0) && (nStreamOffset < nFileSize) && (nStreamOffset + nStreamSize <= nFileSize)) {
            listStreamOffsets.append(nStreamOffset);
            listValidDirectoryIndexes.append(i);
            nNumberOfRecords++;
        } else {
            // Invalid stream, add -1 as placeholder
            listStreamOffsets.append(-1);
        }
    }

    // Reset to first valid stream
    if ((nNumberOfRecords > 0) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nDirectoryIndex = listValidDirectoryIndexes.first();
        nCurrentOffset = listStreamOffsets.at(nDirectoryIndex);
        bResult = true;
    }

    // Clean up if no valid streams found
    if (!bResult) return _minidumpFailSource(&guardedThis, pState);

    MINIDUMP_UNPACK_CONTEXT *pContext = new (std::nothrow) MINIDUMP_UNPACK_CONTEXT;
    if (!pContext) return _minidumpFailSource(&guardedThis, pState);
    pContext->listDirectories = listDirectories;
    pContext->listStreamOffsets = listStreamOffsets;
    pContext->listValidDirectoryIndexes = listValidDirectoryIndexes;

    pState->nCurrentOffset = nCurrentOffset;
    pState->nTotalSize = nFileSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = nNumberOfRecords;
    pState->pContext = pContext;

    bResult = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis) return false;
    if (!bResult) {
        pState->pContext = nullptr;
        const bool bFailed = _minidumpFailSource(&guardedThis, pState);
        delete pContext;
        return bFailed;
    }

    return bResult;
}

XBinary::ARCHIVERECORD XMiniDump::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed()) return XBinary::ARCHIVERECORD();

    XBinary::ARCHIVERECORD result = {};

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext) {
        return result;
    }
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return result;

    if ((pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    MINIDUMP_UNPACK_CONTEXT *pContext = static_cast<MINIDUMP_UNPACK_CONTEXT *>(pState->pContext);

    if (pState->nCurrentIndex >= pContext->listValidDirectoryIndexes.count()) {
        return result;
    }

    qint32 nDirectoryIndex = pContext->listValidDirectoryIndexes.at(pState->nCurrentIndex);
    if ((nDirectoryIndex < 0) || (nDirectoryIndex >= pContext->listDirectories.count()) || (nDirectoryIndex >= pContext->listStreamOffsets.count())) {
        return result;
    }
    MINIDUMP_DIRECTORY directory = pContext->listDirectories.at(nDirectoryIndex);
    qint64 nStreamOffset = pContext->listStreamOffsets.at(nDirectoryIndex);

    // Skip invalid streams
    if (nStreamOffset < 0) {
        return result;
    }

    // Fill ARCHIVERECORD
    result.nStreamOffset = nStreamOffset;
    result.nStreamSize = (qint64)directory.DataSize;

    // Set properties
    QString sStreamName = guardedThis->streamTypeToString(directory.StreamType);
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, sStreamName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)directory.DataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)directory.DataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);

    const bool bFinalSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bFinalSourceCurrent) {
        return XBinary::ARCHIVERECORD();
    }
    return result;
}

bool XMiniDump::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(guardedThis->getDevice());

    if (!pState || !pState->pContext || !guardedSource || !guardedOutput) return false;
    const bool bOutputSupported = guardedThis->isUnpackOutputSupported(guardedOutput.data());
    if (!guardedThis || !guardedSource || !guardedOutput || !bOutputSupported) return false;
    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !guardedSource || !guardedOutput || !bSourceCurrent || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= (qint32)pState->nNumberOfRecords))
        return false;
    const bool bAliases = XBinary::devicesAlias(guardedSource.data(), guardedOutput.data());
    if (!guardedThis || !guardedSource || !guardedOutput || bAliases) return false;

    UNPACK_INFO_AUTHORIZATION infoAuthorization(m_pUnpackGuardState);
    if (!infoAuthorization.isAuthorized()) return false;
    const ARCHIVERECORD archiveRecord = guardedThis->infoCurrent(pState, pPdStruct);
    if (!guardedThis) return false;
    const qint64 nExpectedSize = archiveRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();
    if ((nExpectedSize < 0) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nExpectedSize)) return false;

    // This override bypasses the base decode chain's per-entry gate; account
    // the member here. Produced bytes are charged by _writeDevice through the
    // budget threaded into decompressArchiveRecord below.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, archiveRecord.mapProperties.value(FPART_PROP_ORIGINALNAME).toString())) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(XBinary::createFileBuffer(nExpectedSize, pPdStruct));
    if (!guardedThis || !pStage || !guardedSource || !guardedOutput) return false;
    const bool bStageSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bStageSourceCurrent) return false;

    XDecompress xDecompress;
    connect(&xDecompress, &XDecompress::errorMessage, guardedThis.data(), &XBinary::errorMessage);
    connect(&xDecompress, &XDecompress::infoMessage, guardedThis.data(), &XBinary::infoMessage);

    const bool bResult =
        xDecompress.decompressArchiveRecord(archiveRecord, guardedSource.data(), pStage.get(), pState->mapUnpackProperties, pPdStruct, pState->spOutputBudget);
    if (!guardedThis || !bResult || !guardedSource || !guardedOutput) return false;
    const bool bFinalSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !guardedSource || !guardedOutput || !bFinalSourceCurrent) return false;
    const bool bPublished = guardedThis->publishUnpackOutput(pStage.get(), guardedOutput.data(), pState, pPdStruct);
    return guardedThis && bPublished;
}

bool XMiniDump::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    const bool bSourceCurrent = guardedThis->isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent) return false;
    MINIDUMP_UNPACK_CONTEXT *pContext = static_cast<MINIDUMP_UNPACK_CONTEXT *>(pState->pContext);

    qint32 nNextIndex = pState->nCurrentIndex + 1;
    pState->nCurrentIndex = nNextIndex;

    // Reaching the declared record count is the normal terminal state.  The
    // caller observes false (there is no next record), while the cursor still
    // advances past the record that was just consumed.
    if (nNextIndex >= pContext->listValidDirectoryIndexes.count()) return false;

    qint32 nDirectoryIndex = pContext->listValidDirectoryIndexes.at(nNextIndex);
    if ((nDirectoryIndex < 0) || (nDirectoryIndex >= pContext->listStreamOffsets.count())) return false;

    pState->nCurrentOffset = pContext->listStreamOffsets.at(nDirectoryIndex);

    return true;
}

bool XMiniDump::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired()) return false;

    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !guardedThis->ownsUnpackSource(pState)) return false;
    MINIDUMP_UNPACK_CONTEXT *pContext = static_cast<MINIDUMP_UNPACK_CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    guardedThis->releaseUnpackSource(pState);
    if (!guardedThis) return false;
    delete pContext;
    if (!guardedThis) return false;

    // Reset state fields
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();

    return true;
}

QList<QString> XMiniDump::getSearchSignatures()
{
    return {"'MDMP'"};
}

XBinary *XMiniDump::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XMiniDump(pDevice);
}

bool XMiniDump::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    bool bResult = true;

    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo = static_cast<XArchive::INTERNAL_INFO *>(guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(guardedThis->m_internalInfo) = *pInfo;
    }

    return guardedThis && bResult;
}

void *XMiniDump::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XMiniDump> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XMiniDump::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
