/* Copyright (c) 2017-2024 hors<horsicq@gmail.com>
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
#include "xzip.h"

XZip::XZip(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XZip::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    _MEMORY_MAP memoryMap = XBinary::getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);  // TODO Check

    if (compareSignature(&memoryMap, "'PK'0304", 0, pPdStruct) || compareSignature(&memoryMap, "'PK'0506", 0, pPdStruct)) {
        bResult = true;
    }

    return bResult;
}

bool XZip::isValid(QIODevice *pDevice)
{
    XZip xzip(pDevice);

    return xzip.isValid();
}

QString XZip::getVersion()
{
    QString sResult;

    qint64 nECDOffset = findECDOffset(nullptr);

    quint16 nVersion = 0;

    if (nECDOffset != -1) {
        qint64 nOffset = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

        quint32 nSignature = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nSignature));

        if (nSignature == SIGNATURE_CFD) {
            nVersion = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nVersion));

            if (nVersion == 0) {
                nVersion = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nMinVersion));
            }
        }
    }

    if (nVersion == 0) {
        // The first record
        nVersion = read_uint16(0 + offsetof(CENTRALDIRECTORYFILEHEADER, nVersion));
    }

    if (nVersion) {
        sResult = QString("%1").arg((double)nVersion / 10, 0, 'f', 1);
    }

    return sResult;
}

bool XZip::isEncrypted()
{
    bool bResult = false;

    qint64 nECDOffset = findECDOffset(nullptr);

    bool bSuccess = false;

    if (nECDOffset != -1) {
        qint64 nOffset = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

        quint32 nSignature = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nSignature));

        if (nSignature == SIGNATURE_CFD) {
            quint16 nFlags = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nFlags));

            bResult = (nFlags & 0x1);
            bSuccess = true;
        }
    }

    if (!bSuccess) {
        // The first record
        quint16 nFlags = read_uint16(offsetof(CENTRALDIRECTORYFILEHEADER, nFlags));

        bResult = (nFlags & 0x1);
    }

    return bResult;
}

quint64 XZip::getNumberOfRecords(PDSTRUCT *pPdStruct)
{
    quint64 nResult = 0;

    qint64 nECDOffset = findECDOffset(pPdStruct);

    if (nECDOffset != -1) {
        nResult = read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));
    } else {
        qint64 nOffset = 0;

        for (qint32 i = 0; (!(pPdStruct->bIsStop)); i++) {
            quint32 nLocalSignature = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nSignature));
            quint32 nLocalFileNameSize = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nFileNameLength));
            quint32 nLocalExtraFieldSize = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nExtraFieldLength));
            quint32 nCompressedSize = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nCompressedSize));

            if (nLocalSignature != SIGNATURE_LFD) {
                break;
            }

            nResult++;

            nOffset += sizeof(LOCALFILEHEADER) + nLocalFileNameSize + nLocalExtraFieldSize + nCompressedSize;
        }
    }

    return nResult;
}

QList<XArchive::RECORD> XZip::getRecords(qint32 nLimit, PDSTRUCT *pPdStruct)
{
#ifdef QT_DEBUG
    qDebug("XZip::getRecords");
    QElapsedTimer timer;
    timer.start();
#endif

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    QList<RECORD> listResult;

    qint64 nECDOffset = findECDOffset(pPdStruct);

    qint64 nTotalSize = getSize();

    if (nECDOffset != -1) {
        qint32 nNumberOfRecords = read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));

        if (nLimit != -1) {
            nNumberOfRecords = qMin(nNumberOfRecords, nLimit);
        }

        listResult.reserve(nNumberOfRecords);

        qint32 nFreeIndex = XBinary::getFreeIndex(pPdStruct);
        XBinary::setPdStructInit(pPdStruct, nFreeIndex, nNumberOfRecords);

        qint64 nOffset = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

        for (qint32 i = 0; i < (nNumberOfRecords) && (!(pPdStruct->bIsStop)); i++) {
            RECORD record = {};

            CENTRALDIRECTORYFILEHEADER cdh = read_CENTRALDIRECTORYFILEHEADER(nOffset, pPdStruct);

            if (cdh.nSignature != SIGNATURE_CFD) {
                break;
            }

            record.spInfo.nCRC32 = cdh.nCRC32;
            record.nDataSize = cdh.nCompressedSize;
            record.spInfo.nUncompressedSize = cdh.nUncompressedSize;
            record.spInfo.compressMethod = COMPRESS_METHOD_UNKNOWN;
            quint16 nZipMethod = cdh.nMethod;

            record.spInfo.compressMethod = zipToCompressMethod(nZipMethod);

            record.spInfo.sRecordName = read_ansiString(nOffset + sizeof(CENTRALDIRECTORYFILEHEADER), cdh.nFileNameLength);

            LOCALFILEHEADER lfh = read_LOCALFILEHEADER(cdh.nOffsetToLocalFileHeader, pPdStruct);

            if (lfh.nSignature != SIGNATURE_LFD) {
                break;
            }

            record.nDataOffset = cdh.nOffsetToLocalFileHeader + sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;
            record.nHeaderOffset = cdh.nOffsetToLocalFileHeader;
            record.nHeaderSize = record.nDataOffset - record.nHeaderOffset;

            record.nOptHeaderOffset = nOffset;
            record.nOptHeaderSize = (sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength + cdh.nExtraFieldLength + cdh.nFileCommentLength);

            listResult.append(record);

            nOffset += (sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength + cdh.nExtraFieldLength + cdh.nFileCommentLength);

            XBinary::setPdStructCurrent(pPdStruct, nFreeIndex, i);
        }

        XBinary::setPdStructFinished(pPdStruct, nFreeIndex);
    } else {
        // if no ECD, only the first record
        qint32 nNumberOfRecords = nLimit;

        if (nNumberOfRecords == -1) {
            nNumberOfRecords = 0xFFFFFF;
        }

        qint64 nOffset = 0;

        for (qint32 i = 0; i < (nNumberOfRecords) && (!(pPdStruct->bIsStop)); i++) {
            if ((nOffset + (qint64)sizeof(LOCALFILEHEADER)) > nTotalSize) {
                break;
            }

            LOCALFILEHEADER lfh = read_LOCALFILEHEADER(nOffset, pPdStruct);

            if (lfh.nSignature != SIGNATURE_LFD) {
                break;
            }

            RECORD record = {};

            record.spInfo.nCRC32 = lfh.nCRC32;
            record.nDataSize = lfh.nCompressedSize;
            record.spInfo.nUncompressedSize = lfh.nUncompressedSize;
            record.spInfo.compressMethod = COMPRESS_METHOD_UNKNOWN;
            quint16 nZipMethod = lfh.nMethod;

            record.spInfo.compressMethod = zipToCompressMethod(nZipMethod);

            record.spInfo.sRecordName = read_ansiString(nOffset + sizeof(LOCALFILEHEADER), lfh.nFileNameLength);

            record.nDataOffset = nOffset + sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;
            record.nHeaderOffset = nOffset;
            record.nHeaderSize = record.nDataOffset - record.nHeaderOffset;

            listResult.append(record);

            nOffset += sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength + lfh.nCompressedSize;
        }
    }

#ifdef QT_DEBUG
    qDebug("Elapsed XZip::getRecords %lld", timer.elapsed());
#endif

    return listResult;
}

XBinary::FT XZip::getFileType()
{
    // For extra use getFileFormatInfo
    return FT_ZIP;
}

XBinary::FT XZip::_getFileType(QIODevice *pDevice, QList<RECORD> *pListRecords, bool bDeep, PDSTRUCT *pPdStruct)
{
    FT result = FT_ZIP;

    bool bIsValid = isValid(pDevice);

    if (bIsValid) {
        XZip xzip(pDevice);

        if (xzip.isValid()) {
            qint64 nSize = xzip.getFileFormatSize(pPdStruct);

            if (nSize) {
                // TODO
                if (XArchive::isArchiveRecordPresent("classes.dex", pListRecords, pPdStruct) ||
                    XArchive::isArchiveRecordPresent("AndroidManifest.xml", pListRecords, pPdStruct)) {
                    // result.sString = "APK";
                    // result.sExt = "apk";
                    result = XBinary::FT_APK;
                } else if (XArchive::isArchiveRecordPresent("Payload/", pListRecords, pPdStruct)) {
                    // result.sString = "IPA";
                    // result.sExt = "ipa";
                    result = FT_IPA;
                } else if (XArchive::isArchiveRecordPresent("META-INF/MANIFEST.MF", pListRecords, pPdStruct)) {
                    // result.sString = "JAR";
                    // result.sExt = "jar";
                    result = FT_JAR;
                } else {
                    // result.sString = "ZIP";
                    // result.sExt = "zip";
                    result = FT_ZIP;
                }

                if (bDeep) {
                    if ((result != XBinary::FT_JAR) && (result != XBinary::FT_APK) && (result != XBinary::FT_IPA)) {
                        qint32 nNumberOfRecords = pListRecords->count();

                        bool bAPKS = false;

                        if (nNumberOfRecords) {
                            bAPKS = true;
                        }

                        for (qint32 i = 0; (i < nNumberOfRecords) && (!(pPdStruct->bIsStop)); i++) {
                            if (pListRecords->at(i).spInfo.compressMethod == XArchive::COMPRESS_METHOD_STORE) {
                                XArchive::RECORD record = pListRecords->at(i);

                                SubDevice subDevice(pDevice, record.nDataOffset, record.spInfo.nUncompressedSize);

                                if (subDevice.open(QIODevice::ReadOnly)) {
                                    if (XBinary::getFileTypes(&subDevice, true).contains(FT_ZIP)) {
                                        bool bAPK = false;

                                        if (XArchive::isArchiveRecordPresent("classes.dex", pListRecords, pPdStruct) ||
                                            XArchive::isArchiveRecordPresent("AndroidManifest.xml", pListRecords, pPdStruct)) {
                                            bAPK = true;
                                        }

                                        if (!bAPK) {
                                            bAPKS = false;
                                        }
                                    }

                                    subDevice.close();
                                }
                            } else {
                                bAPKS = false;
                            }

                            if (!bAPKS) {
                                break;
                            }
                        }

                        if (bAPKS) {
                            result = FT_APKS;
                            // result.sString = "APKS";
                            // result.sExt = "apks";
                        }
                    }
                }
            }
        }
    }

    return result;
}

XBinary::FILEFORMATINFO XZip::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    XBinary::FILEFORMATINFO result = {};

    if (isValid(pPdStruct)) {
        result.nSize = getFileFormatSize(pPdStruct);
        result.sExt = "zip";
        result.fileType = FT_ZIP;
        result.sVersion = getVersion();
        result.sOptions = getOptions();

        if (result.nSize > 0) {
            result.bIsValid = true;
        }
    }

    return result;
}

bool XZip::addLocalFileRecord(QIODevice *pSource, QIODevice *pDest, ZIPFILE_RECORD *pZipFileRecord, PDSTRUCT *pPdStruct)
{
    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    if (pZipFileRecord->nMinVersion == 0) {
        pZipFileRecord->nMinVersion = 0x14;
    }

    if (pZipFileRecord->nVersion == 0) {
        pZipFileRecord->nVersion = 0x3F;
    }

    if (pZipFileRecord->nUncompressedSize == 0) {
        pZipFileRecord->nUncompressedSize = pSource->size();
    }

    if (pZipFileRecord->nCRC32 == 0) {
        pZipFileRecord->nCRC32 = XBinary::_getCRC32(pSource);
    }

    if (!pZipFileRecord->dtTime.isValid()) {
        pZipFileRecord->dtTime = QDateTime::currentDateTime();
    }

    pZipFileRecord->nHeaderOffset = pDest->pos();

    XZip::LOCALFILEHEADER localFileHeader = {};
    localFileHeader.nSignature = XZip::SIGNATURE_LFD;
    localFileHeader.nMinVersion = pZipFileRecord->nMinVersion;
    localFileHeader.nFlags = pZipFileRecord->nFlags;
    localFileHeader.nMethod = pZipFileRecord->method;
    localFileHeader.nLastModTime = 0;  // TODO
    localFileHeader.nLastModDate = 0;  // TODO
    localFileHeader.nCRC32 = pZipFileRecord->nCRC32;
    localFileHeader.nCompressedSize = 0;
    localFileHeader.nUncompressedSize = pZipFileRecord->nUncompressedSize;
    localFileHeader.nFileNameLength = pZipFileRecord->sFileName.size();
    localFileHeader.nExtraFieldLength = 0;

    pDest->write((char *)&localFileHeader, sizeof(localFileHeader));
    pDest->write(pZipFileRecord->sFileName.toUtf8().data(), pZipFileRecord->sFileName.toUtf8().size());

    pZipFileRecord->nDataOffset = pDest->pos();

    XArchive::_compress(XArchive::COMPRESS_METHOD_DEFLATE, pSource, pDest, pPdStruct);

    qint64 nEndPosition = pDest->pos();

    pZipFileRecord->nCompressedSize = (nEndPosition) - (pZipFileRecord->nDataOffset);

    XBinary binary(pDest);

    binary.write_uint32(pZipFileRecord->nHeaderOffset + offsetof(XZip::LOCALFILEHEADER, nCompressedSize), pZipFileRecord->nCompressedSize);

    pDest->seek(nEndPosition);

    return true;
}

bool XZip::addCentralDirectory(QIODevice *pDest, QList<XZip::ZIPFILE_RECORD> *pListZipFileRecords, const QString &sComment)
{
    qint64 nStartPosition = pDest->pos();

    qint32 nNumberOfRecords = pListZipFileRecords->count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        XZip::CENTRALDIRECTORYFILEHEADER cdFileHeader = {};

        cdFileHeader.nSignature = SIGNATURE_CFD;
        cdFileHeader.nVersion = pListZipFileRecords->at(i).nVersion;
        cdFileHeader.nMinVersion = pListZipFileRecords->at(i).nMinVersion;
        cdFileHeader.nFlags = pListZipFileRecords->at(i).nFlags;
        cdFileHeader.nMethod = pListZipFileRecords->at(i).method;
        cdFileHeader.nLastModTime = 0;  // TODO
        cdFileHeader.nLastModDate = 0;  // TODO
        cdFileHeader.nCRC32 = pListZipFileRecords->at(i).nCRC32;
        cdFileHeader.nCompressedSize = pListZipFileRecords->at(i).nCompressedSize;
        cdFileHeader.nUncompressedSize = pListZipFileRecords->at(i).nUncompressedSize;
        cdFileHeader.nFileNameLength = pListZipFileRecords->at(i).sFileName.size();
        cdFileHeader.nExtraFieldLength = 0;
        cdFileHeader.nFileCommentLength = 0;
        cdFileHeader.nStartDisk = 0;
        cdFileHeader.nInternalFileAttributes = 0;
        cdFileHeader.nExternalFileAttributes = 0;
        cdFileHeader.nOffsetToLocalFileHeader = pListZipFileRecords->at(i).nHeaderOffset;

        pDest->write((char *)&cdFileHeader, sizeof(cdFileHeader));
        pDest->write(pListZipFileRecords->at(i).sFileName.toUtf8().data(), pListZipFileRecords->at(i).sFileName.toUtf8().size());
    }

    qint64 nCentralDirectorySize = pDest->pos() - nStartPosition;

    ENDOFCENTRALDIRECTORYRECORD endofCD = {};

    endofCD.nSignature = SIGNATURE_ECD;
    endofCD.nDiskNumber = 0;
    endofCD.nStartDisk = 0;
    endofCD.nDiskNumberOfRecords = nNumberOfRecords;
    endofCD.nTotalNumberOfRecords = nNumberOfRecords;
    endofCD.nSizeOfCentralDirectory = nCentralDirectorySize;
    endofCD.nOffsetToCentralDirectory = nStartPosition;
    endofCD.nCommentLength = sComment.size();

    pDest->write((char *)&endofCD, sizeof(endofCD));
    pDest->write(sComment.toUtf8().data(), sComment.toUtf8().size());

    return true;
}

QString XZip::getFileFormatExt()
{
    return "zip";
}

qint64 XZip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    qint64 nResult = 0;
    // TODO the last ECD
    qint64 nECDOffset = 0;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        nECDOffset = find_uint32(nECDOffset, -1, SIGNATURE_ECD, false, pPdStruct);

        if (nECDOffset != -1) {
            qint64 nOffset = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

            if (nOffset < getSize()) {
                quint32 nSignature = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nSignature));
                qint64 nStartOffset = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nOffsetToLocalFileHeader));

                if (nSignature == SIGNATURE_CFD) {
                    nResult = nECDOffset + sizeof(ENDOFCENTRALDIRECTORYRECORD) + read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nCommentLength));
                    if (nStartOffset == 0) {
                        break;
                    }
                }
            }
        } else {
            break;
        }

        nECDOffset += 4;
    }

    if (nECDOffset != -1) {
        nResult = nECDOffset + sizeof(ENDOFCENTRALDIRECTORYRECORD) + read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nCommentLength));
    }

    return nResult;
}

XZip::CENTRALDIRECTORYFILEHEADER XZip::read_CENTRALDIRECTORYFILEHEADER(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    CENTRALDIRECTORYFILEHEADER result = {};

    read_array(nOffset, (char *)&result, sizeof(CENTRALDIRECTORYFILEHEADER), pPdStruct);

    return result;
}

XZip::LOCALFILEHEADER XZip::read_LOCALFILEHEADER(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    LOCALFILEHEADER result = {};

    read_array(nOffset, (char *)&result, sizeof(LOCALFILEHEADER), pPdStruct);

    return result;
}

qint64 XZip::findECDOffset(PDSTRUCT *pPdStruct)
{
    qint64 nResult = -1;
    qint64 nSize = getSize();

    if (nSize >= 22)  // 22 is minimum size [0x50,0x4B,0x05,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]
    {
        qint64 nOffset = qMax((qint64)0, nSize - 0x1000);  // TODO const

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint64 nCurrent = find_uint32(nOffset, -1, SIGNATURE_ECD, false, pPdStruct);

            if (nCurrent == -1) {
                break;
            }

            nResult = nCurrent;
            nOffset = nCurrent + 4;  // Get the last
        }
    }

    return nResult;
}

bool XZip::isAPK(qint64 nECDOffset, PDSTRUCT *pPdStruct)
{
    return _isRecordNamePresent(nECDOffset, "classes.dex", "AndroidManifest.xml", pPdStruct);;
}

bool XZip::isIPA(qint64 nECDOffset, PDSTRUCT *pPdStruct)
{
    return false; // TODO
}

bool XZip::isJAR(qint64 nECDOffset, PDSTRUCT *pPdStruct)
{
    return _isRecordNamePresent(nECDOffset, "META-INF/MANIFEST.MF", "", pPdStruct);
}

bool XZip::_isRecordNamePresent(qint64 nECDOffset, QString sRecordName1, QString sRecordName2, PDSTRUCT *pPdStruct)
{
    qint32 nLimit = 10000;  // TODO
    qint64 nTotalSize = getSize();

    qint32 nRecordNameSize1 = sRecordName1.size();
    qint32 nRecordNameSize2 = sRecordName2.size();

    if (nECDOffset != -1) {
        qint32 nNumberOfRecords = read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));

        if (nLimit != -1) {
            nNumberOfRecords = qMin(nNumberOfRecords, nLimit);
        }

        qint32 nFreeIndex = XBinary::getFreeIndex(pPdStruct);
        XBinary::setPdStructInit(pPdStruct, nFreeIndex, nNumberOfRecords);

        qint64 nOffset = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

        for (qint32 i = 0; i < (nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            CENTRALDIRECTORYFILEHEADER cdh = read_CENTRALDIRECTORYFILEHEADER(nOffset, pPdStruct);

            if (cdh.nSignature != SIGNATURE_CFD) {
                break;
            }

            if ((cdh.nFileNameLength == nRecordNameSize1) && (nRecordNameSize1 == nRecordNameSize2)) {
                QString sRecordName = read_ansiString(nOffset + sizeof(CENTRALDIRECTORYFILEHEADER), cdh.nFileNameLength);

                if ((sRecordName == sRecordName1) || (sRecordName == sRecordName2)) {
                    return true;
                }
            }

            if (cdh.nFileNameLength == nRecordNameSize1) {
                if (read_ansiString(nOffset + sizeof(CENTRALDIRECTORYFILEHEADER), cdh.nFileNameLength) == sRecordName1) {
                    return true;
                }
            }
            if ((nRecordNameSize2) && (cdh.nFileNameLength == nRecordNameSize2)) {
                if (read_ansiString(nOffset + sizeof(CENTRALDIRECTORYFILEHEADER), cdh.nFileNameLength) == sRecordName2) {
                    return true;
                }
            }

            nOffset += (sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength + cdh.nExtraFieldLength + cdh.nFileCommentLength);

            XBinary::setPdStructCurrent(pPdStruct, nFreeIndex, i);
        }

        XBinary::setPdStructFinished(pPdStruct, nFreeIndex);
    } else {
        // if no ECD, only the first record
        qint32 nNumberOfRecords = nLimit;

        if (nNumberOfRecords == -1) {
            nNumberOfRecords = 0xFFFFFF;
        }

        qint64 nOffset = 0;

        for (qint32 i = 0; i < (nNumberOfRecords) && (!(pPdStruct->bIsStop)); i++) {
            if ((nOffset + (qint64)sizeof(LOCALFILEHEADER)) > nTotalSize) {
                break;
            }

            LOCALFILEHEADER lfh = read_LOCALFILEHEADER(nOffset, pPdStruct);

            if (lfh.nSignature != SIGNATURE_LFD) {
                break;
            }

            if ((lfh.nFileNameLength == nRecordNameSize1) && (nRecordNameSize1 == nRecordNameSize2)) {
                QString sRecordName = read_ansiString(nOffset + sizeof(LOCALFILEHEADER), lfh.nFileNameLength);

                if ((sRecordName == sRecordName1) || (sRecordName == sRecordName2)) {
                    return true;
                }
            }

            if (lfh.nFileNameLength == nRecordNameSize1) {
                if (read_ansiString(nOffset + sizeof(LOCALFILEHEADER), lfh.nFileNameLength) == sRecordName1) {
                    return true;
                }
            }
            if ((nRecordNameSize2) && (lfh.nFileNameLength == nRecordNameSize2)) {
                if (read_ansiString(nOffset + sizeof(LOCALFILEHEADER), lfh.nFileNameLength) == sRecordName2) {
                    return true;
                }
            }

            nOffset += sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength + lfh.nCompressedSize;
        }
    }

    return false;
}

XArchive::COMPRESS_METHOD XZip::zipToCompressMethod(quint16 nZipMethod)
{
    COMPRESS_METHOD result = COMPRESS_METHOD_UNKNOWN;

    switch (nZipMethod) {
        case CMETHOD_STORE: result = COMPRESS_METHOD_STORE; break;
        case CMETHOD_DEFLATE: result = COMPRESS_METHOD_DEFLATE; break;
        case CMETHOD_DEFLATE64: result = COMPRESS_METHOD_DEFLATE64; break;  // TODO
        case CMETHOD_BZIP2: result = COMPRESS_METHOD_BZIP2; break;
        case CMETHOD_LZMA: result = COMPRESS_METHOD_LZMA_ZIP; break;
        case CMETHOD_PPMD: result = COMPRESS_METHOD_PPMD; break;  // TODO
    }
    // TODO more methods

    return result;
}
