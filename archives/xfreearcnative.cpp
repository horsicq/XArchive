/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#include "xfreearcnative.h"

#include "Algos/xfreearcdecoder.h"
#include <QDir>
#include <QFile>
#include <limits>

namespace {
const qint64 kControlLimit = 16 * 1024 * 1024;
const qint64 kSolidLimit = 256 * 1024 * 1024;
const qint64 kEntryLimit = 1000000;
// Covers small cursors, temporary UTF-8/UTF-16 name conversions, QFile and
// container-allocation bookkeeping. Variable metadata is charged separately.
const qint64 kMetadataSlack = 64 * 1024;

bool sumFits(qint64 limit, qint64 first, qint64 second)
{
    return first >= 0 && second >= 0 && first <= limit && second <= limit - first;
}

bool decodePlan(const QByteArray &method, qint64 packed, qint64 unpacked, qint64 stageLimit, qint64 allowance, qint64 *stageCap,
                XBinary::PDSTRUCT *pd)
{
    if (packed < 0 || unpacked < 0 || unpacked > stageLimit || allowance < 0) return false;
    if (!method.contains('+')) {
        const qint64 needed = XFreeArcDecoder::memoryRequired(method, packed, unpacked, pd);
        *stageCap = unpacked;
        return needed >= 0 && needed <= allowance;
    }
    // Intermediate framed streams can exceed the final logical file size.
    // Find the largest stage bound whose complete decoder estimate fits.
    qint64 low = 0, high = stageLimit, best = -1;
    while (low <= high && XBinary::isPdStructNotCanceled(pd)) {
        const qint64 middle = low + (high - low) / 2;
        const qint64 needed = XFreeArcDecoder::memoryRequired(method, packed, middle, pd);
        if (needed >= 0 && needed <= allowance) { best = middle; low = middle + 1; }
        else high = middle - 1;
    }
    *stageCap = best;
    return best >= unpacked && XBinary::isPdStructNotCanceled(pd);
}

bool checkedCRC32(const char *data, qint64 size, quint32 *value, XBinary::PDSTRUCT *pd)
{
    quint32 crc = 0xffffffffU;
    qint64 offset = 0;
    while (offset < size && XBinary::isPdStructNotCanceled(pd)) {
        const qint64 length = qMin<qint64>(65536, size - offset);
        crc = XBinary::_getCRC32(data + offset, length, crc, XBinary::_getCRC32Table_EDB88320());
        offset += length;
    }
    *value = crc ^ 0xffffffffU;
    return offset == size && XBinary::isPdStructNotCanceled(pd);
}

quint32 crc32(const QByteArray &bytes)
{
    return XBinary::_getCRC32(bytes, 0xffffffffU, XBinary::_getCRC32Table_EDB88320()) ^ 0xffffffffU;
}

bool readAt(QIODevice *device, qint64 offset, qint64 size, qint64 limit, QByteArray *bytes, XBinary::PDSTRUCT *pd)
{
    QIODevice *source = device;
    if (!bytes || offset < 0 || size < 0 || size > limit || size > kSolidLimit || !XBinary::isPdStructNotCanceled(pd)) return false;
    const qint64 total = source->size();
    if (offset > total || size > total - offset || !XBinary::isPdStructNotCanceled(pd)) return false;
    const bool seeked = source->seek(offset);
    if (!seeked || !XBinary::isPdStructNotCanceled(pd)) return false;
    // One exact-capacity input allocation, not a growing array plus a second
    // chunk array. The decoder's estimate includes this retained input.
    *bytes = QByteArray((qint32)size, Qt::Uninitialized);
    qint64 done = 0;
    while (done < size && source && XBinary::isPdStructNotCanceled(pd)) {
        const qint64 request = qMin<qint64>(65536, size - done);
        const qint64 received = source->read(bytes->data() + done, request);
        if (received <= 0 || received > request) { bytes->clear(); return false; }
        done += received;
    }
    if (done != size || !XBinary::isPdStructNotCanceled(pd)) { bytes->clear(); return false; }
    return true;
}

struct Cursor {
    QByteArray bytes;
    qint64 pos = 0;

