/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xgamestorearchive_p.h"

#include <QtEndian>

#include <new>

namespace {
class GameDevicePositionGuard {
public:
    explicit GameDevicePositionGuard(QIODevice *pDevice)
        : m_pDevice(pDevice), m_nPosition(-1), m_bRestored(false)
    {
        if (!m_pDevice) return;
        const bool bSequential = m_pDevice->isSequential();
        if (!m_pDevice || bSequential) return;
        m_nPosition = m_pDevice->pos();
    }

    ~GameDevicePositionGuard()
    {
        restore();
    }

    bool isValid() const
    {
        return m_pDevice && (m_nPosition >= 0);
    }

    bool restore()
    {
        if (m_bRestored) return true;
        m_bRestored = true;
        if (!m_pDevice || (m_nPosition < 0)) return false;
        const bool bSeeked = m_pDevice->seek(m_nPosition);
        if (!m_pDevice || !bSeeked) return false;
        const qint64 nPosition = m_pDevice->pos();
        return m_pDevice && (nPosition == m_nPosition);
    }

private:
    QPointer<QIODevice> m_pDevice;
    qint64 m_nPosition;
    bool m_bRestored;
};

QString appendDuplicateSuffix(const QString &sComponent, qint32 nSuffix)
{
    const qint32 nDot = sComponent.lastIndexOf(QLatin1Char('.'));
    const QString sSuffix = QStringLiteral("_%1").arg(nSuffix);
    if (nDot > 0) {
        return sComponent.left(nDot) + sSuffix + sComponent.mid(nDot);
    }
    return sComponent + sSuffix;
}
}  // namespace

XGameStoreArchiveBase::XGameStoreArchiveBase(QIODevice *pDevice, FT fileType)
    : XArchive(pDevice), m_fileTypeHint(fileType)
{
    setFileType(fileType);
}

quint32 XGameStoreArchiveBase::readLE32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}

bool XGameStoreArchiveBase::rangeWithin(qint64 nTotalSize, qint64 nOffset,
                                        qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool XGameStoreArchiveBase::rangesOverlap(qint64 nOffset1, qint64 nSize1,
                                          qint64 nOffset2, qint64 nSize2)
{
    if ((nSize1 <= 0) || (nSize2 <= 0)) return false;
    return (nOffset1 < (nOffset2 + nSize2)) &&
           (nOffset2 < (nOffset1 + nSize1));
}

bool XGameStoreArchiveBase::decodeName(const uchar *pData,
                                       qint32 nFieldSize, bool bAsciiOnly,
                                       QString *pName)
{
    if (!pData || !pName || (nFieldSize <= 0)) return false;

    qint32 nLength = 0;
    while ((nLength < nFieldSize) && pData[nLength]) nLength++;
    if (nLength == 0) return false;

    for (qint32 i = 0; i < nLength; ++i) {
        const quint8 nCharacter = pData[i];
        if ((nCharacter < 0x20) ||
            ((nCharacter >= 0x7f) && (nCharacter <= 0x9f)) ||
            (bAsciiOnly && (nCharacter > 0x7e))) {
            return false;
        }
    }

    QString sName = QString::fromLatin1(
        reinterpret_cast<const char *>(pData), nLength);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (sName.isEmpty() || sName.startsWith(QLatin1Char('/'))) return false;

    const QStringList listParts =
        sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) ||
            (sPart == QLatin1String(".."))) {
            return false;
        }
    }

    sName = sName.normalized(QString::NormalizationForm_C);
    if (XBinary::fixFileName(sName) != sName) return false;

    *pName = sName;
    return true;
}

