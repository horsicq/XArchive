/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#include "xsqlitearchive.h"

#include <QtEndian>
#include <functional>
#include <limits>
#include <memory>
#include <new>

namespace {
const qint64 MAX_SOURCE = 128LL * 1024 * 1024;
const qint64 MAX_EXPORT = 128LL * 1024 * 1024;
const qint64 MAX_PAYLOAD = 16LL * 1024 * 1024;
const qint64 MAX_PARSE_BYTES = 256LL * 1024 * 1024;
const qint32 MAX_TABLES = 4096;
const qint32 MAX_COLUMNS = 1024;
const qint64 MAX_ROWS = 1000000;

quint16 be16(const char *p) { return qFromBigEndian<quint16>(reinterpret_cast<const uchar *>(p)); }
quint32 be32(const char *p) { return qFromBigEndian<quint32>(reinterpret_cast<const uchar *>(p)); }

// the ninth varint byte contributes eight bits.
bool varint(const QByteArray &bytes, qint32 *position, qint32 end, quint64 *value)
{
    if (!position || !value || *position < 0 || end < *position || end > bytes.size()) return false;
    *value = 0;
    for (qint32 i = 0; i < 9; ++i) {
        if (*position >= end) return false;
        const quint8 byte = quint8(bytes.at((*position)++));
        if (i == 8) { *value = (*value << 8) | byte; return true; }
        *value = (*value << 7) | (byte & 0x7fU);
        if (!(byte & 0x80U)) return true;
    }
    return false;
}

struct Field {
    quint64 serial = 0;
    quint64 integer = 0;
    QString text;
};
struct Table { QString name; quint32 root = 0; };

// Exact filename transformation used by 00424220/00423900. Collisions are
// detected separately: two table names must never overwrite one export.
QString u3TableName(QString name)
{
    if (name.startsWith('/') || name.startsWith('\\')) name.remove(0, 1);
    if (name.endsWith('/') || name.endsWith('\\')) name.chop(1);
    if (name.endsWith('\r')) name.chop(1);
    while (name.endsWith(' ') || name.endsWith('.')) name.chop(1);
    if (name.isEmpty()) return QStringLiteral("_");
    name = name.left(255);
    const QString forbidden = QStringLiteral("\"*/:<>?\\|");
    for (qint32 i = 0; i < name.size(); ++i) {
        if (name.at(i).unicode() < 32 || forbidden.contains(name.at(i))) name[i] = QLatin1Char('_');
    }
    const QString stem = name.section('.', 0, 0).toUpper();
    const bool numbered = stem.size() == 4 && (stem.startsWith("COM") || stem.startsWith("LPT")) && stem.at(3) >= '1' && stem.at(3) <= '9';
    if (stem == "AUX" || stem == "CON" || stem == "NUL" || stem == "PRN" || stem == "CLOCK$" || numbered) name.prepend('_');
    return name;
}

QByteArray utf16LE(const QString &text)
{
    QByteArray bytes(text.size() * 2, 0);
    for (qint32 i = 0; i < text.size(); ++i) qToLittleEndian<quint16>(text.at(i).unicode(), reinterpret_cast<uchar *>(bytes.data() + i * 2));
    return bytes;
}

// Table pages and overflow chains follow
// 00678c80/00678e40 and 00678fe0. Bounds follow SQLite's published format.
class Reader {
public:
    Reader(const QByteArray &bytes, XBinary::PDSTRUCT *pd) : m_bytes(bytes), m_pd(pd) {}
    QString error;
    bool outputLimitHit = false;

