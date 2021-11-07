// Copyright (c) 2019-2021 hors<horsicq@gmail.com>
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

#include "xbinary.h"
#include "LzmaDec.h"
#include "bzlib.h"
#include "zlib.h"
#ifdef PPMD_SUPPORT
#include "Ppmd7.h"
#endif

class XArchive : public XBinary
{
    Q_OBJECT

public:
    enum COMPRESS_METHOD
    {
        COMPRESS_METHOD_UNKNOWN=0,
        COMPRESS_METHOD_STORE,
        COMPRESS_METHOD_FILE,
        COMPRESS_METHOD_DEFLATE,
        COMPRESS_METHOD_DEFLATE64,
        COMPRESS_METHOD_BZIP2,
        COMPRESS_METHOD_LZMA_ZIP,
        COMPRESS_METHOD_PPMD
        // TODO more
    };

    struct RECORD
    {
        // TODO bIsValid
        QString sFileName;
        quint32 nCRC32;
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
        // TODO more
    };

    static const qint32 COMPRESS_BUFFERSIZE=0x4000; // TODO Check
    static const int DECOMPRESS_BUFFERSIZE=0x4000;

    explicit XArchive(QIODevice *pDevice=nullptr);
    virtual quint64 getNumberOfRecords();
    virtual QList<RECORD> getRecords(qint32 nLimit=-1);
    static COMPRESS_RESULT decompress(COMPRESS_METHOD compressMethod,QIODevice *pSourceDevice,QIODevice *pDestDevice,bool bHeaderOnly=false,bool *pbIsStop=nullptr);
    static COMPRESS_RESULT compress(COMPRESS_METHOD compressMethod,QIODevice *pSourceDevice,QIODevice *pDestDevice);
    static COMPRESS_RESULT compress_deflate(QIODevice *pSourceDevice,QIODevice *pDestDevice,qint32 nLevel,int nMethod,int nWindowsBits,int nMemLevel,int nStrategy);
    QByteArray decompress(const RECORD *pRecord,bool bHeaderOnly=false);
    QByteArray decompress(QList<RECORD> *pListArchive,QString sRecordFileName);
    QByteArray decompress(QString sRecordFileName);
    bool decompressToFile(const RECORD *pRecord,QString sResultFileName,bool *pbIsStop=nullptr);
    bool decompressToFile(QList<RECORD> *pListArchive,QString sRecordFileName,QString sResultFileName);
    bool decompressToPath(QList<RECORD> *pListArchive,QString sRecordFileName,QString sResultPathName);
    bool decompressToFile(QString sArchiveFileName,QString sRecordFileName,QString sResultFileName);
    bool decompressToPath(QString sArchiveFileName,QString sRecordPathName,QString sResultPathName);
    bool dumpToFile(const RECORD *pRecord,QString sFileName);
    static RECORD getArchiveRecord(QString sRecordFileName,QList<RECORD> *pListRecords);
    bool isArchiveRecordPresent(QString sRecordFileName);
    static bool isArchiveRecordPresent(QString sRecordFileName,QList<RECORD> *pListRecords);
    static quint32 getCompressBufferSize();
    static quint32 getDecompressBufferSize();

    static void showRecords(QList<RECORD> *pListArchive);

    virtual MODE getMode();
//    virtual _MEMORY_MAP getMemoryMap(); // TODO
};

#endif // XARCHIVE_H
