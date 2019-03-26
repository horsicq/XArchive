#include "xzip.h"

XZip::XZip(QIODevice *__pDevice) : XArchive(__pDevice)
{

}

bool XZip::isVaild()
{
    bool bResult=false;

    if(compareSignature("'PK'0304",0)||compareSignature("'PK'0506",0))
    {
        bResult=true;
    }

    return bResult;
}

quint64 XZip::getNumberOfRecords()
{
    quint64 nResult=0;

    qint64 nECDOffset=findECDOffset();

    if(nECDOffset!=0)
    {
        nResult=read_uint16(nECDOffset+10);
    }

    return nResult;
}

QList<XArchive::RECORD> XZip::getRecords()
{
    QList<RECORD> listResult;

    qint64 nECDOffset=findECDOffset();

    if(nECDOffset!=0)
    {
        int nNumberOfRecords=read_uint16(nECDOffset+10);
        qint64 nOffset=read_uint32(nECDOffset+16);

        for(int i=0; i<nNumberOfRecords; i++)
        {
            RECORD record={};

            quint32 nSignature=read_uint32(nOffset+0);

            if(nSignature!=CFD)
            {
                break;
            }

            quint32 nFileNameSize=read_uint16(nOffset+28);
            quint32 nExtraFieldSize=read_uint16(nOffset+30);
            quint32 nFileCommentSize=read_uint16(nOffset+32);

            record.nCRC=read_uint32(nOffset+16);
            record.nCompressedSize=read_uint32(nOffset+20);
            record.nUncompressedSize=read_uint32(nOffset+24);
            record.compressMethod=COMPRESS_METHOD_UNKNOWN;
            quint32 nZipMethod=read_uint16(nOffset+10);
            if(nZipMethod==0)
            {
                record.compressMethod=COMPRESS_METHOD_STORE;
            }
            else if(nZipMethod==8)
            {
                record.compressMethod=COMPRESS_METHOD_DEFLATE;
            }
            else if(nZipMethod==9)
            {
                record.compressMethod=COMPRESS_METHOD_DEFLATE64;
            }
            else if(nZipMethod==12)
            {
                record.compressMethod=COMPRESS_METHOD_BZIP2;
            }
            else if(nZipMethod==14)
            {
                record.compressMethod=COMPRESS_METHOD_LZMA_ZIP;
            }
            else if(nZipMethod==98)
            {
                record.compressMethod=COMPRESS_METHOD_PPMD;
            }

            record.sFileName=read_ansiString(nOffset+46,nFileNameSize);

            quint32 nLocalFileHeaderOffset=read_uint32(nOffset+42);

            quint32 nLocalSignature=read_uint32(nLocalFileHeaderOffset+0);
            quint32 nLocalFileNameSize=read_uint16(nLocalFileHeaderOffset+26);
            quint32 nLocalExtraFieldSize=read_uint16(nLocalFileHeaderOffset+28);

            if(nLocalSignature!=LFD)
            {
                break;
            }

            record.nDataOffset=nLocalFileHeaderOffset+30+nLocalFileNameSize+nLocalExtraFieldSize;

            listResult.append(record);

            nOffset+=(46+nFileNameSize+nExtraFieldSize+nFileCommentSize);
        }
    }

    return listResult;
}

qint64 XZip::findECDOffset()
{
    qint64 nResult=-1;
    qint64 nSize=getSize();

    if(nSize>=22) // 22 is minimum size [0x50,0x4B,0x05,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]
    {
        qint64 nOffset=qMax((qint64)0,nSize-256);

        while(true)
        {
            qint64 nCurrent=find_uint32(nOffset,-1,ECD);

            if(nCurrent==-1)
            {
                break;
            }

            nResult=nCurrent;
            nOffset=nCurrent+4; // Get the last
        }
    }

    return nResult;
}
