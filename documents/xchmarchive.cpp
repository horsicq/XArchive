// Functional translation of the reference implementation.
#include "xchmarchive.h"
#include "Algos/xlzxdecoder.h"
#include <QSet>
#include <QTemporaryFile>
#include <QtEndian>
#include <cstring>
#include <new>

namespace {
const qint64 MaxInput = 256LL * 1024 * 1024;
const qint64 MaxSection = 1024LL * 1024 * 1024;
const qint64 MaxGroup = 16LL * 1024 * 1024;
const qint32 FrameSize = 32768;
const qint32 MaxRecords = 100000;
quint16 u16(const QByteArray &b, qint64 p) { return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(b.constData() + p)); }
quint32 u32(const QByteArray &b, qint64 p) { return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(b.constData() + p)); }
quint64 u64(const QByteArray &b, qint64 p) { return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(b.constData() + p)); }
bool within(quint64 size, quint64 pos, quint64 count) { return pos <= size && count <= size - pos; }
bool magic(const QByteArray &b, qint64 p, const char *s) { return !std::memcmp(b.constData() + p, s, 4); }
bool encint(const QByteArray &b, qint64 end, qint64 *pos, quint64 *value)
{
    quint64 result = 0;
    for (int i = 0; i < 9 && *pos < end; ++i) {
        const quint8 byte = quint8(b.at(qsizetype((*pos)++)));
        result = (result << 7) | (byte & 127);
        if (!(byte & 128)) { *value = result; return true; }
    }
    return false;
}
}

XChmArchive::XChmArchive(QIODevice *device) : XArchive(device) {}
XBinary::FT XChmArchive::getFileType() { return FT_CHM; }
XBinary::MODE XChmArchive::getMode() { return MODE_DATA; }
qint32 XChmArchive::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XChmArchive::getEndian() { return ENDIAN_LITTLE; }
QString XChmArchive::getArch() { return QString(); }
QString XChmArchive::getVersion() { CONTEXT context; return readContext(&context, nullptr) ? QString::number(context.version) : QString(); }
QString XChmArchive::getFileFormatExt() { return QStringLiteral("chm"); }
QString XChmArchive::getFileFormatExtsString() { return QStringLiteral("Compiled HTML help (*.chm)"); }
QString XChmArchive::getMIMEString() { return QStringLiteral("application/vnd.ms-htmlhelp"); }
qint64 XChmArchive::getFileFormatSize(PDSTRUCT *progress) { CONTEXT context; return readContext(&context, progress) ? context.input.size() : 0; }
QList<QString> XChmArchive::getSearchSignatures() { return {QStringLiteral("'ITSF'")}; }
XBinary *XChmArchive::createInstance(QIODevice *device, bool image, XADDR address)
{ Q_UNUSED(image) Q_UNUSED(address) return new XChmArchive(device); }

