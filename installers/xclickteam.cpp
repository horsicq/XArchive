/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xclickteam.h"
#include "xmaterializedunpackguard.h"

#include <cstring>
#include <memory>
#include <new>
#include <zlib.h>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QQueue>
#include <QScopedPointer>
#include <QScopedValueRollback>
#include <QSet>
#include <QUuid>

#include "xbzip2decoder.h"
#include "xpe.h"

XClickteam::XClickteam(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackLifetimeState = QSharedPointer<LIFETIME_STATE>::create();
    setIsArchive(true);
}

XClickteam::UNPACK_CONTEXT::~UNPACK_CONTEXT()
{
    delete pSourceGuard;
    for (XMaterializedUnpackGuard *pGuard : listCompanionGuards) delete pGuard;
}

XClickteam::~XClickteam()
{
    QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (pLifetimeState) pLifetimeState->bOwnerAlive = false;
    m_pUnpackLifetimeState.clear();
    if (pLifetimeState && !pLifetimeState->bOperationInProgress) {
        const QSet<UNPACK_CONTEXT *> setContextsCopy = pLifetimeState->setContexts;
        pLifetimeState->setContexts.clear();
        for (UNPACK_CONTEXT *pContext : setContextsCopy) delete pContext;
    }
}

XClickteam::LIFETIME_STATE::~LIFETIME_STATE()
{
    const QSet<UNPACK_CONTEXT *> setContextsCopy = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : setContextsCopy) delete pContext;
}

bool XClickteam::isDeviceReplacementAllowed() const
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    return pLifetimeState && pLifetimeState->bOwnerAlive && !pLifetimeState->bOperationInProgress && pLifetimeState->setContexts.isEmpty();
}

bool XClickteam::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XClickteam> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XClickteam::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XClickteam x(pDevice);
    return x.isValid(pPdStruct);
}

XClickteam::INTERNAL_INFO XClickteam::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XClickteam::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XClickteam> guardedThis(this);
    const bool bAlreadyHandled = guardedThis->isInternalInfoHandled();
    if (!guardedThis) return false;

    if (!bAlreadyHandled) {
        const quint64 nTransaction = guardedThis->beginInternalInfoTransaction();
        if (!nTransaction) return false;

        // The transaction supplies the recursion sentinel. Keep every
        // source-derived value local until the same binding is revalidated.
        guardedThis->m_internalInfo = INTERNAL_INFO();
        INTERNAL_INFO info = guardedThis->_getInternalInfo(pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }

        const XBinary::_MEMORY_MAP memoryMap = guardedThis->getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        info.memoryMap = memoryMap;

        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        guardedThis->m_internalInfo = info;
        if (!guardedThis->commitInternalInfoTransaction(nTransaction, static_cast<XBinary::INTERNAL_INFO *>(&guardedThis->m_internalInfo))) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
    }

    return true;
}

void *XClickteam::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XClickteam> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XClickteam::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        setIsInternalInfoHandled(true);
        XBinary::setInternalInfo(static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        setIsInternalInfoHandled(false);
        XBinary::setInternalInfo(nullptr);
    }
}

XBinary::FT XClickteam::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_CLICKTEAM;
    }

    return FT_PE32_CLICKTEAM;
}

static inline quint32 ctRd32(const quint8 *p);
static const qint64 CT_MAX_CONTAINER_SIZE = 512ll << 20;
static const qint64 CT_MAX_FILE_SIZE = 256ll << 20;
static const qint64 CT_MAX_TOTAL_OUTPUT = 512ll << 20;
static const qint32 CT_MAX_FILE_COUNT = 65536;
static const qint32 CT_MAX_DIRECTORY_ENTRIES = 100000;

XClickteam::INTERNAL_INFO XClickteam::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nContainerOffset = -1;

    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;

    const qint64 nSize = getSize();
    qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if ((nOverlayOffset <= 0) || (nOverlayOffset >= nSize)) return result;

    // Authenticode data is appended after the installer container. It is not
    // part of Clickteam's chunk table.
    qint64 nContainerEnd = nSize;
    XBinary::OFFSETSIZE osSignature = pe.getSignOffsetSize();
    if ((osSignature.nOffset > nOverlayOffset) && (osSignature.nSize > 0) && (osSignature.nOffset <= nSize) && (osSignature.nSize == nSize - osSignature.nOffset)) {
        nContainerEnd = osSignature.nOffset;
    }
    const qint64 nContainerSize = nContainerEnd - nOverlayOffset;
    if ((nContainerSize < 18) || (nContainerSize > CT_MAX_CONTAINER_SIZE)) return result;

    // "wwgT)" tag at the overlay start (Install Creator 2 payload container).
    QByteArray baHead = read_array_process(nOverlayOffset, qMin<qint64>(19, nContainerSize), pPdStruct);
    if ((baHead.size() < 18) || (baHead.left(5) != QByteArray("\x77\x77\x67\x54\x29", 5))) return result;

    // Authenticate at least the first record boundary (or the exact
    // eight-byte declaration used by separate-data builds). This prevents an
    // arbitrary PE overlay beginning with the five-byte tag from detecting.
    const quint8 *p = reinterpret_cast<const quint8 *>(baHead.constData());
    if (nContainerSize == 18) {
        if ((ctRd32(p + 10) == 0) || (ctRd32(p + 14) != 0)) return result;
    } else {
        if (baHead.size() < 19) return result;
        quint32 nCompressedSize = ctRd32(p + 10);
        quint32 nUncompressedSize = ctRd32(p + 14);
        quint8 nMethod = p[18];
        if ((nCompressedSize <= 1) || ((qint64)nCompressedSize > nContainerSize - 18) || (nUncompressedSize == 0) || ((nMethod != 1) && (nMethod != 2))) {
            return result;
        }
    }

    result.bIsValid = true;
    result.sVersion = pe.getFileVersion().trimmed();
    result.nContainerOffset = nOverlayOffset;

    return result;
}

