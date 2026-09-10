/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsevenzipsfx.h"

#include <QScopedValueRollback>

#include "xpe.h"
#include "subdevice.h"
#include "xsevenzip.h"

namespace {

class SEVENZIPSFX_OPERATION_STATE_DELETER {
public:
    explicit SEVENZIPSFX_OPERATION_STATE_DELETER(const QSharedPointer<XSevenZipSFX::UNPACK_DEFERRED_CLEANUP> &pCleanup) : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XSevenZipSFX::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

bool getValidatedSevenZipSize(QIODevice *pDevice, qint64 nOffset, qint64 nAvailableSize, qint64 *pArchiveSize, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pArchiveSize || (nOffset < 0) || (nAvailableSize < 32) || (nOffset > pDevice->size()) || (nAvailableSize > pDevice->size() - nOffset) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    qint64 nLogicalSize = 0;
    {
        SubDevice candidateDevice(pDevice, nOffset, nAvailableSize);
        if (!candidateDevice.open(QIODevice::ReadOnly)) return false;

        XSevenZip candidate(&candidateDevice);
        if (candidate.isValid(pPdStruct)) nLogicalSize = candidate.getFileFormatSize(pPdStruct);
        candidateDevice.close();
    }

    if ((nLogicalSize < 32) || (nLogicalSize > nAvailableSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    // isValid() authenticates the framing CRCs. Also parse the complete header
    // so a CRC-consistent but semantically malformed header is not accepted.
    // An encrypted encoded header cannot be initialized without its password;
    // isEncrypted() still parses and validates its stream description.
    SubDevice archiveDevice(pDevice, nOffset, nLogicalSize);
    if (!archiveDevice.open(QIODevice::ReadOnly)) return false;

    XSevenZip archive(&archiveDevice);
    XBinary::UNPACK_STATE state = {};
    QMap<XBinary::UNPACK_PROP, QVariant> properties;
    bool bValid = archive.initUnpack(&state, properties, pPdStruct);
    if (bValid) {
        bValid = archive.finishUnpack(&state, pPdStruct);
    } else if (XBinary::isPdStructNotCanceled(pPdStruct)) {
        bValid = archive.isEncrypted();
    }
    archiveDevice.close();

    if (!bValid || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pArchiveSize = nLogicalSize;
    return true;
}

}  // namespace

XSevenZipSFX::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
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

XSevenZipSFX::XSevenZipSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackDeferredCleanup = QSharedPointer<UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pUnpackDeferredCleanup;
    m_pUnpackOperationState = QSharedPointer<bool>(new bool(false), SEVENZIPSFX_OPERATION_STATE_DELETER(pDeferredCleanup));
    m_internalInfo = INTERNAL_INFO();
    setIsArchive(true);
}

XSevenZipSFX::~XSevenZipSFX()
{
    if (m_pUnpackOperationState) *m_pUnpackOperationState = true;
    if (m_pUnpackDeferredCleanup) {
        m_pUnpackDeferredCleanup->setContexts.unite(m_setUnpackContexts);
        m_setUnpackContexts.clear();
    }
    m_pUnpackDeferredCleanup.clear();
    m_pUnpackOperationState.clear();
}

bool XSevenZipSFX::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XSevenZipSFX> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XSevenZipSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSevenZipSFX x(pDevice);
    return x.isValid(pPdStruct);
}

XSevenZipSFX::INTERNAL_INFO XSevenZipSFX::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XSevenZipSFX::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSevenZipSFX> guardedThis(this);
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

void *XSevenZipSFX::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSevenZipSFX> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XSevenZipSFX::setInternalInfo(void *pInternalInfo)
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

XBinary::FT XSevenZipSFX::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_7ZSFX;
    }

    return FT_PE32_7ZSFX;
}

