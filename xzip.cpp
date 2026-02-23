/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
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
#include "Algos/xdeflatedecoder.h"
#include "Algos/ximplodedecoder.h"
#include "Algos/xlzmadecoder.h"
#include "Algos/xlzwdecoder.h"
#include "Algos/xbzip2decoder.h"
#include "Algos/xshrinkdecoder.h"
#include "Algos/xreducedecoder.h"
#include "Algos/xstoredecoder.h"
#include "Algos/xzipcryptodecoder.h"
#include "Algos/xzipaesdecoder.h"
#include "Algos/xppmddecoder.h"

XBinary::XCONVERT _TABLE_XZip_STRUCTID[] = {
    {XZip::STRUCTID_UNKNOWN, "Unknown", QObject::tr("Unknown")},
    {XZip::STRUCTID_LOCALFILEHEADER, "LocalFileHeader", QString("Local File Header")},
    {XZip::STRUCTID_CENTRALDIRECTORYFILEHEADER, "CentralDirectoryFileHeader", QString("Central Directory File Header")},
    {XZip::STRUCTID_ENDOFCENTRALDIRECTORYRECORD, "EndOfCentralDirectoryRecord", QString("End of Central Directory Record")},
};

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
            nVersion = read_uint8(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nVersion));

            if (nVersion == 0) {
                nVersion = read_uint8(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nMinVersion));
            }
        }
    }

    if (nVersion == 0) {
        // The first record
        nVersion = read_uint8(0 + offsetof(LOCALFILEHEADER, nMinVersion));
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
        quint16 nFlags = read_uint16(offsetof(LOCALFILEHEADER, nFlags));

        bResult = (nFlags & 0x1);
    }

    return bResult;
}

QString XZip::getCompressMethodString()
{
    QString sResult;

    QSet<HANDLE_METHOD> stMethods;

    qint64 nECDOffset = findECDOffset(nullptr);

    if (nECDOffset != -1) {
        qint64 nOffset = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

        for (int i = 0; i < 20; i++) {
            quint32 nSignature = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nSignature));

            if (nSignature == SIGNATURE_CFD) {
                quint16 nMethod = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nMethod));
                quint16 nFlags = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nFlags));
                quint32 nUncompressedSize = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nUncompressedSize));

                if (nUncompressedSize > 0) {
                    stMethods.insert(zipToCompressMethod(nMethod, nFlags));
                }

                nOffset += (sizeof(CENTRALDIRECTORYFILEHEADER) + read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nFileNameLength)) +
                            read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nExtraFieldLength)) +
                            read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nFileCommentLength)));
            } else {
                break;
            }
        }
    } else {
        qint64 nOffset = 0;

        for (int i = 0; i < 20; i++) {
            quint32 nSignature = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nSignature));

            if (nSignature == SIGNATURE_CFD) {
                quint16 nMethod = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nMethod));
                quint16 nFlags = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nFlags));
                quint32 nUncompressedSize = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nUncompressedSize));

                if (nUncompressedSize > 0) {
                    stMethods.insert(zipToCompressMethod(nMethod, nFlags));
                }

                nOffset += (sizeof(LOCALFILEHEADER) + read_uint16(nOffset + offsetof(LOCALFILEHEADER, nFileNameLength)) +
                            read_uint16(nOffset + offsetof(LOCALFILEHEADER, nExtraFieldLength)));
            } else {
                break;
            }
        }
    }

    // Iterate QSet

    QSetIterator<HANDLE_METHOD> i(stMethods);
    while (i.hasNext()) {
        HANDLE_METHOD cm = (HANDLE_METHOD)i.next();
        QString sMethod = handleMethodToString(cm);

        sResult = XBinary::appendText(sResult, sMethod, ", ");
    }

    return sResult;
}

XBinary::FT XZip::getFileType()
{
    // For extra use getFileFormatInfo
    return FT_ZIP;
}

XBinary::FT XZip::_getFileType(QIODevice *pDevice, QList<RECORD> *pListRecords, bool bDeep, PDSTRUCT *pPdStruct)
{
    FT result = FT_ZIP;

    if (!isValid(pDevice)) {
        return result;
    }

    // We assume pListRecords is already populated by caller for efficiency.
    // If not, we still proceed with defaults.
    const bool hasRecords = (pListRecords && !pListRecords->isEmpty());

    // Fast single-pass classification using record names
    bool seenClassesDex = false;
    bool seenAndroidManifest = false;
    bool seenJarManifest = false;
    bool seenPayloadDir = false;

    if (hasRecords) {
        for (int idx = 0, n = pListRecords->count(); idx < n; ++idx) {
            const RECORD &rec = pListRecords->at(idx);
            const QString &name = rec.spInfo.sRecordName;
            if (!seenClassesDex && name == QLatin1String("classes.dex")) seenClassesDex = true;
            if (!seenAndroidManifest && name == QLatin1String("AndroidManifest.xml")) seenAndroidManifest = true;
            if (!seenJarManifest && name == QLatin1String("META-INF/MANIFEST.MF")) seenJarManifest = true;
            if (!seenPayloadDir && name.startsWith(QLatin1String("Payload/"))) seenPayloadDir = true;

            // Early exit if APK (most common) is detected
            if ((seenClassesDex || seenAndroidManifest)) {
                result = FT_APK;
                break;
            }
        }
    }

    if (result != FT_APK) {
        if (seenPayloadDir) {
            result = FT_IPA;
        } else if (seenJarManifest) {
            result = FT_JAR;
        } else {
            result = FT_ZIP;
        }
    }

    if (bDeep && hasRecords) {
        // Detect APKS: a ZIP where all entries are stored and (the ones we consider) are inner ZIPs.
        if ((result != FT_JAR) && (result != FT_APK) && (result != FT_IPA)) {
            bool bAPKS = !pListRecords->isEmpty();

            for (int idx = 0, n = pListRecords->count(); idx < n; ++idx) {
                if (!isPdStructNotCanceled(pPdStruct)) break;
                const RECORD &rec = pListRecords->at(idx);

                if (rec.spInfo.compressMethod == XArchive::HANDLE_METHOD_STORE) {
                    // Skip directories
                    if (rec.spInfo.nUncompressedSize < 4) {
                        bAPKS = false;
                        break;
                    }

                    SubDevice subDevice(pDevice, rec.nDataOffset, qMin<qint64>(rec.spInfo.nUncompressedSize, 8));
                    if (!subDevice.open(QIODevice::ReadOnly)) {
                        bAPKS = false;
                        break;
                    }

                    char sig[4] = {0};
                    qint64 r = subDevice.read(sig, 4);
                    subDevice.close();
                    if (r != 4) {
                        bAPKS = false;
                        break;
                    }

                    // Check for local file header 'PK\x03\x04'
                    const quint32 ZIP_LFH = 0x04034B50u;
                    quint32 v = ((quint8)sig[0]) | (((quint8)sig[1]) << 8) | (((quint8)sig[2]) << 16) | (((quint8)sig[3]) << 24);
                    if (v != ZIP_LFH) {
                        bAPKS = false;
                        break;
                    }
                } else {
                    bAPKS = false;
                    break;
                }
            }

            if (bAPKS) {
                result = FT_APKS;
            }
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

    XArchive::_compress(XArchive::HANDLE_METHOD_DEFLATE, pSource, pDest, pPdStruct);

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
        cdFileHeader.nOS = pListZipFileRecords->at(i).nOS;
        cdFileHeader.nMinVersion = pListZipFileRecords->at(i).nMinVersion;
        cdFileHeader.nMinOS = pListZipFileRecords->at(i).nMinOS;
        cdFileHeader.nFlags = pListZipFileRecords->at(i).nFlags;
        cdFileHeader.nMethod = (quint16)pListZipFileRecords->at(i).method;
        cdFileHeader.nLastModTime = 0;  // TODO
        cdFileHeader.nLastModDate = 0;  // TODO
        cdFileHeader.nCRC32 = pListZipFileRecords->at(i).nCRC32;
        cdFileHeader.nCompressedSize = (quint32)pListZipFileRecords->at(i).nCompressedSize;
        cdFileHeader.nUncompressedSize = (quint32)pListZipFileRecords->at(i).nUncompressedSize;
        cdFileHeader.nFileNameLength = (quint16)pListZipFileRecords->at(i).sFileName.toUtf8().size();
        cdFileHeader.nExtraFieldLength = 0;
        cdFileHeader.nFileCommentLength = 0;
        cdFileHeader.nStartDisk = 0;
        cdFileHeader.nInternalFileAttributes = 0;
        cdFileHeader.nExternalFileAttributes = pListZipFileRecords->at(i).nExternalFileAttributes;
        cdFileHeader.nOffsetToLocalFileHeader = (quint32)pListZipFileRecords->at(i).nHeaderOffset;

        pDest->write((char *)&cdFileHeader, sizeof(cdFileHeader));

        QByteArray baFileName = pListZipFileRecords->at(i).sFileName.toUtf8();
        pDest->write(baFileName.data(), baFileName.size());
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

QString XZip::getFileFormatExtsString()
{
    return "zip,apk,apks,jar,ipa";  // TODO
}

qint64 XZip::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    // Fast path: use validated last End of Central Directory (ECD) offset.
    const qint64 nTotalSize = getSize();
    qint64 nResult = 0;

    const qint64 nECDOffset = findECDOffset(pPdStruct);
    if (nECDOffset != -1) {
        qint64 nEnd = nECDOffset + sizeof(ENDOFCENTRALDIRECTORYRECORD) + read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nCommentLength));
        // Clamp to file size for robustness
        if (nEnd > nTotalSize) nEnd = nTotalSize;
        nResult = nEnd;
    } else {
        // Fallback: compute real size from Local File Headers when ECD is missing
        qint64 nRealSize = 0;
        _getNumberOfLocalFileHeaders(0, nTotalSize, &nRealSize, pPdStruct);
        nResult = nRealSize;
    }

    return nResult;
}

