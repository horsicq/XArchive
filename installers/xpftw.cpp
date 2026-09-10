/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xpftw.h"

#include <QScopedValueRollback>
#include <QtEndian>

#include "xpe.h"
#include "subdevice.h"
#include "xcab.h"

namespace {
const qint64 PFTW_MIN_HEADER_SIZE = 16;
const qint64 PFTW_MAX_HEADER_SIZE = 0x100000;

enum PFTW_VARIANT {
    PFTW_VARIANT_OVERLAY = 1,
    PFTW_VARIANT_CABINET_SECTION = 2
};

struct PFTW_REGION {
    qint64 nOffset;
    qint64 nSize;
    qint32 nVariant;
    QList<qint64> listHeaderStarts;
};

bool pftwRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}

QString pftwMetadataText(const uchar *pData, qint32 nSize)
{
    if (!pData || nSize <= 0) return QString();
    qint32 nLength = 0;
    while (nLength < nSize && pData[nLength]) ++nLength;
    QString sResult = QString::fromLatin1(
        reinterpret_cast<const char *>(pData), nLength).trimmed();
    sResult.replace(QLatin1Char('\r'), QLatin1Char(' '));
    sResult.replace(QLatin1Char('\n'), QLatin1Char(' '));
    return sResult.simplified();
}

void pftwParseSettings(const QByteArray &baPlain,
                       XPFTW::INTERNAL_INFO *pInfo)
{
    if (!pInfo || baPlain.size() < 12 ||
        baPlain.left(3) != QByteArray("SCG", 3)) {
        return;
    }

    const uchar *p = reinterpret_cast<const uchar *>(baPlain.constData());
    qint32 nOffset = 12;
    while (nOffset < baPlain.size()) {
        const quint8 nTag = p[nOffset++];
        if (nTag >= 0x80) {
            if (nOffset >= baPlain.size()) break;
            ++nOffset;
            continue;
        }

        if (nOffset > baPlain.size() - 2) break;
        const qint32 nLength = qFromLittleEndian<quint16>(p + nOffset);
        nOffset += 2;
        if (nLength < 0 || nOffset > baPlain.size() - nLength) break;

        const QString sValue = pftwMetadataText(p + nOffset, nLength);
        if (!sValue.isEmpty()) {
            if (nTag == 0x03 && pInfo->sCompany.isEmpty()) {
                pInfo->sCompany = sValue;
            } else if (nTag == 0x04 && pInfo->sProduct.isEmpty()) {
                pInfo->sProduct = sValue;
            } else if (nTag == 0x05 && pInfo->sVersion.isEmpty()) {
                pInfo->sVersion = sValue;
            } else if (nTag == 0x07 && pInfo->sRunProgram.isEmpty()) {
                pInfo->sRunProgram = sValue;
            } else if (nTag == 0x08 && pInfo->sRunArguments.isEmpty()) {
                pInfo->sRunArguments = sValue;
            } else if (nTag == 0x0b && pInfo->sCopyright.isEmpty()) {
                pInfo->sCopyright = sValue;
            } else if (nTag == 0x11 && pInfo->sSupportContact.isEmpty()) {
                pInfo->sSupportContact = sValue;
            }
        }
        nOffset += nLength;
    }
}

void pftwPublishMetadata(const XPFTW::INTERNAL_INFO &info,
                         QMap<XBinary::FPART_PROP, QVariant> *pProperties)
{
    if (!pProperties) return;
    QStringList listInfo;
    const QString sExisting =
        pProperties->value(XBinary::FPART_PROP_INFO).toString().trimmed();
    if (!sExisting.isEmpty()) listInfo.append(sExisting);
    if (!info.sCompany.isEmpty())
        listInfo.append(QStringLiteral("Company: %1").arg(info.sCompany));
    if (!info.sProduct.isEmpty())
        listInfo.append(QStringLiteral("Product: %1").arg(info.sProduct));
    if (!info.sVersion.isEmpty())
        listInfo.append(QStringLiteral("Version: %1").arg(info.sVersion));
    if (!info.sRunProgram.isEmpty())
        listInfo.append(QStringLiteral("Run program: %1").arg(info.sRunProgram));
    if (!info.sRunArguments.isEmpty())
        listInfo.append(QStringLiteral("Run arguments: %1").arg(info.sRunArguments));
    if (!info.sCopyright.isEmpty())
        listInfo.append(QStringLiteral("Copyright: %1").arg(info.sCopyright));
    if (!info.sSupportContact.isEmpty())
        listInfo.append(QStringLiteral("Support: %1").arg(info.sSupportContact));
    if (!listInfo.isEmpty()) {
        pProperties->insert(XBinary::FPART_PROP_INFO,
                            listInfo.join(QLatin1String("; ")));
    }
}

