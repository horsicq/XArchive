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

bool XClickteam::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(getInternalInfo(pPdStruct));
    return pInfo && pInfo->bIsValid;
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
    const bool bAlreadyHandled = isInternalInfoHandled();
    if (!bAlreadyHandled) {
        const quint64 nTransaction = beginInternalInfoTransaction();
        if (!nTransaction) return false;

        // The transaction supplies the recursion sentinel. Keep every
        // source-derived value local until the same binding is revalidated.
        m_internalInfo = INTERNAL_INFO();
        INTERNAL_INFO info = _getInternalInfo(pPdStruct);
        if (!isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }

        const XBinary::_MEMORY_MAP memoryMap = getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!isInternalInfoTransactionCurrent(nTransaction) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        info.memoryMap = memoryMap;

        if (!isInternalInfoTransactionCurrent(nTransaction)) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        m_internalInfo = info;
        if (!commitInternalInfoTransaction(nTransaction, static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo))) {
            rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
    }

    return true;
}

void *XClickteam::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
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
static inline quint16 ctRd16(const quint8 *p);
static const qint64 CT_MAX_CONTAINER_SIZE = 512ll << 20;
static const qint64 CT_MAX_FILE_SIZE = 256ll << 20;
static const qint64 CT_MAX_TOTAL_OUTPUT = 512ll << 20;
static const qint32 CT_MAX_FILE_COUNT = 65536;
static const qint32 CT_MAX_DIRECTORY_ENTRIES = 100000;

// Install Creator 1.x ("legacy") container. Its overlay opens directly with the
// first chunk header - there is no "wwgT)H" tag in front of it.
static const quint16 CT_LEGACY_TAG_1239 = 0x1239;
static const quint16 CT_LEGACY_TAG_1241 = 0x1241;
static const quint16 CT_LEGACY_TAG_1242 = 0x1242;
static const quint16 CT_RECORD_TAG_END = 0x7F7F;
static const qint64 CT_LEGACY_MAX_CHUNK = 0x1000000;
static const qint32 CT_LEGACY_ENTRY_HEADER = 0x1A;
static const qint32 CT_LEGACY_MAX_ENTRY = 0x4000;
static const qint32 CT_LEGACY_MAX_BLOCKS = 1 << 22;

// Multimedia Fusion 2 stand-alone build. Same vendor, a different container:
// the overlay opens with {77 77 77 77 49 87 47 12}, a 0x20-byte header size
// and a dword file count at +0x1c, and every packed runtime file is
// {u16 nameLength, name, [u32 uncompressedSize,] u32 packedSize, packed bytes}.
// Everything behind the last file is the application's own ".ccn" chunk stream.
static const quint32 CT_MMF2_MAGIC1 = 0x77777777;
static const quint32 CT_MMF2_MAGIC2 = 0x12478749;
static const quint32 CT_MMF2_HEADER_SIZE = 0x20;
static const qint32 CT_MMF2_MAX_NAME = 512;

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

    // Multimedia Fusion 2 runtime pack. It is not an Install Creator container
    // and carries no "wwgT)" tag, so it has to be recognised before the tag
    // test below - otherwise the carrier falls through to the Install Creator
    // 1.x reading, which rejects it.
    if (nContainerSize >= (qint64)CT_MMF2_HEADER_SIZE) {
        QByteArray baPack = read_array_process(nOverlayOffset, (qint64)CT_MMF2_HEADER_SIZE, pPdStruct);
        if (baPack.size() == (int)CT_MMF2_HEADER_SIZE) {
            const quint8 *pPack = reinterpret_cast<const quint8 *>(baPack.constData());
            const qint32 nPackedFileCount = (qint32)ctRd32(pPack + 28);
            if ((ctRd32(pPack) == CT_MMF2_MAGIC1) && (ctRd32(pPack + 4) == CT_MMF2_MAGIC2) && (ctRd32(pPack + 8) == CT_MMF2_HEADER_SIZE) &&
                (ctRd32(pPack + 20) == 0) && (ctRd32(pPack + 24) == 0) && (nPackedFileCount > 0) && (nPackedFileCount <= CT_MAX_FILE_COUNT)) {
                result.bIsValid = true;
                result.sVersion = pe.getFileVersion().trimmed();
                result.nContainerOffset = nOverlayOffset;

                return result;
            }
        }
    }

    // "wwgT)" tag at the overlay start (Install Creator 2 payload container).
    QByteArray baHead = read_array_process(nOverlayOffset, qMin<qint64>(19, nContainerSize), pPdStruct);

    if (baHead.left(5) != QByteArray("\x77\x77\x67\x54\x29", 5)) {
        // Install Creator 1.x carries no tag: the overlay opens with the first
        // chunk header {u16 tag, u16 flags, u32 size, u32 uncompressed} and the
        // Clickteam-Deflate stream that follows always opens on a block type
        // code of 5, 6 or 7.
        if (baHead.size() < 13) return result;
        const quint8 *pLegacy = reinterpret_cast<const quint8 *>(baHead.constData());
        const quint16 nLegacyTag = ctRd16(pLegacy);
        if ((nLegacyTag != CT_LEGACY_TAG_1239) && (nLegacyTag != CT_LEGACY_TAG_1241) && (nLegacyTag != CT_LEGACY_TAG_1242)) return result;
        if (ctRd16(pLegacy + 2) != 1) return result;
        const quint32 nLegacyChunkSize = ctRd32(pLegacy + 4);
        const quint32 nLegacyUncompressed = ctRd32(pLegacy + 8);
        if ((nLegacyChunkSize <= 4) || ((qint64)nLegacyChunkSize > CT_LEGACY_MAX_CHUNK) || ((qint64)nLegacyChunkSize > nContainerSize - 8)) return result;
        if ((nLegacyUncompressed == 0) || ((qint64)nLegacyUncompressed > CT_LEGACY_MAX_CHUNK)) return result;
        if ((pLegacy[12] & 7) < 5) return result;

        result.bIsValid = true;
        result.sVersion = pe.getFileVersion().trimmed();
        result.nContainerOffset = nOverlayOffset;

        return result;
    }

    if (baHead.size() < 18) return result;

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

