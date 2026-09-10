/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free FlashJester Jugglor 2.x reader.
 * MIT License
 */
#include "xjugglor.h"

#include <QScopedPointer>
#include <QUuid>

#include <limits>
#include <memory>
#include <new>
#include <zlib.h>

#include "xpe.h"

namespace {
const quint32 JUGGLOR_MAGIC = 0x6a4a61a3U;
const qint64 JUGGLOR_HEADER_SIZE = 0x31c;
const qint64 JUGGLOR_TRAILER_SIZE = 220;
const quint32 JUGGLOR_MAX_MEMBERS = 100000;
const char JUGGLOR_VERSION_PREFIX[] = "Jester Jugglor Version ";

quint32 readLe32(const char *pData)
{
    const uchar *p = reinterpret_cast<const uchar *>(pData);
    return quint32(p[0]) | (quint32(p[1]) << 8) |
           (quint32(p[2]) << 16) | (quint32(p[3]) << 24);
}

quint64 readLe64(const char *pData)
{
    return quint64(readLe32(pData)) |
           (quint64(readLe32(pData + 4)) << 32);
}

bool rangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool readExact(QIODevice *pDevice, qint64 nOffset, qint64 nSize,
               QByteArray *pResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pResult || pDevice->isSequential() ||
        nSize < 0 || nSize > std::numeric_limits<int>::max() ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !pDevice->seek(nOffset)) {
        return false;
    }

    pResult->resize(static_cast<int>(nSize));
    qint64 nRead = 0;
    while (nRead < nSize && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nCurrent = pDevice->read(pResult->data() + nRead,
                                              nSize - nRead);
        if (nCurrent <= 0) return false;
        nRead += nCurrent;
    }
    return nRead == nSize;
}

bool decodeShortString(const QByteArray &baField, bool bAllowEmpty,
                       QString *pValue)
{
    if (!pValue || baField.size() != 256) return false;
    const qint32 nLength = static_cast<uchar>(baField.at(0));
    if ((!bAllowEmpty && nLength == 0) || nLength > 255) return false;
    for (qint32 i = 0; i < nLength; ++i) {
        const uchar nValue = static_cast<uchar>(baField.at(i + 1));
        if (nValue < 0x20 || nValue == 0x7f) return false;
    }
    *pValue = QString::fromLatin1(baField.constData() + 1, nLength);
    return true;
}

bool makeOutputName(const QString &sDirectory, const QString &sFileName,
                    QString *pResult)
{
    if (!pResult || sFileName.isEmpty() ||
        sFileName.contains(QLatin1Char('/')) ||
        sFileName.contains(QLatin1Char('\\')) ||
        sFileName.contains(QLatin1Char(':'))) {
        return false;
    }

    QString sPath = sDirectory;
    sPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sPath.size() < 3 || !sPath.at(0).isLetter() ||
        sPath.at(1) != QLatin1Char(':') ||
        sPath.at(2) != QLatin1Char('/')) {
        return false;
    }
    sPath.remove(0, 3);
    while (sPath.endsWith(QLatin1Char('/'))) sPath.chop(1);

    const QStringList listParts = sPath.split(QLatin1Char('/'),
                                               Qt::SkipEmptyParts);
    QStringList listSafeParts;
    for (const QString &sPart : listParts) {
        if (sPart == QStringLiteral(".") || sPart == QStringLiteral("..") ||
            sPart.contains(QLatin1Char(':')) ||
            XBinary::fixFileName(sPart) != sPart) {
            return false;
        }
        listSafeParts.append(sPart);
    }
    if (XBinary::fixFileName(sFileName) != sFileName) return false;
    listSafeParts.append(sFileName);
    *pResult = listSafeParts.join(QLatin1Char('/'));
    return !pResult->isEmpty();
}

