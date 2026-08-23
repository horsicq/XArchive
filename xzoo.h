/* Copyright (c) 2026 hors<horsicq@gmail.com>
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
#ifndef XZOO_H
#define XZOO_H

#include "xarchive.h"

// ZOO archive (Rahul Dhesi's zoo). Header: a ~20-byte text banner, magic
// 0xFDC4A7DC at offset 20, then the offset of the first directory entry. Files
// form a singly linked chain of directory entries (each also tagged with the
// magic). Compression method: 0 = stored, 1 = LZD (LZW), 2 = LZH (lh5-style).
class XZOO : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER
    };

    enum METHOD {
        METHOD_STORE = 0,
        METHOD_LZD = 1,
        METHOD_LZH = 2
    };

    explicit XZOO(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr) override;
    static bool isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct = nullptr);
    virtual FT getFileType() override;
    virtual MODE getMode() override;
    virtual QString getMIMEString() override;
    virtual qint32 getType() override;
    virtual ENDIAN getEndian() override;
    virtual QString getArch() override;
    virtual QString getFileFormatExt() override;
    virtual QString getFileFormatExtsString() override;
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct = nullptr) override;
    virtual OSNAME getOsName() override;
    virtual QString getVersion() override;
    virtual QList<MAPMODE> getMapModesList() override;
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QString structIDToString(quint32 nID) override;
    virtual QString structIDToFtString(quint32 nID) override;
    virtual quint32 ftStringToStructID(const QString &sFtString) override;
    virtual QList<XFHEADER> getXFHeaders(const XFSTRUCT &xfStruct, PDSTRUCT *pPdStruct) override;
    virtual QList<XFRECORD> getXFRecords(FT fileType, quint32 nStructID, const XLOC &xLoc) override;
    virtual QList<FPART> getFileParts(quint32 nFileParts, qint32 nLimit = -1, PDSTRUCT *pPdStruct = nullptr) override;

    virtual QList<QString> getSearchSignatures() override;
    virtual XBinary *createInstance(QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1) override;

    virtual QMap<UNPACK_PROP, QVariant> getDefaultUnpackProperties() override;
    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    static const quint32 ZOO_MAGIC = 0xFDC4A7DC;

    struct ZOO_RECORD {
        QString sFileName;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        quint16 nCRC16;
        quint8 nMethod;
        bool bIsFolder;
        QDateTime mtDateTime;
    };

    struct ZOO_UNPACK_CONTEXT {
        QList<ZOO_RECORD> listRecords;
    };

    HANDLE_METHOD _methodToHandle(quint8 nMethod);
    bool _parseEntries(QList<ZOO_RECORD> *pListRecords, PDSTRUCT *pPdStruct);
private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XZOO_H
