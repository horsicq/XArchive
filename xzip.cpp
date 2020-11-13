// copyright (c) 2017-2020 hors<horsicq@gmail.com>
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

XZip::XZip(QIODevice *pDevice) : XArchive(pDevice)
{

}

bool XZip::isValid()
{
    bool bResult=false;

    _MEMORY_MAP memoryMap=XBinary::getMemoryMap(); // TODO Check

    if(compareSignature(&memoryMap,"'PK'0304",0)||compareSignature(&memoryMap,"'PK'0506",0))
    {
        bResult=true;
    }

    return bResult;
}

QString XZip::getVersion()
{
    QString sResult;

    qint64 nECDOffset=findECDOffset();

    quint16 nVersion=0;
    if(nECDOffset!=0)
    {
        qint64 nOffset=read_uint32(nECDOffset+offsetof(ENDOFCENTRALDIRECTORYRECORD,nOffsetToCentralDirectory));

        quint32 nSignature=read_uint32(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nSignature));

        if(nSignature==SIGNATURE_CFD)
        {
            quint16 nVersion=read_uint16(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nVersion));

            if(nVersion==0)
            {
                nVersion=read_uint16(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nMinVersion));
            }
        }
    }

    if(nVersion==0)
    {
        // The first record
        nVersion=read_uint16(0+offsetof(CENTRALDIRECTORYFILEHEADER,nVersion));
        sResult=QString("%1").arg((double)nVersion/10,0,'f',1);
    }

    return sResult;
}

bool XZip::isEncrypted()
{
    bool bResult=false;

    qint64 nECDOffset=findECDOffset();

    bool bSuccess=false;
    if(nECDOffset!=0)
    {
        qint64 nOffset=read_uint32(nECDOffset+offsetof(ENDOFCENTRALDIRECTORYRECORD,nOffsetToCentralDirectory));

        quint32 nSignature=read_uint32(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nSignature));

        if(nSignature==SIGNATURE_CFD)
        {
            quint16 nFlags=read_uint16(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nFlags));

            bResult=(nFlags&0x1);
            bSuccess=true;
        }
    }

    if(!bSuccess)
    {
        // The first record
        quint16 nFlags=read_uint16(offsetof(CENTRALDIRECTORYFILEHEADER,nFlags));

        bResult=(nFlags&0x1);
    }

    return bResult;
}

quint64 XZip::getNumberOfRecords()
{
    quint64 nResult=0;

    qint64 nECDOffset=findECDOffset();

    if(nECDOffset!=0)
    {
        nResult=read_uint16(nECDOffset+offsetof(ENDOFCENTRALDIRECTORYRECORD,nTotalNumberOfRecords));
    }

    return nResult;
}