bool XChmArchive::parse(const QByteArray &input, CONTEXT *context, PDSTRUCT *progress)
{
    if (!context || input.size() < 88 || input.size() > MaxInput || !magic(input, 0, "ITSF") || !isPdStructNotCanceled(progress)) return false;
    const quint32 version = u32(input, 4), headerSize = u32(input, 8);
    if ((version != 2 && version != 3) || headerSize != (version == 3 ? 96U : 88U) || (quint64)input.size() < (quint64)headerSize) return false;
    const quint64 section0 = u64(input, 56), section0Size = u64(input, 64), directory = u64(input, 72), directorySize = u64(input, 80);
    if (section0 < headerSize || section0Size < 24 || !within(input.size(), section0, section0Size) || directory < section0 + section0Size ||
        directorySize < 84 || !within(input.size(), directory, directorySize) || u32(input, qint64(section0)) != 0x1fe ||
        u64(input, qint64(section0 + 8)) != quint64(input.size())) return false;
    const quint64 content = version == 3 ? u64(input, 88) : directory + directorySize;
    if (content < directory + directorySize || content > quint64(input.size()) || !magic(input, qint64(directory), "ITSP") ||
        u32(input, qint64(directory + 4)) != 1 || u32(input, qint64(directory + 8)) != 84) return false;
    const quint32 blockSize = u32(input, qint64(directory + 16)), blockCount = u32(input, qint64(directory + 44));
    if (blockSize < 32 || blockSize > 1024 * 1024 || !blockCount || blockCount > MaxRecords ||
        directorySize != 84 + quint64(blockSize) * blockCount) return false;
    CONTEXT parsed;
    parsed.input = input; parsed.version = qint32(version);
    SECTION rawSection;
    rawSection.offset = qint64(content); rawSection.size = rawSection.rawSize = input.size() - qint64(content);
    parsed.sections.append(rawSection);
    QMap<QString,MEMBER> all;
    qint32 listings = 0;
    // The reference implementation enumerates every PMGL chunk rather than depending on the search index.
    for (quint32 block = 0; block < blockCount; ++block) {
        if (!isPdStructNotCanceled(progress)) return false;
        const qint64 start = qint64(directory + 84 + quint64(block) * blockSize);
        if (magic(input, start, "PMGI")) continue;
        if (!magic(input, start, "PMGL")) return false;
        ++listings;
        const quint32 free = u32(input, start + 4);
        if (free < 2 || free > blockSize - 20) return false;
        qint64 pos = start + 20;
        const qint64 end = start + blockSize - free;
        qint32 records = 0;
        while (pos < end) {
            quint64 length = 0, section = 0, offset = 0, size = 0;
            if (all.size() >= MaxRecords || !encint(input, end, &pos, &length) || !length || length > 65536 || !within(end, pos, length)) return false;
            const QByteArray nameBytes = input.mid(qsizetype(pos), qsizetype(length));
            pos += qint64(length);
            const QString name = QString::fromUtf8(nameBytes);
            if (name.contains(QChar(0)) || name.toUtf8() != nameBytes || all.contains(name) || !encint(input, end, &pos, &section) ||
                !encint(input, end, &pos, &offset) || !encint(input, end, &pos, &size) || section >= 16 || offset > quint64(MaxSection) || size > quint64(MaxSection)) return false;
            MEMBER member;
            member.name = name; member.section = qint32(section); member.offset = qint64(offset); member.size = qint64(size); member.folder = name.endsWith('/');
            if (member.folder && member.size) return false;
            all.insert(name, member); ++records;
            // Literal strings recovered at 0064b878/888/898/8a8.
            if (!name.startsWith(QStringLiteral("/#")) && !name.startsWith(QStringLiteral("/$")) && !name.startsWith(':') && name != QStringLiteral("/")) {
                if (member.name.startsWith('/')) member.name.remove(0, 1);
                if (member.name.isEmpty()) return false;
                parsed.members.append(member);
            }
        }
        const quint16 footerCount = u16(input, start + blockSize - 2);
        if (pos != end || (footerCount && footerCount != records)) return false;
    }
    if (!listings) return false;
    // MEMBER is private to XChmArchive, so the helper is a local functor of the
    // member function rather than a file-local one.
    struct STORED {
        const QByteArray &input;
        const QMap<QString,XChmArchive::MEMBER> &all;
        quint64 content;
        STORED(const QByteArray &inputRef, const QMap<QString,XChmArchive::MEMBER> &allRef, quint64 contentValue) : input(inputRef), all(allRef), content(contentValue) {}
        bool operator()(const QString &name, QByteArray *result) const {
            const QMap<QString,XChmArchive::MEMBER>::const_iterator i = all.constFind(name);
            if (i == all.cend() || i->section != 0 || !within(input.size() - content, quint64(i->offset), quint64(i->size))) return false;
            *result = input.mid(qsizetype(content + quint64(i->offset)), qsizetype(i->size)); return true;
        }
    };
    const STORED stored(input, all, content);
    qint32 maxSection = 0;
    for (const MEMBER &member : all) maxSection = qMax(maxSection, member.section);
    if (maxSection) {
        QByteArray names;
        if (!stored(QStringLiteral("::DataSpace/NameList"), &names) || names.size() < 4) return false;
        const quint16 count = u16(names, 2);
        if (count < 2 || count > 16 || maxSection >= count) return false;
        qint64 pos = 4;
        QSet<QString> seen;
        for (quint16 index = 0; index < count; ++index) {
            if (!within(names.size(), pos, 2)) return false;
            const quint16 length = u16(names, pos); pos += 2;
            if (!length || length > 32 || !within(names.size(), pos, (quint64(length) + 1) * 2) || u16(names, pos + length * 2)) return false;
            QString sectionName;
            for (quint16 n = 0; n < length; ++n) sectionName.append(QChar(u16(names, pos + n * 2)));
            pos += (qint64(length) + 1) * 2;
            if (sectionName.contains(QChar(0)) || sectionName.contains('/') || sectionName.contains('\\') || seen.contains(sectionName)) return false;
            seen.insert(sectionName);
            if (index == 0) continue;
            const QString prefix = QStringLiteral("::DataSpace/Storage/") + sectionName + '/';
            const QMap<QString,XChmArchive::MEMBER>::const_iterator contentEntry = all.constFind(prefix + QStringLiteral("Content"));
            if (contentEntry == all.cend() || contentEntry->section != 0 ||
                !within(input.size() - content, quint64(contentEntry->offset), quint64(contentEntry->size))) return false;
            SECTION section;
            section.offset = qint64(content) + contentEntry->offset; section.size = section.rawSize = contentEntry->size;
            const QString resetName = prefix + QStringLiteral("Transform/{7FC28940-9D31-11D0-9B27-00A0C91E9C7C}/InstanceData/ResetTable");
            if (all.contains(resetName)) {
                QByteArray control, reset, span;
                if (!stored(prefix + QStringLiteral("ControlData"), &control) || control.size() < 24 || !magic(control, 4, "LZXC") || u32(control, 8) != 2 ||
                    u32(control, 0) < 5 || (quint64(u32(control, 0)) + 1) * 4 != quint64(control.size()) ||
                    !stored(resetName, &reset) || reset.size() < 40 || (u32(reset, 0) != 2 && u32(reset, 0) != 3) ||
                    u32(reset, 8) != 8 || u32(reset, 12) != 40 || u64(reset, 32) != FrameSize) return false;
                const quint32 resetFrames = u32(control, 12), windowUnits = u32(control, 16);
                if (!resetFrames || resetFrames > MaxGroup / FrameSize || !windowUnits || windowUnits > 64 || (windowUnits & (windowUnits - 1))) return false;
                section.windowBits = 15;
                for (quint32 unit = windowUnits; unit > 1; unit >>= 1) ++section.windowBits;
                section.resetFrames = qint32(resetFrames); section.lzx = true;
                const quint64 rawSize = u64(reset, 16), packedSize = u64(reset, 24);
                if (!rawSize || rawSize > quint64(MaxSection) || packedSize != quint64(section.size)) return false;
                section.rawSize = qint64(rawSize);
                if (all.contains(prefix + QStringLiteral("SpanInfo")) &&
                    (!stored(prefix + QStringLiteral("SpanInfo"), &span) || span.size() != 8 || u64(span, 0) != rawSize)) return false;
                const quint64 frames = (rawSize + FrameSize - 1) / FrameSize;
                const quint32 entries = u32(reset, 4);
                if ((entries != frames && entries != frames + 1) || reset.size() != 40 + quint64(entries) * 8) return false;
                for (quint32 n = 0; n < entries; ++n) {
                    const quint64 offset = u64(reset, 40 + qint64(n) * 8);
                    if (offset > packedSize || (n == 0 ? offset != 0 : offset <= quint64(section.frames.last()))) return false;
                    section.frames.append(qint64(offset));
                }
                if (entries == frames) section.frames.append(section.size);
                if (section.frames.last() != section.size) return false;
            } else if (all.contains(prefix + QStringLiteral("ControlData"))) {
                // A transform without its reset table cannot be treated as copy.
                return false;
            }
            parsed.sections.append(section);
        }
        if (pos != names.size()) return false;
    }
    for (const MEMBER &member : all) {
        if (member.section >= parsed.sections.size() || !within(quint64(parsed.sections.at(member.section).rawSize), quint64(member.offset), quint64(member.size))) return false;
    }
    *context = std::move(parsed);
    return isPdStructNotCanceled(progress);
}

