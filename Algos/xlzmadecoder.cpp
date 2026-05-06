/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "xlzmadecoder.h"
#include "algo_utils.h"
#include "xalgo_local.h"
#include <QBuffer>

XLZMADecoder::XLZMADecoder(QObject *parent) : QObject(parent)
{
}

bool XLZMADecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (pDecompressState->nInputLimit < 4) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    qint32 nPropSize = 0;
    char header1[4] = {};
    quint8 properties[32] = {};

    XBinary::_readDevice(header1, sizeof(header1), pDecompressState);
    nPropSize = header1[2];  // TODO Check

    if (!nPropSize || nPropSize >= 30) {
        return false;
    }

    XBinary::_readDevice((char *)properties, nPropSize, pDecompressState);

    CLzmaDec state = {};
    SRes ret = X_LzmaProps_Decode(&state.prop, (Byte *)properties, nPropSize);

    if (ret != 0) {  // S_OK
        return false;
    }

    X_LzmaDec_Construct(&state);
    ret = X_LzmaDec_Allocate(&state, (Byte *)properties, nPropSize, Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    X_LzmaDec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA(&state, pDecompressState, pPdStruct);
    X_LzmaDec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
}

bool XLZMADecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        // qDebug("XLZMADecoder::decompress() FAILED: null pointer check");
        return false;
    }

    if (baProperty.size() <= 0 || baProperty.size() >= 30) {
        // qDebug("XLZMADecoder::decompress() FAILED: invalid baProperty size: %d", baProperty.size());
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    CLzmaDec state = {};
    SRes ret = X_LzmaProps_Decode(&state.prop, (Byte *)baProperty.constData(), baProperty.size());

    if (ret != 0) {
        qDebug("[LZMA] LzmaProps_Decode FAILED: %d", ret);
        return false;
    }

    X_LzmaDec_Construct(&state);
    ret = X_LzmaDec_Allocate(&state, (Byte *)baProperty.constData(), baProperty.size(), Algo_utils::lzmaAlloc());

    if (ret != 0) {
        qDebug("[LZMA] LzmaDec_Allocate FAILED: %d", ret);
        return false;
    }

    X_LzmaDec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA(&state, pDecompressState, pPdStruct);
    X_LzmaDec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
}

bool XLZMADecoder::decompressLZMA2(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (pDecompressState->nInputLimit < 1) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    // Read LZMA2 properties (1 byte)
    char propByte = 0;
    qint32 nPropsRead = XBinary::_readDevice(&propByte, 1, pDecompressState);

    if (nPropsRead != 1) {
        return false;
    }

    // LZMA2 state
    CLzma2Dec state = {};
    SRes ret = X_Lzma2Dec_Allocate(&state, (Byte)propByte, Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    X_Lzma2Dec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA2(&state, pDecompressState, pPdStruct);
    X_Lzma2Dec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
}

bool XLZMADecoder::decompressLZMA2(XBinary::DATAPROCESS_STATE *pDecompressState, const QByteArray &baProperty, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    if (baProperty.size() != 1) {
        return false;
    }

    pDecompressState->pDeviceInput->seek(pDecompressState->nInputOffset);
    pDecompressState->pDeviceOutput->seek(0);

    // LZMA2 state
    CLzma2Dec state = {};
    SRes ret = X_Lzma2Dec_Allocate(&state, (Byte)baProperty[0], Algo_utils::lzmaAlloc());

    if (ret != 0) {  // S_OK
        return false;
    }

    X_Lzma2Dec_Init(&state);
    bool bResult = Algo_utils::decompressLZMA2(&state, pDecompressState, pPdStruct);
    X_Lzma2Dec_Free(&state, Algo_utils::lzmaAlloc());

    return bResult;
}

bool XLZMADecoder::decompressXZ(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput) {
        return false;
    }

    QIODevice *pDevice = pDecompressState->pDeviceInput;
    qint64 nOffset = pDecompressState->nInputOffset;
    qint64 nTotalSize = pDecompressState->nInputLimit;

    if (nTotalSize < 28) {  // stream header(12) + minimal block header(4) + stream footer(12)
        return false;
    }

    // --- 1. Validate XZ stream header (12 bytes) ---
    pDevice->seek(nOffset);
    QByteArray baStreamHeader = pDevice->read(12);
    if (baStreamHeader.size() != 12) {
        return false;
    }
    static const quint8 XZ_MAGIC[6] = {0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00};
    if (memcmp(baStreamHeader.constData(), XZ_MAGIC, 6) != 0) {
        return false;
    }

    // --- 2. Parse block header (starts at offset 12) ---
    qint64 nBlockHeaderOffset = nOffset + 12;
    pDevice->seek(nBlockHeaderOffset);

    // header_size byte: actual block header size = (header_size + 1) * 4
    char nHdrSizeBuf = 0;
    if (pDevice->read(&nHdrSizeBuf, 1) != 1) {
        return false;
    }
    quint8 nHeaderSizeByte = (quint8)nHdrSizeBuf;
    qint32 nActualHeaderSize = (nHeaderSizeByte + 1) * 4;
    if (nActualHeaderSize < 4 || nActualHeaderSize > 1024) {
        return false;
    }

    pDevice->seek(nBlockHeaderOffset);
    QByteArray baBH = pDevice->read(nActualHeaderSize);
    if (baBH.size() != nActualHeaderSize) {
        return false;
    }

    quint8 nBlockFlags = (quint8)baBH.at(1);
    qint32 nNumFilters = (nBlockFlags & 0x03) + 1;
    bool bHasCompressedSize = (nBlockFlags & 0x40) != 0;
    bool bHasUncompressedSize = (nBlockFlags & 0x80) != 0;

    qint32 nBHPos = 2;
    quint64 nCompressedSize64 = 0;
    quint64 nUncompressedSize64 = 0;

    if (bHasCompressedSize) {
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nCompressedSize64)) {
            return false;
        }
    }
    if (bHasUncompressedSize) {
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nUncompressedSize64)) {
            return false;
        }
    }

    // Parse filter chain: find LZMA2 props byte and optional BCJ x86 filter
    bool bHasBCJX86 = false;
    quint8 nLZMA2PropsByte = 0;

    for (qint32 nFilter = 0; nFilter < nNumFilters && nFilter < 4; nFilter++) {
        quint64 nFilterID = 0;
        quint64 nPropSize = 0;
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nFilterID)) {
            return false;
        }
        if (!Algo_utils::xzReadVarInt(baBH, nBHPos, nPropSize)) {
            return false;
        }
        if (nFilterID == 0x21) {  // LZMA2
            if (nPropSize == 1 && nBHPos < baBH.size()) {
                nLZMA2PropsByte = (quint8)baBH.at(nBHPos);
            }
        } else if (nFilterID == 0x04) {  // BCJ x86
            bHasBCJX86 = true;
        }
        nBHPos += (qint32)nPropSize;
    }

    // --- 3. Locate compressed block data ---
    qint64 nDataOffset = nBlockHeaderOffset + nActualHeaderSize;
    qint64 nDataSize = 0;
    if (bHasCompressedSize) {
        nDataSize = (qint64)nCompressedSize64;
    } else {
        // Generous estimate; LZMA2 end-of-stream marker stops decompression naturally
        nDataSize = nTotalSize - 12 - nActualHeaderSize - 12;
        if (nDataSize <= 0) {
            return false;
        }
    }

    pDevice->seek(nDataOffset);
    QByteArray baCompressed = pDevice->read(nDataSize);
    if (baCompressed.isEmpty()) {
        return false;
    }

    QBuffer compressedBuffer(&baCompressed);
    compressedBuffer.open(QIODevice::ReadOnly);

    QByteArray baPropByte;
    baPropByte.append((char)nLZMA2PropsByte);

    bool bDecompressResult = false;

    if (bHasBCJX86) {
        // Decompress LZMA2 to intermediate buffer, then apply BCJ x86 reverse
        QByteArray baIntermediate;
        QBuffer intermediateBuffer(&baIntermediate);
        intermediateBuffer.open(QIODevice::WriteOnly);

        XBinary::DATAPROCESS_STATE lzma2State = {};
        lzma2State.pDeviceInput = &compressedBuffer;
        lzma2State.pDeviceOutput = &intermediateBuffer;
        lzma2State.nInputOffset = 0;
        lzma2State.nInputLimit = baCompressed.size();
        lzma2State.nProcessedOffset = 0;
        lzma2State.nProcessedLimit = -1;

        bDecompressResult = XLZMADecoder::decompressLZMA2(&lzma2State, baPropByte, pPdStruct);
        intermediateBuffer.close();

        if (bDecompressResult) {
            Algo_utils::applyBCJX86Decode(baIntermediate);

            qint64 nWriteFrom = pDecompressState->nProcessedOffset;
            qint64 nWriteLimit = pDecompressState->nProcessedLimit;
            if (nWriteLimit == -1) {
                nWriteLimit = baIntermediate.size();
            }
            qint64 nToWrite = qMin(nWriteLimit, (qint64)(baIntermediate.size() - nWriteFrom));
            if (nToWrite > 0 && nWriteFrom >= 0) {
                pDecompressState->pDeviceOutput->seek(0);
                pDecompressState->pDeviceOutput->write(baIntermediate.constData() + nWriteFrom, nToWrite);
                pDecompressState->nCountOutput = nToWrite;
            }
        }
    } else {
        // Pure LZMA2 — stream directly to output device
        XBinary::DATAPROCESS_STATE lzma2State = {};
        lzma2State.pDeviceInput = &compressedBuffer;
        lzma2State.pDeviceOutput = pDecompressState->pDeviceOutput;
        lzma2State.nInputOffset = 0;
        lzma2State.nInputLimit = baCompressed.size();
        lzma2State.nProcessedOffset = pDecompressState->nProcessedOffset;
        lzma2State.nProcessedLimit = pDecompressState->nProcessedLimit;

        bDecompressResult = XLZMADecoder::decompressLZMA2(&lzma2State, baPropByte, pPdStruct);

        if (bDecompressResult) {
            pDecompressState->nCountInput = lzma2State.nCountInput;
            pDecompressState->nCountOutput = lzma2State.nCountOutput;
        }
    }

    compressedBuffer.close();

    return bDecompressResult;
}