// ---------------------------------------------------------------------------
// extraction (zlib chunks; installed files live in the last "compound" chunk)
// ---------------------------------------------------------------------------

static inline quint32 ctRd32(const quint8 *p)
{
    return (quint32)(p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24));
}

static bool ctIsSafeBaseName(const QString &sName)
{
    if (sName.isEmpty() || (sName.size() > 255) || (sName == ".") || (sName == "..") || sName.endsWith(' ') || sName.endsWith('.')) {
        return false;
    }

    static const QString sForbidden = QStringLiteral("<>:\"/\\|?*");
    for (QChar character : sName) {
        if (sForbidden.contains(character) || !character.isPrint()) return false;
    }

    QString sStem = sName.section('.', 0, 0).toUpper();
    sStem.replace(QChar(0x00B9), QLatin1Char('1'));
    sStem.replace(QChar(0x00B2), QLatin1Char('2'));
    sStem.replace(QChar(0x00B3), QLatin1Char('3'));
    static const QSet<QString> setReserved = {
        QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),    QStringLiteral("NUL"),     QStringLiteral("COM1"),
        QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),   QStringLiteral("COM5"),    QStringLiteral("COM6"),
        QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),   QStringLiteral("LPT1"),    QStringLiteral("LPT2"),
        QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"),   QStringLiteral("LPT6"),    QStringLiteral("LPT7"),
        QStringLiteral("LPT8"), QStringLiteral("LPT9"), QStringLiteral("CONIN$"), QStringLiteral("CONOUT$"), QStringLiteral("CLOCK$")};
    return !setReserved.contains(sStem);
}

// zlib (78 xx) inflate; reports the number of input bytes consumed.
static bool ctInflate(const quint8 *pSrc, qint64 nSrcLen, qint64 nMaxOutput, QByteArray *pOut, qint64 *pnConsumed, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSrc || !pOut || (nSrcLen <= 0) || (nSrcLen > 0x7FFFFFFF) || (nMaxOutput < 0) || (nMaxOutput > CT_MAX_FILE_SIZE) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    z_stream s;
    memset(&s, 0, sizeof(s));
    if (inflateInit2(&s, 15) != Z_OK) return false;  // 15 = expect a zlib header

    s.next_in = (Bytef *)pSrc;
    s.avail_in = (uInt)nSrcLen;
    pOut->clear();

    const qint32 nBufferSize = 65536;
    std::unique_ptr<char[]> pBuffer(new (std::nothrow) char[nBufferSize]);
    if (!pBuffer) {
        inflateEnd(&s);
        return false;
    }
    bool bOk = false;
    while (XBinary::isPdStructNotCanceled(pPdStruct)) {
        s.next_out = (Bytef *)pBuffer.get();
        s.avail_out = (uInt)nBufferSize;
        int rc = inflate(&s, Z_NO_FLUSH);
        qint64 nProduced = (qint64)nBufferSize - s.avail_out;
        if ((nProduced < 0) || (nProduced > nMaxOutput - pOut->size())) break;
        if (nProduced) pOut->append(pBuffer.get(), (int)nProduced);
        if (rc == Z_STREAM_END) {
            bOk = true;
            break;
        }
        if (rc != Z_OK) break;                                               // Z_DATA_ERROR / Z_BUF_ERROR ...
        if ((s.avail_in == 0) && (s.avail_out == (uInt)nBufferSize)) break;  // no progress
    }

    if (pnConsumed) *pnConsumed = (qint64)s.total_in;
    inflateEnd(&s);
    return bOk && XBinary::isPdStructNotCanceled(pPdStruct);
}

class CTBoundedSink : public QIODevice {
public:
    explicit CTBoundedSink(qint64 nLimit) : m_nLimit(nLimit), m_nWritten(0)
    {
        open(QIODevice::WriteOnly);
    }

    qint64 writtenSize() const
    {
        return m_nWritten;
    }
    bool isSequential() const override
    {
        return true;
    }

protected:
    qint64 readData(char *, qint64) override
    {
        return -1;
    }
    qint64 writeData(const char *, qint64 nSize) override
    {
        if ((nSize < 0) || (nSize > m_nLimit - m_nWritten)) return -1;
        m_nWritten += nSize;
        return nSize;
    }

private:
    qint64 m_nLimit;
    qint64 m_nWritten;
};