    bool header()
    {
        if (!alive() || m_bytes.size() < 512 || m_bytes.left(16) != QByteArray("SQLite format 3\0", 16)) return fail("Invalid SQLite header");
        pageSize = be16(m_bytes.constData() + 16);
        if (pageSize == 1) pageSize = 65536;  // Current SQLite representation; the zero sentinel was erroneous.
        if (pageSize < 512 || pageSize > 65536 || (pageSize & (pageSize - 1)) || m_bytes.size() % pageSize) return fail("Invalid SQLite page size");
        usable = pageSize - quint8(m_bytes.at(20));
        if (usable < 480 || quint8(m_bytes.at(21)) != 64 || quint8(m_bytes.at(22)) != 32 || quint8(m_bytes.at(23)) != 32 ||
            quint8(m_bytes.at(18)) < 1 || quint8(m_bytes.at(18)) > 2 || quint8(m_bytes.at(19)) < 1 || quint8(m_bytes.at(19)) > 2) return fail("Unsupported SQLite header");
        pages = quint32(m_bytes.size() / pageSize);
        const quint32 declared = be32(m_bytes.constData() + 28);
        if (declared && be32(m_bytes.constData() + 24) == be32(m_bytes.constData() + 92)) {
            if (declared > pages) return fail("Truncated SQLite database");
            pages = declared;
        }
        encoding = be32(m_bytes.constData() + 56);
        const quint32 schema = be32(m_bytes.constData() + 44);
        if (schema < 1 || schema > 4 || encoding < 1 || encoding > 3) return fail("Unsupported SQLite schema or encoding");
        return true;
    }

    bool schema(QList<Table> *tables)
    {
        return walk(1, SchemaCollector(this, tables));
    }

