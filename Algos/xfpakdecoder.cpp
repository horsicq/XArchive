/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xfpakdecoder.h"

#include <array>
#include <limits>

namespace {
const qint64 MAX_FPAK_OUTPUT = Q_INT64_C(512) * 1024 * 1024;

class FpakBits
{
public:
    FpakBits(const QByteArray &data, qint32 bytePosition)
        : m_data(data), m_bitPosition(qint64(bytePosition) * 8)
    {
    }

    bool readBit(quint32 *value)
    {
        if (!value || (m_bitPosition < 0) ||
            (m_bitPosition >= qint64(m_data.size()) * 8))
            return false;
        const uchar *pData =
            reinterpret_cast<const uchar *>(m_data.constData());
        *value = (pData[m_bitPosition >> 3] >> (m_bitPosition & 7)) & 1U;
        ++m_bitPosition;
        return true;
    }

    bool readBits(quint32 count, quint32 *value)
    {
        if (!value || (count > 24) || (m_bitPosition < 0) ||
            (m_bitPosition > qint64(m_data.size()) * 8 - count))
            return false;
        quint32 result = 0;
        for (quint32 i = 0; i < count; ++i) {
            quint32 bit = 0;
            if (!readBit(&bit)) return false;
            result |= bit << i;
        }
        *value = result;
        return true;
    }

    qint64 consumedSize() const { return (m_bitPosition + 7) / 8; }

private:
    const QByteArray &m_data;
    qint64 m_bitPosition;
};

struct FpakTree
{
    std::array<quint32, 17> count = {};
    std::array<quint32, 17> firstCode = {};
    std::array<qint32, 17> firstSymbol = {};
    std::array<quint8, 64> symbols = {};
};

bool readTree(const QByteArray &packed, qint32 *position, FpakTree *tree)
{
    if (!position || !tree || (*position < 0) ||
        (*position >= packed.size()))
        return false;

    std::array<quint8, 64> lengths = {};
    const quint32 pairCount = quint8(packed.at((*position)++)) + 1U;
    // Every pair describes at least one of the fixed 64 symbols.
    if (pairCount > 64U) return false;

    qint32 symbolCount = 0;
    for (quint32 i = 0; i < pairCount; ++i) {
        if (*position >= packed.size()) return false;
        const quint8 descriptor = quint8(packed.at((*position)++));
        const qint32 repeat = (descriptor >> 4) + 1;
        const quint8 length = (descriptor & 0x0fU) + 1U;
        if ((symbolCount > 64 - repeat) || (length > 16U)) return false;
        for (qint32 j = 0; j < repeat; ++j)
            lengths[symbolCount++] = length;
    }
    if (symbolCount != 64) return false;

    *tree = FpakTree();
    quint32 kraftUnits = 0;
    for (quint8 length : lengths) {
        ++tree->count[length];
        kraftUnits += 1U << (16U - length);
    }
    // Implode stores complete Shannon-Fano trees.  Reject over-subscribed and
    // incomplete descriptions alike; accepting either makes random data look
    // like a usable member and can hide a truncated table.
    if (kraftUnits != (1U << 16)) return false;

    quint32 code = 0;
    qint32 outputIndex = 0;
    for (quint32 length = 1; length <= 16; ++length) {
        code = (code + tree->count[length - 1]) << 1;
        tree->firstCode[length] = code;
        tree->firstSymbol[length] = outputIndex;
        for (qint32 symbol = 0; symbol < 64; ++symbol) {
            if (lengths[symbol] == length)
                tree->symbols[outputIndex++] = quint8(symbol);
        }
    }
    return (outputIndex == 64) &&
           (code + tree->count[16] == (1U << 16));
}

bool decodeSymbol(FpakBits *bits, const FpakTree &tree, quint32 *symbol)
{
    if (!bits || !symbol) return false;

    quint32 code = 0;
    for (quint32 length = 1; length <= 16; ++length) {
        quint32 bit = 0;
        if (!bits->readBit(&bit)) return false;

        // Implode transmits complemented canonical codes least-significant
        // bit first.  Complementing the incoming bit while growing an MSB-
        // first prefix lets us use the ordinary canonical-code ranges.
        code = (code << 1) | (bit ^ 1U);
        const quint32 first = tree.firstCode[length];
        const quint32 count = tree.count[length];
        if (count && (code >= first) && ((code - first) < count)) {
            const qint32 index =
                tree.firstSymbol[length] + qint32(code - first);
            if ((index < 0) || (index >= 64)) return false;
            *symbol = tree.symbols[index];
            return true;
        }
    }
    return false;
}
}  // namespace

bool XFpakDecoder::decode(const QByteArray &packed, qint64 expectedSize,
                          QByteArray *output, qint64 *consumedSize,
                          XBinary::PDSTRUCT *pPdStruct)
{
    if (output) output->clear();
    if (consumedSize) *consumedSize = 0;
    if (!output || packed.isEmpty() || (expectedSize < 1) ||
        (expectedSize > MAX_FPAK_OUTPUT) ||
        (expectedSize > (std::numeric_limits<qint32>::max)()) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    qint32 position = 0;
    FpakTree lengthTree;
    FpakTree distanceTree;
    if (!readTree(packed, &position, &lengthTree) ||
        !readTree(packed, &position, &distanceTree) ||
        (position >= packed.size()))
        return false;

    FpakBits bits(packed, position);
    QByteArray result;
    result.reserve(qint32(expectedSize));
    qint32 nextCancelCheck = 0;

    while (result.size() < expectedSize) {
        if (result.size() >= nextCancelCheck) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            nextCancelCheck = result.size() <=
                                      (std::numeric_limits<qint32>::max)() -
                                          16384
                                  ? result.size() + 16384
                                  : (std::numeric_limits<qint32>::max)();
        }

        quint32 literalFlag = 0;
        if (!bits.readBit(&literalFlag)) return false;
        if (literalFlag) {
            quint32 literal = 0;
            if (!bits.readBits(8, &literal)) return false;
            result.append(char(literal));
            continue;
        }

        quint32 lowDistance = 0;
        quint32 distanceSymbol = 0;
        quint32 lengthSymbol = 0;
        if (!bits.readBits(6, &lowDistance) ||
            !decodeSymbol(&bits, distanceTree, &distanceSymbol) ||
            !decodeSymbol(&bits, lengthTree, &lengthSymbol))
            return false;

        quint32 length = lengthSymbol + 2U;
        if (lengthSymbol == 63U) {
            quint32 extraLength = 0;
            if (!bits.readBits(8, &extraLength)) return false;
            length += extraLength;
        }
        const quint32 distance =
            distanceSymbol * 64U + lowDistance + 1U;
        if (!distance || (distance > 4096U) ||
            (length > quint64(expectedSize - result.size())))
            return false;

        // PKZIP Implode starts with a zero-filled dictionary.  A legal early
        // match may therefore point before the first produced byte; once the
        // source enters produced data, appending one byte at a time naturally
        // implements overlapping LZ copies.
        for (quint32 i = 0; i < length; ++i) {
            const char value = distance <= quint32(result.size())
                                   ? result.at(result.size() - qint32(distance))
                                   : '\0';
            result.append(value);
        }
    }

    const qint64 consumed = bits.consumedSize();
    if ((result.size() != expectedSize) || (consumed != packed.size()) ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    *output = result;
    if (consumedSize) *consumedSize = consumed;
    return true;
}