/* ===== Begin embedded xlzma_local.c ===== */
/* Local renamed copies of the 7-Zip LZMA decoder C entry points. */

#include "xalgo_local.h"

#define LzmaDec_InitDicAndState X_LzmaDec_InitDicAndState
#define LzmaDec_Init X_LzmaDec_Init
#define LzmaDec_DecodeToDic X_LzmaDec_DecodeToDic
#define LzmaDec_DecodeToBuf X_LzmaDec_DecodeToBuf
#define LzmaDec_FreeProbs X_LzmaDec_FreeProbs
#define LzmaDec_Free X_LzmaDec_Free
#define LzmaProps_Decode X_LzmaProps_Decode
#define LzmaDec_AllocateProbs X_LzmaDec_AllocateProbs
#define LzmaDec_Allocate X_LzmaDec_Allocate
#define LzmaDecode X_LzmaDecode
/* LzmaDec.c -- LZMA Decoder
2023-04-07 : Igor Pavlov : Public domain */
#include <string.h>

// #define kNumTopBits 24
#define kTopValue ((UInt32)1 << 24)

#define kNumBitModelTotalBits 11
#define kBitModelTotal (1 << kNumBitModelTotalBits)

#define RC_INIT_SIZE 5

#ifndef Z7_LZMA_DEC_OPT

#define kNumMoveBits 5
#define NORMALIZE if (range < kTopValue) { range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0(p) ttt = *(p); NORMALIZE; bound = (range >> kNumBitModelTotalBits) * (UInt32)ttt; if (code < bound)
#define UPDATE_0(p) range = bound; *(p) = (CLzmaProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define UPDATE_1(p) range -= bound; code -= bound; *(p) = (CLzmaProb)(ttt - (ttt >> kNumMoveBits));
#define GET_BIT2(p, i, A0, A1) IF_BIT_0(p) \
  { UPDATE_0(p)  i = (i + i); A0; } else \
  { UPDATE_1(p)  i = (i + i) + 1; A1; }

#define TREE_GET_BIT(probs, i) { GET_BIT2(probs + i, i, ;, ;); }

#define REV_BIT(p, i, A0, A1) IF_BIT_0(p + i) \
  { UPDATE_0(p + i)  A0; } else \
  { UPDATE_1(p + i)  A1; }
#define REV_BIT_VAR(  p, i, m) REV_BIT(p, i, i += m; m += m, m += m; i += m; )
#define REV_BIT_CONST(p, i, m) REV_BIT(p, i, i += m;       , i += m * 2; )
#define REV_BIT_LAST( p, i, m) REV_BIT(p, i, i -= m        , ; )

#define TREE_DECODE(probs, limit, i) \
  { i = 1; do { TREE_GET_BIT(probs, i); } while (i < limit); i -= limit; }

/* #define Z7_LZMA_SIZE_OPT */

#ifdef Z7_LZMA_SIZE_OPT
#define TREE_6_DECODE(probs, i) TREE_DECODE(probs, (1 << 6), i)
#else
#define TREE_6_DECODE(probs, i) \
  { i = 1; \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  TREE_GET_BIT(probs, i) \
  i -= 0x40; }
#endif

#define NORMAL_LITER_DEC TREE_GET_BIT(prob, symbol)
#define MATCHED_LITER_DEC \
  matchByte += matchByte; \
  bit = offs; \
  offs &= matchByte; \
  probLit = prob + (offs + bit + symbol); \
  GET_BIT2(probLit, symbol, offs ^= bit; , ;)

#endif // Z7_LZMA_DEC_OPT


#define NORMALIZE_CHECK if (range < kTopValue) { if (buf >= bufLimit) return DUMMY_INPUT_EOF; range <<= 8; code = (code << 8) | (*buf++); }

#define IF_BIT_0_CHECK(p) ttt = *(p); NORMALIZE_CHECK bound = (range >> kNumBitModelTotalBits) * (UInt32)ttt; if (code < bound)
#define UPDATE_0_CHECK range = bound;
#define UPDATE_1_CHECK range -= bound; code -= bound;
#define GET_BIT2_CHECK(p, i, A0, A1) IF_BIT_0_CHECK(p) \
  { UPDATE_0_CHECK  i = (i + i); A0; } else \
  { UPDATE_1_CHECK  i = (i + i) + 1; A1; }
#define GET_BIT_CHECK(p, i) GET_BIT2_CHECK(p, i, ; , ;)
#define TREE_DECODE_CHECK(probs, limit, i) \
  { i = 1; do { GET_BIT_CHECK(probs + i, i) } while (i < limit); i -= limit; }


#define REV_BIT_CHECK(p, i, m) IF_BIT_0_CHECK(p + i) \
  { UPDATE_0_CHECK  i += m; m += m; } else \
  { UPDATE_1_CHECK  m += m; i += m; }


#define kNumPosBitsMax 4
#define kNumPosStatesMax (1 << kNumPosBitsMax)

#define kLenNumLowBits 3
#define kLenNumLowSymbols (1 << kLenNumLowBits)
#define kLenNumHighBits 8
#define kLenNumHighSymbols (1 << kLenNumHighBits)

#define LenLow 0
#define LenHigh (LenLow + 2 * (kNumPosStatesMax << kLenNumLowBits))
#define kNumLenProbs (LenHigh + kLenNumHighSymbols)

#define LenChoice LenLow
#define LenChoice2 (LenLow + (1 << kLenNumLowBits))

#define kNumStates 12
#define kNumStates2 16
#define kNumLitStates 7

#define kStartPosModelIndex 4
#define kEndPosModelIndex 14
#define kNumFullDistances (1 << (kEndPosModelIndex >> 1))

#define kNumPosSlotBits 6
#define kNumLenToPosStates 4

#define kNumAlignBits 4
#define kAlignTableSize (1 << kNumAlignBits)

#define kMatchMinLen 2
#define kMatchSpecLenStart (kMatchMinLen + kLenNumLowSymbols * 2 + kLenNumHighSymbols)

#define kMatchSpecLen_Error_Data (1 << 9)
#define kMatchSpecLen_Error_Fail (kMatchSpecLen_Error_Data - 1)

/* External ASM code needs same CLzmaProb array layout. So don't change it. */

/* (probs_1664) is faster and better for code size at some platforms */
/*
#ifdef MY_CPU_X86_OR_AMD64
*/
#define kStartOffset 1664
#define GET_PROBS p->probs_1664
/*
#define GET_PROBS p->probs + kStartOffset
#else
#define kStartOffset 0
#define GET_PROBS p->probs
#endif
*/

#define SpecPos (-kStartOffset)
#define IsRep0Long (SpecPos + kNumFullDistances)
#define RepLenCoder (IsRep0Long + (kNumStates2 << kNumPosBitsMax))
#define LenCoder (RepLenCoder + kNumLenProbs)
#define IsMatch (LenCoder + kNumLenProbs)
#define Align (IsMatch + (kNumStates2 << kNumPosBitsMax))
#define IsRep (Align + kAlignTableSize)
#define IsRepG0 (IsRep + kNumStates)
#define IsRepG1 (IsRepG0 + kNumStates)
#define IsRepG2 (IsRepG1 + kNumStates)
#define PosSlot (IsRepG2 + kNumStates)
#define Literal (PosSlot + (kNumLenToPosStates << kNumPosSlotBits))
#define NUM_BASE_PROBS (Literal + kStartOffset)

#if Align != 0 && kStartOffset != 0
  #error Stop_Compiling_Bad_LZMA_kAlign
#endif

#if NUM_BASE_PROBS != 1984
  #error Stop_Compiling_Bad_LZMA_PROBS
#endif


#define LZMA_LIT_SIZE 0x300

#define LzmaProps_GetNumProbs(p) (NUM_BASE_PROBS + ((UInt32)LZMA_LIT_SIZE << ((p)->lc + (p)->lp)))


#define CALC_POS_STATE(processedPos, pbMask) (((processedPos) & (pbMask)) << 4)
#define COMBINED_PS_STATE (posState + state)
#define GET_LEN_STATE (posState)

#define LZMA_DIC_MIN (1 << 12)

