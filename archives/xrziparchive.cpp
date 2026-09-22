// The reference implementation RZIP wrapper; the private decoder stages and checks complete two-stream chunks.
#include "xrziparchive.h"

#include <QFileInfo>
#include <QTemporaryFile>
#include <new>

XRzipArchive::XRzipArchive(QIODevice *device) : XArchive(device) {}
XBinary::FT XRzipArchive::getFileType() { return FT_RZIP; }
XBinary::MODE XRzipArchive::getMode() { return MODE_DATA; }
qint32 XRzipArchive::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XRzipArchive::getEndian() { return ENDIAN_LITTLE; }
QString XRzipArchive::getArch() { return QString(); }
QString XRzipArchive::getVersion()
{
    XRzipDecoder::HEADER header;
    return readHeader(&header, nullptr) ? QStringLiteral("%1.%2").arg(header.major).arg(header.minor) : QString();
}
QString XRzipArchive::getFileFormatExt() { return QStringLiteral("rz"); }
QString XRzipArchive::getFileFormatExtsString() { return QStringLiteral("RZIP compressed stream (*.rz)"); }
QString XRzipArchive::getMIMEString() { return QStringLiteral("application/x-rzip"); }
qint64 XRzipArchive::getFileFormatSize(PDSTRUCT *progress)
{
    const bool valid = isValid(progress);
    return valid ? getSize() : 0;
}
QList<QString> XRzipArchive::getSearchSignatures() { return {QStringLiteral("'RZIP'02")}; }
XBinary *XRzipArchive::createInstance(QIODevice *device, bool image, XADDR address)
{
    Q_UNUSED(image)
    Q_UNUSED(address)
    return new XRzipArchive(device);
}
bool XRzipArchive::readHeader(XRzipDecoder::HEADER *header, PDSTRUCT *progress)
{
    QIODevice *source = getDevice();
    if (!source->isOpen() || !source->isReadable() || source->isSequential() || !isPdStructNotCanceled(progress)) return false;
    const qint64 size = source->size();
    if (size < 24 || size > XRzipDecoder::MaxInput) return false;
    const QByteArray bytes = read_array_process(0, 24, progress);
    if (!XRzipDecoder::parseHeader(bytes, header)) return false;
    return (header->rawSize == 0 ? size == 24 : size >= 70) && isPdStructNotCanceled(progress);
}
bool XRzipArchive::isValid(PDSTRUCT *progress)
{
    XRzipDecoder::HEADER header;
    return readHeader(&header, progress);
}
bool XRzipArchive::isValid(QIODevice *device, PDSTRUCT *progress)
{
    XRzipArchive archive(device);
    return archive.isValid(progress);
}

bool XRzipArchive::initUnpack(UNPACK_STATE *state, const QMap<UNPACK_PROP,QVariant> &properties, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state) return false;
    if ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state)) return false;
    CONTEXT *old = static_cast<CONTEXT *>(state->pContext);
    releaseUnpackSource(state);
    delete old;
    *state = UNPACK_STATE();
    if (!isPdStructNotCanceled(progress)) return false;
    const bool bound = bindUnpackSource(state, progress);
    if (!bound) return false;
    OUTPUT_POLICY policy = {};
    XRzipDecoder::HEADER header;
    const bool valid = resolveUnpackOutputPolicy(properties, &policy) && readHeader(&header, progress);
    if (!valid || (policy.nMaxEntryOutputSize >= 0 && header.rawSize > policy.nMaxEntryOutputSize) ||
        (properties.contains(UNPACK_PROP_MAX_TOTAL_OUTPUT_SIZE) && policy.nMaxTotalOutputSize >= 0 && header.rawSize > policy.nMaxTotalOutputSize)) {
        releaseUnpackSource(state); *state = UNPACK_STATE(); return false;
    }
    CONTEXT *context = new (std::nothrow) CONTEXT;
    if (!context) { releaseUnpackSource(state); *state = UNPACK_STATE(); return false; }
    context->header = header;
    context->name = QFileInfo(XBinary::getDeviceFileName(getDevice())).completeBaseName();
    if (context->name.isEmpty()) context->name = QStringLiteral("data");
    state->pContext = context;
    state->nNumberOfRecords = 1;
    state->nCurrentIndex = 0;
    state->nCurrentOffset = 0;
    state->nTotalSize = getSize();
    state->mapUnpackProperties = properties;
    const bool finalized = validateAndFinalizeUnpackSource(state, context, progress);
    if (!finalized) {
        state->pContext = nullptr; releaseUnpackSource(state); delete context; *state = UNPACK_STATE(); return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XRzipArchive::infoCurrent(UNPACK_STATE *state, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    ARCHIVERECORD record = {};
    if (!guard.isAllowed() || !state || !state->pContext || state->nCurrentIndex != 0 || state->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(state, progress)) return record;
    const CONTEXT context = *static_cast<CONTEXT *>(state->pContext);
    const qint64 size = getSize();
    if (state->nTotalSize != size) return record;
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.name);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, size);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.header.rawSize);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("RZIP %1.%2 (stored/BZip2 substreams)").arg(context.header.major).arg(context.header.minor));
    if (!markArchiveStreamRecord(&record, 0)) return ARCHIVERECORD();
    return record;
}

