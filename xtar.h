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
#ifndef XTAR_H
#define XTAR_H

#include "xarchive.h"
#include "xgzip.h"

class XTAR : public XArchive {
    Q_OBJECT

#pragma pack(push)
#pragma pack(1)
    struct posix_header {   /* byte offset */
        char name[100];     /*   0 */
        char mode[8];       /* 100 */
        char uid[8];        /* 108 */
        char gid[8];        /* 116 */
        char size[12];      /* 124 */
        char mtime[12];     /* 136 */
        char chksum[8];     /* 148 */
        char typeflag[1];   /* 156 */
        char linkname[100]; /* 157 */
        char magic[6];      /* 257 */
        char version[2];    /* 263 */
        char uname[32];     /* 265 */
        char gname[32];     /* 297 */
        char devmajor[8];   /* 329 */
        char devminor[8];   /* 337 */
        char prefix[155];   /* 345 */
                            /* 500 */
    };
#pragma pack(pop)

    enum TAR_FORMAT {
        TAR_FORMAT_DEFAULT = 0,
        TAR_FORMAT_POSIX,  // POSIX ustar format (default)
        TAR_FORMAT_GNU     // GNU tar format (supports longer filenames)
    };

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_POSIX_HEADER
    };

    struct T_UNPACK_CONTEXT {
        QIODevice *pDevice;
        XTAR *pTar;
        UNPACK_STATE usTar;
    };

    explicit XTAR(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    bool _isValid(_MEMORY_MAP *pMemoryMap, qint64 nOffset, PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual FT getFileType() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString getMIMEString() override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual qint32 readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords,
                                void *pUserData, PDSTRUCT *pPdStruct) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    // Streaming packing API
    virtual bool initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool addFile(PACK_STATE *pState, const QString &sFileName, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool addFolder(PACK_STATE *pState, const QString &sDirectoryPath, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishPack(PACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    static bool sub_initUnpack(FT fileType, QIODevice *pDevice, UNPACK_STATE *pUnpackState, const QMap<UNPACK_PROP, QVariant> &mapProperties,
                               PDSTRUCT *pPdStruct = nullptr);
    static ARCHIVERECORD sub_infoCurrent(FT fileType, UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct = nullptr);
    static bool sub_unpackCurrent(FT fileType, UNPACK_STATE *pUnpackState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    static bool sub_moveToNext(FT fileType, UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct = nullptr);
    static bool sub_finishUnpack(FT fileType, UNPACK_STATE *pUnpackState, PDSTRUCT *pPdStruct = nullptr);

private:
    posix_header read_posix_header(qint64 nOffset);
    qint32 _getNumberOf_posix_headers(qint64 nOffset, PDSTRUCT *pPdStruct);
    qint64 _getSize(const posix_header &header);
    static posix_header createHeader(const QString &sFileName, const QString &sBasePath, qint64 nFileSize, quint32 nMode, qint64 nMTime);
    static quint32 calculateChecksum(const posix_header &header);
    static void writeOctal(char *pDest, qint32 nSize, qint64 nValue);

signals:
};

#endif  // XTAR_H
