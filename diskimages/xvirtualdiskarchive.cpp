/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#include "xvirtualdiskarchive.h"
#include "xntfsarchive.h"

#include "xzstddecoder.h"
#include "zlib.h"

#include <QMap>
#include <QPointer>
#include <QSet>
#include <QTemporaryFile>
#include <QtEndian>
#include <algorithm>
#include <limits>
#include <memory>
#include <new>

namespace {
// Explicit reader limits, independent of the caller's (possibly stricter)
// output budget. Exceeding one fails; no image is silently truncated.
const quint64 kMaxDisk = Q_UINT64_C(64) * 1024 * 1024 * 1024;
const quint64 kMetadataBudget = 32 * 1024 * 1024;
const int kMaxSegments = 262144;
const quint64 kMaxTableEntries = 1048576;
const quint64 kMaxCluster = 2 * 1024 * 1024;
enum Mode { Zero, Stored, Deflate, Zstandard };

struct Segment {
    quint64 offset;
    quint64 inputSize;
    quint64 outputSize;
    quint64 decodedSize;
    Mode mode;
};
struct Guest;
struct DiskPlan {
    std::shared_ptr<Guest> guest;
    quint64 diskSize = 0;
    quint64 sourceSize = 0;
    QString version;
    QString variant;
    QString error;
    QVector<Segment> segments;
};
bool span(quint64 offset, quint64 length, quint64 limit) { return offset <= limit && length <= limit - offset; }
bool power2(quint64 value) { return value && !(value & (value - 1)); }
quint16 u16(const QByteArray &a, int p, bool be = false) {
    const uchar *v = reinterpret_cast<const uchar *>(a.constData() + p);
    return be ? qFromBigEndian<quint16>(v) : qFromLittleEndian<quint16>(v);
}
quint32 u32(const QByteArray &a, int p, bool be = false) {
    const uchar *v = reinterpret_cast<const uchar *>(a.constData() + p);
    return be ? qFromBigEndian<quint32>(v) : qFromLittleEndian<quint32>(v);
}
quint64 u64(const QByteArray &a, int p, bool be = false) {
    const uchar *v = reinterpret_cast<const uchar *>(a.constData() + p);
    return be ? qFromBigEndian<quint64>(v) : qFromLittleEndian<quint64>(v);
}
bool checksum(const QByteArray &a, int p) {
    quint32 sum = 0;
    for (int i = 0; i < a.size(); ++i) if (i < p || i >= p + 4) sum += quint8(a.at(i));
    return ~sum == u32(a, p, true);
}

// Sorted, nonoverlapping metadata or allocation extents. Length and addition
// have already been range checked when they enter this set.
struct Ranges {
    QMap<quint64, quint64> ranges;
    bool intersects(quint64 off, quint64 len) const {
        if (!len) return false;
        QMap<quint64, quint64>::const_iterator next = ranges.lowerBound(off);
        if (next != ranges.cend() && next.key() < off + len) return true;
        if (next != ranges.cbegin()) { --next; if (next.key() + next.value() > off) return true; }
        return false;
    }
    bool add(quint64 off, quint64 len) {
        if (!len || intersects(off, len)) return false;
        ranges.insert(off, len);
        return true;
    }
};

struct Reader {
    QPointer<XVirtualDiskArchive> owner;
    QPointer<QIODevice> source;
    XBinary::PDSTRUCT *pd;
    quint64 size = 0;
    quint64 budget = kMetadataBudget;
    Reader(XVirtualDiskArchive *pOwner, XBinary::PDSTRUCT *pPd) : owner(pOwner), source(pOwner->getDevice()), pd(pPd) {
        if (!active()) return;
        bool sequential = source->isSequential();
        if (!active() || sequential) return;
        if (source->isOpen() && source->isReadable() && !source->isTextModeEnabled()) {
            qint64 n = source->size();
            if (active() && n >= 0) size = quint64(n);
        }
    }
    bool active() const { return owner && source && owner->getDevice() == source.data() && XBinary::isPdStructNotCanceled(pd); }
    QByteArray read(quint64 off, quint64 len, bool metadata = true) {
        if (!active() || !span(off, len, size) || len > 4 * 1024 * 1024 || (metadata && len > budget)) return QByteArray();
        if (metadata) budget -= len;
        QByteArray a(int(len), 0);
        const qint64 n = XBinary::read_array_process(source.data(), qint64(off), a.data(), qint64(len), pd);
        if (!active() || n != qint64(len)) return QByteArray();
        return a;
    }
};

bool append(DiskPlan &p, Mode mode, quint64 offset, quint64 input, quint64 output, quint64 decoded = 0) {
    if (!output) return true;
    if ((mode == Zero || mode == Stored) && !p.segments.isEmpty()) {
        Segment &last = p.segments.last();
        if (last.mode == mode && (mode == Zero || last.offset + last.inputSize == offset)) {
            if (output > kMaxDisk - last.outputSize) return false;
            last.outputSize += output;
            last.inputSize += input;
            return true;
        }
    }
    if (p.segments.size() >= kMaxSegments) return false;
    p.segments.append({offset, input, output, decoded, mode});
    return true;
}

bool parseVHD(Reader &r, DiskPlan &p) {
    if (r.size < 1024 || r.size % 512) return false;
    QByteArray h = r.read(r.size - 512, 512);
    if (h.size() != 512 || !h.startsWith("conectix") || u32(h, 12, true) != 0x10000 || !checksum(h, 64) ||
        (u32(h, 8, true) & ~quint32(3)) || !(u32(h, 8, true) & 2)) return false;
    p.diskSize = u64(h, 48, true);
    if (!p.diskSize || p.diskSize > kMaxDisk || p.diskSize % 512) return false;
    p.version = QStringLiteral("1.0");
    quint32 type = u32(h, 60, true);
    if (type == 2) {
        if (u64(h, 16, true) != ~quint64(0) || p.diskSize != r.size - 512) return false;
        p.variant = QStringLiteral("fixed; raw disk stream");
        return append(p, Stored, 0, p.diskSize, p.diskSize);
    }
    // Differencing images require a separately supplied and validated parent.
    if (type != 3) return false;
    quint64 headerOffset = u64(h, 16, true);
    if (headerOffset < 512 || headerOffset % 512 || !span(headerOffset, 1024, r.size - 512)) return false;
    QByteArray d = r.read(headerOffset, 1024);
    if (d.size() != 1024 || !d.startsWith("cxsparse") || u64(d, 8, true) != ~quint64(0) ||
        u32(d, 24, true) != 0x10000 || !checksum(d, 36) || d.mid(40, 16) != QByteArray(16, 0)) return false;
    QByteArray front = r.read(0, 512);
    if (front != h) return false;
    quint64 bat = u64(d, 16, true), count = u32(d, 28, true), block = u32(d, 32, true);
    if (!power2(block) || block < 512 || block > 32 * 1024 * 1024 || !count || count > kMaxTableEntries ||
        count < (p.diskSize - 1) / block + 1 || bat % 512) return false;
    quint64 batSize = (count * 4 + 511) & ~quint64(511);
    if (!span(bat, batSize, r.size - 512)) return false;
    Ranges allocated;
    if (!allocated.add(0, 512) || !allocated.add(headerOffset, 1024) || !allocated.add(bat, batSize) || !allocated.add(r.size - 512, 512)) return false;
    QByteArray table = r.read(bat, count * 4);
    if (quint64(table.size()) != count * 4) return false;
    quint64 sectors = block / 512, bitmapSize = ((sectors + 7) / 8 + 511) & ~quint64(511);
    for (quint64 i = 0; i < count && r.active(); ++i) {
        quint32 entry = u32(table, int(i * 4), true);
        quint64 output = i * block < p.diskSize ? qMin(block, p.diskSize - i * block) : 0;
        if (entry == 0xffffffff) { if (!append(p, Zero, 0, 0, output)) return false; continue; }
        quint64 off = quint64(entry) * 512;
        if (!span(off, bitmapSize + block, r.size - 512) || !allocated.add(off, bitmapSize + block)) return false;
        QByteArray bitmap = r.read(off, bitmapSize);
        if (quint64(bitmap.size()) != bitmapSize) return false;
        for (quint64 s = 0; s < output / 512 && r.active(); ++s) {
            bool present = quint8(bitmap.at(int(s / 8))) & (0x80 >> (s % 8));
            if (!append(p, present ? Stored : Zero, present ? off + bitmapSize + s * 512 : 0, present ? 512 : 0, 512)) return false;
        }
    }
    p.variant = QStringLiteral("dynamic; allocation bitmap applied; raw disk stream");
    return r.active();
}

bool parseVDI(Reader &r, DiskPlan &p) {
    QByteArray h = r.read(0, 456);
    if (h.size() != 456 || u32(h, 64) != 0xbeda107f || u32(h, 68) != 0x10001) return false;
    quint64 headerSize = u32(h, 72), map = u32(h, 340), data = u32(h, 344);
    quint32 type = u32(h, 76), block = u32(h, 376), extra = u32(h, 380);
    quint64 count = u32(h, 384), allocated = u32(h, 388);
    p.diskSize = u64(h, 368);
    if (headerSize < 384 || !span(72, headerSize, r.size) || (type != 1 && type != 2) || u32(h, 360) != 512 ||
        !p.diskSize || p.diskSize > kMaxDisk || p.diskSize % 512 || !power2(block) || block < 512 || block > 32 * 1024 * 1024 ||
        !count || count > kMaxTableEntries || count != (p.diskSize - 1) / block + 1 || allocated > count ||
        map < 72 + headerSize || data > r.size || !span(map, count * 4, data) ||
        allocated > (r.size - data) / (quint64(block) + extra) || (type == 2 && count != allocated) ||
        h.mid(424, 32) != QByteArray(32, 0)) return false;
    QByteArray table = r.read(map, count * 4);
    if (quint64(table.size()) != count * 4) return false;
    QSet<quint32> used;
    for (quint64 i = 0; i < count && r.active(); ++i) {
        quint32 entry = u32(table, int(i * 4));
        quint64 output = qMin(quint64(block), p.diskSize - i * block);
        if (entry == 0xffffffff || entry == 0xfffffffe) {
            if (type == 2 || !append(p, Zero, 0, 0, output)) return false;
        } else {
            if (entry >= allocated || used.contains(entry)) return false;
            used.insert(entry);
            quint64 off = data + quint64(entry) * (quint64(block) + extra) + extra;
            if (!span(off, block, r.size) || !append(p, Stored, off, output, output)) return false;
        }
    }
    if (quint64(used.size()) != allocated) return false;
    p.version = QStringLiteral("1.1");
    p.variant = type == 1 ? QStringLiteral("dynamic; raw disk stream") : QStringLiteral("fixed; raw disk stream");
    return r.active();
}

// Every host cluster covered by the extent must be referenced by the refcount
// table. The reader members are held by reference, exactly as the caller's
// locals were captured.
struct HasRef {
    Reader &r;
    const QMap<quint64, QByteArray> &refBlocks;
    quint64 cluster;
    HasRef(Reader &reader, const QMap<quint64, QByteArray> &blocks, quint64 clusterSize) : r(reader), refBlocks(blocks), cluster(clusterSize) {}
    bool operator()(quint64 off, quint64 len) const {
        if (!len || !span(off, len, r.size)) return false;
        quint64 first = off / cluster, last = (off + len - 1) / cluster;
        for (quint64 host = first; host <= last; ++host) {
            QMap<quint64, QByteArray>::const_iterator found = refBlocks.constFind(host / (cluster / 2));
            if (found == refBlocks.cend() || !u16(found.value(), int((host % (cluster / 2)) * 2), true)) return false;
        }
        return true;
    }
};

bool parseQCOW(Reader &r, DiskPlan &p) {
    QByteArray h = r.read(0, 112);
    if (h.size() != 112 || h.left(4) != QByteArray::fromHex("514649fb")) return false;
    quint32 version = u32(h, 4, true), bits = u32(h, 20, true);
    p.diskSize = u64(h, 24, true);
    if ((version != 2 && version != 3) || bits < 9 || bits > 21 || !p.diskSize || p.diskSize > kMaxDisk || p.diskSize % 512 ||
        u64(h, 8, true) || u32(h, 32, true) || u32(h, 60, true) || u64(h, 64, true)) return false;
    quint64 cluster = quint64(1) << bits, features = version == 3 ? u64(h, 72, true) : 0;
    quint32 headerSize = version == 3 ? u32(h, 100, true) : 72;
    // The first implementation accepts standalone, clean active images with
    // standard L2 entries. Parent/external/encrypted/snapshot/log recovery is
    // not guessed; neither are unknown incompatible feature bits ignored.
    if ((features & ~quint64(8)) || (version == 3 && (headerSize < 104 || headerSize % 8 || headerSize > cluster || u32(h, 96, true) != 4))) return false;
    Mode compression = Deflate;
    if (features & 8) {
        if (headerSize < 112 || quint8(h.at(104)) != 1) return false;
        compression = Zstandard;
    } else if (version == 3 && headerSize > 104 && h.at(104)) return false;
    quint64 l1 = u64(h, 40, true), count = u32(h, 36, true), ref = u64(h, 48, true), refClusters = u32(h, 56, true);
    quint64 coverage = cluster * (cluster / 8);
    if (!count || count > kMaxTableEntries || count < (p.diskSize - 1) / coverage + 1 ||
        !refClusters || refClusters > (4 * 1024 * 1024) / cluster || l1 % cluster || ref % cluster ||
        !span(l1, ((count * 8 + cluster - 1) / cluster) * cluster, r.size) || !span(ref, refClusters * cluster, r.size)) return false;
    Ranges metadata;
    if (!metadata.add(0, cluster) || !metadata.add(l1, ((count * 8 + cluster - 1) / cluster) * cluster) || !metadata.add(ref, refClusters * cluster)) return false;
    QByteArray table = r.read(l1, count * 8), refs = r.read(ref, refClusters * cluster);
    if (quint64(table.size()) != count * 8 || quint64(refs.size()) != refClusters * cluster) return false;
    QMap<quint64, QByteArray> refBlocks;
    for (quint64 i = 0; i < quint64(refs.size()) / 8 && r.active(); ++i) {
        quint64 off = u64(refs, int(i * 8), true);
        if (!off) continue;
        if (off % cluster || !span(off, cluster, r.size) || !metadata.add(off, cluster)) return false;
        QByteArray block = r.read(off, cluster);
        if (quint64(block.size()) != cluster) return false;
        refBlocks.insert(i, block);
    }
    const quint64 copied = quint64(1) << 63, compressed = quint64(1) << 62, mask = Q_UINT64_C(0x00fffffffffffe00);
    QMap<quint64, QByteArray> l2Tables;
    // Read metadata first, so payload validation can exclude all metadata,
    // including a table that occurs later in logical disk order.
    for (quint64 i = 0; i < count && r.active(); ++i) {
        quint64 value = u64(table, int(i * 8), true), off = value & mask;
        if (value & ~(mask | copied)) return false;
        if (!off) { if (value) return false; continue; }
        if (off % cluster || !span(off, cluster, r.size) || !metadata.add(off, cluster)) return false;
        QByteArray l2 = r.read(off, cluster);
        if (quint64(l2.size()) != cluster) return false;
        l2Tables.insert(i, l2);
    }
    const HasRef hasRef(r, refBlocks, cluster);
    for (QMap<quint64, quint64>::const_iterator it = metadata.ranges.cbegin(); it != metadata.ranges.cend(); ++it) if (!hasRef(it.key(), it.value())) return false;
    for (quint64 i = 0; i < count && r.active(); ++i) {
        QMap<quint64, QByteArray>::const_iterator found = l2Tables.constFind(i);
        if (found == l2Tables.cend()) {
            if (i * coverage < p.diskSize && !append(p, Zero, 0, 0, qMin(coverage, p.diskSize - i * coverage))) return false;
            continue;
        }
        for (quint64 j = 0; j < cluster / 8 && r.active(); ++j) {
            quint64 value = u64(found.value(), int(j * 8), true), logical = i * coverage + j * cluster;
            quint64 output = logical < p.diskSize ? qMin(cluster, p.diskSize - logical) : 0;
            if (value & compressed) {
                if (value & copied) return false;
                quint32 shift = 62 - (bits - 8);
                quint64 off = value & ((quint64(1) << shift) - 1);
                quint64 input = (((value & ~compressed) >> shift) + 1) * 512 - (off & 511);
                if (!off || off >> 56 || input > cluster + 512 || !span(off, input, r.size) || metadata.intersects(off, input) || !hasRef(off, input)) return false;
                if (!append(p, compression, off, input, output, cluster)) return false;
            } else {
                quint64 off = value & mask;
                if (value & ~(mask | copied | quint64(1)) || (version == 2 && (value & 1)) || (!off && (value & copied))) return false;
                if (off && (off % cluster || !span(off, cluster, r.size) || metadata.intersects(off, cluster) || !hasRef(off, cluster))) return false;
                bool zero = !off || (value & 1);
                if (!append(p, zero ? Zero : Stored, zero ? 0 : off, zero ? 0 : output, output)) return false;
            }
        }
    }
    p.version = QString::number(version);
    p.variant = compression == Deflate ? QStringLiteral("standalone; raw DEFLATE clusters; raw disk stream")
                                      : QStringLiteral("standalone; Zstandard clusters; raw disk stream");
    return r.active();
}

// The reference implementation provenance: catalog 759, VMT -> ->;
// dispatch 004a23a0 -> constructor 0042aee0 -> context 0042ab10.
// The payload mapping below ports 0042ad20/0042adc0. Structural/feature
// validation is strengthened using MS-VHDX sections 2.2, 2.5 and 2.6.
const quint64 kVhdxMiB = 1024 * 1024;

bool vhdxFail(DiskPlan &p, const char *message) {
    p.error = QString::fromLatin1(message);
    return false;
}

bool vhdxChecksum(const QByteArray &bytes, Reader &r) {
    if (bytes.size() < 8) return false;
    quint32 crc = 0xffffffffU;
    for (int i = 0; i < bytes.size(); ++i) {
        if (!(i & 4095) && !r.active()) return false;
        crc ^= (i >= 4 && i < 8) ? 0 : quint8(bytes.at(i));
        for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0x82f63b78U & (0U - (crc & 1U)));
    }
    return r.active() && ~crc == u32(bytes, 4);
}

