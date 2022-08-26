/* Copyright (c) 2022 hors<horsicq@gmail.com>
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
#include "x_ar.h"

X_Ar::X_Ar(QIODevice *pDevice) : XArchive(pDevice)
{

}

bool X_Ar::isValid()
{
    bool bResult=false;

    _MEMORY_MAP memoryMap=XBinary::getMemoryMap();

    if(getSize()>8+sizeof(RECORD)) // TODO
    {
        if(compareSignature(&memoryMap,"'!<arch>'0a"))
        {
            bResult=true;
        }
    }

    return bResult;
}

bool X_Ar::isValid(QIODevice *pDevice)
{
    X_Ar x_ar(pDevice);

    return x_ar.isValid();
}

quint64 X_Ar::getNumberOfRecords()
{
    quint64 nResult=0;

    qint64 nOffset=0;
    qint64 nSize=getSize();

    nOffset+=8;
    nSize-=8;

    while(nSize>0)
    {
        char fileSize[16];

        read_array(nOffset+offsetof(FRECORD,fileSize),fileSize,10);

        QString sSize=QString(fileSize);

        sSize.resize(10);

        qint32 nRecordSize=sSize.trimmed().toULongLong();

        if(nRecordSize==0)
        {
            break;
        }

        nOffset+=sizeof(FRECORD);
        nOffset+=S_ALIGN_UP(nRecordSize,2);

        nSize-=sizeof(FRECORD);
        nSize-=S_ALIGN_UP(nRecordSize,2);

        nResult++;
    }

    return nResult;
}

QList<XArchive::RECORD> X_Ar::getRecords(qint32 nLimit)
{
    QList<XArchive::RECORD> listRecords;

    qint64 nOffset=0;
    qint64 nSize=getSize();

    nOffset+=8;
    nSize-=8;

    while(nSize>0)
    {
        FRECORD frecord=readFRECORD(nOffset);

        QString sSize=QString(frecord.fileSize);

        sSize.resize(sizeof(frecord.fileSize));

        qint32 nRecordSize=sSize.trimmed().toULongLong();

        if(nRecordSize==0)
        {
            break;
        }

        RECORD record={};

        record.sFileName=frecord.fileId;
        record.sFileName.resize(sizeof(frecord.fileId));
        record.sFileName=record.sFileName.trimmed();

        record.nDataOffset=nOffset+sizeof(FRECORD);
        record.nCompressedSize=nRecordSize;
        record.nUncompressedSize=nRecordSize;
        record.compressMethod=COMPRESS_METHOD_STORE;

        listRecords.append(record);

        nOffset+=sizeof(FRECORD);
        nOffset+=S_ALIGN_UP(nRecordSize,2);

        nSize-=sizeof(FRECORD);
        nSize-=S_ALIGN_UP(nRecordSize,2);
    }

    return listRecords;
}

X_Ar::FRECORD X_Ar::readFRECORD(qint64 nOffset)
{
    FRECORD record={};

    read_array(nOffset+offsetof(FRECORD,fileId),record.fileId,sizeof(record.fileId));
    read_array(nOffset+offsetof(FRECORD,fileMod),record.fileMod,sizeof(record.fileMod));
    read_array(nOffset+offsetof(FRECORD,ownerId),record.ownerId,sizeof(record.ownerId));
    read_array(nOffset+offsetof(FRECORD,groupId),record.groupId,sizeof(record.groupId));
    read_array(nOffset+offsetof(FRECORD,fileMode),record.fileMode,sizeof(record.fileMode));
    read_array(nOffset+offsetof(FRECORD,fileSize),record.fileSize,sizeof(record.fileSize));
    read_array(nOffset+offsetof(FRECORD,endChar),record.endChar,sizeof(record.endChar));

    return record;
}