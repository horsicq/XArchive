// copyright (c) 2017-2021 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#ifndef XSEVENZIP_H
#define XSEVENZIP_H

#include "xarchive.h"

class XSevenZip : public XArchive
{
    Q_OBJECT

    enum EIdEnum
    {
        k7zIdEnd=0,
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
    };

public:    
#pragma pack(push)
#pragma pack(1)
    struct SignatureHeader
    {
        quint8 kSignature[6];       // {'7','z',0xBC,0xAF,0x27,0x1C}
        quint8 Major;               // now = 0
        quint8 Minor;               // now = 4
        quint32 StartHeaderCRC;
        quint64 NextHeaderOffset;
        quint64 NextHeaderSize;
        quint32 NextHeaderCRC;
    };
#pragma pack(pop)

    struct XPACKINFO
    {
        bool bIsValid;
        qint64 nDataOffset;
        QList<qint64> listSizes;
    };

    struct XUNPACKINFO
    {
        bool bIsValid;
        QList<qint64> listSizes;
        QList<qint64> listCoderSizes;
    };

    struct XHEADER
    {
        bool bIsValid;
        XPACKINFO xPackInfo;
        XUNPACKINFO xUnpackInfo;
    };

    explicit XSevenZip(QIODevice *pDevice=nullptr);
    virtual bool isValid();
    virtual QString getVersion();
    virtual quint64 getNumberOfRecords();
    virtual QList<RECORD> getRecords(qint32 nLimit=-1);

private:
    QString idToSring(EIdEnum id);
    qint64 getHeader(qint64 nOffset,XHEADER *pHeader);
    qint64 getEncodedHeader(qint64 nOffset,XHEADER *pHeader);
    qint64 getPackInfo(qint64 nOffset,XPACKINFO *pPackInfo);
    qint64 getUnpackInfo(qint64 nOffset,XUNPACKINFO *pUnpackInfo);
};

#endif // XSEVENZIP_H