    bool render(quint32 root, QByteArray *output, qint64 *total, qint64 entryLimit, qint64 totalLimit)
    {
        static const QByteArray newline = utf16LE(QStringLiteral("\r\n"));
        static const QByteArray separator = utf16LE(QStringLiteral("\r\n--------------------------------------------------\r\n\r\n"));
        const qint64 initial = *total;
        // 00679d00 omits every non-text/empty field and every text-empty row.
        const bool ok = walk(root, TextExporter(this, output, total, entryLimit, totalLimit, newline, separator));
        if (!ok) { output->clear(); *total = initial; }
        return ok;
    }

private:
    bool alive() { return XBinary::isPdStructNotCanceled(m_pd); }
    bool fail(const char *message) { error = QString::fromLatin1(message); return false; }
    bool page(quint32 number, QByteArray *bytes)
    {
        if (!alive()) return fail("SQLite export canceled");
        if (!number || number > pages || visited.contains(number)) return fail("Invalid SQLite page chain");
        if (++pageReads > 1000000) return fail("SQLite page traversal limit exceeded");
        visited.insert(number);
        const qint64 offset = qint64(number - 1) * pageSize;
        *bytes = m_bytes.mid(offset, pageSize);
        return bytes->size() == pageSize;
    }
    bool text(const QByteArray &bytes, QString *result)
    {
        if (encoding == 1) {
            // A SQL value is a string, not a text-file stream: preserve any
            // initial U+FEFF that Qt would otherwise consume as a BOM.
            QByteArray framed(1, 'x');
            framed.append(bytes);
            *result = QString::fromUtf8(framed).mid(1);
            return result->toUtf8() == bytes;  // Refuse malformed encodings instead of manufacturing replacement text.
        }
        if (bytes.size() & 1) return fail("Invalid SQLite UTF-16 text length");
        result->resize(bytes.size() / 2);
        for (qint32 i = 0; i < result->size(); ++i) {
            const uchar *p = reinterpret_cast<const uchar *>(bytes.constData() + i * 2);
            (*result)[i] = QChar(encoding == 2 ? qFromLittleEndian<quint16>(p) : qFromBigEndian<quint16>(p));
        }
        return true;  // Preserve stored UTF-16 code units, including embedded NULs.
    }
    bool record(const QByteArray &payload, QList<Field> *fields)
    {
        qint32 cursor = 0;
        quint64 headerSize = 0;
        if (!varint(payload, &cursor, payload.size(), &headerSize) || headerSize < quint64(cursor) || headerSize > quint64(payload.size())) return fail("Invalid SQLite record header");
        QList<quint64> serials;
        while (cursor < qint64(headerSize)) {
            quint64 serial = 0;
            if (serials.size() >= MAX_COLUMNS || !varint(payload, &cursor, qint32(headerSize), &serial) || serial == 10 || serial == 11) return fail("Unsupported SQLite record serial type");
            serials.append(serial);
        }
        for (quint64 serial : serials) {
            const qint64 fixed[] = {0, 1, 2, 3, 4, 6, 8, 8, 0, 0};
            const quint64 length = serial < 10 ? quint64(fixed[serial]) : (serial - 12) / 2;
            if (length > quint64(payload.size() - cursor)) return fail("Truncated SQLite record");
            Field field;
            field.serial = serial;
            if (serial >= 13 && (serial & 1)) {
                if (!text(payload.mid(cursor, qint32(length)), &field.text)) return fail("Invalid SQLite text encoding");
            } else if (serial == 9) field.integer = 1;
            else if (serial >= 1 && serial <= 6) {
                // Only schema root page numbers use the integer value; text export skips numbers.
                for (quint64 i = 0; i < length; ++i) field.integer = (field.integer << 8) | quint8(payload.at(cursor + qint32(i)));
                if (length < 8 && (quint8(payload.at(cursor)) & 0x80U)) field.integer |= ~quint64(0) << (length * 8);
            }
            fields->append(field);
            cursor += qint32(length);
        }
        return cursor == payload.size() || fail("SQLite record payload mismatch");
    }
    // Row callbacks are file-local functors holding the state the walk needs.
    struct SchemaCollector {
        SchemaCollector(Reader *reader, QList<Table> *tables) : m_reader(reader), m_tables(tables) {}
        bool operator()(const QList<Field> &fields) const
        {
            if (fields.size() != 5) return m_reader->fail("Invalid SQLite schema record");
            if (fields.at(0).text != "table") return true;
            const Field &name = fields.at(1), &root = fields.at(3);
            if (name.serial < 13 || !(name.serial & 1) || name.text.isEmpty() || name.text.size() > 65536 ||
                root.serial < 1 || root.serial > 9 || root.serial == 7 || root.integer > m_reader->pages || m_tables->size() >= MAX_TABLES) return m_reader->fail("Invalid SQLite table metadata");
            Table table;
            table.name = name.text;
            table.root = quint32(root.integer);
            m_tables->append(table);
            return true;
        }
        Reader *m_reader;
        QList<Table> *m_tables;
    };
    struct TextExporter {
        TextExporter(Reader *reader, QByteArray *output, qint64 *total, qint64 entryLimit, qint64 totalLimit, const QByteArray &newline, const QByteArray &separator)
            : m_reader(reader), m_output(output), m_total(total), m_entryLimit(entryLimit), m_totalLimit(totalLimit), m_newline(newline), m_separator(separator) {}
        bool fits(qint64 size) const
        {
            if (size > m_entryLimit - m_output->size() || size > m_totalLimit - *m_total) {
                m_reader->outputLimitHit = true;
                return m_reader->fail("SQLite text export exceeds the configured limit");
            }
            return true;
        }
        bool append(const QByteArray &bytes) const
        {
            if (!fits(bytes.size())) return false;
            m_output->append(bytes);
            *m_total += bytes.size();
            return true;
        }
        bool operator()(const QList<Field> &fields) const
        {
            bool wrote = false;
            for (const Field &field : fields) {
                if (field.serial >= 13 && (field.serial & 1) && !field.text.isEmpty()) {
                    if (!fits(qint64(field.text.size()) * 2) || !append(utf16LE(field.text)) || !append(m_newline)) return false;
                    wrote = true;
                }
            }
            return !wrote || append(m_separator);
        }
        Reader *m_reader;
        QByteArray *m_output;
        qint64 *m_total;
        qint64 m_entryLimit;
        qint64 m_totalLimit;
        const QByteArray &m_newline;
        const QByteArray &m_separator;
    };
    bool walk(quint32 root, const std::function<bool(const QList<Field> &)> &callback)
    {
        visited.clear();
        bool haveRow = false;
        quint64 lastKey = 0;
        if (!root) return fail("SQLite virtual table has no stored table B-tree");
        return tree(root, 0, &haveRow, &lastKey, callback);
    }
    bool tree(quint32 number, qint32 depth, bool *haveRow, quint64 *lastKey,
              const std::function<bool(const QList<Field> &)> &callback)
    {
        if (depth >= 64) return fail("SQLite B-tree depth limit exceeded");
        QByteArray bytes;
        if (!page(number, &bytes)) return false;
        const qint32 start = number == 1 ? 100 : 0;
        const quint8 type = quint8(bytes.at(start));
        if (type != 5 && type != 13) return fail("SQLite WITHOUT ROWID/index B-tree is not supported by the text export");
        const qint32 cells = be16(bytes.constData() + start + 3);
        const qint32 pointers = start + (type == 5 ? 12 : 8);
        if (cells > (usable - pointers) / 2) return fail("Invalid SQLite cell pointer area");
        qint32 contentStart = be16(bytes.constData() + start + 5);
        if (!contentStart) contentStart = 65536;
        if (contentStart < pointers + cells * 2 || contentStart > usable) return fail("Invalid SQLite cell content area");
        QList<QPair<qint32, qint32>> extents;
        for (qint32 i = 0; i < cells; ++i) {
            if (!alive()) return fail("SQLite export canceled");
            qint32 cursor = be16(bytes.constData() + pointers + i * 2);
            const qint32 cellStart = cursor;
            if (cursor < contentStart || cursor >= usable) return fail("Invalid SQLite cell offset");
            if (type == 5) {
                if (usable - cursor < 4) return fail("Truncated SQLite interior cell");
                const quint32 child = be32(bytes.constData() + cursor);
                cursor += 4;
                quint64 key = 0;
                if (!varint(bytes, &cursor, usable, &key) || !tree(child, depth + 1, haveRow, lastKey, callback)) return false;
            } else {
                quint64 payloadSize = 0, key = 0;
                if (!varint(bytes, &cursor, usable, &payloadSize) || !varint(bytes, &cursor, usable, &key) || payloadSize > MAX_PAYLOAD) return fail("Invalid SQLite leaf payload");
                const quint64 orderKey = key ^ (quint64(1) << 63);  // Signed rowid order without narrowing.
                if (*haveRow && orderKey <= *lastKey) return fail("SQLite rowids are out of order");
                *haveRow = true; *lastKey = orderKey;
                if (++rows > MAX_ROWS || payloadSize > quint64(MAX_PARSE_BYTES - parsedBytes)) return fail("SQLite record traversal limit exceeded");
                parsedBytes += qint64(payloadSize);
                qint64 local = qint64(payloadSize);
                if (local > usable - 35) {
                    const qint64 minimum = ((usable - 12) * 32 / 255) - 23;
                    local = minimum + ((qint64(payloadSize) - minimum) % (usable - 4));
                    if (local > usable - 35) local = minimum;
                }
                const bool overflow = local < qint64(payloadSize);
                if (local > usable - cursor - (overflow ? 4 : 0)) return fail("Truncated SQLite local payload");
                QByteArray payload = bytes.mid(cursor, local);
                cursor += qint32(local);
                if (overflow) {
                    quint32 next = be32(bytes.constData() + cursor);
                    cursor += 4;
                    while (payload.size() < qint64(payloadSize)) {
                        QByteArray block;
                        if (!page(next, &block)) return false;
                        next = be32(block.constData());
                        const qint32 amount = qint32(qMin(qint64(payloadSize) - payload.size(), qint64(usable - 4)));
                        payload.append(block.constData() + 4, amount);
                    }
                    if (next) return fail("SQLite overflow chain has extra pages");
                }
                QList<Field> fields;
                if (!record(payload, &fields) || !callback(fields)) return false;
            }
            extents.append(qMakePair(cellStart, cursor));
        }
        std::sort(extents.begin(), extents.end());
        for (qint32 i = 1; i < extents.size(); ++i) if (extents.at(i).first < extents.at(i - 1).second) return fail("Overlapping SQLite cells");
        return type != 5 || tree(be32(bytes.constData() + start + 8), depth + 1, haveRow, lastKey, callback);
    }
    const QByteArray &m_bytes;
    XBinary::PDSTRUCT *m_pd;
    qint32 pageSize = 0, usable = 0;
    quint32 pages = 0, encoding = 0;
    QSet<quint32> visited;
    qint64 pageReads = 0, rows = 0, parsedBytes = 0;
};
}  // namespace

