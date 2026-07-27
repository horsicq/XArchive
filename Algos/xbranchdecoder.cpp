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
#include "xbranchdecoder.h"

void XBranchDecoder::applyBranchDecode(QByteArray &baData, BTYPE type, quint32 nIp)
{
    qint32 nSize = baData.size();
    unsigned char *pData = reinterpret_cast<unsigned char *>(baData.data());

    if ((nSize <= 0) || (pData == nullptr)) {
        return;
    }

    switch (type) {
        case BTYPE_ARM: _decodeARM(pData, nSize, nIp); break;
        case BTYPE_ARMT: _decodeARMT(pData, nSize, nIp); break;
        case BTYPE_ARM64: _decodeARM64(pData, nSize, nIp); break;
        case BTYPE_PPC: _decodePPC(pData, nSize, nIp); break;
        case BTYPE_SPARC: _decodeSPARC(pData, nSize, nIp); break;
        case BTYPE_IA64: _decodeIA64(pData, nSize, nIp); break;
    }
}

void XBranchDecoder::applyDeltaDecode(QByteArray &baData, qint32 nDistance)
{
    qint32 nSize = baData.size();
    unsigned char *pData = reinterpret_cast<unsigned char *>(baData.data());

    if ((nSize <= 0) || (pData == nullptr) || (nDistance <= 0)) {
        return;
    }

    // decoded[i] = encoded[i] + decoded[i - distance], history before start is zero
    for (qint32 i = nDistance; i < nSize; i++) {
        pData[i] = (unsigned char)(pData[i] + pData[i - nDistance]);
    }
}

void XBranchDecoder::_decodeARM(unsigned char *pData, qint32 nSize, quint32 nIp)
{
    // BL: cond=1110 -> byte3 == 0xEB; imm24 words, PC bias +8
    for (qint32 i = 0; i + 4 <= nSize; i += 4) {
        if (pData[i + 3] == 0xEB) {
            quint32 v = (quint32)pData[i] | ((quint32)pData[i + 1] << 8) | ((quint32)pData[i + 2] << 16);
            v <<= 2;
            v -= (nIp + (quint32)i + 8);
            v >>= 2;
            pData[i] = (unsigned char)v;
            pData[i + 1] = (unsigned char)(v >> 8);
            pData[i + 2] = (unsigned char)(v >> 16);
        }
    }
}

void XBranchDecoder::_decodeARMT(unsigned char *pData, qint32 nSize, quint32 nIp)
{
    // Thumb BL pair: F0xx F8xx; 22-bit halfword offset, PC bias +4
    for (qint32 i = 0; i + 4 <= nSize; i += 2) {
        if (((pData[i + 1] & 0xF8) == 0xF0) && ((pData[i + 3] & 0xF8) == 0xF8)) {
            quint32 v = (((quint32)pData[i + 1] & 0x07) << 19) | ((quint32)pData[i] << 11) | (((quint32)pData[i + 3] & 0x07) << 8) | (quint32)pData[i + 2];
            v <<= 1;
            v -= (nIp + (quint32)i + 4);
            v >>= 1;
            pData[i + 1] = (unsigned char)(0xF0 | ((v >> 19) & 0x07));
            pData[i] = (unsigned char)(v >> 11);
            pData[i + 3] = (unsigned char)(0xF8 | ((v >> 8) & 0x07));
            pData[i + 2] = (unsigned char)v;
            i += 2;
        }
    }
}

void XBranchDecoder::_decodeARM64(unsigned char *pData, qint32 nSize, quint32 nIp)
{
    // BL (0x94xxxxxx) imm26 and ADRP (0x90xxxxxx) page addresses
    const quint32 kFlag = (quint32)1 << 20;
    const quint32 kMask = ((quint32)1 << 24) - (kFlag << 1);

    qint32 nAligned = nSize & ~3;

    for (qint32 i = 0; i < nAligned; i += 4) {
        quint32 v = (quint32)pData[i] | ((quint32)pData[i + 1] << 8) | ((quint32)pData[i + 2] << 16) | ((quint32)pData[i + 3] << 24);

        if (((v - 0x94000000) & 0xFC000000) == 0) {
            quint32 c = (nIp + (quint32)i) >> 2;
            v -= c;
            v &= 0x03FFFFFF;
            v |= 0x94000000;
        } else {
            quint32 vt = v - 0x90000000;
            if ((vt & 0x9F000000) == 0) {
                vt += kFlag;
                if (!(vt & kMask)) {
                    quint32 z = (vt & 0xFFFFFFE0) | (vt >> 26);
                    quint32 c = ((nIp + (quint32)i) >> 9) & ~(quint32)7;
                    z -= c;
                    v &= 0x0000001F;
                    v |= 0x90000000;
                    v |= z << 26;
                    v |= 0x00FFFFE0 & ((z & ((kFlag << 1) - 1)) - kFlag);
                }
            }
        }

        pData[i] = (unsigned char)v;
        pData[i + 1] = (unsigned char)(v >> 8);
        pData[i + 2] = (unsigned char)(v >> 16);
        pData[i + 3] = (unsigned char)(v >> 24);
    }
}

