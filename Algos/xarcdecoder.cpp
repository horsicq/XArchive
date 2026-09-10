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
#include "xarcdecoder.h"
#include "algo_utils.h"

#include <algorithm>
#include <array>
#include <limits>
#include <vector>

namespace {

const quint8 ARC_DLE = 0x90;  // run-length escape
const qint32 ARC_OUTPUT_BUFFER_SIZE = 0x10000;

const qint32 ARC_LZW_MIN_BITS = 9;
const qint32 ARC_LZW_CRUNCH_MAX_BITS = 12;  // method 8
const qint32 ARC_LZW_SQUASH_BITS = 13;      // method 9
const quint32 ARC_LZW_CLEAR = 256;          // dynamic-width methods only
const qint32 ARC_LZW_MAX_BITS = 16;  // ARC 0x7f Unix-compress flag limit.

const qint32 ARC_SQUEEZE_NUMVALS = 257;  // 256 byte values + the end marker
const qint32 ARC_SQUEEZE_EOF = 256;

struct ARC_PARAMS {
    bool bRunLength;  // apply the outer run-length stage
    bool bDynamic;    // start at 9 bits and widen, instead of a fixed width
    qint32 nMaxBits;
    bool bLeadingMaxBitsByte;
};

bool arcParams(qint32 nMethod, ARC_PARAMS *pParams)
{
    if (!pParams) return false;

    switch (nMethod) {
        case 3: *pParams = {true, false, 0, false}; return true;
        case 4: *pParams = {true, false, 0, false}; return true;
        case 5: *pParams = {false, false, 0, false}; return true;
        case 6:
        case 7: *pParams = {true, false, 0, false}; return true;
        case 0x7f: *pParams = {false, true, ARC_LZW_MAX_BITS, true}; return true;
        case 8: *pParams = {true, true, ARC_LZW_CRUNCH_MAX_BITS, true}; return true;
        case 9: *pParams = {false, true, ARC_LZW_SQUASH_BITS, false}; return true;
        default: return false;
    }
}

// The code stream is read in groups of n_bits bytes, which hold exactly eight
// n_bits-wide codes with no bits left over.  In steady state that is identical
// to a flat bit reader, so the grouping is invisible - until a reset abandons
// the remainder of the current group.  A code-width increase always happens to
// land on a group boundary, so only an explicit CLEAR makes the difference
// observable, and then it changes everything that follows.
class ArcCodeReader {
public:
    ArcCodeReader(class ArcSource *pSource, qint32 nInitBits, qint32 nMaxBits)
        : m_pSource(pSource), m_nInitBits(nInitBits), m_nMaxBits(nMaxBits), m_nBits(nInitBits), m_nOffset(0), m_nSize(0), m_bClearPending(false), m_nFreeEnt(0)
    {
        m_nMaxMaxCode = (1u << nMaxBits);
        m_nMaxCode = maxCodeFor(nInitBits);
    }

    void setFreeEnt(quint32 nFreeEnt)
    {
        m_nFreeEnt = nFreeEnt;
    }
    void requestClear()
    {
        m_bClearPending = true;
    }
    qint32 width() const
    {
        return m_nBits;
    }

    // Returns false at end of input.
    bool read(quint32 *pCode);

private:
    quint32 maxCodeFor(qint32 nBits) const
    {
        return (nBits == m_nMaxBits) ? m_nMaxMaxCode : ((1u << nBits) - 1);
    }

