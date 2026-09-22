/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * SPDX-License-Identifier: MIT
 */
#include "xntfsarchive.h"
#include <QMap>
#include <QSet>
#include <QTemporaryFile>
#include <QtEndian>
#include <algorithm>
#include <limits>
#include <memory>
#include <new>

namespace {
const quint64 kMaxVolume = Q_UINT64_C(64) * 1024 * 1024 * 1024;
const quint64 kMaxMft = 64 * 1024 * 1024;
const quint64 kReadBudget = 128 * 1024 * 1024;
const quint64 kKeepBudget = 32 * 1024 * 1024;
const quint64 kMaxRecords = 65536;
const int kMaxRuns = 262144;
bool span(quint64 off, quint64 n, quint64 size) { return off <= size && n <= size - off; }
bool power2(quint64 n) { return n && !(n & (n - 1)); }
quint16 u16(const QByteArray &a, int p) { return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(a.constData() + p)); }
quint32 u32(const QByteArray &a, int p) { return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(a.constData() + p)); }
quint64 u64(const QByteArray &a, int p) { return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(a.constData() + p)); }

struct Run { quint64 logical; quint64 physical; quint64 size; bool sparse; };
struct Stream {
    quint64 size = 0;
    quint64 initialized = 0;
    QByteArray resident;
    QVector<Run> runs;
    bool nonresident = false;
};
struct Name { QString text; quint64 parent = 0; quint16 sequence = 0; bool present = false; };
struct Record {
    quint64 index = 0;
    quint16 sequence = 0;
    bool folder = false;
    bool extension = false;
    bool hasData = false;
    int namedStreams = 0;
    QString unsupported;
    Name names[4];
    Name selected;
    Stream data;
};
struct Entry { QString name; QString info; QString unsupported; bool folder; Stream data; };
struct Plan { quint64 sourceSize = 0; QVector<Entry> entries; QString error; };
struct Volume { quint64 offset; quint64 length; quint64 cluster; quint64 sector; quint64 recordSize; QString prefix; };
struct Reader {
    XNTFSArchive *owner;
    QIODevice *source;
    QIODevice *identity;
    XBinary::PDSTRUCT *pd;
    quint64 size = 0;
    quint64 readBudget = kReadBudget;
    quint64 keepBudget = kKeepBudget;
    int totalRuns = 0;
    Reader(XNTFSArchive *o, XBinary::PDSTRUCT *p) : owner(o), source(o->ntfsDataDevice()), identity(o->getDevice()), pd(p) {
        if (!active() || !source->isOpen() || !source->isReadable()) return;
        bool sequential = source->isSequential();
        if (!active() || sequential || source->isTextModeEnabled()) return;
        qint64 n = source->size();
        if (active() && n >= 0) size = quint64(n);
    }
    bool active() const { return owner && source && identity && owner->ntfsDataDevice() == source && owner->getDevice() == identity && XBinary::isPdStructNotCanceled(pd); }
    bool retain(quint64 n) { if (n > keepBudget) return false; keepBudget -= n; return true; }
    QByteArray read(quint64 off, quint64 n, bool metadata = true) {
        if (!active() || !span(off, n, size) || n > 65536 || (metadata && n > readBudget)) return QByteArray();
        if (metadata) readBudget -= n;
        QByteArray out(int(n), 0);
        qint64 count = XBinary::read_array_process(source, qint64(off), out.data(), qint64(n), pd);
        return active() && count == qint64(n) ? out : QByteArray();
    }
};
bool fail(Plan &p, const char *text) { p.error = QString::fromLatin1(text); return false; }
bool runBefore(quint64 value, const Run &run) { return value < run.logical; }

bool readStream(Reader &r, const Stream &s, quint64 off, quint64 n, QByteArray &out, bool metadata) {
    if (!span(off, n, s.size) || n > 65536 || !r.active()) return false;
    out = QByteArray(int(n), 0);
    if (!s.nonresident) {
        if (quint64(s.resident.size()) != s.size) return false;
        if (n) memcpy(out.data(), s.resident.constData() + off, size_t(n));
        return true;
    }
    quint64 done = 0;
    while (done < n && r.active()) {
        quint64 pos = off + done;
        if (pos >= s.initialized) break; // Valid-data-length tail reads as zero.
        QVector<Run>::const_iterator next = std::upper_bound(s.runs.cbegin(), s.runs.cend(), pos, runBefore);
        if (next == s.runs.cbegin()) return false;
        --next;
        const Run &run = *next;
        if (!span(pos - run.logical, 1, run.size)) return false;
        quint64 count = qMin(n - done, qMin(run.size - (pos - run.logical), s.initialized - pos));
        if (!run.sparse) {
            QByteArray bytes = r.read(run.physical + pos - run.logical, count, metadata);
            if (quint64(bytes.size()) != count) return false;
            memcpy(out.data() + done, bytes.constData(), size_t(count));
        }
        done += count;
    }
    return r.active();
}