QList<XArchive::RECORD> XZip::getRecords(qint32 nLimit)
{
    QList<RECORD> listResult;

    qint64 nECDOffset=findECDOffset();

    if(nECDOffset!=-1) // TODO if no ECD, only the first record
    {
        int nNumberOfRecords=read_uint16(nECDOffset+offsetof(ENDOFCENTRALDIRECTORYRECORD,nTotalNumberOfRecords));

        if(nLimit!=-1)
        {
            nNumberOfRecords=qMin(nNumberOfRecords,nLimit);
        }

        qint64 nOffset=read_uint32(nECDOffset+offsetof(ENDOFCENTRALDIRECTORYRECORD,nOffsetToCentralDirectory));

        for(int i=0; i<nNumberOfRecords; i++)
        {
            RECORD record={};

            quint32 nSignature=read_uint32(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nSignature));

            if(nSignature!=SIGNATURE_CFD)
            {
                break;
            }

            quint32 nFileNameSize=read_uint16(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nFileNameLength));
            quint32 nExtraFieldSize=read_uint16(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nExtraFieldLength));
            quint32 nFileCommentSize=read_uint16(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nFileCommentLength));

            record.nCRC32=read_uint32(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nCRC32));
            record.nCompressedSize=read_uint32(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nCompressedSize));
            record.nUncompressedSize=read_uint32(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nUncompressedSize));
            record.compressMethod=COMPRESS_METHOD_UNKNOWN;
            quint32 nZipMethod=read_uint16(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nMethod));

            switch(nZipMethod)
            {
                case METHOD_STORE:          record.compressMethod=COMPRESS_METHOD_STORE;        break;
                case METHOD_DEFLATE:        record.compressMethod=COMPRESS_METHOD_DEFLATE;      break;
                case METHOD_DEFLATE64:      record.compressMethod=COMPRESS_METHOD_DEFLATE64;    break; // TODO
                case METHOD_BZIP2:          record.compressMethod=COMPRESS_METHOD_BZIP2;        break;
                case METHOD_LZMA:           record.compressMethod=COMPRESS_METHOD_LZMA_ZIP;     break;
                case METHOD_PPMD:           record.compressMethod=COMPRESS_METHOD_PPMD;         break; // TODO
            }
            // TODO more methods

            record.sFileName=read_ansiString(nOffset+sizeof(CENTRALDIRECTORYFILEHEADER),nFileNameSize);

            quint32 nLocalFileHeaderOffset=read_uint32(nOffset+offsetof(CENTRALDIRECTORYFILEHEADER,nOffsetToLocalFileHeader));

            quint32 nLocalSignature=read_uint32(nLocalFileHeaderOffset+offsetof(LOCALFILEHEADER,nSignature));
            quint32 nLocalFileNameSize=read_uint16(nLocalFileHeaderOffset+offsetof(LOCALFILEHEADER,nFileNameLength));
            quint32 nLocalExtraFieldSize=read_uint16(nLocalFileHeaderOffset+offsetof(LOCALFILEHEADER,nExtraFieldLength));

            if(nLocalSignature!=SIGNATURE_LFD)
            {
                break;
            }

            record.nDataOffset=nLocalFileHeaderOffset+sizeof(LOCALFILEHEADER)+nLocalFileNameSize+nLocalExtraFieldSize;

            listResult.append(record);

            nOffset+=(sizeof(CENTRALDIRECTORYFILEHEADER)+nFileNameSize+nExtraFieldSize+nFileCommentSize);
        }
    }

    return listResult;
}

bool XZip::addLocalFileRecord(QIODevice *pSource, QIODevice *pDest, ZIPFILE_RECORD *pZipFileRecord)
{
    if(pZipFileRecord->nMinVersion==0)
    {
        pZipFileRecord->nMinVersion=0x14;
    }

    if(pZipFileRecord->nVersion==0)
    {
        pZipFileRecord->nVersion=0x3F;
    }

    if(pZipFileRecord->nUncompressedSize==0)
    {
        pZipFileRecord->nUncompressedSize=pSource->size();
    }

    if(pZipFileRecord->nCRC32==0)
    {
        pZipFileRecord->nCRC32=XBinary::_getCRC32(pSource);
    }

    if(!pZipFileRecord->dtTime.isValid())
    {
        pZipFileRecord->dtTime=QDateTime::currentDateTime();
    }

    pZipFileRecord->nHeaderOffset=pDest->pos();

    XZip::LOCALFILEHEADER localFileHeader={};
    localFileHeader.nSignature=XZip::SIGNATURE_LFD;
    localFileHeader.nMinVersion=pZipFileRecord->nMinVersion;
    localFileHeader.nFlags=pZipFileRecord->nFlags;
    localFileHeader.nMethod=pZipFileRecord->method;
    localFileHeader.nLastModTime=0; // TODO
    localFileHeader.nLastModDate=0; // TODO
    localFileHeader.nCRC32=pZipFileRecord->nCRC32;
    localFileHeader.nCompressedSize=0;
    localFileHeader.nUncompressedSize=pZipFileRecord->nUncompressedSize;
    localFileHeader.nFileNameLength=pZipFileRecord->sFileName.size();
    localFileHeader.nExtraFieldLength=0;

    pDest->write((char *)&localFileHeader,sizeof(localFileHeader));
    pDest->write(pZipFileRecord->sFileName.toLatin1().data(),pZipFileRecord->sFileName.toLatin1().size());

    pZipFileRecord->nDataOffset=pDest->pos();

    XArchive::compress(XArchive::COMPRESS_METHOD_DEFLATE,pSource,pDest);

    qint64 nEndPosition=pDest->pos();

    pZipFileRecord->nCompressedSize=(nEndPosition)-(pZipFileRecord->nDataOffset);

    XBinary binary(pDest);

    binary.write_uint32(pZipFileRecord->nHeaderOffset+offsetof(XZip::LOCALFILEHEADER,nCompressedSize),pZipFileRecord->nCompressedSize);

    pDest->seek(nEndPosition);

    return true;
}

