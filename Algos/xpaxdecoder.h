/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#ifndef XPAXDECODER_H
#define XPAXDECODER_H

#include "xbinary.h"

namespace XPaxDecoder
{
// Decode the GEM-View PAX "LZF0" member stream.  The packed buffer may
// include alignment bytes up to the following record; consumedSize reports
// the byte containing the final coded bit rounded up to a byte boundary.
bool decode(const QByteArray &packed, qint32 expectedSize,
            QByteArray *output, qint64 *consumedSize = nullptr,
            XBinary::PDSTRUCT *pPdStruct = nullptr);
}

#endif  // XPAXDECODER_H
