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
#include "xwinzipjpegdecoder.h"

#include "xalgo_local.h"

#include <cstdlib>
#include <cstring>
#include <new>

// ZIP method 96: WinZip JPEG recompression, implemented from the official
// specification "JPEG Compression - Method 96" (WinZip Computing, 2008) and
// the log-domain binary arithmetic coder of the expired U.S. patent 4,791,403
// that the specification incorporates by reference (section 6).
//
// Known corrections to the published formulas, byte-verified against real
// WinZip-produced streams (each marked SPEC-ERRATUM below at the site):
//  - AVG (5.6.2.2) sums Bw[x], not Bw[k], and the DC coefficient never
//    participates as an x.
//  - BDR (5.6.2.3) and DC prediction (5.6.7.1) use (Bn[x] + Bc[x]) /
//    (Bw[x] + Bc[x]); the printed minus sign is wrong.
//  - Prediction refinement (5.6.7.2) sums abs(Bn[x] - Bc[x]) without the
//    inner absolute values.
//  - DC sign contexts (5.6.7.3.2) compare the neighbour DC values against the
//    predicted DC, not against zero.
//  - Binarization (5.6.4): the unary magnitude bins are indexed by the count
//    of preceding one bits capped at (cap - 1), and the remainder bits are
//    coded most-significant first with one bin per bit position.

SRes X_LzmaDecode(Byte *dest, SizeT *destLen, const Byte *src, SizeT *srcLen, const Byte *propData, unsigned propSize, ELzmaFinishMode finishMode, ELzmaStatus *status,
                  ISzAllocPtr alloc);

namespace {

const qint64 WZJPEG_MAX_METADATA_SIZE = 16 * 1024 * 1024;  // spec 4.1.1
const qint64 WZJPEG_MAX_SLICE_BUFFER_BYTES = 512 * 1024 * 1024;
const qint32 WZJPEG_OUTPUT_FLUSH_SIZE = 0x40000;

#include "xwinzipjpegdecoder_tables.inc"

// Probability table of the arithmetic coder, reproduced in section 6 of the
// official specification (parameters kavg = 5, kmax = 11; index 48 is the
// appended fixed-statistics entry).
static const quint16 g_wzjpegLogP[49] = {
    1024, 895, 795, 706, 628, 559, 493, 437, 379, 331, 287, 247, 212, 186, 158, 143, 127, 110, 98, 84, 72, 65, 59, 53,   48,
    45,   42,  40,  37,  35,  33,  30,  28,  26,  23,  21,  19,  17,  15,  13,  11,  9,   7,   5,  4,  3,  2,  1,  1024,
};

static const quint16 g_wzjpegLogQP[49] = {
    0,    272,  502,  726,  941,  1150, 1371, 1578, 1819, 2044, 2278, 2521, 2765, 2971, 3227, 3382, 3566, 3788, 3965, 4200, 4435, 4590,  4737,  4899, 5050,
    5147, 5250, 5325, 5441, 5527, 5617, 5758, 5863, 5976, 6157, 6295, 6447, 6616, 6806, 7024, 7278, 7585, 7972, 8495, 8884, 9309, 10065, 11689, 0,
};

static const quint16 g_wzjpegNMaxLP[49] = {
    16384, 16110, 15105, 14826, 14444, 13975, 13804, 13547, 13265, 13240, 12915, 12844, 12720, 12648, 12482, 12441, 12319,
    12320, 12250, 12180, 12168, 12155, 12154, 12084, 12096, 12105, 12096, 12080, 12062, 12075, 12078, 12060, 12068, 12090,
    12075, 12075, 12103, 12121, 12150, 12181, 12221, 12294, 12411, 12615, 13120, 13113, 14574, 21860, 0,
};

static const quint8 g_wzjpegHalfI[49] = {
    8, 8, 7, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8, 9, 10, 10, 10, 10, 10, 10, 9, 9, 8, 8, 7, 7, 6, 6, 6, 5, 5, 4, 4, 3, 3, 3, 3, 2, 2, 1, 0, 0,
};

static const quint8 g_wzjpegDblI[49] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 7, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 8, 8, 9, 9, 10, 10, 10, 10, 10, 10, 9, 8, 7, 6, 6, 5, 4, 3, 3, 3, 2, 1, 0,
};

// Standard JPEG zigzag facts.
static const quint8 g_wzjpegZigZag[8][8] = {
    {0, 1, 5, 6, 14, 15, 27, 28},     {2, 4, 7, 13, 16, 26, 29, 42},    {3, 8, 12, 17, 25, 30, 41, 43},   {9, 11, 18, 24, 31, 40, 44, 53},
    {10, 19, 23, 32, 39, 45, 52, 54}, {20, 22, 33, 38, 46, 51, 55, 60}, {21, 34, 37, 47, 50, 56, 59, 61}, {35, 36, 48, 49, 57, 58, 62, 63},
};

static const quint8 g_wzjpegRow[64] = {
    0, 0, 1, 2, 1, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3,
    4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 4, 5, 6, 7, 7, 6, 5, 6, 7, 7,
};

static const quint8 g_wzjpegColumn[64] = {
    0, 1, 0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4,
    3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 5, 6, 7, 7, 6, 7,
};

static void *wzjpegLzmaAlloc(ISzAllocPtr p, size_t size)
{
    Q_UNUSED(p)
    return malloc(size);
}

static void wzjpegLzmaFree(ISzAllocPtr p, void *address)
{
    Q_UNUSED(p)
    free(address);
}

static const ISzAlloc g_wzjpegLzmaAllocator = {wzjpegLzmaAlloc, wzjpegLzmaFree};

// Bounded sequential reader over the record's compressed extent.
struct WZJPEG_INPUT {
    QIODevice *pDevice;
    qint64 nNextOffset;  // next device offset to read from
    qint64 nRemaining;   // bytes left in the extent; -1 = unbounded
    qint64 nCountInput;  // bytes consumed
    bool bReadError;     // device-level failure (not clean end of data)
    quint8 baBuffer[0x10000];
    qint32 nBufferSize;
    qint32 nBufferPos;
};

static void wzjpegInputInit(WZJPEG_INPUT *pInput, QIODevice *pDevice, qint64 nOffset, qint64 nLimit)
{
    pInput->pDevice = pDevice;
    pInput->nNextOffset = nOffset;
    pInput->nRemaining = nLimit;
    pInput->nCountInput = 0;
    pInput->bReadError = false;
    pInput->nBufferSize = 0;
    pInput->nBufferPos = 0;
}

static bool wzjpegInputFill(WZJPEG_INPUT *pInput)
{
    if (pInput->nBufferPos < pInput->nBufferSize) return true;
    if (pInput->bReadError) return false;
    if (pInput->nRemaining == 0) return false;

    qint64 nRequest = (qint64)sizeof(pInput->baBuffer);
    if ((pInput->nRemaining != -1) && (nRequest > pInput->nRemaining)) nRequest = pInput->nRemaining;
    if (nRequest <= 0) return false;

    if (!pInput->pDevice->seek(pInput->nNextOffset)) {
        pInput->bReadError = true;
        return false;
    }
    const qint64 nRead = pInput->pDevice->read((char *)pInput->baBuffer, nRequest);
    if (nRead < 0) {
        pInput->bReadError = true;
        return false;
    }
    if (nRead == 0) {
        if (pInput->nRemaining != -1) pInput->bReadError = true;  // extent promised more
        return false;
    }
    pInput->nNextOffset += nRead;
    if (pInput->nRemaining != -1) pInput->nRemaining -= nRead;
    pInput->nBufferSize = (qint32)nRead;
    pInput->nBufferPos = 0;
    return true;
}

// Returns -1 at end of data (or on error; check bReadError).
static qint32 wzjpegInputReadByte(WZJPEG_INPUT *pInput)
{
    if (!wzjpegInputFill(pInput)) return -1;
    pInput->nCountInput++;
    return pInput->baBuffer[pInput->nBufferPos++];
}

static bool wzjpegInputReadFull(WZJPEG_INPUT *pInput, quint8 *pBuffer, qint64 nSize)
{
    for (qint64 i = 0; i < nSize; i++) {
        const qint32 nByte = wzjpegInputReadByte(pInput);
        if (nByte < 0) return false;
        pBuffer[i] = (quint8)nByte;
    }
    return true;
}

static bool wzjpegInputSkip(WZJPEG_INPUT *pInput, qint64 nSize)
{
    for (qint64 i = 0; i < nSize; i++) {
        if (wzjpegInputReadByte(pInput) < 0) return false;
    }
    return true;
}

// One adaptive probability bin of the arithmetic coder.
struct WZJPEG_BIN {
    qint32 nIndex;  // index into the probability table
    qint32 nDLRM;   // distance from lr to the next adaptation point
    quint8 nMPS;    // current most-probable-symbol value
    quint8 nK;      // LPS occurrence count
};

static void wzjpegBinInit(WZJPEG_BIN *pBin)
{
    pBin->nIndex = 0;
    pBin->nDLRM = g_wzjpegNMaxLP[0];
    pBin->nMPS = 0;
    pBin->nK = 0;
}

static void wzjpegBinInitFixed(WZJPEG_BIN *pBin)
{
    pBin->nIndex = 48;  // appended fixed-statistics entry
    pBin->nDLRM = g_wzjpegNMaxLP[0];
    pBin->nMPS = 0;
    pBin->nK = 0;
}

