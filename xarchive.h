// copyright (c) 2019 hors<horsicq@gmail.com>
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
#ifndef XARCHIVE_H
#define XARCHIVE_H

#include <QObject>
#include "xbinary.h"
#include "zlib.h"
#include "bzlib.h"
#include "LzmaDec.h"

class XArchive : public XBinary
{
    Q_OBJECT
public:
    enum COMPRESS_METHOD
    {
        COMPRESS_METHOD_UNKNOWN=0,
        COMPRESS_METHOD_STORE,
        COMPRESS_METHOD_DEFLATE,
        COMPRESS_METHOD_DEFLATE64,
        COMPRESS_METHOD_BZIP2,
        COMPRESS_METHOD_LZMA_ZIP,
        COMPRESS_METHOD_PPMD
    };
    struct RECORD
    {
        QString sFileName;
        quint64 nCRC;
        qint64 nDataOffset;
        qint64 nCompressedSize;
        qint64 nUncompressedSize;
        COMPRESS_METHOD compressMethod;
    };
    enum COMPRESS_RESULT
    {
        COMPRESS_RESULT_UNKNOWN=0,
        COMPRESS_RESULT_OK,
        COMPRESS_RESULT_DATAERROR,
        COMPRESS_RESULT_MEMORYERROR,
        COMPRESS_RESULT_BUFFERERROR,
        COMPRESS_RESULT_METHODERROR,
        COMPRESS_RESULT_READERROR,
        COMPRESS_RESULT_WRITEERROR
    };
    explicit XArchive(QIODevice *__pDevice);
    virtual quint64 getNumberOfRecords()=0;
    virtual QList<RECORD> getRecords(qint32 nLimit=-1)=0;

    static COMPRESS_RESULT decompress(COMPRESS_METHOD compressMethos,QIODevice *pSourceDevice,QIODevice *pDestDevice);
    QByteArray decompress(RECORD *pRecord);
    bool decompressToFile(RECORD *pRecord,QString sFileName);
    bool dumpToFile(RECORD *pRecord,QString sFileName);
};

#endif // XARCHIVE_H
