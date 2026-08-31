/*
 * Native C++ translation of the Compact Pro and DiskDoubler stream decoders
 * in XADMaster. Copyright (c) 2017-present MacPaw Inc. and contributors.
 * GNU LGPL 2.1 or later; see xadmaster/COPYING.
 */
#include "xmaclegacydecoders.h"

#include <QVector>

#include <limits>

namespace {

class BitReader
{
public:
    explicit BitReader(const QByteArray &data, qint64 byteOffset = 0)
        : m_data(data), m_bitOffset(byteOffset * 8), m_ok(byteOffset >= 0 &&
                                                         byteOffset <= data.size())
    {
    }

    bool isOk() const { return m_ok; }
    qint64 byteOffset() const { return (m_bitOffset + 7) / 8; }
    qint64 alignedByteOffset() const { return m_bitOffset / 8; }
    bool isByteAligned() const { return (m_bitOffset & 7) == 0; }

    quint32 bits(qint32 count)
    {
        if (!m_ok || count < 0 || count > 32 ||
            m_bitOffset > qint64(m_data.size()) * 8 - count) {
            m_ok = false;
            return 0;
        }
        quint32 value = 0;
        for (qint32 i = 0; i < count; ++i) {
            const qint64 bit = m_bitOffset++;
            value = (value << 1) |
                    ((quint8(m_data.at(bit / 8)) >> (7 - (bit & 7))) & 1U);
        }
        return value;
    }

    quint8 byte()
    {
        if (!m_ok || !isByteAligned() || m_bitOffset / 8 >= m_data.size()) {
            m_ok = false;
            return 0;
        }
        return quint8(m_data.at(m_bitOffset / 8));
    }

    quint8 takeByte()
    {
        const quint8 value = byte();
        if (m_ok) m_bitOffset += 8;
        return value;
    }

    void alignByte()
    {
        if (m_ok) m_bitOffset = (m_bitOffset + 7) & ~qint64(7);
    }

    bool seekByte(qint64 offset)
    {
        if (offset < 0 || offset > m_data.size()) {
            m_ok = false;
            return false;
        }
        m_bitOffset = offset * 8;
        return true;
    }

    void skipBytes(qint64 count)
    {
        if (!isByteAligned() || count < 0 ||
            m_bitOffset / 8 > m_data.size() - count) {
            m_ok = false;
            return;
        }
        m_bitOffset += count * 8;
    }

private:
    const QByteArray &m_data;
    qint64 m_bitOffset;
    bool m_ok;
};

class PrefixCode
{
public:
    struct Node {
        qint32 child[2] = {-1, -1};
        qint32 symbol = -1;
    };

    bool build(const QVector<qint32> &lengths, qint32 maximumLength)
    {
        m_nodes.clear();
        m_nodes.append(Node());
        quint32 code = 0;
        qint32 symbolsLeft = 0;
        for (qint32 length : lengths)
            if (length > 0) ++symbolsLeft;
        if (!symbolsLeft || maximumLength <= 0 || maximumLength > 31)
            return false;

        for (qint32 length = 1; length <= maximumLength; ++length) {
            for (qint32 symbol = 0; symbol < lengths.size(); ++symbol) {
                if (lengths.at(symbol) != length) continue;
                if (length < 32 && code >= (quint32(1) << length)) return false;
                qint32 node = 0;
                for (qint32 bitPosition = length - 1; bitPosition >= 0;
                     --bitPosition) {
                    if (m_nodes.at(node).symbol >= 0) return false;
                    const qint32 bit = (code >> bitPosition) & 1U;
                    qint32 next = m_nodes.at(node).child[bit];
                    if (next < 0) {
                        next = m_nodes.size();
                        m_nodes[node].child[bit] = next;
                        m_nodes.append(Node());
                    }
                    node = next;
                }
                if (m_nodes.at(node).symbol >= 0 ||
                    m_nodes.at(node).child[0] >= 0 ||
                    m_nodes.at(node).child[1] >= 0)
                    return false;
                m_nodes[node].symbol = symbol;
                ++code;
                if (--symbolsLeft == 0) return true;
            }
            code <<= 1;
        }
        return symbolsLeft == 0;
    }

