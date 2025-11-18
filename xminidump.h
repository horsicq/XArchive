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
#ifndef XMINIDUMP_H
#define XMINIDUMP_H

#include "xarchive.h"

class XMiniDump : public XArchive {
    Q_OBJECT

public:
    /*!
        \brief XMiniDump class for handling Windows MiniDump (.dmp) files
        MiniDump files are crash dump files created by Windows when an application crashes.
        They contain memory snapshots, thread information, loaded modules, and other debugging data.
    */
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
        STRUCTID_DIRECTORY,
        STRUCTID_STREAM,
        STRUCTID_MODULE_LIST,
        STRUCTID_MODULE
    };

    // MiniDump file format structures based on Windows DbgHelp.h
#pragma pack(push, 1)
    struct MINIDUMP_HEADER {
        quint32 Signature;           // 'MDMP' (0x504D444D)
        quint32 Version;             // Version (0xA793 for current format)
        quint32 NumberOfStreams;     // Number of directory entries
        quint32 StreamDirectoryRva;  // RVA of directory
        quint32 CheckSum;            // Checksum (often 0)
        quint32 TimeDateStamp;       // Time and date stamp
        quint64 Flags;               // Dump type flags
    };

    struct MINIDUMP_DIRECTORY {
        quint32 StreamType;      // Type of stream
        quint32 DataSize;        // Size of data
        quint32 LocationRva;     // RVA to stream data
    };

    struct MINIDUMP_LOCATION_DESCRIPTOR {
        quint32 DataSize;
        quint32 Rva;
    };

    struct MINIDUMP_MEMORY_DESCRIPTOR {
        quint64 StartOfMemoryRange;
        MINIDUMP_LOCATION_DESCRIPTOR Memory;
    };

    struct MINIDUMP_SYSTEM_INFO {
        quint16 ProcessorArchitecture;
        quint16 ProcessorLevel;
        quint16 ProcessorRevision;
        quint8 NumberOfProcessors;
        quint8 ProductType;
        quint32 MajorVersion;
        quint32 MinorVersion;
        quint32 BuildNumber;
        quint32 PlatformId;
        quint32 CSDVersionRva;
        quint16 SuiteMask;
        quint16 Reserved2;
        // Additional fields for CPU features...
    };

    struct VS_FIXEDFILEINFO {
        quint32 dwSignature;            // 0xFEEF04BD
        quint32 dwStrucVersion;
        quint32 dwFileVersionMS;
        quint32 dwFileVersionLS;
        quint32 dwProductVersionMS;
        quint32 dwProductVersionLS;
        quint32 dwFileFlagsMask;
        quint32 dwFileFlags;
        quint32 dwFileOS;
        quint32 dwFileType;
        quint32 dwFileSubtype;
        quint32 dwFileDateMS;
        quint32 dwFileDateLS;
    };

    struct MINIDUMP_STRING {
        quint32 Length;                 // Length in bytes (not including null terminator)
        quint16 Buffer[1];              // Unicode string data
    };

    struct MINIDUMP_MODULE {
        quint64 BaseOfImage;            // Base address of module
        quint32 SizeOfImage;            // Size of module in bytes
        quint32 CheckSum;               // Module checksum
        quint32 TimeDateStamp;          // Module timestamp
        quint32 ModuleNameRva;          // RVA to module name (MINIDUMP_STRING)
        VS_FIXEDFILEINFO VersionInfo;   // Module version information
        MINIDUMP_LOCATION_DESCRIPTOR CvRecord;      // CodeView record
        MINIDUMP_LOCATION_DESCRIPTOR MiscRecord;    // Misc debug record
        quint64 Reserved0;
        quint64 Reserved1;
    };

    struct MINIDUMP_MODULE_LIST {
        quint32 NumberOfModules;        // Number of modules in the list
        // Followed by NumberOfModules MINIDUMP_MODULE entries
    };
