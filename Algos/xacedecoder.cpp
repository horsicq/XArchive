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
// ACE decompression: direct port of uac_dcpr.c from wdlkmpx/unace1.
// Bit reader: 32-bit little-endian DWORDs, bits extracted MSB-first.
// Huffman: meta-Huffman + delta-encoded widths (not LZH-style).
#include "xacedecoder.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

#include <QBuffer>
#include <QFileDevice>

XAceDecoder::XAceDecoder(QObject *pParent) : QObject(pParent)
{
}

// -------------------------------------------------------------------
// Bit reader: mirroring readdat() + addbits() from uac_dcpr.c
// -------------------------------------------------------------------
qint32 XAceDecoder::readInput(AceDecodeState *pState, char *pBuffer, qint32 nSize)
{
    if (!pState || !pState->pInput || (nSize < 0) || ((nSize > 0) && !pBuffer) || (pState->nInputLimit < 0) || (pState->nInputBytesRead < 0) ||
        (pState->nInputBytesRead > pState->nInputLimit)) {
        if (pState) {
            pState->bError = true;
            pState->bReadError = true;
        }
        return -1;
    }

    const qint32 nTarget = (qint32)qMin((qint64)nSize, pState->nInputLimit - pState->nInputBytesRead);
    qint32 nTotal = 0;
    QPointer<QIODevice> guardedInput(pState->pInput);
    if (!guardedInput) {
        pState->bError = true;
        pState->bReadError = true;
        return -1;
    }

    while (nTotal < nTarget) {
        const qint64 nRead = guardedInput->read(pBuffer + nTotal, nTarget - nTotal);
        if (!guardedInput || (nRead <= 0) || (nRead > (qint64)(nTarget - nTotal))) {
            pState->bError = true;
            pState->bReadError = true;
            return -1;
        }
        nTotal += (qint32)nRead;
        pState->nInputBytesRead += nRead;
    }

    return nTotal;
}

void XAceDecoder::readDat(AceDecodeState *pState)
{
    if (!pState || (pState->nRPos != (ACE_SIZE_RDB - 2))) {
        if (pState) pState->bError = true;
        return;
    }

    // Slide the last 2 DWORDs to positions 0 and 1
    pState->nRPos -= (ACE_SIZE_RDB - 2);
    pState->nBufRd[0] = pState->nBufRd[ACE_SIZE_RDB - 2];
    pState->nBufRd[1] = pState->nBufRd[ACE_SIZE_RDB - 1];

    // Read (ACE_SIZE_RDB - 2) DWORDs = (ACE_SIZE_RDB - 2) * 4 bytes
    qint32 nBytesToRead = (ACE_SIZE_RDB - 2) * 4;
    const qint32 nRead = readInput(pState, (char *)&pState->nBufRd[2], nBytesToRead);
    if (nRead < 0) {
        memset(&pState->nBufRd[2], 0, (size_t)nBytesToRead);
        return;
    }
    if (nRead < nBytesToRead) {
        memset((char *)&pState->nBufRd[2] + nRead, 0, (size_t)(nBytesToRead - nRead));
    }

    // On big-endian systems the DWORDs need byte-swapping; on x86 (LE) no-op.
    // Qt provides Q_BYTE_ORDER; swap if big-endian.
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    for (qint32 i = 2; i < ACE_SIZE_RDB; i++) {
        pState->nBufRd[i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(&pState->nBufRd[i]));
    }
#endif
}

void XAceDecoder::addBits(AceDecodeState *pState, qint32 nBits)
{
    if (!pState || pState->bError || (nBits < 0) || (nBits > 32) || (pState->nRPos < 0) || (pState->nRPos >= (ACE_SIZE_RDB - 2)) || (pState->nBitsRd < 0) ||
        (pState->nBitsRd > 31) || (pState->nInputLimit < 0) || (pState->nBitsConsumed < 0)) {
        if (pState) pState->bError = true;
        return;
    }

    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    const qint64 nAvailableBits = (pState->nInputLimit > (nMax / 8)) ? nMax : pState->nInputLimit * 8;
    if ((pState->nBitsConsumed > nAvailableBits) || (nBits > (nAvailableBits - pState->nBitsConsumed))) {
        pState->bError = true;
        pState->bReadError = true;
        return;
    }
    pState->nBitsConsumed += nBits;

    const qint32 nAccumulatedBits = pState->nBitsRd + nBits;
    pState->nRPos += nAccumulatedBits >> 5;
    pState->nBitsRd = nAccumulatedBits & 31;

    if (pState->nRPos > (ACE_SIZE_RDB - 2)) {
        pState->bError = true;
        return;
    }

    if (pState->nRPos == (ACE_SIZE_RDB - 2)) {
        readDat(pState);
        if (pState->bError) return;
    }

    // Rebuild code_rd: (buf[rpos] << bits_rd) + ((buf[rpos+1] >> (32-bits_rd)) & mask)
    quint32 nHi = pState->nBufRd[pState->nRPos] << pState->nBitsRd;
    quint32 nLo;

    if (pState->nBitsRd == 0) {
        nLo = 0;
    } else {
        nLo = pState->nBufRd[pState->nRPos + 1] >> (32 - pState->nBitsRd);
    }

    pState->nCodeRd = nHi + nLo;
}

