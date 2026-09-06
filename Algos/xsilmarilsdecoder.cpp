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
#include "xsilmarilsdecoder.h"

namespace {

// Walks the token grammar.  When pbaOut is null nothing is materialised, which
// is what the detection probe wants.  On success *pnConsumed holds the number
// of input bytes the grammar claimed, which may exceed the bytes actually
// copied out when the plaintext completes in the middle of a literal run.
bool silmarilsWalk(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaOut, qint64 *pnConsumed)
{
    if (nUncompressedSize < 0) {
        return false;
    }

    const qint64 nPackedSize = baPacked.size();
    const quint8 *pData = reinterpret_cast<const quint8 *>(baPacked.constData());

    qint64 nInPos = 0;
    qint64 nOutPos = 0;

    if (pbaOut) {
        pbaOut->clear();
        pbaOut->reserve(static_cast<int>(nUncompressedSize));
    }

    while ((nInPos < nPackedSize) && (nOutPos < nUncompressedSize)) {
        const quint8 nToken = pData[nInPos++];

        if (nToken < 0x80) {
            // Literal run: nToken raw bytes.
            const qint64 nCount = nToken;

            if ((nPackedSize - nInPos) < nCount) {
                return false;
            }

            qint64 nTake = nCount;
            if (nTake > (nUncompressedSize - nOutPos)) {
                nTake = nUncompressedSize - nOutPos;
            }

            if (pbaOut && (nTake > 0)) {
                pbaOut->append(baPacked.constData() + nInPos, static_cast<int>(nTake));
            }

            nInPos += nCount;
            nOutPos += nTake;
        } else {
            // Byte run: (nToken & 0x7f) copies of the following byte.
            if (nInPos >= nPackedSize) {
                return false;
            }

            const char cValue = static_cast<char>(pData[nInPos++]);
            qint64 nTake = nToken & 0x7f;

            if (nTake > (nUncompressedSize - nOutPos)) {
                nTake = nUncompressedSize - nOutPos;
            }

            if (pbaOut && (nTake > 0)) {
                pbaOut->append(static_cast<int>(nTake), cValue);
            }

            nOutPos += nTake;
        }
    }

    if (nOutPos != nUncompressedSize) {
        return false;
    }

    if (pnConsumed) {
        *pnConsumed = nInPos;
    }

    return true;
}

}  // namespace

bool XSilmarilsDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked) {
        return false;
    }

    qint64 nConsumed = 0;

    if (!silmarilsWalk(baPacked, nUncompressedSize, pbaUnpacked, &nConsumed)) {
        pbaUnpacked->clear();
        return false;
    }

    if (nConsumed != baPacked.size()) {
        pbaUnpacked->clear();
        return false;
    }

    return pbaUnpacked->size() == nUncompressedSize;
}

bool XSilmarilsDecoder::probe(const QByteArray &baPacked, qint64 nUncompressedSize)
{
    qint64 nConsumed = 0;

    if (!silmarilsWalk(baPacked, nUncompressedSize, nullptr, &nConsumed)) {
        return false;
    }

    return nConsumed == baPacked.size();
}