struct VhdxRegions {
    quint64 bat = 0;
    quint64 batLength = 0;
    quint64 metadata = 0;
    quint64 metadataLength = 0;
    Ranges reserved;
};

bool vhdxRegions(Reader &r, DiskPlan &p, const QByteArray &table, quint64 logOffset,
                 quint64 logLength, VhdxRegions &regions) {
    // GUID pointers and.
    const QByteArray batGuid = QByteArray::fromHex("6677c22d23f600429d64115e9bfd4a08");
    const QByteArray metadataGuid = QByteArray::fromHex("06a27c8b90479a4bb8fe575f050f886e");
    if (!regions.reserved.add(0, kVhdxMiB) || !regions.reserved.add(logOffset, logLength)) return false;
    QSet<QByteArray> ids;
    const quint32 count = u32(table, 8);
    for (quint32 i = 0; i < count && r.active(); ++i) {
        const int at = 16 + int(i) * 32;
        const QByteArray id = table.mid(at, 16);
        const quint64 off = u64(table, at + 16), len = u32(table, at + 24);
        const quint32 required = u32(table, at + 28);
        if (ids.contains(id) || required > 1 || off < kVhdxMiB || off % kVhdxMiB ||
            !len || len % kVhdxMiB || !span(off, len, r.size) || !regions.reserved.add(off, len)) return false;
        ids.insert(id);
        if (id == batGuid) {
            if (!required) return false;
            regions.bat = off; regions.batLength = len;
        } else if (id == metadataGuid) {
            if (!required) return false;
            regions.metadata = off; regions.metadataLength = len;
        } else if (required) {
            return vhdxFail(p, "VHDX required region is not supported");
        }
    }
    return r.active() && regions.bat && regions.metadata;
}

