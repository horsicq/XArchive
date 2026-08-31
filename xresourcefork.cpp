/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xresourcefork.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace
{
QString resourceTypePath(const uchar *pType)
{
    bool bPrintable = true;
    for (qint32 i = 0; i < 4; ++i)
    {
        if ((pType[i] < 0x20) || (pType[i] > 0x7e) || (pType[i] == '/') ||
            (pType[i] == '\\'))
        {
            bPrintable = false;
            break;
        }
    }
    if (bPrintable)
    {
        QString sResult =
            QString::fromLatin1(reinterpret_cast<const char *>(pType), 4);
        sResult = XBinary::fixFileName(sResult);
        if (!sResult.isEmpty() && (sResult != QLatin1String("_")) &&
            !sResult.contains(QLatin1Char('/')))
            return sResult;
    }
    const QByteArray baType(reinterpret_cast<const char *>(pType), 4);
    return QStringLiteral("type-") +
           QString::fromLatin1(baType.toHex().toUpper());
}

QString resourceLeafName(qint16 nResourceId, const uchar *pName,
                         qint32 nNameLength)
{
    QString sResult = QString::number(nResourceId);
    if (pName && (nNameLength > 0))
    {
        QString sName = QString::fromLatin1(
            reinterpret_cast<const char *>(pName), nNameLength);
        sName.replace(QLatin1Char('/'), QLatin1Char('_'));
        sName.replace(QLatin1Char('\\'), QLatin1Char('_'));
        sName = XBinary::fixFileName(sName);
        if (!sName.isEmpty() && !sName.contains(QLatin1Char('/')))
        {
            sResult += QLatin1Char('_') + sName;
        }
    }
    return XBinary::fixFileName(sResult + QStringLiteral(".bin"));
}
} // namespace

XResourceFork::XResourceFork(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_RESOURCE_FORK)
{
}

bool XResourceFork::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XResourceFork archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XResourceFork::createInstance(QIODevice *pDevice, bool bIsImage,
                                       XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XResourceFork(pDevice);
}

