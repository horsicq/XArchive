/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsmartinstall.h"
#include "xmaterializedunpackguard.h"

#include <climits>
#include <memory>
#include <new>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopedPointer>
#include <QScopedValueRollback>
#include <QSet>
#include <QUuid>
#include <cstring>
#include <zlib.h>

#include "Algos/xlzxdecoder.h"
#include "xpe.h"

static const qint64 SI_MAX_ARCHIVE_SIZE = 256ll << 20;
static const qint32 SI_MAX_COMPANION_VOLUMES = 256;
static const qint32 SI_MAX_CONFIG_FIELDS = 0x40000;
static const qint32 SI_MAX_FILE_COUNT = 0x10000;
static const qint32 SI_MAX_DIRECTORY_ENTRIES = 100000;

static bool siRetainCompanionGuard(QList<XMaterializedUnpackGuard *> *pGuards, std::unique_ptr<XMaterializedUnpackGuard> *pGuard)
{
    if (!pGuards || !pGuard || !pGuard->get()) return false;
    try {
        pGuards->reserve(pGuards->size() + 1);
        pGuards->append(pGuard->get());
    } catch (const std::bad_alloc &) {
        return false;
    }
    pGuard->release();
    return true;
}

static bool siFinalizeCompanionGuards(const QList<XMaterializedUnpackGuard *> &listGuards, XBinary::PDSTRUCT *pPdStruct)
{
    for (XMaterializedUnpackGuard *pGuard : listGuards) {
        if (!pGuard || !pGuard->validateAndFinalize(pPdStruct)) return false;
    }
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

XSmartInstall::XSmartInstall(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackLifetimeState = QSharedPointer<LIFETIME_STATE>::create();
    setIsArchive(true);
}

XSmartInstall::UNPACK_CONTEXT::~UNPACK_CONTEXT()
{
    delete pSourceGuard;
    for (XMaterializedUnpackGuard *pGuard : listCompanionGuards) delete pGuard;
}

XSmartInstall::~XSmartInstall()
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

XSmartInstall::LIFETIME_STATE::~LIFETIME_STATE()
{
    const QSet<UNPACK_CONTEXT *> setContextsCopy = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : setContextsCopy) delete pContext;
}

bool XSmartInstall::isDeviceReplacementAllowed() const
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    return pLifetimeState && pLifetimeState->bOwnerAlive && !pLifetimeState->bOperationInProgress && pLifetimeState->setContexts.isEmpty();
}

bool XSmartInstall::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XSmartInstall> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XSmartInstall::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSmartInstall x(pDevice);
    return x.isValid(pPdStruct);
}

XSmartInstall::INTERNAL_INFO XSmartInstall::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XSmartInstall::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSmartInstall> guardedThis(this);
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

void *XSmartInstall::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSmartInstall> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XSmartInstall::setInternalInfo(void *pInternalInfo)
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

XBinary::FT XSmartInstall::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_SMARTINSTALL;
    }

    return FT_PE32_SMARTINSTALL;
}

static inline quint32 siRd32(const quint8 *p)
{
    return (quint32)(p[0] | ((quint32)p[1] << 8) | ((quint32)p[2] << 16) | ((quint32)p[3] << 24));
}
static inline quint16 siRd16(const quint8 *p)
{
    return (quint16)(p[0] | ((quint16)p[1] << 8));
}
static inline quint64 siRd64(const quint8 *p)
{
    quint64 v = 0;
    for (int i = 0; i < 8; i++) v |= ((quint64)p[i]) << (8 * i);
    return v;
}

static quint32 siCabChecksum(const quint8 *pData, qint64 nSize, quint32 nSeed = 0)
{
    if (!pData || (nSize < 0)) return 0;

    qint64 nOffset = 0;
    while (nSize - nOffset >= 4) {
        nSeed ^= siRd32(pData + nOffset);
        nOffset += 4;
    }

    const qint64 nRemaining = nSize - nOffset;
    if (nRemaining == 3) {
        nSeed ^= ((quint32)pData[nOffset] << 16) | ((quint32)pData[nOffset + 1] << 8) | pData[nOffset + 2];
    } else if (nRemaining == 2) {
        nSeed ^= ((quint32)pData[nOffset] << 8) | pData[nOffset + 1];
    } else if (nRemaining == 1) {
        nSeed ^= pData[nOffset];
    }
    return nSeed;
}

static qint64 siDataEnd(XPE *pPe, qint64 nFileSize)
{
    if (!pPe || (nFileSize <= 0)) return -1;

    qint64 nResult = nFileSize;
    XBinary::OFFSETSIZE osSignature = pPe->getSignOffsetSize();
    if ((osSignature.nOffset > 0) && (osSignature.nSize > 0) && (osSignature.nOffset <= nFileSize) && (osSignature.nSize == nFileSize - osSignature.nOffset)) {
        nResult = osSignature.nOffset;
    }
    return nResult;
}

static QString siGenericName(qint32 nIndex)
{
    return QString("file_%1").arg(nIndex, 4, 10, QChar('0'));
}

static bool siIsDecimalField(const QByteArray &baValue)
{
    if (baValue.isEmpty() || (baValue.size() > 10)) return false;
    for (int i = 0; i < baValue.size(); i++) {
        if ((baValue.at(i) < '0') || (baValue.at(i) > '9')) return false;
    }
    return true;
}

