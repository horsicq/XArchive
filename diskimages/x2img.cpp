/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "x2img.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace
{
bool add2ImgOptionalRange(qint64 nTotalSize, qint64 nHeaderSize,
                          qint64 nOffset, qint64 nSize,
                          QList<QPair<qint64, qint64> > *pRanges)
{
    if (!pRanges)
        return false;
    if ((nOffset == 0) && (nSize == 0))
        return true;
    if ((nOffset < nHeaderSize) || (nSize <= 0) ||
        (nOffset > nTotalSize) || (nSize > nTotalSize - nOffset))
        return false;

    for (const QPair<qint64, qint64> &range : *pRanges)
    {
        if ((nOffset < range.first + range.second) &&
            (range.first < nOffset + nSize))
            return false;
    }

    pRanges->append(qMakePair(nOffset, nSize));
    return true;
}
}  // namespace

X2IMG::X2IMG(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_APPLE_2IMG)
{
}

bool X2IMG::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    X2IMG archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *X2IMG::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new X2IMG(pDevice);
}

bool X2IMG::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                       PDSTRUCT *pPdStruct)
{
    QPointer<X2IMG> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 64) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const QByteArray baHeader = read_array_process(0, 64, pPdStruct);
    if (!guardedThis || (baHeader.size() != 64) ||
        (std::memcmp(baHeader.constData(), "2IMG", 4) != 0))
        return false;
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const quint16 nHeaderSize = qFromLittleEndian<quint16>(pHeader + 8);
    const quint16 nVersion = qFromLittleEndian<quint16>(pHeader + 10);
    const quint32 nImageFormat = qFromLittleEndian<quint32>(pHeader + 12);
    const quint32 nBlocks = qFromLittleEndian<quint32>(pHeader + 20);
    const qint64 nDataOffset = qFromLittleEndian<quint32>(pHeader + 24);
    const qint64 nDeclaredDataSize =
        qFromLittleEndian<quint32>(pHeader + 28);
    qint64 nDataSize = nDeclaredDataSize;
    const qint64 nCommentOffset = qFromLittleEndian<quint32>(pHeader + 32);
    const qint64 nCommentSize = qFromLittleEndian<quint32>(pHeader + 36);
    const qint64 nCreatorOffset = qFromLittleEndian<quint32>(pHeader + 40);
    const qint64 nCreatorSize = qFromLittleEndian<quint32>(pHeader + 44);

    if ((nHeaderSize < 64) || (nHeaderSize > nTotalSize) ||
        (nVersion > 1) || (nImageFormat > 2) || (nBlocks == 0) ||
        (nDataOffset < nHeaderSize) || (nDataOffset > nTotalSize))
        return false;

    // A number of widely circulated XGS-created images contain a zero or
    // stale data-length field while retaining a correct block count.  The
    // block count is independently bounded by the physical file and is the
    // canonical size for DOS/ProDOS sector images, so recover those images
    // without permitting a read past the declared container.
    if (nImageFormat <= 1)
    {
        const quint64 nBlockBytes = static_cast<quint64>(nBlocks) * 512U;
        if ((nBlockBytes > static_cast<quint64>(std::numeric_limits<qint64>::max())) ||
            (nBlockBytes > static_cast<quint64>(nTotalSize - nDataOffset)))
            return false;
        if (static_cast<quint64>(nDataSize) != nBlockBytes)
            nDataSize = static_cast<qint64>(nBlockBytes);
    }
    if ((nDataSize <= 0) || !rangeWithin(nTotalSize, nDataOffset, nDataSize))
        return false;

    QList<QPair<qint64, qint64> > listRanges;
    listRanges.append(qMakePair(nDataOffset, nDataSize));
    if (!add2ImgOptionalRange(nTotalSize, nHeaderSize, nCommentOffset,
                              nCommentSize, &listRanges) ||
        !add2ImgOptionalRange(nTotalSize, nHeaderSize, nCreatorOffset,
                              nCreatorSize, &listRanges))
        return false;

    if (pEntries)
    {
        QString sBaseName =
            XBinary::fixFileName(XBinary::getDeviceFileBaseName(getDevice()));
        if (sBaseName.isEmpty()) sBaseName = QStringLiteral("disk-image");
        ENTRY image = {};
        image.nHeaderOffset = 0;
        image.nHeaderSize = nHeaderSize;
        image.nDataOffset = nDataOffset;
        image.nDataSize = nDataSize;
        image.sFileName = sBaseName +
                          (nImageFormat == 2 ? QStringLiteral(".nib")
                                             : QStringLiteral(".dsk"));
        pEntries->append(image);
        if (nCommentSize > 0)
        {
            ENTRY comment = {};
            comment.nHeaderOffset = 32;
            comment.nHeaderSize = 8;
            comment.nDataOffset = nCommentOffset;
            comment.nDataSize = nCommentSize;
            comment.sFileName = QStringLiteral("comment.txt");
            pEntries->append(comment);
        }
        if (nCreatorSize > 0)
        {
            ENTRY creator = {};
            creator.nHeaderOffset = 40;
            creator.nHeaderSize = 8;
            creator.nDataOffset = nCreatorOffset;
            creator.nDataSize = nCreatorSize;
            creator.sFileName = QStringLiteral("creator-data.bin");
            pEntries->append(creator);
        }
    }

    qint64 nArchiveEnd = qMax(nDataOffset + nDataSize,
                              static_cast<qint64>(nHeaderSize));
    if (nCommentSize > 0) nArchiveEnd = qMax(nArchiveEnd, nCommentOffset + nCommentSize);
    if (nCreatorSize > 0) nArchiveEnd = qMax(nArchiveEnd, nCreatorOffset + nCreatorSize);
    if ((nArchiveEnd != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}
