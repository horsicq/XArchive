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
#include "xlzxdecoder.h"

#include <limits>

namespace {

const qint32 LZX_FRAME_SIZE = 32768;
const qint32 LZX_PRETREE_SYMBOLS = 20;
const qint32 LZX_LENGTH_SYMBOLS = 249;
const qint32 LZX_ALIGNED_SYMBOLS = 8;
const qint32 LZX_MAX_MAIN_SYMBOLS = 256 + 50 * 8;
const qint32 LZX_MAX_CODE_LENGTH = 16;
const qint32 LZX_MAX_MATCH_LENGTH = 257;

const qint32 LZX_BLOCK_VERBATIM = 1;
const qint32 LZX_BLOCK_ALIGNED = 2;
const qint32 LZX_BLOCK_UNCOMPRESSED = 3;

const qint32 LZX_WIM_MAGIC_FILESIZE = 12000000;

struct LZX_HUFF {
    quint16 nCount[LZX_MAX_CODE_LENGTH + 1];
    quint16 nSymbol[LZX_MAX_MAIN_SYMBOLS];
    bool bEmpty;
};

struct LZX_STATE {
    const quint8 *pIn;
    qint64 nInSize;
    qint64 nInPos;    // bytes consumed into the bit buffer
    quint64 nBitBuf;  // top nBitCount bits are valid
    qint32 nBitCount;
    qint64 nOverrun;  // 16-bit words injected past end of input
    bool bError;

    // Sliding window
    QByteArray baWindow;
    quint32 nWindowSize;
    quint32 nWindowPos;

    quint32 R0, R1, R2;

    qint32 nMainSymbols;
    quint8 mainLens[LZX_MAX_MAIN_SYMBOLS];
    quint8 lengthLens[LZX_LENGTH_SYMBOLS];

    LZX_HUFF mainTree;
    LZX_HUFF lengthTree;
    LZX_HUFF alignedTree;
    LZX_HUFF preTree;

