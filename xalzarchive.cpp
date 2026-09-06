/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#include "xalzarchive.h"
#include <limits>
#include <memory>
#include <new>

namespace {
class Reader {
public:
    Reader(QIODevice *device, XBinary::PDSTRUCT *pd) : d(device), p(pd), saved(device ? device->pos() : -1), size(device ? device->size() : -1) {}
    ~Reader() { if (d && saved >= 0) d->seek(saved); }
    bool read(qint64 offset, qint64 count, QByteArray *bytes) {
        if (!d || !XBinary::isPdStructNotCanceled(p) || offset < 0 || count < 0 || offset > size || count > size - offset ||
            count > 65535 || !d->seek(offset) || !d) return false;
        bytes->resize(qsizetype(count));
        qint64 done = 0;
        while (done < count && d && XBinary::isPdStructNotCanceled(p)) {
            const qint64 n = d->read(bytes->data() + done, count - done);
            if (!d || n <= 0 || n > count - done) return false;
            done += n;
        }
        return d && done == count && XBinary::isPdStructNotCanceled(p);
    }
    QPointer<QIODevice> d;
    XBinary::PDSTRUCT *p;
    qint64 saved, size;
};
quint64 le(const QByteArray &bytes, qsizetype offset, int count)
{
    quint64 value = 0;
    for (int i = 0; i < count; ++i) value |= quint64(uchar(bytes.at(offset + i))) << (8 * i);
    return value;
}
QString component(QString name)
{
    if (name.endsWith('\r')) name.chop(1);
    while (name.endsWith(' ') || name.endsWith('.')) name.chop(1);
    if (name.isEmpty()) return QStringLiteral("_");
    name = name.left(255);
    const QString forbidden = QStringLiteral("\"*/:<>?\\|");
    for (qint32 i = 0; i < name.size(); ++i) if (name.at(i).unicode() < 32 || forbidden.contains(name.at(i))) name[i] = QLatin1Char('_');
    const QString stem = name.section('.', 0, 0).toUpper();
    if (stem == "AUX" || stem == "CON" || stem == "NUL" || stem == "PRN" || stem == "CLOCK$" ||
        (stem.size() == 4 && (stem.startsWith("COM") || stem.startsWith("LPT")) && stem.at(3) >= '1' && stem.at(3) <= '9')) name.prepend('_');
    return name;
}
// U3 004241e0 -> 00423f00: separator normalization, intermediate dot components,
// then the same per-component Windows filename normalization as 00424220.
QString pathName(QString name)
{
    name.replace('\\', '/');
    QStringList parts = name.split('/', Qt::SkipEmptyParts), result;
    for (qsizetype i = 0; i < parts.size(); ++i) {
        if (i + 1 < parts.size() && parts.at(i) == ".") continue;
        if (i + 1 < parts.size() && parts.at(i) == "..") { if (!result.isEmpty()) result.removeLast(); continue; }
        result.append(component(parts.at(i)));
    }
    return result.isEmpty() ? QStringLiteral("_") : result.join('/');
}
}

XAlzArchive::XAlzArchive(QIODevice *device) : XArchive(device) {}

