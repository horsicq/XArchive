/* Copyright (c) 2026 hors<horsicq@gmail.com>
 * MIT License
 */
#include "xgitobjectarchive.h"
#include "Algos/include/zlib.h"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QTemporaryFile>
#include <limits>
#include <memory>
#include <new>

namespace {
class SourcePosition {
public:
    explicit SourcePosition(QIODevice *device) : d(device), position(device ? device->pos() : -1) {}
    ~SourcePosition() { if (d && position >= 0) d->seek(position); }
    QIODevice *d = nullptr;
    qint64 position;
};
struct Inflater {
    z_stream stream = {};
    bool ready = false;
    Inflater() { ready = inflateInit(&stream) == Z_OK; }
    ~Inflater() { if (ready) inflateEnd(&stream); }
};

// decimal length terminated by NUL, at most twenty digits.
bool blobHeader(const QByteArray &header, qint64 *size)
{
    if (header.size() < 7 || !header.startsWith("blob ") || header.back() != '\0' || header.size() > 26) return false;
    qint64 value = 0;
    for (qsizetype i = 5; i + 1 < header.size(); ++i) {
        const int digit = uchar(header.at(i)) - '0';
        if (digit < 0 || digit > 9 || value > (std::numeric_limits<qint64>::max() - digit) / 10) return false;
        value = value * 10 + digit;
    }
    *size = value;
    return true;
}

// applied to the original basename without removing its extension.
QString outputName(QString name)
{
    if (name.startsWith('/') || name.startsWith('\\')) name.remove(0, 1);
    if (name.endsWith('/') || name.endsWith('\\')) name.chop(1);
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
}

XGitObjectArchive::XGitObjectArchive(QIODevice *device) : XArchive(device) {}

bool XGitObjectArchive::probe(QIODevice *device, qint64 *size, PDSTRUCT *pd, bool *raw)
{
    if (raw) *raw = false;
    SourcePosition source(device);
    if (!source.d || source.d->isSequential() || !source.d->isReadable() || !source.d->seek(0) || !isPdStructNotCanceled(pd)) return false;
    // A loose object is normally zlib-wrapped, but the same "blob <n>\0<data>"
    // payload also turns up already inflated; accept that shape too, otherwise
    // every such file is reported as plain binary.
    {
        QByteArray prefix(26, Qt::Uninitialized);
        const qint64 n = source.d->read(prefix.data(), prefix.size());
        if (!source.d) return false;
        if (n > 5) {
            prefix.truncate(qint32(n));
            const qint32 zero = prefix.indexOf('\0');
            qint64 declared = 0;
            if (zero > 0 && blobHeader(prefix.left(zero + 1), &declared) && (source.d->size() - (zero + 1)) == declared) {
                *size = declared;
                if (raw) *raw = true;
                return true;
            }
        }
        if (!source.d || !source.d->seek(0)) return false;
    }
    Inflater z;
    if (!z.ready) return false;
    QByteArray input(4096, Qt::Uninitialized), header;
    qint64 read = 0;
    while (header.size() < 26 && source.d && isPdStructNotCanceled(pd)) {
        if (z.stream.avail_in == 0) {
            if (read >= 65536) return false;
            const qint64 n = source.d->read(input.data(), input.size());
            if (!source.d || n <= 0) return false;
            read += n;
            z.stream.next_in = reinterpret_cast<Bytef *>(input.data());
            z.stream.avail_in = uInt(n);
        }
        char byte = 0;
        z.stream.next_out = reinterpret_cast<Bytef *>(&byte);
        z.stream.avail_out = 1;
        const uInt before = z.stream.avail_in;
        const int status = inflate(&z.stream, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END) return false;
        if (z.stream.avail_out == 0) {
            header.append(byte);
            if (byte == '\0') return blobHeader(header, size);
            if (header.size() <= 5 && header != QByteArray("blob ").left(header.size())) return false;
        }
        if (status == Z_STREAM_END || (z.stream.avail_out && before == z.stream.avail_in)) return false;
    }
    return false;
}

bool XGitObjectArchive::isValid(PDSTRUCT *pd) { return isValid(getDevice(), pd); }
bool XGitObjectArchive::isValid(QIODevice *device, PDSTRUCT *pd) { qint64 size = 0; return probe(device, &size, pd); }
XBinary::FT XGitObjectArchive::getFileType() { return FT_GIT_OBJECT; }
XBinary::MODE XGitObjectArchive::getMode() { return MODE_DATA; }
qint32 XGitObjectArchive::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XGitObjectArchive::getEndian() { return ENDIAN_UNKNOWN; }
QString XGitObjectArchive::getFileFormatExt() { return QStringLiteral("gitobject"); }
QString XGitObjectArchive::getFileFormatExtsString() { return QStringLiteral("Git Object (*)"); }
QString XGitObjectArchive::getMIMEString() { return QStringLiteral("application/octet-stream"); }
qint64 XGitObjectArchive::getFileFormatSize(PDSTRUCT *pd) { return isValid(pd) ? getSize() : 0; }
XBinary::OSNAME XGitObjectArchive::getOsName() { return OSNAME_MULTIPLATFORM; }
QString XGitObjectArchive::getVersion() { return QString(); }
QList<QString> XGitObjectArchive::getSearchSignatures() { return {}; }
XBinary *XGitObjectArchive::createInstance(QIODevice *device, bool image, XADDR address)
{ Q_UNUSED(image) Q_UNUSED(address) return new XGitObjectArchive(device); }

bool XGitObjectArchive::initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP, QVariant> &properties, PDSTRUCT *pd)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    OUTPUT_POLICY policy = {};
    if (!guard.isAcquired() || !state || !resolveUnpackOutputPolicy(properties, &policy) ||
        ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *old = static_cast<CONTEXT *>(state->pContext);
    releaseUnpackSource(state); *state = UNPACK_STATE(); delete old;
    if (!bindUnpackSource(state, pd)) return false;
    std::unique_ptr<CONTEXT> context(new (std::nothrow) CONTEXT);
    if (!context || !probe(getDevice(), &context->size, pd, &context->raw)) {
        releaseUnpackSource(state);
        *state = UNPACK_STATE(); return false;
    }
    const QString name = QFileInfo(getDeviceFileName(getDevice())).fileName();
    context->name = outputName(name);
    if (name.size() == 38) {
        bool hex = true;
        for (QChar ch : name) if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'))) hex = false;
        if (hex) context->expectedHash = QByteArray::fromHex(name.toLatin1());
    }
    state->pContext = context.get(); state->nTotalSize = getSize(); state->nNumberOfRecords = 1; state->mapUnpackProperties = properties;
    const bool finalized = validateAndFinalizeUnpackSource(state, context.get(), pd);
    if (!finalized) { releaseUnpackSource(state); *state = UNPACK_STATE(); return false; }
    context.release(); return true;
}