bool inflateZlibStream(QIODevice *pSource, qint64 nDataOffset,
                       qint64 nCompressedSize, qint64 nUncompressedSize,
                       QIODevice *pOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSource || !pOutput || nCompressedSize < 0 ||
        nUncompressedSize < 0 || !pSource->seek(nDataOffset)) {
        return false;
    }

    z_stream stream = {};
    if (inflateInit2(&stream, 15) != Z_OK) return false;

    QPointer<QIODevice> guardedSource(pSource);
    QPointer<QIODevice> guardedOutput(pOutput);
    QByteArray baInput(64 * 1024, Qt::Uninitialized);
    QByteArray baOutput(64 * 1024, Qt::Uninitialized);
    qint64 nInputRemaining = nCompressedSize;
    qint64 nConsumed = 0;
    qint64 nProduced = 0;
    bool bResult = false;

    while (guardedSource && guardedOutput &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (stream.avail_in == 0 && nInputRemaining > 0) {
            const qint64 nRequest = qMin<qint64>(baInput.size(),
                                                 nInputRemaining);
            const qint64 nRead = guardedSource->read(baInput.data(), nRequest);
            if (nRead <= 0) break;
            nInputRemaining -= nRead;
            stream.next_in = reinterpret_cast<Bytef *>(baInput.data());
            stream.avail_in = static_cast<uInt>(nRead);
        }

        stream.next_out = reinterpret_cast<Bytef *>(baOutput.data());
        stream.avail_out = static_cast<uInt>(baOutput.size());
        const uInt nBeforeInput = stream.avail_in;
        const int nStatus = inflate(&stream, Z_NO_FLUSH);
        const qint64 nUsed = nBeforeInput - stream.avail_in;
        const qint64 nCurrentOutput = baOutput.size() - stream.avail_out;
        nConsumed += nUsed;
        if (nCurrentOutput > nUncompressedSize - nProduced) break;
        if (nCurrentOutput > 0) {
            const qint64 nWritten = guardedOutput->write(baOutput.constData(),
                                                         nCurrentOutput);
            if (nWritten != nCurrentOutput) break;
            nProduced += nCurrentOutput;
        }

        if (nStatus == Z_STREAM_END) {
            bResult = nInputRemaining == 0 && stream.avail_in == 0 &&
                      nConsumed == nCompressedSize &&
                      nProduced == nUncompressedSize;
            break;
        }
        if (nStatus != Z_OK || (nUsed == 0 && nCurrentOutput == 0) ||
            (nInputRemaining == 0 && stream.avail_in == 0)) {
            break;
        }
    }

    inflateEnd(&stream);
    return bResult && guardedOutput &&
           guardedOutput->size() == nUncompressedSize;
}

class JUGGLOR_PARSE_RESULT {
public:
    JUGGLOR_PARSE_RESULT(const QPointer<XJugglor> &pOwner,
                         const QPointer<QIODevice> &pDevice,
                         quint64 nGeneration, qint64 nSavedPosition)
        : m_pOwner(pOwner),
          m_pDevice(pDevice),
          m_nGeneration(nGeneration),
          m_nSavedPosition(nSavedPosition)
    {
    }

    bool operator()(bool bResult) const
    {
        if (!m_pOwner || !m_pDevice ||
            m_pOwner->getDevice() != m_pDevice.data() ||
            m_pOwner->getDeviceGeneration() != m_nGeneration ||
            !m_pDevice->seek(m_nSavedPosition)) {
            return false;
        }
        return bResult;
    }

private:
    QPointer<XJugglor> m_pOwner;
    QPointer<QIODevice> m_pDevice;
    quint64 m_nGeneration;
    qint64 m_nSavedPosition;
};
}  // namespace

XJugglor::XJugglor(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
    : XArchive(pDevice)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    setIsArchive(true);
}

XJugglor::~XJugglor()
{
    const QSet<UNPACK_CONTEXT *> setContexts = m_setContexts;
    m_setContexts.clear();
    for (UNPACK_CONTEXT *pContext : setContexts) delete pContext;
}

bool XJugglor::isDeviceReplacementAllowed() const
{
    return m_setContexts.isEmpty() && XArchive::isDeviceReplacementAllowed();
}

