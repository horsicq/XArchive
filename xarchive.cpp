#include "xarchive.h"

XArchive::XArchive(QIODevice *__pDevice): QBinary(__pDevice)
{

}

XArchive::COMPRESS_RESULT XArchive::decompress(XArchive::COMPRESS_METHOD compressMethos, QIODevice *pSourceDevice, QIODevice *pDestDevice)
{
    COMPRESS_RESULT result=COMPRESS_RESULT_UNKNOWN;

    if(compressMethos==COMPRESS_METHOD_STORE)
    {
        const int CHUNK=4096;
        char buffer[CHUNK];
        qint64 nSize=pSourceDevice->size();

        result=COMPRESS_RESULT_OK;
        while(nSize>0)
        {
            qint64 nTemp=qMin((qint64)CHUNK,nSize);

            if(pSourceDevice->read(buffer,nTemp)!=nTemp)
            {
                result=COMPRESS_RESULT_READERROR;
                break;
            }
            if(pDestDevice->write(buffer,nTemp)!=nTemp)
            {
                result=COMPRESS_RESULT_WRITEERROR;
                break;
            }

            nSize-=nTemp;
        }
    }
    else if(compressMethos==COMPRESS_METHOD_DEFLATE)
    {
        const int CHUNK=16384;

        unsigned char in[CHUNK];
        unsigned char out[CHUNK];

        z_stream strm;

        strm.zalloc=Z_NULL;
        strm.zfree=Z_NULL;
        strm.opaque=Z_NULL;
        strm.avail_in=0;
        strm.next_in=Z_NULL;

        int ret=Z_OK;

        if(inflateInit2(&strm,-8)==Z_OK) // -8 for raw data
        {
            do
            {
                strm.avail_in=pSourceDevice->read((char *)in,CHUNK);

                if(strm.avail_in==0)
                {
                    ret=Z_ERRNO;
                    break;
                }

                strm.next_in=in;

                do
                {
                    strm.avail_out=CHUNK;
                    strm.next_out=out;
                    ret=inflate(&strm,Z_NO_FLUSH);

                    if((ret==Z_DATA_ERROR)||(ret==Z_MEM_ERROR)||(ret==Z_NEED_DICT))
                    {
                        break;
                    }

                    int nTemp=CHUNK-strm.avail_out;

                    if(pDestDevice->write((char *)out,nTemp)!=nTemp)
                    {
                        ret=Z_ERRNO;
                        break;
                    }

                }
                while(strm.avail_out==0);

                if(ret!=Z_OK)
                {
                    break;
                }

            }
            while(ret!=Z_STREAM_END);

            inflateEnd(&strm);

            if(ret==Z_OK)
            {
                result=COMPRESS_RESULT_OK;
            }
            else if(ret==Z_BUF_ERROR)
            {
                result=COMPRESS_RESULT_BUFFERERROR;
            }
            else if(ret==Z_MEM_ERROR)
            {
                result=COMPRESS_RESULT_MEMORYERROR;
            }
            else if(ret==Z_DATA_ERROR)
            {
                result=COMPRESS_RESULT_DATAERROR;
            }
            else
            {
                result=COMPRESS_RESULT_UNKNOWN;
            }
        }
    }
    return result;
}

QByteArray XArchive::decompress(XArchive::RECORD *pRecord)
{
    QByteArray result;

    SubDevice sd(getDevice(),pRecord->nDataOffset,pRecord->nCompressedSize);

    if(sd.open(QIODevice::ReadOnly))
    {
        QBuffer buffer;
        buffer.setBuffer(&result);
        buffer.open(QIODevice::WriteOnly);

        decompress(pRecord->compressMethod,&sd,&buffer);

        buffer.close();

        sd.close();
    }

    return result;
}

bool XArchive::decompressToFile(XArchive::RECORD *pRecord, QString sFileName)
{
    bool bResult=false;

    QFile file;
    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadWrite))
    {
        SubDevice sd(getDevice(),pRecord->nDataOffset,pRecord->nCompressedSize);

        if(sd.open(QIODevice::ReadOnly))
        {
            bResult=(decompress(pRecord->compressMethod,&sd,&file)==COMPRESS_RESULT_OK);

            sd.close();
        }

        file.close();
    }

    return bResult;
}
