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

#include <cstring>

XAceDecoder::XAceDecoder(QObject *pParent) : QObject(pParent)
{
}

// -------------------------------------------------------------------
// Bit reader: mirroring readdat() + addbits() from uac_dcpr.c
// -------------------------------------------------------------------
void XAceDecoder::readDat(AceDecodeState *pState)
{
    // Slide the last 2 DWORDs to positions 0 and 1
    pState->nRPos -= (ACE_SIZE_RDB - 2);
    pState->nBufRd[0] = pState->nBufRd[ACE_SIZE_RDB - 2];
    pState->nBufRd[1] = pState->nBufRd[ACE_SIZE_RDB - 1];

    // Read (ACE_SIZE_RDB - 2) DWORDs = (ACE_SIZE_RDB - 2) * 4 bytes
    qint32 nBytesToRead = (ACE_SIZE_RDB - 2) * 4;
    qint64 nRemaining = pState->nInputLimit - pState->nInputBytesRead;

    if (nRemaining <= 0) {
        // Pad with zeros
        memset(&pState->nBufRd[2], 0, (size_t)nBytesToRead);
        return;
    }

    qint32 nToRead = (nRemaining < (qint64)nBytesToRead) ? (qint32)nRemaining : nBytesToRead;
    qint32 nRead = (qint32)pState->pInput->read((char *)&pState->nBufRd[2], nToRead);

    if (nRead < 0) {
        nRead = 0;
    }

    pState->nInputBytesRead += nRead;

    if (nRead < nBytesToRead) {
        memset((char *)&pState->nBufRd[2] + nRead, 0, (size_t)(nBytesToRead - nRead));
    }

    // On big-endian systems the DWORDs need byte-swapping; on x86 (LE) no-op.
    // Qt provides Q_BYTE_ORDER; swap if big-endian.
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    for (qint32 i = 2; i < ACE_SIZE_RDB; i++) {
        pState->nBufRd[i] = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(&pState->nBufRd[i]));
    }
#endif
}