QString XZip::getMIMEString()
{
    return "application/zip";
}

XZip::CENTRALDIRECTORYFILEHEADER XZip::read_CENTRALDIRECTORYFILEHEADER(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    CENTRALDIRECTORYFILEHEADER result = {};

    result.nSignature = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nSignature));
    result.nVersion = read_uint8(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nVersion));
    result.nOS = read_uint8(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nOS));
    result.nMinVersion = read_uint8(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nMinVersion));
    result.nMinOS = read_uint8(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nMinOS));
    result.nFlags = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nFlags));
    result.nMethod = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nMethod));
    result.nLastModTime = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nLastModTime));
    result.nLastModDate = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nLastModDate));
    result.nCRC32 = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nCRC32));
    result.nCompressedSize = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nCompressedSize));
    result.nUncompressedSize = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nUncompressedSize));
    result.nFileNameLength = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nFileNameLength));
    result.nExtraFieldLength = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nExtraFieldLength));
    result.nFileCommentLength = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nFileCommentLength));
    result.nStartDisk = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nStartDisk));
    result.nInternalFileAttributes = read_uint16(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nInternalFileAttributes));
    result.nExternalFileAttributes = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nExternalFileAttributes));
    result.nOffsetToLocalFileHeader = read_uint32(nOffset + offsetof(CENTRALDIRECTORYFILEHEADER, nOffsetToLocalFileHeader));

    return result;
}

XZip::LOCALFILEHEADER XZip::read_LOCALFILEHEADER(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    LOCALFILEHEADER result = {};

    result.nSignature = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nSignature));
    result.nMinVersion = read_uint8(nOffset + offsetof(LOCALFILEHEADER, nMinVersion));
    result.nMinOS = read_uint8(nOffset + offsetof(LOCALFILEHEADER, nMinOS));
    result.nFlags = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nFlags));
    result.nMethod = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nMethod));
    result.nLastModTime = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nLastModTime));
    result.nLastModDate = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nLastModDate));
    result.nCRC32 = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nCRC32));
    result.nCompressedSize = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nCompressedSize));
    result.nUncompressedSize = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nUncompressedSize));
    result.nFileNameLength = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nFileNameLength));
    result.nExtraFieldLength = read_uint16(nOffset + offsetof(LOCALFILEHEADER, nExtraFieldLength));

    return result;
}

XZip::AES_EXTRA_FIELD XZip::read_AES_EXTRA_FIELD(qint64 nOffset, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    AES_EXTRA_FIELD result = {};

    result.nHeaderID = read_uint16(nOffset + offsetof(AES_EXTRA_FIELD, nHeaderID));
    result.nDataSize = read_uint16(nOffset + offsetof(AES_EXTRA_FIELD, nDataSize));
    result.nAESVersion = read_uint16(nOffset + offsetof(AES_EXTRA_FIELD, nAESVersion));
    result.nVendorID = read_uint16(nOffset + offsetof(AES_EXTRA_FIELD, nVendorID));
    result.nEncryptionMode = read_uint8(nOffset + offsetof(AES_EXTRA_FIELD, nEncryptionMode));
    result.nCompressionMethod = read_uint16(nOffset + offsetof(AES_EXTRA_FIELD, nCompressionMethod));

    return result;
}

qint64 XZip::findECDOffset(PDSTRUCT *pPdStruct)
{
    qint64 nResult = -1;
    qint64 nSize = getSize();

    // qDebug() << "findECDOffset: File size =" << nSize << "Device:" << (getDevice() ? "SET" : "NULL");

    if (nSize >= 22)  // 22 is minimum size [0x50,0x4B,0x05,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00]
    {
        qint64 nOffset = qMax((qint64)0, nSize - 0x1000);  // TODO const

        // qDebug() << "findECDOffset: Starting search from offset" << nOffset;

        // Debug: Read and print last 30 bytes of file
        // if (nSize >= 30) {
        //     QByteArray lastBytes = read_array(nSize - 30, 30);
        //     qDebug() << "findECDOffset: Last 30 bytes of file:";
        //     QString sHex;
        //     for (int i = 0; i < lastBytes.size(); i++) {
        //         sHex += QString("%1 ").arg((quint8)lastBytes[i], 2, 16, QChar('0')).toUpper();
        //     }
        //     qDebug() << sHex;
        // }

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            qint64 nCurrent = find_uint32(nOffset, -1, SIGNATURE_ECD, false, pPdStruct);

            // qDebug() << "findECDOffset: find_uint32 returned" << nCurrent;

            if (nCurrent == -1) {
                break;
            }

            qint64 nOffsetToCentralDirectory = read_uint32(nCurrent + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

            if (nOffsetToCentralDirectory >= nCurrent) {
                nOffset = nCurrent + 4;
                continue;
            }

            quint32 nCFDSignature = read_uint32(nOffsetToCentralDirectory + offsetof(CENTRALDIRECTORYFILEHEADER, nSignature));

            if (nCFDSignature != SIGNATURE_CFD) {
                nOffset = nCurrent + 4;
                continue;
            }

            nResult = nCurrent;
            nOffset = nCurrent + 4;  // Get the last
        }
    }

    return nResult;
}

bool XZip::isAPK(qint64 nECDOffset, PDSTRUCT *pPdStruct)
{
    return _isRecordNamePresent(nECDOffset, "classes.dex", "AndroidManifest.xml", pPdStruct);
}

bool XZip::isIPA(qint64 nECDOffset, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(nECDOffset);
    Q_UNUSED(pPdStruct);

    return false;  // TODO
}

bool XZip::isJAR(qint64 nECDOffset, PDSTRUCT *pPdStruct)
{
    return _isRecordNamePresent(nECDOffset, "META-INF/MANIFEST.MF", "", pPdStruct);
}

QString XZip::structIDToString(quint32 nID)
{
    return XBinary::XCONVERT_idToTransString(nID, _TABLE_XZip_STRUCTID, sizeof(_TABLE_XZip_STRUCTID) / sizeof(XBinary::XCONVERT));
}

qint32 XZip::readTableRow(qint32 nRow, LT locType, XADDR nLocation, const DATA_RECORDS_OPTIONS &dataRecordsOptions, QList<DATA_RECORD_ROW> *pListDataRecords,
                          void *pUserData, PDSTRUCT *pPdStruct)
{
    qint32 nResult = 0;

    if (dataRecordsOptions.dataHeaderFirst.dsID.nID == STRUCTID_LOCALFILEHEADER) {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);

        qint64 nStartOffset = locationToOffset(dataRecordsOptions.pMemoryMap, locType, nLocation);
#ifdef QT_DEBUG
        qDebug("XZip::readTableRow nStartOffset=%llX", nStartOffset);
#endif
        quint32 nLocalSignature = read_uint32(nStartOffset + offsetof(LOCALFILEHEADER, nSignature));
        quint32 nLocalFileNameSize = read_uint16(nStartOffset + offsetof(LOCALFILEHEADER, nFileNameLength));
        quint32 nLocalExtraFieldSize = read_uint16(nStartOffset + offsetof(LOCALFILEHEADER, nExtraFieldLength));
        quint32 nCompressedSize = read_uint32(nStartOffset + offsetof(LOCALFILEHEADER, nCompressedSize));

        if (nLocalSignature == SIGNATURE_LFD) {
            nResult = sizeof(LOCALFILEHEADER) + nLocalFileNameSize + nLocalExtraFieldSize + nCompressedSize;
        }
    } else {
        nResult = XBinary::readTableRow(nRow, locType, nLocation, dataRecordsOptions, pListDataRecords, pUserData, pPdStruct);
    }

    return nResult;
}

QMap<quint64, QString> XZip::getHeaderSignatures()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x06054B50, "SIGNATURE_ECD");
    mapResult.insert(0x02014B50, "SIGNATURE_CFD");
    mapResult.insert(0x04034B50, "SIGNATURE_LFD");

    return mapResult;
}

