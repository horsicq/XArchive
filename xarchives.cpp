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

#include <QTemporaryDir>

#include "xfilteredarchive.h"

namespace {

// Automatic nested-filter probing materializes the decoded stream.  Keep that
// decision bounded on 32-bit builds; larger raw streams remain fully available
// through the directly compiled ip7z source handlers instead of being decoded several
// times merely to discover whether their payload is another archive.
const qint64 NATIVE_FILTER_AUTO_PROBE_MAX_INPUT_SIZE = 256LL * 1024 * 1024;

bool ip7zAllowsNativeFallback(const QString &sError)
{
    // Password, CRC, corruption, cancellation, and unsafe-path failures are
    // authoritative. Only a structured unsupported-format result may fall
    // back to the native XArchive implementation.
    return XArchives::isIp7zUnsupportedFormatError(sError);
}

bool hasLegacyZstdMagic(QIODevice *pDevice)
{
    if (!pDevice || !pDevice->isOpen() || !pDevice->isReadable()) return false;

    const QByteArray baMagic = pDevice->peek(4);
    if (baMagic.size() != 4) return false;

    const unsigned char *p = reinterpret_cast<const unsigned char *>(baMagic.constData());
    const quint32 nMagic = static_cast<quint32>(p[0]) |
                           (static_cast<quint32>(p[1]) << 8) |
                           (static_cast<quint32>(p[2]) << 16) |
                           (static_cast<quint32>(p[3]) << 24);
    return (nMagic >= 0xFD2FB524U) && (nMagic <= 0xFD2FB527U);
}

void setOperationError(XBinary::PDSTRUCT *pPdStruct, const QString &sError)
{
    if (pPdStruct && XBinary::getPdStructErrorString(pPdStruct).isEmpty() && !sError.isEmpty()) {
        XBinary::setPdStructErrorString(pPdStruct, sError);
    }
}

}  // namespace

XArchives::XArchives(QObject *pParent) : QObject(pParent)
{
}

QList<XArchive::RECORD> XArchives::getRecords(QIODevice *pDevice, XBinary::FT fileType, qint32 nLimit, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);
    
    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        listResult = pArchives->getRecords(nLimit, pPdStruct);
    } else {
        if (pBinary) {
            delete pBinary; 
        }

        pBinary = XFormats::createClass(XBinary::FT_ZIP, pDevice);
        pArchives = dynamic_cast<XArchive *>(pBinary);
        
        if (pArchives) {
            listResult = pArchives->getRecords(nLimit, pPdStruct);
        }
    }

    if (pBinary) {
        delete pBinary;
    }

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

    XBinary::FT fileType = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES);

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);
    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        baResult = pArchives->decompress(pRecord, pPdStruct, nDecompressedOffset, nDecompressedSize);
    }

    delete pBinary;

    return baResult;
}

QByteArray XArchives::decompress(const QString &sFileName, const XArchive::RECORD *pRecord, XBinary::PDSTRUCT *pPdStruct, qint64 nDecompressedOffset,
                                 qint64 nDecompressedSize)
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

    XBinary::FT fileType = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES);

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);
    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        bResult = pArchives->decompressToFile(pRecord, sResultFileName, pPdStruct);
    }

    delete pBinary;

    return bResult;
}

bool XArchives::decompressToDevice(QIODevice *pDevice, XArchive::RECORD *pRecord, QIODevice *pDestDevice, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::FT fileType = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES);

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);
    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        bResult = pArchives->decompressToDevice(pRecord, pDestDevice, pPdStruct);
    }

    delete pBinary;

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
    return decompressToFolder(pDevice, sResultFileFolder, QMap<XBinary::UNPACK_PROP, QVariant>(), pPdStruct);
}

