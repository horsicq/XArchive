/* Copyright (c) 2020-2026 hors<horsicq@gmail.com>
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

XArchive *XArchives::getClass(XBinary::FT fileType, QIODevice *pDevice)
{
    XArchive *pResult = nullptr;

    QSet<XBinary::FT> stFileTypes;

    if (fileType == XBinary::FT_UNKNOWN) {
        stFileTypes = XBinary::getFileTypes(pDevice, true);
    } else {
        stFileTypes += fileType;
    }

    if (stFileTypes.contains(XArchive::FT_ZIP)) {
        pResult = new XZip(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_JAR)) {
        pResult = new XJAR(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_APK)) {
        pResult = new XAPK(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_IPA)) {
        pResult = new XIPA(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_APKS)) {
        pResult = new XAPKS(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_7Z)) {
        pResult = new XSevenZip(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_CAB)) {
        pResult = new XCab(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_RAR)) {
        pResult = new XRar(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_MACHOFAT)) {
        pResult = new XMACHOFat(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_AR)) {
        pResult = new X_Ar(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_DEB)) {
        pResult = new XDEB(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_TAR)) {
        pResult = new XTAR(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_NPM)) {
        pResult = new XNPM(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_TARGZ)) {
        pResult = new XTGZ(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_GZIP)) {
        pResult = new XGzip(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_ZLIB)) {
        pResult = new XZlib(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_LHA)) {
        pResult = new XLHA(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_CFBF)) {
        pResult = new XCFBF(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_SZDD)) {
        pResult = new XSZDD(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_BZIP2)) {
        pResult = new XBZIP2(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_XZ)) {
        pResult = new XXZ(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_CPIO)) {
        pResult = new XCPIO(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_SQUASHFS)) {
        pResult = new XSquashfs(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_ISO9660)) {
        pResult = new XISO9660(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_AR)) {
        pResult = new X_Ar(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_MINIDUMP)) {
        pResult = new XMiniDump(pDevice);
    } else if (stFileTypes.contains(XArchive::FT_DOS4G) || stFileTypes.contains(XArchive::FT_DOS16M)) {
        pResult = new XDOS16(pDevice);
    } else {
#ifdef QT_DEBUG
        qDebug("XArchives::getClass: Unknown file type");
#endif
    }

    return pResult;
}

QList<XArchive::RECORD> XArchives::getRecords(QIODevice *pDevice, XBinary::FT fileType, qint32 nLimit, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    XArchive *pArchives = XArchives::getClass(fileType, pDevice);

    if (pArchives) {
        listResult = pArchives->getRecords(nLimit, pPdStruct);
    }

    delete pArchives;

    return listResult;
}

QList<XArchive::RECORD> XArchives::getRecords(const QString &sFileName, XBinary::FT fileType, qint32 nLimit, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        listResult = getRecords(&file, fileType, nLimit, pPdStruct);

        file.close();
    }

    return listResult;
}

QList<XArchive::RECORD> XArchives::getRecordsFromDirectory(const QString &sDirectoryName, qint32 nLimit, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    _findFiles(sDirectoryName, &listResult, nLimit, pPdStruct);

    return listResult;
}

QByteArray XArchives::decompress(QIODevice *pDevice, const XArchive::RECORD *pRecord, XBinary::PDSTRUCT *pPdStruct, qint64 nDecompressedOffset, qint64 nDecompressedSize)
{
    QByteArray baResult;

    XBinary::FT fileType = XBinary::getPrefFileType(pDevice, true);

    XArchive *pArchives = XArchives::getClass(fileType, pDevice);

    if (pArchives) {
        baResult = pArchives->decompress(pRecord, pPdStruct, nDecompressedOffset, nDecompressedSize);
    }

    delete pArchives;

    return baResult;
}

QByteArray XArchives::decompress(const QString &sFileName, XArchive::RECORD *pRecord, XBinary::PDSTRUCT *pPdStruct, qint64 nDecompressedOffset, qint64 nDecompressedSize)
{
    QByteArray baResult;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        baResult = decompress(&file, pRecord, pPdStruct, nDecompressedOffset, nDecompressedSize);
        file.close();
    }

    return baResult;
}

QByteArray XArchives::decompress(QIODevice *pDevice, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listRecords = getRecords(pDevice);

    XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &listRecords);

    return decompress(pDevice, &record, pPdStruct);
}

QByteArray XArchives::decompress(const QString &sFileName, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct)
{
    QByteArray baResult;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        baResult = decompress(&file, sRecordFileName, pPdStruct);
        file.close();
    }

    return baResult;
}

bool XArchives::decompressToFile(QIODevice *pDevice, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::FT fileType = XBinary::getPrefFileType(pDevice, true);

    XArchive *pArchives = XArchives::getClass(fileType, pDevice);

    if (pArchives) {
        bResult = pArchives->decompressToFile(pRecord, sResultFileName, pPdStruct);
    }

    delete pArchives;

    return bResult;
}

bool XArchives::decompressToDevice(QIODevice *pDevice, XArchive::RECORD *pRecord, QIODevice *pDestDevice, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::FT fileType = XBinary::getPrefFileType(pDevice, true);

    XArchive *pArchives = XArchives::getClass(fileType, pDevice);

    if (pArchives) {
        bResult = pArchives->decompressToDevice(pRecord, pDestDevice, pPdStruct);
    }

    delete pArchives;

    return bResult;
}

bool XArchives::decompressToFile(const QString &sFileName, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        bResult = decompressToFile(&file, pRecord, sResultFileName, pPdStruct);

        file.close();
    }

    return bResult;
}

bool XArchives::decompressToFile(const QString &sFileName, const QString &sRecordFileName, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        QList<XArchive::RECORD> listRecords = getRecords(&file, XBinary::FT_UNKNOWN, -1, pPdStruct);  // TODO FT

        XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &listRecords, pPdStruct);

        if (record.spInfo.sRecordName != "") {
            bResult = decompressToFile(&file, &record, sResultFileName, pPdStruct);
        }

        file.close();
    }

    return bResult;
}

bool XArchives::decompressToFolder(QIODevice *pDevice, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::PDSTRUCT pdStructEmpty = {};

    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }

    QList<XArchive::RECORD> listRecords = getRecords(pDevice, XBinary::FT_UNKNOWN, -1, pPdStruct);

    qint32 nNumberOfRecords = listRecords.count();

    for (qint32 i = 0; (i < nNumberOfRecords) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
        XArchive::RECORD record = listRecords.at(i);
        QString sResultFileName = sResultFileFolder + QDir::separator() + record.spInfo.sRecordName;

        bResult = decompressToFile(pDevice, &record, sResultFileName, pPdStruct);

        if (!bResult) {
            //            break;
        }
    }

    return bResult;
}

bool XArchives::decompressToFolder(const QString &sFileName, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        bResult = decompressToFolder(&file, sResultFileFolder, pPdStruct);
        file.close();
    }

    return bResult;
}

bool XArchives::isArchiveRecordPresent(QIODevice *pDevice, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::FT fileType = XBinary::getPrefFileType(pDevice, true);

    XArchive *pArchives = XArchives::getClass(fileType, pDevice);

    if (pArchives) {
        bResult = pArchives->isArchiveRecordPresent(sRecordFileName, pPdStruct);
    }

    delete pArchives;

    return bResult;
}

bool XArchives::isArchiveRecordPresent(const QString &sFileName, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        bResult = isArchiveRecordPresent(&file, sRecordFileName, pPdStruct);
        file.close();
    }

    return bResult;
}

bool XArchives::isArchiveOpenValid(QIODevice *pDevice, const QSet<XBinary::FT> &stAvailable)
{
    bool bResult = false;

    QSet<XBinary::FT> _stAvailable = stAvailable;

    if (pDevice) {
        QSet<XBinary::FT> stFT = XBinary::getFileTypes(pDevice, true);

        if (!_stAvailable.count()) {
            _stAvailable = getArchiveOpenValidFileTypes();
        }

        bResult = XBinary::isFileTypePresent(&stFT, &_stAvailable);
    }

    return bResult;
}

bool XArchives::isArchiveOpenValid(const QString &sFileName, const QSet<XBinary::FT> &stAvailable)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        bResult = isArchiveOpenValid(&file, stAvailable);
        file.close();
    }

    return bResult;
}

QSet<XBinary::FT> XArchives::getArchiveOpenValidFileTypes()
{
    QSet<XBinary::FT> result;

    result.insert(XBinary::FT_ZIP);
    result.insert(XBinary::FT_JAR);
    result.insert(XBinary::FT_APK);
    result.insert(XBinary::FT_MACHOFAT);
    result.insert(XBinary::FT_AR);
    result.insert(XBinary::FT_DEB);
    result.insert(XBinary::FT_TAR);
    result.insert(XBinary::FT_TARGZ);
    result.insert(XBinary::FT_NPM);
    result.insert(XBinary::FT_GZIP);
    result.insert(XBinary::FT_BZIP2);
    result.insert(XBinary::FT_ZLIB);
    result.insert(XBinary::FT_LHA);
    result.insert(XBinary::FT_SZDD);
    result.insert(XBinary::FT_DOS4G);
    result.insert(XBinary::FT_DOS16M);

    return result;
}

void XArchives::_findFiles(const QString &sDirectoryName, QList<XArchive::RECORD> *pListRecords, qint32 nLimit, XBinary::PDSTRUCT *pPdStruct)
{
    if (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if ((nLimit < pListRecords->count()) || (nLimit == -1)) {
            QFileInfo fi(sDirectoryName);

            if (fi.isFile()) {
                XArchive::RECORD record = {};

                record.spInfo.compressMethod = XArchive::HANDLE_METHOD_FILE;
                record.spInfo.sRecordName = fi.absoluteFilePath();
                record.nDataSize = fi.size();
                record.spInfo.nUncompressedSize = fi.size();

                if ((nLimit < pListRecords->count()) || (nLimit == -1)) {
                    pListRecords->append(record);
                }
            } else if (fi.isDir()) {
                QDir dir(sDirectoryName);

                QFileInfoList eil = dir.entryInfoList();

                qint32 nNumberOfFiles = eil.count();

                for (qint32 i = 0; (i < nNumberOfFiles) && XBinary::isPdStructNotCanceled(pPdStruct); i++) {
                    QString sFN = eil.at(i).fileName();

                    if ((sFN != ".") && (sFN != "..")) {
                        _findFiles(eil.at(i).absoluteFilePath(), pListRecords, nLimit, pPdStruct);
                    }
                }
            }
        }
    }
}