quint32 XAceDecoder::peekBits(AceDecodeState *pState, qint32 nBits)
{
    if (!pState || pState->bError || (nBits <= 0) || (nBits > 32)) {
        if (pState) pState->bError = true;
        return 0;
    }
    return pState->nCodeRd >> (32 - nBits);
}

// -------------------------------------------------------------------
// Quicksort: exact port of sortrange()/quicksort() from uac_dcpr.c
// sort_freq[] descending; sort_org[] tracks original indices.
// -------------------------------------------------------------------
void XAceDecoder::sortRange(AceDecodeState *pState, qint32 nLeft, qint32 nRight)
{
    qint32 nZl = nLeft;
    qint32 nZr = nRight;
    quint8 nHyphen = pState->nSortFreq[nRight];

    do {
        while (pState->nSortFreq[nZl] > nHyphen) {
            nZl++;
        }

        while (pState->nSortFreq[nZr] < nHyphen) {
            nZr--;
        }

        if (nZl <= nZr) {
            quint8 nTmpF = pState->nSortFreq[nZl];
            pState->nSortFreq[nZl] = pState->nSortFreq[nZr];
            pState->nSortFreq[nZr] = nTmpF;
            quint16 nTmpO = pState->nSortOrg[nZl];
            pState->nSortOrg[nZl] = pState->nSortOrg[nZr];
            pState->nSortOrg[nZr] = nTmpO;
            nZl++;
            nZr--;
        }
    } while (nZl < nZr);

    if (nLeft < nZr) {
        if (nLeft < nZr - 1) {
            sortRange(pState, nLeft, nZr);
        } else if (pState->nSortFreq[nLeft] < pState->nSortFreq[nZr]) {
            quint8 nTmpF = pState->nSortFreq[nLeft];
            pState->nSortFreq[nLeft] = pState->nSortFreq[nZr];
            pState->nSortFreq[nZr] = nTmpF;
            quint16 nTmpO = pState->nSortOrg[nLeft];
            pState->nSortOrg[nLeft] = pState->nSortOrg[nZr];
            pState->nSortOrg[nZr] = nTmpO;
        }
    }

    if (nRight > nZl) {
        if (nZl < nRight - 1) {
            sortRange(pState, nZl, nRight);
        } else if (pState->nSortFreq[nZl] < pState->nSortFreq[nRight]) {
            quint8 nTmpF = pState->nSortFreq[nZl];
            pState->nSortFreq[nZl] = pState->nSortFreq[nRight];
            pState->nSortFreq[nRight] = nTmpF;
            quint16 nTmpO = pState->nSortOrg[nZl];
            pState->nSortOrg[nZl] = pState->nSortOrg[nRight];
            pState->nSortOrg[nRight] = nTmpO;
        }
    }
}

void XAceDecoder::quickSort(AceDecodeState *pState, qint32 nN)
{
    for (qint32 i = nN + 1; i--;) {
        pState->nSortOrg[i] = (quint16)i;
    }

    sortRange(pState, 0, nN);
}

