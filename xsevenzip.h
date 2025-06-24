/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#ifndef XSEVENZIP_H
#define XSEVENZIP_H

#include "xarchive.h"

// TODO https://py7zr.readthedocs.io/en/latest/archive_format.html
class XSevenZip : public XArchive {
    Q_OBJECT

public:
    enum STRUCTID {
        STRUCTID_UNKNOWN = 0,
        STRUCTID_SIGNATUREHEADER,
        STRUCTID_HEADER
    };

    enum EIdEnum {
        k7zIdEnd = 0,
        k7zIdHeader,
        k7zIdArchiveProperties,
        k7zIdAdditionalStreamsInfo,
        k7zIdMainStreamsInfo,
        k7zIdFilesInfo,
        k7zIdPackInfo,
        k7zIdUnpackInfo,
        k7zIdSubStreamsInfo,
        k7zIdSize,
        k7zIdCRC,
        k7zIdFolder,
        k7zIdCodersUnpackSize,
        k7zIdNumUnpackStream,
        k7zIdEmptyStream,
        k7zIdEmptyFile,
        k7zIdAnti,
        k7zIdName,
        k7zIdCTime,
        k7zIdATime,
        k7zIdMTime,
        k7zIdWinAttrib,
        k7zIdComment,
        k7zIdEncodedHeader,
        k7zIdStartPos,
        k7zIdDummy
        // k7zNtSecure,
        // k7zParent,
        // k7zIsReal
        // Test
    };
#pragma pack(push)
#pragma pack(1)
    struct SIGNATUREHEADER {
        quint8 kSignature[6];  // {'7','z',0xBC,0xAF,0x27,0x1C}
        quint8 Major;          // now = 0
        quint8 Minor;          // now = 4
        quint32 StartHeaderCRC;
        quint64 NextHeaderOffset;
        quint64 NextHeaderSize;
        quint32 NextHeaderCRC;
    };
#pragma pack(pop)

    explicit XSevenZip(QIODevice *pDevice = nullptr);

    virtual bool isValid(PDSTRUCT *pPdStruct = nullptr);
    static bool isValid(QIODevice *pDevice);
    virtual QString getVersion();
    virtual quint64 getNumberOfRecords(PDSTRUCT *pPdStruct);
    virtual QList<RECORD> getRecords(qint32 nLimit, PDSTRUCT *pPdStruct);
    virtual qint64 getFileFormatSize(PDSTRUCT *pPdStruct);
    virtual QString getFileFormatExt();
    virtual QString getFileFormatExtsString();
    virtual QList<MAPMODE> getMapModesList();
    virtual _MEMORY_MAP getMemoryMap(MAPMODE mapMode = MAPMODE_UNKNOWN, PDSTRUCT *pPdStruct = nullptr);
    virtual FT getFileType();
    virtual ENDIAN getEndian();
    virtual MODE getMode();
    virtual QString getArch();
    virtual QString getMIMEString();

    static QMap<quint64, QString> getEIdEnumS();

    SIGNATUREHEADER _read_SIGNATUREHEADER(qint64 nOffset);
    static QString idToSring(EIdEnum id);

    virtual QString structIDToString(quint32 nID) override;
    virtual QList<DATA_HEADER> getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct) override;

private:
    enum SRTYPE {
        SRTYPE_UNKNOWN = 0,
        SRTYPE_ID,
        SRTYPE_NUMBER,
        SRTYPE_BYTE,
        SRTYPE_ARRAY
    };

    struct SZRECORD {
        qint32 nRelOffset;
        qint32 nSize;
        QString sName;
        QVariant varValue;
        SRTYPE srType;
    };

    struct SZSTATE {
        char *pData;
        qint64 nCurrentOffset;
        qint64 nSize;
        bool bIsError;
        QString sErrorString;
    };

    QList<SZRECORD> _handleData(qint64 nOffset, qint64 nSize, PDSTRUCT *pPdStruct);
    void _handleId(QList<SZRECORD> *pListRecords, EIdEnum id, SZSTATE *pState, PDSTRUCT *pPdStruct);
    quint64 _handleNumber(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption);
    quint8 _handleByte(QList<SZRECORD> *pListRecords, SZSTATE *pState, PDSTRUCT *pPdStruct, const QString &sCaption);
    void _handleArray(QList<SZRECORD> *pListRecords, SZSTATE *pState, qint64 nSize, PDSTRUCT *pPdStruct, const QString &sCaption);
};

#endif  // XSEVENZIP_H