void XBranchDecoder::_decodePPC(unsigned char *pData, qint32 nSize, quint32 nIp)
{
    // bl: opcode 18, AA=0, LK=1 -> (b0 & 0xFC) == 0x48 && (b3 & 3) == 1; big-endian imm24 words
    for (qint32 i = 0; i + 4 <= nSize; i += 4) {
        if (((pData[i] & 0xFC) == 0x48) && ((pData[i + 3] & 0x03) == 0x01)) {
            quint32 v = (((quint32)pData[i] & 0x03) << 24) | ((quint32)pData[i + 1] << 16) | ((quint32)pData[i + 2] << 8) | ((quint32)pData[i + 3] & 0xFC);
            v -= (nIp + (quint32)i);
            pData[i] = (unsigned char)(0x48 | ((v >> 24) & 0x03));
            pData[i + 1] = (unsigned char)(v >> 16);
            pData[i + 2] = (unsigned char)(v >> 8);
            pData[i + 3] = (unsigned char)((pData[i + 3] & 0x03) | (v & 0xFC));
        }
    }
}

void XBranchDecoder::_decodeSPARC(unsigned char *pData, qint32 nSize, quint32 nIp)
{
    // call: 01 + disp30, matched as 0x40 00-3F or 0x7F C0-FF; big-endian words
    for (qint32 i = 0; i + 4 <= nSize; i += 4) {
        if (((pData[i] == 0x40) && ((pData[i + 1] & 0xC0) == 0x00)) || ((pData[i] == 0x7F) && ((pData[i + 1] & 0xC0) == 0xC0))) {
            quint32 v = ((quint32)pData[i] << 24) | ((quint32)pData[i + 1] << 16) | ((quint32)pData[i + 2] << 8) | (quint32)pData[i + 3];
            v <<= 2;
            v -= (nIp + (quint32)i);
            v >>= 2;
            v = ((0x40000000 - (v & 0x400000)) | 0x40000000 | (v & 0x3FFFFF));
            pData[i] = (unsigned char)(v >> 24);
            pData[i + 1] = (unsigned char)(v >> 16);
            pData[i + 2] = (unsigned char)(v >> 8);
            pData[i + 3] = (unsigned char)v;
        }
    }
}

void XBranchDecoder::_decodeIA64(unsigned char *pData, qint32 nSize, quint32 nIp)
{
    // 16-byte bundles; template selects which 41-bit slots hold branch instructions
    static const unsigned char kBranchTable[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 4, 6, 6, 0, 0, 7, 7, 4, 4, 0, 0, 4, 4, 0, 0};

    for (qint32 i = 0; i + 16 <= nSize; i += 16) {
        quint32 nTemplate = pData[i] & 0x1F;
        quint32 nMask = kBranchTable[nTemplate];

        if (nMask == 0) {
            continue;
        }

        quint32 nBitPos = 5;

        for (qint32 nSlot = 0; nSlot < 3; nSlot++, nBitPos += 41) {
            if (((nMask >> nSlot) & 1) == 0) {
                continue;
            }

            quint32 nBytePos = nBitPos >> 3;
            quint32 nBitRes = nBitPos & 7;

            quint64 nInstruction = 0;
            for (qint32 j = 0; j < 6; j++) {
                nInstruction |= (quint64)pData[i + (qint32)nBytePos + j] << (8 * j);
            }

            quint64 nInstNorm = nInstruction >> nBitRes;

            if ((((nInstNorm >> 37) & 0xF) == 5) && (((nInstNorm >> 9) & 0x7FFFFFF) == 0)) {
                quint32 nSrc = (quint32)((nInstNorm >> 13) & 0xFFFFF);
                nSrc |= ((quint32)(nInstNorm >> 36) & 1) << 20;
                nSrc <<= 4;

                quint32 nDest = nSrc - (nIp + (quint32)i);
                nDest >>= 4;

                nInstNorm &= ~((quint64)0x8FFFFF << 13);
                nInstNorm |= ((quint64)(nDest & 0xFFFFF) << 13);
                nInstNorm |= ((quint64)(nDest & 0x100000) << (36 - 20));

                nInstruction &= (((quint64)1 << nBitRes) - 1);
                nInstruction |= (nInstNorm << nBitRes);

                for (qint32 j = 0; j < 6; j++) {
                    pData[i + (qint32)nBytePos + j] = (unsigned char)(nInstruction >> (8 * j));
                }
            }
        }
    }
}

static bool _readTransformWrite(XBinary::DATAPROCESS_STATE *pState, QByteArray &baData)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput) {
        return false;
    }

    baData = pState->pDeviceInput->read(pState->nInputLimit);
    pState->nCountInput = baData.size();

    return true;
}

bool XBranchDecoder::decompressBranch(XBinary::DATAPROCESS_STATE *pState, BTYPE type, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QByteArray baData;

    if (!_readTransformWrite(pState, baData)) {
        return false;
    }

    applyBranchDecode(baData, type);

    qint64 nWritten = pState->pDeviceOutput->write(baData);
    pState->nCountOutput = nWritten;

    return (nWritten == (qint64)baData.size());
}

bool XBranchDecoder::decompressDelta(XBinary::DATAPROCESS_STATE *pState, qint32 nDistance, XBinary::PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    QByteArray baData;

    if (!_readTransformWrite(pState, baData)) {
        return false;
    }

    applyDeltaDecode(baData, nDistance);

    qint64 nWritten = pState->pDeviceOutput->write(baData);
    pState->nCountOutput = nWritten;

    return (nWritten == (qint64)baData.size());
}
