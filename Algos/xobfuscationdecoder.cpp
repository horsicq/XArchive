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
#include "xobfuscationdecoder.h"

namespace {
quint8 applyTransform(XObfuscationDecoder::TRANSFORM transform, quint8 nByte, quint8 nKey)
{
    if (transform == XObfuscationDecoder::TRANSFORM_XOR) return (quint8)(nByte ^ nKey);
    if (transform == XObfuscationDecoder::TRANSFORM_ROL3_XOR) return (quint8)((quint8)((nByte << 3) | (nByte >> 5)) ^ nKey);
    if (transform == XObfuscationDecoder::TRANSFORM_SWAP_XOR) return (quint8)((quint8)((nByte >> 4) | (nByte << 4)) ^ nKey);
    return nByte;
}

bool matches(XObfuscationDecoder::TRANSFORM transform, quint8 nKey, const QByteArray &baHeader, const QByteArray &baPlainText)
{
    for (qint32 i = 0; i < baPlainText.size(); ++i) {
        if (applyTransform(transform, (quint8)baHeader.at(i), nKey) != (quint8)baPlainText.at(i)) return false;
    }
    return true;
}
}  // namespace

XObfuscationDecoder::METHOD XObfuscationDecoder::detect(const QByteArray &baHeader, const QByteArray &baPlainText)
{
    METHOD result = {TRANSFORM_UNKNOWN, 0};
    if (baPlainText.isEmpty() || (baHeader.size() < baPlainText.size())) return result;

    // The XOR key is fully determined by the first byte, so there is nothing to
    // search.  A key of zero would mean the file is NOT obfuscated - claiming
    // it here would shadow the plain format.
    const quint8 nXorKey = (quint8)((quint8)baHeader.at(0) ^ (quint8)baPlainText.at(0));
    if ((nXorKey != 0) && matches(TRANSFORM_XOR, nXorKey, baHeader, baPlainText)) {
        result.transform = TRANSFORM_XOR;
        result.nKey = nXorKey;
        return result;
    }

    // The other two mix the nibbles or rotate before the XOR, so their key
    // follows from the first byte just as directly once the shape is fixed.
    const quint8 nRolKey = (quint8)((quint8)(((quint8)baHeader.at(0) << 3) | ((quint8)baHeader.at(0) >> 5)) ^ (quint8)baPlainText.at(0));
    if (matches(TRANSFORM_ROL3_XOR, nRolKey, baHeader, baPlainText)) {
        result.transform = TRANSFORM_ROL3_XOR;
        result.nKey = nRolKey;
        return result;
    }

    const quint8 nSwapKey = (quint8)((quint8)(((quint8)baHeader.at(0) >> 4) | ((quint8)baHeader.at(0) << 4)) ^ (quint8)baPlainText.at(0));
    if (matches(TRANSFORM_SWAP_XOR, nSwapKey, baHeader, baPlainText)) {
        result.transform = TRANSFORM_SWAP_XOR;
        result.nKey = nSwapKey;
        return result;
    }

    return result;
}

QByteArray XObfuscationDecoder::methodToProperty(const METHOD &method)
{
    QByteArray baResult(2, (char)0);
    baResult[0] = (char)(quint8)method.transform;
    baResult[1] = (char)method.nKey;
    return baResult;
}

bool XObfuscationDecoder::propertyToMethod(const QByteArray &baProperty, METHOD *pMethod)
{
    if (!pMethod || (baProperty.size() != 2)) return false;
    const quint8 nTransform = (quint8)baProperty.at(0);
    if ((nTransform != TRANSFORM_XOR) && (nTransform != TRANSFORM_ROL3_XOR) && (nTransform != TRANSFORM_SWAP_XOR)) return false;
    pMethod->transform = (TRANSFORM)nTransform;
    pMethod->nKey = (quint8)baProperty.at(1);
    return true;
}

bool XObfuscationDecoder::decode(const QByteArray &baPacked, const METHOD &method, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult || (method.transform == TRANSFORM_UNKNOWN)) return false;
    pbaResult->clear();
    if (baPacked.isEmpty()) return false;

    QByteArray baOut(baPacked.size(), (char)0);
    const quint8 *pIn = (const quint8 *)baPacked.constData();
    quint8 *pOut = (quint8 *)baOut.data();
    for (qint32 i = 0; i < baPacked.size(); ++i) {
        if ((i & 0xffff) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }
        pOut[i] = applyTransform(method.transform, pIn[i], method.nKey);
    }

    *pbaResult = baOut;

    return true;
}

QString XObfuscationDecoder::methodToString(const METHOD &method)
{
    if (method.transform == TRANSFORM_XOR) return QStringLiteral("XOR %1").arg(method.nKey, 2, 16, QChar('0'));
    if (method.transform == TRANSFORM_ROL3_XOR) return QStringLiteral("ROL3+XOR %1").arg(method.nKey, 2, 16, QChar('0'));
    if (method.transform == TRANSFORM_SWAP_XOR) return QStringLiteral("SWAP+XOR %1").arg(method.nKey, 2, 16, QChar('0'));
    return QStringLiteral("Unknown");
}
