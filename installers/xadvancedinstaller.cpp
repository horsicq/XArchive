/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xadvancedinstaller.h"

#include <memory>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QScopedValueRollback>
#include <QSet>

#include <algorithm>
#include <limits>

#include "xmsi.h"
#include "xpe.h"
#include "xarchive.h"

namespace {
class ADVANCEDINSTALLER_OPERATION_STATE_DELETER {
public:
    explicit ADVANCEDINSTALLER_OPERATION_STATE_DELETER(const QSharedPointer<XAdvancedInstaller::UNPACK_DEFERRED_CLEANUP> &pCleanup) : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XAdvancedInstaller::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

static bool isSafeAdvancedRecordName(const QString &sName)
{
    if (sName.isEmpty() || (sName.size() > 255) || (sName == ".") || (sName == "..") || sName.endsWith(' ') || sName.endsWith('.')) {
        return false;
    }

    static const QString sForbidden = QStringLiteral("<>:\"/\\|?*");
    for (QChar character : sName) {
        if (sForbidden.contains(character) || (character == QChar::ReplacementCharacter) || (character.category() == QChar::Other_Control) ||
            (character.category() == QChar::Other_Surrogate)) {
            return false;
        }
    }

    QString sStem = sName.section('.', 0, 0).toUpper();
    sStem.replace(QChar(0x00B9), QLatin1Char('1'));
    sStem.replace(QChar(0x00B2), QLatin1Char('2'));
    sStem.replace(QChar(0x00B3), QLatin1Char('3'));
    static const QSet<QString> setReservedNames = {
        QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),    QStringLiteral("NUL"),     QStringLiteral("COM1"),
        QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),   QStringLiteral("COM5"),    QStringLiteral("COM6"),
        QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),   QStringLiteral("LPT1"),    QStringLiteral("LPT2"),
        QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"),   QStringLiteral("LPT6"),    QStringLiteral("LPT7"),
        QStringLiteral("LPT8"), QStringLiteral("LPT9"), QStringLiteral("CONIN$"), QStringLiteral("CONOUT$"), QStringLiteral("CLOCK$")};
    return !setReservedNames.contains(sStem);
}

static bool isExpectedAdvancedRecordType(const QString &sName, quint32 nType)
{
    if (sName.endsWith(QStringLiteral(".msi"), Qt::CaseInsensitive)) {
        return nType == 1;
    }
    if (sName.endsWith(QStringLiteral(".cab"), Qt::CaseInsensitive)) {
        return nType == 7;
    }
    if (sName.endsWith(QStringLiteral(".ini"), Qt::CaseInsensitive)) {
        return nType == 0;
    }
    return false;
}

static QString resolveCaseInsensitiveSibling(const QDir &baseDirectory, const QString &sName)
{
    QFileInfo matchedInfo;
    qint32 nMatches = 0;
    const QFileInfoList listEntries = baseDirectory.entryInfoList(QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo &entryInfo : listEntries) {
        if (entryInfo.fileName().compare(sName, Qt::CaseInsensitive) == 0) {
            matchedInfo = entryInfo;
            nMatches++;
        }
    }

    if ((nMatches != 1) || !matchedInfo.isFile() || matchedInfo.isSymLink()) {
        return QString();
    }

    const QString sBaseCanonical = QFileInfo(baseDirectory.absolutePath()).canonicalFilePath();
    const QString sCandidateCanonical = matchedInfo.canonicalFilePath();
    if (sBaseCanonical.isEmpty() || sCandidateCanonical.isEmpty()) {
        return QString();
    }

#ifdef Q_OS_WIN
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity caseSensitivity = Qt::CaseSensitive;
#endif

    if (QFileInfo(sCandidateCanonical).absolutePath().compare(sBaseCanonical, caseSensitivity) != 0) {
        return QString();
    }

    return sCandidateCanonical;
}
}  // namespace

XAdvancedInstaller::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) XAdvancedInstaller::_deleteUnpackContext(pContext, nullptr);
}

XAdvancedInstaller::XAdvancedInstaller(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackDeferredCleanup = QSharedPointer<UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pUnpackDeferredCleanup;
    m_pUnpackOperationState = QSharedPointer<bool>(new bool(false), ADVANCEDINSTALLER_OPERATION_STATE_DELETER(pDeferredCleanup));
    setIsArchive(true);
}

XAdvancedInstaller::~XAdvancedInstaller()
{
    if (m_pUnpackOperationState) *m_pUnpackOperationState = true;
    if (m_pUnpackDeferredCleanup) {
        m_pUnpackDeferredCleanup->setContexts.unite(m_setUnpackContexts);
        m_setUnpackContexts.clear();
    }
    m_pUnpackDeferredCleanup.clear();
    m_pUnpackOperationState.clear();
}

bool XAdvancedInstaller::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XAdvancedInstaller> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XAdvancedInstaller::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAdvancedInstaller x(pDevice);
    return x.isValid(pPdStruct);
}

XAdvancedInstaller::INTERNAL_INFO XAdvancedInstaller::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XAdvancedInstaller::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XAdvancedInstaller> guardedThis(this);
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

void *XAdvancedInstaller::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XAdvancedInstaller> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XAdvancedInstaller::setInternalInfo(void *pInternalInfo)
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

