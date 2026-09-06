/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xkolibrikpackdecoder.h"

#include "algo_utils.h"

#include <limits>
#include <vector>

namespace {

const quint32 KPACK_NUM_BIT_MODEL_TOTAL_BITS = 11;
const quint32 KPACK_NUM_MOVE_BITS = 5;
const quint16 KPACK_PROB_INIT = (quint16)((1u << KPACK_NUM_BIT_MODEL_TOTAL_BITS) / 2);
const quint32 KPACK_TOP_VALUE = 1u << 24;

// kpack hard-wires these; there is no properties byte in the container.
const qint32 KPACK_LC = 3;
const qint32 KPACK_PB_MASK = 3;  // pb == 2

const quint32 KPACK_FLAG_LZMA = 0x01;
const quint32 KPACK_FLAG_CALLTRICK1 = 0x40;
const quint32 KPACK_FLAG_CALLTRICK2 = 0x80;
const qint64 KPACK_CALLTRICK_TRAILER_SIZE = 5;

class KPackRangeDecoder {
public:
    KPackRangeDecoder(const quint8 *pData, qint64 nSize, qint64 nPos)
        : m_pData(pData), m_nSize(nSize), m_nPos(nPos), m_nRange(0xFFFFFFFF), m_nCode(0), m_bOverread(false)
    {
        // kpack's initialiser consumes exactly four bytes, little-endian.
        quint32 nB0 = nextByte();
        quint32 nB1 = nextByte();
        quint32 nB2 = nextByte();
        quint32 nB3 = nextByte();
        m_nCode = nB0 | (nB1 << 8) | (nB2 << 16) | (nB3 << 24);
    }

    bool isOverread() const
    {
        return m_bOverread;
    }

    quint32 decodeBit(quint16 *pProb)
    {
        const quint32 nBound = (m_nRange >> KPACK_NUM_BIT_MODEL_TOTAL_BITS) * (quint32)(*pProb);
        quint32 nResult = 0;

        if (m_nCode < nBound) {
            m_nRange = nBound;
            *pProb = (quint16)((quint32)(*pProb) + (((1u << KPACK_NUM_BIT_MODEL_TOTAL_BITS) - (quint32)(*pProb)) >> KPACK_NUM_MOVE_BITS));
        } else {
            m_nRange -= nBound;
            m_nCode -= nBound;
            *pProb = (quint16)((quint32)(*pProb) - ((quint32)(*pProb) >> KPACK_NUM_MOVE_BITS));
            nResult = 1;
        }

        normalize();

        return nResult;
    }

    quint32 decodeDirectBits(qint32 nCount)
    {
        quint32 nResult = 0;

        for (qint32 i = 0; i < nCount; i++) {
            m_nRange >>= 1;
            m_nCode -= m_nRange;
            const quint32 nT = 0 - (m_nCode >> 31);
            m_nCode += m_nRange & nT;
            nResult = (nResult << 1) + nT + 1;
            normalize();
        }

        return nResult;
    }

    quint32 bitTreeDecode(quint16 *pProbs, qint32 nBits)
    {
        quint32 nModel = 1;

        for (qint32 i = 0; i < nBits; i++) {
            nModel = (nModel << 1) | decodeBit(pProbs + nModel);
        }

        return nModel - (1u << nBits);
    }

    quint32 bitTreeReverseDecode(quint16 *pProbs, qint32 nBits)
    {
        quint32 nModel = 1;
        quint32 nSymbol = 0;

        for (qint32 i = 0; i < nBits; i++) {
            const quint32 nBit = decodeBit(pProbs + nModel);
            nModel = (nModel << 1) | nBit;
            nSymbol |= nBit << i;
        }

        return nSymbol;
    }

private:
    quint32 nextByte()
    {
        if ((m_nPos >= 0) && (m_nPos < m_nSize)) {
            const quint32 nResult = m_pData[m_nPos];
            m_nPos++;
            return nResult;
        }

        // A well formed LZMA stream may be normalised a few bytes past its own
        // end; keep feeding zeroes but remember that it happened.
        m_nPos++;
        m_bOverread = true;

        return 0;
    }

