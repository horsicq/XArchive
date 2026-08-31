/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xhog.h"

#include <cstring>

XHOG::XHOG(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_DESCENT_HOG)
{
}

bool XHOG::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XHOG archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XHOG::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XHOG(pDevice);
}

bool XHOG::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XHOG> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 20) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baMagic = read_array_process(0, 3, pPdStruct);
    if (!guardedThis || (baMagic.size() != 3) ||
        (memcmp(baMagic.constData(), "DHF", 3) != 0)) return false;

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    qint64 nOffset = 3;
    qint32 nRecordCount = 0;
    while (nOffset < nTotalSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) ||
            (nRecordCount >= MAX_RECORDS) ||
            !rangeWithin(nTotalSize, nOffset, 17)) {
            return false;
        }

        const QByteArray baHeader =
            read_array_process(nOffset, 17, pPdStruct);
        if (!guardedThis || (baHeader.size() != 17)) return false;
        const uchar *pHeader =
            reinterpret_cast<const uchar *>(baHeader.constData());
        const qint64 nDataSize = readLE32(pHeader + 13);
        const qint64 nDataOffset = nOffset + 17;
        QString sName;
        QString sUniqueName;
        if (!decodeName(pHeader, 13, true, &sName) ||
            !makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName) ||
            !rangeWithin(nTotalSize, nDataOffset, nDataSize)) {
            return false;
        }

        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = nOffset;
            entry.nHeaderSize = 17;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
        nOffset = nDataOffset + nDataSize;
        nRecordCount++;
    }

    if ((nRecordCount == 0) || (nOffset != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (pArchiveEnd) *pArchiveEnd = nOffset;
    return true;
}
