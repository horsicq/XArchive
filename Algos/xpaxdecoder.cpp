/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xpaxdecoder.h"

#include <QVector>

#include <algorithm>
#include <limits>

namespace
{
class PaxBits
{
public:
    explicit PaxBits(const QByteArray &data) : m_data(data), m_bitPosition(0) {}

    bool get(quint32 count, quint32 *value)
    {
        if (!value || count > 24 || m_bitPosition < 0 ||
            m_bitPosition > qint64(m_data.size()) * 8 - count)
            return false;
        const uchar *p = reinterpret_cast<const uchar *>(m_data.constData());
        quint32 result = 0;
        for (quint32 i = 0; i < count; ++i) {
            const qint64 bit = m_bitPosition++;
            result = (result << 1) |
                     ((p[bit >> 3] >> (7 - (bit & 7))) & 1U);
        }
        *value = result;
        return true;
    }

    qint64 consumedSize() const { return (m_bitPosition + 7) / 8; }

private:
    const QByteArray &m_data;
    qint64 m_bitPosition;
};

struct PaxNode
{
    qint32 left = -1;
    qint32 right = -1;
    qint32 parent = -1;
    quint32 frequency = 0;
};

struct PaxPair
{
    quint32 frequency = 0;
    qint32 node = -1;
};

bool paxPairLess(const PaxPair &a, const PaxPair &b)
{
    return (a.frequency < b.frequency) ||
           ((a.frequency == b.frequency) && (a.node < b.node));
}

class PaxModel
{
public:
    PaxModel(PaxBits *bits, qint32 alphabetSize, quint32 maximum)
        : m_bits(bits), m_alphabetSize(alphabetSize), m_maximum(maximum),
          m_count(0), m_nextRebuild(quint32(alphabetSize / 2)),
          m_root(alphabetSize * 2 - 2), m_valid(false)
    {
        if (!bits || alphabetSize < 2 || alphabetSize > 2048 || !maximum)
            return;
        m_nodes.resize(alphabetSize * 2 - 1);
        for (qint32 i = 0; i < alphabetSize; ++i)
            m_nodes[i].frequency = 1;
        m_valid = rebuild();
    }

    bool isValid() const { return m_valid; }

    bool decode(qint32 *symbol)
    {
        if (!symbol || !m_valid) return false;
        qint32 node = m_root;
        while (node >= m_alphabetSize) {
            quint32 bit = 0;
            if (!m_bits->get(1, &bit)) return false;
            node = bit ? m_nodes.at(node).left : m_nodes.at(node).right;
            if (node < 0 || node >= m_nodes.size()) return false;
        }
        if (m_nodes[node].frequency ==
            (std::numeric_limits<quint32>::max)()) return false;
        ++m_nodes[node].frequency;
        const quint32 oldCount = m_count++;
        if (oldCount >= m_nextRebuild) {
            if (!rebuild()) return false;
            if (m_nextRebuild <= (std::numeric_limits<quint32>::max)() / 2)
                m_nextRebuild *= 2;
            else
                m_nextRebuild = m_maximum;
            if (m_nextRebuild > m_maximum) m_nextRebuild = m_maximum;
            m_count = 0;
        }
        *symbol = node;
        return true;
    }

private:
    bool popLowest(const QVector<PaxPair> &leaves, qint32 *pLeafPosition,
                   qint32 *pInternalPosition, qint32 output, PaxPair *pPair)
    {
        if (!pLeafPosition || !pInternalPosition || !pPair) return false;
        const quint32 leafFrequency = *pLeafPosition < leaves.size()
            ? leaves.at(*pLeafPosition).frequency : 0x7fffU;
        const quint32 internalFrequency = *pInternalPosition < output
            ? m_nodes.at(*pInternalPosition).frequency : 0x7fffU;
        if (internalFrequency < leafFrequency) {
            pPair->frequency = internalFrequency;
            pPair->node = (*pInternalPosition)++;
        } else {
            if (*pLeafPosition >= leaves.size()) return false;
            *pPair = leaves.at((*pLeafPosition)++);
        }
        return pPair->node >= 0 && pPair->node < m_nodes.size();
    }