bool XJugglor::_parse(QList<FILE_ENTRY> *pEntries, qint64 *pSourceSize,
                      QString *pVersion, PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pSourceSize) *pSourceSize = 0;
    if (pVersion) pVersion->clear();
    if (!isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XJugglor> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!guardedThis || !guardedDevice || !guardedDevice->isOpen() ||
        !guardedDevice->isReadable() || guardedDevice->isSequential()) {
        return false;
    }
    const quint64 nGeneration = getDeviceGeneration();
    const qint64 nTotalSize = guardedDevice->size();
    const qint64 nSavedPosition = guardedDevice->pos();
    if (nSavedPosition < 0) return false;

    const JUGGLOR_PARSE_RESULT restoreAndReturn(
        guardedThis, guardedDevice, nGeneration, nSavedPosition);

    XPE pe(guardedDevice.data());
    if (!pe.isValid(pPdStruct) || !guardedThis || !guardedDevice || pe.is64()) {
        return restoreAndReturn(false);
    }
    const qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    const qint64 nTrailerOffset = nTotalSize - JUGGLOR_TRAILER_SIZE;
    if (!guardedThis || !guardedDevice || nOverlayOffset <= 0 ||
        !rangeWithin(nTotalSize, nOverlayOffset,
                     JUGGLOR_HEADER_SIZE + JUGGLOR_TRAILER_SIZE) ||
        nTrailerOffset <= nOverlayOffset) {
        return restoreAndReturn(false);
    }

    QByteArray baTrailer;
    if (!readExact(guardedDevice.data(), nTrailerOffset,
                   JUGGLOR_TRAILER_SIZE, &baTrailer, pPdStruct) ||
        readLe32(baTrailer.constData()) != JUGGLOR_MAGIC ||
        readLe32(baTrailer.constData() + 4) !=
            static_cast<quint64>(nOverlayOffset) ||
        readLe32(baTrailer.constData() + 8) !=
            static_cast<quint64>(nTrailerOffset - nOverlayOffset)) {
        return restoreAndReturn(false);
    }

    const quint32 nMemberCount = readLe32(baTrailer.constData() + 12);
    const quint32 nDeclaredTotal = readLe32(baTrailer.constData() + 20);
    if (nMemberCount < 1 || nMemberCount > JUGGLOR_MAX_MEMBERS ||
        readLe32(baTrailer.constData() + 16) != 0) {
        return restoreAndReturn(false);
    }
    const qint32 nVersionSize = static_cast<uchar>(baTrailer.at(24));
    if (nVersionSize < qint32(sizeof(JUGGLOR_VERSION_PREFIX) - 1) ||
        nVersionSize > baTrailer.size() - 25) {
        return restoreAndReturn(false);
    }
    const QByteArray baVersion = baTrailer.mid(25, nVersionSize);
    for (char cValue : baVersion) {
        const uchar nValue = static_cast<uchar>(cValue);
        if (nValue < 0x20 || nValue > 0x7e) return restoreAndReturn(false);
    }
    if (!baVersion.startsWith(JUGGLOR_VERSION_PREFIX)) {
        return restoreAndReturn(false);
    }

    QList<FILE_ENTRY> entries;
    entries.reserve(static_cast<qint32>(nMemberCount));
    QSet<QString> usedNames;
    quint64 nActualTotal = 0;
    qint64 nPosition = nOverlayOffset;

    for (quint32 i = 0; i < nMemberCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !rangeWithin(nTrailerOffset, nPosition, JUGGLOR_HEADER_SIZE)) {
            return restoreAndReturn(false);
        }
        QByteArray baHeader;
        if (!readExact(guardedDevice.data(), nPosition, JUGGLOR_HEADER_SIZE,
                       &baHeader, pPdStruct) ||
            readLe32(baHeader.constData()) != JUGGLOR_MAGIC) {
            return restoreAndReturn(false);
        }

        QString sFileName;
        QString sSourceDirectory;
        if (!decodeShortString(baHeader.mid(4, 256), false, &sFileName) ||
            !decodeShortString(baHeader.mid(0x104, 256), false,
                               &sSourceDirectory)) {
            return restoreAndReturn(false);
        }
        QString sOutputName;
        if (!makeOutputName(sSourceDirectory, sFileName, &sOutputName) ||
            usedNames.contains(sOutputName.toLower())) {
            return restoreAndReturn(false);
        }
        usedNames.insert(sOutputName.toLower());

        const qint64 nUncompressedSize =
            readLe32(baHeader.constData() + 0x304);
        const qint64 nCompressedSize =
            readLe32(baHeader.constData() + 0x308);
        const qint64 nDataOffset = nPosition + JUGGLOR_HEADER_SIZE;
        if (nCompressedSize < 6 ||
            !rangeWithin(nTrailerOffset, nDataOffset, nCompressedSize)) {
            return restoreAndReturn(false);
        }

        QByteArray baZlibHeader;
        if (!readExact(guardedDevice.data(), nDataOffset, 2, &baZlibHeader,
                       pPdStruct)) {
            return restoreAndReturn(false);
        }
        const quint8 nCMF = static_cast<quint8>(baZlibHeader.at(0));
        const quint8 nFLG = static_cast<quint8>(baZlibHeader.at(1));
        if ((nCMF & 0x0fU) != 8 || (nCMF >> 4) > 7 ||
            ((quint16(nCMF) << 8) + nFLG) % 31 != 0 ||
            (nFLG & 0x20U) != 0) {
            return restoreAndReturn(false);
        }

        const quint64 nFileTime = readLe64(baHeader.constData() + 0x30c);
        const QDateTime dtEncoded = winFileTimeToQDateTime(nFileTime);
        QDateTime mtDateTime;
        if (dtEncoded.isValid()) {
            // Jugglor writes a local wall-clock value into a FILETIME-shaped
            // integer. Reinterpret its fields as local instead of applying a
            // second timezone conversion (the reference implementation's historical DST bug).
            mtDateTime = QDateTime(dtEncoded.date(), dtEncoded.time());
        }

        FILE_ENTRY entry;
        entry.sName = sOutputName;
        entry.nHeaderOffset = nPosition;
        entry.nDataOffset = nDataOffset;
        entry.nCompressedSize = nCompressedSize;
        entry.nUncompressedSize = nUncompressedSize;
        entry.mtDateTime = mtDateTime;
        entries.append(entry);

        nActualTotal += static_cast<quint64>(nUncompressedSize);
        if (nActualTotal > std::numeric_limits<quint32>::max()) {
            return restoreAndReturn(false);
        }
        nPosition = nDataOffset + nCompressedSize;
    }

    if (!guardedThis || !guardedDevice || nPosition != nTrailerOffset ||
        entries.size() != static_cast<qint32>(nMemberCount) ||
        nActualTotal != nDeclaredTotal ||
        getDeviceGeneration() != nGeneration ||
        getDevice() != guardedDevice.data()) {
        return restoreAndReturn(false);
    }

    if (pEntries) *pEntries = entries;
    if (pSourceSize) *pSourceSize = nTotalSize;
    if (pVersion) *pVersion = QString::fromLatin1(baVersion);
    return restoreAndReturn(true);
}

