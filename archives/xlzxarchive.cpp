/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xlzxarchive.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace
{
const qint64 LZX_HEADER_SIZE = 10;
const qint64 LZX_ENTRY_HEADER_SIZE = 31;
const qint64 LZX_MAX_GROUP_OUTPUT = 1024LL * 1024LL * 1024LL;

bool decodeLZXName(const QByteArray &data, QString *pName)
{
    if (!pName || data.isEmpty() || (data.size() > 255)) return false;
    for (char c : data)
    {
        const quint8 nByte = static_cast<quint8>(c);
        if ((nByte < 0x20) || ((nByte >= 0x7f) && (nByte <= 0x9f)))
            return false;
    }
    QString sName = QString::fromLatin1(data);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    // An Amiga volume prefix is a root designator, not a member directory.
    const qint32 nColon = sName.indexOf(QLatin1Char(':'));
    if (nColon >= 0) sName = sName.mid(nColon + 1);
    while (sName.startsWith(QLatin1Char('/'))) sName.remove(0, 1);
    if (sName.isEmpty()) return false;
    const QStringList parts = sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &part : parts)
    {
        if (part.isEmpty() || (part == QLatin1String(".")) ||
            (part == QLatin1String("..")))
            return false;
    }
    sName = sName.normalized(QString::NormalizationForm_C);
    if (XBinary::fixFileName(sName) != sName) return false;
    *pName = sName;
    return true;
}
}  // namespace

XLZXArchive::XLZXArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_AMIGA_LZX)
{
}

bool XLZXArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XLZXArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XLZXArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                     XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XLZXArchive(pDevice);
}

bool XLZXArchive::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                             PDSTRUCT *pPdStruct)
{
    QPointer<XLZXArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < (LZX_HEADER_SIZE + LZX_ENTRY_HEADER_SIZE)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    const QByteArray archiveHeader = read_array_process(0, LZX_HEADER_SIZE,
                                                        pPdStruct);
    if (!guardedThis || (archiveHeader.size() != LZX_HEADER_SIZE) ||
        (std::memcmp(archiveHeader.constData(), "LZX", 3) != 0))
        return false;

    QList<ENTRY> pending;
    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;
    qint64 nOffset = LZX_HEADER_SIZE;
    qint64 nGroupSize = 0;
    qint64 nGroupIndex = 0;
    qint64 nRecords = 0;
    while (nOffset < nTotalSize)
    {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            ((nTotalSize - nOffset) < LZX_ENTRY_HEADER_SIZE) ||
            (++nRecords > MAX_RECORDS))
            return false;
        const QByteArray header = read_array_process(
            nOffset, LZX_ENTRY_HEADER_SIZE, pPdStruct);
        if (!guardedThis || (header.size() != LZX_ENTRY_HEADER_SIZE))
            return false;
        const uchar *p = reinterpret_cast<const uchar *>(header.constData());
        const qint64 nFileSize = qFromLittleEndian<quint32>(p + 2);
        const qint64 nCompressedSize = qFromLittleEndian<quint32>(p + 6);
        const quint8 nMethod = p[11];
        const qint64 nCommentLength = p[14];
        const quint32 nDataCRC = qFromLittleEndian<quint32>(p + 22);
        const qint64 nNameLength = p[30];
        if ((nFileSize < 0) || (nNameLength <= 0) ||
            ((nMethod != 0) && (nMethod != 2)) ||
            (nNameLength > (nTotalSize - nOffset - LZX_ENTRY_HEADER_SIZE)) ||
            (nCommentLength > (nTotalSize - nOffset - LZX_ENTRY_HEADER_SIZE - nNameLength)))
            return false;
        const qint64 nVariableSize = nNameLength + nCommentLength;
        const QByteArray nameData = read_array_process(
            nOffset + LZX_ENTRY_HEADER_SIZE, nNameLength, pPdStruct);
        QString sName;
        if (!guardedThis || (nameData.size() != nNameLength) ||
            !decodeLZXName(nameData, &sName))
            return false;
        QString sUniqueName;
        if (!makeUniquePath(sName, &usedFiles, &usedDirectories,
                            &nextSuffixes, &resolvedDirectories,
                            &sUniqueName))
            return false;

        if ((nGroupSize > LZX_MAX_GROUP_OUTPUT) ||
            (nFileSize > (LZX_MAX_GROUP_OUTPUT - nGroupSize)))
            return false;
        ENTRY entry;
        entry.nHeaderOffset = nOffset;
        entry.nHeaderSize = LZX_ENTRY_HEADER_SIZE + nVariableSize;
        entry.nUncompressedSize = nFileSize;
        entry.nSubstreamOffset = nGroupSize;
        entry.bIsSolid = true;
        entry.bCRC32Defined = true;
        entry.nCRC32 = nDataCRC;
        entry.sFileName = sUniqueName;
        pending.append(entry);
        nGroupSize += nFileSize;

        const qint64 nDataOffset = nOffset + LZX_ENTRY_HEADER_SIZE +
                                   nVariableSize;
        if ((nCompressedSize < 0) ||
            !rangeWithin(nTotalSize, nDataOffset, nCompressedSize))
            return false;
        if (nCompressedSize > 0)
        {
            if ((nMethod == 0) && (nCompressedSize != nGroupSize))
                return false;
            if ((nMethod == 2) && (nCompressedSize & 1)) return false;
            for (ENTRY &groupEntry : pending)
            {
                groupEntry.nDataOffset = nDataOffset;
                groupEntry.nDataSize = nCompressedSize;
                groupEntry.nStreamUnpackedSize = nGroupSize;
                groupEntry.nSolidFolderIndex = nGroupIndex;
                groupEntry.handleMethod = (nMethod == 0)
                    ? HANDLE_METHOD_STORE : HANDLE_METHOD_AMIGA_LZX;
                if (pEntries) pEntries->append(groupEntry);
            }
            pending.clear();
            nGroupSize = 0;
            ++nGroupIndex;
        }
        nOffset = nDataOffset + nCompressedSize;
    }

    if (!pending.isEmpty() || (nRecords == 0) || (nOffset != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    if (pArchiveEnd) *pArchiveEnd = nOffset;
    return true;
}
