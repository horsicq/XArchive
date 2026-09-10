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
#include "xtersedecoder.h"

#include <QtEndian>

#include <cstring>

namespace {
const qint32 TERSE_NODES = 4096;         // 0x000 .. 0xFFF
const qint32 TERSE_RING_FIRST = 0x101;   // first recyclable node
const qint32 TERSE_RING_LAST = 0xffe;    // last recyclable node
const qint32 TERSE_MAX_DEPTH = 0x1000;   // the reference expansion guard
const qint32 TERSE_STACK_SIZE = 0x1010;  // one push in, two out per expansion
const quint32 TERSE_MAGIC = 0xa5698901;
const qint64 TERSE_CANCEL_STEP = 0x10000;

// The reference sanity probe over the first five 12-bit codes.  Even and odd
// codes are read by two different helpers, which is why the cursor advances by
// one byte and then by two.
bool terseProbe(const quint8 *pData, qint64 nSize, qint64 nOffset)
{
    static const qint32 nLimits[5] = {0x100, 0x100, 0x102, 0x103, 0x104};

    if ((nOffset < 0) || (nSize < (nOffset + 8))) return false;

    qint64 nPosition = nOffset;
    for (qint32 i = 0; i < 5; i++) {
        qint32 nValue = 0;
        if (i & 1) {
            nValue = (qint32)(((quint32)(pData[nPosition] & 0x0f) << 8) + pData[nPosition + 1]) - 1;
            nPosition += 2;
        } else {
            nValue = (qint32)(((quint32)pData[nPosition] * 16) + (pData[nPosition + 1] >> 4)) - 1;
            nPosition += 1;
        }
        if ((nValue < 0) || (nValue >= nLimits[i])) return false;
    }

    return true;
}

// The whole decoder state.  pbaOut may be null, in which case nothing is
// produced and only the length is accumulated - that is how the reader learns
// the size the container never stores.
class TerseWalker {
public:
    TerseWalker(const QByteArray &baFile, qint32 nHeaderSize, QByteArray *pbaOut, qint64 nLimit);

    bool run(XBinary::PDSTRUCT *pPdStruct);
    qint64 getCount() const;

private:
    bool getCode(qint32 *pnCode);
    bool expand(qint32 nCode);
    void touch(qint32 nNode);
    void bump(qint32 nNode, qint32 nDelta);
    void put(quint8 nByte);

