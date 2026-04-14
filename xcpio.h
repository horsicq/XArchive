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
#ifndef XCPIO_H
#define XCPIO_H

#include "xarchive.h"

class XCPIO : public XArchive {
    Q_OBJECT

#pragma pack(push)
#pragma pack(1)
    struct CPIO_NEWC_HEADER {
        char magic[6];      // "070701" or "070702"
        char ino[8];        // Inode number
        char mode[8];       // File mode
        char uid[8];        // User ID
        char gid[8];        // Group ID
        char nlink[8];      // Number of hard links
        char mtime[8];      // Modification time
        char filesize[8];   // File size
        char devmajor[8];   // Device major number
        char devminor[8];   // Device minor number
        char rdevmajor[8];  // Special device major number
        char rdevminor[8];  // Special device minor number
        char namesize[8];   // Filename size
        char check[8];      // Checksum
    };

    struct CPIO_ODC_HEADER {
        char magic[6];      // "070707"
        char dev[6];        // Device number
        char ino[6];        // Inode number
        char mode[6];       // File mode
        char uid[6];        // User ID
        char gid[6];        // Group ID
        char nlink[6];      // Number of hard links
        char rdev[6];       // Special device number
        char mtime[11];     // Modification time
        char namesize[6];   // Filename size
        char filesize[11];  // File size
    };

    struct CPIO_BINARY_HEADER {
        quint16 magic;       // 070707 in host byte order
        quint16 dev;         // Device number
        quint16 ino;         // Inode number
        quint16 mode;        // File mode
        quint16 uid;         // User ID
        quint16 gid;         // Group ID
        quint16 nlink;       // Number of hard links
        quint16 rdev;        // Special device number
        quint16 mtimeHigh;   // High 16 bits of modification time
        quint16 mtimeLow;    // Low 16 bits of modification time
        quint16 namesize;    // Filename size
        quint16 filesizeHigh;  // High 16 bits of file size
        quint16 filesizeLow;   // Low 16 bits of file size
    };
#pragma pack(pop)

    enum CPIO_FORMAT {
        CPIO_FORMAT_UNKNOWN = 0,
        CPIO_FORMAT_NEWC,  // new C format (070701)
        CPIO_FORMAT_CRC,   // CRC format (070702)
        CPIO_FORMAT_ODC,   // Old ASCII C format (070707)
        CPIO_FORMAT_BINARY_LE,
        CPIO_FORMAT_BINARY_BE
    };

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_NEWC_HEADER,
        STRUCTID_CRC_HEADER,
        STRUCTID_ODC_HEADER,
        STRUCTID_BINARY_HEADER
    };

    explicit XCPIO(QIODevice *pDevice = nullptr);
    ~XCPIO();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual QString getMIMEString() override;
    virtual FT getFileType() override;
    virtual ENDIAN getEndian() override;
    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) override;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CPIO_UNPACK_CONTEXT {
        CPIO_FORMAT format;
        qint64 nHeaderSize;
        QList<RECORD> listRecords;
        qint32 nCurrentRecord;
    };

    struct CPIO_RECORD_INFO {
        CPIO_FORMAT format;
        qint64 nHeaderOffset;
        qint64 nHeaderSize;
        qint64 nDataOffset;
        qint64 nDataSize;
        qint64 nNextOffset;
        QString sFileName;
        quint32 nMode;
        quint32 nUID;
        quint32 nGID;
        quint32 nNLink;
        quint32 nRDev;
        quint64 nMTime;
        bool bIsFolder;
    };

    CPIO_FORMAT _detectFormat(qint64 nOffset);
    qint64 _readHexValue(const char *pValue, qint32 nSize);
    qint64 _readOctValue(const char *pValue, qint32 nSize);
    quint16 _readBinaryUInt16(qint64 nOffset, bool bIsBigEndian);
    quint32 _readBinaryUInt32(qint64 nOffset, bool bIsBigEndian);
    CPIO_NEWC_HEADER _readNewcHeader(qint64 nOffset);
    CPIO_ODC_HEADER _readOdcHeader(qint64 nOffset);
    bool _parseRecord(qint64 nOffset, CPIO_RECORD_INFO *pInfo);
    bool _isTrailerRecord(const QString &sFileName);
};

#endif  // XCPIO_H

