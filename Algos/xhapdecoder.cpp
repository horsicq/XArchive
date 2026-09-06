/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xhapdecoder.h"

namespace {

// The model is one flat block, addressed exactly the way the original does it.
// Keeping the byte offsets rather than inventing named arrays is deliberate:
// several of the regions are aliased (the count table is the symbol table plus
// 0x8000, and the tree's two child arrays share memory with the position map),
// and every index is computed with 16-bit wrap-around.
const qint32 HAP_STATE_SIZE = 0x50600;

const qint32 HAP_OFF_LISTCOUNT = 0x2c;      // symbols in the context being scanned
const qint32 HAP_OFF_LISTSTART = 0x2e;      // arena index of that context
const qint32 HAP_OFF_STACKPTR = 0x30;       // cumulative-frequency stack pointer
const qint32 HAP_OFF_SAVE32 = 0x32;
const qint32 HAP_OFF_SAVE34 = 0x34;
const qint32 HAP_OFF_SAVE36 = 0x36;
const qint32 HAP_OFF_SAVE38 = 0x38;
const qint32 HAP_OFF_MARKER = 0x3b;         // per level: 0 coded, -1 new, -4 escape, -6 grow
const qint32 HAP_OFF_SYMBOL = 0x45;         // the byte just decoded
const qint32 HAP_OFF_LEVELSTART = 0x46;     // per level: arena index
const qint32 HAP_OFF_LEVEL = 0x50;          // 8, 6, 4, 2, 0
const qint32 HAP_OFF_LEVELCOUNT = 0x53;     // per level: symbol count
const qint32 HAP_OFF_BITCOUNT = 0x5d;
const qint32 HAP_OFF_CUMLOW = 0x5e;
const qint32 HAP_OFF_TOTAL = 0x60;
const qint32 HAP_OFF_CUMHIGH = 0x62;
const qint32 HAP_OFF_EXCLTOP = 0x64;        // next free slot in the exclusion log
const qint32 HAP_OFF_NEXTNODE = 0x68;       // next context id, stepped by 2
const qint32 HAP_OFF_SAVEA3 = 0x75;         // per level: previous node
const qint32 HAP_OFF_NODE = 0x7f;           // per level: current context node
const qint32 HAP_OFF_SAVEAD = 0x6b;         // per level: previous symbol slot
const qint32 HAP_OFF_MATCHPOS = 0x22;       // per level: symbol slot of the decoded byte
const qint32 HAP_OFF_STACK = 0x89;          // cumulative-frequency stack base
const qint32 HAP_OFF_INBYTE = 0x10089;
const qint32 HAP_OFF_VALUE = 0x1008a;
const qint32 HAP_OFF_LOW = 0x1008c;
const qint32 HAP_OFF_HIGH = 0x1008e;
const qint32 HAP_OFF_EXCL = 0x10090;        // 0xff allowed, 0 excluded
const qint32 HAP_OFF_CHILD = 0x102e0;       // per symbol slot: child context, 16 bit
const qint32 HAP_OFF_SYM = 0x202e0;         // arena: symbol bytes
const qint32 HAP_OFF_CNT = 0x282e0;         // arena: count bytes (SYM + 0x8000)
const qint32 HAP_OFF_NODESTART = 0x302e0;   // per node: arena index
const qint32 HAP_OFF_NODECOUNT = 0x402e0;   // per node: symbol count minus one
const qint32 HAP_OFF_FREELIST = 0x502e0;    // free-list head per size class

const qint64 HAP_MAX_UNPACKED_SIZE = qint64(512) * 1024 * 1024;

class HapModel {
public:
    HapModel(const quint8 *pInput, qint64 nInputSize) : m_baState(HAP_STATE_SIZE, char(0)), m_pInput(pInput), m_nInputSize(nInputSize), m_nInputPos(0)
    {
        m_s = reinterpret_cast<quint8 *>(m_baState.data());
    }