    quint32 positionBase[51];
    quint8 extraBits[51];
    qint32 nPositionSlots;
};

void lzx_initBitReader(LZX_STATE *pState, const quint8 *pIn, qint64 nInSize)
{
    pState->pIn = pIn;
    pState->nInSize = nInSize;
    pState->nInPos = 0;
    pState->nBitBuf = 0;
    pState->nBitCount = 0;
    pState->nOverrun = 0;
    pState->bError = false;
}

void lzx_ensureBits(LZX_STATE *pState, qint32 nBits)
{
    while (pState->nBitCount < nBits) {
        quint32 nWord = 0;

        if (pState->nInPos + 1 < pState->nInSize) {
            nWord = (quint32)pState->pIn[pState->nInPos] | ((quint32)pState->pIn[pState->nInPos + 1] << 8);
        } else {
            pState->nOverrun++;
            pState->bError = true;
            return;
        }

        pState->nInPos += 2;
        pState->nBitBuf |= ((quint64)nWord) << (48 - pState->nBitCount);
        pState->nBitCount += 16;
    }
}

quint32 lzx_readBits(LZX_STATE *pState, qint32 nBits)
{
    if (nBits == 0) {
        return 0;
    }

    lzx_ensureBits(pState, nBits);
    if (pState->bError || (pState->nBitCount < nBits)) return 0;

    quint32 nResult = (quint32)(pState->nBitBuf >> (64 - nBits));
    pState->nBitBuf <<= nBits;
    pState->nBitCount -= nBits;

    return nResult;
}

// Canonical Huffman table: codes assigned in order of (length, symbol)
enum LZX_HUFF_MODE {
    LZX_HUFF_FULL,
    LZX_HUFF_FULL_OR_EMPTY
};

bool lzx_buildHuff(LZX_HUFF *pTable, const quint8 *pLens, qint32 nSymbols, LZX_HUFF_MODE mode)
{
    for (qint32 i = 0; i <= LZX_MAX_CODE_LENGTH; i++) {
        pTable->nCount[i] = 0;
    }

    for (qint32 i = 0; i < nSymbols; i++) {
        pTable->nCount[pLens[i]]++;
    }

    pTable->bEmpty = (pTable->nCount[0] == nSymbols);

    if (pTable->bEmpty) {
        return mode == LZX_HUFF_FULL_OR_EMPTY;
    }

    // Check the code space is not over-subscribed
    qint32 nLeft = 1;
    for (qint32 nLen = 1; nLen <= LZX_MAX_CODE_LENGTH; nLen++) {
        nLeft <<= 1;
        nLeft -= pTable->nCount[nLen];
        if (nLeft < 0) {
            return false;
        }
    }

    if (nLeft != 0) return false;

    // Sort symbols by (length, symbol index)
    qint32 nOffsets[LZX_MAX_CODE_LENGTH + 1];
    nOffsets[1] = 0;
    for (qint32 nLen = 1; nLen < LZX_MAX_CODE_LENGTH; nLen++) {
        nOffsets[nLen + 1] = nOffsets[nLen] + pTable->nCount[nLen];
    }

    for (qint32 i = 0; i < nSymbols; i++) {
        if (pLens[i]) {
            pTable->nSymbol[nOffsets[pLens[i]]++] = (quint16)i;
        }
    }

    return true;
}

qint32 lzx_decodeHuff(LZX_STATE *pState, const LZX_HUFF *pTable)
{
    if (pTable->bEmpty) {
        pState->bError = true;
        return -1;
    }

    qint32 nCode = 0;
    qint32 nFirst = 0;
    qint32 nIndex = 0;

    for (qint32 nLen = 1; nLen <= LZX_MAX_CODE_LENGTH; nLen++) {
        nCode |= (qint32)lzx_readBits(pState, 1);
        qint32 nCount = pTable->nCount[nLen];
        if (nCode - nFirst < nCount) {
            return pTable->nSymbol[nIndex + (nCode - nFirst)];
        }
        nIndex += nCount;
        nFirst += nCount;
        nFirst <<= 1;
        nCode <<= 1;
    }

    pState->bError = true;
    return -1;
}

// Delta-coded code lengths for [nFirst, nLast) of pLens, via a 20-symbol pretree
bool lzx_readLengths(LZX_STATE *pState, quint8 *pLens, qint32 nFirst, qint32 nLast)
{
    quint8 preLens[LZX_PRETREE_SYMBOLS];

    for (qint32 i = 0; i < LZX_PRETREE_SYMBOLS; i++) {
        preLens[i] = (quint8)lzx_readBits(pState, 4);
    }

    if (!lzx_buildHuff(&pState->preTree, preLens, LZX_PRETREE_SYMBOLS, LZX_HUFF_FULL)) {
        return false;
    }

    qint32 i = nFirst;

    while (i < nLast) {
        qint32 z = lzx_decodeHuff(pState, &pState->preTree);

        if (pState->bError) {
            return false;
        }

        if (z == 17) {
            qint32 n = (qint32)lzx_readBits(pState, 4) + 4;
            if (n > nLast - i) return false;
            while (n--) pLens[i++] = 0;
        } else if (z == 18) {
            qint32 n = (qint32)lzx_readBits(pState, 5) + 20;
            if (n > nLast - i) return false;
            while (n--) pLens[i++] = 0;
        } else if (z == 19) {
            qint32 n = (qint32)lzx_readBits(pState, 1) + 4;
            qint32 z2 = lzx_decodeHuff(pState, &pState->preTree);
            if (pState->bError || (z2 < 0) || (z2 > 16)) {
                return false;
            }
            if (n > nLast - i) return false;
            quint8 nValue = (quint8)((pLens[i] + 17 - z2) % 17);
            while (n--) pLens[i++] = nValue;
        } else if ((z >= 0) && (z <= 16)) {
            pLens[i] = (quint8)((pLens[i] + 17 - z) % 17);
            i++;
        } else {
            return false;
        }
    }

    return true;
}

void lzx_initPositionSlots(LZX_STATE *pState, qint32 nWindowBits)
{
    static const qint32 slotsByBits[7] = {30, 32, 34, 36, 38, 42, 50};  // 15..21 window bits

    pState->nPositionSlots = slotsByBits[qBound(15, nWindowBits, 21) - 15];

    quint32 nBase = 0;
    for (qint32 i = 0; i < pState->nPositionSlots; i++) {
        pState->extraBits[i] = (quint8)((i < 4) ? 0 : qMin(17, (i / 2) - 1));
        pState->positionBase[i] = nBase;
        nBase += ((quint32)1 << pState->extraBits[i]);
    }
}

// Discard bits so the next read starts at a 16-bit stream boundary.
// bAtLeastOne: consume a full padding word even when already aligned
// (uncompressed-block rule); otherwise only when misaligned (frame rule).
bool lzx_align16(LZX_STATE *pState, bool bAtLeastOne)
{
    qint32 nMisaligned = pState->nBitCount & 15;

    if (nMisaligned) {
        if ((pState->nBitBuf >> (64 - nMisaligned)) != 0) return false;
        pState->nBitBuf <<= nMisaligned;
        pState->nBitCount -= nMisaligned;
    } else if (bAtLeastOne) {
        if (lzx_readBits(pState, 16) != 0) return false;
    }
    return !pState->bError;
}

// Current raw byte position in the input (all buffered bits are whole words here)
qint64 lzx_rawPosition(LZX_STATE *pState)
{
    return pState->nInPos - (pState->nBitCount / 8);
}

void lzx_outputByte(LZX_STATE *pState, QByteArray *pbaOut, quint8 nByte)
{
    pState->baWindow[(int)pState->nWindowPos] = (char)nByte;
    pState->nWindowPos = (pState->nWindowPos + 1) & (pState->nWindowSize - 1);
    pbaOut->append((char)nByte);
}

// Reverse the Intel E8 call translation on one frame
void lzx_undoE8(QByteArray &baData, qint64 nFrameOffset, qint64 nFrameSize, qint64 nDataBase, qint32 nFileSize)
{
    if ((nFileSize == 0) || (nFrameSize <= 10) || (nFrameOffset >= 0x40000000)) {
        return;
    }

    unsigned char *pData = reinterpret_cast<unsigned char *>(baData.data()) + nDataBase;

    for (qint64 i = 0; i < nFrameSize - 10;) {
        if (pData[i] != 0xE8) {
            i++;
            continue;
        }

        qint64 nCurPos = nFrameOffset + i;
        qint32 nAbsOff = (qint32)((quint32)pData[i + 1] | ((quint32)pData[i + 2] << 8) | ((quint32)pData[i + 3] << 16) | ((quint32)pData[i + 4] << 24));

        if (((qint64)nAbsOff >= -nCurPos) && (nAbsOff < nFileSize)) {
            quint32 nRelOff = (nAbsOff >= 0) ? (quint32)(nAbsOff - nCurPos) : (quint32)(nAbsOff + nFileSize);
            pData[i + 1] = (unsigned char)nRelOff;
            pData[i + 2] = (unsigned char)(nRelOff >> 8);
            pData[i + 3] = (unsigned char)(nRelOff >> 16);
            pData[i + 4] = (unsigned char)(nRelOff >> 24);
        }

        i += 5;
    }
}

bool lzx_decompressStream(LZX_STATE *pState, QByteArray *pbaOut, qint64 nUncompressedSize, qint32 nWindowBits, bool bWIMVariant, XBinary::PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    pState->nWindowSize = (quint32)1 << nWindowBits;
    pState->baWindow.resize((int)pState->nWindowSize);
    pState->baWindow.fill(0);
    pState->nWindowPos = 0;
    pState->R0 = pState->R1 = pState->R2 = 1;
    lzx_initPositionSlots(pState, nWindowBits);

    pState->nMainSymbols = 256 + pState->nPositionSlots * 8;
    memset(pState->mainLens, 0, sizeof(pState->mainLens));
    memset(pState->lengthLens, 0, sizeof(pState->lengthLens));

    pbaOut->clear();
    pbaOut->reserve((int)nUncompressedSize);

    qint32 nIntelFileSize = 0;

    if (!bWIMVariant) {
        // CAB stream header: 1-bit Intel E8 flag, then optional 32-bit translation size
        if (lzx_readBits(pState, 1)) {
            quint32 nHigh = lzx_readBits(pState, 16);
            quint32 nLow = lzx_readBits(pState, 16);
            nIntelFileSize = (qint32)((nHigh << 16) | nLow);
        }
    }

    qint64 nOutCount = 0;
    qint64 nNextFrame = LZX_FRAME_SIZE;
    qint64 nBlockRemaining = 0;
    qint32 nBlockType = 0;

    while ((nOutCount < nUncompressedSize) && !pState->bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (nBlockRemaining == 0) {
            // Block header
            nBlockType = (qint32)lzx_readBits(pState, 3);

            qint64 nBlockSize = 0;

            if (bWIMVariant) {
                if (lzx_readBits(pState, 1)) {
                    nBlockSize = LZX_FRAME_SIZE;
                } else {
                    nBlockSize = lzx_readBits(pState, 16);
                    // WIM LZX dictionaries larger than 32 KiB use the 24-bit
                    // explicit block-size form used by wimlib and 7-Zip.
                    if (nWindowBits >= 16) {
                        nBlockSize = (nBlockSize << 8) | lzx_readBits(pState, 8);
                    }
                }
            } else {
                nBlockSize = lzx_readBits(pState, 24);
            }

            if ((nBlockSize <= 0) || (nBlockSize > nUncompressedSize - nOutCount)) {
                return false;
            }

            nBlockRemaining = nBlockSize;

            if (nBlockType == LZX_BLOCK_ALIGNED) {
                quint8 alignedLens[LZX_ALIGNED_SYMBOLS];
                for (qint32 i = 0; i < LZX_ALIGNED_SYMBOLS; i++) {
                    alignedLens[i] = (quint8)lzx_readBits(pState, 3);
                }
                if (!lzx_buildHuff(&pState->alignedTree, alignedLens, LZX_ALIGNED_SYMBOLS, LZX_HUFF_FULL)) {
                    return false;
                }
            }

            if ((nBlockType == LZX_BLOCK_VERBATIM) || (nBlockType == LZX_BLOCK_ALIGNED)) {
                if (!lzx_readLengths(pState, pState->mainLens, 0, 256)) {
                    return false;
                }
                if (!lzx_readLengths(pState, pState->mainLens, 256, pState->nMainSymbols)) {
                    return false;
                }
                if (!lzx_buildHuff(&pState->mainTree, pState->mainLens, pState->nMainSymbols, LZX_HUFF_FULL)) {
                    return false;
                }
                if (!lzx_readLengths(pState, pState->lengthLens, 0, LZX_LENGTH_SYMBOLS)) {
                    return false;
                }
                if (!lzx_buildHuff(&pState->lengthTree, pState->lengthLens, LZX_LENGTH_SYMBOLS, LZX_HUFF_FULL_OR_EMPTY)) {
                    return false;
                }
            } else if (nBlockType == LZX_BLOCK_UNCOMPRESSED) {
                // Align to a 16-bit boundary (1-16 pad bits), then 12 bytes of R0/R1/R2
                if (!lzx_align16(pState, true)) return false;

                qint64 nRawPos = lzx_rawPosition(pState);
                pState->nBitBuf = 0;
                pState->nBitCount = 0;

                if (nRawPos + 12 > pState->nInSize) {
                    return false;
                }

                const quint8 *p = pState->pIn + nRawPos;
                pState->R0 = (quint32)p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24);
                pState->R1 = (quint32)p[4] | ((quint32)p[5] << 8) | ((quint32)p[6] << 16) | ((quint32)p[7] << 24);
                pState->R2 = (quint32)p[8] | ((quint32)p[9] << 8) | ((quint32)p[10] << 16) | ((quint32)p[11] << 24);
                nRawPos += 12;

                const quint32 nMaxRepeatOffset = pState->nWindowSize - 3;
                if ((pState->R0 == 0) || (pState->R0 > nMaxRepeatOffset) || (pState->R1 == 0) || (pState->R1 > nMaxRepeatOffset) || (pState->R2 == 0) ||
                    (pState->R2 > nMaxRepeatOffset)) {
                    return false;
                }

                // Copy raw bytes
                if (nRawPos + nBlockRemaining > pState->nInSize) {
                    return false;
                }

                for (qint64 i = 0; i < nBlockRemaining; i++) {
                    if (((i & 0x3FFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                    lzx_outputByte(pState, pbaOut, pState->pIn[nRawPos + i]);
                }

                nOutCount += nBlockRemaining;
                nRawPos += nBlockRemaining;

                if (nBlockRemaining & 1) {
                    // A zero pad byte is required before another block.  The
                    // final odd uncompressed block may end exactly at EOF.
                    if (nRawPos < pState->nInSize) {
                        if (pState->pIn[nRawPos] != 0) return false;
                        nRawPos++;
                    } else if (nOutCount != nUncompressedSize) {
                        return false;
                    }
                }

                nBlockRemaining = 0;
                pState->nInPos = nRawPos;

                while (nOutCount >= nNextFrame) {
                    nNextFrame += LZX_FRAME_SIZE;
                }

                continue;
            } else {
                return false;
            }
        }

        // Decode one symbol from a verbatim/aligned block
        qint32 nMainSym = lzx_decodeHuff(pState, &pState->mainTree);

        if (pState->bError || (nMainSym < 0)) {
            return false;
        }

        if (nMainSym < 256) {
            if ((nBlockRemaining <= 0) || (nOutCount >= nUncompressedSize)) return false;
            lzx_outputByte(pState, pbaOut, (quint8)nMainSym);
            nOutCount++;
            nBlockRemaining--;
        } else {
            nMainSym -= 256;
            qint32 nLenHeader = nMainSym & 7;
            qint32 nPosSlot = nMainSym >> 3;

            qint32 nMatchLen = nLenHeader + 2;
            if (nLenHeader == 7) {
                qint32 nLenSym = lzx_decodeHuff(pState, &pState->lengthTree);
                if (pState->bError || (nLenSym < 0)) {
                    return false;
                }
                nMatchLen = nLenSym + 9;
            }

            quint32 nMatchOffset = 0;

            if (nPosSlot == 0) {
                nMatchOffset = pState->R0;
            } else if (nPosSlot == 1) {
                nMatchOffset = pState->R1;
                pState->R1 = pState->R0;
                pState->R0 = nMatchOffset;
            } else if (nPosSlot == 2) {
                nMatchOffset = pState->R2;
                pState->R2 = pState->R0;
                pState->R0 = nMatchOffset;
            } else {
                const qint32 nPositionTableSize = (qint32)(sizeof(pState->extraBits) / sizeof(pState->extraBits[0]));
                if ((nPosSlot < 0) || (nPosSlot >= pState->nPositionSlots) || (nPosSlot >= nPositionTableSize)) {
                    return false;
                }

                qint32 nExtra = pState->extraBits[nPosSlot];
                quint32 nVerbatim = 0;

                if ((nBlockType == LZX_BLOCK_ALIGNED) && (nExtra >= 3)) {
                    nVerbatim = lzx_readBits(pState, nExtra - 3);
                    qint32 nAlignedSym = lzx_decodeHuff(pState, &pState->alignedTree);
                    if (pState->bError || (nAlignedSym < 0)) {
                        return false;
                    }
                    nMatchOffset = pState->positionBase[nPosSlot] - 2 + (nVerbatim << 3) + (quint32)nAlignedSym;
                } else {
                    if (nExtra) {
                        nVerbatim = lzx_readBits(pState, nExtra);
                    }
                    nMatchOffset = pState->positionBase[nPosSlot] - 2 + nVerbatim;
                }

                pState->R2 = pState->R1;
                pState->R1 = pState->R0;
                pState->R0 = nMatchOffset;
            }

            if ((nMatchOffset == 0) || (nMatchOffset > pState->nWindowSize) || ((quint64)nMatchOffset > qMin<quint64>((quint64)nOutCount, pState->nWindowSize)) ||
                ((qint64)nMatchLen > nBlockRemaining) || ((qint64)nMatchLen > nUncompressedSize - nOutCount) ||
                (!bWIMVariant && ((qint64)nMatchLen > nNextFrame - nOutCount))) {
                return false;
            }

            quint32 nSrc = (pState->nWindowPos + pState->nWindowSize - nMatchOffset) & (pState->nWindowSize - 1);

            for (qint32 i = 0; i < nMatchLen; i++) {
                lzx_outputByte(pState, pbaOut, (quint8)pState->baWindow.at((int)nSrc));
                nSrc = (nSrc + 1) & (pState->nWindowSize - 1);
            }

            nOutCount += nMatchLen;
            nBlockRemaining -= nMatchLen;
        }

        if (nBlockRemaining < 0) {
            return false;
        }

        // Bitstream realigns to 16 bits at every 32KB output frame boundary (CAB)
        while (nOutCount >= nNextFrame) {
            if (!bWIMVariant) {
                if (!lzx_align16(pState, false)) return false;
            }
            nNextFrame += LZX_FRAME_SIZE;
        }
    }

    if (pState->bError || pState->nOverrun || (nOutCount != nUncompressedSize) || (nBlockRemaining != 0) || (pState->nInPos != pState->nInSize) ||
        (pState->nBitBuf != 0) || (pbaOut->size() != nUncompressedSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    // Intel E8 post-processing
    if (bWIMVariant) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        lzx_undoE8(*pbaOut, 0, nUncompressedSize, 0, LZX_WIM_MAGIC_FILESIZE);
    } else if (nIntelFileSize != 0) {
        for (qint64 nFrame = 0; nFrame < nUncompressedSize; nFrame += LZX_FRAME_SIZE) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
            qint64 nFrameSize = qMin((qint64)LZX_FRAME_SIZE, nUncompressedSize - nFrame);
            lzx_undoE8(*pbaOut, nFrame, nFrameSize, nFrame, nIntelFileSize);
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

// CAB frames every CFDATA payload independently, but the LZX dictionary,
// repeated offsets, Huffman lengths and the current block all survive across
// entries.  In particular, an uncompressed LZX block may continue as raw bytes
// in the next CFDATA, and its odd-byte padding can be the first byte of that
// next entry.  Keep this path separate from lzx_decompressStream() so the
// legacy concatenated-CAB and WIM entry points retain their established
// behaviour.
bool lzx_decompressCABFramedBlocks(const QList<QByteArray> &listCompressedBlocks, const QList<qint32> &listUncompressedSizes, QByteArray *pbaOut,
                                   qint64 nUncompressedSize, qint32 nWindowBits, XBinary::PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    LZX_STATE state = {};
    state.nWindowSize = (quint32)1 << nWindowBits;
    state.baWindow.resize((int)state.nWindowSize);
    state.baWindow.fill(0);
    state.nWindowPos = 0;
    state.R0 = state.R1 = state.R2 = 1;
    lzx_initPositionSlots(&state, nWindowBits);

    state.nMainSymbols = 256 + state.nPositionSlots * 8;
    memset(state.mainLens, 0, sizeof(state.mainLens));
    memset(state.lengthLens, 0, sizeof(state.lengthLens));

    pbaOut->clear();
    pbaOut->reserve((int)nUncompressedSize);

    qint32 nIntelFileSize = 0;
    qint64 nOutCount = 0;
    qint64 nNextFrame = LZX_FRAME_SIZE;
    qint64 nBlockRemaining = 0;
    qint32 nBlockType = 0;
    bool bUncompressedBlockOdd = false;
    bool bUncompressedPadPending = false;

    for (qint32 nEntry = 0; nEntry < listCompressedBlocks.size(); nEntry++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const QByteArray &baCompressed = listCompressedBlocks.at(nEntry);
        const qint64 nEntryUncompressed = listUncompressedSizes.at(nEntry);
        const qint64 nEntryOutStart = nOutCount;
        qint64 nEntryOutCount = 0;

        // Discard only the prior entry's bit-reader framing.  All dictionary,
        // tree, repeat-offset and current-block fields above remain intact.
        lzx_initBitReader(&state, reinterpret_cast<const quint8 *>(baCompressed.constData()), baCompressed.size());

        if (nEntry == 0) {
            // CAB stream header: 1-bit Intel E8 flag, then optional 32-bit size.
            if (lzx_readBits(&state, 1)) {
                quint32 nHigh = lzx_readBits(&state, 16);
                quint32 nLow = lzx_readBits(&state, 16);
                nIntelFileSize = (qint32)((nHigh << 16) | nLow);
            }
            if (state.bError) {
                return false;
            }
        }

        // 7-Zip accepts the zero pad for a final odd uncompressed block either
        // in the entry that ended the block or at the start of the next entry.
        if (bUncompressedPadPending) {
            if ((state.nInSize <= 0) || (state.pIn[0] != 0)) {
                return false;
            }
            state.nInPos = 1;
            bUncompressedPadPending = false;
        }

        while ((nEntryOutCount < nEntryUncompressed) && !state.bError && XBinary::isPdStructNotCanceled(pPdStruct)) {
            if (nBlockRemaining == 0) {
                nBlockType = (qint32)lzx_readBits(&state, 3);
                qint64 nBlockSize = lzx_readBits(&state, 24);

                if (state.bError || (nBlockSize <= 0) || (nBlockSize > nUncompressedSize - nOutCount)) {
                    return false;
                }

                nBlockRemaining = nBlockSize;
                bUncompressedBlockOdd = false;

                if (nBlockType == LZX_BLOCK_ALIGNED) {
                    quint8 alignedLens[LZX_ALIGNED_SYMBOLS];
                    for (qint32 i = 0; i < LZX_ALIGNED_SYMBOLS; i++) {
                        alignedLens[i] = (quint8)lzx_readBits(&state, 3);
                    }
                    if (!lzx_buildHuff(&state.alignedTree, alignedLens, LZX_ALIGNED_SYMBOLS, LZX_HUFF_FULL)) {
                        return false;
                    }
                }

                if ((nBlockType == LZX_BLOCK_VERBATIM) || (nBlockType == LZX_BLOCK_ALIGNED)) {
                    if (!lzx_readLengths(&state, state.mainLens, 0, 256) || !lzx_readLengths(&state, state.mainLens, 256, state.nMainSymbols) ||
                        !lzx_buildHuff(&state.mainTree, state.mainLens, state.nMainSymbols, LZX_HUFF_FULL) ||
                        !lzx_readLengths(&state, state.lengthLens, 0, LZX_LENGTH_SYMBOLS) ||
                        !lzx_buildHuff(&state.lengthTree, state.lengthLens, LZX_LENGTH_SYMBOLS, LZX_HUFF_FULL_OR_EMPTY)) {
                        return false;
                    }
                } else if (nBlockType == LZX_BLOCK_UNCOMPRESSED) {
                    // Align by 1..16 zero bits, then read the three raw LE32
                    // repeat offsets.  Byte mode may continue into later entries.
                    if (!lzx_align16(&state, true)) {
                        return false;
                    }

                    qint64 nRawPos = lzx_rawPosition(&state);
                    state.nBitBuf = 0;
                    state.nBitCount = 0;

                    if ((nRawPos < 0) || (nRawPos + 12 > state.nInSize)) {
                        return false;
                    }

                    const quint8 *p = state.pIn + nRawPos;
                    state.R0 = (quint32)p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24);
                    state.R1 = (quint32)p[4] | ((quint32)p[5] << 8) | ((quint32)p[6] << 16) | ((quint32)p[7] << 24);
                    state.R2 = (quint32)p[8] | ((quint32)p[9] << 8) | ((quint32)p[10] << 16) | ((quint32)p[11] << 24);
                    nRawPos += 12;

                    const quint32 nMaxRepeatOffset = state.nWindowSize - 3;
                    if ((state.R0 == 0) || (state.R0 > nMaxRepeatOffset) || (state.R1 == 0) || (state.R1 > nMaxRepeatOffset) || (state.R2 == 0) ||
                        (state.R2 > nMaxRepeatOffset)) {
                        return false;
                    }

                    state.nInPos = nRawPos;
                    bUncompressedBlockOdd = (nBlockSize & 1) != 0;
                } else {
                    return false;
                }
            }

            if (nBlockType == LZX_BLOCK_UNCOMPRESSED) {
                qint64 nRawPos = lzx_rawPosition(&state);
                const qint64 nCopySize = qMin(nBlockRemaining, nEntryUncompressed - nEntryOutCount);

                if ((nCopySize <= 0) || (nRawPos < 0) || (nCopySize > state.nInSize - nRawPos)) {
                    return false;
                }

                for (qint64 i = 0; i < nCopySize; i++) {
                    if (((i & 0x3FFF) == 0) && !XBinary::isPdStructNotCanceled(pPdStruct)) {
                        return false;
                    }
                    lzx_outputByte(&state, pbaOut, state.pIn[nRawPos + i]);
                }

                nRawPos += nCopySize;
                nBlockRemaining -= nCopySize;
                nOutCount += nCopySize;
                nEntryOutCount += nCopySize;
                state.nInPos = nRawPos;

                if (nBlockRemaining == 0) {
                    if (bUncompressedBlockOdd) {
                        if (nRawPos < state.nInSize) {
                            if (state.pIn[nRawPos] != 0) {
                                return false;
                            }
                            nRawPos++;
                            state.nInPos = nRawPos;
                        } else {
                            bUncompressedPadPending = true;
                        }
                    }
                    bUncompressedBlockOdd = false;
                }

                while (nOutCount >= nNextFrame) {
                    nNextFrame += LZX_FRAME_SIZE;
                }
                continue;
            }

            qint32 nMainSym = lzx_decodeHuff(&state, &state.mainTree);
            if (state.bError || (nMainSym < 0)) {
                return false;
            }

            if (nMainSym < 256) {
                if ((nBlockRemaining <= 0) || (nOutCount >= nUncompressedSize) || (nEntryOutCount >= nEntryUncompressed)) {
                    return false;
                }
                lzx_outputByte(&state, pbaOut, (quint8)nMainSym);
                nOutCount++;
                nEntryOutCount++;
                nBlockRemaining--;
            } else {
                nMainSym -= 256;
                qint32 nLenHeader = nMainSym & 7;
                qint32 nPosSlot = nMainSym >> 3;

                qint32 nMatchLen = nLenHeader + 2;
                if (nLenHeader == 7) {
                    qint32 nLenSym = lzx_decodeHuff(&state, &state.lengthTree);
                    if (state.bError || (nLenSym < 0)) {
                        return false;
                    }
                    nMatchLen = nLenSym + 9;
                }

                quint32 nMatchOffset = 0;
                if (nPosSlot == 0) {
                    nMatchOffset = state.R0;
                } else if (nPosSlot == 1) {
                    nMatchOffset = state.R1;
                    state.R1 = state.R0;
                    state.R0 = nMatchOffset;
                } else if (nPosSlot == 2) {
                    nMatchOffset = state.R2;
                    state.R2 = state.R0;
                    state.R0 = nMatchOffset;
                } else {
                    const qint32 nPositionTableSize = (qint32)(sizeof(state.extraBits) / sizeof(state.extraBits[0]));
                    if ((nPosSlot < 0) || (nPosSlot >= state.nPositionSlots) || (nPosSlot >= nPositionTableSize)) {
                        return false;
                    }

                    qint32 nExtra = state.extraBits[nPosSlot];
                    quint32 nVerbatim = 0;
                    if ((nBlockType == LZX_BLOCK_ALIGNED) && (nExtra >= 3)) {
                        nVerbatim = lzx_readBits(&state, nExtra - 3);
                        qint32 nAlignedSym = lzx_decodeHuff(&state, &state.alignedTree);
                        if (state.bError || (nAlignedSym < 0)) {
                            return false;
                        }
                        nMatchOffset = state.positionBase[nPosSlot] - 2 + (nVerbatim << 3) + (quint32)nAlignedSym;
                    } else {
                        if (nExtra) {
                            nVerbatim = lzx_readBits(&state, nExtra);
                        }
                        nMatchOffset = state.positionBase[nPosSlot] - 2 + nVerbatim;
                    }

                    state.R2 = state.R1;
                    state.R1 = state.R0;
                    state.R0 = nMatchOffset;
                }

                if ((nMatchOffset == 0) || (nMatchOffset > state.nWindowSize) || ((quint64)nMatchOffset > qMin<quint64>((quint64)nOutCount, state.nWindowSize)) ||
                    ((qint64)nMatchLen > nBlockRemaining) || ((qint64)nMatchLen > nUncompressedSize - nOutCount) ||
                    ((qint64)nMatchLen > nEntryUncompressed - nEntryOutCount) || ((qint64)nMatchLen > nNextFrame - nOutCount)) {
                    return false;
                }

                quint32 nSrc = (state.nWindowPos + state.nWindowSize - nMatchOffset) & (state.nWindowSize - 1);
                for (qint32 i = 0; i < nMatchLen; i++) {
                    lzx_outputByte(&state, pbaOut, (quint8)state.baWindow.at((int)nSrc));
                    nSrc = (nSrc + 1) & (state.nWindowSize - 1);
                }

                nOutCount += nMatchLen;
                nEntryOutCount += nMatchLen;
                nBlockRemaining -= nMatchLen;
            }

            if (nBlockRemaining < 0) {
                return false;
            }

            while (nOutCount >= nNextFrame) {
                if (!lzx_align16(&state, false)) {
                    return false;
                }
                nNextFrame += LZX_FRAME_SIZE;
            }
        }

        if (state.bError || state.nOverrun || (nEntryOutCount != nEntryUncompressed) || (nOutCount - nEntryOutStart != nEntryUncompressed)) {
            return false;
        }

        // A compressed entry may finish before a folder-wide 32 KiB frame only
        // for the final short CFDATA.  Its remaining 0..15 padding bits must be
        // zero.  Raw uncompressed continuations are already byte-aligned.
        if (nBlockType != LZX_BLOCK_UNCOMPRESSED) {
            if (!lzx_align16(&state, false)) {
                return false;
            }
        }

        if (state.bError || state.nOverrun || (state.nBitBuf != 0) || (state.nBitCount != 0) || (lzx_rawPosition(&state) != state.nInSize)) {
            return false;
        }
    }

    if ((nOutCount != nUncompressedSize) || (nBlockRemaining != 0) || (pbaOut->size() != nUncompressedSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if (nIntelFileSize != 0) {
        for (qint64 nFrame = 0; nFrame < nUncompressedSize; nFrame += LZX_FRAME_SIZE) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
            qint64 nFrameSize = qMin((qint64)LZX_FRAME_SIZE, nUncompressedSize - nFrame);
            lzx_undoE8(*pbaOut, nFrame, nFrameSize, nFrame, nIntelFileSize);
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

}  // namespace

bool XLZXDecoder::decompressCABFolder(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint64 nUncompressedSize, qint32 nWindowBits,
                                      XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUncompressed || (pbaUncompressed == &baCompressed) || !XBinary::isPdStructNotCanceled(pPdStruct) || baCompressed.isEmpty() || (nUncompressedSize <= 0) ||
        (nUncompressedSize > (std::numeric_limits<qint32>::max)() - LZX_MAX_MATCH_LENGTH) || (nWindowBits < 15) || (nWindowBits > 21)) {
        return false;
    }

    LZX_STATE state = {};
    lzx_initBitReader(&state, reinterpret_cast<const quint8 *>(baCompressed.constData()), baCompressed.size());

    return lzx_decompressStream(&state, pbaUncompressed, nUncompressedSize, nWindowBits, false, pPdStruct);
}

bool XLZXDecoder::decompressCABDataBlocks(const QList<QByteArray> &listCompressedBlocks, const QList<qint32> &listUncompressedSizes, QByteArray *pbaUncompressed,
                                          qint32 nWindowBits, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUncompressed || !XBinary::isPdStructNotCanceled(pPdStruct) || listCompressedBlocks.isEmpty() ||
        (listCompressedBlocks.size() != listUncompressedSizes.size()) || (nWindowBits < 15) || (nWindowBits > 21)) {
        return false;
    }

    qint64 nUncompressedSize = 0;
    for (qint32 i = 0; i < listCompressedBlocks.size(); i++) {
        const QByteArray &baCompressed = listCompressedBlocks.at(i);
        const qint32 nBlockSize = listUncompressedSizes.at(i);

        if ((pbaUncompressed == &baCompressed) || baCompressed.isEmpty() || (nBlockSize <= 0) || (nBlockSize > LZX_FRAME_SIZE) ||
            ((i + 1 < listCompressedBlocks.size()) && (nBlockSize != LZX_FRAME_SIZE)) ||
            (nUncompressedSize > (std::numeric_limits<qint32>::max)() - LZX_MAX_MATCH_LENGTH - nBlockSize)) {
            return false;
        }
        nUncompressedSize += nBlockSize;
    }

    return lzx_decompressCABFramedBlocks(listCompressedBlocks, listUncompressedSizes, pbaUncompressed, nUncompressedSize, nWindowBits, pPdStruct);
}

bool XLZXDecoder::decompressWIMChunk(const QByteArray &baCompressed, QByteArray *pbaUncompressed, qint32 nUncompressedSize, qint32 nWindowBits,
                                     XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUncompressed || (pbaUncompressed == &baCompressed) || !XBinary::isPdStructNotCanceled(pPdStruct) || baCompressed.isEmpty() || (nUncompressedSize <= 0) ||
        (nWindowBits < 15) || (nWindowBits > 21) || (nUncompressedSize > ((qint32)1 << nWindowBits))) {
        return false;
    }

    LZX_STATE state = {};
    lzx_initBitReader(&state, reinterpret_cast<const quint8 *>(baCompressed.constData()), baCompressed.size());

    return lzx_decompressStream(&state, pbaUncompressed, nUncompressedSize, nWindowBits, true, pPdStruct);
}