/*
p->remainLen : shows status of LZMA decoder:
    < kMatchSpecLenStart  : the number of bytes to be copied with (p->rep0) offset
    = kMatchSpecLenStart  : the LZMA stream was finished with end mark
    = kMatchSpecLenStart + 1  : need init range coder
    = kMatchSpecLenStart + 2  : need init range coder and state
    = kMatchSpecLen_Error_Fail                : Internal Code Failure
    = kMatchSpecLen_Error_Data + [0 ... 273]  : LZMA Data Error
*/

/* ---------- LZMA_DECODE_REAL ---------- */
/*
LzmaDec_DecodeReal_3() can be implemented in external ASM file.
3 - is the code compatibility version of that function for check at link time.
*/

#define LZMA_DECODE_REAL LzmaDec_DecodeReal_3

/*
LZMA_DECODE_REAL()
In:
  RangeCoder is normalized
  if (p->dicPos == limit)
  {
    LzmaDec_TryDummy() was called before to exclude LITERAL and MATCH-REP cases.
    So first symbol can be only MATCH-NON-REP. And if that MATCH-NON-REP symbol
    is not END_OF_PAYALOAD_MARKER, then the function doesn't write any byte to dictionary,
    the function returns SZ_OK, and the caller can use (p->remainLen) and (p->reps[0]) later.
  }

Processing:
  The first LZMA symbol will be decoded in any case.
  All main checks for limits are at the end of main loop,
  It decodes additional LZMA-symbols while (p->buf < bufLimit && dicPos < limit),
  RangeCoder is still without last normalization when (p->buf < bufLimit) is being checked.
  But if (p->buf < bufLimit), the caller provided at least (LZMA_REQUIRED_INPUT_MAX + 1) bytes for
  next iteration  before limit (bufLimit + LZMA_REQUIRED_INPUT_MAX),
  that is enough for worst case LZMA symbol with one additional RangeCoder normalization for one bit.
  So that function never reads bufLimit [LZMA_REQUIRED_INPUT_MAX] byte.

Out:
  RangeCoder is normalized
  Result:
    SZ_OK - OK
      p->remainLen:
        < kMatchSpecLenStart : the number of bytes to be copied with (p->reps[0]) offset
        = kMatchSpecLenStart : the LZMA stream was finished with end mark

    SZ_ERROR_DATA - error, when the MATCH-Symbol refers out of dictionary
      p->remainLen : undefined
      p->reps[*]    : undefined
*/


#ifdef Z7_LZMA_DEC_OPT

int Z7_FASTCALL LZMA_DECODE_REAL(CLzmaDec *p, SizeT limit, const Byte *bufLimit);

#else

static
int Z7_FASTCALL LZMA_DECODE_REAL(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
  CLzmaProb *probs = GET_PROBS;
  unsigned state = (unsigned)p->state;
  UInt32 rep0 = p->reps[0], rep1 = p->reps[1], rep2 = p->reps[2], rep3 = p->reps[3];
  unsigned pbMask = ((unsigned)1 << (p->prop.pb)) - 1;
  unsigned lc = p->prop.lc;
  unsigned lpMask = ((unsigned)0x100 << p->prop.lp) - ((unsigned)0x100 >> lc);

  Byte *dic = p->dic;
  SizeT dicBufSize = p->dicBufSize;
  SizeT dicPos = p->dicPos;
  
  UInt32 processedPos = p->processedPos;
  UInt32 checkDicSize = p->checkDicSize;
  unsigned len = 0;

  const Byte *buf = p->buf;
  UInt32 range = p->range;
  UInt32 code = p->code;

  do
  {
    CLzmaProb *prob;
    UInt32 bound;
    unsigned ttt;
    unsigned posState = CALC_POS_STATE(processedPos, pbMask);

    prob = probs + IsMatch + COMBINED_PS_STATE;
    IF_BIT_0(prob)
    {
      unsigned symbol;
      UPDATE_0(prob)
      prob = probs + Literal;
      if (processedPos != 0 || checkDicSize != 0)
        prob += (UInt32)3 * ((((processedPos << 8) + dic[(dicPos == 0 ? dicBufSize : dicPos) - 1]) & lpMask) << lc);
      processedPos++;

      if (state < kNumLitStates)
      {
        state -= (state < 4) ? state : 3;
        symbol = 1;
        #ifdef Z7_LZMA_SIZE_OPT
        do { NORMAL_LITER_DEC } while (symbol < 0x100);
        #else
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        NORMAL_LITER_DEC
        #endif
      }
      else
      {
        unsigned matchByte = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
        unsigned offs = 0x100;
        state -= (state < 10) ? 3 : 6;
        symbol = 1;
        #ifdef Z7_LZMA_SIZE_OPT
        do
        {
          unsigned bit;
          CLzmaProb *probLit;
          MATCHED_LITER_DEC
        }
        while (symbol < 0x100);
        #else
        {
          unsigned bit;
          CLzmaProb *probLit;
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
          MATCHED_LITER_DEC
        }
        #endif
      }

      dic[dicPos++] = (Byte)symbol;
      continue;
    }
    
    {
      UPDATE_1(prob)
      prob = probs + IsRep + state;
      IF_BIT_0(prob)
      {
        UPDATE_0(prob)
        state += kNumStates;
        prob = probs + LenCoder;
      }
      else
      {
        UPDATE_1(prob)
        prob = probs + IsRepG0 + state;
        IF_BIT_0(prob)
        {
          UPDATE_0(prob)
          prob = probs + IsRep0Long + COMBINED_PS_STATE;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob)
  
            // that case was checked before with kBadRepCode
            // if (checkDicSize == 0 && processedPos == 0) { len = kMatchSpecLen_Error_Data + 1; break; }
            // The caller doesn't allow (dicPos == limit) case here
            // so we don't need the following check:
            // if (dicPos == limit) { state = state < kNumLitStates ? 9 : 11; len = 1; break; }
            
            dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
            dicPos++;
            processedPos++;
            state = state < kNumLitStates ? 9 : 11;
            continue;
          }
          UPDATE_1(prob)
        }
        else
        {
          UInt32 distance;
          UPDATE_1(prob)
          prob = probs + IsRepG1 + state;
          IF_BIT_0(prob)
          {
            UPDATE_0(prob)
            distance = rep1;
          }
          else
          {
            UPDATE_1(prob)
            prob = probs + IsRepG2 + state;
            IF_BIT_0(prob)
            {
              UPDATE_0(prob)
              distance = rep2;
            }
            else
            {
              UPDATE_1(prob)
              distance = rep3;
              rep3 = rep2;
            }
            rep2 = rep1;
          }
          rep1 = rep0;
          rep0 = distance;
        }
        state = state < kNumLitStates ? 8 : 11;
        prob = probs + RepLenCoder;
      }
      
      #ifdef Z7_LZMA_SIZE_OPT
      {
        unsigned lim, offset;
        CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0(probLen)
        {
          UPDATE_0(probLen)
          probLen = prob + LenLow + GET_LEN_STATE;
          offset = 0;
          lim = (1 << kLenNumLowBits);
        }
        else
        {
          UPDATE_1(probLen)
          probLen = prob + LenChoice2;
          IF_BIT_0(probLen)
          {
            UPDATE_0(probLen)
            probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
            offset = kLenNumLowSymbols;
            lim = (1 << kLenNumLowBits);
          }
          else
          {
            UPDATE_1(probLen)
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols * 2;
            lim = (1 << kLenNumHighBits);
          }
        }
        TREE_DECODE(probLen, lim, len)
        len += offset;
      }
      #else
      {
        CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0(probLen)
        {
          UPDATE_0(probLen)
          probLen = prob + LenLow + GET_LEN_STATE;
          len = 1;
          TREE_GET_BIT(probLen, len)
          TREE_GET_BIT(probLen, len)
          TREE_GET_BIT(probLen, len)
          len -= 8;
        }
        else
        {
          UPDATE_1(probLen)
          probLen = prob + LenChoice2;
          IF_BIT_0(probLen)
          {
            UPDATE_0(probLen)
            probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
            len = 1;
            TREE_GET_BIT(probLen, len)
            TREE_GET_BIT(probLen, len)
            TREE_GET_BIT(probLen, len)
          }
          else
          {
            UPDATE_1(probLen)
            probLen = prob + LenHigh;
            TREE_DECODE(probLen, (1 << kLenNumHighBits), len)
            len += kLenNumLowSymbols * 2;
          }
        }
      }
      #endif

      if (state >= kNumStates)
      {
        UInt32 distance;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates ? len : kNumLenToPosStates - 1) << kNumPosSlotBits);
        TREE_6_DECODE(prob, distance)
        if (distance >= kStartPosModelIndex)
        {
          unsigned posSlot = (unsigned)distance;
          unsigned numDirectBits = (unsigned)(((distance >> 1) - 1));
          distance = (2 | (distance & 1));
          if (posSlot < kEndPosModelIndex)
          {
            distance <<= numDirectBits;
            prob = probs + SpecPos;
            {
              UInt32 m = 1;
              distance++;
              do
              {
                REV_BIT_VAR(prob, distance, m)
              }
              while (--numDirectBits);
              distance -= m;
            }
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE
              range >>= 1;
              
              {
                UInt32 t;
                code -= range;
                t = (0 - ((UInt32)code >> 31)); /* (UInt32)((Int32)code >> 31) */
                distance = (distance << 1) + (t + 1);
                code += range & t;
              }
              /*
              distance <<= 1;
              if (code >= range)
              {
                code -= range;
                distance |= 1;
              }
              */
            }
            while (--numDirectBits);
            prob = probs + Align;
            distance <<= kNumAlignBits;
            {
              unsigned i = 1;
              REV_BIT_CONST(prob, i, 1)
              REV_BIT_CONST(prob, i, 2)
              REV_BIT_CONST(prob, i, 4)
              REV_BIT_LAST (prob, i, 8)
              distance |= i;
            }
            if (distance == (UInt32)0xFFFFFFFF)
            {
              len = kMatchSpecLenStart;
              state -= kNumStates;
              break;
            }
          }
        }
        
        rep3 = rep2;
        rep2 = rep1;
        rep1 = rep0;
        rep0 = distance + 1;
        state = (state < kNumStates + kNumLitStates) ? kNumLitStates : kNumLitStates + 3;
        if (distance >= (checkDicSize == 0 ? processedPos: checkDicSize))
        {
          len += kMatchSpecLen_Error_Data + kMatchMinLen;
          // len = kMatchSpecLen_Error_Data;
          // len += kMatchMinLen;
          break;
        }
      }

      len += kMatchMinLen;

      {
        SizeT rem;
        unsigned curLen;
        SizeT pos;
        
        if ((rem = limit - dicPos) == 0)
        {
          /*
          We stop decoding and return SZ_OK, and we can resume decoding later.
          Any error conditions can be tested later in caller code.
          For more strict mode we can stop decoding with error
          // len += kMatchSpecLen_Error_Data;
          */
          break;
        }
        
        curLen = ((rem < len) ? (unsigned)rem : len);
        pos = dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0);

        processedPos += (UInt32)curLen;

        len -= curLen;
        if (curLen <= dicBufSize - pos)
        {
          Byte *dest = dic + dicPos;
          ptrdiff_t src = (ptrdiff_t)pos - (ptrdiff_t)dicPos;
          const Byte *lim = dest + curLen;
          dicPos += (SizeT)curLen;
          do
            *(dest) = (Byte)*(dest + src);
          while (++dest != lim);
        }
        else
        {
          do
          {
            dic[dicPos++] = dic[pos];
            if (++pos == dicBufSize)
              pos = 0;
          }
          while (--curLen != 0);
        }
      }
    }
  }
  while (dicPos < limit && buf < bufLimit);

  NORMALIZE
  
  p->buf = buf;
  p->range = range;
  p->code = code;
  p->remainLen = (UInt32)len; // & (kMatchSpecLen_Error_Data - 1); // we can write real length for error matches too.
  p->dicPos = dicPos;
  p->processedPos = processedPos;
  p->reps[0] = rep0;
  p->reps[1] = rep1;
  p->reps[2] = rep2;
  p->reps[3] = rep3;
  p->state = (UInt32)state;
  if (len >= kMatchSpecLen_Error_Data)
    return SZ_ERROR_DATA;
  return SZ_OK;
}
#endif



