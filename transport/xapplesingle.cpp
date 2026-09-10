/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xapplesingle.h"

#include <QtEndian>

namespace {
QString appleEntryName(quint32 nEntryId)
{
    switch (nEntryId) {
        case 1: return QStringLiteral("data-fork.bin");
        case 2: return QStringLiteral("resource-fork.bin");
        case 3: return QStringLiteral("real-name.bin");
        case 4: return QStringLiteral("comment.bin");
        case 5: return QStringLiteral("black-and-white-icon.bin");
        case 6: return QStringLiteral("color-icon.bin");
        case 8: return QStringLiteral("file-dates.bin");
        case 9: return QStringLiteral("finder-info.bin");
        case 10: return QStringLiteral("macintosh-file-info.bin");
        case 11: return QStringLiteral("prodos-file-info.bin");
        case 12: return QStringLiteral("msdos-file-info.bin");
        case 13: return QStringLiteral("afp-short-name.bin");
        case 14: return QStringLiteral("afp-file-info.bin");
        case 15: return QStringLiteral("afp-directory-id.bin");
        default:
            return QStringLiteral("entry-%1.bin").arg(nEntryId);
    }
}
}  // namespace

XAppleSingle::XAppleSingle(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_APPLESINGLE)
{
}

bool XAppleSingle::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAppleSingle archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAppleSingle::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAppleSingle(pDevice);
}

bool XAppleSingle::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                              PDSTRUCT *pPdStruct)
{
    QPointer<XAppleSingle> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 38) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baHeader = read_array_process(0, 26, pPdStruct);
    if (!guardedThis || (baHeader.size() != 26)) return false;
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const quint32 nMagic = qFromBigEndian<quint32>(pHeader);
    const quint32 nVersion = qFromBigEndian<quint32>(pHeader + 4);
    const quint16 nRecordCount = qFromBigEndian<quint16>(pHeader + 24);
    if (((nMagic != 0x00051600U) && (nMagic != 0x00051607U)) ||
        ((nVersion != 0x00010000U) && (nVersion != 0x00020000U)) ||
        (nRecordCount == 0) || (nRecordCount > MAX_RECORDS)) {
        return false;
    }

    const qint64 nDirectorySize = (qint64)nRecordCount * 12;
    const qint64 nDataFloor = 26 + nDirectorySize;
    if (!rangeWithin(nTotalSize, 26, nDirectorySize)) return false;
    const QByteArray baDirectory =
        read_array_process(26, nDirectorySize, pPdStruct);
    if (!guardedThis || (baDirectory.size() != nDirectorySize)) return false;

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    QList<QPair<qint64, qint64> > listRanges;
    const uchar *pDirectory =
        reinterpret_cast<const uchar *>(baDirectory.constData());
    qint64 nArchiveEnd = nDataFloor;
    for (quint16 i = 0; i < nRecordCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pRecord = pDirectory + ((qint64)i * 12);
        const quint32 nEntryId = qFromBigEndian<quint32>(pRecord);
        const qint64 nDataOffset = qFromBigEndian<quint32>(pRecord + 4);
        const qint64 nDataSize = qFromBigEndian<quint32>(pRecord + 8);
        if ((nEntryId == 0) || (nDataOffset < nDataFloor) ||
            !rangeWithin(nTotalSize, nDataOffset, nDataSize)) {
            return false;
        }
        for (const QPair<qint64, qint64> &range : listRanges) {
            if (rangesOverlap(nDataOffset, nDataSize,
                              range.first, range.second)) {
                return false;
            }
        }
        if (nDataSize > 0)
            listRanges.append(qMakePair(nDataOffset, nDataSize));

        QString sUniqueName;
        if (!makeUniquePath(appleEntryName(nEntryId), &stUsedFiles,
                            &stUsedDirectories, &mapNextSuffixes,
                            &mapResolvedDirectories, &sUniqueName)) {
            return false;
        }
        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = 26 + ((qint64)i * 12);
            entry.nHeaderSize = 12;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
        nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nDataSize);
    }

    if ((nArchiveEnd != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}
