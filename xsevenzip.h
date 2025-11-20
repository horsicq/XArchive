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
#ifndef XSEVENZIP_H
#define XSEVENZIP_H

#include "xarchive.h"

// TODO https://py7zr.readthedocs.io/en/latest/archive_format.html
class XSevenZip : public XArchive {
    Q_OBJECT

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_SIGNATUREHEADER,
        STRUCTID_HEADER
    };

    enum EIdEnum {
        k7zIdEnd = 0,
        k7zIdHeader,
        k7zIdArchiveProperties,
        k7zIdAdditionalStreamsInfo,
        k7zIdMainStreamsInfo,
        k7zIdFilesInfo,
        k7zIdPackInfo,
        k7zIdUnpackInfo,
        k7zIdSubStreamsInfo,
        k7zIdSize,
        k7zIdCRC,
        k7zIdFolder,
        k7zIdCodersUnpackSize,
        k7zIdNumUnpackStream,
        k7zIdEmptyStream,
        k7zIdEmptyFile,
        k7zIdAnti,
        k7zIdName,
        k7zIdCTime,
        k7zIdATime,
        k7zIdMTime,
        k7zIdWinAttrib,
        k7zIdComment,
        k7zIdEncodedHeader,
        k7zIdStartPos,
        k7zIdDummy
        // k7zNtSecure,
        // k7zParent,
        // k7zIsReal
        // Test
    };
#pragma pack(push)
#pragma pack(1)
    struct SIGNATUREHEADER {
        quint8 kSignature[6];  // {'7','z',0xBC,0xAF,0x27,0x1C}
        quint8 Major;          // now = 0
        quint8 Minor;          // now = 4
        quint32 StartHeaderCRC;
        quint64 NextHeaderOffset;
        quint64 NextHeaderSize;
        quint32 NextHeaderCRC;
    };
#pragma pack(pop)

    explicit XSevenZip(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    virtual QString getVersion();
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct);
    virtual QString getFileFormatExt();
    virtual QString getFileFormatExtsString();
    virtual QList<MAPMODE> getMapModesList();
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    virtual qint64 getImageSize() override;
    virtual FT getFileType();
    virtual ENDIAN getEndian();
    virtual MODE getMode();
    virtual QString getArch();
    virtual QString getMIMEString();

    SIGNATUREHEADER _read_SIGNATUREHEADER(qint64 nOffset);
    static QString idToSring(EIdEnum id);
    static COMPRESS_METHOD codecToCompressMethod(const QByteArray &baCodec);
    static void _applyBCJFilter(QByteArray &baData, qint32 nOffset);

    static const QString PREFIX_k7zId;

    static QMap<quint64, QString> get_k7zId();
    static QMap<quint64, QString> get_k7zId_s();

    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    // Streaming packing API
    virtual bool initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool addDevice(PACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool addFile(PACK_STATE *pState, const QString &sFileName, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool addFolder(PACK_STATE *pState, const QString &sDirectoryPath, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishPack(PACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct SEVENZ_FOLDER_INFO {
        qint64 nStreamOffset;            // Offset to compressed stream
        qint64 nStreamSize;              // Size of compressed stream
        COMPRESS_METHOD compressMethod;  // Compression method
        COMPRESS_METHOD filterMethod;    // Filter method (BCJ, etc.)
        QByteArray baProperties;         // Compression properties
        qint64 nUnpackSize;              // Total uncompressed size of folder
        QList<qint32> listFileIndices;   // Indices of files in this folder
    };

    struct SEVENZ_UNPACK_CONTEXT {
        qint64 nSignatureSize;
        QList<qint64> listRecordOffsets;
        QList<ARCHIVERECORD> listArchiveRecords;  // Pre-parsed archive records
        QList<SEVENZ_FOLDER_INFO> listFolders;    // Folder (solid block) information
        QMap<qint32, qint32> mapFileToFolder;     // Maps file index to folder index
        QMap<qint32, QByteArray> mapFolderCache;  // Cache of decompressed folder data
        QString sPassword;                        // Archive password (if encrypted)
    };

    struct SEVENZ_PACK_CONTEXT {
        qint64 nHeaderOffset;
        QList<ARCHIVERECORD> listArchiveRecords;  // Records to pack
        QList<QByteArray> listCompressedData;     // Compressed data streams
        QList<quint32> listCRCs;                  // CRC values for streams
        COMPRESS_METHOD compressMethod;           // Compression method to use
        qint32 nCompressionLevel;                 // Compression level
    };

    // Helper functions for writing 7z format
    static QByteArray _writePackedNumber(quint64 nValue);
    static void _writeId(QIODevice *pDevice, quint8 nId);
    static void _writeNumber(QIODevice *pDevice, quint64 nValue);
    static void _writeByte(QIODevice *pDevice, quint8 nByte);

    enum SRTYPE {
        SRTYPE_UNKNOWN = 0,
        SRTYPE_ID,
        SRTYPE_NUMBER,
        SRTYPE_BYTE,
        SRTYPE_UINT32,
        SRTYPE_ARRAY
    };

    enum IMPTYPE {
        IMPTYPE_UNKNOWN = 0,
        IMPTYPE_NUMBEROFFILES,  // Number of files in archive
        IMPTYPE_STREAMCRC,
        IMPTYPE_STREAMOFFSET,
        IMPTYPE_STREAMPACKEDSIZE,
        IMPTYPE_STREAMUNPACKEDSIZE,
        IMPTYPE_NUMBEROFSTREAMS,
        IMPTYPE_CODER,
        IMPTYPE_CODERPROPERTY,
        IMPTYPE_FILENAME,
        IMPTYPE_FILECRC,
        IMPTYPE_FILEATTRIBUTES,
        IMPTYPE_FILETIME,
        IMPTYPE_FILEPACKEDSIZE,
        IMPTYPE_FILEUNPACKEDSIZE,
        IMPTYPE_NUMUNPACKSTREAM  // Number of files in each folder (NumUnpackStream)
    };

    struct SZRECORD {
        qint32 nRelOffset;
        qint32 nSize;
        QString sName;
        QVariant varValue;
        SRTYPE srType;
        VT valType;
        quint32 nFlags;
        IMPTYPE impType;
    };

    struct SZSTATE {
        char *pData;
        qint64 nCurrentOffset;
        qint64 nSize;
        bool bIsError;
        QString sErrorString;
        quint64 nNumberOfFolders;  // Track folder count for SubStreamsInfo
    };

    QList<SZRECORD> _handleData(char *pData, qint64 nSize, PDSTRUCT *pPdStruct);
    bool _handleId(QList<SZRECORD> *pListRecords, EIdEnum id, SZSTATE *pState, qint32 nCount, bool bCheck, PDSTRUCT *pPdStruct, IMPTYPE impType);
    quint64 _handleNumber(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, quint32 nFlags, IMPTYPE impType);
    quint8 _handleByte(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType);
    quint32 _handleUINT32(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType);
    void _handleArray(QList<SZRECORD> *pListRecords, SZSTATE *pState, qint64 nSize, PDSTRUCT *pPdStruct, const QString &sCaption, IMPTYPE impType);
};

#endif  // XSEVENZIP_H