static bool siSplitConfigFields(const QByteArray &baConfig, QList<QByteArray> *pListFields)
{
    if (!pListFields) return false;
    pListFields->clear();

    int nPosition = 0;
    while (nPosition <= baConfig.size()) {
        if (pListFields->size() >= SI_MAX_CONFIG_FIELDS) {
            pListFields->clear();
            return false;
        }

        int nEnd = baConfig.indexOf('\0', nPosition);
        if (nEnd < 0) nEnd = baConfig.size();
        pListFields->append(baConfig.mid(nPosition, nEnd - nPosition));
        if (nEnd == baConfig.size()) break;
        nPosition = nEnd + 1;
    }

    return true;
}

// Manifest paths have the form "@$&%NN\<relative path>".  Accept only clean,
// relative paths so an unrelated config string can never become an output name.
static QString siManifestPath(const QByteArray &baValue)
{
    if ((baValue.size() > 4096) || !baValue.startsWith("@$&%")) return QString();

    int nPos = 4;
    int nDigits = 0;
    while ((nPos < baValue.size()) && (baValue.at(nPos) >= '0') && (baValue.at(nPos) <= '9')) {
        nPos++;
        nDigits++;
    }
    if ((nDigits == 0) || (nPos >= baValue.size()) || ((baValue.at(nPos) != '\\') && (baValue.at(nPos) != '/'))) return QString();

    QString sPath = QString::fromLatin1(baValue.mid(nPos + 1));
    sPath.replace('\\', '/');
    sPath = QDir::cleanPath(sPath);
    if (sPath.isEmpty() || (sPath == ".") || QDir::isAbsolutePath(sPath) || sPath.contains(':') || (sPath == "..") || sPath.startsWith("../") || sPath.contains("/../")) {
        return QString();
    }

    static const QString sForbidden = QStringLiteral("<>:\"\\|?*");
    static const QSet<QString> setReserved = {
        QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),    QStringLiteral("NUL"),     QStringLiteral("COM1"),
        QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),   QStringLiteral("COM5"),    QStringLiteral("COM6"),
        QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),   QStringLiteral("LPT1"),    QStringLiteral("LPT2"),
        QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"),   QStringLiteral("LPT6"),    QStringLiteral("LPT7"),
        QStringLiteral("LPT8"), QStringLiteral("LPT9"), QStringLiteral("CONIN$"), QStringLiteral("CONOUT$"), QStringLiteral("CLOCK$")};
    const QStringList listComponents = sPath.split('/');
    for (const QString &sComponent : listComponents) {
        if (sComponent.isEmpty() || (sComponent == ".") || (sComponent == "..") || (sComponent.size() > 255) || sComponent.endsWith(' ') || sComponent.endsWith('.')) {
            return QString();
        }
        for (QChar character : sComponent) {
            if (sForbidden.contains(character) || !character.isPrint()) return QString();
        }
        QString sStem = sComponent.section('.', 0, 0).toUpper();
        sStem.replace(QChar(0x00B9), QLatin1Char('1'));
        sStem.replace(QChar(0x00B2), QLatin1Char('2'));
        sStem.replace(QChar(0x00B3), QLatin1Char('3'));
        if (setReserved.contains(sStem)) return QString();
    }

    return sPath;
}

// A real file-manifest entry is a path followed by two decimal fields.  Other
// path-like config values do not have this shape.  The three decimal fields
// immediately before the manifest include the total compressed-region size.
// Select only a unique longest run so suffixes and unrelated target paths
// cannot be mistaken for the declaration.
static bool siManifestDeclaration(const QByteArray &baConfig, QStringList *pListNames, qint64 *pnArchiveSize)
{
    if (!pListNames || !pnArchiveSize) return false;
    pListNames->clear();
    *pnArchiveSize = -1;

    QList<QByteArray> listFields;
    if (!siSplitConfigFields(baConfig, &listFields)) return false;
    QMap<QString, QPair<QStringList, qint64> > mapCandidates;
    qint32 nBestCount = 0;

    for (int i = 0; i + 2 < listFields.size();) {
        const int nStart = i;
        QStringList listRun;
        int nPos = nStart;

        while (nPos + 2 < listFields.size()) {
            QString sPath = siManifestPath(listFields.at(nPos));
            if (sPath.isEmpty() || !siIsDecimalField(listFields.at(nPos + 1)) || !siIsDecimalField(listFields.at(nPos + 2))) break;
            listRun.append(sPath);
            if (listRun.size() > 0x10000) return false;
            nPos += 3;
        }

        if (listRun.isEmpty()) {
            i++;
            continue;
        }

        if ((nStart >= 3) && siIsDecimalField(listFields.at(nStart - 3)) && siIsDecimalField(listFields.at(nStart - 2)) && siIsDecimalField(listFields.at(nStart - 1))) {
            bool bSizeValid = false;
            qint64 nArchiveSize = listFields.at(nStart - 3).toLongLong(&bSizeValid);
            if (bSizeValid && (nArchiveSize >= 0) && (nArchiveSize <= SI_MAX_ARCHIVE_SIZE)) {
                if (listRun.size() > nBestCount) {
                    nBestCount = listRun.size();
                    mapCandidates.clear();
                }
                if (listRun.size() == nBestCount) {
                    QString sKey = listRun.join(QChar(0x1f)) + QChar(0x1e) + QString::number(nArchiveSize);
                    mapCandidates.insert(sKey, qMakePair(listRun, nArchiveSize));
                }
            }
        }

        i = nPos;
    }

    if ((nBestCount <= 0) || (mapCandidates.size() != 1)) return false;

    *pListNames = mapCandidates.constBegin().value().first;
    *pnArchiveSize = mapCandidates.constBegin().value().second;
    return true;
}

