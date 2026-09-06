/*
 * DiskDoubler/DDAR/DDA2 reader. Structure and codec dispatch were translated
 * from XADMaster (LGPL 2.1 or later); see Algos/xadmaster/COPYING.
 */
#include "xdiskdoublerarchive.h"

#include <QtEndian>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QStringList>
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
#include <QTextCodec>  // Qt5 Compat; removed from Qt6 core
#endif

#include <limits>

namespace {
const qint64 MAX_DISKDOUBLER_PARSE_SIZE = Q_INT64_C(512) * 1024 * 1024;
const quint32 DISKDOUBLER_FILE_MAGIC = 0xabcd0054U;

quint16 diskDoublerHeaderCRC(const uchar *pData, qint32 nSize)
{
    quint32 crc = 0;
    for (qint32 i = 0; i < nSize; ++i) {
        crc ^= quint32(pData[i]) << 8;
        for (qint32 bit = 0; bit < 8; ++bit)
            crc = ((crc << 1) ^ ((crc & 0x8000U) ? 0x1021U : 0U)) & 0xffffU;
    }
    return quint16(crc);
}

bool diskDoublerSumMatches(const uchar *pData, qint64 nSize,
                          quint16 nExpected, XBinary::PDSTRUCT *pPdStruct)
{
    quint32 sum = 0;
    for (qint64 i = 0; i < nSize; ++i) {
        if (!(i & 0xffff) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        sum = (sum + pData[i]) & 0xffffU;
    }
    return sum == nExpected && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool diskDoublerXorMatches(const uchar *pData, qint64 nSize,
                          quint8 nExpected, XBinary::PDSTRUCT *pPdStruct)
{
    quint8 sum = 0;
    for (qint64 i = 0; i < nSize; ++i) {
        if (!(i & 0xffff) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        sum ^= pData[i];
    }
    return sum == nExpected && XBinary::isPdStructNotCanceled(pPdStruct);
}

QString diskDoublerName(const QByteArray &baName)
{
    // Qt6 dropped QTextCodec from QtCore, and this build does not link
    // Core5Compat. Fall back to the same Latin-1 decoding the Qt5 path already
    // uses when the "macintosh" codec is unavailable.
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
    QTextCodec *pCodec = QTextCodec::codecForName("macintosh");
    QString sName = pCodec ? pCodec->toUnicode(baName)
                           : QString::fromLatin1(baName);
#else
    QString sName = QString::fromLatin1(baName);
#endif
    sName.replace(QLatin1Char('/'), QLatin1Char('_'));
    sName.replace(QLatin1Char(':'), QLatin1Char('_'));
    sName = sName.normalized(QString::NormalizationForm_C).trimmed();
    if (sName.isEmpty() || (sName == QLatin1String(".")) ||
        (sName == QLatin1String(".."))) return QString();
    return XBinary::fixFileName(sName);
}

XBinary::HANDLE_METHOD diskDoublerMethod(quint8 nMethod)
{
    switch (nMethod & 0x7f) {
        case 0: return XBinary::HANDLE_METHOD_STORE;
        case 1: return XBinary::HANDLE_METHOD_DISKDOUBLER_LZW;
        case 6:
        case 9: return XBinary::HANDLE_METHOD_DISKDOUBLER_ADN;
        case 8: return XBinary::HANDLE_METHOD_DISKDOUBLER_COMPACT_PRO;
        case 10: return XBinary::HANDLE_METHOD_DISKDOUBLER_DDN;
        default: return XBinary::HANDLE_METHOD_UNKNOWN;
    }
}
}  // namespace

struct XDiskDoublerArchive::PARSE_CONTEXT
{
    qint64 nTotalSize;
    const uchar *pData;
    QSet<QString> *pUsedFiles;
    QSet<QString> *pUsedDirectories;
    QHash<QString, qint32> *pNextSuffixes;
    QHash<QString, QString> *pResolvedDirectories;
    QList<ENTRY> *pEntries;
};

XDiskDoublerArchive::XDiskDoublerArchive(QIODevice *pDevice, FT fileType)
    : XGameStoreArchiveBase(pDevice, fileType)
{
}

bool XDiskDoublerArchive::isValid(QIODevice *pDevice, FT fileType,
                                  PDSTRUCT *pPdStruct)
{
    XDiskDoublerArchive archive(pDevice, fileType);
    return archive.isValid(pPdStruct);
}

XBinary *XDiskDoublerArchive::createInstance(QIODevice *pDevice,
                                             bool bIsImage,
                                             XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDiskDoublerArchive(pDevice, getFileType());
}

XBinary::ARCHIVERECORD XDiskDoublerArchive::infoCurrent(
    UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XDiskDoublerArchive> guardedThis(this);
    ARCHIVERECORD record = XGameStoreArchiveBase::infoCurrent(pState, pPdStruct);
    if (!guardedThis || record.mapProperties.isEmpty()) return ARCHIVERECORD();
    if (record.mapProperties.value(FPART_PROP_HANDLEMETHOD).toUInt() !=
        HANDLE_METHOD_DISKDOUBLER_LZW) return record;

    const qint64 headerOffset = record.mapProperties.value(FPART_PROP_HEADER_OFFSET).toLongLong();
    const QByteArray header = read_array_process(headerOffset, 84, pPdStruct);
    if (!guardedThis || header.size() != 84 || !isUnpackSourceCurrent(pState, pPdStruct))
        return ARCHIVERECORD();
    const uchar *h = reinterpret_cast<const uchar *>(header.constData());
    if (qFromBigEndian<quint32>(h) != DISKDOUBLER_FILE_MAGIC) return ARCHIVERECORD();
    const bool resource = record.nStreamOffset != headerOffset + 84 ||
                          (!qFromBigEndian<quint32>(h + 4) && qFromBigEndian<quint32>(h + 12));
    if (record.nStreamOffset != headerOffset + 84 + (resource ? qFromBigEndian<quint32>(h + 8) : 0) ||
        (h[resource ? 21 : 20] & 0x7f) != 1) return ARCHIVERECORD();
    QByteArray properties;
    properties.append(char(h[22]));  // format generation controls the XOR transform
    properties.append(char(h[52]));
    properties.append(header.constData() + (resource ? 50 : 48), 2);
    record.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, properties);
    return record;
}

bool XDiskDoublerArchive::addUniqueEntry(PARSE_CONTEXT *pContext,
                                         ENTRY *pEntry,
                                         const QString &sPath)
{
    if (!pContext || !pEntry || !pContext->pEntries) return false;
    QString sUnique;
    if (!makeUniquePath(sPath, pContext->pUsedFiles,
                        pContext->pUsedDirectories,
                        pContext->pNextSuffixes,
                        pContext->pResolvedDirectories, &sUnique))
        return false;
    pEntry->sFileName = sUnique;
    pContext->pEntries->append(*pEntry);
    return pContext->pEntries->count() <= MAX_RECORDS;
}

bool XDiskDoublerArchive::parseFileHeader(PARSE_CONTEXT *pContext,
                                          qint64 nMagicOffset,
                                          const QString &sPath,
                                          qint64 nContainerEnd)
{
    if (!pContext || !rangeWithin(pContext->nTotalSize, nMagicOffset, 84) ||
        qFromBigEndian<quint32>(pContext->pData + nMagicOffset) !=
            DISKDOUBLER_FILE_MAGIC) return false;
    const uchar *pHeader = pContext->pData + nMagicOffset + 4;
    const quint16 nHeaderCRC = qFromBigEndian<quint16>(pHeader + 78);
    // Very early DiskDoubler files have no header CRC (a zero field).
    if (nHeaderCRC && diskDoublerHeaderCRC(pHeader - 4, 82) != nHeaderCRC)
        return false;
    const qint64 nDataSize = qFromBigEndian<quint32>(pHeader);
    const qint64 nDataPacked = qFromBigEndian<quint32>(pHeader + 4);
    const qint64 nResourceSize = qFromBigEndian<quint32>(pHeader + 8);
    const qint64 nResourcePacked = qFromBigEndian<quint32>(pHeader + 12);
    const quint8 nDataMethod = pHeader[16];
    const quint8 nResourceMethod = pHeader[17];
    const quint16 nDataDelta = qFromBigEndian<quint16>(pHeader + 50);
    const quint16 nResourceDelta = qFromBigEndian<quint16>(pHeader + 52);
    const qint64 nDataOffset = nMagicOffset + 84;
    const qint64 nResourceOffset = nDataOffset + nDataPacked;
    const qint64 nEnd = nResourceOffset + nResourcePacked;
    const HANDLE_METHOD dataMethod = diskDoublerMethod(nDataMethod);
    const HANDLE_METHOD resourceMethod = diskDoublerMethod(nResourceMethod);
    if (nDataDelta || nResourceDelta ||
        ((nDataSize || !nResourceSize) && dataMethod == HANDLE_METHOD_UNKNOWN) ||
        (nResourceSize && resourceMethod == HANDLE_METHOD_UNKNOWN) ||
        !rangeWithin(pContext->nTotalSize, nDataOffset, nDataPacked) ||
        !rangeWithin(pContext->nTotalSize, nResourceOffset, nResourcePacked) ||
        nEnd > nContainerEnd ||
        (dataMethod == HANDLE_METHOD_STORE && nDataPacked != nDataSize) ||
        (resourceMethod == HANDLE_METHOD_STORE && nResourcePacked != nResourceSize))
        return false;

    if (nDataSize || !nResourceSize) {
        ENTRY entry = {};
        entry.nHeaderOffset = nMagicOffset;
        entry.nHeaderSize = 84;
        entry.nDataOffset = nDataOffset;
        entry.nDataSize = nDataPacked;
        entry.nUncompressedSize = nDataSize;
        entry.handleMethod = nDataSize ? dataMethod : HANDLE_METHOD_STORE;
        if (!addUniqueEntry(pContext, &entry, sPath)) return false;
    }
    if (nResourceSize) {
        ENTRY entry = {};
        entry.nHeaderOffset = nMagicOffset;
        entry.nHeaderSize = 84;
        entry.nDataOffset = nResourceOffset;
        entry.nDataSize = nResourcePacked;
        entry.nUncompressedSize = nResourceSize;
        entry.handleMethod = resourceMethod;
        if (!addUniqueEntry(pContext, &entry, sPath + QStringLiteral(".rsrc")))
            return false;
    }
    return true;
}

bool XDiskDoublerArchive::scanFormat(QList<ENTRY> *pEntries,
                                     qint64 *pArchiveEnd,
                                     PDSTRUCT *pPdStruct)
{
    QPointer<XDiskDoublerArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    const FT fileType = getFileType();
    if (!guardedThis || nTotalSize < 84 ||
        nTotalSize > MAX_DISKDOUBLER_PARSE_SIZE ||
        nTotalSize > (std::numeric_limits<int>::max)() ||
        ((fileType != FT_DISK_DOUBLER) &&
         (fileType != FT_DISK_DOUBLER_DDAR) &&
         (fileType != FT_DISK_DOUBLER_DDA2)) ||
        !isPdStructNotCanceled(pPdStruct)) return false;
    const QByteArray baData = read_array_process(0, nTotalSize, pPdStruct);
    if (!guardedThis || baData.size() != nTotalSize) return false;
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData());

    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;
    QList<ENTRY> entries;
    PARSE_CONTEXT parseContext = {nTotalSize, pData, &usedFiles,
                                  &usedDirectories, &nextSuffixes,
                                  &resolvedDirectories, &entries};

    if (fileType == FT_DISK_DOUBLER) {
        if (qFromBigEndian<quint32>(pData) != DISKDOUBLER_FILE_MAGIC)
            return false;
        QString sName = XBinary::getDeviceFileBaseName(getDevice());
        if (sName.endsWith(QStringLiteral(".dd"), Qt::CaseInsensitive))
            sName.chop(3);
        sName.replace(QLatin1Char('/'), QLatin1Char('_'));
        sName.replace(QLatin1Char(':'), QLatin1Char('_'));
        sName = XBinary::fixFileName(sName);
        if (sName.isEmpty()) sName = QStringLiteral("unpacked");
        const qint64 nDataPacked = qFromBigEndian<quint32>(pData + 8);
        const qint64 nResourcePacked = qFromBigEndian<quint32>(pData + 16);
        const qint64 nPayloadEnd = 84 + nDataPacked + nResourcePacked;
        if (!rangeWithin(nTotalSize, 0, nPayloadEnd) ||
            !parseFileHeader(&parseContext, 0, sName, nPayloadEnd)) return false;
        const qint64 nTrailing = nTotalSize - nPayloadEnd;
        if (nTrailing != 0 &&
            (nTrailing != 84 ||
             baData.mid(nPayloadEnd, 84) != baData.left(84))) return false;
    } else if (fileType == FT_DISK_DOUBLER_DDAR) {
        if (nTotalSize < 78 || qFromBigEndian<quint32>(pData) != 0x44444152U ||
            qFromBigEndian<quint32>(pData + 8) != quint64(nTotalSize) ||
            diskDoublerHeaderCRC(pData, 76) != qFromBigEndian<quint16>(pData + 76))
            return false;
        qint64 position = 78;
        qint32 records = 0;
        QStringList directories;
        while (position < nTotalSize) {
            if (++records > MAX_RECORDS || !rangeWithin(nTotalSize, position, 124) ||
                !isPdStructNotCanceled(pPdStruct)) return false;
            const uchar *h = pData + position;
            if (qFromBigEndian<quint32>(h) != 0x44444152U || h[8] > 63 ||
                diskDoublerHeaderCRC(h, 122) != qFromBigEndian<quint16>(h + 122))
                return false;
            const qint64 dataSize = qFromBigEndian<quint32>(h + 74);
            const qint64 resourceSize = qFromBigEndian<quint32>(h + 78);
            const qint64 dataOffset = position + 124;
            if (h[73]) {
                if (directories.isEmpty() || dataSize || resourceSize) return false;
                directories.removeLast();
            } else {
                const QString component = diskDoublerName(baData.mid(position + 9, h[8]));
                if (component.isEmpty()) return false;
                if (h[72]) {
                    if (dataSize || resourceSize || directories.size() >= 128) return false;
                    directories.append(component);
                } else {
                    if (!rangeWithin(nTotalSize, dataOffset, dataSize + resourceSize) ||
                        !diskDoublerSumMatches(pData + dataOffset, dataSize,
                                              qFromBigEndian<quint16>(h + 118), pPdStruct) ||
                        !diskDoublerSumMatches(pData + dataOffset + dataSize, resourceSize,
                                              qFromBigEndian<quint16>(h + 120), pPdStruct)) return false;
                    const QString path = directories.isEmpty() ? component
                        : directories.join(QLatin1Char('/')) + QLatin1Char('/') + component;
                    // DDAR stores the complete Macintosh forks. Preserve the
                    // outer bytes, including any contained DiskDoubler file;
                    // decoding that file is a separate archive operation.
                    if (dataSize || !resourceSize) {
                        ENTRY entry = {};
                        entry.nHeaderOffset = position;
                        entry.nHeaderSize = 124;
                        entry.nDataOffset = dataOffset;
                        entry.nDataSize = dataSize;
                        if (!addUniqueEntry(&parseContext, &entry, path)) return false;
                    }
                    if (resourceSize) {
                        ENTRY entry = {};
                        entry.nHeaderOffset = position;
                        entry.nHeaderSize = 124;
                        entry.nDataOffset = dataOffset + dataSize;
                        entry.nDataSize = resourceSize;
                        if (!addUniqueEntry(&parseContext, &entry, path + QStringLiteral(".rsrc"))) return false;
                    }
                }
            }
            position = dataOffset + dataSize + resourceSize;
        }
        if (!directories.isEmpty()) return false;
    } else {
        if (nTotalSize < 68 ||
            qFromBigEndian<quint32>(pData) != 0x44444132U ||
            qFromBigEndian<quint16>(pData + 4) != 62 ||
            diskDoublerHeaderCRC(pData, 60) != qFromBigEndian<quint16>(pData + 60)) return false;
        qint64 nPosition = 62;
        QHash<qint32, QString> directoryAtLevel;
        bool bTerminated = false;
        qint32 nRecords = 0;
        while (rangeWithin(nTotalSize, nPosition, 6)) {
            if (!guardedThis || !isPdStructNotCanceled(pPdStruct) ||
                qFromBigEndian<quint32>(pData + nPosition) != 0x44444132U)
                return false;
            const quint16 nEntryType =
                qFromBigEndian<quint16>(pData + nPosition + 4);
            if (nEntryType == 0xbbbb) {
                bTerminated = true;
                nPosition += 6;
                break;
            }
            if (++nRecords > MAX_RECORDS ||
                !rangeWithin(nTotalSize, nPosition, 46)) return false;
            const qint32 nNameSize = qMin<qint32>(pData[nPosition + 6], 31);
            const QString sComponent = diskDoublerName(
                baData.mid(nPosition + 7, nNameSize));
            const quint32 nRawLevel =
                qFromBigEndian<quint32>(pData + nPosition + 38);
            const qint64 nEntrySize =
                qFromBigEndian<quint32>(pData + nPosition + 42);
            const qint32 nHeaderSize = (nEntryType & 0x8000) ? 88 :
                                      (nEntryType & 0x4000) ? 56 : 90;
            if (sComponent.isEmpty() || nEntrySize < nHeaderSize ||
                !rangeWithin(nTotalSize, nPosition, nEntrySize) ||
                diskDoublerHeaderCRC(pData + nPosition, nHeaderSize - 2) !=
                    qFromBigEndian<quint16>(pData + nPosition + nHeaderSize - 2)) return false;
            if (nRawLevel >= 2) {
                if (nRawLevel > 130) return false;
                const qint32 nLevel = qint32(nRawLevel - 2);
                for (QHash<qint32, QString>::iterator it = directoryAtLevel.begin();
                     it != directoryAtLevel.end();) {
                    if (it.key() >= nLevel) it = directoryAtLevel.erase(it);
                    else ++it;
                }
                QString sPath = sComponent;
                if (nLevel > 0) {
                    if (!directoryAtLevel.contains(nLevel - 1)) return false;
                    sPath = directoryAtLevel.value(nLevel - 1) +
                            QLatin1Char('/') + sComponent;
                }
                if (nEntryType & 0x8000) {
                    directoryAtLevel.insert(nLevel, sPath);
                } else if (nEntryType & 0x4000) {
                    const qint64 nFileMagicOffset = nPosition + 56;
                    if (nEntrySize < 140 ||
                        !parseFileHeader(&parseContext, nFileMagicOffset, sPath,
                                         nPosition + nEntrySize)) return false;
                } else {
                    // Stored DDA2 records have a 90-byte header instead of
                    // the 56-byte wrapper plus an embedded compressed file.
                    // U3's 006a2830/006a2530 also validates each raw fork's
                    // XOR checksum independently of the header CRC.
                    const qint64 nDataSize = qFromBigEndian<quint32>(pData + nPosition + 78);
                    const qint64 nResourceSize = qFromBigEndian<quint32>(pData + nPosition + 82);
                    const qint64 nDataOffset = nPosition + 90;
                    if (nDataSize + nResourceSize != nEntrySize - 90 ||
                        !diskDoublerXorMatches(pData + nDataOffset, nDataSize,
                                              pData[nPosition + 86], pPdStruct) ||
                        !diskDoublerXorMatches(pData + nDataOffset + nDataSize, nResourceSize,
                                              pData[nPosition + 87], pPdStruct)) return false;
                    if (nDataSize || !nResourceSize) {
                        ENTRY entry = {};
                        entry.nHeaderOffset = nPosition;
                        entry.nHeaderSize = 90;
                        entry.nDataOffset = nDataOffset;
                        entry.nDataSize = nDataSize;
                        if (!addUniqueEntry(&parseContext, &entry, sPath)) return false;
                    }
                    if (nResourceSize) {
                        ENTRY entry = {};
                        entry.nHeaderOffset = nPosition;
                        entry.nHeaderSize = 90;
                        entry.nDataOffset = nDataOffset + nDataSize;
                        entry.nDataSize = nResourceSize;
                        if (!addUniqueEntry(&parseContext, &entry, sPath + QStringLiteral(".rsrc"))) return false;
                    }
                }
            }
            nPosition += nEntrySize;
        }
        if (!bTerminated) return false;
        // DDA2 keeps a fixed archive summary after the BBBB end record. The
        // entry chain above is authoritative; this footer is not member data.
        if (nTotalSize - nPosition > 4096) return false;
    }

    if (entries.isEmpty()) return false;
    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
