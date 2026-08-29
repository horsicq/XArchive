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
#include "xquantumdecoder.h"

#include <algorithm>
#include <vector>

// Clean-room from russotto.net/quantumcomp.html; a 1:1 port of the validated
// Python prototype. See the header for the (thin) verification scope.

namespace {

// Appendix A: position slots (42) and length slots (27).
const quint32 POS_BASE[42] = {0x00000, 0x00001, 0x00002, 0x00003, 0x00004, 0x00006, 0x00008, 0x0000c, 0x00010, 0x00018, 0x00020, 0x00030, 0x00040,  0x00060,
                              0x00080, 0x000c0, 0x00100, 0x00180, 0x00200, 0x00300, 0x00400, 0x00600, 0x00800, 0x00c00, 0x01000, 0x01800, 0x02000,  0x03000,
                              0x04000, 0x06000, 0x08000, 0x0c000, 0x10000, 0x18000, 0x20000, 0x30000, 0x40000, 0x60000, 0x80000, 0xc0000, 0x100000, 0x180000};
const int POS_XB[42] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19};
const quint32 LEN_BASE[27] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x12, 0x16, 0x1a,
                              0x1e, 0x26, 0x2e, 0x36, 0x3e, 0x4e, 0x5e, 0x6e, 0x7e, 0x9e, 0xbe, 0xde, 0xfe};
const int LEN_XB[27] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

inline int numPositionSlots(int nOrder)
{
    return 20 + 2 * (nOrder - 10);  // order 10 -> 20 ... order 21 -> 42
}

// Adaptive frequency model. cf[i] is the cumulative frequency of slots i..n-1,
// cf[n] == 0, cf[0] == total. syms[] holds the symbol values in slot order.
struct QModel {
    int n;
    std::vector<int> syms;
    std::vector<int> cf;  // size n + 1
    int nTimeToReorder;

    QModel(int nsyms, int nFirst) : n(nsyms), nTimeToReorder(4)
    {
        syms.resize(n);
        cf.resize(n + 1);
        for (int i = 0; i < n; i++) syms[i] = nFirst + i;
        for (int i = 0; i <= n; i++) cf[i] = n - i;  // each freq == 1
    }

    void update(int nSlot)
    {
        for (int i = 0; i <= nSlot; i++) cf[i] += 8;
        if (cf[0] > 3800) {
            nTimeToReorder--;
            if (nTimeToReorder != 0) {
                rescale();
            } else {
                reorder();
            }
        }
    }

    void rescale()
    {
        for (int k = n - 1; k >= 0; k--) {
            cf[k] >>= 1;
            if (cf[k] <= cf[k + 1]) cf[k] = cf[k + 1] + 1;
        }
    }

    void reorder()
    {
        std::vector<int> freqs(n);
        for (int i = 0; i < n; i++) freqs[i] = ((cf[i] - cf[i + 1]) + 1) >> 1;
        std::vector<int> idx(n);
        for (int i = 0; i < n; i++) idx[i] = i;
        // Selection sort by frequency descending (matches the prototype's
        // strictly-greater comparison and swap order exactly).
        for (int i = 0; i < n; i++) {
            int m = i;
            for (int j = i + 1; j < n; j++) {
                if (freqs[idx[j]] > freqs[idx[m]]) m = j;
            }
            std::swap(idx[i], idx[m]);
        }
        std::vector<int> newsyms(n), f2(n);
        for (int i = 0; i < n; i++) {
            newsyms[i] = syms[idx[i]];
            f2[i] = freqs[idx[i]];
        }
        syms = newsyms;
        cf[n] = 0;
        for (int i = n - 1; i >= 0; i--) cf[i] = cf[i + 1] + f2[i];
        nTimeToReorder = 50;
    }
};

// MSB-first bit reader over one CFDATA payload; past-end reads yield 0.
struct QBitReader {
    const quint8 *pData;
    qint64 nSize;
    qint64 nPos;  // bit index

    QBitReader(const QByteArray &ba) : pData((const quint8 *)ba.constData()), nSize(ba.size()), nPos(0)
    {
    }

    int getBit()
    {
        const qint64 nByte = nPos >> 3;
        if (nByte >= nSize) {
            nPos++;
            return 0;
        }
        const int nBit = (pData[nByte] >> (7 - (nPos & 7))) & 1;
        nPos++;
        return nBit;
    }

    quint32 getBits(int nCount)
    {
        quint32 nValue = 0;
        for (int i = 0; i < nCount; i++) nValue = (nValue << 1) | (quint32)getBit();
        return nValue;
    }
};

// Arithmetic (range) decoder, 16-bit L/H/C, with the 0x8000/0x4000 underflow
// handling.
struct QDecoder {
    QBitReader br;
    quint32 nL, nH, nC;

    QDecoder(const QByteArray &ba) : br(ba), nL(0x0000), nH(0xFFFF)
    {
        nC = br.getBits(16);
    }

    quint32 getFreq(quint32 nTotFreq)
    {
        const quint32 nRange = ((nH - nL) & 0xFFFF) + 1;
        return (quint32)(((((quint64)nC - nL + 1) * nTotFreq - 1) / nRange) & 0xFFFF);
    }