bool XChmArchive::readContext(CONTEXT *context, PDSTRUCT *progress)
{
    QPointer<XChmArchive> owner(this);
    QPointer<QIODevice> source(getDevice());
    if (!context || !source || !source->isOpen() || !source->isReadable() || source->isSequential() || !isPdStructNotCanceled(progress)) return false;
    if (!owner || !source) return false;
    const qint64 size = source->size();
    if (!owner || !source || size < 88 || size > MaxInput) return false;
    const QByteArray signature = read_array_process(0, 88, progress);
    if (!owner || !source || signature.size() != 88 || !magic(signature, 0, "ITSF") || (u32(signature, 4) != 2 && u32(signature, 4) != 3)) return false;
    const QByteArray input = read_array_process(0, size, progress);
    if (!owner || !source || input.size() != size) return false;
    try { return parse(input, context, progress); } catch (const std::bad_alloc &) { return false; }
}
bool XChmArchive::isValid(PDSTRUCT *progress) { CONTEXT context; return readContext(&context, progress); }
bool XChmArchive::isValid(QIODevice *device, PDSTRUCT *progress) { XChmArchive archive(device); return archive.isValid(progress); }

bool XChmArchive::initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP,QVariant> &properties, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XChmArchive> owner(this);
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

XBinary::ARCHIVERECORD XChmArchive::infoCurrent(UNPACK_STATE *state, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    QPointer<XChmArchive> owner(this);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !state || !state->pContext || !isUnpackSourceCurrent(state, progress) || !owner) return record;
    const CONTEXT context = *static_cast<CONTEXT *>(state->pContext);
    if (state->nCurrentIndex < 0 || state->nCurrentIndex >= context.members.size() || state->nNumberOfRecords != context.members.size()) return record;
    const MEMBER member = context.members.at(qsizetype(state->nCurrentIndex));
    const SECTION section = context.sections.at(member.section);
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.name);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.size);
    record.mapProperties.insert(FPART_PROP_ISFOLDER, member.folder);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, section.lzx ? QStringLiteral("CHM LZX:%1").arg(section.windowBits) : QStringLiteral("CHM Copy"));
    if (!markArchiveStreamRecord(&record, state->nCurrentIndex)) return ARCHIVERECORD();
    return record;
}

