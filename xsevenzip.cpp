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
#include "xsevenzip.h"

XSevenZip::XSevenZip(QIODevice *__pDevice) : XArchive(__pDevice)
{

}

bool XSevenZip::isValid()
{
    bool bResult=false;

    if(getSize()>sizeof(SignatureHeader))
    {
        if(compareSignature("'7z'BCAF271C"))
        {
            bResult=true;
        }
    }

    return bResult;
}

QString XSevenZip::getVersion()
{
    return QString("%1.%2").arg(read_uint8(6)).arg(read_uint8(7),1,10,QChar('0'));
}

quint64 XSevenZip::getNumberOfRecords()
{
    quint64 nResult=0;

    SignatureHeader signatureHeader;

    if(read_array(0,(char *)&signatureHeader,sizeof(SignatureHeader))==sizeof(SignatureHeader))
    {
        _MEMORY_MAP memoryMap=getMemoryMap();

        qint64 nCurrentOffset=sizeof(SignatureHeader)+signatureHeader.NextHeaderOffset;

        if(isOffsetAndSizeValid(&memoryMap,nCurrentOffset,signatureHeader.NextHeaderSize)) // TODO Handle errors!
        {
            quint64 nCurrentSize=0;

            while(nCurrentSize<signatureHeader.NextHeaderSize)
            {
                PACKEDNUMBER pn=get_packedNumber(nCurrentOffset);

                if(pn.nValue==k7zIdPackInfo)
                {
                    nCurrentOffset+=pn.nByteSize;
                    nCurrentSize+=pn.nByteSize;
                    pn=get_packedNumber(nCurrentOffset); // Offset
                    nCurrentOffset+=pn.nByteSize;
                    nCurrentSize+=pn.nByteSize;
                    pn=get_packedNumber(nCurrentOffset); // Number of Streams

                    nResult=pn.nValue;

                    break;
                }

//                QString sDebugString=QString("%1 %2").arg(uleb.nValue).arg(uleb.nByteSize);
//                qDebug("%s",sDebugString.toLatin1().data());
                nCurrentOffset+=pn.nByteSize;
                nCurrentSize+=pn.nByteSize;
            }

            // TODO Encrypted
        }
    }

    return nResult;
}

QList<XArchive::RECORD> XSevenZip::getRecords(qint32 nLimit)
{
    QList<XArchive::RECORD> listResult;

    SignatureHeader signatureHeader;

    if(read_array(0,(char *)&signatureHeader,sizeof(SignatureHeader))==sizeof(SignatureHeader))
    {
        _MEMORY_MAP memoryMap=getMemoryMap();

        qint64 nCurrentOffset=sizeof(SignatureHeader)+signatureHeader.NextHeaderOffset;

        if(isOffsetAndSizeValid(&memoryMap,nCurrentOffset,signatureHeader.NextHeaderSize)) // TODO Handle errors!
        {
            quint64 nCurrentSize=0;

            qint64 nDataOffset=0;
            qint64 nNumberOfFiles;

            while(nCurrentSize<signatureHeader.NextHeaderSize)
            {
                PACKEDNUMBER pn=get_packedNumber(nCurrentOffset);

//                if(pn.nValue==k7zIdPackInfo)
//                {
//                    nCurrentOffset+=pn.nByteSize;
//                    nCurrentSize+=pn.nByteSize;
//                    pn=get_packedNumber(nCurrentOffset); // Offset
//                    nDataOffset=pn.nValue;
//                    nCurrentSize+=pn.nByteSize;
//                    pn=get_packedNumber(nCurrentOffset); // Number of Streams
//                    nNumberOfFiles=pn.nValue;

//                    for(qint64 i=0;i<nNumberOfFiles;i++)
//                    {
//                        RECORD record={};
//                        listResult.append(record);
//                    }
//                }

                QString sDebugString=QString("%1 %2 %3").arg(pn.nValue).arg(pn.nByteSize).arg(idToSring((EIdEnum)pn.nValue));
                qDebug("%s",sDebugString.toLatin1().data());
                nCurrentOffset+=pn.nByteSize;
                nCurrentSize+=pn.nByteSize;

                if(pn.nValue==k7zIdEncodedHeader)
                {

                }
                if(pn.nValue==k7zIdPackInfo)
                {

                }
            }

            // TODO Encrypted
        }
    }

    return listResult;
}

QString XSevenZip::idToSring(XSevenZip::EIdEnum id)
{
    QString sResult="Unknown";

    switch(id)
    {
        case k7zIdEnd:                      sResult=QString("k7zIdEnd");                        break;
        case k7zIdHeader:                   sResult=QString("k7zIdHeader");                     break;
        case k7zIdArchiveProperties:        sResult=QString("k7zIdArchiveProperties");          break;
        case k7zIdAdditionalStreamsInfo:    sResult=QString("k7zIdAdditionalStreamsInfo");      break;
        case k7zIdMainStreamsInfo:          sResult=QString("k7zIdMainStreamsInfo");            break;
        case k7zIdFilesInfo:                sResult=QString("k7zIdFilesInfo");                  break;
        case k7zIdPackInfo:                 sResult=QString("k7zIdPackInfo");                   break;
        case k7zIdUnpackInfo:               sResult=QString("k7zIdUnpackInfo");                 break;
        case k7zIdSubStreamsInfo:           sResult=QString("k7zIdSubStreamsInfo");             break;
        case k7zIdSize:                     sResult=QString("k7zIdSize");                       break;
        case k7zIdCRC:                      sResult=QString("k7zIdCRC");                        break;
        case k7zIdFolder:                   sResult=QString("k7zIdFolder");                     break;
        case k7zIdCodersUnpackSize:         sResult=QString("k7zIdCodersUnpackSize");           break;
        case k7zIdNumUnpackStream:          sResult=QString("k7zIdNumUnpackStream");            break;
        case k7zIdEmptyStream:              sResult=QString("k7zIdEmptyStream");                break;
        case k7zIdEmptyFile:                sResult=QString("k7zIdEmptyFile");                  break;
        case k7zIdAnti:                     sResult=QString("k7zIdAnti");                       break;
        case k7zIdName:                     sResult=QString("k7zIdName");                       break;
        case k7zIdCTime:                    sResult=QString("k7zIdCTime");                      break;
        case k7zIdATime:                    sResult=QString("k7zIdATime");                      break;
        case k7zIdMTime:                    sResult=QString("k7zIdMTime");                      break;
        case k7zIdWinAttrib:                sResult=QString("k7zIdWinAttrib");                  break;
        case k7zIdComment:                  sResult=QString("k7zIdComment");                    break;
        case k7zIdEncodedHeader:            sResult=QString("k7zIdEncodedHeader");              break;
        case k7zIdStartPos:                 sResult=QString("k7zIdStartPos");                   break;
        case k7zIdDummy:                    sResult=QString("k7zIdDummy");                      break;
    }

    return sResult;
}