XBinary::ARCHIVERECORD XGitObjectArchive::infoCurrent(UNPACK_STATE *state, PDSTRUCT *pd)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!guard.isAllowed() || !state || !state->pContext || !isUnpackSourceCurrent(state, pd) || state->nNumberOfRecords != 1 || state->nCurrentIndex != 0) return {};
    const CONTEXT *context = static_cast<const CONTEXT *>(state->pContext);
    ARCHIVERECORD record = {};
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context->name);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context->size);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, state->nTotalSize);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Git blob (zlib)"));
    markArchiveStreamRecord(&record, 0);
    return record;
}

bool XGitObjectArchive::unpackCurrent(UNPACK_STATE *state, QIODevice *device, PDSTRUCT *pd)
{
    QIODevice *destination = device;
    QIODevice *source = getDevice();
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || !state->pContext || !destination || !source || !isUnpackOutputSupported(device) || !isUnpackSourceCurrent(state, pd) || state->nNumberOfRecords != 1 ||
        state->nCurrentIndex != 0 || devicesAlias(source, destination)) return false;
    const CONTEXT item = *static_cast<CONTEXT *>(state->pContext);
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(state->mapUnpackProperties, &policy) || !isUnpackOutputSizeAllowed(state->mapUnpackProperties, item.size) ||
        (dynamic_cast<QBuffer *>(destination) && policy.nMaxMemoryOutputSize >= 0 && item.size > policy.nMaxMemoryOutputSize)) {
        setPdStructErrorString(pd, tr("Git blob exceeds the configured output limit")); return false;
    }
    DATAPROCESS_STATE output = {};
    output.nProcessedLimit = -1; output.mapUnpackProperties = state->mapUnpackProperties; output.spOutputBudget = state->spOutputBudget;
    if (output.spOutputBudget && !output.spOutputBudget->beginEntry(0, item.name)) {
        if (output.spOutputBudget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(output.spOutputBudget.data());
    }
    QTemporaryFile stage;
    if (!stage.open()) return false;
    output.pDeviceOutput = &stage;
    SourcePosition saved(source);
    if (!source->seek(0) || !source || !destination) return false;

    // An already-inflated object: the body follows the NUL that ends the
    // header, and there is no zlib stream to run.
    if (item.raw) {
        const qint64 headerSize = source->size() - item.size;
        if (headerSize <= 0 || !source->seek(headerSize) || !source || !destination) return false;
        QByteArray chunk(65536, Qt::Uninitialized);
        qint64 produced = 0;
        while (produced < item.size && source && destination && isPdStructNotCanceled(pd)) {
            const qint64 want = qMin<qint64>(chunk.size(), item.size - produced);
            const qint64 n = source->read(chunk.data(), want);
            if (!source || !destination || n <= 0) return false;
            if (_writeDevice(chunk.constData(), qint32(n), &output) != n) return false;
            produced += n;
        }
        if (!source || !destination || produced != item.size || output.bWriteError || !isPdStructNotCanceled(pd)) return false;
        if (!isUnpackSourceCurrent(state, pd) || !source || !destination) return false;
        return publishUnpackOutput(&stage, destination, state, pd);
    }

    Inflater z;
    if (!z.ready) return false;
    QByteArray input(65536, Qt::Uninitialized), decoded(65536, Qt::Uninitialized), header;
    QCryptographicHash hash(QCryptographicHash::Sha1);
    bool headerDone = false, ended = false;
    qint64 produced = 0;
    while (!ended && source && destination && isPdStructNotCanceled(pd)) {
        if (z.stream.avail_in == 0) {
            const qint64 n = source->read(input.data(), input.size());
            if (!source || !destination || n <= 0) return false;
            z.stream.next_in = reinterpret_cast<Bytef *>(input.data()); z.stream.avail_in = uInt(n);
        }
        z.stream.next_out = reinterpret_cast<Bytef *>(decoded.data()); z.stream.avail_out = uInt(decoded.size());
        const uInt before = z.stream.avail_in;
        const int status = inflate(&z.stream, Z_NO_FLUSH);
        if (status != Z_OK && status != Z_STREAM_END) return false;
        const qint32 n = qint32(decoded.size() - z.stream.avail_out);
#if QT_VERSION_MAJOR >= 6
        hash.addData(QByteArrayView(decoded.constData(), n));
#else
        hash.addData(decoded.constData(), n);
#endif
        qint32 start = 0;
        while (!headerDone && start < n) {
            const char byte = decoded.at(start++);
            header.append(byte);
            if (header.size() > 26) return false;
            if (byte == '\0') {
                qint64 declared = 0;
                if (!blobHeader(header, &declared) || declared != item.size) return false;
                headerDone = true;
            }
        }
        const qint32 amount = n - start;
        if (amount > item.size - produced || (amount && _writeDevice(decoded.constData() + start, amount, &output) != amount)) return false;
        produced += amount;
        ended = status == Z_STREAM_END;
        if (!ended && n == 0 && before == z.stream.avail_in) return false;
    }
    if (!source || !destination || !ended || !headerDone || produced != item.size || output.bWriteError ||
        !isPdStructNotCanceled(pd)) return false;
    // The reference implementation checks the nineteen hash bytes represented by a loose object's basename.
    if (!item.expectedHash.isEmpty() && hash.result().mid(1) != item.expectedHash) {
        setPdStructErrorString(pd, tr("Git object name does not match its SHA-1")); return false;
    }
    if (!isUnpackSourceCurrent(state, pd) || !source || !destination) return false;
    return publishUnpackOutput(&stage, destination, state, pd);
}

bool XGitObjectArchive::moveToNext(UNPACK_STATE *state, PDSTRUCT *pd)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || !state->pContext || !isUnpackSourceCurrent(state, pd) || state->nNumberOfRecords != 1 || state->nCurrentIndex != 0) return false;
    ++state->nCurrentIndex; return false;
}
bool XGitObjectArchive::finishUnpack(UNPACK_STATE *state, PDSTRUCT *pd)
{
    Q_UNUSED(pd)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *context = static_cast<CONTEXT *>(state->pContext);
    releaseUnpackSource(state); *state = UNPACK_STATE(); delete context; return true;
}
QList<XBinary::FPART_PROP> XGitObjectArchive::getAvailableFPARTProperties()
{ return {FPART_PROP_ORIGINALNAME, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD, FPART_PROP_REPORTEDMETHOD}; }
