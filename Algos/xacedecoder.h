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
#ifndef XACEDECODER_H
#define XACEDECODER_H

#include "xbinary.h"

class XAceDecoder : public QObject {
    Q_OBJECT

public:
    explicit XAceDecoder(QObject *pParent = nullptr);

    // Decompress ACE method 1: LZ77 + Huffman
    static bool decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);
    // Decompress ACE method 2: LZ77 + Huffman + inverse byte-delta filter
    static bool decompressDelta(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct = nullptr);

private:
    // ACE 1.x constants (from unace1/src/unace.h, wdlkmpx/unace1)
    static const qint32 ACE_MAXDIC      = 22;
    static const qint32 ACE_MAXWD_MN    = 11;   // main Huffman max code width
    static const qint32 ACE_MAXWD_LG    = 11;   // length Huffman max code width
    static const qint32 ACE_MAXWD_SVWD  = 7;    // meta-Huffman max code width
    static const qint32 ACE_MAXLENGTH   = 259;  // max match length
    static const qint32 ACE_MAXDIS2     = 255;  // max dist for 2-byte match
    static const qint32 ACE_MAXDIS3     = 8191; // max dist for 3-byte match
    static const qint32 ACE_MAXCODE     = 281;  // 255 + 4 + ACE_MAXDIC
    static const qint32 ACE_SVWD_CNT    = 15;   // meta-Huffman symbol count
    static const qint32 ACE_MAX_CD_MN   = 282;  // 256 + 4 + (ACE_MAXDIC+1) - 1
    static const qint32 ACE_MAX_CD_LG   = 255;  // 256 - 1
    static const qint32 ACE_SIZE_RDB    = 2048; // DWORD elements in read buffer
    static const qint32 ACE_CODE_MN_SZ  = (1 << ACE_MAXWD_MN);  // 2048
    static const qint32 ACE_CODE_LG_SZ  = (1 << ACE_MAXWD_LG);  // 2048
    static const qint32 ACE_CODE_SV_SZ  = (1 << ACE_MAXWD_SVWD); // 128

    struct AceDecodeState {
        // DWORD-based LE MSB-first bit reader (exactly as in uac_dcpr.c)
        quint32 nBufRd[ACE_SIZE_RDB + 2];  // read buffer (DWORDs)
        qint32  nRPos;                      // current DWORD index
        qint32  nBitsRd;                    // bit offset within DWORD (0-31)
        quint32 nCodeRd;                    // current 32-bit code window

        // Huffman decode tables
        quint16 nCodeMn[ACE_CODE_MN_SZ];    // main symbol decode table
        quint16 nCodeLg[ACE_CODE_LG_SZ];    // length symbol decode table
        quint16 nCodeSv[ACE_CODE_SV_SZ];    // meta-Huffman decode table (temp)
        quint8  nWdMn[ACE_MAX_CD_MN + 2];  // main code widths
        quint8  nWdLg[ACE_MAX_CD_LG + 2];  // length code widths
        quint8  nWdSvwd[ACE_SVWD_CNT + 1]; // meta-Huffman widths (temp)

        // Quicksort scratch arrays (per makecode call)
        quint16 nSortOrg[ACE_MAX_CD_MN + 2];
        quint8  nSortFreq[ACE_MAX_CD_MN + 2];

        // Dictionary
        char   *pText;       // ring buffer
        qint32  nDPos;       // current write position
        qint32  nDicSiz;     // 1 << dicbits (from PARM)
        qint32  nDicAnd;     // nDicSiz - 1

        // LZ77 state
        quint32 nOldDist[4]; // recent distances (ring)
        qint32  nOldNum;     // oldest slot index

        // Block state
        qint32  nBlockSize;  // remaining symbols in current block
        qint32  nDcrDo;      // symbols output in current call
        qint32  nDcrDoMax;   // target symbols for current call
        qint64  nDcrSize;    // total bytes remaining to decompress

        // I/O
        QIODevice *pInput;
        qint64     nInputBytesRead;
        qint64     nInputLimit;  // compressed bytes available

        bool bError;
    };

    // Bit reader
    static void readDat(AceDecodeState *pState);
    static void addBits(AceDecodeState *pState, qint32 nBits);
    static quint32 peekBits(AceDecodeState *pState, qint32 nBits);

    // Quicksort (exact port of sortrange/quicksort from uac_dcpr.c)
    static void sortRange(AceDecodeState *pState, qint32 nLeft, qint32 nRight);
    static void quickSort(AceDecodeState *pState, qint32 nN);

    // Huffman
    static qint32 makeCode(AceDecodeState *pState, quint32 nMaxWd, quint32 nSize1t, quint8 *pWd, quint16 *pCode);
    static qint32 readWd(AceDecodeState *pState, quint32 nMaxWd, quint16 *pCode, quint8 *pWd, quint32 nMaxEl);
    static qint32 calcDecTabs(AceDecodeState *pState);

    // LZ77 output
    static void copyStr(AceDecodeState *pState, qint32 nDist, qint32 nLen);
    static void decompressBlock(AceDecodeState *pState);

    // Block-level decompressor
    static qint32 decompressBlk(AceDecodeState *pState, char *pBuf, qint32 nLen);

    // Internal entry: decompress nOrigSize bytes to pOutput
    static bool decompressInternal(XBinary::DATAPROCESS_STATE *pDecompressState, QIODevice *pOutput, XBinary::PDSTRUCT *pPdStruct);
};

#endif  // XACEDECODER_H
