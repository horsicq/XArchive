// Functional translation of U3 0065d610 and 0065cf90.
#include "xpboarchive.h"
#include <QTemporaryFile>
#include <QCryptographicHash>
#include <QtEndian>
#include <cstring>
#include <new>

namespace {
const qint64 MaxInput = 256LL * 1024 * 1024;
const qint64 MaxOutput = 1024LL * 1024 * 1024;
const qint64 MaxHeader = 8LL * 1024 * 1024;
quint32 u32(const QByteArray &b, qint64 p) { return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(b.constData() + p)); }
bool within(qint64 total, qint64 pos, qint64 size) { return pos >= 0 && size >= 0 && pos <= total && size <= total - pos; }
bool string(const QByteArray &data, qint64 *position, QByteArray *result)
{
    const qint64 start = *position;
    while (*position < data.size() && *position < MaxHeader && *position - start <= 1024) {
        if (!data.at(qsizetype((*position)++))) { *result = data.mid(qsizetype(start), qsizetype(*position - start - 1)); return true; }
    }
    return false;
}
bool flushPbo(const char *output, qint32 *buffered, XBinary::DATAPROCESS_STATE *writer, XBinary::PDSTRUCT *progress)
{
    const qint32 count = *buffered;
    if (!XBinary::isPdStructNotCanceled(progress) || XBinary::_writeDevice(output, count, writer) != count) return false;
    *buffered = 0; return true;
}
bool decodePbo(const QByteArray &input, qint64 offset, qint64 packed, qint64 size, XBinary::DATAPROCESS_STATE *writer, XBinary::PDSTRUCT *progress)
{
    if (!writer || packed < 5 || size <= 0 || size > MaxOutput || !within(input.size(), offset, packed)) return false;
    XBinary::OUTPUT_POLICY policy = {};
    if (!XBinary::resolveUnpackOutputPolicy(writer->mapUnpackProperties, &policy) ||
        (policy.nMaxMemoryOutputSize >= 0 && policy.nMaxMemoryOutputSize < 4096)) return false;
    unsigned char ring[4096]; std::memset(ring, 0x20, sizeof(ring));
    char output[4096]; qint32 buffered = 0;
    quint32 ringPos = 0, checksum = 0;
    const qint64 end = offset + packed - 4;
    qint64 pos = offset, produced = 0;
    quint32 flags = 0;
    while (produced < size) {
        if (!XBinary::isPdStructNotCanceled(progress)) return false;
        flags >>= 1;
        if (!(flags & 0x100)) {
            if (pos >= end) return false;
            flags = quint8(input.at(qsizetype(pos++))) | 0xff00;
        }
        if (pos >= end) return false;
        const quint8 low = quint8(input.at(qsizetype(pos++)));
        quint32 source = 0, count = 1;
        if (!(flags & 1)) {
            if (pos >= end) return false;
            const quint8 high = quint8(input.at(qsizetype(pos++)));
            source = (ringPos - (low | ((high & 0xf0U) << 4))) & 4095;
            count = (high & 15U) + 3;
        }
        if (qint64(count) > size - produced) return false;
        for (quint32 i = 0; i < count; ++i) {
            const quint8 value = (flags & 1) ? low : ring[(source + i) & 4095];
            ring[ringPos] = value; ringPos = (ringPos + 1) & 4095;
            checksum += value; output[buffered++] = char(value); ++produced;
            if (buffered == 4096 && !flushPbo(output, &buffered, writer, progress)) return false;
        }
    }
    // U3 ends at the declared output length. The native path additionally
    // verifies the four-byte additive checksum and exact packet extent.
    return pos == end && checksum == u32(input, end) && (!buffered || flushPbo(output, &buffered, writer, progress)) && XBinary::isPdStructNotCanceled(progress);
}
// Captured state for the unpack path, held by reference the way the xancient*
// decoders in this tree carry their per-call scratch.
struct PboCanceled {
    const QPointer<XPboArchive> &owner;
    const QPointer<QIODevice> &source;
    const QPointer<QIODevice> &output;
    XBinary::PDSTRUCT *progress;
    PboCanceled(const QPointer<XPboArchive> &owner, const QPointer<QIODevice> &source, const QPointer<QIODevice> &output, XBinary::PDSTRUCT *progress)
        : owner(owner), source(source), output(output), progress(progress) {}
    bool operator()() const { return !owner || !source || !output || !XBinary::isPdStructNotCanceled(progress); }
};
struct PboWrite {
    const PboCanceled &canceled;
    XBinary::DATAPROCESS_STATE *writer;
    PboWrite(const PboCanceled &canceled, XBinary::DATAPROCESS_STATE *writer) : canceled(canceled), writer(writer) {}
    bool operator()(const char *data, qint64 size) const
    {
        for (qint64 done = 0; done < size;) {
            const qint64 take = qMin<qint64>(65536, size - done);
            if (canceled() || XBinary::_writeDevice(data + done, take, writer) != take) return false;
            done += take;
        }
        return !canceled();
    }
};
}