    qint32 symbol(BitReader *reader) const
    {
        if (!reader || m_nodes.isEmpty()) return -1;
        qint32 node = 0;
        for (qint32 depth = 0; depth <= 31; ++depth) {
            if (m_nodes.at(node).symbol >= 0) return m_nodes.at(node).symbol;
            const qint32 bit = reader->bits(1);
            if (!reader->isOk()) return -1;
            node = m_nodes.at(node).child[bit];
            if (node < 0 || node >= m_nodes.size()) return -1;
        }
        return -1;
    }

private:
    QVector<Node> m_nodes;
};

bool readCompactCode(BitReader *reader, qint32 size, PrefixCode *code)
{
    if (!reader || !code || size <= 0 || !reader->isByteAligned()) return false;
    const qint32 byteCount = reader->takeByte();
    if (!reader->isOk() || byteCount * 2 > size) return false;
    QVector<qint32> lengths(size, 0);
    for (qint32 i = 0; i < byteCount; ++i) {
        const quint8 value = reader->takeByte();
        if (!reader->isOk()) return false;
        lengths[2 * i] = value >> 4;
        lengths[2 * i + 1] = value & 15;
    }
    return code->build(lengths, 15);
}

class CompactLzhStream
{
public:
    CompactLzhStream(const QByteArray &data, qint32 blockSize)
        : m_reader(data), m_blockSize(blockSize), m_blockCount(blockSize),
          m_blockStart(0), m_position(0), m_matchLength(0), m_matchOffset(0),
          m_window(8192, 0)
    {
    }

    bool next(quint8 *value)
    {
        if (!value || !m_reader.isOk()) return false;
        if (m_matchLength > 0) {
            *value = quint8(m_window.at(m_matchOffset & 8191));
            ++m_matchOffset;
            --m_matchLength;
        } else {
            if (m_blockCount >= m_blockSize && !startBlock()) return false;
            if (m_reader.bits(1)) {
                m_blockCount += 2;
                const qint32 literal = m_literal.symbol(&m_reader);
                if (literal < 0 || literal > 255) return false;
                *value = quint8(literal);
            } else {
                m_blockCount += 3;
                const qint32 length = m_length.symbol(&m_reader);
                const qint32 highOffset = m_offset.symbol(&m_reader);
                if (length < 0 || highOffset < 0 || highOffset > 127)
                    return false;
                const qint32 offset = (highOffset << 6) | m_reader.bits(6);
                if (!m_reader.isOk() || length <= 0 || length > 63 ||
                    offset < 0 || offset > 8191)
                    return false;
                m_matchLength = length;
                m_matchOffset = qint32(m_position) - offset;
                *value = quint8(m_window.at(m_matchOffset & 8191));
                ++m_matchOffset;
                --m_matchLength;
            }
        }
        m_window[qint32(m_position) & 8191] = char(*value);
        ++m_position;
        return true;
    }

private:
    bool startBlock()
    {
        if (m_blockStart) {
            m_reader.alignByte();
            const qint64 current = m_reader.alignedByteOffset();
            m_reader.skipBytes(((current - m_blockStart) & 1) ? 3 : 2);
        }
        if (!readCompactCode(&m_reader, 256, &m_literal) ||
            !readCompactCode(&m_reader, 64, &m_length) ||
            !readCompactCode(&m_reader, 128, &m_offset))
            return false;
        m_blockCount = 0;
        m_blockStart = m_reader.alignedByteOffset();
        return true;
    }

    BitReader m_reader;
    qint32 m_blockSize;
    qint32 m_blockCount;
    qint64 m_blockStart;
    qint64 m_position;
    qint32 m_matchLength;
    qint32 m_matchOffset;
    QByteArray m_window;
    PrefixCode m_literal;
    PrefixCode m_length;
    PrefixCode m_offset;
};

class CompactByteSource
{
public:
    virtual ~CompactByteSource() {}
    virtual bool next(quint8 *pValue) = 0;
};

class CompactLzhByteSource : public CompactByteSource
{
public:
    explicit CompactLzhByteSource(CompactLzhStream *pStream)
        : m_pStream(pStream)
    {
    }