bool XResourceFork::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                               PDSTRUCT *pPdStruct)
{
    QPointer<XResourceFork> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 44) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const QByteArray baHeader = read_array_process(0, 16, pPdStruct);
    if (!guardedThis || (baHeader.size() != 16))
        return false;
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const qint64 nDataOffset = qFromBigEndian<quint32>(pHeader);
    const qint64 nMapOffset = qFromBigEndian<quint32>(pHeader + 4);
    const qint64 nDataSize = qFromBigEndian<quint32>(pHeader + 8);
    const qint64 nMapSize = qFromBigEndian<quint32>(pHeader + 12);
    if ((nDataOffset < 16) || (nMapOffset < 16) || (nMapSize < 28) ||
        (nMapSize > std::numeric_limits<int>::max()) ||
        !rangeWithin(nTotalSize, nDataOffset, nDataSize) ||
        !rangeWithin(nTotalSize, nMapOffset, nMapSize) ||
        rangesOverlap(nDataOffset, nDataSize, nMapOffset, nMapSize) ||
        (qMax(nDataOffset + nDataSize, nMapOffset + nMapSize) != nTotalSize))
        return false;

    const QByteArray baMap =
        read_array_process(nMapOffset, nMapSize, pPdStruct);
    if (!guardedThis || (baMap.size() != nMapSize))
        return false;
    const uchar *pMap = reinterpret_cast<const uchar *>(baMap.constData());

    bool bZeroHeaderCopy = true;
    for (qint32 i = 0; i < 16; ++i)
    {
        if (pMap[i] != 0)
        {
            bZeroHeaderCopy = false;
            break;
        }
    }
    if (!bZeroHeaderCopy && (memcmp(pMap, pHeader, 16) != 0))
        return false;

    const qint64 nTypeListOffset = qFromBigEndian<quint16>(pMap + 24);
    const qint64 nNameListOffset = qFromBigEndian<quint16>(pMap + 26);
    if ((nTypeListOffset < 28) || !rangeWithin(nMapSize, nTypeListOffset, 2) ||
        (nNameListOffset < (nTypeListOffset + 2)) ||
        !rangeWithin(nMapSize, nNameListOffset, 0))
        return false;

    const quint16 nRawTypeCount =
        qFromBigEndian<quint16>(pMap + nTypeListOffset);
    const qint32 nTypeCount =
        (nRawTypeCount == 0xffffU) ? 0 : ((qint32)nRawTypeCount + 1);
    const qint64 nTypeRecordsOffset = nTypeListOffset + 2;
    const qint64 nTypeRecordsSize = (qint64)nTypeCount * 8;
    if ((nTypeCount > MAX_RECORDS) ||
        !rangeWithin(nMapSize, nTypeRecordsOffset, nTypeRecordsSize) ||
        ((nTypeRecordsOffset + nTypeRecordsSize) > nNameListOffset))
        return false;

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    QList<QPair<qint64, qint64>> listReferenceRanges;
    QList<QPair<qint64, qint64>> listDataRanges;
    qint32 nRecordCount = 0;

    QString sMapName;
    if (!makeUniquePath(QStringLiteral("resource-map.bin"), &stUsedFiles,
                        &stUsedDirectories, &mapNextSuffixes,
                        &mapResolvedDirectories, &sMapName))
        return false;
    if (pEntries)
    {
        ENTRY entry = {};
        entry.nHeaderOffset = 0;
        entry.nHeaderSize = 16;
        entry.nDataOffset = nMapOffset;
        entry.nDataSize = nMapSize;
        entry.sFileName = sMapName;
        pEntries->append(entry);
    }
    nRecordCount++;

    for (qint32 i = 0; i < nTypeCount; ++i)
    {
        if (!XBinary::isPdStructNotCanceled(pPdStruct))
            return false;
        const qint64 nTypeRecordOffset = nTypeRecordsOffset + ((qint64)i * 8);
        const uchar *pTypeRecord = pMap + nTypeRecordOffset;
        const quint16 nRawResourceCount =
            qFromBigEndian<quint16>(pTypeRecord + 4);
        const qint32 nResourceCount = (nRawResourceCount == 0xffffU)
                                          ? 0
                                          : ((qint32)nRawResourceCount + 1);
        const qint64 nReferenceOffset =
            nTypeListOffset + qFromBigEndian<quint16>(pTypeRecord + 6);
        const qint64 nReferenceSize = (qint64)nResourceCount * 12;
        if ((nResourceCount > (MAX_RECORDS - nRecordCount)) ||
            (nReferenceOffset < (nTypeRecordsOffset + nTypeRecordsSize)) ||
            !rangeWithin(nMapSize, nReferenceOffset, nReferenceSize) ||
            ((nReferenceOffset + nReferenceSize) > nNameListOffset))
            return false;
        for (const QPair<qint64, qint64> &range : listReferenceRanges)
        {
            if (rangesOverlap(nReferenceOffset, nReferenceSize, range.first,
                              range.second))
                return false;
        }
        if (nReferenceSize > 0)
        {
            listReferenceRanges.append(
                qMakePair(nReferenceOffset, nReferenceSize));
        }

        const QString sTypePath = resourceTypePath(pTypeRecord);
        for (qint32 j = 0; j < nResourceCount; ++j)
        {
            const qint64 nReferenceRecordOffset =
                nReferenceOffset + ((qint64)j * 12);
            const uchar *pReference = pMap + nReferenceRecordOffset;
            const qint16 nResourceId =
                (qint16)qFromBigEndian<quint16>(pReference);
            const quint16 nResourceNameOffset =
                qFromBigEndian<quint16>(pReference + 2);
            const qint64 nRelativeDataOffset = ((qint64)pReference[5] << 16) |
                                               ((qint64)pReference[6] << 8) |
                                               pReference[7];
            if (!rangeWithin(nDataSize, nRelativeDataOffset, 4))
                return false;
            const QByteArray baResourceLength = read_array_process(
                nDataOffset + nRelativeDataOffset, 4, pPdStruct);
            if (!guardedThis || (baResourceLength.size() != 4))
                return false;
            const qint64 nResourceSize = qFromBigEndian<quint32>(
                reinterpret_cast<const uchar *>(baResourceLength.constData()));
            const qint64 nStoredResourceSize = 4 + nResourceSize;
            if (!rangeWithin(nDataSize, nRelativeDataOffset,
                             nStoredResourceSize))
                return false;
            for (const QPair<qint64, qint64> &range : listDataRanges)
            {
                if (rangesOverlap(nRelativeDataOffset, nStoredResourceSize,
                                  range.first, range.second))
                    return false;
            }
            listDataRanges.append(
                qMakePair(nRelativeDataOffset, nStoredResourceSize));

            const uchar *pResourceName = nullptr;
            qint32 nResourceNameSize = 0;
            if (nResourceNameOffset != 0xffffU)
            {
                const qint64 nNameOffset =
                    nNameListOffset + nResourceNameOffset;
                if (!rangeWithin(nMapSize, nNameOffset, 1))
                    return false;
                nResourceNameSize = pMap[nNameOffset];
                if (!rangeWithin(nMapSize, nNameOffset + 1, nResourceNameSize))
                    return false;
                pResourceName = pMap + nNameOffset + 1;
            }

            const QString sSourceName =
                sTypePath + QLatin1Char('/') +
                resourceLeafName(nResourceId, pResourceName, nResourceNameSize);
            QString sUniqueName;
            if (!makeUniquePath(sSourceName, &stUsedFiles, &stUsedDirectories,
                                &mapNextSuffixes, &mapResolvedDirectories,
                                &sUniqueName))
                return false;
            if (pEntries)
            {
                ENTRY entry = {};
                entry.nHeaderOffset = nMapOffset + nReferenceRecordOffset;
                entry.nHeaderSize = 12;
                entry.nDataOffset = nDataOffset + nRelativeDataOffset + 4;
                entry.nDataSize = nResourceSize;
                entry.sFileName = sUniqueName;
                pEntries->append(entry);
            }
            nRecordCount++;
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    if (pArchiveEnd)
        *pArchiveEnd = nTotalSize;
    return true;
}
