/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#ifndef XBZIP2_H
#define XBZIP2_H

#include "xarchive.h"

class XBZIP2 : public XArchive {
    Q_OBJECT

public:
    enum BZIP2_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_BZ2
    };

    struct BZIP2_HEADER {
        char magic[3];     // BZh
        quint8 blockSize;  // '1' - '9' (file-level compression)
    };

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_BZIP2_HEADER,
        STRUCTID_BLOCK_HEADER
    };

    explicit XBZIP2(QIODevice *pDevice = nullptr);
    ~XBZIP2();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    virtual MODE getMode();
    virtual qint32 getType();
    virtual QString typeIdToString(qint32 nType);
    virtual QString getFileFormatExt();
    virtual FT getFileType();
    virtual QString getFileFormatExtsString();
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct);
    virtual QString getMIMEString() override;
    virtual ENDIAN getEndian() override;
    virtual OSNAME getOsName();
    virtual QList<MAPMODE> getMapModesList();
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);
    virtual QString structIDToString(quint32 nID);
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct);
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct);
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct);

    BZIP2_HEADER _read_BZIP2_HEADER(qint64 nOffset);
};

#endif  // XBZIP2_H