bool XArchives::decompressToFolder(QIODevice *pDevice, const QString &sResultFileFolder,
                                   const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice) return false;

    XBinary::PDSTRUCT pdStructEmpty = {};
    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const XBinary::FT fileType = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES);
    XBinary *pBinary = XFormats::createClass(fileType, pDevice);
    XArchive *pArchive = dynamic_cast<XArchive *>(pBinary);
    if (!pArchive) {
        delete pBinary;
        return false;
    }

    // XArchive has a legacy overload with the same name which hides the
    // property-aware implementation inherited from XBinary.  Call the base
    // implementation explicitly so passwords and the other unpack options are
    // preserved for the complete streaming operation.
    bool bResult = pArchive->XBinary::unpackToFolder(sResultFileFolder, mapProperties, pPdStruct);

    // Keep compatibility with formats that only implement the legacy RECORD
    // API.  Property-aware streaming is always attempted first; the legacy
    // path is reached only when it fails without cancellation.
    //
    // The legacy path re-extracts the WHOLE archive from raw (offset,size)
    // pairs, so running it after a partially completed streaming attempt both
    // swallows the streaming diagnostic and rewrites members that were already
    // produced correctly.  Restrict it to formats that do not implement the
    // streaming API at all: for every other format a streaming failure is
    // authoritative and must surface as an error rather than as a second,
    // weaker extraction attempt.
    if (!bResult && XBinary::isPdStructNotCanceled(pPdStruct)) {
        XBinary::UNPACK_STATE probeState = {};
        const bool bStreamingImplemented =
            pArchive->initUnpack(&probeState, mapProperties, pPdStruct);
        pArchive->finishUnpack(&probeState, nullptr);

        if (!bStreamingImplemented &&
            XBinary::isPdStructNotCanceled(pPdStruct)) {
            QList<XArchive::RECORD> listRecords = pArchive->getRecords(-1, pPdStruct);
            bResult = !listRecords.isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct) &&
                      pArchive->decompressToPath(&listRecords, QString(), sResultFileFolder, pPdStruct);
        }
    }

    delete pBinary;

    return bResult;
}

bool XArchives::decompressToFolder(const QString &sFileName, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct)
{
    return decompressToFolder(sFileName, sResultFileFolder, QMap<XBinary::UNPACK_PROP, QVariant>(), pPdStruct);
}

bool XArchives::decompressToFolder(const QString &sFileName, const QString &sResultFileFolder,
                                   const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;
    QString sIp7zError;
    const QString sPassword = mapProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
    const XBinary::FT fileType = XFormats::getPrefFileType(sFileName, XBinary::FT_FLAG_ARCHIVES, pPdStruct);
    QFile nativeProbe(sFileName);
    const bool bProbeOpened = nativeProbe.open(QIODevice::ReadOnly);
    const bool bPreferNative = bProbeOpened &&
        isNativeReaderPreferredFileType(fileType, &nativeProbe, pPdStruct);
    if (bProbeOpened) nativeProbe.close();

    if (!bPreferNative && isIp7zSourceAvailable()) {
        QList<XBinary::ARCHIVERECORD> listProbe;
        if (listArchiveWithIp7zSource(sFileName, sPassword, &listProbe, &sIp7zError, pPdStruct)) {
            bResult = extractArchiveWithIp7zSource(sFileName, sPassword, sResultFileFolder, &sIp7zError, pPdStruct);
            if (bResult) return true;

            // Extraction is staged by the source-built ip7z bridge, so an
            // unsupported coder has committed no destination entries.  Retry
            // that narrowly classified case through the native XArchive
            // reader; password, CRC, corruption, cancellation and path-safety
            // errors remain authoritative and never fall back.
            if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
                !ip7zAllowsNativeFallback(sIp7zError)) {
                setOperationError(pPdStruct, sIp7zError);
                return false;
            }
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct) || !ip7zAllowsNativeFallback(sIp7zError)) {
            setOperationError(pPdStruct, sIp7zError);
            return false;
        }
    }

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        bResult = decompressToFolder(&file, sResultFileFolder, mapProperties, pPdStruct);
        file.close();
    }

    if (!bResult) {
        setOperationError(pPdStruct, sIp7zError);
    }

    return bResult;
}

