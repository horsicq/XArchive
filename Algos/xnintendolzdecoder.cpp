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
#include "xnintendolzdecoder.h"

namespace {

const qint64 N_TAG_SIZE = 4;
const qint64 N_BASE_HEADER_SIZE = 4;
const qint64 N_EXTENSION_SIZE = 4;
const quint8 N_TYPE_LZ10 = 0x10U;
const quint8 N_TYPE_LZ11 = 0x11U;
const qint64 N_MAX_OUTPUT = 0x7fffffff;
const qint64 N_WINDOW_SIZE = 0x1000;  // the largest encodable distance
const qint64 N_WINDOW_MASK = 0xfff;
const qint32 N_CANCEL_MASK = 0xffff;  // cancellation is checked this often (items)
const qint64 N_LZ10_MIN_LENGTH = 3;
const qint64 N_LZ11_SHORT_LENGTH_BASE = 1;    // indicator + 1
const qint64 N_LZ11_MEDIUM_LENGTH_BASE = 0x11;
const qint64 N_LZ11_LONG_LENGTH_BASE = 0x111;
// Exact expansion bounds of the codec (the reader's gate uses the same ones):
// LZ10's densest group is 1 flag + 8 two-byte references = 17 bytes for
// 8 x 18 = 144 plaintext bytes; LZ11's is 1 flag + 8 four-byte references =
// 33 bytes for 8 x 0x10110 = 526464.  The slack covers streams so short that
// the first (always literal) item dominates.
const qint64 N_MAX_RATIO_LZ10 = 9;
const qint64 N_MAX_RATIO_LZ11 = 16384;
const qint64 N_RATIO_SLACK = 64;

bool nlzIsTag(const quint8 *pData)
{
    return (pData[0] == 'L') && (pData[1] == 'Z') && (pData[2] == '7') && (pData[3] == '7');
}

// Output access.  With a real output buffer the output IS the history; the
// scanning mode keeps only the last N_WINDOW_SIZE bytes in a ring, which is
// exact because no reference can reach further back than that.
void nlzEmitByte(quint8 *pOut, quint8 *pRing, qint64 nIndex, quint8 nByte)
{
    if (pOut) {
        pOut[nIndex] = nByte;
    } else {
        pRing[nIndex & N_WINDOW_MASK] = nByte;
    }
}

// Reads one back reference at *pnPos, advancing it.  Every byte the reference
// occupies is checked against nIn before it is touched.
bool nlzReadReference(const quint8 *pIn, qint64 nIn, bool bLZ11, qint64 *pnPos, qint64 *pnLength, qint64 *pnDistance)
{
    const qint64 nPos = *pnPos;
    if (nPos >= nIn) return false;
    const quint8 nByte0 = pIn[nPos];

    if (!bLZ11) {
        if (nPos + 2 > nIn) return false;
        *pnLength = (qint64)(nByte0 >> 4) + N_LZ10_MIN_LENGTH;
        *pnDistance = (qint64)(((quint32)(nByte0 & 0x0fU) << 8) | pIn[nPos + 1]) + 1;
        *pnPos = nPos + 2;
        return true;
    }

    const quint8 nIndicator = nByte0 >> 4;
    if (nIndicator == 0) {
        if (nPos + 3 > nIn) return false;
        const quint8 nByte1 = pIn[nPos + 1];
        *pnLength = (qint64)(((quint32)(nByte0 & 0x0fU) << 4) | (nByte1 >> 4)) + N_LZ11_MEDIUM_LENGTH_BASE;
        *pnDistance = (qint64)(((quint32)(nByte1 & 0x0fU) << 8) | pIn[nPos + 2]) + 1;
        *pnPos = nPos + 3;
        return true;
    }
    if (nIndicator == 1) {
        if (nPos + 4 > nIn) return false;
        const quint8 nByte1 = pIn[nPos + 1];
        const quint8 nByte2 = pIn[nPos + 2];
        *pnLength = (qint64)(((quint32)(nByte0 & 0x0fU) << 12) | ((quint32)nByte1 << 4) | (nByte2 >> 4)) + N_LZ11_LONG_LENGTH_BASE;
        *pnDistance = (qint64)(((quint32)(nByte2 & 0x0fU) << 8) | pIn[nPos + 3]) + 1;
        *pnPos = nPos + 4;
        return true;
    }

    if (nPos + 2 > nIn) return false;
    *pnLength = (qint64)nIndicator + N_LZ11_SHORT_LENGTH_BASE;
    *pnDistance = (qint64)(((quint32)(nByte0 & 0x0fU) << 8) | pIn[nPos + 1]) + 1;
    *pnPos = nPos + 2;
    return true;
}

// The one decoder both entry points share.  pOut is the plaintext buffer of
// nOut bytes, or null to decode into the ring only (scan mode).
bool nlzDecodeCore(const quint8 *pIn, qint64 nIn, bool bLZ11, qint64 nOut, quint8 *pOut, quint8 *pRing, qint64 *pnConsumed, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pIn || !pnConsumed || (nOut < 0) || (nOut > N_MAX_OUTPUT)) return false;
    if (!pOut && !pRing) return false;