static bool siAssignManifestNames(QList<XSmartInstall::FILE_ENTRY> *pListEntries, const QStringList &listManifest)
{
    if (!pListEntries || (listManifest.size() != pListEntries->size())) return false;

    QSet<qint32> setIndexes;
    QSet<QString> setNames;
    for (int i = 0; i < pListEntries->size(); i++) {
        qint32 nIndex = pListEntries->at(i).nNameIndex;
        if ((nIndex < 0) || (nIndex >= listManifest.size()) || setIndexes.contains(nIndex)) return false;
        QString sNameKey = listManifest.at(nIndex).toCaseFolded();
        if (setNames.contains(sNameKey)) return false;
        setIndexes.insert(nIndex);
        setNames.insert(sNameKey);
    }

    for (int i = 0; i < pListEntries->size(); i++) {
        XSmartInstall::FILE_ENTRY &entry = (*pListEntries)[i];
        entry.sName = listManifest.at(entry.nNameIndex);
    }
    return true;
}

static bool siLooksLikeCabHeader(const quint8 *p, qint64 nRegionSize)
{
    if (!p || (nRegionSize < 48) || (nRegionSize > SI_MAX_ARCHIVE_SIZE)) return false;

    quint32 nCabinetSize = siRd32(p + 4);
    quint32 nFilesOffset = siRd32(p + 12);
    quint16 nFolders = siRd16(p + 22);
    quint16 nFiles = siRd16(p + 24);
    quint32 nDataOffset = siRd32(p + 32);
    quint16 nDataBlocks = siRd16(p + 36);
    quint16 nCompression = siRd16(p + 38);
    quint16 nMethod = nCompression & 0x000f;

    // Every observed SIM region is literally a complete CAB with only "MSCF"
    // removed, so all CAB absolute offsets/sizes are four bytes larger.
    if ((siRd32(p) != 0) || (siRd32(p + 8) != 0) || (siRd32(p + 16) != 0) || (p[20] != 3) || (p[21] != 1) || (siRd16(p + 26) != 0)) {
        return false;
    }
    if ((nCabinetSize != (quint32)nRegionSize + 4) || (nFolders != 1) || (nFiles == 0)) return false;
    if ((nFilesOffset != 44) || ((qint64)nFilesOffset - 4 >= nRegionSize)) return false;
    if (nDataOffset < 44) return false;
    if (nDataBlocks) {
        if ((qint64)nDataOffset - 4 + 8 > nRegionSize) return false;
    } else if ((qint64)nDataOffset - 4 != nRegionSize) {
        // A CAB containing only empty files has no CFDATA records; its folder
        // data offset points exactly one byte past this signature-less region.
        return false;
    }
    if ((nMethod != 1) && (nMethod != 3)) return false;  // MSZIP or LZX
    if ((nMethod == 3) && ((((nCompression >> 8) & 0x1f) < 15) || (((nCompression >> 8) & 0x1f) > 21))) return false;

    return true;
}

static bool siLooksLikeCabRegion(const QByteArray &baRegion)
{
    return siLooksLikeCabHeader(reinterpret_cast<const quint8 *>(baRegion.constData()), baRegion.size());
}

static bool siVolumePattern(const QByteArray &baConfig, QString *psPrefix, QString *psSuffix)
{
    if (!psPrefix || !psSuffix) return false;

    QMap<QString, QPair<QString, QString> > mapPatterns;
    QList<QByteArray> listFields;
    if (!siSplitConfigFields(baConfig, &listFields)) return false;
    for (int i = 0; i < listFields.size(); i++) {
        QByteArray baField = listFields.at(i);
        if (baField.size() > 4096) continue;
        int nMacro = baField.indexOf("@$&%");
        if (nMacro < 0) continue;

        int nPos = nMacro + 4;
        int nDigits = 0;
        while ((nPos < baField.size()) && (baField.at(nPos) >= '0') && (baField.at(nPos) <= '9')) {
            nPos++;
            nDigits++;
        }
        if ((nDigits == 0) || (nMacro == 0) || (nPos >= baField.size())) continue;

        QString sPrefix = QString::fromLatin1(baField.left(nMacro));
        QString sSuffix = QString::fromLatin1(baField.mid(nPos));
        if (!sSuffix.endsWith(".pak", Qt::CaseInsensitive)) continue;
        mapPatterns.insert(sPrefix.toLower() + QChar(0x1f) + sSuffix.toLower(), qMakePair(sPrefix, sSuffix));
    }

    if (mapPatterns.size() != 1) return false;
    *psPrefix = mapPatterns.constBegin().value().first;
    *psSuffix = mapPatterns.constBegin().value().second;
    return true;
}

static QString siDeviceFileName(QIODevice *pDevice)
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