static inline quint16 ctRd16(const quint8 *p)
{
    return (quint16)(p[0] | ((quint16)p[1] << 8));
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
    QIODevice *guardedDevice = pDevice;
    if (!guardedDevice) return QString();
    const bool bSourceIdentityBound = guardedDevice->property("XStaticUnpacker.SourceIdentityBound").toBool();
    if (!guardedDevice) return QString();
    const QString sSourceFileName = guardedDevice->property("XStaticUnpacker.SourceFileName").toString();
    if (!guardedDevice) return QString();
    if (bSourceIdentityBound || !sSourceFileName.isEmpty()) {
        return sSourceFileName;
    }

    QFile *pFile = dynamic_cast<QFile *>(guardedDevice);
    QFile *guardedFile = pFile;
    if (!guardedDevice || !guardedFile) return QString();
    const QString sResult = guardedFile->fileName();
    return (guardedDevice && guardedFile) ? sResult : QString();
}

static bool ctReadSeparateVolume(QIODevice *pDevice, qint64 nDeclaredRegionSize, XClickteam::UNPACK_CONTEXT *pContext, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pContext || (nDeclaredRegionSize <= 0) || (nDeclaredRegionSize > CT_MAX_CONTAINER_SIZE - 4) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QIODevice *guardedInputDevice = pDevice;
    if (!guardedInputDevice) return false;
    const QString sInputFileName = ctDeviceFileName(guardedInputDevice);
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
    QIODevice *guardedVolumeDevice = pVolumeGuard ? pVolumeGuard->device() : nullptr;
    QFile *pVolumeFile = guardedVolumeDevice ? dynamic_cast<QFile *>(guardedVolumeDevice) : nullptr;
    QFile *guardedVolumeFile = pVolumeFile;
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

// ---------------------------------------------------------------------------
// Install Creator 1.x payload: Clickteam-Deflate
//
// Raw Deflate whose block header is [3 bit type][1 bit final] instead of
// RFC1951's [1 bit final][2 bit type]; type 5 = fixed Huffman, 6 = dynamic
// Huffman, 7 = stored.  A stored block carries only a 16 bit length (no one's
// complement copy), and a dynamic block reads the code-length alphabet in the
// order {18,17,16,0,1..15} instead of {16,17,18,0,8,7,...}.  Everything else
// (length/distance bases, extra bits, the 16/17/18 repeat codes) is stock
// Deflate, so zlib cannot be used but its tables can.
// ---------------------------------------------------------------------------
static const quint8 g_arrCtLegacyCodeLengthOrder[19] = {18, 17, 16, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static const quint16 g_arrCtLengthBase[29] = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                                              31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
static const quint8 g_arrCtLengthExtra[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
static const quint16 g_arrCtDistanceBase[30] = {1,    2,    3,    4,    5,    7,    9,    13,   17,    25,    33,   49,   65,   97,    129,
                                                193,  257,  385,  513,  769,  1025, 1537, 2049, 3073,  4097,  6145, 8193, 12289, 16385, 24577};
static const quint8 g_arrCtDistanceExtra[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

struct CT_BITSTREAM {
    const quint8 *pData;
    qint64 nSize;
    qint64 nBitPos;
    bool bError;
};

struct CT_HUFFMAN {
    qint32 arrCount[16];
    qint32 arrSymbol[288];
};

static quint32 ctBitsRead(CT_BITSTREAM *pStream, qint32 nCount)
{
    quint32 nResult = 0;

    for (qint32 i = 0; i < nCount; i++) {
        const qint64 nByte = pStream->nBitPos >> 3;
        if (nByte >= pStream->nSize) {
            pStream->bError = true;
            return 0;
        }
        nResult |= (quint32)((pStream->pData[nByte] >> (pStream->nBitPos & 7)) & 1) << i;
        pStream->nBitPos++;
    }

    return nResult;
}

static bool ctHuffmanBuild(CT_HUFFMAN *pTable, const quint8 *pLengths, qint32 nCount)
{
    qint32 i = 0;

    if ((nCount <= 0) || (nCount > 288)) return false;
    for (i = 0; i < 16; i++) pTable->arrCount[i] = 0;
    for (i = 0; i < nCount; i++) {
        if (pLengths[i] > 15) return false;
        pTable->arrCount[pLengths[i]]++;
    }

    // Incomplete codes are accepted - Deflate emits a one symbol distance tree
    // for a block without matches. An over-subscribed code is rejected.
    qint32 nLeft = 1;
    for (i = 1; i < 16; i++) {
        nLeft <<= 1;
        nLeft -= pTable->arrCount[i];
        if (nLeft < 0) return false;
    }

    qint32 arrOffsets[16];
    arrOffsets[0] = 0;
    arrOffsets[1] = 0;
    for (i = 1; i < 15; i++) arrOffsets[i + 1] = arrOffsets[i] + pTable->arrCount[i];
    for (i = 0; i < nCount; i++) {
        if (pLengths[i]) {
            pTable->arrSymbol[arrOffsets[pLengths[i]]] = i;
            arrOffsets[pLengths[i]]++;
        }
    }

    return true;
}

static qint32 ctHuffmanDecode(CT_BITSTREAM *pStream, const CT_HUFFMAN *pTable)
{
    qint32 nCode = 0;
    qint32 nFirst = 0;
    qint32 nIndex = 0;

    for (qint32 nLength = 1; nLength <= 15; nLength++) {
        nCode |= (qint32)ctBitsRead(pStream, 1);
        if (pStream->bError) return -1;
        const qint32 nCount = pTable->arrCount[nLength];
        if (nCode - nCount < nFirst) return pTable->arrSymbol[nIndex + (nCode - nFirst)];
        nIndex += nCount;
        nFirst += nCount;
        nFirst <<= 1;
        nCode <<= 1;
    }

    return -1;
}

static bool ctLegacyReadDynamicTables(CT_BITSTREAM *pStream, quint8 *pLengths, CT_HUFFMAN *pLengthTable, CT_HUFFMAN *pDistanceTable, XBinary::PDSTRUCT *pPdStruct)
{
    const qint32 nLiteralCount = (qint32)ctBitsRead(pStream, 5) + 257;
    const qint32 nDistanceCount = (qint32)ctBitsRead(pStream, 5) + 1;
    const qint32 nCodeCount = (qint32)ctBitsRead(pStream, 4) + 4;
    if (pStream->bError || (nLiteralCount > 288) || (nDistanceCount > 32) || (nCodeCount > 19)) return false;

    qint32 i = 0;
    for (i = 0; i < 19; i++) pLengths[i] = 0;
    for (i = 0; i < nCodeCount; i++) {
        const quint32 nValue = ctBitsRead(pStream, 3);
        if (pStream->bError) return false;
        pLengths[g_arrCtLegacyCodeLengthOrder[i]] = (quint8)nValue;
    }

    CT_HUFFMAN codeTable;
    if (!ctHuffmanBuild(&codeTable, pLengths, 19)) return false;

    const qint32 nTotal = nLiteralCount + nDistanceCount;
    qint32 nIndex = 0;
    while (nIndex < nTotal) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nSymbol = ctHuffmanDecode(pStream, &codeTable);
        if ((nSymbol < 0) || pStream->bError) return false;
        if (nSymbol < 16) {
            pLengths[nIndex++] = (quint8)nSymbol;
        } else {
            qint32 nRepeat = 0;
            quint8 nValue = 0;
            if (nSymbol == 16) {
                if (nIndex == 0) return false;
                nValue = pLengths[nIndex - 1];
                nRepeat = 3 + (qint32)ctBitsRead(pStream, 2);
            } else if (nSymbol == 17) {
                nRepeat = 3 + (qint32)ctBitsRead(pStream, 3);
            } else {
                nRepeat = 11 + (qint32)ctBitsRead(pStream, 7);
            }
            if (pStream->bError || (nRepeat > nTotal - nIndex)) return false;
            for (qint32 k = 0; k < nRepeat; k++) pLengths[nIndex++] = nValue;
        }
    }

    if (nIndex != nTotal) return false;
    if (!ctHuffmanBuild(pLengthTable, pLengths, nLiteralCount)) return false;
    if (!ctHuffmanBuild(pDistanceTable, pLengths + nLiteralCount, nDistanceCount)) return false;

    return true;
}

static bool ctLegacyInflateBlock(CT_BITSTREAM *pStream, const CT_HUFFMAN *pLengthTable, const CT_HUFFMAN *pDistanceTable, QByteArray *pOut, qint64 nMaxOutput,
                                 XBinary::PDSTRUCT *pPdStruct)
{
    while (true) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint32 nSymbol = ctHuffmanDecode(pStream, pLengthTable);
        if ((nSymbol < 0) || pStream->bError) return false;
        if (nSymbol == 256) return true;
        if (nSymbol < 256) {
            if ((qint64)pOut->size() >= nMaxOutput) return false;
            pOut->append((char)(quint8)nSymbol);
        } else {
            const qint32 nIndex = nSymbol - 257;
            if (nIndex >= 29) return false;
            const qint32 nLength = (qint32)g_arrCtLengthBase[nIndex] + (qint32)ctBitsRead(pStream, g_arrCtLengthExtra[nIndex]);
            if (pStream->bError) return false;
            const qint32 nDistanceSymbol = ctHuffmanDecode(pStream, pDistanceTable);
            if ((nDistanceSymbol < 0) || (nDistanceSymbol >= 30) || pStream->bError) return false;
            const qint64 nDistance = (qint64)g_arrCtDistanceBase[nDistanceSymbol] + (qint64)ctBitsRead(pStream, g_arrCtDistanceExtra[nDistanceSymbol]);
            if (pStream->bError || (nDistance <= 0) || (nDistance > (qint64)pOut->size())) return false;
            if ((qint64)nLength > nMaxOutput - (qint64)pOut->size()) return false;
            const qint64 nSource = (qint64)pOut->size() - nDistance;
            for (qint32 i = 0; i < nLength; i++) pOut->append(pOut->at((int)(nSource + i)));
        }
    }
}

static bool ctLegacyInflate(const quint8 *pSrc, qint64 nSrcLen, qint64 nMaxOutput, QByteArray *pOut, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSrc || !pOut || (nSrcLen <= 0) || (nSrcLen > CT_MAX_CONTAINER_SIZE) || (nMaxOutput < 0) || (nMaxOutput > CT_MAX_FILE_SIZE)) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    pOut->clear();
    // Reserve a working buffer, never the declared size: the declared size is
    // attacker controlled and the class's other decoder (ctInflate) grows in
    // 64 KiB steps rather than trusting it.
    pOut->reserve((qint32)qMin<qint64>(nMaxOutput + 1, 1ll << 20));

    CT_BITSTREAM stream;
    stream.pData = pSrc;
    stream.nSize = nSrcLen;
    stream.nBitPos = 0;
    stream.bError = false;

    quint8 arrLengths[320];
    CT_HUFFMAN lengthTable;
    CT_HUFFMAN distanceTable;

    bool bFinal = false;
    qint32 nBlocks = 0;
    while (!bFinal) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (++nBlocks > CT_LEGACY_MAX_BLOCKS) return false;
        const quint32 nType = ctBitsRead(&stream, 3);
        const quint32 nFinal = ctBitsRead(&stream, 1);
        if (stream.bError) return false;
        bFinal = (nFinal != 0);

        if (nType == 7) {
            stream.nBitPos = (stream.nBitPos + 7) & ~(qint64)7;
            const qint64 nStoredOffset = stream.nBitPos >> 3;
            if (nStoredOffset + 2 > nSrcLen) return false;
            const qint64 nStoredSize = (qint64)ctRd16(pSrc + nStoredOffset);
            if ((nStoredSize > nSrcLen - nStoredOffset - 2) || (nStoredSize > nMaxOutput - (qint64)pOut->size())) return false;
            if (nStoredSize) pOut->append((const char *)(pSrc + nStoredOffset + 2), (qint32)nStoredSize);
            stream.nBitPos = (nStoredOffset + 2 + nStoredSize) * 8;
        } else if ((nType == 5) || (nType == 6)) {
            if (nType == 5) {
                qint32 i = 0;
                for (i = 0; i < 144; i++) arrLengths[i] = 8;
                for (; i < 256; i++) arrLengths[i] = 9;
                for (; i < 280; i++) arrLengths[i] = 7;
                for (; i < 288; i++) arrLengths[i] = 8;
                if (!ctHuffmanBuild(&lengthTable, arrLengths, 288)) return false;
                for (i = 0; i < 30; i++) arrLengths[i] = 5;
                if (!ctHuffmanBuild(&distanceTable, arrLengths, 30)) return false;
            } else if (!ctLegacyReadDynamicTables(&stream, arrLengths, &lengthTable, &distanceTable, pPdStruct)) {
                return false;
            }
            if (!ctLegacyInflateBlock(&stream, &lengthTable, &distanceTable, pOut, nMaxOutput, pPdStruct)) return false;
        } else {
            return false;
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

// Install Creator 1.x directory record. Each entry is
//   {u32 entrySize, .., u8 flags@0x0D, .., u32 uncompressed@0x12,
//    u32 compressed@0x16, [0x14 bytes if flags&6][0x18 bytes if flags&8],
//    NUL terminated name}
// and the payloads sit back to back in the trailing 0x7F7F region, so the sum
// of the compressed sizes has to reproduce that region exactly. That identity
// is what picks the directory out of the other decoded chunks.
// Install Creator 1.x entry names are relative paths ("Pictures\na.gif"), not
// bare file names. Every component is checked with the same rules a flat name
// gets and the result is joined with '/', so the extractor's own containment
// check sees an ordinary relative path instead of losing the real name.
static QString ctLegacySafeRelativePath(const QString &sValue)
{
    if (sValue.isEmpty() || (sValue.size() > 1024)) return QString();

    QString sNormalized = sValue;
    sNormalized.replace(QLatin1Char('\\'), QLatin1Char('/'));

    const QStringList listParts = sNormalized.split(QLatin1Char('/'));
    if ((listParts.size() < 1) || (listParts.size() > 32)) return QString();

    for (qint32 i = 0; i < listParts.size(); i++) {
        if (!ctIsSafeBaseName(listParts.at(i))) return QString();
    }

    return listParts.join(QLatin1Char('/'));
}

struct CT_LEGACY_TOC_ENTRY {
    QString sName;
    quint32 nUncompressedSize;
    quint32 nCompressedSize;
};

static bool ctParseLegacyToc(const QByteArray &baToc, qint64 nRegionSize, QList<CT_LEGACY_TOC_ENTRY> *pList, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pList) return false;
    pList->clear();
    if (baToc.size() < 8) return false;

    const quint8 *p = (const quint8 *)baToc.constData();
    const qint64 n = baToc.size();
    const quint32 nCount = ctRd32(p);
    if ((nCount == 0) || (nCount > (quint32)CT_MAX_FILE_COUNT)) return false;

    qint64 nTotal = 0;
    qint64 pos = 4;
    for (quint32 i = 0; i < nCount; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (pos + CT_LEGACY_ENTRY_HEADER > n) return false;
        const quint32 nEntrySize = ctRd32(p + pos);
        if ((nEntrySize <= (quint32)CT_LEGACY_ENTRY_HEADER) || (nEntrySize > (quint32)CT_LEGACY_MAX_ENTRY) || ((qint64)nEntrySize > n - pos)) return false;

        const quint8 nEntryFlags = p[pos + 0x0D];
        const quint32 nUncompressedSize = ctRd32(p + pos + 0x12);
        const quint32 nCompressedSize = ctRd32(p + pos + 0x16);
        if (((qint64)nUncompressedSize > CT_MAX_FILE_SIZE) || ((qint64)nCompressedSize > CT_MAX_CONTAINER_SIZE)) return false;
        if ((qint64)nCompressedSize > nRegionSize - nTotal) return false;

        qint64 nNameOffset = pos + CT_LEGACY_ENTRY_HEADER;
        if (nEntryFlags & 6) nNameOffset += 0x14;
        if (nEntryFlags & 8) nNameOffset += 0x18;
        const qint64 nEntryEnd = pos + (qint64)nEntrySize;
        if (nNameOffset >= nEntryEnd) return false;
        qint64 nNameEnd = nNameOffset;
        while ((nNameEnd < nEntryEnd) && p[nNameEnd]) nNameEnd++;
        if (nNameEnd >= nEntryEnd) return false;

        CT_LEGACY_TOC_ENTRY entry;
        entry.sName = QString::fromLatin1((const char *)p + nNameOffset, (qint32)(nNameEnd - nNameOffset));
        entry.nUncompressedSize = nUncompressedSize;
        entry.nCompressedSize = nCompressedSize;
        pList->append(entry);

        nTotal += (qint64)nCompressedSize;
        pos = nEntryEnd;
    }

    return (pos == n) && (nTotal == nRegionSize);
}

// Walk the Install Creator 1.x chunk chain {u16 tag, u16 flags, u32 size},
// decode every metadata chunk, pick the directory, then cut the trailing
// region into the members it declares.
static bool ctBuildLegacyEntries(XClickteam::UNPACK_CONTEXT *pContext, const quint8 *p, qint64 n, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pContext || !p || (n < 13)) return false;

    QList<QByteArray> listCandidates;
    qint64 nRegionOffset = -1;
    qint64 nRegionSize = 0;
    qint64 nMetadataOutput = 0;
    qint32 nRecords = 0;
    qint64 pos = 0;

    while (pos + 8 <= n) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (++nRecords > CT_MAX_DIRECTORY_ENTRIES) return false;

        const quint16 nTag = ctRd16(p + pos);
        const quint16 nRecordFlags = ctRd16(p + pos + 2);
        const quint32 nSize = ctRd32(p + pos + 4);

        if (nTag == CT_RECORD_TAG_END) {
            // The terminator declares the size of the member region that
            // follows its own four byte prefix.
            if ((nRecordFlags != 0) || (pos + 12 > n)) return false;
            nRegionOffset = pos + 12;
            nRegionSize = n - nRegionOffset;
            if ((qint64)nSize != nRegionSize) return false;
            pos = n;
            break;
        }

        if (((qint64)nSize > n - pos - 8) || ((qint64)nSize > CT_MAX_CONTAINER_SIZE)) return false;
        const qint64 nBodyOffset = pos + 8;
        QByteArray baBlob;

        if (nRecordFlags) {
            if (nSize < 5) return false;
            const quint32 nUncompressedSize = ctRd32(p + nBodyOffset);
            if ((nUncompressedSize == 0) || ((qint64)nUncompressedSize > CT_MAX_FILE_SIZE) || ((qint64)nUncompressedSize > CT_MAX_TOTAL_OUTPUT - nMetadataOutput)) {
                return false;
            }
            if (!ctLegacyInflate(p + nBodyOffset + 4, (qint64)nSize - 4, (qint64)nUncompressedSize, &baBlob, pPdStruct) ||
                ((quint32)baBlob.size() != nUncompressedSize)) {
                return false;
            }
            nMetadataOutput += (qint64)nUncompressedSize;
        } else {
            if ((qint64)nSize > CT_MAX_TOTAL_OUTPUT - nMetadataOutput) return false;
            baBlob = QByteArray((const char *)p + nBodyOffset, (qint32)nSize);
            nMetadataOutput += (qint64)nSize;
        }

        listCandidates.append(baBlob);
        pos = nBodyOffset + (qint64)nSize;
    }

    if ((pos != n) || (nRegionOffset < 0) || (nRegionSize <= 0)) return false;

    QList<CT_LEGACY_TOC_ENTRY> listToc;
    bool bTocFound = false;
    for (qint32 i = 0; i < listCandidates.size(); i++) {
        QList<CT_LEGACY_TOC_ENTRY> listCandidateToc;
        if (ctParseLegacyToc(listCandidates.at(i), nRegionSize, &listCandidateToc, pPdStruct)) {
            listToc = listCandidateToc;
            bTocFound = true;
        }
    }
    if (!bTocFound || listToc.isEmpty()) return false;

    QSet<QString> setUsedNames;
    qint64 nOffset = nRegionOffset;
    for (qint32 i = 0; i < listToc.size(); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const CT_LEGACY_TOC_ENTRY &tocEntry = listToc.at(i);
        if ((qint64)tocEntry.nCompressedSize > n - nOffset) return false;

        QByteArray baFile;
        if (tocEntry.nCompressedSize) {
            if (!ctLegacyInflate(p + nOffset, (qint64)tocEntry.nCompressedSize, (qint64)tocEntry.nUncompressedSize, &baFile, pPdStruct)) return false;
        }
        if ((quint32)baFile.size() != tocEntry.nUncompressedSize) return false;
        nOffset += (qint64)tocEntry.nCompressedSize;

        QString sName = ctLegacySafeRelativePath(tocEntry.sName);
        if (sName.isEmpty()) sName = QString("file_%1").arg(i, 4, 10, QChar('0'));
        QString sNameKey = sName.toCaseFolded();
        if (setUsedNames.contains(sNameKey)) {
            sName = QString("%1_%2").arg(sName).arg(i, 4, 10, QChar('0'));
            sNameKey = sName.toCaseFolded();
            if (setUsedNames.contains(sNameKey)) return false;
        }
        setUsedNames.insert(sNameKey);

        if ((pContext->listEntries.size() >= CT_MAX_FILE_COUNT) || ((qint64)baFile.size() > CT_MAX_TOTAL_OUTPUT - pContext->nTotalOutput)) return false;

        XClickteam::FILE_ENTRY entry;
        entry.sName = sName;
        entry.baData = baFile;
        pContext->listEntries.append(entry);
        pContext->nTotalOutput += (qint64)baFile.size();
    }

    return (nOffset == n) && !pContext->listEntries.isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct);
}

