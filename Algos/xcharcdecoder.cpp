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
#include "xcharcdecoder.h"

#include <new>
#include <string.h>

namespace {
// 256 byte contexts, then 0x100 (length / distance high), 0x101 (distance low)
// and 0x102 (the fallback table borrowed by every mode-0 context).
const qint32 CHARC_CONTEXT_COUNT = 0x103;
const qint32 CHARC_LITERAL_CONTEXTS = 0x100;
const qint32 CHARC_CONTEXT_LENGTH = 0x100;
const qint32 CHARC_CONTEXT_DISTANCE_LOW = 0x101;
const qint32 CHARC_CONTEXT_FALLBACK = 0x102;
// Per-context tables are cut out of one flat arena, exactly as the reference
// implementation does; the two budgets below are its own.
const qint32 CHARC_ARENA_SIZE = 0x10000;
const qint32 CHARC_TABLE_BUDGET = 0x400;
const qint32 CHARC_META_BUDGET = 0x279;
const qint32 CHARC_WINDOW_SIZE = 0x10000;
const qint32 CHARC_WINDOW_MASK = 0xffff;
const qint32 CHARC_SYMBOL_COUNT = 0x100;
const qint64 CHARC_MAX_OUTPUT = 0x7fffffff;
// A match is at most 0xff + 7 + 1 bytes, so this is the only overshoot the
// last match of a member can produce.
const qint64 CHARC_OVERSHOOT_LIMIT = 0x200;

// MSB-first out of a 16-bit accumulator topped up one byte at a time.  No
// request is wider than eight bits, which is why one refill is enough.
class BitReader {
public:
    BitReader(const quint8 *pData, qint64 nSize) : m_pData(pData), m_nSize(nSize), m_nPosition(0), m_nBuffer(0), m_nCount(0)
    {
    }