XBinary::FT XAdvancedInstaller::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_ADVANCEDINSTALLER;
    }

    return FT_PE32_ADVANCEDINSTALLER;
}

static quint32 aiReadFooter32(const QByteArray &baFooter, qint32 nOffset)
{
    return XBinary::_read_uint32(baFooter.data() + nOffset, false);
}

static void aiAppendMarkerOffset(QList<qint64> *pListMarkerOffsets, qint64 nFooterPrefixSize, qint64 nMarkerSize, qint64 nSize, qint64 nMarkerOffset)
{
    if ((nMarkerOffset >= nFooterPrefixSize) && ((nMarkerOffset + nMarkerSize) <= nSize) && !pListMarkerOffsets->contains(nMarkerOffset)) {
        pListMarkerOffsets->append(nMarkerOffset);
    }
}

static bool aiIsValidCertificateTable(XAdvancedInstaller *pBinary, const XBinary::OFFSETSIZE &signRegion, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    if ((signRegion.nSize < 8) || (signRegion.nOffset < 0) || (signRegion.nOffset & 7) || (signRegion.nOffset > nSize) ||
        (signRegion.nSize > (nSize - signRegion.nOffset)) || ((signRegion.nOffset + signRegion.nSize) != nSize)) {
        return false;
    }

    qint64 nCurrentOffset = signRegion.nOffset;
    qint64 nRemaining = signRegion.nSize;
    qint32 nRecords = 0;
    while (nRemaining > 0) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nRemaining < 8) || (++nRecords > 4096)) {
            return false;
        }
        QByteArray baCertificateHeader = pBinary->read_array_process(nCurrentOffset, 8, pPdStruct);
        if (baCertificateHeader.size() != 8) return false;

        const quint32 nCertificateLength = XBinary::_read_uint32(baCertificateHeader.data(), false);
        const quint16 nRevision = XBinary::_read_uint16(baCertificateHeader.data() + 4, false);
        const quint16 nCertificateType = XBinary::_read_uint16(baCertificateHeader.data() + 6, false);
        if ((nCertificateLength < 8) || (nCertificateLength > (quint64)nRemaining) || ((nRevision != 0x0100) && (nRevision != 0x0200)) || (nCertificateType < 1) ||
            (nCertificateType > 4)) {
            return false;
        }

        const qint64 nAlignedLength = ((qint64)nCertificateLength + 7) & ~(qint64)7;
        if ((nAlignedLength < nCertificateLength) || (nAlignedLength > nRemaining)) return false;
        nCurrentOffset += nAlignedLength;
        nRemaining -= nAlignedLength;
    }
    return nRecords > 0;
}

XAdvancedInstaller::EXE_FOOTER XAdvancedInstaller::_parseExeMarker(qint64 nSize, qint64 nFooterPrefixSize, const QByteArray &baMarker, qint64 nMarkerOffset,
                                                                   PDSTRUCT *pPdStruct)
{
    EXE_FOOTER result = {};
    const qint64 nFooterOffset = nMarkerOffset - nFooterPrefixSize;
    if ((nFooterOffset < 0) || ((nMarkerOffset + baMarker.size()) > nSize) || (read_array_process(nMarkerOffset, baMarker.size(), pPdStruct) != baMarker)) {
        return EXE_FOOTER();
    }

    QByteArray baFooter = read_array_process(nFooterOffset, nFooterPrefixSize, pPdStruct);
    if (baFooter.size() != nFooterPrefixSize) return EXE_FOOTER();

    result.nFooterOffset = nFooterOffset;
    result.nMarkerOffset = nMarkerOffset;
    result.nMode = aiReadFooter32(baFooter, 0);
    result.nExternalNameOffset = aiReadFooter32(baFooter, 4);
    result.nNumberOfFiles = aiReadFooter32(baFooter, 8);
    result.nFormatVersion = aiReadFooter32(baFooter, 12);
    result.nMetadataEndOffset = aiReadFooter32(baFooter, 16);
    result.nInfoOffset = aiReadFooter32(baFooter, 20);
    result.nFileDataOffset = aiReadFooter32(baFooter, 24);

    for (qint32 i = 28; i < 60; i++) {
        const char ch = baFooter.at(i);
        if (!(((ch >= '0') && (ch <= '9')) || ((ch >= 'A') && (ch <= 'F')) || ((ch >= 'a') && (ch <= 'f')))) {
            return EXE_FOOTER();
        }
    }

    // These fields are stable for the version-100 SFX container. Requiring
    // a coherent, in-file layout prevents a trailer string alone from
    // detecting.
    if ((result.nMode > 1) || (result.nFormatVersion != 100) || (result.nNumberOfFiles == 0) || (result.nNumberOfFiles > 4096) ||
        (result.nFileDataOffset >= result.nInfoOffset) || (result.nInfoOffset >= (quint64)nFooterOffset) || (result.nMetadataEndOffset != (quint64)nFooterOffset)) {
        return EXE_FOOTER();
    }

    if (result.nMode == 0) {
        if (result.nExternalNameOffset != (quint64)nFooterOffset) return EXE_FOOTER();
    } else if ((result.nExternalNameOffset < result.nInfoOffset) || (result.nExternalNameOffset >= (quint64)nFooterOffset)) {
        return EXE_FOOTER();
    }

    result.bIsValid = true;
    return result;
}

