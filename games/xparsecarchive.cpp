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
#include "xparsecarchive.h"

#include <cstring>

XParsecArchive::XParsecArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_PARSEC_ARCHIVE)
{
}

bool XParsecArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XParsecArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XParsecArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                        XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XParsecArchive(pDevice);
}

bool XParsecArchive::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                                PDSTRUCT *pPdStruct)
{
    QPointer<XParsecArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || (nTotalSize < 20) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray baFirstOffset = read_array_process(0, 4, pPdStruct);
    if (!guardedThis || (baFirstOffset.size() != 4)) return false;
    const uchar *pFirstOffset = reinterpret_cast<const uchar *>(
        baFirstOffset.constData());
    const quint32 nHeaderSizeValue = readLE32(pFirstOffset);

    // headerSize = N * sizeof(offset) + sizeof(zero sentinel) +
    //              N * sizeof(size) = 8*N + 4.
    if ((nHeaderSizeValue < 12) ||
        (((nHeaderSizeValue - 4) % 8) != 0) ||
        ((qint64)nHeaderSizeValue > nTotalSize)) {
        return false;
    }
    const quint32 nRecordCountValue = (nHeaderSizeValue - 4) / 8;
    if ((nRecordCountValue == 0) ||
        (nRecordCountValue > (quint32)MAX_RECORDS)) {
        return false;
    }
    const qint32 nRecordCount = (qint32)nRecordCountValue;
    const qint64 nHeaderSize = nHeaderSizeValue;

    const QByteArray baHeader = read_array_process(
        0, nHeaderSize, pPdStruct);
    if (!guardedThis || (baHeader.size() != nHeaderSize)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(
        baHeader.constData());
    if (readLE32(pHeader + ((qint64)nRecordCount * 4)) != 0) return false;

    qint64 nExpectedOffset = nHeaderSize;
    for (qint32 i = 0; i < nRecordCount; ++i) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nDataOffset = readLE32(pHeader + ((qint64)i * 4));
        const qint64 nSizeFieldOffset =
            ((qint64)nRecordCount + 1 + i) * 4;
        const qint64 nDataSize = readLE32(pHeader + nSizeFieldOffset);
        if ((nDataOffset != nExpectedOffset) || (nDataSize < 8) ||
            !rangeWithin(nTotalSize, nDataOffset, nDataSize)) {
            return false;
        }

        const qint64 nNextOffset = nDataOffset + nDataSize;
        const qint64 nDeclaredNext = (i + 1 < nRecordCount)
            ? (qint64)readLE32(pHeader + ((qint64)(i + 1) * 4))
            : nTotalSize;
        if (nNextOffset != nDeclaredNext) return false;

        const QByteArray baSignature = read_array_process(
            nDataOffset, 6, pPdStruct);
        if (!guardedThis || (baSignature.size() != 6)) return false;
        const bool bRib =
            (memcmp(baSignature.constData(), "RIB\0", 4) == 0);
        const bool bSm8 =
            (memcmp(baSignature.constData(), "SM8\0\0\1", 6) == 0);
        if (!bRib && !bSm8) return false;

        // Both supported record types have at least their fixed 8/10-byte
        // headers.  Checking the minimum separately keeps tiny signature-only
        // blobs from turning arbitrary offset tables into false positives.
        if ((bRib && (nDataSize < 8)) || (bSm8 && (nDataSize < 10))) {
            return false;
        }

        if (pEntries) {
            ENTRY entry = {};
            entry.nHeaderOffset = (qint64)i * 4;
            entry.nHeaderSize = 4;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nDataSize;
            entry.sFileName = QStringLiteral("record_%1.%2")
                .arg(i + 1, 6, 10, QLatin1Char('0'))
                .arg(bRib ? QStringLiteral("rib")
                          : QStringLiteral("sm8"));
            pEntries->append(entry);
        }

        nExpectedOffset = nNextOffset;
    }

    if ((nExpectedOffset != nTotalSize) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
