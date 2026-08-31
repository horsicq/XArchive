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
 *
 * The method descriptions and state transitions were cross-checked against
 * unarc-rs by Mike Krueger (used under its MIT option). The corresponding notice and license
 * are in Algos/licenses/unarc-rs/.
 */
#include "xpakdecoder.h"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace {
const qint32 PAK_METHOD_CRUSHED = 10;
const qint32 PAK_METHOD_DISTILLED = 11;
const qint32 PAK_OUTPUT_BUFFER_SIZE = 0x10000;
const quint8 PAK_RLE_ESCAPE = 0x90;

const quint32 CRUSHED_TABLE_SIZE = 8192;
const quint32 CRUSHED_RESERVED = 257;
const quint32 CRUSHED_EOF = 256;
const quint32 CRUSHED_RING_SIZE = 500;
const quint32 CRUSHED_STRING_THRESHOLD = 375;

const quint32 DISTILLED_MAX_NODES = 628;
const quint32 DISTILLED_WINDOW_SIZE = 8192;
const quint32 DISTILLED_EOF = 256;

class PakBitSource {
public:
    explicit PakBitSource(XBinary::DATAPROCESS_STATE *pState) : m_pState(pState), m_nBuffer(0), m_nBits(0)
    {
    }

    bool readBits(qint32 nCount, quint32 *pValue)
    {
        if (!pValue || (nCount < 0) || (nCount > 24)) return false;
        if (nCount == 0) {
            *pValue = 0;
            return true;
        }

        while (m_nBits < nCount) {
            if (!m_pState || (m_pState->nInputLimit < 0) || (m_pState->nCountInput >= m_pState->nInputLimit)) return false;
            char c = 0;
            if (XBinary::_readDevice(&c, 1, m_pState) != 1) return false;
            m_nBuffer |= static_cast<quint64>(static_cast<quint8>(c)) << m_nBits;
            m_nBits += 8;
        }

        const quint64 nMask = (static_cast<quint64>(1) << nCount) - 1;
        *pValue = static_cast<quint32>(m_nBuffer & nMask);
        m_nBuffer >>= nCount;
        m_nBits -= nCount;
        return true;
    }

    bool readBit(bool *pValue)
    {
        quint32 nBit = 0;
        if (!pValue || !readBits(1, &nBit)) return false;
        *pValue = nBit != 0;
        return true;
    }

    bool atPhysicalEnd() const
    {
        return m_pState && (m_pState->nInputLimit >= 0) && (m_pState->nCountInput == m_pState->nInputLimit);
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    quint64 m_nBuffer;
    qint32 m_nBits;
};

// The entropy stage feeds this sink. Crushed enables ARC's 0x90 RLE layer;
// Distilled writes final bytes directly. Produced bytes are counted before any
// caller-requested output window is clipped by XBinary::_writeDevice().
class PakSink {
public:
    PakSink(XBinary::DATAPROCESS_STATE *pState, bool bRunLength, qint64 nExpectedSize)
        : m_pState(pState),
          m_bRunLength(bRunLength),
          m_nExpectedSize(nExpectedSize),
          m_nProduced(0),
          m_bInRepeat(false),
          m_bHasLast(false),
          m_nLast(0),
          m_bError(false)
    {
        m_baOutput.reserve(PAK_OUTPUT_BUFFER_SIZE);
    }

    bool put(quint8 nByte)
    {
        if (m_bError) return false;
        if (!m_bRunLength) return emitByte(nByte);

        if (m_bInRepeat) {
            m_bInRepeat = false;
            if (nByte == 0) {
                m_nLast = PAK_RLE_ESCAPE;
                m_bHasLast = true;
                return emitByte(PAK_RLE_ESCAPE);
            }
            if (!m_bHasLast) {
                m_bError = true;
                return false;
            }
            for (quint32 i = 1; i < nByte; ++i) {
                if (!emitByte(m_nLast)) return false;
            }
            return true;
        }

        if (nByte == PAK_RLE_ESCAPE) {
            m_bInRepeat = true;
            return true;
        }

        m_nLast = nByte;
        m_bHasLast = true;
        return emitByte(nByte);
    }