QMap<quint64, QString> XZip::getHeaderSignaturesS()
{
    QMap<quint64, QString> mapResult;

    mapResult.insert(0x06054B50, "ECD");
    mapResult.insert(0x02014B50, "CFD");
    mapResult.insert(0x04034B50, "LFD");

    return mapResult;
}

XBinary::_MEMORY_MAP XZip::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    XBinary::_MEMORY_MAP result = {};

    if (mapMode == MAPMODE_UNKNOWN) {
        mapMode = MAPMODE_DATA;  // Default mode
    }

    if (mapMode == MAPMODE_REGIONS) {
        result = _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    } else if (mapMode == MAPMODE_STREAMS) {
        result = _getMemoryMap(FILEPART_STREAM, pPdStruct);
    } else if (mapMode == MAPMODE_DATA) {
        result = _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
    }

    return result;
}

QList<XBinary::MAPMODE> XZip::getMapModesList()
{
    QList<MAPMODE> listResult;

    listResult.append(MAPMODE_DATA);
    listResult.append(MAPMODE_REGIONS);
    listResult.append(MAPMODE_STREAMS);

    return listResult;
}

QList<XBinary::DATA_HEADER> XZip::getDataHeaders(const DATA_HEADERS_OPTIONS &dataHeadersOptions, PDSTRUCT *pPdStruct)
{
    QList<XBinary::DATA_HEADER> listResult;

    if (dataHeadersOptions.nID == STRUCTID_UNKNOWN) {
        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
        _dataHeadersOptions.bChildren = true;
        _dataHeadersOptions.dsID_parent = _addDefaultHeaders(&listResult, pPdStruct);

        qint64 nECDOffset = findECDOffset(pPdStruct);

        if (nECDOffset != -1) {
            _dataHeadersOptions.dhMode = XBinary::DHMODE_HEADER;
            _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
            _dataHeadersOptions.nID = STRUCTID_ENDOFCENTRALDIRECTORYRECORD;
            _dataHeadersOptions.locType = LT_OFFSET;
            _dataHeadersOptions.nLocation = nECDOffset;

            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        } else {
            qint64 nRealSize = 0;
            qint32 nCount = _getNumberOfLocalFileHeaders(0, getSize(), &nRealSize, pPdStruct);

            _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
            _dataHeadersOptions.fileType = dataHeadersOptions.pMemoryMap->fileType;
            _dataHeadersOptions.nID = STRUCTID_LOCALFILEHEADER;
            _dataHeadersOptions.locType = LT_OFFSET;
            _dataHeadersOptions.nLocation = 0;
            _dataHeadersOptions.nCount = nCount;
            _dataHeadersOptions.nSize = nRealSize;

            listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
        }
    } else {
        qint64 nStartOffset = locationToOffset(dataHeadersOptions.pMemoryMap, dataHeadersOptions.locType, dataHeadersOptions.nLocation);

        if (nStartOffset != -1) {
            if (dataHeadersOptions.nID == STRUCTID_ENDOFCENTRALDIRECTORYRECORD) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(dataHeadersOptions.nID));

                qint16 nCommentLength = read_uint16(nStartOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nCommentLength));

                dataHeader.nSize = sizeof(ENDOFCENTRALDIRECTORYRECORD) + nCommentLength;

                dataHeader.listRecords.append(getDataRecordDV(offsetof(ENDOFCENTRALDIRECTORYRECORD, nSignature), sizeof(quint32), "Signature", XBinary::VT_UINT32, 0,
                                                              XBinary::ENDIAN_LITTLE, XZip::getHeaderSignaturesS(), VL_TYPE_LIST));
                dataHeader.listRecords.append(getDataRecord(offsetof(ENDOFCENTRALDIRECTORYRECORD, nDiskNumber), sizeof(quint16), "DiskNumber", XBinary::VT_UINT16,
                                                            DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(ENDOFCENTRALDIRECTORYRECORD, nStartDisk), sizeof(quint16), "StartDisk", XBinary::VT_UINT16,
                                                            DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(ENDOFCENTRALDIRECTORYRECORD, nDiskNumberOfRecords), sizeof(quint16), "DiskNumberOfRecords",
                                                            XBinary::VT_UINT16, DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords), sizeof(quint16), "TotalNumberOfRecords",
                                                            XBinary::VT_UINT16, DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(ENDOFCENTRALDIRECTORYRECORD, nSizeOfCentralDirectory), sizeof(quint32), "SizeOfCentralDirectory",
                                                            XBinary::VT_UINT32, DRF_SIZE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory), sizeof(quint32), "OffsetToCentralDirectory",
                                                            XBinary::VT_UINT32, DRF_OFFSET, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(ENDOFCENTRALDIRECTORYRECORD, nCommentLength), sizeof(quint16), "CommentLength", XBinary::VT_UINT16,
                                                            DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(sizeof(ENDOFCENTRALDIRECTORYRECORD), nCommentLength, "Comment", XBinary::VT_CHAR_ARRAY, DRF_VOLATILE, XBinary::ENDIAN_LITTLE));

                listResult.append(dataHeader);

                if (dataHeadersOptions.bChildren) {
                    quint16 nTotalNumberOfRecords = read_uint16(nStartOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));
                    quint32 nSizeOfCentralDirectory = read_uint32(nStartOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nSizeOfCentralDirectory));
                    quint32 nOffsetToCentralDirectory = read_uint32(nStartOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

                    {
                        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                        _dataHeadersOptions.nLocation = nOffsetToCentralDirectory;
                        _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                        _dataHeadersOptions.nCount = nTotalNumberOfRecords;
                        _dataHeadersOptions.nSize = nSizeOfCentralDirectory;
                        _dataHeadersOptions.nID = STRUCTID_CENTRALDIRECTORYFILEHEADER;
                        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                    }
                    {
                        DATA_HEADERS_OPTIONS _dataHeadersOptions = dataHeadersOptions;
                        _dataHeadersOptions.nLocation = 0;
                        _dataHeadersOptions.dsID_parent = dataHeader.dsID;
                        _dataHeadersOptions.dhMode = XBinary::DHMODE_TABLE;
                        _dataHeadersOptions.nCount = nTotalNumberOfRecords;
                        _dataHeadersOptions.nSize = nOffsetToCentralDirectory;
                        _dataHeadersOptions.nID = STRUCTID_LOCALFILEHEADER;
                        listResult.append(getDataHeaders(_dataHeadersOptions, pPdStruct));
                    }
                }
            } else if (dataHeadersOptions.nID == STRUCTID_CENTRALDIRECTORYFILEHEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(dataHeadersOptions.nID));

                CENTRALDIRECTORYFILEHEADER cdh = read_CENTRALDIRECTORYFILEHEADER(nStartOffset, pPdStruct);

                dataHeader.nSize = sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength + cdh.nExtraFieldLength + cdh.nFileCommentLength;

                dataHeader.listRecords.append(getDataRecordDV(offsetof(CENTRALDIRECTORYFILEHEADER, nSignature), sizeof(quint32), "Signature", XBinary::VT_UINT32, 0,
                                                              XBinary::ENDIAN_LITTLE, XZip::getHeaderSignaturesS(), VL_TYPE_LIST));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nVersion), sizeof(quint8), "Version", XBinary::VT_UINT8, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nOS), sizeof(quint8), "OS", XBinary::VT_UINT8, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nMinVersion), sizeof(quint8), "MinVersion", XBinary::VT_UINT8, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nMinOS), sizeof(quint8), "MinOS", XBinary::VT_UINT8, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nFlags), sizeof(quint16), "Flags", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nMethod), sizeof(quint16), "Method", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nLastModTime), sizeof(quint16), "LastModTime", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nLastModDate), sizeof(quint16), "LastModDate", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nCRC32), sizeof(quint32), "CRC32", XBinary::VT_UINT32, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nCompressedSize), sizeof(quint32), "CompressedSize", XBinary::VT_UINT32,
                                                            DRF_SIZE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nUncompressedSize), sizeof(quint32), "UncompressedSize",
                                                            XBinary::VT_UINT32, DRF_SIZE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nFileNameLength), sizeof(quint16), "FileNameLength", XBinary::VT_UINT16,
                                                            DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nExtraFieldLength), sizeof(quint16), "ExtraFieldLength",
                                                            XBinary::VT_UINT16, DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nFileCommentLength), sizeof(quint16), "FileCommentLength",
                                                            XBinary::VT_UINT16, DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nStartDisk), sizeof(quint16), "StartDisk", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nInternalFileAttributes), sizeof(quint16), "InternalFileAttributes",
                                                            XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nExternalFileAttributes), sizeof(quint32), "ExternalFileAttributes",
                                                            XBinary::VT_UINT32, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(CENTRALDIRECTORYFILEHEADER, nOffsetToLocalFileHeader), sizeof(quint32), "OffsetToLocalFileHeader",
                                                            XBinary::VT_UINT32, DRF_OFFSET, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(sizeof(CENTRALDIRECTORYFILEHEADER), cdh.nFileNameLength, "FileName", XBinary::VT_CHAR_ARRAY, DRF_VOLATILE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength, cdh.nExtraFieldLength, "ExtraField",
                                                            XBinary::VT_BYTE_ARRAY, DRF_VOLATILE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength + cdh.nExtraFieldLength, cdh.nFileCommentLength,
                                                            "FileComment", XBinary::VT_CHAR_ARRAY, DRF_VOLATILE, XBinary::ENDIAN_LITTLE));

                listResult.append(dataHeader);
            } else if (dataHeadersOptions.nID == STRUCTID_LOCALFILEHEADER) {
                XBinary::DATA_HEADER dataHeader = _initDataHeader(dataHeadersOptions, structIDToString(dataHeadersOptions.nID));

                LOCALFILEHEADER lfh = read_LOCALFILEHEADER(nStartOffset, pPdStruct);

                dataHeader.nSize = sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;

                dataHeader.listRecords.append(getDataRecordDV(offsetof(LOCALFILEHEADER, nSignature), sizeof(quint32), "Signature", XBinary::VT_UINT32, 0,
                                                              XBinary::ENDIAN_LITTLE, XZip::getHeaderSignaturesS(), VL_TYPE_LIST));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(LOCALFILEHEADER, nMinVersion), sizeof(quint8), "MinVersion", XBinary::VT_UINT8, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(LOCALFILEHEADER, nMinOS), sizeof(quint8), "MinOS", XBinary::VT_UINT8, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(LOCALFILEHEADER, nFlags), sizeof(quint16), "Flags", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(LOCALFILEHEADER, nMethod), sizeof(quint16), "Method", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(LOCALFILEHEADER, nLastModTime), sizeof(quint16), "LastModTime", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(LOCALFILEHEADER, nLastModDate), sizeof(quint16), "LastModDate", XBinary::VT_UINT16, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(LOCALFILEHEADER, nCRC32), sizeof(quint32), "CRC32", XBinary::VT_UINT32, 0, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(LOCALFILEHEADER, nCompressedSize), sizeof(quint32), "CompressedSize", XBinary::VT_UINT32, DRF_SIZE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(LOCALFILEHEADER, nUncompressedSize), sizeof(quint32), "UncompressedSize", XBinary::VT_UINT32,
                                                            DRF_SIZE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(offsetof(LOCALFILEHEADER, nFileNameLength), sizeof(quint16), "FileNameLength", XBinary::VT_UINT16, DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(offsetof(LOCALFILEHEADER, nExtraFieldLength), sizeof(quint16), "ExtraFieldLength", XBinary::VT_UINT16,
                                                            DRF_COUNT, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(
                    getDataRecord(sizeof(LOCALFILEHEADER), lfh.nFileNameLength, "FileName", XBinary::VT_CHAR_ARRAY, DRF_VOLATILE, XBinary::ENDIAN_LITTLE));
                dataHeader.listRecords.append(getDataRecord(sizeof(LOCALFILEHEADER) + lfh.nFileNameLength, lfh.nExtraFieldLength, "ExtraField", XBinary::VT_BYTE_ARRAY,
                                                            DRF_VOLATILE, XBinary::ENDIAN_LITTLE));

                listResult.append(dataHeader);
            }
        }
    }

    return listResult;
}

