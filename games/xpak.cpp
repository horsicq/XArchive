/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xpak.h"

#include <cstring>

XPAK::XPAK(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_QUAKE_PAK)
{
}

bool XPAK::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPAK archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPAK::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPAK(pDevice);
}

bool XPAK::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XPAK> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 12) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baHeader = read_array_process(0, 12, pPdStruct);
    if (!guardedThis || (baHeader.size() != 12) ||
        (memcmp(baHeader.constData(), "PACK", 4) != 0)) return false;

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const qint64 nDirectoryOffset = readLE32(pHeader + 4);
    const qint64 nDirectorySize = readLE32(pHeader + 8);
    if ((nDirectoryOffset < 12) || ((nDirectorySize % 64) != 0) ||
        !rangeWithin(nTotalSize, nDirectoryOffset, nDirectorySize)) {
        return false;
    }

    const qint64 nRecordCount64 = nDirectorySize / 64;
    if (nRecordCount64 > MAX_RECORDS) return false;
    const qint32 nRecordCount = (qint32)nRecordCount64;
    // With no directory records there is nothing else that can validate an
    // apparent PACK signature.  Accept only the canonical empty container;
    // non-empty archives may still have an overlay after their logical end.
    if ((nRecordCount == 0) &&
        ((nDirectoryOffset != 12) || (nTotalSize != 12))) {
        return false;
    }

    QByteArray baDirectory;
    if (nDirectorySize > 0) {
        baDirectory = read_array_process(nDirectoryOffset, nDirectorySize,
                                         pPdStruct);
        if (!guardedThis || (baDirectory.size() != nDirectorySize))
            return false;
    }

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    qint64 nArchiveEnd = nDirectoryOffset + nDirectorySize;
    const uchar *pDirectory = reinterpret_cast<const uchar *>(
        baDirectory.constData());
    for (qint32 i = 0; i < nRecordCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pRecord = pDirectory + ((qint64)i * 64);
        QString sName;
        QString sUniqueName;
        if (!decodeName(pRecord, 56, false, &sName) ||
            !makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName)) {
            return false;
        }

        const qint64 nDataOffset = readLE32(pRecord + 56);
        const qint64 nDataSize = readLE32(pRecord + 60);
        if (!rangeWithin(nTotalSize, nDataOffset, nDataSize) ||
            ((nDataSize > 0) && (nDataOffset < 12)) ||
            rangesOverlap(nDataOffset, nDataSize,
                          nDirectoryOffset, nDirectorySize)) {
            return false;
        }

        if (nDataSize > 0) {
            nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nDataSize);
        }
        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = nDirectoryOffset + ((qint64)i * 64);
            entry.nHeaderSize = 64;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
    }

    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
