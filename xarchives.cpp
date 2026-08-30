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

#include <QBuffer>
#include <QFileInfo>
#include <QSaveFile>
#include <QTemporaryDir>

namespace {

XBinary::FT preferredUnpackerFileType(QIODevice *pDevice, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice) return XBinary::FT_UNKNOWN;

    // Installer/packer formats are also executables.  Probe their dedicated
    // readers before generic archive readers can reinterpret the same PE as raw
    // sections or an overlay container.
    const XBinary::FT staticType = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_EXECUTABLES | XBinary::FT_FLAG_STATICUNPACKERS, pPdStruct);
    if (XFormats::isStaticUnpacker(staticType)) return staticType;

    // MSI/WiX static unpackers are CFBF containers, so their dedicated probes
    // become available only when archive detection has first identified the
    // compound-file base type.  Keep STATICUNPACKERS enabled for this pass as
    // well; otherwise FT_UNKNOWN silently falls back to the raw CFBF archive.
    const XBinary::FT archiveType = XFormats::getPrefFileType(pDevice, XBinary::FT_FLAG_ARCHIVES | XBinary::FT_FLAG_STATICUNPACKERS, pPdStruct);
    return archiveType;
}

bool hasAuthoritativeStreamingReader(XBinary::FT fileType)
{
    switch (fileType) {
        case XBinary::FT_ZPAQ:
        case XBinary::FT_BCM:
        case XBinary::FT_LPAQ8:
        case XBinary::FT_PEA:
        case XBinary::FT_FREEARC:
        case XBinary::FT_CKP:
        case XBinary::FT_EDP:
        case XBinary::FT_MPQ:
        case XBinary::FT_BIGF:
        case XBinary::FT_RIB:
        case XBinary::FT_PARSEC_ARCHIVE:
        case XBinary::FT_PMM: return true;
        default: return false;
    }
}

XArchive::RECORD legacyRecordFromArchiveRecord(const XBinary::ARCHIVERECORD &archiveRecord)
{
    XArchive::RECORD record = {};

    record.nDataOffset = archiveRecord.nStreamOffset;
    record.nDataSize = archiveRecord.nStreamSize;
    record.mapProperties = archiveRecord.mapProperties;
    record.spInfo.nUncompressedSize = archiveRecord.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    record.spInfo.sRecordName = archiveRecord.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
    record.spInfo.compressMethod = (XBinary::HANDLE_METHOD)archiveRecord.mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD, XBinary::HANDLE_METHOD_UNKNOWN).toInt();
    record.spInfo.compressMethod2 = (XBinary::HANDLE_METHOD)archiveRecord.mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD2, XBinary::HANDLE_METHOD_UNKNOWN).toInt();
    record.spInfo.nCRC32 = archiveRecord.mapProperties.value(XBinary::FPART_PROP_RESULTCRC).toUInt();
    record.spInfo.nWindowSize = archiveRecord.mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE).toULongLong();
    record.spInfo.bIsSolid = archiveRecord.mapProperties.value(XBinary::FPART_PROP_ISSOLID).toBool();
    record.nHeaderOffset = archiveRecord.mapProperties.value(XBinary::FPART_PROP_HEADER_OFFSET).toLongLong();
    record.nHeaderSize = archiveRecord.mapProperties.value(XBinary::FPART_PROP_HEADER_SIZE).toLongLong();
    record.nOptHeaderOffset = archiveRecord.mapProperties.value(XBinary::FPART_PROP_OPTHEADER_OFFSET).toLongLong();
    record.nOptHeaderSize = archiveRecord.mapProperties.value(XBinary::FPART_PROP_OPTHEADER_SIZE).toLongLong();
    record.sUUID = XBinary::generateUUID();

    return record;
}

XBinary::ARCHIVERECORD archiveRecordFromLegacyRecord(const XArchive::RECORD &record)
{
    XBinary::ARCHIVERECORD result = {};
    result.nStreamOffset = record.nDataOffset;
    result.nStreamSize = record.nDataSize;
    result.mapProperties = record.mapProperties;
    return result;
}