bool XJugglor::isValid(PDSTRUCT *pPdStruct)
{
    return _parse(nullptr, nullptr, nullptr, pPdStruct);
}

bool XJugglor::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XJugglor x(pDevice);
    return x.isValid(pPdStruct);
}

XBinary::FT XJugglor::getFileType()
{
    return FT_PE32_JUGGLOR;
}

XBinary::MODE XJugglor::getMode()
{
    return MODE_32;
}

QString XJugglor::getArch()
{
    return QStringLiteral("i386");
}

QString XJugglor::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XJugglor::getFileFormatExtsString()
{
    return QStringLiteral("FlashJester Jugglor executable archive (*.exe)");
}

QString XJugglor::getMIMEString()
{
    return QStringLiteral("application/x-flashjester-jugglor");
}

QString XJugglor::getVersion()
{
    QString sVersion;
    if (!_parse(nullptr, nullptr, &sVersion, nullptr)) return QString();
    const QString sPrefix = QString::fromLatin1(JUGGLOR_VERSION_PREFIX);
    return sVersion.startsWith(sPrefix) ? sVersion.mid(sPrefix.size()) : sVersion;
}

QList<QString> XJugglor::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("4D5A*A3614A6A");
}

XBinary *XJugglor::createInstance(QIODevice *pDevice, bool bIsImage,
                                  XADDR nModuleAddress)
{
    return new XJugglor(pDevice, bIsImage, nModuleAddress);
}

bool XJugglor::_isContextCurrent(const UNPACK_STATE *pState,
                                 const UNPACK_CONTEXT *pContext)
{
    return pState && pContext &&
           m_setContexts.contains(const_cast<UNPACK_CONTEXT *>(pContext)) &&
           pState->pContext == pContext && pContext->pOwnerState == pState &&
           !pState->baUnpackSourceToken.isEmpty() &&
           pState->baUnpackSourceToken == pContext->baToken &&
           pContext->pSourceDevice.data() == getDevice() &&
           pContext->nDeviceGeneration == getDeviceGeneration() &&
           pState->nTotalSize == pContext->nSourceSize &&
           pState->nNumberOfRecords == pContext->listEntries.size() &&
           pState->nCurrentIndex == pContext->nCurrentIndex &&
           pState->nCurrentOffset == pContext->nCurrentOffset;
}

