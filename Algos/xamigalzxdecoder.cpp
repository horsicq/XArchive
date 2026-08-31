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
#include "xamigalzxdecoder.h"

#include "algo_utils.h"

#include <QVector>

#include <array>
#include <limits>

namespace
{
const qint64 LZX_MAX_PACKED_SIZE = 512LL * 1024LL * 1024LL;
const qint32 LZX_WINDOW_SIZE = 65536;
const qint32 LZX_WINDOW_MASK = LZX_WINDOW_SIZE - 1;
const qint32 LZX_OUTPUT_BUFFER_SIZE = 65536;

const quint8 ADDITIONAL_BITS[32] = {
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
    7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13, 14, 14};
const quint32 BASE[32] = {
    0, 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192,
    256, 384, 512, 768, 1024, 1536, 2048, 3072, 4096, 6144,
    8192, 12288, 16384, 24576, 32768, 49152};

class BitReader
{
public:
    explicit BitReader(const QByteArray &data)
        : m_data(data), m_nByte(0), m_nBit(0)
    {
    }

    bool bits(qint32 nCount, quint32 *pValue)
    {
        if (!pValue || (nCount < 0) || (nCount > 24)) return false;
        quint32 nValue = 0;
        for (qint32 i = 0; i < nCount; ++i)
        {
            if (m_nByte >= m_data.size()) return false;
            // LZX swaps each input word before feeding its little-endian bit
            // reader.  Address the swapped view without making another copy.
            const qint64 nSource = (m_nByte & 1) ? (m_nByte - 1)
                                                 : (m_nByte + 1);
            if ((nSource < 0) || (nSource >= m_data.size())) return false;
            const quint8 nByte = static_cast<quint8>(m_data.at(nSource));
            nValue |= static_cast<quint32>((nByte >> m_nBit) & 1U) << i;
            if (++m_nBit == 8)
            {
                m_nBit = 0;
                ++m_nByte;
            }
        }
        *pValue = nValue;
        return true;
    }

private:
    const QByteArray &m_data;
    qint64 m_nByte;
    qint32 m_nBit;
};

class PrefixCode
{
public:
    PrefixCode()
    {
    }

    bool build(const qint32 *pLengths, qint32 nSymbols, qint32 nMaximumLength)
    {
        if (!pLengths || (nSymbols <= 0) || (nMaximumLength <= 0) ||
            (nMaximumLength > 24))
            return false;
        m_nodes.clear();
        m_nodes.append(Node());
        quint32 nCode = 0;
        qint32 nDefined = 0;
        for (qint32 nLength = 1; nLength <= nMaximumLength; ++nLength)
        {
            for (qint32 nSymbol = 0; nSymbol < nSymbols; ++nSymbol)
            {
                if (pLengths[nSymbol] != nLength) continue;
                if (nCode >= (1U << nLength)) return false;
                qint32 nNode = 0;
                for (qint32 nBit = nLength - 1; nBit >= 0; --nBit)
                {
                    if (m_nodes.at(nNode).nSymbol >= 0) return false;
                    const qint32 nBranch = (nCode >> nBit) & 1U;
                    qint32 nNext = m_nodes.at(nNode).anChild[nBranch];
                    if (nNext < 0)
                    {
                        nNext = m_nodes.count();
                        m_nodes[nNode].anChild[nBranch] = nNext;
                        m_nodes.append(Node());
                    }
                    nNode = nNext;
                }
                Node &leaf = m_nodes[nNode];
                if ((leaf.nSymbol >= 0) || (leaf.anChild[0] >= 0) ||
                    (leaf.anChild[1] >= 0))
                    return false;
                leaf.nSymbol = nSymbol;
                ++nDefined;
                ++nCode;
            }
            if (nLength != nMaximumLength) nCode <<= 1;
        }
        return nDefined > 0;
    }