// RFC 1950 header test. A Multimedia Fusion 2 pack declares nothing about how
// a file is stored; the reference implementation decides from the bytes, and a
// stored member always opens on "MZ", which is not a legal zlib header.
static bool ctIsZlibHeader(const quint8 *p, qint64 nAvailable)
{
    if (!p || (nAvailable < 3)) return false;
    if ((p[0] & 0x0F) != 8) return false;
    if ((p[0] >> 4) > 7) return false;
    if (((((quint32)p[0] << 8) + (quint32)p[1]) % 31) != 0) return false;
    if (p[1] & 0x20) return false;
    if (((p[2] >> 1) & 3) == 3) return false;
    return true;
}

// The size fields of a Multimedia Fusion 2 entry are not self-describing: a
// build writes either a single packed size, or an uncompressed size followed
// by the packed size, and the name is either ANSI or UTF-16LE. The reference
// implementation probes the FIRST entry with both name encodings and requires
// EXACTLY ONE of them to leave a readable payload behind the name - which is
// what stops an ANSI reading of a UTF-16 name from being accepted - then
// applies that answer to the whole pack.
static bool ctMmf2ProbeEntry(const quint8 *p, qint64 n, bool bUnicodeName, bool *pbTwoSizeFields)
{
    if (!p || !pbTwoSizeFields || (n < 2)) return false;

    const qint32 nNameLength = (qint32)ctRd16(p);
    if ((nNameLength <= 0) || (nNameLength > CT_MMF2_MAX_NAME)) return false;
    const qint64 nNameBytes = bUnicodeName ? ((qint64)nNameLength * 2) : (qint64)nNameLength;
    if (nNameBytes > n - 2) return false;

    const qint64 nPosition = 2 + nNameBytes;
    if (11 > n - nPosition) return false;

    const qint32 nFirst = (qint32)ctRd32(p + nPosition);
    const qint32 nSecond = (qint32)ctRd32(p + nPosition + 4);

    if ((nFirst == 0) && (nSecond > 0)) {
        if ((ctRd16(p + nPosition + 8) == 0x5A4D) || ctIsZlibHeader(p + nPosition + 8, 3)) {
            *pbTwoSizeFields = true;
            return true;
        }
        return false;
    }

    if (nFirst >= 1) {
        if (((nSecond & 0xFFFF) == 0x5A4D) || ctIsZlibHeader(p + nPosition + 4, 3)) {
            *pbTwoSizeFields = false;
            return true;
        }
    }

    return false;
}

