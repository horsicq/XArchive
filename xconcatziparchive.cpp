/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xconcatziparchive.h"

#include "subdevice.h"
#include "xzip.h"

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QtEndian>

#include <cstring>
#include <limits>

namespace {
const qint64 MAX_CONCAT_ZIP_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint32 MAX_EOCD_ATTEMPTS = 4096;

bool rangeWithin(qint64 total, qint64 offset, qint64 size)
{
    return offset >= 0 && size >= 0 && offset <= total && size <= total - offset;
}
}  // namespace

XConcatZipArchive::XConcatZipArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_ZIP)
{
}

bool XConcatZipArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XConcatZipArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XConcatZipArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                            XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XConcatZipArchive(pDevice);
}

bool XConcatZipArchive::scanFormat(QList<ENTRY> *pEntries,
                                   qint64 *pArchiveEnd,
                                   PDSTRUCT *pPdStruct)
{
    PDSTRUCT localPdStruct = {};
    if (!pPdStruct) {
        localPdStruct = XBinary::createPdStruct();
        pPdStruct = &localPdStruct;
    }
    QPointer<XConcatZipArchive> guardedThis(this);
    const qint64 total = getSize();
    if (!guardedThis || total < 44 || total > MAX_CONCAT_ZIP_SIZE ||
        total > (std::numeric_limits<int>::max)() ||
        !isPdStructNotCanceled(pPdStruct)) return false;
    const QByteArray data = read_array_process(0, total, pPdStruct);
    if (!guardedThis || data.size() != total ||
        !data.startsWith(QByteArrayLiteral("PK\x03\x04"))) return false;

    QList<ENTRY> entries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirs;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirs;
    qint64 componentStart = 0;
    qint64 lastEnd = 0;
    qint32 componentCount = 0;

    while (componentStart >= 0 && componentStart < total &&
           isPdStructNotCanceled(pPdStruct)) {
        qint64 eocd = data.indexOf(QByteArrayLiteral("PK\x05\x06"),
                                   qint32(componentStart + 4));
        qint64 componentEnd = -1;
        QList<RECORD> records;
        for (qint32 attempt = 0;
             attempt < MAX_EOCD_ATTEMPTS && eocd >= 0 && eocd <= total - 22;
             ++attempt) {
            const uchar *p = reinterpret_cast<const uchar *>(data.constData());
            const quint16 commentSize = qFromLittleEndian<quint16>(p + eocd + 20);
            const qint64 trialEnd = eocd + 22 + commentSize;
            if (trialEnd >= eocd + 22 && trialEnd <= total) {
                SubDevice sub(getDevice(), componentStart,
                              trialEnd - componentStart);
                if (sub.open(QIODevice::ReadOnly)) {
                    XZip zip(&sub);
                    if (zip.isValid(pPdStruct) &&
                        zip.getFileFormatSize(pPdStruct) == sub.size()) {
                        records = zip.getRecords(MAX_RECORDS, pPdStruct);
                        if (!records.isEmpty()) componentEnd = trialEnd;
                    }
                    sub.close();
                }
            }
            if (componentEnd >= 0) break;
            eocd = data.indexOf(QByteArrayLiteral("PK\x05\x06"),
                                qint32(eocd + 4));
        }
        if (componentEnd < 0 || records.isEmpty()) return false;

        for (const RECORD &record : records) {
            if (!isPdStructNotCanceled(pPdStruct) ||
                record.spInfo.sRecordName.isEmpty() ||
                record.mapProperties.value(FPART_PROP_ENCRYPTED, false).toBool() ||
                record.spInfo.compressMethod == HANDLE_METHOD_UNKNOWN ||
                !rangeWithin(componentEnd - componentStart,
                             record.nHeaderOffset, record.nHeaderSize) ||
                !rangeWithin(componentEnd - componentStart,
                             record.nDataOffset, record.nDataSize) ||
                record.spInfo.nUncompressedSize < 0 ||
                record.spInfo.nUncompressedSize > MAX_CONCAT_ZIP_SIZE)
                return false;
            QString unique;
            if (!makeUniquePath(record.spInfo.sRecordName, &usedFiles, &usedDirs,
                                &nextSuffixes, &resolvedDirs, &unique)) return false;
            ENTRY entry = {};
            entry.nHeaderOffset = componentStart + record.nHeaderOffset;
            entry.nHeaderSize = record.nHeaderSize;
            entry.nDataOffset = componentStart + record.nDataOffset;
            entry.nDataSize = record.nDataSize;
            entry.nUncompressedSize = record.spInfo.nUncompressedSize;
            entry.handleMethod = record.spInfo.compressMethod;
            entry.sFileName = unique;
            if (record.mapProperties.contains(FPART_PROP_RESULTCRC)) {
                entry.bCRC32Defined = true;
                entry.nCRC32 = record.mapProperties.value(FPART_PROP_RESULTCRC).toUInt();
            }
            entries.append(entry);
            if (entries.size() > MAX_RECORDS) return false;
        }

        ++componentCount;
        lastEnd = componentEnd;
        qint64 nextStart = data.indexOf(QByteArrayLiteral("PK\x03\x04"),
                                        qint32(componentEnd));
        if (nextStart < 0) break;
        componentStart = nextStart;
    }

    // Ordinary ZIP files remain delegated to XZip. Requiring two components
    // keeps this adapter narrowly scoped and prevents recursive selection.
    if (componentCount < 2 || entries.isEmpty() || lastEnd <= 0) return false;
    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = lastEnd;
    return true;
}