bool parseVHDX(Reader &r, DiskPlan &p) {
    p.error = QStringLiteral("Invalid or incomplete VHDX structure");
    if (r.size < kVhdxMiB || r.size % kVhdxMiB || r.read(0, 8) != QByteArray("vhdxfile", 8)) return false;

    // The reference implementation validate both 4-KiB headers and select the
    // unique newest sequence. CRC32C over a zeroed checksum field is
    // equivalent to the reference implementation's precomputed initial state after 8 bytes.
    const QByteArray first = r.read(0x10000, 4096), second = r.read(0x20000, 4096);
    const bool firstValid = first.size() == 4096 && first.startsWith("head") && vhdxChecksum(first, r);
    const bool secondValid = second.size() == 4096 && second.startsWith("head") && vhdxChecksum(second, r);
    if ((!firstValid && !secondValid) || (firstValid && secondValid && u64(first, 8) == u64(second, 8))) return false;
    const QByteArray &header = !secondValid || (firstValid && u64(first, 8) > u64(second, 8)) ? first : second;
    if (u16(header, 66) != 1) return vhdxFail(p, "VHDX format version is not supported");
    if (header.mid(48, 16) != QByteArray(16, 0)) return vhdxFail(p, "VHDX log replay is not supported");
    // A zero LogGuid means there is no log to replay; MS-VHDX permits an
    // unknown LogVersion in that case. No input is opened for writing.
    const quint64 logOffset = u64(header, 72), logLength = u32(header, 68);
    if (logOffset < kVhdxMiB || logOffset % kVhdxMiB || !logLength || logLength % kVhdxMiB ||
        !span(logOffset, logLength, r.size)) return false;

    const QByteArray region1 = r.read(0x30000, 65536), region2 = r.read(0x40000, 65536);
    const bool region1Valid = region1.size() == 65536 && region1.startsWith("regi") && u32(region1, 8) <= 2047 && vhdxChecksum(region1, r);
    const bool region2Valid = region2.size() == 65536 && region2.startsWith("regi") && u32(region2, 8) <= 2047 && vhdxChecksum(region2, r);
    if (!region1Valid && !region2Valid) return false;
    if (region1Valid && region2Valid && (u32(region1, 8) != u32(region2, 8) ||
        region1.mid(16, int(u32(region1, 8)) * 32) != region2.mid(16, int(u32(region2, 8)) * 32))) {
        return vhdxFail(p, "VHDX region copies disagree; recovery is not supported");
    }
    VhdxRegions regions;
    if (!vhdxRegions(r, p, region1Valid ? region1 : region2, logOffset, logLength, regions)) return false;

    // The reference implementation metadata GUIDs/field widths are recovered from the EXE.
    // Virtual/physical sector and disk-ID requirements are additionally
    // checked instead of accepting the reference implementation's partially populated context.
    const QByteArray fileGuid = QByteArray::fromHex("3767a1ca36fa434db3b633f0aa44e76b");
    const QByteArray sizeGuid = QByteArray::fromHex("2442a52f1bcd7648b2115dbed83bf4b8");
    const QByteArray logicalGuid = QByteArray::fromHex("1dbf41816fa90947ba47f233a8faab5f");
    const QByteArray physicalGuid = QByteArray::fromHex("c748a3cd5d4471449cc9e9885251c556");
    const QByteArray diskGuid = QByteArray::fromHex("ab12cabee6b2234593efc309e000c746");
    const QByteArray parentGuid = QByteArray::fromHex("2d5fd3a80bb34d45abf7d3d84834ab0c");
    const QByteArray metadata = r.read(regions.metadata, 65536);
    if (metadata.size() != 65536 || !metadata.startsWith("metadata") || u16(metadata, 10) > 2047) return false;
    Ranges itemRanges;
    QSet<QByteArray> itemIds;
    quint32 blockSize = 0, flags = 0, logical = 0, physical = 0, userCount = 0;
    bool haveFile = false, haveSize = false, haveDiskId = false;
    for (quint32 i = 0; i < u16(metadata, 10) && r.active(); ++i) {
        const int at = 32 + int(i) * 32;
        const QByteArray id = metadata.mid(at, 16);
        const quint32 off = u32(metadata, at + 16), len = u32(metadata, at + 20), itemFlags = u32(metadata, at + 24);
        if ((itemFlags & ~7U) || u32(metadata, at + 28) || len > kVhdxMiB ||
            (len ? (off < 65536 || !span(off, len, regions.metadataLength) || !itemRanges.add(off, len)) : off != 0)) return false;
        QByteArray key = id; key.append(char(itemFlags & 1));
        if (itemIds.contains(key)) return false;
        itemIds.insert(key);
        if (itemFlags & 1) {
            if (++userCount > 1024) return false;
            if (itemFlags & 4) return vhdxFail(p, "VHDX required user metadata is not supported");
            continue;
        }
        const bool known = id == fileGuid || id == sizeGuid || id == logicalGuid || id == physicalGuid || id == diskGuid;
        if (id == parentGuid) return vhdxFail(p, "VHDX parent/differencing images are not supported");
        if (!known) {
            if (itemFlags & 4) return vhdxFail(p, "VHDX required metadata is not supported");
            continue;
        }
        if (itemFlags != (id == fileGuid ? 4U : 6U)) return false;
        const quint32 expected = id == diskGuid ? 16 : (id == logicalGuid || id == physicalGuid) ? 4 : 8;
        if (len != expected) return false;
        const QByteArray value = r.read(regions.metadata + off, len);
        if (quint32(value.size()) != len) return false;
        if (id == fileGuid) { blockSize = u32(value, 0); flags = u32(value, 4); haveFile = true; }
        else if (id == sizeGuid) { p.diskSize = u64(value, 0); haveSize = true; }
        else if (id == logicalGuid) logical = u32(value, 0);
        else if (id == physicalGuid) physical = u32(value, 0);
        else haveDiskId = true;
    }
    if (!r.active() || !haveFile || !haveSize || !haveDiskId || (logical != 512 && logical != 4096) ||
        (physical != 512 && physical != 4096) || !power2(blockSize) || blockSize < kVhdxMiB || blockSize > 256 * kVhdxMiB ||
        !p.diskSize || p.diskSize % logical) return false;
    if (flags & ~3U) return vhdxFail(p, "VHDX file-parameter flags are not supported");
    if (flags & 2) return vhdxFail(p, "VHDX parent/differencing images are not supported");
    if (p.diskSize > kMaxDisk) return vhdxFail(p, "VHDX virtual disk exceeds the reader's 64-GiB size limit");

    // Direct port of the reference implementation (0x60 chunk ratio) and.
    const quint64 chunkRatio = (quint64(logical) << 23) / blockSize;
    if (!chunkRatio) return false;
    const quint64 blocks = (p.diskSize - 1) / blockSize + 1;
    const quint64 entries = blocks + (blocks - 1) / chunkRatio;
    if (entries > kMaxTableEntries || entries > regions.batLength / 8) return false;
    const QByteArray bat = r.read(regions.bat, entries * 8);
    if (quint64(bat.size()) != entries * 8) return false;
    Ranges allocated = regions.reserved;
    for (quint64 i = 0; i < entries && r.active(); ++i) {
        const quint64 entry = u64(bat, int(i * 8));
        if (entry & Q_UINT64_C(0x00000000000ffff8)) return false;
        if (i % (chunkRatio + 1) == chunkRatio) {
            if (entry != 0) return vhdxFail(p, "VHDX allocated sector bitmap is not supported in standalone images");
        }
    }
    for (quint64 i = 0; i < blocks && r.active(); ++i) {
        const quint64 index = (i / chunkRatio) * (chunkRatio + 1) + i % chunkRatio;
        const quint64 entry = u64(bat, int(index * 8));
        const quint64 off = entry & Q_UINT64_C(0xfffffffffff00000);
        const quint64 output = qMin(quint64(blockSize), p.diskSize - i * blockSize);
        const quint32 state = quint32(entry & 7);
        if (state == 4 || state == 5 || state == 7) return vhdxFail(p, "VHDX reserved or partially-present BAT state is not supported");
        if (off && (off < kVhdxMiB || !span(off, blockSize, r.size) || !allocated.add(off, blockSize))) return false;
        if (state == 6) {
            if (!off || !append(p, Stored, off, output, output)) return false;
        } else {
            // The reference implementation zero-filled every unsuccessful BAT lookup. Here
            // only defined standalone states 0/1/2/3 use that behavior;
            // failed reads, partial blocks and unknown states fail above.
            // MS-VHDX 2.5.1.1 explicitly permits zero reads for these states.
            if (!append(p, Zero, 0, 0, output)) return false;
        }
    }
    p.version = QStringLiteral("1");
    p.variant = QStringLiteral("%1; logical sector %2; physical sector %3; block %4; raw disk stream")
                    .arg(flags & 1 ? QStringLiteral("fixed") : QStringLiteral("dynamic"))
                    .arg(logical).arg(physical).arg(blockSize);
    p.error.clear();
    return r.active();
}

