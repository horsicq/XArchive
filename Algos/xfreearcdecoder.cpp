/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * Native FreeArc decoding only. See xfreearcdecoder.PROVENANCE.md.
 * The x86 converter follows Igor Pavlov's public-domain Bra86.c.
 * DELTA binary tables algorithm: (c) Bulat.Ziganshin@gmail.com, 2013-09-18.
 * All rights reserved. You can for free use decompression part of the algorithm
 * for decompression of FreeArc archives. For any other usage ask me for the license.
 */
#include "xfreearcdecoder.h"

#include "xlzmadecoder.h"
#include <QBuffer>
#include <QtEndian>
#include <limits>
#include <new>

namespace {
const qint64 MAX_BUFFER = Q_INT64_C(256) * 1024 * 1024;
const quint64 MAX_DICTIONARY = Q_UINT64_C(64) * 1024 * 1024;
enum Kind { Store, Lzma, Rep, Exe, Delta };
struct Method { Kind kind = Store; QByteArray properties; };

bool decimal(QByteArray value, quint64 *number)
{
    if (!number || value.isEmpty() || value.size() > 20) return false;
    if (value.startsWith('=')) value.remove(0, 1);
    if (value.isEmpty()) return false;
    quint64 result = 0;
    for (char c : value) {
        if (c < '0' || c > '9' || result > ((std::numeric_limits<quint64>::max)() - quint64(c - '0')) / 10) return false;
        result = result * 10 + quint64(c - '0');
    }
    *number = result;
    return true;
}

bool memory(QByteArray value, quint64 *number)
{
    quint64 multiplier = 1;
    bool power = false;
    if (value.endsWith("gb")) { multiplier = Q_UINT64_C(1) << 30; value.chop(2); }
    else if (value.endsWith('g')) { multiplier = Q_UINT64_C(1) << 30; value.chop(1); }
    else if (value.endsWith("mb")) { multiplier = Q_UINT64_C(1) << 20; value.chop(2); }
    else if (value.endsWith('m')) { multiplier = Q_UINT64_C(1) << 20; value.chop(1); }
    else if (value.endsWith("kb")) { multiplier = 1024; value.chop(2); }
    else if (value.endsWith('k')) { multiplier = 1024; value.chop(1); }
    else if (value.endsWith('b')) value.chop(1);
    else { power = true; if (value.endsWith('^')) value.chop(1); }
    quint64 n = 0;
    if (!decimal(value, &n)) return false;
    if (power) {
        if (n >= 63) return false;
        *number = Q_UINT64_C(1) << n;
    } else {
        if (n > (std::numeric_limits<quint64>::max)() / multiplier) return false;
        *number = n * multiplier;
    }
    return *number != 0;
}

bool parseMethod(const QByteArray &text, Method *method)
{
    const QList<QByteArray> parts = text.split(':');
    const QByteArray &name = parts.first();
    if (name == "storing" || name == "exe") {
        method->kind = name == "storing" ? Store : Exe;
        return parts.size() == 1;
    }
    if (name == "rep" || name == "delta") {
        method->kind = name == "rep" ? Rep : Delta;
        if (parts.size() == 1) return true;
        if (parts.size() != 2) return false;
        QByteArray option = parts.at(1);
        if (option.startsWith('b')) option.remove(0, 1);
        quint64 block = 0;
        // These encoder buffer options do not replace the stream framing.
        return memory(option, &block) && block <= quint64(MAX_BUFFER);
    }
    if (name != "lzma") return false;
    method->kind = Lzma;
    quint64 dictionary = MAX_DICTIONARY, lc = 3, lp = 0, pb = 2;
    for (int i = 1; i < parts.size(); ++i) {
        QByteArray option = parts.at(i);
        if (option.startsWith('*')) option.remove(0, 1);
        quint64 n = 0;
        if (option == "fastest" || option == "fast" || option == "normal" || option == "max" || option == "ultra" ||
            option == "ht4" || option == "hc4" || option == "bt2" || option == "bt3" || option == "bt4") continue;
        if (option.startsWith("lc") || option.startsWith("lp") || option.startsWith("pb")) {
            if (!decimal(option.mid(2), &n)) return false;
            if (option.startsWith("lc")) lc = n;
            else if (option.startsWith("lp")) lp = n;
            else pb = n;
        } else if (option.startsWith('d')) {
            if (!memory(option.mid(1), &dictionary)) return false;
        } else if (option.startsWith("fb") || option.startsWith("mc")) {
            if (!decimal(option.mid(2), &n) || n > 0x7fffffffU) return false;
        } else if (option.startsWith("mf")) {
            QByteArray finder = option.mid(2);
            if (finder.startsWith('=')) finder.remove(0, 1);
            if (finder != "ht4" && finder != "hc4" && finder != "bt2" && finder != "bt3" && finder != "bt4") return false;
        } else if (option.startsWith('a')) {
            if (!decimal(option.mid(1), &n) || n > 2) return false;
        } else if (option.startsWith('h')) {
            if (!memory(option.mid(1), &n) || n > quint64(MAX_BUFFER)) return false;
        } else if (decimal(option, &n)) {
            if (n < 5 || n > 273) return false; // encoder fast-byte count
        } else if (!memory(option, &dictionary)) return false;
    }
    if (!dictionary || dictionary > MAX_DICTIONARY || lc > 8 || lp > 4 || pb > 4 || lc + lp > 4) return false;
    method->properties = QByteArray(5, 0);
    method->properties[0] = char((pb * 5 + lp) * 9 + lc);
    qToLittleEndian<quint32>(quint32(dictionary), reinterpret_cast<uchar *>(method->properties.data() + 1));
    return true;
}

bool parseChain(const QByteArray &text, QList<Method> *methods)
{
    if (text.isEmpty() || text.size() > 1024 || text.contains('\0')) return false;
    const QList<QByteArray> chain = text.split('+');
    if (chain.size() > 16) return false;
    for (const QByteArray &part : chain) {
        Method method;
        if (part.isEmpty() || !parseMethod(part, &method)) return false;
        methods->append(method);
    }
    return true;
}

quint32 le32(const QByteArray &data, qint64 offset)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(data.constData() + offset));
}

