/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xspissfx.h"

#include <QPointer>
#include <QScopedValueRollback>
#include <QtEndian>

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

#include "subdevice.h"
#include "xpe.h"
#include "xspis.h"

namespace {
const qint32 SPISSFX_MAX_BLOBS = 10000;
const qint32 SPISSFX_MAX_RECORDS = 100000;
const qint64 SPISSFX_MIN_BLOB_SIZE = 21;

bool spisHeaderPrefix(const QByteArray &baData)
{
    if ((baData.size() < 8) || (memcmp(baData.constData(), "SPIS\x1a", 5) != 0)) return false;
    const QByteArray baTag = baData.mid(5, 3);
    return (baTag == QByteArrayLiteral("NON")) || (baTag == QByteArrayLiteral("RLE")) || (baTag == QByteArrayLiteral("LZH")) ||
           (baTag == QByteArrayLiteral("CUS")) || (baTag == QByteArrayLiteral("LH5"));
}

bool spisResourceLess(const XPE::RESOURCE_RECORD &a,
                      const XPE::RESOURCE_RECORD &b)
{
    if (a.nOffset != b.nOffset) return a.nOffset < b.nOffset;
    return a.nSize < b.nSize;
}

class SPISSFX_OPERATION_STATE_DELETER {
public:
    explicit SPISSFX_OPERATION_STATE_DELETER(
        const QSharedPointer<XSpisSFX::UNPACK_DEFERRED_CLEANUP> &pCleanup)
        : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XSpisSFX::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};
}  // namespace

XSpisSFX::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) {
        if (pContext->pArchive) {
            pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
            delete pContext->pArchive;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
        }
        delete pContext;
    }
}

XSpisSFX::XSpisSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackDeferredCleanup = QSharedPointer<UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pUnpackDeferredCleanup;
    m_pUnpackOperationState = QSharedPointer<bool>(
        new bool(false), SPISSFX_OPERATION_STATE_DELETER(pDeferredCleanup));
    setIsArchive(true);
}

XSpisSFX::~XSpisSFX()
{
    if (m_pUnpackOperationState) *m_pUnpackOperationState = true;
    if (m_pUnpackDeferredCleanup) {
        m_pUnpackDeferredCleanup->setContexts.unite(m_setUnpackContexts);
        m_setUnpackContexts.clear();
    }
    m_pUnpackDeferredCleanup.clear();
    m_pUnpackOperationState.clear();
}

QString XSpisSFX::resourceToken(bool bIsName, quint32 nID, const QString &sName)
{
    if (!bIsName) return QString::number(nID);
    QString result;
    bool bUnderscore = false;
    for (const QChar character : sName.trimmed()) {
        if (character.isLetterOrNumber() || (character == QLatin1Char('.')) || (character == QLatin1Char('-'))) {
            result.append(character.toUpper());
            bUnderscore = false;
        } else if (!bUnderscore) {
            result.append(QLatin1Char('_'));
            bUnderscore = true;
        }
    }
    while (result.startsWith(QLatin1Char('_'))) result.remove(0, 1);
    while (result.endsWith(QLatin1Char('_'))) result.chop(1);
    return result.isEmpty() ? QStringLiteral("NAMED") : result;
}

