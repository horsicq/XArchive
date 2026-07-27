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
#include "xxpressdecoder.h"

namespace {

const qint32 XPRESS_NUM_SYMBOLS = 512;
const qint32 XPRESS_TABLE_BYTES = 256;  // 512 nibbles
const qint32 XPRESS_MAX_CODEWORD_LEN = 15;
const qint32 XPRESS_MIN_MATCH_LEN = 3;

// ---- Plain XPRESS LZ77 (MS-XCA 2.1) ----
//
// The stream is a sequence of: a 32-bit LE "flags" word (LSB-first), followed by
// tokens. A 0 flag bit => literal byte; a 1 flag bit => match. A match is a
// 16-bit LE value: high 13 bits = offset-1, low 3 bits = length-3 header; if the
// header == 7 the real (length-3) continues as a nibble, then a byte, then a
// 16-bit value, using the documented escalation.

bool xpress_plain(const QByteArray &baIn, QByteArray *pbaOut, qint32 nOutSize)
{
    const quint8 *pIn = reinterpret_cast<const quint8 *>(baIn.constData());
    qint64 nInSize = baIn.size();
    qint64 nInPos = 0;

    pbaOut->clear();
    pbaOut->reserve(nOutSize);

    quint32 nFlags = 0;
    qint32 nFlagCount = 0;
    qint32 nNibblePos = 0;  // position of a half-consumed nibble byte, 0 = none

    while ((qint32)pbaOut->size() < nOutSize) {
        if (nFlagCount == 0) {
            if (nInPos + 4 > nInSize) {
                return false;
            }
            nFlags = (quint32)pIn[nInPos] | ((quint32)pIn[nInPos + 1] << 8) | ((quint32)pIn[nInPos + 2] << 16) | ((quint32)pIn[nInPos + 3] << 24);
            nInPos += 4;
            nFlagCount = 32;
        }

        bool bMatch = (nFlags & 0x80000000u) != 0;
        nFlags <<= 1;
        nFlagCount--;

        if (!bMatch) {
            if (nInPos >= nInSize) {
                return false;
            }
            pbaOut->append((char)pIn[nInPos++]);
        } else {
            if (nInPos + 2 > nInSize) {
                return false;
            }
            quint32 nMatch = (quint32)pIn[nInPos] | ((quint32)pIn[nInPos + 1] << 8);
            nInPos += 2;

            qint32 nOffset = (qint32)(nMatch >> 3) + 1;
            qint32 nLength = (qint32)(nMatch & 0x07);

            if (nLength == 7) {
                // Escalate: nibble, then byte, then 16-bit
                qint32 nNibble;
                if (nNibblePos == 0) {
                    if (nInPos >= nInSize) {
                        return false;
                    }
                    nNibblePos = (qint32)nInPos;
                    nNibble = pIn[nInPos] & 0x0F;
                    nInPos++;
                } else {
                    nNibble = (pIn[nNibblePos] >> 4) & 0x0F;
                    nNibblePos = 0;
                }

                nLength = nNibble;

                if (nLength == 15) {
                    if (nInPos >= nInSize) {
                        return false;
                    }
                    nLength = pIn[nInPos++];

                    if (nLength == 255) {
                        if (nInPos + 2 > nInSize) {
                            return false;
                        }
                        nLength = (qint32)((quint32)pIn[nInPos] | ((quint32)pIn[nInPos + 1] << 8));
                        nInPos += 2;

                        if (nLength < 15 + 7) {
                            return false;
                        }
                        nLength -= (15 + 7);
                    }

                    nLength += 15;
                }

                nLength += 7;
            }

            nLength += XPRESS_MIN_MATCH_LEN;

            qint32 nOutPos = (qint32)pbaOut->size();
            if ((nOffset > nOutPos) || (nOutPos + nLength > nOutSize + 0)) {
                // Allow overrun only up to a small guard; strict bound keeps us safe
                if (nOffset > nOutPos) {
                    return false;
                }
            }

            qint32 nSrc = nOutPos - nOffset;
            for (qint32 i = 0; i < nLength; i++) {
                pbaOut->append(pbaOut->at(nSrc + i));
            }
        }
    }

    return ((qint32)pbaOut->size() == nOutSize);
}

// ---- XPRESS Huffman (MS-XCA 2.2) ----

struct XPRESS_HUFF {
    quint16 nCount[XPRESS_MAX_CODEWORD_LEN + 1];
    quint16 nSymbol[XPRESS_NUM_SYMBOLS];
    bool bValid;
};

struct XPRESS_BITS {
    const quint8 *pIn;
    qint64 nInSize;
    qint64 nInPos;
    quint32 nBitBuf;
    qint32 nBitCount;
    bool bError;
};

void xpress_initBits(XPRESS_BITS *pBits, const quint8 *pIn, qint64 nInSize, qint64 nStart)
{
    pBits->pIn = pIn;
    pBits->nInSize = nInSize;
    pBits->nInPos = nStart;
    pBits->nBitBuf = 0;
    pBits->nBitCount = 0;
    pBits->bError = false;

    // Prime with two 16-bit LE words in the high bits
    for (qint32 i = 0; i < 2; i++) {
        quint32 nWord = 0;
        if (pBits->nInPos + 1 < pBits->nInSize) {
            nWord = (quint32)pBits->pIn[pBits->nInPos] | ((quint32)pBits->pIn[pBits->nInPos + 1] << 8);
        }
        pBits->nInPos += 2;
        pBits->nBitBuf |= nWord << (16 - pBits->nBitCount);
        pBits->nBitCount += 16;
    }
}

quint32 xpress_readBits(XPRESS_BITS *pBits, qint32 nBits)
{
    if (nBits == 0) {
        return 0;
    }

    quint32 nResult = pBits->nBitBuf >> (32 - nBits);
    pBits->nBitBuf <<= nBits;
    pBits->nBitCount -= nBits;

    if (pBits->nBitCount < 16) {
        quint32 nWord = 0;
        if (pBits->nInPos + 1 < pBits->nInSize) {
            nWord = (quint32)pBits->pIn[pBits->nInPos] | ((quint32)pBits->pIn[pBits->nInPos + 1] << 8);
        } else if (pBits->nInPos < pBits->nInSize) {
            nWord = (quint32)pBits->pIn[pBits->nInPos];
        }
        pBits->nInPos += 2;
        pBits->nBitBuf |= nWord << (16 - pBits->nBitCount);
        pBits->nBitCount += 16;
    }

    return nResult;
}

bool xpress_buildHuff(XPRESS_HUFF *pTable, const quint8 *pLens)
{
    for (qint32 i = 0; i <= XPRESS_MAX_CODEWORD_LEN; i++) {
        pTable->nCount[i] = 0;
    }

    for (qint32 i = 0; i < XPRESS_NUM_SYMBOLS; i++) {
        pTable->nCount[pLens[i]]++;
    }

    qint32 nLeft = 1;
    for (qint32 nLen = 1; nLen <= XPRESS_MAX_CODEWORD_LEN; nLen++) {
        nLeft <<= 1;
        nLeft -= pTable->nCount[nLen];
        if (nLeft < 0) {
            return false;
        }
    }

    qint32 nOffsets[XPRESS_MAX_CODEWORD_LEN + 2];
    nOffsets[1] = 0;
    for (qint32 nLen = 1; nLen <= XPRESS_MAX_CODEWORD_LEN; nLen++) {
        nOffsets[nLen + 1] = nOffsets[nLen] + pTable->nCount[nLen];
    }

    for (qint32 i = 0; i < XPRESS_NUM_SYMBOLS; i++) {
        if (pLens[i]) {
            pTable->nSymbol[nOffsets[pLens[i]]++] = (quint16)i;
        }
    }

    pTable->bValid = true;
    return true;
}

qint32 xpress_decodeSym(XPRESS_BITS *pBits, const XPRESS_HUFF *pTable)
{
    qint32 nCode = 0;
    qint32 nFirst = 0;
    qint32 nIndex = 0;

    for (qint32 nLen = 1; nLen <= XPRESS_MAX_CODEWORD_LEN; nLen++) {
        nCode |= (qint32)xpress_readBits(pBits, 1);
        qint32 nCount = pTable->nCount[nLen];
        if (nCode - nFirst < nCount) {
            return pTable->nSymbol[nIndex + (nCode - nFirst)];
        }
        nIndex += nCount;
        nFirst += nCount;
        nFirst <<= 1;
        nCode <<= 1;
    }

    pBits->bError = true;
    return -1;
}

bool xpress_huffman(const QByteArray &baIn, QByteArray *pbaOut, qint32 nOutSize)
{
    if (baIn.size() < XPRESS_TABLE_BYTES) {
        return false;
    }

    const quint8 *pIn = reinterpret_cast<const quint8 *>(baIn.constData());

    quint8 lens[XPRESS_NUM_SYMBOLS];
    for (qint32 i = 0; i < XPRESS_TABLE_BYTES; i++) {
        lens[2 * i] = pIn[i] & 0x0F;
        lens[2 * i + 1] = (pIn[i] >> 4) & 0x0F;
    }

    XPRESS_HUFF table = {};
    if (!xpress_buildHuff(&table, lens)) {
        return false;
    }

    XPRESS_BITS bits;
    xpress_initBits(&bits, pIn, baIn.size(), XPRESS_TABLE_BYTES);

    pbaOut->clear();
    pbaOut->reserve(nOutSize);

    while (((qint32)pbaOut->size() < nOutSize) && !bits.bError) {
        qint32 nSym = xpress_decodeSym(&bits, &table);

        if (bits.bError || (nSym < 0)) {
            return false;
        }

        if (nSym < 256) {
            pbaOut->append((char)nSym);
        } else {
            qint32 nLength = nSym & 0x0F;
            qint32 nOffsetSlot = (nSym >> 4) & 0x0F;

            if (nLength == 15) {
                qint32 nExtra = (qint32)xpress_readBits(&bits, 8);
                if (nExtra == 255) {
                    nLength = (qint32)xpress_readBits(&bits, 16);
                    if (nLength < 15) {
                        return false;
                    }
                    nLength -= 15;
                } else {
                    nLength = nExtra;
                }
                nLength += 15;
            }

            nLength += XPRESS_MIN_MATCH_LEN;

            qint32 nOffset = (qint32)((1u << nOffsetSlot) + xpress_readBits(&bits, nOffsetSlot));

            qint32 nOutPos = (qint32)pbaOut->size();
            if ((nOffset > nOutPos) || (nOffset <= 0)) {
                return false;
            }

            qint32 nSrc = nOutPos - nOffset;
            for (qint32 i = 0; (i < nLength) && ((qint32)pbaOut->size() < nOutSize); i++) {
                pbaOut->append(pbaOut->at(nSrc + i));
            }
        }
    }

    return ((qint32)pbaOut->size() == nOutSize);
}

}  // namespace

bool XXPressDecoder::decompressPlain(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint32 nUncompressedSize)
{
    if (!pbaUncompressed || (nUncompressedSize <= 0)) {
        return false;
    }

    return xpress_plain(baCompressed, pbaUncompressed, nUncompressedSize);
}

bool XXPressDecoder::decompressHuffman(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint32 nUncompressedSize)
{
    if (!pbaUncompressed || (nUncompressedSize <= 0)) {
        return false;
    }

    return xpress_huffman(baCompressed, pbaUncompressed, nUncompressedSize);
}