static bool ctValidateBzip(const quint8 *pSrc, qint64 nSrcLen, qint64 nExpectedOutput, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSrc || (nSrcLen < 14) || (nSrcLen > CT_MAX_CONTAINER_SIZE) || (nExpectedOutput < 0) || (nExpectedOutput > CT_MAX_FILE_SIZE) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QByteArray baInput(reinterpret_cast<const char *>(pSrc), (int)nSrcLen);
    QBuffer input(&baInput);
    if (!input.open(QIODevice::ReadOnly)) return false;
    CTBoundedSink output(nExpectedOutput);

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &input;
    state.pDeviceOutput = &output;
    state.nInputOffset = 0;
    state.nInputLimit = nSrcLen;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    bool bResult = XBZIP2Decoder::decompress(&state, pPdStruct) && (state.nCountInput == nSrcLen) && (state.nCountOutput == nExpectedOutput) &&
                   (output.writtenSize() == nExpectedOutput) && XBinary::isPdStructNotCanceled(pPdStruct);
    input.close();
    output.close();
    return bResult;
}

static bool ctAppendFile(XClickteam::UNPACK_CONTEXT *pContext, const QByteArray &baData)
{
    if (!pContext || (baData.size() > CT_MAX_FILE_SIZE) || (pContext->listEntries.size() >= CT_MAX_FILE_COUNT) ||
        ((qint64)baData.size() > CT_MAX_TOTAL_OUTPUT - pContext->nTotalOutput)) {
        return false;
    }

    XClickteam::FILE_ENTRY e;
    e.sName = QString("file_%1").arg(pContext->listEntries.size(), 4, 10, QChar('0'));
    e.baData = baData;
    pContext->listEntries.append(e);
    pContext->nTotalOutput += baData.size();
    return true;
}

static bool ctApplyTocNames(XClickteam::UNPACK_CONTEXT *pContext, const QByteArray &baToc, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    QHash<quint32, QQueue<QString>> mapNames;
    qint32 nNameCount = 0;
    const quint8 *p = (const quint8 *)baToc.constData();
    const qint64 n = baToc.size();

    // A packaged-file descriptor keeps its uncompressed size 40 bytes before
    // the final NUL-terminated file name. This excludes the preceding
    // uninstaller/product strings, which do not use that layout.
    for (qint64 i = 40; i < n;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        qint64 j = i;
        while ((j < n) && (p[j] >= 0x20) && (p[j] < 0x7F) && (j - i < 260)) j++;
        if ((j > i) && (j < n) && (p[j] == 0)) {
            QByteArray baName((const char *)p + i, (int)(j - i));
            quint32 nSize = ctRd32(p + i - 40);
            const QString sName = QString::fromLatin1(baName);
            if (ctIsSafeBaseName(sName) && (nSize <= (256U << 20))) {
                if (nNameCount >= CT_MAX_FILE_COUNT) return false;
                mapNames[nSize].enqueue(sName);
                nNameCount++;
            }
            i = j + 1;
        } else {
            i++;
        }
    }

    // Reserve every generic fallback before applying metadata. Otherwise a
    // real TOC name such as "file_0001" could collide with another entry's
    // fallback name.
    QSet<QString> setUsedNames;
    for (int i = 0; i < pContext->listEntries.size(); i++) {
        setUsedNames.insert(pContext->listEntries.at(i).sName.toCaseFolded());
    }
    for (int i = 0; i < pContext->listEntries.size(); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const QString sOldNameKey = pContext->listEntries.at(i).sName.toCaseFolded();
        setUsedNames.remove(sOldNameKey);
        QHash<quint32, QQueue<QString>>::iterator it = mapNames.find((quint32)pContext->listEntries[i].baData.size());
        if ((it != mapNames.end()) && !it.value().isEmpty()) {
            const QString sName = it.value().dequeue();
            const QString sNameKey = sName.toCaseFolded();
            if (!setUsedNames.contains(sNameKey)) {
                pContext->listEntries[i].sName = sName;
            }
        }
        setUsedNames.insert(pContext->listEntries.at(i).sName.toCaseFolded());
    }

    return true;
}

static QString ctDeviceFileName(QIODevice *pDevice)
{
    QPointer<QIODevice> guardedDevice(pDevice);
    if (!guardedDevice) return QString();
    const bool bSourceIdentityBound = guardedDevice->property("XStaticUnpacker.SourceIdentityBound").toBool();
    if (!guardedDevice) return QString();
    const QString sSourceFileName = guardedDevice->property("XStaticUnpacker.SourceFileName").toString();
    if (!guardedDevice) return QString();
    if (bSourceIdentityBound || !sSourceFileName.isEmpty()) {
        return sSourceFileName;
    }

    QFile *pFile = dynamic_cast<QFile *>(guardedDevice.data());
    QPointer<QFile> guardedFile(pFile);
    if (!guardedDevice || !guardedFile) return QString();
    const QString sResult = guardedFile->fileName();
    return (guardedDevice && guardedFile) ? sResult : QString();
}

