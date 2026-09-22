// The reference implementation Crunch/CPMLZH/Unix Compact handler port.
#include "xcpmcompressedarchive.h"

#include <QBuffer>
#include <QTemporaryFile>
#include <new>

namespace {
// The decoder's cancellation hook is a plain function pointer, so the state it
// needs travels through the opaque argument instead of a capture.
struct Cancellation {
    XCpmCompressedArchive *owner;
    QIODevice *source, *output;
    XBinary::PDSTRUCT *progress;
};

bool cpmProgressCanceled(void *opaque) { return !XBinary::isPdStructNotCanceled(static_cast<XBinary::PDSTRUCT *>(opaque)); }

bool cpmCancellationCanceled(void *opaque)
{
    const Cancellation *state = static_cast<const Cancellation *>(opaque);
    return !state->owner || !state->source || !state->output || !XBinary::isPdStructNotCanceled(state->progress);
}
}  // namespace

XCpmCompressedArchive::XCpmCompressedArchive(QIODevice *pDevice, FT fileType) : XArchive(pDevice), m_fileType(fileType) {}
XBinary::FT XCpmCompressedArchive::getFileType() { return m_fileType; }
XBinary::MODE XCpmCompressedArchive::getMode() { return MODE_DATA; }
qint32 XCpmCompressedArchive::getType() { return TYPE_ARCHIVE; }
XBinary::ENDIAN XCpmCompressedArchive::getEndian() { return ENDIAN_LITTLE; }
QString XCpmCompressedArchive::getArch() { return QString(); }
QString XCpmCompressedArchive::getVersion() { return QString(); }
QString XCpmCompressedArchive::getFileFormatExt()
{
    return m_fileType == FT_UNIX_COMPACT ? QStringLiteral("C") : m_fileType == FT_CPM_CRUNCH ? QStringLiteral("?z?") : QStringLiteral("?y?");
}
QString XCpmCompressedArchive::getFileFormatExtsString()
{
    return m_fileType == FT_UNIX_COMPACT ? QStringLiteral("Unix Compact (*.C)") :
        m_fileType == FT_CPM_CRUNCH ? QStringLiteral("CP/M Crunch (*.?z?)") : QStringLiteral("CP/M LZH (*.?y?)");
}
QString XCpmCompressedArchive::getMIMEString() { return QStringLiteral("application/octet-stream"); }
qint64 XCpmCompressedArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    const bool valid = isValid(pPdStruct);
    return valid ? getSize() : 0;
}
QList<QString> XCpmCompressedArchive::getSearchSignatures()
{
    return {m_fileType == FT_UNIX_COMPACT ? QStringLiteral("FF1F") : m_fileType == FT_CPM_CRUNCH ? QStringLiteral("76FE") : QStringLiteral("76FD")};
}
XBinary *XCpmCompressedArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCpmCompressedArchive(pDevice, m_fileType);
}

bool XCpmCompressedArchive::readHeader(XCpmCrunchDecoder::Header *pHeader, PDSTRUCT *pPdStruct, qint64 *pUncompressedSize, qint64 nOutputLimit)
{
    const qint64 nSize = getSize();
    if (!pHeader || !isPdStructNotCanceled(pPdStruct) || nSize < 4 || nSize > static_cast<qint64>(XCpmCrunchDecoder::MaxInput)) return false;
    const QByteArray data = read_array_process(0, qMin<qint64>(nSize, 160), pPdStruct);
    if (!XCpmCrunchDecoder::parseHeader(reinterpret_cast<const std::uint8_t *>(data.constData()), static_cast<std::size_t>(data.size()), pHeader)) return false;
    const bool bType = (m_fileType == FT_CPM_CRUNCH && pHeader->format == XCpmCrunchDecoder::Format::Crunch) ||
                       (m_fileType == FT_CPM_LZH && pHeader->format == XCpmCrunchDecoder::Format::Crlzh) ||
                       (m_fileType == FT_UNIX_COMPACT && pHeader->format == XCpmCrunchDecoder::Format::UnixCompact);
    if (!bType || nOutputLimit < 0 || !isPdStructNotCanceled(pPdStruct)) return false;
    // Compact's two-byte identifier is not an adequate structural probe.
    // Initialization also measures every format without retaining its output.
    if (pUncompressedSize || pHeader->format == XCpmCrunchDecoder::Format::UnixCompact) {
        const QByteArray packed = read_array_process(0, nSize, pPdStruct);
        if (packed.size() != nSize) return false;
        XCpmCrunchDecoder::Result measured;
        const bool valid = XCpmCrunchDecoder::decode(reinterpret_cast<const std::uint8_t *>(packed.constData()), static_cast<std::size_t>(packed.size()),
                                                static_cast<std::size_t>(nOutputLimit), &measured, cpmProgressCanceled, pPdStruct, true);
        if (!valid) return false;
        if (pUncompressedSize) *pUncompressedSize = static_cast<qint64>(measured.uncompressedSize);
    }
    return isPdStructNotCanceled(pPdStruct);
}
bool XCpmCompressedArchive::isValid(PDSTRUCT *pPdStruct)
{
    XCpmCrunchDecoder::Header header;
    return readHeader(&header, pPdStruct);
}
bool XCpmCompressedArchive::isValid(QIODevice *pDevice, FT fileType, PDSTRUCT *pPdStruct)
{
    XCpmCompressedArchive archive(pDevice, fileType);
    return archive.isValid(pPdStruct);
}