    class ArcSource *m_pSource;
    qint32 m_nInitBits;
    qint32 m_nMaxBits;
    qint32 m_nBits;
    qint32 m_nOffset;  // bit offset into the current group
    qint32 m_nSize;    // usable bits in the current group
    bool m_bClearPending;
    quint32 m_nFreeEnt;
    quint32 m_nMaxCode;
    quint32 m_nMaxMaxCode;
    quint8 m_group[ARC_LZW_MAX_BITS];
};

// Output side: buffers, optionally expands the run-length stage, and refuses to
// exceed the record's declared original size.  Overrunning the declared size is
// a decode failure, not something to silently truncate.
class ArcSink {
public:
    ArcSink(XBinary::DATAPROCESS_STATE *pState, bool bRunLength, bool bHasExpectedSize, qint64 nExpectedSize)
        : m_pState(pState),
          m_bRunLength(bRunLength),
          m_bHasExpectedSize(bHasExpectedSize),
          m_nExpectedSize(nExpectedSize),
          m_nProduced(0),
          m_bInRepeat(false),
          m_bHasLast(false),
          m_nLast(0),
          m_bOverflow(false),
          m_bFramingError(false)
    {
        m_baOutput.reserve(ARC_OUTPUT_BUFFER_SIZE);
    }

    // One byte out of the entropy stage.
    bool put(quint8 nByte)
    {
        if (!m_bRunLength) return emit_(nByte);

        if (m_bInRepeat) {
            m_bInRepeat = false;
            if (nByte == 0) {
                // An escape byte that stands for itself.
                m_nLast = ARC_DLE;
                m_bHasLast = true;
                return emit_(ARC_DLE);
            }
            if (!m_bHasLast) {
                // A run before any literal has no character to repeat.
                m_bFramingError = true;
                return false;
            }
            // The first occurrence was already emitted, so nByte - 1 follow.
            for (quint32 i = 1; i < nByte; i++) {
                if (!emit_(m_nLast)) return false;
            }
            return true;
        }

        if (nByte == ARC_DLE) {
            m_bInRepeat = true;
            return true;
        }

        m_nLast = nByte;
        m_bHasLast = true;
        return emit_(nByte);
    }

    bool flush()
    {
        if (m_baOutput.isEmpty()) return true;
        const qint32 nSize = m_baOutput.size();
        if (XBinary::_writeDevice(m_baOutput.constData(), nSize, m_pState) != nSize) return false;
        m_baOutput.clear();
        return true;
    }

    qint64 produced() const
    {
        return m_nProduced;
    }
    bool isTruncated() const
    {
        return m_bInRepeat;
    }  // stream ended mid-escape
    bool isOverflow() const
    {
        return m_bOverflow;
    }
    bool isFramingError() const
    {
        return m_bFramingError;
    }
    bool isComplete() const
    {
        return !m_bHasExpectedSize || (m_nProduced == m_nExpectedSize);
    }

private:
    bool emit_(quint8 nByte)
    {
        if (m_bHasExpectedSize && (m_nProduced >= m_nExpectedSize)) {
            m_bOverflow = true;
            return false;
        }
        if (m_nProduced == (std::numeric_limits<qint64>::max)()) {
            m_bOverflow = true;
            return false;
        }
        m_baOutput.append((char)nByte);
        m_nProduced++;
        return (m_baOutput.size() < ARC_OUTPUT_BUFFER_SIZE) || flush();
    }

    XBinary::DATAPROCESS_STATE *m_pState;
    bool m_bRunLength;
    bool m_bHasExpectedSize;
    qint64 m_nExpectedSize;
    qint64 m_nProduced;
    bool m_bInRepeat;
    bool m_bHasLast;
    quint8 m_nLast;
    bool m_bOverflow;
    bool m_bFramingError;
    QByteArray m_baOutput;
};

// Input side.  A byte read past the record's bounded input is end-of-stream,
// not an error: ARC's entropy stages other than squeeze have no end marker and
// stop when the declared original size has been produced.
class ArcSource {
public:
    explicit ArcSource(XBinary::DATAPROCESS_STATE *pState) : m_pState(pState), m_nBits(0), m_nBuffer(0), m_nBytesRead(0)
    {
    }

    bool readByte(quint8 *pByte)
    {
        if (!pByte) return false;
        if ((m_pState->nInputLimit >= 0) && (m_pState->nCountInput >= m_pState->nInputLimit)) return false;
        char c = 0;
        if (XBinary::_readDevice(&c, 1, m_pState) != 1) return false;
        m_nBytesRead++;
        *pByte = (quint8)c;
        return true;
    }

