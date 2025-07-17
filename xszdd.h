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
#ifndef XSZDD_H
#define XSZDD_H

#include "xarchive.h"

class XSZDD : public XArchive {
    Q_OBJECT
public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_SZDD_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct SZDD_HEADER {
        char signature[8];          // "SZDD" magic
        quint8 compression_mode;    // Always 0x41 ("A")
        quint8 missing_char;        // Character missing from end of filename (0=unknown)
        quint32 uncompressed_size;  // Uncompressed file size
    };
#pragma pack(pop)
    explicit XSZDD(QIODevice *pDevice = nullptr);
    ~XSZDD() override;

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    virtual FT getFileType() override;
    virtual MODE getMode() override;
    virtual QString getMIMEString() override;
    virtual qint32 getType() override;
    virtual ENDIAN getEndian() override;
    virtual QString getArch() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString();
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct) override;
    virtual bool isSigned() override;
    virtual OSNAME getOsName() override;
    virtual QString getOsVersion() override;
    virtual QString getVersion() override;
    virtual QString getInfo() override;
    virtual bool isEncrypted() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QList<HREGION> getHData(PDSTRUCT *pPdStruct = nullptr) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;

    SZDD_HEADER _read_SZDD_HEADER(qint64 nOffset);

    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct) override;
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct) override;
};

#endif  // XSZDD_H