QList<XBinary::FPART> XZip::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<XBinary::FPART> listResult;

    qint64 nECDOffset = findECDOffset(pPdStruct);
    qint64 nMaxOffset = 0;
    qint64 nTotalSize = getSize();

    if (nECDOffset != -1) {
        if (nECDOffset <= nTotalSize - sizeof(ENDOFCENTRALDIRECTORYRECORD)) {
            quint16 nTotalNumberOfRecords = read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));
            quint32 nSizeOfCentralDirectory = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nSizeOfCentralDirectory));
            quint32 nOffsetToCentralDirectory = read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));
            qint16 nCommentLength = read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nCommentLength));

            nMaxOffset = qMin((qint64)(nECDOffset + sizeof(ENDOFCENTRALDIRECTORYRECORD) + nCommentLength), nTotalSize);

            if (nFileParts & FILEPART_HEADER) {
                FPART record = {};

                record.filePart = FILEPART_HEADER;
                record.nFileOffset = nECDOffset;
                record.nFileSize = nMaxOffset - nECDOffset;
                record.nVirtualAddress = -1;
                record.sName = "End of Central Directory Record";

                listResult.append(record);
            }

            if ((nFileParts & FILEPART_HEADER) || (nFileParts & FILEPART_STREAM)) {
                if ((nOffsetToCentralDirectory < nECDOffset) && (nOffsetToCentralDirectory + nSizeOfCentralDirectory <= nECDOffset)) {
                    qint64 nOffset = nOffsetToCentralDirectory;

                    for (qint32 i = 0; i < nTotalNumberOfRecords && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                        if (nOffset + sizeof(CENTRALDIRECTORYFILEHEADER) <= nTotalSize) {
                            CENTRALDIRECTORYFILEHEADER cdh = read_CENTRALDIRECTORYFILEHEADER(nOffset, pPdStruct);

                            if (cdh.nSignature == SIGNATURE_CFD) {
                                if (nFileParts & FILEPART_HEADER) {
                                    FPART record = {};

                                    record.filePart = FILEPART_HEADER;
                                    record.nFileOffset = nOffset;
                                    record.nFileSize = (sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength + cdh.nExtraFieldLength + cdh.nFileCommentLength);
                                    record.nVirtualAddress = -1;
                                    record.sName = QString("%1 %2").arg(tr("Stream"), QString::number(i));
                                    QString sOriginalName = read_ansiString(nOffset + sizeof(CENTRALDIRECTORYFILEHEADER), cdh.nFileNameLength);
                                    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, sOriginalName);

                                    listResult.append(record);
                                }

                                qint64 nLocalOffset = cdh.nOffsetToLocalFileHeader;

                                if (nLocalOffset < nECDOffset) {
                                    LOCALFILEHEADER lfh = read_LOCALFILEHEADER(nLocalOffset, pPdStruct);

                                    if (lfh.nSignature == SIGNATURE_LFD) {
                                        if ((nFileParts & FILEPART_HEADER) || (nFileParts & FILEPART_STREAM)) {
                                            QString sName = QString("%1 %2").arg(tr("Stream"), QString::number(i));
                                            QString sOriginalName = read_ansiString(nLocalOffset + sizeof(LOCALFILEHEADER), lfh.nFileNameLength);

                                            if (nFileParts & FILEPART_HEADER) {
                                                FPART record = {};

                                                record.filePart = FILEPART_HEADER;
                                                record.nFileOffset = nLocalOffset;
                                                record.nFileSize = sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;
                                                record.nVirtualAddress = -1;
                                                record.sName = sName;
                                                record.mapProperties.insert(FPART_PROP_ORIGINALNAME, sOriginalName);

                                                listResult.append(record);
                                            }

                                            if (nFileParts & FILEPART_STREAM) {
                                                FPART record = {};

                                                record.filePart = FILEPART_STREAM;
                                                record.nFileOffset = nLocalOffset + sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;
                                                record.nFileSize = cdh.nCompressedSize;
                                                record.nVirtualAddress = -1;
                                                record.sName = sName;
                                                record.mapProperties.insert(FPART_PROP_ORIGINALNAME, sOriginalName);
                                                record.mapProperties.insert(FPART_PROP_HANDLEMETHOD1, zipToCompressMethod(cdh.nMethod, cdh.nFlags));
                                                record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, cdh.nCompressedSize);
                                                record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, cdh.nUncompressedSize);
                                                record.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_EDB88320);
                                                record.mapProperties.insert(FPART_PROP_CRC_VALUE, cdh.nCRC32);

                                                if (cdh.nFlags & 0x01) record.mapProperties.insert(FPART_PROP_ENCRYPTED, true);

                                                record.mapProperties.insert(FPART_PROP_DATETIME, XBinary::dosDateTimeToQDateTime(cdh.nLastModDate, cdh.nLastModTime));

                                                listResult.append(record);
                                            }
                                        }
                                    }
                                }
                            }

                            nOffset += (sizeof(CENTRALDIRECTORYFILEHEADER) + cdh.nFileNameLength + cdh.nExtraFieldLength + cdh.nFileCommentLength);
                        }
                    }
                }
            }
        }
    } else {
        qint64 nRealSize = 0;
        qint32 nCount = _getNumberOfLocalFileHeaders(0, nTotalSize, &nRealSize, pPdStruct);

        qint64 nOffset = 0;

        for (qint32 i = 0; i < nCount && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
            if (nOffset + sizeof(LOCALFILEHEADER) <= nTotalSize) {
                LOCALFILEHEADER lfh = read_LOCALFILEHEADER(nOffset, pPdStruct);

                if (lfh.nSignature == SIGNATURE_LFD) {
                    if ((nFileParts & FILEPART_HEADER) || (nFileParts & FILEPART_STREAM)) {
                        QString sOriginalName = read_ansiString(nOffset + sizeof(LOCALFILEHEADER), lfh.nFileNameLength);

                        if (nFileParts & FILEPART_HEADER) {
                            FPART record = {};

                            record.filePart = FILEPART_HEADER;
                            record.nFileOffset = nOffset;
                            record.nFileSize = sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;
                            record.nVirtualAddress = -1;
                            record.mapProperties.insert(FPART_PROP_ORIGINALNAME, sOriginalName);

                            listResult.append(record);
                        }
                        if (nFileParts & FILEPART_STREAM) {
                            FPART record = {};

                            record.filePart = FILEPART_STREAM;
                            record.nFileOffset = nOffset + sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;
                            record.nFileSize = lfh.nCompressedSize;
                            record.nVirtualAddress = -1;
                            record.mapProperties.insert(FPART_PROP_ORIGINALNAME, sOriginalName);
                            record.mapProperties.insert(FPART_PROP_HANDLEMETHOD1, zipToCompressMethod(lfh.nMethod, lfh.nFlags));
                            record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, lfh.nCompressedSize);
                            record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, lfh.nUncompressedSize);
                            record.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_EDB88320);
                            record.mapProperties.insert(FPART_PROP_CRC_VALUE, lfh.nCRC32);
                            record.mapProperties.insert(FPART_PROP_DATETIME, XBinary::dosDateTimeToQDateTime(lfh.nLastModDate, lfh.nLastModTime));

                            listResult.append(record);
                        }
                    }
                } else {
                    break;
                }

                nOffset += (sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength + lfh.nCompressedSize);
