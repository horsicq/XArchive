/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xarjsfx.h"

#include <QScopedValueRollback>

#include <limits>
#include <new>

#include "subdevice.h"
#include "xarj.h"

namespace {

const qint32 ARJSFX_MAX_ARCHIVES = 256;

class ARJSFX_OPERATION_STATE_DELETER {
public:
    explicit ARJSFX_OPERATION_STATE_DELETER(
        const QSharedPointer<XArjSFX::ARJSFX_UNPACK_DEFERRED_CLEANUP> &pCleanup)
        : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XArjSFX::ARJSFX_UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

}  // namespace

XArjSFX::ARJSFX_UNPACK_DEFERRED_CLEANUP::~ARJSFX_UNPACK_DEFERRED_CLEANUP()
{
    const QSet<ARJSFX_UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (ARJSFX_UNPACK_CONTEXT *pContext : contexts) {
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

XArjSFX::XArjSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_ARJ)
{
    m_pArjUnpackDeferredCleanup = QSharedPointer<ARJSFX_UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<ARJSFX_UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pArjUnpackDeferredCleanup;
    m_pArjUnpackOperationState = QSharedPointer<bool>(
        new bool(false), ARJSFX_OPERATION_STATE_DELETER(pDeferredCleanup));
}

XArjSFX::~XArjSFX()
{
    if (m_pArjUnpackOperationState) *m_pArjUnpackOperationState = true;
    if (m_pArjUnpackDeferredCleanup) {
        m_pArjUnpackDeferredCleanup->setContexts.unite(m_setArjUnpackContexts);
        m_setArjUnpackContexts.clear();
    }
    m_pArjUnpackDeferredCleanup.clear();
    m_pArjUnpackOperationState.clear();
}

bool XArjSFX::isValid(PDSTRUCT *pPdStruct)
{
    return XSFX::isValid(pPdStruct);
}

bool XArjSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XArjSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}

QMap<XBinary::UNPACK_PROP, QVariant> XArjSFX::getDefaultUnpackProperties()
{
    return XSFX::getDefaultUnpackProperties();
}

bool XArjSFX::_scanArchives(QList<ARJSFX_ENTRY> *pList, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pList || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    pList->clear();
    QPointer<XArjSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource || guardedSource->isSequential()) return false;

    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(guardedThis->XSFX::getInternalInfo(pPdStruct));
    if (!guardedThis || !guardedSource || !pInfo || !pInfo->bIsValid || (pInfo->arcType != FT_ARJ) || pInfo->bUseOuterDevice ||
        (pInfo->nArchiveOffset < 0) || (pInfo->nArchiveSize <= 0))
        return false;

    const qint64 nDeviceSize = guardedSource->size();
    if (!guardedThis || !guardedSource || (nDeviceSize < 0) || !checkOffsetSize(pInfo->nArchiveOffset, pInfo->nArchiveSize)) return false;
    // A fixed horizon would recreate issue 05 for a valid archive whose next
    // payload starts beyond that horizon: the first payload would be reported
    // as a complete result. The scan is still bounded by the finite device,
    // the shared probe deadline/cancellation state and the candidate cap.
    const qint64 nScanEnd = nDeviceSize;
    qint64 nCandidateOffset = pInfo->nArchiveOffset;

    qint32 nAttempt = 0;
    for (; (nAttempt < ARJSFX_MAX_ARCHIVES) && (nCandidateOffset >= 0) && (nCandidateOffset < nScanEnd) &&
           XBinary::isPdStructNotCanceled(pPdStruct);
         ++nAttempt) {
        const qint64 nAvailable = nDeviceSize - nCandidateOffset;
        SubDevice candidateDevice(guardedSource.data(), nCandidateOffset, nAvailable);
        qint64 nArchiveSize = 0;
        if (candidateDevice.open(QIODevice::ReadOnly)) {
            XARJ candidate(&candidateDevice);
            if (candidate.isValid(pPdStruct)) nArchiveSize = candidate.getFileFormatSize(pPdStruct);
            candidateDevice.close();
        }
        if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

        if ((nArchiveSize > 0) && (nArchiveSize <= nAvailable)) {
            SubDevice exactDevice(guardedSource.data(), nCandidateOffset, nArchiveSize);
            UNPACK_STATE state = {};
            qint32 nRecords = 0;
            bool bInitialized = exactDevice.open(QIODevice::ReadOnly);
            if (bInitialized) {
                XARJ archive(&exactDevice);
                bInitialized = archive.initUnpack(&state, mapProperties, pPdStruct);
                if (bInitialized) nRecords = state.nNumberOfRecords;
                const bool bFinished = archive.finishUnpack(&state, nullptr);
                bInitialized = bInitialized && bFinished;
                exactDevice.close();
            }
            if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            if (bInitialized && (nRecords > 0)) {
                qint64 nAccumulatedRecords = nRecords;
                for (const ARJSFX_ENTRY &existing : *pList) nAccumulatedRecords += existing.nNumberOfRecords;
                if (nAccumulatedRecords > (std::numeric_limits<qint32>::max)()) return false;
                ARJSFX_ENTRY entry = {};
                entry.nArchiveOffset = nCandidateOffset;
                entry.nArchiveSize = nArchiveSize;
                entry.nNumberOfRecords = nRecords;
                pList->append(entry);
                nCandidateOffset += nArchiveSize;
            } else {
                ++nCandidateOffset;
            }
        } else {
            ++nCandidateOffset;
        }

        if (nCandidateOffset >= nScanEnd) break;
        nCandidateOffset = guardedThis->find_signature(nCandidateOffset, nScanEnd - nCandidateOffset, "60EA", nullptr, pPdStruct);
    }

