/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#ifndef XCOMPRESSZ_H
#define XCOMPRESSZ_H

#include "xarchive.h"

class XCompressZ : public XArchive {
    Q_OBJECT

public:
    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;
    enum COMPRESSZ_TYPE {
        TYPE_UNKNOWN = 0,
        TYPE_Z
    };

    struct COMPRESSZ_HEADER {
        quint8 nMagic0;  // 0x1F
        quint8 nMagic1;  // 0x9D
        quint8 nFlags;   // bit 7 = block_compress, bits 0-4 = max_bits
    };

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_COMPRESSZ_HEADER
    };

    explicit XCompressZ(QIODevice *pDevice = nullptr);
    ~XCompressZ();

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
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
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    // Streaming unpacking API
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual QList<FPART_PROP> getAvailableFPARTProperties() override;

    COMPRESSZ_HEADER _read_COMPRESSZ_HEADER(qint64 nOffset);

private:
    struct COMPRESSZ_UNPACK_CONTEXT {
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        QString sFileName;
    };
};

#endif  // XCOMPRESSZ_H