// -------------------------------------------------------------------
// makecode: port of makecode() from uac_dcpr.c.
// Builds canonical Huffman decode table in pCode[0..2^maxwd).
// pWd[0..size1t]: code widths (input, may be modified for 1-symbol case).
// Returns 1 on success, 0 on error (overcomplete tree).
// -------------------------------------------------------------------
qint32 XAceDecoder::makeCode(AceDecodeState *pState, quint32 nMaxWd, quint32 nSize1t, quint8 *pWd, quint16 *pCode)
{
    if (!pState || !pWd || !pCode || (nMaxWd == 0) || (nMaxWd > (quint32)ACE_MAXWD_MN) || (nSize1t > (quint32)ACE_MAX_CD_MN)) {
        if (pState) pState->bError = true;
        return 0;
    }

    // Copy widths into sort_freq, with nSortOrg = identity
    memcpy(pState->nSortFreq, pWd, (nSize1t + 1) * sizeof(quint8));

    if (nSize1t > 0) {
        quickSort(pState, (qint32)nSize1t);
    } else {
        pState->nSortOrg[0] = 0;
    }

    // Find size2_t: index of first zero in sorted (descending) -- i.e. count non-zero
    pState->nSortFreq[nSize1t + 1] = 0;
    quint32 nSize2t = 0;

    while (pState->nSortFreq[nSize2t]) {
        nSize2t++;
    }

    // Handle 0 or 1 non-zero symbol
    if (nSize2t < 2) {
        quint16 nIdx = pState->nSortOrg[0];
        pWd[nIdx] = 1;

        if (nSize2t == 0) {
            nSize2t = 1;
        }
    }

    nSize2t--;

    quint32 nMaxMakeCode = quint32(1) << nMaxWd;
    quint32 nC = 0;

    // Fill decode table: iterate sorted entries from index nSize2t down to 0
    for (quint32 i = nSize2t + 1; i-- != 0 && nC < nMaxMakeCode;) {
        const quint32 nWidth = pState->nSortFreq[i];
        if (nWidth > nMaxWd) {
            pState->bError = true;
            return 0;
        }
        quint32 nMaxc = quint32(1) << (nMaxWd - nWidth);
        quint32 nL = pState->nSortOrg[i];

        if (nMaxc > nMaxMakeCode - nC) {
            pState->bError = true;
            return 0;
        }

        // memset16 equivalent: fill nMaxc shorts with value nL
        for (quint32 k = 0; k < nMaxc; k++) {
            pCode[nC + k] = (quint16)nL;
        }

        nC += nMaxc;
    }

    return 1;
}

// -------------------------------------------------------------------
// read_wd: port of read_wd() from uac_dcpr.c.
// Reads one Huffman table from the bitstream.
// -------------------------------------------------------------------
qint32 XAceDecoder::readWd(AceDecodeState *pState, quint32 nMaxWd, quint16 *pCode, quint8 *pWd, quint32 nMaxEl)
{
    if (!pState || !pCode || !pWd || (nMaxWd == 0) || (nMaxWd > (quint32)ACE_MAXWD_MN) || (nMaxEl > (quint32)ACE_MAX_CD_MN)) {
        if (pState) pState->bError = true;
        return 0;
    }

    memset(pWd, 0, (nMaxEl + 1) * sizeof(quint8));
    const size_t nCodeCount = (size_t)1 << nMaxWd;
    const quint16 nInvalidCode = (std::numeric_limits<quint16>::max)();
    std::fill_n(pCode, nCodeCount, nInvalidCode);
    std::fill_n(pState->nCodeSv, (size_t)ACE_CODE_SV_SZ, nInvalidCode);

    // Read num_el (9 bits)
    quint32 nNumEl = peekBits(pState, 9);
    addBits(pState, 9);

    if (pState->bError || (nNumEl > nMaxEl)) {
        pState->bError = true;
        return 0;
    }

    // Read lolim (4 bits)
    quint32 nLolim = peekBits(pState, 4);
    addBits(pState, 4);
    if (pState->bError) return 0;

    // Read uplim (4 bits)
    quint32 nUplim = peekBits(pState, 4);
    addBits(pState, 4);
    if (pState->bError || (nUplim > (quint32)ACE_SVWD_CNT)) {
        pState->bError = true;
        return 0;
    }

    // Read meta-Huffman widths: wd_svwd[0..uplim] each 3 bits
    for (quint32 i = 0; i <= nUplim; i++) {
        pState->nWdSvwd[i] = (quint8)peekBits(pState, 3);
        addBits(pState, 3);
        if (pState->bError) return 0;
    }

    // Build meta-Huffman table
    if (!makeCode(pState, (quint32)ACE_MAXWD_SVWD, nUplim, pState->nWdSvwd, pState->nCodeSv)) {
        return 0;
    }

    // Decode num_el+1 widths using meta-Huffman
    quint32 j = 0;

    while (j <= nNumEl) {
        quint32 nC = pState->nCodeSv[peekBits(pState, ACE_MAXWD_SVWD)];
        if (pState->bError || (nC > nUplim) || (nC > (quint32)ACE_SVWD_CNT) || (pState->nWdSvwd[nC] == 0)) {
            pState->bError = true;
            return 0;
        }
        addBits(pState, pState->nWdSvwd[nC]);
        if (pState->bError) return 0;

        if (nC < nUplim) {
            pWd[j++] = (quint8)nC;
        } else {
            // Run of zeros: read 4 bits for run length - 4
            quint32 nRunLen = (peekBits(pState, 4)) + 4;
            addBits(pState, 4);
            if (pState->bError) return 0;

            for (quint32 k = 0; k < nRunLen && j <= nNumEl; k++) {
                pWd[j++] = 0;
            }
        }
    }

    // Delta decode widths: wd[i] = (wd[i] + wd[i-1]) % uplim   for i=1..num_el
    if (nUplim > 0) {
        for (quint32 i = 1; i <= nNumEl; i++) {
            pWd[i] = (quint8)((pWd[i] + pWd[i - 1]) % nUplim);
        }
    }

    // Add lolim to non-zero entries
    for (quint32 i = 0; i <= nNumEl; i++) {
        if (pWd[i]) {
            pWd[i] += (quint8)nLolim;
        }
    }

    return makeCode(pState, nMaxWd, nNumEl, pWd, pCode);
}