    void getCode(quint32 nCumFreqM1, quint32 nCumFreq, quint32 nTotFreq)
    {
        const quint32 nRange = (nH - nL) + 1;
        nH = (quint32)((nL + (quint64)nCumFreqM1 * nRange / nTotFreq - 1) & 0xFFFF);
        nL = (quint32)((nL + (quint64)nCumFreq * nRange / nTotFreq) & 0xFFFF);
        for (;;) {
            if ((nL & 0x8000) != (nH & 0x8000)) {
                if ((nL & 0x4000) && !(nH & 0x4000)) {
                    nC ^= 0x4000;
                    nL &= 0x3FFF;
                    nH |= 0x4000;
                } else {
                    break;
                }
            }
            nL = (nL << 1) & 0xFFFF;
            nH = ((nH << 1) | 1) & 0xFFFF;
            nC = ((nC << 1) | (quint32)br.getBit()) & 0xFFFF;
        }
    }

    // Returns the decoded symbol, or -1 on a malformed model index.
    int getSym(QModel &model)
    {
        const quint32 nFreq = getFreq((quint32)model.cf[0]);
        int i = 1;
        while ((i <= model.n) && ((quint32)model.cf[i] > nFreq)) i++;
        if (i > model.n) return -1;  // no slot covers the value
        const int nSlot = i - 1;
        getCode((quint32)model.cf[nSlot], (quint32)model.cf[nSlot + 1], (quint32)model.cf[0]);
        const int nSym = model.syms[nSlot];
        model.update(nSlot);
        return nSym;
    }

    quint32 getBitsRaw(int nCount)
    {
        return br.getBits(nCount);
    }
};

// The 9 adaptive models plus the persistent LZ history (which is exactly the
// output produced so far, so the caller's output buffer serves as the window).
struct QuantumState {
    int nOrder;
    int nSlots;
    QModel selector;
    std::vector<QModel> lit;  // 4 literal banks of 64 symbols
    std::vector<QModel> pos;  // 3 position models (selectors 4/5/6)
    QModel length;

    explicit QuantumState(int order) : nOrder(order), nSlots(numPositionSlots(order)), selector(7, 0), length(27, 0)
    {
        for (int k = 0; k < 4; k++) lit.push_back(QModel(64, 64 * k));
        pos.push_back(QModel(std::min(nSlots, 24), 0));
        pos.push_back(QModel(std::min(nSlots, 36), 0));
        pos.push_back(QModel(std::min(nSlots, 42), 0));
    }
};

bool decodeBlock(QuantumState &st, const QByteArray &baData, qint64 nOutLen, QByteArray *pOut)
{
    QDecoder dec(baData);
    const qint64 nStart = pOut->size();
    while ((pOut->size() - nStart) < nOutLen) {
        const int nSel = dec.getSym(st.selector);
        if ((nSel < 0) || (nSel > 6)) return false;
        if (nSel <= 3) {
            const int nByte = dec.getSym(st.lit[nSel]);
            if (nByte < 0) return false;
            pOut->append((char)(quint8)nByte);
        } else {
            qint64 nLength;
            int nPm;
            if (nSel == 4) {
                nLength = 3;
                nPm = 0;
            } else if (nSel == 5) {
                nLength = 4;
                nPm = 1;
            } else {  // nSel == 6
                const int nLenSlot = dec.getSym(st.length);
                if ((nLenSlot < 0) || (nLenSlot >= 27)) return false;
                nLength = (qint64)LEN_BASE[nLenSlot] + dec.getBitsRaw(LEN_XB[nLenSlot]) + 5;
                nPm = 2;
            }
            const int nPosSlot = dec.getSym(st.pos[nPm]);
            if ((nPosSlot < 0) || (nPosSlot >= 42)) return false;
            const quint64 nOffset = (quint64)POS_BASE[nPosSlot] + dec.getBitsRaw(POS_XB[nPosSlot]) + 1;
            qint64 nSrc = (qint64)pOut->size() - (qint64)nOffset;
            if ((nSrc < 0) || (nLength <= 0)) return false;
            // A match must not overrun this block's declared output (valid
            // Quantum never does; fail closed on malformed data).
            if (((pOut->size() - nStart) + nLength) > nOutLen) return false;
            for (qint64 k = 0; k < nLength; k++) {
                // Re-fetch constData() each iteration: append() below may
                // reallocate. nSrc stays < size() because it trails the tail.
                const quint8 c = (quint8)(pOut->constData()[nSrc]);
                pOut->append((char)c);
                nSrc++;
            }
        }
    }
    return ((pOut->size() - nStart) == nOutLen);
}

}  // namespace

bool XQuantumDecoder::decompressCABDataBlocks(const QList<QByteArray> &listCompressedBlocks, const QList<qint32> &listUncompressedSizes, QByteArray *pbaUncompressed,
                                              qint32 nWindowBits, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaUncompressed) return false;
    pbaUncompressed->clear();
    if (listCompressedBlocks.isEmpty() || (listCompressedBlocks.size() != listUncompressedSizes.size())) return false;
    if ((nWindowBits < 10) || (nWindowBits > 21)) return false;

    QuantumState state((int)nWindowBits);
    for (int i = 0; i < listCompressedBlocks.size(); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nUncomp = listUncompressedSizes.at(i);
        if (nUncomp <= 0) return false;
        if (!decodeBlock(state, listCompressedBlocks.at(i), (qint64)nUncomp, pbaUncompressed)) return false;
    }

    return true;
}