bool XJugglor::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    if (pState->pContext || !pState->baUnpackSourceToken.isEmpty()) {
        UNPACK_CONTEXT *pOld =
            static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!_isContextCurrent(pState, pOld)) return false;
        m_setContexts.remove(pOld);
        delete pOld;
    }
    *pState = UNPACK_STATE();

    QList<FILE_ENTRY> entries;
    qint64 nSourceSize = 0;
    if (!_parse(&entries, &nSourceSize, nullptr, pPdStruct) ||
        entries.isEmpty()) {
        return false;
    }

    std::unique_ptr<UNPACK_CONTEXT> pContext(
        new (std::nothrow) UNPACK_CONTEXT);
    if (!pContext) return false;
    pContext->listEntries = entries;
    pContext->pSourceDevice = getDevice();
    pContext->pOwnerState = pState;
    pContext->nDeviceGeneration = getDeviceGeneration();
    pContext->nSourceSize = nSourceSize;
    pContext->baToken = QUuid::createUuid().toRfc4122();
    if (!pContext->pSourceDevice || pContext->baToken.isEmpty()) return false;

    pState->nTotalSize = nSourceSize;
    pState->nNumberOfRecords = entries.size();
    pState->mapUnpackProperties = mapProperties;
    pState->pContext = pContext.get();
    pState->baUnpackSourceToken = pContext->baToken;
    m_setContexts.insert(pContext.release());
    return true;
}

XBinary::ARCHIVERECORD XJugglor::infoCurrent(UNPACK_STATE *pState,
                                             PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext)
        return result;
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext) ||
        pContext->nCurrentIndex < 0 ||
        pContext->nCurrentIndex >= pContext->listEntries.size()) {
        return result;
    }

    const FILE_ENTRY &entry =
        pContext->listEntries.at(pContext->nCurrentIndex);
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nCompressedSize;
    result.mapProperties[FPART_PROP_ORIGINALNAME] = entry.sName;
    result.mapProperties[FPART_PROP_COMPRESSEDSIZE] = entry.nCompressedSize;
    result.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] =
        entry.nUncompressedSize;
    result.mapProperties[FPART_PROP_HANDLEMETHOD] = HANDLE_METHOD_ZLIB;
    result.mapProperties[FPART_PROP_HEADER_OFFSET] = entry.nHeaderOffset;
    result.mapProperties[FPART_PROP_HEADER_SIZE] = JUGGLOR_HEADER_SIZE;
    result.mapProperties[FPART_PROP_FILEMODE] = quint32(0644);
    result.mapProperties[FPART_PROP_ISFOLDER] = false;
    if (entry.mtDateTime.isValid()) {
        result.mapProperties[FPART_PROP_DATETIME] = entry.mtDateTime;
        result.mapProperties[FPART_PROP_MTIME] = entry.mtDateTime;
    }
    // Extraction of a Jugglor member must stay paired with this parser
    // session.  Publishing the archive-stream contract routes the record back
    // through unpackCurrent(), whose bounded system-zlib path authenticates
    // the complete RFC 1950 member before committing any output.
    if (!markArchiveStreamRecord(&result, pContext->nCurrentIndex)) {
        return ARCHIVERECORD();
    }
    return result;
}