// Log-domain binary arithmetic decoder (U.S. patent 4,791,403 with the two
// WinZip modifications to QSMALLER/QBIGGER shown in the specification).
struct WZJPEG_BAC {
    WZJPEG_INPUT *pInput;
    bool bEndOfData;
    quint8 nCurrentByte;
    quint8 nLastByte;
    quint32 nX;   // finite-precision window on the code stream
    qint32 nLR;   // minus log2 of the range, 10 fraction bits
    qint32 nLRM;  // lr bound before the next adaptation check
    qint32 nLX;   // minus log2 of x
};

static quint8 wzjpegBacByteIn(WZJPEG_BAC *pBac)
{
    pBac->nLastByte = pBac->nCurrentByte;
    const qint32 nByte = wzjpegInputReadByte(pBac->pInput);
    if (nByte < 0) {
        pBac->bEndOfData = true;
        pBac->nCurrentByte = 0;
    } else {
        pBac->nCurrentByte = (quint8)nByte;
    }
    return pBac->nCurrentByte;
}

static qint32 wzjpegBacLogX(quint32 nX)
{
    const quint32 nHighBits = nX >> 12;
    if (nHighBits == 0) return 0x2000;

    qint32 nWhole = 0;
    if (nHighBits < 512) {
        // characteristic: 8 - floor(log2(highbits))
        quint32 nValue = nHighBits;
        qint32 nLog = 0;
        while (nValue > 1) {
            nValue >>= 1;
            nLog++;
        }
        nWhole = 8 - nLog;
    }

    const qint32 nShift = 8 - nWhole;
    qint32 nNegFraction = 0;
    if (nShift >= 0) nNegFraction = g_wzjpegLogTable[(nX >> nShift) & 0xfff];
    else nNegFraction = g_wzjpegLogTable[(nX << (-nShift)) & 0xfff];

    return (nWhole << 10) - nNegFraction;
}

static quint32 wzjpegBacAntilogX(qint32 nLR)
{
    const qint32 nWhole = nLR >> 10;
    const quint32 nFraction = (quint32)(nLR & 0x3ff);
    const qint32 nShift = 7 - nWhole;
    if (nShift >= 0) return (quint32)g_wzjpegAntilogTable[nFraction] << nShift;
    return (quint32)g_wzjpegAntilogTable[nFraction] >> (-nShift);
}

static void wzjpegBacRenorm(WZJPEG_BAC *pBac)
{
    while (pBac->nLR > 0x1fff) {
        if ((pBac->nCurrentByte == 0xff) && (pBac->nLastByte == 0xff)) {
            pBac->nX += wzjpegBacByteIn(pBac);  // stuffed carry byte
        }
        pBac->nX = (pBac->nX << 8) | wzjpegBacByteIn(pBac);
        pBac->nLR -= 0x2000;
        pBac->nLRM -= 0x2000;
    }
    pBac->nLX = wzjpegBacLogX(pBac->nX);
}

static void wzjpegBacLRMBig(WZJPEG_BAC *pBac)
{
    if (pBac->nLRM > 0x7ff) wzjpegBacRenorm(pBac);
}

static void wzjpegBacInit(WZJPEG_BAC *pBac, WZJPEG_INPUT *pInput)
{
    pBac->pInput = pInput;
    pBac->bEndOfData = false;
    pBac->nCurrentByte = 0;
    pBac->nLastByte = 0;

    const quint8 nByte1 = wzjpegBacByteIn(pBac);
    const quint8 nByte2 = wzjpegBacByteIn(pBac);
    pBac->nX = ((quint32)nByte1 << 8) | nByte2;

    pBac->nLR = 0x1001;
    pBac->nLRM = pBac->nLR;
    pBac->nLX = wzjpegBacLogX(pBac->nX);

    if (pBac->nX == 0xffff) wzjpegBacByteIn(pBac);  // skip stuffed byte
}

static void wzjpegBacFlush(WZJPEG_BAC *pBac)
{
    wzjpegBacRenorm(pBac);
    if ((pBac->nCurrentByte == 0xff) && (pBac->nLastByte == 0xff)) wzjpegBacByteIn(pBac);
}

// QSMALLER with the WinZip modification: the index saturates at 47 instead of
// backing up from the sentinel.
static void wzjpegBacQSmaller(WZJPEG_BIN *pBin)
{
    if (pBin->nIndex >= 47) return;
    pBin->nIndex++;
    if (pBin->nK <= 1) {  // kmin1
        pBin->nIndex += g_wzjpegHalfI[pBin->nIndex];
        if (pBin->nK <= 0) {  // kmin2
            pBin->nIndex += g_wzjpegHalfI[pBin->nIndex];
        }
    }
}

static void wzjpegBacUpdateMPS(WZJPEG_BAC *pBac, WZJPEG_BIN *pBin)
{
    if (pBin->nK <= 5) wzjpegBacQSmaller(pBin);  // kmin
    pBin->nK = 0;
    pBac->nLRM = pBac->nLR + g_wzjpegNMaxLP[pBin->nIndex];
    wzjpegBacLRMBig(pBac);
}

static void wzjpegBacIncrIndex(qint32 *pnIndex, qint32 *pnIncrSaved)
{
    if (*pnIndex > 0) (*pnIndex)--;
    else (*pnIncrSaved)++;
}

static void wzjpegBacDblIndex(qint32 *pnIndex, qint32 *pnIncrSaved)
{
    if (*pnIndex > 0) *pnIndex -= g_wzjpegDblI[*pnIndex];
    else *pnIncrSaved += g_wzjpegDblI[*pnIndex];
}

// QBIGGER with the WinZip modification: the fixed-statistics entry (index 48)
// never adapts.
static void wzjpegBacQBigger(WZJPEG_BAC *pBac, WZJPEG_BIN *pBin)
{
    if (pBin->nIndex >= 48) return;

    qint32 nDLRM = pBac->nLRM - pBac->nLR;
    qint32 nIncrSaved = 0;

    if (nDLRM >= g_wzjpegNMaxLP[pBin->nIndex] / 2) {
        nDLRM = g_wzjpegNMaxLP[pBin->nIndex] - nDLRM;
        if (nDLRM <= g_wzjpegNMaxLP[pBin->nIndex] / 4) wzjpegBacDblIndex(&pBin->nIndex, &nIncrSaved);
        wzjpegBacDblIndex(&pBin->nIndex, &nIncrSaved);
    } else {
        if (nDLRM >= g_wzjpegNMaxLP[pBin->nIndex] / 4) wzjpegBacIncrIndex(&pBin->nIndex, &nIncrSaved);
        wzjpegBacIncrIndex(&pBin->nIndex, &nIncrSaved);
    }

    if (pBin->nIndex <= 0) {
        pBin->nIndex = nIncrSaved;
        pBin->nMPS = pBin->nMPS ^ 1;
    }

    pBac->nLRM = pBac->nLR + nDLRM;
}

static void wzjpegBacUpdateLPS(WZJPEG_BAC *pBac, WZJPEG_BIN *pBin)
{
    pBac->nLR += g_wzjpegLogQP[pBin->nIndex];
    pBac->nLRM += g_wzjpegLogQP[pBin->nIndex];

    if (pBin->nK >= 11) {  // kmax
        wzjpegBacQBigger(pBac, pBin);
        pBin->nK = 0;
        pBac->nLRM = pBac->nLR + g_wzjpegNMaxLP[pBin->nIndex];
    } else {
        if (pBac->nLRM < pBac->nLR) pBac->nLRM = pBac->nLR;
    }
}

static qint32 wzjpegBacDecodeBit(WZJPEG_BAC *pBac, WZJPEG_BIN *pBin)
{
    pBac->nLRM = pBac->nLR + pBin->nDLRM;
    wzjpegBacLRMBig(pBac);

    pBac->nLR += g_wzjpegLogP[pBin->nIndex];

    qint32 nBit = pBin->nMPS;

    qint32 nLRT = pBac->nLRM;
    if (pBac->nLX < nLRT) nLRT = pBac->nLX;

    if (pBac->nLR >= nLRT) {
        if (pBac->nLR < pBac->nLX) {
            wzjpegBacUpdateMPS(pBac, pBin);
        } else {
            wzjpegBacRenorm(pBac);
            if (pBac->nLR < pBac->nLX) {
                if (pBac->nLR >= pBac->nLRM) wzjpegBacUpdateMPS(pBac, pBin);
            } else {
                nBit ^= 1;
                pBin->nK++;
                pBac->nX -= wzjpegBacAntilogX(pBac->nLR);
                pBac->nLX = wzjpegBacLogX(pBac->nX);
                wzjpegBacUpdateLPS(pBac, pBin);
            }
        }
    }

    pBin->nDLRM = pBac->nLRM - pBac->nLR;

    return nBit;
}

// JPEG metadata state, persistent across bundles.
struct WZJPEG_HUFFCODE {
    quint16 nCode;
    quint8 nLength;  // 0 = value has no code in this table
};

struct WZJPEG_COMPONENT {
    qint32 nIdentifier;
    qint32 nHorizontalFactor;
    qint32 nVerticalFactor;
    qint32 nQuantIndex;
};

struct WZJPEG_SCANCOMPONENT {
    qint32 nComponentIndex;
    qint32 nDCTable;
    qint32 nACTable;
};