    // LSB-first within each byte, low-order bits of the code first.
    bool readCode(qint32 nWidth, quint32 *pCode)
    {
        if (!pCode || (nWidth <= 0) || (nWidth > 32)) return false;
        while (m_nBits < nWidth) {
            quint8 nByte = 0;
            if (!readByte(&nByte)) return false;
            m_nBuffer |= ((quint64)nByte << m_nBits);
            m_nBits += 8;
        }
        *pCode = (quint32)(m_nBuffer & ((((quint64)1) << nWidth) - 1));
        m_nBuffer >>= nWidth;
        m_nBits -= nWidth;
        return true;
    }

    // One bit, LSB-first.  Used by the squeeze tree walk.
    bool readBit(quint32 *pBit)
    {
        quint32 nValue = 0;
        if (!readCode(1, &nValue)) return false;
        *pBit = nValue;
        return true;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    qint32 m_nBits;
    quint64 m_nBuffer;
    qint64 m_nBytesRead;
};

bool ArcCodeReader::read(quint32 *pCode)
{
    if (!pCode) return false;

    if (m_bClearPending || (m_nOffset >= m_nSize) || (m_nFreeEnt > m_nMaxCode)) {
        if (m_nFreeEnt > m_nMaxCode) {
            m_nBits++;
            m_nMaxCode = maxCodeFor(m_nBits);
        }
        if (m_bClearPending) {
            m_nBits = m_nInitBits;
            m_nMaxCode = maxCodeFor(m_nBits);
            m_bClearPending = false;
        }
        if ((m_nBits < 1) || (m_nBits > ARC_LZW_MAX_BITS)) return false;

        qint32 nGot = 0;
        while (nGot < m_nBits) {
            quint8 nByte = 0;
            if (!m_pSource->readByte(&nByte)) break;
            m_group[nGot++] = nByte;
        }
        if (nGot <= 0) return false;

        m_nOffset = 0;
        m_nSize = (nGot << 3) - (m_nBits - 1);
    }

    if (m_nOffset >= m_nSize) return false;

    quint32 nValue = 0;
    for (qint32 i = 0; i < m_nBits; i++) {
        const qint32 nBit = m_nOffset + i;
        nValue |= (quint32)((m_group[nBit >> 3] >> (nBit & 7)) & 1) << i;
    }
    m_nOffset += m_nBits;

    *pCode = nValue;
    return true;
}

// Method 3: nothing but the run-length stage.
bool arcDecodePack(ArcSource *pSource, ArcSink *pSink, XBinary::PDSTRUCT *pPdStruct)
{
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pSink->isComplete()) break;
        quint8 nByte = 0;
        if (!pSource->readByte(&nByte)) break;
        if (!pSink->put(nByte)) return false;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

// Method 4: Greenlaw's squeeze.  A decoding tree of signed 16-bit child pairs is
// stored first: a non-negative child is the next node index, a negative child is
// the leaf value -(child + 1), and leaf 256 terminates the stream.
bool arcDecodeSqueeze(ArcSource *pSource, ArcSink *pSink, XBinary::PDSTRUCT *pPdStruct)
{
    quint8 nLow = 0;
    quint8 nHigh = 0;
    if (!pSource->readByte(&nLow) || !pSource->readByte(&nHigh)) return false;
    const qint32 nNodes = (qint32)nLow | ((qint32)nHigh << 8);
    if ((nNodes < 0) || (nNodes > ARC_SQUEEZE_NUMVALS)) return false;

    std::vector<qint32> listChild((size_t)nNodes * 2, 0);
    for (qint32 i = 0; i < nNodes * 2; i++) {
        quint8 a = 0;
        quint8 b = 0;
        if (!pSource->readByte(&a) || !pSource->readByte(&b)) return false;
        const qint32 nValue = (qint32)(qint16)((quint16)a | ((quint16)b << 8));
        if (nValue >= nNodes) return false;                                     // forward index out of range
        if ((nValue < 0) && ((-(nValue + 1)) > ARC_SQUEEZE_EOF)) return false;  // leaf out of range
        listChild[(size_t)i] = nValue;
    }

    if (nNodes == 0) {
        // No tree: the record can only be empty.
        return pSink->isComplete();
    }

    // ARC does not always emit the end-of-stream leaf: an encoder may simply let
    // the bit stream run out once the record's declared original size has been
    // produced.  Both terminations are accepted; the caller's size and CRC
    // checks are what actually decide whether the decode was complete.
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pSink->isComplete()) break;