XPboArchive::XPboArchive(QIODevice *device) : XArchive(device) {}
XBinary::FT XPboArchive::getFileType() { return FT_BOHEMIA_PBO; }
XBinary::MODE XPboArchive::getMode() { return MODE_DATA; }
qint32 XPboArchive::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XPboArchive::getEndian() { return ENDIAN_LITTLE; }
QString XPboArchive::getArch() { return QString(); }
QString XPboArchive::getVersion() { return QString(); }
QString XPboArchive::getFileFormatExt() { return QStringLiteral("pbo"); }
QString XPboArchive::getFileFormatExtsString() { return QStringLiteral("Bohemia PBO archive (*.pbo)"); }
QString XPboArchive::getMIMEString() { return QStringLiteral("application/octet-stream"); }
qint64 XPboArchive::getFileFormatSize(PDSTRUCT *progress) { CONTEXT context; return readContext(&context, progress) ? context.input.size() : 0; }
QList<QString> XPboArchive::getSearchSignatures() { return {QStringLiteral("00'sreV'00000000000000000000000000000000")}; }
XBinary *XPboArchive::createInstance(QIODevice *device, bool image, XADDR address)
{ Q_UNUSED(image) Q_UNUSED(address) return new XPboArchive(device); }

bool XPboArchive::parse(const QByteArray &input, CONTEXT *context, PDSTRUCT *progress)
{
    if (!context || input.size() < 21 || input.size() > MaxInput || !isPdStructNotCanceled(progress)) return false;
    CONTEXT parsed; parsed.input = input;
    qint64 pos = 0, packedTotal = 0;
    if (!input.at(0) && u32(input, 1) == 0x56657273) {
        if (u32(input, 5) || u32(input, 9) || u32(input, 13) || u32(input, 17)) return false;
        pos = 21; parsed.properties = true;
        for (qint32 count = 0;; ++count) {
            QByteArray key, value;
            if (count >= 4096 || !string(input, &pos, &key)) return false;
            if (key.isEmpty()) break;
            if (!string(input, &pos, &value)) return false;
        }
    }
    for (;;) {
        if (!isPdStructNotCanceled(progress)) return false;
        QByteArray nameBytes;
        if (!string(input, &pos, &nameBytes) || !within(input.size(), pos, 20) || pos + 20 > MaxHeader) return false;
        const quint32 method = u32(input, pos), raw = u32(input, pos + 4), reserved = u32(input, pos + 8);
        const quint32 timestamp = u32(input, pos + 12), packed = u32(input, pos + 16); pos += 20;
        if (nameBytes.isEmpty()) {
            if (method || raw || reserved || timestamp || packed) return false;
            break;
        }
        if (parsed.members.size() >= 65536 || (method != 0 && method != 0x43707273) || reserved || raw > quint64(MaxOutput) || packed > 0x7fffffffU) return false;
        for (char byte : nameBytes) if (quint8(byte) < 0x20 || quint8(byte) == 0x7f) return false;
        MEMBER member;
        member.name = QString::fromLatin1(nameBytes); member.name.replace('\\', '/');
        if (member.name.isEmpty() || member.name.startsWith('/') || fixFileName(member.name) != member.name) return false;
        const QStringList components = member.name.split('/', Qt::KeepEmptyParts);
        for (const QString &part : components) if (part.isEmpty() || part == QStringLiteral(".") || part == QStringLiteral("..")) return false;
        member.compressed = method != 0; member.packedSize = packed; member.size = member.compressed ? raw : packed; member.timestamp = timestamp;
        if (member.size > MaxOutput || (member.compressed ? (raw == 0 || packed < 5) : (raw != 0 && raw != packed)) || !within(input.size(), packedTotal, packed)) return false;
        packedTotal += packed; parsed.members.append(member);
    }
    if (parsed.members.isEmpty() || !within(input.size(), pos, packedTotal)) return false;
    const qint64 dataEnd = pos + packedTotal, tail = input.size() - dataEnd;
    if (tail) {
        if (tail != 21 || input.at(qsizetype(dataEnd))) return false;
        QCryptographicHash hash(QCryptographicHash::Sha1);
        for (qint64 offset = 0; offset < dataEnd;) {
            if (!isPdStructNotCanceled(progress)) return false;
            const qint32 take = qint32(qMin<qint64>(65536, dataEnd - offset));
            hash.addData(QByteArray::fromRawData(input.constData() + offset, take)); offset += take;
        }
        if (hash.result() != input.mid(qsizetype(dataEnd + 1), 20)) return false;
        parsed.sha1 = true;
    }
    for (MEMBER &member : parsed.members) { member.offset = pos; pos += member.packedSize; }
    *context = std::move(parsed); return isPdStructNotCanceled(progress);
}

