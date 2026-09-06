/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xrompaqdecoder.h"

#include <QtEndian>

#include <limits>

#include "xdcldecoder.h"

namespace {
const qint64 ROMPAQDEC_HEADER_SIZE = 0x48;
const qint64 ROMPAQDEC_OFFSET_VERSION = 0x0a;
const qint64 ROMPAQDEC_OFFSET_METHOD = 0x3c;
const qint64 ROMPAQDEC_OFFSET_PARTSIZE = 0x3f;

const quint16 ROMPAQDEC_VERSION_100 = 0x0100;
const quint16 ROMPAQDEC_VERSION_101 = 0x0101;

const quint8 ROMPAQDEC_METHOD_STORED = 1;
const quint8 ROMPAQDEC_METHOD_IMPLODE = 2;
}  // namespace

bool XRomPaqDecoder::decode(const QByteArray &baPacked,
                            qint64 nUncompressedSize, QByteArray *pbaUnpacked,
                            XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUnpacked || (nUncompressedSize <= 0) ||
        (nUncompressedSize > (std::numeric_limits<qint32>::max)())) {
        return false;
    }

    const qint64 nTotalSize = baPacked.size();
    if (nTotalSize < ROMPAQDEC_HEADER_SIZE) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baPacked.constData());

    QByteArray baResult;
    baResult.reserve(static_cast<qint32>(nUncompressedSize));

    qint64 nOffset = 0;

    while (nOffset < nTotalSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (nOffset > nTotalSize - ROMPAQDEC_HEADER_SIZE) return false;

        const quint16 nVersion =
            qFromLittleEndian<quint16>(pData + nOffset + ROMPAQDEC_OFFSET_VERSION);
        if ((nVersion != ROMPAQDEC_VERSION_100) &&
            (nVersion != ROMPAQDEC_VERSION_101)) {
            return false;
        }

        const quint8 nMethod = pData[nOffset + ROMPAQDEC_OFFSET_METHOD];
        const qint32 nPartSize = static_cast<qint32>(
            qFromLittleEndian<quint32>(pData + nOffset + ROMPAQDEC_OFFSET_PARTSIZE));

        qint64 nPayloadOffset = nOffset + ROMPAQDEC_HEADER_SIZE;

        if (nMethod == ROMPAQDEC_METHOD_IMPLODE) {
            // A bank whose stream begins with a zero word carries a two-byte
            // pad before its PKWARE DCL header; a non-zero word IS the DCL
            // header (00 04/05/06) and must be kept.
            if (nPayloadOffset > nTotalSize - 2) return false;
            if (qFromLittleEndian<quint16>(pData + nPayloadOffset) == 0) {
                nPayloadOffset += 2;
            }
            if ((nPartSize <= 0) ||
                (static_cast<qint64>(nPartSize) > nTotalSize - nPayloadOffset)) {
                return false;
            }
            const qint64 nRemaining = nUncompressedSize - baResult.size();
            if (nRemaining <= 0) return false;
            QByteArray baBank;
            if (!XDclDecoder::decode(
                    QByteArray(baPacked.constData() + nPayloadOffset,
                               static_cast<qint32>(nPartSize)),
                    &baBank, nRemaining, nullptr)) {
                return false;
            }
            if (baBank.size() > nRemaining) return false;
            baResult.append(baBank);
        } else if (nMethod == ROMPAQDEC_METHOD_STORED) {
            if ((nPartSize < 0) ||
                (static_cast<qint64>(nPartSize) > nTotalSize - nPayloadOffset) ||
                (static_cast<qint64>(nPartSize) >
                 nUncompressedSize - baResult.size())) {
                return false;
            }
            if (nPartSize > 0) {
                baResult.append(baPacked.constData() + nPayloadOffset, nPartSize);
            }
        } else {
            return false;
        }

        nOffset = nPayloadOffset + static_cast<qint64>(nPartSize);
    }

    if ((nOffset != nTotalSize) || (baResult.size() != nUncompressedSize)) {
        return false;
    }

    *pbaUnpacked = baResult;
    return true;
}