bool pftwHasVersionMarker(XPE *pPe, qint32 *pnVersionMarker,
                          XBinary::PDSTRUCT *pPdStruct)
{
    if (!pPe || !pnVersionMarker) return false;
    if (*pnVersionMarker >= 0) return *pnVersionMarker != 0;
    *pnVersionMarker = 0;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const QStringList listValues = {
        pPe->getResourcesVersionValue(QStringLiteral("CompanyName")),
        pPe->getResourcesVersionValue(QStringLiteral("ProductName")),
        pPe->getResourcesVersionValue(QStringLiteral("FileDescription"))};
    for (const QString &sValue : listValues) {
        if (sValue.contains(QStringLiteral("PackageForTheWeb"),
                            Qt::CaseInsensitive)) {
            *pnVersionMarker = 1;
            break;
        }
    }
    return *pnVersionMarker != 0;
}

class PFTW_OPERATION_STATE_DELETER {
public:
    explicit PFTW_OPERATION_STATE_DELETER(
        const QSharedPointer<XPFTW::UNPACK_DEFERRED_CLEANUP> &pCleanup)
        : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XPFTW::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};
}  // namespace

XPFTW::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
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

XPFTW::XPFTW(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
    : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_pUnpackDeferredCleanup =
        QSharedPointer<UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<UNPACK_DEFERRED_CLEANUP> pDeferredCleanup =
        m_pUnpackDeferredCleanup;
    m_pUnpackOperationState = QSharedPointer<bool>(
        new bool(false),
        PFTW_OPERATION_STATE_DELETER(pDeferredCleanup));
    m_internalInfo = INTERNAL_INFO();
    setIsArchive(true);
}

XPFTW::~XPFTW()
{
    if (m_pUnpackOperationState) *m_pUnpackOperationState = true;
    if (m_pUnpackDeferredCleanup) {
        m_pUnpackDeferredCleanup->setContexts.unite(m_setUnpackContexts);
        m_setUnpackContexts.clear();
    }
    m_pUnpackDeferredCleanup.clear();
    m_pUnpackOperationState.clear();
}

bool XPFTW::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XPFTW> guardedThis(this);
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(
        guardedThis->getInternalInfo(pPdStruct));
    return guardedThis && pInfo && pInfo->bIsValid;
}

bool XPFTW::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPFTW x(pDevice);
    return x.isValid(pPdStruct);
}

XPFTW::INTERNAL_INFO XPFTW::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

bool XPFTW::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XPFTW> guardedThis(this);
    const bool bAlreadyHandled = guardedThis->isInternalInfoHandled();
    if (!guardedThis) return false;

    if (!bAlreadyHandled) {
        const quint64 nTransaction = guardedThis->beginInternalInfoTransaction();
        if (!nTransaction) return false;

        guardedThis->m_internalInfo = INTERNAL_INFO();
        INTERNAL_INFO info = guardedThis->_getInternalInfo(pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }

        const XBinary::_MEMORY_MAP memoryMap =
            guardedThis->getMemoryMap(MAPMODE_UNKNOWN, pPdStruct);
        if (!guardedThis) return false;
        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        info.memoryMap = memoryMap;

        if (!guardedThis->isInternalInfoTransactionCurrent(nTransaction)) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
        guardedThis->m_internalInfo = info;
        if (!guardedThis->commitInternalInfoTransaction(
                nTransaction,
                static_cast<XBinary::INTERNAL_INFO *>(
                    &guardedThis->m_internalInfo))) {
            guardedThis->rollbackInternalInfoTransaction(nTransaction);
            return false;
        }
    }

    return true;
}

