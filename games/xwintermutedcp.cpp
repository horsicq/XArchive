/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xwintermutedcp.h"

#include <QtEndian>

#include <cstring>

namespace
{
bool readByteString(const QByteArray &baDirectory, qint64 *pOffset,
                    QByteArray *pValue)
{
    if (!pOffset || !pValue || (*pOffset < 0) ||
        (*pOffset >= baDirectory.size()))
        return false;
    const quint8 nSize =
        static_cast<quint8>(baDirectory.at(static_cast<int>(*pOffset)));
    ++*pOffset;
    if (nSize > (baDirectory.size() - *pOffset)) return false;
    *pValue = baDirectory.mid(static_cast<int>(*pOffset), nSize);
    *pOffset += nSize;
    while (!pValue->isEmpty() && pValue->endsWith('\0')) pValue->chop(1);
    return !pValue->isEmpty();
}

QString decodeDCPName(const QByteArray &baName)
{
    QString sName = QString::fromLatin1(baName).normalized(
        QString::NormalizationForm_C);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    return sName;
}
} // namespace

XWintermuteDCP::XWintermuteDCP(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_WINTERMUTE_DCP)
{
}

bool XWintermuteDCP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWintermuteDCP archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWintermuteDCP::createInstance(QIODevice *pDevice, bool bIsImage,
                                        XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWintermuteDCP(pDevice);
}

bool XWintermuteDCP::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                                PDSTRUCT *pPdStruct)
{
    QPointer<XWintermuteDCP> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 128) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    const QByteArray baHeader = read_array_process(0, 132, pPdStruct);
    if (!guardedThis || (baHeader.size() < 128) ||
        (std::memcmp(baHeader.constData(), "\xde\xad\xc0\xdeJUNK", 8) != 0))
        return false;
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const quint32 nVersion = qFromLittleEndian<quint32>(pHeader + 8);
    const quint32 nDirectoryCount = qFromLittleEndian<quint32>(pHeader + 124);
    if (((nVersion != 0x100U) && (nVersion != 0x200U)) ||
        (nDirectoryCount == 0) || (nDirectoryCount > MAX_RECORDS))
        return false;

    qint64 nDirectoryOffset = 128;
    if (nVersion == 0x200U)
    {
        if (baHeader.size() < 132) return false;
        nDirectoryOffset = qFromLittleEndian<quint32>(pHeader + 128);
    }
    if ((nDirectoryOffset < 128) || (nDirectoryOffset >= nTotalSize))
        return false;

    const qint64 nDirectorySize = nTotalSize - nDirectoryOffset;
    if (nDirectorySize > (256LL * 1024LL * 1024LL)) return false;
    const QByteArray baDirectory =
        read_array_process(nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedThis || (baDirectory.size() != nDirectorySize)) return false;

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    qint64 nPosition = 0;
    qint64 nArchiveEnd = nDirectoryOffset;
    quint64 nTotalRecords = 0;
    for (quint32 i = 0; i < nDirectoryCount; ++i)
    {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        QByteArray baDirectoryName;
        if (!readByteString(baDirectory, &nPosition, &baDirectoryName) ||
            (nPosition > baDirectory.size() - 5))
            return false;
        const quint32 nFileCount = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(baDirectory.constData()) +
            nPosition + 1);
        nPosition += 5; // media/CD byte plus little-endian entry count
        if ((nFileCount > MAX_RECORDS) ||
            (nTotalRecords + nFileCount > MAX_RECORDS))
            return false;
        nTotalRecords += nFileCount;
        const QString sDirectory = decodeDCPName(baDirectoryName);

        for (quint32 j = 0; j < nFileCount; ++j)
        {
            QByteArray baEncodedName;
            if (!readByteString(baDirectory, &nPosition, &baEncodedName))
                return false;
            // Version 1 packages store names verbatim; version 2 obfuscates
            // every filename byte with 0x44.
            if (nVersion == 0x200U)
                for (qint32 k = 0; k < baEncodedName.size(); ++k)
                    baEncodedName[k] = static_cast<char>(baEncodedName.at(k) ^ 0x44);
            while (!baEncodedName.isEmpty() && baEncodedName.endsWith('\0'))
                baEncodedName.chop(1);
            if (baEncodedName.isEmpty()) return false;
            const qint64 nRecordSize = (nVersion == 0x200U) ? 24 : 16;
            if (nPosition > baDirectory.size() - nRecordSize) return false;
            const uchar *pRecord = reinterpret_cast<const uchar *>(
                baDirectory.constData() + nPosition);
            const qint64 nDataOffset = qFromLittleEndian<quint32>(pRecord);
            const qint64 nRawSize = qFromLittleEndian<quint32>(pRecord + 4);
            const qint64 nCompressedSize =
                qFromLittleEndian<quint32>(pRecord + 8);
            const qint64 nStoredSize = nCompressedSize ? nCompressedSize : nRawSize;
            const qint64 nRecordOffset = nDirectoryOffset + nPosition;
            nPosition += nRecordSize;
            if ((nRawSize < 0) || (nStoredSize < 0) ||
                !rangeWithin(nTotalSize, nDataOffset, nStoredSize))
                return false;

            QString sPath = decodeDCPName(baEncodedName);
            if (!sDirectory.isEmpty()) sPath = sDirectory + QLatin1Char('/') + sPath;
            QString sUniquePath;
            if (!makeUniquePath(sPath, &stUsedFiles, &stUsedDirectories,
                                &mapNextSuffixes, &mapResolvedDirectories,
                                &sUniquePath))
                return false;
            if (pEntries)
            {
                ENTRY entry = {};
                entry.nHeaderOffset = nRecordOffset;
                entry.nHeaderSize = nRecordSize;
                entry.nDataOffset = nDataOffset;
                entry.nDataSize = nStoredSize;
                entry.nUncompressedSize = nRawSize;
                entry.handleMethod = nCompressedSize ? HANDLE_METHOD_ZLIB
                                                     : HANDLE_METHOD_STORE;
                entry.sFileName = sUniquePath;
                pEntries->append(entry);
            }
            nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nStoredSize);
        }
    }

    nArchiveEnd = qMax(nArchiveEnd, nDirectoryOffset + nPosition);
    if ((nTotalRecords == 0) || (nArchiveEnd != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}
