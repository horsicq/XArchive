// The reference implementation; see provenance for exact address roles.
#include "xrzipdecoder.h"
#include "xbzip2decoder.h"

#include <QBuffer>
#include <QMap>
#include <QtEndian>
#include <cstring>
#include <new>

namespace {
const qint32 MaxBlocks = 100000;
const qint32 MaxEmptyHeaders = 1024;
quint32 le32(const char *p) { return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(p)); }
bool within(qint64 total, qint64 pos, qint64 size) { return pos >= 0 && size >= 0 && pos <= total && size <= total - pos; }

class RzipStreams final {
public:
    RzipStreams(const QByteArray &data, qint64 start, qint64 memoryLimit, XBinary::PDSTRUCT *progress)
        : m_data(data), m_start(start), m_memoryLimit(memoryLimit), m_progress(progress) {}
    bool open()
    {
        // rzip 2.0 could leave an empty first header when closing the previous
        // chunk. The reference implementation advances the offset base for every such header.
        qint32 empty = 0;
        while (within(m_data.size(), m_start, 13)) {
            const char *p = m_data.constData() + m_start;
            if (quint8(p[0]) != 3 || le32(p + 1) || le32(p + 5) || le32(p + 9)) break;
            if (++empty > MaxEmptyHeaders || !XBinary::isPdStructNotCanceled(m_progress)) return false;
            m_start += 13;
        }
        if (!within(m_data.size(), m_start, 26)) return false;
        for (int i = 0; i < 2; ++i) {
            const char *p = m_data.constData() + m_start + i * 13;
            if (quint8(p[0]) != 3 || le32(p + 1) || le32(p + 5)) return false;
            m_streams[i].next = le32(p + 9);
            if (m_streams[i].next && m_streams[i].next < 26) return false;
        }
        return m_streams[0].next != 0;
    }
    bool read(int stream, char *destination, qint32 count)
    {
        if (stream < 0 || stream > 1 || count < 0 || (count && !destination)) return false;
        Stream &s = m_streams[stream];
        while (count) {
            if (!XBinary::isPdStructNotCanceled(m_progress)) return false;
            if (s.position == s.buffer.size() && !load(stream)) return false;
            const qint32 take = qint32(qMin<qint64>(count, s.buffer.size() - s.position));
            if (take <= 0) return false;
            std::memcpy(destination, s.buffer.constData() + s.position, size_t(take));
            s.position += take;
            destination += take;
            count -= take;
        }
        return true;
    }
    bool finish(qint64 *end, qint32 *blocks) const
    {
        for (const Stream &s : m_streams) if (s.next || s.position != s.buffer.size()) return false;
        // Every physical header/body must be visited exactly once. Linked stream
        // offsets may interleave, but they cannot overlap or hide a hole/overlay.
        qint64 expected = m_start + 26;
        for (QMap<qint64,qint64>::const_iterator i = m_extents.cbegin(); i != m_extents.cend(); ++i) {
            if (i.key() != expected) return false;
            expected = i.value();
        }
        *end = expected;
        *blocks = m_extents.size();
        return XBinary::isPdStructNotCanceled(m_progress);
    }
private:
    struct Stream { quint32 next = 0; QByteArray buffer; qint64 position = 0; };
    bool load(int stream)
    {
        Stream &s = m_streams[stream];
        if (!s.next || m_extents.size() >= MaxBlocks) return false;
        const qint64 pos = m_start + s.next;
        if (!within(m_data.size(), pos, 13)) return false;
        const char *p = m_data.constData() + pos;
        const quint8 type = quint8(p[0]);
        const qint64 packed = le32(p + 1), raw = le32(p + 5);
        const quint32 next = le32(p + 9);
        if ((type != 3 && type != 4) || packed <= 0 || raw <= 0 || packed > XRzipDecoder::MaxBlock || raw > m_memoryLimit ||
            !within(m_data.size(), pos + 13, packed) || (type == 3 && packed != raw) ||
            (next && qint64(next) < pos + 13 + packed - m_start)) return false;
        const qint64 end = pos + 13 + packed;
        const QMap<qint64,qint64>::iterator after = m_extents.lowerBound(pos);
        if (after != m_extents.end() && after.key() < end) return false;
        if (after != m_extents.begin() && std::prev(after).value() > pos) return false;
        QByteArray decoded;
        if (type == 3) {
            decoded = QByteArray(p + 13, qsizetype(raw));
        } else {
            QByteArray input = QByteArray::fromRawData(p + 13, qsizetype(packed));
            QBuffer source(&input), output(&decoded);
            if (!source.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly)) return false;
            XBinary::DATAPROCESS_STATE state = {};
            state.pDeviceInput = &source;
            state.pDeviceOutput = &output;
            state.nInputLimit = packed;
            state.nProcessedLimit = -1;
            state.mapUnpackProperties.insert(XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE, raw);
            state.mapUnpackProperties.insert(XBinary::UNPACK_PROP_MAX_MEMORY_OUTPUT_SIZE, raw);
            if (!XBZIP2Decoder::decompress(&state, m_progress) || state.nCountInput != packed || state.nCountOutput != raw || decoded.size() != raw)
                return false;
        }
        s.buffer = std::move(decoded);
        s.position = 0;
        s.next = next;
        m_extents.insert(pos, end);
        return true;
    }
    const QByteArray &m_data;
    qint64 m_start;
    qint64 m_memoryLimit;
    XBinary::PDSTRUCT *m_progress;
    Stream m_streams[2];
    QMap<qint64,qint64> m_extents;
};
}

