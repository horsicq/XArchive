// Reconstructed from U3.unp.exe; see xu3cpmdecoder.PROVENANCE.md.
#include "xu3cpmdecoder.h"

#include <algorithm>
#include <array>
#include <new>
#include "xancientdynamichuffman_p.h"
constexpr std::size_t XU3CpmDecoder::MaxOutput;

namespace {
class Bits {
public:
    Bits(const std::uint8_t *data, std::size_t size, std::size_t position)
        : m_data(data), m_size(size), m_position(position) {}
    bool read(unsigned count, unsigned *value)
    {
        unsigned result = 0;
        for (unsigned i = 0; i < count; ++i) {
            if (!m_remaining) {
                if (m_position == m_size) return false;
                m_byte = m_data[m_position++];
                m_remaining = 8;
            }
            result = (result << 1) | ((m_byte >> --m_remaining) & 1);
        }
        *value = result;
        return true;
    }
    std::size_t position() const { return m_position; }
    bool finishedWithZeroPadding() const
    {
        return m_position == m_size && (m_byte & ((1u << m_remaining) - 1)) == 0;
    }
private:
    const std::uint8_t *m_data;
    std::size_t m_size;
    std::size_t m_position;
    unsigned m_byte = 0;
    unsigned m_remaining = 0;
};

class Output {
public:
    Output(std::vector<std::uint8_t> *data, std::size_t limit, XU3CpmDecoder::Cancel canceled, void *context, bool measureOnly)
        : m_data(data), m_limit(limit), m_canceled(canceled), m_context(context), m_measureOnly(measureOnly) {}
    bool stopped() const { return m_canceled && m_canceled(m_context); }
    bool byte(unsigned value)
    {
        if (m_size >= m_limit || ((m_size & 1023) == 0 && stopped())) return false;
        if (!m_measureOnly) m_data->push_back(static_cast<std::uint8_t>(value));
        ++m_size;
        m_checksum = static_cast<std::uint16_t>(m_checksum + value);
        return true;
    }
    // U3 004c50b0: RLE90 count includes the already emitted preceding byte.
    bool rle(unsigned value)
    {
        if (m_escape) {
            m_escape = false;
            if (!value) return byte(0x90);
            if (!m_havePrevious) return false;
            for (unsigned i = 1; i < value; ++i) if (!byte(m_previous)) return false;
            return true;
        }
        if (value == 0x90) { m_escape = true; return true; }
        m_previous = value;
        m_havePrevious = true;
        return byte(value);
    }
    bool complete() const { return !m_escape && !stopped(); }
    std::uint16_t checksum() const { return m_checksum; }
    std::size_t size() const { return m_size; }
private:
    std::vector<std::uint8_t> *m_data;
    std::size_t m_limit;
    XU3CpmDecoder::Cancel m_canceled;
    void *m_context;
    bool m_measureOnly;
    std::size_t m_size = 0;
    std::uint16_t m_checksum = 0;
    unsigned m_previous = 0;
    bool m_escape = false;
    bool m_havePrevious = false;
};

// U3 004f38c0..004f3fc0. The original global dictionary is operation-local.
class Crunch {
public:
    Crunch(bool oldRevision, Bits *bits, Output *output) : m_old(oldRevision), m_bits(bits), m_output(output) {}
    bool decode()
    {
        if (!reset()) return false;
        unsigned width = m_old ? 12 : 9;
        int previous = -1;
        int firstByte = 0;
        while (!m_output->stopped()) {
            unsigned next = 0;
            do { if (!m_bits->read(width, &next)) return false; } while (!m_old && (next == 258 || next == 259));
            if (next == (m_old ? 0u : 256u)) return m_output->complete();
            if (!m_old && next == 257) {
                if (!reset()) return false;
                width = 9;
                previous = -1;
                continue;
            }
            const bool known = m_old ? m_suffix[next] != Empty : static_cast<int>(next) <= m_last;
            if (known) {
                if (!emit(static_cast<int>(next), &firstByte)) return false;
            } else {
                if (previous < 0 || (!m_old && static_cast<int>(next) != m_last + 1)) return false;
                if (!emit(previous, &firstByte) || !m_output->rle(static_cast<unsigned>(firstByte))) return false;
            }
            m_used[next] = true;
            if (m_last == 4095) {
                if (!m_old && previous >= 0 && !replace(previous, firstByte)) return false;
            } else if (previous >= 0) {
                if (!insert(previous, firstByte)) return false;
                if (!m_old && m_last < 4094 && m_last == (1 << width) - 2) ++width;
            }
            previous = static_cast<int>(next);
        }
        return false;
    }
private:
    static constexpr int Empty = 0x7fff;
    bool reset()
    {
        m_prefix.fill(Empty); m_suffix.fill(Empty); m_first.fill(Empty);
        m_links.fill(Empty); m_used.fill(false); m_hash.fill(-1);
        m_last = m_old ? 0 : -1;
        if (m_old) m_prefix[0] = m_suffix[0] = m_first[0] = 0xffff;
        for (int i = 0; i < (m_old ? 256 : 260); ++i) {
            if (!insert(m_old ? 0xffff : (i < 256 ? 0x6fff : Empty), i < 256 ? i : 0)) return false;
            if (!m_old) m_used[static_cast<std::size_t>(i)] = true;
        }
        return true;
    }
    static unsigned hash(int prefix, int suffix)
    {
        return ((static_cast<unsigned>(prefix) >> 4 ^ static_cast<unsigned>(suffix)) |
                ((static_cast<unsigned>(prefix) & 15) << 8)) + 1;
    }
    bool oldSlot(int prefix, int suffix, int *slot)
    {
        unsigned index;
        if (prefix == 0xffff && suffix == 0) index = 0x800;
        else {
            const unsigned sum = static_cast<unsigned>(prefix + suffix);
            index = ((sum & 0x1fff) | 0x800) >> 1;
            index = ((index * (index + (sum & 1))) >> 4) & 0xfff;
        }
        unsigned visited = 0;
        while (m_suffix[index] != Empty && m_links[index] != Empty) {
            if (++visited > 4096) return false;
            index = static_cast<unsigned>(m_links[index]);
            if (index >= 4096) return false;
        }
        if (m_suffix[index] != Empty) {
            unsigned candidate = (index + 0x65) & 0xfff;
            unsigned count = 0;
            while (m_suffix[candidate] != Empty) {
                if (++count >= 4096) return false;
                candidate = (candidate + 1) & 0xfff;
            }
            m_links[index] = static_cast<int>(candidate);
            index = candidate;
        }
        *slot = static_cast<int>(index);
        return true;
    }
    bool assign(int slot, int prefix, int suffix)
    {
        if (slot < 0 || slot >= 4096 || suffix < 0 || suffix > 255) return false;
        m_suffix[slot] = suffix;
        if (prefix < 4096) {
            if (prefix < 0) return false;
            m_prefix[slot] = prefix;
            m_first[slot] = m_prefix[prefix] == Empty ? prefix : m_first[prefix];
        }
        return true;
    }
    bool insert(int prefix, int suffix)
    {
        if (++m_last >= 4096) { m_last = 4095; return true; }
        int slot = m_last;
        if (m_old) {
            if (!oldSlot(prefix, suffix, &slot)) return false;
        } else {
            const unsigned step = hash(prefix, suffix);
            unsigned index = step;
            if (index >= m_hash.size()) return false;
            unsigned count = 0;
            while (m_hash[index] != -1) {
                if (++count >= m_hash.size()) return false;
                index = (index + step) % static_cast<unsigned>(m_hash.size());
            }
            m_hash[index] = slot;
        }
        return assign(slot, prefix, suffix);
    }
    bool replace(int prefix, int suffix)
    {
        const unsigned step = hash(prefix, suffix);
        unsigned index = step;
        if (index >= m_hash.size()) return false;
        for (std::size_t count = 0; count < m_hash.size(); ++count) {
            const int slot = m_hash[index];
            if (slot == -1) return true;
            if (!m_used[slot]) return assign(slot, prefix, suffix);
            index = (index + step) % static_cast<unsigned>(m_hash.size());
        }
        return false;
    }
    bool emit(int code, int *firstByte)
    {
        std::array<unsigned, 4096> stack;
        std::size_t count = 0;
        while (code >= 0 && code < 4096 && m_prefix[code] != Empty) {
            if (count == stack.size() || m_suffix[code] > 255) return false;
            stack[count++] = static_cast<unsigned>(m_suffix[code]);
            code = m_prefix[code];
        }
        if (code < 0 || code >= 4096 || m_suffix[code] < 0 || m_suffix[code] > 255) return false;
        *firstByte = m_suffix[code];
        if (!m_output->rle(static_cast<unsigned>(*firstByte))) return false;
        while (count) if (!m_output->rle(stack[--count])) return false;
        return true;
    }
    bool m_old;
    Bits *m_bits;
    Output *m_output;
    std::array<int, 4096> m_prefix, m_suffix, m_first, m_links;
    std::array<bool, 4096> m_used;
    std::array<int, 5003> m_hash;
    int m_last = -1;
};

constexpr int Crunch::Empty;

// U3 004ed030/004ed220/004ed5a0/004ed820: adaptive 315-symbol Huffman.
// U3 004f4a70 passes stop=1 and RLE=0 to the shared 004edae0 engine.
class Crlzh {
public:
    Crlzh(bool oldRevision, Bits *bits, Output *output) : m_old(oldRevision), m_bits(bits), m_output(output)
    {
        m_window.fill(0x20);
        for (int i = 0; i < Symbols; ++i) {
            m_frequency[i] = 1;
            m_child[i] = i + Nodes;
            m_parent[i + Nodes] = i;
        }
        for (int child = 0, node = Symbols; node <= Root; child += 2, ++node) {
            m_frequency[node] = m_frequency[child] + m_frequency[child + 1];
            m_child[node] = child;
            m_parent[child] = m_parent[child + 1] = node;
        }
        m_frequency[Nodes] = 0xffff;
        m_parent[Root] = 0;
    }
    bool decode()
    {
        unsigned cursor = 0;
        while (!m_output->stopped()) {
            int code = m_child[Root];
            while (code < Nodes) {
                unsigned bit;
                if (!m_bits->read(1, &bit)) return false;
                code = m_child[code + bit];
            }
            code -= Nodes;
            update(code);
            if (code == 256) return m_output->complete();
            if (code < 256) {
                if (!m_output->byte(static_cast<unsigned>(code))) return false;
                m_window[cursor] = static_cast<std::uint8_t>(code);
                cursor = (cursor + 1) & 8191;
            } else {
                unsigned distance;
                if (!position(&distance)) return false;
                unsigned source = (cursor - distance - 1) & 8191;
                const int count = code - 254;
                for (int i = 0; i < count; ++i) {
                    const unsigned value = m_window[source];
                    if (!m_output->byte(value)) return false;
                    m_window[cursor] = static_cast<std::uint8_t>(value);
                    source = (source + 1) & 8191;
                    cursor = (cursor + 1) & 8191;
                }
            }
        }
        return false;
    }
private:
    static constexpr int Symbols = 315;
    static constexpr int Nodes = 2 * Symbols - 1;
    static constexpr int Root = Nodes - 1;
    void reconstruct()
    {
        int next = 0;
        for (int node = 0; node < Nodes; ++node) {
            if (m_child[node] >= Nodes) {
                m_frequency[next] = (m_frequency[node] + 1) / 2;
                m_child[next++] = m_child[node];
            }
        }
        for (int child = 0, node = Symbols; node < Nodes; child += 2, ++node) {
            const unsigned frequency = m_frequency[child] + m_frequency[child + 1];
            int slot = node;
            while (slot > 0 && frequency < m_frequency[slot - 1]) --slot;
            for (int i = node; i > slot; --i) {
                m_frequency[i] = m_frequency[i - 1];
                m_child[i] = m_child[i - 1];
            }
            m_frequency[slot] = frequency;
            m_child[slot] = child;
        }
        for (int node = 0; node < Nodes; ++node) {
            const int child = m_child[node];
            m_parent[child] = node;
            if (child < Nodes) m_parent[child + 1] = node;
        }
    }
    void update(int symbol)
    {
        if (m_frequency[Root] == 0x8000) reconstruct();
        int node = m_parent[symbol + Nodes];
        do {
            const unsigned frequency = ++m_frequency[node];
            int slot = node;
            if (frequency > m_frequency[node + 1]) {
                slot = node + 1;
                while (frequency > m_frequency[slot + 1]) ++slot;
                m_frequency[node] = m_frequency[slot];
                m_frequency[slot] = frequency;
                const int left = m_child[node];
                const int right = m_child[slot];
                m_parent[left] = slot;
                if (left < Nodes) m_parent[left + 1] = slot;
                m_parent[right] = node;
                if (right < Nodes) m_parent[right + 1] = node;
                std::swap(m_child[node], m_child[slot]);
            }
            node = m_parent[slot];
        } while (node);
    }
    bool position(unsigned *distance)
    {
        unsigned value;
        if (!m_bits->read(8, &value)) return false;
        // Canonical expansion equals U3 bytes 007d14c0[256] and 007d15c0[16].
        unsigned code = 0, boundary = 0, length = 3;
        for (; code < 64; ++code) {
            length = code < 1 ? 3u : code < 4 ? 4u : code < 12 ? 5u : code < 24 ? 6u : code < 48 ? 7u : 8u;
            boundary += 1u << (8 - length);
            if (value < boundary) break;
        }
        const unsigned lowBits = m_old ? 6 : 5;
        for (unsigned i = 0; i < length - (m_old ? 2u : 3u); ++i) {
            unsigned bit;
            if (!m_bits->read(1, &bit)) return false;
            value = (value << 1) | bit;
        }
        *distance = (code << lowBits) | (value & ((1u << lowBits) - 1));
        return true;
    }
    bool m_old;
    Bits *m_bits;
    Output *m_output;
    std::array<unsigned, Nodes + 1> m_frequency = {};
    std::array<int, Nodes + Symbols> m_parent = {};
    std::array<int, Nodes> m_child = {};
    std::array<std::uint8_t, 8192> m_window;
};

struct CompactBitReader {
    explicit CompactBitReader(Bits *bits) : m_bits(bits) {}
    unsigned operator()() const
    {
        unsigned bit;
        if (!m_bits->read(1, &bit)) throw XAncientPrivate::DecompressionError();
        // U3 has literal at root bit 1, escape at 00, EOF at 01.
        return bit ^ 1;
    }
    Bits *m_bits;
};

// U3 006d8220, with strict final-byte padding from recognizer 006d84b0.
// The existing Ancient primitive represents U3's ordered-frequency tree
// (006d8040/006d7d00/006d7b70) without its pointer/bucket allocation layout.
bool compact(Bits *bits, Output *output)
{
    unsigned first;
    if (!bits->read(8, &first) || !output->byte(first)) return false;
    std::array<bool, 256> seen = {};
    seen[first] = true;
    std::array<unsigned, 258> symbols = {};
    symbols[0] = 256;  // EOF
    symbols[1] = 257;  // Previously unseen literal
    symbols[2] = first;
    unsigned count = 3;
    XAncientPrivate::DynamicHuffmanDecoder<258> tree(3);
    const CompactBitReader readBit(bits);
    while (!output->stopped()) {
        const unsigned index = tree.decode(readBit);
        if (index >= count) return false;
        unsigned symbol = symbols[index];
        if (symbol == 256) return output->complete() && bits->finishedWithZeroPadding();
        tree.update(index);
        if (symbol == 257) {
            if (count >= symbols.size() || !bits->read(8, &symbol) || seen[symbol]) return false;
            seen[symbol] = true;
            tree.addCode();
            symbols[count] = symbol;
            tree.update(count++);
        }
        if (!output->byte(symbol)) return false;
    }
    return false;
}
}  // namespace