    // Candidate-density exhaustion is not completeness. Refuse the archive
    // instead of publishing the first 256 payloads as if they were all of it.
    if ((nAttempt >= ARJSFX_MAX_ARCHIVES) && (nCandidateOffset >= 0) && (nCandidateOffset < nScanEnd)) return false;
    return guardedThis && guardedSource && !pList->isEmpty() && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XArjSFX::_releaseEntry(ARJSFX_UNPACK_CONTEXT *pContext)
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
    return bResult;
}

bool XArjSFX::_bindEntry(ARJSFX_UNPACK_CONTEXT *pContext, qint32 nEntryIndex, PDSTRUCT *pPdStruct)
{
    QPointer<XArjSFX> guardedThis(this);
    if (!pContext || (nEntryIndex < 0) || (nEntryIndex >= pContext->listEntries.count()) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || !_releaseEntry(pContext))
        return false;

    const ARJSFX_ENTRY &entry = pContext->listEntries.at(nEntryIndex);
    pContext->pSubDevice = new (std::nothrow) SubDevice(pContext->pOuterSourceDevice.data(), entry.nArchiveOffset, entry.nArchiveSize);
    if (!pContext->pSubDevice || !pContext->pSubDevice->open(QIODevice::ReadOnly)) {
        delete pContext->pSubDevice;
        pContext->pSubDevice = nullptr;
        return false;
    }
    pContext->pArchive = new (std::nothrow) XARJ(pContext->pSubDevice);
    const bool bInitialized = pContext->pArchive &&
                              pContext->pArchive->initUnpack(&pContext->innerState, pContext->mapUnpackProperties, pPdStruct);
    if (!guardedThis) return false;  // The deferred owner now cleans pContext.
    if (!bInitialized || (pContext->innerState.nNumberOfRecords != entry.nNumberOfRecords)) {
        guardedThis->_releaseEntry(pContext);
        return false;
    }
    pContext->nEntryIndex = nEntryIndex;
    return true;
}

bool XArjSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pArjUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XArjSFX> guardedThis(this);

    if (pState->pContext) {
        ARJSFX_UNPACK_CONTEXT *pOldContext = static_cast<ARJSFX_UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setArjUnpackContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        m_setArjUnpackContexts.remove(pOldContext);
        pState->pContext = nullptr;
        const bool bFinishOK = _releaseEntry(pOldContext);
        delete pOldContext;
        *pState = UNPACK_STATE();
        if (!guardedThis || !bFinishOK) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource || guardedSource->isSequential()) return false;
    QList<ARJSFX_ENTRY> listEntries;
    if (!guardedThis->_scanArchives(&listEntries, mapProperties, pPdStruct) || !guardedThis || !guardedSource || listEntries.isEmpty()) return false;

    qint32 nTotalRecords = 0;
    for (const ARJSFX_ENTRY &entry : listEntries) {
        if ((entry.nNumberOfRecords <= 0) || (entry.nNumberOfRecords > ((std::numeric_limits<qint32>::max)() - nTotalRecords))) return false;
        nTotalRecords += entry.nNumberOfRecords;
    }

    ARJSFX_UNPACK_CONTEXT *pContext = new (std::nothrow) ARJSFX_UNPACK_CONTEXT;
    if (!pContext) return false;
    pContext->listEntries = listEntries;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pOwnerState = pState;
    pContext->nEntryIndex = -1;
    pContext->nEntryFirstRecord = 0;
    pContext->pSubDevice = nullptr;
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();
    pContext->mapUnpackProperties = mapProperties;
    // Register before entering callback-bearing inner initialization. If a
    // progress callback deletes this object, the destructor transfers the
    // live context to the operation state's deferred cleanup owner.
    m_setArjUnpackContexts.insert(pContext);
    if (!guardedThis->_bindEntry(pContext, 0, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) {
            guardedThis->m_setArjUnpackContexts.remove(pContext);
            guardedThis->_releaseEntry(pContext);
            delete pContext;
        }
        return false;
    }

    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = nTotalRecords;
    pState->nCurrentOffset = listEntries.first().nArchiveOffset + pContext->innerState.nCurrentOffset;
    pState->nTotalSize = guardedSource->size();
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pState->pContext = pContext;
    return true;
}

