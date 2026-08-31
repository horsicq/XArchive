/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xmacbinary.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace
{
bool align128(qint64 nValue, qint64 *pResult)
{
    if (!pResult || (nValue < 0) ||
        (nValue > (std::numeric_limits<qint64>::max() - 127)))
    {
        return false;
    }
    *pResult = (nValue + 127) & ~((qint64)127);
    return true;
}

quint16 macBinaryCRC(const uchar *pData, qint32 nSize)
{
    quint16 nCRC = 0;
    for (qint32 i = 0; i < nSize; ++i)
    {
        nCRC ^= (quint16)pData[i] << 8;
        for (qint32 j = 0; j < 8; ++j)
        {
            nCRC = (nCRC & 0x8000U) ? (quint16)((nCRC << 1) ^ 0x1021U)
                                    : (quint16)(nCRC << 1);
        }
    }
    return nCRC;
}

bool decodeMacBinaryName(const uchar *pHeader, QString *pName)
{
    if (!pHeader || !pName)
        return false;
    const qint32 nLength = pHeader[1];
    if ((nLength < 1) || (nLength > 63))
        return false;

    QString sName = QString::fromLatin1(
        reinterpret_cast<const char *>(pHeader + 2), nLength);
    sName.replace(QLatin1Char('/'), QLatin1Char('_'));
    sName.replace(QLatin1Char('\\'), QLatin1Char('_'));
    sName = XBinary::fixFileName(sName);
    if (sName.isEmpty() || sName.contains(QLatin1Char('/')))
        return false;
    *pName = sName;
    return true;
}
} // namespace

struct XMacBinary::ENTRY_CONTEXT
{
    QList<ENTRY> *pEntries;
    QSet<QString> *pUsedFiles;
    QSet<QString> *pUsedDirectories;
    QHash<QString, qint32> *pNextSuffixes;
    QHash<QString, QString> *pResolvedDirectories;
};

XMacBinary::XMacBinary(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_MACBINARY)
{
}

bool XMacBinary::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMacBinary archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMacBinary::createInstance(QIODevice *pDevice, bool bIsImage,
                                    XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMacBinary(pDevice);
}

bool XMacBinary::appendEntry(ENTRY_CONTEXT *pContext,
                             const QString &sSourceName, qint64 nOffset,
                             qint64 nSize)
{
    if (!pContext) return false;
    if (nSize == 0) return true;
    QString sUniqueName;
    if (!makeUniquePath(sSourceName, pContext->pUsedFiles,
                        pContext->pUsedDirectories,
                        pContext->pNextSuffixes,
                        pContext->pResolvedDirectories, &sUniqueName))
        return false;
    if (pContext->pEntries)
    {
        ENTRY entry = {};
        entry.nHeaderOffset = 0;
        entry.nHeaderSize = 128;
        entry.nDataOffset = nOffset;
        entry.nDataSize = nSize;
        entry.sFileName = sUniqueName;
        pContext->pEntries->append(entry);
    }
    return true;
}

bool XMacBinary::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                            PDSTRUCT *pPdStruct)
{
    QPointer<XMacBinary> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 128) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const QByteArray baHeader = read_array_process(0, 128, pPdStruct);
    if (!guardedThis || (baHeader.size() != 128))
        return false;
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if ((pHeader[0] != 0) || (pHeader[74] != 0) || (pHeader[82] != 0))
        return false;

    QString sBaseName;
    if (!decodeMacBinaryName(pHeader, &sBaseName))
        return false;

    const quint8 nVersion = pHeader[122];
    const quint8 nMinimumVersion = pHeader[123];
    const quint16 nStoredCRC = qFromBigEndian<quint16>(pHeader + 124);
    if (nVersion == 0)
    {
        if ((nMinimumVersion != 0) || (nStoredCRC != 0))
            return false;
    }
    else if (nVersion == 129)
    {
        if ((nMinimumVersion != 129) ||
            (nStoredCRC != macBinaryCRC(pHeader, 124)))
            return false;
    }
    else if (nVersion == 130)
    {
        if (memcmp(pHeader + 102, "mBIN", 4) != 0 ||
            ((nMinimumVersion != 0) && (nMinimumVersion != 129) &&
             (nMinimumVersion != 130)) ||
            (nStoredCRC != macBinaryCRC(pHeader, 124)))
            return false;
    }
    else
    {
        return false;
    }

    const qint64 nDataSize = qFromBigEndian<quint32>(pHeader + 83);
    const qint64 nResourceSize = qFromBigEndian<quint32>(pHeader + 87);
    const qint64 nCommentSize = qFromBigEndian<quint16>(pHeader + 99);
    const qint64 nSecondarySize = qFromBigEndian<quint16>(pHeader + 120);
    if ((nDataSize == 0) && (nResourceSize == 0) && (nCommentSize == 0))
        return false;

    qint64 nSecondaryPadded = 0;
    qint64 nDataPadded = 0;
    qint64 nResourcePadded = 0;
    qint64 nCommentPadded = 0;
    if (!align128(nSecondarySize, &nSecondaryPadded) ||
        !align128(nDataSize, &nDataPadded) ||
        !align128(nResourceSize, &nResourcePadded) ||
        !align128(nCommentSize, &nCommentPadded))
        return false;

    const qint64 nDataOffset = 128 + nSecondaryPadded;
    if (!rangeWithin(nTotalSize, nDataOffset, nDataPadded))
        return false;
    const qint64 nResourceOffset = nDataOffset + nDataPadded;
    if (!rangeWithin(nTotalSize, nResourceOffset, nResourcePadded))
        return false;
    const qint64 nCommentOffset = nResourceOffset + nResourcePadded;
    if (!rangeWithin(nTotalSize, nCommentOffset, nCommentPadded))
        return false;
    const qint64 nArchiveEnd = nCommentOffset + nCommentPadded;

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    ENTRY_CONTEXT entryContext = {pEntries, &stUsedFiles,
                                  &stUsedDirectories, &mapNextSuffixes,
                                  &mapResolvedDirectories};

    if (!appendEntry(&entryContext, sBaseName, nDataOffset, nDataSize) ||
        !appendEntry(&entryContext, sBaseName + QStringLiteral(".rsrc"),
                     nResourceOffset, nResourceSize) ||
        !appendEntry(&entryContext, sBaseName + QStringLiteral(".comment"),
                     nCommentOffset, nCommentSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    if (pArchiveEnd)
        *pArchiveEnd = nArchiveEnd;
    return true;
}