static void Z7_FASTCALL LzmaDec_WriteRem(CLzmaDec *p, SizeT limit)
{
  unsigned len = (unsigned)p->remainLen;
  if (len == 0 /* || len >= kMatchSpecLenStart */)
    return;
  {
    SizeT dicPos = p->dicPos;
    Byte *dic;
    SizeT dicBufSize;
    SizeT rep0;   /* we use SizeT to avoid the BUG of VC14 for AMD64 */
    {
      SizeT rem = limit - dicPos;
      if (rem < len)
      {
        len = (unsigned)(rem);
        if (len == 0)
          return;
      }
    }

    if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= len)
      p->checkDicSize = p->prop.dicSize;

    p->processedPos += (UInt32)len;
    p->remainLen -= (UInt32)len;
    dic = p->dic;
    rep0 = p->reps[0];
    dicBufSize = p->dicBufSize;
    do
    {
      dic[dicPos] = dic[dicPos - rep0 + (dicPos < rep0 ? dicBufSize : 0)];
      dicPos++;
    }
    while (--len);
    p->dicPos = dicPos;
  }
}


/*
At staring of new stream we have one of the following symbols:
  - Literal        - is allowed
  - Non-Rep-Match  - is allowed only if it's end marker symbol
  - Rep-Match      - is not allowed
We use early check of (RangeCoder:Code) over kBadRepCode to simplify main decoding code
*/

#define kRange0 0xFFFFFFFF
#define kBound0 ((kRange0 >> kNumBitModelTotalBits) << (kNumBitModelTotalBits - 1))
#define kBadRepCode (kBound0 + (((kRange0 - kBound0) >> kNumBitModelTotalBits) << (kNumBitModelTotalBits - 1)))
#if kBadRepCode != (0xC0000000 - 0x400)
  #error Stop_Compiling_Bad_LZMA_Check
#endif


/*
LzmaDec_DecodeReal2():
  It calls LZMA_DECODE_REAL() and it adjusts limit according (p->checkDicSize).

We correct (p->checkDicSize) after LZMA_DECODE_REAL() and in LzmaDec_WriteRem(),
and we support the following state of (p->checkDicSize):
  if (total_processed < p->prop.dicSize) then
  {
    (total_processed == p->processedPos)
    (p->checkDicSize == 0)
  }
  else
    (p->checkDicSize == p->prop.dicSize)
*/

static int Z7_FASTCALL LzmaDec_DecodeReal2(CLzmaDec *p, SizeT limit, const Byte *bufLimit)
{
  if (p->checkDicSize == 0)
  {
    UInt32 rem = p->prop.dicSize - p->processedPos;
    if (limit - p->dicPos > rem)
      limit = p->dicPos + rem;
  }
  {
    int res = LZMA_DECODE_REAL(p, limit, bufLimit);
    if (p->checkDicSize == 0 && p->processedPos >= p->prop.dicSize)
      p->checkDicSize = p->prop.dicSize;
    return res;
  }
}



typedef enum
{
  DUMMY_INPUT_EOF, /* need more input data */
  DUMMY_LIT,
  DUMMY_MATCH,
  DUMMY_REP
} ELzmaDummy;


#define IS_DUMMY_END_MARKER_POSSIBLE(dummyRes) ((dummyRes) == DUMMY_MATCH)

static ELzmaDummy LzmaDec_TryDummy(const CLzmaDec *p, const Byte *buf, const Byte **bufOut)
{
  UInt32 range = p->range;
  UInt32 code = p->code;
  const Byte *bufLimit = *bufOut;
  const CLzmaProb *probs = GET_PROBS;
  unsigned state = (unsigned)p->state;
  ELzmaDummy res;

  for (;;)
  {
    const CLzmaProb *prob;
    UInt32 bound;
    unsigned ttt;
    unsigned posState = CALC_POS_STATE(p->processedPos, ((unsigned)1 << p->prop.pb) - 1);

    prob = probs + IsMatch + COMBINED_PS_STATE;
    IF_BIT_0_CHECK(prob)
    {
      UPDATE_0_CHECK

      prob = probs + Literal;
      if (p->checkDicSize != 0 || p->processedPos != 0)
        prob += ((UInt32)LZMA_LIT_SIZE *
            ((((p->processedPos) & (((unsigned)1 << (p->prop.lp)) - 1)) << p->prop.lc) +
            ((unsigned)p->dic[(p->dicPos == 0 ? p->dicBufSize : p->dicPos) - 1] >> (8 - p->prop.lc))));

      if (state < kNumLitStates)
      {
        unsigned symbol = 1;
        do { GET_BIT_CHECK(prob + symbol, symbol) } while (symbol < 0x100);
      }
      else
      {
        unsigned matchByte = p->dic[p->dicPos - p->reps[0] +
            (p->dicPos < p->reps[0] ? p->dicBufSize : 0)];
        unsigned offs = 0x100;
        unsigned symbol = 1;
        do
        {
          unsigned bit;
          const CLzmaProb *probLit;
          matchByte += matchByte;
          bit = offs;
          offs &= matchByte;
          probLit = prob + (offs + bit + symbol);
          GET_BIT2_CHECK(probLit, symbol, offs ^= bit; , ; )
        }
        while (symbol < 0x100);
      }
      res = DUMMY_LIT;
    }
    else
    {
      unsigned len;
      UPDATE_1_CHECK

      prob = probs + IsRep + state;
      IF_BIT_0_CHECK(prob)
      {
        UPDATE_0_CHECK
        state = 0;
        prob = probs + LenCoder;
        res = DUMMY_MATCH;
      }
      else
      {
        UPDATE_1_CHECK
        res = DUMMY_REP;
        prob = probs + IsRepG0 + state;
        IF_BIT_0_CHECK(prob)
        {
          UPDATE_0_CHECK
          prob = probs + IsRep0Long + COMBINED_PS_STATE;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK
            break;
          }
          else
          {
            UPDATE_1_CHECK
          }
        }
        else
        {
          UPDATE_1_CHECK
          prob = probs + IsRepG1 + state;
          IF_BIT_0_CHECK(prob)
          {
            UPDATE_0_CHECK
          }
          else
          {
            UPDATE_1_CHECK
            prob = probs + IsRepG2 + state;
            IF_BIT_0_CHECK(prob)
            {
              UPDATE_0_CHECK
            }
            else
            {
              UPDATE_1_CHECK
            }
          }
        }
        state = kNumStates;
        prob = probs + RepLenCoder;
      }
      {
        unsigned limit, offset;
        const CLzmaProb *probLen = prob + LenChoice;
        IF_BIT_0_CHECK(probLen)
        {
          UPDATE_0_CHECK
          probLen = prob + LenLow + GET_LEN_STATE;
          offset = 0;
          limit = 1 << kLenNumLowBits;
        }
        else
        {
          UPDATE_1_CHECK
          probLen = prob + LenChoice2;
          IF_BIT_0_CHECK(probLen)
          {
            UPDATE_0_CHECK
            probLen = prob + LenLow + GET_LEN_STATE + (1 << kLenNumLowBits);
            offset = kLenNumLowSymbols;
            limit = 1 << kLenNumLowBits;
          }
          else
          {
            UPDATE_1_CHECK
            probLen = prob + LenHigh;
            offset = kLenNumLowSymbols * 2;
            limit = 1 << kLenNumHighBits;
          }
        }
        TREE_DECODE_CHECK(probLen, limit, len)
        len += offset;
      }

      if (state < 4)
      {
        unsigned posSlot;
        prob = probs + PosSlot +
            ((len < kNumLenToPosStates - 1 ? len : kNumLenToPosStates - 1) <<
            kNumPosSlotBits);
        TREE_DECODE_CHECK(prob, 1 << kNumPosSlotBits, posSlot)
        if (posSlot >= kStartPosModelIndex)
        {
          unsigned numDirectBits = ((posSlot >> 1) - 1);

          if (posSlot < kEndPosModelIndex)
          {
            prob = probs + SpecPos + ((2 | (posSlot & 1)) << numDirectBits);
          }
          else
          {
            numDirectBits -= kNumAlignBits;
            do
            {
              NORMALIZE_CHECK
              range >>= 1;
              code -= range & (((code - range) >> 31) - 1);
              /* if (code >= range) code -= range; */
            }
            while (--numDirectBits);
            prob = probs + Align;
            numDirectBits = kNumAlignBits;
          }
          {
            unsigned i = 1;
            unsigned m = 1;
            do
            {
              REV_BIT_CHECK(prob, i, m)
            }
            while (--numDirectBits);
          }
        }
      }
    }
    break;
  }
  NORMALIZE_CHECK

  *bufOut = buf;
  return res;
}