static bool ctBuildMmf2Entries(XClickteam::UNPACK_CONTEXT *pContext, const quint8 *p, qint64 n, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pContext || !p || (n < (qint64)CT_MMF2_HEADER_SIZE)) return false;

    const qint32 nPackedFileCount = (qint32)ctRd32(p + 28);
    if ((nPackedFileCount <= 0) || (nPackedFileCount > CT_MAX_FILE_COUNT)) return false;

    bool bUnicodeTwoSizeFields = false;
    bool bAnsiTwoSizeFields = false;
    const bool bUnicodeAnswer = ctMmf2ProbeEntry(p + CT_MMF2_HEADER_SIZE, n - (qint64)CT_MMF2_HEADER_SIZE, true, &bUnicodeTwoSizeFields);
    const bool bAnsiAnswer = ctMmf2ProbeEntry(p + CT_MMF2_HEADER_SIZE, n - (qint64)CT_MMF2_HEADER_SIZE, false, &bAnsiTwoSizeFields);
    if (bUnicodeAnswer == bAnsiAnswer) return false;
    const bool bUnicodeNames = bUnicodeAnswer;
    const bool bTwoSizeFields = bUnicodeAnswer ? bUnicodeTwoSizeFields : bAnsiTwoSizeFields;

    QSet<QString> setUsedNames;
    qint64 nPosition = (qint64)CT_MMF2_HEADER_SIZE;

    for (qint32 i = 0; i < nPackedFileCount; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        if (2 > n - nPosition) return false;
        const qint32 nNameLength = (qint32)ctRd16(p + nPosition);
        if ((nNameLength <= 0) || (nNameLength > CT_MMF2_MAX_NAME)) return false;
        nPosition += 2;

        const qint64 nNameBytes = bUnicodeNames ? ((qint64)nNameLength * 2) : (qint64)nNameLength;
        if (nNameBytes > n - nPosition) return false;

        QString sRawName;
        sRawName.reserve(nNameLength);
        for (qint32 k = 0; k < nNameLength; k++) {
            if (bUnicodeNames) {
                sRawName.append(QChar((ushort)ctRd16(p + nPosition + (qint64)k * 2)));
            } else {
                sRawName.append(QChar((ushort)p[nPosition + k]));
            }
        }
        nPosition += nNameBytes;

        const qint64 nSizeFieldBytes = bTwoSizeFields ? 8 : 4;
        if (nSizeFieldBytes > n - nPosition) return false;
        const qint32 nPackedSize = bTwoSizeFields ? (qint32)ctRd32(p + nPosition + 4) : (qint32)ctRd32(p + nPosition);
        nPosition += nSizeFieldBytes;
        if ((nPackedSize <= 0) || ((qint64)nPackedSize > n - nPosition)) return false;

        QByteArray baFile;
        if (ctIsZlibHeader(p + nPosition, n - nPosition)) {
            const qint64 nRemainingOutput = CT_MAX_TOTAL_OUTPUT - pContext->nTotalOutput;
            qint64 nConsumed = 0;
            if ((nRemainingOutput < 0) || !ctInflate(p + nPosition, nPackedSize, qMin(CT_MAX_FILE_SIZE, nRemainingOutput), &baFile, &nConsumed, pPdStruct) ||
                (nConsumed <= 0) || (nConsumed > (qint64)nPackedSize)) {
                return false;
            }
        } else {
            baFile = QByteArray(reinterpret_cast<const char *>(p + nPosition), nPackedSize);
        }
        nPosition += nPackedSize;

        QString sName = sRawName;
        if (!ctIsSafeBaseName(sName)) sName = QString("file_%1").arg(i, 4, 10, QChar('0'));
        QString sNameKey = sName.toCaseFolded();
        if (setUsedNames.contains(sNameKey)) {
            sName = QString("%1_%2").arg(sName).arg(i, 4, 10, QChar('0'));
            sNameKey = sName.toCaseFolded();
            if (setUsedNames.contains(sNameKey)) return false;
        }
        setUsedNames.insert(sNameKey);

        if ((pContext->listEntries.size() >= CT_MAX_FILE_COUNT) || ((qint64)baFile.size() > CT_MAX_TOTAL_OUTPUT - pContext->nTotalOutput)) return false;

        XClickteam::FILE_ENTRY entry;
        entry.sName = sName;
        entry.baData = baFile;
        pContext->listEntries.append(entry);
        pContext->nTotalOutput += (qint64)baFile.size();
    }

    // Everything behind the packed runtime is the application's own chunk
    // stream, stored; the reference implementation publishes it as "1.ccn".
    if (nPosition < n) {
        const qint64 nChunkStreamSize = n - nPosition;
        QString sName = QStringLiteral("1.ccn");
        if (setUsedNames.contains(sName.toCaseFolded())) sName = QString("%1_%2").arg(sName).arg(nPackedFileCount, 4, 10, QChar('0'));
        if (setUsedNames.contains(sName.toCaseFolded())) return false;

        if ((pContext->listEntries.size() >= CT_MAX_FILE_COUNT) || (nChunkStreamSize > CT_MAX_FILE_SIZE) ||
            (nChunkStreamSize > CT_MAX_TOTAL_OUTPUT - pContext->nTotalOutput)) {
            return false;
        }

        XClickteam::FILE_ENTRY entry;
        entry.sName = sName;
        entry.baData = QByteArray(reinterpret_cast<const char *>(p + nPosition), (int)nChunkStreamSize);
        pContext->listEntries.append(entry);
        pContext->nTotalOutput += nChunkStreamSize;
    }

    return !pContext->listEntries.isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct);
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

    if ((n >= (qint64)CT_MMF2_HEADER_SIZE) && (ctRd32(p) == CT_MMF2_MAGIC1) && (ctRd32(p + 4) == CT_MMF2_MAGIC2) && (ctRd32(p + 8) == CT_MMF2_HEADER_SIZE)) {
        return ctBuildMmf2Entries(pContext, p, n, pPdStruct);
    }

    if (baOv.left(5) != QByteArray("\x77\x77\x67\x54\x29", 5)) {
        return ctBuildLegacyEntries(pContext, p, n, pPdStruct);
    }

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
    if (!isProgressAlive() || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    const quint64 nGeneration = getDeviceGeneration();
    const bool bIsImage = isImage();
    const XADDR nModuleAddress = getModuleAddress();
    const qint64 nSourceSize = guardedSource->size();
    if (!isProgressAlive() || (nSourceSize < 0) || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource))
        return false;
    std::unique_ptr<XMaterializedUnpackGuard> pSourceGuard(XMaterializedUnpackGuard::bind(guardedSource, pPdStruct));
    if (!isProgressAlive() || !pSourceGuard || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource))
        return false;
    if (!m_bTrustedSnapshot) {
        QScopedPointer<QIODevice> pSnapshot(createFileBuffer(nSourceSize, pPdStruct));
        if (!isProgressAlive() || !pSnapshot || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource))
            return false;
        const QString sSourceFileName = ctDeviceFileName(guardedSource);
        if (!isProgressAlive() || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource)) return false;
        pSnapshot->setProperty("XStaticUnpacker.SourceIdentityBound", true);
        if (!sSourceFileName.isEmpty()) pSnapshot->setProperty("XStaticUnpacker.SourceFileName", sSourceFileName);
        const bool bCopied = copyDeviceMemory(guardedSource, 0, pSnapshot.data(), 0, nSourceSize, pPdStruct);
        if (!isProgressAlive() || !bCopied || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource))
            return false;
        XClickteam worker(pSnapshot.data(), bIsImage, nModuleAddress);
        worker.m_bTrustedSnapshot = true;
        UNPACK_STATE materializedState = {};
        const bool bMaterialized = worker.initUnpack(&materializedState, mapProperties, pPdStruct);
        if (!isProgressAlive() || !bMaterialized || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource))
            return false;
        UNPACK_CONTEXT *pMaterializedContext = static_cast<UNPACK_CONTEXT *>(materializedState.pContext);
        if (!pMaterializedContext) return false;
        std::unique_ptr<UNPACK_CONTEXT> pContext(new (std::nothrow) UNPACK_CONTEXT);
        if (!pContext) return false;
        pContext->listEntries = pMaterializedContext->listEntries;
        pContext->nTotalOutput = pMaterializedContext->nTotalOutput;
        pContext->listCompanionGuards.swap(pMaterializedContext->listCompanionGuards);
        if (!worker.finishUnpack(&materializedState, nullptr) || !isProgressAlive() || (getDeviceGeneration() != nGeneration) ||
            (getDevice() != guardedSource) || !isPdStructNotCanceled(pPdStruct) || pContext->listEntries.isEmpty())
            return false;
        pContext->pSourceDevice = guardedSource;
        pContext->pOwnerState = pState;
        pContext->baToken = QUuid::createUuid().toRfc4122();
        pContext->nDeviceGeneration = nGeneration;
        pContext->nSourceSize = nSourceSize;
        if (pContext->baToken.isEmpty()) return false;
        const bool bSourceFinal = pSourceGuard->validateAndFinalize(pPdStruct);
        const bool bCompanionsCurrent = isProgressAlive() && XMaterializedUnpackGuard::areCurrent(pSourceGuard.get(), pContext->listCompanionGuards, pPdStruct);
        if (!isProgressAlive() || !bSourceFinal || !bCompanionsCurrent || (getDeviceGeneration() != nGeneration) ||
            (getDevice() != guardedSource) || !isPdStructNotCanceled(pPdStruct))
            return false;
        pContext->pSourceGuard = pSourceGuard.release();
        pState->nTotalSize = nSourceSize;
        pState->nNumberOfRecords = pContext->listEntries.size();
        pState->mapUnpackProperties = mapProperties;
        pState->pContext = pContext.get();
        pState->baUnpackSourceToken = pContext->baToken;
        pLifetimeState->setContexts.insert(pContext.release());
        return pLifetimeState->bOwnerAlive;
    }
    pState->nTotalSize = nSourceSize;
    pState->mapUnpackProperties = mapProperties;

    INTERNAL_INFO info = _detect(pPdStruct);
    if (!isProgressAlive() || !info.bIsValid || (info.nContainerOffset < 0)) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) return false;
    const bool bBuilt = _buildEntries(pContext, info.nContainerOffset, pPdStruct);
    if (!isProgressAlive() || !bBuilt) {
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
    if (!isProgressAlive() || !bSourceFinal || !bCompanionsCurrent || (getDeviceGeneration() != nGeneration) ||
        (getDevice() != guardedSource) || !isPdStructNotCanceled(pPdStruct)) {
        delete pContext;
        return false;
    }
    pContext->pSourceGuard = pSourceGuard.release();
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;
    pState->baUnpackSourceToken = pContext->baToken;
    pLifetimeState->setContexts.insert(pContext);
    return pLifetimeState->bOwnerAlive;
}