    // A default member initialiser keeps this out of the aggregate category in
    // C++11, so the brace-initialised cursors below need a real constructor.
    Cursor(const QByteArray &data, qint64 nPos = 0) : bytes(data), pos(nPos)
    {
    }
    bool number(qint64 *value)
    {
        if (pos >= bytes.size()) return false;
        const quint8 first = quint8(bytes.at(pos++));
        quint64 result = first;
        qint32 extra = 0;
        while (extra < 8 && (first & (1U << extra))) ++extra;
        if (extra > bytes.size() - pos) return false;
        if (extra == 8) result = 0;
        for (qint32 i = 0; i < extra; ++i) result |= quint64(quint8(bytes.at(pos++))) << (8 * (extra == 8 ? i : i + 1));
        if (extra != 8) result >>= extra + 1;
        if (result > quint64((std::numeric_limits<qint64>::max)())) return false;
        *value = qint64(result);
        return true;
    }
    bool u32(quint32 *value)
    {
        if (bytes.size() - pos < 4) return false;
        *value = 0;
        for (qint32 i = 0; i < 4; ++i) *value |= quint32(quint8(bytes.at(pos++))) << (8 * i);
        return true;
    }
    bool flag(bool *value)
    {
        if (pos >= bytes.size() || quint8(bytes.at(pos)) > 1) return false;
        *value = bytes.at(pos++) != 0;
        return true;
    }
    bool string(QByteArray *value, qint64 limit = 4096)
    {
        const qsizetype end = bytes.indexOf('\0', pos);
        if (end < pos || end - pos > limit) return false;
        *value = bytes.mid(pos, end - pos);
        pos = end + 1;
        return true;
    }
    bool zeroTail() const
    {
        if (bytes.size() - pos > 8) return false;
        for (qint64 i = pos; i < bytes.size(); ++i) if (bytes.at(i)) return false;
        return true;
    }
};

struct Descriptor {
    qint64 type = 0, offset = 0, unpacked = 0, packed = 0;
    QByteArray method;
    quint32 crc = 0;
};

bool localDescriptor(const QByteArray &tail, qint64 start, qint64 tailOffset, Descriptor *descriptor)
{
    Cursor c{tail, start + 4};
    Descriptor d;
    quint32 ownCRC = 0;
    if (!c.number(&d.type) || !c.string(&d.method) || !c.number(&d.unpacked) || !c.number(&d.packed) || !c.u32(&d.crc)) return false;
    const qint64 checkedEnd = c.pos;
    if (!c.u32(&ownCRC) || ownCRC != crc32(tail.mid(start, checkedEnd - start))) return false;
    const qint64 physical = tailOffset + start;
    if (d.packed > physical) return false;
    d.offset = physical - d.packed;
    *descriptor = d;
    return true;
}

XFreeArcNative::RESULT decodeControl(QIODevice *device, const Descriptor &d, qint64 allowance, QByteArray *output, XBinary::PDSTRUCT *pd)
{
    if (!XFreeArcDecoder::supports(d.method)) return XFreeArcNative::UNSUPPORTED;
    qint64 stageCap = 0;
    if (d.unpacked > kControlLimit || d.packed > kControlLimit || !decodePlan(d.method, d.packed, d.unpacked, kControlLimit, allowance, &stageCap, pd))
        return XBinary::isPdStructNotCanceled(pd) ? XFreeArcNative::RESOURCE_LIMIT : XFreeArcNative::CANCELED;
    QByteArray packed;
    quint32 crc = 0;
    if (!readAt(device, d.offset, d.packed, kControlLimit, &packed, pd) ||
        !XFreeArcDecoder::decode(packed, d.method, d.unpacked, stageCap, output, pd) ||
        !checkedCRC32(output->constData(), output->size(), &crc, pd) || crc != d.crc)
        return XBinary::isPdStructNotCanceled(pd) ? XFreeArcNative::INVALID : XFreeArcNative::CANCELED;
    return XFreeArcNative::READY;
}

bool utf8Name(const QByteArray &raw, QString *name)
{
    *name = QString::fromUtf8(raw);
    if (name->toUtf8() != raw) return false;
    name->replace(QLatin1Char('\\'), QLatin1Char('/'));
    return true;
}
}  // namespace

bool XFreeArcNative::chargeMetadata(qint64 amount, qint64 liveBufferBytes)
{
    if (!sumFits(m_memoryLimit, m_metadataCharge, liveBufferBytes) || !sumFits(m_memoryLimit - liveBufferBytes, m_metadataCharge, amount)) return false;
    m_metadataCharge += amount;
    return true;
}

