/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhdcopydecoder.h"

#include <QtEndian>

namespace {
const qint64 HDCOPY_HEADER_SIZE = 0xb8;
const qint64 HDCOPY_MAP_OFFSET = 0x10;
const qint64 HDCOPY_MAP_SIZE = 168;
const qint64 HDCOPY_SECTOR_SIZE = 512;
const quint8 HDCOPY_MAGIC_0 = 0xffU;
const quint8 HDCOPY_MAGIC_1 = 0x18U;
const quint8 HDCOPY_MAX_LABEL = 11U;
const quint8 HDCOPY_MIN_LAST_CYLINDER = 79U;
const quint8 HDCOPY_MAX_LAST_CYLINDER = 83U;

bool hdcopyIsValidSectorCount(quint8 nSectors)
{
    return (nSectors == 9U) || (nSectors == 10U) || (nSectors == 15U) || (nSectors == 17U) || (nSectors == 18U) || (nSectors == 20U) ||
           (nSectors == 21U);
}

bool hdcopyGeometry(const QByteArray &baData, qint64 *pnTrackCount, qint64 *pnTrackSize)
{
    if (baData.size() < HDCOPY_HEADER_SIZE) return false;
    const uchar *p = reinterpret_cast<const uchar *>(baData.constData());
    if ((p[0] != HDCOPY_MAGIC_0) || (p[1] != HDCOPY_MAGIC_1)) return false;
    if (p[2] > HDCOPY_MAX_LABEL) return false;
    const quint8 nLastCylinder = p[0x0e];
    const quint8 nSectors = p[0x0f];
    if ((nLastCylinder < HDCOPY_MIN_LAST_CYLINDER) || (nLastCylinder > HDCOPY_MAX_LAST_CYLINDER)) return false;
    if (!hdcopyIsValidSectorCount(nSectors)) return false;
    const qint64 nTrackCount = (qint64(nLastCylinder) + 1) * 2;
    if (nTrackCount > HDCOPY_MAP_SIZE) return false;
    if (pnTrackCount) *pnTrackCount = nTrackCount;
    if (pnTrackSize) *pnTrackSize = qint64(nSectors) * HDCOPY_SECTOR_SIZE;
    return true;
}
}  // namespace

qint64 XHDCopyDecoder::imageSize(const QByteArray &baHeader)
{
    qint64 nTrackCount = 0;
    qint64 nTrackSize = 0;
    if (!hdcopyGeometry(baHeader, &nTrackCount, &nTrackSize)) return -1;
    return nTrackCount * nTrackSize;
}

bool XHDCopyDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) return false;
    qint64 nTrackCount = 0;
    qint64 nTrackSize = 0;
    if (!hdcopyGeometry(baPacked, &nTrackCount, &nTrackSize)) return false;

    const qint64 nImageSize = nTrackCount * nTrackSize;
    if ((nUncompressedSize >= 0) && (nUncompressedSize != nImageSize)) return false;
    if (nImageSize > 0x7fffffff) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());
    const qint64 nInputSize = baPacked.size();

    QByteArray baResult;
    baResult.reserve(qint32(nImageSize));

    qint64 nOffset = HDCOPY_HEADER_SIZE;
    QByteArray baTrack;
    for (qint64 nTrack = 0; nTrack < nTrackCount; ++nTrack) {
        if (pData[HDCOPY_MAP_OFFSET + nTrack] == 0) {
            // Never read off the disk; hand back the format filler.
            baResult.append(QByteArray(qint32(nTrackSize), char(FILL_BYTE)));
            continue;
        }
        if (nOffset + 2 > nInputSize) return false;
        const qint64 nBlockSize = qint64(qFromLittleEndian<quint16>(pData + nOffset));
        nOffset += 2;
        // The block has to hold at least the escape byte.
        if ((nBlockSize < 1) || (nOffset + nBlockSize > nInputSize)) return false;

        const quint8 nEscape = pData[nOffset];
        qint64 nPosition = nOffset + 1;
        const qint64 nBlockEnd = nOffset + nBlockSize;
        nOffset = nBlockEnd;

        baTrack.clear();
        baTrack.reserve(qint32(nTrackSize));
        while (nPosition < nBlockEnd) {
            const quint8 nByte = pData[nPosition++];
            if (nByte == nEscape) {
                if (nPosition + 2 > nBlockEnd) return false;
                const quint8 nValue = pData[nPosition];
                const quint8 nCount = pData[nPosition + 1];
                nPosition += 2;
                if (qint64(baTrack.size()) + qint64(nCount) > nTrackSize) return false;
                if (nCount) baTrack.append(QByteArray(qint32(nCount), char(nValue)));
            } else {
                if (qint64(baTrack.size()) + 1 > nTrackSize) return false;
                baTrack.append(char(nByte));
            }
        }
        if (qint64(baTrack.size()) != nTrackSize) return false;
        baResult.append(baTrack);
    }

    // The block chain must consume the container exactly; anything left over
    // means this was not an HD-COPY image after all.
    if (nOffset != nInputSize) return false;
    if (qint64(baResult.size()) != nImageSize) return false;

    *pbaUnpacked = baResult;
    return true;
}
