/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#ifndef XRAR_H
#define XRAR_H

#include "xarchive.h"

class XRar : public XArchive {
    Q_OBJECT

    const quint16 RAR4_FILE_LARGE = 0x0100;
    const quint16 RAR4_FILE_UNICODE_FILENAME = 0x0200;
    const quint16 RAR4_FILE_SALT = 0x0400;
    const quint16 RAR4_FILE_EXT_TIME = 0x1000;
    const quint16 RAR4_FILE_COMMENT = 0x0008;

    const quint8 RAR_OS_MSDOS = 0;  // MS-DOS
    const quint8 RAR_OS_OS2 = 1;    // OS/2
    const quint8 RAR_OS_WIN32 = 2;  // Windows
    const quint8 RAR_OS_UNIX = 3;   // Unix/Linux
    const quint8 RAR_OS_MACOS = 4;  // Mac OS
    const quint8 RAR_OS_BEOS = 5;   // BeOS

    // RAR 5.0 hostOS values
    const quint8 RAR5_OS_WINDOWS = 0;  // Windows
    const quint8 RAR5_OS_UNIX = 1;     // Unix/Linux

    const quint8 RAR_METHOD_STORE = 0x30;    // Storing without compression
    const quint8 RAR_METHOD_FASTEST = 0x31;  // Fastest compression
    const quint8 RAR_METHOD_FAST = 0x32;     // Fast compression
    const quint8 RAR_METHOD_NORMAL = 0x33;   // Normal compression (default)
    const quint8 RAR_METHOD_GOOD = 0x34;     // Good compression
    const quint8 RAR_METHOD_BEST = 0x35;     // Best compression

    const quint8 RAR5_METHOD_STORE = 0x00;    // RAR 5.0 storing without compression
    const quint8 RAR5_METHOD_FASTEST = 0x01;  // RAR 5.0 fastest compression
    const quint8 RAR5_METHOD_FAST = 0x02;     // RAR 5.0 fast compression
    const quint8 RAR5_METHOD_NORMAL = 0x03;   // RAR 5.0 normal compression
    const quint8 RAR5_METHOD_GOOD = 0x04;     // RAR 5.0 good compression
    const quint8 RAR5_METHOD_BEST = 0x05;     // RAR 5.0 best compression

    struct GENERICBLOCK4 {
        quint16 nCRC16;
        quint8 nType;
        quint16 nFlags;
        quint16 nHeaderSize;
    };

    struct FILEBLOCK4 {
        GENERICBLOCK4 genericBlock4;
        quint32 packSize;      // Packed file size
        quint32 unpSize;       // Unpacked file size
        quint8 hostOS;         // Operating system used for archiving
        quint32 fileCRC;       // File CRC
        quint32 fileTime;      // Date and time in standard MS-DOS format
        quint8 unpVer;         // RAR version needed to extract file
        quint8 method;         // Packing method
        quint16 nameSize;      // Size of filename field
        quint32 fileAttr;      // File attributes
        quint32 highPackSize;  // High 4 bytes of 64-bit value of packed file size
        quint32 highUnpSize;   // High 4 bytes of 64-bit value of unpacked file size
        QString sFileName;
    };

    struct GENERICHEADER5 {
        quint32 nCRC32;
        quint64 _nHeaderSize;
        quint64 nHeaderSize;
        quint64 nType;
        quint64 nFlags;
        quint64 nExtraAreaSize;
        quint64 nDataSize;
    };

    struct FILEHEADER5 {
        quint32 nCRC32;          // Header CRC32
        quint64 _nHeaderSize;    // Internal variable for header size
        quint64 nHeaderSize;     // Size of the header
        quint64 nType;           // Header type (2 for file header, 3 for service header)
        quint64 nFlags;          // Common header flags
        quint64 nExtraAreaSize;  // Size of extra area (if 0x0001 flag set)
        quint64 nDataSize;       // Size of data area (if 0x0002 flag set)
        quint64 nFileFlags;      // Flags specific for file/service headers
        quint64 nUnpackedSize;   // Unpacked file or service data size
        quint64 nAttributes;     // OS-specific file attributes
        quint32 nMTime;          // File modification time (Unix format, if 0x0002 file flag set)
        quint32 nDataCRC32;      // CRC32 of unpacked data (if 0x0004 file flag set)
        quint64 nCompInfo;       // Compression algorithm information
        quint64 nHostOS;         // Type of OS used to create the archive
        quint64 nNameLength;     // Length of name field
        QString sFileName;       // File or service name
        QByteArray baExtraArea;  // Optional extra area (if 0x0001 header flag set)
        QByteArray baDataArea;   // Optional data area (if 0x0002 header flag set)
    };

    enum BLOCKTYPE4 {
        BLOCKTYPE4_MARKER = 0x72,        // Marker block
        BLOCKTYPE4_ARCHIVE = 0x73,       // Archive header
        BLOCKTYPE4_FILE = 0x74,          // File header
        BLOCKTYPE4_COMMENT = 0x75,       // Comment header
        BLOCKTYPE4_EXTRA = 0x76,         // Extra information
        BLOCKTYPE4_SUBBLOCK = 0x77,      // Subblock
        BLOCKTYPE4_RECOVERY = 0x78,      // Recovery record
        BLOCKTYPE4_AUTH = 0x79,          // Archive authentication
        BLOCKTYPE4_SUBBLOCK_NEW = 0x7A,  // Subblock for new-format file data
        BLOCKTYPE4_END = 0x7B            // End of archive
    };

    enum HEADERTYPE5 {
        HEADERTYPE5_MAIN = 1,        // Main archive header
        HEADERTYPE5_FILE = 2,        // File header
        HEADERTYPE5_SERVICE = 3,     // Service header
        HEADERTYPE5_ENCRYPTION = 4,  // Archive encryption header
        HEADERTYPE5_ENDARC = 5,      // End of archive header
    };

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_RAR14_SIGNATURE,
        STRUCTID_RAR40_SIGNATURE,
        STRUCTID_RAR50_SIGNATURE,
        STRUCTID_RAR14_MAINHEADER,
        STRUCTID_RAR40_MAINHEADER,
        STRUCTID_RAR50_MAINHEADER,
    };

    explicit XRar(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    virtual QString getVersion();
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct);
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct);

    virtual QString getFileFormatExt();
    virtual QString getFileFormatExtsString();
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct);

    virtual QList<MAPMODE> getMapModesList();
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);
    virtual FT getFileType();

    QString blockType4ToString(BLOCKTYPE4 type);
    QString headerType5ToString(HEADERTYPE5 type);

    virtual FILEFORMATINFO getFileFormatInfo(PDSTRUCT *pPdStruct);

    virtual QString getMIMEString();

    virtual QString structIDToString(quint32 nID);
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct);

private:
    qint32 getInternVersion(PDSTRUCT *pPdStruct);
    GENERICHEADER5 readGenericHeader5(qint64 nOffset);
    GENERICBLOCK4 readGenericBlock4(qint64 nOffset);
    FILEBLOCK4 readFileBlock4(qint64 nOffset);
    FILEHEADER5 readFileHeader5(qint64 nOffset);
    QString decodeRarUnicodeName(const QByteArray &nameData);
};

#endif  // XRAR_H
