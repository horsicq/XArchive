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
#include "xti99arcdecoder.h"

#include "xsharedlzwdecoder.h"

#include <QtEndian>

namespace {
const qint32 TI99_PROPERTIES_HEADER = 0x10;

void ti99AppendU32(QByteArray *pbaTarget, quint32 nValue)
{
    char buffer[4];
    qToLittleEndian<quint32>(nValue, (uchar *)buffer);
    pbaTarget->append(buffer, 4);
}

quint32 ti99ReadU32(const QByteArray &baSource, qint32 nOffset)
{
    return qFromLittleEndian<quint32>((const uchar *)baSource.constData() + nOffset);
}

}  // namespace

// Out-of-class definitions so the constants may be odr-used (passed by
// reference into templates such as qMin) from any translation unit.
const qint64 XTI99ARCDecoder::MAX_PLAIN_SIZE;
const qint64 XTI99ARCDecoder::PROBE_SIZE;

bool XTI99ARCDecoder::expand(const QByteArray &baPayload, qint64 nPlainSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nPlainSize <= 0) || (nPlainSize > MAX_PLAIN_SIZE)) return false;
    if (baPayload.isEmpty()) return false;

    XSharedLZWDecoder::OPTIONS options;
    options.nMaxBits = 12;
    options.bHasClearCode = true;
    options.bHasEndCode = true;
    options.bMsbFirst = true;
    options.bUnRle90 = false;
    options.bBlockPadding = false;
    options.nWidthStepBias = 0;

    // The return value is deliberately ignored: it only reports whether the
    // stream produced EXACTLY nPlainSize bytes, and both callers ask for more
    // than the stream holds on purpose.  The buffer is filled either way.
    XSharedLZWDecoder::decode(baPayload, options, nPlainSize, pbaResult, pPdStruct);

    return !pbaResult->isEmpty();
}

QByteArray XTI99ARCDecoder::buildProperties(qint64 nPlainSize, qint64 nMemberOffset, qint64 nMemberSize, const QByteArray &baPrefix)
{
    QByteArray baResult;
    if ((nPlainSize < 0) || (nMemberOffset < 0) || (nMemberSize < 0) || (baPrefix.size() < 0)) return baResult;

    ti99AppendU32(&baResult, (quint32)nPlainSize);
    ti99AppendU32(&baResult, (quint32)nMemberOffset);
    ti99AppendU32(&baResult, (quint32)nMemberSize);
    ti99AppendU32(&baResult, (quint32)baPrefix.size());
    baResult.append(baPrefix);

    return baResult;
}

bool XTI99ARCDecoder::decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                             XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baProperties.size() < TI99_PROPERTIES_HEADER) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_PLAIN_SIZE)) return false;

    const qint64 nPlainSize = (qint64)ti99ReadU32(baProperties, 0);
    const qint64 nMemberOffset = (qint64)ti99ReadU32(baProperties, 4);
    const qint64 nMemberSize = (qint64)ti99ReadU32(baProperties, 8);
    const qint64 nPrefixSize = (qint64)ti99ReadU32(baProperties, 12);

    if ((nPrefixSize < 0) || (nPrefixSize > (baProperties.size() - TI99_PROPERTIES_HEADER))) return false;
    if ((nPlainSize <= 0) || (nPlainSize > MAX_PLAIN_SIZE)) return false;
    if (nMemberOffset > (nPlainSize - nMemberSize)) return false;

    QByteArray baPlain;
    if (!expand(baPacked, nPlainSize, &baPlain, pPdStruct)) return false;
    // A short expansion is only fatal when the member itself is not fully in it.
    if (nMemberSize > (baPlain.size() - nMemberOffset)) return false;

    QByteArray baOut = baProperties.mid(TI99_PROPERTIES_HEADER, (qint32)nPrefixSize);
    baOut.append(baPlain.constData() + nMemberOffset, (qint32)nMemberSize);

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}