#ifdef QT_DEBUG
                qDebug("Offset: %X", nOffset);
#endif
            }
        }

        nMaxOffset = nRealSize;
    }

    if (nFileParts & FILEPART_DATA) {
        FPART record = {};

        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = nMaxOffset;
        record.nVirtualAddress = -1;
        record.sName = tr("Data");

        listResult.append(record);
    }

    if (nFileParts & FILEPART_OVERLAY) {
        if (nMaxOffset < getSize()) {
            FPART record = {};

            record.filePart = FILEPART_OVERLAY;
            record.nFileOffset = nMaxOffset;
            record.nFileSize = nTotalSize - nMaxOffset;
            record.nVirtualAddress = -1;
            record.sName = tr("Overlay");

            listResult.append(record);
        }
    }

    return listResult;
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

        for (qint32 i = 0; i < (nNumberOfRecords) && isPdStructNotCanceled(pPdStruct); i++) {
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

qint32 XZip::_getNumberOfLocalFileHeaders(qint64 nOffset, qint64 nSize, qint64 *pnRealSize, PDSTRUCT *pPdStruct)
{
    qint32 nResult = 0;

    if (nOffset != -1) {
        qint64 nCurrentOffset = nOffset;

        while (XBinary::isPdStructNotCanceled(pPdStruct)) {
            if ((nCurrentOffset + (qint64)sizeof(LOCALFILEHEADER)) > nSize) {
                break;
            }

            quint32 nLocalSignature = read_uint32(nCurrentOffset + offsetof(LOCALFILEHEADER, nSignature));
            quint32 nLocalFileNameSize = read_uint16(nCurrentOffset + offsetof(LOCALFILEHEADER, nFileNameLength));
            quint32 nLocalExtraFieldSize = read_uint16(nCurrentOffset + offsetof(LOCALFILEHEADER, nExtraFieldLength));
            quint32 nCompressedSize = read_uint32(nCurrentOffset + offsetof(LOCALFILEHEADER, nCompressedSize));

            if (nLocalSignature != SIGNATURE_LFD) {
                break;
            }

            nResult++;

            nCurrentOffset += sizeof(LOCALFILEHEADER) + nLocalFileNameSize + nLocalExtraFieldSize + nCompressedSize;
        }

        if (pnRealSize) {
            *pnRealSize = nCurrentOffset - nOffset;
        }
    }

    return nResult;
}

XArchive::HANDLE_METHOD XZip::zipToCompressMethod(quint16 nZipMethod, quint32 nFlags)
{
    HANDLE_METHOD result = HANDLE_METHOD_UNKNOWN;

    switch (nZipMethod) {
        case CMETHOD_STORE: result = HANDLE_METHOD_STORE; break;
        case CMETHOD_SHRINK: result = HANDLE_METHOD_SHRINK; break;
        case CMETHOD_REDUCED_1: result = HANDLE_METHOD_REDUCE_1; break;
        case CMETHOD_REDUCED_2: result = HANDLE_METHOD_REDUCE_2; break;
        case CMETHOD_REDUCED_3: result = HANDLE_METHOD_REDUCE_3; break;
        case CMETHOD_REDUCED_4: result = HANDLE_METHOD_REDUCE_4; break;

        case CMETHOD_IMPLODED: {
            bool b8kdict = (nFlags & 0x02) ? true : false;
            bool b3tree = (nFlags & 0x04) ? true : false;

            if (!b8kdict && !b3tree) {
                result = HANDLE_METHOD_IMPLODED_4KDICT_2TREES;
            } else if (!b8kdict && b3tree) {
                result = HANDLE_METHOD_IMPLODED_4KDICT_3TREES;
            } else if (b8kdict && !b3tree) {
                result = HANDLE_METHOD_IMPLODED_8KDICT_2TREES;
            } else if (b8kdict && b3tree) {
                result = HANDLE_METHOD_IMPLODED_8KDICT_3TREES;
            }
        } break;
        case CMETHOD_DEFLATE: result = HANDLE_METHOD_DEFLATE; break;
        case CMETHOD_DEFLATE64: result = HANDLE_METHOD_DEFLATE64; break;  // TODO
        case CMETHOD_BZIP2: result = HANDLE_METHOD_BZIP2; break;
        case CMETHOD_LZMA: result = HANDLE_METHOD_LZMA; break;
        case CMETHOD_XZ: result = HANDLE_METHOD_XZ; break;
        case CMETHOD_PPMD: result = HANDLE_METHOD_PPMD8; break;
        case CMETHOD_AES: result = HANDLE_METHOD_AES; break;
    }
    // TODO more methods

    return result;
}

bool XZip::initPack(PACK_STATE *pState, QIODevice *pDevice, const QMap<PACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState) {
        return false;
    }

    pState->pDevice = pDevice;
    pState->mapProperties = mapProperties;

    // ZIP archives don't have a signature at the beginning
    // They start with local file headers
    // Initialize state
    pState->nCurrentOffset = 0;
    pState->nNumberOfRecords = 0;

    // Allocate context to store list of ZIPFILE_RECORD structures and options
    ZIP_PACK_CONTEXT *pContext = new ZIP_PACK_CONTEXT();
    pContext->pListZipFileRecords = new QList<ZIPFILE_RECORD>();

    pState->pContext = (void *)pContext;

    return true;
}