bool parse(Reader &r, XVirtualDiskArchive::KIND kind, DiskPlan &p) {
    p.sourceSize = r.size;
    bool ok = kind == XVirtualDiskArchive::KIND_VHD ? parseVHD(r, p) : kind == XVirtualDiskArchive::KIND_VDI ? parseVDI(r, p) :
              kind == XVirtualDiskArchive::KIND_VHDX ? parseVHDX(r, p) : parseQCOW(r, p);
    quint64 total = 0;
    for (const Segment &s : p.segments) { if (s.outputSize > p.diskSize - total) return false; total += s.outputSize; }
    return ok && r.active() && total == p.diskSize && !p.segments.isEmpty();
}

bool writeAll(QIODevice *out, const char *data, qint64 size, Reader &r) {
    QPointer<QIODevice> guard(out);
    while (size > 0 && r.active() && guard) {
        qint64 n = guard->write(data, size);
        if (!guard || !r.active() || n <= 0 || n > size) return false;
        data += n; size -= n;
    }
    return !size && guard && r.active();
}

bool decodeCluster(const Segment &s, const QByteArray &input, QByteArray &output, Reader &r) {
    if (!s.decodedSize || s.decodedSize > kMaxCluster || quint64(input.size()) != s.inputSize) return false;
    output.resize(int(s.decodedSize));
    quint64 consumed = 0, produced = 0;
    bool ok = false;
    if (s.mode == Deflate) {
        z_stream stream = {};
        if (inflateInit2(&stream, -15) != Z_OK) return false;
        stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(input.constData()));
        stream.avail_in = uInt(input.size());
        // One extra byte detects a stream exceeding the declared cluster.
        QByteArray buffer(65536, 0);
        int status = Z_OK;
        while (r.active() && status == Z_OK) {
            stream.next_out = reinterpret_cast<Bytef *>(buffer.data()); stream.avail_out = uInt(buffer.size());
            uInt before = stream.avail_in;
            status = inflate(&stream, Z_NO_FLUSH);
            quint64 n = quint64(buffer.size() - stream.avail_out);
            if (n > s.decodedSize - produced) break;
            if (n) memcpy(output.data() + produced, buffer.constData(), size_t(n));
            produced += n;
            if (status == Z_STREAM_END) { ok = true; break; }
            if (!n && before == stream.avail_in) break;
        }
        consumed = stream.total_in;
        inflateEnd(&stream);
    } else if (s.mode == Zstandard) {
        ZSTD_DStream *stream = ZSTD_createDStream();
        if (!stream) return false;
        bool initialized = !ZSTD_isError(ZSTD_initDStream(stream)) && !ZSTD_isError(ZSTD_DCtx_setMaxWindowSize(stream, size_t(qMax(s.decodedSize, quint64(1024)) )));
        ZSTD_inBuffer in = {input.constData(), size_t(input.size()), 0};
        QByteArray buffer(65536, 0);
        while (initialized && r.active()) {
            ZSTD_outBuffer out = {buffer.data(), size_t(buffer.size()), 0};
            size_t before = in.pos;
            size_t status = ZSTD_decompressStream(stream, &out, &in);
            if (ZSTD_isError(status) || out.pos > s.decodedSize - produced) break;
            if (out.pos) memcpy(output.data() + produced, buffer.constData(), out.pos);
            produced += out.pos;
            if (!status) { ok = true; break; }
            if (!out.pos && before == in.pos) break;
        }
        consumed = in.pos;
        ZSTD_freeDStream(stream);
    }
    // QCOW stores a sector-rounded compressed extent, and may put another
    // compressed cluster in its final sector. Its tail is not stream input.
    return r.active() && ok && produced == s.decodedSize && consumed > 0 && consumed <= s.inputSize && s.inputSize - consumed < 512;
}

