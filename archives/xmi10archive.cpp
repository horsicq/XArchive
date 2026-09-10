/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xmi10archive.h"

#include <QtEndian>

#include <cstring>
#include <limits>

namespace
{
const qint64 MI10_HEADER_SIZE = 16;
const qint64 MI10_STREAM_PREFIX_SIZE = 2;
const qint64 MI10_MAX_PADDING = 2;
}

XMI10Archive::XMI10Archive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_MI10)
{
}

bool XMI10Archive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMI10Archive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMI10Archive::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMI10Archive(pDevice);
}

bool XMI10Archive::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                              PDSTRUCT *pPdStruct)
{
    QPointer<XMI10Archive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < (MI10_HEADER_SIZE +
                                       MI10_STREAM_PREFIX_SIZE + 1)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    qint64 nOffset = 0;
    qint64 nRecords = 0;
    while (nOffset < nTotalSize)
    {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            ((nOffset & 1) != 0) ||
            ((nTotalSize - nOffset) < MI10_HEADER_SIZE) ||
            (++nRecords > MAX_RECORDS))
            return false;

        const QByteArray header = read_array_process(
            nOffset, MI10_HEADER_SIZE, pPdStruct);
        if (!guardedThis || (header.size() != MI10_HEADER_SIZE) ||
            (std::memcmp(header.constData(), "MI10", 4) != 0))
            return false;
        const uchar *p = reinterpret_cast<const uchar *>(header.constData());
        const quint32 nChecksum = qFromBigEndian<quint32>(p + 4);
        const quint32 nUncompressedSize = qFromBigEndian<quint32>(p + 8);
        const quint32 nDeclaredPackedSize = qFromBigEndian<quint32>(p + 12);
        if ((nUncompressedSize == 0) || (nDeclaredPackedSize == 0) ||
            (nUncompressedSize >
             static_cast<quint32>((std::numeric_limits<qint32>::max)())) ||
            (nDeclaredPackedSize >
             static_cast<quint32>((std::numeric_limits<qint32>::max)() -
                                  MI10_STREAM_PREFIX_SIZE)) ||
            (static_cast<quint64>(nChecksum) >
             static_cast<quint64>(nUncompressedSize) * 255ULL))
            return false;

        const qint64 nDataOffset = nOffset + MI10_HEADER_SIZE;
        const qint64 nDataSize =
            static_cast<qint64>(nDeclaredPackedSize) +
            MI10_STREAM_PREFIX_SIZE;
        if (!rangeWithin(nTotalSize, nDataOffset, nDataSize)) return false;

        // The first byte is invariant in all known streams and sharply reduces
        // collisions with arbitrary files beginning with the printable magic.
        const QByteArray prefix = read_array_process(
            nDataOffset, MI10_STREAM_PREFIX_SIZE, pPdStruct);
        if (!guardedThis || (prefix.size() != MI10_STREAM_PREFIX_SIZE) ||
            (static_cast<quint8>(prefix.at(0)) != 0))
            return false;

        ENTRY entry;
        entry.nHeaderOffset = nOffset;
        entry.nHeaderSize = MI10_HEADER_SIZE;
        entry.nDataOffset = nDataOffset;
        entry.nDataSize = nDataSize;
        entry.nUncompressedSize = nUncompressedSize;
        entry.handleMethod = HANDLE_METHOD_MI10;
        entry.bIsSolid = false;
        entry.sChecksum = QStringLiteral("%1").arg(
            nChecksum, 8, 16, QLatin1Char('0'));
        entry.sChecksumType = QStringLiteral("MI10 byte sum");
        entry.sFileName = QStringLiteral("%1.bin").arg(nRecords);
        if (pEntries) pEntries->append(entry);

        qint64 nNext = nDataOffset + nDataSize;
        if (nNext == nTotalSize)
        {
            nOffset = nNext;
            break;
        }

        // Encoders use zero, one, or two 0x6b pad bytes between blocks (and
        // occasionally after the final block).  Do not perform a loose magic
        // search: every skipped byte must be authenticated padding.
        qint64 nPadding = 0;
        while ((nNext < nTotalSize) &&
               (nPadding < MI10_MAX_PADDING) &&
               (read_uint8(nNext) == 0x6b))
        {
            ++nNext;
            ++nPadding;
        }
        if (nNext == nTotalSize)
        {
            nOffset = nNext;
            break;
        }
        if (((nNext & 1) != 0) || ((nTotalSize - nNext) < 4) ||
            !compareSignature("'MI10'", nNext))
            return false;
        nOffset = nNext;
    }

    if ((nRecords == 0) || (nOffset != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    if (pArchiveEnd) *pArchiveEnd = nOffset;
    return true;
}