    bool decode(qint64 nUncompressedSize, QByteArray *pbaOut);

private:
    quint32 g8(qint32 nOffset) const
    {
        return m_s[nOffset];
    }
    void p8(qint32 nOffset, quint32 nValue)
    {
        m_s[nOffset] = static_cast<quint8>(nValue & 0xff);
    }
    quint32 g16(qint32 nOffset) const
    {
        return static_cast<quint32>(m_s[nOffset]) | (static_cast<quint32>(m_s[nOffset + 1]) << 8);
    }
    qint32 gi16(qint32 nOffset) const
    {
        const quint32 nValue = g16(nOffset);
        return (nValue >= 0x8000) ? static_cast<qint32>(nValue) - 0x10000 : static_cast<qint32>(nValue);
    }
    void p16(qint32 nOffset, quint32 nValue)
    {
        nValue &= 0xffff;
        m_s[nOffset] = static_cast<quint8>(nValue & 0xff);
        m_s[nOffset + 1] = static_cast<quint8>(nValue >> 8);
    }
    quint32 g32(qint32 nOffset) const
    {
        return static_cast<quint32>(m_s[nOffset]) | (static_cast<quint32>(m_s[nOffset + 1]) << 8) | (static_cast<quint32>(m_s[nOffset + 2]) << 16) |
               (static_cast<quint32>(m_s[nOffset + 3]) << 24);
    }
    void p32(qint32 nOffset, quint32 nValue)
    {
        m_s[nOffset] = static_cast<quint8>(nValue & 0xff);
        m_s[nOffset + 1] = static_cast<quint8>((nValue >> 8) & 0xff);
        m_s[nOffset + 2] = static_cast<quint8>((nValue >> 16) & 0xff);
        m_s[nOffset + 3] = static_cast<quint8>((nValue >> 24) & 0xff);
    }

    void push(quint32 nValue)
    {
        const quint32 nStackPtr = (g16(HAP_OFF_STACKPTR) - 2) & 0xffff;
        p16(HAP_OFF_STACKPTR, nStackPtr);
        p16(HAP_OFF_STACK + static_cast<qint32>(nStackPtr), nValue);
    }

    void init();
    bool arithUpdate();
    void fixupEscape();
    qint32 newContext();
    qint32 growContext();
    qint32 decodeSymbol();
    qint32 tail(qint32 nResult, quint32 nExtra);