void *XPFTW::getInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XPFTW> guardedThis(this);
    const bool bHandled = guardedThis->handleInternalInfo(pPdStruct);
    if (!guardedThis || !bHandled) return nullptr;
    return &guardedThis->m_internalInfo;
}

void XPFTW::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        setIsInternalInfoHandled(true);
        XBinary::setInternalInfo(
            static_cast<XBinary::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        setIsInternalInfoHandled(false);
        XBinary::setInternalInfo(nullptr);
    }
}

XBinary::FT XPFTW::getFileType()
{
    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (pe.isValid() && pe.is64()) return FT_PE64_PFTW;
    return FT_PE32_PFTW;
}

XPFTW::INTERNAL_INFO XPFTW::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nHeaderOffset = -1;
    result.nArchiveOffset = -1;

    QPointer<XPFTW> guardedThis(this);
    QPointer<QIODevice> guardedDevice(getDevice());
    if (!guardedDevice || guardedDevice->isSequential() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return result;
    }

    const qint64 nTotalSize = guardedDevice->size();
    if (nTotalSize < 64) return result;

    XPE pe(guardedDevice.data(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct) || !guardedThis || !guardedDevice) return result;

    QList<PFTW_REGION> listRegions;
    const QList<XPE_DEF::IMAGE_SECTION_HEADER> listSectionHeaders =
        pe.getSectionHeaders(pPdStruct);
    QList<XPE_DEF::IMAGE_SECTION_HEADER> mutableSectionHeaders =
        listSectionHeaders;
    const QList<XPE::SECTION_RECORD> listSectionRecords =
        pe.getSectionRecords(&mutableSectionHeaders, pPdStruct);
    if (!guardedThis || !guardedDevice ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return result;
    }

    for (const XPE::SECTION_RECORD &section : listSectionRecords) {
        if (section.sName != QLatin1String("_cabinet")) continue;
        if (!pftwRangeWithin(nTotalSize, section.nOffset, section.nSize)) {
            return result;
        }
        PFTW_REGION region = {};
        region.nOffset = section.nOffset;
        region.nSize = section.nSize;
        region.nVariant = PFTW_VARIANT_CABINET_SECTION;
        region.listHeaderStarts << 0 << 12;
        listRegions.append(region);
    }

    if (listRegions.isEmpty()) {
        const qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
        if (!guardedThis || !guardedDevice || nOverlayOffset <= 0 ||
            nOverlayOffset >= nTotalSize) {
            return result;
        }
        PFTW_REGION region = {};
        region.nOffset = nOverlayOffset;
        region.nSize = nTotalSize - nOverlayOffset;
        region.nVariant = PFTW_VARIANT_OVERLAY;
        region.listHeaderStarts << 0;
        listRegions.append(region);
    }

    qint32 nVersionMarker = -1;

    for (const PFTW_REGION &region : listRegions) {
        for (qint64 nHeaderStart : region.listHeaderStarts) {
            if (!guardedThis || !guardedDevice ||
                !XBinary::isPdStructNotCanceled(pPdStruct) ||
                nHeaderStart < 0 || nHeaderStart > region.nSize - 4) {
                continue;
            }

            const qint64 nSizeOffset = region.nOffset + nHeaderStart;
            const QByteArray baSize =
                read_array_process(nSizeOffset, 4, pPdStruct);
            if (!guardedThis || !guardedDevice || baSize.size() != 4) continue;
            const qint64 nHeaderSize = qFromLittleEndian<quint32>(
                reinterpret_cast<const uchar *>(baSize.constData()));
            if (nHeaderSize < PFTW_MIN_HEADER_SIZE ||
                nHeaderSize > PFTW_MAX_HEADER_SIZE) {
                continue;
            }

            const qint64 nAfterSize = region.nSize - nHeaderStart - 4;
            if (nAfterSize < 36 || nHeaderSize > nAfterSize - 36) continue;
            const qint64 nHeaderOffset = nSizeOffset + 4;
            const qint64 nArchiveOffset = nHeaderOffset + nHeaderSize;
            if (!pftwRangeWithin(nTotalSize, nHeaderOffset, nHeaderSize) ||
                !pftwRangeWithin(nTotalSize, nArchiveOffset, 36)) {
                continue;
            }

            const QByteArray baCabHeader =
                read_array_process(nArchiveOffset, 12, pPdStruct);
            if (!guardedThis || !guardedDevice || baCabHeader.size() != 12 ||
                baCabHeader.left(4) != QByteArray("MSCF", 4)) {
                continue;
            }
            const uchar *pCab = reinterpret_cast<const uchar *>(
                baCabHeader.constData());
            if (qFromLittleEndian<quint32>(pCab + 4) != 0) continue;
            const qint64 nCabinetSize =
                qFromLittleEndian<quint32>(pCab + 8);
            if (nCabinetSize < 36) continue;

            const QByteArray baRawHeader =
                read_array_process(nHeaderOffset, nHeaderSize, pPdStruct);
            if (!guardedThis || !guardedDevice ||
                baRawHeader.size() != nHeaderSize) {
                continue;
            }

            QByteArray baPlainHeader = baRawHeader;
            for (qint32 i = 0; i < baPlainHeader.size(); ++i) {
                baPlainHeader[i] = char(
                    quint8(baPlainHeader.at(i)) ^ quint8(0x61 + (i % 26)));
            }
            bool bOldHeader = false;
            if (baPlainHeader.size() >= 8 &&
                baPlainHeader.left(3) == QByteArray("SCG", 3)) {
                const quint32 nHeaderCabinetSize =
                    qFromLittleEndian<quint32>(
                        reinterpret_cast<const uchar *>(
                            baPlainHeader.constData()) + 4);
                bOldHeader = (nHeaderCabinetSize == quint32(nCabinetSize));
            }
            const bool bNewHeader =
                baRawHeader.size() >= 3 &&
                quint8(baRawHeader.at(0)) == 0xdc &&
                quint8(baRawHeader.at(1)) == 0xed &&
                quint8(baRawHeader.at(2)) == 0xbd;
            if (!bOldHeader && !bNewHeader &&
                !pftwHasVersionMarker(&pe, &nVersionMarker, pPdStruct)) {
                continue;
            }

            result.bIsValid = true;
            result.nVariant = region.nVariant;
            result.nHeaderOffset = nHeaderOffset;
            result.nHeaderSize = nHeaderSize;
            result.nArchiveOffset = nArchiveOffset;
            const qint64 nAvailable = nTotalSize - nArchiveOffset;
            result.bTruncated = nCabinetSize > nAvailable;
            result.nArchiveSize = qMin(nCabinetSize, nAvailable);
            if (bOldHeader) pftwParseSettings(baPlainHeader, &result);
            if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
                result.bIsValid = false;
            }
            return result;
        }
    }

    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XPFTW::getDefaultUnpackProperties()
{
    return XBinary::getDefaultUnpackProperties();
}

