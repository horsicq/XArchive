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
#ifndef XBRANCHDECODER_H
#define XBRANCHDECODER_H

#include "xbinary.h"

// Inverse (decode) branch-convert filters and the delta filter used by 7z and XZ
// filter chains. Semantics follow the public-domain LZMA SDK reference (Bra.c/Delta.c).
class XBranchDecoder {
public:
    enum BTYPE {
        BTYPE_ARM = 0,  // ARM (LE) BL
        BTYPE_ARMT,     // ARM Thumb (LE) BL pairs
        BTYPE_ARM64,    // ARM64 BL/ADRP
        BTYPE_PPC,      // PowerPC (BE) bl
        BTYPE_SPARC,    // SPARC call
        BTYPE_IA64      // IA64 branch bundles
    };

    static void applyBranchDecode(QByteArray &baData, BTYPE type, quint32 nIp = 0, XBinary::PDSTRUCT *pPdStruct = nullptr);
    static void applyDeltaDecode(QByteArray &baData, qint32 nDistance, XBinary::PDSTRUCT *pPdStruct = nullptr);

    // Whole-buffer stream wrappers for the XDecompress dispatch
    static bool decompressBranch(XBinary::DATAPROCESS_STATE *pState, BTYPE type, XBinary::PDSTRUCT *pPdStruct, quint32 nIp = 0);
    static bool decompressDelta(XBinary::DATAPROCESS_STATE *pState, qint32 nDistance, XBinary::PDSTRUCT *pPdStruct);

private:
    static void _decodeARM(unsigned char *pData, qint32 nSize, quint32 nIp, XBinary::PDSTRUCT *pPdStruct);
    static void _decodeARMT(unsigned char *pData, qint32 nSize, quint32 nIp, XBinary::PDSTRUCT *pPdStruct);
    static void _decodeARM64(unsigned char *pData, qint32 nSize, quint32 nIp, XBinary::PDSTRUCT *pPdStruct);
    static void _decodePPC(unsigned char *pData, qint32 nSize, quint32 nIp, XBinary::PDSTRUCT *pPdStruct);
    static void _decodeSPARC(unsigned char *pData, qint32 nSize, quint32 nIp, XBinary::PDSTRUCT *pPdStruct);
    static void _decodeIA64(unsigned char *pData, qint32 nSize, quint32 nIp, XBinary::PDSTRUCT *pPdStruct);
};

#endif  // XBRANCHDECODER_H