    bool flush()
    {
        if (m_bError) return false;
        if (m_baOutput.isEmpty()) return true;
        const qint32 nSize = m_baOutput.size();
        if (XBinary::_writeDevice(m_baOutput.constData(), nSize, m_pState) != nSize) {
            m_bError = true;
            return false;
        }
        m_baOutput.clear();
        return true;
    }

    qint64 produced() const
    {
        return m_nProduced;
    }

    bool complete() const
    {
        return !m_bError && !m_bInRepeat && (m_nProduced == m_nExpectedSize);
    }

private:
    bool emitByte(quint8 nByte)
    {
        if ((m_nProduced < 0) || (m_nProduced >= m_nExpectedSize) || (m_nProduced == (std::numeric_limits<qint64>::max)())) {
            m_bError = true;
            return false;
        }
        m_baOutput.append(static_cast<char>(nByte));
        ++m_nProduced;
        return (m_baOutput.size() < PAK_OUTPUT_BUFFER_SIZE) || flush();
    }

    XBinary::DATAPROCESS_STATE *m_pState;
    bool m_bRunLength;
    qint64 m_nExpectedSize;
    qint64 m_nProduced;
    bool m_bInRepeat;
    bool m_bHasLast;
    quint8 m_nLast;
    bool m_bError;
    QByteArray m_baOutput;
};

struct CrushedEntry {
    qint32 nParent = -1;
    quint8 nByte = 0;
};

class CrushedState {
public:
    CrushedState()
        : m_nTableSize(CRUSHED_RESERVED),
          m_nCodeBits(1),
          m_nNextBump(2),
          m_bLiteralMode(true),
          m_nRingPosition(0),
          m_nStringCount(0),
          m_nUsagePosition(CRUSHED_RESERVED),
          m_nPreviousSymbol(0),
          m_bHasPrevious(false)
    {
        for (quint32 i = 0; i < 256; ++i) {
            m_aTable[i].nParent = -1;
            m_aTable[i].nByte = static_cast<quint8>(i);
        }
        m_aTable[CRUSHED_EOF].nParent = -1;
        for (quint32 i = 0; i < CRUSHED_RESERVED; ++i) m_aUsage[i] = 4;
    }

    bool readSymbol(PakBitSource *pSource, quint32 *pSymbol) const
    {
        if (!pSource || !pSymbol) return false;
        quint32 nCode = 0;
        if (m_bLiteralMode) {
            bool bString = false;
            if (!pSource->readBit(&bString)) return false;
            if (bString) {
                if (!pSource->readBits(static_cast<qint32>(m_nCodeBits), &nCode)) return false;
                nCode += 256;
            } else if (!pSource->readBits(8, &nCode)) {
                return false;
            }
        } else {
            if (!pSource->readBits(static_cast<qint32>(m_nCodeBits), &nCode)) return false;
            if (nCode < 256) nCode ^= 0xff;
        }
        *pSymbol = nCode;
        return true;
    }

    quint32 tableSize() const
    {
        return m_nTableSize;
    }

    bool hasPrevious() const
    {
        return m_bHasPrevious;
    }

    quint32 previousSymbol() const
    {
        return m_nPreviousSymbol;
    }

    bool markUsed(quint32 nSymbol)
    {
        quint32 nDepth = 0;
        while (nSymbol < CRUSHED_TABLE_SIZE) {
            if (++nDepth > CRUSHED_TABLE_SIZE) return false;
            m_aUsage[nSymbol] = 4;
            const qint32 nParent = m_aTable[nSymbol].nParent;
            if (nParent < 0) return true;
            if (static_cast<quint32>(nParent) >= m_nTableSize) return false;
            nSymbol = static_cast<quint32>(nParent);
        }
        return false;
    }