    bool rebuild()
    {
        QVector<PaxPair> leaves;
        QVector<PaxPair> high;
        leaves.reserve(m_alphabetSize);
        high.reserve(m_alphabetSize);
        for (qint32 symbol = m_alphabetSize - 1; symbol >= 0; --symbol) {
            PaxNode &leaf = m_nodes[symbol];
            leaf.parent = -1;
            if (leaf.frequency <= 1) {
                leaves.append({leaf.frequency, symbol});
            } else {
                const quint32 original = leaf.frequency;
                leaf.frequency = (original + 1) >> 1;
                high.append({original, symbol});
            }
        }
        std::sort(high.begin(), high.end(), paxPairLess);
        leaves += high;
        if (leaves.size() != m_alphabetSize) return false;

        qint32 leafPosition = 0;
        qint32 internalPosition = m_alphabetSize;
        qint32 output = m_alphabetSize;
        while (output <= m_root) {
            PaxPair first;
            PaxPair second;
            m_nodes[output].frequency = 0x7fffU;
            if (!popLowest(leaves, &leafPosition, &internalPosition, output, &first) ||
                !popLowest(leaves, &leafPosition, &internalPosition, output, &second) ||
                first.frequency > (std::numeric_limits<quint32>::max)() -
                                  second.frequency)
                return false;
            // The lowest candidate is the zero-bit/right child.  This unusual
            // ordering is part of PAX's deterministic initial tree.
            m_nodes[output].right = first.node;
            m_nodes[output].left = second.node;
            m_nodes[first.node].parent = output;
            m_nodes[second.node].parent = output;
            m_nodes[output].frequency = first.frequency + second.frequency;
            ++output;
        }
        m_nodes[m_root].parent = -1;
        return true;
    }

private:
    PaxBits *m_bits;
    qint32 m_alphabetSize;
    quint32 m_maximum;
    quint32 m_count;
    quint32 m_nextRebuild;
    qint32 m_root;
    bool m_valid;
    QVector<PaxNode> m_nodes;
};

void putPaxByte(QByteArray *pWindow, qint32 nWindowSize,
                qint32 nMirrorSize, qint32 *pWindowPosition,
                QByteArray *pResult, char value)
{
    if (!pWindow || !pWindowPosition || !pResult) return;
    (*pWindow)[*pWindowPosition] = value;
    if (*pWindowPosition < nMirrorSize)
        (*pWindow)[*pWindowPosition + nWindowSize] = value;
    if (++*pWindowPosition >= nWindowSize) *pWindowPosition = 0;
    pResult->append(value);
}
}  // namespace

bool XPaxDecoder::decode(const QByteArray &packed, qint32 expectedSize,
                         QByteArray *output, qint64 *consumedSize,
                         XBinary::PDSTRUCT *pPdStruct)
{
    if (output) output->clear();
    if (consumedSize) *consumedSize = 0;
    if (!output || expectedSize < 1 || packed.size() < 8 ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    PaxBits bits(packed);
    quint32 windowBits = 0, lowBits = 0, subtractor = 0;
    quint32 lengthSymbols = 0, minimumLength = 0;
    quint32 mainMaximum = 0, lengthMaximum = 0;
    if (!bits.get(4, &windowBits) || !bits.get(4, &lowBits) ||
        !bits.get(5, &subtractor) || !bits.get(10, &lengthSymbols) ||
        !bits.get(3, &minimumLength) || !bits.get(16, &mainMaximum) ||
        !bits.get(16, &lengthMaximum) || windowBits < 9 || windowBits > 15 ||
        lowBits > windowBits || (windowBits - lowBits) > 10 ||
        !subtractor || subtractor >= (1U << windowBits) ||
        lengthSymbols < 2 || lengthSymbols > 2048 || minimumLength < 2 ||
        mainMaximum < 1 || lengthMaximum < 1)
        return false;

    const qint32 windowSize = qint32((1U << windowBits) - subtractor);
    const qint32 mirrorSize = qint32(minimumLength + lengthSymbols - 1);
    const qint32 mainSymbols = 256 + (1 << (windowBits - lowBits));
    if (windowSize < 1 || mirrorSize < 1 || mirrorSize > windowSize ||
        mainSymbols < 258 || mainSymbols > 2048 ||
        windowSize > (std::numeric_limits<qint32>::max)() - mirrorSize)
        return false;

    PaxModel mainModel(&bits, mainSymbols, mainMaximum);
    PaxModel lengthModel(&bits, qint32(lengthSymbols), lengthMaximum);
    if (!mainModel.isValid() || !lengthModel.isValid()) return false;

    QByteArray window(windowSize + mirrorSize, ' ');
    QByteArray result;
    result.reserve(expectedSize);
    qint32 windowPosition = 0;

    while (result.size() < expectedSize) {
        if ((result.size() & 0x3fff) == 0 &&
            !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        qint32 symbol = 0;
        if (!mainModel.decode(&symbol)) return false;
        if (symbol < 256) {
            putPaxByte(&window, windowSize, mirrorSize, &windowPosition,
                       &result, char(symbol));
            continue;
        }

        quint32 lowDistance = 0;
        qint32 lengthSymbol = 0;
        if (!bits.get(lowBits, &lowDistance) ||
            !lengthModel.decode(&lengthSymbol)) return false;
        const qint32 length = qint32(minimumLength) + lengthSymbol;
        if (length < 1 || length > expectedSize - result.size()) return false;
        const quint32 highDistance = quint32(symbol - 256) << lowBits;
        if (highDistance > quint32(windowSize - 1) - lowDistance)
            return false;
        const qint32 distance = qint32(highDistance + lowDistance);
        qint32 source = windowPosition - distance;
        if (source < 0) source += windowSize;
        source -= length;
        if (source < 0) source += windowSize;
        if (source < 0 || source >= windowSize ||
            source > window.size() - length) return false;

        QByteArray overlap;
        const char *copySource = window.constData() + source;
        if (source < windowPosition && windowPosition < source + length) {
            overlap = QByteArray(copySource, length);
            copySource = overlap.constData();
        }
        for (qint32 i = 0; i < length; ++i)
            putPaxByte(&window, windowSize, mirrorSize, &windowPosition,
                       &result, copySource[i]);
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (consumedSize) *consumedSize = bits.consumedSize();
    *output = result;
    return true;
}
