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
#ifndef XISO9660_H
#define XISO9660_H

#include "xarchive.h"

class XISO9660 : public XArchive {
    Q_OBJECT

#pragma pack(push)
#pragma pack(1)
    struct ISO9660_PVDESC {
        quint8 nDescType;           // Descriptor type: 1 for PVD
        char szStandard[5];         // Standard identifier: "CD001"
        quint8 nDescVersion;        // Descriptor version
        quint8 nUnused1;            // Unused
        char szSystemId[32];        // System identifier
        char szVolumeId[32];        // Volume identifier
        quint8 nUnused2[8];         // Unused
        quint32 nLogicalBlockSize;  // Logical block size (little/big endian)
        quint32 nBigEndianBlockSize;
        quint32 nPathTableSize;  // Path table size (little/big endian)
        quint32 nBigEndianPathTableSize;
        quint32 nRootDirExtent;  // Logical block number of root directory
        quint32 nBigEndianRootDirExtent;
        char szVolSetId[128];         // Volume set identifier
        char szPublisherId[128];      // Publisher identifier
        char szDataPreparerId[128];   // Data preparer identifier
        char szApplicationId[128];    // Application identifier
        char szCopyrightFile[37];     // Copyright file identifier
        char szAbstractFile[36];      // Abstract file identifier
        char szBiblioFile[37];        // Bibliographic file identifier
        char szCreationTime[17];      // Creation time
        char szModificationTime[17];  // Modification time
        char szExpirationTime[17];    // Expiration time
        char szEffectiveTime[17];     // Effective time
        quint8 nFileStructVersion;    // File structure version
        quint8 nUnused3;              // Unused
        quint8 nAppData[512];         // Application-specific data
        quint8 nUnused4[653];         // Unused
    };

    struct ISO9660_DIR_RECORD {
        quint8 nLength;             // Directory record length
        quint8 nExtentAttrLength;   // Extended attribute record length
        quint32 nExtentLocationLE;  // Logical block number (little-endian)
        quint32 nExtentLocationBE;  // Logical block number (big-endian)
        quint32 nDataLengthLE;      // File size (little-endian)
        quint32 nDataLengthBE;      // File size (big-endian)
        quint8 nYear;               // Year (offset from 1900)
        quint8 nMonth;              // Month
        quint8 nDay;                // Day
        quint8 nHour;               // Hour
        quint8 nMinute;             // Minute
        quint8 nSecond;             // Second
        quint8 nGMTOffset;          // GMT offset
        quint8 nFileFlags;          // File flags
        quint8 nFileUnitSize;       // File unit size
        quint8 nInterleaveGapSize;  // Interleave gap size
        quint32 nSequenceNumberLE;  // Volume sequence number (little-endian)
        quint32 nSequenceNumberBE;  // Volume sequence number (big-endian)
        quint8 nFileIdLength;       // Length of file identifier
        // Followed by file identifier and padding
    };
#pragma pack(pop)

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_PVDESC,
        STRUCTID_DIR_RECORD
    };

    explicit XISO9660(QIODevice *pDevice = nullptr);
    ~XISO9660();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString getMIMEString() override;
    virtual FT getFileType() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    ISO9660_PVDESC _readPrimaryVolumeDescriptor(qint64 nOffset);

private:
    struct ISO9660_SCAN_CONTEXT {
        qint32 nLogicalBlockSize;
        qint64 nRootDirOffset;
        qint64 nRootDirSize;
    };

    struct ISO9660_UNPACK_CONTEXT {
        qint32 nLogicalBlockSize;
        QList<QPair<qint64, qint64>> listDirStack;   // Stack of (offset, size) for directories to process
        QList<QString> listPathStack;                // Stack of folder paths corresponding to directories
        QString sCurrentPath;                        // Current folder path (e.g., "FolderName1/FolderName2")
        QList<ARCHIVERECORD> listCurrentDirRecords;  // Records in current directory
        qint32 nCurrentRecordIndex;                  // Index in current directory
        QSet<qint64> setProcessedBlocks;             // Track processed directory blocks to avoid loops
    };

    qint32 _getLogicalBlockSize();
    qint64 _getPrimaryVolumeDescriptorOffset();
    bool _isValidDescriptor(qint64 nOffset, PDSTRUCT *pPdStruct);
    QList<ARCHIVERECORD> _parseDirectory(qint64 nOffset, qint64 nSize, qint32 nBlockSize, const QString &sParentPath, PDSTRUCT *pPdStruct);
    qint64 _countAllRecords(qint64 nRootOffset, qint64 nRootSize, qint32 nBlockSize, PDSTRUCT *pPdStruct);
    QString _cleanFileName(const QString &sFileName);
};

#endif  // XISO9660_H