bool append(QByteArray *out, const char *data, qint64 length, qint64 limit)
{
    if (length < 0 || out->size() > limit || length > limit - out->size()) return false;
    // Explicit reserve avoids geometric append growth outside memoryRequired.
    const int size = static_cast<int>(out->size() + length);
    if (size > out->capacity()) out->reserve(size);
    out->append(data, static_cast<int>(length));
    return true;
}

class LimitedBuffer : public QBuffer {
public:
    LimitedBuffer(QByteArray *data, qint64 limit) : QBuffer(data), m_limit(limit) {}
protected:
    qint64 writeData(const char *data, qint64 length) override
    {
        if (length < 0 || pos() > m_limit || length > m_limit - pos()) return -1;
        const int size = static_cast<int>(pos() + length);
        if (size > buffer().capacity()) buffer().reserve(size);
        return QBuffer::writeData(data, length);
    }
private:
    qint64 m_limit;
};

bool lzma(const QByteArray &input, const Method &method, qint64 expected, qint64 limit, QByteArray *out, XBinary::PDSTRUCT *pd)
{
    QBuffer source;
    source.setData(input);
    LimitedBuffer sink(out, limit);
    if (!source.open(QIODevice::ReadOnly) || !sink.open(QIODevice::ReadWrite)) return false;
    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &source;
    state.pDeviceOutput = &sink;
    state.nInputOffset = 0;
    state.nInputLimit = input.size();
    state.nProcessedLimit = -1;
    state.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, expected);
    // Keep the independent stage-output bound in LimitedBuffer. The legacy
    // MAX_OUTPUT_SIZE key also caps the SDK dictionary plus I/O reservation,
    // so using a tiny final-output size there would reject valid empty/tiny
    // streams. parseMethod/memoryRequired bound and account SDK workspace.
    return XLZMADecoder::decompressWithResult(&state, method.properties, pd) == XLZMADecoder::DECOMPRESS_RESULT_SUCCESS &&
           state.nCountInput == input.size() && state.nCountOutput == out->size() && out->size() <= limit &&
           (expected < 0 || out->size() == expected) && XBinary::isPdStructNotCanceled(pd);
}

