/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarcv2decoder.h"

bool XARCV2Decoder::descramble(const QByteArray &packed, quint8 nSeed,
                               QByteArray *output)
{
    if (!output || packed.isEmpty()) return false;

    QByteArray result(packed.size(), 0);
    const quint8 *pInput =
        reinterpret_cast<const quint8 *>(packed.constData());
    quint8 *pOutput = reinterpret_cast<quint8 *>(result.data());
    quint8 nPrevious = nSeed;
    const qint32 nSize = packed.size();
    for (qint32 i = 0; i < nSize; ++i) {
        nPrevious = static_cast<quint8>(pInput[i] ^ nPrevious);
        pOutput[i] = nPrevious;
    }
    *output = result;
    return true;
}

bool XARCV2Decoder::decode(const QByteArray &packed,
                           qint64 nUncompressedSize, quint8 nSeed,
                           QByteArray *output)
{
    // A stored member declares equal packed and unpacked sizes; the filter is
    // length-preserving, so anything else is a caller error rather than a
    // recoverable stream.
    if (!output || (nUncompressedSize != packed.size())) return false;
    return descramble(packed, nSeed, output);
}