bool XSpisSFX::appendBlob(INTERNAL_INFO *pInfo, qint64 nOffset, qint64 nSize, const QString &sHint, PDSTRUCT *pPdStruct)
{
    if (!pInfo || (nOffset < 0) || (nSize < SPISSFX_MIN_BLOB_SIZE) || (pInfo->listBlobs.count() >= SPISSFX_MAX_BLOBS) ||
        !checkOffsetSize(nOffset, nSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    QPointer<XSpisSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource) return false;
    SubDevice subDevice(guardedSource.data(), nOffset, nSize);
    subDevice.setProperty("FileName", sHint);
    if (!subDevice.open(QIODevice::ReadOnly)) return false;

    XSPIS archive(&subDevice);
    const XSPIS::INTERNAL_INFO *pSpisInfo = static_cast<const XSPIS::INTERNAL_INFO *>(archive.getInternalInfo(pPdStruct));
    if (!guardedThis || !guardedSource || !pSpisInfo || !pSpisInfo->bIsValid || (pSpisInfo->nFileSize != nSize) || pSpisInfo->listMembers.isEmpty() ||
        (pSpisInfo->listMembers.count() > SPISSFX_MAX_RECORDS - pInfo->listLocations.count())) {
        subDevice.close();
        return false;
    }

    BLOB blob;
    blob.nOffset = nOffset;
    blob.nSize = nSize;
    blob.nRecords = pSpisInfo->listMembers.count();
    blob.sHint = sHint;
    const qint32 nBlobIndex = pInfo->listBlobs.count();
    pInfo->listBlobs.append(blob);
    for (qint32 i = 0; i < blob.nRecords; ++i) {
        LOCATION location;
        location.nBlob = nBlobIndex;
        location.nRecord = i;
        pInfo->listLocations.append(location);
    }
    subDevice.close();
    return guardedThis && guardedSource && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XSpisSFX::discover(INTERNAL_INFO *pInfo, PDSTRUCT *pPdStruct)
{
    if (pInfo) *pInfo = INTERNAL_INFO();
    if (!pInfo || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XSpisSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource || guardedSource->isSequential()) return false;

    XPE pe(guardedSource.data(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct) || !guardedThis || !guardedSource) return false;
    const qint64 nFileSize = getSize();
    if (!guardedThis || !guardedSource || (nFileSize <= 0)) return false;

    // Prefer a completely framed overlay chain. A single malformed/trailing
    // byte invalidates the candidate and falls through to resource discovery.
    const qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    if ((nOverlayOffset >= 0) && (nOverlayOffset < nFileSize) && (nFileSize - nOverlayOffset >= 4 + SPISSFX_MIN_BLOB_SIZE)) {
        INTERNAL_INFO overlayInfo;
        overlayInfo.nFileSize = nFileSize;
        overlayInfo.bOverlayChain = true;
        qint64 nOffset = nOverlayOffset;
        bool bChainValid = true;
        while (bChainValid && (nOffset < nFileSize)) {
            if ((nFileSize - nOffset < 4 + SPISSFX_MIN_BLOB_SIZE) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
                bChainValid = false;
                break;
            }
            const QByteArray baHead = read_array_process(nOffset, 12, pPdStruct);
            if (!guardedThis || !guardedSource || (baHead.size() != 12)) return false;
            const qint64 nBlobSize = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baHead.constData()));
            if ((nBlobSize < SPISSFX_MIN_BLOB_SIZE) || (nBlobSize > nFileSize - nOffset - 4) || !spisHeaderPrefix(baHead.mid(4, 8))) {
                bChainValid = false;
                break;
            }
            const QString sHint = QStringLiteral("spis_%1").arg(nOffset + 4, 0, 16);
            if (!appendBlob(&overlayInfo, nOffset + 4, nBlobSize, sHint, pPdStruct)) {
                bChainValid = false;
                break;
            }
            nOffset += 4 + nBlobSize;
        }
        if (bChainValid && (nOffset == nFileSize) && !overlayInfo.listBlobs.isEmpty() && !overlayInfo.listLocations.isEmpty()) {
            overlayInfo.bIsValid = true;
            *pInfo = overlayInfo;
            return guardedThis && guardedSource;
        }
    }

    INTERNAL_INFO resourceInfo;
    resourceInfo.nFileSize = nFileSize;
    const QList<XPE::RESOURCE_RECORD> listResources = pe.getResources(SPISSFX_MAX_BLOBS, pPdStruct);
    if (!guardedThis || !guardedSource) return false;
    QList<XPE::RESOURCE_RECORD> listSorted = listResources;
    std::sort(listSorted.begin(), listSorted.end(), spisResourceLess);

    for (const XPE::RESOURCE_RECORD &resource : listSorted) {
        if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if ((resource.nSize < SPISSFX_MIN_BLOB_SIZE) || !checkOffsetSize(resource.nOffset, resource.nSize)) continue;
        const QByteArray baPrefix = read_array_process(resource.nOffset, 8, pPdStruct);
        if (!guardedThis || !guardedSource || (baPrefix.size() != 8)) return false;
        if (!spisHeaderPrefix(baPrefix)) continue;

        const QString sHint = QStringLiteral("%1_%2_%3")
                                  .arg(resourceToken(resource.irin[0].bIsName, resource.irin[0].nID, resource.irin[0].sName),
                                       resourceToken(resource.irin[1].bIsName, resource.irin[1].nID, resource.irin[1].sName),
                                       resourceToken(resource.irin[2].bIsName, resource.irin[2].nID, resource.irin[2].sName));
        appendBlob(&resourceInfo, resource.nOffset, resource.nSize, sHint, pPdStruct);
    }
    if (!guardedThis || !guardedSource || resourceInfo.listBlobs.isEmpty() || resourceInfo.listLocations.isEmpty()) return false;
    resourceInfo.bIsValid = true;
    *pInfo = resourceInfo;
    return XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XSpisSFX::isValid(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO info;
    return discover(&info, pPdStruct);
}

bool XSpisSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSpisSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}

