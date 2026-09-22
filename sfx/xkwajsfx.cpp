/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xkwajsfx.h"

#include <QScopedValueRollback>

#include <algorithm>
#include <limits>
#include <new>

#include "subdevice.h"
#include "xne.h"
#include "xpe.h"
#include "xkwaj.h"

namespace {

const qint32 KWAJSFX_MAX_RESOURCES = 10000;
const uchar KWAJ_SIGNATURE[8] = {'K', 'W', 'A', 'J', 0x88, 0xF0, 0x27, 0xD1};

QString numericResourceToken(quint32 nValue, bool bNeOrdinal)
{
    if (bNeOrdinal) nValue |= 0x8000;
    return QStringLiteral("%1").arg(nValue, 4, 16, QLatin1Char('0')).toLower();
}

QString namedResourceToken(const QString &sValue)
{
    QString result;
    result.reserve(sValue.size());
    bool bLastUnderscore = false;
    for (const QChar ch : sValue) {
        const bool bAllowed = ch.isLetterOrNumber() || (ch == QLatin1Char('.')) || (ch == QLatin1Char('-'));
        if (bAllowed) {
            result.append(ch.toLower());
            bLastUnderscore = false;
        } else if (!bLastUnderscore) {
            result.append(QLatin1Char('_'));
            bLastUnderscore = true;
        }
    }
    while (result.startsWith(QLatin1Char('_'))) result.remove(0, 1);
    while (result.endsWith(QLatin1Char('_'))) result.chop(1);
    return result.isEmpty() ? QStringLiteral("named") : result;
}

bool hasKwajSignature(XKwajSFX *pOwner, qint64 nOffset, qint64 nSize)
{
    if (!pOwner || (nSize < 8) || !pOwner->checkOffsetSize(nOffset, 8)) return false;
    const QByteArray baSignature = pOwner->read_array(nOffset, 8);
    return (baSignature.size() == 8) && (memcmp(baSignature.constData(), KWAJ_SIGNATURE, sizeof(KWAJ_SIGNATURE)) == 0);
}

QString readNeResourceName(XKwajSFX *pOwner,
                           qint64 nTableOffset, quint16 nToken)
{
    if ((nToken & 0x8000) || !pOwner) return QString();
    const qint64 nOffset = nTableOffset + nToken;
    if (!pOwner->checkOffsetSize(nOffset, 1)) return QString();
    const quint8 nLength = pOwner->read_uint8(nOffset);
    if (!pOwner || !pOwner->checkOffsetSize(nOffset + 1, nLength)) {
        return QString();
    }
    return QString::fromLatin1(pOwner->read_array(nOffset + 1, nLength));
}

bool kwajSfxEntryLess(const XKwajSFX::KWAJSFX_ENTRY &a,
                      const XKwajSFX::KWAJSFX_ENTRY &b)
{
    if (a.nResourceOffset != b.nResourceOffset) {
        return a.nResourceOffset < b.nResourceOffset;
    }
    if (a.nResourceSize != b.nResourceSize) {
        return a.nResourceSize < b.nResourceSize;
    }
    return a.sName < b.sName;
}

class KWAJSFX_OPERATION_STATE_DELETER {
public:
    explicit KWAJSFX_OPERATION_STATE_DELETER(
        const QSharedPointer<XKwajSFX::KWAJSFX_UNPACK_DEFERRED_CLEANUP> &pCleanup)
        : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XKwajSFX::KWAJSFX_UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

}  // namespace

XKwajSFX::KWAJSFX_UNPACK_DEFERRED_CLEANUP::~KWAJSFX_UNPACK_DEFERRED_CLEANUP()
{
    const QSet<KWAJSFX_UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();

    for (KWAJSFX_UNPACK_CONTEXT *pContext : contexts) {
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

XKwajSFX::XKwajSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, FT_KWAJ)
{
    m_pKwajUnpackDeferredCleanup = QSharedPointer<KWAJSFX_UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<KWAJSFX_UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pKwajUnpackDeferredCleanup;
    m_pKwajUnpackOperationState = QSharedPointer<bool>(
        new bool(false), KWAJSFX_OPERATION_STATE_DELETER(pDeferredCleanup));
}

XKwajSFX::~XKwajSFX()
{
    if (m_pKwajUnpackOperationState) *m_pKwajUnpackOperationState = true;
    if (m_pKwajUnpackDeferredCleanup) {
        m_pKwajUnpackDeferredCleanup->setContexts.unite(m_setKwajUnpackContexts);
        m_setKwajUnpackContexts.clear();
    }
    m_pKwajUnpackDeferredCleanup.clear();
    m_pKwajUnpackOperationState.clear();
}

bool XKwajSFX::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QList<KWAJSFX_ENTRY> listEntries;
    const bool bResourceContainer = _buildResourceEntries(&listEntries, pPdStruct);
    if (bResourceContainer && !listEntries.isEmpty()) return true;
    return XSFX::isValid(pPdStruct);
}

bool XKwajSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XKwajSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}