XBinary::ARCHIVERECORD XClickteam::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return result;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    qint32 nIndex = pState->nCurrentIndex;
    if (!pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken) ||
        (pContext->nDeviceGeneration != getDeviceGeneration()) || (pContext->pSourceDevice != getDevice()) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) || (nIndex != pContext->nCurrentIndex) || (pState->nNumberOfRecords != pContext->listEntries.size()) ||
        (pState->nTotalSize != pContext->nSourceSize) || (nIndex < 0) || (nIndex >= pContext->listEntries.size()))
        return result;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !pLifetimeState->bOwnerAlive ||
        !pLifetimeState->setContexts.contains(pContext) || (pState->pContext != pContext) || (pContext->pOwnerState != pState) ||
        (pContext->baToken != pState->baUnpackSourceToken) || (pState->nCurrentIndex != pContext->nCurrentIndex))
        return result;

    const FILE_ENTRY &e = pContext->listEntries.at(nIndex);
    result.nStreamSize = e.baData.size();
    result.mapProperties[FPART_PROP_ORIGINALNAME] = e.sName;
    result.mapProperties[FPART_PROP_UNCOMPRESSEDSIZE] = (qint64)e.baData.size();
    result.mapProperties[FPART_PROP_ISFOLDER] = false;
    return result;
}