void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState);
void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState)
{
  p->remainLen = kMatchSpecLenStart + 1;
  p->tempBufSize = 0;

  if (initDic)
  {
    p->processedPos = 0;
    p->checkDicSize = 0;
    p->remainLen = kMatchSpecLenStart + 2;
  }
  if (initState)
    p->remainLen = kMatchSpecLenStart + 2;
}

void LzmaDec_Init(CLzmaDec *p)
{
  p->dicPos = 0;
  LzmaDec_InitDicAndState(p, SZ_True, SZ_True);
}


/*
LZMA supports optional end_marker.
So the decoder can lookahead for one additional LZMA-Symbol to check end_marker.
That additional LZMA-Symbol can require up to LZMA_REQUIRED_INPUT_MAX bytes in input stream.
When the decoder reaches dicLimit, it looks (finishMode) parameter:
  if (finishMode == LZMA_FINISH_ANY), the decoder doesn't lookahead
  if (finishMode != LZMA_FINISH_ANY), the decoder lookahead, if end_marker is possible for current position

When the decoder lookahead, and the lookahead symbol is not end_marker, we have two ways:
  1) Strict mode (default) : the decoder returns SZ_ERROR_DATA.
  2) The relaxed mode (alternative mode) : we could return SZ_OK, and the caller
     must check (status) value. The caller can show the error,
     if the end of stream is expected, and the (status) is noit
     LZMA_STATUS_FINISHED_WITH_MARK or LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK.
*/


#define RETURN_NOT_FINISHED_FOR_FINISH \
  *status = LZMA_STATUS_NOT_FINISHED; \
  return SZ_ERROR_DATA; // for strict mode
  // return SZ_OK; // for relaxed mode


SRes LzmaDec_DecodeToDic(CLzmaDec *p, SizeT dicLimit, const Byte *src, SizeT *srcLen,
    ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT inSize = *srcLen;
  (*srcLen) = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;

  if (p->remainLen > kMatchSpecLenStart)
  {
    if (p->remainLen > kMatchSpecLenStart + 2)
      return p->remainLen == kMatchSpecLen_Error_Fail ? SZ_ERROR_FAIL : SZ_ERROR_DATA;

    for (; inSize > 0 && p->tempBufSize < RC_INIT_SIZE; (*srcLen)++, inSize--)
      p->tempBuf[p->tempBufSize++] = *src++;
    if (p->tempBufSize != 0 && p->tempBuf[0] != 0)
      return SZ_ERROR_DATA;
    if (p->tempBufSize < RC_INIT_SIZE)
    {
      *status = LZMA_STATUS_NEEDS_MORE_INPUT;
      return SZ_OK;
    }
    p->code =
        ((UInt32)p->tempBuf[1] << 24)
      | ((UInt32)p->tempBuf[2] << 16)
      | ((UInt32)p->tempBuf[3] << 8)
      | ((UInt32)p->tempBuf[4]);

    if (p->checkDicSize == 0
        && p->processedPos == 0
        && p->code >= kBadRepCode)
      return SZ_ERROR_DATA;

    p->range = 0xFFFFFFFF;
    p->tempBufSize = 0;

    if (p->remainLen > kMatchSpecLenStart + 1)
    {
      SizeT numProbs = LzmaProps_GetNumProbs(&p->prop);
      SizeT i;
      CLzmaProb *probs = p->probs;
      for (i = 0; i < numProbs; i++)
        probs[i] = kBitModelTotal >> 1;
      p->reps[0] = p->reps[1] = p->reps[2] = p->reps[3] = 1;
      p->state = 0;
    }

    p->remainLen = 0;
  }

  for (;;)
  {
    if (p->remainLen == kMatchSpecLenStart)
    {
      if (p->code != 0)
        return SZ_ERROR_DATA;
      *status = LZMA_STATUS_FINISHED_WITH_MARK;
      return SZ_OK;
    }

    LzmaDec_WriteRem(p, dicLimit);

    {
      // (p->remainLen == 0 || p->dicPos == dicLimit)

      int checkEndMarkNow = 0;

      if (p->dicPos >= dicLimit)
      {
        if (p->remainLen == 0 && p->code == 0)
        {
          *status = LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK;
          return SZ_OK;
        }
        if (finishMode == LZMA_FINISH_ANY)
        {
          *status = LZMA_STATUS_NOT_FINISHED;
          return SZ_OK;
        }
        if (p->remainLen != 0)
        {
          RETURN_NOT_FINISHED_FOR_FINISH
        }
        checkEndMarkNow = 1;
      }

      // (p->remainLen == 0)

      if (p->tempBufSize == 0)
      {
        const Byte *bufLimit;
        int dummyProcessed = -1;
        
        if (inSize < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          const Byte *bufOut = src + inSize;
          
          ELzmaDummy dummyRes = LzmaDec_TryDummy(p, src, &bufOut);
          
          if (dummyRes == DUMMY_INPUT_EOF)
          {
            size_t i;
            if (inSize >= LZMA_REQUIRED_INPUT_MAX)
              break;
            (*srcLen) += inSize;
            p->tempBufSize = (unsigned)inSize;
            for (i = 0; i < inSize; i++)
              p->tempBuf[i] = src[i];
            *status = LZMA_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
 
          dummyProcessed = (int)(bufOut - src);
          if ((unsigned)dummyProcessed > LZMA_REQUIRED_INPUT_MAX)
            break;
          
          if (checkEndMarkNow && !IS_DUMMY_END_MARKER_POSSIBLE(dummyRes))
          {
            unsigned i;
            (*srcLen) += (unsigned)dummyProcessed;
            p->tempBufSize = (unsigned)dummyProcessed;
            for (i = 0; i < (unsigned)dummyProcessed; i++)
              p->tempBuf[i] = src[i];
            // p->remainLen = kMatchSpecLen_Error_Data;
            RETURN_NOT_FINISHED_FOR_FINISH
          }
          
          bufLimit = src;
          // we will decode only one iteration
        }
        else
          bufLimit = src + inSize - LZMA_REQUIRED_INPUT_MAX;

        p->buf = src;
        
        {
          int res = LzmaDec_DecodeReal2(p, dicLimit, bufLimit);
          
          SizeT processed = (SizeT)(p->buf - src);

          if (dummyProcessed < 0)
          {
            if (processed > inSize)
              break;
          }
          else if ((unsigned)dummyProcessed != processed)
            break;

          src += processed;
          inSize -= processed;
          (*srcLen) += processed;

          if (res != SZ_OK)
          {
            p->remainLen = kMatchSpecLen_Error_Data;
            return SZ_ERROR_DATA;
          }
        }
        continue;
      }

      {
        // we have some data in (p->tempBuf)
        // in strict mode: tempBufSize is not enough for one Symbol decoding.
        // in relaxed mode: tempBufSize not larger than required for one Symbol decoding.

        unsigned rem = p->tempBufSize;
        unsigned ahead = 0;
        int dummyProcessed = -1;
        
        while (rem < LZMA_REQUIRED_INPUT_MAX && ahead < inSize)
          p->tempBuf[rem++] = src[ahead++];
        
        // ahead - the size of new data copied from (src) to (p->tempBuf)
        // rem   - the size of temp buffer including new data from (src)
        
        if (rem < LZMA_REQUIRED_INPUT_MAX || checkEndMarkNow)
        {
          const Byte *bufOut = p->tempBuf + rem;
        
          ELzmaDummy dummyRes = LzmaDec_TryDummy(p, p->tempBuf, &bufOut);
          
          if (dummyRes == DUMMY_INPUT_EOF)
          {
            if (rem >= LZMA_REQUIRED_INPUT_MAX)
              break;
            p->tempBufSize = rem;
            (*srcLen) += (SizeT)ahead;
            *status = LZMA_STATUS_NEEDS_MORE_INPUT;
            return SZ_OK;
          }
          
          dummyProcessed = (int)(bufOut - p->tempBuf);

          if ((unsigned)dummyProcessed < p->tempBufSize)
            break;

          if (checkEndMarkNow && !IS_DUMMY_END_MARKER_POSSIBLE(dummyRes))
          {
            (*srcLen) += (unsigned)dummyProcessed - p->tempBufSize;
            p->tempBufSize = (unsigned)dummyProcessed;
            // p->remainLen = kMatchSpecLen_Error_Data;
            RETURN_NOT_FINISHED_FOR_FINISH
          }
        }

        p->buf = p->tempBuf;
        
        {
          // we decode one symbol from (p->tempBuf) here, so the (bufLimit) is equal to (p->buf)
          int res = LzmaDec_DecodeReal2(p, dicLimit, p->buf);

          SizeT processed = (SizeT)(p->buf - p->tempBuf);
          rem = p->tempBufSize;
          
          if (dummyProcessed < 0)
          {
            if (processed > LZMA_REQUIRED_INPUT_MAX)
              break;
            if (processed < rem)
              break;
          }
          else if ((unsigned)dummyProcessed != processed)
            break;
          
          processed -= rem;

          src += processed;
          inSize -= processed;
          (*srcLen) += processed;
          p->tempBufSize = 0;
          
          if (res != SZ_OK)
          {
            p->remainLen = kMatchSpecLen_Error_Data;
            return SZ_ERROR_DATA;
          }
        }
      }
    }
  }

  /*  Some unexpected error: internal error of code, memory corruption or hardware failure */
  p->remainLen = kMatchSpecLen_Error_Fail;
  return SZ_ERROR_FAIL;
}



SRes LzmaDec_DecodeToBuf(CLzmaDec *p, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT outSize = *destLen;
  SizeT inSize = *srcLen;
  *srcLen = *destLen = 0;
  for (;;)
  {
    SizeT inSizeCur = inSize, outSizeCur, dicPos;
    ELzmaFinishMode curFinishMode;
    SRes res;
    if (p->dicPos == p->dicBufSize)
      p->dicPos = 0;
    dicPos = p->dicPos;
    if (outSize > p->dicBufSize - dicPos)
    {
      outSizeCur = p->dicBufSize;
      curFinishMode = LZMA_FINISH_ANY;
    }
    else
    {
      outSizeCur = dicPos + outSize;
      curFinishMode = finishMode;
    }

    res = LzmaDec_DecodeToDic(p, outSizeCur, src, &inSizeCur, curFinishMode, status);
    src += inSizeCur;
    inSize -= inSizeCur;
    *srcLen += inSizeCur;
    outSizeCur = p->dicPos - dicPos;
    memcpy(dest, p->dic + dicPos, outSizeCur);
    dest += outSizeCur;
    outSize -= outSizeCur;
    *destLen += outSizeCur;
    if (res != 0)
      return res;
    if (outSizeCur == 0 || outSize == 0)
      return SZ_OK;
  }
}

void LzmaDec_FreeProbs(CLzmaDec *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->probs);
  p->probs = NULL;
}