bool XJugglor::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                             PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext || !guardedOutput ||
        !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() ||
        (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !isResizeEnable(guardedOutput.data()) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext) ||
        devicesAlias(pContext->pSourceDevice.data(), guardedOutput.data()) ||
        pContext->nCurrentIndex < 0 ||
        pContext->nCurrentIndex >= pContext->listEntries.size()) {
        return false;
    }
    const FILE_ENTRY &entry =
        pContext->listEntries.at(pContext->nCurrentIndex);
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   entry.nUncompressedSize)) {
        setPdStructErrorString(
            pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (pState->spOutputBudget &&
        !pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                            entry.sName) &&
        pState->spOutputBudget->isEnforcing()) {
        setPdStructErrorString(
            pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    QScopedPointer<QIODevice> pStage(
        createFileBuffer(entry.nUncompressedSize, pPdStruct));
    QPointer<QIODevice> guardedStage(pStage.data());
    if (!guardedStage || !resize(guardedStage.data(), 0) ||
        !guardedStage->seek(0)) {
        return false;
    }

    QPointer<QIODevice> guardedSource(pContext->pSourceDevice.data());
    if (!guardedSource) return false;
    const qint64 nSavedSourcePosition = guardedSource->pos();
    const bool bInflated = nSavedSourcePosition >= 0 &&
        inflateZlibStream(guardedSource.data(), entry.nDataOffset,
                          entry.nCompressedSize, entry.nUncompressedSize,
                          guardedStage.data(), pPdStruct);
    const bool bRestored = guardedSource &&
                           guardedSource->seek(nSavedSourcePosition);
    if (!bInflated || !bRestored || !_isContextCurrent(pState, pContext) ||
        !guardedStage || !guardedOutput ||
        guardedStage->size() != entry.nUncompressedSize ||
        !guardedStage->seek(0)) {
        if (guardedOutput) {
            resize(guardedOutput.data(), 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }

    // Charge the decoded member exactly once.  The following stage-to-output
    // copy is publication of bytes already accounted for, not another decode.
    if (pState->spOutputBudget) {
        const OUTPUT_BUDGET::REFUSAL refusalBefore =
            pState->spOutputBudget->refusal();
        const bool bAccepted =
            pState->spOutputBudget->debit(entry.nUncompressedSize);
        if (!bAccepted && pState->spOutputBudget->isEnforcing()) {
            setPdStructErrorString(
                pPdStruct,
                tr("Unpacked output exceeds the configured limit"));
            return false;
        }
        if (refusalBefore == OUTPUT_BUDGET::REFUSAL_NONE &&
            pState->spOutputBudget->refusal() !=
                OUTPUT_BUDGET::REFUSAL_NONE) {
            OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }

    if (!resize(guardedOutput.data(), 0) || !guardedOutput ||
        !resize(guardedOutput.data(), entry.nUncompressedSize) ||
        !guardedOutput || !guardedOutput->seek(0)) {
        if (guardedOutput) {
            resize(guardedOutput.data(), 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }

    QByteArray baBuffer(64 * 1024, Qt::Uninitialized);
    qint64 nPublished = 0;
    bool bPublished = true;
    while (guardedStage && guardedOutput && !guardedStage->atEnd() &&
           isPdStructNotCanceled(pPdStruct)) {
        const qint64 nRead = guardedStage->read(baBuffer.data(),
                                                baBuffer.size());
        if (nRead <= 0 ||
            safeWriteData(guardedOutput.data(), nPublished,
                          baBuffer.constData(), nRead, pPdStruct) != nRead) {
            bPublished = false;
            break;
        }
        nPublished += nRead;
    }
    const bool bFinal = bPublished && guardedStage && guardedOutput &&
        guardedStage->atEnd() &&
        guardedOutput->size() == entry.nUncompressedSize &&
        nPublished == entry.nUncompressedSize &&
        _isContextCurrent(pState, pContext) &&
        isPdStructNotCanceled(pPdStruct);
    if (!bFinal) {
        if (guardedOutput) {
            resize(guardedOutput.data(), 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }

    pContext->nCurrentOffset = nPublished;
    pState->nCurrentOffset = nPublished;
    return true;
}

bool XJugglor::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!isPdStructNotCanceled(pPdStruct) || !pState || !pState->pContext)
        return false;
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext) ||
        pContext->nCurrentIndex < 0 ||
        pContext->nCurrentIndex >= pContext->listEntries.size()) {
        return false;
    }
    ++pContext->nCurrentIndex;
    pContext->nCurrentOffset = 0;
    pState->nCurrentIndex = pContext->nCurrentIndex;
    pState->nCurrentOffset = 0;
    return pContext->nCurrentIndex < pContext->listEntries.size();
}

bool XJugglor::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState) return false;
    if (!pState->pContext && pState->baUnpackSourceToken.isEmpty()) {
        *pState = UNPACK_STATE();
        return true;
    }
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!_isContextCurrent(pState, pContext)) return false;
    m_setContexts.remove(pContext);
    *pState = UNPACK_STATE();
    delete pContext;
    return true;
}