// The reference implementation and its record reader apply update-sequence fixups before
// interpreting attributes. Require all sector trailers to match the sequence.
bool fixup(QByteArray &a, const Volume &v) {
    if (quint64(a.size()) != v.recordSize || !a.startsWith("FILE") || a.size() < 48) return false;
    quint64 off = u16(a, 4), count = u16(a, 6);
    if (off < 42 || off % 2 || count != v.recordSize / v.sector + 1 || !span(off, count * 2, v.sector - 2)) return false;
    quint16 sequence = u16(a, int(off));
    for (quint64 i = 1; i < count; ++i) {
        quint64 trailer = i * v.sector - 2;
        if (u16(a, int(trailer)) != sequence) return false;
        a[int(trailer)] = a[int(off + i * 2)];
        a[int(trailer + 1)] = a[int(off + i * 2 + 1)];
    }
    quint64 used = u32(a, 24), allocated = u32(a, 28), first = u16(a, 20);
    return allocated == v.recordSize && used >= 48 && used <= allocated && first >= 48 && first >= off + count * 2 && first % 8 == 0 && first <= used - 4;
}

bool textName(const QByteArray &a, int off, int count, QString &name) {
    if (count < 1 || count > 255 || off < 0 || off > a.size() || count * 2 > a.size() - off) return false;
    name.clear();name.reserve(count);
    for (int i = 0; i < count; ++i) {
        ushort c = u16(a, off + i * 2);
        if (!c || c == '/' || c == '\\') return false;
        if (QChar::isHighSurrogate(c)) {
            if (++i >= count) return false;
            ushort low = u16(a, off + i * 2);
            if (!QChar::isLowSurrogate(low)) return false;
            name.append(QChar(c));name.append(QChar(low));
        } else {
            if (QChar::isLowSurrogate(c)) return false;
            name.append(QChar(c));
        }
    }
    return name != QStringLiteral("..");
}

// Read signed relative-LCN deltas without signed shifts or overflowing a
// negation. A sparse run has no delta and does not alter the preceding LCN.
bool runs(Reader &r, const QByteArray &a, const Volume &v, Stream &s) {
    if (a.size() < 64 || u64(a, 16) != 0) return false;
    quint64 high = u64(a, 24), off = u16(a, 32);
    if (off < 64 || off >= quint64(a.size())) return false;
    s.size = u64(a, 48);s.initialized = u64(a, 56);s.nonresident = true;
    if (s.size > kMaxVolume || s.initialized > s.size) return false;
    quint64 logical = 0, lcn = 0, clusters = v.length / v.cluster;
    bool ended = false;
    while (off < quint64(a.size()) && r.active()) {
        quint8 head = quint8(a.at(int(off++)));
        if (!head) { ended = true;break; }
        quint32 lengths = head & 15, offsets = head >> 4;
        if (!lengths || lengths > 8 || offsets > 8 || !span(off, lengths + offsets, quint64(a.size()))) return false;
        quint64 length = 0, delta = 0;
        for (quint32 i = 0; i < lengths; ++i) length |= quint64(quint8(a.at(int(off++)))) << (8 * i);
        for (quint32 i = 0; i < offsets; ++i) delta |= quint64(quint8(a.at(int(off++)))) << (8 * i);
        // NTFS3 run_unpack zero-extends the length; only the LCN delta is
        // signed. Accept that encoding too, including a one-byte length 128.
        if (!length || length > clusters || length > (kMaxVolume - logical) / v.cluster) return false;
        if (offsets) {
            bool negative = delta & (Q_UINT64_C(1) << (offsets * 8 - 1));
            if (negative) {
                quint64 mask = offsets == 8 ? ~quint64(0) : (Q_UINT64_C(1) << (offsets * 8)) - 1;
                quint64 magnitude = ((~delta) & mask) + 1;
                if (magnitude > lcn) return false;
                lcn -= magnitude;
            } else {
                if (delta > clusters || lcn > clusters - delta) return false;
                lcn += delta;
            }
            if (!span(lcn, length, clusters)) return false;
        }
        if (++r.totalRuns > kMaxRuns || !r.retain(sizeof(Run))) return false;
        quint64 bytes = length * v.cluster;
        s.runs.append({logical, offsets ? v.offset + lcn * v.cluster : 0, bytes, offsets == 0});
        logical += bytes;
    }
    if (!ended || !r.active() || s.size > logical || u64(a, 40) > logical) return false;
    if (logical == 0) return s.size == 0 && (high == 0 || high == ~quint64(0));
    return high == logical / v.cluster - 1;
}

