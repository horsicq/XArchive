/* Copyright (c) 2025 hors<horsicq@gmail.com>
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

    XXZ(QIODevice *pDevice = nullptr);
    ~XXZ();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    virtual FT getFileType();
    virtual MODE getMode();
    virtual QString getMIMEString();
    virtual qint32 getType();
    virtual QString typeIdToString(qint32 nType);
    virtual ENDIAN getEndian();
    virtual QString getArch();
    virtual QString getFileFormatExt();
    virtual QString getFileFormatExtsString();
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct);
    virtual bool isSigned();
    virtual OSNAME getOsName();
    virtual QString getOsVersion();
    virtual QString getVersion();
    virtual QString getInfo();
    virtual bool isEncrypted();
    virtual QList<MAPMODE> getMapModesList();
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);

    virtual QList<HREGION> getHData(PDSTRUCT *pPdStruct = nullptr);

    virtual QString structIDToString(quint32 nID);
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct);
    virtual qint32 readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<QVariant> *pListValues,
                                PDSTRUCT *pPdStruct);

    STREAM_HEADER _read_STREAM_HEADER(qint64 nOffset);
    STREAM_FOOTER _read_STREAM_FOOTER(qint64 nOffset);
    BLOCK_HEADER _read_BLOCK_HEADER(qint64 nOffset);
    INDEX _read_INDEX(qint64 nOffset);

    // XArchive interface
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct);
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct);
};

#endif  // XXZ_H