bool XChmArchive::unpackCurrent(UNPACK_STATE *state, QIODevice *device, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XChmArchive> owner(this);
    QPointer<QIODevice> source(getDevice()), output(device);
    if (!guard.isAcquired() || !state || !state->pContext || !source || !output || !isUnpackSourceCurrent(state, progress) || !owner || !source || !output) return false;
    const bool supported = isUnpackOutputSupported(output.data());
    if (!owner || !source || !output || !supported) return false;
    const bool aliases = devicesAlias(source.data(), output.data());
    if (!owner || !source || !output || aliases) return false;
    const CONTEXT context = *static_cast<CONTEXT *>(state->pContext);
    if (state->nCurrentIndex < 0 || state->nCurrentIndex >= context.members.size() || state->nNumberOfRecords != context.members.size()) return false;
    const MEMBER member = context.members.at(qsizetype(state->nCurrentIndex));
    const SECTION section = context.sections.at(member.section);
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
    // Both helpers hold references to the locals above, so every read observes
    // the state at call time exactly as the captured-by-reference form did.
    struct CANCELED {
        const QPointer<XChmArchive> &owner;
        const QPointer<QIODevice> &source;
        const QPointer<QIODevice> &output;
        XBinary::PDSTRUCT *progress;
        CANCELED(const QPointer<XChmArchive> &ownerRef, const QPointer<QIODevice> &sourceRef, const QPointer<QIODevice> &outputRef, XBinary::PDSTRUCT *pProgress)
            : owner(ownerRef), source(sourceRef), output(outputRef), progress(pProgress) {}
        bool operator()() const { return !owner || !source || !output || !XBinary::isPdStructNotCanceled(progress); }
    };
    const CANCELED canceled(owner, source, output, progress);
    struct WRITE {
        const CANCELED &canceled;
        XBinary::DATAPROCESS_STATE &writer;
        WRITE(const CANCELED &canceledRef, XBinary::DATAPROCESS_STATE &writerRef) : canceled(canceledRef), writer(writerRef) {}
        bool operator()(const char *data, qint64 size) const {
            for (qint64 done = 0; done < size;) {
                const qint64 take = qMin<qint64>(65536, size - done);
                if (canceled() || XBinary::_writeDevice(data + done, take, &writer) != take) return false;
                done += take;
            }
            return !canceled();
        }
    };
    const WRITE write(canceled, writer);
    try {
        if (!section.lzx) {
            if (!write(context.input.constData() + section.offset + member.offset, member.size)) return false;
        } else if (member.size) {
            const qint64 firstFrame = member.offset / FrameSize, lastFrame = (member.offset + member.size - 1) / FrameSize;
            const qint64 totalFrames = section.frames.size() - 1;
            for (qint64 group = firstFrame / section.resetFrames; group <= lastFrame / section.resetFrames; ++group) {
                if (canceled()) return false;
                const qint64 first = group * section.resetFrames, count = qMin<qint64>(section.resetFrames, totalFrames - first);
                const qint64 decodedSize = count * FrameSize;
                if (count <= 0 || decodedSize > MaxGroup || (policy.nMaxMemoryOutputSize >= 0 && decodedSize > policy.nMaxMemoryOutputSize)) return false;
                QList<QByteArray> blocks;
                QList<qint32> sizes;
                for (qint64 frame = first; frame < first + count; ++frame) {
                    const qint64 begin = section.frames.at(qsizetype(frame)), end = section.frames.at(qsizetype(frame + 1));
                    if (end <= begin || end - begin > MaxGroup) return false;
                    blocks.append(QByteArray::fromRawData(context.input.constData() + section.offset + begin, qsizetype(end - begin)));
                    sizes.append(FrameSize);
                }
                QByteArray decoded;
                if (!XLZXDecoder::decompressCABDataBlocks(blocks, sizes, &decoded, section.windowBits, progress) || decoded.size() != decodedSize || canceled()) return false;
                // The final CHM frame is padded to 32 KiB. Only the logical
                // section extent and selected member intersection are exposed.
                const qint64 base = first * FrameSize;
                const qint64 from = qMax(member.offset, base), to = qMin(member.offset + member.size, base + decodedSize);
                if (to <= from || !write(decoded.constData() + from - base, to - from)) return false;
            }
        }
    } catch (const std::bad_alloc &) { return false; }
    if (canceled() || stage.size() != member.size || !stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(state, progress) || canceled()) return false;
    const bool published = publishUnpackOutput(&stage, output.data(), state, progress);
    return owner && output && published;
}
bool XChmArchive::moveToNext(UNPACK_STATE *state, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QPointer<XChmArchive> owner(this);
    if (!guard.isAcquired() || !state || !state->pContext || !isUnpackSourceCurrent(state, progress) || !owner) return false;
    const qint64 count = static_cast<CONTEXT *>(state->pContext)->members.size();
    if (state->nNumberOfRecords != count || state->nCurrentIndex < 0 || state->nCurrentIndex >= count) return false;
    ++state->nCurrentIndex; return state->nCurrentIndex < count;
}
bool XChmArchive::finishUnpack(UNPACK_STATE *state, PDSTRUCT *progress)
{
    Q_UNUSED(progress)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *context = static_cast<CONTEXT *>(state->pContext);
    state->pContext = nullptr; releaseUnpackSource(state); delete context; *state = UNPACK_STATE(); return true;
}
