// copyright (c) 2017-2019 hors<horsicq@gmail.com>
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

QList<XArchive::RECORD> XZip::getRecords(qint32 nLimit)
{
    QList<RECORD> listResult;

    qint64 nECDOffset=findECDOffset();

    if(nECDOffset!=0)
    {
        int nNumberOfRecords=read_uint16(nECDOffset+10);

        if(nLimit!=-1)
        {
            nNumberOfRecords=qMin(nNumberOfRecords,nLimit);
        }

        qint64 nOffset=read_uint32(nECDOffset+16);

        for(int i=0; i<nNumberOfRecords; i++)
        {
            RECORD record= {};

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

            switch(nZipMethod)
            {
                case 0:     record.compressMethod=COMPRESS_METHOD_STORE;        break;
                case 8:     record.compressMethod=COMPRESS_METHOD_DEFLATE;      break;
                case 9:     record.compressMethod=COMPRESS_METHOD_DEFLATE64;    break; // TODO
                case 12:    record.compressMethod=COMPRESS_METHOD_BZIP2;        break;
                case 14:    record.compressMethod=COMPRESS_METHOD_LZMA_ZIP;     break;
                case 98:    record.compressMethod=COMPRESS_METHOD_PPMD;         break; // TODO
            }
            // TODO more methods

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