bool XAlzArchive::parse(QIODevice *device, QList<ARCHIVERECORD> *items, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pd)
{
    Reader r(device, pd);
    if (!r.d || r.d->isSequential() || !r.d->isReadable()) return false;
    QByteArray bytes;
    if (!r.read(0, 12, &bytes) || bytes.left(4) != QByteArray("ALZ\1", 4) || bytes.mid(8, 4) != QByteArray("BLZ\1", 4)) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(properties, &policy)) return false;
    qint64 offset = 8;
    while (offset < r.size) {
        if (!r.read(offset, 4, &bytes)) return false;
        const quint32 magic = quint32(le(bytes, 0, 4));
        offset += 4;
        if (magic == 0x015a4c43) { if (!r.read(offset, 8, &bytes)) return false; offset += 8; continue; }
        if (magic == 0x025a4c43) continue;
        if (magic != 0x015a4c42 || !r.read(offset, 9, &bytes)) return false;
        const qint64 headerOffset = offset - 4;
        const int nameSize = int(le(bytes, 0, 2));
        const quint8 attributes = quint8(bytes.at(2));
        const quint32 dosTime = quint32(le(bytes, 3, 4));
        const quint8 descriptor = quint8(bytes.at(7));
        const int width = descriptor >> 4;
        offset += 9;
        if (width > 8 || (policy.nMaxEntryCount >= 0 && items->size() >= policy.nMaxEntryCount) || items->size() >= std::numeric_limits<qint32>::max()) return false;
        quint8 method = 0;
        quint32 crc = 0;
        quint64 packed = 0, unpacked = 0;
        if (width) {
            if (!r.read(offset, 6 + 2 * width, &bytes)) return false;
            method = quint8(bytes.at(0)); crc = quint32(le(bytes, 2, 4));
            packed = le(bytes, 6, width); unpacked = le(bytes, 6 + width, width);
            offset += 6 + 2 * width;
            if (packed > quint64(std::numeric_limits<qint64>::max()) || unpacked > quint64(std::numeric_limits<qint64>::max())) return false;
        }
        if (!r.read(offset, nameSize, &bytes)) return false;
        offset += nameSize;
        if (packed > quint64(r.size - offset) || ((descriptor & 1) && width && packed < 12)) return false;
        // U3's ANSI conversion uses the Windows system code page; allow the caller's
        // existing code-page override through the common Formats conversion API.
        QBuffer nameBuffer(&bytes);
        if (!nameBuffer.open(QIODevice::ReadOnly)) return false;
        XBinary nameReader(&nameBuffer);
        const QString name = pathName(nameReader.read_codePageString(0, bytes.size(), properties.value(UNPACK_PROP_CODEPAGE, "System").toString()));
        ARCHIVERECORD record = {};
        record.nStreamOffset = offset; record.nStreamSize = qint64(packed);
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, name);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, qint64(unpacked));
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, qint64(packed));
        record.mapProperties.insert(FPART_PROP_ISFOLDER, width == 0);
        record.mapProperties.insert(FPART_PROP_ENCRYPTED, (descriptor & 1) != 0);
        record.mapProperties.insert(FPART_PROP_FLAGS, descriptor);
        record.mapProperties.insert(FPART_PROP_HEADER_OFFSET, headerOffset);
        record.mapProperties.insert(FPART_PROP_HEADER_SIZE, offset - headerOffset);
        record.mapProperties.insert(FPART_PROP_DATETIME, dosDateTimeToQDateTime(quint16(dosTime >> 16), quint16(dosTime)));
        record.mapProperties.insert(FPART_PROP_ISREADONLY, (attributes & 1) != 0);
        record.mapProperties.insert(FPART_PROP_ISHIDDEN, (attributes & 2) != 0);
        record.mapProperties.insert(FPART_PROP_ISSYSTEM, (attributes & 4) != 0);
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("ALZ method %1").arg(method));
        const HANDLE_METHOD handle = (descriptor & 1) ? HANDLE_METHOD_UNKNOWN : method == 0 ? HANDLE_METHOD_STORE : method == 2 ? HANDLE_METHOD_DEFLATE : HANDLE_METHOD_UNKNOWN;
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handle);
        if (width) {
            record.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
            record.mapProperties.insert(FPART_PROP_RESULTCRC, crc);
        }
        items->append(record);
        offset += qint64(packed);
    }
    return !items->isEmpty() && r.d && isPdStructNotCanceled(pd);
}

bool XAlzArchive::isValid(PDSTRUCT *pd) { return isValid(getDevice(), pd); }
bool XAlzArchive::isValid(QIODevice *device, PDSTRUCT *pd) { QList<ARCHIVERECORD> items; return parse(device, &items, {}, pd); }
XBinary::FT XAlzArchive::getFileType() { return FT_ALZ; }
XBinary::MODE XAlzArchive::getMode() { return MODE_DATA; }
qint32 XAlzArchive::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XAlzArchive::getEndian() { return ENDIAN_LITTLE; }
QString XAlzArchive::getFileFormatExt() { return QStringLiteral("alz"); }
QString XAlzArchive::getFileFormatExtsString() { return QStringLiteral("ALZ (*.alz)"); }
QString XAlzArchive::getMIMEString() { return QStringLiteral("application/x-alz"); }
qint64 XAlzArchive::getFileFormatSize(PDSTRUCT *pd) { return isValid(pd) ? getSize() : 0; }
XBinary::OSNAME XAlzArchive::getOsName() { return OSNAME_WINDOWS; }
QString XAlzArchive::getVersion() { return QString(); }
QList<QString> XAlzArchive::getSearchSignatures() { return {QStringLiteral("'ALZ'01")}; }
XBinary *XAlzArchive::createInstance(QIODevice *device, bool image, XADDR address)
{ Q_UNUSED(image) Q_UNUSED(address) return new XAlzArchive(device); }