XAdvancedInstaller::EXE_FOOTER XAdvancedInstaller::_readExeFooter(PDSTRUCT *pPdStruct)
{
    const qint64 nSize = getSize();
    const qint64 nFooterPrefixSize = 64;
    const QByteArray baMarker("ADVINSTSFX", 10);
    if (nSize < (nFooterPrefixSize + baMarker.size())) return EXE_FOOTER();

    QList<qint64> listMarkerOffsets;

    // Unsigned bootstrappers end in the marker. If Authenticode signing was
    // applied after the container was built, the certificate table starts
    // immediately after that marker (apart from at most seven alignment zeroes).
    QList<qint64> listLogicalEnds;
    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (pe.isValid(pPdStruct)) {
        OFFSETSIZE signRegion = pe.getSignOffsetSize();
        if (aiIsValidCertificateTable(this, signRegion, nSize, pPdStruct)) {
            listLogicalEnds.append(signRegion.nOffset);
        }
    }
    if (listLogicalEnds.isEmpty()) {
        listLogicalEnds.append(nSize);
    }

    for (qint64 nLogicalEnd : listLogicalEnds) {
        for (qint32 nPadding = 0; nPadding <= 7; nPadding++) {
            const qint64 nMarkerOffset = nLogicalEnd - baMarker.size() - nPadding;
            if (nMarkerOffset < nFooterPrefixSize) continue;

            bool bZeroPadding = true;
            if (nPadding) {
                QByteArray baPadding = read_array_process(nMarkerOffset + baMarker.size(), nPadding, pPdStruct);
                bZeroPadding = (baPadding.size() == nPadding);
                for (char ch : baPadding) {
                    if (ch != 0) {
                        bZeroPadding = false;
                        break;
                    }
                }
            }
            if (bZeroPadding && (read_array_process(nMarkerOffset, baMarker.size(), pPdStruct) == baMarker)) {
                aiAppendMarkerOffset(&listMarkerOffsets, nFooterPrefixSize, baMarker.size(), nSize, nMarkerOffset);
            }
        }
    }

    // Preserve compatibility with older wrappers carrying a small unindexed
    // suffix, but examine every candidate rather than letting a later decoy
    // marker hide a coherent footer.
    for (qint64 nLogicalEnd : listLogicalEnds) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return EXE_FOOTER();
        const qint64 nSearchSize = qMin<qint64>(nLogicalEnd, 0x4000 + baMarker.size());
        const qint64 nSearchOffset = nLogicalEnd - nSearchSize;
        QByteArray baTail = read_array_process(nSearchOffset, nSearchSize, pPdStruct);
        if (baTail.size() == nSearchSize) {
            qint64 nBefore = baTail.size();
            while (nBefore > 0) {
                qint64 nRelativeMarker = baTail.lastIndexOf(baMarker, nBefore - 1);
                if (nRelativeMarker < 0) break;
                aiAppendMarkerOffset(&listMarkerOffsets, nFooterPrefixSize, baMarker.size(), nSize, nSearchOffset + nRelativeMarker);
                nBefore = nRelativeMarker;
            }
        }
    }

    EXE_FOOTER selectedFooter = {};
    for (qint64 nMarkerOffset : listMarkerOffsets) {
        EXE_FOOTER result = _parseExeMarker(nSize, nFooterPrefixSize, baMarker, nMarkerOffset, pPdStruct);
        if (result.bIsValid) {
            if (!_validateExeFooter(result, nullptr, nullptr, pPdStruct)) continue;

            // Multiple independently coherent layouts make the logical
            // container boundary ambiguous; never let candidate ordering pick
            // one, nor let a structurally plausible decoy hide the real one.
            if (selectedFooter.bIsValid) return EXE_FOOTER();
            selectedFooter = result;
        }
    }

    return selectedFooter;
}

static bool aiRegionLess(const QPair<qint64, qint64> &a, const QPair<qint64, qint64> &b)
{
    if (a.first != b.first) return a.first < b.first;
    return a.second < b.second;
}

QString XAdvancedInstaller::_readUTF16Name(qint64 nOffset, quint32 nNumberOfCharacters, qint64 nLimit, PDSTRUCT *pPdStruct)
{
    if ((nNumberOfCharacters == 0) || (nNumberOfCharacters > 32767) || (nOffset < 0) || (nLimit < nOffset)) return QString();

    const qint64 nByteSize = (qint64)nNumberOfCharacters * 2;
    if ((nByteSize < 0) || (nByteSize > (nLimit - nOffset))) return QString();

    QByteArray baName = read_array_process(nOffset, nByteSize, pPdStruct);
    if (baName.size() != nByteSize) return QString();

    QString sResult;
    sResult.reserve((qint32)nNumberOfCharacters);
    for (quint32 i = 0; i < nNumberOfCharacters; i++) {
        quint16 nCharacter = (quint8)baName.at((qint32)(i * 2));
        nCharacter |= (quint16)((quint8)baName.at((qint32)(i * 2 + 1))) << 8;
        if (nCharacter == 0) return QString();
        sResult.append(QChar(nCharacter));
    }

    return sResult;
}