XFreeArcNative::RESULT XFreeArcNative::open(QIODevice *device, const XBinary::OUTPUT_POLICY &policy, XBinary::PDSTRUCT *pd)
{
    m_device = device;
    m_policy = policy;
    m_entries.clear();
    m_blocks.clear();
    m_metadataCharge = 0;
    m_memoryLimit = policy.nMaxMemoryOutputSize >= 0 ? qMin(kSolidLimit, policy.nMaxMemoryOutputSize) : kSolidLimit;
    if (!chargeMetadata(kMetadataSlack)) return RESOURCE_LIMIT;
    if (!m_device || !XBinary::isPdStructNotCanceled(pd)) return CANCELED;
    const bool sequential = m_device->isSequential();
    if (!m_device || !XBinary::isPdStructNotCanceled(pd)) return CANCELED;
    if (sequential) return INVALID;
    const qint64 size = m_device->size();
    if (!m_device || !XBinary::isPdStructNotCanceled(pd)) return CANCELED;
    QByteArray header, tail;
    if (!readAt(m_device, 0, 12, 12, &header, pd)) return XBinary::isPdStructNotCanceled(pd) ? INVALID : CANCELED;
    if (header.left(4) != QByteArray("ArC\1", 4) || header.mid(8, 4) != QByteArray("ArC\1", 4)) return INVALID;
    const qint64 tailOffset = qMax<qint64>(0, size - 4096);
    if (!readAt(m_device, tailOffset, size - tailOffset, 4096, &tail, pd)) return XBinary::isPdStructNotCanceled(pd) ? INVALID : CANCELED;
    // The reference implementation locates the final descriptor verifies its CRC.
    const qint64 start = tail.lastIndexOf(QByteArray("ArC\1", 4));
    Descriptor footer;
    if (start < 0 || !localDescriptor(tail, start, tailOffset, &footer) || footer.type != 4) return INVALID;
    QByteArray footerData;
    RESULT result = decodeControl(m_device, footer, m_memoryLimit - m_metadataCharge, &footerData, pd);
    if (result != READY) return result;
    Cursor fc{footerData};
    qint64 count = 0;
    if (!fc.number(&count) || count < 1 || count > kEntryLimit) return INVALID;
    QList<Descriptor> directories;
    // The footer remains live while referenced descriptors are collected.
    // Charge descriptor/string storage before retaining it in a Qt container.
    bool authenticatedHeader = false;
    for (qint64 i = 0; i < count; ++i) {
        if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
        Descriptor d;
        qint64 relative = 0;
        if (!fc.number(&d.type) || !fc.string(&d.method) || !fc.number(&relative) || !fc.number(&d.unpacked) || !fc.number(&d.packed) || !fc.u32(&d.crc) ||
            relative > footer.offset) return INVALID;
        d.offset = footer.offset - relative;
        if (d.packed > footer.offset - d.offset) return INVALID;
        if (!chargeMetadata(256 + 2 * d.method.size(), footerData.size())) return RESOURCE_LIMIT;
        if (d.type == 1) {
            QByteArray data;
            result = decodeControl(m_device, d, m_memoryLimit - m_metadataCharge - footerData.size(), &data, pd);
            if (result != READY) return result;
            if (d.offset != 0 || data.size() < 8 || data.left(8) != header.left(8) || authenticatedHeader) return INVALID;
            authenticatedHeader = true;
        } else if (d.type == 3) {
            directories.append(d);
        } else if (d.type != 5) {
            return UNSUPPORTED;
        }
    }
    bool locked = false;
    QByteArray recoverySettings;
    qint64 oldCommentSize = 0;
    if (!authenticatedHeader || !fc.flag(&locked) || !fc.number(&oldCommentSize) || oldCommentSize > (fc.bytes.size() - fc.pos) / 4) return INVALID;
    fc.pos += oldCommentSize * 4;
    // Historical footers end after a UTF-32 comment. Later versions append
    // recovery settings and then a length-prefixed UTF-8 comment.
    if (fc.pos < fc.bytes.size()) {
        if (!fc.string(&recoverySettings)) return INVALID;
        if (fc.pos < fc.bytes.size()) {
            qint64 commentSize = 0;
            if (!fc.number(&commentSize) || commentSize != fc.bytes.size() - fc.pos) return INVALID;
            fc.pos += commentSize;
        }
    }
    // Only the parsed descriptors are needed after this point.
    fc.bytes.clear();
    footerData.clear();
    tail.clear();
    header.clear();
    qint64 totalSize = 0;
    const qint64 entryLimit = policy.nMaxEntryCount >= 0 ? qMin(kEntryLimit, policy.nMaxEntryCount) : kEntryLimit;
    for (const Descriptor &directory : directories) {
        if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
        QByteArray data;
        result = decodeControl(m_device, directory, m_memoryLimit - m_metadataCharge, &data, pd);
        if (result != READY) return result;
        Cursor c{data};
        qint64 blockCount = 0, fileCount = 0;
        if (!c.number(&blockCount) || blockCount > kEntryLimit || blockCount > kEntryLimit - m_blocks.size()) return INVALID;
        // Includes temporary block-file counts and both append-side block
        // structures. Names/method strings receive separate charges below.
        if (!chargeMetadata(blockCount * 256, data.size())) return RESOURCE_LIMIT;
        QList<qint64> blockFiles;
        QList<BLOCK> blocks;
        for (qint64 i = 0; i < blockCount; ++i) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            qint64 files = 0;
            if (!c.number(&files) || files > entryLimit - fileCount - m_entries.size()) return RESOURCE_LIMIT;
            fileCount += files;
            blockFiles.append(files);
            blocks.append(BLOCK());
        }
        for (BLOCK &block : blocks) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            if (!c.string(&block.method)) return INVALID;
            if (!chargeMetadata(2 * (block.method.size() + 1), data.size())) return RESOURCE_LIMIT;
            if (!XFreeArcDecoder::supports(block.method)) return UNSUPPORTED;
        }
        for (BLOCK &block : blocks) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            qint64 relative = 0;
            if (!c.number(&relative) || relative > directory.offset) return INVALID;
            block.offset = directory.offset - relative;
        }
        for (BLOCK &block : blocks) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            if (!c.number(&block.packed) || block.packed > directory.offset - block.offset) return INVALID;
            if (block.packed > kSolidLimit) return RESOURCE_LIMIT;
        }
        qint64 directoryCount = 0;
        if (!c.number(&directoryCount) || directoryCount > kEntryLimit) return INVALID;
        if (!chargeMetadata(directoryCount * 128 + fileCount * 256, data.size())) return RESOURCE_LIMIT;
        QList<QString> paths;
        for (qint64 i = 0; i < directoryCount; ++i) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            QByteArray raw;
            QString path;
            if (!c.string(&raw)) return INVALID;
            if (!chargeMetadata(4 * (raw.size() + 1), data.size())) return RESOURCE_LIMIT;
            if (!utf8Name(raw, &path)) return INVALID;
            paths.append(path);
        }
        QList<ENTRY> entries;
        for (qint64 i = 0; i < fileCount; ++i) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            QByteArray raw;
            ENTRY entry;
            if (!c.string(&raw) || raw.isEmpty()) return INVALID;
            if (!chargeMetadata(4 * (raw.size() + 1), data.size())) return RESOURCE_LIMIT;
            if (!utf8Name(raw, &entry.name)) return INVALID;
            entries.append(entry);
        }
        for (ENTRY &entry : entries) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            qint64 index = 0;
            if (!c.number(&index) || (paths.isEmpty() ? index != 0 : index >= paths.size())) return INVALID;
            if (!paths.isEmpty() && !paths.at(index).isEmpty()) {
                // A short directory index may expand to a long path for every
                // file. Bound this amplification before constructing the name.
                if (!chargeMetadata(4 * (paths.at(index).size() + 1), data.size())) return RESOURCE_LIMIT;
                entry.name.prepend(paths.at(index) + QLatin1Char('/'));
            }
        }
        for (ENTRY &entry : entries) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            if (!c.number(&entry.size)) return INVALID;
            if ((policy.nMaxEntryOutputSize >= 0 && entry.size > policy.nMaxEntryOutputSize) || entry.size > (std::numeric_limits<qint64>::max)() - totalSize)
                return RESOURCE_LIMIT;
            totalSize += entry.size;
            if (policy.nMaxTotalOutputSize >= 0 && totalSize > policy.nMaxTotalOutputSize) return RESOURCE_LIMIT;
        }
        for (ENTRY &entry : entries) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            if (!c.u32(&entry.time)) return INVALID;
        }
        for (ENTRY &entry : entries) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            if (!c.flag(&entry.folder) || (entry.folder && entry.size)) return INVALID;
        }
        for (ENTRY &entry : entries) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            if (!c.u32(&entry.crc)) return INVALID;
        }
        if (!c.zeroTail()) return UNSUPPORTED;
        qint32 index = 0;
        for (qint32 b = 0; b < blocks.size(); ++b) {
            if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
            BLOCK &block = blocks[b];
            for (qint64 i = 0; i < blockFiles.at(b); ++i) {
                if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
                ENTRY &entry = entries[index++];
                entry.block = m_blocks.size() + b;
                entry.offset = block.unpacked;
                if (entry.size > kSolidLimit - block.unpacked) return RESOURCE_LIMIT;
                block.unpacked += entry.size;
            }
        }
        m_blocks.append(blocks);
        m_entries.append(entries);
    }
    // All parsed metadata remains live during solid decoding. Refuse a known
    // oversized block now, before the archive advertises a usable member list.
    for (const BLOCK &block : m_blocks) {
        if (!XBinary::isPdStructNotCanceled(pd)) return CANCELED;
        qint64 stageCap = 0;
        if (!decodePlan(block.method, block.packed, block.unpacked, kSolidLimit, m_memoryLimit - m_metadataCharge, &stageCap, pd))
            return XBinary::isPdStructNotCanceled(pd) ? RESOURCE_LIMIT : CANCELED;
    }
    return XBinary::isPdStructNotCanceled(pd) ? READY : CANCELED;
}