    bool next(quint8 *pValue) override
    {
        return m_pStream && m_pStream->next(pValue);
    }

private:
    CompactLzhStream *m_pStream;
};

class CompactBufferByteSource : public CompactByteSource
{
public:
    explicit CompactBufferByteSource(const QByteArray *pData)
        : m_pData(pData), m_nPosition(0)
    {
    }

    bool next(quint8 *pValue) override
    {
        if (!pValue || !m_pData || m_nPosition >= m_pData->size()) return false;
        *pValue = quint8(m_pData->at(m_nPosition++));
        return true;
    }

private:
    const QByteArray *m_pData;
    qint64 m_nPosition;
};

bool expandCompactRle(CompactByteSource *pPackedSource,
                      qint64 rawSize, QByteArray *output)
{
    if (!pPackedSource || !output || rawSize < 0 ||
        rawSize > (std::numeric_limits<int>::max)())
        return false;
    output->clear();
    output->reserve(qint32(rawSize));
    quint8 saved = 0;
    qint32 repeat = 0;
    bool halfEscaped = false;
    while (output->size() < rawSize) {
        if (repeat) {
            output->append(char(saved));
            --repeat;
            continue;
        }
        quint8 byte = 0;
        if (halfEscaped) {
            byte = 0x81;
            halfEscaped = false;
        } else if (!pPackedSource->next(&byte)) {
            return false;
        }
        if (byte != 0x81) {
            saved = byte;
            output->append(char(byte));
            continue;
        }
        if (!pPackedSource->next(&byte)) return false;
        if (byte == 0x82) {
            if (!pPackedSource->next(&byte)) return false;
            if (byte) {
                repeat = byte - 2;
                output->append(char(saved));
            } else {
                repeat = 1;
                saved = 0x82;
                output->append(char(0x81));
            }
        } else if (byte == 0x81) {
            halfEscaped = true;
            saved = 0x81;
            output->append(char(0x81));
        } else {
            repeat = 1;
            saved = byte;
            output->append(char(0x81));
        }
        if (output->size() > rawSize) return false;
    }
    return output->size() == rawSize;
}

quint16 be16(const QByteArray &data, qint64 offset, bool *ok)
{
    if (!ok || !*ok || offset < 0 || offset > data.size() - 2) {
        if (ok) *ok = false;
        return 0;
    }
    return (quint16(quint8(data.at(offset))) << 8) |
           quint8(data.at(offset + 1));
}

quint32 be32(const QByteArray &data, qint64 offset, bool *ok)
{
    const quint32 high = be16(data, offset, ok);
    const quint32 low = be16(data, offset + 2, ok);
    return (high << 16) | low;
}

bool readDdnCode(const QByteArray &packed, qint64 start, PrefixCode *code,
                 qint64 *endOffset)
{
    bool ok = true;
    const quint32 header = be32(packed, start, &ok);
    if (!ok) return false;
    const qint32 codeCount = ((header >> 24) & 0xff) + 1;
    const qint32 byteCount = (header >> 13) & 0x7ff;
    const qint32 maximumLength = (header >> 8) & 0x1f;
    const qint32 bitCount = (header >> 3) & 0x1f;
    if (codeCount <= 0 || codeCount > 256 || byteCount < 0 ||
        start + 4 > packed.size() - byteCount || maximumLength <= 0 ||
        maximumLength > 31 || bitCount <= 0 || bitCount > 16)
        return false;
    BitReader reader(packed, start + 4);
    QVector<qint32> lengths(codeCount, 0);
    const bool zeroCoding = header & 4;
    for (qint32 i = 0; i < codeCount; ++i) {
        if (!zeroCoding || reader.bits(1))
            lengths[i] = qint32(reader.bits(bitCount));
        if (!reader.isOk() || lengths.at(i) > maximumLength) return false;
    }
    if (!code->build(lengths, maximumLength)) return false;
    if (endOffset) *endOffset = start + 4 + byteCount;
    return true;
}

}  // namespace

