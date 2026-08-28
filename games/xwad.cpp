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
#include "xwad.h"

#include <cstring>

XWAD::XWAD(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_DOOM_WAD)
{
}

bool XWAD::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWAD archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWAD::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWAD(pDevice);
}

bool XWAD::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XWAD> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 12) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baHeader = read_array_process(0, 12, pPdStruct);
    if (!guardedThis || (baHeader.size() != 12) ||
        ((memcmp(baHeader.constData(), "IWAD", 4) != 0) &&
         (memcmp(baHeader.constData(), "PWAD", 4) != 0))) {
        return false;
    }

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const quint32 nRecordCountValue = readLE32(pHeader + 4);
    if (nRecordCountValue > (quint32)MAX_RECORDS) return false;
    const qint32 nRecordCount = (qint32)nRecordCountValue;
    const qint64 nDirectoryOffset = readLE32(pHeader + 8);
    const qint64 nDirectorySize = (qint64)nRecordCount * 16;
    if ((nDirectoryOffset < 12) ||
        !rangeWithin(nTotalSize, nDirectoryOffset, nDirectorySize)) {
        return false;
    }
    // IWAD/PWAD alone is too weak a discriminator when no lump records are
    // present.  Require the exact canonical 12-byte empty WAD in that case.
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
        const uchar *pRecord = pDirectory + ((qint64)i * 16);
        const qint64 nDataOffset = readLE32(pRecord);
        const qint64 nDataSize = readLE32(pRecord + 4);
        QString sName;
        QString sUniqueName;
        // PC Doom lump names are 7-bit.  Rejecting the high bit also keeps
        // Jaguar's big-endian/compressed WAD dialect out of this reader.
        if (!decodeName(pRecord + 8, 8, true, &sName) ||
            !makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName) ||
            !rangeWithin(nTotalSize, nDataOffset, nDataSize) ||
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
            entry.nHeaderOffset = nDirectoryOffset + ((qint64)i * 16);
            entry.nHeaderSize = 16;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
    }

    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}