bool stageDisk(Reader &r, const DiskPlan &p, QIODevice *out) {
    QByteArray zeros(65536, 0);
    quint64 total = 0;
    for (const Segment &s : p.segments) {
        if (!r.active()) return false;
        if (s.mode == Deflate || s.mode == Zstandard) {
            QByteArray input = r.read(s.offset, s.inputSize, false), decoded;
            if (!decodeCluster(s, input, decoded, r) || !writeAll(out, decoded.constData(), qint64(s.outputSize), r)) return false;
        } else {
            quint64 done = 0;
            while (done < s.outputSize && r.active()) {
                quint64 n = qMin(quint64(zeros.size()), s.outputSize - done);
                QByteArray bytes;
                const char *data = zeros.constData();
                if (s.mode == Stored) {
                    bytes = r.read(s.offset + done, n, false);
                    if (quint64(bytes.size()) != n) return false;
                    data = bytes.constData();
                }
                if (!writeAll(out, data, qint64(n), r)) return false;
                done += n;
            }
            if (done != s.outputSize) return false;
        }
        total += s.outputSize;
    }
    return r.active() && total == p.diskSize;
}

// Seekable view over a validated allocation plan. Listing guest files reads
// bounded metadata only; it does not stage or allocate a full raw disk.
class DiskDevice : public QIODevice {
public:
    DiskDevice(XVirtualDiskArchive *owner, const DiskPlan &plan, XBinary::PDSTRUCT *pd)
        : m_owner(owner), m_plan(plan), m_pd(pd) {
        quint64 at = 0;
        for (const Segment &segment : m_plan.segments) { m_starts.append(at);at += segment.outputSize; }
        if (at == m_plan.diskSize) open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }
    qint64 size() const override { return qint64(m_plan.diskSize); }
    bool isSequential() const override { return false; }
    bool seek(qint64 pos) override {
        if (pos < 0 || quint64(pos) > m_plan.diskSize || !QIODevice::seek(pos)) return false;
        m_position = quint64(pos);return true;
    }
    void progress(XBinary::PDSTRUCT *pd) { m_pd = pd; }
protected:
    qint64 readData(char *data, qint64 max) override {
        QPointer<DiskDevice> self(this);
        if (!m_owner || max < 0 || !XBinary::isPdStructNotCanceled(m_pd)) return -1;
        Reader reader(m_owner.data(), m_pd);
        quint64 wanted = qMin(quint64(max), m_plan.diskSize - m_position), done = 0;
        while (done < wanted && reader.active()) {
            QVector<quint64>::const_iterator next = std::upper_bound(m_starts.cbegin(), m_starts.cend(), m_position);
            if (next == m_starts.cbegin()) return -1;
            int index = int(next - m_starts.cbegin() - 1);
            const Segment segment = m_plan.segments.at(index);
            quint64 relative = m_position - m_starts.at(index);
            quint64 count = qMin(quint64(65536), qMin(wanted - done, segment.outputSize - relative));
            if (segment.mode == Zero) memset(data + done, 0, size_t(count));
            else if (segment.mode == Stored) {
                QByteArray bytes = reader.read(segment.offset + relative, count, false);
                if (!self || quint64(bytes.size()) != count) return -1;
                memcpy(data + done, bytes.constData(), size_t(count));
            } else {
                if (m_cachedIndex != index) {
                    QByteArray input = reader.read(segment.offset, segment.inputSize, false), decoded;
                    if (!self || !decodeCluster(segment, input, decoded, reader) || !self) return -1;
                    m_cache = decoded;m_cachedIndex = index;
                }
                if (!span(relative, count, quint64(m_cache.size()))) return -1;
                memcpy(data + done, m_cache.constData() + relative, size_t(count));
            }
            if (!self || !reader.active()) return -1;
            m_position += count;done += count;
        }
        return self && reader.active() && done == wanted ? qint64(done) : -1;
    }
    qint64 writeData(const char *, qint64) override { return -1; }
private:
    QPointer<XVirtualDiskArchive> m_owner;
    DiskPlan m_plan;
    XBinary::PDSTRUCT *m_pd;
    QVector<quint64> m_starts;
    quint64 m_position = 0;
    int m_cachedIndex = -1;
    QByteArray m_cache;
};
struct Guest {
    std::unique_ptr<DiskDevice> raw;
    std::unique_ptr<XNTFSArchive> archive;
    XBinary::UNPACK_STATE state = {};
    ~Guest() { if (archive) archive->finishUnpack(&state, nullptr); }
};

} // namespace