bool parseRecord(Reader &r, const Volume &v, QByteArray a, quint64 index, Record &record) {
    if (!fixup(a, v)) return false;
    record.index = index;record.sequence = u16(a, 16);record.folder = (u16(a, 22) & 2) != 0;
    record.extension = u64(a, 32) != 0;
    quint64 used = u32(a, 24), off = u16(a, 20);
    int count = 0;bool ended = false;
    while (span(off, 4, used) && r.active()) {
        quint32 type = u32(a, int(off));
        if (type == 0xffffffff) { ended = true;break; }
        if (++count > 1024 || !span(off, 16, used)) return false;
        quint64 length = u32(a, int(off + 4));
        if (length < 24 || length % 8 || !span(off, length, used)) return false;
        QByteArray attr = a.mid(int(off), int(length));
        quint8 form = quint8(attr.at(8)), nameLength = quint8(attr.at(9));
        quint64 nameOffset = u16(attr, 10);quint16 flags = u16(attr, 12);
        if (form > 1 || (form && length < 64) || (nameLength && (nameOffset < (form ? 64U : 24U) || !span(nameOffset, quint64(nameLength) * 2, length)))) return false;
        QByteArray value;
        if (!form) {
            quint64 n = u32(attr, 16), p = u16(attr, 20);
            if (p < 24 || !span(p, n, length)) return false;
            value = attr.mid(int(p), int(n));
        }
        if (type == 0x20) record.unsupported = QStringLiteral("NTFS attribute-list extensions are not supported");
        if (type == 0xc0) record.unsupported = QStringLiteral("NTFS reparse/compressed-provider files are not supported");
        if (type == 0x30 && !nameLength) {
            if (form || value.size() < 66) return false;
            quint8 ns = quint8(value.at(65));int n = quint8(value.at(64));Name name;
            if (ns > 3 || !textName(value, 66, n, name.text) || !r.retain(quint64(n) * 2 + sizeof(Name))) return false;
            name.parent = u64(value, 0) & Q_UINT64_C(0xffffffffffff);name.sequence = u16(value, 6);name.present = true;
            record.names[ns] = name;
        }
        if (type == 0x80) {
            if (nameLength) { ++record.namedStreams; }
            else {
                if (record.hasData) record.unsupported = QStringLiteral("NTFS multiple unnamed data attributes are not supported");
                record.hasData = true;
                if ((flags & ~quint16(0x8000)) || (form && attr.at(34) != 0)) record.unsupported = QStringLiteral("NTFS compressed/encrypted data is not supported");
                if (form) {
                    if (u64(attr, 16) != 0) record.unsupported = QStringLiteral("NTFS external data-attribute continuation is not supported");
                    else if (record.unsupported.isEmpty() && !runs(r, attr, v, record.data)) return false;
                    // The header still supplies truthful size for unsupported entries.
                    if (!record.unsupported.isEmpty()) { record.data.size = u64(attr, 48);record.data.initialized = u64(attr, 56); }
                } else {
                    if (!r.retain(quint64(value.size()))) return false;
                    record.data.size = quint64(value.size());record.data.initialized = record.data.size;record.data.resident = value;
                }
            }
        }
        off += length;
    }
    // The reference implementation chooses Win32, POSIX, Win32+DOS, then DOS names.
    for (int ns : {1, 0, 3, 2}) if (record.names[ns].present) { record.selected = record.names[ns];break; }
    if (record.data.size > kMaxVolume) return false;
    return ended && r.active();
}

