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
#include "xapk.h"

XAPK::XAPK(QIODevice *pDevice) : XJAR(pDevice)
{
}

bool XAPK::isValid(PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XZip xzip(getDevice());

    if (xzip.isValid()) {
        qint64 nECDOffset = xzip.findECDOffset(pPdStruct);
        bResult = xzip.isAPK(nECDOffset, pPdStruct);
    }

    return bResult;
}

bool XAPK::isValid(QIODevice *pDevice)
{
    XAPK xapk(pDevice);

    return xapk.isValid();
}

bool XAPK::isValid(QList<RECORD> *pListRecords, PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    bResult =
        (XArchive::isArchiveRecordPresent("classes.dex", pListRecords, pPdStruct) || XArchive::isArchiveRecordPresent("AndroidManifest.xml", pListRecords, pPdStruct));

    return bResult;
}

XBinary::FT XAPK::getFileType()
{
    return FT_APK;
}

XBinary::FILEFORMATINFO XAPK::getFileFormatInfo(PDSTRUCT *pPdStruct)
{
    XBinary::FILEFORMATINFO result = {};

    QList<XArchive::RECORD> listArchiveRecords = getRecords(20000, pPdStruct);

    if (isValid(&listArchiveRecords, pPdStruct)) {
        result.bIsValid = true;
        result.nSize = getFileFormatSize(pPdStruct);
        result.sExt = "apk";
        result.fileType = FT_APK;

        result.osName = OSNAME_ANDROID;

        result.sArch = getArch();
        result.mode = getMode();
        result.sType = typeIdToString(getType());
        result.endian = getEndian();
#ifdef USE_DEX
        QByteArray baAndroidManifest = decompress(&listArchiveRecords, "AndroidManifest.xml", pPdStruct);

        if (baAndroidManifest.size() > 0) {
            QString sAndroidManifest = XAndroidBinary::getDecoded(&baAndroidManifest, pPdStruct);

            QString sCompileSdkVersion = XBinary::regExp("android:compileSdkVersion=\"(.*?)\"", sAndroidManifest, 1);
            QString sCompileSdkVersionCodename = XBinary::regExp("android:compileSdkVersionCodename=\"(.*?)\"", sAndroidManifest, 1);
            QString sPlatformBuildVersionCode = XBinary::regExp("platformBuildVersionCode=\"(.*?)\"", sAndroidManifest, 1);
            QString sPlatformBuildVersionName = XBinary::regExp("platformBuildVersionName=\"(.*?)\"", sAndroidManifest, 1);
            QString sTargetSdkVersion = XBinary::regExp("android:targetSdkVersion=\"(.*?)\"", sAndroidManifest, 1);
            QString sMinSdkVersion = XBinary::regExp("android:minSdkVersion=\"(.*?)\"", sAndroidManifest, 1);

            // Check
            if (!XBinary::checkStringNumber(sCompileSdkVersion, 1, 40)) sCompileSdkVersion = "";
            if (!XBinary::checkStringNumber(sPlatformBuildVersionCode, 1, 40)) sPlatformBuildVersionCode = "";
            if (!XBinary::checkStringNumber(sTargetSdkVersion, 1, 40)) sTargetSdkVersion = "";
            if (!XBinary::checkStringNumber(sMinSdkVersion, 1, 40)) sMinSdkVersion = "";

            if (!XBinary::checkStringNumber(sCompileSdkVersionCodename.section(".", 0, 0), 1, 15)) sCompileSdkVersionCodename = "";
            if (!XBinary::checkStringNumber(sPlatformBuildVersionName.section(".", 0, 0), 1, 15)) sPlatformBuildVersionName = "";

            if ((sCompileSdkVersion != "") || (sCompileSdkVersionCodename != "") || (sPlatformBuildVersionCode != "") || (sPlatformBuildVersionName != "") ||
                (sTargetSdkVersion != "") || (sMinSdkVersion != "")) {
                QString _sVersion;
                QString _sAndroidVersion;

                if (_sVersion == "") _sVersion = sTargetSdkVersion;
                if (_sVersion == "") _sVersion = sMinSdkVersion;
                if (_sVersion == "") _sVersion = sCompileSdkVersion;
                if (_sVersion == "") _sVersion = sPlatformBuildVersionCode;

                if (_sAndroidVersion == "") _sAndroidVersion = sCompileSdkVersionCodename;
                if (_sAndroidVersion == "") _sAndroidVersion = sPlatformBuildVersionName;

                if (_sAndroidVersion == "") {
                    _sAndroidVersion = XBinary::getAndroidVersionFromApi(_sVersion.toUInt());
                }

                result.sOsVersion = _sAndroidVersion;
                result.sOsBuild = sPlatformBuildVersionCode;
            }
        }
#endif
        qint32 nNumberOfRecords = listArchiveRecords.count();

        if (nNumberOfRecords < 20000) {
            result.nNumberOfRecords = nNumberOfRecords;
        } else {
            result.nNumberOfRecords = getNumberOfRecords(pPdStruct);
        }
    }

    return result;
}