    void normalize()
    {
        while (m_nRange < KPACK_TOP_VALUE) {
            m_nRange <<= 8;
            m_nCode = (m_nCode << 8) | nextByte();
        }
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPos;
    quint32 m_nRange;
    quint32 m_nCode;
    bool m_bOverread;
};

struct KPackLzmaProbs {
    KPackLzmaProbs()
        : isMatch(192, KPACK_PROB_INIT),
          isRep(12, KPACK_PROB_INIT),
          isRepG0(12, KPACK_PROB_INIT),
          isRepG1(12, KPACK_PROB_INIT),
          isRepG2(12, KPACK_PROB_INIT),
          isRep0Long(192, KPACK_PROB_INIT),
          posSlot(256, KPACK_PROB_INIT),
          specPos(128, KPACK_PROB_INIT),
          align(16, KPACK_PROB_INIT),
          lenChoice(2, KPACK_PROB_INIT),
          lenLow(128, KPACK_PROB_INIT),
          lenMid(128, KPACK_PROB_INIT),
          lenHigh(256, KPACK_PROB_INIT),
          repLenChoice(2, KPACK_PROB_INIT),
          repLenLow(128, KPACK_PROB_INIT),
          repLenMid(128, KPACK_PROB_INIT),
          repLenHigh(256, KPACK_PROB_INIT),
          literal((size_t)(0x300 << KPACK_LC), KPACK_PROB_INIT)
    {
    }