bool XU3CpmDecoder::parseHeader(const std::uint8_t *data, std::size_t size, Header *header)
{
    if (!header) return false;
    *header = Header();
    if (!data || size < 4 || size > MaxInput) return false;
    if (data[0] == 0xff && data[1] == 0x1f) {
        header->format = Format::UnixCompact;
        header->dataOffset = 2;
        return true;
    }
    if (size < 10 || data[0] != 0x76 || (data[1] != 0xfe && data[1] != 0xfd)) return false;
    Header parsed;
    parsed.format = data[1] == 0xfe ? Format::Crunch : Format::Crlzh;
    std::size_t position = 2;
    // U3 00432950: at most 64 filename bytes, optional 15-byte BBS stamp,
    // optional bracketed comment (64 bytes), then NUL and four parameters.
    while (position < size && parsed.name.size() < 64 && data[position] != 0 && data[position] != 1 && data[position] != '[') {
        unsigned c = data[position++];
        if (c < 0x20 || (c & 0x7f) < 0x20) return false;
        parsed.name.push_back(static_cast<char>(c & 0x7f));
    }
    if (parsed.name.empty() || position >= size) return false;
    if (data[position] == 1) {
        if (size - position <= 16) return false;
        position += 16;
    }
    if (data[position] == '[') {
        unsigned count = 0;
        do {
            ++position;
            if (position >= size || ++count > 64 || (data[position] && data[position] < 0x20)) return false;
        } while (data[position]);
    }
    if (data[position] || size - position < 7) return false;
    ++position;
    parsed.revision = data[position + 1] >> 4;
    parsed.checksumType = data[position + 2];
    parsed.dataOffset = position + 4;
    // Only the standard additive16 checksum is authenticated by this port.
    if ((parsed.revision != 1 && parsed.revision != 2) || parsed.checksumType != 0) return false;
    *header = parsed;
    return true;
}