static void LzmaDec_FreeDict(CLzmaDec *p, ISzAllocPtr alloc)
{
  ISzAlloc_Free(alloc, p->dic);
  p->dic = NULL;
}

void LzmaDec_Free(CLzmaDec *p, ISzAllocPtr alloc)
{
  LzmaDec_FreeProbs(p, alloc);
  LzmaDec_FreeDict(p, alloc);
}

SRes LzmaProps_Decode(CLzmaProps *p, const Byte *data, unsigned size)
{
  UInt32 dicSize;
  Byte d;
  
  if (size < LZMA_PROPS_SIZE)
    return SZ_ERROR_UNSUPPORTED;
  else
    dicSize = data[1] | ((UInt32)data[2] << 8) | ((UInt32)data[3] << 16) | ((UInt32)data[4] << 24);
 
  if (dicSize < LZMA_DIC_MIN)
    dicSize = LZMA_DIC_MIN;
  p->dicSize = dicSize;

  d = data[0];
  if (d >= (9 * 5 * 5))
    return SZ_ERROR_UNSUPPORTED;

  p->lc = (Byte)(d % 9);
  d /= 9;
  p->pb = (Byte)(d / 5);
  p->lp = (Byte)(d % 5);

  return SZ_OK;
}

static SRes LzmaDec_AllocateProbs2(CLzmaDec *p, const CLzmaProps *propNew, ISzAllocPtr alloc)
{
  UInt32 numProbs = LzmaProps_GetNumProbs(propNew);
  if (!p->probs || numProbs != p->numProbs)
  {
    LzmaDec_FreeProbs(p, alloc);
    p->probs = (CLzmaProb *)ISzAlloc_Alloc(alloc, numProbs * sizeof(CLzmaProb));
    if (!p->probs)
      return SZ_ERROR_MEM;
    p->probs_1664 = p->probs + 1664;
    p->numProbs = numProbs;
  }
  return SZ_OK;
}

SRes LzmaDec_AllocateProbs(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc)
{
  CLzmaProps propNew;
  RINOK(LzmaProps_Decode(&propNew, props, propsSize))
  RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc))
  p->prop = propNew;
  return SZ_OK;
}

SRes LzmaDec_Allocate(CLzmaDec *p, const Byte *props, unsigned propsSize, ISzAllocPtr alloc)
{
  CLzmaProps propNew;
  SizeT dicBufSize;
  RINOK(LzmaProps_Decode(&propNew, props, propsSize))
  RINOK(LzmaDec_AllocateProbs2(p, &propNew, alloc))

  {
    UInt32 dictSize = propNew.dicSize;
    SizeT mask = ((UInt32)1 << 12) - 1;
         if (dictSize >= ((UInt32)1 << 30)) mask = ((UInt32)1 << 22) - 1;
    else if (dictSize >= ((UInt32)1 << 22)) mask = ((UInt32)1 << 20) - 1;
    dicBufSize = ((SizeT)dictSize + mask) & ~mask;
    if (dicBufSize < dictSize)
      dicBufSize = dictSize;
  }

  if (!p->dic || dicBufSize != p->dicBufSize)
  {
    LzmaDec_FreeDict(p, alloc);
    p->dic = (Byte *)ISzAlloc_Alloc(alloc, dicBufSize);
    if (!p->dic)
    {
      LzmaDec_FreeProbs(p, alloc);
      return SZ_ERROR_MEM;
    }
  }
  p->dicBufSize = dicBufSize;
  p->prop = propNew;
  return SZ_OK;
}