XSevenZipSFX::INTERNAL_INFO XSevenZipSFX::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nArchiveOffset = -1;

    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;

    const qint64 nTotalSize = getSize();
    qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if ((nOverlayOffset <= 0) || (nOverlayOffset >= nTotalSize)) return result;

    QByteArray baHead = read_array_process(nOverlayOffset, 20, pPdStruct);
    if (baHead.size() < 6) return result;
    const quint8 *p = (const quint8 *)baHead.constData();

    static const QByteArray k7z = QByteArray::fromHex("377ABCAF271C");
    const char *kConfig = ";!@Install@!UTF-8!";
    const int nConfigLen = 18;

    bool bConfig = (baHead.size() >= nConfigLen) && (memcmp(baHead.constData(), kConfig, nConfigLen) == 0);
    bool bBomConfig = false;
    if ((baHead.size() >= 3) && ((quint8)p[0] == 0xEF) && ((quint8)p[1] == 0xBB) && ((quint8)p[2] == 0xBF)) {
        QByteArray baAfter = read_array_process(nOverlayOffset + 3, nConfigLen, pPdStruct);
        bBomConfig = (baAfter.size() == nConfigLen) && (memcmp(baAfter.constData(), kConfig, nConfigLen) == 0);
    }
    bool bPlain7z = baHead.startsWith(k7z);

    if (bConfig || bBomConfig) {
        // Config SFX: the 7z archive follows the ";!@InstallEnd@!" text block.
        static const char kConfigEnd[] = ";!@InstallEnd@!";
        const qint64 nConfigLimit = qMin<qint64>(nTotalSize - nOverlayOffset, 1 << 20);
        qint64 nEndOffset = find_array(nOverlayOffset, nConfigLimit, kConfigEnd, sizeof(kConfigEnd) - 1, pPdStruct);
        if (nEndOffset >= 0) {
            qint64 nArchiveOffset = nEndOffset + (qint64)sizeof(kConfigEnd) - 1;
            qint32 nPaddingSize = 0;
            while ((nArchiveOffset < nTotalSize) && (nPaddingSize < 4096) && XBinary::isPdStructNotCanceled(pPdStruct)) {
                quint8 nByte = read_uint8(nArchiveOffset);
                if ((nByte != ' ') && (nByte != '\t') && (nByte != '\r') && (nByte != '\n')) break;
                nArchiveOffset++;
                nPaddingSize++;
            }

            qint64 nArchiveSize = 0;
            if ((nArchiveOffset <= nTotalSize - 6) && (read_array_process(nArchiveOffset, 6, pPdStruct) == k7z) &&
                getValidatedSevenZipSize(getDevice(), nArchiveOffset, nTotalSize - nArchiveOffset, &nArchiveSize, pPdStruct)) {
                result.bIsValid = true;
                result.nArchiveOffset = nArchiveOffset;
                result.nArchiveSize = nArchiveSize;
            }
        }
    } else if (bPlain7z) {
        // Plain-magic branch: require a 7-Zip stub attribution to avoid firing on
        // any PE that merely appends a .7z file.
        QString sInternal = pe.getResourcesVersionValue("InternalName").trimmed().toLower();
        QString sProduct = pe.getResourcesVersionValue("ProductName").trimmed();
        bool bHarden = (sInternal.startsWith("7z") && sInternal.endsWith(".sfx")) || (sProduct == "7-Zip");
        if (bHarden) {
            qint64 nArchiveSize = 0;
            if (getValidatedSevenZipSize(getDevice(), nOverlayOffset, nTotalSize - nOverlayOffset, &nArchiveSize, pPdStruct)) {
                result.bIsValid = true;
                result.nArchiveOffset = nOverlayOffset;
                result.nArchiveSize = nArchiveSize;
            }
        }
    }

    if (!result.bIsValid || !XBinary::isPdStructNotCanceled(pPdStruct)) return INTERNAL_INFO();

    QString sVer = pe.getResourcesVersionValue("FileVersion").trimmed();
    if (sVer.isEmpty()) sVer = pe.getFileVersion().trimmed();
    result.sVersion = sVer;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) result.bIsValid = false;

    return result;
}