XSQLiteArchive::XSQLiteArchive(QIODevice *pDevice) : XArchive(pDevice) {}

bool XSQLiteArchive::readSource(QIODevice *pDevice, QByteArray *pBytes, PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> source(pDevice);
    if (!source || !pBytes || !source->isOpen() || !source->isReadable() || source->isSequential()) return false;
    const qint64 position = source->pos();
    if (!source || position < 0) return false;
    const qint64 size = source->size();
    if (!source || size < 512 || size > MAX_SOURCE) return false;
    // Archive probes share this route: reject unrelated inputs after only
    // the signature, before materializing a candidate database.
    const QByteArray signature = XBinary::read_array_process(source.data(), 0, 16, pPdStruct);
    if (!source) return false;
    if (signature != QByteArray("SQLite format 3\0", 16) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        source->seek(position);
        return false;
    }
    *pBytes = XBinary::read_array_process(source.data(), 0, size, pPdStruct);
    const bool restored = source && source->seek(position);
    return restored && source && pBytes->size() == size && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XSQLiteArchive::parse(const QByteArray &bytes, QList<ITEM> *pItems, bool render, PDSTRUCT *pPdStruct, const OUTPUT_POLICY *pPolicy)
{
    Reader reader(bytes, pPdStruct);
    QList<Table> tables;
    if (!reader.header() || !reader.schema(&tables)) return false;
    qint64 entryLimit = MAX_EXPORT, totalLimit = MAX_EXPORT;
    if (render) {
        if (!pPolicy) return false;
        if (pPolicy->nMaxEntryCount >= 0 && tables.size() > pPolicy->nMaxEntryCount) {
            setPdStructErrorString(pPdStruct, tr("SQLite table count exceeds the configured limit"));
            return false;
        }
        if (pPolicy->nMaxEntryOutputSize >= 0) entryLimit = qMin(entryLimit, pPolicy->nMaxEntryOutputSize);
        if (pPolicy->nMaxTotalOutputSize >= 0) totalLimit = qMin(totalLimit, pPolicy->nMaxTotalOutputSize);
        // Retained table exports are one in-memory route, even for folder output.
        if (pPolicy->nMaxMemoryOutputSize >= 0) totalLimit = qMin(totalLimit, pPolicy->nMaxMemoryOutputSize);
    }
    QSet<QString> names;
    qint64 total = 0;
    for (const Table &table : tables) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        ITEM item;
        item.table = table.name;
        item.name = u3TableName(table.name);
        item.root = table.root;
        if (names.contains(item.name.toCaseFolded())) return false;
        names.insert(item.name.toCaseFolded());
        if (render && !reader.render(table.root, &item.bytes, &total, entryLimit, totalLimit)) {
            if (reader.outputLimitHit) { setPdStructErrorString(pPdStruct, reader.error); return false; }
            item.error = reader.error.isEmpty() ? QStringLiteral("SQLite table export failed") : reader.error;
        }
        pItems->append(item);
    }
    return true;
}