QMap<XBinary::UNPACK_PROP, QVariant> XKwajSFX::getDefaultUnpackProperties()
{
    XKWAJ archive;
    return archive.getDefaultUnpackProperties();
}

bool XKwajSFX::_buildResourceEntries(QList<KWAJSFX_ENTRY> *pList, PDSTRUCT *pPdStruct)
{
    if (!pList || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    pList->clear();

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    QList<KWAJSFX_ENTRY> listCandidates;
    bool bNE = false;

    XNE ne(guardedSource, isImage(), getModuleAddress());
    if (ne.isValid(pPdStruct)) {
        bNE = true;
        // Preserve the raw TYPEINFO/name tokens while walking the bounded NE
        // table. XNE::getResourceStructs() deliberately normalizes a named
        // type to nType=0, but these installers use the table-relative type
        // token (for example 0x0170) as part of their stable resource identity.
        const qint64 nTableOffset = ne.getResourceTableOffset();
        if (!checkOffsetSize(nTableOffset, 2)) return false;
        const quint16 nShift = read_uint16(nTableOffset);
        if (nShift > 47) return false;

        qint64 nCurrentOffset = nTableOffset + 2;
        qint32 nGuard = 0;
        bool bTerminated = false;
        while (checkOffsetSize(nCurrentOffset, 8) && (nGuard++ < 0x4000)) {
            const quint16 nRawType = read_uint16(nCurrentOffset);
            if (nRawType == 0) {
                bTerminated = true;
                break;
            }
            const quint16 nCount = read_uint16(nCurrentOffset + 2);
            if ((nCount > 0x4000) || !checkOffsetSize(nCurrentOffset + 8, (qint64)nCount * 12)) return false;
            const quint32 nTypeToken = (nRawType & 0x8000) ? (nRawType & 0x7fff) : nRawType;
            const QString sType = numericResourceToken(nTypeToken, false);
            nCurrentOffset += 8;

            for (quint16 i = 0; i < nCount; ++i) {
                if (!guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
                const qint64 nNameInfoOffset = nCurrentOffset + (qint64)i * 12;
                const quint16 nRawID = read_uint16(nNameInfoOffset + 6);
                const qint64 nResourceOffset = ((qint64)read_uint16(nNameInfoOffset)) << nShift;
                const qint64 nResourceSize = ((qint64)read_uint16(nNameInfoOffset + 2)) << nShift;
                if ((nResourceSize <= 0) || !checkOffsetSize(nResourceOffset, nResourceSize)) continue;

                KWAJSFX_ENTRY entry = {};
                entry.nResourceOffset = nResourceOffset;
                entry.nResourceSize = nResourceSize;
                entry.nType = nTypeToken;
                entry.nID = (nRawID & 0x8000) ? (nRawID & 0x7fff) : nRawID;
                entry.bKwaj = hasKwajSignature(this, entry.nResourceOffset, entry.nResourceSize);
                entry.sGroupKey = QStringLiteral("ne:%1").arg(nRawType);
                QString sID = (nRawID & 0x8000) ? numericResourceToken(nRawID, false)
                                                : namedResourceToken(readNeResourceName(this, nTableOffset, nRawID));
                if (sID.isEmpty()) sID = numericResourceToken(nRawID, false);
                entry.sName = QStringLiteral("res_%1_%2").arg(sType, sID);
                listCandidates.append(entry);
                if (listCandidates.count() > KWAJSFX_MAX_RESOURCES) return false;
            }
            nCurrentOffset += (qint64)nCount * 12;
        }
        if (!bTerminated) return false;
    } else {
        if (!guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        XPE pe(guardedSource, isImage(), getModuleAddress());
        if (!pe.isValid(pPdStruct)) return false;
        const QList<XPE::RESOURCE_RECORD> resources = pe.getResources(KWAJSFX_MAX_RESOURCES, pPdStruct);
        for (const XPE::RESOURCE_RECORD &resource : resources) {
            if (!guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
            if ((resource.nSize <= 0) || !checkOffsetSize(resource.nOffset, resource.nSize)) continue;

            const XPE::RESOURCES_ID_NAME &type = resource.irin[0];
            const XPE::RESOURCES_ID_NAME &name = resource.irin[1];
            const XPE::RESOURCES_ID_NAME &language = resource.irin[2];
            KWAJSFX_ENTRY entry = {};
            entry.nResourceOffset = resource.nOffset;
            entry.nResourceSize = resource.nSize;
            entry.nType = type.bIsName ? 0 : type.nID;
            entry.nID = name.bIsName ? 0 : name.nID;
            entry.bKwaj = hasKwajSignature(this, entry.nResourceOffset, entry.nResourceSize);
            const QString sType = type.bIsName ? namedResourceToken(type.sName) : numericResourceToken(type.nID, false);
            const QString sName = name.bIsName ? namedResourceToken(name.sName) : numericResourceToken(name.nID, false);
            const QString sLanguage = language.bIsName ? namedResourceToken(language.sName) : numericResourceToken(language.nID, false);
            entry.sGroupKey = type.bIsName ? (QStringLiteral("pe:s:") + type.sName) : QStringLiteral("pe:n:%1").arg(type.nID);
            entry.sName = QStringLiteral("res_%1_%2_%3").arg(sType, sName, sLanguage);
            listCandidates.append(entry);
        }
    }

    if (!guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || listCandidates.isEmpty()) return false;

    QMap<QString, qint32> mapKwajCounts;
    QMap<QString, qint64> mapFirstOffsets;
    for (const KWAJSFX_ENTRY &entry : listCandidates) {
        if (!mapFirstOffsets.contains(entry.sGroupKey) || (entry.nResourceOffset < mapFirstOffsets.value(entry.sGroupKey))) {
            mapFirstOffsets.insert(entry.sGroupKey, entry.nResourceOffset);
        }
        if (entry.bKwaj) mapKwajCounts[entry.sGroupKey]++;
    }

    QString sSelectedGroup;
    qint32 nSelectedCount = 0;
    qint64 nSelectedOffset = (std::numeric_limits<qint64>::max)();
    for (QMap<QString, qint32>::const_iterator it = mapKwajCounts.constBegin();
         it != mapKwajCounts.constEnd(); ++it) {
        const qint64 nFirstOffset = mapFirstOffsets.value(it.key(), (std::numeric_limits<qint64>::max)());
        if ((it.value() > nSelectedCount) || ((it.value() == nSelectedCount) && (nFirstOffset < nSelectedOffset))) {
            sSelectedGroup = it.key();
            nSelectedCount = it.value();
            nSelectedOffset = nFirstOffset;
        }
    }
    if (nSelectedCount < 2) return false;

    QList<KWAJSFX_ENTRY> listResult;
    for (KWAJSFX_ENTRY entry : listCandidates) {
        if (entry.sGroupKey != sSelectedGroup) continue;

        if (entry.bKwaj) {
            SubDevice subDevice(guardedSource, entry.nResourceOffset, entry.nResourceSize);
            subDevice.setProperty("FileName", entry.sName);
            if (!subDevice.open(QIODevice::ReadOnly)) return false;

            XKWAJ archive(&subDevice);
            UNPACK_STATE state = {};
            const bool bInitialized = archive.initUnpack(&state, archive.getDefaultUnpackProperties(), pPdStruct);
            if (!guardedSource || !bInitialized) {
                archive.finishUnpack(&state, nullptr);
                return false;
            }
            const ARCHIVERECORD record = archive.infoCurrent(&state, pPdStruct);
            if (!guardedSource || record.mapProperties.value(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_UNKNOWN).toUInt() == HANDLE_METHOD_UNKNOWN ||
                (record.nStreamOffset < 0) || (record.nStreamSize < 0) || (record.nStreamOffset > entry.nResourceSize) ||
                (record.nStreamSize > (entry.nResourceSize - record.nStreamOffset))) {
                archive.finishUnpack(&state, nullptr);
                return false;
            }

            const quint16 nHeaderFlags = read_uint16(entry.nResourceOffset + offsetof(XKWAJ::KWAJ_HEADER, header_flags));
            const bool bHasEmbeddedName = (nHeaderFlags & (XKWAJ::HDR_FLAG_HASFILENAME | XKWAJ::HDR_FLAG_HASFILEEXT)) != 0;
            if (bHasEmbeddedName) {
                const QString sEmbeddedName = record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
                if (!sEmbeddedName.isEmpty()) entry.sName = sEmbeddedName;
            }

            const bool bFinished = archive.finishUnpack(&state, nullptr);
            if (!guardedSource || !bFinished) return false;
        }
        listResult.append(entry);
    }

    std::sort(listResult.begin(), listResult.end(), kwajSfxEntryLess);

    if (!bNE && listResult.isEmpty()) return false;
    *pList = listResult;
    return !pList->isEmpty();
}

bool XKwajSFX::_releaseEntry(KWAJSFX_UNPACK_CONTEXT *pContext)
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

bool XKwajSFX::_bindEntry(KWAJSFX_UNPACK_CONTEXT *pContext, qint32 nIndex, PDSTRUCT *pPdStruct)
{
    if (!pContext || (nIndex < 0) || (nIndex >= pContext->listEntries.count()) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (!_releaseEntry(pContext)) return false;

    const KWAJSFX_ENTRY &entry = pContext->listEntries.at(nIndex);
    if (!entry.bKwaj) return true;

    SubDevice *pSubDevice = new (std::nothrow) SubDevice(pContext->pOuterSourceDevice, entry.nResourceOffset, entry.nResourceSize);
    if (!pSubDevice) return false;
    pSubDevice->setProperty("FileName", entry.sName);
    if (!pSubDevice->open(QIODevice::ReadOnly)) {
        delete pSubDevice;
        return false;
    }

    XKWAJ *pArchive = new (std::nothrow) XKWAJ(pSubDevice);
    if (!pArchive) {
        pSubDevice->close();
        delete pSubDevice;
        return false;
    }

    pContext->pSubDevice = pSubDevice;
    pContext->pArchive = pArchive;
    pContext->innerState = UNPACK_STATE();
    if (!pArchive->initUnpack(&pContext->innerState, pContext->mapUnpackProperties, pPdStruct)) {
        pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pArchive;
        pSubDevice->close();
        delete pSubDevice;
        pContext->pArchive = nullptr;
        pContext->pSubDevice = nullptr;
        pContext->innerState = UNPACK_STATE();
        return false;
    }
    return true;
}

bool XKwajSFX::_isOwnContext(const UNPACK_STATE *pState) const
{
    if (!pState || !pState->pContext) return false;
    return m_setKwajUnpackContexts.contains(static_cast<KWAJSFX_UNPACK_CONTEXT *>(pState->pContext));
}

bool XKwajSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QList<KWAJSFX_ENTRY> listEntries;
    const bool bResourceContainer = _buildResourceEntries(&listEntries, pPdStruct);

    if (!bResourceContainer || listEntries.isEmpty()) {
        if (_isOwnContext(pState) && !finishUnpack(pState, nullptr)) return false;
        return XSFX::initUnpack(pState, mapProperties, pPdStruct);
    }

    if (pState->pContext && !_isOwnContext(pState)) {
        if (!XSFX::finishUnpack(pState, nullptr)) return false;
    }

    QSharedPointer<bool> pOperationState = m_pKwajUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);

    if (pState->pContext) {
        KWAJSFX_UNPACK_CONTEXT *pOldContext = static_cast<KWAJSFX_UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setKwajUnpackContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        m_setKwajUnpackContexts.remove(pOldContext);
        pState->pContext = nullptr;
        const bool bFinishOK = _releaseEntry(pOldContext);
        delete pOldContext;
        *pState = UNPACK_STATE();
        if (!bFinishOK) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    qint64 nOutputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(mapProperties, &nOutputLimit)) return false;
    Q_UNUSED(nOutputLimit)

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;
    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    KWAJSFX_UNPACK_CONTEXT *pContext = new (std::nothrow) KWAJSFX_UNPACK_CONTEXT;
    if (!pContext) return false;
    pContext->listEntries = listEntries;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pOwnerState = pState;
    pContext->pSubDevice = nullptr;
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();
    pContext->mapUnpackProperties = mapProperties;

    if (!_bindEntry(pContext, 0, pPdStruct) || !guardedSource) {
        _releaseEntry(pContext);
        delete pContext;
        return false;
    }

    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nCurrentOffset = listEntries.first().nResourceOffset;
    pState->nTotalSize = guardedSource->size();
    if (pContext->pArchive) pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pState->pContext = pContext;
    m_setKwajUnpackContexts.insert(pContext);
    return true;
}

XBinary::ARCHIVERECORD XKwajSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!_isOwnContext(pState)) return XSFX::infoCurrent(pState, pPdStruct);
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pKwajUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return result;
    }

    KWAJSFX_UNPACK_CONTEXT *pContext = static_cast<KWAJSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setKwajUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return result;
    }

    const KWAJSFX_ENTRY &entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (entry.bKwaj) {
        if (!pContext->pArchive || (pContext->innerState.nCurrentIndex != 0) || (pContext->innerState.nNumberOfRecords != 1)) return result;
        result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
        if (!m_setKwajUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return ARCHIVERECORD();
        result.nStreamOffset += entry.nResourceOffset;
        result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sName);
    } else {
        result.nStreamOffset = entry.nResourceOffset;
        result.nStreamSize = entry.nResourceSize;
        result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sName);
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, entry.nResourceSize);
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, entry.nResourceSize);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    }
    return result;
}