namespace XMacLegacyDecoders {

bool decodeCompactPro(const QByteArray &packed, qint64 rawSize, bool lzh,
                      qint32 blockSize, QByteArray *output)
{
    if (!output || rawSize < 0 || packed.isEmpty()) return false;
    if (lzh) {
        CompactLzhStream stream(packed, blockSize);
        CompactLzhByteSource source(&stream);
        return expandCompactRle(&source, rawSize, output);
    }
    CompactBufferByteSource source(&packed);
    return expandCompactRle(&source, rawSize, output);
}

bool decodeDiskDoublerADn(const QByteArray &packed, qint64 rawSize,
                         QByteArray *output)
{
    if (!output || rawSize < 0 ||
        rawSize > (std::numeric_limits<int>::max)())
        return false;
    output->clear();
    output->reserve(qint32(rawSize));
    qint64 position = 0;
    while (output->size() < rawSize) {
        bool ok = true;
        const quint16 compressedSize = be16(packed, position, &ok);
        const quint16 uncompressedSize = be16(packed, position + 2, &ok);
        if (!ok || !compressedSize || !uncompressedSize ||
            uncompressedSize > 0x2000 || position > packed.size() - 12 ||
            position + 12 > packed.size() - compressedSize ||
            uncompressedSize > rawSize - output->size())
            return false;
        quint8 headerXor = quint8(compressedSize ^ (compressedSize >> 8) ^
                                  uncompressedSize ^ (uncompressedSize >> 8));
        for (qint32 i = 4; i < 11; ++i)
            headerXor ^= quint8(packed.at(position + i));
        if (headerXor != quint8(packed.at(position + 11))) return false;
        const quint8 flags = quint8(packed.at(position + 9));
        const qint64 dataStart = position + 12;
        const qint64 nextBlock = dataStart + compressedSize;
        if (flags & 1) {
            if (compressedSize < uncompressedSize) return false;
            output->append(packed.constData() + dataStart, uncompressedSize);
        } else {
            BitReader reader(packed, dataStart);
            QByteArray block;
            block.reserve(uncompressedSize);
            while (block.size() < uncompressedSize) {
                if (!reader.bits(1)) {
                    block.append(char(reader.bits(8)));
                } else {
                    const bool farOffset = reader.bits(1);
                    const qint32 offset = qint32(reader.bits(farOffset ? 12 : 8));
                    qint32 length = 0;
                    if (!reader.bits(1)) length = 2;
                    else if (!reader.bits(1)) length = reader.bits(1) ? 4 : 3;
                    else length = qint32(reader.bits(4)) + 5;
                    if (!reader.isOk() || offset <= 0 || offset > block.size() ||
                        length > offset)
                        return false;
                    length = qMin(length, uncompressedSize - block.size());
                    for (qint32 i = 0; i < length; ++i)
                        block.append(block.at(block.size() - offset));
                }
                if (!reader.isOk()) return false;
            }
            output->append(block);
        }
        position = nextBlock;
    }
    return output->size() == rawSize;
}

bool decodeDiskDoublerDDn(const QByteArray &packed, qint64 rawSize,
                         QByteArray *output)
{
    if (!output || rawSize < 0 ||
        rawSize > (std::numeric_limits<int>::max)())
        return false;
    output->clear();
    output->reserve(qint32(rawSize));
    qint64 nextBlock = 0;
    while (output->size() < rawSize) {
        const qint64 start = nextBlock;
        bool ok = true;
        const quint32 uncompressedSize = be32(packed, start, &ok);
        const quint16 literalCount = be16(packed, start + 4, &ok);
        const quint16 offsetCount = be16(packed, start + 6, &ok);
        const quint16 lengthCompressedSize = be16(packed, start + 8, &ok);
        const quint16 literalCompressedSize = be16(packed, start + 10, &ok);
        const quint16 offsetCompressedSize = be16(packed, start + 12, &ok);
        if (!ok || !uncompressedSize || uncompressedSize > 65536 ||
            uncompressedSize > quint64(rawSize - output->size()) ||
            start > packed.size() - 22)
            return false;
        quint8 headerXor = 0;
        for (qint32 i = 0; i < 21; ++i)
            headerXor ^= quint8(packed.at(start + i));
        if (headerXor != quint8(packed.at(start + 21))) return false;
        const quint8 flags = quint8(packed.at(start + 14));
        const qint64 dataStart = start + 22;
        if (flags & 0x40) {
            nextBlock = dataStart + uncompressedSize;
            if (nextBlock > packed.size()) return false;
            output->append(packed.constData() + dataStart, uncompressedSize);
            continue;
        }
        const qint64 literalStart = dataStart + offsetCompressedSize;
        const qint64 lengthStart = literalStart + literalCompressedSize;
        nextBlock = lengthStart + lengthCompressedSize;
        if (literalStart < dataStart || lengthStart < literalStart ||
            nextBlock < lengthStart || nextBlock > packed.size())
            return false;

        PrefixCode offsetCode;
        qint64 codeEnd = 0;
        if (!readDdnCode(packed, dataStart, &offsetCode, &codeEnd) ||
            codeEnd > literalStart)
            return false;
        BitReader offsetReader(packed, codeEnd);
        QVector<quint16> offsets;
        offsets.reserve(offsetCount);
        for (qint32 i = 0; i < offsetCount; ++i) {
            const qint32 slot = offsetCode.symbol(&offsetReader);
            if (slot < 0 || slot > 31) return false;
            quint32 offset = 0;
            if (slot < 4) offset = slot + 1;
            else {
                const qint32 bits = slot / 2 - 1;
                const quint32 base = ((2U + (slot & 1)) << bits) + 1U;
                offset = base + offsetReader.bits(bits);
            }
            if (!offsetReader.isOk() || offset == 0 || offset > 65535)
                return false;
            offsets.append(quint16(offset));
        }

        QByteArray literals;
        literals.reserve(literalCount);
        if (flags & 0x80) {
            PrefixCode literalCode;
            if (!readDdnCode(packed, literalStart, &literalCode, &codeEnd) ||
                codeEnd > lengthStart)
                return false;
            BitReader literalReader(packed, codeEnd);
            for (qint32 i = 0; i < literalCount; ++i) {
                const qint32 symbol = literalCode.symbol(&literalReader);
                if (symbol < 0 || symbol > 255) return false;
                literals.append(char(symbol));
            }
        } else {
            if (literalStart > packed.size() - literalCount ||
                literalStart + literalCount > lengthStart)
                return false;
            literals = packed.mid(literalStart, literalCount);
        }

        PrefixCode lengthCode;
        if (!readDdnCode(packed, lengthStart, &lengthCode, &codeEnd) ||
            codeEnd > nextBlock)
            return false;
        BitReader lengthReader(packed, codeEnd);
        qint32 literalIndex = 0;
        qint32 offsetIndex = 0;
        const qint64 blockEnd = output->size() + uncompressedSize;
        while (output->size() < blockEnd) {
            const qint32 symbol = lengthCode.symbol(&lengthReader);
            if (symbol < 0) return false;
            if (symbol == 0) {
                if (literalIndex >= literals.size()) return false;
                output->append(literals.at(literalIndex++));
            } else if (symbol < 128) {
                qint32 length = symbol + 2;
                if (offsetIndex >= offsets.size()) return false;
                const qint32 offset = offsets.at(offsetIndex++);
                if (offset <= 0 || offset > output->size()) return false;
                length = qMin<qint64>(length, blockEnd - output->size());
                for (qint32 i = 0; i < length; ++i)
                    output->append(output->at(output->size() - offset));
            } else {
                if (symbol - 128 > 16) return false;
                qint32 length = 1 << (symbol - 128);
                length = qMin<qint64>(length, blockEnd - output->size());
                if (literalIndex > literals.size() - length) return false;
                output->append(literals.constData() + literalIndex, length);
                literalIndex += length;
            }
        }
    }
    return output->size() == rawSize;
}

}  // namespace XMacLegacyDecoders