bool XAdvancedInstaller::_readExeFiles(const EXE_FOOTER &footer, QList<EXE_FILE> *pListFiles, qint64 *pnMetadataEnd, PDSTRUCT *pPdStruct)
{
    if (!footer.bIsValid || !pListFiles) return false;
    pListFiles->clear();

    qint64 nOffset = footer.nInfoOffset;
    const qint64 nMetadataLimit = (footer.nMode == 0) ? footer.nFooterOffset : footer.nExternalNameOffset;
    qint64 nDecodedSize = 0;
    QSet<quint32> setIndexes;
    QSet<QString> setNames;
    QList<QPair<qint64, qint64> > listRegions;

    for (quint32 i = 0; i < footer.nNumberOfFiles; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nOffset < 0) || (nOffset > (nMetadataLimit - 24))) return false;

        QByteArray baHeader = read_array_process(nOffset, 24, pPdStruct);
        if (baHeader.size() != 24) return false;

        EXE_FILE file = {};
        file.nType = XBinary::_read_uint32(baHeader.data(), false);
        file.nIndex = XBinary::_read_uint32(baHeader.data() + 4, false);
        file.nXorFlag = XBinary::_read_uint32(baHeader.data() + 8, false);
        file.nSize = XBinary::_read_uint32(baHeader.data() + 12, false);
        file.nDataOffset = XBinary::_read_uint32(baHeader.data() + 16, false);
        quint32 nNameCharacters = XBinary::_read_uint32(baHeader.data() + 20, false);

        const qint64 nNameOffset = nOffset + 24;
        file.sName = _readUTF16Name(nNameOffset, nNameCharacters, nMetadataLimit, pPdStruct);
        QString sNormalizedName = file.sName.toCaseFolded();
        if (!isSafeAdvancedRecordName(file.sName) || !isExpectedAdvancedRecordType(file.sName, file.nType) || ((file.nXorFlag != 0) && (file.nXorFlag != 2)) ||
            setIndexes.contains(file.nIndex) || setNames.contains(sNormalizedName)) {
            return false;
        }
        setIndexes.insert(file.nIndex);
        setNames.insert(sNormalizedName);

        const qint64 nNameBytes = (qint64)nNameCharacters * 2;
        nOffset = nNameOffset + nNameBytes;

        const qint64 nDataOffset = file.nDataOffset;
        const qint64 nDataSize = file.nSize;
        if (((nDataSize == 0) && (file.nType != 0)) || (nDataOffset < footer.nFileDataOffset) || (nDataOffset > ((qint64)footer.nInfoOffset - nDataSize))) {
            return false;
        }

        // A corrupt table with repeatedly overlapping records must not amplify
        // one input region into an unbounded collection of QByteArrays.
        if (nDecodedSize > (getSize() - nDataSize)) return false;
        nDecodedSize += nDataSize;
        if (nDecodedSize > getSize()) return false;

        listRegions.append(qMakePair(nDataOffset, nDataOffset + nDataSize));
        pListFiles->append(file);
    }

    std::sort(listRegions.begin(), listRegions.end(), aiRegionLess);
    if (listRegions.isEmpty() || (listRegions.first().first != footer.nFileDataOffset) || (listRegions.last().second != footer.nInfoOffset)) {
        return false;
    }
    for (qint32 i = 1; i < listRegions.size(); i++) {
        if (listRegions.at(i).first != listRegions.at(i - 1).second) {
            return false;
        }
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct) || (nOffset > nMetadataLimit)) return false;
    if (footer.nMode == 0) {
        if (nOffset != footer.nFooterOffset) return false;
    } else if (nOffset != footer.nExternalNameOffset) {
        return false;
    }

    if (pnMetadataEnd) *pnMetadataEnd = nOffset;
    return true;
}

QByteArray XAdvancedInstaller::_readExeFile(const EXE_FILE &file, PDSTRUCT *pPdStruct)
{
    if ((file.nSize == 0) || (file.nSize > (quint64)(std::numeric_limits<int>::max)()) || ((file.nXorFlag != 0) && (file.nXorFlag != 2))) {
        return QByteArray();
    }

    QByteArray baResult = read_array_process(file.nDataOffset, file.nSize, pPdStruct);
    if (baResult.size() != (qint64)file.nSize) return QByteArray();

    if (file.nXorFlag == 2) {
        const qint32 nXorSize = qMin<qint32>(baResult.size(), 0x200);
        for (qint32 i = 0; i < nXorSize; i++) {
            baResult[i] = (char)((quint8)baResult.at(i) ^ 0xFF);
        }
    }

    return baResult;
}