bool volume(Reader &r, Plan &p, quint64 offset, quint64 length, const QString &prefix) {
    QByteArray boot = r.read(offset, 512);
    if (boot.size() != 512 || boot.mid(3, 8) != QByteArray("NTFS    ", 8) || quint8(boot.at(510)) != 0x55 || quint8(boot.at(511)) != 0xaa) return fail(p, "Invalid NTFS boot sector");
    quint64 sector = u16(boot, 11), spc = quint8(boot.at(13)), sectors = u64(boot, 40);
    if (!power2(sector) || sector < 512 || sector > 4096 || !power2(spc) || spc > 128 || !sectors || sectors > length / sector) return fail(p, "Unsupported NTFS volume geometry");
    quint64 cluster = sector * spc, bytes = sectors * sector;
    qint32 encoded = qint8(boot.at(64));quint64 recordSize = 0;
    if (encoded < 0 && encoded >= -16) recordSize = Q_UINT64_C(1) << (-encoded);
    else if (encoded > 0 && quint64(encoded) <= 65536 / cluster) recordSize = quint64(encoded) * cluster;
    if (!power2(recordSize) || recordSize < sector || recordSize > 65536 || recordSize % sector || bytes > kMaxVolume) return fail(p, "Unsupported NTFS file-record size");
    quint64 mft = u64(boot, 48);
    if (mft > bytes / cluster || !span(mft * cluster, recordSize, bytes)) return fail(p, "Invalid NTFS MFT location");
    Volume v = {offset, bytes, cluster, sector, recordSize, prefix};
    Record first;
    if (!parseRecord(r, v, r.read(offset + mft * cluster, recordSize), 0, first) || !first.hasData || !first.unsupported.isEmpty() ||
        !first.data.nonresident || !first.data.size || first.data.size > kMaxMft || first.data.size % recordSize || first.data.size / recordSize > kMaxRecords) return fail(p, "Unsupported or invalid NTFS MFT data mapping");
    for (const Run &run : first.data.runs) if (run.sparse) return fail(p, "Sparse NTFS MFT is not supported");
    if (first.data.initialized != first.data.size) return fail(p, "Uninitialized NTFS MFT tail is not supported");
    QMap<quint64, Record> records;
    quint64 number = first.data.size / recordSize;
    for (quint64 i = 0; i < number && r.active(); ++i) {
        QByteArray a;
        if (!readStream(r, first.data, i * recordSize, recordSize, a, true)) return fail(p, "Cannot read complete NTFS MFT");
        if (!a.startsWith("FILE")) {
            if (a == QByteArray(int(recordSize), 0)) continue;
            return fail(p, "Invalid NTFS MFT record signature");
        }
        if (!(u16(a, 22) & 1)) continue; // Unallocated entries are not exported.
        Record record;
        if (!r.retain(sizeof(Record)) || !parseRecord(r, v, a, i, record)) return fail(p, "Invalid or over-limit NTFS MFT record");
        if (!record.extension) records.insert(i, record);
    }
    if (!r.active() || !records.contains(5) || !records.value(5).folder) return fail(p, "NTFS root directory is unavailable");
    QSet<QString> paths;
    for (QMap<quint64, Record>::const_iterator it = records.cbegin(); it != records.cend(); ++it) {
        if (!r.active()) return false;
        const Record &record = it.value();
        if (it.key() < 16 || !record.selected.present) continue;
        QStringList parts;QSet<quint64> visited;quint64 id = it.key();bool excluded = false;
        while (id != 5) {
            if (!r.active()) return false;
            if (id < 16) { excluded = true;break; } // Reserved metadata ancestors are excluded.
            if (visited.contains(id) || visited.size() >= 128 || !records.contains(id)) return fail(p, "Unsupported NTFS parent path");
            visited.insert(id);const Record &item = records[id];
            if (!item.selected.present || item.selected.text == QStringLiteral(".")) return fail(p, "Invalid NTFS filename path");
            parts.prepend(item.selected.text);
            quint64 parent = item.selected.parent;
            if (parent >= 16 || parent == 5) {
                QMap<quint64, Record>::const_iterator parentIt = records.constFind(parent);
                if (parentIt == records.cend() || !parentIt->folder || parentIt->sequence != item.selected.sequence) return fail(p, "Stale or unavailable NTFS parent reference");
            }
            id = parent;
        }
        if (excluded) continue;
        if (record.folder && !record.unsupported.isEmpty()) return fail(p, "Unsupported NTFS directory metadata; no guest files were published");
        QString name = prefix + parts.join('/');
        if (name.size() > 4096 || paths.contains(name) || !r.retain(quint64(name.size()) * 2 + sizeof(Entry))) return fail(p, "Duplicate or over-limit NTFS path");
        paths.insert(name);
        Entry entry = {name, QStringLiteral("NTFS; MFT %1; unnamed DATA; %2 additional streams not exported").arg(record.index).arg(record.namedStreams), record.unsupported, record.folder, record.data};
        if (!record.folder && !record.hasData && entry.unsupported.isEmpty()) entry.unsupported = QStringLiteral("NTFS file has no supported unnamed data stream");
        if (record.folder) entry.data = Stream();
        p.entries.append(entry);
    }
    return r.active();
}