    *pnConsumed = 0;
    if (nOut == 0) return true;

    qint64 nPos = 0;
    qint64 nProduced = 0;
    qint32 nCounter = 0;

    while (nProduced < nOut) {
        if (nPos >= nIn) return false;  // E1: no flag byte left
        const quint8 nFlags = pIn[nPos];
        ++nPos;

        for (qint32 nBit = 7; nBit >= 0; --nBit) {
            if (nProduced >= nOut) break;  // unused slots of the last flag byte

            ++nCounter;
            if ((nCounter & N_CANCEL_MASK) == 0) {
                if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            }

            if (nPos >= nIn) return false;  // E1: item missing

            if ((nFlags >> nBit) & 1) {
                qint64 nLength = 0;
                qint64 nDistance = 0;
                if (!nlzReadReference(pIn, nIn, bLZ11, &nPos, &nLength, &nDistance)) return false;  // E1
                if (nDistance > nProduced) return false;      // E2: before the start
                if (nLength > (nOut - nProduced)) return false;  // E3: past the declared end

                // Byte by byte, in forward order, on purpose: distance 1 with
                // length 18 replicates the last byte, which a block copy
                // would get wrong.  The mode is selected once per reference,
                // not once per byte, because a 512 MiB declaration makes this
                // loop the whole cost of the detection gate.
                const qint64 nSource = nProduced - nDistance;
                if (pOut) {
                    for (qint64 i = 0; i < nLength; ++i) {
                        pOut[nProduced + i] = pOut[nSource + i];
                    }
                } else {
                    for (qint64 i = 0; i < nLength; ++i) {
                        pRing[(nProduced + i) & N_WINDOW_MASK] = pRing[(nSource + i) & N_WINDOW_MASK];
                    }
                }
                nProduced += nLength;
            } else {
                nlzEmitByte(pOut, pRing, nProduced, pIn[nPos]);
                ++nPos;
                ++nProduced;
            }
        }
    }

    *pnConsumed = nPos;

    return true;
}

}  // namespace