bool resolveStaticRecord(XBinary *pBinary, const XArchive::RECORD *pRecord, qint32 *pnRecordIndex, XBinary::ARCHIVERECORD *pArchiveRecord, XBinary::PDSTRUCT *pPdStruct)
{
    if (pnRecordIndex) *pnRecordIndex = -1;
    if (!pBinary || !pRecord || !pnRecordIndex || !pArchiveRecord || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const XBinary::ARCHIVERECORD expectedRecord = archiveRecordFromLegacyRecord(*pRecord);
    const QList<XBinary::ARCHIVERECORD> listRecords = pBinary->getArchiveRecords(-1, pPdStruct);

    for (qint32 i = 0; (i < listRecords.count()) && XBinary::isPdStructNotCanceled(pPdStruct); ++i) {
        if (XBinary::isSameArchiveRecordIdentity(listRecords.at(i), expectedRecord)) {
            *pnRecordIndex = i;
            *pArchiveRecord = listRecords.at(i);
            return true;
        }
    }

    return false;
}

bool unpackStaticRecord(XBinary *pBinary, const XArchive::RECORD *pRecord, QIODevice *pDestDevice, XBinary::PDSTRUCT *pPdStruct,
                        const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    qint32 nRecordIndex = -1;
    XBinary::ARCHIVERECORD archiveRecord = {};
    if (!resolveStaticRecord(pBinary, pRecord, &nRecordIndex, &archiveRecord, pPdStruct)) {
        return false;
    }

    return pBinary->unpackRecordByIndex(nRecordIndex, &archiveRecord, pDestDevice, mapProperties, pPdStruct);
}

}  // namespace

XArchives::XArchives(QObject *pParent) : QObject(pParent)
{
}

QList<XArchive::RECORD> XArchives::getRecords(QIODevice *pDevice, XBinary::FT fileType, qint32 nLimit, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listResult;

    if (!pDevice || (nLimit == 0) || (nLimit < -1) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return listResult;
    }

    if (fileType == XBinary::FT_UNKNOWN) {
        fileType = preferredUnpackerFileType(pDevice, pPdStruct);
    }

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);

    if (pBinary && XFormats::isStaticUnpacker(fileType)) {
        const QList<XBinary::ARCHIVERECORD> listArchiveRecords = pBinary->getArchiveRecords(nLimit, pPdStruct);
        listResult.reserve(listArchiveRecords.count());
        for (const XBinary::ARCHIVERECORD &archiveRecord : listArchiveRecords) {
            listResult.append(legacyRecordFromArchiveRecord(archiveRecord));
        }
        delete pBinary;
        return listResult;
    }

    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        listResult = pArchives->getRecords(nLimit, pPdStruct);
    } else {
        delete pBinary;

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

QByteArray XArchives::decompress(QIODevice *pDevice, const XArchive::RECORD *pRecord, XBinary::PDSTRUCT *pPdStruct, qint64 nDecompressedOffset, qint64 nDecompressedSize,
                                 const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    QByteArray baResult;

    if (!pDevice || !pRecord || (nDecompressedOffset < 0) || (nDecompressedSize < -1) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return baResult;
    }

    XBinary::FT fileType = preferredUnpackerFileType(pDevice, pPdStruct);

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);

    if (pBinary && XFormats::isStaticUnpacker(fileType)) {
        QBuffer buffer(&baResult);
        const bool bOpened = buffer.open(QIODevice::ReadWrite);
        const bool bUnpacked = bOpened && unpackStaticRecord(pBinary, pRecord, &buffer, pPdStruct, mapProperties);
        buffer.close();

        if (!bUnpacked || !XBinary::isPdStructNotCanceled(pPdStruct) || (nDecompressedOffset > baResult.size())) {
            baResult.clear();
        } else if ((nDecompressedOffset != 0) || (nDecompressedSize != -1)) {
            const qint64 nAvailable = baResult.size() - nDecompressedOffset;
            const qint64 nResultSize = (nDecompressedSize == -1) ? nAvailable : qMin(nAvailable, nDecompressedSize);
            baResult = baResult.mid((qint32)nDecompressedOffset, (qint32)nResultSize);
        }

        delete pBinary;
        return baResult;
    }

    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        baResult = pArchives->decompress(pRecord, pPdStruct, nDecompressedOffset, nDecompressedSize, mapProperties);
    }

    delete pBinary;

    return baResult;
}

QByteArray XArchives::decompress(const QString &sFileName, const XArchive::RECORD *pRecord, XBinary::PDSTRUCT *pPdStruct, qint64 nDecompressedOffset,
                                 qint64 nDecompressedSize, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    QByteArray baResult;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        baResult = decompress(&file, pRecord, pPdStruct, nDecompressedOffset, nDecompressedSize, mapProperties);
        file.close();
    }

    return baResult;
}

QByteArray XArchives::decompress(QIODevice *pDevice, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct,
                                 const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    QList<XArchive::RECORD> listRecords = getRecords(pDevice, XBinary::FT_UNKNOWN, -1, pPdStruct);

    XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &listRecords, pPdStruct);

    if (record.spInfo.sRecordName.isEmpty()) return QByteArray();

    return decompress(pDevice, &record, pPdStruct, 0, -1, mapProperties);
}