bool XSpisSFX::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSpisSFX> guardedThis(this);
    if (!isInternalInfoHandled()) {
        INTERNAL_INFO info;
        if (!discover(&info, pPdStruct) || !guardedThis) return false;
        if (!XBinary::handleInternalInfo(pPdStruct) || !guardedThis) return false;
        XBinary::INTERNAL_INFO *pBase = static_cast<XBinary::INTERNAL_INFO *>(XBinary::getInternalInfo(pPdStruct));
        if (!guardedThis || !pBase) return false;
        static_cast<XBinary::INTERNAL_INFO &>(info) = *pBase;
        m_internalInfo = info;
    }
    return true;
}

void *XSpisSFX::getInternalInfo(PDSTRUCT *pPdStruct)
{
    return handleInternalInfo(pPdStruct) ? &m_internalInfo : nullptr;
}

void XSpisSFX::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XBinary::setInternalInfo(static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XBinary::setInternalInfo(nullptr);
    }
}

XBinary::FT XSpisSFX::getFileType()
{
    return FT_SPISSFX;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSpisSFX::getDefaultUnpackProperties()
{
    XSPIS archive;
    return archive.getDefaultUnpackProperties();
}

bool XSpisSFX::releaseBlob(UNPACK_CONTEXT *pContext)
{
    if (!pContext) return false;
    bool bResult = true;
    if (pContext->pArchive) {
        bResult = pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pArchive = nullptr;
    }
    if (pContext->pSubDevice) {
        pContext->pSubDevice->close();
        delete pContext->pSubDevice;
        pContext->pSubDevice = nullptr;
    }
    pContext->innerState = UNPACK_STATE();
    pContext->nBoundBlob = -1;
    return bResult;
}

bool XSpisSFX::bindLocation(UNPACK_CONTEXT *pContext, qint32 nGlobalIndex, PDSTRUCT *pPdStruct)
{
    if (!pContext || (nGlobalIndex < 0) || (nGlobalIndex >= pContext->info.listLocations.count()) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const LOCATION location = pContext->info.listLocations.at(nGlobalIndex);
    if ((location.nBlob < 0) || (location.nBlob >= pContext->info.listBlobs.count())) return false;

    if (pContext->pArchive && (pContext->nBoundBlob == location.nBlob)) {
        if (pContext->innerState.nCurrentIndex == location.nRecord) return true;
        if ((pContext->innerState.nCurrentIndex + 1 == location.nRecord) && pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct)) return true;
    }
    if (!releaseBlob(pContext)) return false;

    const BLOB blob = pContext->info.listBlobs.at(location.nBlob);
    SubDevice *pSubDevice = new (std::nothrow) SubDevice(pContext->pOuterSourceDevice.data(), blob.nOffset, blob.nSize);
    if (!pSubDevice) return false;
    pSubDevice->setProperty("FileName", blob.sHint);
    if (!pSubDevice->open(QIODevice::ReadOnly)) {
        delete pSubDevice;
        return false;
    }
    XSPIS *pArchive = new (std::nothrow) XSPIS(pSubDevice);
    if (!pArchive) {
        pSubDevice->close();
        delete pSubDevice;
        return false;
    }

    pContext->pSubDevice = pSubDevice;
    pContext->pArchive = pArchive;
    pContext->nBoundBlob = location.nBlob;
    pContext->innerState = UNPACK_STATE();
    if (!pArchive->initUnpack(&pContext->innerState, pContext->mapUnpackProperties, pPdStruct) ||
        (pContext->innerState.nNumberOfRecords != blob.nRecords)) {
        releaseBlob(pContext);
        return false;
    }
    while (pContext->innerState.nCurrentIndex < location.nRecord) {
        if (!pArchive->moveToNext(&pContext->innerState, pPdStruct)) {
            releaseBlob(pContext);
            return false;
        }
    }
    return pContext->innerState.nCurrentIndex == location.nRecord;
}

bool XSpisSFX::isOwnContext(const UNPACK_STATE *pState) const
{
    return pState && pState->pContext && m_setUnpackContexts.contains(static_cast<UNPACK_CONTEXT *>(pState->pContext));
}

bool XSpisSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSpisSFX> guardedThis(this);

    if (pState->pContext) {
        if (!isOwnContext(pState)) return false;
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (pOldContext->pOwnerState != pState) return false;
        m_setUnpackContexts.remove(pOldContext);
        pState->pContext = nullptr;
        const bool bReleased = releaseBlob(pOldContext);
        delete pOldContext;
        *pState = UNPACK_STATE();
        if (!guardedThis || !bReleased) return false;
    }

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(mapProperties, &nOutputLimit)) return false;
    Q_UNUSED(nOutputLimit)
    INTERNAL_INFO info;
    if (!discover(&info, pPdStruct) || !guardedThis) return false;
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) return false;
    pContext->info = info;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pOwnerState = pState;
    pContext->mapUnpackProperties = mapProperties;
    if (!bindLocation(pContext, 0, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseBlob(pContext);
        delete pContext;
        return false;
    }

    *pState = UNPACK_STATE();
    pState->pContext = pContext;
    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = info.listLocations.count();
    pState->nCurrentOffset = info.listBlobs.at(0).nOffset + pContext->innerState.nCurrentOffset;
    pState->nTotalSize = info.nFileSize;
    m_setUnpackContexts.insert(pContext);
    return true;
}