// -------------------------------------------------------------------
// calc_dectabs: port of calc_dectabs() from uac_dcpr.c.
// -------------------------------------------------------------------
qint32 XAceDecoder::calcDecTabs(AceDecodeState *pState)
{
    if (!readWd(pState, (quint32)ACE_MAXWD_MN, pState->nCodeMn, pState->nWdMn, (quint32)ACE_MAX_CD_MN)) {
        return 0;
    }

    if (!readWd(pState, (quint32)ACE_MAXWD_LG, pState->nCodeLg, pState->nWdLg, (quint32)ACE_MAX_CD_LG)) {
        return 0;
    }

    // blocksize: 15 bits
    pState->nBlockSize = (qint32)peekBits(pState, 15);
    addBits(pState, 15);

    if (pState->bError || (pState->nBlockSize <= 0)) {
        pState->bError = true;
        return 0;
    }

    return 1;
}

// -------------------------------------------------------------------
// copystr: port of copystr() from uac_dcpr.c.
// -------------------------------------------------------------------
void XAceDecoder::copyStr(AceDecodeState *pState, qint32 nDist, qint32 nLen)
{
    if (!pState || !pState->pText || (nDist <= 0) || (nDist > pState->nDicSiz) || (nDist > pState->nHistorySize) || (nLen <= 0) || (nLen > ACE_MAXLENGTH) ||
        (pState->nDcrDo < 0) || (pState->nDcrSize < nLen) || ((qint64)pState->nDcrDo > (pState->nDcrSize - nLen)) ||
        (pState->nDcrDo > ((std::numeric_limits<qint32>::max)() - nLen))) {
        if (pState) pState->bError = true;
        return;
    }

    pState->nDcrDo += nLen;

    qint32 nMPos = pState->nDPos - nDist;
    nMPos &= pState->nDicAnd;

    if ((nMPos >= pState->nDicSiz - ACE_MAXLENGTH) || (pState->nDPos >= pState->nDicSiz - ACE_MAXLENGTH)) {
        // Safe (wrapping) copy
        for (qint32 i = 0; i < nLen; i++) {
            pState->pText[pState->nDPos] = pState->pText[nMPos];
            pState->nDPos = (pState->nDPos + 1) & pState->nDicAnd;
            nMPos = (nMPos + 1) & pState->nDicAnd;
        }
    } else {
        // Fast (non-wrapping) copy
        for (qint32 i = 0; i < nLen; i++) {
            pState->pText[pState->nDPos++] = pState->pText[nMPos++];
        }

        pState->nDPos &= pState->nDicAnd;
    }

    pState->nHistorySize = qMin(pState->nDicSiz, pState->nHistorySize + nLen);
}

