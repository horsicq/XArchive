/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xpcommos2decoder.h"

#include <limits>
#include <string.h>

namespace {

// The container is a floppy-install payload; nothing legitimate approaches
// this.  The ceiling also keeps scan() from reporting a size the QByteArray
// decode below could never hold.
const qint64 PCOMM_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;

}  // namespace

bool XPCommOS2Decoder::scan(const char *pData, qint64 nSize, qint64 *pnUncompressedSize)
{
    if (!pData || !pnUncompressedSize) return false;

    *pnUncompressedSize = 0;

    // Shortest possible file: one literal run of one byte, then the two
    // terminating block markers.
    if (nSize < 4) return false;

    const quint8 *pIn = reinterpret_cast<const quint8 *>(pData);

    // A block always opens with a literal run - there is nothing to match
    // against yet - and the stream always closes with the two markers.  Both
    // are cheap and reject essentially everything that is not this format.
    if (pIn[0] < 0xE1) return false;
    if ((pIn[nSize - 1] != 0xE0) || (pIn[nSize - 2] != 0xE0)) return false;

    qint64 nInPos = 0;
    qint64 nOutPos = 0;
    qint64 nBase = 0;  // start of the current block in the plaintext
    // Only the last two closed blocks need to be remembered: every earlier one
    // must be a full block, and that is checked two closes later.
    qint64 nPrevSize = -1;
    qint64 nPrevPrevSize = -1;
    qint64 nBlockCount = 0;

    while (nInPos < nSize) {
        const quint8 nControl = pIn[nInPos++];

        if (nControl >= 0xE0) {
            const qint64 nCount = static_cast<qint64>(nControl) - 0xE0;

            if (nCount == 0) {
                const qint64 nBlockSize = nOutPos - nBase;
                if ((nBlockCount >= 2) && (nPrevPrevSize != BLOCK_SIZE)) return false;
                nPrevPrevSize = nPrevSize;
                nPrevSize = nBlockSize;
                nBlockCount++;
                nBase = nOutPos;
                continue;
            }

            if (nInPos + nCount > nSize) return false;
            nInPos += nCount;
            nOutPos += nCount;
        } else if (nControl >= 0x20) {
            if (nInPos >= nSize) return false;
            const quint8 nLow = pIn[nInPos++];
            const qint64 nLength = (static_cast<qint64>(nControl) >> 5) + 2;
            const qint64 nDistance = ((static_cast<qint64>(nControl) & 0x1F) << 8 | static_cast<qint64>(nLow)) + 1;
            if (nDistance > (nOutPos - nBase)) return false;
            nOutPos += nLength;
        } else {
            if (nInPos + 1 >= nSize) return false;
            const quint8 nLow = pIn[nInPos];
            const quint8 nHigh = pIn[nInPos + 1];
            nInPos += 2;
            const qint64 nLength = static_cast<qint64>(nControl) + 4;
            const qint64 nSourceOffset = static_cast<qint64>(nLow) | (static_cast<qint64>(nHigh) << 8);
            if (nSourceOffset >= (nOutPos - nBase)) return false;
            nOutPos += nLength;
        }

        if ((nOutPos - nBase) > BLOCK_SIZE) return false;
        if (nOutPos > PCOMM_MAX_UNCOMPRESSED_SIZE) return false;
    }

    // The stream has to end on a block boundary, with an empty terminator
    // block behind a non-empty data block.
    if (nOutPos != nBase) return false;
    if (nBlockCount < 2) return false;
    if (nPrevSize != 0) return false;
    if ((nPrevPrevSize <= 0) || (nPrevPrevSize > BLOCK_SIZE)) return false;
    if (nOutPos <= 0) return false;

    *pnUncompressedSize = nOutPos;

    return true;
}

bool XPCommOS2Decoder::scan(const QByteArray &baPacked, qint64 *pnUncompressedSize)
{
    return scan(baPacked.constData(), static_cast<qint64>(baPacked.size()), pnUncompressedSize);
}

bool XPCommOS2Decoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) return false;

    pbaUnpacked->clear();

    if ((nUncompressedSize <= 0) || (nUncompressedSize > PCOMM_MAX_UNCOMPRESSED_SIZE) ||
        (nUncompressedSize > static_cast<qint64>((std::numeric_limits<int>::max)()))) {
        return false;
    }

    const qint64 nSize = static_cast<qint64>(baPacked.size());
    if (nSize < 4) return false;

    const quint8 *pIn = reinterpret_cast<const quint8 *>(baPacked.constData());

    QByteArray baOut;
    baOut.resize(static_cast<qint32>(nUncompressedSize));
    if (static_cast<qint64>(baOut.size()) != nUncompressedSize) return false;

    quint8 *pOut = reinterpret_cast<quint8 *>(baOut.data());

    qint64 nInPos = 0;
    qint64 nOutPos = 0;
    qint64 nBase = 0;

    while (nInPos < nSize) {
        const quint8 nControl = pIn[nInPos++];

        if (nControl >= 0xE0) {
            const qint64 nCount = static_cast<qint64>(nControl) - 0xE0;

            if (nCount == 0) {
                nBase = nOutPos;
                continue;
            }

            if (nInPos + nCount > nSize) return false;
            if (nCount > (nUncompressedSize - nOutPos)) return false;
            memcpy(pOut + nOutPos, pIn + nInPos, static_cast<size_t>(nCount));
            nInPos += nCount;
            nOutPos += nCount;
        } else if (nControl >= 0x20) {
            if (nInPos >= nSize) return false;
            const quint8 nLow = pIn[nInPos++];
            const qint64 nLength = (static_cast<qint64>(nControl) >> 5) + 2;
            const qint64 nDistance = ((static_cast<qint64>(nControl) & 0x1F) << 8 | static_cast<qint64>(nLow)) + 1;
            if (nDistance > (nOutPos - nBase)) return false;
            if (nLength > (nUncompressedSize - nOutPos)) return false;
            // Byte-at-a-time on purpose: a match may overlap its own output.
            qint64 nSource = nOutPos - nDistance;
            for (qint64 i = 0; i < nLength; i++) {
                pOut[nOutPos++] = pOut[nSource++];
            }
        } else {
            if (nInPos + 1 >= nSize) return false;
            const quint8 nLow = pIn[nInPos];
            const quint8 nHigh = pIn[nInPos + 1];
            nInPos += 2;
            const qint64 nLength = static_cast<qint64>(nControl) + 4;
            qint64 nSource = nBase + (static_cast<qint64>(nLow) | (static_cast<qint64>(nHigh) << 8));
            if (nSource >= nOutPos) return false;
            if (nLength > (nUncompressedSize - nOutPos)) return false;
            for (qint64 i = 0; i < nLength; i++) {
                pOut[nOutPos++] = pOut[nSource++];
            }
        }

        if ((nOutPos - nBase) > BLOCK_SIZE) return false;
    }

    // Both conditions matter: a stream that stops short of the declared size,
    // and one that stops mid-block, are corrupt rather than merely truncated.
    if (nOutPos != nUncompressedSize) return false;
    if (nOutPos != nBase) return false;

    *pbaUnpacked = baOut;

    return true;
}