    bool symbol(BitReader *pReader, qint32 *pSymbol) const
    {
        if (!pReader || !pSymbol || m_nodes.isEmpty()) return false;
        qint32 nNode = 0;
        for (qint32 nDepth = 0; nDepth <= 24; ++nDepth)
        {
            const Node &node = m_nodes.at(nNode);
            if (node.nSymbol >= 0)
            {
                *pSymbol = node.nSymbol;
                return true;
            }
            quint32 nBit = 0;
            if (!pReader->bits(1, &nBit)) return false;
            nNode = node.anChild[nBit];
            if ((nNode < 0) || (nNode >= m_nodes.count())) return false;
        }
        return false;
    }

private:
    struct Node
    {
        Node() : nSymbol(-1)
        {
            anChild[0] = -1;
            anChild[1] = -1;
        }
        qint32 anChild[2];
        qint32 nSymbol;
    };
    QVector<Node> m_nodes;
};

bool readDeltaLengths(BitReader *pReader, qint32 *pLengths, qint32 nCount,
                      bool bAlternate)
{
    if (!pReader || !pLengths || (nCount <= 0)) return false;
    qint32 anPreLengths[20] = {};
    for (qint32 i = 0; i < 20; ++i)
    {
        quint32 nValue = 0;
        if (!pReader->bits(4, &nValue)) return false;
        anPreLengths[i] = static_cast<qint32>(nValue);
    }
    PrefixCode preCode;
    if (!preCode.build(anPreLengths, 20, 15)) return false;

    const qint32 nFix = bAlternate ? 1 : 0;
    qint32 i = 0;
    while (i < nCount)
    {
        qint32 nValue = -1;
        if (!preCode.symbol(pReader, &nValue)) return false;
        qint32 nRepeat = 1;
        qint32 nLength = 0;
        quint32 nExtra = 0;
        if ((nValue >= 0) && (nValue <= 16))
        {
            nLength = (pLengths[i] + 17 - nValue) % 17;
        }
        else if (nValue == 17)
        {
            if (!pReader->bits(4, &nExtra)) return false;
            nRepeat = static_cast<qint32>(nExtra) + 4 - nFix;
        }
        else if (nValue == 18)
        {
            if (!pReader->bits(5 + nFix, &nExtra)) return false;
            nRepeat = static_cast<qint32>(nExtra) + 20 - nFix;
        }
        else if (nValue == 19)
        {
            if (!pReader->bits(1, &nExtra)) return false;
            nRepeat = static_cast<qint32>(nExtra) + 4 - nFix;
            qint32 nNewValue = -1;
            if (!preCode.symbol(pReader, &nNewValue) ||
                (nNewValue < 0) || (nNewValue > 16))
                return false;
            nLength = (pLengths[i] + 17 - nNewValue) % 17;
        }
        else
        {
            return false;
        }
        if ((nRepeat <= 0) || (nRepeat > (nCount - i))) return false;
        for (qint32 j = 0; j < nRepeat; ++j) pLengths[i + j] = nLength;
        i += nRepeat;
    }
    return true;
}

class Output
{
public:
    Output(XBinary::DATAPROCESS_STATE *pState, XBinary::PDSTRUCT *pPdStruct,
           qint64 nExpectedSize)
        : m_pState(pState), m_pPdStruct(pPdStruct), m_nExpectedSize(nExpectedSize),
          m_nProduced(0), m_nBuffered(0)
    {
        m_window.fill(0);
    }

    qint64 produced() const { return m_nProduced; }

    bool byte(quint8 nByte)
    {
        if ((m_nProduced < 0) || (m_nProduced >= m_nExpectedSize) ||
            !XBinary::isPdStructNotCanceled(m_pPdStruct))
            return false;
        m_window[static_cast<quint32>(m_nProduced) & LZX_WINDOW_MASK] = nByte;
        m_buffer[m_nBuffered++] = static_cast<char>(nByte);
        ++m_nProduced;
        return (m_nBuffered < LZX_OUTPUT_BUFFER_SIZE) || flush();
    }

    bool match(quint32 nOffset, quint32 nLength, qint64 nBlockEnd)
    {
        if ((nOffset == 0) || (nOffset > LZX_WINDOW_SIZE) || (nLength == 0) ||
            (m_nProduced > nBlockEnd) ||
            (static_cast<qint64>(nLength) > (nBlockEnd - m_nProduced)) ||
            (m_nProduced > m_nExpectedSize) ||
            (static_cast<qint64>(nLength) > (m_nExpectedSize - m_nProduced)))
            return false;
        for (quint32 i = 0; i < nLength; ++i)
        {
            const quint8 nByte = m_window[
                static_cast<quint32>(m_nProduced - nOffset) & LZX_WINDOW_MASK];
            if (!byte(nByte)) return false;
        }
        return true;
    }

    bool flush()
    {
        if (m_nBuffered == 0) return true;
        const qint32 nWritten = XBinary::_writeDevice(
            m_buffer.data(), m_nBuffered, m_pState);
        if (nWritten != m_nBuffered) return false;
        m_nBuffered = 0;
        return true;
    }

private:
    XBinary::DATAPROCESS_STATE *m_pState;
    XBinary::PDSTRUCT *m_pPdStruct;
    qint64 m_nExpectedSize;
    qint64 m_nProduced;
    std::array<quint8, LZX_WINDOW_SIZE> m_window;
    std::array<char, LZX_OUTPUT_BUFFER_SIZE> m_buffer;
    qint32 m_nBuffered;
};
}  // namespace

XAmigaLZXDecoder::XAmigaLZXDecoder(QObject *pParent) : QObject(pParent)
{
}