// -------------------------------------------------------------------
// decompressBlock: port of decompress() from uac_dcpr.c.
// Outputs up to nDcrDoMax symbols into ring buffer pText.
// -------------------------------------------------------------------
void XAceDecoder::decompressBlock(AceDecodeState *pState)
{
    while (pState->nDcrDo < pState->nDcrDoMax) {
        if (pState->bError) {
            return;
        }

        if (!pState->pProgressLifetime || !XBinary::isPdStructLifetimeAlive(*pState->pProgressLifetime) || !XBinary::isPdStructNotCanceled(pState->pPdStruct)) {
            pState->bError = true;
            return;
        }

        if (!pState->nBlockSize) {
            if (!calcDecTabs(pState)) {
                pState->bError = true;
                return;
            }
        }

        // Decode main symbol
        const quint32 nMainIndex = peekBits(pState, ACE_MAXWD_MN);
        if (pState->bError) return;
        qint32 nC = (qint32)pState->nCodeMn[nMainIndex];
        if ((nC < 0) || (nC > ACE_MAX_CD_MN) || (pState->nWdMn[nC] == 0) || (pState->nWdMn[nC] > ACE_MAXWD_MN)) {
            pState->bError = true;
            return;
        }
        addBits(pState, pState->nWdMn[nC]);
        if (pState->bError) return;
        pState->nBlockSize--;

        if (nC > 255) {
            qint32 nI;
            quint32 nDist;

            if (nC > 259) {
                // New distance reference
                nC -= 260;

                if ((nC < 0) || (nC > ACE_MAXDIC)) {
                    pState->bError = true;
                    return;
                }

                if (nC > 1) {
                    nDist = (pState->nCodeRd >> (33 - nC)) + (quint32(1) << (nC - 1));
                    addBits(pState, nC - 1);
                    if (pState->bError) return;
                } else {
                    nDist = (quint32)nC;
                }

                pState->nOldNum = (pState->nOldNum + 1) & 3;
                pState->nOldDist[pState->nOldNum] = nDist;
                nI = 2;

                if (nDist > (quint32)ACE_MAXDIS2) {
                    nI++;

                    if (nDist > (quint32)ACE_MAXDIS3) {
                        nI++;
                    }
                }
            } else {
                // Old distance reference (symbols 256-259)
                qint32 nRef = nC & 255;  // 0..3
                nDist = pState->nOldDist[(pState->nOldNum - nRef) & 3];

                // Rotate nOldDist so the referenced entry becomes current
                for (qint32 k = nRef + 1; k--;) {
                    pState->nOldDist[(pState->nOldNum - k) & 3] = pState->nOldDist[(pState->nOldNum - k + 1) & 3];
                }

                pState->nOldDist[pState->nOldNum] = nDist;
                nI = 2;

                if (nRef > 1) {
                    nI++;
                }
            }

            // Decode length
            const quint32 nLengthIndex = peekBits(pState, ACE_MAXWD_LG);
            if (pState->bError) return;
            qint32 nLg = (qint32)pState->nCodeLg[nLengthIndex];
            if ((nLg < 0) || (nLg > ACE_MAX_CD_LG) || (pState->nWdLg[nLg] == 0) || (pState->nWdLg[nLg] > ACE_MAXWD_LG)) {
                pState->bError = true;
                return;
            }
            addBits(pState, pState->nWdLg[nLg]);
            if (pState->bError || (nDist == (std::numeric_limits<quint32>::max)())) {
                pState->bError = true;
                return;
            }
            nDist++;
            nLg += nI;

            copyStr(pState, (qint32)nDist, nLg);
            if (pState->bError) return;
        } else {
            // Literal
            if ((pState->nDcrDo < 0) || ((qint64)pState->nDcrDo >= pState->nDcrSize)) {
                pState->bError = true;
                return;
            }
            pState->nDcrDo++;
            pState->pText[pState->nDPos] = (char)nC;
            pState->nDPos = (pState->nDPos + 1) & pState->nDicAnd;
            pState->nHistorySize = qMin(pState->nDicSiz, pState->nHistorySize + 1);
        }
    }
}