static bool ctReadSeparateVolume(QIODevice *pDevice, qint64 nDeclaredRegionSize, XClickteam::UNPACK_CONTEXT *pContext, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pContext || (nDeclaredRegionSize <= 0) || (nDeclaredRegionSize > CT_MAX_CONTAINER_SIZE - 4) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<QIODevice> guardedInputDevice(pDevice);
    if (!guardedInputDevice) return false;
    const QString sInputFileName = ctDeviceFileName(guardedInputDevice.data());
    if (!guardedInputDevice) return false;
    if (sInputFileName.isEmpty()) return false;

    QFileInfo inputInfo(sInputFileName);
    if (inputInfo.fileName().isEmpty()) return false;

    QString sCanonicalDirectory = QFileInfo(inputInfo.absolutePath()).canonicalFilePath();
    if (sCanonicalDirectory.isEmpty()) return false;
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif

    // Resolve the declared sibling through a bounded directory enumeration.
    // This makes case-folding deterministic and rejects ambiguous aliases
    // instead of letting QFile choose an arbitrary directory entry.
    const QString sExpectedName = inputInfo.completeBaseName() + ".D01";
    QDir inputDirectory(inputInfo.absolutePath());
    const QFileInfoList listDirectoryEntries = inputDirectory.entryInfoList(QDir::Files | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name);
    if (listDirectoryEntries.size() > CT_MAX_DIRECTORY_ENTRIES) return false;

    QFileInfo volumeInfo;
    qint32 nMatchingEntries = 0;
    for (const QFileInfo &candidate : listDirectoryEntries) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (candidate.fileName().compare(sExpectedName, pathCaseSensitivity) != 0) continue;
        nMatchingEntries++;
        if (nMatchingEntries > 1) return false;
        volumeInfo = candidate;
    }
    if ((nMatchingEntries != 1) || !volumeInfo.exists() || !volumeInfo.isFile() || !volumeInfo.isReadable() || volumeInfo.isSymLink()) return false;

    QString sCanonicalVolume = volumeInfo.canonicalFilePath();
    QString sCanonicalInput = inputInfo.canonicalFilePath();
    if (sCanonicalVolume.isEmpty() || (QFileInfo(sCanonicalVolume).absolutePath().compare(sCanonicalDirectory, pathCaseSensitivity) != 0) ||
        (!sCanonicalInput.isEmpty() && (sCanonicalVolume.compare(sCanonicalInput, pathCaseSensitivity) == 0))) {
        return false;
    }

    qint64 nVolumeSize = volumeInfo.size();
    if ((nVolumeSize != nDeclaredRegionSize + 4) || (nVolumeSize < 5) || (nVolumeSize > CT_MAX_CONTAINER_SIZE)) return false;

    std::unique_ptr<XMaterializedUnpackGuard> pVolumeGuard(XMaterializedUnpackGuard::openFile(sCanonicalVolume, pPdStruct));
    QPointer<QIODevice> guardedVolumeDevice(pVolumeGuard ? pVolumeGuard->device() : nullptr);
    QFile *pVolumeFile = guardedVolumeDevice ? dynamic_cast<QFile *>(guardedVolumeDevice.data()) : nullptr;
    QPointer<QFile> guardedVolumeFile(pVolumeFile);
    if (!guardedVolumeDevice || !guardedVolumeFile) return false;
    const qint64 nObservedVolumeSize = guardedVolumeFile->size();
    if (!guardedVolumeDevice || !guardedVolumeFile || (nObservedVolumeSize != nVolumeSize)) return false;
    QByteArray baVolume;
    baVolume.reserve((int)nVolumeSize);
    while ((qint64)baVolume.size() < nVolumeSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        qint64 nToRead = qMin(Q_INT64_C(1024) * 1024, nVolumeSize - (qint64)baVolume.size());
        if (!guardedVolumeDevice || !guardedVolumeFile) return false;
        QByteArray baChunk = guardedVolumeFile->read(nToRead);
        if (!guardedVolumeDevice || !guardedVolumeFile || baChunk.isEmpty() || (baChunk.size() > nToRead)) return false;
        baVolume.append(baChunk);
    }
    if (!guardedVolumeDevice || !guardedVolumeFile) return false;
    const bool bAtEnd = guardedVolumeFile->atEnd();
    if (!guardedVolumeDevice || !guardedVolumeFile) return false;
    const qint64 nFinalVolumeSize = guardedVolumeFile->size();
    const bool bExactRead = guardedVolumeDevice && guardedVolumeFile && (baVolume.size() == nVolumeSize) && bAtEnd && (nFinalVolumeSize == nVolumeSize);
    if (!bExactRead || !pVolumeGuard->validateAndFinalize(pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const quint8 *p = (const quint8 *)baVolume.constData();
    qint64 nRegionSize = ctRd32(p);
    if ((nRegionSize != nDeclaredRegionSize) || (nRegionSize != baVolume.size() - 4)) return false;

    qint64 q = 4;
    const qint64 nEnd = 4 + nRegionSize;
    bool bFirst = true;  // uninstall string table
    qint32 nStreams = 0;
    while (q < nEnd) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        quint8 nMethod = p[q];
        if (nMethod != 1) return false;
        QByteArray baFile;
        qint64 nConsumed = 0;
        qint64 nRemainingOutput = CT_MAX_TOTAL_OUTPUT - pContext->nTotalOutput;
        if ((nRemainingOutput < 0) || !ctInflate(p + q + 1, nEnd - (q + 1), qMin(CT_MAX_FILE_SIZE, nRemainingOutput), &baFile, &nConsumed, pPdStruct) ||
            (nConsumed <= 0) || (nConsumed > nEnd - (q + 1))) {
            return false;
        }
        if (!bFirst && !ctAppendFile(pContext, baFile)) return false;
        bFirst = false;
        nStreams++;
        q += 1 + nConsumed;
    }

    const bool bResult = (q == nEnd) && (nStreams >= 2) && !pContext->listEntries.isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct);
    if (!bResult) return false;
    if (pContext->listCompanionGuards.size() >= CT_MAX_FILE_COUNT) return false;
    pContext->listCompanionGuards.reserve(pContext->listCompanionGuards.size() + 1);
    // Keep ownership until QList has accepted the pointer.  append() can
    // allocate while detaching/growing and must not leak the open guard.
    pContext->listCompanionGuards.append(pVolumeGuard.get());
    pVolumeGuard.release();
    return true;
}