    qint32 readBits(qint32 nBits)
    {
        if ((nBits < 1) || (nBits > 8)) return -1;

        if (m_nCount < nBits) {
            if (m_nPosition >= m_nSize) return -1;
            const quint32 nByte = (quint32)m_pData[m_nPosition];
            m_nPosition++;
            m_nBuffer = (quint16)(m_nBuffer + (quint16)(nByte << (8 - m_nCount)));
            m_nCount += 8;
        }

        const qint32 nValue = (qint32)((m_nBuffer >> (16 - nBits)) & 0xff);
        m_nBuffer = (quint16)(m_nBuffer << nBits);
        m_nCount -= nBits;

        return nValue;
    }

private:
    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint16 m_nBuffer;
    qint32 m_nCount;
};

struct STATE {
    quint8 arrLengths[CHARC_SYMBOL_COUNT];
    quint8 arrMetaTable[CHARC_META_BUDGET];
    quint8 arrContextMode[CHARC_CONTEXT_COUNT];
    quint8 arrEscape[CHARC_CONTEXT_COUNT];
    quint8 arrArena[CHARC_ARENA_SIZE];
    quint8 arrWindow[CHARC_WINDOW_SIZE];
    // -1 means "this context reads a raw eight-bit byte"; anything else is a
    // byte offset into arrArena.
    qint32 arrContextTable[CHARC_CONTEXT_COUNT];
    qint32 nArenaUsed;
    qint32 nWindowPosition;
};

// Lay out one table as [count][symbol x count] blocks, one block per code
// length from zero upwards, and stop as soon as the code space is exhausted.
// Returns the number of bytes written, or 0 when the budget ran out first.
qint32 charcBuildTable(const quint8 *pLengths, quint8 *pDestination, qint32 nCapacity)
{
    if (nCapacity < 1) return 0;

    pDestination[0] = 0;
    qint32 nWritten = 1;
    qint32 nRemainingCapacity = nCapacity - 1;
    quint8 nCurrentLength = 0;
    qint32 nAvailable = 1;
    qint32 nPosition = 1;

    while (true) {
        const qint32 nCountPosition = nPosition;
        nCurrentLength = (quint8)(nCurrentLength + 1);
        nAvailable = (qint32)(qint16)(nAvailable * 2);

        if (nRemainingCapacity < 1) return 0;
        pDestination[nCountPosition] = 0;
        nPosition = nCountPosition + 1;
        nWritten++;
        nRemainingCapacity--;

        qint32 nIndex = 0;
        qint32 nLeft = CHARC_SYMBOL_COUNT;

        while (true) {
            quint8 nLength = 0;

            while (nLeft >= 1) {
                nLeft--;
                nLength = pLengths[nIndex];
                nIndex++;
                if (nLength == nCurrentLength) break;
            }

            if (nLength != nCurrentLength) break;

            if (nRemainingCapacity < 1) return 0;
            pDestination[nPosition] = (quint8)(255 - nLeft);
            nPosition++;
            nWritten++;
            nRemainingCapacity--;
            pDestination[nCountPosition] = (quint8)(pDestination[nCountPosition] + 1);
            nAvailable = (qint32)(qint16)(nAvailable - 1);
            if (nAvailable == 0) return nWritten;
            if (nLeft <= 0) break;
        }
    }
}

// Walk the blocks laid out above.  nAvailable is the number of codes still
// open at the current length and the symbols of that length take the LAST
// `nCount` of them.  nBudget is the format's own accounting; nBytes is the
// hard end of the buffer and exists only so a corrupt table cannot read past
// the arena.
qint32 charcDecodeSymbol(BitReader *pReader, const quint8 *pTable, qint32 nBytes, qint32 nBudget)
{
    qint32 nCode = 0;
    qint32 nAvailable = 1;
    qint32 nOffset = 0;
    qint32 nRemaining = nBudget;

    while (true) {
        if ((nOffset < 0) || (nOffset >= nBytes)) return -1;
        const qint32 nCount = (qint32)pTable[nOffset];

        if ((nAvailable - nCount) <= nCode) {
            nRemaining -= (nCount - nAvailable);
            if (nRemaining < 0) return -1;
            if (nRemaining < (nCode + 1)) return -1;
            const qint32 nIndex = nOffset + nCode + (nCount - nAvailable) + 1;
            if ((nIndex <= nOffset) || (nIndex >= nBytes)) return -1;
            return (qint32)pTable[nIndex];
        }

        const qint32 nBit = pReader->readBits(1);
        if (nBit < 0) return -1;

        nCode = (qint32)(quint16)(nCode * 2 + nBit);
        nAvailable = (qint32)(quint16)((nAvailable - nCount) * 2);
        nRemaining -= (nCount + 1);
        nOffset += (nCount + 1);
        if (nRemaining < 0) return -1;
    }
}

qint32 charcDecodeContext(STATE *pState, BitReader *pReader, qint32 nContext)
{
    if ((nContext < 0) || (nContext >= CHARC_CONTEXT_COUNT)) return -1;

    const qint32 nTable = pState->arrContextTable[nContext];
    if (nTable < 0) return pReader->readBits(8);
    if (nTable >= CHARC_ARENA_SIZE) return -1;

    return charcDecodeSymbol(pReader, pState->arrArena + nTable, CHARC_ARENA_SIZE - nTable, CHARC_TABLE_BUDGET);
}

// One context's table.  bSkipMode2 leaves the length of any symbol whose own
// context mode is 2 at zero instead of spending a meta symbol on it; the two
// distance/length contexts are built without it.
bool charcBuildContext(STATE *pState, BitReader *pReader, qint32 nContext, bool bSkipMode2)
{
    const quint8 nMode = pState->arrContextMode[nContext];

    if (nMode == 3) {
        memset(pState->arrLengths, 0, CHARC_SYMBOL_COUNT);

        for (qint32 i = 0; i < CHARC_SYMBOL_COUNT; i++) {
            bool bDecode = true;
            if (bSkipMode2 && (pState->arrContextMode[i] == 2)) bDecode = false;
            if (bDecode) {
                const qint32 nValue = charcDecodeSymbol(pReader, pState->arrMetaTable, CHARC_META_BUDGET, CHARC_META_BUDGET);
                if (nValue < 0) return false;
                pState->arrLengths[i] = (quint8)nValue;
            }
        }

        if ((pState->nArenaUsed < 0) || (pState->nArenaUsed >= CHARC_ARENA_SIZE)) return false;

        qint32 nCapacity = CHARC_ARENA_SIZE - pState->nArenaUsed;
        if (nCapacity > CHARC_TABLE_BUDGET) nCapacity = CHARC_TABLE_BUDGET;

        const qint32 nWritten = charcBuildTable(pState->arrLengths, pState->arrArena + pState->nArenaUsed, nCapacity);
        if (nWritten <= 0) return false;

        pState->arrContextTable[nContext] = pState->nArenaUsed;
        pState->nArenaUsed += nWritten;
    } else if (nMode != 0) {
        pState->arrContextTable[nContext] = -1;
    }

    return true;
}

bool charcInitModel(STATE *pState, BitReader *pReader)
{
    const qint32 nAllLiteral = pReader->readBits(1);
    if (nAllLiteral < 0) return false;

    if (nAllLiteral == 0) {
        memset(pState->arrLengths, 0, CHARC_SYMBOL_COUNT);

        for (qint32 i = 0; i < 4; i++) {
            const qint32 nValue = pReader->readBits(3);
            if (nValue < 0) return false;
            pState->arrLengths[i] = (quint8)nValue;
        }

        if (charcBuildTable(pState->arrLengths, pState->arrMetaTable, CHARC_META_BUDGET) <= 0) return false;

        for (qint32 i = 0; i < CHARC_CONTEXT_COUNT; i++) {
            const qint32 nValue = charcDecodeSymbol(pReader, pState->arrMetaTable, CHARC_META_BUDGET, CHARC_META_BUDGET);
            if (nValue < 0) return false;
            pState->arrContextMode[i] = (quint8)nValue;
        }
    } else {
        memset(pState->arrContextMode, 1, CHARC_CONTEXT_COUNT);
    }

    const qint32 nHasEscapeTable = pReader->readBits(1);
    if (nHasEscapeTable < 0) return false;

    if (nHasEscapeTable != 0) {
        const qint32 nDefaultEscape = pReader->readBits(8);
        if (nDefaultEscape < 0) return false;
        memset(pState->arrEscape, (quint8)nDefaultEscape, CHARC_LITERAL_CONTEXTS);

        while (true) {
            const qint32 nMore = pReader->readBits(1);
            if (nMore < 0) return false;
            if (nMore == 0) break;

            const qint32 nValue = pReader->readBits(8);
            if (nValue < 0) return false;
            const qint32 nIndex = pReader->readBits(8);
            if (nIndex < 0) return false;
            pState->arrEscape[nIndex] = (quint8)nValue;
        }
    }

    if (nAllLiteral == 0) {
        const qint32 nHighest = pReader->readBits(5);
        if (nHighest < 0) return false;

        for (qint32 i = 0; i <= nHighest; i++) {
            const qint32 nValue = pReader->readBits(4);
            if (nValue < 0) return false;
            pState->arrLengths[i] = (quint8)nValue;
        }

        if (charcBuildTable(pState->arrLengths, pState->arrMetaTable, CHARC_META_BUDGET) <= 0) return false;
    }

    for (qint32 i = 0; i < CHARC_LITERAL_CONTEXTS; i++) {
        if (!charcBuildContext(pState, pReader, i, true)) return false;
    }
    for (qint32 i = CHARC_CONTEXT_LENGTH; i < CHARC_CONTEXT_FALLBACK; i++) {
        if (!charcBuildContext(pState, pReader, i, false)) return false;
    }
    if (!charcBuildContext(pState, pReader, CHARC_CONTEXT_FALLBACK, true)) return false;

    for (qint32 i = 0; i < CHARC_CONTEXT_FALLBACK; i++) {
        if (pState->arrContextMode[i] == 0) pState->arrContextTable[i] = pState->arrContextTable[CHARC_CONTEXT_FALLBACK];
    }

    return true;
}

void charcEmit(STATE *pState, QByteArray *pbaOut, quint8 nByte)
{
    pbaOut->append((char)nByte);
    pState->arrWindow[pState->nWindowPosition] = nByte;
    pState->nWindowPosition = (pState->nWindowPosition + 1) & CHARC_WINDOW_MASK;
}

bool charcDecodeStream(STATE *pState, const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaOut, XBinary::PDSTRUCT *pPdStruct)
{
    BitReader reader((const quint8 *)baPacked.constData(), baPacked.size());

    if (!charcInitModel(pState, &reader)) return false;

    const qint32 nLengthBias = reader.readBits(3);
    if (nLengthBias < 0) return false;

    const qint32 nFirstByte = reader.readBits(8);
    if (nFirstByte < 0) return false;

    charcEmit(pState, pbaOut, (quint8)nFirstByte);
    qint32 nContext = nFirstByte;

    while ((qint64)pbaOut->size() < nUncompressedSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const qint32 nSymbol = charcDecodeContext(pState, &reader, nContext);
        if (nSymbol < 0) return false;

        if (nSymbol != (qint32)pState->arrEscape[nContext & 0xff]) {
            charcEmit(pState, pbaOut, (quint8)nSymbol);
            nContext = nSymbol;
            continue;
        }

        const qint32 nLengthCode = charcDecodeContext(pState, &reader, CHARC_CONTEXT_LENGTH);
        if (nLengthCode < 0) return false;

        if (nLengthCode == 0) {
            charcEmit(pState, pbaOut, (quint8)nSymbol);
            nContext = nSymbol;
            continue;
        }

        qint32 nLength = (qint32)(qint16)(nLengthCode + nLengthBias + 1);

        const qint32 nDistanceLow = charcDecodeContext(pState, &reader, CHARC_CONTEXT_DISTANCE_LOW);
        if (nDistanceLow < 0) return false;
        const qint32 nDistanceHigh = charcDecodeContext(pState, &reader, CHARC_CONTEXT_LENGTH);
        if (nDistanceHigh < 0) return false;

        const qint32 nDistance = (nDistanceLow + nDistanceHigh * 256) & CHARC_WINDOW_MASK;

        while (nLength > 0) {
            const quint8 nByte = pState->arrWindow[(pState->nWindowPosition - nDistance) & CHARC_WINDOW_MASK];
            charcEmit(pState, pbaOut, nByte);
            nContext = (qint32)nByte;
            nLength--;
            if ((qint64)pbaOut->size() > (nUncompressedSize + CHARC_OVERSHOOT_LIMIT)) return false;
        }
    }

    return true;
}
}  // namespace

bool XChArcDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if ((nUncompressedSize < 0) || (nUncompressedSize > CHARC_MAX_OUTPUT)) return false;
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    STATE *pState = new (std::nothrow) STATE;
    if (!pState) return false;

    memset(pState, 0, sizeof(STATE));
    for (qint32 i = 0; i < CHARC_CONTEXT_COUNT; i++) pState->arrContextTable[i] = -1;

    QByteArray baOut;
    baOut.reserve((qint32)nUncompressedSize);

    const bool bDecoded = charcDecodeStream(pState, baPacked, nUncompressedSize, &baOut, pPdStruct);

    delete pState;

    if (!bDecoded) return false;
    if ((qint64)baOut.size() > nUncompressedSize) baOut.truncate((qint32)nUncompressedSize);

    *pbaResult = baOut;

    return (qint64)pbaResult->size() == nUncompressedSize;
}