// Named functor replacing the former isAuthenticated capture-lambda in unpackCurrent().
struct CT_UNPACK_AUTH {
    XClickteam *guardedThis;
    const QSharedPointer<XClickteam::LIFETIME_STATE> &pLifetimeState;
    XBinary::UNPACK_STATE *pState;
    XClickteam::UNPACK_CONTEXT *pContext;
    qint32 nIndex;

    bool operator()() const
    {
        return pLifetimeState->bOwnerAlive && pLifetimeState->setContexts.contains(pContext) && (pState->pContext == pContext) &&
               (pContext->pOwnerState == pState) && (pState->baUnpackSourceToken == pContext->baToken) &&
               (pContext->nDeviceGeneration == guardedThis->getDeviceGeneration()) && (pContext->pSourceDevice == guardedThis->getDevice()) &&
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
    QIODevice *guardedOutput = pDevice;
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint32 nIndex = pState->nCurrentIndex;
    const CT_UNPACK_AUTH isAuthenticated = {this, pLifetimeState, pState, pContext, nIndex};
    if (!isAuthenticated()) return false;
    const bool bOpen = guardedOutput->isOpen();
    if (!isAuthenticated() || !bOpen) return false;
    const bool bWritable = guardedOutput->isWritable();
    if (!isAuthenticated() || !bWritable) return false;
    const bool bSequential = guardedOutput->isSequential();
    if (!isAuthenticated() || bSequential) return false;
    const QIODevice::OpenMode openMode = guardedOutput->openMode();
    if (!isAuthenticated() || (openMode & (QIODevice::Append | QIODevice::Text)) || !isResizeEnable(guardedOutput) ||
        devicesAlias(pContext->pSourceDevice, guardedOutput) || !isAuthenticated())
        return false;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !isAuthenticated()) return false;
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
    QIODevice *guardedStage = pStage.data();
    if (!isAuthenticated() || !guardedStage) return false;
    UNPACK_STATE writeState = *pState;
    writeState.pContext = nullptr;
    writeState.baUnpackSourceToken.clear();
    // The stage copy re-writes bytes charged again at publication below;
    // detach the budget here so each produced member is charged exactly once.
    writeState.spOutputBudget.clear();
    if (!writeUnpackData(&writeState, guardedStage, baData, pPdStruct) || !guardedStage || !isAuthenticated()) return false;
    writeState.nCurrentOffset = 0;
    writeState.spOutputBudget = pState->spOutputBudget;
    const bool bPublished = writeUnpackData(&writeState, guardedOutput, baData, pPdStruct);
    const bool bSourceCurrent = bPublished && XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct);
    const bool bFinal = bSourceCurrent && guardedOutput && isAuthenticated() && isPdStructNotCanceled(pPdStruct);
    if (!bFinal) {
        if (bPublished && guardedOutput) {
            resize(guardedOutput, 0);
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
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pLifetimeState->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->baToken != pState->baUnpackSourceToken) ||
        (pContext->nDeviceGeneration != getDeviceGeneration()) || (pContext->pSourceDevice != getDevice()) || (pState->nCurrentIndex != pContext->nCurrentIndex) ||
        (pState->nCurrentOffset != pContext->nCurrentOffset) || (pState->nNumberOfRecords != pContext->listEntries.size()) ||
        (pState->nTotalSize != pContext->nSourceSize) || (pContext->nCurrentIndex < 0) || (pContext->nCurrentIndex >= pContext->listEntries.size()))
        return false;
    if (!XMaterializedUnpackGuard::areCurrent(pContext->pSourceGuard, pContext->listCompanionGuards, pPdStruct) || !pLifetimeState->bOwnerAlive ||
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