static bool siCompanionCabRegions(QIODevice *pDevice, const QByteArray &baConfig, qint64 nDeclaredSize, QList<QByteArray> *pListRegions,
                                  QList<XMaterializedUnpackGuard *> *pCompanionGuards, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pListRegions || !pCompanionGuards || (nDeclaredSize <= 0) || (nDeclaredSize > SI_MAX_ARCHIVE_SIZE) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QList<QByteArray> listRegions;

    QPointer<QIODevice> guardedInputDevice(pDevice);
    if (!guardedInputDevice) return false;
    const QString sInputFileName = siDeviceFileName(guardedInputDevice.data());
    if (!guardedInputDevice) return false;
    if (sInputFileName.isEmpty()) return false;

    QString sPrefix;
    QString sSuffix;
    if (!siVolumePattern(baConfig, &sPrefix, &sSuffix)) return false;

    QFileInfo inputInfo(sInputFileName);
    QDir inputDir(inputInfo.absolutePath());
    QString sCanonicalDirectory = QFileInfo(inputDir.absolutePath()).canonicalFilePath();
    QString sCanonicalInput = inputInfo.canonicalFilePath();
    if (sCanonicalDirectory.isEmpty()) return false;
#if defined(Q_OS_WIN) || defined(Q_OS_MAC)
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCaseSensitivity = Qt::CaseSensitive;
#endif

    QFileInfoList listFiles = inputDir.entryInfoList(QDir::Files | QDir::System | QDir::Hidden | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    if (listFiles.size() > SI_MAX_DIRECTORY_ENTRIES) return false;
    QMap<qint32, QFileInfo> mapVolumes;
    qint32 nCandidates = 0;

    for (int i = 0; i < listFiles.size(); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        const QFileInfo &fileInfo = listFiles.at(i);
        if (fileInfo.absoluteFilePath() == inputInfo.absoluteFilePath()) continue;

        QString sFileName = fileInfo.fileName();
        if (!sFileName.startsWith(sPrefix, Qt::CaseInsensitive) || !sFileName.endsWith(sSuffix, Qt::CaseInsensitive) ||
            (sFileName.size() <= sPrefix.size() + sSuffix.size())) {
            continue;
        }

        QString sNumber = sFileName.mid(sPrefix.size(), sFileName.size() - sPrefix.size() - sSuffix.size());
        bool bDecimal = !sNumber.isEmpty();
        for (int j = 0; j < sNumber.size(); j++) {
            if ((sNumber.at(j) < QChar('0')) || (sNumber.at(j) > QChar('9'))) {
                bDecimal = false;
                break;
            }
        }
        QString sCanonicalCandidate = fileInfo.canonicalFilePath();
        if (!bDecimal || fileInfo.isSymLink() || !fileInfo.isReadable() || sCanonicalCandidate.isEmpty() ||
            (QFileInfo(sCanonicalCandidate).absolutePath().compare(sCanonicalDirectory, pathCaseSensitivity) != 0) ||
            (!sCanonicalInput.isEmpty() && (sCanonicalCandidate.compare(sCanonicalInput, pathCaseSensitivity) == 0))) {
            continue;
        }

        bool bNumberValid = false;
        qint32 nVolume = sNumber.toInt(&bNumberValid);
        if (!bNumberValid || (nVolume < 2)) continue;

        qint64 nCandidateSize = fileInfo.size();
        if ((nCandidateSize < 48) || (nCandidateSize > SI_MAX_ARCHIVE_SIZE)) continue;
        QFile headerFile(sCanonicalCandidate);
        if (!headerFile.open(QIODevice::ReadOnly)) continue;
        QByteArray baHeader = headerFile.read(40);
        headerFile.close();
        if ((baHeader.size() != 40) || !siLooksLikeCabHeader(reinterpret_cast<const quint8 *>(baHeader.constData()), nCandidateSize)) {
            continue;
        }

        nCandidates++;
        if (nCandidates > SI_MAX_COMPANION_VOLUMES || mapVolumes.contains(nVolume)) return false;
        mapVolumes.insert(nVolume, fileInfo);
    }

    if (mapVolumes.isEmpty()) return false;

    qint32 nExpectedVolume = 2;
    qint64 nAggregateSize = 0;
    for (QMap<qint32, QFileInfo>::const_iterator it = mapVolumes.constBegin(); it != mapVolumes.constEnd(); ++it, ++nExpectedVolume) {
        if (it.key() != nExpectedVolume) return false;

        qint64 nFileSize = it.value().size();
        if ((nFileSize < 48) || (nFileSize > SI_MAX_ARCHIVE_SIZE) || (nFileSize > SI_MAX_ARCHIVE_SIZE - nAggregateSize)) return false;
        nAggregateSize += nFileSize;
    }

    if (nAggregateSize != nDeclaredSize) return false;

    for (QMap<qint32, QFileInfo>::const_iterator it = mapVolumes.constBegin(); it != mapVolumes.constEnd(); ++it) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        qint64 nFileSize = it.value().size();
        QString sCanonicalVolume = it.value().canonicalFilePath();
        if (sCanonicalVolume.isEmpty() || (QFileInfo(sCanonicalVolume).absolutePath().compare(sCanonicalDirectory, pathCaseSensitivity) != 0)) {
            return false;
        }
        std::unique_ptr<XMaterializedUnpackGuard> pVolumeGuard(XMaterializedUnpackGuard::openFile(sCanonicalVolume, pPdStruct));
        QPointer<QIODevice> guardedVolumeDevice(pVolumeGuard ? pVolumeGuard->device() : nullptr);
        QFile *pVolumeFile = guardedVolumeDevice ? dynamic_cast<QFile *>(guardedVolumeDevice.data()) : nullptr;
        QPointer<QFile> guardedVolumeFile(pVolumeFile);
        if (!guardedVolumeDevice || !guardedVolumeFile) return false;
        const qint64 nObservedFileSize = guardedVolumeFile->size();
        if (!guardedVolumeDevice || !guardedVolumeFile || (nObservedFileSize != nFileSize)) return false;

        QByteArray baRegion;
        baRegion.reserve((int)nFileSize);
        while (baRegion.size() < nFileSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            qint64 nToRead = qMin(Q_INT64_C(1024) * 1024, nFileSize - baRegion.size());
            if (!guardedVolumeDevice || !guardedVolumeFile) return false;
            QByteArray baChunk = guardedVolumeFile->read(nToRead);
            if (!guardedVolumeDevice || !guardedVolumeFile || baChunk.isEmpty() || (baChunk.size() > nToRead)) return false;
            baRegion.append(baChunk);
        }
        if (!guardedVolumeDevice || !guardedVolumeFile) return false;
        const qint64 nFinalFileSize = guardedVolumeFile->size();
        if (!guardedVolumeDevice || !guardedVolumeFile) return false;
        const bool bAtEnd = guardedVolumeFile->atEnd();
        const bool bExactFile = guardedVolumeDevice && guardedVolumeFile && (nFinalFileSize == nFileSize) && bAtEnd;

        if (!XBinary::isPdStructNotCanceled(pPdStruct) || !bExactFile || (baRegion.size() != nFileSize) || !siLooksLikeCabRegion(baRegion)) return false;
        try {
            listRegions.append(baRegion);
        } catch (const std::bad_alloc &) {
            return false;
        }
        if (!siRetainCompanionGuard(pCompanionGuards, &pVolumeGuard)) return false;
    }

    if (listRegions.isEmpty()) return false;
    *pListRegions = listRegions;
    return true;
}