QByteArray XArchives::decompress(const QString &sFileName, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct,
                                 const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    QByteArray baResult;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        baResult = decompress(&file, sRecordFileName, pPdStruct, mapProperties);
        file.close();
    }

    return baResult;
}

bool XArchives::decompressToFile(QIODevice *pDevice, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct,
                                 const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    bool bResult = false;

    if (!pDevice || !pRecord || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    XBinary::FT fileType = preferredUnpackerFileType(pDevice, pPdStruct);

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);

    if (pBinary && XFormats::isStaticUnpacker(fileType)) {
        qint32 nRecordIndex = -1;
        XBinary::ARCHIVERECORD archiveRecord = {};
        const bool bRecordResolved = resolveStaticRecord(pBinary, pRecord, &nRecordIndex, &archiveRecord, pPdStruct);

        if (bRecordResolved && archiveRecord.mapProperties.value(XBinary::FPART_PROP_ISFOLDER, false).toBool()) {
            bResult = XBinary::createDirectory(sResultFileName) && XBinary::isPdStructNotCanceled(pPdStruct);
        } else if (bRecordResolved) {
            const QFileInfo fileInfo(sResultFileName);
            if (XBinary::createDirectory(fileInfo.absolutePath())) {
                QSaveFile outputFile(sResultFileName);
                if (outputFile.open(QIODevice::WriteOnly)) {
                    bResult = pBinary->unpackRecordByIndex(nRecordIndex, &archiveRecord, &outputFile, mapProperties, pPdStruct);
                    if (bResult && XBinary::isPdStructNotCanceled(pPdStruct) && (outputFile.error() == QFile::NoError)) {
                        bResult = outputFile.commit();
                    } else {
                        outputFile.cancelWriting();
                        bResult = false;
                    }
                }
            }
        }

        delete pBinary;
        return bResult;
    }

    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        bResult = pArchives->decompressToFile(pRecord, sResultFileName, pPdStruct, mapProperties);
    }

    delete pBinary;

    return bResult;
}

bool XArchives::decompressToDevice(QIODevice *pDevice, XArchive::RECORD *pRecord, QIODevice *pDestDevice, XBinary::PDSTRUCT *pPdStruct,
                                   const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    bool bResult = false;

    if (!pDevice || !pRecord || !pDestDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    XBinary::FT fileType = preferredUnpackerFileType(pDevice, pPdStruct);

    XBinary *pBinary = XFormats::createClass(fileType, pDevice);

    if (pBinary && XFormats::isStaticUnpacker(fileType)) {
        bResult = unpackStaticRecord(pBinary, pRecord, pDestDevice, pPdStruct, mapProperties);
        delete pBinary;
        return bResult;
    }

    XArchive *pArchives = dynamic_cast<XArchive *>(pBinary);

    if (pArchives) {
        bResult = pArchives->decompressToDevice(pRecord, pDestDevice, pPdStruct, mapProperties);
    }

    delete pBinary;

    return bResult;
}

bool XArchives::decompressToFile(const QString &sFileName, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct,
                                 const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        bResult = decompressToFile(&file, pRecord, sResultFileName, pPdStruct, mapProperties);

        file.close();
    }

    return bResult;
}

bool XArchives::decompressToFile(const QString &sFileName, const QString &sRecordFileName, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct,
                                 const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties)
{
    bool bResult = false;

    QFile file;
    file.setFileName(sFileName);

    if (file.open(QIODevice::ReadOnly)) {
        QList<XArchive::RECORD> listRecords = getRecords(&file, XBinary::FT_UNKNOWN, -1, pPdStruct);  // TODO FT

        XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &listRecords, pPdStruct);

        if (record.spInfo.sRecordName != "") {
            bResult = decompressToFile(&file, &record, sResultFileName, pPdStruct, mapProperties);
        }

        file.close();
    }

    return bResult;
}

bool XArchives::decompressToFolder(QIODevice *pDevice, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct)
{
    return decompressToFolder(pDevice, sResultFileFolder, QMap<XBinary::UNPACK_PROP, QVariant>(), pPdStruct);
}