// -------------------------------------------------------------------
// decompressBlk: port of decompress_blk() from uac_dcpr.c.
// Fills pBuf with up to nLen decompressed bytes.
// Returns number of bytes actually written.
// -------------------------------------------------------------------
qint32 XAceDecoder::decompressBlk(AceDecodeState *pState, char *pBuf, qint32 nLen)
{
    if (!pState || !pBuf || (nLen <= ACE_MAXLENGTH) || (pState->nDcrSize <= 0)) {
        if (pState) pState->bError = true;
        return 0;
    }

    qint32 nOldPos = pState->nDPos;

    pState->nDcrDo = 0;
    pState->nDcrDoMax = nLen - ACE_MAXLENGTH;

    if ((qint64)pState->nDcrDoMax > pState->nDcrSize) {
        pState->nDcrDoMax = (qint32)pState->nDcrSize;
    }

    if (pState->nDcrSize > 0 && pState->nDcrDoMax > 0) {
        decompressBlock(pState);

        if (pState->bError || (pState->nDcrDo <= 0) || (pState->nDcrDo > nLen) || ((qint64)pState->nDcrDo > pState->nDcrSize)) {
            pState->bError = true;
            return 0;
        }

        if (nOldPos + pState->nDcrDo > pState->nDicSiz) {
            qint32 nFirst = pState->nDicSiz - nOldPos;
            memcpy(pBuf, pState->pText + nOldPos, (size_t)nFirst);
            memcpy(pBuf + nFirst, pState->pText, (size_t)(pState->nDcrDo - nFirst));
        } else {
            memcpy(pBuf, pState->pText + nOldPos, (size_t)pState->nDcrDo);
        }
    }

    pState->nDcrSize -= pState->nDcrDo;
    return pState->nDcrDo;
}

// -------------------------------------------------------------------
// decompressInternal: main entry, drives decompressBlk in a loop.
// -------------------------------------------------------------------
bool XAceDecoder::decompressInternal(XBinary::DATAPROCESS_STATE *pDecompressState, QIODevice *pOutput, XBinary::PDSTRUCT *pPdStruct,
                                     const XBinary::PDSTRUCTLIFETIME &progressLifetime)
{
    QPointer<QIODevice> guardedInput(pDecompressState ? pDecompressState->pDeviceInput : nullptr);
    QPointer<QIODevice> guardedOutput(pOutput);
    if (!pDecompressState || !guardedInput || !guardedOutput || (pDecompressState->nInputLimit < 0) || ((pDecompressState->nInputLimit & 3) != 0) ||
        (pDecompressState->nInputLimit > ((std::numeric_limits<qint64>::max)() / 8)) || !XBinary::isPdStructLifetimeAlive(progressLifetime) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pDecompressState && (!guardedInput || (pDecompressState->nInputLimit < 0))) {
            pDecompressState->bReadError = true;
        }
        if (pDecompressState && !guardedOutput) pDecompressState->bWriteError = true;
        return false;
    }

    if (!pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        return false;
    }

    qint64 nOrigSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    if (nOrigSize < 0) {
        return false;
    }

    if (nOrigSize == 0) {
        pDecompressState->nCountInput = 0;
        pDecompressState->nCountOutput = 0;
        return pDecompressState->nInputLimit == 0;
    }

    // XACE converts TECH.PARM's low nibble to an exact power-of-two window.
    // Keep the legacy 1 MiB default for callers that do not provide one, but
    // reject malformed/non-power-of-two values instead of silently changing it.
    const qint64 nDictionarySize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_WINDOWSIZE, (qint64)(1 << 20)).toLongLong();
    qint32 nDicBits = 10;
    qint64 nExpectedDictionarySize = (qint64)1 << nDicBits;
    while ((nExpectedDictionarySize < nDictionarySize) && (nDicBits < ACE_MAXDIC)) {
        nDicBits++;
        nExpectedDictionarySize <<= 1;
    }
    if ((nDictionarySize < ((qint64)1 << 10)) || (nDictionarySize > ((qint64)1 << ACE_MAXDIC)) || (nExpectedDictionarySize != nDictionarySize)) {
        return false;
    }

    std::unique_ptr<AceDecodeState> pState(new (std::nothrow) AceDecodeState());
    if (!pState) return false;
    memset(pState.get(), 0, sizeof(AceDecodeState));
    AceDecodeState &state = *pState;

    state.nDicSiz = 1 << nDicBits;
    state.nDicAnd = state.nDicSiz - 1;
    std::unique_ptr<char[]> pDictionary(new (std::nothrow) char[(size_t)state.nDicSiz]);
    if (!pDictionary) return false;
    state.pText = pDictionary.get();

    memset(state.pText, 0, (size_t)state.nDicSiz);

    state.pInput = guardedInput.data();
    state.pPdStruct = pPdStruct;
    state.pProgressLifetime = &progressLifetime;
    state.nInputBytesRead = 0;
    state.nInputLimit = pDecompressState->nInputLimit;
    state.nDcrSize = nOrigSize;
    state.nDPos = 0;
    state.nHistorySize = 0;
    state.nOldNum = 0;
    state.nBlockSize = 0;
    state.bError = false;
    state.bReadError = false;
    memset(state.nOldDist, 0, sizeof(state.nOldDist));

    // Initial fill: read size_rdb DWORDs = size_rdb * 4 bytes
    qint32 nInitBytes = ACE_SIZE_RDB * 4;
    const qint32 nRead = readInput(&state, (char *)state.nBufRd, nInitBytes);
    if (!guardedInput || (nRead < 0)) {
        pDecompressState->nCountInput = state.nInputBytesRead;
        pDecompressState->bReadError = true;
        return false;
    }
    if (nRead < nInitBytes) {
        memset((char *)state.nBufRd + nRead, 0, (size_t)(nInitBytes - nRead));
    }

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    for (qint32 i = 0; i < ACE_SIZE_RDB; i++) {
        state.nBufRd[i] = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(&state.nBufRd[i]));
    }
