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
#ifndef XCPIO_H
#define XCPIO_H

#include "xarchive.h"

class XCPIO : public XArchive {
    Q_OBJECT

#pragma pack(push)
#pragma pack(1)
    struct CPIO_NEWC_HEADER {
        char magic[6];              // "070701" or "070702"
        char ino[8];                // Inode number
        char mode[8];               // File mode
        char uid[8];                // User ID
        char gid[8];                // Group ID
        char nlink[8];              // Number of hard links
        char mtime[8];              // Modification time
        char filesize[8];           // File size
        char devmajor[8];           // Device major number
        char devminor[8];           // Device minor number
        char rdevmajor[8];          // Special device major number
        char rdevminor[8];          // Special device minor number
        char namesize[8];           // Filename size
        char check[8];              // Checksum
    };

    struct CPIO_ODC_HEADER {
        char magic[6];              // "070707"
        char ino[6];                // Inode number
        char mode[6];               // File mode
        char uid[6];                // User ID
        char gid[6];                // Group ID
        char nlink[6];              // Number of hard links
        char mtime[11];             // Modification time
        char filesize[11];          // File size
        char devmajor[6];           // Device major number
        char devminor[6];           // Device minor number
        char rdevmajor[6];          // Special device major number
        char rdevminor[6];          // Special device minor number
        char namesize[6];           // Filename size
        char check[11];             // Checksum
    };
#pragma pack(pop)

    enum CPIO_FORMAT {
        CPIO_FORMAT_UNKNOWN = 0,
        CPIO_FORMAT_NEWC,           // new C format (070701)
        CPIO_FORMAT_CRC,            // CRC format (070702)
        CPIO_FORMAT_ODC             // Old C format (070707)
    };

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_NEWC_HEADER,
        STRUCTID_CRC_HEADER,
        STRUCTID_ODC_HEADER
    };

    explicit XCPIO(QIODevice *pDevice = nullptr);
    ~XCPIO();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual QString getMIMEString() override;
    virtual FT getFileType() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) override;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct CPIO_UNPACK_CONTEXT {
        CPIO_FORMAT format;
        qint64 nHeaderSize;
    };

    CPIO_FORMAT _detectFormat(qint64 nOffset);
    qint64 _readHexValue(const char *pValue, qint32 nSize);
    CPIO_NEWC_HEADER _readNewcHeader(qint64 nOffset);
    CPIO_ODC_HEADER _readOdcHeader(qint64 nOffset);
    bool _isTrailerRecord(const QString &sFileName);
};

#endif  // XCPIO_H