bool XFreeArcNative::materialize(const QString &directory, XBinary::PDSTRUCT *pd)
{
    // Decode each solid stream once. Files remain in a private numeric stage
    // until all stream lengths and all member CRCs have been verified.
    qint64 decodedTotal = 0;
    qint32 entryIndex = 0;
    for (qint32 b = 0; b < m_blocks.size(); ++b) {
        if (!m_device || !XBinary::isPdStructNotCanceled(pd)) return false;
        const BLOCK &block = m_blocks.at(b);
        qint64 stageCap = 0;
        if (!decodePlan(block.method, block.packed, block.unpacked, kSolidLimit, m_memoryLimit - m_metadataCharge, &stageCap, pd)) return false;
        QByteArray packed, decoded;
        if (!readAt(m_device, block.offset, block.packed, kSolidLimit, &packed, pd) ||
            !XFreeArcDecoder::decode(packed, block.method, block.unpacked, stageCap, &decoded, pd)) return false;
        decodedTotal += decoded.size();
        if (m_policy.nMaxTotalOutputSize >= 0 && decodedTotal > m_policy.nMaxTotalOutputSize) return false;
        while (entryIndex < m_entries.size() && m_entries.at(entryIndex).block == b) {
            const qint32 i = entryIndex++;
            ENTRY &entry = m_entries[i];
            if (!XBinary::isPdStructNotCanceled(pd) || entry.offset > decoded.size() || entry.size > decoded.size() - entry.offset) return false;
            // A non-owning view keeps a large member from duplicating the
            // decoded solid block while its CRC and staged bytes are written.
            const QByteArray contents = QByteArray::fromRawData(decoded.constData() + entry.offset, (qint32)entry.size);
            quint32 crc = 0;
            if (!checkedCRC32(contents.constData(), contents.size(), &crc, pd) || crc != entry.crc) return false;
            if (entry.folder) continue;
            const QString stagedPath = QDir(directory).filePath(QString::number(i));
            if (!chargeMetadata(128 + 4 * (stagedPath.size() + 1), packed.capacity() + decoded.capacity())) return false;
            entry.stagedPath = stagedPath;
            QFile output(entry.stagedPath);
            if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return false;
            qint64 written = 0;
            while (written < contents.size() && XBinary::isPdStructNotCanceled(pd)) {
                const qint64 part = output.write(contents.constData() + written, qMin<qint64>(65536, contents.size() - written));
                if (part <= 0) return false;
                written += part;
            }
            if (written != contents.size() || !output.flush()) return false;
        }
    }
    return m_device && entryIndex == m_entries.size() && XBinary::isPdStructNotCanceled(pd);
}