bool XAmigaLZXDecoder::decompress(XBinary::DATAPROCESS_STATE *pState,
                                  XBinary::PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->pDeviceInput || !pState->pDeviceOutput ||
        (pState->nInputOffset < 0) || (pState->nInputLimit <= 0) ||
        (pState->nInputLimit > LZX_MAX_PACKED_SIZE) ||
        (pState->nInputLimit & 1) ||
        !pState->mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE))
        return false;
    bool bConverted = false;
    const qint64 nExpectedSize = pState->mapProperties.value(
        XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bConverted);
    if (!bConverted || (nExpectedSize < 0) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            nExpectedSize))
        return false;

    Algo_utils::prepareState(pState);
    if (pState->bReadError || pState->bWriteError ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    QByteArray packed;
    packed.resize(static_cast<int>(pState->nInputLimit));
    qint64 nReadTotal = 0;
    while ((nReadTotal < pState->nInputLimit) &&
           XBinary::isPdStructNotCanceled(pPdStruct))
    {
        const qint32 nChunk = static_cast<qint32>(qMin<qint64>(
            1024 * 1024, pState->nInputLimit - nReadTotal));
        const qint32 nRead = XBinary::_readDevice(
            packed.data() + nReadTotal, nChunk, pState);
        if (nRead != nChunk) return false;
        nReadTotal += nRead;
    }
    if ((nReadTotal != pState->nInputLimit) || pState->bReadError ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    BitReader reader(packed);
    Output output(pState, pPdStruct, nExpectedSize);
    qint32 anMainLengths[768] = {};
    quint32 nLastOffset = 1;
    qint64 nBlockEnd = 0;
    qint32 nBlockType = 0;
    PrefixCode mainCode;
    PrefixCode offsetCode;

    while ((output.produced() < nExpectedSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct))
    {
        if (output.produced() >= nBlockEnd)
        {
            quint32 nValue = 0;
            if (!reader.bits(3, &nValue) || (nValue < 1) || (nValue > 3))
                return false;
            nBlockType = static_cast<qint32>(nValue);
            // The original decoder also refuses aligned/uncompressed type 1;
            // no released Amiga LZX archive is known to use it.
            if (nBlockType == 1) return false;

            if (nBlockType == 3)
            {
                qint32 anOffsetLengths[8] = {};
                for (qint32 i = 0; i < 8; ++i)
                {
                    if (!reader.bits(3, &nValue)) return false;
                    anOffsetLengths[i] = static_cast<qint32>(nValue);
                }
                if (!offsetCode.build(anOffsetLengths, 8, 7)) return false;
            }

            quint32 nHigh = 0, nMiddle = 0, nLow = 0;
            if (!reader.bits(8, &nHigh) || !reader.bits(8, &nMiddle) ||
                !reader.bits(8, &nLow))
                return false;
            const qint64 nBlockSize =
                (static_cast<qint64>(nHigh) << 16) |
                (static_cast<qint64>(nMiddle) << 8) | nLow;
            if ((nBlockSize <= 0) ||
                (nBlockSize > (nExpectedSize - output.produced())))
                return false;
            nBlockEnd = output.produced() + nBlockSize;

            if (!readDeltaLengths(&reader, anMainLengths, 256, false) ||
                !readDeltaLengths(&reader, anMainLengths + 256, 512, true) ||
                !mainCode.build(anMainLengths, 768, 16))
                return false;
        }

        qint32 nSymbol = -1;
        if (!mainCode.symbol(&reader, &nSymbol) ||
            (nSymbol < 0) || (nSymbol >= 768))
            return false;
        if (nSymbol < 256)
        {
            if ((output.produced() >= nBlockEnd) ||
                !output.byte(static_cast<quint8>(nSymbol)))
                return false;
            continue;
        }

        const qint32 nOffsetClass = nSymbol & 31;
        quint32 nOffset = BASE[nOffsetClass];
        const qint32 nOffsetBits = ADDITIONAL_BITS[nOffsetClass];
        quint32 nExtra = 0;
        if (nOffset == 0)
        {
            nOffset = nLastOffset;
        }
        else if ((nBlockType == 3) && (nOffsetBits >= 3))
        {
            if (!reader.bits(nOffsetBits - 3, &nExtra)) return false;
            nOffset += nExtra << 3;
            qint32 nLowOffset = -1;
            if (!offsetCode.symbol(&reader, &nLowOffset) ||
                (nLowOffset < 0) || (nLowOffset > 7))
                return false;
            nOffset += static_cast<quint32>(nLowOffset);
        }
        else
        {
            if (!reader.bits(nOffsetBits, &nExtra)) return false;
            nOffset += nExtra;
        }

        const qint32 nLengthClass = ((nSymbol - 256) >> 5) & 15;
        quint32 nLength = BASE[nLengthClass] + 3;
        const qint32 nLengthBits = ADDITIONAL_BITS[nLengthClass];
        if (!reader.bits(nLengthBits, &nExtra)) return false;
        nLength += nExtra;
        if (!output.match(nOffset, nLength, nBlockEnd)) return false;
        nLastOffset = nOffset;
    }

    return (output.produced() == nExpectedSize) && output.flush() &&
           !pState->bReadError && !pState->bWriteError &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}