    const quint8 *m_pData;
    qint64 m_nSize;
    qint64 m_nPosition;
    quint32 m_nBits;
    qint32 m_nBitCount;
    QByteArray *m_pbaOut;
    qint64 m_nLimit;
    qint64 m_nCount;
    bool m_bOverflow;
    qint32 m_nHead;
    quint16 m_weights[TERSE_NODES];
    quint16 m_next[TERSE_NODES];
    quint16 m_prev[TERSE_NODES];
    quint16 m_left[TERSE_NODES];
    quint16 m_right[TERSE_NODES];
    qint32 m_stack[TERSE_STACK_SIZE];
};

TerseWalker::TerseWalker(const QByteArray &baFile, qint32 nHeaderSize, QByteArray *pbaOut, qint64 nLimit)
{
    m_pData = (const quint8 *)baFile.constData();
    m_nSize = baFile.size();
    m_nPosition = nHeaderSize;
    m_nBits = 0;
    m_nBitCount = 0;
    m_pbaOut = pbaOut;
    m_nLimit = nLimit;
    m_nCount = 0;
    m_bOverflow = false;
    m_nHead = TERSE_RING_FIRST;

    memset(m_weights, 0, sizeof(m_weights));
    memset(m_next, 0, sizeof(m_next));
    memset(m_prev, 0, sizeof(m_prev));
    memset(m_left, 0, sizeof(m_left));
    memset(m_right, 0, sizeof(m_right));
    memset(m_stack, 0, sizeof(m_stack));

    for (qint32 i = TERSE_RING_FIRST; i <= TERSE_RING_LAST; i++) {
        m_next[i] = (quint16)(i + 1);
        m_prev[i] = (quint16)(i - 1);
    }
    m_next[TERSE_RING_LAST] = (quint16)TERSE_RING_FIRST;
    m_prev[TERSE_RING_FIRST] = (quint16)TERSE_RING_LAST;
}

qint64 TerseWalker::getCount() const
{
    return m_nCount;
}

void TerseWalker::put(quint8 nByte)
{
    if (m_nCount >= m_nLimit) {
        m_bOverflow = true;
        return;
    }
    if (m_pbaOut) m_pbaOut->append((char)nByte);
    m_nCount++;
}

// 12 bits, MSB first, out of a 32-bit MSB-aligned accumulator.
bool TerseWalker::getCode(qint32 *pnCode)
{
    while (m_nBitCount <= 11) {
        if (m_nPosition >= m_nSize) return false;
        const quint32 nByte = m_pData[m_nPosition];
        m_nPosition++;
        m_nBits = (quint32)(m_nBits + (nByte << (24 - m_nBitCount)));
        m_nBitCount += 8;
    }
    *pnCode = (qint32)(m_nBits >> 20);
    m_nBits = (quint32)(m_nBits << 12);
    m_nBitCount -= 12;

    return true;
}

// Pre-order walk of the pair tree.  Pushing RT before LT makes LT pop first.
bool TerseWalker::expand(qint32 nCode)
{
    qint32 nDepth = 0;
    qint32 nTop = 0;
    m_stack[nTop++] = nCode;

    while (nTop > 0) {
        const qint32 nNode = m_stack[--nTop];
        if (nNode < 0x100) {
            put((quint8)nNode);
            if (m_bOverflow) return false;
        } else if (nNode != 0x100) {
            nDepth++;
            if (nDepth > TERSE_MAX_DEPTH) return false;
            if ((nTop + 2) > TERSE_STACK_SIZE) return false;
            if ((nNode < 0) || (nNode >= TERSE_NODES)) return false;
            m_stack[nTop++] = m_right[nNode];
            m_stack[nTop++] = m_left[nNode];
        }
    }

    return true;
}

// Unlink the node and re-insert it right in front of the head, i.e. at the MRU
// tail of the circular list.
void TerseWalker::touch(qint32 nNode)
{
    const qint32 nBefore = m_prev[nNode];
    const qint32 nAfter = m_next[nNode];
    m_next[nBefore] = (quint16)nAfter;
    m_prev[nAfter] = (quint16)nBefore;

    const qint32 nHead = m_nHead;
    const qint32 nTail = m_prev[nHead];
    m_next[nTail] = (quint16)nNode;
    m_prev[nNode] = (quint16)nTail;
    m_prev[nHead] = (quint16)nNode;
    m_next[nNode] = (quint16)nHead;
}

void TerseWalker::bump(qint32 nNode, qint32 nDelta)
{
    if (nNode > 0x100) {
        // 16-bit wrap-around is intentional: a zero weight decremented becomes
        // 0xFFFF and the node stops being a recycling candidate.
        m_weights[nNode] = (quint16)(m_weights[nNode] + nDelta);
        touch(nNode);
    }
}

bool TerseWalker::run(XBinary::PDSTRUCT *pPdStruct)
{
    qint32 nCode = 0;
    if (!getCode(&nCode)) return false;
    if (nCode == 0) return true;
    nCode--;
    if (nCode >= 0x100) return false;
    put((quint8)nCode);
    if (m_bOverflow) return false;

    qint32 nPrevious = nCode;
    qint64 nIterations = 0;

    for (;;) {
        nIterations++;
        if ((nIterations % TERSE_CANCEL_STEP) == 0) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        }

        if (!getCode(&nCode)) return false;
        if (nCode == 0) return true;
        nCode--;
        if (!expand(nCode)) return false;

        qint32 nSteps = 0;
        while (m_weights[m_nHead] != 0) {
            m_nHead = m_next[m_nHead];
            nSteps++;
            if (nSteps > TERSE_MAX_DEPTH) return false;
        }

        const qint32 nNode = m_nHead;
        bump(m_left[nNode], -1);
        bump(m_right[nNode], -1);
        m_left[nNode] = (quint16)nPrevious;
        m_right[nNode] = (quint16)nCode;
        bump(nPrevious, 1);
        bump(nCode, 1);
        nPrevious = nCode;
        m_nHead = m_next[nNode];
        touch(nNode);
    }
}
}  // namespace