bool XCpmCompressedArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    CONTEXT *old = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete old;
    *pState = UNPACK_STATE();
    if (!isPdStructNotCanceled(pPdStruct)) return false;
    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!bBound) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(mapProperties, &policy)) { releaseUnpackSource(pState); *pState = UNPACK_STATE(); return false; }
    qint64 nLimit = static_cast<qint64>(XCpmCrunchDecoder::MaxOutput);
    if (policy.nMaxEntryOutputSize >= 0) nLimit = qMin(nLimit, policy.nMaxEntryOutputSize);
    if (policy.nMaxMemoryOutputSize >= 0) nLimit = qMin(nLimit, policy.nMaxMemoryOutputSize);
    if (mapProperties.contains(UNPACK_PROP_MAX_TOTAL_OUTPUT_SIZE) && policy.nMaxTotalOutputSize >= 0) nLimit = qMin(nLimit, policy.nMaxTotalOutputSize);
    XCpmCrunchDecoder::Header header;
    qint64 nUncompressedSize = 0;
    const bool bHeader = readHeader(&header, pPdStruct, &nUncompressedSize, nLimit);
    if (!bHeader) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    CONTEXT *context = new (std::nothrow) CONTEXT;
    if (!context) { releaseUnpackSource(pState); *pState = UNPACK_STATE(); return false; }
    context->header = std::move(header);
    context->nUncompressedSize = nUncompressedSize;
    context->sFileName = QString::fromLatin1(context->header.name.data(), static_cast<int>(context->header.name.size()));
    if (context->header.format == XCpmCrunchDecoder::Format::UnixCompact) {
        context->sFileName = XBinary::getDeviceFileBaseName(getDevice());
        if (context->sFileName.isEmpty()) context->sFileName = QStringLiteral("data");
    }
    pState->pContext = context;
    pState->nNumberOfRecords = 1;
    pState->nCurrentIndex = 0;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = getSize();
    pState->mapUnpackProperties = mapProperties;
    const bool bFinalized = validateAndFinalizeUnpackSource(pState, context, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete context;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XCpmCompressedArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    ARCHIVERECORD record = {};
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || pState->nCurrentIndex != 0 || pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct) || pState->nTotalSize != getSize()) return record;
    const XCpmCrunchDecoder::Header &header = static_cast<CONTEXT *>(pState->pContext)->header;
    record.mapProperties.insert(FPART_PROP_ORIGINALNAME, static_cast<CONTEXT *>(pState->pContext)->sFileName);
    record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pState->nTotalSize);
    record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, static_cast<CONTEXT *>(pState->pContext)->nUncompressedSize);
    record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, m_fileType == FT_UNIX_COMPACT ? QStringLiteral("Unix Compact") :
                                QStringLiteral("%1 %2.x").arg(m_fileType == FT_CPM_CRUNCH ? QStringLiteral("Crunch") : QStringLiteral("CP/M LZH")).arg(header.revision));
    if (!markArchiveStreamRecord(&record, 0)) return ARCHIVERECORD();
    return record;
}