bool XSQLiteArchive::isValid(PDSTRUCT *pPdStruct) { return isValid(getDevice(), pPdStruct); }
bool XSQLiteArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QByteArray bytes;
    QList<ITEM> items;
    return readSource(pDevice, &bytes, pPdStruct) && parse(bytes, &items, false, pPdStruct);
}
XBinary::FT XSQLiteArchive::getFileType() { return FT_SQLITE; }
XBinary::MODE XSQLiteArchive::getMode() { return MODE_DATA; }
qint32 XSQLiteArchive::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XSQLiteArchive::getEndian() { return ENDIAN_BIG; }
QString XSQLiteArchive::getFileFormatExt() { return QStringLiteral("sqlite"); }
QString XSQLiteArchive::getFileFormatExtsString() { return QStringLiteral("SQLite 3 (*.sqlite *.sqlite3 *.db)"); }
QString XSQLiteArchive::getMIMEString() { return QStringLiteral("application/vnd.sqlite3"); }
qint64 XSQLiteArchive::getFileFormatSize(PDSTRUCT *pPdStruct) { return isValid(pPdStruct) ? getSize() : 0; }
XBinary::OSNAME XSQLiteArchive::getOsName() { return OSNAME_MULTIPLATFORM; }
QString XSQLiteArchive::getVersion() { return QStringLiteral("3"); }
QList<QString> XSQLiteArchive::getSearchSignatures() { return {QStringLiteral("'SQLite format 3'00")}; }
XBinary *XSQLiteArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSQLiteArchive(pDevice);
}

