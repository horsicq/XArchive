/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xiexpress.h"

#include <QScopedValueRollback>

#include "xpe.h"
#include "subdevice.h"
#include "xcab.h"

// RT_RCDATA resource type id.
#define IEXPRESS_RT_RCDATA 10

namespace {
class IEXPRESS_OPERATION_STATE_DELETER {
public:
    explicit IEXPRESS_OPERATION_STATE_DELETER(const QSharedPointer<XIExpress::UNPACK_DEFERRED_CLEANUP> &pCleanup) : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XIExpress::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};
}  // namespace

XIExpress::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
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

XIExpress::XIExpress(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackDeferredCleanup = QSharedPointer<UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pUnpackDeferredCleanup;
    m_pUnpackOperationState = QSharedPointer<bool>(new bool(false), IEXPRESS_OPERATION_STATE_DELETER(pDeferredCleanup));
    m_internalInfo = INTERNAL_INFO();
    setIsArchive(true);
}

XIExpress::~XIExpress()
{
    if (m_pUnpackOperationState) *m_pUnpackOperationState = true;
    if (m_pUnpackDeferredCleanup) {
        m_pUnpackDeferredCleanup->setContexts.unite(m_setUnpackContexts);
        m_setUnpackContexts.clear();
    }
    m_pUnpackDeferredCleanup.clear();
    m_pUnpackOperationState.clear();
}

bool XIExpress::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XIExpress> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XIExpress::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIExpress x(pDevice);
    return x.isValid(pPdStruct);
}

XIExpress::INTERNAL_INFO XIExpress::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XIExpress::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XIExpress> guardedThis(this);
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

void *XIExpress::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XIExpress> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;

    return &guardedThis->m_internalInfo;
}

void XIExpress::setInternalInfo(void *pInternalInfo)
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

XBinary::FT XIExpress::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_IEXPRESS;
    }

    return FT_PE32_IEXPRESS;
}

static bool iexFindStubString(XIExpress *pThis, qint64 nCabinetOffset, qint64 nCabinetEnd, qint64 nTotalSize, XBinary::PDSTRUCT *pPdStruct, const QString &sString,
                              bool bCaseInsensitive)
{
    qint64 nFound = bCaseInsensitive ? pThis->find_ansiStringI(0, nCabinetOffset, sString, pPdStruct) : pThis->find_ansiString(0, nCabinetOffset, sString, pPdStruct);
    if (nFound != -1) return true;
    nFound = bCaseInsensitive ? pThis->find_ansiStringI(nCabinetEnd, nTotalSize - nCabinetEnd, sString, pPdStruct)
                              : pThis->find_ansiString(nCabinetEnd, nTotalSize - nCabinetEnd, sString, pPdStruct);
    return nFound != -1;
}

XIExpress::INTERNAL_INFO XIExpress::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nArchiveOffset = -1;

    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;

    QList<XPE::RESOURCE_RECORD> listResources = pe.getResources(10000, pPdStruct);
    if ((listResources.size() >= 10000) || !XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    // The MSCF cabinet lives in RT_RCDATA "CABINET".
    XPE::RESOURCE_RECORD rrCab = {};
    rrCab.nOffset = -1;
    for (const XPE::RESOURCE_RECORD &record : listResources) {
        if (record.irin[0].bIsName || !record.irin[1].bIsName || (record.irin[0].nID != IEXPRESS_RT_RCDATA) || (record.irin[1].sName != QStringLiteral("CABINET")))
            continue;
        if (rrCab.nOffset < 0) {
            rrCab = record;
        } else if ((rrCab.nOffset != record.nOffset) || (rrCab.nSize != record.nSize)) {
            // Different language entries must not select ambiguous payloads.
            return result;
        }
    }
    qint64 nTotalSize = getSize();
    if ((rrCab.nOffset <= 0) || (rrCab.nSize < (qint64)sizeof(XCab::CFHEADER)) || (rrCab.nOffset > nTotalSize) || (rrCab.nSize > nTotalSize - rrCab.nOffset)) {
        return result;
    }

    QByteArray baMagic = read_array_process(rrCab.nOffset, 4, pPdStruct);
    if (baMagic != QByteArray("MSCF", 4)) return result;

    // Harden: a Wextract-only sibling resource / marker (kills generic cab-in-resource).
    const qint64 nCabinetEnd = rrCab.nOffset + rrCab.nSize;
    bool bRunResource = false;
    for (const XPE::RESOURCE_RECORD &record : listResources) {
        if (!record.irin[0].bIsName && record.irin[1].bIsName && (record.irin[0].nID == IEXPRESS_RT_RCDATA) && (record.irin[1].sName == QStringLiteral("RUNPROGRAM")) &&
            (record.nOffset > 0) && (record.nSize > 0) && (record.nSize <= (1 << 20)) && (record.nOffset <= nTotalSize) &&
            (record.nSize <= nTotalSize - record.nOffset) && (((record.nOffset + record.nSize) <= rrCab.nOffset) || (record.nOffset >= nCabinetEnd))) {
            bRunResource = true;
            break;
        }
    }
    bool bWextract = bRunResource || iexFindStubString(this, rrCab.nOffset, nCabinetEnd, nTotalSize, pPdStruct, QStringLiteral("wextract"), true) ||
                     iexFindStubString(this, rrCab.nOffset, nCabinetEnd, nTotalSize, pPdStruct, QStringLiteral("IExpress extraction tool"), false);
    if (!bWextract) return result;

    // Authenticate the complete cabinet table during detection.  A resource
    // named CABINET with only an MSCF prefix is not a usable IExpress payload.
    SubDevice cabinetDevice(getDevice(), rrCab.nOffset, rrCab.nSize);
    if (!cabinetDevice.open(QIODevice::ReadOnly)) return result;

    XCab cabinet(&cabinetDevice);
    UNPACK_STATE state = {};
    QMap<UNPACK_PROP, QVariant> mapProperties;
    bool bCabinetValid = cabinet.initUnpack(&state, mapProperties, pPdStruct);
    qint64 nLogicalCabinetSize = 0;
    if (bCabinetValid) {
        nLogicalCabinetSize = cabinet.getFileFormatSize(pPdStruct);
        bCabinetValid = cabinet.finishUnpack(&state, pPdStruct);
    }
    cabinetDevice.close();

    if (!bCabinetValid || (nLogicalCabinetSize < (qint64)sizeof(XCab::CFHEADER)) || (nLogicalCabinetSize > rrCab.nSize) || !XBinary::isPdStructNotCanceled(pPdStruct))
        return result;

    result.bIsValid = true;
    result.nArchiveOffset = rrCab.nOffset;
    result.nArchiveSize = nLogicalCabinetSize;

    QString sVer = pe.getResourcesVersionValue("FileVersion").trimmed();
    if (sVer.isEmpty()) sVer = pe.getFileVersion().trimmed();
    result.sVersion = sVer;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) result.bIsValid = false;

    return result;
}

// --- streaming extraction: delegate to the XArchive CAB handler ---

QMap<XBinary::UNPACK_PROP, QVariant> XIExpress::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();

    return result;
}

bool XIExpress::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XIExpress> guardedThis(this);
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
    XIExpress detector(guardedSource.data(), isImage(), getModuleAddress());
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

    pContext->pArchive = new XCab(pContext->pSubDevice);
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

XBinary::ARCHIVERECORD XIExpress::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XIExpress> guardedThis(this);
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

bool XIExpress::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XIExpress> guardedThis(this);
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

bool XIExpress::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XIExpress> guardedThis(this);
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

bool XIExpress::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState) return false;
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XIExpress> guardedThis(this);
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
