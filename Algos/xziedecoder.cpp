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
#include "xziedecoder.h"

#include <string.h>

namespace {

// The two 16-byte tables in the reference's data section XOR to this.
const char ZIE_CONSTANT[17] = "ProtectIt/2 OS/2";
const qint32 ZIE_NAME_SIZE = 0x0D;
const qint32 ZIE_NAME_OFFSET = 0x14;
const char ZIE_ZIP_PLAIN[5] = "PK\x03\x04";

// The reference's 8.3-ish file name sanity check, ported branch for branch.
// It never steps past the 13-byte field: the length counter is decremented in
// lock step with the cursor and the function returns before the read that would
// leave the field.
bool zieNameOk(const quint8 *pName, qint32 nSize)
{
    if (!pName || (nSize < 1)) return false;
    qint32 n = nSize;
    qint32 p = 0;
    if (!((pName[0] >= 0x20) && (pName[0] != 0x2E))) return false;

    qint32 i = 2;
    bool bDotSeen = false;
    while (true) {
        p++;
        n--;
        if (n < 1) return false;
        if (p >= nSize) return false;
        if (pName[p] == 0) return true;
        if (pName[p] < 0x20) return false;
        if (pName[p] == 0x2E) {
            bDotSeen = true;
            break;
        }
        i++;
        if (i == 10) break;
    }
    if (!bDotSeen) {
        if (p >= nSize) return false;
        if (pName[p] != 0x2E) return false;
    }

    i = 1;
    while (true) {
        p++;
        n--;
        if (n < 1) return (i == 4);
        if (p >= nSize) return false;
        if (pName[p] == 0) return true;
        if (pName[p] < 0x20) return false;
        i++;
        if (i == 5) return false;
    }
}

void zieXorRange(char *pData, const quint8 *pKey, qint32 nBase, qint64 nFrom, qint64 nTo)
{
    for (qint64 nPosition = nFrom; nPosition < nTo; nPosition++) {
        pData[nPosition] = (char)((quint8)pData[nPosition] ^ pKey[(qint32)((nPosition + nBase) & 0xF)]);
    }
}

}  // namespace

bool XZIEDecoder::isValidHeader(const QByteArray &baHeader)
{
    if ((qint64)baHeader.size() < HEADER_SIZE) return false;
    const quint8 *pHeader = (const quint8 *)baHeader.constData();
    if ((pHeader[0] != 'P') || (pHeader[1] != 'I') || (pHeader[2] != 'T') || (pHeader[3] != '2')) return false;

    return zieNameOk(pHeader + ZIE_NAME_OFFSET, ZIE_NAME_SIZE);
}

QString XZIEDecoder::fileNameFromHeader(const QByteArray &baHeader)
{
    if ((qint64)baHeader.size() < HEADER_SIZE) return QString();
    QByteArray baName = baHeader.mid(ZIE_NAME_OFFSET, ZIE_NAME_SIZE);
    const qint32 nZero = baName.indexOf((char)0);
    if (nZero >= 0) baName = baName.left(nZero);

    return QString::fromLatin1(baName);
}

bool XZIEDecoder::resolveMethod(const QByteArray &baHeader, const QByteArray &baProbe, qint64 nPayloadSize, METHOD *pMethod)
{
    if (!pMethod) return false;
    if (!isValidHeader(baHeader)) return false;
    if ((nPayloadSize < 4) || (baProbe.size() < 4)) return false;

    METHOD method = {};
    const quint8 *pHeader = (const quint8 *)baHeader.constData();
    for (qint32 i = 0; i < 16; i++) method.nKey[i] = (quint8)(pHeader[4 + i] ^ (quint8)ZIE_CONSTANT[i]);

    const qint64 nSize = nPayloadSize & ~(qint64)3;
    if (nSize < 4) return false;

    const qint32 nLengthBase = (qint32)((0x10 - (nSize & 0xF)) & 0xF);
    const qint32 nCandidates[2] = {nLengthBase, 0};
    const quint8 *pProbe = (const quint8 *)baProbe.constData();

    for (qint32 nIndex = 0; nIndex < 2; nIndex++) {
        const qint32 nBase = nCandidates[nIndex];
        // Position 0..3 always sits inside the first 0xC0000 bytes, so the
        // large-payload phase shift applies to the probe whenever it applies at
        // all.
        const qint32 nRot = (nSize >= LARGE_PAYLOAD_SIZE) ? ((nBase + 8) & 0xF) : nBase;
        bool bMatch = true;
        for (qint32 i = 0; i < 4; i++) {
            const quint8 nPlain = (quint8)(pProbe[i] ^ method.nKey[(i + nRot) & 0xF]);
            if (nPlain != (quint8)ZIE_ZIP_PLAIN[i]) {
                bMatch = false;
                break;
            }
        }
        if (bMatch) {
            method.nBase = nBase;
            method.bRecovered = (nIndex != 0);
            *pMethod = method;
            return true;
        }
        if ((nIndex == 0) && (nLengthBase == 0)) break;
    }

    return false;
}

QByteArray XZIEDecoder::methodToProperty(const METHOD &method)
{
    QByteArray baResult;
    baResult.append((const char *)method.nKey, 16);
    baResult.append((char)(quint8)(method.nBase & 0xF));
    baResult.append((char)(method.bRecovered ? 1 : 0));

    return baResult;
}

bool XZIEDecoder::propertyToMethod(const QByteArray &baProperty, METHOD *pMethod)
{
    if (!pMethod || (baProperty.size() != 18)) return false;
    METHOD method = {};
    memcpy(method.nKey, baProperty.constData(), 16);
    method.nBase = (qint32)((quint8)baProperty.at(16)) & 0xF;
    method.bRecovered = ((quint8)baProperty.at(17) != 0);
    *pMethod = method;

    return true;
}

QString XZIEDecoder::methodToString(const METHOD &method)
{
    if (method.bRecovered) return QStringLiteral("ProtectIt/2 XOR (truncated, phase 0)");

    return QStringLiteral("ProtectIt/2 XOR");
}

bool XZIEDecoder::decode(const QByteArray &baPacked, const METHOD &method, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baPacked.isEmpty()) return false;

    QByteArray baResult = baPacked;
    baResult.detach();
    char *pData = baResult.data();
    if (!pData) return false;

    const qint64 nTotal = (qint64)baResult.size();
    const qint64 nSize = nTotal & ~(qint64)3;
    const qint32 nBase = method.nBase & 0xF;

    // The tail bytes (payload % 4) are never encrypted.
    if (nSize >= LARGE_PAYLOAD_SIZE) {
        const qint64 nCut = LARGE_PAYLOAD_SIZE;
        qint64 nPosition = 0;
        while (nPosition < nCut) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const qint64 nChunk = qMin<qint64>(0x100000, nCut - nPosition);
            zieXorRange(pData, method.nKey, (nBase + 8) & 0xF, nPosition, nPosition + nChunk);
            nPosition += nChunk;
        }
        while (nPosition < nSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const qint64 nChunk = qMin<qint64>(0x100000, nSize - nPosition);
            zieXorRange(pData, method.nKey, nBase, nPosition, nPosition + nChunk);
            nPosition += nChunk;
        }
    } else {
        qint64 nPosition = 0;
        while (nPosition < nSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            const qint64 nChunk = qMin<qint64>(0x100000, nSize - nPosition);
            zieXorRange(pData, method.nKey, nBase, nPosition, nPosition + nChunk);
            nPosition += nChunk;
        }
    }

    *pbaResult = baResult;

    return true;
}