QString XAdvancedInstaller::_readExternalMSIName(const EXE_FOOTER &footer, qint64 nMetadataEnd, PDSTRUCT *pPdStruct)
{
    if (!footer.bIsValid || (footer.nMode != 1) || (nMetadataEnd != footer.nExternalNameOffset) || ((qint64)footer.nExternalNameOffset > (footer.nFooterOffset - 8))) {
        return QString();
    }

    QByteArray baHeader = read_array_process(footer.nExternalNameOffset, 8, pPdStruct);
    if ((baHeader.size() != 8) || (XBinary::_read_uint32(baHeader.data(), false) != 0)) return QString();

    quint32 nNameCharacters = XBinary::_read_uint32(baHeader.data() + 4, false);
    qint64 nNameOffset = (qint64)footer.nExternalNameOffset + 8;
    QString sName = _readUTF16Name(nNameOffset, nNameCharacters, footer.nFooterOffset, pPdStruct);
    if (sName.isEmpty() || ((nNameOffset + (qint64)nNameCharacters * 2) != footer.nFooterOffset)) return QString();

    // The name is used for filesystem access, so only an exact basename is
    // permitted. Drive-relative paths and NTFS alternate streams are rejected.
    if ((sName == ".") || (sName == "..") || sName.contains('/') || sName.contains('\\') || sName.contains(':') || (QFileInfo(sName).fileName() != sName) ||
        !sName.endsWith(QStringLiteral(".msi"), Qt::CaseInsensitive) || !isSafeAdvancedRecordName(sName)) {
        return QString();
    }

    return sName;
}

bool XAdvancedInstaller::_validateExeFooter(const EXE_FOOTER &footer, QList<EXE_FILE> *pListFiles, qint64 *pnMetadataEnd, PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QList<EXE_FILE> listFiles;
    QList<EXE_FILE> *pFiles = pListFiles ? pListFiles : &listFiles;
    qint64 nMetadataEnd = 0;
    if (!_readExeFiles(footer, pFiles, &nMetadataEnd, pPdStruct)) return false;

    qint32 nMSIRecords = 0;
    qint32 nINIRecords = 0;
    for (const EXE_FILE &file : *pFiles) {
        if (file.sName.endsWith(QStringLiteral(".msi"), Qt::CaseInsensitive)) {
            if ((++nMSIRecords != 1) || (file.nXorFlag != 2)) return false;
        } else if (file.sName.endsWith(QStringLiteral(".cab"), Qt::CaseInsensitive)) {
            if (file.nXorFlag != 2) return false;
        } else if (file.sName.endsWith(QStringLiteral(".ini"), Qt::CaseInsensitive)) {
            if ((++nINIRecords != 1) || (file.nXorFlag != 0)) return false;
        } else {
            return false;
        }
    }

    if (footer.nMode == 1) {
        if ((pFiles->size() != 1) || (nMSIRecords != 0) || (nINIRecords != 1) || _readExternalMSIName(footer, nMetadataEnd, pPdStruct).isEmpty()) {
            return false;
        }
    } else if ((nMSIRecords != 1) || (nINIRecords != 1)) {
        return false;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (pnMetadataEnd) *pnMetadataEnd = nMetadataEnd;
    return true;
}

QIODevice *XAdvancedInstaller::_openExternalMSI(const QString &sName)
{
    QPointer<XAdvancedInstaller> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    const quint64 nGeneration = getDeviceGeneration();
    if (!guardedThis || !guardedDevice || sName.isEmpty()) return nullptr;

    // qobject_cast invokes the caller-controlled virtual meta-object hooks.
    // RTTI performs only the type query; the typed QPointer then guards the
    // QFile-specific inspection.
    QFile *pContainerFile = dynamic_cast<QFile *>(guardedDevice.data());
    QPointer<QFile> guardedContainerFile(pContainerFile);
    if (!guardedThis || !guardedDevice || !guardedContainerFile || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedDevice.data())) return nullptr;

    const QString sContainerFileName = guardedContainerFile->fileName();
    if (!guardedThis || !guardedDevice || !guardedContainerFile || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedDevice.data())) return nullptr;

    QFileInfo containerInfo(sContainerFileName);
    const QString sCandidateCanonical = resolveCaseInsensitiveSibling(containerInfo.absoluteDir(), sName);
    if (!guardedThis || !guardedDevice || !guardedContainerFile || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedDevice.data()) ||
        sCandidateCanonical.isEmpty())
        return nullptr;

    QFile *pResult = new QFile(sCandidateCanonical);
    if (!pResult->open(QIODevice::ReadOnly)) {
        delete pResult;
        return nullptr;
    }

    if (!guardedThis || !guardedDevice || !guardedContainerFile || (getDeviceGeneration() != nGeneration) || (getDevice() != guardedDevice.data())) {
        delete pResult;
        return nullptr;
    }

    return pResult;
}

bool XAdvancedInstaller::_deleteUnpackContext(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pContext) return true;

    QPointer<QIODevice> guardedOwnedDevice(pContext->pOwnedDevice);
    bool bResult = true;
    if (pContext->pMSI) {
        if (pContext->bInnerInitialized && !pContext->pMSI->finishUnpack(&pContext->innerState, nullptr)) {
            bResult = false;
        }
        delete pContext->pMSI;
    }

    if (pContext->pSourceValidator) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
    }

    if (guardedOwnedDevice) {
        if (guardedOwnedDevice->isOpen()) guardedOwnedDevice->close();
        if (guardedOwnedDevice) delete guardedOwnedDevice.data();
    }

    delete pContext;
    return bResult;
}