QString XAPK::getFileFormatExt()
{
    return "apk";
}

XBinary::MODE XAPK::getMode()
{
    return MODE_DATA;
}

QString XAPK::getArch()
{
    return tr("Universal");
}

qint32 XAPK::getType()
{
    return TYPE_PACKAGE;
}

QString XAPK::typeIdToString(qint32 nType)
{
    QString sResult = tr("Unknown");

    switch (nType) {
        case TYPE_PACKAGE: sResult = tr("Package");
    }

    return sResult;
}

bool XAPK::isSigned()
{
    // TODO Check more !!!
    return isAPKSignBlockPresent();
}

XBinary::OFFSETSIZE XAPK::getSignOffsetSize()
{
    OFFSETSIZE osResult = {};

    // TODO optimize

    qint64 nOffset = findAPKSignBlockOffset();

    quint64 nBlockSize1 = read_uint64(nOffset - 8);
    quint64 nBlockSize2 = read_uint64(nOffset - nBlockSize1 + 8);

    if ((nBlockSize1) && (nBlockSize1 == nBlockSize2)) {
        nOffset = nOffset - nBlockSize1 + 16;

        qint64 nCentralDirectoryOffset = findECDOffset(nullptr);
        nCentralDirectoryOffset = read_uint32(nCentralDirectoryOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

        osResult.nOffset = nOffset;
        osResult.nSize = qMax((qint64)0, nCentralDirectoryOffset - nOffset);
    }

    return osResult;
}

bool XAPK::isAPKSignBlockPresent()
{
    return (findAPKSignBlockOffset() != -1);
}

QList<XAPK::APK_SIG_BLOCK_RECORD> XAPK::getAPKSignaturesBlockRecordsList()
{
    QList<XAPK::APK_SIG_BLOCK_RECORD> listResult;

    qint64 nOffset = findAPKSignBlockOffset();

    if (nOffset != -1) {
        quint64 nBlockSize1 = read_uint64(nOffset - 8);
        quint64 nBlockSize2 = read_uint64(nOffset - nBlockSize1 + 8);

        if ((nBlockSize1) && (nBlockSize1 == nBlockSize2)) {
            qint64 nEndOffset = nOffset - 8;
            nOffset = nOffset - nBlockSize1 + 16;

            while (nOffset < nEndOffset) {
                APK_SIG_BLOCK_RECORD record = {};
                record.nID = read_uint32(nOffset);
                nOffset += 4;

                record.nDataOffset = nOffset + 4;
                record.nDataSize = read_uint32(nOffset);

                listResult.append(record);

                nOffset += 4;
                nOffset += record.nDataSize;

                if (record.nID == 0x42726577)  // End TODO CONST
                {
                    break;
                }

                if (record.nID == 0) {
                    break;
                }
            }
        }
    }

    return listResult;
}

bool XAPK::isAPKSignatureBlockRecordPresent(QList<APK_SIG_BLOCK_RECORD> *pList, quint32 nID)
{
    return (getAPKSignatureBlockRecord(pList, nID).nID == nID);
}

XAPK::APK_SIG_BLOCK_RECORD XAPK::getAPKSignatureBlockRecord(QList<APK_SIG_BLOCK_RECORD> *pList, quint32 nID)
{
    XAPK::APK_SIG_BLOCK_RECORD result = {};

    qint32 nNumberOfRecords = pList->count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        if (pList->at(i).nID == nID) {
            result = pList->at(i);

            break;
        }
    }

    return result;
}

qint64 XAPK::findAPKSignBlockOffset(PDSTRUCT *pPdStruct)
{
    qint64 nResult = -1;

    qint64 nOffset = findECDOffset(pPdStruct);
    nOffset = read_uint32(nOffset + offsetof(ENDOFCENTRALDIRECTORYRECORD, nOffsetToCentralDirectory));

    nOffset = qMax((qint64)0, nOffset - 0x100);  // TODO const

    while (true) {
        qint64 nCurrent = find_ansiString(nOffset, -1, "APK Sig Block 42", pPdStruct);

        if (nCurrent == -1) {
            break;
        }

        nResult = nCurrent;
        nOffset = nCurrent + 8;  // Get the last
    }

    return nResult;
}
