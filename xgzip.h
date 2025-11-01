/* Copyright (c) 2022-2025 hors<horsicq@gmail.com>
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
#ifndef XGZIP_H
#define XGZIP_H

#include "xarchive.h"

class XGzip : public XArchive {
    Q_OBJECT
public:
    enum GZIP_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_GZ
    };

#pragma pack(push)
#pragma pack(1)
    struct GZIP_HEADER {
        quint16 nMagic;             // 0x1f 0x8b	Magic number identifying file type
        quint8 nCompressionMethod;  // Compression Method * 0-7 (Reserved) * 8 (Deflate)
        quint8 nFileFlags;          // File Flags
        quint32 nTimeStamp;         // 32-bit timestamp
        quint8 nCompressionFlags;   // Compression flags
        quint8 nOS;                 // Operating system ID
    };
#pragma pack(pop)

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_GZIP_HEADER,
        STRUCTID_STREAM
    };

    explicit XGzip(QIODevice *pDevice = nullptr);
    ~XGzip();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual MODE getMode() override;
    virtual qint32 getType() override;
    virtual QString typeIdToString(qint32 nType) override;
    virtual QString getFileFormatExt() override;
    virtual FT getFileType() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    virtual QString getMIMEString() override;
    virtual ENDIAN getEndian() override;
    virtual OSNAME getOsName() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) override;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

    GZIP_HEADER _read_GZIP_HEADER(qint64 nOffset);

private:
    // Format-specific unpacking context
    struct GZIP_UNPACK_CONTEXT {
        qint64 nHeaderSize;        // Size of GZIP header (variable)
        qint64 nCompressedSize;    // Size of compressed data
        qint64 nUncompressedSize;  // Size of uncompressed data
        QString sFileName;         // Original file name (if available)
    };
};

#endif  // XGZIP_H