bool XGameStoreArchiveBase::makeUniquePath(
    const QString &sSource, QSet<QString> *pUsedFiles,
    QSet<QString> *pUsedDirectories,
    QHash<QString, qint32> *pNextSuffixes,
    QHash<QString, QString> *pResolvedDirectories, QString *pResult)
{
    if (!pUsedFiles || !pUsedDirectories || !pNextSuffixes ||
        !pResolvedDirectories || !pResult || sSource.isEmpty()) {
        return false;
    }

    const QStringList listSource =
        sSource.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    if (listSource.isEmpty()) return false;

    QStringList listResolved;
    for (qint32 i = 0; i < listSource.count(); ++i) {
        const bool bLeaf = (i + 1 == listSource.count());
        const QString sOriginalComponent = listSource.at(i);
        QString sComponent = sOriginalComponent;
        QString sPath;
        QString sKey;
        bool bFound = false;
        const QString sBasePath = listResolved.isEmpty()
            ? sOriginalComponent
            : (listResolved.join(QLatin1Char('/')) + QLatin1Char('/') +
               sOriginalComponent);
        const QString sBaseKey = sBasePath.toCaseFolded();
        qint32 nSuffix = 1;

        if (!bLeaf && pResolvedDirectories->contains(sBaseKey)) {
            sPath = pResolvedDirectories->value(sBaseKey);
            sKey = sPath.toCaseFolded();
            const qint32 nSeparator = sPath.lastIndexOf(QLatin1Char('/'));
            sComponent = sPath.mid(nSeparator + 1);
            if (sComponent.isEmpty() || pUsedFiles->contains(sKey) ||
                !pUsedDirectories->contains(sKey) ||
                (XBinary::fixFileName(sPath) != sPath)) {
                return false;
            }
            listResolved.append(sComponent);
            continue;
        }

        while (nSuffix <= (MAX_RECORDS + 1)) {
            sComponent = (nSuffix == 1)
                ? sOriginalComponent
                : appendDuplicateSuffix(sOriginalComponent, nSuffix);
            sPath = listResolved.isEmpty()
                ? sComponent
                : (listResolved.join(QLatin1Char('/')) +
                   QLatin1Char('/') + sComponent);
            sKey = sPath.toCaseFolded();
            const bool bCollision = pUsedFiles->contains(sKey) ||
                (bLeaf && pUsedDirectories->contains(sKey));
            if (!bCollision) {
                bFound = true;
                break;
            }
            nSuffix = (nSuffix == 1)
                ? pNextSuffixes->value(sBaseKey, 2)
                : (nSuffix + 1);
        }

        if (!bFound || (XBinary::fixFileName(sPath) != sPath)) return false;
        const qint32 nNextSuffix = (nSuffix <= MAX_RECORDS)
            ? qMax(2, nSuffix + 1) : (MAX_RECORDS + 1);
        pNextSuffixes->insert(sBaseKey, nNextSuffix);
        listResolved.append(sComponent);
        if (bLeaf) {
            pUsedFiles->insert(sKey);
        } else {
            pUsedDirectories->insert(sKey);
            pResolvedDirectories->insert(sBaseKey, sPath);
        }
    }

    *pResult = listResolved.join(QLatin1Char('/'));
    return !pResult->isEmpty();
}

bool XGameStoreArchiveBase::isValid(PDSTRUCT *pPdStruct)
{
    return scanArchive(nullptr, nullptr, pPdStruct);
}

bool XGameStoreArchiveBase::scanArchive(QList<ENTRY> *pEntries,
                                        qint64 *pArchiveEnd,
                                        PDSTRUCT *pPdStruct)
{
    if (pEntries) pEntries->clear();
    if (pArchiveEnd) *pArchiveEnd = 0;

    QPointer<XGameStoreArchiveBase> guardedThis(this);
    const FT fileType = getFileType();
    if (((fileType != FT_QUAKE_PAK) && (fileType != FT_DOOM_WAD) &&
         (fileType != FT_BUILD_GRP) &&
         (fileType != FT_PARSEC_ARCHIVE) && (fileType != FT_PMM)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    GameDevicePositionGuard positionGuard(getDevice());
    if (!guardedThis || !positionGuard.isValid()) return false;

    QList<ENTRY> listEntries;
    qint64 nArchiveEnd = 0;
    const bool bResult = scanFormat(
        pEntries ? &listEntries : nullptr, &nArchiveEnd, pPdStruct);

    const bool bRestored = positionGuard.restore();
    if (!guardedThis || !bRestored || !bResult ||
        (getFileType() != fileType) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
    return true;
}

XBinary::FT XGameStoreArchiveBase::getFileType()
{
    return m_fileTypeHint;
}

XBinary::MODE XGameStoreArchiveBase::getMode()
{
    return MODE_DATA;
}

qint32 XGameStoreArchiveBase::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XGameStoreArchiveBase::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XGameStoreArchiveBase::getArch()
{
    return QString();
}

QString XGameStoreArchiveBase::getFileFormatExt()
{
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK) return QStringLiteral("pak");
    if (fileType == FT_DOOM_WAD) return QStringLiteral("wad");
    if (fileType == FT_BUILD_GRP) return QStringLiteral("grp");
    if (fileType == FT_PARSEC_ARCHIVE) return QStringLiteral("dat");
    if (fileType == FT_PMM) return QStringLiteral("pmm");
    return QString();
}

QString XGameStoreArchiveBase::getFileFormatExtsString()
{
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK)
        return QStringLiteral("Quake PAK (*.pak)");
    if (fileType == FT_DOOM_WAD)
        return QStringLiteral("Doom WAD (*.wad)");
    if (fileType == FT_BUILD_GRP)
        return QStringLiteral("Build GRP (*.grp)");
    if (fileType == FT_PARSEC_ARCHIVE)
        return QStringLiteral("Parsec resource archive (*.dat)");
    if (fileType == FT_PMM)
        return QStringLiteral("Parsec PSM music module (*.pmm)");
    return QString();
}

QString XGameStoreArchiveBase::getMIMEString()
{
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK)
        return QStringLiteral("application/x-quake-pak");
    if (fileType == FT_DOOM_WAD)
        return QStringLiteral("application/x-doom-wad");
    if (fileType == FT_BUILD_GRP)
        return QStringLiteral("application/x-build-grp");
    if (fileType == FT_PARSEC_ARCHIVE)
        return QStringLiteral("application/x-parsec-resource-archive");
    if (fileType == FT_PMM)
        return QStringLiteral("audio/x-parsec-pmm");
    return QStringLiteral("application/octet-stream");
}