bool XAdvancedInstaller::_initMSIDelegate(UNPACK_STATE *pState, QIODevice *pSourceDevice, QIODevice *pOwnedDevice, XArchive *pSourceValidator,
                                          UNPACK_STATE *pSourceValidationState, const QMap<QString, QByteArray> &mapExternalCabinets,
                                          const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XAdvancedInstaller> guardedThis(this);
    QPointer<QIODevice> guardedSource(pSourceDevice);
    QPointer<QIODevice> guardedOwnedDevice(pOwnedDevice);
    if (!pState || !guardedSource || !pSourceValidator || !pSourceValidationState || pSourceValidationState->baUnpackSourceToken.isEmpty() ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || !pSourceValidator->isUnpackSourceCurrent(pSourceValidationState, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedOwnedDevice) {
            if (guardedOwnedDevice->isOpen()) guardedOwnedDevice->close();
            if (guardedOwnedDevice) delete guardedOwnedDevice.data();
        }
        if (pSourceValidator) {
            pSourceValidator->releaseUnpackSource(pSourceValidationState);
            delete pSourceValidator;
        }
        return false;
    }

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT();
    if (!pContext) {
        if (guardedOwnedDevice) {
            if (guardedOwnedDevice->isOpen()) guardedOwnedDevice->close();
            if (guardedOwnedDevice) delete guardedOwnedDevice.data();
        }
        pSourceValidator->releaseUnpackSource(pSourceValidationState);
        delete pSourceValidator;
        return false;
    }
    pContext->pOuterSourceDevice = getDevice();
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pOwnedDevice = pOwnedDevice;
    pContext->pSourceValidator = pSourceValidator;
    if (!pContext->pSourceValidator->transferUnpackSourceOwnership(pSourceValidationState, &pContext->sourceValidationState)) {
        pContext->pSourceValidator->releaseUnpackSource(pSourceValidationState);
        _deleteUnpackContext(pContext, pPdStruct);
        return false;
    }
    pContext->pMSI = new (std::nothrow) XMSI(pSourceDevice);
    if (!pContext->pMSI) {
        _deleteUnpackContext(pContext, pPdStruct);
        return false;
    }
    pContext->pMSI->setExternalCabinetData(mapExternalCabinets);

    if (!pContext->pOuterSourceDevice || !guardedThis || !guardedSource) {
        _deleteUnpackContext(pContext, pPdStruct);
        return false;
    }

    const bool bInnerInitialized = pContext->pMSI->initUnpack(&pContext->innerState, mapProperties, pPdStruct);
    pContext->bInnerInitialized = bInnerInitialized;
    if (!bInnerInitialized || !guardedThis || !guardedSource ||
        !pContext->pSourceValidator->validateAndFinalizeUnpackSource(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedSource) {
        _deleteUnpackContext(pContext, pPdStruct);
        return false;
    }

    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    m_setUnpackContexts.insert(pContext);
    return true;
}

QMap<XBinary::UNPACK_PROP, QVariant> XAdvancedInstaller::getDefaultUnpackProperties()
{
    return XBinary::getDefaultUnpackProperties();
}

bool XAdvancedInstaller::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XAdvancedInstaller> guardedThis(this);
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        m_setUnpackContexts.remove(pOldContext);
        pState->pContext = nullptr;
        const bool bFinishOK = _deleteUnpackContext(pOldContext, nullptr);
        *pState = UNPACK_STATE();
        if (!guardedThis || !bFinishOK) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    std::unique_ptr<XArchive> initialSourceValidator(new XArchive(guardedSource.data()));
    UNPACK_STATE initialSourceState = {};
    if (!initialSourceValidator->bindUnpackSource(&initialSourceState, pPdStruct) || !guardedThis || !guardedSource) return false;
    XAdvancedInstaller detector(guardedSource.data(), isImage(), getModuleAddress());
    const qint64 nTotalSize = guardedSource->size();
    if (!guardedThis || !guardedSource || (nTotalSize < 0)) return false;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = nTotalSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->pContext = nullptr;
    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.clear();

    INTERNAL_INFO info = detector._detect(pPdStruct);
    if (!guardedThis || !guardedSource || !info.bIsValid || !initialSourceValidator->isUnpackSourceCurrent(&initialSourceState, pPdStruct) || !guardedThis ||
        !guardedSource)
        return false;

    if (info.subType == SUBTYPE_MSI) {
        if (!initialSourceValidator->isUnpackSourceCurrent(&initialSourceState, pPdStruct) || !guardedThis || !guardedSource) return false;
        return _initMSIDelegate(pState, guardedSource.data(), nullptr, initialSourceValidator.release(), &initialSourceState, QMap<QString, QByteArray>(), mapProperties,
                                pPdStruct);
    }
    if (info.subType != SUBTYPE_EXE) return false;

    EXE_FOOTER footer = detector._readExeFooter(pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    QList<EXE_FILE> listFiles;
    qint64 nMetadataEnd = 0;
    if (!footer.bIsValid || !detector._validateExeFooter(footer, &listFiles, &nMetadataEnd, pPdStruct) || !guardedThis || !guardedSource) return false;

    if (footer.nMode == 1) {
        if ((listFiles.size() != 1) || !listFiles.first().sName.endsWith(QStringLiteral(".ini"), Qt::CaseInsensitive) || (listFiles.first().nXorFlag != 0)) {
            return false;
        }
        QString sExternalMSIName = detector._readExternalMSIName(footer, nMetadataEnd, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        QIODevice *pExternalMSI = detector._openExternalMSI(sExternalMSIName);
        if (!guardedThis || !pExternalMSI) {
            delete pExternalMSI;
            return false;
        }

        XAdvancedInstaller externalInstaller(pExternalMSI);
        const INTERNAL_INFO *pExternalInfo = static_cast<const INTERNAL_INFO *>(externalInstaller.getInternalInfo(pPdStruct));
        if (!guardedThis || !pExternalInfo) {
            pExternalMSI->close();
            delete pExternalMSI;
            return false;
        }
        const INTERNAL_INFO externalInfo = *pExternalInfo;
        if (!guardedThis || !externalInfo.bIsValid || (externalInfo.subType != SUBTYPE_MSI)) {
            pExternalMSI->close();
            delete pExternalMSI;
            return false;
        }

        if (!initialSourceValidator->isUnpackSourceCurrent(&initialSourceState, pPdStruct) || !guardedThis || !guardedSource) {
            pExternalMSI->close();
            delete pExternalMSI;
            return false;
        }

        return _initMSIDelegate(pState, pExternalMSI, pExternalMSI, initialSourceValidator.release(), &initialSourceState, QMap<QString, QByteArray>(), mapProperties,
                                pPdStruct);
    }

    QByteArray baMSI;
    QMap<QString, QByteArray> mapExternalCabinets;
    qint32 nMSIRecords = 0;
    qint32 nINIRecords = 0;

    for (int i = 0; i < listFiles.size(); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const EXE_FILE &file = listFiles.at(i);

        if (file.sName.endsWith(QStringLiteral(".msi"), Qt::CaseInsensitive) && baMSI.isEmpty()) {
            if ((++nMSIRecords != 1) || (file.nXorFlag != 2)) return false;
            baMSI = detector._readExeFile(file, pPdStruct);
            if (!guardedThis || !guardedSource || baMSI.isEmpty()) return false;
        } else if (file.sName.endsWith(QStringLiteral(".cab"), Qt::CaseInsensitive)) {
            if (file.nXorFlag != 2) return false;
            QString sCabinetName = QFileInfo(file.sName).fileName();
            bool bDuplicate = false;
            for (QMap<QString, QByteArray>::const_iterator it = mapExternalCabinets.constBegin(); it != mapExternalCabinets.constEnd(); ++it) {
                if (it.key().compare(sCabinetName, Qt::CaseInsensitive) == 0) {
                    bDuplicate = true;
                    break;
                }
            }
            if ((sCabinetName != file.sName) || (sCabinetName == ".") || (sCabinetName == "..") || sCabinetName.contains('/') || sCabinetName.contains('\\') ||
                sCabinetName.contains(':') || bDuplicate) {
                return false;
            }
            QByteArray baCabinet = detector._readExeFile(file, pPdStruct);
            if (!guardedThis || !guardedSource || baCabinet.isEmpty()) return false;
            mapExternalCabinets.insert(sCabinetName, baCabinet);
        } else if (file.sName.endsWith(QStringLiteral(".ini"), Qt::CaseInsensitive)) {
            if ((++nINIRecords != 1) || (file.nXorFlag != 0)) return false;
        } else {
            return false;
        }
    }

    if (baMSI.isEmpty() || (nMSIRecords != 1) || (nINIRecords != 1)) return false;

    QBuffer *pMSIBuffer = new QBuffer;
    pMSIBuffer->setData(baMSI);
    if (!pMSIBuffer->open(QIODevice::ReadOnly)) {
        delete pMSIBuffer;
        return false;
    }

    if (!initialSourceValidator->isUnpackSourceCurrent(&initialSourceState, pPdStruct) || !guardedThis || !guardedSource) {
        pMSIBuffer->close();
        delete pMSIBuffer;
        return false;
    }

    return _initMSIDelegate(pState, pMSIBuffer, pMSIBuffer, initialSourceValidator.release(), &initialSourceState, mapExternalCabinets, mapProperties, pPdStruct);
}

XBinary::ARCHIVERECORD XAdvancedInstaller::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XAdvancedInstaller> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pMSI || !pContext->bInnerInitialized ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return result;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis ||
        !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext))
        return result;

    result = pContext->pMSI->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return ARCHIVERECORD();
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !m_setUnpackContexts.contains(pContext) ||
        (pState->pContext != pContext))
        return ARCHIVERECORD();
    return result;
}

