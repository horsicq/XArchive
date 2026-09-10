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
#include "xpanoramadecoder.h"

#include <QtEndian>

namespace {

void panoramaBuildKeystream(quint32 nSeed, quint8 *pKeystream)
{
    quint32 nValue = nSeed;
    for (qint32 i = 0; i < 256; i++) {
        pKeystream[i * 4 + 0] = (quint8)(nValue & 0xFF);
        pKeystream[i * 4 + 1] = (quint8)((nValue >> 8) & 0xFF);
        pKeystream[i * 4 + 2] = (quint8)((nValue >> 16) & 0xFF);
        pKeystream[i * 4 + 3] = (quint8)((nValue >> 24) & 0xFF);
        nValue = nValue * XPanoramaDecoder::MULTIPLIER;
    }
}

}  // namespace

bool XPanoramaDecoder::seedFromHeader(const QByteArray &baHeader, quint32 *pnSeed)
{
    if (!pnSeed || (baHeader.size() < 8)) return false;
    const quint32 nLow = qFromLittleEndian<quint32>((const uchar *)baHeader.constData());
    const quint32 nHigh = qFromLittleEndian<quint32>((const uchar *)baHeader.constData() + 4);
    const quint32 nSeed = nLow ^ RAR_SIGNATURE_LOW;
    if (nSeed == 0) return false;
    if (((nHigh ^ (nSeed * MULTIPLIER)) & 0xFFFFFF) != RAR_SIGNATURE_HIGH) return false;
    *pnSeed = nSeed;

    return true;
}

QByteArray XPanoramaDecoder::seedToProperty(quint32 nSeed)
{
    QByteArray baResult(4, (char)0);
    qToLittleEndian<quint32>(nSeed, (uchar *)baResult.data());

    return baResult;
}

bool XPanoramaDecoder::propertyToSeed(const QByteArray &baProperty, quint32 *pnSeed)
{
    if (!pnSeed || (baProperty.size() != 4)) return false;
    *pnSeed = qFromLittleEndian<quint32>((const uchar *)baProperty.constData());

    return true;
}

QString XPanoramaDecoder::methodToString()
{
    return QStringLiteral("Panorama LCG XOR");
}

bool XPanoramaDecoder::decode(const QByteArray &baPacked, quint32 nSeed, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baPacked.isEmpty()) return false;

    quint8 nKeystream[KEYSTREAM_SIZE];
    panoramaBuildKeystream(nSeed, nKeystream);

    QByteArray baResult = baPacked;
    baResult.detach();
    char *pData = baResult.data();
    if (!pData) return false;

    const qint64 nTotal = (qint64)baResult.size();
    qint64 nPosition = 0;
    while (nPosition < nTotal) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nEnd = qMin<qint64>(nTotal, nPosition + 0x100000);
        for (qint64 i = nPosition; i < nEnd; i++) {
            pData[i] = (char)((quint8)pData[i] ^ nKeystream[(qint32)(i & (KEYSTREAM_SIZE - 1))]);
        }
        nPosition = nEnd;
    }

    *pbaResult = baResult;

    return true;
}