bool XZip::addFile(PACK_STATE *pState, const QString &sFilePath, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDevice || !pState->pContext) {
        return false;
    }

    ZIP_PACK_CONTEXT *pContext = (ZIP_PACK_CONTEXT *)pState->pContext;
    QList<ZIPFILE_RECORD> *pListZipFileRecords = pContext->pListZipFileRecords;

    // Check if file exists and is readable
    QFileInfo fileInfo(sFilePath);

    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        return false;
    }

    // Determine file path to store in archive based on PATH_MODE
    QString sStoredPath;
    PATH_MODE pathMode = (PATH_MODE)pState->mapProperties.value(PACK_PROP_PATHMODE, PATH_MODE_BASENAME).toInt();
    QString sBasePath = pState->mapProperties.value(PACK_PROP_BASEPATH).toString();
    HANDLE_METHOD compressMethod = (HANDLE_METHOD)pState->mapProperties.value(PACK_PROP_COMPRESSMETHOD, HANDLE_METHOD_DEFLATE).toInt();
    CRYPTO_METHOD cryptoMethod = (CRYPTO_METHOD)pState->mapProperties.value(PACK_PROP_ENCRYPTIONMETHOD, CRYPTO_METHOD_NONE).toInt();
    QString sPassword = pState->mapProperties.value(PACK_PROP_PASSWORD).toString();
    qint32 nCompressionLevel = pState->mapProperties.value(PACK_PROP_COMPRESSIONLEVEL, -1).toInt();

    switch (pathMode) {
        case XBinary::PATH_MODE_ABSOLUTE: sStoredPath = fileInfo.absoluteFilePath(); break;
        case XBinary::PATH_MODE_RELATIVE:
            if (!sBasePath.isEmpty()) {
                QDir baseDir(sBasePath);
                sStoredPath = baseDir.relativeFilePath(fileInfo.absoluteFilePath());
            } else {
                sStoredPath = fileInfo.fileName();
            }
            break;
        case XBinary::PATH_MODE_BASENAME:
        default: sStoredPath = fileInfo.fileName(); break;
    }

    // Normalize path separators to forward slashes (ZIP standard)
    sStoredPath = sStoredPath.replace("\\", "/");

    CMETHOD cmethod = CMETHOD_STORE;

    if (compressMethod == XArchive::HANDLE_METHOD_DEFLATE) {
        cmethod = CMETHOD_DEFLATE;
    } else if (compressMethod == XArchive::HANDLE_METHOD_STORE) {
        cmethod = CMETHOD_STORE;
    }

    ZIPFILE_RECORD zipFileRecord = {};
    zipFileRecord.sFileName = sStoredPath;
    zipFileRecord.nVersion = 0x14;
    zipFileRecord.nOS = 0;
    zipFileRecord.nMinVersion = 0x14;
    zipFileRecord.nMinOS = 0;
    zipFileRecord.nFlags = 0;
    zipFileRecord.method = cmethod;  // Use compression method from options
    zipFileRecord.dtTime = fileInfo.lastModified();
    zipFileRecord.nUncompressedSize = fileInfo.size();

    if (nCompressionLevel == -1) {
        nCompressionLevel = 8;
    }

    // Get file permissions and convert to external attributes
    QFile::Permissions permissions = fileInfo.permissions();
    zipFileRecord.nExternalFileAttributes = filePermissionsToExternalAttributes(permissions);

    // Calculate CRC32
    QFile file(sFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    zipFileRecord.nCRC32 = XBinary::_getCRC32(&file);
    file.close();

    // Convert QDateTime to DOS date/time
    QPair<quint16, quint16> dosDateTime = XBinary::qDateTimeToDosDateTime(zipFileRecord.dtTime);
    quint16 nDosDate = dosDateTime.first;
    quint16 nDosTime = dosDateTime.second;

    // Check if encryption is needed
    bool bEncrypt = !sPassword.isEmpty() && (cryptoMethod == XBinary::CRYPTO_METHOD_ZIPCRYPTO);
    if (bEncrypt) {
        zipFileRecord.nFlags |= 0x01;  // Set encryption flag
    }

    // Write local file header
    zipFileRecord.nHeaderOffset = pState->nCurrentOffset;

    LOCALFILEHEADER localFileHeader = {};
    localFileHeader.nSignature = SIGNATURE_LFD;
    localFileHeader.nMinVersion = zipFileRecord.nMinVersion;
    localFileHeader.nMinOS = zipFileRecord.nMinOS;
    localFileHeader.nFlags = zipFileRecord.nFlags;
    localFileHeader.nMethod = zipFileRecord.method;
    localFileHeader.nLastModTime = nDosTime;
    localFileHeader.nLastModDate = nDosDate;
    localFileHeader.nCRC32 = zipFileRecord.nCRC32;
    localFileHeader.nCompressedSize = zipFileRecord.nUncompressedSize;  // STORE: compressed = uncompressed
    localFileHeader.nUncompressedSize = zipFileRecord.nUncompressedSize;
    localFileHeader.nFileNameLength = zipFileRecord.sFileName.toUtf8().size();
    localFileHeader.nExtraFieldLength = 0;

    if (pState->pDevice->write((char *)&localFileHeader, sizeof(localFileHeader)) != sizeof(localFileHeader)) {
        return false;
    }

    QByteArray baFileName = zipFileRecord.sFileName.toUtf8();
    if (pState->pDevice->write(baFileName.data(), localFileHeader.nFileNameLength) != localFileHeader.nFileNameLength) {
        return false;
    }

    pState->nCurrentOffset += sizeof(localFileHeader) + localFileHeader.nFileNameLength;
    zipFileRecord.nDataOffset = pState->nCurrentOffset;

    // Write file data (with optional compression)
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    qint64 nBytesWritten = 0;
    qint64 nFileSize = fileInfo.size();

    if (zipFileRecord.method == CMETHOD_STORE) {
        // No compression - direct copy (with optional encryption)
        if (bEncrypt) {
            // Encrypt the file data
            QBuffer dataBuffer;
            dataBuffer.open(QIODevice::WriteOnly);

            // First, read all data to a buffer for encryption
            QByteArray baFileData = file.readAll();
            file.close();

            if (baFileData.size() != nFileSize) {
                return false;
            }

            // Prepare encryption
            QBuffer inputBuffer(&baFileData);
            inputBuffer.open(QIODevice::ReadOnly);

            XBinary::DATAPROCESS_STATE encryptState = {};
            encryptState.pDeviceInput = &inputBuffer;
            encryptState.pDeviceOutput = &dataBuffer;
            encryptState.nInputOffset = 0;
            encryptState.nInputLimit = nFileSize;

            bool bEncryptResult = XZipCryptoDecoder::encrypt(&encryptState, sPassword, zipFileRecord.nCRC32, pPdStruct);

            if (bEncryptResult) {
                dataBuffer.close();
                QByteArray baEncrypted = dataBuffer.data();

                if (pState->pDevice->write(baEncrypted) != baEncrypted.size()) {
                    return false;
                }

                nBytesWritten = baEncrypted.size();
                zipFileRecord.nCompressedSize = nBytesWritten;  // Encrypted size (original + 12 byte header)
            } else {
                return false;
            }
        } else {
            // No encryption - direct copy
            while (nBytesWritten < nFileSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
                QByteArray baBuffer = file.read(qMin((qint64)0x10000, nFileSize - nBytesWritten));

                if (baBuffer.isEmpty() && nBytesWritten < nFileSize) {
                    file.close();
                    return false;
                }

                if (pState->pDevice->write(baBuffer) != baBuffer.size()) {
                    file.close();
                    return false;
                }

                nBytesWritten += baBuffer.size();
            }

            file.close();
            zipFileRecord.nCompressedSize = nFileSize;  // STORE: compressed = uncompressed
        }
    } else if (zipFileRecord.method == CMETHOD_DEFLATE) {
        // Compress data using DEFLATE (with optional encryption)
        QBuffer compressedBuffer;
        compressedBuffer.open(QIODevice::WriteOnly);

        XBinary::DATAPROCESS_STATE compressState = {};
        compressState.pDeviceInput = &file;
        compressState.pDeviceOutput = &compressedBuffer;
        compressState.nInputOffset = 0;
        compressState.nInputLimit = nFileSize;

        bool bCompress = XDeflateDecoder::compress(&compressState, pPdStruct, nCompressionLevel);

        if (bCompress) {
            compressedBuffer.close();
            QByteArray baCompressed = compressedBuffer.data();

            if (bEncrypt) {
                // Encrypt the compressed data
                QBuffer encryptedBuffer;
                encryptedBuffer.open(QIODevice::WriteOnly);

                QBuffer inputBuffer(&baCompressed);
                inputBuffer.open(QIODevice::ReadOnly);

                XBinary::DATAPROCESS_STATE encryptState = {};
                encryptState.pDeviceInput = &inputBuffer;
                encryptState.pDeviceOutput = &encryptedBuffer;
                encryptState.nInputOffset = 0;
                encryptState.nInputLimit = baCompressed.size();

                bool bEncryptResult = XZipCryptoDecoder::encrypt(&encryptState, sPassword, zipFileRecord.nCRC32, pPdStruct);

                if (bEncryptResult) {
                    encryptedBuffer.close();
                    QByteArray baEncrypted = encryptedBuffer.data();

                    if (pState->pDevice->write(baEncrypted) != baEncrypted.size()) {
                        file.close();
                        return false;
                    }

                    nBytesWritten = baEncrypted.size();
                    zipFileRecord.nCompressedSize = nBytesWritten;
                } else {
                    file.close();
                    return false;
                }
            } else {
                // No encryption
                if (pState->pDevice->write(baCompressed) != baCompressed.size()) {
                    file.close();
                    return false;
                }

                nBytesWritten = compressState.nCountOutput;
                zipFileRecord.nCompressedSize = nBytesWritten;
            }
        } else {
            file.close();
            return false;
        }

        file.close();
    } else {
        // Unsupported compression method
        file.close();
        return false;
    }

    file.close();

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Update local file header with compressed size
    localFileHeader.nCompressedSize = zipFileRecord.nCompressedSize;
    localFileHeader.nMethod = zipFileRecord.method;

    // Write updated local file header
    if (pState->pDevice->seek(zipFileRecord.nHeaderOffset) && pState->pDevice->write((char *)&localFileHeader, sizeof(localFileHeader)) != sizeof(localFileHeader)) {
        return false;
    }

    // Update current offset to point to end of written data
    pState->nCurrentOffset += zipFileRecord.nCompressedSize;

    // Seek to end of data
    pState->pDevice->seek(pState->nCurrentOffset);

    // Store the record for later when writing central directory
    pListZipFileRecords->append(zipFileRecord);

    return true;
}

