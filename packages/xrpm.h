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
#ifndef XRPM_H
#define XRPM_H

#include "xarchive.h"

// RPM package (RedHat Package Manager). Layout: 96-byte lead, an optional
// signature header, the main header, then a compressed cpio payload. Both
// headers use the "header structure" (magic 8E AD E8 01). The payload
// compressor (gzip/xz/zstd/lzma/bzip2) is taken from tag 1125 or sniffed.
class XRPM : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_LEAD,
        STRUCTID_HEADER
    };

#pragma pack(push)
#pragma pack(1)
    struct RPMLEAD {
        quint8 magic[4];  // ED AB EE DB
        quint8 major;
        quint8 minor;
        quint16 type;
        quint16 archnum;
        char name[66];
        quint16 osnum;
        quint16 signature_type;
        char reserved[16];
    };
#pragma pack(pop)

    explicit XRPM(QIODevice *pDevice = nullptr);

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

    // Locate the payload: returns offset just after the main header, and the
    // detected payload compression method.
    qint64 getPayloadOffset(PDSTRUCT *pPdStruct = nullptr);
    HANDLE_METHOD getPayloadCompression(qint64 nPayloadOffset);

private:
    // Header structure size (including its 16-byte intro), starting at nOffset.
    qint64 _readHeaderSize(qint64 nOffset);
    qint64 _getMainHeaderOffset();
    QString _readPayloadCompressorTag(qint64 nHeaderOffset);

    struct RPM_UNPACK_CONTEXT {
        qint64 nPayloadOffset;
        qint64 nPayloadSize;
        qint64 nUncompressedSize;
        quint32 nCRC32;
        bool bHasCRC32;
        HANDLE_METHOD compressMethod;
        QString sFileName;
    };

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XRPM_H