        qint32 nNode = 0;
        qint32 nGuard = 0;
        bool bOutOfInput = false;
        for (;;) {
            quint32 nBit = 0;
            if (!pSource->readBit(&nBit)) {
                bOutOfInput = true;
                break;
            }
            nNode = listChild[(size_t)nNode * 2 + nBit];
            if (nNode < 0) break;
            if (++nGuard > ARC_SQUEEZE_NUMVALS) return false;  // cycle in the tree
        }
        if (bOutOfInput) break;

        const qint32 nValue = -(nNode + 1);
        if (nValue == ARC_SQUEEZE_EOF) break;
        if (!pSink->put((quint8)nValue)) return false;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}


// original 12-bit hash-table crunch. Unlike the
// later ARC LZW method, literal codes are themselves hashed and there is no
// CLEAR code. Keep the reference implementation's dictionary and pending nibble local to this decode.
struct ArcHashEntry {
    quint16 next = 0;
    qint16 prefix = -1;
    quint8 suffix = 0;
    bool occupied = false;
};

class ArcNibbleReader {
public:
    explicit ArcNibbleReader(ArcSource *source) : m_source(source), m_nibble(-1) {}
    // -1 is the end of the bounded stream; -2 is a missing second byte.
    qint32 read()
    {
        quint8 first = 0, second = 0;
        if (!m_source->readByte(&first)) return -1;
        if (m_nibble >= 0) {
            const qint32 code = (m_nibble << 8) | first;
            m_nibble = -1;
            return code;
        }
        if (!m_source->readByte(&second)) return -2;
        m_nibble = second & 15;
        return (qint32(first) << 4) | (second >> 4);
    }
private:
    ArcSource *m_source;
    qint32 m_nibble;
};

qint32 arcHashInsert(std::array<ArcHashEntry, 4096> &table, qint32 method, qint16 prefix, quint8 suffix)
{
    const quint32 sum = quint32(qint32(prefix) + qint32(suffix));
    const quint32 folded = (sum & 0xffffU) | 0x800U;
    const quint32 hash = method == 7 ? sum * 0x3ae1U : (folded * folded) >> 6;
    quint32 slot = hash & 0xfffU;
    if (table[slot].occupied) {
        // 004e82b0 returns the LAST collision node in AX, despite the
        // decompiler's void prototype. 004e82f0 then probes tail+101.
        quint32 tail = slot;
        quint32 steps = 0;
        while (table[tail].next != 0) {
            tail = table[tail].next;
            if (tail >= table.size() || ++steps >= table.size()) return -1;
        }
        slot = (tail + 101) & 0xfffU;
        steps = 0;
        while (table[slot].occupied) {
            if (++steps >= table.size()) return -1;
            slot = (slot + 1) & 0xfffU;
        }
        table[tail].next = quint16(slot);
    }
    table[slot].occupied = true;
    table[slot].next = 0;
    table[slot].prefix = prefix;
    table[slot].suffix = suffix;
    return qint32(slot);
}

bool arcDecodeHash(ArcSource *source, ArcSink *sink, qint32 method, XBinary::PDSTRUCT *pd)
{
    std::array<ArcHashEntry, 4096> table = {};
    std::array<quint8, 4096> stack = {};
    for (qint32 value = 0; value < 256; ++value) {
        if (arcHashInsert(table, method, -1, quint8(value)) < 0) return false;
    }
    ArcNibbleReader reader(source);
    qint32 previous = reader.read();
    if (previous == -1) return XBinary::isPdStructNotCanceled(pd);
    if (previous < 0 || !table[size_t(previous)].occupied || table[size_t(previous)].prefix != -1) return false;
    quint8 first = table[size_t(previous)].suffix;
    if (!sink->put(first)) return false;
    qint32 remaining = 3840;
    while (XBinary::isPdStructNotCanceled(pd)) {
        const qint32 code = reader.read();
        if (code == -1) return true;
        if (code < 0) return false;
        const bool undefined = !table[size_t(code)].occupied;
        qint32 cursor = code;
        size_t length = 0;
        if (undefined) {
            if (remaining == 0) return false;
            stack[length++] = first;
            cursor = previous;
        }
        while (table[size_t(cursor)].prefix != -1) {
            if (!table[size_t(cursor)].occupied || length >= stack.size()) return false;
            stack[length++] = table[size_t(cursor)].suffix;
            cursor = table[size_t(cursor)].prefix;
            if (cursor < 0 || cursor >= qint32(table.size())) return false;
        }
        if (!table[size_t(cursor)].occupied || length >= stack.size()) return false;
        first = table[size_t(cursor)].suffix;
        stack[length++] = first;
        if (remaining != 0) {
            const qint32 assigned = arcHashInsert(table, method, qint16(previous), first);
            if (assigned < 0 || (undefined && assigned != code)) return false;
            --remaining;
        }
        while (length != 0) {
            if (!sink->put(stack[--length])) return false;
        }
        previous = code;
    }
    return false;
}

// ARC 8/9/0x7f use grouped, variable-width Unix-compress codes.
bool arcDecodeLzw(ArcSource *pSource, ArcSink *pSink, const ARC_PARAMS &params, XBinary::PDSTRUCT *pPdStruct)
{
    qint32 nMaxBits = params.nMaxBits;

    if (params.bLeadingMaxBitsByte) {
        quint8 nDeclared = 0;
        if (!pSource->readByte(&nDeclared)) return false;
        if (params.nMaxBits == ARC_LZW_MAX_BITS) {
            // ARC 0x7f uses the low five flag bits for the maximum width.
            // ARC always reserves CLEAR, even when flag 0x80 is absent.
            if ((nDeclared & 0x60) || ((nDeclared & 0x1f) < ARC_LZW_MIN_BITS) || ((nDeclared & 0x1f) > params.nMaxBits)) return false;
        } else if ((nDeclared < ARC_LZW_MIN_BITS) || (nDeclared > ARC_LZW_CRUNCH_MAX_BITS)) {
            return false;  // Preserve method 8's original plain 9..12 range.
        }
        nMaxBits = nDeclared & 0x1f;
    }
    if ((nMaxBits < ARC_LZW_MIN_BITS) || (nMaxBits > ARC_LZW_MAX_BITS)) return false;

    const qint32 nInitBits = params.bDynamic ? ARC_LZW_MIN_BITS : nMaxBits;
    const quint32 nFirstFree = ARC_LZW_CLEAR + 1;
    const quint32 nTableSize = (1u << nMaxBits);

    std::vector<quint16> listPrefix(nTableSize, 0);
    std::vector<quint8> listSuffix(nTableSize, 0);
    std::vector<quint8> listStack(nTableSize, 0);
    for (quint32 i = 0; i < 256; i++) listSuffix[i] = (quint8)i;

    ArcCodeReader reader(pSource, nInitBits, nMaxBits);
    quint32 nFreeEnt = nFirstFree;
    reader.setFreeEnt(nFreeEnt);

    quint32 nCode = 0;
    if (!reader.read(&nCode)) return true;  // empty stream; the size check decides
    if (nCode >= 256) return false;

    quint32 nOldCode = nCode;
    quint8 nFinalChar = (quint8)nCode;
    if (!pSink->isComplete() && !pSink->put(nFinalChar)) return false;

    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (pSink->isComplete()) break;
        if (!reader.read(&nCode)) break;  // end of the bounded input

        if (nCode == ARC_LZW_CLEAR) {
            std::fill(listPrefix.begin(), listPrefix.end(), (quint16)0);
            nFreeEnt = nFirstFree;
            reader.setFreeEnt(nFreeEnt);
            reader.requestClear();

            if (!reader.read(&nCode)) break;
            if (nCode >= 256) return false;
            nOldCode = nCode;
            nFinalChar = (quint8)nCode;
            if (!pSink->isComplete() && !pSink->put(nFinalChar)) return false;
            continue;
        }

        const quint32 nInCode = nCode;
        quint32 nStackSize = 0;
        quint32 nWork = nCode;

        if (nWork >= nFreeEnt) {
            // KwKwK: the code currently being defined.
            if (nWork > nFreeEnt) return false;
            listStack[nStackSize++] = nFinalChar;
            nWork = nOldCode;
        }

        quint32 nDepth = 0;
        while (nWork >= 256) {
            if ((nWork >= nTableSize) || (nStackSize >= nTableSize) || (++nDepth > nTableSize)) return false;
            listStack[nStackSize++] = listSuffix[nWork];
            nWork = listPrefix[nWork];
        }
        if (nStackSize >= nTableSize) return false;
        nFinalChar = listSuffix[nWork];
        listStack[nStackSize++] = nFinalChar;

        while (nStackSize > 0) {
            if (pSink->isComplete()) break;
            if (!pSink->put(listStack[--nStackSize])) return false;
        }

        if (nFreeEnt < nTableSize) {
            listPrefix[nFreeEnt] = (quint16)nOldCode;
            listSuffix[nFreeEnt] = nFinalChar;
            nFreeEnt++;
            reader.setFreeEnt(nFreeEnt);
        }

        nOldCode = nInCode;
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

}  // namespace

XArcDecoder::XArcDecoder(QObject *parent) : QObject(parent)
{
}

bool XArcDecoder::decompress(XBinary::DATAPROCESS_STATE *pDecompressState, qint32 nMethod, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDecompressState || !pDecompressState->pDeviceInput || !pDecompressState->pDeviceOutput || (pDecompressState->nInputOffset < 0) ||
        (pDecompressState->nInputLimit < -1)) {
        return false;
    }