struct WZJPEG_METADATA {
    quint16 quantTables[4][64];
    WZJPEG_HUFFCODE huffCodes[2][4][256];  // [class][index][value]
    qint32 nBits;
    qint32 nHeight;
    qint32 nWidth;
    qint32 nNumComponents;
    WZJPEG_COMPONENT components[4];
    qint32 nMaxHorizontalFactor;
    qint32 nMaxVerticalFactor;
    qint32 nHorizontalMCUs;
    qint32 nVerticalMCUs;
    qint32 nNumScanComponents;
    WZJPEG_SCANCOMPONENT scanComponents[4];
    qint32 nRestartInterval;
};

enum WZJPEG_PARSE_RESULT {
    WZJPEG_PARSE_FAILED = 0,
    WZJPEG_PARSE_FOUND_SOS,
    WZJPEG_PARSE_FOUND_EOI
};

static qint32 wzjpegParseUInt16BE(const quint8 *pData)
{
    return ((qint32)pData[0] << 8) | pData[1];
}

// Find the next marker byte, skipping 0xff fill bytes. Returns -1 on failure.
static qint64 wzjpegFindNextMarker(const quint8 *pData, qint64 nSize, qint64 nOffset)
{
    if (nOffset >= nSize) return -1;
    if (pData[nOffset] != 0xff) return -1;
    while (pData[nOffset] == 0xff) {
        nOffset++;
        if (nOffset >= nSize) return -1;
    }
    return nOffset;
}

// Parse and bound-check a marker segment size.
static qint32 wzjpegParseSegmentSize(const quint8 *pData, qint64 nSize, qint64 nOffset)
{
    if (nOffset + 2 > nSize) return 0;
    const qint32 nSegmentSize = wzjpegParseUInt16BE(pData + nOffset);
    if (nSegmentSize < 2) return 0;
    if (nOffset + nSegmentSize > nSize) return 0;
    return nSegmentSize;
}

// Parse the recognized markers of one bundle's metadata (spec 5.2), updating
// the persistent state. Stops at SOS or EOI.
static WZJPEG_PARSE_RESULT wzjpegParseMetadata(WZJPEG_METADATA *pMeta, const quint8 *pData, qint64 nSize)
{
    qint64 nOffset = 0;

    for (;;) {
        nOffset = wzjpegFindNextMarker(pData, nSize, nOffset);
        if (nOffset < 0) return WZJPEG_PARSE_FAILED;

        const quint8 nMarker = pData[nOffset];
        nOffset++;

        if (nMarker == 0xd8) {  // SOI
            continue;
        } else if (nMarker == 0xd9) {  // EOI
            return WZJPEG_PARSE_FOUND_EOI;
        } else if (nMarker == 0xc4) {  // DHT
            const qint32 nSegmentSize = wzjpegParseSegmentSize(pData, nSize, nOffset);
            if (!nSegmentSize) return WZJPEG_PARSE_FAILED;
            const qint64 nNext = nOffset + nSegmentSize;
            qint64 nPos = nOffset + 2;

            while (nPos + 17 <= nNext) {
                const qint32 nClass = pData[nPos] >> 4;
                const qint32 nIndex = pData[nPos] & 0x0f;
                nPos++;
                if ((nClass != 0) && (nClass != 1)) return WZJPEG_PARSE_FAILED;
                if (nIndex >= 4) return WZJPEG_PARSE_FAILED;

                qint32 nCodesPerLength[16];
                qint32 nTotalCodes = 0;
                for (qint32 i = 0; i < 16; i++) {
                    nCodesPerLength[i] = pData[nPos + i];
                    nTotalCodes += nCodesPerLength[i];
                }
                nPos += 16;
                if (nPos + nTotalCodes > nNext) return WZJPEG_PARSE_FAILED;

                // Canonical code assignment (ITU T.81 Annex C).
                memset(pMeta->huffCodes[nClass][nIndex], 0, sizeof(pMeta->huffCodes[nClass][nIndex]));
                quint32 nCode = 0;
                for (qint32 i = 0; i < 16; i++) {
                    for (qint32 j = 0; j < nCodesPerLength[i]; j++) {
                        const qint32 nValue = pData[nPos];
                        nPos++;
                        pMeta->huffCodes[nClass][nIndex][nValue].nCode = (quint16)nCode;
                        pMeta->huffCodes[nClass][nIndex][nValue].nLength = (quint8)(i + 1);
                        nCode++;
                    }
                    nCode <<= 1;
                }
            }

            nOffset = nNext;
        } else if (nMarker == 0xdb) {  // DQT
            const qint32 nSegmentSize = wzjpegParseSegmentSize(pData, nSize, nOffset);
            if (!nSegmentSize) return WZJPEG_PARSE_FAILED;
            const qint64 nNext = nOffset + nSegmentSize;
            qint64 nPos = nOffset + 2;

            while (nPos + 1 <= nNext) {
                const qint32 nPrecision = pData[nPos] >> 4;
                const qint32 nIndex = pData[nPos] & 0x0f;
                nPos++;
                if (nIndex >= 4) return WZJPEG_PARSE_FAILED;

                if (nPrecision == 0) {
                    if (nPos + 64 > nNext) return WZJPEG_PARSE_FAILED;
                    for (qint32 i = 0; i < 64; i++) pMeta->quantTables[nIndex][i] = pData[nPos + i];
                    nPos += 64;
                } else if (nPrecision == 1) {
                    if (nPos + 128 > nNext) return WZJPEG_PARSE_FAILED;
                    for (qint32 i = 0; i < 64; i++) pMeta->quantTables[nIndex][i] = (quint16)wzjpegParseUInt16BE(pData + nPos + 2 * i);
                    nPos += 128;
                } else {
                    return WZJPEG_PARSE_FAILED;
                }
            }

            nOffset = nNext;
        } else if (nMarker == 0xdd) {  // DRI
            const qint32 nSegmentSize = wzjpegParseSegmentSize(pData, nSize, nOffset);
            if (!nSegmentSize || (nSegmentSize < 4)) return WZJPEG_PARSE_FAILED;
            pMeta->nRestartInterval = wzjpegParseUInt16BE(pData + nOffset + 2);
            nOffset += nSegmentSize;
        } else if ((nMarker == 0xc0) || (nMarker == 0xc1)) {  // SOF0/SOF1
            const qint32 nSegmentSize = wzjpegParseSegmentSize(pData, nSize, nOffset);
            if (!nSegmentSize || (nSegmentSize < 8)) return WZJPEG_PARSE_FAILED;

            pMeta->nBits = pData[nOffset + 2];
            pMeta->nHeight = wzjpegParseUInt16BE(pData + nOffset + 3);
            pMeta->nWidth = wzjpegParseUInt16BE(pData + nOffset + 5);
            pMeta->nNumComponents = pData[nOffset + 7];
            if ((pMeta->nNumComponents < 1) || (pMeta->nNumComponents > 4)) return WZJPEG_PARSE_FAILED;
            if (nSegmentSize < 8 + pMeta->nNumComponents * 3) return WZJPEG_PARSE_FAILED;
            if ((pMeta->nWidth == 0) || (pMeta->nHeight == 0)) return WZJPEG_PARSE_FAILED;

            pMeta->nMaxHorizontalFactor = 1;
            pMeta->nMaxVerticalFactor = 1;

            for (qint32 i = 0; i < pMeta->nNumComponents; i++) {
                pMeta->components[i].nIdentifier = pData[nOffset + 8 + i * 3];
                pMeta->components[i].nHorizontalFactor = pData[nOffset + 9 + i * 3] >> 4;
                pMeta->components[i].nVerticalFactor = pData[nOffset + 9 + i * 3] & 0x0f;
                pMeta->components[i].nQuantIndex = pData[nOffset + 10 + i * 3];
                if (pMeta->components[i].nQuantIndex >= 4) return WZJPEG_PARSE_FAILED;
                if ((pMeta->components[i].nHorizontalFactor < 1) || (pMeta->components[i].nHorizontalFactor > 4)) return WZJPEG_PARSE_FAILED;
                if ((pMeta->components[i].nVerticalFactor < 1) || (pMeta->components[i].nVerticalFactor > 4)) return WZJPEG_PARSE_FAILED;

                if (pMeta->components[i].nHorizontalFactor > pMeta->nMaxHorizontalFactor) pMeta->nMaxHorizontalFactor = pMeta->components[i].nHorizontalFactor;
                if (pMeta->components[i].nVerticalFactor > pMeta->nMaxVerticalFactor) pMeta->nMaxVerticalFactor = pMeta->components[i].nVerticalFactor;
            }

            // Single-component frames are stored as if they used 1x1 sampling
            // regardless of the declared factors.
            if (pMeta->nNumComponents == 1) {
                pMeta->components[0].nHorizontalFactor = 1;
                pMeta->components[0].nVerticalFactor = 1;
                pMeta->nMaxHorizontalFactor = 1;
                pMeta->nMaxVerticalFactor = 1;
            }

            const qint32 nMCUWidth = pMeta->nMaxHorizontalFactor * 8;
            const qint32 nMCUHeight = pMeta->nMaxVerticalFactor * 8;
            pMeta->nHorizontalMCUs = (pMeta->nWidth + nMCUWidth - 1) / nMCUWidth;
            pMeta->nVerticalMCUs = (pMeta->nHeight + nMCUHeight - 1) / nMCUHeight;

            nOffset += nSegmentSize;
        } else if (nMarker == 0xda) {  // SOS
            const qint32 nSegmentSize = wzjpegParseSegmentSize(pData, nSize, nOffset);
            if (!nSegmentSize || (nSegmentSize < 6)) return WZJPEG_PARSE_FAILED;

            pMeta->nNumScanComponents = pData[nOffset + 2];
            if ((pMeta->nNumScanComponents < 1) || (pMeta->nNumScanComponents > 4)) return WZJPEG_PARSE_FAILED;
            if (nSegmentSize < 6 + pMeta->nNumScanComponents * 2) return WZJPEG_PARSE_FAILED;

            for (qint32 i = 0; i < pMeta->nNumScanComponents; i++) {
                const qint32 nIdentifier = pData[nOffset + 3 + i * 2];
                qint32 nComponentIndex = -1;
                for (qint32 j = 0; j < pMeta->nNumComponents; j++) {
                    if (pMeta->components[j].nIdentifier == nIdentifier) {
                        nComponentIndex = j;
                        break;
                    }
                }
                if (nComponentIndex < 0) return WZJPEG_PARSE_FAILED;

                pMeta->scanComponents[i].nComponentIndex = nComponentIndex;
                pMeta->scanComponents[i].nDCTable = pData[nOffset + 4 + i * 2] >> 4;
                pMeta->scanComponents[i].nACTable = pData[nOffset + 4 + i * 2] & 0x0f;
                if ((pMeta->scanComponents[i].nDCTable >= 4) || (pMeta->scanComponents[i].nACTable >= 4)) return WZJPEG_PARSE_FAILED;
            }

            // Only full-spectrum sequential scans exist in method 96 streams.
            if (pData[nOffset + 3 + pMeta->nNumScanComponents * 2] != 0) return WZJPEG_PARSE_FAILED;
            if (pData[nOffset + 4 + pMeta->nNumScanComponents * 2] != 63) return WZJPEG_PARSE_FAILED;
            if (pData[nOffset + 5 + pMeta->nNumScanComponents * 2] != 0) return WZJPEG_PARSE_FAILED;

            return WZJPEG_PARSE_FOUND_SOS;
        } else {
            const qint32 nSegmentSize = wzjpegParseSegmentSize(pData, nSize, nOffset);
            if (!nSegmentSize) return WZJPEG_PARSE_FAILED;
            nOffset += nSegmentSize;
        }
    }
}

