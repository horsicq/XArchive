/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef XXZ_H
#define XXZ_H

#include "xarchive.h"
#include "xlzmadecoder.h"

class XXZ : public XArchive {
    Q_OBJECT
public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_STREAM_HEADER,
        STRUCTID_BLOCK_HEADER,
        STRUCTID_INDEX,
        STRUCTID_STREAM_FOOTER,
        STRUCTID_RECORD,
    };

    struct STREAM_HEADER {
        quint8 header_magic[6];
        quint8 stream_flags[2];
        quint32 crc32;
    };

    struct STREAM_FOOTER {
        quint32 crc32;
        quint32 backward_size;
        quint8 stream_flags[2];
        quint8 footer_magic[2];
    };

    struct BLOCK_HEADER {
        quint8 header_size;
        quint8 flags;
        // Compressed and Uncompressed sizes are variable-size, not fixed!
        // Filter info, etc., not parsed here for brevity
    };

    struct INDEX {
        quint8 indicator;
        quint64 num_records;
        // Records array, variable size, not included here
    };

    // Add more format data structs as needed

    explicit XXZ(QIODevice *pDevice = nullptr);
    ~XXZ();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice);
    virtual FT getFileType() override;
    virtual MODE getMode() override;
    virtual QString getMIMEString() override;
    virtual qint32 getType() override;
    virtual QString typeIdToString(qint32 nType) override;
    virtual ENDIAN getEndian() override;
    virtual QString getArch() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    virtual bool isSigned() override;
    virtual OSNAME getOsName() override;
    virtual QString getOsVersion() override;
    virtual QString getVersion() override;
    virtual bool isEncrypted() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;

    STREAM_HEADER _read_STREAM_HEADER(qint64 nOffset);
    STREAM_FOOTER _read_STREAM_FOOTER(qint64 nOffset);
    BLOCK_HEADER _read_BLOCK_HEADER(qint64 nOffset);
    INDEX _read_INDEX(qint64 nOffset);

    // XArchive interface
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) override;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct XXZ_UNPACK_CONTEXT {
        QString sFileName;
        qint64 nHeaderSize;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint32 nCRC32;
    };
};

#endif  // XXZ_H
