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
#ifndef XKWAJ_H
#define XKWAJ_H

#include "xarchive.h"

// KWAJ: the MS-DOS 6 COMPRESS.EXE format (libmspack kwajd). Header: magic
// "KWAJ" 88 F0 27 D1, comp_type(2), data_offset(2), header_flags(2), then a
// variable extension (optional length/filename/extension), then the data.
class XKWAJ : public XArchive {
    Q_OBJECT

public:
    struct INTERNAL_INFO : XArchive::INTERNAL_INFO {};

    bool handleInternalInfo(PDSTRUCT *pPdStruct) override;
    void *getInternalInfo(PDSTRUCT *pPdStruct) override;
    void setInternalInfo(void *pInternalInfo) override;

    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_KWAJ_HEADER
    };

    enum COMP_TYPE {
        COMP_TYPE_STORE = 0,
        COMP_TYPE_XOR = 1,
        COMP_TYPE_SZDD = 2,
        COMP_TYPE_LZH = 3,
        COMP_TYPE_MSZIP = 4
    };

    enum HDR_FLAG {
        HDR_FLAG_HASLENGTH = 0x01,
        HDR_FLAG_HASUNKNOWN1 = 0x02,
        HDR_FLAG_HASUNKNOWN2 = 0x04,
        HDR_FLAG_HASFILENAME = 0x08,
        HDR_FLAG_HASFILEEXT = 0x10,
        HDR_FLAG_HASEXTRATEXT = 0x20
    };

#pragma pack(push)
#pragma pack(1)
    struct KWAJ_HEADER {
        quint8 signature[8];  // 4B 57 41 4A 88 F0 27 D1
        quint16 comp_type;
        quint16 data_offset;
        quint16 header_flags;
    };
#pragma pack(pop)

    explicit XKWAJ(QIODevice *pDevice = nullptr);

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
    HANDLE_METHOD _compTypeToMethod(quint16 nCompType);
    static bool failUnpackInitialization(XKWAJ *pArchive, UNPACK_STATE *pState);
    static bool hasExtensionBytes(qint64 nExtensionOffset, qint64 nDataOffset, qint64 nSize);
    static bool skipExtensionBytes(qint64 nDataOffset, qint64 nSize, qint64 *pExtensionOffset);
    bool readBoundedExtensionString(qint64 nDataOffset, qint64 *pExtensionOffset,
                                    qint32 nMaximumBytes, QString *pString);

    struct KWAJ_UNPACK_CONTEXT {
        qint64 nDataOffset = 0;
        qint64 nDataSize = 0;
        qint64 nUncompressedSize = 0;
        bool bUncompressedSizeDefined = false;
        HANDLE_METHOD compressMethod = HANDLE_METHOD_UNKNOWN;
        QString sFileName;
    };

private:
    INTERNAL_INFO m_internalInfo;
};

#endif  // XKWAJ_H