bool parse(Reader &r, Plan &p) {
    p.sourceSize = r.size;
    if (r.size < 512 || r.size > kMaxVolume) return fail(p, "Unsupported raw NTFS disk size");
    QByteArray boot = r.read(0, 512);
    if (boot.size() != 512) return false;
    if (boot.mid(3, 8) == QByteArray("NTFS    ", 8)) return volume(r, p, 0, r.size, QString());
    if (quint8(boot.at(510)) != 0x55 || quint8(boot.at(511)) != 0xaa) return fail(p, "No NTFS volume or supported MBR partition table");
    bool found = false;QVector<QPair<quint64, quint64> > extents;
    for (int i = 0; i < 4; ++i) {
        int at = 446 + i * 16;quint8 type = quint8(boot.at(at + 4));quint64 start = u32(boot, at + 8), count = u32(boot, at + 12);
        if (!type && !count) continue;
        if (!type || !count || !start || !span(start * 512, count * 512, r.size)) return fail(p, "Invalid MBR partition extent");
        if (type != 7) return fail(p, "Filesystem mode supports only NTFS primary MBR partitions; GPT/extended/other filesystems are unsupported");
        quint64 off = start * 512, len = count * 512;
        for (const QPair<quint64, quint64> &old : extents) if (off < old.first + old.second && old.first < off + len) return fail(p, "Overlapping MBR partitions");
        extents.append(qMakePair(off, len));
        if (!volume(r, p, off, len, QStringLiteral("Partition.%1/").arg(i + 1))) return false;
        found = true;
    }
    return found && r.active();
}
bool writeAll(Reader &r, QIODevice *output, const QByteArray &a) {
    QIODevice *guard = output;qint64 done = 0;
    while (done < a.size() && r.active() && guard) {
        qint64 n = guard->write(a.constData() + done, a.size() - done);
        if (!guard || !r.active() || n <= 0 || n > a.size() - done) return false;
        done += n;
    }
    return done == a.size() && r.active() && guard;
}
} // namespace

XNTFSArchive::XNTFSArchive(QIODevice *device) : XArchive(device) {}
XNTFSArchive::XNTFSArchive(QIODevice *identity, QIODevice *logical) : XArchive(identity), m_logicalDevice(logical), m_mapped(true) {}
QIODevice *XNTFSArchive::ntfsDataDevice() { return m_mapped ? m_logicalDevice : getDevice(); }
XBinary *XNTFSArchive::createInstance(QIODevice *device, bool image, XADDR address) { Q_UNUSED(image) Q_UNUSED(address) return new XNTFSArchive(device); }
bool XNTFSArchive::isValid(PDSTRUCT *pd) { Reader r(this, pd);Plan p;return parse(r, p); }
XBinary::FT XNTFSArchive::getFileType() { return FT_NTFS; }
QString XNTFSArchive::getFileFormatExt() { return QStringLiteral("ntfs"); }
QString XNTFSArchive::getMIMEString() { return QStringLiteral("application/octet-stream"); }
QString XNTFSArchive::getVersion() { return QString(); }
QList<XBinary::PM_INFO> XNTFSArchive::unpackImplemented() { PM_INFO p = {};p.hm[0] = HANDLE_METHOD_ARCHIVE_STREAM;return {p}; }

