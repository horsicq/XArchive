/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xsevenzipsfx.h"

#include <QScopedValueRollback>

#include "xpe.h"
#include "subdevice.h"
#include "xarchives.h"
#include "xsevenzip.h"

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
    m_internalInfo = INTERNAL_INFO();
    setIsArchive(true);
}

XSevenZipSFX::~XSevenZipSFX()
{
}

bool XSevenZipSFX::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(getInternalInfo(pPdStruct));
    return pInfo && pInfo->bIsValid;
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

void *XSevenZipSFX::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
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
                XArchives::getValidatedArchiveSize(getDevice(), XBinary::FT_7Z, nArchiveOffset, nTotalSize - nArchiveOffset, &nArchiveSize, pPdStruct)) {
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
            result.bCustom = true;
        }

        qint64 nArchiveSize = 0;
        if (XArchives::getValidatedArchiveSize(getDevice(), XBinary::FT_7Z, nOverlayOffset, nTotalSize - nOverlayOffset, &nArchiveSize, pPdStruct)) {
            result.bIsValid = true;
            result.nArchiveOffset = nOverlayOffset;
            result.nArchiveSize = nArchiveSize;
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

    // QIODevice *pDevice = getDevice();

    // if (pDevice) {
    //     INTERNAL_INFO info = _detect(nullptr);

    //     if (info.bIsValid && (info.nArchiveOffset >= 0) && (info.nArchiveSize > 0)) {
    //         SubDevice subDevice(pDevice, info.nArchiveOffset, info.nArchiveSize);

    //         if (subDevice.open(QIODevice::ReadOnly)) {
    //             {
    //                 XSevenZip archive(&subDevice);
    //                 QMap<UNPACK_PROP, QVariant> mapInnerProperties = archive.getDefaultUnpackProperties();

    //                 if (mapInnerProperties.contains(UNPACK_PROP_PASSWORD)) {
    //                     result.insert(UNPACK_PROP_PASSWORD, mapInnerProperties.value(UNPACK_PROP_PASSWORD));
    //                 }

    //                 for (QMap<UNPACK_PROP, QVariant>::const_iterator it = mapInnerProperties.constBegin(); it != mapInnerProperties.constEnd(); ++it) {
    //                     if (XBinary::isUnpackCRCProperty(it.key())) {
    //                         result.insert(it.key(), it.value());
    //                     }
    //                 }
    //             }

    //             subDevice.close();
    //         }
    //     }
    // }

    return result;
}

bool XSevenZipSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    if (!pState->baUnpackSourceToken.isEmpty()) return false;

    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (pOldContext->pOwnerState != pState) return false;
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
        if (!bFinishOK) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    XSevenZipSFX detector(guardedSource, isImage(), getModuleAddress());
    INTERNAL_INFO info = detector._detect(pPdStruct);
    if (!guardedSource || !info.bIsValid || (info.nArchiveOffset < 0) || (info.nArchiveSize <= 0)) return false;
    const qint64 nTotalSize = guardedSource->size();
    if (!guardedSource || (nTotalSize < 0)) return false;

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pSubDevice = new SubDevice(guardedSource, info.nArchiveOffset, info.nArchiveSize);
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();

    if (!pContext->pSubDevice->open(QIODevice::ReadOnly)) {
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pContext->pArchive = new XSevenZip(pContext->pSubDevice);
    if (!pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct) || !guardedSource) {
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
    return true;
}

XBinary::ARCHIVERECORD XSevenZipSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return result;
    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return ARCHIVERECORD();
    return result;
}

bool XSevenZipSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedOutput = pDevice;
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || !guardedOutput || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput, pPdStruct);
    if (!guardedOutput || (pState->pContext != pContext)) return false;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XSevenZipSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive || (pContext->innerState.nCurrentIndex != pState->nCurrentIndex) ||
        (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) || (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    bool bResult = pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return false;
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
    bool bResult = true;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (pContext->pOwnerState != pState) return false;
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
    }
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    return bResult;
}