bool XPboArchive::readContext(CONTEXT *context, PDSTRUCT *progress)
{
    QPointer<XPboArchive> owner(this);
    QPointer<QIODevice> source(getDevice());
    if (!context || !source || !source->isOpen() || !source->isReadable() || source->isSequential() || !isPdStructNotCanceled(progress)) return false;
    if (!owner || !source) return false;
    const qint64 size = source->size();
    if (!owner || !source || size < 21 || size > MaxInput) return false;
    const QByteArray prefix = read_array_process(0, qMin<qint64>(1045, size), progress);
    if (!owner || !source || prefix.isEmpty()) return false;
    // There is no mandatory magic. A bounded plausible first record is only
    // a preflight; recognition still requires the complete table and extent.
    const qsizetype zero = prefix.indexOf(char(0));
    if (zero < 0 || zero > 1024 || prefix.size() - zero < 21 ||
        (u32(prefix, zero + 1) != 0 && u32(prefix, zero + 1) != 0x43707273 && u32(prefix, zero + 1) != 0x56657273)) return false;
    if (zero == 0 && u32(prefix, 1) != 0x56657273) return false;
    for (qsizetype i = 0; i < zero; ++i) if (quint8(prefix.at(i)) < 0x20 || quint8(prefix.at(i)) == 0x7f) return false;
    const QByteArray input = read_array_process(0, size, progress);
    if (!owner || !source || input.size() != size) return false;
    try { return parse(input, context, progress); } catch (const std::bad_alloc &) { return false; }
}
bool XPboArchive::isValid(PDSTRUCT *progress) { CONTEXT context; return readContext(&context, progress); }
bool XPboArchive::isValid(QIODevice *device, PDSTRUCT *progress) { XPboArchive archive(device); return archive.isValid(progress); }

bool XPboArchive::initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP,QVariant> &properties, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XPboArchive> owner(this);
    if (!guard.isAcquired() || !state || ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *old = static_cast<CONTEXT *>(state->pContext);
    releaseUnpackSource(state); delete old; *state = UNPACK_STATE();
    if (!isPdStructNotCanceled(progress)) return false;
    const bool bound = bindUnpackSource(state, progress);
    if (!owner || !bound) return false;
    CONTEXT *context = new (std::nothrow) CONTEXT;
    OUTPUT_POLICY policy = {};
    const bool valid = context && resolveUnpackOutputPolicy(properties, &policy) && readContext(context, progress);
    if (!owner) { delete context; return false; }
    if (!valid) { releaseUnpackSource(state); delete context; *state = UNPACK_STATE(); return false; }
    state->pContext = context;
    state->nNumberOfRecords = context->members.size(); state->nCurrentIndex = 0;
    state->nTotalSize = context->input.size(); state->mapUnpackProperties = properties;
    const bool finalized = validateAndFinalizeUnpackSource(state, context, progress);
    if (!owner) return false;
    if (!finalized) { state->pContext = nullptr; releaseUnpackSource(state); delete context; *state = UNPACK_STATE(); return false; }
    return true;
}