qint64 XGameStoreArchiveBase::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    qint64 nArchiveEnd = 0;
    if (!scanArchive(nullptr, &nArchiveEnd, pPdStruct)) return 0;
    return nArchiveEnd;
}

XBinary::OSNAME XGameStoreArchiveBase::getOsName()
{
    return OSNAME_MULTIPLATFORM;
}

QString XGameStoreArchiveBase::getVersion()
{
    return QString();
}

QList<QString> XGameStoreArchiveBase::getSearchSignatures()
{
    QList<QString> listResult;
    const FT fileType = getFileType();
    if (fileType == FT_QUAKE_PAK) {
        listResult.append(QStringLiteral("'PACK'"));
    } else if (fileType == FT_DOOM_WAD) {
        listResult.append(QStringLiteral("'IWAD'"));
        listResult.append(QStringLiteral("'PWAD'"));
    } else if (fileType == FT_BUILD_GRP) {
        listResult.append(QStringLiteral("'KenSilverman'"));
    } else if (fileType == FT_PMM) {
        listResult.append(QStringLiteral("'MTCVTS PSM 2.00'00"));
    }
    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant>
XGameStoreArchiveBase::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XGameStoreArchiveBase::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    UNPACK_CONTEXT *pOldContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;

    const bool bBound = bindUnpackSource(pState, pPdStruct);
    if (!guardedThis || !bBound) return false;

    const FT fileType = getFileType();
    QList<ENTRY> listEntries;
    qint64 nArchiveEnd = 0;
    const bool bScanned = scanArchive(&listEntries, &nArchiveEnd, pPdStruct);
    if (!guardedThis) return false;
    if (!bScanned || (getFileType() != fileType) ||
        !isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    const qint64 nTotalSize = getSize();
    if (!guardedThis || !rangeWithin(nTotalSize, 0, nArchiveEnd)) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pContext->listEntries = listEntries;
    pContext->fileType = fileType;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nCurrentOffset = listEntries.isEmpty()
        ? nArchiveEnd : listEntries.constFirst().nHeaderOffset;
    pState->nTotalSize = nTotalSize;
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XGameStoreArchiveBase::infoCurrent(
    UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext)
        return ARCHIVERECORD();

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) return ARCHIVERECORD();

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) ||
        (pContext->fileType != getFileType()) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return ARCHIVERECORD();
    }

    const ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (!rangeWithin(nCurrentSize, entry.nHeaderOffset, entry.nHeaderSize) ||
        !rangeWithin(nCurrentSize, entry.nDataOffset, entry.nDataSize) ||
        entry.sFileName.isEmpty()) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET,
                                entry.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, entry.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XGameStoreArchiveBase::moveToNext(UNPACK_STATE *pState,
                                       PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext)
        return false;

    const bool bSourceCurrent = isUnpackSourceCurrent(pState, pPdStruct);
    if (!guardedThis || !bSourceCurrent ||
        !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if (!guardedThis || (pState->nTotalSize != nCurrentSize) ||
        (pContext->fileType != getFileType()) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    pState->nCurrentIndex++;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XGameStoreArchiveBase::finishUnpack(UNPACK_STATE *pState,
                                         PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    Q_UNUSED(pPdStruct)

    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }

    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete pContext;
    return true;
}

QList<XBinary::FPART_PROP>
XGameStoreArchiveBase::getAvailableFPARTProperties()
{
    QList<FPART_PROP> listResult;
    listResult.append(FPART_PROP_ORIGINALNAME);
    listResult.append(FPART_PROP_UNCOMPRESSEDSIZE);
    listResult.append(FPART_PROP_COMPRESSEDSIZE);
    listResult.append(FPART_PROP_STREAMOFFSET);
    listResult.append(FPART_PROP_STREAMSIZE);
    listResult.append(FPART_PROP_HEADER_OFFSET);
    listResult.append(FPART_PROP_HEADER_SIZE);
    listResult.append(FPART_PROP_FILEMODE);
    return listResult;
}

bool XGameStoreArchiveBase::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    bool bResult = true;
    if (!isInternalInfoHandled()) {
        bResult = guardedThis->XArchive::handleInternalInfo(pPdStruct);
        if (!guardedThis || !bResult) return false;
        XArchive::INTERNAL_INFO *pInfo =
            static_cast<XArchive::INTERNAL_INFO *>(
                guardedThis->XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pInfo) return false;
        static_cast<XArchive::INTERNAL_INFO &>(
            guardedThis->m_internalInfo) = *pInfo;
    }
    return guardedThis && bResult;
}

void *XGameStoreArchiveBase::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XGameStoreArchiveBase> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XGameStoreArchiveBase::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(
            static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}