bool XArchives::testArchive(const QString &sFileName, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, XBinary::PDSTRUCT *pPdStruct)
{
    QString sIp7zError;
    const QString sPassword = mapProperties.value(XBinary::UNPACK_PROP_PASSWORD).toString();
    const XBinary::FT fileType = XFormats::getPrefFileType(sFileName, XBinary::FT_FLAG_ARCHIVES, pPdStruct);
    QFile nativeProbe(sFileName);
    const bool bProbeOpened = nativeProbe.open(QIODevice::ReadOnly);
    const bool bPreferNative = bProbeOpened &&
        isNativeReaderPreferredFileType(fileType, &nativeProbe, pPdStruct);
    if (bProbeOpened) nativeProbe.close();

    if (!bPreferNative && isIp7zSourceAvailable()) {
        QList<XBinary::ARCHIVERECORD> listProbe;
        if (listArchiveWithIp7zSource(sFileName, sPassword, &listProbe, &sIp7zError, pPdStruct)) {
            const bool bResult = testArchiveWithIp7zSource(sFileName, sPassword, &sIp7zError, pPdStruct);
            if (bResult) return true;
            if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
                !ip7zAllowsNativeFallback(sIp7zError)) {
                setOperationError(pPdStruct, sIp7zError);
                return false;
            }
        }

        if (!XBinary::isPdStructNotCanceled(pPdStruct) || !ip7zAllowsNativeFallback(sIp7zError)) {
            setOperationError(pPdStruct, sIp7zError);
            return false;
        }
    }

    QTemporaryDir temporaryDir;
    bool bResult = false;

    if (temporaryDir.isValid()) {
        QFile file(sFileName);

        if (file.open(QIODevice::ReadOnly)) {
            bResult = decompressToFolder(&file, temporaryDir.path(), mapProperties, pPdStruct);
            file.close();
        }
    } else if (pPdStruct) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Cannot create temporary directory"));
    }

    if (!bResult) {
        setOperationError(pPdStruct, sIp7zError);
    }

    return bResult;
}

bool XArchives::isArchiveRecordPresent(QIODevice *pDevice, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct)
{
    bool bResult = false;

    XBinary::FT fileType = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES);

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);
    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        bResult = pArchives->isArchiveRecordPresent(sRecordFileName, pPdStruct);
    }

    delete pBinary;

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
        QSet<XBinary::FT> stFT = XFormats::getFileTypes(pDevice, XBinary::FT_FLAG_ARCHIVES);

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

bool XArchives::isNativeReaderPreferredFileType(XBinary::FT fileType,
                                                QIODevice *pDevice,
                                                XBinary::PDSTRUCT *pPdStruct)
{
    switch (fileType) {
        case XBinary::FT_TAR_GZ:
        case XBinary::FT_TAR_BZIP2:
        case XBinary::FT_TAR_LZIP:
        case XBinary::FT_TAR_LZMA:
        case XBinary::FT_TAR_LZOP:
        case XBinary::FT_TAR_XZ:
        case XBinary::FT_TAR_Z:
        case XBinary::FT_TAR_ZSTD:
        case XBinary::FT_TAR_LZ4:
        case XBinary::FT_BROTLI:
        case XBinary::FT_LZ4:
        case XBinary::FT_LZ5:
        case XBinary::FT_LIZARD:
        case XBinary::FT_WARC:
        case XBinary::FT_MTREE:
        case XBinary::FT_UU: return true;
        // The source-built ip7z handler is substantially faster for very large
        // current-format Zstandard streams. A leading legacy v0.4-v0.7 frame
        // must stay on the native adapter path; bounded current streams fall
        // through to the generic native-filter probe below.
        case XBinary::FT_ZSTD:
            if (hasLegacyZstdMagic(pDevice)) return true;
            break;
        default: break;
    }

    if (!pDevice || !XFilteredArchive::isFilterFileType(fileType) ||
        !pDevice->isOpen() || !pDevice->isReadable() || pDevice->isSequential()) {
        return false;
    }

    const qint64 nSize = pDevice->size();
    if ((nSize < 0) || (nSize > NATIVE_FILTER_AUTO_PROBE_MAX_INPUT_SIZE)) {
        return false;
    }

    const QString sOriginalError = pPdStruct
        ? XBinary::getPdStructErrorString(pPdStruct)
        : QString();
    const bool bResult = XFilteredArchive::isValid(pDevice, fileType, pPdStruct);
    if (!bResult && pPdStruct && XBinary::isPdStructNotCanceled(pPdStruct)) {
        XBinary::setPdStructErrorString(pPdStruct, sOriginalError);
    }
    return bResult;
}