bool XU3CpmDecoder::decode(const std::uint8_t *data, std::size_t size, std::size_t outputLimit,
                         Result *result, Cancel canceled, void *context, bool measureOnly)
{
    if (!result) return false;
    *result = Result();
    try {
        Result decoded;
        if (!parseHeader(data, size, &decoded.header) || (canceled && canceled(context))) return false;
        Output output(&decoded.data, std::min(outputLimit, MaxOutput), canceled, context, measureOnly);
        Bits bits(data, size, decoded.header.dataOffset);
        bool okay = false;
        if (decoded.header.format == Format::Crunch) {
            Crunch decoder(decoded.header.revision == 1, &bits, &output);
            okay = decoder.decode();
        } else if (decoded.header.format == Format::Crlzh) {
            Crlzh decoder(decoded.header.revision == 1, &bits, &output);
            okay = decoder.decode();
        } else if (decoded.header.format == Format::UnixCompact) {
            okay = compact(&bits, &output);
            if (!okay) return false;
            decoded.consumed = bits.position();
            decoded.uncompressedSize = output.size();
            *result = std::move(decoded);
            return true;
        }
        if (!okay || !output.complete() || size - bits.position() < 2) return false;
        decoded.consumed = bits.position() + 2;
        decoded.checksum = static_cast<std::uint16_t>(data[bits.position()] | (static_cast<unsigned>(data[bits.position() + 1]) << 8));
        if (decoded.checksum != output.checksum()) return false;
        // CP/M files can fill their final 128-byte record after the checksum.
        if (size != decoded.consumed && ((size % 128) != 0 || size - decoded.consumed > 127)) return false;
        decoded.uncompressedSize = output.size();
        *result = std::move(decoded);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    } catch (const XAncientPrivate::CodecError &) {
        return false;
    }
}