XSmartInstall::INTERNAL_INFO XSmartInstall::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nDataStart = -1;

    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;

    qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if ((nOverlayOffset <= 0) || (nOverlayOffset >= getSize())) return result;

    // Literal product tag "Smart Install Maker v" at the overlay start.
    QByteArray baTag = read_array_process(nOverlayOffset, 21, pPdStruct);
    if (baTag != QByteArray("Smart Install Maker v", 21)) return result;

    result.bIsValid = true;

    // Version digits: NUL-terminated ASCII at overlay + 0x17 ("v. <ver>\0").
    QString sVer = read_ansiString(nOverlayOffset + 0x17, 32).trimmed();
    if (!sVer.isEmpty()) result.sVersion = sVer;

    // 36-byte EOF footer: 4x u64 region pointers + [method:1][magic:3 = 03 78 F1].
    const qint64 nSize = getSize();
    const qint64 nDataEnd = siDataEnd(&pe, nSize);
    if ((nDataEnd >= 36) && (nDataEnd >= (nOverlayOffset + 36))) {
        QByteArray baFooter = read_array_process(nDataEnd - 36, 36, pPdStruct);
        if (baFooter.size() == 36) {
            const quint8 *pf = (const quint8 *)baFooter.constData();
            quint32 nFoot = siRd32(pf + 32);
            if ((nFoot >> 8) == 0xF17803) {
                quint8 nMethod = (quint8)(nFoot & 0xFF);
                quint64 q3 = siRd64(pf + 24);  // file-data region start (absolute)
                // Split builds legitimately point q3 at EOF-36: their CAB
                // region starts in a sibling diskN.pak rather than the EXE.
                if ((nMethod <= 1) && ((qint64)q3 >= nOverlayOffset) && ((qint64)q3 <= nDataEnd - 36)) {
                    result.bExtractable = true;
                    result.nMethod = nMethod;
                    result.nDataStart = (qint64)q3;
                }
            }
        }
    }

    return result;
}

// Raw-deflate (wbits -15) inflate of one block, with an optional preset dictionary.
// The reduced zlib in the build has no inflateSetDictionary, so the dictionary is
// supplied by prepending a stored (uncompressed) deflate block holding it and then
// stripping that prefix from the output. Reports block-N input bytes consumed.
static bool simInflate(const quint8 *pSrc, qint64 nSrcLen, const QByteArray &baDict, qint32 nExpectedSize, QByteArray *pOut, qint64 *pnConsumed,
                       XBinary::PDSTRUCT *pPdStruct)
{
    if (!pSrc || !pOut || (nSrcLen <= 0) || (nSrcLen > 0xFFFF) || (baDict.size() > 32768) || (nExpectedSize <= 0) || (nExpectedSize > 32768)) {
        return false;
    }

    QByteArray baIn;
    int nPrefixIn = 0;
    if (!baDict.isEmpty()) {
        int nLen = baDict.size();  // <= 32768
        int nNlen = (~nLen) & 0xffff;
        baIn.append((char)0x00);  // stored block, BFINAL=0
        baIn.append((char)(nLen & 0xff));
        baIn.append((char)((nLen >> 8) & 0xff));
        baIn.append((char)(nNlen & 0xff));
        baIn.append((char)((nNlen >> 8) & 0xff));
        baIn.append(baDict);
        nPrefixIn = baIn.size();
    }
    baIn.append((const char *)pSrc, (int)qMin(nSrcLen, (qint64)0x7fffffff));

    static const uInt SIM_INFLATE_BUFFER_SIZE = 65536;
    QScopedArrayPointer<char> pBuffer(new (std::nothrow) char[SIM_INFLATE_BUFFER_SIZE]);
    if (pBuffer.isNull()) return false;

    z_stream s;
    memset(&s, 0, sizeof(s));
    if (inflateInit2(&s, -15) != Z_OK) return false;
    s.next_in = (Bytef *)baIn.constData();
    s.avail_in = (uInt)baIn.size();

    QByteArray baFull;
    bool bOk = false;
    for (;;) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) break;

        s.next_out = reinterpret_cast<Bytef *>(pBuffer.data());
        s.avail_out = SIM_INFLATE_BUFFER_SIZE;
        int rc = inflate(&s, Z_NO_FLUSH);
        qint32 nProduced = (qint32)(SIM_INFLATE_BUFFER_SIZE - s.avail_out);
        if (nProduced > baDict.size() + nExpectedSize - baFull.size()) break;
        baFull.append(pBuffer.data(), nProduced);
        if (rc == Z_STREAM_END) {
            bOk = true;
            break;
        }
        if (rc != Z_OK) break;
        if ((s.avail_in == 0) && (s.avail_out == SIM_INFLATE_BUFFER_SIZE)) break;
    }
    qint64 nTotalIn = (qint64)s.total_in;
    inflateEnd(&s);
    if (!bOk || !XBinary::isPdStructNotCanceled(pPdStruct) || (nTotalIn - nPrefixIn != nSrcLen) || (baFull.size() != baDict.size() + nExpectedSize)) {
        return false;
    }

    *pOut = baFull.mid(baDict.size());  // strip the dictionary prefix
    if (pnConsumed) *pnConsumed = nTotalIn - nPrefixIn;
    return true;
}