// One decoded 8x8 DCT block, coefficients in zigzag order.
struct WZJPEG_BLOCK {
    qint16 c[64];
    quint8 nEOB;
};

static const WZJPEG_BLOCK g_wzjpegZeroBlock = {{0}, 0};

static qint32 wzjpegMin(qint32 a, qint32 b)
{
    return (a < b) ? a : b;
}

static qint32 wzjpegAbs(qint32 x)
{
    return (x >= 0) ? x : -x;
}

static qint32 wzjpegSign(qint32 x)
{
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

// CAT (5.6.3): ceil(log2(abs(value) + 1)).
static qint32 wzjpegCategory(quint32 nValue)
{
    qint32 nCategory = 0;
    while (nValue) {
        nValue >>= 1;
        nCategory++;
    }
    return nCategory;
}

static qint32 wzjpegRowOf(qint32 k)
{
    return g_wzjpegRow[k];
}

static qint32 wzjpegColumnOf(qint32 k)
{
    return g_wzjpegColumn[k];
}

static qint32 wzjpegZigZagAt(qint32 nRow, qint32 nColumn)
{
    return g_wzjpegZigZag[nRow][nColumn];
}

static bool wzjpegIsFirstRow(qint32 k)
{
    return wzjpegRowOf(k) == 0;
}

static bool wzjpegIsFirstColumn(qint32 k)
{
    return wzjpegColumnOf(k) == 0;
}

static bool wzjpegIsFirstRowOrColumn(qint32 k)
{
    return wzjpegIsFirstRow(k) || wzjpegIsFirstColumn(k);
}

static qint32 wzjpegLeftOf(qint32 k)
{
    return wzjpegZigZagAt(wzjpegRowOf(k), wzjpegColumnOf(k) - 1);
}

static qint32 wzjpegUpOf(qint32 k)
{
    return wzjpegZigZagAt(wzjpegRowOf(k) - 1, wzjpegColumnOf(k));
}

static qint32 wzjpegUpLeftOf(qint32 k)
{
    return wzjpegZigZagAt(wzjpegRowOf(k) - 1, wzjpegColumnOf(k) - 1);
}

static qint32 wzjpegRightOf(qint32 k)
{
    return wzjpegZigZagAt(wzjpegRowOf(k), wzjpegColumnOf(k) + 1);
}

static qint32 wzjpegDownOf(qint32 k)
{
    return wzjpegZigZagAt(wzjpegRowOf(k) + 1, wzjpegColumnOf(k));
}

// SUM (5.6.2.1): coefficients below and to the right of k. All such positions
// have a larger zigzag index than k, so at decode time (EOB downward) every
// contributing coefficient has already been decoded.
static qint32 wzjpegSum(qint32 k, const WZJPEG_BLOCK *pBlock)
{
    qint32 nSum = 0;
    const qint32 nRow = wzjpegRowOf(k);
    const qint32 nColumn = wzjpegColumnOf(k);
    for (qint32 i = 0; i < 64; i++) {
        if ((i != k) && (wzjpegRowOf(i) >= nRow) && (wzjpegColumnOf(i) >= nColumn)) nSum += wzjpegAbs(pBlock->c[i]);
    }
    return nSum;
}

// AVG (5.6.2.2). SPEC-ERRATUM: the summed neighbour term is Bw[x], not Bw[k],
// and the DC coefficient never participates.
static qint32 wzjpegAverage(qint32 k, const WZJPEG_BLOCK *pNorth, const WZJPEG_BLOCK *pWest, const quint16 *pQuant)
{
    if ((k == 0) || (k == 1) || (k == 2)) {
        return (wzjpegAbs(pNorth->c[k]) + wzjpegAbs(pWest->c[k]) + 1) / 2;
    } else if (wzjpegIsFirstRow(k)) {
        const qint32 nLeft = wzjpegLeftOf(k);
        return ((wzjpegAbs(pNorth->c[nLeft]) + wzjpegAbs(pWest->c[nLeft])) * pQuant[nLeft] / pQuant[k] + wzjpegAbs(pNorth->c[k]) + wzjpegAbs(pWest->c[k]) + 2) / (2 * 2);
    } else if (wzjpegIsFirstColumn(k)) {
        const qint32 nUp = wzjpegUpOf(k);
        return ((wzjpegAbs(pNorth->c[nUp]) + wzjpegAbs(pWest->c[nUp])) * pQuant[nUp] / pQuant[k] + wzjpegAbs(pNorth->c[k]) + wzjpegAbs(pWest->c[k]) + 2) / (2 * 2);
    } else if (k == 4) {
        const qint32 nUp = wzjpegUpOf(k);
        const qint32 nLeft = wzjpegLeftOf(k);
        return ((wzjpegAbs(pNorth->c[nUp]) + wzjpegAbs(pWest->c[nUp])) * pQuant[nUp] / pQuant[k] +
                (wzjpegAbs(pNorth->c[nLeft]) + wzjpegAbs(pWest->c[nLeft])) * pQuant[nLeft] / pQuant[k] + wzjpegAbs(pNorth->c[k]) + wzjpegAbs(pWest->c[k]) + 3) /
               (2 * 3);
    } else {
        const qint32 nUp = wzjpegUpOf(k);
        const qint32 nLeft = wzjpegLeftOf(k);
        const qint32 nUpLeft = wzjpegUpLeftOf(k);
        return ((wzjpegAbs(pNorth->c[nUp]) + wzjpegAbs(pWest->c[nUp])) * pQuant[nUp] / pQuant[k] +
                (wzjpegAbs(pNorth->c[nLeft]) + wzjpegAbs(pWest->c[nLeft])) * pQuant[nLeft] / pQuant[k] +
                (wzjpegAbs(pNorth->c[nUpLeft]) + wzjpegAbs(pWest->c[nUpLeft])) * pQuant[nUpLeft] / pQuant[k] + wzjpegAbs(pNorth->c[k]) + wzjpegAbs(pWest->c[k]) + 4) /
               (2 * 4);
    }
}

// BDR (5.6.2.3). SPEC-ERRATUM: the bracketed term is a sum, not a difference.
static qint32 wzjpegBDR(qint32 k, const WZJPEG_BLOCK *pCurrent, const WZJPEG_BLOCK *pNorth, const WZJPEG_BLOCK *pWest, const quint16 *pQuant)
{
    if (wzjpegIsFirstRow(k)) {
        const qint32 nDown = wzjpegDownOf(k);
        return pNorth->c[k] - (pNorth->c[nDown] + pCurrent->c[nDown]) * pQuant[nDown] / pQuant[k];
    } else if (wzjpegIsFirstColumn(k)) {
        const qint32 nRight = wzjpegRightOf(k);
        return pWest->c[k] - (pWest->c[nRight] + pCurrent->c[nRight]) * pQuant[nRight] / pQuant[k];
    }
    return 0;
}

// The whole per-record decode session.
struct WZJPEG_SESSION {
    WZJPEG_INPUT input;
    WZJPEG_BAC bac;
    WZJPEG_METADATA meta;

    // Arithmetic model bins, initialized per bundle (fixed bin per session).
    WZJPEG_BIN eobBins[4][13][63];
    WZJPEG_BIN zeroBins[4][62][3][6];
    WZJPEG_BIN pivotBins[4][63][5][7];
    WZJPEG_BIN acMagnitudeBins[4][3][9][9][9];
    WZJPEG_BIN acRemainderBins[4][3][7][13];
    WZJPEG_BIN acSignBins[4][27][3][2];
    WZJPEG_BIN dcMagnitudeBins[4][13][10];
    WZJPEG_BIN dcRemainderBins[4][13][14];
    WZJPEG_BIN dcSignBins[4][2][2][2];
    WZJPEG_BIN fixedBin;

    qint32 nSignContextForK[64];  // n for the 27 k that use a sign context

    // Slice geometry and buffers.
    qint32 nSliceValue;
    qint32 nSliceHeight;
    qint32 nCurrentHeight;
    qint32 nFinishedRows;
    bool bSlicesAvailable;
    WZJPEG_BLOCK *pBlocks[4];
    qint64 nBlocksPerComponent[4];

    // Huffman re-encoder state, persistent across the slices of a bundle.
    quint64 nBitString;
    qint32 nBitLength;
    qint32 nHuffmanPredicted[4];
    qint32 nMCUCounter;
    qint32 nRestartMarkerIndex;

    // Output accumulator. Plain storage only: the whole session is zeroed
    // with memset at construction.
    XBinary::DATAPROCESS_STATE *pState;
    char baOutputBuffer[WZJPEG_OUTPUT_FLUSH_SIZE + 0x8000];
    qint32 nOutputSize;
    bool bWriteFailed;
    qint64 nTotalWritten;
};

static void wzjpegInitSignContextTable(WZJPEG_SESSION *pSession)
{
    // The 27 zigzag positions within the first or second rows or columns use
    // an adaptive sign context, numbered in ascending zigzag order (5.6.6.4).
    qint32 nNext = 0;
    for (qint32 k = 0; k < 64; k++) pSession->nSignContextForK[k] = 0;
    for (qint32 k = 1; k < 64; k++) {
        const bool bEligible = wzjpegIsFirstRowOrColumn(k) || (wzjpegRowOf(k) == 1) || (wzjpegColumnOf(k) == 1);
        if (bEligible) {
            pSession->nSignContextForK[k] = nNext;
            nNext++;
        }
    }
}

static void wzjpegInitBins(WZJPEG_BIN *pBins, qint64 nCount)
{
    for (qint64 i = 0; i < nCount; i++) wzjpegBinInit(&pBins[i]);
}

static void wzjpegInitAllBins(WZJPEG_SESSION *pSession)
{
    wzjpegInitBins(&pSession->eobBins[0][0][0], sizeof(pSession->eobBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->zeroBins[0][0][0][0], sizeof(pSession->zeroBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->pivotBins[0][0][0][0], sizeof(pSession->pivotBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->acMagnitudeBins[0][0][0][0][0], sizeof(pSession->acMagnitudeBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->acRemainderBins[0][0][0][0], sizeof(pSession->acRemainderBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->acSignBins[0][0][0][0], sizeof(pSession->acSignBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->dcMagnitudeBins[0][0][0], sizeof(pSession->dcMagnitudeBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->dcRemainderBins[0][0][0], sizeof(pSession->dcRemainderBins) / sizeof(WZJPEG_BIN));
    wzjpegInitBins(&pSession->dcSignBins[0][0][0][0], sizeof(pSession->dcSignBins) / sizeof(WZJPEG_BIN));
}

// Generalized Elias gamma binarization (5.6.4, with the reverse-engineered
// bin-mapping corrections noted at the top of this file).
static qint32 wzjpegDecodeBinarization(WZJPEG_BAC *pBac, WZJPEG_BIN *pMagnitudeBins, WZJPEG_BIN *pRemainderBins, qint32 nMaxBits, qint32 nCap)
{
    qint32 nOnes = 0;
    while (nOnes < nMaxBits) {
        qint32 nContext = nOnes;
        if (nContext >= nCap) nContext = nCap - 1;
        const qint32 nUnaryBit = wzjpegBacDecodeBit(pBac, &pMagnitudeBins[nContext]);
        if (nUnaryBit == 1) nOnes++;
        else break;
    }

    if (nOnes == 0) return 0;
    if (nOnes == 1) return 1;

    const qint32 nNumBits = nOnes - 1;
    qint32 nValue = 1 << nNumBits;
    for (qint32 i = nNumBits - 1; i >= 0; i--) {
        const qint32 nBit = wzjpegBacDecodeBit(pBac, &pRemainderBins[i]);
        nValue |= nBit << i;
    }
    return nValue;
}

static qint32 wzjpegDecodeACSign(WZJPEG_SESSION *pSession, qint32 nComp, qint32 k, qint32 nAbsValue, const WZJPEG_BLOCK *pCurrent, const WZJPEG_BLOCK *pNorth,
                                 const WZJPEG_BLOCK *pWest, const quint16 *pQuant)
{
    // AC sign coding (5.6.6.4).
    qint32 nPredictedSign = 0;
    if (wzjpegIsFirstRowOrColumn(k)) {
        const qint32 nBdr = wzjpegBDR(k, pCurrent, pNorth, pWest, pQuant);
        if (nBdr == 0) return wzjpegBacDecodeBit(&pSession->bac, &pSession->fixedBin);
        nPredictedSign = (nBdr < 0) ? 1 : 0;
    } else if (k == 4) {
        const qint32 nSign1 = wzjpegSign(pNorth->c[k]);
        const qint32 nSign2 = wzjpegSign(pWest->c[k]);
        if (nSign1 + nSign2 == 0) return wzjpegBacDecodeBit(&pSession->bac, &pSession->fixedBin);
        nPredictedSign = (nSign1 + nSign2 < 0) ? 1 : 0;
    } else if (wzjpegRowOf(k) == 1) {
        if (pNorth->c[k] == 0) return wzjpegBacDecodeBit(&pSession->bac, &pSession->fixedBin);
        nPredictedSign = (pNorth->c[k] < 0) ? 1 : 0;
    } else if (wzjpegColumnOf(k) == 1) {
        if (pWest->c[k] == 0) return wzjpegBacDecodeBit(&pSession->bac, &pSession->fixedBin);
        nPredictedSign = (pWest->c[k] < 0) ? 1 : 0;
    } else {
        return wzjpegBacDecodeBit(&pSession->bac, &pSession->fixedBin);
    }

    const qint32 n = pSession->nSignContextForK[k];
    const qint32 nSignContext1 = wzjpegMin(wzjpegCategory((quint32)nAbsValue) / 2, 2);

    return wzjpegBacDecodeBit(&pSession->bac, &pSession->acSignBins[nComp][n][nSignContext1][nPredictedSign]);
}

static qint32 wzjpegDecodeACComponent(WZJPEG_SESSION *pSession, qint32 nComp, qint32 k, bool bCanBeZero, const WZJPEG_BLOCK *pCurrent, const WZJPEG_BLOCK *pNorth,
                                      const WZJPEG_BLOCK *pWest, const quint16 *pQuant)
{
    if (!pNorth) pNorth = &g_wzjpegZeroBlock;
    if (!pWest) pWest = &g_wzjpegZeroBlock;

    qint32 nVal1 = 0;
    if (wzjpegIsFirstRowOrColumn(k)) nVal1 = wzjpegAbs(wzjpegBDR(k, pCurrent, pNorth, pWest, pQuant));
    else nVal1 = wzjpegAverage(k, pNorth, pWest, pQuant);

    const qint32 nVal2 = wzjpegSum(k, pCurrent);

    if (bCanBeZero) {
        // Zero/non-zero decision (5.6.6.1).
        const qint32 nZeroContext1 = wzjpegMin(wzjpegCategory((quint32)nVal1), 2);
        const qint32 nZeroContext2 = wzjpegMin(wzjpegCategory((quint32)nVal2), 5);

        const qint32 nNonZero = wzjpegBacDecodeBit(&pSession->bac, &pSession->zeroBins[nComp][k - 1][nZeroContext1][nZeroContext2]);
        if (!nNonZero) return 0;
    }

    qint32 nAbsValue = 0;

    // Pivot decision, abs >= 2 (5.6.6.2).
    const qint32 nPivotContext1 = wzjpegMin(wzjpegCategory((quint32)nVal1), 4);
    const qint32 nPivotContext2 = wzjpegMin(wzjpegCategory((quint32)nVal2), 6);

    const qint32 nPivot = wzjpegBacDecodeBit(&pSession->bac, &pSession->pivotBins[nComp][k - 1][nPivotContext1][nPivotContext2]);

    if (!nPivot) {
        nAbsValue = 1;
    } else {
        // Absolute value (5.6.6.3).
        qint32 nVal3 = 0;
        qint32 n = 0;
        if (wzjpegIsFirstRow(k)) {
            nVal3 = wzjpegColumnOf(k) - 1;
            n = 0;
        } else if (wzjpegIsFirstColumn(k)) {
            nVal3 = wzjpegRowOf(k) - 1;
            n = 1;
        } else {
            nVal3 = wzjpegCategory((quint32)(k - 4));
            n = 2;
        }

        const qint32 nMagnitudeContext1 = wzjpegMin(wzjpegCategory((quint32)nVal1), 8);
        const qint32 nMagnitudeContext2 = wzjpegMin(wzjpegCategory((quint32)nVal2), 8);

        nAbsValue = wzjpegDecodeBinarization(&pSession->bac, pSession->acMagnitudeBins[nComp][n][nMagnitudeContext1][nMagnitudeContext2],
                                             pSession->acRemainderBins[nComp][n][nVal3], 14, 9) +
                    2;
    }

    if (wzjpegDecodeACSign(pSession, nComp, k, nAbsValue, pCurrent, pNorth, pWest, pQuant)) return -nAbsValue;
    return nAbsValue;
}

static qint32 wzjpegDecodeDCComponent(WZJPEG_SESSION *pSession, qint32 nComp, const WZJPEG_BLOCK *pCurrent, const WZJPEG_BLOCK *pNorth, const WZJPEG_BLOCK *pWest,
                                      const quint16 *pQuant)
{
    // DC prediction (5.6.7.1). SPEC-ERRATUM: the neighbour AC terms are added
    // to the current block's, not subtracted.
    qint32 nPredicted = 0;
    if (!pNorth && !pWest) {
        nPredicted = 0;
    } else if (!pNorth) {
        const qint64 t1 = (qint64)pWest->c[0] * 10000 - (qint64)11038 * pQuant[1] * (pWest->c[1] + pCurrent->c[1]) / pQuant[0];
        nPredicted = (qint32)(((t1 < 0) ? (t1 - 5000) : (t1 + 5000)) / 10000);
    } else if (!pWest) {
        const qint64 t0 = (qint64)pNorth->c[0] * 10000 - (qint64)11038 * pQuant[2] * (pNorth->c[2] + pCurrent->c[2]) / pQuant[0];
        nPredicted = (qint32)(((t0 < 0) ? (t0 - 5000) : (t0 + 5000)) / 10000);
    } else {
        const qint64 t0 = (qint64)pNorth->c[0] * 10000 - (qint64)11038 * pQuant[2] * (pNorth->c[2] + pCurrent->c[2]) / pQuant[0];
        const qint32 p0 = (qint32)(((t0 < 0) ? (t0 - 5000) : (t0 + 5000)) / 10000);

        const qint64 t1 = (qint64)pWest->c[0] * 10000 - (qint64)11038 * pQuant[1] * (pWest->c[1] + pCurrent->c[1]) / pQuant[0];
        const qint32 p1 = (qint32)(((t1 < 0) ? (t1 - 5000) : (t1 + 5000)) / 10000);

        // Prediction refinement (5.6.7.2). SPEC-ERRATUM: plain differences,
        // without the inner absolute values.
        qint32 d0 = 0;
        qint32 d1 = 0;
        for (qint32 i = 1; i < 8; i++) {
            d0 += wzjpegAbs(pNorth->c[wzjpegZigZagAt(i, 0)] - pCurrent->c[wzjpegZigZagAt(i, 0)]);
            d1 += wzjpegAbs(pWest->c[wzjpegZigZagAt(0, i)] - pCurrent->c[wzjpegZigZagAt(0, i)]);
        }

        if (d0 > d1) {
            const qint64 nWeight = (qint64)1 << wzjpegMin(d0 - d1, 31);
            nPredicted = (qint32)((nWeight * p1 + p0) / (1 + nWeight));
        } else {
            const qint64 nWeight = (qint64)1 << wzjpegMin(d1 - d0, 31);
            nPredicted = (qint32)((nWeight * p0 + p1) / (1 + nWeight));
        }
    }

    // DC residual absolute value (5.6.7.3.1).
    const qint32 nSum = wzjpegSum(0, pCurrent);
    const qint32 nValueContext = wzjpegMin(wzjpegCategory((quint32)nSum), 12);

    const qint32 nAbsValue =
        wzjpegDecodeBinarization(&pSession->bac, pSession->dcMagnitudeBins[nComp][nValueContext], pSession->dcRemainderBins[nComp][nValueContext], 15, 10);
    if (nAbsValue == 0) return nPredicted;

    // DC residual sign (5.6.7.3.2). SPEC-ERRATUM: the neighbour DC values are
    // compared against the predicted DC, not against zero.
    if (!pNorth) pNorth = &g_wzjpegZeroBlock;
    if (!pWest) pWest = &g_wzjpegZeroBlock;
    const qint32 nNorthSign = (pNorth->c[0] < nPredicted) ? 1 : 0;
    const qint32 nWestSign = (pWest->c[0] < nPredicted) ? 1 : 0;
    const qint32 nPredictedSign = (nPredicted < 0) ? 1 : 0;

    const qint32 nSign = wzjpegBacDecodeBit(&pSession->bac, &pSession->dcSignBins[nComp][nNorthSign][nWestSign][nPredictedSign]);

    if (nSign) return nPredicted - nAbsValue;
    return nPredicted + nAbsValue;
}

static void wzjpegDecodeBlock(WZJPEG_SESSION *pSession, qint32 nComp, WZJPEG_BLOCK *pCurrent, const WZJPEG_BLOCK *pNorth, const WZJPEG_BLOCK *pWest,
                              const quint16 *pQuant)
{
    // EOB context (5.6.5.2).
    qint32 nAverage = 0;
    if (!pNorth && !pWest) nAverage = 0;
    else if (!pNorth) nAverage = wzjpegSum(0, pWest);
    else if (!pWest) nAverage = wzjpegSum(0, pNorth);
    else nAverage = (wzjpegSum(0, pNorth) + wzjpegSum(0, pWest) + 1) / 2;

    const qint32 nEOBContext = wzjpegMin(wzjpegCategory((quint32)nAverage), 12);

    // Binary-tree EOB decode (5.6.5.1): six bits, bins indexed by the partial
    // bit string.
    quint32 nBitString = 1;
    for (qint32 i = 0; i < 6; i++) {
        nBitString = (nBitString << 1) | (quint32)wzjpegBacDecodeBit(&pSession->bac, &pSession->eobBins[nComp][nEOBContext][nBitString - 1]);
    }
    const qint32 nEOB = (qint32)(nBitString & 0x3f);
    pCurrent->nEOB = (quint8)nEOB;

    for (qint32 k = nEOB + 1; k <= 63; k++) pCurrent->c[k] = 0;

    // AC coefficients from EOB down to AC1 (5.6.6); the coefficient at EOB is
    // non-zero by definition.
    for (qint32 k = nEOB; k >= 1; k--) {
        const qint32 nValue = wzjpegDecodeACComponent(pSession, nComp, k, k != nEOB, pCurrent, pNorth, pWest, pQuant);
        pCurrent->c[k] = (qint16)nValue;
    }

    // DC (5.6.7).
    pCurrent->c[0] = (qint16)wzjpegDecodeDCComponent(pSession, nComp, pCurrent, pNorth, pWest, pQuant);
}

// Output side: bit writer that reproduces the original entropy-coded scan,
// including 0xFF00 byte stuffing.
static bool wzjpegFlushOutput(WZJPEG_SESSION *pSession, bool bForce)
{
    if (pSession->bWriteFailed) return false;
    if (!bForce && (pSession->nOutputSize < WZJPEG_OUTPUT_FLUSH_SIZE)) return true;
    if (pSession->nOutputSize == 0) return true;

    const qint32 nSize = pSession->nOutputSize;
    if (XBinary::_writeDevice(pSession->baOutputBuffer, nSize, pSession->pState) != nSize) {
        pSession->bWriteFailed = true;
        return false;
    }
    pSession->nTotalWritten += nSize;
    pSession->nOutputSize = 0;
    return true;
}

static void wzjpegAppendOutputByte(WZJPEG_SESSION *pSession, quint8 nByte)
{
    if (pSession->nOutputSize >= (qint32)sizeof(pSession->baOutputBuffer)) {
        if (!wzjpegFlushOutput(pSession, true)) return;
    }
    pSession->baOutputBuffer[pSession->nOutputSize] = (char)nByte;
    pSession->nOutputSize++;
}

static void wzjpegEmitScanByte(WZJPEG_SESSION *pSession, quint8 nByte)
{
    wzjpegAppendOutputByte(pSession, nByte);
    if (nByte == 0xff) wzjpegAppendOutputByte(pSession, 0x00);  // byte stuffing
}

static void wzjpegPushBits(WZJPEG_SESSION *pSession, quint32 nBits, qint32 nLength)
{
    if (nLength <= 0) return;
    pSession->nBitString |= (quint64)nBits << (64 - pSession->nBitLength - nLength);
    pSession->nBitLength += nLength;
    while (pSession->nBitLength >= 8) {
        const quint8 nByte = (quint8)(pSession->nBitString >> 56);
        wzjpegEmitScanByte(pSession, nByte);
        pSession->nBitString <<= 8;
        pSession->nBitLength -= 8;
    }
}

static bool wzjpegPushHuffmanCode(WZJPEG_SESSION *pSession, const WZJPEG_HUFFCODE *pTable, qint32 nSymbol)
{
    if (pTable[nSymbol].nLength == 0) return false;  // symbol not in table: corrupt stream
    wzjpegPushBits(pSession, pTable[nSymbol].nCode, pTable[nSymbol].nLength);
    return true;
}

static bool wzjpegPushEncodedValue(WZJPEG_SESSION *pSession, const WZJPEG_HUFFCODE *pTable, qint32 nValue, qint32 nHighBits)
{
    qint32 nCategory = 0;
    quint32 nBitString = 0;
    if (nValue >= 0) {
        nCategory = wzjpegCategory((quint32)nValue);
        const quint32 nMask = ((quint32)1 << nCategory) - 1;
        nBitString = (quint32)nValue & nMask;
    } else {
        nCategory = wzjpegCategory((quint32)(-nValue));
        const quint32 nMask = ((quint32)1 << nCategory) - 1;
        nBitString = ((quint32)nValue & nMask) - 1;
    }
    if (nCategory > 15) return false;  // not representable in a JPEG code

    if (!wzjpegPushHuffmanCode(pSession, pTable, nCategory | (nHighBits << 4))) return false;
    wzjpegPushBits(pSession, nBitString, nCategory);
    return true;
}

// Re-encode one slice's decoded blocks in MCU order, recreating restart
// markers and, at the end of the scan, the closing 1-bit padding.
static bool wzjpegEncodeSlice(WZJPEG_SESSION *pSession)
{
    WZJPEG_METADATA *pMeta = &pSession->meta;

    for (qint32 nMCURow = 0; nMCURow < pSession->nCurrentHeight; nMCURow++) {
        for (qint32 nMCUCol = 0; nMCUCol < pMeta->nHorizontalMCUs; nMCUCol++) {
            // Restart marker between MCUs at the defined interval (5.4.2),
            // never at the very end of the scan.
            if (pMeta->nRestartInterval && (pSession->nMCUCounter == pMeta->nRestartInterval)) {
                if (pSession->nBitLength) {
                    const qint32 nPad = 8 - pSession->nBitLength;
                    wzjpegPushBits(pSession, ((quint32)1 << nPad) - 1, nPad);
                }
                // The marker itself must not trigger byte stuffing.
                wzjpegAppendOutputByte(pSession, 0xff);
                wzjpegAppendOutputByte(pSession, (quint8)(0xd0 + pSession->nRestartMarkerIndex));
                pSession->nRestartMarkerIndex = (pSession->nRestartMarkerIndex + 1) & 7;
                pSession->nMCUCounter = 0;
                memset(pSession->nHuffmanPredicted, 0, sizeof(pSession->nHuffmanPredicted));
            }

            for (qint32 nComp = 0; nComp < pMeta->nNumScanComponents; nComp++) {
                const WZJPEG_COMPONENT *pComponent = &pMeta->components[pMeta->scanComponents[nComp].nComponentIndex];
                const WZJPEG_HUFFCODE *pDCTable = pMeta->huffCodes[0][pMeta->scanComponents[nComp].nDCTable];
                const WZJPEG_HUFFCODE *pACTable = pMeta->huffCodes[1][pMeta->scanComponents[nComp].nACTable];
                const qint32 nHBlocks = pComponent->nHorizontalFactor;
                const qint32 nVBlocks = pComponent->nVerticalFactor;
                const qint32 nBlocksPerRow = pMeta->nHorizontalMCUs * nHBlocks;

                for (qint32 nY = 0; nY < nVBlocks; nY++) {
                    for (qint32 nX = 0; nX < nHBlocks; nX++) {
                        const qint32 nBlockX = nMCUCol * nHBlocks + nX;
                        const qint32 nBlockY = nMCURow * nVBlocks + nY;
                        const WZJPEG_BLOCK *pBlock = &pSession->pBlocks[nComp][nBlockX + (qint64)nBlockY * nBlocksPerRow];

                        // DC difference.
                        const qint32 nDiff = pBlock->c[0] - pSession->nHuffmanPredicted[nComp];
                        if (!wzjpegPushEncodedValue(pSession, pDCTable, nDiff, 0)) return false;
                        pSession->nHuffmanPredicted[nComp] = pBlock->c[0];

                        // AC run-length coding.
                        qint32 nCoeff = 1;
                        while (nCoeff <= pBlock->nEOB) {
                            const qint32 nFirstCoeff = nCoeff;
                            const qint32 nEndRun = nCoeff + 15;
                            while ((nCoeff < 63) && (nCoeff < nEndRun) && (pBlock->c[nCoeff] == 0)) nCoeff++;

                            const qint32 nZeroes = nCoeff - nFirstCoeff;
                            const qint32 nValue = pBlock->c[nCoeff];
                            if (!wzjpegPushEncodedValue(pSession, pACTable, nValue, nZeroes)) return false;
                            nCoeff++;
                        }
                        if (pBlock->nEOB != 63) {
                            if (!wzjpegPushHuffmanCode(pSession, pACTable, 0x00)) return false;  // EOB
                        }
                    }
                }
            }

            pSession->nMCUCounter++;

            if (!wzjpegFlushOutput(pSession, false)) return false;
        }
    }

    if (!pSession->bSlicesAvailable) {
        // End of scan: pad the final partial byte with one bits (F.1.2.3).
        const qint32 nPad = (-pSession->nBitLength) & 7;
        if (nPad) wzjpegPushBits(pSession, ((quint32)1 << nPad) - 1, nPad);
    }

    return wzjpegFlushOutput(pSession, false);
}

// Decode one slice: per scan component, a fresh arithmetic-decoder segment
// over the component's blocks in cartesian order (5.4, 5.5).
static bool wzjpegDecodeSlice(WZJPEG_SESSION *pSession, XBinary::PDSTRUCT *pPdStruct)
{
    WZJPEG_METADATA *pMeta = &pSession->meta;

    pSession->nCurrentHeight = pSession->nSliceHeight;
    if (pSession->nFinishedRows + pSession->nCurrentHeight >= pMeta->nVerticalMCUs) {
        pSession->nCurrentHeight = pMeta->nVerticalMCUs - pSession->nFinishedRows;
        pSession->bSlicesAvailable = false;
    }

    for (qint32 nComp = 0; nComp < pMeta->nNumScanComponents; nComp++) {
        wzjpegBacInit(&pSession->bac, &pSession->input);

        const WZJPEG_COMPONENT *pComponent = &pMeta->components[pMeta->scanComponents[nComp].nComponentIndex];
        const qint32 nHBlocks = pComponent->nHorizontalFactor;
        const qint32 nVBlocks = pComponent->nVerticalFactor;
        const qint32 nBlocksPerRow = pMeta->nHorizontalMCUs * nHBlocks;
        const quint16 *pQuant = pMeta->quantTables[pComponent->nQuantIndex];

        const qint32 nRows = pSession->nCurrentHeight * nVBlocks;
        for (qint32 nY = 0; nY < nRows; nY++) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

            for (qint32 nX = 0; nX < nBlocksPerRow; nX++) {
                WZJPEG_BLOCK *pCurrent = &pSession->pBlocks[nComp][nX + (qint64)nY * nBlocksPerRow];

                const WZJPEG_BLOCK *pNorth = nullptr;
                if (nY != 0) pNorth = &pSession->pBlocks[nComp][nX + (qint64)(nY - 1) * nBlocksPerRow];
                else if (pSession->nFinishedRows != 0) pNorth = &pSession->pBlocks[nComp][nX + (qint64)(pSession->nSliceHeight * nVBlocks - 1) * nBlocksPerRow];

                const WZJPEG_BLOCK *pWest = nullptr;
                if (nX != 0) pWest = &pSession->pBlocks[nComp][nX - 1 + (qint64)nY * nBlocksPerRow];

                wzjpegDecodeBlock(pSession, nComp, pCurrent, pNorth, pWest, pQuant);

                if (pSession->bac.bEndOfData) return false;
            }
        }

        wzjpegBacFlush(&pSession->bac);
    }

    pSession->nFinishedRows += pSession->nCurrentHeight;

    return true;
}

static bool wzjpegWriteMetadata(WZJPEG_SESSION *pSession, const quint8 *pData, qint64 nSize)
{
    for (qint64 i = 0; i < nSize; i++) {
        wzjpegAppendOutputByte(pSession, pData[i]);
        if (pSession->bWriteFailed) return false;
    }
    return wzjpegFlushOutput(pSession, false);
}

static bool wzjpegAllocateSliceBuffers(WZJPEG_SESSION *pSession)
{
    WZJPEG_METADATA *pMeta = &pSession->meta;

    for (qint32 i = 0; i < 4; i++) {
        if (pSession->pBlocks[i]) {
            free(pSession->pBlocks[i]);
            pSession->pBlocks[i] = nullptr;
        }
        pSession->nBlocksPerComponent[i] = 0;
    }

    for (qint32 nComp = 0; nComp < pMeta->nNumScanComponents; nComp++) {
        const WZJPEG_COMPONENT *pComponent = &pMeta->components[pMeta->scanComponents[nComp].nComponentIndex];
        const qint64 nBlocks = (qint64)pMeta->nHorizontalMCUs * pSession->nSliceHeight * pComponent->nHorizontalFactor * pComponent->nVerticalFactor;
        if ((nBlocks <= 0) || (nBlocks > WZJPEG_MAX_SLICE_BUFFER_BYTES / (qint64)sizeof(WZJPEG_BLOCK))) return false;

        pSession->pBlocks[nComp] = (WZJPEG_BLOCK *)malloc((size_t)(nBlocks * (qint64)sizeof(WZJPEG_BLOCK)));
        if (!pSession->pBlocks[nComp]) return false;
        memset(pSession->pBlocks[nComp], 0, (size_t)(nBlocks * (qint64)sizeof(WZJPEG_BLOCK)));
        pSession->nBlocksPerComponent[nComp] = nBlocks;
    }

    return true;
}

static bool wzjpegProcessStream(WZJPEG_SESSION *pSession, XBinary::PDSTRUCT *pPdStruct)
{
    // Properties Header (3.2).
    quint8 baHeader[4];
    if (!wzjpegInputReadFull(&pSession->input, baHeader, 4)) return false;
    if (baHeader[0] < 4) return false;
    if (baHeader[1] != 0x10) return false;  // version 1.0
    if (baHeader[2] != 0x01) return false;  // compression method 1
    if (baHeader[3] & 0xe0) return false;
    if (baHeader[0] > 4) {
        if (!wzjpegInputSkip(&pSession->input, baHeader[0] - 4)) return false;
    }
    pSession->nSliceValue = baHeader[3] & 0x1f;

    bool bFirstBundle = true;

    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        // Bundle header (4.1.1).
        quint8 baBundleHeader[8];
        if (!wzjpegInputReadFull(&pSession->input, baBundleHeader, 4)) return false;
        qint64 nUncompressedSize = (qint64)baBundleHeader[0] | ((qint64)baBundleHeader[1] << 8);
        qint64 nCompressedSize = (qint64)baBundleHeader[2] | ((qint64)baBundleHeader[3] << 8);
        if ((nUncompressedSize == 0xffff) && (nCompressedSize == 0xffff)) {
            if (!wzjpegInputReadFull(&pSession->input, baBundleHeader, 8)) return false;
            nUncompressedSize = (qint64)baBundleHeader[0] | ((qint64)baBundleHeader[1] << 8) | ((qint64)baBundleHeader[2] << 16) | ((qint64)baBundleHeader[3] << 24);
            nCompressedSize = (qint64)baBundleHeader[4] | ((qint64)baBundleHeader[5] << 8) | ((qint64)baBundleHeader[6] << 16) | ((qint64)baBundleHeader[7] << 24);
        }
        if ((nUncompressedSize <= 0) || (nUncompressedSize > WZJPEG_MAX_METADATA_SIZE)) return false;
        if ((nCompressedSize < 0) || (nCompressedSize > WZJPEG_MAX_METADATA_SIZE)) return false;

        // Bundle metadata (4.1.2): LZMA with synthesized coder properties, or
        // stored verbatim when the compressed size field is zero.
        QByteArray baMetadata;
        baMetadata.resize((qint32)nUncompressedSize);
        if (baMetadata.size() != (qint32)nUncompressedSize) return false;

        if (nCompressedSize) {
            QByteArray baCompressed;
            baCompressed.resize((qint32)nCompressedSize);
            if (baCompressed.size() != (qint32)nCompressedSize) return false;
            if (!wzjpegInputReadFull(&pSession->input, (quint8 *)baCompressed.data(), nCompressedSize)) return false;

            // Dictionary size (5.3): ceil(size / 512) * 512, clamped to
            // [1 KiB, 512 KiB]; coder properties are the LZMA defaults
            // lc=3, lp=0, pb=2 and are not stored in the stream.
            qint64 nDictionarySize = (nUncompressedSize + 511) & ~(qint64)511;
            if (nDictionarySize < 1024) nDictionarySize = 1024;
            if (nDictionarySize > 512 * 1024) nDictionarySize = 512 * 1024;

            Byte baProperties[5];
            baProperties[0] = 3 + 0 * 9 + 2 * 5 * 9;  // lc=3 lp=0 pb=2
            baProperties[1] = (Byte)(nDictionarySize & 0xff);
            baProperties[2] = (Byte)((nDictionarySize >> 8) & 0xff);
            baProperties[3] = (Byte)((nDictionarySize >> 16) & 0xff);
            baProperties[4] = (Byte)((nDictionarySize >> 24) & 0xff);

            SizeT nDestLen = (SizeT)nUncompressedSize;
            SizeT nSrcLen = (SizeT)nCompressedSize;
            ELzmaStatus lzmaStatus = LZMA_STATUS_NOT_SPECIFIED;
            const SRes nRes = X_LzmaDecode((Byte *)baMetadata.data(), &nDestLen, (const Byte *)baCompressed.constData(), &nSrcLen, baProperties, 5, LZMA_FINISH_END,
                                           &lzmaStatus, &g_wzjpegLzmaAllocator);
            if ((nRes != SZ_OK) || (nDestLen != (SizeT)nUncompressedSize)) return false;
        } else {
            if (!wzjpegInputReadFull(&pSession->input, (quint8 *)baMetadata.data(), nUncompressedSize)) return false;
        }

        if (!wzjpegWriteMetadata(pSession, (const quint8 *)baMetadata.constData(), baMetadata.size())) return false;

        // Data before the SOI marker of the first bundle is unknown metadata
        // and is not parsed (5.1).
        qint64 nParseOffset = 0;
        if (bFirstBundle) {
            qint64 nFound = -1;
            for (qint64 i = 0; i + 2 <= baMetadata.size(); i++) {
                if (((quint8)baMetadata.at((qint32)i) == 0xff) && ((quint8)baMetadata.at((qint32)(i + 1)) == 0xd8)) {
                    nFound = i;
                    break;
                }
            }
            if (nFound < 0) return false;
            nParseOffset = nFound;
            bFirstBundle = false;
        }

        const WZJPEG_PARSE_RESULT parseResult =
            wzjpegParseMetadata(&pSession->meta, (const quint8 *)baMetadata.constData() + nParseOffset, baMetadata.size() - nParseOffset);
        if (parseResult == WZJPEG_PARSE_FAILED) return false;
        if (parseResult == WZJPEG_PARSE_FOUND_EOI) break;

        // A scan follows: prepare the models and the slice geometry.
        if ((pSession->meta.nHorizontalMCUs <= 0) || (pSession->meta.nVerticalMCUs <= 0)) return false;

        wzjpegInitAllBins(pSession);

        if (pSession->nSliceValue) {
            const qint64 nPow2Size = (qint64)1 << (pSession->nSliceValue + 6);
            qint64 nDiv1 = nPow2Size / pSession->meta.nHorizontalMCUs;
            if (nDiv1 < 1) nDiv1 = 1;
            const qint64 nDiv2 = (pSession->meta.nVerticalMCUs + nDiv1 - 1) / nDiv1;
            pSession->nSliceHeight = (qint32)((pSession->meta.nVerticalMCUs + nDiv2 - 1) / nDiv2);
        } else {
            pSession->nSliceHeight = pSession->meta.nVerticalMCUs;
        }
        if (pSession->nSliceHeight <= 0) return false;

        if (!wzjpegAllocateSliceBuffers(pSession)) return false;

        pSession->bSlicesAvailable = true;
        pSession->nFinishedRows = 0;

        pSession->nBitString = 0;
        pSession->nBitLength = 0;
        memset(pSession->nHuffmanPredicted, 0, sizeof(pSession->nHuffmanPredicted));
        pSession->nMCUCounter = 0;
        pSession->nRestartMarkerIndex = 0;

        while (pSession->bSlicesAvailable) {
            if (!wzjpegDecodeSlice(pSession, pPdStruct)) return false;
            if (!wzjpegEncodeSlice(pSession)) return false;
        }
    }

    return wzjpegFlushOutput(pSession, true);
}

}  // namespace

XWinZipJPEGDecoder::XWinZipJPEGDecoder(QObject *parent) : QObject(parent)
{
}

bool XWinZipJPEGDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1)) {
        return false;
    }
    if ((pDecompressState->nProcessedOffset < 0) || (pDecompressState->nProcessedLimit < -1)) {
        pDecompressState->bWriteError = true;
        return false;
    }

    WZJPEG_SESSION *pSession = new (std::nothrow) WZJPEG_SESSION;
    if (!pSession) return false;

    memset((void *)pSession, 0, sizeof(WZJPEG_SESSION));
    wzjpegInputInit(&pSession->input, pDecompressState->pDeviceInput, pDecompressState->nInputOffset, pDecompressState->nInputLimit);
    wzjpegBinInitFixed(&pSession->fixedBin);
    wzjpegInitSignContextTable(pSession);
    pSession->pState = pDecompressState;

    bool bResult = wzjpegProcessStream(pSession, pPdStruct);

    if (pSession->input.bReadError) pDecompressState->bReadError = true;
    if (pSession->bWriteFailed) pDecompressState->bWriteError = true;

    if (bResult) {
        const bool bHasExpectedSize = pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
        const qint64 nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
        if (bHasExpectedSize && (nExpectedSize >= 0) && (pSession->nTotalWritten != nExpectedSize)) bResult = false;
    }

    pDecompressState->nCountInput = pSession->input.nCountInput;

    for (qint32 i = 0; i < 4; i++) {
        if (pSession->pBlocks[i]) free(pSession->pBlocks[i]);
    }
    delete pSession;

    return bResult;
}
