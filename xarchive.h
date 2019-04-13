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
