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
#ifndef XSQUASHFS_H
#define XSQUASHFS_H

#include "xarchive.h"

class XSquashfs : public XArchive {
    Q_OBJECT

#pragma pack(push)
#pragma pack(1)
    struct SQUASHFS_HEADER {
        quint32 nMagic;             // Magic number: 0x73717368 ("sqsh")
        quint32 nInodesCount;       // Number of inodes
        quint32 nCreationTime;      // Creation time (seconds since epoch)
        quint32 nBlockSize;         // Block size (power of 2)
        quint32 nFragmentsCount;    // Number of fragments
        quint16 nCompressionType;   // Compression method
        quint16 nBlockLog;          // Log2 of block size
        quint16 nFlags;             // Flags
        quint16 nNoIds;             // Number of unique IDs
        quint32 nVersionMajor;      // Version major
        quint32 nVersionMinor;      // Version minor
        quint64 nRootInodeRef;      // Root inode reference
        quint64 nBytesUsed;         // Bytes used
        quint64 nIdTableStart;      // ID table start block
        quint64 nXattrTableStart;   // Xattr table start block
        quint64 nInodeTableStart;   // Inode table start block
        quint64 nDirectoryTableStart; // Directory table start block
        quint64 nFragmentTableStart;  // Fragment table start block
        quint64 nExportTableStart;  // Export table start block
    };
#pragma pack(pop)

    enum SQUASHFS_COMPRESSION {
        COMP_UNKNOWN = 0,
        COMP_GZIP = 1,
        COMP_LZMA = 2,
        COMP_LZO = 3,
        COMP_XZ = 4,
        COMP_LZ4 = 5,
        COMP_ZSTD = 6
    };

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER,
        STRUCTID_SUPERBLOCK
    };

    explicit XSquashfs(QIODevice *pDevice = nullptr);
    ~XSquashfs();

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

    SQUASHFS_HEADER _readHeader(qint64 nOffset);

private:
    SQUASHFS_COMPRESSION _getCompressionMethod(quint16 nType);
    QString _getCompressionMethodString(SQUASHFS_COMPRESSION comp);
};

#endif  // XSQUASHFS_H