bool XRzipArchive::unpackCurrent(UNPACK_STATE *state, QIODevice *device, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    QIODevice *source = getDevice();
    QIODevice *output = device;
    if (!guard.isAcquired() || !state || !state->pContext || state->nCurrentIndex != 0 || state->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(state, progress)) return false;
    const bool supported = isUnpackOutputSupported(output);
    if (!supported) return false;
    const bool aliases = devicesAlias(source, output);
    if (aliases) return false;
    const CONTEXT context = *static_cast<CONTEXT *>(state->pContext);
    const qint64 inputSize = state->nTotalSize;
    const QMap<UNPACK_PROP,QVariant> properties = state->mapUnpackProperties;
    const QSharedPointer<OUTPUT_BUDGET> budget = state->spOutputBudget;
    if (inputSize < 24 || inputSize > XRzipDecoder::MaxInput) return false;
    if (budget && !budget->beginEntry(0, context.name)) {
        if (budget->isEnforcing()) return false;
        OUTPUT_BUDGET::noteShadowRefusal(budget.data());
    }
    if (budget && budget->isEnforcing() && budget->totalLimit() >= 0 &&
        (budget->totalWritten() > budget->totalLimit() || context.header.rawSize > budget->totalLimit() - budget->totalWritten())) return false;
    const QByteArray input = read_array_process(0, inputSize, progress);
    if (input.size() != inputSize) return false;
    XRzipDecoder::HEADER header;
    if (!XRzipDecoder::parseHeader(input, &header) || header.rawSize != context.header.rawSize || header.major != context.header.major || header.minor != context.header.minor)
        return false;
    QTemporaryFile stage;
    if (!stage.open()) return false;
    DATAPROCESS_STATE writer = {};
    writer.pDeviceOutput = &stage;
    writer.nProcessedLimit = -1;
    writer.mapUnpackProperties = properties;
    writer.spOutputBudget = budget;
    XRzipDecoder::RESULT decoded;
    if (!XRzipDecoder::decode(input, &writer, &decoded, progress) ||
        decoded.outputSize != context.header.rawSize || decoded.consumed != inputSize) return false;
    if (!stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(state, progress)) return false;
    const bool published = publishUnpackOutput(&stage, output, state, progress);
    if (!published) return false;
    state->nCurrentOffset = decoded.consumed;
    return true;
}

bool XRzipArchive::moveToNext(UNPACK_STATE *state, PDSTRUCT *progress)
{
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || !state->pContext || state->nCurrentIndex != 0 || state->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(state, progress)) return false;
    state->nCurrentIndex = 1;
    return false;
}
bool XRzipArchive::finishUnpack(UNPACK_STATE *state, PDSTRUCT *progress)
{
    Q_UNUSED(progress)
    UNPACK_OPERATION_GUARD guard(&m_bUnpackOperationInProgress);
    if (!guard.isAcquired() || !state || ((state->pContext || !state->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(state))) return false;
    CONTEXT *context = static_cast<CONTEXT *>(state->pContext);
    state->pContext = nullptr;
    releaseUnpackSource(state);
    delete context;
    *state = UNPACK_STATE();
    return true;
}
