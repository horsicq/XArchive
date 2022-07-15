/* Copyright (c) 2020-2022 hors<horsicq@gmail.com>
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
#include "xarchives.h"

XArchives::XArchives(QObject *pParent) : QObject(pParent)
{

}

QList<XArchive::RECORD> XArchives::getRecords(QIODevice *pDevice,qint32 nLimit)
{
    QList<XArchive::RECORD> listResult;

    // TODO more !!!
    // CAB RAR
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

QList<XArchive::RECORD> XArchives::getRecords(QString sFileName,qint32 nLimit)
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

QList<XArchive::RECORD> XArchives::getRecordsFromDirectory(QString sDirectoryName,qint32 nLimit)
{
    QList<XArchive::RECORD> listResult;

    _findFiles(sDirectoryName,&listResult,nLimit);

    return listResult;
}

QByteArray XArchives::decompress(QIODevice *pDevice,XArchive::RECORD *pRecord,bool bHeaderOnly,XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(pDevice,true);

    if(stFileTypes.contains(XArchive::FT_ZIP))
    {
        XZip xzip(pDevice);

        baResult=xzip.decompress(pRecord,bHeaderOnly,pPdStruct);
    }
    else if(stFileTypes.contains(XArchive::FT_MACHOFAT))
    {
        XMACHOFat xmachofat(pDevice);

        baResult=xmachofat.decompress(pRecord,bHeaderOnly,pPdStruct);
    }

    return baResult;
}

QByteArray XArchives::decompress(QString sFileName,XArchive::RECORD *pRecord,bool bHeaderOnly,XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        baResult=decompress(&file,pRecord,bHeaderOnly,pPdStruct);

        file.close();
    }

    return baResult;
}

QByteArray XArchives::decompress(QIODevice *pDevice, QString sRecordFileName, bool bHeaderOnly, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listRecords=getRecords(pDevice);

    XArchive::RECORD record=XArchive::getArchiveRecord(sRecordFileName,&listRecords);

    return decompress(pDevice,&record,bHeaderOnly,pPdStruct);
}

QByteArray XArchives::decompress(QString sFileName, QString sRecordFileName, bool bHeaderOnly, XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        baResult=decompress(&file,sRecordFileName,bHeaderOnly,pPdStruct);

        file.close();
    }

    return baResult;
}

bool XArchives::decompressToFile(QIODevice *pDevice, XArchive::RECORD *pRecord, QString sResultFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult=false;

    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(pDevice,true);

    // TODO more !!!
    if(stFileTypes.contains(XArchive::FT_ZIP))
    {
        XZip xzip(pDevice);

        bResult=xzip.decompressToFile(pRecord,sResultFileName,pPdStruct);
    }
    else if(stFileTypes.contains(XArchive::FT_MACHOFAT))
    {
        XMACHOFat xmachofat(pDevice);

        bResult=xmachofat.decompressToFile(pRecord,sResultFileName,pPdStruct);
    }

    return bResult;
}

bool XArchives::decompressToFile(QString sFileName, XArchive::RECORD *pRecord, QString sResultFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult=false;

    QFile file;

    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        bResult=decompressToFile(&file,pRecord,sResultFileName,pPdStruct);

        file.close();
    }

    return bResult;
}

bool XArchives::decompressToFile(QString sFileName, QString sRecordFileName, QString sResultFileName, XBinary::PDSTRUCT *pPdStruct)
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
             bResult=decompressToFile(&file,&record,sResultFileName,pPdStruct);
        }

        file.close();
    }

    return bResult;
}

bool XArchives::isArchiveRecordPresent(QIODevice *pDevice,QString sRecordFileName)
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

bool XArchives::isArchiveRecordPresent(QString sFileName,QString sRecordFileName)
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

bool XArchives::isArchiveOpenValid(QIODevice *pDevice,QSet<XBinary::FT> stAvailable)
{
    bool bResult=false;

    if(pDevice)
    {
        QSet<XBinary::FT> stFT=XBinary::getFileTypes(pDevice,true);

        if(!stAvailable.count())
        {
            stAvailable.insert(XBinary::FT_ZIP);
            stAvailable.insert(XBinary::FT_MACHOFAT);
        }

        bResult=XBinary::isFileTypePresent(&stFT,&stAvailable);
    }

    return bResult;
}

bool XArchives::isArchiveOpenValid(QString sFileName,QSet<XBinary::FT> stAvailable)
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

void XArchives::_findFiles(QString sDirectoryName,QList<XArchive::RECORD> *pListRecords,qint32 nLimit)
{
    if((nLimit<pListRecords->count())||(nLimit==-1))
    {
        QFileInfo fi(sDirectoryName);

        if(fi.isFile())
        {
            XArchive::RECORD record={};

            record.compressMethod=XArchive::COMPRESS_METHOD_FILE;
            record.sFileName=fi.absoluteFilePath();
            record.nCompressedSize=fi.size();
            record.nUncompressedSize=fi.size();

            if((nLimit<pListRecords->count())||(nLimit==-1))
            {
                pListRecords->append(record);
            }
        }
        else if(fi.isDir())
        {
            QDir dir(sDirectoryName);

            QFileInfoList eil=dir.entryInfoList();

            qint32 nNumberOfFiles=eil.count();

            for(qint32 i=0;i<nNumberOfFiles;i++)
            {
                QString sFN=eil.at(i).fileName();

                if((sFN!=".")&&(sFN!=".."))
                {
                    _findFiles(eil.at(i).absoluteFilePath(),pListRecords,nLimit);
                }
            }
        }
    }
}