bool XClickteam::_buildEntries(UNPACK_CONTEXT *pContext, qint64 nContainerOffset, PDSTRUCT *pPdStruct)
{
    if (!pContext || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    qint64 nContainerEnd = getSize();
    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return false;
    XBinary::OFFSETSIZE osSignature = pe.getSignOffsetSize();
    if ((osSignature.nOffset > nContainerOffset) && (osSignature.nSize > 0) && (osSignature.nOffset <= getSize()) &&
        (osSignature.nSize == getSize() - osSignature.nOffset)) {
        nContainerEnd = osSignature.nOffset;
    }
    qint64 nTail = nContainerEnd - nContainerOffset;
    if ((nTail < 10) || (nTail > CT_MAX_CONTAINER_SIZE)) return false;
    QByteArray baOv = read_array_process(nContainerOffset, nTail, pPdStruct);
    if ((baOv.size() != nTail) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    const quint8 *p = (const quint8 *)baOv.constData();
    const qint64 n = baOv.size();
    if (n < 10) return false;

    // records begin after the 5-byte "wwgT)" tag + 5-byte sub-header.
    QByteArray baToc;
    bool bCompoundFound = false;
    qint64 nDeclaredSeparateRegionSize = -1;
    qint64 nValidatedResourceOutput = 0;
    qint64 pos = 10;
    while (pos < n) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (pos + 9 > n) {
            // Separate-data builds end with {u32 D01 region size, u32 0}.
            // The exact declaration is authenticated again against the
            // sibling file's own length prefix before any stream is decoded.
            if ((n - pos != 8) || (ctRd32(p + pos + 4) != 0)) return false;
            nDeclaredSeparateRegionSize = ctRd32(p + pos);
            if ((nDeclaredSeparateRegionSize <= 0) || (nDeclaredSeparateRegionSize > CT_MAX_CONTAINER_SIZE - 4)) return false;
            pos = n;
            break;
        }

        quint32 nCompSize = ctRd32(p + pos);
        quint32 nUncompressedSize = ctRd32(p + pos + 4);
        quint8 nMethod = p[pos + 8];
        if ((nMethod != 1) && (nMethod != 2)) return false;

        qint64 nStreamStart = pos + 9;
        qint64 nRegionEnd = pos + 8 + (qint64)nCompSize;
        if ((nRegionEnd <= nStreamStart) || (nRegionEnd > n) || (nUncompressedSize == 0) || (nUncompressedSize > CT_MAX_FILE_SIZE)) return false;

        if (nMethod == 1) {
            QByteArray baOut;
            qint64 nConsumed = 0;
            qint64 nRegionInputSize = nRegionEnd - nStreamStart;
            if (!ctInflate(p + nStreamStart, nRegionInputSize, nUncompressedSize, &baOut, &nConsumed, pPdStruct) || (nConsumed <= 0) || (nConsumed > nRegionInputSize)) {
                return false;
            }

            bool bHasTocMarker = baOut.contains(QByteArray("Uninstal.exe\0", 13));
            qint64 nLeftover = nRegionInputSize - nConsumed;
            if (nLeftover == 4) {
                // Ordinary top-level records end in a four-byte trailer.
                if ((quint32)baOut.size() != nUncompressedSize) return false;
                if (bHasTocMarker) baToc = baOut;
            } else if (nLeftover > 4) {
                // The final compound region replaces the normal trailer with a
                // sequence of bare [method][zlib stream] installed files.
                if (baToc.isEmpty() || bCompoundFound || (nRegionEnd != n) || (nUncompressedSize != nCompSize)) return false;

                UNPACK_CONTEXT compoundContext;
                qint64 q = nStreamStart + nConsumed;
                while (q < nRegionEnd) {
                    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (p[q] != 1)) return false;

                    QByteArray baFile;
                    qint64 nFileConsumed = 0;
                    qint64 nAvailable = nRegionEnd - (q + 1);
                    qint64 nRemainingOutput = CT_MAX_TOTAL_OUTPUT - compoundContext.nTotalOutput;
                    if ((nRemainingOutput < 0) || !ctInflate(p + q + 1, nAvailable, qMin(CT_MAX_FILE_SIZE, nRemainingOutput), &baFile, &nFileConsumed, pPdStruct) ||
                        (nFileConsumed <= 0) || (nFileConsumed > nAvailable) || !ctAppendFile(&compoundContext, baFile)) {
                        return false;
                    }
                    q += 1 + nFileConsumed;
                }

                if ((q != nRegionEnd) || compoundContext.listEntries.isEmpty()) return false;
                if ((qint64)compoundContext.nTotalOutput > CT_MAX_TOTAL_OUTPUT - pContext->nTotalOutput ||
                    (pContext->listEntries.size() > CT_MAX_FILE_COUNT - compoundContext.listEntries.size())) {
                    return false;
                }

                pContext->listEntries.append(compoundContext.listEntries);
                pContext->nTotalOutput += compoundContext.nTotalOutput;
                bCompoundFound = true;
            } else {
                return false;
            }
        } else {
            // Method 2 resources are not exposed as installed files, but they
            // are still authenticated. Accepting an unchecked BZip2 body made
            // corruption in a skipped UI chunk invisible to the extractor.
            qint64 nCompressedPayload = nRegionEnd - nStreamStart - 4;
            if ((nCompressedPayload < 14) || ((qint64)nUncompressedSize > CT_MAX_TOTAL_OUTPUT - nValidatedResourceOutput) ||
                !ctValidateBzip(p + nStreamStart, nCompressedPayload, nUncompressedSize, pPdStruct)) {
                return false;
            }
            nValidatedResourceOutput += nUncompressedSize;
        }

        pos = nRegionEnd;
    }

    if (pos != n) return false;
    if (pContext->listEntries.isEmpty()) {
        if (!ctReadSeparateVolume(getDevice(), nDeclaredSeparateRegionSize, pContext, pPdStruct)) return false;
    } else if (nDeclaredSeparateRegionSize != -1) {
        return false;
    }
    if (!baToc.isEmpty() && !ctApplyTocNames(pContext, baToc, pPdStruct)) return false;

    return !pContext->listEntries.isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XClickteam::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    const PDSTRUCTLIFETIME progressLifetime = pPdStruct ? retainPdStructLifetime(pPdStruct) : PDSTRUCTLIFETIME();
    struct PROGRESS_ALIVE_PROBE {
        PDSTRUCT *pPdStruct;
        const PDSTRUCTLIFETIME *pProgressLifetime;
        bool operator()() const { return !pPdStruct || XBinary::isPdStructLifetimeAlive(*pProgressLifetime); }
    };
    const PROGRESS_ALIVE_PROBE isProgressAlive = {pPdStruct, &progressLifetime};
    if (!isProgressAlive()) return false;
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XClickteam> guardedThis(this);
    if (pState->pContext || !pState->baUnpackSourceToken.isEmpty()) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pOldContext || !pLifetimeState->setContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState) ||
            (pOldContext->baToken != pState->baUnpackSourceToken))
            return false;
        pLifetimeState->setContexts.remove(pOldContext);
        *pState = UNPACK_STATE();
        delete pOldContext;
    } else {
        *pState = UNPACK_STATE();
    }
    if (!isProgressAlive() || !guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<QIODevice> guardedSource(getDevice());
    const quint64 nGeneration = getDeviceGeneration();
    const bool bIsImage = isImage();
    const XADDR nModuleAddress = getModuleAddress();
    if (!guardedSource) return false;
    const qint64 nSourceSize = guardedSource->size();
    if (!isProgressAlive() || !guardedThis || !guardedSource || (nSourceSize < 0) || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
        return false;
    std::unique_ptr<XMaterializedUnpackGuard> pSourceGuard(XMaterializedUnpackGuard::bind(guardedSource.data(), pPdStruct));
    if (!isProgressAlive() || !pSourceGuard || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
        return false;
    if (!m_bTrustedSnapshot) {
        QScopedPointer<QIODevice> pSnapshot(createFileBuffer(nSourceSize, pPdStruct));
        if (!isProgressAlive() || !guardedThis || !guardedSource || !pSnapshot || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
            return false;
        const QString sSourceFileName = ctDeviceFileName(guardedSource.data());
        if (!isProgressAlive() || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data())) return false;
        pSnapshot->setProperty("XStaticUnpacker.SourceIdentityBound", true);
        if (!sSourceFileName.isEmpty()) pSnapshot->setProperty("XStaticUnpacker.SourceFileName", sSourceFileName);
        const bool bCopied = copyDeviceMemory(guardedSource.data(), 0, pSnapshot.data(), 0, nSourceSize, pPdStruct);
        if (!isProgressAlive() || !bCopied || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
            return false;
        XClickteam worker(pSnapshot.data(), bIsImage, nModuleAddress);
        worker.m_bTrustedSnapshot = true;
        UNPACK_STATE materializedState = {};
        const bool bMaterialized = worker.initUnpack(&materializedState, mapProperties, pPdStruct);
        if (!isProgressAlive() || !guardedThis || !guardedSource || !bMaterialized || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
            return false;
        UNPACK_CONTEXT *pMaterializedContext = static_cast<UNPACK_CONTEXT *>(materializedState.pContext);
        if (!pMaterializedContext) return false;
        std::unique_ptr<UNPACK_CONTEXT> pContext(new (std::nothrow) UNPACK_CONTEXT);
        if (!pContext) return false;
        pContext->listEntries = pMaterializedContext->listEntries;
        pContext->nTotalOutput = pMaterializedContext->nTotalOutput;
        pContext->listCompanionGuards.swap(pMaterializedContext->listCompanionGuards);
        if (!worker.finishUnpack(&materializedState, nullptr) || !isProgressAlive() || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) ||
            (getDevice() != guardedSource.data()) || !isPdStructNotCanceled(pPdStruct) || pContext->listEntries.isEmpty())
            return false;
        pContext->pSourceDevice = guardedSource;
        pContext->pOwnerState = pState;
        pContext->baToken = QUuid::createUuid().toRfc4122();
        pContext->nDeviceGeneration = nGeneration;
        pContext->nSourceSize = nSourceSize;
        if (pContext->baToken.isEmpty()) return false;
        const bool bSourceFinal = pSourceGuard->validateAndFinalize(pPdStruct);
        const bool bCompanionsCurrent = isProgressAlive() && XMaterializedUnpackGuard::areCurrent(pSourceGuard.get(), pContext->listCompanionGuards, pPdStruct);
        if (!isProgressAlive() || !bSourceFinal || !bCompanionsCurrent || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) ||
            (getDevice() != guardedSource.data()) || !isPdStructNotCanceled(pPdStruct))
            return false;
        pContext->pSourceGuard = pSourceGuard.release();
        pState->nTotalSize = nSourceSize;
        pState->nNumberOfRecords = pContext->listEntries.size();
        pState->mapUnpackProperties = mapProperties;
        pState->pContext = pContext.get();
        pState->baUnpackSourceToken = pContext->baToken;
        pLifetimeState->setContexts.insert(pContext.release());
        return guardedThis && pLifetimeState->bOwnerAlive;
    }
    pState->nTotalSize = nSourceSize;
    pState->mapUnpackProperties = mapProperties;

    INTERNAL_INFO info = _detect(pPdStruct);
    if (!isProgressAlive() || !guardedThis || !guardedSource || !info.bIsValid || (info.nContainerOffset < 0)) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) return false;
    const bool bBuilt = _buildEntries(pContext, info.nContainerOffset, pPdStruct);
    if (!isProgressAlive() || !guardedThis || !guardedSource || !bBuilt) {
        delete pContext;
        return false;
    }

    pContext->pSourceDevice = guardedSource;
    pContext->pOwnerState = pState;
    pContext->baToken = QUuid::createUuid().toRfc4122();
    pContext->nDeviceGeneration = nGeneration;
    pContext->nSourceSize = nSourceSize;
    if (pContext->baToken.isEmpty()) {
        delete pContext;
        return false;
    }
    const bool bSourceFinal = pSourceGuard->validateAndFinalize(pPdStruct);
    const bool bCompanionsCurrent = isProgressAlive() && XMaterializedUnpackGuard::areCurrent(pSourceGuard.get(), pContext->listCompanionGuards, pPdStruct);
    if (!isProgressAlive() || !bSourceFinal || !bCompanionsCurrent || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) ||
        (getDevice() != guardedSource.data()) || !isPdStructNotCanceled(pPdStruct)) {
        delete pContext;
        return false;
    }
    pContext->pSourceGuard = pSourceGuard.release();
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;
    pState->baUnpackSourceToken = pContext->baToken;
    pLifetimeState->setContexts.insert(pContext);
    return guardedThis && pLifetimeState->bOwnerAlive;
}