XBinary::ARCHIVERECORD XArjSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pArjUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XArjSFX> guardedThis(this);
    if (!pState || !pState->pContext || !pState->baUnpackSourceToken.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;
    ARJSFX_UNPACK_CONTEXT *pContext = static_cast<ARJSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setArjUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive ||
        (pContext->nEntryIndex < 0) || (pContext->nEntryIndex >= pContext->listEntries.count()) ||
        (pState->nCurrentIndex != (pContext->nEntryFirstRecord + pContext->innerState.nCurrentIndex)))
        return result;
    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setArjUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return ARCHIVERECORD();
    const ARJSFX_ENTRY &entry = pContext->listEntries.at(pContext->nEntryIndex);
    if ((result.nStreamOffset < 0) || (entry.nArchiveOffset > ((std::numeric_limits<qint64>::max)() - result.nStreamOffset))) {
        return ARCHIVERECORD();
    }
    result.nStreamOffset += entry.nArchiveOffset;
    if (result.mapProperties.contains(FPART_PROP_STREAMOFFSET)) {
        result.mapProperties.insert(FPART_PROP_STREAMOFFSET, result.nStreamOffset);
    }
    return result;
}

bool XArjSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pArjUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XArjSFX> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->pContext || !pState->baUnpackSourceToken.isEmpty() || !guardedOutput || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ARJSFX_UNPACK_CONTEXT *pContext = static_cast<ARJSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setArjUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive ||
        (pState->nCurrentIndex != (pContext->nEntryFirstRecord + pContext->innerState.nCurrentIndex)))
        return false;
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    const bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput.data(), pPdStruct);
    if (!guardedThis || !guardedOutput || !m_setArjUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    const ARJSFX_ENTRY &entry = pContext->listEntries.at(pContext->nEntryIndex);
    pState->nCurrentOffset = entry.nArchiveOffset + pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XArjSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pArjUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XArjSFX> guardedThis(this);
    if (!pState || !pState->pContext || !pState->baUnpackSourceToken.isEmpty() || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ARJSFX_UNPACK_CONTEXT *pContext = static_cast<ARJSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setArjUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pArchive ||
        (pState->nCurrentIndex != (pContext->nEntryFirstRecord + pContext->innerState.nCurrentIndex)))
        return false;

    const bool bInnerNext = pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    ++pState->nCurrentIndex;
    if (bInnerNext) {
        if (pState->nCurrentIndex != (pContext->nEntryFirstRecord + pContext->innerState.nCurrentIndex)) return false;
        const ARJSFX_ENTRY &entry = pContext->listEntries.at(pContext->nEntryIndex);
        pState->nCurrentOffset = entry.nArchiveOffset + pContext->innerState.nCurrentOffset;
        return true;
    }

    const ARJSFX_ENTRY &completed = pContext->listEntries.at(pContext->nEntryIndex);
    if (pContext->innerState.nCurrentIndex != completed.nNumberOfRecords) return false;
    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        _releaseEntry(pContext);
        pState->nCurrentOffset = pState->nTotalSize;
        return false;
    }

    pContext->nEntryFirstRecord += completed.nNumberOfRecords;
    const qint32 nNextEntry = pContext->nEntryIndex + 1;
    if ((pState->nCurrentIndex != pContext->nEntryFirstRecord) || !guardedThis->_bindEntry(pContext, nNextEntry, pPdStruct) || !guardedThis) return false;
    const ARJSFX_ENTRY &next = pContext->listEntries.at(nNextEntry);
    pState->nCurrentOffset = next.nArchiveOffset + pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return true;
}

bool XArjSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pArjUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XArjSFX> guardedThis(this);
    bool bResult = true;
    if (pState->pContext) {
        ARJSFX_UNPACK_CONTEXT *pContext = static_cast<ARJSFX_UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setArjUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        m_setArjUnpackContexts.remove(pContext);
        pState->pContext = nullptr;
        bResult = _releaseEntry(pContext);
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