bool XPFTW::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    if (!pState) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XPFTW> guardedThis(this);
    if (!pState->baUnpackSourceToken.isEmpty()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext =
            static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pOldContext) ||
            pOldContext->pOwnerState != pState) {
            return false;
        }
        m_setUnpackContexts.remove(pOldContext);
        pState->pContext = nullptr;
        bool bFinishOK = true;
        if (pOldContext->pArchive) {
            bFinishOK = pOldContext->pArchive->finishUnpack(
                &pOldContext->innerState, nullptr);
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
    XPFTW detector(guardedSource.data(), isImage(), getModuleAddress());
    const INTERNAL_INFO info = detector._detect(pPdStruct);
    if (!guardedThis || !guardedSource || !info.bIsValid ||
        info.nArchiveOffset < 0 || info.nArchiveSize <= 0) {
        return false;
    }
    const qint64 nTotalSize = guardedSource->size();
    if (!guardedThis || !guardedSource || nTotalSize < 0) return false;

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pSubDevice = new SubDevice(
        guardedSource.data(), info.nArchiveOffset, info.nArchiveSize);
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();

    if (!pContext->pSubDevice->open(QIODevice::ReadOnly)) {
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pContext->pArchive = new XCab(pContext->pSubDevice);
    if (!pContext->pArchive->initUnpack(
            &pContext->innerState, mapProperties, pPdStruct) ||
        !guardedThis || !guardedSource) {
        pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pSubDevice->close();
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pftwPublishMetadata(info, &pContext->innerState.mapArchiveProperties);
    pState->nNumberOfRecords = pContext->innerState.nNumberOfRecords;
    pState->nTotalSize = nTotalSize;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties =
        pContext->innerState.mapArchiveProperties;
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    m_setUnpackContexts.insert(pContext);
    return true;
}

XBinary::ARCHIVERECORD XPFTW::infoCurrent(UNPACK_STATE *pState,
                                          PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XPFTW> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() ||
        !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return result;
    }
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) ||
        pContext->pOwnerState != pState ||
        !pContext->pOuterSourceDevice ||
        pContext->pOuterSourceDevice != getDevice() ||
        pContext->nOwnerDeviceGeneration != getDeviceGeneration() ||
        !pContext->pArchive ||
        pContext->innerState.nCurrentIndex != pState->nCurrentIndex ||
        pContext->innerState.nCurrentOffset != pState->nCurrentOffset ||
        pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords) {
        return result;
    }
    result = pContext->pArchive->infoCurrent(
        &pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) ||
        pState->pContext != pContext) {
        return ARCHIVERECORD();
    }
    return result;
}