// --- streaming extraction: delegate to the XArchive 7z handler ---

QMap<XBinary::UNPACK_PROP, QVariant> XSevenZipSFX::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();

    QIODevice *pDevice = getDevice();

    if (pDevice) {
        INTERNAL_INFO info = _detect(nullptr);

        if (info.bIsValid && (info.nArchiveOffset >= 0) && (info.nArchiveSize > 0)) {
            SubDevice subDevice(pDevice, info.nArchiveOffset, info.nArchiveSize);

            if (subDevice.open(QIODevice::ReadOnly)) {
                {
                    XSevenZip archive(&subDevice);
                    QMap<UNPACK_PROP, QVariant> mapInnerProperties = archive.getDefaultUnpackProperties();

                    if (mapInnerProperties.contains(UNPACK_PROP_PASSWORD)) {
                        result.insert(UNPACK_PROP_PASSWORD, mapInnerProperties.value(UNPACK_PROP_PASSWORD));
                    }

                    for (QMap<UNPACK_PROP, QVariant>::const_iterator it = mapInnerProperties.constBegin(); it != mapInnerProperties.constEnd(); ++it) {
                        if (XBinary::isUnpackCRCProperty(it.key())) {
                            result.insert(it.key(), it.value());
                        }
                    }
                }

                subDevice.close();
            }
        }
    }

    return result;
}

bool XSevenZipSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSevenZipSFX> guardedThis(this);
    if (!pState->baUnpackSourceToken.isEmpty()) return false;

    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        m_setUnpackContexts.remove(pOldContext);
        pState->pContext = nullptr;
        bool bFinishOK = true;
        if (pOldContext->pArchive) {
            bFinishOK = pOldContext->pArchive->finishUnpack(&pOldContext->innerState, nullptr);
            delete pOldContext->pArchive;
        }
        if (pOldContext->pSubDevice) {
            pOldContext->pSubDevice->close();
            delete pOldContext->pSubDevice;
        }
        delete pOldContext;
        *pState = UNPACK_STATE();
        if (!guardedThis || !bFinishOK) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;

    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    XSevenZipSFX detector(guardedSource.data(), isImage(), getModuleAddress());
    INTERNAL_INFO info = detector._detect(pPdStruct);
    if (!guardedThis || !guardedSource || !info.bIsValid || (info.nArchiveOffset < 0) || (info.nArchiveSize <= 0)) return false;
    const qint64 nTotalSize = guardedSource->size();
    if (!guardedThis || !guardedSource || (nTotalSize < 0)) return false;

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pSubDevice = new SubDevice(guardedSource.data(), info.nArchiveOffset, info.nArchiveSize);
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();

    if (!pContext->pSubDevice->open(QIODevice::ReadOnly)) {
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pContext->pArchive = new XSevenZip(pContext->pSubDevice);
    if (!pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct) || !guardedThis || !guardedSource) {
        pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pSubDevice->close();
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->nTotalSize = nTotalSize;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    m_setUnpackContexts.insert(pContext);
    return true;
}

XBinary::ARCHIVERECORD XSevenZipSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSevenZipSFX> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return result;
    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return ARCHIVERECORD();
    return result;
}

bool XSevenZipSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSevenZipSFX> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || !guardedThis || !guardedOutput || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput.data(), pPdStruct);
    if (!guardedThis || !guardedOutput || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XSevenZipSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSevenZipSFX> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    bool bResult = pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XSevenZipSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState) return false;
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSevenZipSFX> guardedThis(this);
    bool bResult = true;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        m_setUnpackContexts.remove(pContext);
        pState->pContext = nullptr;
        if (pContext->pArchive) {
            bResult = pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
            delete pContext->pArchive;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
        }
        delete pContext;
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
