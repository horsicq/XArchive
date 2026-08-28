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
#include "xgrp.h"

#include <cstring>

XGRP::XGRP(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_BUILD_GRP)
{
}

bool XGRP::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XGRP archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XGRP::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XGRP(pDevice);
}

bool XGRP::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XGRP> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 16) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baHeader = read_array_process(0, 16, pPdStruct);
    if (!guardedThis || (baHeader.size() != 16) ||
        (memcmp(baHeader.constData(), "KenSilverman", 12) != 0)) {
        return false;
    }

    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    const quint32 nRecordCountValue = readLE32(pHeader + 12);
    if (nRecordCountValue > (quint32)MAX_RECORDS) return false;
    const qint32 nRecordCount = (qint32)nRecordCountValue;
    const qint64 nDirectorySize = (qint64)nRecordCount * 16;
    const qint64 nDataStart = 16 + nDirectorySize;
    if (!rangeWithin(nTotalSize, 16, nDirectorySize)) return false;

    QByteArray baDirectory;
    if (nDirectorySize > 0) {
        baDirectory = read_array_process(16, nDirectorySize, pPdStruct);
        if (!guardedThis || (baDirectory.size() != nDirectorySize))
            return false;
    }

    QSet<QString> stUsedFiles;
    QSet<QString> stUsedDirectories;
    QHash<QString, qint32> mapNextSuffixes;
    QHash<QString, QString> mapResolvedDirectories;
    qint64 nDataOffset = nDataStart;
    const uchar *pDirectory = reinterpret_cast<const uchar *>(
        baDirectory.constData());
    for (qint32 i = 0; i < nRecordCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const uchar *pRecord = pDirectory + ((qint64)i * 16);
        const qint64 nDataSize = readLE32(pRecord + 12);
        QString sName;
        QString sUniqueName;
        if (!decodeName(pRecord, 12, false, &sName) ||
            !makeUniquePath(sName, &stUsedFiles, &stUsedDirectories,
                            &mapNextSuffixes, &mapResolvedDirectories,
                            &sUniqueName) ||
            !rangeWithin(nTotalSize, nDataOffset, nDataSize)) {
            return false;
        }

        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = 16 + ((qint64)i * 16);
            entry.nHeaderSize = 16;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sFileName = sUniqueName;
            pEntries->append(entry);
        }
        nDataOffset += nDataSize;
    }

    // GRP has no offsets or footer: every byte after the table belongs to the
    // sequential member stream.  Equality rejects both truncation and junk.
    if ((nDataOffset != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (pArchiveEnd) *pArchiveEnd = nDataOffset;
    return true;
}