void XAceDecoder::addBits(AceDecodeState *pState, qint32 nBits)
{
    pState->nRPos += (pState->nBitsRd += nBits) >> 5;
    pState->nBitsRd &= 31;

    if (pState->nRPos == (ACE_SIZE_RDB - 2)) {
        readDat(pState);
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

    quint32 nMaxMakeCode = (quint32)(1 << nMaxWd);
    quint32 nC = 0;

    // Fill decode table: iterate sorted entries from index nSize2t down to 0
    for (quint32 i = nSize2t + 1; i-- != 0 && nC < nMaxMakeCode;) {
        quint32 nMaxc = (quint32)(1 << (nMaxWd - pState->nSortFreq[i]));
        quint32 nL = pState->nSortOrg[i];

        if (nC + nMaxc > nMaxMakeCode) {
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
    memset(pWd, 0, (nMaxEl + 1) * sizeof(quint8));
    memset(pCode, 0, (quint32)(1 << nMaxWd) * sizeof(quint16));

    // Read num_el (9 bits)
    quint32 nNumEl = peekBits(pState, 9);
    addBits(pState, 9);

    if (nNumEl > nMaxEl) {
        nNumEl = nMaxEl;
    }

    // Read lolim (4 bits)
    quint32 nLolim = peekBits(pState, 4);
    addBits(pState, 4);

    // Read uplim (4 bits)
    quint32 nUplim = peekBits(pState, 4);
    addBits(pState, 4);

    // Read meta-Huffman widths: wd_svwd[0..uplim] each 3 bits
    for (quint32 i = 0; i <= nUplim; i++) {
        pState->nWdSvwd[i] = (quint8)peekBits(pState, 3);
        addBits(pState, 3);
    }

    // Build meta-Huffman table
    if (!makeCode(pState, (quint32)ACE_MAXWD_SVWD, nUplim, pState->nWdSvwd, pState->nCodeSv)) {
        return 0;
    }

    // Decode num_el+1 widths using meta-Huffman
    quint32 j = 0;

    while (j <= nNumEl) {
        quint32 nC = pState->nCodeSv[peekBits(pState, ACE_MAXWD_SVWD)];
        addBits(pState, pState->nWdSvwd[nC]);

        if (nC < nUplim) {
            pWd[j++] = (quint8)nC;
        } else {
            // Run of zeros: read 4 bits for run length - 4
            quint32 nRunLen = (peekBits(pState, 4)) + 4;
            addBits(pState, 4);

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

    return 1;
}

// -------------------------------------------------------------------
// copystr: port of copystr() from uac_dcpr.c.
// -------------------------------------------------------------------
void XAceDecoder::copyStr(AceDecodeState *pState, qint32 nDist, qint32 nLen)
{
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

        if (!pState->nBlockSize) {
            if (!calcDecTabs(pState)) {
                pState->bError = true;
                return;
            }
        }

        // Decode main symbol
        qint32 nC = (qint32)pState->nCodeMn[peekBits(pState, ACE_MAXWD_MN)];
        addBits(pState, pState->nWdMn[nC]);
        pState->nBlockSize--;

        if (nC > 255) {
            qint32 nI;
            quint32 nDist;

            if (nC > 259) {
                // New distance reference
                nC -= 260;

                if (nC > 1) {
                    nDist = (pState->nCodeRd >> (33 - nC)) + (quint32)(1 << (nC - 1));
                    addBits(pState, nC - 1);
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
            qint32 nLg = (qint32)pState->nCodeLg[peekBits(pState, ACE_MAXWD_LG)];
            addBits(pState, pState->nWdLg[nLg]);
            nDist++;
            nLg += nI;

            copyStr(pState, (qint32)nDist, nLg);
        } else {
            // Literal
            pState->nDcrDo++;
            pState->pText[pState->nDPos] = (char)nC;
            pState->nDPos = (pState->nDPos + 1) & pState->nDicAnd;
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
    qint32 nOldPos = pState->nDPos;

    pState->nDcrDo = 0;
    pState->nDcrDoMax = nLen - ACE_MAXLENGTH;

    if (pState->nDcrDoMax > (qint32)pState->nDcrSize) {
        pState->nDcrDoMax = (qint32)pState->nDcrSize;
    }

    if (pState->nDcrSize > 0 && pState->nDcrDoMax > 0) {
        decompressBlock(pState);

        if (pState->bError) {
            return 0;
        }

        if (pState->nDcrDo <= nLen) {
            if (nOldPos + pState->nDcrDo > pState->nDicSiz) {
                qint32 nFirst = pState->nDicSiz - nOldPos;
                memcpy(pBuf, pState->pText + nOldPos, (size_t)nFirst);
                memcpy(pBuf + nFirst, pState->pText, (size_t)(pState->nDcrDo - nFirst));
            } else {
                memcpy(pBuf, pState->pText + nOldPos, (size_t)pState->nDcrDo);
            }
        }
    }

    pState->nDcrSize -= pState->nDcrDo;
    return pState->nDcrDo;
}

// -------------------------------------------------------------------
// decompressInternal: main entry, drives decompressBlk in a loop.
// -------------------------------------------------------------------
bool XAceDecoder::decompressInternal(XBinary::DATAPROCESS_STATE *pDecompressState, QIODevice *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    qint64 nOrigSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    if (nOrigSize <= 0) {
        return false;
    }

    // Determine dictionary size from PARM property (bits 0-3 of TECH.PARM + 10)
    QVariant vParm = pDecompressState->mapProperties.value(XBinary::FPART_PROP_TYPE);
    qint32 nDicBits = 20;  // default (20 == 1MB, same as dcpr_init default)

    if (vParm.isValid()) {
        qint32 nParm = vParm.toInt();
        qint32 nParmBits = (nParm & 15) + 10;  // low 4 bits of PARM + 10

        if (nParmBits >= 10 && nParmBits <= ACE_MAXDIC) {
            nDicBits = nParmBits;
        }
    }

    AceDecodeState state;
    memset(&state, 0, sizeof(AceDecodeState));

    state.nDicSiz = 1 << nDicBits;
    state.nDicAnd = state.nDicSiz - 1;
    state.pText = new char[(size_t)state.nDicSiz];

    if (!state.pText) {
        return false;
    }

    memset(state.pText, 0, (size_t)state.nDicSiz);

    state.pInput = pDecompressState->pDeviceInput;
    state.nInputBytesRead = 0;
    state.nInputLimit = pDecompressState->nInputLimit;
    state.nDcrSize = nOrigSize;
    state.nDPos = 0;
    state.nOldNum = 0;
    state.nBlockSize = 0;
    state.bError = false;
    memset(state.nOldDist, 0, sizeof(state.nOldDist));

    // Initial fill: read size_rdb DWORDs = size_rdb * 4 bytes
    qint32 nInitBytes = ACE_SIZE_RDB * 4;
    qint64 nRemaining = state.nInputLimit - state.nInputBytesRead;
    qint32 nToRead = (nRemaining < (qint64)nInitBytes) ? (qint32)nRemaining : nInitBytes;
    qint32 nRead = (qint32)state.pInput->read((char *)state.nBufRd, nToRead);

    if (nRead < 0) {
        nRead = 0;
    }

    state.nInputBytesRead += nRead;

    if (nRead < nInitBytes) {
        memset((char *)state.nBufRd + nRead, 0, (size_t)(nInitBytes - nRead));
    }

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    for (qint32 i = 0; i < ACE_SIZE_RDB; i++) {
        state.nBufRd[i] = qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(&state.nBufRd[i]));
    }
#endif

    state.nBitsRd = 0;
    state.nRPos = 0;
    state.nCodeRd = state.nBufRd[0];

    // Decompression loop: drain blocks of at most (DicSiz) bytes at once
    const qint32 nChunkSize = state.nDicSiz;
    char *pChunkBuf = new char[(size_t)nChunkSize];

    if (!pChunkBuf) {
        delete[] state.pText;
        return false;
    }

    pOutput->seek(0);
    qint64 nOutputWritten = 0;
    bool bOk = true;

    while (state.nDcrSize > 0 && !state.bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint32 nDone = decompressBlk(&state, pChunkBuf, nChunkSize);

        if (nDone <= 0) {
            break;
        }

        qint64 nWrite = qMin((qint64)nDone, nOrigSize - nOutputWritten);

        if (nWrite > 0) {
            qint64 nWritten = pOutput->write(pChunkBuf, nWrite);

            if (nWritten != nWrite) {
                bOk = false;
                break;
            }

            nOutputWritten += nWritten;
        }
    }

    pDecompressState->nCountInput = state.nInputBytesRead;
    pDecompressState->nCountOutput = nOutputWritten;

    delete[] pChunkBuf;
    delete[] state.pText;

    if (state.bError) {
        bOk = false;
    }

    return bOk;
}

bool XAceDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    XBinary::PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    return decompressInternal(pDecompressState, pDecompressState->pDeviceOutput, pPdStruct);
}

bool XAceDecoder::decompressDelta(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    XBinary::PDSTRUCT pdStructEmpty = XBinary::createPdStruct();

    if (!pPdStruct) {
        pPdStruct = &pdStructEmpty;
    }

    qint64 nOrigSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    if (nOrigSize <= 0) {
        return false;
    }

    // Decompress into a temporary buffer
    QIODevice *pTempDevice = XBinary::createFileBuffer(nOrigSize, pPdStruct);

    if (!pTempDevice) {
        return false;
    }

    bool bResult = decompressInternal(pDecompressState, pTempDevice, pPdStruct);

    if (bResult) {
        // Apply inverse byte delta filter: output[i] += output[i-1]
        pTempDevice->seek(0);
        QByteArray baData = pTempDevice->readAll();
        quint8 *pData = (quint8 *)baData.data();
        qint64 nSize = baData.size();

        for (qint64 i = 1; i < nSize; i++) {
            pData[i] = (quint8)(pData[i] + pData[i - 1]);
        }

        pDecompressState->pDeviceOutput->seek(0);
        qint64 nWritten = pDecompressState->pDeviceOutput->write(baData);
        bResult = (nWritten == nSize);

        if (bResult) {
            pDecompressState->nCountOutput = nWritten;
        }
    }

    XBinary::freeFileBuffer(&pTempDevice);

    return bResult;
}