#pragma pack(pop)

    // Processor architectures from Windows headers
    enum X_PROCESSOR_ARCHITECTURE {
        X_PROCESSOR_ARCHITECTURE_INTEL = 0,
        X_PROCESSOR_ARCHITECTURE_MIPS = 1,
        X_PROCESSOR_ARCHITECTURE_ALPHA = 2,
        X_PROCESSOR_ARCHITECTURE_PPC = 3,
        X_PROCESSOR_ARCHITECTURE_SHX = 4,
        X_PROCESSOR_ARCHITECTURE_ARM = 5,
        X_PROCESSOR_ARCHITECTURE_IA64 = 6,
        X_PROCESSOR_ARCHITECTURE_ALPHA64 = 7,
        X_PROCESSOR_ARCHITECTURE_MSIL = 8,
        X_PROCESSOR_ARCHITECTURE_AMD64 = 9,
        X_PROCESSOR_ARCHITECTURE_IA32_ON_WIN64 = 10,
        X_PROCESSOR_ARCHITECTURE_NEUTRAL = 11,
        X_PROCESSOR_ARCHITECTURE_ARM64 = 12,
        X_PROCESSOR_ARCHITECTURE_ARM32_ON_WIN64 = 13,
        X_PROCESSOR_ARCHITECTURE_IA32_ON_ARM64 = 14,
        X_PROCESSOR_ARCHITECTURE_UNKNOWN = 0xFFFF
    };

    // Stream types from Windows DbgHelp.h
    enum STREAM_TYPE {
        /// Reserved/Unused stream type
        UnusedStream = 0,
        /// Reserved stream type (1-2)
        // ReservedStream0 = 1,
        // ReservedStream1 = 2,
        /// List of threads in the process at crash time
        ///
        /// Contains thread IDs, context records, and stack information
        ThreadListStream = 3,
        /// List of modules (DLLs/executables) loaded in the process
        ///
        /// Includes module names, base addresses, versions, and timestamps
        ModuleListStream = 4,
        /// List of memory regions saved in the dump
        ///
        /// Contains descriptors for each saved memory block
        MemoryListStream = 5,
        /// Exception information that caused the crash
        ///
        /// Includes exception code, address, and thread context
        ExceptionStream = 6,
        /// System information (OS, CPU architecture)
        ///
        /// Contains OS version, processor type, and system configuration
        SystemInfoStream = 7,
        /// Extended thread list with additional information
        ///
        /// Enhanced version of ThreadListStream with more details
        ThreadExListStream = 8,
        /// Large memory regions (64-bit addresses)
        ///
        /// Used for dumps with >4GB memory ranges
        Memory64ListStream = 9,
        /// User-defined comment in ANSI format
        CommentStreamA = 10,
        /// User-defined comment in Unicode format
        CommentStreamW = 11,
        /// Information about open handles
        ///
        /// Lists process/thread/file handles at crash time
        HandleDataStream = 12,
        /// Function table for stack unwinding
        ///
        /// Runtime function entries for exception handling
        FunctionTableStream = 13,
        /// List of modules that were unloaded before crash
        ///
        /// Helps diagnose use-after-unload bugs
        UnloadedModuleListStream = 14,
        /// Miscellaneous information
        ///
        /// Process ID, times, build string, and other metadata
        MiscInfoStream = 15,
        /// Detailed memory region information
        ///
        /// Contains protection flags, state, and type for each region
        MemoryInfoListStream = 16,
        /// Extended thread information
        ///
        /// Thread times, priorities, and additional attributes
        ThreadInfoListStream = 17,
        /// Handle operation tracking information
        ///
        /// Records of handle creation/closure operations
        HandleOperationListStream = 18,
        /// Security token information
        ///
        /// Process and thread security tokens
        TokenStream = 19,
        /// JavaScript engine data
        ///
        /// V8/Chakra runtime information for browser crashes
        JavaScriptDataStream = 20,
        /// System memory statistics
        ///
        /// Total/available physical and virtual memory
        SystemMemoryInfoStream = 21,
        /// Process virtual memory counters
        ///
        /// Working set, page faults, and memory usage stats
        ProcessVmCountersStream = 22,
        /// Intel Processor Trace data
        ///
        /// Hardware trace information for debugging
        IptTraceStream = 23,
        /// Names of threads
        ///
        /// See ['MINIDUMP_THREAD_NAME']
        ThreadNamesStream = 24,
        /// Reserved for future use
        LastReservedStream = 0xffff
    };

    explicit XMiniDump(QIODevice *pDevice = nullptr);
    virtual ~XMiniDump();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual FT getFileType() override;
    virtual QString getMIMEString() override;
    virtual QString getArch() override;
    virtual MODE getMode() override;
    virtual ENDIAN getEndian() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString getVersion() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;

    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    MINIDUMP_HEADER read_MINIDUMP_HEADER();
    MINIDUMP_DIRECTORY read_MINIDUMP_DIRECTORY(qint32 nIndex);
    QList<MINIDUMP_DIRECTORY> read_MINIDUMP_DIRECTORY_list(PDSTRUCT *pPdStruct);
    MINIDUMP_SYSTEM_INFO read_MINIDUMP_SYSTEM_INFO(qint64 nOffset);
    MINIDUMP_MODULE_LIST read_MINIDUMP_MODULE_LIST(qint64 nOffset);
    MINIDUMP_MODULE read_MINIDUMP_MODULE(qint64 nOffset);
    QString read_MINIDUMP_STRING(qint64 nOffset);
    QList<MINIDUMP_MODULE> read_MINIDUMP_MODULE_list(qint64 nOffset, PDSTRUCT *pPdStruct);
    
    MINIDUMP_DIRECTORY findStream(quint32 nStreamType, PDSTRUCT *pPdStruct);
    QString streamTypeToString(quint32 nStreamType);
    QString processorArchitectureToString(quint16 nArchitecture);
    static QMap<quint64, QString> getStreamTypes();
    static QMap<quint64, QString> getStreamTypesS();
    static QMap<quint64, QString> getProcessorArchitectures();

private:
    // Format-specific unpack context for streaming API
    struct MINIDUMP_UNPACK_CONTEXT {
        QList<qint64> listStreamOffsets;    // Pre-computed offsets for each stream
        QList<MINIDUMP_DIRECTORY> listDirectories;  // Cached directory entries
    };
};

#endif  // XMINIDUMP_H
