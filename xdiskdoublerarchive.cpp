/*
 * DiskDoubler/DDAR/DDA2 reader. Structure and codec dispatch were translated
 * from XADMaster (LGPL 2.1 or later); see Algos/xadmaster/COPYING.
 */
#include "xdiskdoublerarchive.h"

#include <QtEndian>
#include <QHash>
#include <QPointer>
#include <QSet>
#if (QT_VERSION_MAJOR < 6) || defined(QT_CORE5COMPAT_LIB)
#include <QTextCodec>  // Qt5 Compat; removed from Qt6 core
#endif

#include <limits>

namespace {
const qint64 MAX_DISKDOUBLER_PARSE_SIZE = Q_INT64_C(512) * 1024 * 1024;
const quint32 DISKDOUBLER_FILE_MAGIC = 0xabcd0054U;

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
    } else {
        if (nTotalSize < 68 ||
            qFromBigEndian<quint32>(pData) != 0x44444132U) return false;
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
            if (sComponent.isEmpty() || nEntrySize < 46 ||
                !rangeWithin(nTotalSize, nPosition, nEntrySize)) return false;
            if (nRawLevel >= 2) {
                const qint32 nLevel = qint32(nRawLevel - 2);
                if (nLevel > 128) return false;
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
                } else {
                    const qint64 nFileMagicOffset = nPosition + 56;
                    if (nEntrySize < 140 ||
                        !parseFileHeader(&parseContext, nFileMagicOffset, sPath,
                                         nPosition + nEntrySize)) return false;
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