    QByteArray m_baState;
    quint8 *m_s;
    const quint8 *m_pInput;
    qint64 m_nInputSize;
    qint64 m_nInputPos;
};

void HapModel::init()
{
    qint32 u = 0;
    for (qint32 i = 0; i < 0x100; i++) {
        p16(HAP_OFF_FREELIST + u, 0xffff);
        u += 2;
    }
    p16(HAP_OFF_FREELIST + u, 0x300);  // u == 0x200
    p16(HAP_OFF_FREELIST + 2, 0x203);

    p16(HAP_OFF_NODECOUNT + 0, 0xff);
    p16(HAP_OFF_NODECOUNT + 2, 0xff);
    p16(HAP_OFF_NODECOUNT + 4, 0);
    p16(HAP_OFF_NODECOUNT + 6, 0);
    p16(HAP_OFF_NODECOUNT + 8, 0);

    p16(HAP_OFF_NODESTART + 0, 0);
    p16(HAP_OFF_NODESTART + 2, 0x100);
    p16(HAP_OFF_NODESTART + 4, 0x200);
    p16(HAP_OFF_NODESTART + 6, 0x201);
    p16(HAP_OFF_NODESTART + 8, 0x202);

    quint32 c = 0;
    for (qint32 i = 0; i < 0x200; i++) {
        p8(HAP_OFF_SYM + i, c);
        c = (c + 1) & 0xff;
    }
    p8(HAP_OFF_SYM + 0x200, 0x68);
    p8(HAP_OFF_SYM + 0x201, 0x65);
    p8(HAP_OFF_SYM + 0x202, 0x65);

    u = 0;
    for (qint32 i = 0; i < 0x80; i++) {
        p16(HAP_OFF_CNT + u, 0x101);
        u = (u + 2) & 0xffff;
    }
    qint32 nPrevious = u;
    for (qint32 i = 0; i < 0x80; i++) {
        nPrevious = u;
        p16(HAP_OFF_CNT + u, 0);
        u = (u + 2) & 0xffff;
    }
    p16(HAP_OFF_CNT + u, 0);
    p8(HAP_OFF_CNT + ((nPrevious + 4) & 0xffff), 0);

    u = 0x200;
    for (qint32 i = 0; i < 0x100; i++) {
        p16(HAP_OFF_CHILD + u, 0xffff);
        u += 2;
    }
    p16(HAP_OFF_CHILD + 0x2d0, 4);
    p16(HAP_OFF_CHILD + 0x2e8, 2);
    p16(HAP_OFF_CHILD + 0x400, 3);
    p16(HAP_OFF_CHILD + 0x402, 0xffff);
    p16(HAP_OFF_CHILD + 0x404, 0xffff);

    u = 0x406;
    quint32 v = 0x204;
    for (qint32 i = 0; i < 0xfc; i++) {
        p16(HAP_OFF_CHILD + u, v);
        v++;
        u += 2;
    }
    p16(HAP_OFF_CHILD + u, 0xffff);  // u == 0x600

    u = 0x600;
    v = 0x400;
    for (qint32 i = 0; i < 0x7c; i++) {
        p16(HAP_OFF_CHILD + u, v);
        v = (v + 0x100) & 0xffff;
        u = (u + 0x200) & 0xffff;
    }
    p16(HAP_OFF_CHILD + u, 0xffff);

    u = 0;
    for (qint32 i = 0; i < 0x80; i++) {
        p16(HAP_OFF_EXCL + u, 0xffff);
        u += 2;
    }

    p16(HAP_OFF_NODE + 0, 0);
    p16(HAP_OFF_NODE + 2, 1);
    p16(HAP_OFF_NODE + 4, 0xffff);
    p16(HAP_OFF_NODE + 6, 0xffff);
    p16(HAP_OFF_NODE + 8, 0xffff);
    p16(HAP_OFF_SAVEA3 + 2, 1);
    p16(HAP_OFF_SAVEA3 + 4, 3);
    p16(HAP_OFF_SAVEA3 + 6, 4);
    p16(HAP_OFF_SAVEAD + 2, 0x65);
    p16(HAP_OFF_SAVEAD + 4, 0);
    p16(HAP_OFF_SAVEAD + 6, 0);
    p16(HAP_OFF_NEXTNODE, 10);
    p32(HAP_OFF_EXCLTOP, 0x100);
}

// Witten-Neal-Cleary interval narrowing plus renormalisation, including the
// underflow (E3) case.  Returns false only when the input runs out.
bool HapModel::arithUpdate()
{
    const quint32 nLow = g16(HAP_OFF_LOW);
    const quint32 nHigh = g16(HAP_OFF_HIGH);
    const quint32 nTotal = g16(HAP_OFF_TOTAL);
    const quint32 nCumHigh = g16(HAP_OFF_CUMHIGH);
    const quint32 nCumLow = g16(HAP_OFF_CUMLOW);
    if (nTotal == 0) return false;

    const quint32 nRange = (nHigh - nLow) & 0xffff;
    quint32 nNewLow = nLow;
    quint32 nNewHigh = nHigh;
    if (nCumHigh != nTotal) {
        nNewHigh = (((nRange * nCumHigh + nCumHigh) / nTotal) + nLow - 1) & 0xffff;
    }
    if (nCumLow != 0) {
        nNewLow = (nLow + ((nRange * nCumLow + nCumLow) / nTotal)) & 0xffff;
    }
    p16(HAP_OFF_LOW, nNewLow);
    p16(HAP_OFF_HIGH, nNewHigh);

    while (true) {
        if ((g8(HAP_OFF_HIGH + 1) ^ g8(HAP_OFF_LOW + 1)) & 0x80) {
            if (g16(HAP_OFF_HIGH) & 0x4000) return true;
            const quint32 nMask = g8(HAP_OFF_LOW + 1) & 0x40;
            if (nMask == 0) return true;
            p8(HAP_OFF_VALUE + 1, g8(HAP_OFF_VALUE + 1) ^ nMask);
            p8(HAP_OFF_HIGH + 1, g8(HAP_OFF_HIGH + 1) | nMask);
            p8(HAP_OFF_LOW + 1, g8(HAP_OFF_LOW + 1) & 0x3f);
        }
        p16(HAP_OFF_LOW, g16(HAP_OFF_LOW) * 2);
        p16(HAP_OFF_HIGH, g16(HAP_OFF_HIGH) * 2 + 1);
        if (g8(HAP_OFF_BITCOUNT) == 0) {
            p8(HAP_OFF_BITCOUNT, 8);
            if (m_nInputPos >= m_nInputSize) return false;
            p8(HAP_OFF_INBYTE, m_pInput[m_nInputPos++]);
        }
        p8(HAP_OFF_BITCOUNT, g8(HAP_OFF_BITCOUNT) - 1);
        p16(HAP_OFF_VALUE, g16(HAP_OFF_VALUE) * 2 + (g8(HAP_OFF_INBYTE) >> 7));
        p8(HAP_OFF_INBYTE, g8(HAP_OFF_INBYTE) * 2);
    }
}

// After a symbol was coded in a shorter context, bump its count in the level we
// escaped from - or mark that level as "needs to grow" when the symbol is not
// in it at all.
void HapModel::fixupEscape()
{
    const qint32 nLevel = static_cast<qint32>(g16(HAP_OFF_LEVEL));
    qint32 nCount = gi16(HAP_OFF_LEVELCOUNT + nLevel);
    quint32 nIndex = g16(HAP_OFF_LEVELSTART + nLevel);
    while (true) {
        if (nCount == 0) {
            p16(HAP_OFF_MARKER + nLevel, 0xfffa);
            return;
        }
        if (g8(HAP_OFF_SYMBOL) == g8(HAP_OFF_SYM + static_cast<qint32>(nIndex))) break;
        nIndex = (nIndex + 1) & 0xffff;
        nCount--;
    }
    p8(HAP_OFF_CNT + static_cast<qint32>(nIndex), g8(HAP_OFF_CNT + static_cast<qint32>(nIndex)) + 1);
    p16(HAP_OFF_MARKER + nLevel, 0);
}

// Allocate a brand new one-symbol context.  Returns the node id, or -1 when the
// arena is exhausted (the caller then resets the whole model).
qint32 HapModel::newContext()
{
    const quint32 nNode = g16(HAP_OFF_NEXTNODE);
    p16(HAP_OFF_NEXTNODE, nNode + 2);
    if (nNode == 0xffff) return -1;

    const quint32 nHead = g16(HAP_OFF_FREELIST + 2);
    if (nHead == 0xffff) {
        qint32 u = 4;
        for (qint32 k = 0xff; k != 0; k--) {
            if (gi16(HAP_OFF_FREELIST + u) != -1) {
                const quint32 nBlock = g16(HAP_OFF_FREELIST + u);
                p16(HAP_OFF_FREELIST + u - 2, nBlock + 1);
                p8(HAP_OFF_SYM + static_cast<qint32>(nBlock), g8(HAP_OFF_SYMBOL));
                p8(HAP_OFF_CNT + static_cast<qint32>(nBlock), 1);
                p16(HAP_OFF_NODESTART + static_cast<qint32>(nNode), nBlock);
                p16(HAP_OFF_NODECOUNT + static_cast<qint32>(nNode), 0);
                const qint32 nBlock2 = static_cast<qint32>((nBlock * 2) & 0xffff);
                const quint32 nNext = g16(HAP_OFF_CHILD + nBlock2);
                p16(HAP_OFF_CHILD + nBlock2, 0xffff);
                p16(HAP_OFF_CHILD + nBlock2 + 2, 0xffff);
                p16(HAP_OFF_FREELIST + u, nNext);
                return static_cast<qint32>(nNode >> 1);
            }
            u = (u + 2) & 0xffff;
        }
        return -1;
    }

    p8(HAP_OFF_SYM + static_cast<qint32>(nHead), g8(HAP_OFF_SYMBOL));
    p8(HAP_OFF_CNT + static_cast<qint32>(nHead), 1);
    p16(HAP_OFF_NODESTART + static_cast<qint32>(nNode), nHead);
    p16(HAP_OFF_NODECOUNT + static_cast<qint32>(nNode), 0);
    const qint32 nHead2 = static_cast<qint32>((nHead * 2) & 0xffff);
    const quint32 nNext = g16(HAP_OFF_CHILD + nHead2);
    p16(HAP_OFF_CHILD + nHead2, 0xffff);
    p16(HAP_OFF_FREELIST + 2, nNext);
    return static_cast<qint32>(nNode >> 1);
}

// Move the current context to a one-slot-larger arena block and append the new
// symbol.  Returns the old symbol count, or -1 when no block is available.
qint32 HapModel::growContext()
{
    const qint32 nLevel = static_cast<qint32>(g16(HAP_OFF_LEVEL));
    const qint32 nNode2 = static_cast<qint32>((gi16(HAP_OFF_NODE + nLevel) * 2) & 0xffff);
    p16(HAP_OFF_SAVE38, static_cast<quint32>(nNode2));
    qint32 nCount = gi16(HAP_OFF_NODECOUNT + nNode2);
    p16(HAP_OFF_NODECOUNT + nNode2, static_cast<quint32>(nCount + 1));
    qint32 k = 0xff - nCount;
    nCount = nCount + 2;
    const quint32 nSizeClass = static_cast<quint32>((nCount * 2) & 0xffff);

    bool bEmpty = false;
    qint32 u = static_cast<qint32>(nSizeClass);
    while (true) {
        if (k == 0) break;
        if (u > 0x210) return -1;
        bEmpty = (gi16(HAP_OFF_FREELIST + u) == -1);
        u = static_cast<qint32>((u + 2) & 0xffff);
        k--;
        if (!bEmpty) break;
    }
    if (bEmpty) return -1;

    u = static_cast<qint32>((u - 2) & 0xffff);
    if (u >= 0x211) return -1;

    const quint32 nBlock = g16(HAP_OFF_FREELIST + u);
    p16(HAP_OFF_FREELIST + u, g16(HAP_OFF_CHILD + static_cast<qint32>((nBlock * 2) & 0xffff)));
    const qint32 nRemainder = static_cast<qint32>((u - static_cast<qint32>(nSizeClass)) & 0xffff);
    if (nRemainder != 0) {
        const qint32 nTail = static_cast<qint32>((nBlock * 2 + nSizeClass) & 0xffff);
        quint32 nTemp = static_cast<quint32>(nTail) >> 1;
        if (nRemainder > 0x210) return -1;
        const quint32 nOther = g16(HAP_OFF_FREELIST + nRemainder);
        p16(HAP_OFF_FREELIST + nRemainder, nTemp);
        nTemp = nOther;
        p16(HAP_OFF_CHILD + nTail, nTemp);
    }

    qint32 n = static_cast<qint32>(nSizeClass >> 1) - 1;
    quint32 nOldStart = g16(HAP_OFF_NODESTART + static_cast<qint32>(g16(HAP_OFF_SAVE38)));
    p16(HAP_OFF_NODESTART + static_cast<qint32>(g16(HAP_OFF_SAVE38)), nBlock);

    if (static_cast<quint32>((n * 2) & 0xffff) >= 0x211) return -1;
    quint32 nSpare = g16(HAP_OFF_FREELIST + static_cast<qint32>((n * 2) & 0xffff));
    p16(HAP_OFF_FREELIST + static_cast<qint32>((n * 2) & 0xffff), nOldStart);

    p16(HAP_OFF_SAVE36, nBlock);
    p16(HAP_OFF_SAVE34, nOldStart);
    p16(HAP_OFF_SAVE32, static_cast<quint32>(n));

    quint32 a = nBlock;
    quint32 b = nOldStart;
    for (qint32 m = n; m != 0; m--) {
        p8(HAP_OFF_SYM + static_cast<qint32>(a), g8(HAP_OFF_SYM + static_cast<qint32>(b)));
        a = (a + 1) & 0xffff;
        b = (b + 1) & 0xffff;
    }
    p8(HAP_OFF_SYM + static_cast<qint32>(a), g8(HAP_OFF_SYMBOL));

    a = static_cast<quint32>((gi16(HAP_OFF_SAVE36) + 0x8000) & 0xffff);
    b = static_cast<quint32>((gi16(HAP_OFF_SAVE34) + 0x8000) & 0xffff);
    for (qint32 m = gi16(HAP_OFF_SAVE32); m != 0; m--) {
        p8(HAP_OFF_SYM + static_cast<qint32>(a), g8(HAP_OFF_SYM + static_cast<qint32>(b)));
        a = (a + 1) & 0xffff;
        b = (b + 1) & 0xffff;
    }
    p8(HAP_OFF_SYM + static_cast<qint32>(a), 1);

    quint32 nSource = static_cast<quint32>((gi16(HAP_OFF_SAVE34) * 2) & 0xffff);
    quint32 nDest = static_cast<quint32>((gi16(HAP_OFF_SAVE36) * 2) & 0xffff);
    const quint32 nResult = g16(HAP_OFF_SAVE32);
    for (quint32 m = nResult; m != 0; m--) {
        p16(HAP_OFF_CHILD + static_cast<qint32>(nDest), g16(HAP_OFF_CHILD + static_cast<qint32>(nSource)));
        nDest = (nDest + 2) & 0xffff;
        nSource = (nSource + 2) & 0xffff;
    }
    p16(HAP_OFF_CHILD + static_cast<qint32>(nDest), 0xffff);
    p16(HAP_OFF_CHILD + static_cast<qint32>((gi16(HAP_OFF_SAVE34) * 2) & 0xffff), nSpare);
    return static_cast<qint32>(nResult);
}

// Returns 0 when a symbol was decoded into HAP_OFF_SYMBOL, -4 on an escape and
// -9 when the context has no codable symbol left.
qint32 HapModel::decodeSymbol()
{
    p16(HAP_OFF_STACKPTR, 0x8000);
    const qint32 nLevel = static_cast<qint32>(g16(HAP_OFF_LEVEL));
    const qint32 nNode2 = static_cast<qint32>((gi16(HAP_OFF_NODE + nLevel) * 2) & 0xffff);
    qint32 nCount = gi16(HAP_OFF_NODECOUNT + nNode2);
    const qint32 nStart = gi16(HAP_OFF_NODESTART + nNode2);
    p16(HAP_OFF_LEVELSTART + nLevel, static_cast<quint32>(nStart));
    p16(HAP_OFF_LISTSTART, static_cast<quint32>(nStart));
    quint32 nIndex = static_cast<quint32>((nStart + nCount) & 0xffff);
    quint32 nListCount = static_cast<quint32>((nCount + 1) & 0xffff);
    p16(HAP_OFF_LISTCOUNT, nListCount);
    p16(HAP_OFF_LEVELCOUNT + nLevel, nListCount);

    quint32 nTotal = 0;
    quint32 nSymbols = 0;
    bool bZero = false;
    while (true) {
        nSymbols = 0;
        nTotal = 0;
        quint32 n = nListCount;
        while (true) {
            push(nTotal);
            const quint32 nSymbol = g8(HAP_OFF_SYM + static_cast<qint32>(nIndex));
            const quint32 nWeight = g8(HAP_OFF_EXCL + static_cast<qint32>(nSymbol)) & g8(HAP_OFF_SYM + static_cast<qint32>((nIndex + 0x8000) & 0xffff));
            nIndex = (nIndex - 1) & 0xffff;
            if (nWeight != 0) {
                nSymbols = (nSymbols + 1) & 0xffff;
                nTotal = (nTotal + nWeight) & 0xffff;
            }
            n = (n - 1) & 0xffff;
            if (n == 0) break;
        }
        push(nTotal);
        if (nTotal == 0) {
            bZero = true;
            break;
        }
        if (g16(HAP_OFF_LEVEL) == 0) break;
        const quint32 nWithEscape = (nTotal + nSymbols) & 0xffff;
        quint32 nAdjusted = nWithEscape;
        if ((nWithEscape & 1) && (nSymbols != 1)) nAdjusted = (nWithEscape - 1) & 0xffff;
        if (nAdjusted < 0x3fff) {
            nTotal = nAdjusted;
            break;
        }
        // Halve every count in this context and rebuild the frequency stack.
        qint32 m = gi16(HAP_OFF_LISTCOUNT);
        p16(HAP_OFF_STACKPTR, g16(HAP_OFF_STACKPTR) + static_cast<quint32>(m * 2));
        quint32 p = g16(HAP_OFF_LISTSTART);
        while (m != 0) {
            p8(HAP_OFF_CNT + static_cast<qint32>(p), g8(HAP_OFF_CNT + static_cast<qint32>(p)) >> 1);
            p = (p + 1) & 0xffff;
            m--;
        }
        nIndex = (p - 1) & 0xffff;
        nListCount = g16(HAP_OFF_LISTCOUNT);
    }

    if (bZero) return tail(-9, 0);

    push(nTotal);
    p16(HAP_OFF_TOTAL, nTotal);

    const quint32 nSpan = (g16(HAP_OFF_VALUE) - g16(HAP_OFF_LOW) + 1) & 0xffff;
    quint32 nHi = 0;
    quint32 nLo = 0;
    if (nSpan == 0) {
        nHi = (nTotal - 1) & 0xffff;
        nLo = 0xffff;
    } else {
        const quint32 nProduct = (nSpan * nTotal - 1) & 0xffffffff;
        nLo = nProduct & 0xffff;
        nHi = (nProduct >> 16) & 0xffff;
    }
    const quint32 nDenominator = (g16(HAP_OFF_HIGH) - g16(HAP_OFF_LOW) + 1) & 0xffff;
    quint32 nTarget = 0;
    if (nDenominator == 0) {
        nTarget = nHi & 0xffff;
    } else {
        nTarget = (((nHi << 16) + nLo) / nDenominator) & 0xffff;
    }

    quint32 nCursor = g16(HAP_OFF_STACKPTR);
    quint32 nSteps = 0;
    while (true) {
        nSteps = (nSteps + 1) & 0xffff;
        nCursor = (nCursor + 2) & 0xffff;
        if (!(nTarget < g16(HAP_OFF_STACK - 2 + static_cast<qint32>(nCursor)))) break;
    }
    nCursor = (nCursor - 4) & 0xffff;
    p16(HAP_OFF_CUMHIGH, g16(HAP_OFF_STACK + static_cast<qint32>(nCursor)));
    p16(HAP_OFF_CUMLOW, g16(HAP_OFF_STACK + 2 + static_cast<qint32>(nCursor)));

    if (nCursor == g16(HAP_OFF_STACKPTR)) {
        // Escape: exclude everything this context could have coded, logging the
        // symbols so the exclusions can be undone before the next byte.
        quint32 r = g16(HAP_OFF_EXCLTOP);
        qint32 m = static_cast<qint32>(g16(HAP_OFF_LISTCOUNT));
        quint32 p = g16(HAP_OFF_LISTSTART);
        while (m != 0) {
            if (g8(HAP_OFF_CNT + static_cast<qint32>(p)) != 0) {
                const quint32 nSymbol = g8(HAP_OFF_SYM + static_cast<qint32>(p));
                if (g8(HAP_OFF_EXCL + static_cast<qint32>(nSymbol)) != 0) {
                    p8(HAP_OFF_EXCL + static_cast<qint32>(r), nSymbol);
                    p8(HAP_OFF_EXCL + static_cast<qint32>(nSymbol), 0);
                    r = (r + 1) & 0xffff;
                }
            }
            p = (p + 1) & 0xffff;
            m--;
        }
        p32(HAP_OFF_EXCLTOP, r);
        return tail(-4, 1);
    }

    const quint32 nPosition = ((nSteps - 3) + g16(HAP_OFF_LISTSTART)) & 0xffff;
    p8(HAP_OFF_SYMBOL, g8(HAP_OFF_SYM + static_cast<qint32>(nPosition)));
    if (g8(HAP_OFF_CNT + static_cast<qint32>(nPosition)) == 0xff) {
        qint32 m = gi16(HAP_OFF_LISTCOUNT);
        quint32 q = g16(HAP_OFF_LISTSTART);
        while (m != 0) {
            p8(HAP_OFF_CNT + static_cast<qint32>(q), g8(HAP_OFF_CNT + static_cast<qint32>(q)) >> 1);
            q = (q + 1) & 0xffff;
            m--;
        }
    }
    p8(HAP_OFF_CNT + static_cast<qint32>(nPosition), g8(HAP_OFF_CNT + static_cast<qint32>(nPosition)) + 1);
    return tail(0, 1);
}

// Unwinds the frequency stack.  The original also uses the restored pointer as
// a sanity check and downgrades the result to "symbol coded" when it does not
// come back to 0x8000; that behaviour is reproduced verbatim.
qint32 HapModel::tail(qint32 nResult, quint32 nExtra)
{
    quint32 v = (nExtra + static_cast<quint32>(gi16(HAP_OFF_LISTCOUNT)) + 1) & 0xffff;
    v = (v * 2) & 0xffff;
    p16(HAP_OFF_STACKPTR, g16(HAP_OFF_STACKPTR) + v);
    if (g16(HAP_OFF_STACKPTR) != 0x8000) nResult = 0;
    return nResult;
}

bool HapModel::decode(qint64 nUncompressedSize, QByteArray *pbaOut)
{
    if (nUncompressedSize == 0) return true;
    if (m_nInputPos + 2 > m_nInputSize) return false;
    const quint32 nPrimed = static_cast<quint32>(m_pInput[m_nInputPos]) | (static_cast<quint32>(m_pInput[m_nInputPos + 1]) << 8);
    m_nInputPos += 2;
    p16(HAP_OFF_VALUE, ((nPrimed & 0xff) << 8) | (nPrimed >> 8));
    p8(HAP_OFF_BITCOUNT, 0);
    p16(HAP_OFF_LOW, 0);
    p16(HAP_OFF_HIGH, 0xffff);
    init();

    qint64 nRemaining = nUncompressedSize;
    while (nRemaining > 0) {
        p16(HAP_OFF_LEVEL, 8);

        quint32 u = 0x100;
        qint32 n = static_cast<qint32>(g32(HAP_OFF_EXCLTOP)) - 0x100;
        if (n != 0) {
            while (n > 0) {
                const quint32 t = g8(HAP_OFF_EXCL + static_cast<qint32>(u));
                p8(HAP_OFF_EXCL + static_cast<qint32>(t), g8(HAP_OFF_EXCL + static_cast<qint32>(t)) - 1);
                u = (u + 1) & 0xffff;
                n--;
            }
            p32(HAP_OFF_EXCLTOP, 0x100);
        }
        // The order-0 context always exists, so this walk always stops; the
        // counter only keeps a corrupt state from indexing off the block.
        for (qint32 nGuard = 0; nGuard < 5; nGuard++) {
            if (gi16(HAP_OFF_NODE + static_cast<qint32>(g16(HAP_OFF_LEVEL))) != -1) break;
            p16(HAP_OFF_MARKER + static_cast<qint32>(g16(HAP_OFF_LEVEL)), 0xffff);
            if (g16(HAP_OFF_LEVEL) == 0) return false;
            p16(HAP_OFF_LEVEL, g16(HAP_OFF_LEVEL) - 2);
        }

        while (true) {
            const qint32 r = decodeSymbol();
            if (r == 0) break;
            if (r != -9) {
                if (!arithUpdate()) return false;
            }
            p16(HAP_OFF_MARKER + static_cast<qint32>(g16(HAP_OFF_LEVEL)), 0xfffc);
            if (g16(HAP_OFF_LEVEL) == 0) return false;
            p16(HAP_OFF_LEVEL, g16(HAP_OFF_LEVEL) - 2);
        }

        if (!arithUpdate()) return false;
        p16(HAP_OFF_MARKER + static_cast<qint32>(g16(HAP_OFF_LEVEL)), 0);
        pbaOut->append(static_cast<char>(g8(HAP_OFF_SYMBOL)));
        nRemaining--;

        bool bRestart = false;
        if (g16(HAP_OFF_LEVEL) != 8) {
            while (true) {
                const qint32 nLevel = static_cast<qint32>(g16(HAP_OFF_LEVEL));
                if (gi16(HAP_OFF_MARKER + nLevel) == -4) fixupEscape();
                if (gi16(HAP_OFF_MARKER + nLevel) == -1) {
                    const qint32 r = newContext();
                    if (r < 0) {
                        init();
                        bRestart = true;
                        break;
                    }
                    p16(HAP_OFF_NODE + nLevel, static_cast<quint32>(r));
                    const qint32 nPrevious = (nLevel - 2) & 0xffff;
                    const qint32 a = gi16(HAP_OFF_SAVEA3 + nPrevious);
                    const qint32 b = gi16(HAP_OFF_SAVEAD + nPrevious);
                    const qint32 nOffset = gi16(HAP_OFF_NODESTART + static_cast<qint32>((a * 2) & 0xffff));
                    p16(HAP_OFF_CHILD + static_cast<qint32>(((nOffset + b) * 2) & 0xffff), static_cast<quint32>(r));
                } else if (gi16(HAP_OFF_MARKER + nLevel) == -6) {
                    if (growContext() < 0) {
                        init();
                        bRestart = true;
                        break;
                    }
                }
                p16(HAP_OFF_LEVEL, g16(HAP_OFF_LEVEL) + 2);
                if (!(g16(HAP_OFF_LEVEL) < 9)) break;
            }
            if (bRestart) continue;
        }

        const quint32 nSymbol = g8(HAP_OFF_SYMBOL);
        p16(HAP_OFF_MATCHPOS + 2, nSymbol);
        u = 4;
        while (true) {
            const quint32 nBase = g16(HAP_OFF_NODESTART + static_cast<qint32>((gi16(HAP_OFF_NODE + static_cast<qint32>(u)) * 2) & 0xffff));
            quint32 j = nBase;
            for (qint32 c = 0x100; c > 0; c--) {
                if (nSymbol == g8(HAP_OFF_SYM + static_cast<qint32>(j))) break;
                j = (j + 1) & 0xffff;
            }
            p16(HAP_OFF_MATCHPOS + static_cast<qint32>(u), (j - nBase) & 0xffff);
            u = (u + 2) & 0xffff;
            if (!(u < 8)) break;
        }
        u = 6;
        for (qint32 c = 0; c < 3; c++) {
            p16(HAP_OFF_SAVEA3 + static_cast<qint32>(u), g16(HAP_OFF_NODE + static_cast<qint32>(u)));
            const qint32 nOffset = gi16(HAP_OFF_NODESTART + static_cast<qint32>((gi16(HAP_OFF_NODE + static_cast<qint32>(u)) * 2) & 0xffff));
            p16(HAP_OFF_SAVEAD + static_cast<qint32>(u), g16(HAP_OFF_MATCHPOS + static_cast<qint32>(u)));
            p16(HAP_OFF_NODE + static_cast<qint32>(u) + 2,
                g16(HAP_OFF_CHILD + static_cast<qint32>(((nOffset + gi16(HAP_OFF_MATCHPOS + static_cast<qint32>(u))) * 2) & 0xffff)));
            u = (u - 2) & 0xffff;
        }
    }
    return true;
}

}  // namespace

bool XHAPDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaUnpacked)
{
    if (!pbaUnpacked || (nUncompressedSize < 0) || (nUncompressedSize > HAP_MAX_UNPACKED_SIZE)) return false;
    pbaUnpacked->clear();
    if (nUncompressedSize == 0) return true;
    if (baPacked.isEmpty()) return false;

    HapModel model(reinterpret_cast<const quint8 *>(baPacked.constData()), baPacked.size());
    QByteArray baOut;
    baOut.reserve(static_cast<qint32>(nUncompressedSize));
    if (!model.decode(nUncompressedSize, &baOut)) return false;
    if (baOut.size() != nUncompressedSize) return false;
    *pbaUnpacked = baOut;
    return true;
}