bool XArchives::decompressToFolder(QIODevice *pDevice, const QString &sResultFileFolder, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties,
                                   XBinary::PDSTRUCT *pPdStruct, qint32 *pnSkippedEntries)
{
    if (pnSkippedEntries) *pnSkippedEntries = 0;
    if (!pDevice) return false;

    XBinary::PDSTRUCT pdStructEmpty = {};
    if (!pPdStruct) {
        pdStructEmpty = XBinary::createPdStruct();
        pPdStruct = &pdStructEmpty;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const XBinary::FT fileType = preferredUnpackerFileType(pDevice, pPdStruct);
    XBinary *pBinary = XFormats::createClass(fileType, pDevice);

    if (pBinary && XFormats::isStaticUnpacker(fileType)) {
        const bool bResult = pBinary->unpackToFolder(sResultFileFolder, mapProperties, pPdStruct, pnSkippedEntries);
        delete pBinary;
        return bResult;
    }

    XArchive *pArchive = dynamic_cast<XArchive *>(pBinary);
    if (!pArchive) {
        delete pBinary;
        return false;
    }

    // XArchive has a legacy overload with the same name which hides the
    // property-aware implementation inherited from XBinary.  Call the base
    // implementation explicitly so passwords and the other unpack options are
    // preserved for the complete streaming operation.
    bool bResult = pArchive->XBinary::unpackToFolder(sResultFileFolder, mapProperties, pPdStruct, pnSkippedEntries);

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
        bool bStreamingImplemented = hasAuthoritativeStreamingReader(fileType);
        if (!bStreamingImplemented) {
            XBinary::UNPACK_STATE probeState = {};
            bStreamingImplemented = pArchive->initUnpack(&probeState, mapProperties, pPdStruct);
            pArchive->finishUnpack(&probeState, nullptr);
        }

        if (!bStreamingImplemented && XBinary::isPdStructNotCanceled(pPdStruct)) {
            QList<XArchive::RECORD> listRecords = pArchive->getRecords(-1, pPdStruct);
            bResult =
                !listRecords.isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct) && pArchive->decompressToPath(&listRecords, QString(), sResultFileFolder, pPdStruct);
        }
    }

    delete pBinary;

    return bResult;
}

bool XArchives::decompressToFolder(const QString &sFileName, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct)
{
    return decompressToFolder(sFileName, sResultFileFolder, QMap<XBinary::UNPACK_PROP, QVariant>(), pPdStruct);
}

bool XArchives::decompressToFolder(const QString &sFileName, const QString &sResultFileFolder, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties,
                                   XBinary::PDSTRUCT *pPdStruct)
{
    // Keep the filename overload's eager policy validation. The native device
    // path validates again when extraction starts, but retaining this check
    // preserves the existing error contract for malformed limits.
    qint64 nMaxOutputSize = -1;
    if (!XBinary::getUnpackOutputLimit(mapProperties, &nMaxOutputSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Invalid unpacked-output limit"));
        return false;
    }
    XBinary::OUTPUT_POLICY policy = {};
    if (!XBinary::resolveUnpackOutputPolicy(mapProperties, &policy)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Invalid unpacked-output limit"));
        return false;
    }

    QFile file(sFileName);

    if (!file.open(QIODevice::ReadOnly)) return false;

    const bool bResult = decompressToFolder(&file, sResultFileFolder, mapProperties, pPdStruct);
    file.close();

    return bResult;
}

bool XArchives::testArchive(const QString &sFileName, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties, XBinary::PDSTRUCT *pPdStruct)
{
    QTemporaryDir temporaryDir;
    if (!temporaryDir.isValid()) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Cannot create temporary directory"));
        return false;
    }

    QFile file(sFileName);
    if (!file.open(QIODevice::ReadOnly)) return false;

    const bool bResult = decompressToFolder(&file, temporaryDir.path(), mapProperties, pPdStruct);
    file.close();

    return bResult;
}