static bool siDecodeCabRegion(const QByteArray &baRegion, qint64 nMaxOutput, QList<quint32> *pListSizes, QList<qint32> *pListIndexes, QByteArray *pbaCombined,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pListSizes || !pListIndexes || !pbaCombined || (nMaxOutput < 0) || (nMaxOutput > SI_MAX_ARCHIVE_SIZE) || !siLooksLikeCabRegion(baRegion) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QList<quint32> listSizes;
    QList<qint32> listIndexes;
    QByteArray baCombined;

    const quint8 *p = reinterpret_cast<const quint8 *>(baRegion.constData());
    const qint64 nRegionSize = baRegion.size();
    quint32 nFilesOffset = siRd32(p + 12);
    quint16 nFiles = siRd16(p + 24);
    quint32 nDataOffset = siRd32(p + 32);
    quint16 nDataBlocks = siRd16(p + 36);
    quint16 nCompression = siRd16(p + 38);
    quint16 nMethod = nCompression & 0x000f;

    qint64 nFilePos = (qint64)nFilesOffset - 4;  // CAB offsets include the omitted signature
    quint64 nFileBytes = 0;

    for (quint16 i = 0; i < nFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        // Standard CFFILE: cbFile/uoffFolderStart/iFolder/date/time/attribs/name.
        if ((nFilePos < 0) || (nFilePos + 16 > nRegionSize)) return false;

        quint32 nFileSize = siRd32(p + nFilePos);
        quint32 nFolderOffset = siRd32(p + nFilePos + 4);
        quint16 nFolderIndex = siRd16(p + nFilePos + 8);
        if ((nFolderIndex != 0) || (nFolderOffset != nFileBytes)) return false;

        qint64 nNameEnd = nFilePos + 16;
        qint64 nNameLimit = qMin(nRegionSize, nNameEnd + 4096);
        while ((nNameEnd < nNameLimit) && p[nNameEnd]) nNameEnd++;
        if (nNameEnd >= nNameLimit) return false;

        QByteArray baCabName(reinterpret_cast<const char *>(p + nFilePos + 16), (int)(nNameEnd - (nFilePos + 16)));
        bool bIndexValid = false;
        qint32 nNameIndex = baCabName.toInt(&bIndexValid);
        if (!bIndexValid || !siIsDecimalField(baCabName)) return false;

        nFileBytes += nFileSize;
        if ((nFileBytes > (quint64)SI_MAX_ARCHIVE_SIZE) || (nFileBytes > (quint64)nMaxOutput)) return false;
        listSizes.append(nFileSize);
        listIndexes.append(nNameIndex);
        nFilePos = nNameEnd + 1;
    }

    qint64 nBlockPos = (qint64)nDataOffset - 4;
    if (nFilePos != nBlockPos) return false;
    quint64 nBlockBytes = 0;
    QByteArray baLzxStream;

    for (quint16 i = 0; i < nDataBlocks; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        if ((nBlockPos < 0) || (nBlockPos + 8 > nRegionSize)) return false;

        quint16 nCompressedSize = siRd16(p + nBlockPos + 4);
        quint16 nUncompressedSize = siRd16(p + nBlockPos + 6);
        qint64 nPayloadPos = nBlockPos + 8;
        if ((nCompressedSize == 0) || (nUncompressedSize == 0) || (nUncompressedSize > 32768) || (nPayloadPos + nCompressedSize > nRegionSize)) {
            return false;
        }

        quint32 nStoredChecksum = siRd32(p + nBlockPos);
        if (nStoredChecksum) {
            quint32 nCalculatedChecksum = siCabChecksum(p + nBlockPos + 4, 4);
            nCalculatedChecksum = siCabChecksum(p + nPayloadPos, nCompressedSize, nCalculatedChecksum);
            if (nCalculatedChecksum != nStoredChecksum) return false;
        }

        nBlockBytes += nUncompressedSize;
        if (nBlockBytes > (quint64)SI_MAX_ARCHIVE_SIZE) return false;

        if (nMethod == 1) {
            // MSZIP CFDATA payload = "CK" + raw DEFLATE.  Its dictionary is
            // the final 32 KiB produced by all preceding CFDATA blocks.
            if ((nCompressedSize < 3) || (p[nPayloadPos] != 'C') || (p[nPayloadPos + 1] != 'K')) return false;

            QByteArray baBlock;
            qint64 nConsumed = 0;
            QByteArray baDictionary = (baCombined.size() > 32768) ? baCombined.right(32768) : baCombined;
            if (!simInflate(p + nPayloadPos + 2, nCompressedSize - 2, baDictionary, nUncompressedSize, &baBlock, &nConsumed, pPdStruct) ||
                (nConsumed != nCompressedSize - 2) || (baBlock.size() != nUncompressedSize)) {
                return false;
            }
            baCombined.append(baBlock);
        } else {
            baLzxStream.append(reinterpret_cast<const char *>(p + nPayloadPos), nCompressedSize);
        }

        nBlockPos = nPayloadPos + nCompressedSize;
    }

    if ((nBlockBytes != nFileBytes) || (listSizes.size() != nFiles) || (nBlockPos != nRegionSize)) return false;

    if ((nMethod == 3) && nBlockBytes) {
        qint32 nWindowBits = (nCompression >> 8) & 0x1f;
        if (!XLZXDecoder::decompressCABFolder(baLzxStream, &baCombined, (qint64)nBlockBytes, nWindowBits, pPdStruct)) return false;
    } else if (!nBlockBytes && !baLzxStream.isEmpty()) {
        return false;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || ((quint64)baCombined.size() != nFileBytes)) return false;
    *pListSizes = listSizes;
    *pListIndexes = listIndexes;
    *pbaCombined = baCombined;
    return true;
}