bool XZip::addCentralDirectory(QIODevice *pDest, QList<XZip::ZIPFILE_RECORD> *pListZipFileRecords, QString sComment)
{
    qint64 nStartPosition=pDest->pos();

    int nNumberOfRecords=pListZipFileRecords->count();

    for(int i=0;i<nNumberOfRecords;i++)
    {
        XZip::CENTRALDIRECTORYFILEHEADER cdFileHeader={};

        cdFileHeader.nSignature=SIGNATURE_CFD;
        cdFileHeader.nVersion=pListZipFileRecords->at(i).nVersion;
        cdFileHeader.nMinVersion=pListZipFileRecords->at(i).nMinVersion;
        cdFileHeader.nFlags=pListZipFileRecords->at(i).nFlags;
        cdFileHeader.nMethod=pListZipFileRecords->at(i).method;
        cdFileHeader.nLastModTime=0; // TODO
        cdFileHeader.nLastModDate=0; // TODO
        cdFileHeader.nCRC32=pListZipFileRecords->at(i).nCRC32;
        cdFileHeader.nCompressedSize=pListZipFileRecords->at(i).nCompressedSize;
        cdFileHeader.nUncompressedSize=pListZipFileRecords->at(i).nUncompressedSize;
        cdFileHeader.nFileNameLength=pListZipFileRecords->at(i).sFileName.size();
        cdFileHeader.nExtraFieldLength=0;
        cdFileHeader.nFileCommentLength=0;
        cdFileHeader.nStartDisk=0;
        cdFileHeader.nInternalFileAttributes=0;
        cdFileHeader.nExternalFileAttributes=0;
        cdFileHeader.nOffsetToLocalFileHeader=pListZipFileRecords->at(i).nHeaderOffset;

        pDest->write((char *)&cdFileHeader,sizeof(cdFileHeader));
        pDest->write(pListZipFileRecords->at(i).sFileName.toLatin1().data(),pListZipFileRecords->at(i).sFileName.toLatin1().size());
    }

    qint64 nCentralDirectorySize=pDest->pos()-nStartPosition;

    ENDOFCENTRALDIRECTORYRECORD endofCD={};

    endofCD.nSignature=SIGNATURE_ECD;
    endofCD.nDiskNumber=0;
    endofCD.nStartDisk=0;
    endofCD.nDiskNumberOfRecords=nNumberOfRecords;
    endofCD.nTotalNumberOfRecords=nNumberOfRecords;
    endofCD.nSizeOfCentralDirectory=nCentralDirectorySize;
    endofCD.nOffsetToCentralDirectory=nStartPosition;
    endofCD.nCommentLength=sComment.size();

    pDest->write((char *)&endofCD,sizeof(endofCD));
    pDest->write(sComment.toLatin1().data(),sComment.toLatin1().size());

    return true;
}

qint64 XZip::findECDOffset()
{
    qint64 nResult=-1;
    qint64 nSize=getSize();

    if(nSize>=22) // 22 is minimum size [0x50,0x4B,0x05,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]
    {
        qint64 nOffset=qMax((qint64)0,nSize-0x1000);  // TODO const

        while(true)
        {
            qint64 nCurrent=find_uint32(nOffset,-1,SIGNATURE_ECD);

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
