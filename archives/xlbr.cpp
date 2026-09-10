/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xlbr.h"

#include <QtEndian>

namespace
{
bool decodeCPMComponent(const uchar *pData, qint32 nSize, QString *pResult)
{
    if (!pData || !pResult || (nSize <= 0))
        return false;
    qint32 nLength = nSize;
    while ((nLength > 0) && (pData[nLength - 1] == ' '))
        nLength--;
    if (nLength == 0)
        return false;
    for (qint32 i = 0; i < nLength; ++i)
    {
        const quint8 nCharacter = pData[i] & 0x7fU;
        if ((nCharacter < 0x21) || (nCharacter > 0x7e) || (nCharacter == '/') ||
            (nCharacter == '\\'))
            return false;
    }
    for (qint32 i = nLength; i < nSize; ++i)
    {
        if (pData[i] != ' ')
            return false;
    }
    *pResult =
        QString::fromLatin1(reinterpret_cast<const char *>(pData), nLength);
    return true;
}
} // namespace

XLBR::XLBR(QIODevice *pDevice) : XGameStoreArchiveBase(pDevice, FT_CPM_LBR) {}

bool XLBR::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLBR archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLBR::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLBR(pDevice);
}

bool XLBR::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XLBR> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 128) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const QByteArray baControl = read_array_process(0, 32, pPdStruct);
    if (!guardedThis || (baControl.size() != 32))
        return false;
    const uchar *pControl =
        reinterpret_cast<const uchar *>(baControl.constData());
    if ((pControl[0] != 0) ||
        (QByteArray(reinterpret_cast<const char *>(pControl + 1), 11) !=
         QByteArray(11, ' ')) ||
        (qFromLittleEndian<quint16>(pControl + 12) != 0))
        return false;

    const qint64 nDirectorySectors = qFromLittleEndian<quint16>(pControl + 14);
    const qint64 nDirectorySize = nDirectorySectors * 128;
    const qint64 nDirectoryRecords = nDirectorySectors * 4;
    if ((nDirectorySectors == 0) || (nDirectoryRecords > MAX_RECORDS) ||
        !rangeWithin(nTotalSize, 0, nDirectorySize))
        return false;

    const QByteArray baDirectory =
        read_array_process(0, nDirectorySize, pPdStruct);
    if (!guardedThis || (baDirectory.size() != nDirectorySize))
        return false;
    const uchar *pDirectory =
        reinterpret_cast<const uchar *>(baDirectory.constData());

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    QList<QPair<qint64, qint64>> listRanges;
    bool bSawUnused = false;
    qint32 nSafeRecordCount = 0;
    qint64 nArchiveEnd = nDirectorySize;

    for (qint64 i = 1; i < nDirectoryRecords; ++i)
    {
        if (!XBinary::isPdStructNotCanceled(pPdStruct))
            return false;
        const qint64 nRecordOffset = i * 32;
        const uchar *pRecord = pDirectory + nRecordOffset;
        const quint8 nStatus = pRecord[0];
        if (nStatus == 0xffU)
        {
            bSawUnused = true;
            continue;
        }
        if (bSawUnused)
            return false;
        if (nStatus != 0)
            continue;

        QString sName;
        QString sExtension;
        if (!decodeCPMComponent(pRecord + 1, 8, &sName))
            continue;
        qint32 nExtensionLength = 3;
        while ((nExtensionLength > 0) &&
               (pRecord[9 + nExtensionLength - 1] == ' '))
        {
            nExtensionLength--;
        }
        if (nExtensionLength > 0)
        {
            if (!decodeCPMComponent(pRecord + 9, 3, &sExtension))
                continue;
            sName += QLatin1Char('.') + sExtension;
        }
        sName = XBinary::fixFileName(sName);
        if (sName.isEmpty() || sName.contains(QLatin1Char('/')))
            continue;

        const qint64 nStartSector = qFromLittleEndian<quint16>(pRecord + 12);
        const qint64 nSectorCount = qFromLittleEndian<quint16>(pRecord + 14);
        const qint64 nPadCount = pRecord[26];
        const qint64 nDataOffset = nStartSector * 128;
        const qint64 nPhysicalSize = nSectorCount * 128;
        if ((nPadCount > 127) || ((nSectorCount == 0) && (nPadCount != 0)) ||
            ((nSectorCount > 0) &&
             ((nStartSector < nDirectorySectors) ||
              !rangeWithin(nTotalSize, nDataOffset, nPhysicalSize))))
        {
            continue;
        }
        bool bOverlap = false;
        for (const QPair<qint64, qint64> &range : listRanges)
        {
            if (rangesOverlap(nDataOffset, nPhysicalSize, range.first,
                              range.second))
            {
                bOverlap = true;
                break;
            }
        }
        if (bOverlap)
            continue;
        if (nPhysicalSize > 0)
        {
            listRanges.append(qMakePair(nDataOffset, nPhysicalSize));
        }

        QString sUniqueName;
        if (!makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName))
            continue;
        if (pEntries)
        {
            ENTRY entry = {};
            entry.nHeaderOffset = nRecordOffset;
            entry.nHeaderSize = 32;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nPhysicalSize - nPadCount;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
        nSafeRecordCount++;
        nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nPhysicalSize);
    }

    if ((nSafeRecordCount == 0) || !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    if (pArchiveEnd)
        *pArchiveEnd = nArchiveEnd;
    return true;
}
