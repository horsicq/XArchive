/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XDCLDECODER_H
#define XDCLDECODER_H

#include <QByteArray>
#include <QtGlobal>

// Bounded decoder for the PKWARE Data Compression Library stream format.
// This is not the unrelated PKZIP "Implode" method.
class XDclDecoder
{
public:
    static bool decode(const QByteArray &packed, QByteArray *raw,
                       qint64 maxOutputSize, qint64 *consumed = nullptr);
    static bool scan(const uchar *packed, qint64 packedSize,
                     qint64 maxOutputSize, qint64 *consumed,
                     qint64 *rawSize);
};

#endif  // XDCLDECODER_H
