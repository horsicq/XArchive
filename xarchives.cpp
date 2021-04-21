// copyright (c) 2020-2021 hors<horsicq@gmail.com>
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
#include "xarchives.h"

XArchives::XArchives(QObject *pParent) : QObject(pParent)
{

}

QList<XArchive::RECORD> XArchives::getRecords(QIODevice *pDevice, qint32 nLimit)
{
    QList<XArchive::RECORD> listResult;

    // TODO more
    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(pDevice,true);

    if(stFileTypes.contains(XArchive::FT_ZIP))
    {
        XZip xzip(pDevice);

        listResult=xzip.getRecords(nLimit);
    }
    else if(stFileTypes.contains(XArchive::FT_MACHOFAT))
    {
        XMACHOFat xmachofat(pDevice);

        listResult=xmachofat.getRecords(nLimit);
    }

    return listResult;
}

QList<XArchive::RECORD> XArchives::getRecords(QString sFileName, qint32 nLimit)
{
    QList<XArchive::RECORD> listResult;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        listResult=getRecords(&file,nLimit);

        file.close();
    }

    return listResult;
}

QByteArray XArchives::decompress(QIODevice *pDevice, XArchive::RECORD *pRecord, bool bHeaderOnly)
{
    QByteArray baResult;

    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(pDevice,true);

    if(stFileTypes.contains(XArchive::FT_ZIP))
    {
        XZip xzip(pDevice);

        baResult=xzip.decompress(pRecord,bHeaderOnly);
    }
    else if(stFileTypes.contains(XArchive::FT_MACHOFAT))
    {
        XMACHOFat xmachofat(pDevice);

        baResult=xmachofat.decompress(pRecord,bHeaderOnly);
    }

    return baResult;
}

QByteArray XArchives::decompress(QString sFileName, XArchive::RECORD *pRecord, bool bHeaderOnly)
{
    QByteArray baResult;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        baResult=decompress(&file,pRecord,bHeaderOnly);

        file.close();
    }

    return baResult;
}

bool XArchives::decompressToFile(QIODevice *pDevice, XArchive::RECORD *pRecord, QString sResultFileName, bool *pbIsStop)
{
    bool bResult=false;

    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(pDevice,true);

    // TODO more
    if(stFileTypes.contains(XArchive::FT_ZIP))
    {
        XZip xzip(pDevice);

        bResult=xzip.decompressToFile(pRecord,sResultFileName,pbIsStop);
    }
    else if(stFileTypes.contains(XArchive::FT_MACHOFAT))
    {
        XMACHOFat xmachofat(pDevice);

        bResult=xmachofat.decompressToFile(pRecord,sResultFileName,pbIsStop);
    }

    return bResult;
}

bool XArchives::decompressToFile(QString sFileName, XArchive::RECORD *pRecord, QString sResultFileName, bool *pbIsStop)
{
    bool bResult=false;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        bResult=decompressToFile(&file,pRecord,sResultFileName,pbIsStop);

        file.close();
    }

    return bResult;
}

bool XArchives::decompressToFile(QString sFileName, QString sRecordFileName, QString sResultFileName, bool *pbIsStop)
{
    bool bResult=false;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        QList<XArchive::RECORD> listRecords=getRecords(&file);

        XArchive::RECORD record=XArchive::getArchiveRecord(sRecordFileName,&listRecords);

        if(record.sFileName!="")
        {
             bResult=decompressToFile(&file,&record,sResultFileName,pbIsStop);
        }

        file.close();
    }

    return bResult;
}

bool XArchives::isArchiveRecordPresent(QIODevice *pDevice, QString sRecordFileName)
{
    bool bResult=false;

    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(pDevice,true);

    // TODO more
    if(stFileTypes.contains(XArchive::FT_ZIP))
    {
        XZip xzip(pDevice);

        bResult=xzip.isArchiveRecordPresent(sRecordFileName);
    }
    else if(stFileTypes.contains(XArchive::FT_MACHOFAT))
    {
        XMACHOFat xmachofat(pDevice);

        bResult=xmachofat.isArchiveRecordPresent(sRecordFileName);
    }

    return bResult;
}

bool XArchives::isArchiveRecordPresent(QString sFileName, QString sRecordFileName)
{
    bool bResult=false;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        bResult=isArchiveRecordPresent(&file,sRecordFileName);

        file.close();
    }

    return bResult;
}

bool XArchives::isArchiveOpenValid(QIODevice *pDevice, QSet<XBinary::FT> stAvailable)
{
    QSet<XBinary::FT> stFT=XBinary::getFileTypes(pDevice,true);

    if(!stAvailable.count())
    {
        stAvailable.insert(XBinary::FT_ZIP);
        stAvailable.insert(XBinary::FT_MACHOFAT);
    }

    return XBinary::isFileTypePresent(&stFT,&stAvailable);
}

bool XArchives::isArchiveOpenValid(QString sFileName, QSet<XBinary::FT> stAvailable)
{
    bool bResult=false;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        bResult=isArchiveOpenValid(&file,stAvailable);

        file.close();
    }

    return bResult;
}