XBinary::ARCHIVERECORD XPboArchive::infoCurrent(UNPACK_STATE *state, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XPboArchive> owner(this);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !state || !state->pContext || !isUnpackSourceCurrent(state, progress) || !owner) return record;
    const CONTEXT context = *static_cast<CONTEXT *>(state->pContext);
    if (state->nCurrentIndex < 0 || state->nCurrentIndex >= context.members.size() || state->nNumberOfRecords != context.members.size()) return record;
    const MEMBER member = context.members.at(qsizetype(state->nCurrentIndex));
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.name);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.size);
    record.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.packedSize);
    record.mapProperties.insert(FPART_PROP_MTIME, QDateTime::fromSecsSinceEpoch(member.timestamp).toUTC());
    record.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch(member.timestamp).toUTC());
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, member.compressed ? QStringLiteral("PBO Cprs LZSS") : QStringLiteral("PBO Copy"));
    if (!markArchiveStreamRecord(&record, state->nCurrentIndex)) return ARCHIVERECORD();
    return record;
}

bool XPboArchive::unpackCurrent(UNPACK_STATE *state, QIODevice *device, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XPboArchive> owner(this);
    QPointer<QIODevice> source(getDevice()), output(device);
    if (!guard.isAcquired() || !state || !state->pContext || !source || !output || !isUnpackSourceCurrent(state, progress) || !owner || !source || !output) return false;
    const bool supported = isUnpackOutputSupported(output.data());
    if (!owner || !source || !output || !supported) return false;
    const bool aliases = devicesAlias(source.data(), output.data());
    if (!owner || !source || !output || aliases) return false;
    const CONTEXT context = *static_cast<CONTEXT *>(state->pContext);
    if (state->nCurrentIndex < 0 || state->nCurrentIndex >= context.members.size() || state->nNumberOfRecords != context.members.size()) return false;
    const MEMBER member = context.members.at(qsizetype(state->nCurrentIndex));
    const QMap<UNPACK_PROP,QVariant> properties = state->mapUnpackProperties;
    const QSharedPointer<OUTPUT_BUDGET> budget = state->spOutputBudget;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(properties, &policy) || (policy.nMaxEntryOutputSize >= 0 && member.size > policy.nMaxEntryOutputSize)) return false;
    if (budget && !budget->beginEntry(state->nCurrentIndex, member.name)) {
        if (budget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(budget.data());
    }
    if (budget && budget->isEnforcing() && budget->totalLimit() >= 0 &&
        (budget->totalWritten() > budget->totalLimit() || member.size > budget->totalLimit() - budget->totalWritten())) return false;
    QTemporaryFile stage;
    if (!stage.open()) return false;
    DATAPROCESS_STATE writer = {};
    writer.pDeviceOutput = &stage; writer.nProcessedLimit = -1; writer.mapUnpackProperties = properties; writer.spOutputBudget = budget;
    const PboCanceled canceled(owner, source, output, progress);
    const PboWrite write(canceled, &writer);
    try {
        if (member.compressed) {
            if (!decodePbo(context.input, member.offset, member.packedSize, member.size, &writer, progress)) return false;
        } else if (!write(context.input.constData() + member.offset, member.size)) return false;
    } catch (const std::bad_alloc &) { return false; }
    if (canceled() || stage.size() != member.size || !stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(state, progress) || canceled()) return false;
    const bool published = publishUnpackOutput(&stage, output.data(), state, progress);
    return owner && output && published;
}
bool XPboArchive::moveToNext(UNPACK_STATE *state, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XPboArchive> owner(this);
    if (!guard.isAcquired() || !state || !state->pContext || !isUnpackSourceCurrent(state, progress) || !owner) return false;
    const qint64 count = static_cast<CONTEXT *>(state->pContext)->members.size();
    if (state->nNumberOfRecords != count || state->nCurrentIndex < 0 || state->nCurrentIndex >= count) return false;
    ++state->nCurrentIndex; return state->nCurrentIndex < count;
}
bool XPboArchive::finishUnpack(UNPACK_STATE *state, PDSTRUCT *progress)
{
    Q_UNUSED(progress)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *context = static_cast<CONTEXT *>(state->pContext);
    state->pContext = nullptr; releaseUnpackSource(state); delete context; *state = UNPACK_STATE(); return true;
}
