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
#ifndef XXAR_H
#define XXAR_H

#include "xarchive.h"

// XAR (eXtensible ARchive), used by macOS .pkg installers. Header (big-endian):
// magic 'xar!', header_size(2), version(2), toc_compressed(8), toc_uncompressed(8),
// checksum_alg(4). The TOC is a zlib-compressed XML document; the heap of file
// data follows it. File offsets in the TOC are relative to the heap start.
class XXAR : public XArchive {
    Q_OBJECT

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct XAR_HEADER {
        quint8 magic[4];  // 'xar!'
        quint16 header_size;
        quint16 version;
        quint64 toc_length_compressed;
        quint64 toc_length_uncompressed;
        quint32 cksum_alg;
    };
#pragma pack(pop)

    explicit XXAR(QIODevice *pDevice = nullptr);

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

    virtual bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override;
    virtual ARCHIVERECORD infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;
    virtual bool finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct = nullptr) override;

private:
    struct XAR_RECORD {
        QString sFileName;
        qint64 nOffset;   // absolute (heap-relative + heap start)
        qint64 nLength;   // stored (compressed) size on disk
        qint64 nSize;     // uncompressed size
        HANDLE_METHOD compressMethod;
        bool bIsFolder;
    };

    struct XAR_UNPACK_CONTEXT {
        QList<XAR_RECORD> listRecords;
    };

    qint64 _getHeapOffset();
    QByteArray _readTOC(PDSTRUCT *pPdStruct);
    bool _parseTOC(const QByteArray &baXML, qint64 nHeapOffset, QList<XAR_RECORD> *pListRecords);
    HANDLE_METHOD _encodingToMethod(const QString &sStyle);
};

#endif  // XXAR_H
