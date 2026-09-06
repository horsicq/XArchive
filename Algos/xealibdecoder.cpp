/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xealibdecoder.h"

namespace {
const qint32 EALIB_RING_SIZE = 4096;
const qint32 EALIB_RING_MASK = EALIB_RING_SIZE - 1;
const qint32 EALIB_LZSS_F = 18;
// A single EALIB member is one DOS-era resource; the largest in the reference
// corpus is well under a megabyte.  The cap only exists so a corrupt size field
// cannot ask for a multi-gigabyte allocation.
const qint64 EALIB_MAX_UNPACKED_SIZE = qint64(256) * 1024 * 1024;
}  // namespace

bool XEALIBDecoder::decodeLZSS(const QByteArray &baPacked,
                               qint64 nUncompressedSize,
                               QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked || (nUncompressedSize < 0) ||
        (nUncompressedSize > EALIB_MAX_UNPACKED_SIZE)) {
        return false;
    }
    pbaUnpacked->clear();
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    // The producer clears the whole ring, so a back reference into the
    // pre-history yields 0x00 bytes.
    QByteArray baRing(EALIB_RING_SIZE, char(0));

    QByteArray baOut;
    baOut.resize(static_cast<qint32>(nUncompressedSize));
    if (baOut.size() != nUncompressedSize) return false;

    quint8 *pRing = reinterpret_cast<quint8 *>(baRing.data());
    char *pOut = baOut.data();
    const quint8 *pIn = reinterpret_cast<const quint8 *>(baPacked.constData());
    const qint64 nInSize = baPacked.size();

    qint64 nInPosition = 0;
    qint64 nOutPosition = 0;
    qint32 nRing = EALIB_RING_SIZE - EALIB_LZSS_F;
    quint32 nFlags = 0;

    while (nOutPosition < nUncompressedSize) {
        nFlags >>= 1;
        if ((nFlags & 0x100U) == 0) {
            if (nInPosition >= nInSize) return false;
            nFlags = quint32(pIn[nInPosition++]) | 0xff00U;
        }

        if (nFlags & 1U) {
            if (nInPosition >= nInSize) return false;
            const quint8 nByte = pIn[nInPosition++];
            pOut[nOutPosition++] = static_cast<char>(nByte);
            pRing[nRing] = nByte;
            nRing = (nRing + 1) & EALIB_RING_MASK;
        } else {
            if (nInPosition + 1 >= nInSize) return false;
            const quint32 nFirst = pIn[nInPosition];
            const quint32 nSecond = pIn[nInPosition + 1];
            nInPosition += 2;
            qint32 nSource = static_cast<qint32>(nFirst | ((nSecond & 0xf0U) << 4));
            const qint32 nLength = static_cast<qint32>(nSecond & 0x0fU) + 3;
            for (qint32 k = 0; k < nLength; k++) {
                if (nOutPosition >= nUncompressedSize) break;
                const quint8 nByte = pRing[nSource & EALIB_RING_MASK];
                nSource = (nSource + 1) & EALIB_RING_MASK;
                pOut[nOutPosition++] = static_cast<char>(nByte);
                pRing[nRing] = nByte;
                nRing = (nRing + 1) & EALIB_RING_MASK;
            }
        }
    }

    if (nOutPosition != nUncompressedSize) return false;
    *pbaUnpacked = baOut;
    return true;
}