bool XAdvancedInstaller::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XAdvancedInstaller> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || !guardedThis || !guardedOutput || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pMSI || !pContext->bInnerInitialized ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    if (XBinary::devicesAlias(pContext->pOuterSourceDevice.data(), guardedOutput.data()) || !guardedThis || !guardedOutput) return false;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis ||
        !guardedOutput || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext))
        return false;

    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = pContext->pMSI->unpackCurrent(&pContext->innerState, guardedOutput.data(), pPdStruct);
    if (!guardedThis || !guardedOutput || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !guardedOutput ||
        !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) {
        if (guardedOutput) {
            XBinary::resize(guardedOutput.data(), 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XAdvancedInstaller::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XAdvancedInstaller> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pMSI || !pContext->bInnerInitialized ||
        (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis ||
        !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext))
        return false;

    bool bResult = pContext->pMSI->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !guardedThis || !m_setUnpackContexts.contains(pContext) ||
        (pState->pContext != pContext))
        return false;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XAdvancedInstaller::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XAdvancedInstaller> guardedThis(this);

    bool bResult = true;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        m_setUnpackContexts.remove(pContext);
        pState->pContext = nullptr;
        bResult = _deleteUnpackContext(pContext, pPdStruct);
        if (!guardedThis) return false;
    }

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    return bResult;
}