XVirtualDiskArchive::XVirtualDiskArchive(QIODevice *device, KIND kind) : XArchive(device), m_kind(kind) {}
XBinary *XVirtualDiskArchive::createInstance(QIODevice *device, bool image, XADDR address) {
    Q_UNUSED(image)
    Q_UNUSED(address)
    return new XVirtualDiskArchive(device, m_kind);
}
bool XVirtualDiskArchive::isValid(PDSTRUCT *pd) {
    Reader reader(this, pd);
    DiskPlan plan;
    return parse(reader, m_kind, plan);
}
XBinary::FT XVirtualDiskArchive::getFileType() {
    return m_kind == KIND_VHD ? FT_VHD : m_kind == KIND_VDI ? FT_VDI : m_kind == KIND_VHDX ? FT_VHDX : FT_QCOW2;
}
QString XVirtualDiskArchive::getFileFormatExt() {
    return m_kind == KIND_VHD ? QStringLiteral("vhd") : m_kind == KIND_VDI ? QStringLiteral("vdi") : m_kind == KIND_VHDX ? QStringLiteral("vhdx") : QStringLiteral("qcow2");
}
QString XVirtualDiskArchive::getMIMEString() { return QStringLiteral("application/octet-stream"); }
QString XVirtualDiskArchive::getVersion() {
    Reader reader(this, nullptr);
    DiskPlan plan;
    return parse(reader, m_kind, plan) ? plan.version : QString();
}
QList<XBinary::PM_INFO> XVirtualDiskArchive::unpackImplemented() {
    PM_INFO info = {};
    info.hm[0] = HANDLE_METHOD_ARCHIVE_STREAM;
    return {info};
}
bool XVirtualDiskArchive::initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pd) {
    QPointer<XVirtualDiskArchive> self(this);
    if (!state || m_bUnpackOperationInProgress) return false;
    if (!finishUnpack(state, nullptr) || !self) return false;
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !isPdStructNotCanceled(pd)) return false;
    bool bound = bindUnpackSource(state, pd);
    if (!self || !bound) return false;
    std::unique_ptr<DiskPlan> plan(new (std::nothrow) DiskPlan);
    Reader reader(this, pd);
    bool parsed = plan && parse(reader, m_kind, *plan);
    if (!self) return false;
    if (parsed && properties.value(UNPACK_PROP_DISK_FILESYSTEM).toBool()) {
        std::shared_ptr<Guest> guest = std::make_shared<Guest>();
        guest->raw.reset(new DiskDevice(this, *plan, pd));
        guest->archive.reset(new XNTFSArchive(getDevice(), guest->raw.get()));
        parsed = guest->raw->isOpen() && guest->archive->initUnpack(&guest->state, properties, pd);
        if (!self) return false;
        if (parsed) plan->guest = guest;
    }
    if (!parsed) {
        const QString error = plan ? plan->error : QString();
        releaseUnpackSource(state); *state = UNPACK_STATE();
        if (!error.isEmpty()) setPdStructErrorString(pd, error);
        return false;
    }
    state->pContext = plan.get();
    state->mapUnpackProperties = properties;
    state->nTotalSize = qint64(plan->sourceSize);
    state->nNumberOfRecords = plan->guest ? plan->guest->state.nNumberOfRecords : 1;
    state->nCurrentIndex = 0;
    state->nCurrentOffset = 0;
    if (!registerUnpackContextCleanup(state, plan.get(), &deleteUnpackContext<DiskPlan>)) {
        state->pContext = nullptr;releaseUnpackSource(state);*state = UNPACK_STATE();return false;
    }
    DiskPlan *context = plan.release();
    if (!validateAndFinalizeUnpackSource(state, context, pd)) {
        if (!self) return false;
        state->pContext = nullptr; releaseUnpackSource(state); delete context; *state = UNPACK_STATE(); return false;
    }
    return true;
}
XBinary::ARCHIVERECORD XVirtualDiskArchive::infoCurrent(UNPACK_STATE *state, PDSTRUCT *pd) {
    QPointer<XVirtualDiskArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !state || !state->pContext || state->nCurrentIndex < 0 || state->nCurrentIndex >= state->nNumberOfRecords) return record;
    bool current = isUnpackSourceCurrent(state, pd);
    if (!self || !current) return record;
    const DiskPlan &plan = *static_cast<DiskPlan *>(state->pContext);
    if (plan.guest) {
        plan.guest->raw->progress(pd);
        record = plan.guest->archive->infoCurrent(&plan.guest->state, pd);
        if (!self || record.mapProperties.isEmpty() || !markArchiveStreamRecord(&record, state->nCurrentIndex)) return ARCHIVERECORD();
        return record;
    }
    if (state->nCurrentIndex != 0 || state->nNumberOfRecords != 1) return record;
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, QStringLiteral("disk.raw"));
    record.mapProperties.insert(FPART_PROP_EXT, QStringLiteral("raw"));
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, qint64(plan.diskSize));
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, qint64(plan.sourceSize));
    record.mapProperties.insert(FPART_PROP_VERSION, plan.version);
    record.mapProperties.insert(FPART_PROP_INFO, plan.variant);
    if (!markArchiveStreamRecord(&record, 0)) return ARCHIVERECORD();
    return record;
}
bool XVirtualDiskArchive::unpackCurrent(UNPACK_STATE *state, QIODevice *output, PDSTRUCT *pd) {
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XVirtualDiskArchive> self(this);
    QPointer<QIODevice> destination(output);
    if (!guard.isAcquired() || !state || !state->pContext || state->nCurrentIndex < 0 || state->nCurrentIndex >= state->nNumberOfRecords) return false;
    bool current = isUnpackSourceCurrent(state, pd);
    if (!self || !current || !destination || !isUnpackOutputSupported(destination.data())) return false;
    if (!self || !destination) return false;
    bool aliased = devicesAlias(getDevice(), destination.data());
    if (!self || !destination || aliased || !isPdStructNotCanceled(pd)) return false;
    const DiskPlan &plan = *static_cast<DiskPlan *>(state->pContext);
    if (plan.guest) {
        std::shared_ptr<Guest> guest = plan.guest;
        if (guest->state.nCurrentIndex != state->nCurrentIndex || guest->state.nNumberOfRecords != state->nNumberOfRecords) return false;
        guest->raw->progress(pd);guest->state.spOutputBudget = state->spOutputBudget;
        QTemporaryFile stage;
        if (!stage.open() || !guest->archive->unpackCurrent(&guest->state, &stage, pd) || !self || !destination || !stage.seek(0)) return false;
        current = isUnpackSourceCurrent(state, pd);
        if (!self || !current || !destination) return false;
        bool published = publishUnpackOutput(&stage, destination.data(), state, pd);
        if (!self || !published) return false;
        state->nCurrentOffset = stage.size();return true;
    }
    if (state->nCurrentIndex != 0 || state->nNumberOfRecords != 1) return false;
    if (!isUnpackOutputSizeAllowed(state->mapUnpackProperties, qint64(plan.diskSize))) return false;
    if (state->spOutputBudget) {
        bool accepted = state->spOutputBudget->beginEntry(0, QStringLiteral("disk.raw")) && state->spOutputBudget->debit(qint64(plan.diskSize));
        if (!accepted) {
            if (state->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(pd, tr("Unpacked output exceeds the configured limit")); return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(state->spOutputBudget.data());
        }
    }
    Reader reader(this, pd);
    QTemporaryFile stage;
    if (!stage.open() || !stageDisk(reader, plan, &stage) || !self || !destination || stage.size() != qint64(plan.diskSize) || !stage.seek(0)) return false;
    current = isUnpackSourceCurrent(state, pd);
    if (!self || !current || !destination) return false;
    bool published = publishUnpackOutput(&stage, destination.data(), state, pd);
    if (!self || !published) return false;
    state->nCurrentOffset = qint64(plan.diskSize);
    return true;
}
bool XVirtualDiskArchive::moveToNext(UNPACK_STATE *state, PDSTRUCT *pd) {
    QPointer<XVirtualDiskArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || !state->pContext || state->nCurrentIndex < 0 || state->nCurrentIndex >= state->nNumberOfRecords) return false;
    bool current = isUnpackSourceCurrent(state, pd);
    if (!self || !current) return false;
    const DiskPlan &plan = *static_cast<DiskPlan *>(state->pContext);
    if (plan.guest) {
        plan.guest->raw->progress(pd);
        bool moved = plan.guest->archive->moveToNext(&plan.guest->state, pd);
        if (!self) return false;
        state->nCurrentIndex = plan.guest->state.nCurrentIndex;return moved;
    }
    state->nCurrentIndex = 1;
    return false;
}
bool XVirtualDiskArchive::finishUnpack(UNPACK_STATE *state, PDSTRUCT *pd) {
    Q_UNUSED(pd)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state) return false;
    if ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state)) return false;
    DiskPlan *context = static_cast<DiskPlan *>(state->pContext);
    releaseUnpackSource(state);
    *state = UNPACK_STATE();
    delete context;
    return true;
}