const qint64 XTERSEDecoder::MAX_UNCOMPRESSED_SIZE;

qint32 XTERSEDecoder::detect(const QByteArray &baFile)
{
    const qint64 nSize = baFile.size();
    if (nSize < 5) return -1;
    const quint8 *pData = (const quint8 *)baFile.constData();

    if (qFromLittleEndian<quint32>(pData) == TERSE_MAGIC) return 4;
    if (nSize < 12) return -1;

    bool bHeader = false;
    // The reference implementation
    if ((pData[0] == 0x05) && (pData[1] < 2) && ((pData[2] | pData[3]) != 0) && (pData[4] == 0) && (pData[5] == 0) && (pData[6] == 0) && (pData[7] == 0) &&
        (pData[8] == 0) && (pData[9] == 0) && (pData[10] == 0) && (pData[11] == 0)) {
        bHeader = true;
    }
    // The reference implementation
    if (!bHeader && (pData[0] == 0x09) && (pData[1] < 2) && ((pData[2] | pData[3]) != 0) && (pData[4] != 0) && (pData[8] == 0) && (pData[9] == 0) &&
        (pData[10] == 0) && (pData[11] == 0)) {
        bHeader = true;
    }
    // The reference implementation
    if (!bHeader && (pData[0] == 0x02) && (pData[1] < 2) && ((pData[2] | pData[3]) != 0) && (pData[4] != 0) && (pData[8] == 0) && (pData[9] == 0) &&
        (pData[10] == 0) && (pData[11] == 0)) {
        bHeader = true;
    }
    if (!bHeader) return -1;
    if (nSize <= 0x13) return -1;
    if (!terseProbe(pData, nSize, 12)) return -1;

    return 12;
}

bool XTERSEDecoder::measure(const QByteArray &baFile, qint64 *pnUncompressedSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pnUncompressedSize) return false;
    const qint32 nHeaderSize = detect(baFile);
    if (nHeaderSize < 0) return false;

    TerseWalker walker(baFile, nHeaderSize, nullptr, MAX_UNCOMPRESSED_SIZE);
    // A truncated stream still yields everything that was decoded before the
    // cut, exactly as the reference does; only an empty result is a failure.
    walker.run(pPdStruct);
    const qint64 nCount = walker.getCount();
    if ((nCount <= 0) || (nCount > MAX_UNCOMPRESSED_SIZE)) return false;
    *pnUncompressedSize = nCount;

    return true;
}

bool XTERSEDecoder::decode(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;
    const qint32 nHeaderSize = detect(baPacked);
    if (nHeaderSize < 0) return false;

    const qint64 nLimit = (nUncompressedSize > 0) ? nUncompressedSize : MAX_UNCOMPRESSED_SIZE;
    QByteArray baOut;
    baOut.reserve((qint32)qMin<qint64>(nLimit, 0x400000));

    TerseWalker walker(baPacked, nHeaderSize, &baOut, nLimit);
    walker.run(pPdStruct);
    if (baOut.isEmpty()) return false;
    if ((nUncompressedSize > 0) && ((qint64)baOut.size() != nUncompressedSize)) return false;

    *pbaResult = baOut;

    return true;
}