// U3 00587200: window size, framed table/literal blocks and a zero block EOF.
bool rep(const QByteArray &input, qint64 limit, QByteArray *out, XBinary::PDSTRUCT *pd)
{
    if (input.size() < 8) return false;
    const quint32 window = le32(input, 0);
    if (!window || window > 0x7fffffffU) return false;
    qint64 cursor = 4;
    while (cursor <= input.size() - 4 && XBinary::isPdStructNotCanceled(pd)) {
        const quint32 length = le32(input, cursor);
        cursor += 4;
        if (!length) return cursor == input.size();
        if (length < 8 || length > quint64(input.size() - cursor)) return false;
        const qint64 end = cursor + length;
        const quint32 count = le32(input, cursor);
        if (count > (length - 8) / 12) return false;
        const qint64 lengths = cursor + 4, distances = lengths + qint64(count) * 4;
        const qint64 literalLengths = distances + qint64(count) * 4;
        qint64 literals = literalLengths + (qint64(count) + 1) * 4;
        for (quint32 i = 0; i <= count; ++i) {
            if (!XBinary::isPdStructNotCanceled(pd)) return false;
            const quint32 literalSize = le32(input, literalLengths + qint64(i) * 4);
            if (literalSize > quint64(end - literals) || !append(out, input.constData() + literals, literalSize, limit)) return false;
            literals += literalSize;
            if (i == count) break;
            const quint32 matchSize = le32(input, lengths + qint64(i) * 4);
            const quint32 distance = le32(input, distances + qint64(i) * 4);
            if (matchSize > quint64(limit - out->size()) || (matchSize && (!distance || distance > window || distance > quint64(out->size())))) return false;
            const qint64 start = out->size();
            if (start + matchSize > out->capacity()) out->reserve(static_cast<int>(start + matchSize));
            out->resize(static_cast<int>(start + matchSize));
            char *bytes = out->data();
            for (quint32 j = 0; j < matchSize; ++j) {
                if ((j & 4095U) == 0 && !XBinary::isPdStructNotCanceled(pd)) return false;
                bytes[start + j] = bytes[start + j - distance];
            }
        }
        if (literals != end) return false;
        cursor = end;
    }
    return false;
}