bool XArchives::isArchiveRecordPresent(QIODevice *pDevice, const QString &sRecordFileName, XBinary::PDSTRUCT *pPdStruct)
{
    QList<XArchive::RECORD> listRecords = getRecords(pDevice, XBinary::FT_UNKNOWN, -1, pPdStruct);
    return XArchive::isArchiveRecordPresent(sRecordFileName, &listRecords, pPdStruct);
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
        // InstallShield cabinets are implemented by XStaticUnpacker rather
        // than the Microsoft-CAB archive reader.  Keep the static flag in this
        // openability probe so FT_ISCAB can actually match the entry returned
        // by getArchiveOpenValidFileTypes().
        QSet<XBinary::FT> stFT = XFormats::getFileTypes(pDevice, XBinary::FT_FLAG_ARCHIVES | XBinary::FT_FLAG_STATICUNPACKERS);

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
    result.insert(XBinary::FT_SAR);
    result.insert(XBinary::FT_ARX);
    result.insert(XBinary::FT_ARJ);
    result.insert(XBinary::FT_ACE);
    result.insert(XBinary::FT_ARC);
    result.insert(XBinary::FT_FREEARC);
    result.insert(XBinary::FT_ZPAQ);
    result.insert(XBinary::FT_BCM);
    result.insert(XBinary::FT_LPAQ8);
    result.insert(XBinary::FT_PEA);
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
    result.insert(XBinary::FT_STK);
    result.insert(XBinary::FT_WARC);
    result.insert(XBinary::FT_MTREE);
    result.insert(XBinary::FT_UU);
    result.insert(XBinary::FT_QUAKE_PAK);
    result.insert(XBinary::FT_DOOM_WAD);
    result.insert(XBinary::FT_BUILD_GRP);
    result.insert(XBinary::FT_DESCENT_HOG);
    result.insert(XBinary::FT_WOLF_VSWAP);
    result.insert(XBinary::FT_WINTERMUTE_DCP);
    result.insert(XBinary::FT_PYINSTALLER_PYZ);
    result.insert(XBinary::FT_AMIGA_LZX);
    result.insert(XBinary::FT_DEARK_LEGACY_ARCHIVE);
    result.insert(XBinary::FT_LIBDSK_IMAGE);
    result.insert(XBinary::FT_COMPACT_PRO);
    result.insert(XBinary::FT_DISK_DOUBLER);
    result.insert(XBinary::FT_DISK_DOUBLER_DDA2);
    result.insert(XBinary::FT_LEGACY_CAT);
    result.insert(XBinary::FT_KA_ARCHIVE);
    result.insert(XBinary::FT_MLB_ARCHIVE);
    result.insert(XBinary::FT_LEGACY_RES);
    result.insert(XBinary::FT_LEGACY_RSC);
    result.insert(XBinary::FT_SHRINKWRAP_IMAGE);
    result.insert(XBinary::FT_LPAK);
    result.insert(XBinary::FT_DISKJUGGLER_CDI);
    result.insert(XBinary::FT_INSTALLSHIELD_BOOT);
    result.insert(XBinary::FT_SABDU_IMAGE);
    result.insert(XBinary::FT_COMPAQ_LZH);
    result.insert(XBinary::FT_WISE_SFX);
    result.insert(XBinary::FT_EPFS_ARCHIVE);
    result.insert(XBinary::FT_STUNTS_DSI);
    result.insert(XBinary::FT_FINSTALL_ARCHIVE);
    result.insert(XBinary::FT_IS_STORED);
    result.insert(XBinary::FT_INSTALLSHIELD3_ARCHIVE);
    result.insert(XBinary::FT_EMT_IMAGE);
    result.insert(XBinary::FT_GPFPACK);
    result.insert(XBinary::FT_PAX);
    result.insert(XBinary::FT_SCF);
    result.insert(XBinary::FT_SOLITAIRE_DELUXE);
    result.insert(XBinary::FT_INSTALIT_DATA);
    result.insert(XBinary::FT_ARCV);
    result.insert(XBinary::FT_PIMP_SFX);
    result.insert(XBinary::FT_VISE_SFX);
    result.insert(XBinary::FT_FTCOMP);
    result.insert(XBinary::FT_DN_ARCHIVE);
    result.insert(XBinary::FT_FPAK);
    result.insert(XBinary::FT_SOFTPAQ1_SFX);
    result.insert(XBinary::FT_INSTALIT_SFX);
    result.insert(XBinary::FT_LIF_COMPRESSED);
    result.insert(XBinary::FT_JASC_ARCHIVE);
    result.insert(XBinary::FT_SSM_MODULE);
    result.insert(XBinary::FT_LHASFX);
    result.insert(XBinary::FT_C64_T64);
    result.insert(XBinary::FT_APPLESINGLE);
    result.insert(XBinary::FT_APPLE_2IMG);
    result.insert(XBinary::FT_MACBINARY);
    result.insert(XBinary::FT_RESOURCE_FORK);
    result.insert(XBinary::FT_CPM_LBR);
    result.insert(XBinary::FT_DMS);
    result.insert(XBinary::FT_PP20);
    result.insert(XBinary::FT_RNC);
    result.insert(XBinary::FT_TPWM);
    result.insert(XBinary::FT_FREEZE);
    result.insert(XBinary::FT_UNIX_PACK);
    result.insert(XBinary::FT_BINHEX);
    result.insert(XBinary::FT_BTOA);
    result.insert(XBinary::FT_RIB);
    result.insert(XBinary::FT_PARSEC_ARCHIVE);
    result.insert(XBinary::FT_PMM);
    result.insert(XBinary::FT_CKP);
    result.insert(XBinary::FT_EDP);
    result.insert(XBinary::FT_MPQ);
    result.insert(XBinary::FT_BIGF);
    result.insert(XBinary::FT_ISCAB);
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