    void updateMode(bool bString)
    {
        if (m_aRing[m_nRingPosition] && (m_nStringCount != 0)) --m_nStringCount;
        m_aRing[m_nRingPosition] = bString;
        if (bString) ++m_nStringCount;
        m_nRingPosition = (m_nRingPosition + 1) % CRUSHED_RING_SIZE;

        const bool bNewLiteralMode = m_nStringCount < CRUSHED_STRING_THRESHOLD;
        if (bNewLiteralMode != m_bLiteralMode) {
            m_bLiteralMode = bNewLiteralMode;
            m_nNextBump = static_cast<quint32>(1) << m_nCodeBits;
            if (!m_bLiteralMode) m_nNextBump -= 0x100;
        }
    }

    // The resulting vector is reversed, allowing a bounded parent-chain walk
    // without recursion or a second dictionary-sized temporary allocation.
    bool decodeString(quint32 nSymbol, std::vector<quint8> *pReversed) const
    {
        if (!pReversed) return false;
        pReversed->clear();

        if (nSymbol == m_nTableSize) {
            if (!m_bHasPrevious || (m_nTableSize >= CRUSHED_TABLE_SIZE) ||
                !decodeNormal(m_nPreviousSymbol, pReversed) || pReversed->empty()) {
                return false;
            }
            pReversed->insert(pReversed->begin(), pReversed->back());
            return true;
        }

        return (nSymbol < m_nTableSize) && decodeNormal(nSymbol, pReversed);
    }

    bool addEntry(quint32 nSymbol)
    {
        if (!m_bHasPrevious) {
            m_nPreviousSymbol = nSymbol;
            m_bHasPrevious = true;
            return true;
        }

        quint8 nFirst = 0;
        if (nSymbol == m_nTableSize) {
            if (!firstByte(m_nPreviousSymbol, &nFirst)) return false;
        } else if (!firstByte(nSymbol, &nFirst)) {
            return false;
        }

        if (m_nTableSize < CRUSHED_TABLE_SIZE) {
            m_aTable[m_nTableSize].nParent = static_cast<qint32>(m_nPreviousSymbol);
            m_aTable[m_nTableSize].nByte = nFirst;
            m_aUsage[m_nTableSize] = 2;
            ++m_nTableSize;
        } else {
            quint32 nMinimumIndex = CRUSHED_RESERVED;
            quint8 nMinimumUsage = 0xff;
            quint32 nIndex = m_nUsagePosition;
            do {
                ++nIndex;
                if (nIndex >= CRUSHED_TABLE_SIZE) nIndex = CRUSHED_RESERVED;
                if (m_aUsage[nIndex] < nMinimumUsage) {
                    nMinimumIndex = nIndex;
                    nMinimumUsage = m_aUsage[nIndex];
                }
                if (m_aUsage[nIndex] > 0) --m_aUsage[nIndex];
                if (m_aUsage[nIndex] == 0) break;
            } while (nIndex != m_nUsagePosition);

            m_nUsagePosition = nIndex;
            m_aTable[nMinimumIndex].nParent = static_cast<qint32>(m_nPreviousSymbol);
            m_aTable[nMinimumIndex].nByte = nFirst;
            m_aUsage[nMinimumIndex] = 2;
        }

        m_nPreviousSymbol = nSymbol;
        return true;
    }

    void checkCodeSize()
    {
        const quint32 nAdded = m_nTableSize - CRUSHED_RESERVED;
        if ((nAdded >= m_nNextBump) && (m_nCodeBits < 13)) {
            ++m_nCodeBits;
            m_nNextBump = static_cast<quint32>(1) << m_nCodeBits;
            if (!m_bLiteralMode) m_nNextBump -= 0x100;
        }
    }

private:
    bool decodeNormal(quint32 nSymbol, std::vector<quint8> *pReversed) const
    {
        quint32 nDepth = 0;
        while (nSymbol < m_nTableSize) {
            if (++nDepth > CRUSHED_TABLE_SIZE) return false;
            pReversed->push_back(m_aTable[nSymbol].nByte);
            const qint32 nParent = m_aTable[nSymbol].nParent;
            if (nParent < 0) return true;
            if (static_cast<quint32>(nParent) >= m_nTableSize) return false;
            nSymbol = static_cast<quint32>(nParent);
        }
        return false;
    }