bool XZip::addFolder(PACK_STATE *pState, const QString &sDirectoryPath, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDevice || !pState->pContext) {
        return false;
    }

    // Check if directory exists
    if (!XBinary::isDirectoryExists(sDirectoryPath)) {
        return false;
    }

    ZIP_PACK_CONTEXT *pContext = (ZIP_PACK_CONTEXT *)pState->pContext;

    // Set base path for relative path calculation if not already set
    QString sOriginalBasePath;
    bool bRestoreBasePath = false;
    PATH_MODE pathMode = (PATH_MODE)pState->mapProperties.value(PACK_PROP_PATHMODE, PATH_MODE_BASENAME).toInt();
    QString sBasePath = pState->mapProperties.value(PACK_PROP_BASEPATH).toString();

    if (pathMode == XBinary::PATH_MODE_RELATIVE && sBasePath.isEmpty()) {
        sOriginalBasePath = sBasePath;
        sBasePath = sDirectoryPath;
        pState->mapProperties[PACK_PROP_BASEPATH] = sBasePath;  // Update the map so addFile() can use it
        bRestoreBasePath = true;
    }

    // Enumerate all files in directory
    QList<QString> listFiles;
    XBinary::findFiles(sDirectoryPath, &listFiles, true, 0, pPdStruct);

    qint32 nNumberOfFiles = listFiles.count();

    // Add each file
    for (qint32 i = 0; (i < nNumberOfFiles) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        QString sFilePath = listFiles.at(i);
        QFileInfo fileInfo(sFilePath);

        // Skip directories (ZIP stores files only)
        if (fileInfo.isDir()) {
            continue;
        }

        // Add file to archive
        if (!addFile(pState, sFilePath, pPdStruct)) {
            if (bRestoreBasePath) {
                pState->mapProperties[PACK_PROP_BASEPATH] = sOriginalBasePath;
            }
            return false;
        }
    }

    // Restore original base path if we changed it
    if (bRestoreBasePath) {
        pState->mapProperties[PACK_PROP_BASEPATH] = sOriginalBasePath;
    }

    return true;
}

bool XZip::finishPack(PACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (!pState || !pState->pDevice || !pState->pContext) {
        return false;
    }

    ZIP_PACK_CONTEXT *pContext = (ZIP_PACK_CONTEXT *)pState->pContext;
    QList<ZIPFILE_RECORD> *pListZipFileRecords = pContext->pListZipFileRecords;

    // Write central directory and end of central directory record
    bool bResult = addCentralDirectory(pState->pDevice, pListZipFileRecords, "");

    // Clean up context
    delete pListZipFileRecords;
    delete pContext;
    pState->pContext = nullptr;

    return bResult;
}

bool XZip::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    pState->mapProperties = mapProperties;

    bool bResult = false;

    PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    if (pState) {
        pState->nCurrentOffset = 0;
        pState->nTotalSize = getSize();
        pState->nCurrentIndex = 0;
        pState->nNumberOfRecords = 0;
        pState->pContext = nullptr;

        // Try to get number of records from end of central directory
        qint64 nECDOffset = findECDOffset(pPdStruct);
        bool bIsECD = false;
        qint64 nCDFHOffset = 0;

        if (nECDOffset != -1) {
            nCDFHOffset = (qint64)read_uint32(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

            if (read_uint32(nCDFHOffset) == SIGNATURE_CFD) {
                bIsECD = true;
            }
        }

        if (bIsECD) {
            pState->nCurrentOffset = nCDFHOffset;
            pState->nNumberOfRecords = read_uint16(nECDOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nTotalNumberOfRecords));
            bResult = (pState->nNumberOfRecords > 0);
        } else {
            // Fallback: count local file headers
            qint64 nOffset = 0;
            qint64 nTotalSize = pState->nTotalSize;

            while ((nOffset < nTotalSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                if ((nOffset + (qint64)sizeof(LOCALFILEHEADER)) > nTotalSize) {
                    break;
                }

                quint32 nLocalSignature = read_uint32(nOffset + offsetof(LOCALFILEHEADER, nSignature));

                if (nLocalSignature != SIGNATURE_LFD) {
                    break;
                }

                LOCALFILEHEADER lfh = read_LOCALFILEHEADER(nOffset, pPdStruct);

                pState->nNumberOfRecords++;

                nOffset += sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength + lfh.nCompressedSize;
            }

            pState->nCurrentOffset = 0;
            bResult = (pState->nNumberOfRecords > 0);
        }

        if (bResult) {
            pState->pContext = new ZIP_UNPACK_CONTEXT;
            ((ZIP_UNPACK_CONTEXT *)pState->pContext)->bIsECD = bIsECD;
        }
    }

    return bResult;
}

XBinary::ARCHIVERECORD XZip::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    XBinary::ARCHIVERECORD result = {};

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        bool bIsECD = ((ZIP_UNPACK_CONTEXT *)pState->pContext)->bIsECD;
        qint64 nLocalHeaderOffset = 0;
        QString sFileName;

        quint8 nVersion = 0;
        quint8 nOS = 0;
        quint8 nMinVersion = 0;
        quint8 nMinOS = 0;
        quint16 nFlags = 0;
        quint16 nMethod = 0;
        quint16 nLastModTime = 0;
        quint16 nLastModDate = 0;
        quint32 nCRC32 = 0;
        quint32 nCompressedSize = 0;
        quint32 nUncompressedSize = 0;
        // Extra field and file comment information
        qint64 nExtraFieldOffset = 0;
        qint64 nExtraFieldLength = 0;
        qint64 nFileCommentOffset = 0;
        qint64 nFileCommentLength = 0;

        if (bIsECD) {
            CENTRALDIRECTORYFILEHEADER cdfh = read_CENTRALDIRECTORYFILEHEADER(pState->nCurrentOffset, pPdStruct);
            nVersion = cdfh.nVersion;
            nOS = cdfh.nOS;
            nMinVersion = cdfh.nMinVersion;
            nMinOS = cdfh.nMinOS;
            nFlags = cdfh.nFlags;
            nMethod = cdfh.nMethod;
            nLastModTime = cdfh.nLastModTime;
            nLastModDate = cdfh.nLastModDate;
            nCRC32 = cdfh.nCRC32;
            nCompressedSize = cdfh.nCompressedSize;
            nUncompressedSize = cdfh.nUncompressedSize;
            sFileName = read_ansiString(pState->nCurrentOffset + sizeof(CENTRALDIRECTORYFILEHEADER), cdfh.nFileNameLength);

            nLocalHeaderOffset = cdfh.nOffsetToLocalFileHeader;

            nExtraFieldOffset = pState->nCurrentOffset + sizeof(CENTRALDIRECTORYFILEHEADER) + cdfh.nFileNameLength;
            nExtraFieldLength = cdfh.nExtraFieldLength;
            nFileCommentOffset = nExtraFieldOffset + nExtraFieldLength;
            nFileCommentLength = cdfh.nFileCommentLength;
        } else {
            nLocalHeaderOffset = pState->nCurrentOffset;
        }

        LOCALFILEHEADER lfh = read_LOCALFILEHEADER(nLocalHeaderOffset, pPdStruct);

        if (!bIsECD) {
            nMinVersion = lfh.nMinVersion;
            nMinOS = lfh.nMinOS;
            nCRC32 = lfh.nCRC32;
            nFlags = lfh.nFlags;
            nMethod = lfh.nMethod;
            nLastModTime = lfh.nLastModTime;
            nLastModDate = lfh.nLastModDate;
            nCompressedSize = lfh.nCompressedSize;
            nUncompressedSize = lfh.nUncompressedSize;
            sFileName = read_ansiString(nLocalHeaderOffset + sizeof(LOCALFILEHEADER), lfh.nFileNameLength);

            nExtraFieldOffset = nLocalHeaderOffset + sizeof(LOCALFILEHEADER) + lfh.nFileNameLength;
            nExtraFieldLength = lfh.nExtraFieldLength;
            nFileCommentOffset = 0;
            nFileCommentLength = 0;
        }

        bool bIsFolder = sFileName.endsWith(QLatin1Char('/'));

        result.mapProperties.insert(XBinary::FPART_PROP_ISFOLDER, bIsFolder);

        result.nStreamSize = nCompressedSize;
        result.nStreamOffset = nLocalHeaderOffset + sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength;

        result.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sFileName);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMOFFSET, result.nStreamOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_STREAMSIZE, result.nStreamSize);

        // Compression method
        HANDLE_METHOD compressMethod = zipToCompressMethod(nMethod, nFlags);
        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD1, compressMethod);
        result.mapProperties.insert(XBinary::FPART_PROP_TYPE, (quint32)nMethod);  // Raw ZIP method number

        // CRC32
        result.mapProperties.insert(XBinary::FPART_PROP_CRC_VALUE, nCRC32);
        result.mapProperties.insert(XBinary::FPART_PROP_CRC_TYPE, XBinary::CRC_TYPE_EDB88320);

        // Sizes
        result.mapProperties.insert(XBinary::FPART_PROP_COMPRESSEDSIZE, nCompressedSize);
        result.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nUncompressedSize);

        // Date/Time
        QDateTime dateTime = dosDateTimeToQDateTime(nLastModDate, nLastModTime);
        if (dateTime.isValid()) {
            result.mapProperties.insert(XBinary::FPART_PROP_DATETIME, dateTime);
        }

        result.mapProperties.insert(XBinary::FPART_PROP_FLAGS, nFlags);

        if (nFlags & 0x01) {
            result.mapProperties.insert(XBinary::FPART_PROP_ENCRYPTED, true);
            result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD2, HANDLE_METHOD_ZIPCRYPTO);
        }

        if (nMethod == CMETHOD_AES) {
            result.mapProperties.insert(XBinary::FPART_PROP_ENCRYPTED, true);
            result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD1, HANDLE_METHOD_STORE);
            result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD2, HANDLE_METHOD_AES);
        }

        result.mapProperties.insert(XBinary::FPART_PROP_EXTRAFIELDOFFSET, nExtraFieldOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_EXTRAFIELDLENGTH, nExtraFieldLength);
        result.mapProperties.insert(XBinary::FPART_PROP_FILECOMMENTOFFSET, nFileCommentOffset);
        result.mapProperties.insert(XBinary::FPART_PROP_FILECOMMENTLENGTH, nFileCommentLength);

        nExtraFieldLength = qMin(nExtraFieldLength, (qint64)0xFFFF);
        // read Extra field data if present
        for (qint32 i = 0; (i < nExtraFieldLength) && isPdStructNotCanceled(pPdStruct);) {
            quint16 nHeaderID = read_uint16(nExtraFieldOffset + i + 0);
            quint16 nDataSize = read_uint16(nExtraFieldOffset + i + 2);

            if (nHeaderID == ZIP_AES_EXTRA_FIELD_HEADER_ID) {
                if ((nMethod == CMETHOD_AES) && (nDataSize >= ZIP_AES_EXTRA_FIELD_DATA_SIZE)) {
                    AES_EXTRA_FIELD aesExtraField = read_AES_EXTRA_FIELD(nExtraFieldOffset + i, pPdStruct);

                    // Original compression method
                    HANDLE_METHOD compressMethod = zipToCompressMethod(aesExtraField.nCompressionMethod, nFlags);
                    result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD1, compressMethod);

                    if (aesExtraField.nEncryptionMode == 1) {
                        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD2, HANDLE_METHOD_AES128);
                    } else if (aesExtraField.nEncryptionMode == 2) {
                        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD2, HANDLE_METHOD_AES192);
                    } else if (aesExtraField.nEncryptionMode == 3) {
                        result.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD2, HANDLE_METHOD_AES256);
                    }
                }
            }

            i += (4 + nDataSize);
        }
    }

    return result;
}

