/*
 * Native C++ translation of selected XADMaster legacy Macintosh decoders.
 * Copyright (c) 2017-present MacPaw Inc. and contributors.
 * GNU LGPL 2.1 or later; see xadmaster/COPYING.
 */
#ifndef XMACLEGACYDECODERS_H
#define XMACLEGACYDECODERS_H

#include <QByteArray>
#include <QtGlobal>

namespace XMacLegacyDecoders {

bool decodeCompactPro(const QByteArray &packed, qint64 rawSize, bool lzh,
                      qint32 blockSize, QByteArray *output);
bool decodeDiskDoublerADn(const QByteArray &packed, qint64 rawSize,
                         QByteArray *output);
bool decodeDiskDoublerDDn(const QByteArray &packed, qint64 rawSize,
                         QByteArray *output);

}  // namespace XMacLegacyDecoders

#endif  // XMACLEGACYDECODERS_H