bool XNTFSArchive::initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pd) {
    if (!state || m_bUnpackOperationInProgress || !finishUnpack(state, nullptr)) return false;
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !isPdStructNotCanceled(pd)) return false;
    bool bound = bindUnpackSource(state, pd);if (!bound) return false;
    std::unique_ptr<Plan> plan(new (std::nothrow) Plan);Reader reader(this, pd);
    bool parsed = plan && parse(reader, *plan);
    if (!parsed) { QString error = plan ? plan->error : QString();releaseUnpackSource(state);*state = UNPACK_STATE();if (!error.isEmpty()) setPdStructErrorString(pd, error);return false; }
    state->pContext = plan.get();state->mapUnpackProperties = properties;state->nTotalSize = qint64(plan->sourceSize);
    state->nNumberOfRecords = int(plan->entries.size());state->nCurrentIndex = 0;state->nCurrentOffset = 0;
    if (!registerUnpackContextCleanup(state, plan.get(), &deleteUnpackContext<Plan>)) {
        state->pContext = nullptr;releaseUnpackSource(state);*state = UNPACK_STATE();return false;
    }
    Plan *context = plan.release();
    if (!validateAndFinalizeUnpackSource(state, context, pd)) { state->pContext = nullptr;releaseUnpackSource(state);delete context;*state = UNPACK_STATE();return false; }
    return true;
}
XBinary::ARCHIVERECORD XNTFSArchive::infoCurrent(UNPACK_STATE *state, PDSTRUCT *pd) {
    XNTFSArchive *self = this;UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !state || !state->pContext || state->nCurrentIndex < 0 || state->nCurrentIndex >= state->nNumberOfRecords) return record;
    bool current = isUnpackSourceCurrent(state, pd);if (!current) return record;
    const Plan &plan = *static_cast<Plan *>(state->pContext);if (state->nNumberOfRecords != plan.entries.size()) return record;
    const Entry &entry = plan.entries.at(state->nCurrentIndex);
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.name);record.mapProperties.insert(FPART_PROP_ISFOLDER, entry.folder);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, qint64(entry.data.size));record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, qint64(entry.data.size));
    record.mapProperties.insert(FPART_PROP_INFO, entry.unsupported.isEmpty() ? entry.info : entry.info + QStringLiteral("; unsupported: ") + entry.unsupported);
    if (!markArchiveStreamRecord(&record, state->nCurrentIndex)) return ARCHIVERECORD();return record;
}
bool XNTFSArchive::unpackCurrent(UNPACK_STATE *state, QIODevice *output, PDSTRUCT *pd) {
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);QIODevice *destination = output;
    if (!guard.isAcquired() || !state || !state->pContext || state->nCurrentIndex < 0 || state->nCurrentIndex >= state->nNumberOfRecords) return false;
    bool current = isUnpackSourceCurrent(state, pd);if (!current || !destination || !isUnpackOutputSupported(destination)) return false;
    if (!destination) return false;bool alias = devicesAlias(getDevice(), destination);if (!destination || alias) return false;
    const Plan &plan = *static_cast<Plan *>(state->pContext);if (state->nNumberOfRecords != plan.entries.size()) return false;
    const Entry entry = plan.entries.at(state->nCurrentIndex);
    if (!entry.unsupported.isEmpty()) { setPdStructErrorString(pd, entry.unsupported);return false; }
    if (!isUnpackOutputSizeAllowed(state->mapUnpackProperties, qint64(entry.data.size))) return false;
    if (state->spOutputBudget) {
        bool accepted = state->spOutputBudget->beginEntry(state->nCurrentIndex, entry.name) && state->spOutputBudget->debit(qint64(entry.data.size));
        if (!accepted) { if (state->spOutputBudget->isEnforcing()) { setPdStructErrorString(pd, tr("Unpacked output exceeds the configured limit"));return false; } OUTPUT_BUDGET::noteShadowRefusal(state->spOutputBudget.data()); }
    }
    Reader reader(this, pd);QTemporaryFile stage;if (!stage.open()) return false;
    for (quint64 off = 0; off < entry.data.size && reader.active();) {
        QByteArray bytes;quint64 n = qMin(quint64(65536), entry.data.size - off);
        if (!readStream(reader, entry.data, off, n, bytes, false) || !writeAll(reader, &stage, bytes)) return false;off += n;
    }
    if (!reader.active() || !destination || stage.size() != qint64(entry.data.size) || !stage.seek(0)) return false;
    current = isUnpackSourceCurrent(state, pd);if (!current || !destination) return false;
    bool published = publishUnpackOutput(&stage, destination, state, pd);if (!published) return false;
    state->nCurrentOffset = qint64(entry.data.size);return true;
}
bool XNTFSArchive::moveToNext(UNPACK_STATE *state, PDSTRUCT *pd) {
    XNTFSArchive *self = this;UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || !state->pContext || state->nCurrentIndex < 0 || state->nCurrentIndex >= state->nNumberOfRecords) return false;
    bool current = isUnpackSourceCurrent(state, pd);if (!current) return false;
    ++state->nCurrentIndex;return state->nCurrentIndex < state->nNumberOfRecords;
}
bool XNTFSArchive::finishUnpack(UNPACK_STATE *state, PDSTRUCT *pd) {
    Q_UNUSED(pd) UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state) return false;
    if ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state)) return false;
    Plan *context = static_cast<Plan *>(state->pContext);releaseUnpackSource(state);*state = UNPACK_STATE();delete context;return true;
}