    bool firstByte(quint32 nSymbol, quint8 *pByte) const
    {
        if (!pByte) return false;
        quint32 nDepth = 0;
        while (nSymbol < m_nTableSize) {
            if (++nDepth > CRUSHED_TABLE_SIZE) return false;
            const qint32 nParent = m_aTable[nSymbol].nParent;
            if (nParent < 0) {
                *pByte = m_aTable[nSymbol].nByte;
                return true;
            }
            if (static_cast<quint32>(nParent) >= m_nTableSize) return false;
            nSymbol = static_cast<quint32>(nParent);
        }
        return false;
    }

    std::array<CrushedEntry, CRUSHED_TABLE_SIZE> m_aTable = {};
    std::array<quint8, CRUSHED_TABLE_SIZE> m_aUsage = {};
    std::array<bool, CRUSHED_RING_SIZE> m_aRing = {};
    quint32 m_nTableSize;
    quint32 m_nCodeBits;
    quint32 m_nNextBump;
    bool m_bLiteralMode;
    quint32 m_nRingPosition;
    quint32 m_nStringCount;
    quint32 m_nUsagePosition;
    quint32 m_nPreviousSymbol;
    bool m_bHasPrevious;
};

bool decodeCrushed(PakBitSource *pSource, PakSink *pSink, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSource || !pSink) return false;
    CrushedState state;
    std::vector<quint8> reversed;
    reversed.reserve(CRUSHED_TABLE_SIZE + 1);
    bool bFoundEnd = false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint32 nSymbol = 0;
        if (!state.readSymbol(pSource, &nSymbol)) return false;
        if (nSymbol == CRUSHED_EOF) {
            bFoundEnd = true;
            break;
        }
        if ((nSymbol >= CRUSHED_TABLE_SIZE) || (nSymbol > state.tableSize())) return false;

        if (nSymbol < state.tableSize()) {
            if (!state.markUsed(nSymbol)) return false;
        } else {
            if (!state.hasPrevious() || !state.markUsed(state.previousSymbol())) return false;
        }

        state.updateMode(nSymbol >= 256);
        if (!state.decodeString(nSymbol, &reversed)) return false;
        for (std::vector<quint8>::const_reverse_iterator it = reversed.crbegin(); it != reversed.crend(); ++it) {
            if (!pSink->put(*it)) return false;
        }
        if (!state.addEntry(nSymbol)) return false;
        state.checkCodeSize();
    }

    return bFoundEnd && XBinary::isPdStructNotCanceled(pPdStruct);
}

qint32 distilledOffsetLength(quint32 nSymbol)
{
    if (nSymbol == 0) return 3;
    if (nSymbol < 4) return 4;
    if (nSymbol < 12) return 5;
    if (nSymbol < 24) return 6;
    if (nSymbol < 48) return 7;
    return 8;
}

bool distilledOffsetSymbol(PakBitSource *pSource, quint32 *pSymbol)
{
    static const std::array<quint8, 64> CODES = {
        0x00, 0x02, 0x04, 0x0c, 0x01, 0x06, 0x0a, 0x0e, 0x11, 0x16, 0x1a, 0x1e, 0x05, 0x09, 0x0d, 0x15,
        0x19, 0x1d, 0x25, 0x29, 0x2d, 0x35, 0x39, 0x3d, 0x03, 0x07, 0x0b, 0x13, 0x17, 0x1b, 0x23, 0x27,
        0x2b, 0x33, 0x37, 0x3b, 0x43, 0x47, 0x4b, 0x53, 0x57, 0x5b, 0x63, 0x67, 0x6b, 0x73, 0x77, 0x7b,
        0x0f, 0x1f, 0x2f, 0x3f, 0x4f, 0x5f, 0x6f, 0x7f, 0x8f, 0x9f, 0xaf, 0xbf, 0xcf, 0xdf, 0xef, 0xff};

    if (!pSource || !pSymbol) return false;
    quint32 nCode = 0;
    for (qint32 nLength = 1; nLength <= 8; ++nLength) {
        bool bBit = false;
        if (!pSource->readBit(&bBit)) return false;
        if (bBit) nCode |= static_cast<quint32>(1) << (nLength - 1);
        for (quint32 nSymbol = 0; nSymbol < CODES.size(); ++nSymbol) {
            if ((distilledOffsetLength(nSymbol) == nLength) && (CODES[nSymbol] == nCode)) {
                *pSymbol = nSymbol;
                return true;
            }
        }
    }
    return false;
}