    std::vector<quint16> isMatch;
    std::vector<quint16> isRep;
    std::vector<quint16> isRepG0;
    std::vector<quint16> isRepG1;
    std::vector<quint16> isRepG2;
    std::vector<quint16> isRep0Long;
    std::vector<quint16> posSlot;
    std::vector<quint16> specPos;
    std::vector<quint16> align;
    std::vector<quint16> lenChoice;
    std::vector<quint16> lenLow;
    std::vector<quint16> lenMid;
    std::vector<quint16> lenHigh;
    std::vector<quint16> repLenChoice;
    std::vector<quint16> repLenLow;
    std::vector<quint16> repLenMid;
    std::vector<quint16> repLenHigh;
    std::vector<quint16> literal;
};

quint32 kpackDecodeLength(KPackRangeDecoder *pRc, std::vector<quint16> *pChoice, std::vector<quint16> *pLow, std::vector<quint16> *pMid,
                          std::vector<quint16> *pHigh, quint32 nPosState)
{
    if (pRc->decodeBit(&(*pChoice)[0]) == 0) {
        return pRc->bitTreeDecode(&(*pLow)[nPosState << 3], 3) + 2;
    }

    if (pRc->decodeBit(&(*pChoice)[1]) == 0) {
        return pRc->bitTreeDecode(&(*pMid)[nPosState << 3], 3) + 10;
    }

    return pRc->bitTreeDecode(&(*pHigh)[0], 8) + 18;
}

// Raw LZMA1 body with kpack's fixed lc=3 lp=0 pb=2 model.
bool kpackDecodeLzma(const quint8 *pInput, qint64 nInputSize, qint64 nStreamOffset, quint8 *pOutput, qint64 nOutputSize)
{
    if ((!pInput) || (!pOutput) || (nOutputSize <= 0) || (nStreamOffset < 0) || (nStreamOffset + 4 > nInputSize)) {
        return false;
    }

    KPackRangeDecoder rc(pInput, nInputSize, nStreamOffset);
    KPackLzmaProbs probs;

    qint64 nOutPos = 0;
    quint32 nState = 0;
    quint32 nRep0 = 0;
    quint32 nRep1 = 0;
    quint32 nRep2 = 0;
    quint32 nRep3 = 0;

    while (nOutPos < nOutputSize) {
        const quint32 nPosState = (quint32)(nOutPos & KPACK_PB_MASK);

        if (rc.decodeBit(&probs.isMatch[(nState << 4) + nPosState]) == 0) {
            const quint32 nPrevByte = (nOutPos > 0) ? pOutput[nOutPos - 1] : 0;
            quint16 *pLit = &probs.literal[(size_t)((nPrevByte >> (8 - KPACK_LC)) * 0x300)];
            quint32 nSymbol = 1;

            if (nState < 7) {
                while (nSymbol < 0x100) {
                    nSymbol = (nSymbol << 1) | rc.decodeBit(pLit + nSymbol);
                }
            } else {
                if ((qint64)nRep0 + 1 > nOutPos) return false;
                quint32 nMatchByte = pOutput[nOutPos - (qint64)nRep0 - 1];

                while (nSymbol < 0x100) {
                    const quint32 nMatchBit = (nMatchByte >> 7) & 1;
                    nMatchByte = (nMatchByte << 1) & 0xFF;
                    const quint32 nBit = rc.decodeBit(pLit + (((1 + nMatchBit) << 8) + nSymbol));
                    nSymbol = (nSymbol << 1) | nBit;

                    if (nMatchBit != nBit) {
                        while (nSymbol < 0x100) {
                            nSymbol = (nSymbol << 1) | rc.decodeBit(pLit + nSymbol);
                        }
                        break;
                    }
                }
            }

            pOutput[nOutPos++] = (quint8)(nSymbol & 0xFF);
            nState = (nState < 4) ? 0 : ((nState < 10) ? (nState - 3) : (nState - 6));
            continue;
        }

        quint32 nLength = 0;

        if (rc.decodeBit(&probs.isRep[nState]) != 0) {
            if (nOutPos == 0) return false;

            if (rc.decodeBit(&probs.isRepG0[nState]) == 0) {
                if (rc.decodeBit(&probs.isRep0Long[(nState << 4) + nPosState]) == 0) {
                    nState = (nState < 7) ? 9 : 11;
                    if ((qint64)nRep0 + 1 > nOutPos) return false;
                    pOutput[nOutPos] = pOutput[nOutPos - (qint64)nRep0 - 1];
                    nOutPos++;
                    continue;
                }
            } else {
                quint32 nDist = 0;

                if (rc.decodeBit(&probs.isRepG1[nState]) == 0) {
                    nDist = nRep1;
                } else {
                    if (rc.decodeBit(&probs.isRepG2[nState]) == 0) {
                        nDist = nRep2;
                    } else {
                        nDist = nRep3;
                        nRep3 = nRep2;
                    }
                    nRep2 = nRep1;
                }

                nRep1 = nRep0;
                nRep0 = nDist;
            }

            nLength = kpackDecodeLength(&rc, &probs.repLenChoice, &probs.repLenLow, &probs.repLenMid, &probs.repLenHigh, nPosState);
            nState = (nState < 7) ? 8 : 11;
        } else {
            nRep3 = nRep2;
            nRep2 = nRep1;
            nRep1 = nRep0;

            nLength = kpackDecodeLength(&rc, &probs.lenChoice, &probs.lenLow, &probs.lenMid, &probs.lenHigh, nPosState);
            nState = (nState < 7) ? 7 : 10;

            const quint32 nLenToPosState = (nLength - 2 < 3) ? (nLength - 2) : 3;
            const quint32 nPosSlot = rc.bitTreeDecode(&probs.posSlot[nLenToPosState << 6], 6);

            if (nPosSlot < 4) {
                nRep0 = nPosSlot;
            } else {
                const qint32 nNumDirect = (qint32)((nPosSlot >> 1) - 1);
                nRep0 = (2 | (nPosSlot & 1)) << nNumDirect;

                if (nPosSlot < 14) {
                    const qint64 nBase = (qint64)nRep0 - (qint64)nPosSlot;
                    if ((nBase < 0) || (nBase + ((qint64)1 << nNumDirect) > (qint64)probs.specPos.size())) return false;
                    nRep0 += rc.bitTreeReverseDecode(&probs.specPos[(size_t)nBase], nNumDirect);
                } else {
                    nRep0 += rc.decodeDirectBits(nNumDirect - 4) << 4;
                    nRep0 += rc.bitTreeReverseDecode(&probs.align[0], 4);
                }
            }

            if (nRep0 == 0xFFFFFFFF) {
                // End marker.
                break;
            }
        }

        if ((qint64)nRep0 + 1 > nOutPos) return false;

        for (quint32 i = 0; i < nLength; i++) {
            if (nOutPos >= nOutputSize) break;
            pOutput[nOutPos] = pOutput[nOutPos - (qint64)nRep0 - 1];
            nOutPos++;
        }
    }

    return (nOutPos == nOutputSize);
}

// Undo kpack's x86 "call trick": absolute 32-bit targets stored big-endian
// behind E8/E9 (and, for calltrick2, 0F 80..8F) are converted back to
// relative displacements.  The number of conversions and the marker byte come
// from the container's 5-byte trailer.
bool kpackUndoCallTrick(quint8 *pData, qint64 nSize, quint32 nCount, quint8 nMarker, bool bCallTrick2)
{
    qint64 nPos = 0;

    while (nCount) {
        if (nPos >= nSize) return false;

        quint32 nByte = pData[nPos++];
        bool bAccepted = false;

        if (bCallTrick2 && (nByte == 0x0F)) {
            for (;;) {
                if (nPos >= nSize) return false;
                const quint32 nNext = pData[nPos++];

                if (nNext < 0x80) {
                    if (nNext == 0x0F) continue;
                    nByte = nNext;
                    break;
                }

                if (nNext < 0x90) bAccepted = true;
                nByte = nNext;
                break;
            }
        }

        if (!bAccepted) {
            if ((nByte != 0xE8) && (nByte != 0xE9)) continue;
        }

        if (nPos >= nSize) return false;
        if (pData[nPos] != nMarker) continue;
        if (nPos + 4 > nSize) return false;

        const quint32 nValue = ((quint32)pData[nPos + 1] << 16) | ((quint32)pData[nPos + 2] << 8) | (quint32)pData[nPos + 3];
        nPos += 4;

        const quint32 nRelative = nValue - (quint32)nPos;

        pData[nPos - 4] = (quint8)(nRelative & 0xFF);
        pData[nPos - 3] = (quint8)((nRelative >> 8) & 0xFF);
        pData[nPos - 2] = (quint8)((nRelative >> 16) & 0xFF);
        pData[nPos - 1] = (quint8)((nRelative >> 24) & 0xFF);

        nCount--;
    }

    return true;
}

}  // namespace

XKolibriKPackDecoder::XKolibriKPackDecoder(QObject *parent) : QObject(parent)
{
}

bool XKolibriKPackDecoder::checkHeader(const char *pHeader, qint64 nHeaderSize, qint64 nFileSize, quint32 *pnUnpackedSize, quint32 *pnFlags)
{
    if ((!pHeader) || (nHeaderSize < KPACK_HEADER_SIZE)) return false;

    const quint8 *pData = (const quint8 *)pHeader;

    if ((pData[0] != 'K') || (pData[1] != 'P') || (pData[2] != 'C') || (pData[3] != 'K')) return false;

    const quint32 nUnpackedSize = (quint32)pData[4] | ((quint32)pData[5] << 8) | ((quint32)pData[6] << 16) | ((quint32)pData[7] << 24);
    const quint32 nFlags = (quint32)pData[8] | ((quint32)pData[9] << 8) | ((quint32)pData[10] << 16) | ((quint32)pData[11] << 24);

    // kpack's own unpacker rejects everything but "LZMA plus at most one of the
    // two call-trick filters"; the upper 24 bits of the method dword are unused.
    if ((nFlags & ~(KPACK_FLAG_CALLTRICK1 | KPACK_FLAG_CALLTRICK2)) != KPACK_FLAG_LZMA) return false;
    if ((nFlags & (KPACK_FLAG_CALLTRICK1 | KPACK_FLAG_CALLTRICK2)) == (KPACK_FLAG_CALLTRICK1 | KPACK_FLAG_CALLTRICK2)) return false;

    if ((nUnpackedSize == 0) || ((qint64)nUnpackedSize > KPACK_MAX_OUTPUT_SIZE)) return false;

    qint64 nMinimumSize = KPACK_HEADER_SIZE + 4;
    if (nFlags & (KPACK_FLAG_CALLTRICK1 | KPACK_FLAG_CALLTRICK2)) {
        nMinimumSize += KPACK_CALLTRICK_TRAILER_SIZE;
    }

    if ((nFileSize >= 0) && (nFileSize < nMinimumSize)) return false;

    if (pnUnpackedSize) *pnUnpackedSize = nUnpackedSize;
    if (pnFlags) *pnFlags = nFlags;

    return true;
}

bool XKolibriKPackDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if ((!pDecompressState) || (!pDecompressState->pDeviceInput) || (!pDecompressState->pDeviceOutput) || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < 0)) {
        return false;
    }

    const qint64 nInputSize = pDecompressState->nInputLimit;

    if ((nInputSize < (KPACK_HEADER_SIZE + 4)) || (nInputSize > KPACK_MAX_INPUT_SIZE)) {
        return false;
    }

    Algo_utils::prepareState(pDecompressState);

    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QByteArray baInput;
    baInput.resize((qint32)nInputSize);

    if (baInput.size() != (qint32)nInputSize) {
        pDecompressState->bReadError = true;
        return false;
    }

    {
        qint64 nDone = 0;

        while (nDone < nInputSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

            const qint64 nPortion = qMin<qint64>(0x10000, nInputSize - nDone);
            const qint32 nRead = XBinary::_readDevice(baInput.data() + nDone, (qint32)nPortion, pDecompressState);

            if (nRead <= 0) {
                pDecompressState->bReadError = true;
                return false;
            }

            nDone += nRead;
        }
    }

    quint32 nUnpackedSize = 0;
    quint32 nFlags = 0;

    if (!checkHeader(baInput.constData(), baInput.size(), nInputSize, &nUnpackedSize, &nFlags)) {
        return false;
    }

    if (pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        bool bConverted = false;
        const qint64 nDeclaredSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
        if (!bConverted || (nDeclaredSize != (qint64)nUnpackedSize)) return false;
    }

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pDecompressState->mapUnpackProperties, &nOutputLimit) ||
        !XBinary::isUnpackOutputSizeAllowed(pDecompressState->mapUnpackProperties, (qint64)nUnpackedSize)) {
        return false;
    }

    if ((nOutputLimit >= 0) && ((qint64)nUnpackedSize > nOutputLimit)) {
        return false;
    }

    QByteArray baOutput;
    baOutput.resize((qint32)nUnpackedSize);

    if (baOutput.size() != (qint32)nUnpackedSize) {
        pDecompressState->bWriteError = true;
        return false;
    }

    qint64 nStreamSize = nInputSize;
    const bool bCallTrick1 = (nFlags & KPACK_FLAG_CALLTRICK1) != 0;
    const bool bCallTrick2 = (nFlags & KPACK_FLAG_CALLTRICK2) != 0;

    if (bCallTrick1 || bCallTrick2) {
        nStreamSize -= KPACK_CALLTRICK_TRAILER_SIZE;
    }

    if (nStreamSize < (KPACK_HEADER_SIZE + 4)) return false;

    if (!kpackDecodeLzma((const quint8 *)baInput.constData(), nStreamSize, KPACK_HEADER_SIZE, (quint8 *)baOutput.data(), baOutput.size())) {
        return false;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    if (bCallTrick1 || bCallTrick2) {
        const quint8 *pTrailer = (const quint8 *)baInput.constData() + (nInputSize - KPACK_CALLTRICK_TRAILER_SIZE);
        const quint32 nCount = (quint32)pTrailer[0] | ((quint32)pTrailer[1] << 8) | ((quint32)pTrailer[2] << 16) | ((quint32)pTrailer[3] << 24);
        const quint8 nMarker = pTrailer[4];

        if (nCount > (quint32)baOutput.size()) return false;

        if (!kpackUndoCallTrick((quint8 *)baOutput.data(), baOutput.size(), nCount, nMarker, bCallTrick2)) {
            return false;
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    if (XBinary::_writeDevice(baOutput.data(), baOutput.size(), pDecompressState) != baOutput.size()) {
        return false;
    }

    return !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
