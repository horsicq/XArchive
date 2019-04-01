#include "xarchive.h"

static void *SzAlloc(ISzAllocPtr, size_t size) { return malloc(size); }
static void SzFree(ISzAllocPtr, void *address) { free(address); }
static ISzAlloc g_Alloc = { SzAlloc, SzFree };

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

        if(inflateInit2(&strm,-MAX_WBITS)==Z_OK) // -MAX_WBITS for raw data
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
//                    strm.avail_out=1;
                    strm.next_out=out;
                    ret=inflate(&strm,Z_NO_FLUSH);
//                    ret=inflate(&strm,Z_SYNC_FLUSH);

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
    else if(compressMethos==COMPRESS_METHOD_BZIP2)
    {
        const int CHUNK=16384;

        char in[CHUNK];
        char out[CHUNK];

        bz_stream strm= {0};
        int ret=BZ_MEM_ERROR;

        int rc=BZ2_bzDecompressInit(&strm,0,0);

        if(rc==BZ_OK)
        {
            do
            {
                strm.avail_in=pSourceDevice->read((char *)in,CHUNK);

                if(strm.avail_in==0)
                {
                    ret=BZ_MEM_ERROR;
                    break;
                }

                strm.next_in=in;

                do
                {
                    strm.avail_out=CHUNK;
                    strm.next_out=out;
                    ret=BZ2_bzDecompress(&strm);

                    if((ret!=BZ_STREAM_END)&&(ret!=BZ_OK))
                    {
                        break;
                    }

                    int nTemp=CHUNK-strm.avail_out;

                    if(pDestDevice->write((char *)out,nTemp)!=nTemp)
                    {
                        ret=BZ_MEM_ERROR;
                        break;
                    }
                }
                while(strm.avail_out==0);

                if(ret!=BZ_OK)
                {
                    break;
                }
            }
            while(ret!=BZ_STREAM_END);

            BZ2_bzDecompressEnd(&strm);
        }

        // TODO more error codes
        if((ret==BZ_OK)||(ret==BZ_STREAM_END))
        {
            result=COMPRESS_RESULT_OK;
        }
        else if(ret==BZ_MEM_ERROR)
        {
            result=COMPRESS_RESULT_DATAERROR;
        }
        else
        {
            result=COMPRESS_RESULT_UNKNOWN;
        }
    }
    else if(compressMethos==COMPRESS_METHOD_LZMA_ZIP)
    {
        // TODO more codes
        int nPropSize=0;
        char header1[4]= {0};
        quint8 properties[32]= {0};

        pSourceDevice->read(header1,sizeof(header1));
        nPropSize=header1[2]; // TODO Check

        if(nPropSize&&(nPropSize<30))
        {
            pSourceDevice->read((char *)properties,nPropSize);

            CLzmaDec state={0};

            SRes ret=LzmaProps_Decode(&state.prop,(Byte *)properties,nPropSize);

            if(ret==S_OK)
            {
                LzmaDec_Construct(&state);
                ret=LzmaDec_Allocate(&state,(Byte *)properties,nPropSize,&g_Alloc);

                if(ret==S_OK)
                {
                    LzmaDec_Init(&state);

                    const int CHUNK=16384;

                    char in[CHUNK];
                    char out[CHUNK];

                    bool bRun=true;

                    while(bRun)
                    {
                        qint32 nSize=pSourceDevice->read((char *)in,CHUNK);

                        if(nSize)
                        {
                            qint64 nPos=0;

                            while(true)
                            {
                                ELzmaStatus status;
                                SizeT inProcessed=nSize-nPos;
                                SizeT outProcessed=CHUNK;

                                ret=LzmaDec_DecodeToBuf(&state,(Byte *)out,&outProcessed,(Byte *)(in+nPos),&inProcessed,LZMA_FINISH_ANY,&status);

                                nPos+=inProcessed;

                                if(pDestDevice->write((char *)out,outProcessed)!=outProcessed)
                                {
                                    result=COMPRESS_RESULT_WRITEERROR;
                                    bRun=false;
                                    break;
                                }

                                if(status!=LZMA_STATUS_NOT_FINISHED)
                                {
                                    if(status==LZMA_STATUS_FINISHED_WITH_MARK)
                                    {
                                        result=COMPRESS_RESULT_OK;
                                        bRun=false;
                                    }

                                    break;
                                }
                            }
                        }
                        else
                        {
                            result=COMPRESS_RESULT_READERROR;
                            bRun=false;
                        }
                    }
                }

                LzmaDec_Free(&state, &g_Alloc);
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