bool XNintendoLZDecoder::parseHeader(const quint8 *pData, qint64 nSize, HEADER *pHeader)
{
    if (!pData || !pHeader || (nSize < N_BASE_HEADER_SIZE)) return false;

    HEADER header = {};
    qint64 nOffset = 0;
    if ((nSize >= N_TAG_SIZE + N_BASE_HEADER_SIZE) && nlzIsTag(pData)) {
        header.bHasTag = true;
        nOffset = N_TAG_SIZE;
    }
    if (nOffset + N_BASE_HEADER_SIZE > nSize) return false;

    const quint8 nType = pData[nOffset];
    if (nType == N_TYPE_LZ10) {
        header.variant = VARIANT_LZ10;
    } else if (nType == N_TYPE_LZ11) {
        header.variant = VARIANT_LZ11;
    } else {
        return false;
    }

    qint64 nLength = (qint64)pData[nOffset + 1] | ((qint64)pData[nOffset + 2] << 8) | ((qint64)pData[nOffset + 3] << 16);
    qint64 nHeaderSize = nOffset + N_BASE_HEADER_SIZE;

    if (nLength == 0) {
        // The DSDecmp form: a zero 24-bit field defers to a 32-bit one.  Read
        // as unsigned; anything with the top bit set (DSDecmp's negative) or
        // zero is a reject, because no plaintext of that length exists.
        if (nHeaderSize + N_EXTENSION_SIZE > nSize) return false;
        const quint32 nExtended = (quint32)pData[nHeaderSize] | ((quint32)pData[nHeaderSize + 1] << 8) | ((quint32)pData[nHeaderSize + 2] << 16) |
                                  ((quint32)pData[nHeaderSize + 3] << 24);
        if ((nExtended == 0) || (nExtended > (quint32)N_MAX_OUTPUT)) return false;
        nLength = (qint64)nExtended;
        nHeaderSize += N_EXTENSION_SIZE;
        header.bExtendedLength = true;
    }

    header.nUncompressedSize = nLength;
    header.nHeaderSize = nHeaderSize;
    *pHeader = header;

    return true;
}

bool XNintendoLZDecoder::decode(const QByteArray &baPacked, VARIANT variant, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((variant != VARIANT_LZ10) && (variant != VARIANT_LZ11)) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > N_MAX_OUTPUT)) return false;
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    // A record may not demand more plaintext than its packed bytes could
    // possibly encode; this is the codec's exact bound, so no valid stream is
    // refused, and it keeps a forged length from allocating the whole buffer
    // before the first item is even read.
    const qint64 nMaxRatio = (variant == VARIANT_LZ11) ? N_MAX_RATIO_LZ11 : N_MAX_RATIO_LZ10;
    if (nUncompressedSize > ((qint64)baPacked.size() * nMaxRatio) + N_RATIO_SLACK) return false;

    QByteArray baOut;
    baOut.resize((qint32)nUncompressedSize);
    if (baOut.size() != nUncompressedSize) return false;

    qint64 nConsumed = 0;
    const bool bResult = nlzDecodeCore((const quint8 *)baPacked.constData(), baPacked.size(), (variant == VARIANT_LZ11), nUncompressedSize,
                                       (quint8 *)baOut.data(), nullptr, &nConsumed, pPdStruct);
    if (!bResult) return false;

    *pbaResult = baOut;

    return baOut.size() == nUncompressedSize;
}

bool XNintendoLZDecoder::decodeLZ10(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    return decode(baPacked, VARIANT_LZ10, nUncompressedSize, pbaResult, pPdStruct);
}

bool XNintendoLZDecoder::decodeLZ11(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    return decode(baPacked, VARIANT_LZ11, nUncompressedSize, pbaResult, pPdStruct);
}

bool XNintendoLZDecoder::scan(const quint8 *pData, qint64 nSize, VARIANT variant, qint64 nUncompressedSize, qint64 *pnConsumed, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pData || !pnConsumed || (nSize < 0)) return false;
    *pnConsumed = 0;
    if ((variant != VARIANT_LZ10) && (variant != VARIANT_LZ11)) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > N_MAX_OUTPUT)) return false;
    if (nUncompressedSize == 0) return true;
    if (nSize == 0) return false;

    QByteArray baRing(N_WINDOW_SIZE, (char)0);

    return nlzDecodeCore(pData, nSize, (variant == VARIANT_LZ11), nUncompressedSize, nullptr, (quint8 *)baRing.data(), pnConsumed, pPdStruct);
}