#endif

    state.nBitsRd = 0;
    state.nRPos = 0;
    state.nCodeRd = state.nBufRd[0];
    state.nBitsConsumed = 0;

    // Decompression loop: drain blocks of at most (DicSiz) bytes at once
    const qint32 nChunkSize = state.nDicSiz;
    std::unique_ptr<char[]> pChunkBuffer(new (std::nothrow) char[(size_t)nChunkSize]);
    if (!pChunkBuffer) return false;

    XBinary::DATAPROCESS_STATE writeState = *pDecompressState;
    writeState.pDeviceOutput = guardedOutput.data();
    writeState.nCountOutput = 0;
    writeState.bWriteError = false;
    if (pOutput != pDecompressState->pDeviceOutput) {
        writeState.nProcessedOffset = 0;
        writeState.nProcessedLimit = -1;
    }

    bool bOk = true;

    while (state.nDcrSize > 0 && !state.bError) {
        if (!guardedInput || !guardedOutput || !XBinary::isPdStructLifetimeAlive(progressLifetime) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            bOk = false;
            break;
        }

        const qint64 nRemainingBefore = state.nDcrSize;
        qint32 nDone = decompressBlk(&state, pChunkBuffer.get(), nChunkSize);

        if (!guardedInput || !guardedOutput || !XBinary::isPdStructLifetimeAlive(progressLifetime) || (nDone <= 0) || ((qint64)nDone > nRemainingBefore) ||
            (state.nDcrSize != (nRemainingBefore - nDone))) {
            state.bError = true;
            bOk = false;
            break;
        }

        if (XBinary::_writeDevice(pChunkBuffer.get(), nDone, &writeState) != nDone || !guardedOutput) {
            bOk = false;
            break;
        }
    }

    pDecompressState->nCountInput = state.nInputBytesRead;
    pDecompressState->nCountOutput = writeState.nCountOutput;
    pDecompressState->bReadError = pDecompressState->bReadError || state.bReadError;
    pDecompressState->bWriteError = pDecompressState->bWriteError || writeState.bWriteError;

    const qint64 nAvailableBits = state.nInputLimit * 8;
    bOk = bOk && !state.bError && !state.bReadError && !writeState.bWriteError && guardedInput && guardedOutput && XBinary::isPdStructLifetimeAlive(progressLifetime) &&
          XBinary::isPdStructNotCanceled(pPdStruct) && (state.nDcrSize == 0) && (state.nBlockSize == 0) && (writeState.nCountOutput == nOrigSize) &&
          (state.nBitsConsumed <= nAvailableBits) && ((nAvailableBits - state.nBitsConsumed) < 32);

    return bOk;
}