bool XAlzArchive::initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pd)
{
    QPointer<XAlzArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *old = static_cast<CONTEXT *>(state->pContext);
    releaseUnpackSource(state); *state = UNPACK_STATE(); delete old;
    if (!self || !bindUnpackSource(state, pd) || !self) return false;
    std::unique_ptr<CONTEXT> context(new (std::nothrow) CONTEXT);
    if (!context || !parse(getDevice(), &context->items, properties, pd) || !self) {
        if (self) releaseUnpackSource(state);
        *state = UNPACK_STATE(); return false;
    }
    state->pContext = context.get(); state->nTotalSize = getSize(); state->nNumberOfRecords = qint32(context->items.size()); state->mapUnpackProperties = properties;
    const bool finalized = validateAndFinalizeUnpackSource(state, context.get(), pd);
    if (!self) { context.release(); *state = UNPACK_STATE(); return false; }
    if (!finalized) { releaseUnpackSource(state); *state = UNPACK_STATE(); return false; }
    context.release(); return true;
}
XBinary::ARCHIVERECORD XAlzArchive::infoCurrent(UNPACK_STATE *state, PDSTRUCT *pd)
{
    QPointer<XAlzArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!guard.isAllowed() || !state || !state->pContext || !isUnpackSourceCurrent(state, pd) || !self ||
        state->nCurrentIndex < 0 || state->nCurrentIndex >= state->nNumberOfRecords) return {};
    const CONTEXT *context = static_cast<const CONTEXT *>(state->pContext);
    if (context->items.size() != state->nNumberOfRecords) return {};
    return context->items.at(state->nCurrentIndex);
}
bool XAlzArchive::unpackCurrent(UNPACK_STATE *state, QIODevice *device, PDSTRUCT *pd)
{
    QPointer<XAlzArchive> self(this);
    const ARCHIVERECORD record = infoCurrent(state, pd);
    if (!self || record.mapProperties.isEmpty()) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(state->mapUnpackProperties, &policy) ||
        (dynamic_cast<QBuffer *>(device) && policy.nMaxMemoryOutputSize >= 0 &&
         record.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong() > policy.nMaxMemoryOutputSize)) {
        setPdStructErrorString(pd, tr("ALZ entry exceeds the configured memory output limit")); return false;
    }
    if (record.mapProperties.value(FPART_PROP_ENCRYPTED).toBool()) {
        setPdStructErrorString(pd, tr("Encrypted ALZ entries are not supported")); return false;
    }
    if (record.mapProperties.value(FPART_PROP_HANDLEMETHOD).toInt() == HANDLE_METHOD_UNKNOWN) {
        setPdStructErrorString(pd, tr("Unsupported ALZ compression method")); return false;
    }
    // U3 004c1980 supports STORE and raw DEFLATE with CRC-32; method 1 is unsupported.
    return XArchive::unpackCurrent(state, device, pd);
}
bool XAlzArchive::moveToNext(UNPACK_STATE *state, PDSTRUCT *pd)
{
    QPointer<XAlzArchive> self(this);
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || !state->pContext || !isUnpackSourceCurrent(state, pd) || !self || state->nCurrentIndex < 0 ||
        state->nCurrentIndex >= state->nNumberOfRecords || static_cast<CONTEXT *>(state->pContext)->items.size() != state->nNumberOfRecords) return false;
    ++state->nCurrentIndex; return state->nCurrentIndex < state->nNumberOfRecords;
}
bool XAlzArchive::finishUnpack(UNPACK_STATE *state, PDSTRUCT *pd)
{
    Q_UNUSED(pd)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *context = static_cast<CONTEXT *>(state->pContext);
    releaseUnpackSource(state); *state = UNPACK_STATE(); delete context; return true;
}
QList<XBinary::FPART_PROP> XAlzArchive::getAvailableFPARTProperties()
{ return {FPART_PROP_ORIGINALNAME, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD,
          FPART_PROP_DATETIME, FPART_PROP_ENCRYPTED, FPART_PROP_ISFOLDER, FPART_PROP_RESULTCRC}; }
