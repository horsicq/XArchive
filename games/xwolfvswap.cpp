/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xwolfvswap.h"

#include <QtEndian>

XWolfVSwap::XWolfVSwap(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_WOLF_VSWAP)
{
}

bool XWolfVSwap::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWolfVSwap archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWolfVSwap::createInstance(QIODevice *pDevice, bool bIsImage,
                                    XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWolfVSwap(pDevice);
}

bool XWolfVSwap::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                            PDSTRUCT *pPdStruct)
{
    QPointer<XWolfVSwap> guardedThis(this);
    const qint64 totalSize = getSize();
    if (!guardedThis || totalSize < 12 ||
        !isPdStructNotCanceled(pPdStruct))
        return false;
    const QByteArray fixed = read_array_process(0, 6, pPdStruct);
    if (!guardedThis || fixed.size() != 6) return false;
    const uchar *fixedData = reinterpret_cast<const uchar *>(fixed.constData());
    const quint16 count = qFromLittleEndian<quint16>(fixedData);
    const quint16 spriteStart = qFromLittleEndian<quint16>(fixedData + 2);
    const quint16 soundStart = qFromLittleEndian<quint16>(fixedData + 4);
    const qint64 tableSize = 6 + qint64(count) * 6;
    if (count == 0 || count > MAX_RECORDS || spriteStart > soundStart ||
        soundStart > count || !rangeWithin(totalSize, 0, tableSize))
        return false;
    const QByteArray table = read_array_process(6, qint64(count) * 6,
                                                pPdStruct);
    if (!guardedThis || table.size() != qint64(count) * 6) return false;
    const uchar *offsets = reinterpret_cast<const uchar *>(table.constData());
    const uchar *lengths = offsets + qint64(count) * 4;

    qint64 previousEnd = tableSize;
    qint32 presentCount = 0;
    for (quint32 i = 0; i < count; ++i) {
        const qint64 offset = qFromLittleEndian<quint32>(offsets + i * 4);
        const qint64 size = qFromLittleEndian<quint16>(lengths + i * 2);
        if (offset == 0 && size == 0) continue;
        if (offset < tableSize || size <= 0 ||
            !rangeWithin(totalSize, offset, size) || offset < previousEnd)
            return false;
        previousEnd = offset + size;
        ++presentCount;
        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = 6 + i * 4;
            entry.nHeaderSize = 6;
            entry.nDataOffset = offset;
            entry.nDataSize = size;
            const QString kind = i < spriteStart
                                     ? QStringLiteral("wall")
                                     : (i < soundStart
                                            ? QStringLiteral("sprite")
                                            : QStringLiteral("sound"));
            entry.sFileName = QStringLiteral("%1/%2_%3.bin")
                                  .arg(kind)
                                  .arg(kind)
                                  .arg(i, 4, 10, QLatin1Char('0'));
            pEntries->append(entry);
        }
    }
    // Some original WL1 data sets are sector-padded after their last chunk.
    // Accept only a small all-zero tail; arbitrary overlays must not turn a
    // random table-shaped file into a VSWAP archive.
    if (presentCount == 0 || previousEnd > totalSize ||
        totalSize - previousEnd > 4096 ||
        (previousEnd < totalSize &&
         read_array_process(previousEnd, totalSize - previousEnd, pPdStruct)
             .count(char(0)) != totalSize - previousEnd) ||
        !isPdStructNotCanceled(pPdStruct))
        return false;
    if (pArchiveEnd) *pArchiveEnd = totalSize;
    return true;
}