qint32 distilledExtraBits(qint64 nPosition)
{
    if (nPosition >= 0x0fc4) return 7;
    if (nPosition >= 0x07c4) return 6;
    if (nPosition >= 0x03c4) return 5;
    if (nPosition >= 0x01c4) return 4;
    if (nPosition >= 0x00c4) return 3;
    if (nPosition >= 0x0044) return 2;
    if (nPosition >= 0x0004) return 1;
    return 0;
}

bool distilledSymbol(PakBitSource *pSource, const std::vector<quint32> &nodes, quint32 nNodeCount, quint32 *pSymbol)
{
    if (!pSource || !pSymbol || (nodes.size() != nNodeCount) || (nNodeCount < 2)) return false;
    quint32 nNode = nNodeCount - 2;
    for (quint32 nDepth = 0; nDepth <= nNodeCount; ++nDepth) {
        bool bBit = false;
        if (!pSource->readBit(&bBit) || (nNode > nNodeCount - 2)) return false;
        const quint32 nValue = nodes[nNode + (bBit ? 1 : 0)];
        if (nValue >= nNodeCount) {
            *pSymbol = nValue - nNodeCount;
            return true;
        }
        nNode = nValue;
    }
    return false;
}

bool validateDistilledTree(const std::vector<quint32> &nodes, quint32 nNodeCount)
{
    if ((nNodeCount < 2) || (nNodeCount > DISTILLED_MAX_NODES) || (nNodeCount & 1) || (nodes.size() != nNodeCount)) return false;
    for (quint32 nValue : nodes) {
        if (nValue < nNodeCount) {
            if (nValue > nNodeCount - 2) return false;
        } else if ((nValue - nNodeCount) > 314) {
            return false;
        }
    }

    // Validate every path reachable from the root. A pair may be shared by
    // separate branches, but it may not recur on its own active path.
    struct FRAME {
        quint32 nNode;
        quint32 nNextBranch;
    };
    std::vector<quint8> colors(nNodeCount, 0);  // 0=new, 1=active, 2=complete
    std::vector<FRAME> stack;
    const quint32 nRoot = nNodeCount - 2;
    colors[nRoot] = 1;
    stack.push_back({nRoot, 0});
    while (!stack.empty()) {
        FRAME &frame = stack.back();
        if (frame.nNextBranch >= 2) {
            colors[frame.nNode] = 2;
            stack.pop_back();
            continue;
        }
        const quint32 nValue = nodes[frame.nNode + frame.nNextBranch++];
        if (nValue >= nNodeCount) continue;
        if (colors[nValue] == 1) return false;
        if (colors[nValue] == 0) {
            colors[nValue] = 1;
            stack.push_back({nValue, 0});
        }
    }
    return true;
}