SRes LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode,
    ELzmaStatus *status, ISzAllocPtr alloc)
{
  CLzmaDec p;
  SRes res;
  SizeT outSize = *destLen, inSize = *srcLen;
  *destLen = *srcLen = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;
  if (inSize < RC_INIT_SIZE)
    return SZ_ERROR_INPUT_EOF;
  LzmaDec_CONSTRUCT(&p)
  RINOK(LzmaDec_AllocateProbs(&p, propData, propSize, alloc))
  p.dic = dest;
  p.dicBufSize = outSize;
  LzmaDec_Init(&p);
  *srcLen = inSize;
  res = LzmaDec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
  *destLen = p.dicPos;
  if (res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
    res = SZ_ERROR_INPUT_EOF;
  LzmaDec_FreeProbs(&p, alloc);
  return res;
}
#undef LzmaDecode
#undef LzmaDec_Allocate
#undef LzmaDec_AllocateProbs
#undef LzmaProps_Decode
#undef LzmaDec_Free
#undef LzmaDec_FreeProbs
#undef LzmaDec_DecodeToBuf
#undef LzmaDec_DecodeToDic
#undef LzmaDec_Init
#undef LzmaDec_InitDicAndState

#define LzmaDec_InitDicAndState X_LzmaDec_InitDicAndState
#define LzmaDec_Init X_LzmaDec_Init
#define LzmaDec_DecodeToDic X_LzmaDec_DecodeToDic
#define LzmaDec_AllocateProbs X_LzmaDec_AllocateProbs
#define LzmaDec_Allocate X_LzmaDec_Allocate
#define LzmaDec_FreeProbs X_LzmaDec_FreeProbs
#define Lzma2Dec_AllocateProbs X_Lzma2Dec_AllocateProbs
#define Lzma2Dec_Allocate X_Lzma2Dec_Allocate
#define Lzma2Dec_Init X_Lzma2Dec_Init
#define Lzma2Dec_DecodeToDic X_Lzma2Dec_DecodeToDic
#define Lzma2Dec_Parse X_Lzma2Dec_Parse
#define Lzma2Dec_DecodeToBuf X_Lzma2Dec_DecodeToBuf
#define Lzma2Decode X_Lzma2Decode
/* Lzma2Dec.c -- LZMA2 Decoder
2024-03-01 : Igor Pavlov : Public domain */

/* #define SHOW_DEBUG_INFO */
#ifdef SHOW_DEBUG_INFO
#include <stdio.h>
#endif

#include <string.h>
/*
00000000  -  End of data
00000001 U U  -  Uncompressed, reset dic, need reset state and set new prop
00000010 U U  -  Uncompressed, no reset
100uuuuu U U P P  -  LZMA, no reset
101uuuuu U U P P  -  LZMA, reset state
110uuuuu U U P P S  -  LZMA, reset state + set new prop
111uuuuu U U P P S  -  LZMA, reset state + set new prop, reset dic

  u, U - Unpack Size
  P - Pack Size
  S - Props
*/

#define LZMA2_CONTROL_COPY_RESET_DIC 1

#define LZMA2_IS_UNCOMPRESSED_STATE(p) (((p)->control & (1 << 7)) == 0)

#define LZMA2_LCLP_MAX 4
#define LZMA2_DIC_SIZE_FROM_PROP(p) (((UInt32)2 | ((p) & 1)) << ((p) / 2 + 11))

#ifdef SHOW_DEBUG_INFO
#define PRF(x) x
#else
#define PRF(x)
#endif

typedef enum
{
  LZMA2_STATE_CONTROL,
  LZMA2_STATE_UNPACK0,
  LZMA2_STATE_UNPACK1,
  LZMA2_STATE_PACK0,
  LZMA2_STATE_PACK1,
  LZMA2_STATE_PROP,
  LZMA2_STATE_DATA,
  LZMA2_STATE_DATA_CONT,
  LZMA2_STATE_FINISHED,
  LZMA2_STATE_ERROR
} ELzma2State;

static SRes Lzma2Dec_GetOldProps(Byte prop, Byte *props)
{
  UInt32 dicSize;
  if (prop > 40)
    return SZ_ERROR_UNSUPPORTED;
  dicSize = (prop == 40) ? 0xFFFFFFFF : LZMA2_DIC_SIZE_FROM_PROP(prop);
  props[0] = (Byte)LZMA2_LCLP_MAX;
  props[1] = (Byte)(dicSize);
  props[2] = (Byte)(dicSize >> 8);
  props[3] = (Byte)(dicSize >> 16);
  props[4] = (Byte)(dicSize >> 24);
  return SZ_OK;
}

SRes Lzma2Dec_AllocateProbs(CLzma2Dec *p, Byte prop, ISzAllocPtr alloc)
{
  Byte props[LZMA_PROPS_SIZE];
  RINOK(Lzma2Dec_GetOldProps(prop, props))
  return LzmaDec_AllocateProbs(&p->decoder, props, LZMA_PROPS_SIZE, alloc);
}

SRes Lzma2Dec_Allocate(CLzma2Dec *p, Byte prop, ISzAllocPtr alloc)
{
  Byte props[LZMA_PROPS_SIZE];
  RINOK(Lzma2Dec_GetOldProps(prop, props))
  return LzmaDec_Allocate(&p->decoder, props, LZMA_PROPS_SIZE, alloc);
}

void Lzma2Dec_Init(CLzma2Dec *p)
{
  p->state = LZMA2_STATE_CONTROL;
  p->needInitLevel = 0xE0;
  p->isExtraMode = SZ_False;
  p->unpackSize = 0;
  
  // p->decoder.dicPos = 0; // we can use it instead of full init
  LzmaDec_Init(&p->decoder);
}

// ELzma2State
static unsigned Lzma2Dec_UpdateState(CLzma2Dec *p, Byte b)
{
  switch (p->state)
  {
    case LZMA2_STATE_CONTROL:
      p->isExtraMode = SZ_False;
      p->control = b;
      PRF(printf("\n %8X", (unsigned)p->decoder.dicPos));
      PRF(printf(" %02X", (unsigned)b));
      if (b == 0)
        return LZMA2_STATE_FINISHED;
      if (LZMA2_IS_UNCOMPRESSED_STATE(p))
      {
        if (b == LZMA2_CONTROL_COPY_RESET_DIC)
          p->needInitLevel = 0xC0;
        else if (b > 2 || p->needInitLevel == 0xE0)
          return LZMA2_STATE_ERROR;
      }
      else
      {
        if (b < p->needInitLevel)
          return LZMA2_STATE_ERROR;
        p->needInitLevel = 0;
        p->unpackSize = (UInt32)(b & 0x1F) << 16;
      }
      return LZMA2_STATE_UNPACK0;
    
    case LZMA2_STATE_UNPACK0:
      p->unpackSize |= (UInt32)b << 8;
      return LZMA2_STATE_UNPACK1;
    
    case LZMA2_STATE_UNPACK1:
      p->unpackSize |= (UInt32)b;
      p->unpackSize++;
      PRF(printf(" %7u", (unsigned)p->unpackSize));
      return LZMA2_IS_UNCOMPRESSED_STATE(p) ? LZMA2_STATE_DATA : LZMA2_STATE_PACK0;
    
    case LZMA2_STATE_PACK0:
      p->packSize = (UInt32)b << 8;
      return LZMA2_STATE_PACK1;

    case LZMA2_STATE_PACK1:
      p->packSize |= (UInt32)b;
      p->packSize++;
      // if (p->packSize < 5) return LZMA2_STATE_ERROR;
      PRF(printf(" %5u", (unsigned)p->packSize));
      return (p->control & 0x40) ? LZMA2_STATE_PROP : LZMA2_STATE_DATA;

    case LZMA2_STATE_PROP:
    {
      unsigned lc, lp;
      if (b >= (9 * 5 * 5))
        return LZMA2_STATE_ERROR;
      lc = b % 9;
      b /= 9;
      p->decoder.prop.pb = (Byte)(b / 5);
      lp = b % 5;
      if (lc + lp > LZMA2_LCLP_MAX)
        return LZMA2_STATE_ERROR;
      p->decoder.prop.lc = (Byte)lc;
      p->decoder.prop.lp = (Byte)lp;
      return LZMA2_STATE_DATA;
    }
    
    default:
      return LZMA2_STATE_ERROR;
  }
}

static void LzmaDec_UpdateWithUncompressed(CLzmaDec *p, const Byte *src, SizeT size)
{
  memcpy(p->dic + p->dicPos, src, size);
  p->dicPos += size;
  if (p->checkDicSize == 0 && p->prop.dicSize - p->processedPos <= size)
    p->checkDicSize = p->prop.dicSize;
  p->processedPos += (UInt32)size;
}

void LzmaDec_InitDicAndState(CLzmaDec *p, BoolInt initDic, BoolInt initState);


SRes Lzma2Dec_DecodeToDic(CLzma2Dec *p, SizeT dicLimit,
    const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT inSize = *srcLen;
  *srcLen = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;

  while (p->state != LZMA2_STATE_ERROR)
  {
    SizeT dicPos;

    if (p->state == LZMA2_STATE_FINISHED)
    {
      *status = LZMA_STATUS_FINISHED_WITH_MARK;
      return SZ_OK;
    }
    
    dicPos = p->decoder.dicPos;
    
    if (dicPos == dicLimit && finishMode == LZMA_FINISH_ANY)
    {
      *status = LZMA_STATUS_NOT_FINISHED;
      return SZ_OK;
    }

    if (p->state != LZMA2_STATE_DATA && p->state != LZMA2_STATE_DATA_CONT)
    {
      if (*srcLen == inSize)
      {
        *status = LZMA_STATUS_NEEDS_MORE_INPUT;
        return SZ_OK;
      }
      (*srcLen)++;
      p->state = Lzma2Dec_UpdateState(p, *src++);
      if (dicPos == dicLimit && p->state != LZMA2_STATE_FINISHED)
        break;
      continue;
    }
    
    {
      SizeT inCur = inSize - *srcLen;
      SizeT outCur = dicLimit - dicPos;
      ELzmaFinishMode curFinishMode = LZMA_FINISH_ANY;
      
      if (outCur >= p->unpackSize)
      {
        outCur = (SizeT)p->unpackSize;
        curFinishMode = LZMA_FINISH_END;
      }

      if (LZMA2_IS_UNCOMPRESSED_STATE(p))
      {
        if (inCur == 0)
        {
          *status = LZMA_STATUS_NEEDS_MORE_INPUT;
          return SZ_OK;
        }

        if (p->state == LZMA2_STATE_DATA)
        {
          BoolInt initDic = (p->control == LZMA2_CONTROL_COPY_RESET_DIC);
          LzmaDec_InitDicAndState(&p->decoder, initDic, SZ_False);
        }

        if (inCur > outCur)
          inCur = outCur;
        if (inCur == 0)
          break;

        LzmaDec_UpdateWithUncompressed(&p->decoder, src, inCur);

        src += inCur;
        *srcLen += inCur;
        p->unpackSize -= (UInt32)inCur;
        p->state = (p->unpackSize == 0) ? LZMA2_STATE_CONTROL : LZMA2_STATE_DATA_CONT;
      }
      else
      {
        SRes res;

        if (p->state == LZMA2_STATE_DATA)
        {
          BoolInt initDic = (p->control >= 0xE0);
          BoolInt initState = (p->control >= 0xA0);
          LzmaDec_InitDicAndState(&p->decoder, initDic, initState);
          p->state = LZMA2_STATE_DATA_CONT;
        }
  
        if (inCur > p->packSize)
          inCur = (SizeT)p->packSize;
        
        res = LzmaDec_DecodeToDic(&p->decoder, dicPos + outCur, src, &inCur, curFinishMode, status);

        src += inCur;
        *srcLen += inCur;
        p->packSize -= (UInt32)inCur;
        outCur = p->decoder.dicPos - dicPos;
        p->unpackSize -= (UInt32)outCur;

        if (res != 0)
          break;
        
        if (*status == LZMA_STATUS_NEEDS_MORE_INPUT)
        {
          if (p->packSize == 0)
            break;
          return SZ_OK;
        }

        if (inCur == 0 && outCur == 0)
        {
          if (*status != LZMA_STATUS_MAYBE_FINISHED_WITHOUT_MARK
              || p->unpackSize != 0
              || p->packSize != 0)
            break;
          p->state = LZMA2_STATE_CONTROL;
        }
        
        *status = LZMA_STATUS_NOT_SPECIFIED;
      }
    }
  }
  
  *status = LZMA_STATUS_NOT_SPECIFIED;
  p->state = LZMA2_STATE_ERROR;
  return SZ_ERROR_DATA;
}




ELzma2ParseStatus Lzma2Dec_Parse(CLzma2Dec *p,
    SizeT outSize,
    const Byte *src, SizeT *srcLen,
    int checkFinishBlock)
{
  SizeT inSize = *srcLen;
  *srcLen = 0;

  while (p->state != LZMA2_STATE_ERROR)
  {
    if (p->state == LZMA2_STATE_FINISHED)
      return (ELzma2ParseStatus)LZMA_STATUS_FINISHED_WITH_MARK;

    if (outSize == 0 && !checkFinishBlock)
      return (ELzma2ParseStatus)LZMA_STATUS_NOT_FINISHED;
    
    if (p->state != LZMA2_STATE_DATA && p->state != LZMA2_STATE_DATA_CONT)
    {
      if (*srcLen == inSize)
        return (ELzma2ParseStatus)LZMA_STATUS_NEEDS_MORE_INPUT;
      (*srcLen)++;

      p->state = Lzma2Dec_UpdateState(p, *src++);

      if (p->state == LZMA2_STATE_UNPACK0)
      {
        // if (p->decoder.dicPos != 0)
        if (p->control == LZMA2_CONTROL_COPY_RESET_DIC || p->control >= 0xE0)
          return LZMA2_PARSE_STATUS_NEW_BLOCK;
        // if (outSize == 0) return LZMA_STATUS_NOT_FINISHED;
      }

      // The following code can be commented.
      // It's not big problem, if we read additional input bytes.
      // It will be stopped later in LZMA2_STATE_DATA / LZMA2_STATE_DATA_CONT state.

      if (outSize == 0 && p->state != LZMA2_STATE_FINISHED)
      {
        // checkFinishBlock is true. So we expect that block must be finished,
        // We can return LZMA_STATUS_NOT_SPECIFIED or LZMA_STATUS_NOT_FINISHED here
        // break;
        return (ELzma2ParseStatus)LZMA_STATUS_NOT_FINISHED;
      }

      if (p->state == LZMA2_STATE_DATA)
        return LZMA2_PARSE_STATUS_NEW_CHUNK;

      continue;
    }

    if (outSize == 0)
      return (ELzma2ParseStatus)LZMA_STATUS_NOT_FINISHED;

    {
      SizeT inCur = inSize - *srcLen;

      if (LZMA2_IS_UNCOMPRESSED_STATE(p))
      {
        if (inCur == 0)
          return (ELzma2ParseStatus)LZMA_STATUS_NEEDS_MORE_INPUT;
        if (inCur > p->unpackSize)
          inCur = p->unpackSize;
        if (inCur > outSize)
          inCur = outSize;
        p->decoder.dicPos += inCur;
        src += inCur;
        *srcLen += inCur;
        outSize -= inCur;
        p->unpackSize -= (UInt32)inCur;
        p->state = (p->unpackSize == 0) ? LZMA2_STATE_CONTROL : LZMA2_STATE_DATA_CONT;
      }
      else
      {
        p->isExtraMode = SZ_True;

        if (inCur == 0)
        {
          if (p->packSize != 0)
            return (ELzma2ParseStatus)LZMA_STATUS_NEEDS_MORE_INPUT;
        }
        else if (p->state == LZMA2_STATE_DATA)
        {
          p->state = LZMA2_STATE_DATA_CONT;
          if (*src != 0)
          {
            // first byte of lzma chunk must be Zero
            *srcLen += 1;
            p->packSize--;
            break;
          }
        }
  
        if (inCur > p->packSize)
          inCur = (SizeT)p->packSize;

        src += inCur;
        *srcLen += inCur;
        p->packSize -= (UInt32)inCur;

        if (p->packSize == 0)
        {
          SizeT rem = outSize;
          if (rem > p->unpackSize)
            rem = p->unpackSize;
          p->decoder.dicPos += rem;
          p->unpackSize -= (UInt32)rem;
          outSize -= rem;
          if (p->unpackSize == 0)
            p->state = LZMA2_STATE_CONTROL;
        }
      }
    }
  }
  
  p->state = LZMA2_STATE_ERROR;
  return (ELzma2ParseStatus)LZMA_STATUS_NOT_SPECIFIED;
}




SRes Lzma2Dec_DecodeToBuf(CLzma2Dec *p, Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, ELzmaFinishMode finishMode, ELzmaStatus *status)
{
  SizeT outSize = *destLen, inSize = *srcLen;
  *srcLen = *destLen = 0;
  
  for (;;)
  {
    SizeT inCur = inSize, outCur, dicPos;
    ELzmaFinishMode curFinishMode;
    SRes res;
    
    if (p->decoder.dicPos == p->decoder.dicBufSize)
      p->decoder.dicPos = 0;
    dicPos = p->decoder.dicPos;
    curFinishMode = LZMA_FINISH_ANY;
    outCur = p->decoder.dicBufSize - dicPos;
    
    if (outCur >= outSize)
    {
      outCur = outSize;
      curFinishMode = finishMode;
    }

    res = Lzma2Dec_DecodeToDic(p, dicPos + outCur, src, &inCur, curFinishMode, status);
    
    src += inCur;
    inSize -= inCur;
    *srcLen += inCur;
    outCur = p->decoder.dicPos - dicPos;
    memcpy(dest, p->decoder.dic + dicPos, outCur);
    dest += outCur;
    outSize -= outCur;
    *destLen += outCur;
    if (res != 0)
      return res;
    if (outCur == 0 || outSize == 0)
      return SZ_OK;
  }
}


SRes Lzma2Decode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen,
    Byte prop, ELzmaFinishMode finishMode, ELzmaStatus *status, ISzAllocPtr alloc)
{
  CLzma2Dec p;
  SRes res;
  SizeT outSize = *destLen, inSize = *srcLen;
  *destLen = *srcLen = 0;
  *status = LZMA_STATUS_NOT_SPECIFIED;
  Lzma2Dec_CONSTRUCT(&p)
  RINOK(Lzma2Dec_AllocateProbs(&p, prop, alloc))
  p.decoder.dic = dest;
  p.decoder.dicBufSize = outSize;
  Lzma2Dec_Init(&p);
  *srcLen = inSize;
  res = Lzma2Dec_DecodeToDic(&p, outSize, src, srcLen, finishMode, status);
  *destLen = p.decoder.dicPos;
  if (res == SZ_OK && *status == LZMA_STATUS_NEEDS_MORE_INPUT)
    res = SZ_ERROR_INPUT_EOF;
  Lzma2Dec_FreeProbs(&p, alloc);
  return res;
}

#undef PRF
#undef Lzma2Decode
#undef Lzma2Dec_DecodeToBuf
#undef Lzma2Dec_Parse
#undef Lzma2Dec_DecodeToDic
#undef Lzma2Dec_Init
#undef Lzma2Dec_Allocate
#undef Lzma2Dec_AllocateProbs
#undef LzmaDec_Allocate
#undef LzmaDec_AllocateProbs
#undef LzmaDec_FreeProbs
#undef LzmaDec_DecodeToDic
#undef LzmaDec_Init
#undef LzmaDec_InitDicAndState
/* ===== End embedded xlzma_local.c ===== */