// U3 00586dd0/00586c00/00586cc0/00586c40: unshuffle then undiff tables.
bool delta(const QByteArray &input, qint64 limit, QByteArray *out, XBinary::PDSTRUCT *pd)
{
    qint64 cursor = 0;
    while (cursor < input.size() && XBinary::isPdStructNotCanceled(pd)) {
        if (input.size() - cursor < 8) return false;
        const quint32 size = le32(input, cursor), tableBytes = le32(input, cursor + 4);
        cursor += 8;
        if (size > quint64(limit - out->size()) || tableBytes > 0x7fffffffU || (tableBytes & 3U) ||
            qint64(tableBytes) * 3 + size > input.size() - cursor) return false;
        const qint64 skips = cursor, types = skips + tableBytes, rows = types + tableBytes;
        cursor += qint64(tableBytes) * 3;
        QByteArray block = input.mid(static_cast<int>(cursor), static_cast<int>(size));
        cursor += size;
        qint64 position = 0;
        for (quint32 i = 0; i < tableBytes / 4; ++i) {
            if (!XBinary::isPdStructNotCanceled(pd)) return false;
            quint32 type = le32(input, types + qint64(i) * 4);
            const quint32 skip = le32(input, skips + qint64(i) * 4), rowCount = le32(input, rows + qint64(i) * 4);
            bool immutable[31] = {};
            int width = 0, immutableCount = 0;
            while (type > 1 && width < 31) {
                immutable[width++] = (type & 1U) != 0;
                immutableCount += int(type & 1U);
                type >>= 1;
            }
            const qint64 bytes = qint64(width) * rowCount;
            if (!width || type != 1 || !rowCount || skip > quint64(size - position) || bytes > size - position - skip) return false;
            position += skip;
            if (immutableCount && immutableCount != width) {
                const QByteArray shuffled = block.mid(static_cast<int>(position), static_cast<int>(bytes));
                qint64 a = 0, b = qint64(immutableCount) * rowCount;
                for (quint32 row = 0; row < rowCount; ++row) {
                    if ((row & 4095U) == 0 && !XBinary::isPdStructNotCanceled(pd)) return false;
                    for (int column = 0; column < width; ++column) {
                        block[static_cast<int>(position + qint64(row) * width + column)] = shuffled.at(static_cast<int>(immutable[column] ? a++ : b++));
                    }
                }
            }
            for (quint32 row = 1; row < rowCount; ++row) {
                if ((row & 4095U) == 0 && !XBinary::isPdStructNotCanceled(pd)) return false;
                quint32 carry = 0;
                for (int column = 0; column < width; ++column) {
                    if (immutable[column]) { carry = 0; continue; }
                    const int at = static_cast<int>(position + qint64(row) * width + column);
                    const quint32 sum = quint8(block.at(at)) + quint8(block.at(at - width)) + carry;
                    block[at] = char(sum & 255U);
                    carry = sum >> 8;
                }
            }
            position += bytes;
        }
        if (!append(out, block.constData(), block.size(), limit)) return false;
    }
    return cursor == input.size() && XBinary::isPdStructNotCanceled(pd);
}

bool msByte(quint32 byte) { return byte == 0 || byte == 255; }

// Public-domain Bra86 state machine also recovered at U3 00438670.
int exeBlock(char *buffer, int size, quint32 ip, quint32 *state, XBinary::PDSTRUCT *pd)
{
    static const quint8 allowed[8] = {1, 1, 1, 0, 1, 0, 0, 0};
    static const quint8 bit[8] = {0, 1, 2, 2, 3, 3, 3, 3};
    uchar *data = reinterpret_cast<uchar *>(buffer);
    int position = 0, previous = -1;
    quint32 mask = *state & 7U;
    ip += 5;
    for (;;) {
        while (position < size - 4 && (data[position] & 0xfe) != 0xe8) {
            if ((position & 4095) == 0 && !XBinary::isPdStructNotCanceled(pd)) return -1;
            ++position;
        }
        if (position >= size - 4) break;
        if (!XBinary::isPdStructNotCanceled(pd)) return -1;
        const int distance = position - previous;
        if (distance > 3) mask = 0;
        else {
            mask = (mask << (distance - 1)) & 7U;
            if (mask && (!allowed[mask] || msByte(data[position + 4 - bit[mask]]))) {
                previous = position++;
                mask = ((mask << 1) & 7U) | 1U;
                continue;
            }
        }
        previous = position;
        if (msByte(data[position + 4])) {
            quint32 source = qFromLittleEndian<quint32>(data + position + 1), target = 0;
            for (;;) {
                target = source - (ip + quint32(position));
                if (!mask) break;
                const int shift = bit[mask] * 8;
                if (!msByte(quint8(target >> (24 - shift)))) break;
                source = target ^ ((quint32(1) << (32 - shift)) - 1U);
            }
            qToLittleEndian<quint32>(target, data + position + 1);
            data[position + 4] = quint8(0U - ((target >> 24) & 1U));
            position += 5;
        } else {
            mask = ((mask << 1) & 7U) | 1U;
            ++position;
        }
    }
    const int distance = position - previous;
    *state = distance > 3 ? 0 : (mask << (distance - 1)) & 7U;
    return position;
}