bool XKwajSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    if (!_isOwnContext(pState)) return XSFX::unpackCurrent(pState, pDevice, pPdStruct);
    QSharedPointer<bool> pOperationState = m_pKwajUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    if (!pState || !pState->pContext || !guardedOutput || !guardedSource || XBinary::devicesAlias(guardedSource, guardedOutput) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    KWAJSFX_UNPACK_CONTEXT *pContext = static_cast<KWAJSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setKwajUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || (pContext->pOuterSourceDevice != guardedSource) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return false;
    }

    const KWAJSFX_ENTRY &entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (entry.bKwaj) {
        if (!pContext->pArchive || (pContext->innerState.nCurrentIndex != 0) || (pContext->innerState.nNumberOfRecords != 1)) return false;
        pContext->innerState.spOutputBudget = pState->spOutputBudget;
        const bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput, pPdStruct);
        if (!guardedOutput || !guardedSource || !m_setKwajUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
        pState->nCurrentOffset = entry.nResourceOffset + pContext->innerState.nCurrentOffset;
        pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
        return bResult;
    }

    if ((entry.nResourceSize > (std::numeric_limits<int>::max)()) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, entry.nResourceSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (pState->spOutputBudget && !pState->spOutputBudget->beginEntry(pState->nCurrentIndex, entry.sName)) {
        if (pState->spOutputBudget->isEnforcing()) {
            XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
            return false;
        }
        XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
    }

    const QByteArray baData = read_array_process(entry.nResourceOffset, entry.nResourceSize, pPdStruct);
    if (!guardedOutput || !guardedSource || (baData.size() != entry.nResourceSize) ||
        (pContext->pOuterSourceDevice != guardedSource) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration())) {
        return false;
    }
    const bool bResult = XBinary::writeUnpackData(pState, guardedOutput, baData, pPdStruct);
    if (!guardedOutput || !guardedSource || !m_setKwajUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    if (bResult) pState->nCurrentOffset = entry.nResourceOffset + entry.nResourceSize;
    return bResult;
}

bool XKwajSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!_isOwnContext(pState)) return XSFX::moveToNext(pState, pPdStruct);
    QSharedPointer<bool> pOperationState = m_pKwajUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    if (!pState || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    KWAJSFX_UNPACK_CONTEXT *pContext = static_cast<KWAJSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setKwajUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        _releaseEntry(pContext);
        pState->nCurrentOffset = pState->nTotalSize;
        return false;
    }

    if (!_bindEntry(pContext, pState->nCurrentIndex, pPdStruct)) return false;
    pState->nCurrentOffset = pContext->listEntries.at(pState->nCurrentIndex).nResourceOffset;
    pState->mapArchiveProperties = pContext->pArchive ? pContext->innerState.mapArchiveProperties : QMap<FPART_PROP, QVariant>();
    return true;
}

bool XKwajSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!_isOwnContext(pState)) return XSFX::finishUnpack(pState, pPdStruct);
    Q_UNUSED(pPdStruct)
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pKwajUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    KWAJSFX_UNPACK_CONTEXT *pContext = static_cast<KWAJSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setKwajUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
    m_setKwajUnpackContexts.remove(pContext);
    pState->pContext = nullptr;
    const bool bResult = _releaseEntry(pContext);
    delete pContext;

    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    return bResult;
}