XBinary::ARCHIVERECORD XClickteam::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return result;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XClickteam> guardedThis(this);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    qint32 nIndex = pState->nCurrentIndex;
    if (!pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken) ||
        (pContext->nDeviceGeneration != getDeviceGeneration()) || (pContext->pSourceDevice.data() != getDevice()) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) || (nIndex != pContext->nCurrentIndex) || (pState->nNumberOfRecords != pContext->listEntries.size()) ||
        (pState->nTotalSize != pContext->nSourceSize) || (nIndex < 0) || (nIndex >= pContext->listEntries.size()))
        return result;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !guardedThis || !pLifetimeState->bOwnerAlive ||
        !pLifetimeState->setContexts.contains(pContext) || (pState->pContext != pContext) || (pContext->pOwnerState != pState) ||
        (pContext->baToken != pState->baUnpackSourceToken) || (pState->nCurrentIndex != pContext->nCurrentIndex))
        return result;

    const FILE_ENTRY &e = pContext->listEntries.at(nIndex);
    result.nStreamSize = e.baData.size();
    result.mapProperties[FPART_PROP_ORIGINALNAME] = e.sName;
    result.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)e.baData.size();
    result.mapProperties[FPART_PROP_ISFOLDER] = false;
    return guardedThis ? result : ARCHIVERECORD();
}