static QString aiNormalizeVersion(const QString &sText)
{
    QString sVersion;
    for (QChar character : sText) {
        if (character.isDigit() || (character == '.')) {
            sVersion.append(character);
        } else {
            break;
        }
    }

    QStringList listParts = sVersion.split('.', Qt::KeepEmptyParts);
    bool bMajor = false;
    bool bMinor = false;
    if ((listParts.size() >= 2) && !listParts.at(0).isEmpty() && !listParts.at(1).isEmpty()) {
        listParts.at(0).toUInt(&bMajor);
        listParts.at(1).toUInt(&bMinor);
    }
    return (bMajor && bMinor) ? (listParts.at(0) + "." + listParts.at(1)) : QString();
}

// Read the version token after the "Advanced Installer " string (e.g. "23.9").
static QString aiReadVersion(XAdvancedInstaller *pBinary, qint64 nSize, XBinary::PDSTRUCT *pPdStruct)
{
    QString sFallback;
    qint64 nPos = pBinary->find_ansiString(0, nSize, "Advanced Installer ", pPdStruct);
    if (nPos != -1) {
        QString sTail = pBinary->read_ansiString(nPos + 19, 32).trimmed();  // after "Advanced Installer "
        QString sVersion = aiNormalizeVersion(sTail);
        if (!sVersion.isEmpty()) return sVersion;

        for (QChar character : sTail) {
            if (character.isDigit()) {
                sFallback.append(character);
            } else {
                break;
            }
        }
    }

    // Version-100 bootstrappers and some MSI custom-action resources store
    // the authoring version as a UTF-16 value near an "Advanced Installer"
    // label.
    qint64 nSearchOffset = 0;
    const QString sUnicodeLabel = QStringLiteral("Advanced Installer");
    while (nSearchOffset < nSize) {
        nPos = pBinary->find_unicodeString(nSearchOffset, nSize - nSearchOffset, sUnicodeLabel, false, pPdStruct);
        if (nPos == -1) break;

        qint64 nScanStart = nPos + (qint64)sUnicodeLabel.size() * 2;
        qint64 nScanEnd = qMin(nSize - 2, nScanStart + 256);
        for (qint64 nCandidateOffset = nScanStart; nCandidateOffset <= nScanEnd; nCandidateOffset += 2) {
            quint16 nCharacter = pBinary->read_uint16(nCandidateOffset, false);
            quint16 nPrevious = (nCandidateOffset >= 2) ? pBinary->read_uint16(nCandidateOffset - 2, false) : 0;
            if ((nCharacter < '0') || (nCharacter > '9') || ((nPrevious != 0) && !QChar(nPrevious).isSpace())) {
                continue;
            }

            QString sCandidate;
            qint64 nAfter = nCandidateOffset;
            while ((nAfter <= nScanEnd) && (sCandidate.size() < 32)) {
                quint16 nValue = pBinary->read_uint16(nAfter, false);
                if (((nValue < '0') || (nValue > '9')) && (nValue != '.')) break;
                sCandidate.append(QChar(nValue));
                nAfter += 2;
            }

            quint16 nTerminator = (nAfter <= (nSize - 2)) ? pBinary->read_uint16(nAfter, false) : 0;
            QString sVersion = aiNormalizeVersion(sCandidate);
            if (!sVersion.isEmpty() && ((nTerminator == 0) || QChar(nTerminator).isSpace())) {
                return sVersion;
            }
        }

        nSearchOffset = nPos + 2;
    }

    return sFallback;
}

XAdvancedInstaller::INTERNAL_INFO XAdvancedInstaller::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.subType = SUBTYPE_UNKNOWN;

    const qint64 nSize = getSize();
    if (nSize < 0x40) return result;

    // EXE bootstrapper: validated version-100 "ADVINSTSFX" footer.
    EXE_FOOTER footer = _readExeFooter(pPdStruct);
    if (footer.bIsValid) {
        XPE pe(getDevice(), isImage(), getModuleAddress());
        if (!pe.isValid(pPdStruct)) return result;

        result.bIsValid = true;
        result.subType = SUBTYPE_EXE;
        result.sVersion = aiReadVersion(this, nSize, pPdStruct);
        return result;
    }

    // MSI authored by Advanced Installer.
    if (XMSI::isValid(getDevice(), pPdStruct)) {
        bool bAI = (find_ansiString(0, nSize, "aicustact.dll", pPdStruct) != -1) && (find_ansiString(0, nSize, "Advanced Installer", pPdStruct) != -1);
        if (bAI) {
            result.bIsValid = true;
            result.subType = SUBTYPE_MSI;
            result.sVersion = aiReadVersion(this, nSize, pPdStruct);
            return result;
        }
    }

    return result;
}