bool XCpmCompressedArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QIODevice *source = getDevice(), *output = pDevice;
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || pState->nCurrentIndex != 0 || pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    const bool bSupported = isUnpackOutputSupported(output);
    if (!bSupported) return false;
    const bool bAliases = devicesAlias(source, output);
    if (bAliases || !isPdStructNotCanceled(pPdStruct)) return false;
    const XCpmCrunchDecoder::Header header = static_cast<CONTEXT *>(pState->pContext)->header;
    const QString sFileName = static_cast<CONTEXT *>(pState->pContext)->sFileName;
    const qint64 nExpectedSize = static_cast<CONTEXT *>(pState->pContext)->nUncompressedSize;
    const qint64 nInputSize = pState->nTotalSize;
    if (nInputSize < 4 || nInputSize > static_cast<qint64>(XCpmCrunchDecoder::MaxInput)) return false;
    OUTPUT_POLICY policy = {};
    if (!resolveUnpackOutputPolicy(pState->mapUnpackProperties, &policy)) return false;
    qint64 nLimit = static_cast<qint64>(XCpmCrunchDecoder::MaxOutput);
    if (policy.nMaxEntryOutputSize >= 0) nLimit = qMin(nLimit, policy.nMaxEntryOutputSize);
    // The private codec's result is in memory even when publication is to disk.
    if (policy.nMaxMemoryOutputSize >= 0) nLimit = qMin(nLimit, policy.nMaxMemoryOutputSize);
    const QSharedPointer<OUTPUT_BUDGET> budget = pState->spOutputBudget;
    if (budget) {
        if (!budget->beginEntry(0, sFileName)) {
            if (budget->isEnforcing()) return false;
            OUTPUT_BUDGET::noteShadowRefusal(budget.data());
        }
        if (budget->isEnforcing() && budget->totalLimit() >= 0) {
            if (budget->totalWritten() > budget->totalLimit()) return false;
            nLimit = qMin(nLimit, budget->totalLimit() - budget->totalWritten());
        }
    }
    const QByteArray input = read_array_process(0, nInputSize, pPdStruct);
    if (input.size() != nInputSize) return false;
    Cancellation cancellation = {this, source, output, pPdStruct};
    XCpmCrunchDecoder::Result decoded;
    if (!XCpmCrunchDecoder::decode(reinterpret_cast<const std::uint8_t *>(input.constData()), static_cast<std::size_t>(input.size()),
                               static_cast<std::size_t>(nLimit), &decoded, cpmCancellationCanceled, &cancellation) || cpmCancellationCanceled(&cancellation) ||
        decoded.header.format != header.format || decoded.header.name != header.name || decoded.header.revision != header.revision) return false;
    if (static_cast<qint64>(decoded.uncompressedSize) != nExpectedSize || decoded.data.size() != decoded.uncompressedSize) return false;
    QTemporaryFile stage;
    if (!stage.open()) return false;
    DATAPROCESS_STATE writeState = {};
    writeState.pDeviceOutput = &stage;
    writeState.nProcessedLimit = -1;
    writeState.mapUnpackProperties = pState->mapUnpackProperties;
    writeState.spOutputBudget = budget;
    for (std::size_t position = 0; position < decoded.data.size();) {
        if (cpmCancellationCanceled(&cancellation)) return false;
        const qint64 count = static_cast<qint64>(qMin<std::size_t>(0x10000, decoded.data.size() - position));
        if (_writeDevice(reinterpret_cast<const char *>(decoded.data.data() + position), count, &writeState) != count) return false;
        position += static_cast<std::size_t>(count);
    }
    if (!stage.flush() || !stage.seek(0) || !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    const bool bPublished = publishUnpackOutput(&stage, output, pState, pPdStruct);
    if (!bPublished) return false;
    pState->nCurrentOffset = static_cast<qint64>(decoded.consumed);
    return true;
}

bool XCpmCompressedArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || pState->nCurrentIndex != 0 || pState->nNumberOfRecords != 1 ||
        !isUnpackSourceCurrent(pState, pPdStruct)) return false;
    pState->nCurrentIndex = 1;
    return false;
}
bool XCpmCompressedArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState))) return false;
    CONTEXT *context = static_cast<CONTEXT *>(pState->pContext);
    pState->pContext = nullptr;
    releaseUnpackSource(pState);
    delete context;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    return true;
}