    ARC_PARAMS params = {};
    if (!arcParams(nMethod, &params)) return false;

    // Every ARC record declares its original size, and the caller checks the
    // stored CRC-16 afterwards.  Refuse to guess when the size is missing.
    if (!pDecompressState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) return false;
    const qint64 nExpectedSize = pDecompressState->mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, (qint64)-1).toLongLong();
    if (nExpectedSize < 0) return false;

    Algo_utils::prepareState(pDecompressState);
    if (pDecompressState->bReadError || pDecompressState->bWriteError || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    ArcSource source(pDecompressState);
    ArcSink sink(pDecompressState, params.bRunLength, true, nExpectedSize);

    bool bResult = false;
    if (nMethod == 3) {
        bResult = arcDecodePack(&source, &sink, pPdStruct);
    } else if (nMethod == 4) {
        bResult = arcDecodeSqueeze(&source, &sink, pPdStruct);
    } else if (nMethod >= 5 && nMethod <= 7) {
        bResult = arcDecodeHash(&source, &sink, nMethod, pPdStruct);
    } else {
        bResult = arcDecodeLzw(&source, &sink, params, pPdStruct);
    }

    if (bResult && !pDecompressState->bWriteError && !sink.flush()) bResult = false;

    return bResult && sink.isComplete() && !sink.isTruncated() && !sink.isOverflow() && !sink.isFramingError() && (pDecompressState->nCountOutput == sink.produced()) &&
           !pDecompressState->bReadError && !pDecompressState->bWriteError && XBinary::isPdStructNotCanceled(pPdStruct);
}