QSet<XBinary::FT> XArchives::getArchiveOpenValidFileTypes()
{
    QSet<XBinary::FT> result;

    result.insert(XBinary::FT_ZIP);
    result.insert(XBinary::FT_JAR);
    result.insert(XBinary::FT_APK);
    result.insert(XBinary::FT_IPA);
    result.insert(XBinary::FT_APKS);
    result.insert(XBinary::FT_7Z);
    result.insert(XBinary::FT_WIM);
    result.insert(XBinary::FT_CAB);
    result.insert(XBinary::FT_RAR);
    result.insert(XBinary::FT_MACHOFAT);
    result.insert(XBinary::FT_AR);
    result.insert(XBinary::FT_DEB);
    result.insert(XBinary::FT_TAR);
    result.insert(XBinary::FT_TAR_GZ);
    result.insert(XBinary::FT_TAR_BZIP2);
    result.insert(XBinary::FT_TAR_LZIP);
    result.insert(XBinary::FT_TAR_LZMA);
    result.insert(XBinary::FT_TAR_LZOP);
    result.insert(XBinary::FT_TAR_XZ);
    result.insert(XBinary::FT_TAR_Z);
    result.insert(XBinary::FT_TAR_ZSTD);
    result.insert(XBinary::FT_TAR_LZ4);
    result.insert(XBinary::FT_NPM);
    result.insert(XBinary::FT_GZIP);
    result.insert(XBinary::FT_BZIP2);
    result.insert(XBinary::FT_BROTLI);
    result.insert(XBinary::FT_LZ4);
    result.insert(XBinary::FT_LZ5);
    result.insert(XBinary::FT_LIZARD);
    result.insert(XBinary::FT_LZMA);
    result.insert(XBinary::FT_LZO);
    result.insert(XBinary::FT_COMPRESS);
    result.insert(XBinary::FT_ZSTD);
    result.insert(XBinary::FT_ZLIB);
    result.insert(XBinary::FT_LHA);
    result.insert(XBinary::FT_ARJ);
    result.insert(XBinary::FT_ACE);
    result.insert(XBinary::FT_ARC);
    result.insert(XBinary::FT_FREEARC);
    result.insert(XBinary::FT_CFBF);
    result.insert(XBinary::FT_SZDD);
    result.insert(XBinary::FT_LZIP);
    result.insert(XBinary::FT_XZ);
    result.insert(XBinary::FT_CPIO);
    result.insert(XBinary::FT_SQUASHFS);
    result.insert(XBinary::FT_ISO9660);
    result.insert(XBinary::FT_UDF);
    result.insert(XBinary::FT_DMG);
    result.insert(XBinary::FT_MINIDUMP);
    result.insert(XBinary::FT_RPM);
    result.insert(XBinary::FT_KWAJ);
    result.insert(XBinary::FT_ASAR);
    result.insert(XBinary::FT_XAR);
    result.insert(XBinary::FT_ZOO);
    result.insert(XBinary::FT_WARC);
    result.insert(XBinary::FT_MTREE);
    result.insert(XBinary::FT_UU);
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