XBinary::ARCHIVERECORD XSpisSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState || !isOwnContext(pState)) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSpisSFX> guardedThis(this);

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pContext->info.listLocations.count()) || !pContext->pArchive ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return result;
    }

    const LOCATION location = pContext->info.listLocations.at(pState->nCurrentIndex);
    if ((pContext->nBoundBlob != location.nBlob) || (pContext->innerState.nCurrentIndex != location.nRecord)) return result;
    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext) || result.mapProperties.isEmpty() ||
        !XBinary::isArchiveRecordExtentValid(result)) {
        return ARCHIVERECORD();
    }
    const BLOB blob = pContext->info.listBlobs.at(location.nBlob);
    result.mapProperties.insert(FPART_PROP_INFO, QStringLiteral("SPIS blob at 0x%1").arg(blob.nOffset, 0, 16));
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XSpisSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState || !isOwnContext(pState)) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSpisSFX> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedOutput || !guardedSource || XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->pOuterSourceDevice != guardedSource) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->info.listLocations.count()) || !pContext->pArchive) {
        return false;
    }
    const LOCATION location = pContext->info.listLocations.at(pState->nCurrentIndex);
    if ((pContext->nBoundBlob != location.nBlob) || (pContext->innerState.nCurrentIndex != location.nRecord)) return false;
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    const bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput.data(), pPdStruct);
    if (!guardedThis || !guardedOutput || !guardedSource || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    pState->nCurrentOffset = pContext->info.listBlobs.at(location.nBlob).nOffset + pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XSpisSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState || !isOwnContext(pState)) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSpisSFX> guardedThis(this);
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!pContext || !m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        releaseBlob(pContext);
        pState->nCurrentOffset = pState->nTotalSize;
        return false;
    }
    if (!bindLocation(pContext, pState->nCurrentIndex, pPdStruct) || !guardedThis) return false;
    const LOCATION location = pContext->info.listLocations.at(pState->nCurrentIndex);
    pState->nCurrentOffset = pContext->info.listBlobs.at(location.nBlob).nOffset + pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return true;
}

bool XSpisSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !isOwnContext(pState)) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSpisSFX> guardedThis(this);

    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
    m_setUnpackContexts.remove(pContext);
    pState->pContext = nullptr;
    const bool bResult = releaseBlob(pContext);
    delete pContext;
    if (!guardedThis) return false;
    *pState = UNPACK_STATE();
    return bResult;
}