// Named functor replacing the former isAuthenticated capture-lambda in unpackCurrent().
struct CT_UNPACK_AUTH {
    const QPointer<XClickteam> &guardedThis;
    const QSharedPointer<XClickteam::LIFETIME_STATE> &pLifetimeState;
    XBinary::UNPACK_STATE *pState;
    XClickteam::UNPACK_CONTEXT *pContext;
    qint32 nIndex;

    bool operator()() const
    {
        return guardedThis && pLifetimeState->bOwnerAlive && pLifetimeState->setContexts.contains(pContext) && (pState->pContext == pContext) &&
               (pContext->pOwnerState == pState) && (pState->baUnpackSourceToken == pContext->baToken) &&
               (pContext->nDeviceGeneration == guardedThis->getDeviceGeneration()) && (pContext->pSourceDevice.data() == guardedThis->getDevice()) &&
               (pState->nCurrentIndex == pContext->nCurrentIndex) && (pState->nCurrentOffset == pContext->nCurrentOffset) &&
               (pState->nNumberOfRecords == pContext->listEntries.size()) && (pState->nTotalSize == pContext->nSourceSize) && (nIndex >= 0) &&
               (nIndex < pContext->listEntries.size());
    }
};

bool XClickteam::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XClickteam> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !guardedOutput || !isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint32 nIndex = pState->nCurrentIndex;
    const CT_UNPACK_AUTH isAuthenticated = {guardedThis, pLifetimeState, pState, pContext, nIndex};
    if (!isAuthenticated()) return false;
    const bool bOpen = guardedOutput->isOpen();
    if (!isAuthenticated() || !guardedOutput || !bOpen) return false;
    const bool bWritable = guardedOutput->isWritable();
    if (!isAuthenticated() || !guardedOutput || !bWritable) return false;
    const bool bSequential = guardedOutput->isSequential();
    if (!isAuthenticated() || !guardedOutput || bSequential) return false;
    const QIODevice::OpenMode openMode = guardedOutput->openMode();
    if (!isAuthenticated() || !guardedOutput || (openMode & (QIODevice::Append | QIODevice::Text)) || !isResizeEnable(guardedOutput.data()) || !guardedOutput ||
        devicesAlias(pContext->pSourceDevice.data(), guardedOutput.data()) || !isAuthenticated() || !guardedOutput)
        return false;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !guardedOutput || !isAuthenticated()) return false;
    // This override bypasses the base decode chain's per-entry gate; account the member here.
    // Produced bytes are charged by writeUnpackData at publication below.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, pContext->listEntries.at(nIndex).sName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }
    const QByteArray baData = pContext->listEntries.at(nIndex).baData;
    QScopedPointer<QIODevice> pStage(createFileBuffer(baData.size(), pPdStruct));
    QPointer<QIODevice> guardedStage(pStage.data());
    if (!isAuthenticated() || !guardedOutput || !guardedStage) return false;
    UNPACK_STATE writeState = *pState;
    writeState.pContext = nullptr;
    writeState.baUnpackSourceToken.clear();
    // The stage copy re-writes bytes charged again at publication below;
    // detach the budget here so each produced member is charged exactly once.
    writeState.spOutputBudget.clear();
    if (!writeUnpackData(&writeState, guardedStage.data(), baData, pPdStruct) || !guardedStage || !guardedOutput || !isAuthenticated()) return false;
    writeState.nCurrentOffset = 0;
    writeState.spOutputBudget = pState->spOutputBudget;
    const bool bPublished = writeUnpackData(&writeState, guardedOutput.data(), baData, pPdStruct);
    const bool bSourceCurrent = bPublished && XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct);
    const bool bFinal = bSourceCurrent && guardedOutput && isAuthenticated() && isPdStructNotCanceled(pPdStruct);
    if (!bFinal) {
        if (bPublished && guardedOutput) {
            resize(guardedOutput.data(), 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }
    pContext->nCurrentOffset = writeState.nCurrentOffset;
    pState->nCurrentOffset = writeState.nCurrentOffset;
    return true;
}

bool XClickteam::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XClickteam> guardedThis(this);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken) ||
        (pContext->nDeviceGeneration != getDeviceGeneration()) || (pContext->pSourceDevice.data() != getDevice()) || (pState->nCurrentIndex != pContext->nCurrentIndex) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) || (pState->nNumberOfRecords != pContext->listEntries.size()) ||
        (pState->nTotalSize != pContext->nSourceSize) || (pContext->nCurrentIndex < 0) || (pContext->nCurrentIndex >= pContext->listEntries.size()))
        return false;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !guardedThis || !pLifetimeState->bOwnerAlive ||
        !pLifetimeState->setContexts.contains(pContext) || (pState->pContext != pContext) || (pContext->pOwnerState != pState) ||
        (pContext->baToken != pState->baUnpackSourceToken) || (pContext->nCurrentIndex >= pContext->listEntries.size()))
        return false;
    ++pContext->nCurrentIndex;
    pContext->nCurrentOffset = 0;
    pState->nCurrentIndex = pContext->nCurrentIndex;
    pState->nCurrentOffset = 0;
    return (pContext->nCurrentIndex < pContext->listEntries.size());
}

bool XClickteam::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState) return false;
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    if (!pState->pContext && pState->baUnpackSourceToken.isEmpty()) {
        *pState = UNPACK_STATE();
        return true;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken))
        return false;
    pLifetimeState->setContexts.remove(pContext);
    *pState = UNPACK_STATE();
    delete pContext;
    return true;
}
