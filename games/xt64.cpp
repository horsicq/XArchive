/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xt64.h"

#include <QtEndian>

#include <cstring>

namespace {
bool decodeT64Name(const uchar *pData, QString *pName)
{
    if (!pData || !pName) return false;

    qint32 nLength = 16;
    while ((nLength > 0) && ((pData[nLength - 1] == 0) ||
                             (pData[nLength - 1] == 0x20) ||
                             (pData[nLength - 1] == 0xa0))) {
        nLength--;
    }
    if (nLength == 0) return false;
    for (qint32 i = 0; i < nLength; ++i) {
        if ((pData[i] < 0x20) || (pData[i] > 0x7e)) return false;
    }

    const QString sName = QString::fromLatin1(
        reinterpret_cast<const char *>(pData), nLength)
        .normalized(QString::NormalizationForm_C);
    if (sName.isEmpty() || (XBinary::fixFileName(sName) != sName))
        return false;
    *pName = sName;
    return true;
}
}  // namespace

XT64::XT64(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_C64_T64)
{
}

bool XT64::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XT64 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XT64::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XT64(pDevice);
}

bool XT64::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XT64> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 96) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baHeader = read_array_process(0, 64, pPdStruct);
    if (!guardedThis || (baHeader.size() != 64) ||
        ((memcmp(baHeader.constData(), "C64 tape image file", 19) != 0) &&
         (memcmp(baHeader.constData(), "C64S tape image file", 20) != 0))) {
        return false;
    }

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const quint16 nVersion = qFromLittleEndian<quint16>(pHeader + 32);
    const quint16 nMaxRecords = qFromLittleEndian<quint16>(pHeader + 34);
    const quint16 nUsedRecords = qFromLittleEndian<quint16>(pHeader + 36);
    if (((nVersion != 0x0100) && (nVersion != 0x0101)) ||
        (nMaxRecords == 0) || (nMaxRecords > MAX_RECORDS) ||
        (nUsedRecords == 0) || (nUsedRecords > nMaxRecords)) {
        return false;
    }

    const qint64 nDirectorySize = (qint64)nMaxRecords * 32;
    const qint64 nDataFloor = 64 + nDirectorySize;
    if (!rangeWithin(nTotalSize, 64, nDirectorySize)) return false;
    const QByteArray baDirectory =
        read_array_process(64, nDirectorySize, pPdStruct);
    if (!guardedThis || (baDirectory.size() != nDirectorySize)) return false;

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    const uchar *pDirectory =
        reinterpret_cast<const uchar *>(baDirectory.constData());
    qint32 nActiveRecords = 0;
    qint64 nArchiveEnd = nDataFloor;
    for (quint16 i = 0; i < nMaxRecords; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pRecord = pDirectory + ((qint64)i * 32);
        if (pRecord[0] == 0) continue;
        if (pRecord[0] != 1) return false;

        const quint16 nStartAddress =
            qFromLittleEndian<quint16>(pRecord + 2);
        const quint16 nEndAddress =
            qFromLittleEndian<quint16>(pRecord + 4);
        const qint64 nDataOffset =
            qFromLittleEndian<quint32>(pRecord + 8);
        if (nEndAddress < nStartAddress) return false;
        const qint64 nDataSize = nEndAddress - nStartAddress;
        QString sName;
        QString sUniqueName;
        if (!decodeT64Name(pRecord + 16, &sName) ||
            !makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName) ||
            (nDataOffset < nDataFloor) ||
            !rangeWithin(nTotalSize, nDataOffset, nDataSize)) {
            return false;
        }

        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = 64 + ((qint64)i * 32);
            entry.nHeaderSize = 32;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
        nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nDataSize);
        nActiveRecords++;
    }

    if ((nActiveRecords != nUsedRecords) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}