bool exe(const QByteArray &input, qint64 limit, QByteArray *out, XBinary::PDSTRUCT *pd)
{
    if (input.size() > limit) return false;
    *out = QByteArray(input.constData(), input.size());
    qint64 offset = 0;
    quint32 state = 0;
    // U3 00438a20 reads 64 KiB, retaining the unfinished converter suffix.
    while (offset < out->size() && XBinary::isPdStructNotCanceled(pd)) {
        const int available = static_cast<int>(qMin<qint64>(65536, out->size() - offset));
        if (available <= 5) break;
        const int processed = exeBlock(out->data() + offset, available, quint32(offset), &state, pd);
        if (processed <= 0 || processed > available) return false;
        offset += processed;
    }
    return XBinary::isPdStructNotCanceled(pd);
}
} // namespace

bool XFreeArcDecoder::supports(const QByteArray &method)
{
    QList<Method> methods;
    return parseChain(method, &methods);
}

qint64 XFreeArcDecoder::memoryRequired(const QByteArray &method, qint64 inputCapacity,
                                     qint64 maxStageBytes, XBinary::PDSTRUCT *pd)
{
    QList<Method> methods;
    if (inputCapacity < 0 || maxStageBytes < 0 || maxStageBytes > MAX_BUFFER ||
        !parseChain(method, &methods) || !XBinary::isPdStructNotCanceled(pd)) return -1;
    qint64 workspace = 0;
    for (const Method &stage : methods) {
        if (stage.kind == Lzma) {
            qint64 required = 0;
            if (!XLZMADecoder::getMemoryRequirement(stage.properties, &required, pd)) return -1;
            workspace = qMax(workspace, required);
        }
    }
    // Sequential stages retain their input while producing output. This also
    // covers an old allocation during reserve, DELTA's block/shuffle scratch,
    // and the maximum LZMA dictionary/probability/I/O workspace. The caller
    // charges its original input once; QByteArray copies share that allocation.
    const qint64 extra = 6 * maxStageBytes + workspace + 65536;
    if (inputCapacity > (std::numeric_limits<qint64>::max)() - extra) return -1;
    return inputCapacity + extra;
}

bool XFreeArcDecoder::decode(const QByteArray &input, const QByteArray &method,
                            qint64 expectedSize, qint64 maxBytes, QByteArray *output,
                            XBinary::PDSTRUCT *pd)
try
{
    if (!output) return false;
    QByteArray current = input;
    QList<Method> methods;
    const bool supported = parseChain(method, &methods);
    output->clear();
    const qint64 limit = qMin(maxBytes, MAX_BUFFER);
    if (!supported || maxBytes < 0 || expectedSize < -1 || expectedSize > limit || current.size() > MAX_BUFFER ||
        !XBinary::isPdStructNotCanceled(pd)) return false;
    for (int i = static_cast<int>(methods.size()) - 1; i >= 0; --i) {
        const Method &stage = methods.at(i);
        QByteArray decoded;
        bool ok = false;
        switch (stage.kind) {
        case Store: ok = append(&decoded, current.constData(), current.size(), limit); break;
        case Lzma: ok = lzma(current, stage, i == 0 ? expectedSize : -1, limit, &decoded, pd); break;
        case Rep: ok = rep(current, limit, &decoded, pd); break;
        case Exe: ok = exe(current, limit, &decoded, pd); break;
        case Delta: ok = delta(current, limit, &decoded, pd); break;
        }
        if (!ok || !XBinary::isPdStructNotCanceled(pd)) return false;
        current = decoded;
    }
    if (expectedSize >= 0 && current.size() != expectedSize) return false;
    *output = current;
    return true;
}
catch (const std::bad_alloc &)
{
    if (output) output->clear();
    return false;
}
