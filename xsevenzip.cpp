/* Copyright (c) 2017-2025 hors<horsicq@gmail.com>
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
#include "xsevenzip.h"

XSevenZip::XSevenZip(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XSevenZip::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (getSize() > (qint64)sizeof(SIGNATUREHEADER)) {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (compareSignature(&memoryMap, "'7z'BCAF271C", 0, pPdStruct)) {
            // More checks
            SIGNATUREHEADER signatureHeader = _read_SIGNATUREHEADER(0);
            bResult = isOffsetAndSizeValid(&memoryMap, sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset, signatureHeader.NextHeaderSize);
        }
    }

    return bResult;
}

bool XSevenZip::isValid(QIODevice *pDevice)
{
    XSevenZip xsevenzip(pDevice);

    return xsevenzip.isValid();
}

QString XSevenZip::getVersion()
{
    return QString("%1.%2").arg(read_uint8(6)).arg(read_uint8(7), 1, 10, QChar('0'));
}

quint64 XSevenZip::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    quint64 nResult = 0;

    SIGNATUREHEADER signatureHeader = {};

    if (read_array(0, (char *)&signatureHeader, sizeof(SIGNATUREHEADER)) == sizeof(SIGNATUREHEADER)) {
        qint64 nCurrentOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset;
        qint64 nMaxOffset = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset + signatureHeader.NextHeaderSize;

        // bool bSuccess = true;

        STATE _state = {};
        _state.nCurrentOffset = nCurrentOffset;
        _state.nMaxOffset = nMaxOffset;

        bool bSuccess = true;
        quint64 nHeaderId = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);

        if (nHeaderId == k7zIdHeader) {
            // TODO
        } else if (nHeaderId == k7zIdEncodedHeader) {
            // k7zIdEncodedHeader
            // k7zIdPackInfo
            // k7zIdUnpackInfo
            nHeaderId = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
            bSuccess &= (nHeaderId == k7zIdPackInfo);

            _state.nPackPosition = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
            _state.nNumberOfPackStreams = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);

            for (qint32 i = 0; (i < (qint32)(_state.nNumberOfPackStreams)) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nPackSize = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
                _state.listPackSizes.append(nPackSize);
            }

            for (qint32 i = 0; (i < (qint32)(_state.nNumberOfPackStreams)) && isPdStructNotCanceled(pPdStruct); i++) {
                quint64 nCRC = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
                _state.listCRC.append(nCRC);
            }

            nHeaderId = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
            bSuccess &= (nHeaderId == k7zIdEnd);

            nHeaderId = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
            bSuccess &= (nHeaderId == k7zIdUnpackInfo);
            nHeaderId = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
            bSuccess &= (nHeaderId == k7zIdFolder);
            qint32 nNumberOfFolders = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
            // TODO Check
            quint8 nExtraByte = _readBYTE(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);

            if (nExtraByte == 0) {
                // TODO
            } else {
                // TODO
            }

            for (qint32 i = 0; i < nNumberOfFolders; i++) {
                qint32 nNumberOfCoders = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
                qDebug("Number of coders: %d", nNumberOfCoders);

                for (qint32 j = 0; j < nNumberOfCoders; j++) {
                    quint8 nMainByte = _readBYTE(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
                    bSuccess = ((nMainByte & 0xC0) == 0);
                    qint32 nIdSize = nMainByte & 0x0F;
                    bSuccess = (nIdSize <= 8);
                    // TODO SKIP function
                    qint32 nCodersInStreams = 1;
                    qint32 nCodersOutStreams = 0;
                    qint32 nPropertySize = 0;

                    Q_UNUSED(nCodersInStreams)

                    if ((nMainByte & 0x10) != 0) {
                        nCodersInStreams = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess) + 1;
                        nCodersOutStreams = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
                        bSuccess = (nCodersOutStreams == 1);
                    }

                    if ((nMainByte & 0x20) != 0) {
                        nPropertySize = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
                        bSuccess = _state.nCurrentOffset + nPropertySize < nMaxOffset;
                    }
                }
            }

            quint32 nTest = _readIntPackedValue(&(_state.nCurrentOffset), _state.nMaxOffset, &bSuccess);
            qDebug("Test: %d", nTest);

            qDebug("%s", idToSring((EIdEnum)nHeaderId).toLatin1().data());
        }

        // _handle(&_state, pPdStruct);

        nResult = _state.nNumberOfPackStreams;

        // quint64 nHeaderId = _readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess);

        // if (bSuccess) {
        //     if (nHeaderId == k7zIdHeader) {
        //         quint64 nSubHeaderId = _readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess);
        //         bSuccess &= (nSubHeaderId == k7zIdMainStreamsInfo);

        //         // Pack Information
        //         if (bSuccess) {
        //             bSuccess &= (_readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess) == k7zIdPackInfo);

        //             quint64 nPackPosition = _readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess);
        //             quint64 nCountOfPack = _readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess);

        //             Q_UNUSED(nPackPosition)

        //             if (bSuccess) {
        //                 for (quint64 i = 0; i < nCountOfPack; i++) {
        //                     quint64 nSizeOfPackStream = _readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess);

        //                     Q_UNUSED(nSizeOfPackStream)
        //                     // TODO
        //                 }

        //                 for (quint64 i = 0; i < nCountOfPack; i++) {
        //                     quint64 nCRCOfPackStream = _readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess);

        //                     Q_UNUSED(nCRCOfPackStream)
        //                     // TODO
        //                 }
        //             }

        //             bSuccess &= (_readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess) == k7zIdEnd);
        //         }

        //         // Coders Information
        //         if (bSuccess) {
        //             bSuccess &= (_readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess) == k7zIdUnpackInfo);

        //             bSuccess &= (_readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess) == k7zIdFolder);

        //             quint64 nNumberOfFolders = _readIntPackedValue(&nCurrentOffset, nMaxOffset, &bSuccess);

        //             Q_UNUSED(nNumberOfFolders)
        //             // qDebug("TEST");
        //         }
        //     } else if (nHeaderId == k7zIdPackInfo) {
        //         // qDebug("k7zIdPackInfo");
        //     }
        // }

        //        if(isOffsetAndSizeValid(&memoryMap,nCurrentOffset,signatureHeader.NextHeaderSize)) // TODO Handle errors!
        //        {
        //            quint64 nCurrentSize=0;

        //            while(nCurrentSize<signatureHeader.NextHeaderSize)
        //            {
        //                PACKED pn=get_packedNumber(nCurrentOffset);

        //                if(pn.nValue==k7zIdHeader)
        //                {
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;

        //                    pn=get_packedNumber(nCurrentOffset);
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;
        //                }
        //                else if(pn.nValue==k7zIdPackInfo)
        //                {
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;
        //                    pn=get_packedNumber(nCurrentOffset); // Offset
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;
        //                    pn=get_packedNumber(nCurrentOffset); // Number of Streams

        //                    nResult=pn.nValue;

        //                    break;
        //                }

        ////                QString sDebugString=QString("%1 %2").arg(uleb.nValue).arg(uleb.nByteSize);
        ////                qDebug("%s",sDebugString.toLatin1().data());
        //                nCurrentOffset+=pn.nByteSize;
        //                nCurrentSize+=pn.nByteSize;
        //            }

        //            // TODO Encrypted
        //        }
    }

    return nResult;
}

QList<XArchive::RECORD> XSevenZip::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nLimit)

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    QList<XArchive::RECORD> listResult;

    return listResult;

    // TODO

    XINFO xinfo = {};

    if (read_array(0, (char *)&(xinfo.signatureRecord), sizeof(SIGNATUREHEADER)) == sizeof(SIGNATUREHEADER))  // TODO read function
    {
        _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);

        qint64 nCurrentOffset = sizeof(SIGNATUREHEADER) + xinfo.signatureRecord.NextHeaderOffset;

        qint32 nXSize = getXRecord(&memoryMap, nCurrentOffset, &(xinfo.mainXRecord));

        nCurrentOffset += nXSize;

        //        if(isOffsetAndSizeValid(&memoryMap,nCurrentOffset,xinfo.signatureRecord.NextHeaderSize)) // TODO Handle errors!
        //        {

        // Header
        // k7zIdHeader
        // k7zIdMainStreamsInfo
        //  k7zIdPackInfo
        //      dataOffset
        //      numberOfFiles;
        //      Sizes[numberOfFiles]
        //  k7zIdUnpackInfo
        //  k7zIdEnd
        //  k7zIdUnpackInfo
        //      k7zIdFolder
        //      numberOfFiles
        //      0
        //      NumCoders[numberOfFiles]
        //      MainByte[numberOfFiles]
        //      Methods[numberOfFiles]
        //      k7zIdCodersUnpackSize
        //      numberOfFiles
        //      Sizes[numberOfFiles]

        //            PACKED pn=get_packedNumber(nCurrentOffset);

        //            while(true)
        //            {
        //                PACKED pn=get_packedNumber(nCurrentOffset);

        //                if(pn.nValue==k7zIdHeader)
        //                {
        //                    qDebug("k7zIdHeader");
        //                }
        //                else if(pn.nValue==k7zIdMainStreamsInfo)
        //                {
        //                    qDebug("k7zIdMainStreamsInfo");
        //                }
        //                else if(pn.nValue==k7zIdPackInfo)
        //                {
        //                    qDebug("k7zIdPackInfo");
        //                }
        //                else if(pn.nValue==k7zIdUnpackInfo)
        //                {
        //                    qDebug("k7zIdUnpackInfo");
        //                }
        //                else if(pn.nValue==k7zIdFolder)
        //                {
        //                    qDebug("k7zIdFolder");
        //                }
        //                else if(pn.nValue==k7zIdCodersUnpackSize)
        //                {
        //                    qDebug("k7zIdCodersUnpackSize");
        //                }
        //                else if(pn.nValue==k7zIdEncodedHeader)
        //                {
        //                    qDebug("k7zIdEncodedHeader");
        //                }

        //                qDebug("Test %lld",pn.nValue);

        //                nCurrentOffset+=pn.nByteSize;

        //                if(pn.nValue==0)
        //                {
        //                    break;
        //                }
        //            }

        //            XHEADER header={};

        //            getEncodedHeader(nCurrentOffset,&header);

        //            quint64 nCurrentSize=0;

        //            qint64 nDataOffset=0;

        //            while(nCurrentSize<signatureHeader.NextHeaderSize)
        //            {
        //                PACKED pn=get_packedNumber(nCurrentOffset);

        ////                if(pn.nValue==k7zIdPackInfo)
        ////                {
        ////                    nCurrentOffset+=pn.nByteSize;
        ////                    nCurrentSize+=pn.nByteSize;
        ////                    pn=get_packedNumber(nCurrentOffset); // Offset
        ////                    nDataOffset=pn.nValue;
        ////                    nCurrentSize+=pn.nByteSize;
        ////                    pn=get_packedNumber(nCurrentOffset); // Number of Streams
        ////                    nNumberOfFiles=pn.nValue;

        ////                    for(qint64 i=0;i<nNumberOfFiles;i++)
        ////                    {
        ////                        RECORD record={};
        ////                        listResult.append(record);
        ////                    }
        ////                }

        //                QString sDebugString=QString("%1 %2 %3").arg(pn.nValue).arg(pn.nByteSize).arg(idToSring((EIdEnum)pn.nValue));
        //                qDebug("%s",sDebugString.toLatin1().data());
        //                nCurrentOffset+=pn.nByteSize;
        //                nCurrentSize+=pn.nByteSize;

        //                if(pn.nValue==k7zIdEncodedHeader)
        //                {

        //                }
        //                if(pn.nValue==k7zIdPackInfo)
        //                {
        //                    pn=get_packedNumber(nCurrentOffset);
        //                    qDebug("Current Offset: %llu",pn.nValue);
        //                    nDataOffset=pn.nValue;
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;
        //                    pn=get_packedNumber(nCurrentOffset);
        //                    qint64 nNumberOfStreams=pn.nValue;
        //                    qDebug("Number Of Streams: %lld",nNumberOfStreams);
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;

        //                    pn=get_packedNumber(nCurrentOffset);
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;

        //                    if(pn.nValue==k7zIdSize)
        //                    {
        //                        qDebug("Size");

        //                        for(qint32 i=0;i<nNumberOfStreams;i++)
        //                        {
        //                            pn=get_packedNumber(nCurrentOffset);
        //                            qDebug("Current Size: %llu",pn.nValue);
        //                            nDataOffset=pn.nValue;
        //                            nCurrentOffset+=pn.nByteSize;
        //                            nCurrentSize+=pn.nByteSize;
        //                        }
        //                    }

        //                    pn=get_packedNumber(nCurrentOffset);

        //                    if(pn.nValue==k7zIdEnd)
        //                    {
        //                        qDebug("End");
        //                    }

        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;
        //                }
        //                if(pn.nValue==k7zIdUnpackInfo)
        //                {
        //                    pn=get_packedNumber(nCurrentOffset);
        //                    nCurrentOffset+=pn.nByteSize;
        //                    nCurrentSize+=pn.nByteSize;

        //                    if(pn.nValue==k7zIdFolder)
        //                    {
        //                        pn=get_packedNumber(nCurrentOffset);
        //                        qint64 nNumberOfFolders=pn.nValue;
        //                        qDebug("Number of folders: %llu",pn.nValue);
        //                        nCurrentOffset+=pn.nByteSize;
        //                        nCurrentSize+=pn.nByteSize;

        //                        quint8 nExtra=read_uint8(nCurrentOffset);

        //                        Q_UNUSED(nExtra)
        //                        // TODO compare to 0
        //                        nCurrentOffset++;
        //                        nCurrentSize++;

        //                        for(qint32 i=0;i<nNumberOfFolders;i++)
        //                        {
        //                            pn=get_packedNumber(nCurrentOffset);
        //                            qint64 nNumberOfCoders=pn.nValue;
        //                            qDebug("Number of coders: %lld",nNumberOfCoders);
        //                            // TODO
        //                            nCurrentOffset+=pn.nByteSize;
        //                            nCurrentSize+=pn.nByteSize;

        //                            for(qint32 j=0;j<nNumberOfCoders;j++)
        //                            {
        //                                quint8 nMainByte=read_uint8(nCurrentOffset);

        //                                nCurrentOffset++;
        //                                nCurrentSize++;

        //                                if((nMainByte&0xC0)==0)
        //                                {
        //                                    qint32 nCoderSize=nMainByte&0x0F;

        //                                    if((nCoderSize<=8)&&(nCoderSize<getSize()))
        //                                    {
        //                                        nCurrentOffset+=nCoderSize;
        //                                        nCurrentSize+=nCoderSize;
        //                                    }
        //                                    else
        //                                    {
        //                                        qDebug("Error");
        //                                    }
        //                                }
        //                                else
        //                                {
        //                                    qDebug("Error");
        //                                }
        //                                // TODO compare to 0
        //                                // TODO Check !!!
        //                            }
        //                        }
        //                    }
        //                }
        //            }
        //            // TODO Encrypted
        //            Q_UNUSED(nDataOffset)
        //        }
    }

    return listResult;
}

qint64 XSevenZip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    qint64 nResult = 0;

    SIGNATUREHEADER signatureHeader = {};

    // TODO Check
    if (read_array(0, (char *)&signatureHeader, sizeof(SIGNATUREHEADER)) == sizeof(SIGNATUREHEADER)) {
        nResult = sizeof(SIGNATUREHEADER) + signatureHeader.NextHeaderOffset + signatureHeader.NextHeaderSize;
    }

    return nResult;
}

QString XSevenZip::getFileFormatExt()
{
    return "7z";
}

QString XSevenZip::getFileFormatExtsString()
{
    return "7-Zip (*.7z)";
}

XBinary::MODE XSevenZip::getMode()
{
    return XBinary::MODE_DATA;
}

QString XSevenZip::getMIMEString()
{
    return "application/x-7z-compressed";
}

QString XSevenZip::getArch()
{
    return QString();
}

XSevenZip::SIGNATUREHEADER XSevenZip::_read_SIGNATUREHEADER(qint64 nOffset)
{
    SIGNATUREHEADER result = {};

    read_array(nOffset, (char *)result.kSignature, 6);
    result.Major = read_uint8(nOffset + 6);
    result.Minor = read_uint8(nOffset + 7);
    result.StartHeaderCRC = read_uint32(nOffset + 8);
    result.NextHeaderOffset = read_uint64(nOffset + 12);
    result.NextHeaderSize = read_uint64(nOffset + 20);
    result.NextHeaderCRC = read_uint32(nOffset + 28);

    return result;
}

XBinary::ENDIAN XSevenZip::getEndian()
{
    return ENDIAN_LITTLE;
}

QList<XBinary::MAPMODE> XSevenZip::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_REGIONS);

    return listResult;
}

XBinary::_MEMORY_MAP XSevenZip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(mapMode)

    _MEMORY_MAP result = {};

    result.nModuleAddress = getModuleAddress();
    result.nBinarySize = getSize();

    result.fileType = getFileType();
    result.mode = getMode();
    result.sArch = getArch();
    result.endian = getEndian();
    result.sType = getTypeAsString();

    qint64 nOffset = 0;
    qint32 nIndex = 0;

    {
        _MEMORY_RECORD record = {};

        record.nIndex = nIndex++;
        record.type = MMT_HEADER;
        record.nOffset = nOffset;
        record.nSize = sizeof(SIGNATUREHEADER);
        record.nAddress = -1;
        record.sName = tr("Header");

        result.listRecords.append(record);
    }

    qint64 nNextHeaderOffset = sizeof(SIGNATUREHEADER) + read_uint32(nOffset + offsetof(SIGNATUREHEADER, NextHeaderOffset));
    qint64 nNextHeaderSize = read_uint32(nOffset + offsetof(SIGNATUREHEADER, NextHeaderSize));

    if (nNextHeaderOffset - sizeof(SIGNATUREHEADER) > 0) {
        _MEMORY_RECORD record = {};

        record.nIndex = nIndex++;
        record.type = MMT_DATA;
        record.nOffset = sizeof(SIGNATUREHEADER);
        record.nSize = nNextHeaderOffset - sizeof(SIGNATUREHEADER);
        record.nAddress = -1;
        record.sName = tr("Data");

        result.listRecords.append(record);
    }

    if (nNextHeaderSize) {
        _MEMORY_RECORD record = {};

        record.nIndex = nIndex++;
        record.type = MMT_HEADER;
        record.nOffset = nNextHeaderOffset;
        record.nSize = nNextHeaderSize;
        record.nAddress = -1;
        record.sName = tr("Header");

        result.listRecords.append(record);
    }

    _handleOverlay(&result);

    return result;
}

XBinary::FT XSevenZip::getFileType()
{
    return FT_7Z;
}

QString XSevenZip::idToSring(XSevenZip::EIdEnum id)
{
    QString sResult = tr("Unknown");

    switch (id) {
        case k7zIdEnd: sResult = QString("k7zIdEnd"); break;
        case k7zIdHeader: sResult = QString("k7zIdHeader"); break;
        case k7zIdArchiveProperties: sResult = QString("k7zIdArchiveProperties"); break;
        case k7zIdAdditionalStreamsInfo: sResult = QString("k7zIdAdditionalStreamsInfo"); break;
        case k7zIdMainStreamsInfo: sResult = QString("k7zIdMainStreamsInfo"); break;
        case k7zIdFilesInfo: sResult = QString("k7zIdFilesInfo"); break;
        case k7zIdPackInfo: sResult = QString("k7zIdPackInfo"); break;
        case k7zIdUnpackInfo: sResult = QString("k7zIdUnpackInfo"); break;
        case k7zIdSubStreamsInfo: sResult = QString("k7zIdSubStreamsInfo"); break;
        case k7zIdSize: sResult = QString("k7zIdSize"); break;
        case k7zIdCRC: sResult = QString("k7zIdCRC"); break;
        case k7zIdFolder: sResult = QString("k7zIdFolder"); break;
        case k7zIdCodersUnpackSize: sResult = QString("k7zIdCodersUnpackSize"); break;
        case k7zIdNumUnpackStream: sResult = QString("k7zIdNumUnpackStream"); break;
        case k7zIdEmptyStream: sResult = QString("k7zIdEmptyStream"); break;
        case k7zIdEmptyFile: sResult = QString("k7zIdEmptyFile"); break;
        case k7zIdAnti: sResult = QString("k7zIdAnti"); break;
        case k7zIdName: sResult = QString("k7zIdName"); break;
        case k7zIdCTime: sResult = QString("k7zIdCTime"); break;
        case k7zIdATime: sResult = QString("k7zIdATime"); break;
        case k7zIdMTime: sResult = QString("k7zIdMTime"); break;
        case k7zIdWinAttrib: sResult = QString("k7zIdWinAttrib"); break;
        case k7zIdComment: sResult = QString("k7zIdComment"); break;
        case k7zIdEncodedHeader: sResult = QString("k7zIdEncodedHeader"); break;
        case k7zIdStartPos: sResult = QString("k7zIdStartPos"); break;
        case k7zIdDummy: sResult = QString("k7zIdDummy"); break;
    }

    return sResult;
}

qint32 XSevenZip::getXRecord(_MEMORY_MAP *pMemoryMap, qint64 nOffset, XRECORD *pXRecord, qint64 nExtra)
{
    qint32 nResult = 0;

    // if (isOffsetValid(pMemoryMap, nOffset)) {
    //     PACKED_INT pnMain = get_packedNumber(nOffset);

    //     nOffset += pnMain.nByteSize;
    //     nResult += pnMain.nByteSize;

    //     pXRecord->nID = pnMain.nValue;

    //     if (pXRecord->nID == k7zIdHeader) {
    //         PACKED_INT pnHeader = get_packedNumber(nOffset);

    //         if (pnHeader.nValue == k7zIdMainStreamsInfo) {
    //             XRECORD xrecord = {};

    //             qint32 nRSize = getXRecord(pMemoryMap, nOffset, &xrecord);

    //             pXRecord->listRecords.append(xrecord);

    //             nOffset += nRSize;
    //             nResult += nRSize;
    //         }
    //     } else if (pXRecord->nID == k7zIdMainStreamsInfo) {
    //         PACKED pnPackInfo = get_packedNumber(nOffset);

    //         if (pnPackInfo.nValue == k7zIdPackInfo) {
    //             XRECORD xrecord = {};

    //             qint32 nRSize = getXRecord(pMemoryMap, nOffset, &xrecord);

    //             pXRecord->listRecords.append(xrecord);

    //             nOffset += nRSize;
    //             nResult += nRSize;
    //         }

    //         PACKED pnUnpackInfo = get_packedNumber(nOffset);

    //         if (pnUnpackInfo.nValue == k7zIdUnpackInfo) {
    //             XRECORD xrecord = {};

    //             qint32 nRSize = getXRecord(pMemoryMap, nOffset, &xrecord);

    //             pXRecord->listRecords.append(xrecord);

    //             nOffset += nRSize;
    //             nResult += nRSize;
    //         }
    //     } else if (pXRecord->nID == k7zIdPackInfo) {
    //         PACKED pnPackPos = get_packedNumber(nOffset);
    //         pXRecord->nPackPos = pnPackPos.nValue;
    //         nOffset += pnPackPos.nByteSize;
    //         nResult += pnPackPos.nByteSize;

    //         PACKED pnNumPackStreams = get_packedNumber(nOffset);
    //         pXRecord->nNumPackStreams = pnNumPackStreams.nValue;
    //         nOffset += pnNumPackStreams.nByteSize;
    //         nResult += pnNumPackStreams.nByteSize;

    //         PACKED pnSize = get_packedNumber(nOffset);

    //         if (pnSize.nValue == k7zIdSize) {
    //             XRECORD xrecord = {};

    //             qint32 nRSize = getXRecord(pMemoryMap, nOffset, &xrecord, pXRecord->nNumPackStreams);

    //             pXRecord->listRecords.append(xrecord);

    //             nOffset += nRSize;
    //             nResult += nRSize;
    //         }

    //         PACKED pnCRC = get_packedNumber(nOffset);

    //         if (pnCRC.nValue == k7zIdCRC) {
    //             //                // TODO !!!
    //             //                qDebug("TODO: CRC");
    //         }

    //         PACKED pnEnd = get_packedNumber(nOffset);

    //         if (pnEnd.nValue == k7zIdEnd) {
    //             nOffset += pnEnd.nByteSize;
    //             nResult += pnEnd.nByteSize;
    //         }
    //     } else if (pXRecord->nID == k7zIdSize) {
    //         for (qint64 i = 0; i < nExtra; i++) {
    //             PACKED pnSize = get_packedNumber(nOffset);

    //             pXRecord->listSizes.append(pnSize.nValue);
    //             nOffset += pnSize.nByteSize;
    //             nResult += pnSize.nByteSize;
    //         }
    //     } else if (pXRecord->nID == k7zIdUnpackInfo) {
    //         PACKED pnFolder = get_packedNumber(nOffset);

    //         if (pnFolder.nValue == k7zIdFolder) {
    //             XRECORD xrecord = {};

    //             qint32 nRSize = getXRecord(pMemoryMap, nOffset, &xrecord);

    //             pXRecord->listRecords.append(xrecord);

    //             nOffset += nRSize;
    //             nResult += nRSize;
    //         }
    //     } else if (pXRecord->nID == k7zIdFolder) {
    //         PACKED pnNumFolders = get_packedNumber(nOffset);
    //         pXRecord->nNumFolders = pnNumFolders.nValue;
    //         nOffset += pnNumFolders.nByteSize;
    //         nResult += pnNumFolders.nByteSize;

    //         //            quint8 nExtraByte=read_uint8(nOffset);
    //         //            nOffset++;
    //         //            nResult++;

    //         // TODO
    //         //            QString _sTest=getSignature(nOffset,16);
    //         //            qDebug(_sTest.toLatin1().data());
    //         //            QString sTest=idToSring((XSevenZip::EIdEnum)pnPackPos.nValue);
    //     }
    // }

    return nResult;
}

quint64 XSevenZip::_readIntPackedValue(qint64 *pnOffset, qint64 nMaxOffset, bool *pbSuccess)
{
    quint64 nResult = 0;

    // if (((*pnOffset) < nMaxOffset) && (*pbSuccess)) {
    //     PACKED pnValue = get_packedNumber(*pnOffset);

    //     if (((*pnOffset) + pnValue.nByteSize) <= nMaxOffset) {
    //         nResult = pnValue.nValue;

    //         (*pnOffset) += pnValue.nByteSize;

    //         *pbSuccess = true;
    //     }
    // } else {
    //     *pbSuccess = false;
    // }

    return nResult;
}

quint8 XSevenZip::_readBYTE(qint64 *pnOffset, qint64 nMaxOffset, bool *pbSuccess)
{
    quint8 nResult = 0;

    if (((*pnOffset) < nMaxOffset) && (*pbSuccess)) {
        nResult = read_uint8(*pnOffset);

        (*pnOffset) += 1;
        *pbSuccess = true;
    } else {
        *pbSuccess = false;
    }

    return nResult;
}

quint64 XSevenZip::_handle(STATE *pState, PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;
    // https://py7zr.readthedocs.io/en/latest/archive_format.html
    if (!(pPdStruct->bIsStop)) {
        bool bSuccess = true;
        nResult = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);

#ifdef QT_DEBUG
        qDebug("%llX", nResult);
        qDebug("%s", idToSring((XSevenZip::EIdEnum)nResult).toUtf8().data());
#endif

        if (bSuccess) {
            if (nResult == k7zIdHeader) {
                // k7zIdHeader
                // k7zIdMainStreamsInfo
                // k7zIdFilesInfo
                // k7zIdPackInfo
                // k7zIdUnpackInfo
                // k7zIdSubStreamsInfo
                // k7zIdSize
                // Sizes[NumPackStreams]
                // k7zIdCRC
                // CRCs[NumPackStreams]
                // k7zIdEnd
                _handle(pState, pPdStruct);
            } else if (nResult == k7zIdEncodedHeader) {
                // k7zIdEncodedHeader
                // k7zIdPackInfo
                // k7zIdUnpackInfo
                // k7zIdSubStreamsInfo
                // k7zIdSize
                // Sizes[NumPackStreams]
                // k7zIdCRC
                // CRCs[NumPackStreams]
                // k7zIdEnd
                _handle(pState, pPdStruct);  // k7zIdPackInfo
                _handle(pState, pPdStruct);  // k7zIdUnpackInfo
                _handle(pState, pPdStruct);  // k7zIdSubStreamsInfo
            } else if (nResult == k7zIdPackInfo) {
                // k7zIdPackInfo
                // PackPosition
                // NumPackStreams
                // Sizes[NumPackStreams]
                // CRCs[NumPackStreams]
                // k7zIdEnd

                pState->nPackPosition = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);
                pState->nNumberOfPackStreams = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);

                for (qint32 i = 0; (i < (qint32)(pState->nNumberOfPackStreams)) && isPdStructNotCanceled(pPdStruct); i++) {
                    quint64 nPackSize = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);
                    pState->listPackSizes.append(nPackSize);
                }

                for (qint32 i = 0; (i < (qint32)(pState->nNumberOfPackStreams)) && isPdStructNotCanceled(pPdStruct); i++) {
                    quint64 nCRC = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);
                    pState->listCRC.append(nCRC);
                }

                _handle(pState, pPdStruct);  // k7zIdEnd
            } else if (nResult == k7zIdUnpackInfo) {
                // k7zIdUnpackInfo
                // k7zIdFolder
                // NumFolders
                // Extra
                // NumCoders[NumFolders]
                // MainByte[NumFolders]
                // Methods[NumFolders]
                // k7zIdCodersUnpackSize
                // NumFolders
                // Sizes[NumFolders]
                _handle(pState, pPdStruct);  // k7zIdFolder
                _handle(pState, pPdStruct);  // k7zIdCodersUnpackSize
                _handle(pState, pPdStruct);  // k7zIdEnd
            } else if (nResult == k7zIdFolder) {
                pState->nNumberOfFolders = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);
                pState->nExtraByte = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);

                for (qint32 i = 0; ((quint64)i < pState->nNumberOfFolders) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                    pState->nNumberOfProperties = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);

                    for (qint32 j = 0; ((quint64)j < pState->nNumberOfProperties) && (!(pPdStruct->bIsStop)); j++) {
                        quint64 nFlag = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);
                        qint32 nCodecSize = getBits_uint8(nFlag, 0, 4);
                        // qint32 nIsComplex = getBits_uint8(nFlag, 4, 1);
                        qint32 nIsAttr = getBits_uint8(nFlag, 5, 1);
                        // TODO read codec
                        pState->nCurrentOffset += nCodecSize;

                        if (nIsAttr) {
                            quint64 nPropertySize = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);
                            // TODO read property
                            pState->nCurrentOffset += nPropertySize;
                        }
                    }
                }
            } else if (nResult == k7zIdCodersUnpackSize) {
                for (qint32 i = 0; ((quint64)i < pState->nNumberOfPackStreams) && (!(pPdStruct->bIsStop)); i++) {
                    quint64 nUnpackSize = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);
                    pState->listUnpackSizes.append(nUnpackSize);
                }
            } else if (nResult == k7zIdFilesInfo) {
                quint64 nNumberOfFiles = _readIntPackedValue(&(pState->nCurrentOffset), pState->nMaxOffset, &bSuccess);

                for (qint32 i = 0; ((quint64)i < nNumberOfFiles) && (!(pPdStruct->bIsStop)); i++) {
                    // TODO
                }
            } else if (nResult == k7zIdEnd) {
                // END
            }
        }
    }

    return nResult;
}

// qint64 XSevenZip::getHeader(qint64 nOffset,XSevenZip::XHEADER *pHeader)
//{
//     qint64 nCurrent=0;

//    PACKED pn={};

//    pn=get_packedNumber(nOffset+nCurrent);

//    if(pn.nValue==k7zIdHeader)
//    {
//        nCurrent+=pn.nByteSize;
//        pHeader->bIsValid=true;

//        nCurrent+=getPackInfo(nOffset+nCurrent,&(pHeader->xPackInfo));
//        nCurrent+=getUnpackInfo(nOffset+nCurrent,&(pHeader->xUnpackInfo));
//    }

//    return nCurrent;
//}

// qint64 XSevenZip::getEncodedHeader(qint64 nOffset,XSevenZip::XHEADER *pHeader)
//{
//     qint64 nCurrent=0;

//    PACKED pn={};

//    pn=get_packedNumber(nOffset+nCurrent);

//    if(pn.nValue==k7zIdEncodedHeader)
//    {
//        nCurrent+=pn.nByteSize;
//        pHeader->bIsValid=true;

//        nCurrent+=getPackInfo(nOffset+nCurrent,&(pHeader->xPackInfo));
//        nCurrent+=getUnpackInfo(nOffset+nCurrent,&(pHeader->xUnpackInfo));
//    }

//    return nCurrent;
//}

// qint64 XSevenZip::getPackInfo(qint64 nOffset,XSevenZip::XPACKINFO *pPackInfo)
//{
//     qint64 nCurrent=0;

//    PACKED pn={};
//    pn=get_packedNumber(nOffset+nCurrent);

//    if(pn.nValue==k7zIdPackInfo)
//    {
//        nCurrent+=pn.nByteSize;
//        pn=get_packedNumber(nOffset+nCurrent);
//        pPackInfo->nDataOffset=pn.nValue;
//        nCurrent+=pn.nByteSize;
//        pn=get_packedNumber(nOffset+nCurrent);

//        quint64 nSize=pn.nValue;
//        nCurrent+=pn.nByteSize;

//        pn=get_packedNumber(nOffset+nCurrent);

//        if(pn.nValue==k7zIdSize)
//        {
//            nCurrent+=pn.nByteSize;

//            for(qint32 i=0;i<(qint32)nSize;i++)
//            {
//                pn=get_packedNumber(nOffset+nCurrent);

//                pPackInfo->listSizes.append(pn.nValue);
//                nCurrent+=pn.nByteSize;
//            }
//            pn=get_packedNumber(nOffset+nCurrent);

//            if(pn.nValue==k7zIdEnd)
//            {
//                pPackInfo->bIsValid=true;
//            }

//            nCurrent+=pn.nByteSize;
//        }
//    }

//    return nCurrent;
//}

// qint64 XSevenZip::getUnpackInfo(qint64 nOffset,XSevenZip::XUNPACKINFO *pUnpackInfo)
//{
//     qint64 nCurrent=0;

//    PACKED pn={};
//    pn=get_packedNumber(nOffset+nCurrent);

//    if(pn.nValue==k7zIdUnpackInfo)
//    {
//        nCurrent+=pn.nByteSize;
//        pn=get_packedNumber(nOffset+nCurrent);

//        if(pn.nValue==k7zIdFolder)
//        {
//            nCurrent+=pn.nByteSize;
//            pn=get_packedNumber(nOffset+nCurrent);

//            quint64 nFolderCount=pn.nValue;
//            nCurrent+=pn.nByteSize;

//            pn=get_packedNumber(nOffset+nCurrent);

//            if(pn.nValue==0) // Extra
//            {
//                nCurrent+=pn.nByteSize;

//                // TODO Check!
//                for(qint32 i=0;i<(qint32)nFolderCount;i++)
//                {
//                    pn=get_packedNumber(nOffset+nCurrent);

//                    quint64 nNumberOfCoders=pn.nValue;
//                    nCurrent+=pn.nByteSize;

//                    for(qint32 j=0;j<(qint32)nNumberOfCoders;j++)
//                    {
//                        // TODO read coder function
//                        pn=get_packedNumber(nOffset+nCurrent);
//                        nCurrent+=pn.nByteSize;
//                    }
//                }

//                pn=get_packedNumber(nOffset+nCurrent);

//                // TODO Check k7zIdAdditionalStreamsInfo

//                if(pn.nValue==k7zIdCodersUnpackSize)
//                {
//                    nCurrent+=pn.nByteSize;

//                    for(qint32 i=0;i<(qint32)nFolderCount;i++)
//                    {
//                        pn=get_packedNumber(nOffset+nCurrent);

//                        pUnpackInfo->listCoderSizes.append(pn.nValue);
//                        nCurrent+=pn.nByteSize;
//                    }
//                }

//                pn=get_packedNumber(nOffset+nCurrent);

//                if(pn.nValue==k7zIdEnd)
//                {
//                    pUnpackInfo->bIsValid=true;
//                }

//                nCurrent+=pn.nByteSize;
//            }
//        }
//    }

//    return nCurrent;
//}
