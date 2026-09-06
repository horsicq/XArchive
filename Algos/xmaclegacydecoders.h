/*
 * Native C++ translation of selected XADMaster legacy Macintosh decoders.
 * Copyright (c) 2017-present MacPaw Inc. and contributors.
 * GNU LGPL 2.1 or later; see xadmaster/COPYING.
 */
#ifndef XMACLEGACYDECODERS_H
#define XMACLEGACYDECODERS_H

#include <QByteArray>
#include <QtGlobal>
#include "xbinary.h"

namespace XMacLegacyDecoders {

bool decodeCompactPro(const QByteArray &packed, qint64 rawSize, bool lzh,
                      qint32 blockSize, QByteArray *output);
bool decodeDiskDoublerADn(const QByteArray &packed, qint64 rawSize,
                         QByteArray *output);
bool decodeDiskDoublerDDn(const QByteArray &packed, qint64 rawSize,
                         QByteArray *output);
// Method 1: Unix-compress stream, conditional output XOR, and additive
// 16-bit checksum including the three decoded stream-header bytes.
bool decodeDiskDoublerLZW(const QByteArray &packed, qint64 rawSize,
                         quint8 info1, quint8 info2, quint16 checksum,
                         QByteArray *output, XBinary::PDSTRUCT *pPdStruct = nullptr);

}  // namespace XMacLegacyDecoders

#endif  // XMACLEGACYDECODERS_H