bool XRzipDecoder::parseHeader(const QByteArray &data, HEADER *header)
{
    if (!header || data.size() < 24 || std::memcmp(data.constData(), "RZIP", 4)) return false;
    const uchar *p = reinterpret_cast<const uchar *>(data.constData());
    // Recovered framing is corroborated by the original rzip 2.1 writer.
    if (p[4] != 2 || p[5] > 1) return false;
    for (int i = 14; i < 24; ++i) if (p[i]) return false;
    const quint64 raw = qFromBigEndian<quint32>(p + 6) | (quint64(qFromBigEndian<quint32>(p + 10)) << 32);
    if (raw > quint64(MaxOutput)) return false;
    header->major = p[4]; header->minor = p[5]; header->rawSize = qint64(raw);
    return true;
}

bool XRzipDecoder::decode(const QByteArray &data, XBinary::DATAPROCESS_STATE *writer, RESULT *result, XBinary::PDSTRUCT *progress)
{
    HEADER header;
    if (!writer || !result || data.size() > MaxInput || !parseHeader(data, &header) || !writer->pDeviceOutput ||
        writer->nProcessedOffset != 0 || writer->nProcessedLimit != -1 || !XBinary::isPdStructNotCanceled(progress)) return false;
    *result = RESULT();
    QIODevice *history = writer->pDeviceOutput;
    if (!history->isOpen() || !history->isReadable() || !history->isWritable() || history->isSequential() || history->pos() != 0 || history->size() != 0)
        return false;
    XBinary::OUTPUT_POLICY policy = {};
    if (!XBinary::resolveUnpackOutputPolicy(writer->mapUnpackProperties, &policy) ||
        (policy.nMaxEntryOutputSize >= 0 && header.rawSize > policy.nMaxEntryOutputSize)) return false;
    qint64 memoryLimit = MaxBlock;
    if (policy.nMaxMemoryOutputSize >= 0) memoryLimit = qMin(memoryLimit, policy.nMaxMemoryOutputSize);
    writer->nCountInput = 0; writer->nCountOutput = 0;
    qint64 physical = 24;
    char scratch[65535];
    try {
        while (writer->nCountOutput < header.rawSize) {
            if (!history || !XBinary::isPdStructNotCanceled(progress) || result->chunks >= MaxBlocks) return false;
            RzipStreams streams(data, physical, memoryLimit, progress);
            if (!streams.open()) return false;
            const qint64 chunkStart = writer->nCountOutput;
            quint32 checksum = 0;
            while (true) {
                char token[3];
                if (!streams.read(0, token, 3)) return false;
                const quint32 length = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(token + 1));
                if (length == 0) {
                    char stored[4];
                    if (token[0] || !streams.read(0, stored, 4) || le32(stored) != checksum) return false;
                    break;
                }
                if (!history || !XBinary::isPdStructNotCanceled(progress) || qint64(length) > header.rawSize - writer->nCountOutput) return false;
                if (token[0] == 0) {
                    if (!streams.read(1, scratch, qint32(length))) return false;
                } else {
                    char offsetBytes[4];
                    if (!streams.read(0, offsetBytes, 4)) return false;
                    const quint32 distance = le32(offsetBytes);
                    const qint64 position = writer->nCountOutput;
                    if (!distance || qint64(distance) > position || !history || !history->seek(position - distance) || !history) return false;
                    const qint32 initial = qint32(qMin(length, distance));
                    if (history->read(scratch, initial) != initial || !history || !history->seek(position) || !history) return false;
                    for (quint32 i = quint32(initial); i < length; ++i) scratch[i] = scratch[i - distance];
                }
                if (XBinary::_writeDevice(scratch, qint32(length), writer) != qint32(length) || !history) return false;
                checksum = XBinary::_getCRC32(scratch, qint32(length), checksum, XBinary::_getCRC32Table_EDB88320());
            }
            qint64 end = 0;
            qint32 blocks = 0;
            if (writer->nCountOutput == chunkStart || !streams.finish(&end, &blocks) || end <= physical) return false;
            if (blocks > MaxBlocks - result->blocks) return false;
            result->blocks += blocks;
            ++result->chunks;
            physical = end;
        }
    } catch (const std::bad_alloc &) { return false; }
    if (!history || !XBinary::isPdStructNotCanceled(progress) || physical != data.size() || writer->nCountOutput != header.rawSize ||
        history->size() != header.rawSize) return false;
    writer->nCountInput = physical;
    result->consumed = physical;
    result->outputSize = writer->nCountOutput;
    return true;
}