bool XZip::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    if (pState && pDevice && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        ARCHIVERECORD archiveRecord = infoCurrent(pState, pPdStruct);

        XDecompress xDecompress;
        connect(&xDecompress, &XDecompress::errorMessage, this, &XBinary::errorMessage);
        connect(&xDecompress, &XDecompress::infoMessage, this, &XBinary::infoMessage);

        bResult = xDecompress.decompressArchiveRecord(archiveRecord, getDevice(), pDevice, pState->mapProperties, pPdStruct);
    }

    return bResult;
}

bool XZip::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    bool bResult = false;

    if (pState && (pState->nCurrentIndex < pState->nNumberOfRecords)) {
        bool bIsECD = ((ZIP_UNPACK_CONTEXT *)pState->pContext)->bIsECD;

        if (bIsECD) {
            CENTRALDIRECTORYFILEHEADER cdfh = read_CENTRALDIRECTORYFILEHEADER(pState->nCurrentOffset, pPdStruct);
            pState->nCurrentOffset += sizeof(CENTRALDIRECTORYFILEHEADER) + cdfh.nFileNameLength + cdfh.nExtraFieldLength + cdfh.nFileCommentLength;
        } else {
            LOCALFILEHEADER lfh = read_LOCALFILEHEADER(pState->nCurrentOffset, pPdStruct);
            pState->nCurrentOffset += sizeof(LOCALFILEHEADER) + lfh.nFileNameLength + lfh.nExtraFieldLength + lfh.nCompressedSize;
        }

        pState->nCurrentIndex++;

        bResult = (pState->nCurrentIndex < pState->nNumberOfRecords);
    }

    return bResult;
}

bool XZip::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (pState && pState->pContext) {
        ZIP_UNPACK_CONTEXT *pZipUnpackContext = (ZIP_UNPACK_CONTEXT *)pState->pContext;
        delete pZipUnpackContext;
        pState->pContext = nullptr;
    }

    return true;
}

QVariant XZip::calculateHash(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!pDevice) {
        return 0;
    }

    XBinary binary(pDevice);

    quint32 nCRC = binary._getCRC32(0, binary.getSize(), 0xFFFFFFFF, _getCRC32Table_EDB88320(), pPdStruct);

    pDevice->reset();

    return nCRC;
}

quint32 XZip::filePermissionsToExternalAttributes(QFile::Permissions permissions)
{
    quint32 nResult = 0;

    // Unix file permissions format (stored in high 16 bits)
    // Format: 0xFFFF0000 where the high word contains Unix mode
    quint16 nUnixMode = 0;

    // File type (regular file)
    nUnixMode |= 0x8000;  // S_IFREG

    // Owner permissions
    if (permissions & QFile::ReadOwner) nUnixMode |= 0x0100;   // S_IRUSR
    if (permissions & QFile::WriteOwner) nUnixMode |= 0x0080;  // S_IWUSR
    if (permissions & QFile::ExeOwner) nUnixMode |= 0x0040;    // S_IXUSR

    // Group permissions
    if (permissions & QFile::ReadGroup) nUnixMode |= 0x0020;   // S_IRGRP
    if (permissions & QFile::WriteGroup) nUnixMode |= 0x0010;  // S_IWGRP
    if (permissions & QFile::ExeGroup) nUnixMode |= 0x0008;    // S_IXGRP

    // Other permissions
    if (permissions & QFile::ReadOther) nUnixMode |= 0x0004;   // S_IROTH
    if (permissions & QFile::WriteOther) nUnixMode |= 0x0002;  // S_IWOTH
    if (permissions & QFile::ExeOther) nUnixMode |= 0x0001;    // S_IXOTH

    // Store Unix mode in high 16 bits
    nResult = ((quint32)nUnixMode) << 16;

    // Low 16 bits: DOS attributes (optional)
    // Bit 0: Read-only
    // Bit 1: Hidden
    // Bit 2: System
    // Bit 5: Archive
    if (!(permissions & QFile::WriteOwner)) {
        nResult |= 0x01;  // Read-only
    }
    nResult |= 0x20;  // Archive bit

    return nResult;
}

QFile::Permissions XZip::externalAttributesToFilePermissions(quint32 nExternalAttributes)
{
    QFile::Permissions permissions = QFile::Permissions();

    // Extract Unix mode from high 16 bits
    quint16 nUnixMode = (quint16)((nExternalAttributes >> 16) & 0xFFFF);

    if (nUnixMode != 0) {
        // Owner permissions
        if (nUnixMode & 0x0100) permissions |= QFile::ReadOwner;   // S_IRUSR
        if (nUnixMode & 0x0080) permissions |= QFile::WriteOwner;  // S_IWUSR
        if (nUnixMode & 0x0040) permissions |= QFile::ExeOwner;    // S_IXUSR

        // Group permissions
        if (nUnixMode & 0x0020) permissions |= QFile::ReadGroup;   // S_IRGRP
        if (nUnixMode & 0x0010) permissions |= QFile::WriteGroup;  // S_IWGRP
        if (nUnixMode & 0x0008) permissions |= QFile::ExeGroup;    // S_IXGRP

        // Other permissions
        if (nUnixMode & 0x0004) permissions |= QFile::ReadOther;   // S_IROTH
        if (nUnixMode & 0x0002) permissions |= QFile::WriteOther;  // S_IWOTH
        if (nUnixMode & 0x0001) permissions |= QFile::ExeOther;    // S_IXOTH
    } else {
        // Fallback: use DOS attributes from low 16 bits
        bool bReadOnly = (nExternalAttributes & 0x01) != 0;

        if (bReadOnly) {
            permissions = QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther;
        } else {
            permissions = QFile::ReadOwner | QFile::ReadUser | QFile::ReadGroup | QFile::ReadOther | QFile::WriteOwner | QFile::WriteUser;
        }
    }

    return permissions;
}