bool XAceDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState) {
        return false;
    }

    XBinary::PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    const XBinary::PDSTRUCTLIFETIME progressLifetime = XBinary::retainPdStructLifetime(pPdStruct);
    QPointer<QIODevice> guardedInput(pDecompressState->pDeviceInput);
    QPointer<QIODevice> guardedOutput(pDecompressState->pDeviceOutput);

    pDecompressState->bReadError = false;
    pDecompressState->bWriteError = false;
    pDecompressState->nCountInput = 0;
    pDecompressState->nCountOutput = 0;

    if (!guardedInput) {
        pDecompressState->bReadError = true;
        return false;
    }
    if (!guardedOutput) {
        pDecompressState->bWriteError = true;
        return false;
    }
    if (!XBinary::isPdStructLifetimeAlive(progressLifetime)) return false;
    if (XBinary::devicesAlias(guardedInput.data(), guardedOutput.data()) || !guardedInput || !guardedOutput) {
        pDecompressState->bReadError = true;
        pDecompressState->bWriteError = true;
        return false;
    }

    // Algo_utils::prepareState operates on raw pointers. Do the ACE preflight
    // locally so a hostile seek()/pos() implementation cannot leave the next
    // access pointed at a synchronously deleted device.
    if (!guardedInput || (pDecompressState->nInputOffset < 0)) {
        pDecompressState->bReadError = true;
    } else {
        const bool bInputSeeked = guardedInput->seek(pDecompressState->nInputOffset);
        if (!guardedInput || !bInputSeeked) {
            pDecompressState->bReadError = true;
        } else {
            const qint64 nInputPosition = guardedInput->pos();
            if (!guardedInput || (nInputPosition != pDecompressState->nInputOffset)) {
                pDecompressState->bReadError = true;
            }
        }
    }

    if (!guardedOutput) {
        pDecompressState->bWriteError = true;
    } else {
        const bool bOutputSeeked = guardedOutput->seek(0);
        if (!guardedOutput || !bOutputSeeked) {
            pDecompressState->bWriteError = true;
        } else {
            const qint64 nOutputPosition = guardedOutput->pos();
            if (!guardedOutput || (nOutputPosition != 0)) {
                pDecompressState->bWriteError = true;
            }
        }
    }

    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructLifetimeAlive(progressLifetime) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // The public decoder owns the destination contents.  Seeking to zero is
    // insufficient for reusable random-access devices because a shorter
    // decode would otherwise leave stale suffix bytes behind.
    if (!guardedOutput->isWritable() || !guardedOutput) {
        pDecompressState->bWriteError = true;
        return false;
    }

    const bool bSequentialOutput = guardedOutput->isSequential();
    if (!guardedOutput) {
        pDecompressState->bWriteError = true;
        return false;
    }
    qint64 nOutputSize = 0;
    if (!bSequentialOutput) {
        nOutputSize = guardedOutput->size();
        if (!guardedOutput || (nOutputSize < 0)) {
            pDecompressState->bWriteError = true;
            return false;
        }
    }

    if (!bSequentialOutput && (nOutputSize > 0)) {
        bool bTruncated = false;

        QBuffer *pBuffer = dynamic_cast<QBuffer *>(guardedOutput.data());
        QPointer<QBuffer> guardedBuffer(pBuffer);
        if (!guardedOutput) {
            pDecompressState->bWriteError = true;
            return false;
        }
        if (guardedBuffer) {
            guardedBuffer->buffer().clear();
            if (guardedOutput && guardedBuffer) {
                bTruncated = guardedBuffer->seek(0);
                bTruncated = guardedOutput && guardedBuffer && bTruncated;
            }
        } else {
            QFileDevice *pFile = dynamic_cast<QFileDevice *>(guardedOutput.data());
            QPointer<QFileDevice> guardedFile(pFile);
            if (!guardedOutput) {
                pDecompressState->bWriteError = true;
                return false;
            }
            if (guardedFile) {
                const bool bResized = guardedFile->resize(0);
                if (guardedOutput && guardedFile && bResized) {
                    const bool bSeeked = guardedFile->seek(0);
                    bTruncated = guardedOutput && guardedFile && bSeeked;
                }
            }
        }

        if (!guardedOutput || !bTruncated) {
            pDecompressState->bWriteError = true;
            return false;
        }
    }

    if ((pDecompressState->nInputLimit < 0) || ((pDecompressState->nInputLimit & 3) != 0) ||
        (pDecompressState->nInputLimit > ((std::numeric_limits<qint64>::max)() / 8))) {
        pDecompressState->bReadError = true;
        return false;
    }

    if (!guardedInput || !guardedOutput || !XBinary::isPdStructLifetimeAlive(progressLifetime)) return false;
    return decompressInternal(pDecompressState, guardedOutput.data(), pPdStruct, progressLifetime);
}

bool XAceDecoder::decompressDelta(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    if (pDecompressState) {
        pDecompressState->bReadError = false;
        pDecompressState->bWriteError = false;
        pDecompressState->nCountInput = 0;
        pDecompressState->nCountOutput = 0;
    }

    // ACE TECH.TYPE 2 is ACE 2.x BLOCKED_1, which multiplexes several codec
    // families. Treating it as method 1 followed by a byte delta is corrupt.
    return false;
}