bool XPFTW::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                          PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XPFTW> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() ||
        !pState->pContext || !guardedOutput || !guardedOutput->isOpen() ||
        !guardedOutput->isWritable() || guardedOutput->isSequential() ||
        !guardedThis || !guardedOutput ||
        (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) ||
        pContext->pOwnerState != pState ||
        !pContext->pOuterSourceDevice ||
        pContext->pOuterSourceDevice != getDevice() ||
        pContext->nOwnerDeviceGeneration != getDeviceGeneration() ||
        !pContext->pArchive ||
        pContext->innerState.nCurrentIndex != pState->nCurrentIndex ||
        pContext->innerState.nCurrentOffset != pState->nCurrentOffset ||
        pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords) {
        return false;
    }
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    const bool bResult = pContext->pArchive->unpackCurrent(
        &pContext->innerState, guardedOutput.data(), pPdStruct);
    if (!guardedThis || !guardedOutput ||
        !m_setUnpackContexts.contains(pContext) ||
        pState->pContext != pContext) {
        return false;
    }
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties =
        pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XPFTW::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XPFTW> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() ||
        !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    UNPACK_CONTEXT *pContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setUnpackContexts.contains(pContext) ||
        pContext->pOwnerState != pState ||
        !pContext->pOuterSourceDevice ||
        pContext->pOuterSourceDevice != getDevice() ||
        pContext->nOwnerDeviceGeneration != getDeviceGeneration() ||
        !pContext->pArchive ||
        pContext->innerState.nCurrentIndex != pState->nCurrentIndex ||
        pContext->innerState.nCurrentOffset != pState->nCurrentOffset ||
        pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords) {
        return false;
    }
    const bool bResult = pContext->pArchive->moveToNext(
        &pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setUnpackContexts.contains(pContext) ||
        pState->pContext != pContext) {
        return false;
    }
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties =
        pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XPFTW::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XPFTW> guardedThis(this);
    bool bResult = true;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext =
            static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setUnpackContexts.contains(pContext) ||
            pContext->pOwnerState != pState) {
            return false;
        }
        m_setUnpackContexts.remove(pContext);
        pState->pContext = nullptr;
        if (pContext->pArchive) {
            bResult = pContext->pArchive->finishUnpack(
                &pContext->innerState, nullptr);
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