bool XSQLiteArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pPdStruct)
{
    QPointer<XSQLiteArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(properties, &policy)) return false;
    CONTEXT *old = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    *pState = UNPACK_STATE();
    delete old;
    if (!self) return false;
    const bool bound = bindUnpackSource(pState, pPdStruct);
    if (!self || !bound) { *pState = UNPACK_STATE(); return false; }
    std::unique_ptr<CONTEXT> context(new (std::nothrow) CONTEXT);
    QByteArray bytes;
    if (!context || !readSource(getDevice(), &bytes, pPdStruct) || !self || !parse(bytes, &context->items, true, pPdStruct, &policy)) {
        if (self) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    pState->pContext = context.get();
    pState->nTotalSize = bytes.size();
    pState->nNumberOfRecords = context->items.size();
    pState->mapUnpackProperties = properties;
    const bool finalized = validateAndFinalizeUnpackSource(pState, context.get(), pPdStruct);
    if (!self) { context.release(); *pState = UNPACK_STATE(); return false; }
    if (!finalized) { releaseUnpackSource(pState); *pState = UNPACK_STATE(); return false; }
    context.release();
    return true;
}

XBinary::ARCHIVERECORD XSQLiteArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSQLiteArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!guard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !self ||
        pState->nCurrentIndex < 0 || pState->nCurrentIndex >= pState->nNumberOfRecords) return ARCHIVERECORD();
    const CONTEXT *context = static_cast<const CONTEXT *>(pState->pContext);
    if (context->items.size() != pState->nNumberOfRecords) return ARCHIVERECORD();
    const ITEM &item = context->items.at(pState->nCurrentIndex);
    ARCHIVERECORD result = {};
    result.nStreamOffset = item.root;
    result.nStreamSize = pState->nTotalSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, item.name);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, qint64(item.bytes.size()));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("SQLite table text (UTF-16LE)"));
    result.mapProperties.insert(FPART_PROP_TYPE, item.error.isEmpty() ? QStringLiteral("Table text export") : item.error);
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XSQLiteArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XSQLiteArchive> self(this);
    QPointer<QIODevice> destination(pDevice), source(getDevice());
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || !pState->pContext || !destination || !source ||
        !isUnpackOutputSupported(destination.data()) || !self || !destination || !source ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !self || !destination || !source ||
        pState->nCurrentIndex < 0 || pState->nCurrentIndex >= pState->nNumberOfRecords || devicesAlias(source.data(), destination.data())) return false;
    const CONTEXT *context = static_cast<const CONTEXT *>(pState->pContext);
    if (context->items.size() != pState->nNumberOfRecords) return false;
    const ITEM item = context->items.at(pState->nCurrentIndex);
    if (!item.error.isEmpty()) { setPdStructErrorString(pPdStruct, item.error); return false; }
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, item.bytes.size())) {
        setPdStructErrorString(pPdStruct, tr("SQLite text output exceeds the configured limit"));
        return false;
    }
    DATAPROCESS_STATE output = {};
    output.nProcessedLimit = -1;
    output.mapUnpackProperties = pState->mapUnpackProperties;
    output.spOutputBudget = pState->spOutputBudget;
    if (output.spOutputBudget && !output.spOutputBudget->beginEntry(pState->nCurrentIndex, item.name)) {
        if (output.spOutputBudget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(output.spOutputBudget.data());
    }
    std::unique_ptr<QIODevice> stage(createFileBuffer(item.bytes.size(), pPdStruct));
    if (!stage || !self || !destination || !source) return false;
    output.pDeviceOutput = stage.get();
    qint64 offset = 0;
    while (offset < item.bytes.size()) {
        if (!self || !source || !destination || !isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 amount = qint32(qMin(qint64(65536), item.bytes.size() - offset));
        if (_writeDevice(item.bytes.constData() + offset, amount, &output) != amount) return false;
        offset += amount;
    }
    if (output.bWriteError || !self || !source || !destination || !isUnpackSourceCurrent(pState, pPdStruct) || !self || !source || !destination ||
        !isPdStructNotCanceled(pPdStruct)) return false;
    return publishUnpackOutput(stage.get(), destination.data(), pState, pPdStruct);
}

bool XSQLiteArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSQLiteArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !self ||
        pState->nCurrentIndex < 0 || pState->nCurrentIndex >= pState->nNumberOfRecords) return false;
    if (static_cast<const CONTEXT *>(pState->pContext)->items.size() != pState->nNumberOfRecords) return false;
    ++pState->nCurrentIndex;
    return pState->nCurrentIndex < pState->nNumberOfRecords;
}
bool XSQLiteArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *context = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    *pState = UNPACK_STATE();
    delete context;
    return true;
}
QList<XBinary::FPART_PROP> XSQLiteArchive::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD, FPART_PROP_TYPE};
}