bool XSmartInstall::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    QPointer<XSmartInstall> guardedThis(this);
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
        const QString sSourceFileName = siDeviceFileName(guardedSource.data());
        if (!isProgressAlive() || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data())) return false;
        pSnapshot->setProperty("XStaticUnpacker.SourceIdentityBound", true);
        if (!sSourceFileName.isEmpty()) pSnapshot->setProperty("XStaticUnpacker.SourceFileName", sSourceFileName);
        const bool bCopied = copyDeviceMemory(guardedSource.data(), 0, pSnapshot.data(), 0, nSourceSize, pPdStruct);
        if (!isProgressAlive() || !bCopied || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedSource.data()))
            return false;
        XSmartInstall worker(pSnapshot.data(), bIsImage, nModuleAddress);
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
    if (!isProgressAlive() || !guardedThis || !guardedSource || !info.bIsValid || !info.bExtractable || (info.nDataStart < 0)) return false;

    const qint64 nSize = getSize();
    if (!isProgressAlive() || !guardedThis || !guardedSource || (nSize < 0)) return false;
    XPE pe(getDevice(), isImage(), getModuleAddress());
    const bool bPeValid = pe.isValid(pPdStruct);
    if (!isProgressAlive() || !guardedThis || !guardedSource || !bPeValid) return false;
    const qint64 nInstallerEnd = siDataEnd(&pe, nSize);
    if ((nInstallerEnd < 36) || (nInstallerEnd < info.nDataStart + 36)) return false;
    qint64 nDataEnd = nInstallerEnd - 36;
    if (!isProgressAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) return false;

    QByteArray baConfig;
    QByteArray baFooter = read_array_process(nInstallerEnd - 36, 36, pPdStruct);
    if (!isProgressAlive() || !guardedThis || !guardedSource) {
        delete pContext;
        return false;
    }
    if ((baFooter.size() == 36) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint64 nOverlayOffset = (qint64)siRd64(reinterpret_cast<const quint8 *>(baFooter.constData()));
        qint64 nConfigEnd = qMin(info.nDataStart, nDataEnd);
        qint64 nConfigSize = nConfigEnd - nOverlayOffset;
        if ((nOverlayOffset >= 0) && (nConfigSize > 0) && (nConfigSize <= (16 << 20))) {
            baConfig = read_array_process(nOverlayOffset, nConfigSize, pPdStruct);
            if (!isProgressAlive() || !guardedThis || !guardedSource) {
                delete pContext;
                return false;
            }
            if ((baConfig.size() != nConfigSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) baConfig.clear();
        }
    }

    QStringList listManifest;
    qint64 nDeclaredArchiveSize = -1;
    if (baConfig.isEmpty() || !siManifestDeclaration(baConfig, &listManifest, &nDeclaredArchiveSize)) {
        delete pContext;
        return false;
    }

    QList<FILE_ENTRY> listEntries;

    if (info.nMethod == 0) {
        // STORE: sequential {u32 idx; u32 size; u32 rsv; u64 FILETIME; u32 attr} + raw bytes.
        const qint64 nStoreSize = nDataEnd - info.nDataStart;
        if ((nDeclaredArchiveSize != 0) || (nStoreSize <= 0) || (nStoreSize > SI_MAX_ARCHIVE_SIZE) || !isProgressAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            delete pContext;
            return false;
        }

        QByteArray baData = read_array_process(info.nDataStart, nStoreSize, pPdStruct);
        if (!isProgressAlive() || !guardedThis || !guardedSource || (baData.size() != nStoreSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            delete pContext;
            return false;
        }

        const quint8 *p = (const quint8 *)baData.constData();
        const qint64 n = baData.size();
        qint64 pos = 0;
        int nRecord = 0;

        while (pos < n) {
            if (!isProgressAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                delete pContext;
                return false;
            }

            if (n - pos < 24) {
                delete pContext;
                return false;
            }

            quint32 nNameIndex = siRd32(p + pos);
            quint32 nRecSize = siRd32(p + 4 + pos);
            qint64 nDataOff = pos + 24;
            if ((siRd32(p + pos + 8) != 0) || (nNameIndex > INT_MAX) || (nRecSize > (quint32)SI_MAX_ARCHIVE_SIZE) || ((qint64)nRecSize > n - nDataOff) ||
                (listEntries.size() >= listManifest.size()) || (listEntries.size() >= SI_MAX_FILE_COUNT)) {
                delete pContext;
                return false;
            }

            FILE_ENTRY e;
            e.sName = siGenericName(nRecord++);
            e.nNameIndex = (qint32)nNameIndex;
            e.baData = QByteArray((const char *)p + nDataOff, (int)nRecSize);
            listEntries.append(e);
            pos = nDataOff + (qint64)nRecSize;
        }

        if (listEntries.isEmpty() || (pos != n)) {
            delete pContext;
            return false;
        }
    } else {
        // COMPRESSED: this is a complete CAB with only its "MSCF" signature
        // omitted.  The primary EXE holds it inline; split builds put the same
        // region in diskN.pak companion files.
        if ((nDeclaredArchiveSize <= 0) || (nDeclaredArchiveSize > SI_MAX_ARCHIVE_SIZE)) {
            delete pContext;
            return false;
        }

        QList<QByteArray> listRegions;
        const qint64 nPrimarySize = nDataEnd - info.nDataStart;
        if ((nPrimarySize < 0) || (nPrimarySize > SI_MAX_ARCHIVE_SIZE) || !isProgressAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            delete pContext;
            return false;
        }

        if (nPrimarySize > 0) {
            QByteArray baPrimaryRegion = read_array_process(info.nDataStart, nPrimarySize, pPdStruct);
            if (!isProgressAlive() || !guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || (baPrimaryRegion.size() != nPrimarySize) ||
                (nPrimarySize != nDeclaredArchiveSize) || !siLooksLikeCabRegion(baPrimaryRegion)) {
                delete pContext;
                return false;
            }
            listRegions.append(baPrimaryRegion);
        } else {
            const bool bCompanionsRead = siCompanionCabRegions(getDevice(), baConfig, nDeclaredArchiveSize, &listRegions, &pContext->listCompanionGuards, pPdStruct);
            if (!isProgressAlive() || !guardedThis || !guardedSource || !bCompanionsRead) {
                delete pContext;
                return false;
            }
        }

        if (listRegions.isEmpty()) {
            delete pContext;
            return false;
        }

        qint32 nRecord = 0;
        qint64 nTotalDecodedSize = 0;
        for (int nRegion = 0; nRegion < listRegions.size(); nRegion++) {
            if (!isProgressAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                delete pContext;
                return false;
            }

            QList<quint32> listSizes;
            QList<qint32> listIndexes;
            QByteArray baCombined;
            qint64 nRemainingDecoded = SI_MAX_ARCHIVE_SIZE - nTotalDecodedSize;
            if ((nRemainingDecoded < 0) || !siDecodeCabRegion(listRegions.at(nRegion), nRemainingDecoded, &listSizes, &listIndexes, &baCombined, pPdStruct) ||
                !isProgressAlive() || !guardedThis || !guardedSource || (listSizes.size() != listIndexes.size())) {
                delete pContext;
                return false;
            }

            qint64 nOffset = 0;
            for (int i = 0; i < listSizes.size(); i++) {
                quint32 nFileSize = listSizes.at(i);
                if (!isProgressAlive() || !XBinary::isPdStructNotCanceled(pPdStruct) || (nOffset + nFileSize > baCombined.size()) ||
                    ((qint64)nFileSize > SI_MAX_ARCHIVE_SIZE - nTotalDecodedSize) || (nRecord >= listManifest.size()) || (nRecord >= SI_MAX_FILE_COUNT)) {
                    delete pContext;
                    return false;
                }

                FILE_ENTRY entry;
                entry.sName = siGenericName(nRecord++);
                entry.nNameIndex = listIndexes.at(i);
                entry.baData = baCombined.mid((int)nOffset, (int)nFileSize);
                listEntries.append(entry);
                nOffset += nFileSize;
                nTotalDecodedSize += nFileSize;
            }

            if (nOffset != baCombined.size()) {
                delete pContext;
                return false;
            }
        }
    }

    if (listEntries.isEmpty() || (listEntries.size() != listManifest.size()) || !isProgressAlive() || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        delete pContext;
        return false;
    }

    if (!siAssignManifestNames(&listEntries, listManifest)) {
        delete pContext;
        return false;
    }
    pContext->listEntries = listEntries;

    pContext->pSourceDevice = guardedSource;
    pContext->pOwnerState = pState;
    pContext->baToken = QUuid::createUuid().toRfc4122();
    pContext->nDeviceGeneration = nGeneration;
    pContext->nSourceSize = nSourceSize;
    if (pContext->baToken.isEmpty()) {
        delete pContext;
        return false;
    }
    const bool bCompanionsFinal = siFinalizeCompanionGuards(pContext->listCompanionGuards, pPdStruct);
    const bool bSourceFinal = isProgressAlive() && bCompanionsFinal && pSourceGuard->validateAndFinalize(pPdStruct);
    const bool bCompanionsCurrent =
        isProgressAlive() && bSourceFinal && XMaterializedUnpackGuard::areCurrent(pSourceGuard.get(), pContext->listCompanionGuards, pPdStruct);
    if (!isProgressAlive() || !bCompanionsFinal || !bSourceFinal || !bCompanionsCurrent || !guardedThis || !guardedSource || (getDeviceGeneration() != nGeneration) ||
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

XBinary::ARCHIVERECORD XSmartInstall::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return result;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XSmartInstall> guardedThis(this);
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

static bool siFailOutput(bool bSeekableOutput, QIODevice *pDevice, XBinary::UNPACK_STATE *pState)
{
    if (bSeekableOutput) {
        XBinary::resize(pDevice, 0);
        pDevice->seek(0);
        pState->nCurrentOffset = 0;
    }
    return false;
}

bool XSmartInstall::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XSmartInstall> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext || pState->baUnpackSourceToken.isEmpty() || !guardedOutput || !isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint32 nIndex = pState->nCurrentIndex;
    struct AUTHENTICATION_PROBE {
        QPointer<XSmartInstall> guardedThis;
        QSharedPointer<LIFETIME_STATE> pLifetimeState;
        UNPACK_CONTEXT *pContext;
        UNPACK_STATE *pState;
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
    const AUTHENTICATION_PROBE isAuthenticated = {guardedThis, pLifetimeState, pContext, pState, nIndex};
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

bool XSmartInstall::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    const QSharedPointer<LIFETIME_STATE> pLifetimeState = m_pUnpackLifetimeState;
    if (!pLifetimeState || !pLifetimeState->bOwnerAlive || pLifetimeState->bOperationInProgress) return false;
    QScopedValueRollback<bool> operationGuard(pLifetimeState->bOperationInProgress, true);
    QPointer<XSmartInstall> guardedThis(this);
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

bool XSmartInstall::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