bool decodeDistilled(PakBitSource *pSource, PakSink *pSink, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSource || !pSink) return false;
    quint32 nNodeCount = 0;
    quint32 nCodeLength = 0;
    if (!pSource->readBits(16, &nNodeCount) || !pSource->readBits(8, &nCodeLength) || (nNodeCount < 2) ||
        (nNodeCount > DISTILLED_MAX_NODES) || (nNodeCount & 1) || (nCodeLength < 1) || (nCodeLength > 12)) {
        return false;
    }

    std::vector<quint32> nodes(nNodeCount, 0);
    for (quint32 i = 0; i < nNodeCount; ++i) {
        if (!pSource->readBits(static_cast<qint32>(nCodeLength), &nodes[i])) return false;
    }
    if (!validateDistilledTree(nodes, nNodeCount)) return false;

    std::array<quint8, DISTILLED_WINDOW_SIZE> window = {};
    window.fill(0x20);
    quint32 nWindowPosition = 0;
    bool bFoundEnd = false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        quint32 nSymbol = 0;
        if (!distilledSymbol(pSource, nodes, nNodeCount, &nSymbol)) return false;
        if (nSymbol < 256) {
            const quint8 nByte = static_cast<quint8>(nSymbol);
            if (!pSink->put(nByte)) return false;
            window[nWindowPosition] = nByte;
            nWindowPosition = (nWindowPosition + 1) & (DISTILLED_WINDOW_SIZE - 1);
        } else if (nSymbol == DISTILLED_EOF) {
            bFoundEnd = true;
            break;
        } else {
            const quint32 nLength = nSymbol - 254;
            if ((nLength < 3) || (nLength > 60)) return false;
            quint32 nOffsetSymbol = 0;
            if (!distilledOffsetSymbol(pSource, &nOffsetSymbol)) return false;
            const qint32 nExtraBits = distilledExtraBits(pSink->produced());
            quint32 nExtra = 0;
            if (!pSource->readBits(nExtraBits, &nExtra)) return false;
            const quint32 nDistance = (nOffsetSymbol << nExtraBits) | nExtra;
            if (nDistance >= DISTILLED_WINDOW_SIZE) return false;

            quint32 nSourcePosition = (nWindowPosition - 1 - nDistance) & (DISTILLED_WINDOW_SIZE - 1);
            for (quint32 i = 0; i < nLength; ++i) {
                const quint8 nByte = window[nSourcePosition];
                nSourcePosition = (nSourcePosition + 1) & (DISTILLED_WINDOW_SIZE - 1);
                if (!pSink->put(nByte)) return false;
                window[nWindowPosition] = nByte;
                nWindowPosition = (nWindowPosition + 1) & (DISTILLED_WINDOW_SIZE - 1);
            }
        }
    }

    return bFoundEnd && XBinary::isPdStructNotCanceled(pPdStruct);
}
}  // namespace

bool XPakDecoder::decompress(XBinary::DATAPROCESS_STATE *pState, qint32 nMethod, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput || ((nMethod != PAK_METHOD_CRUSHED) && (nMethod != PAK_METHOD_DISTILLED)) ||
        (pState->nInputOffset < 0) || (pState->nInputLimit < 0) || (pState->nProcessedOffset < 0) || (pState->nProcessedLimit < -1)) {
        return false;
    }
    const qint64 nMax = (std::numeric_limits<qint64>::max)();
    if ((pState->nProcessedLimit != -1) && (pState->nProcessedOffset > nMax - pState->nProcessedLimit)) return false;

    bool bExpectedOK = false;
    const qint64 nExpectedSize = pState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bExpectedOK);
    if (!pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE) || !bExpectedOK || (nExpectedSize < 0) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nExpectedSize)) {
        return false;
    }

    pState->bReadError = false;
    pState->bWriteError = false;
    pState->nCountInput = 0;
    pState->nCountOutput = 0;
    if (!pState->pDeviceInput->seek(pState->nInputOffset) || (pState->pDeviceInput->pos() != pState->nInputOffset)) pState->bReadError = true;
    if ((!pState->pDeviceOutput->isSequential() && !pState->pDeviceOutput->seek(0)) || (pState->pDeviceOutput->pos() != 0)) pState->bWriteError = true;
    if (pState->bReadError || pState->bWriteError || !pState->pDeviceInput->isReadable() || !pState->pDeviceOutput->isWritable() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    PakBitSource source(pState);
    PakSink sink(pState, nMethod == PAK_METHOD_CRUSHED, nExpectedSize);
    bool bResult = (nMethod == PAK_METHOD_CRUSHED) ? decodeCrushed(&source, &sink, pPdStruct) : decodeDistilled(&source, &sink, pPdStruct);
    if (bResult && !sink.flush()) bResult = false;

    return bResult && source.atPhysicalEnd() && sink.complete() && (pState->nCountOutput == sink.produced()) && !pState->bReadError && !pState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}
