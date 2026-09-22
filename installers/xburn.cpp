/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xburn.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QTemporaryFile>
#include <QXmlStreamReader>
#include <limits>

#include "subdevice.h"
#include "xpe.h"
#include "xarchive.h"
#include "xcab.h"

namespace {
const qint32 BURN_SECTION_FIXED_SIZE = 48;
const qint32 BURN_SECTION_MIN_SIZE = 52;
const quint32 BURN_SECTION_MAGIC = 0x00F14300U;
const quint32 BURN_SECTION_VERSION = 2U;
const quint32 BURN_CONTAINER_FORMAT_CAB = 1U;
const qint64 BURN_MAX_MANIFEST_SIZE = 16ll << 20;
const qint32 BURN_MAX_RECORDS = 100000;
const qint32 BURN_MAX_CONTAINERS = 116;
const QString BURN_NAMESPACE_V3 = QStringLiteral("http://schemas.microsoft.com/wix/2008/Burn");
const QString BURN_NAMESPACE_V4 = QStringLiteral("http://wixtoolset.org/schemas/v4/2008/Burn");

class BurnOperationGuard {
public:
    explicit BurnOperationGuard(const QSharedPointer<XBurn::UNPACK_LIFETIME_STATE> &pState) : m_pState(pState), m_bAcquired(false)
    {
        if (m_pState && m_pState->bOwnerAlive && !m_pState->bOperationInProgress) {
            m_pState->bOperationInProgress = true;
            m_bAcquired = true;
        }
    }

    ~BurnOperationGuard()
    {
        if (m_pState && m_bAcquired) m_pState->bOperationInProgress = false;
    }

    bool isAcquired() const
    {
        return m_bAcquired;
    }

private:
    QSharedPointer<XBurn::UNPACK_LIFETIME_STATE> m_pState;
    bool m_bAcquired;
};

class BurnPublisher : public XArchive {
public:
    explicit BurnPublisher(QIODevice *pDevice) : XArchive(pDevice)
    {
    }
    using XArchive::publishUnpackOutput;
};

struct BURN_CAB_ENTRY {
    qint32 nIndex;
    QString sName;
    qint64 nSize;
};

struct BURN_MANIFEST_CONTAINER {
    QString sId;
    bool bAttached;
    qint32 nAttachedIndex;
    qint64 nFileSize;
    QByteArray baHash;
};

struct BURN_PENDING_PAYLOAD {
    XBurn::PAYLOAD_RECORD payload;
    bool bUX;
    bool bEmbedded;
    QString sContainerId;
};

struct BURN_PACKAGE_INFO {
    QString sId;
    QString sType;
};

struct BURN_MANIFEST_INFO {
    QList<XBurn::PAYLOAD_RECORD> listUXPayloads;
    QMap<qint32, QList<XBurn::PAYLOAD_RECORD> > mapAttachedPayloads;
    QMap<qint32, BURN_MANIFEST_CONTAINER> mapAttachedContainers;
    bool bIsV4;
};

static quint32 burnReadLE32(const QByteArray &baData, qint32 nOffset)
{
    return (quint32)(quint8)baData.at(nOffset) | ((quint32)(quint8)baData.at(nOffset + 1) << 8) | ((quint32)(quint8)baData.at(nOffset + 2) << 16) |
           ((quint32)(quint8)baData.at(nOffset + 3) << 24);
}

static bool burnRangeIsValid(qint64 nOffset, qint64 nSize, qint64 nTotalSize)
{
    return (nOffset >= 0) && (nSize >= 0) && (nTotalSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

static bool burnIsAllZero(const QByteArray &baData)
{
    for (char ch : baData) {
        if (ch != 0) return false;
    }
    return true;
}

static bool burnParseUnsignedDecimal(const QString &sValue, qint64 *pValue)
{
    if (!pValue || sValue.isEmpty() || (sValue.size() > 20)) return false;

    quint64 nValue = 0;
    const quint64 nMax = (quint64)(std::numeric_limits<qint64>::max)();
    for (QChar ch : sValue) {
        const ushort nUnicode = ch.unicode();
        if ((nUnicode < '0') || (nUnicode > '9')) return false;
        const quint64 nDigit = nUnicode - '0';
        if ((nValue > (nMax / 10)) || ((nValue == (nMax / 10)) && (nDigit > (nMax % 10)))) return false;
        nValue = nValue * 10 + nDigit;
    }

    *pValue = (qint64)nValue;
    return true;
}

static bool burnParseHash(const QString &sValue, qint32 nExpectedBytes, QByteArray *pValue)
{
    if (!pValue || ((nExpectedBytes != 20) && (nExpectedBytes != 64)) || (sValue.size() != (nExpectedBytes * 2))) return false;
    QByteArray baHex = sValue.toLatin1();
    if (baHex.size() != (nExpectedBytes * 2)) return false;

    for (char ch : baHex) {
        const bool bDigit = (ch >= '0') && (ch <= '9');
        const bool bLower = (ch >= 'a') && (ch <= 'f');
        const bool bUpper = (ch >= 'A') && (ch <= 'F');
        if (!bDigit && !bLower && !bUpper) return false;
    }

    QByteArray baResult = QByteArray::fromHex(baHex);
    if (baResult.size() != nExpectedBytes) return false;
    *pValue = baResult;
    return true;
}

static bool burnHashAlgorithm(const QByteArray &baExpected, QCryptographicHash::Algorithm *pAlgorithm)
{
    if (!pAlgorithm) return false;
    if (baExpected.size() == 20) {
        *pAlgorithm = QCryptographicHash::Sha1;
        return true;
    }
    if (baExpected.size() == 64) {
        *pAlgorithm = QCryptographicHash::Sha512;
        return true;
    }
    return false;
}

static bool burnIsSafeRelativePath(const QString &sValue, QString *pNormalized)
{
    if (!pNormalized || sValue.isEmpty() || (sValue.size() > 32767) || sValue.contains(QChar(0))) return false;

    QString sPath = QDir::fromNativeSeparators(sValue);
    if (QDir::isAbsolutePath(sPath) || sPath.startsWith('/') || sPath.startsWith(QStringLiteral("//")) ||
        ((sPath.size() >= 2) && sPath.at(0).isLetter() && (sPath.at(1) == ':')) || sPath.contains(':')) {
        return false;
    }

    const QStringList listParts = sPath.split('/', Qt::KeepEmptyParts);
    if (listParts.isEmpty()) return false;
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QStringLiteral(".")) || (sPart == QStringLiteral(".."))) return false;
        for (QChar ch : sPart) {
            if (ch.unicode() < 0x20) return false;
        }
    }

    const QString sClean = QDir::cleanPath(sPath);
    if (sClean.isEmpty() || (sClean == QStringLiteral(".")) || sClean.startsWith(QStringLiteral("../"))) return false;
    *pNormalized = sClean;
    return true;
}

static bool burnEmbeddedNameIndex(const QString &sName, QChar prefix, qint32 *pIndex)
{
    if (!pIndex || (sName.size() < 2) || (sName.at(0) != prefix)) return false;
    qint64 nIndex = 0;
    if (!burnParseUnsignedDecimal(sName.mid(1), &nIndex) || (nIndex > (std::numeric_limits<qint32>::max)()) || (QString::number(nIndex) != sName.mid(1))) {
        return false;
    }
    *pIndex = (qint32)nIndex;
    return true;
}

static bool burnHashDevice(QIODevice *pDevice, QCryptographicHash::Algorithm algorithm, QByteArray *pHash, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pHash || !pDevice->isOpen() || !pDevice->isReadable() || pDevice->isSequential() || !pDevice->seek(0)) {
        return false;
    }

    const qint64 nSize = pDevice->size();
    if (nSize < 0) return false;

    QCryptographicHash hash(algorithm);
    QByteArray baBuffer;
    baBuffer.resize(0x10000);
    if (baBuffer.size() != 0x10000) return false;

    qint64 nReadTotal = 0;
    while (nReadTotal < nSize) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRequest = qMin<qint64>(baBuffer.size(), nSize - nReadTotal);
        const qint64 nRead = pDevice->read(baBuffer.data(), nRequest);
        if ((nRead <= 0) || (nRead > nRequest)) return false;
        hash.addData(QByteArray::fromRawData(baBuffer.constData(), (int)nRead));
        nReadTotal += nRead;
    }

    if ((pDevice->pos() != nSize) || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    *pHash = hash.result();
    const qint32 nExpectedSize = (algorithm == QCryptographicHash::Sha1) ? 20 : ((algorithm == QCryptographicHash::Sha512) ? 64 : 0);
    return (nExpectedSize != 0) && (pHash->size() == nExpectedSize);
}

static bool burnHashRange(QIODevice *pDevice, qint64 nOffset, qint64 nSize, QCryptographicHash::Algorithm algorithm, QByteArray *pHash, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pHash || !burnRangeIsValid(nOffset, nSize, pDevice->size())) return false;
    SubDevice subDevice(pDevice, nOffset, nSize);
    if (!subDevice.open(QIODevice::ReadOnly)) return false;
    const bool bResult = burnHashDevice(&subDevice, algorithm, pHash, pPdStruct);
    subDevice.close();
    return bResult;
}

static bool burnScanCabinet(QIODevice *pDevice, qint64 nOffset, qint64 nSize, bool bUX, QList<BURN_CAB_ENTRY> *pEntries, QByteArray *pManifest,
                            XBinary::PDSTRUCT *pPdStruct)
{
    if (!pDevice || !pEntries || (bUX && !pManifest) || !burnRangeIsValid(nOffset, nSize, pDevice->size()) || (nSize < (qint64)sizeof(XCab::CFHEADER))) {
        return false;
    }

    pEntries->clear();
    if (pManifest) pManifest->clear();

    SubDevice subDevice(pDevice, nOffset, nSize);
    if (!subDevice.open(QIODevice::ReadOnly)) return false;

    XCab cabinet(&subDevice);
    XBinary::UNPACK_STATE state = {};
    bool bInitialized = cabinet.initUnpack(&state, cabinet.getDefaultUnpackProperties(), pPdStruct);
    bool bResult = bInitialized;

    if (bResult) {
        const qint64 nCabinetSize = cabinet.getFileFormatSize(pPdStruct);
        bResult = (nCabinetSize == nSize) && (state.nNumberOfRecords >= (bUX ? 1 : 0)) && (state.nNumberOfRecords <= BURN_MAX_RECORDS);
    }

    QSet<QString> setNames;
    for (qint32 i = 0; bResult && (i < state.nNumberOfRecords); i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) {
            bResult = false;
            break;
        }

        const XBinary::ARCHIVERECORD record = cabinet.infoCurrent(&state, pPdStruct);
        const QString sName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
        bool bSizeOK = false;
        const qint64 nFileSize = record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bSizeOK);
        const QString sNameKey = sName.toCaseFolded();
        if (sName.isEmpty() || sName.contains(QChar(0)) || !bSizeOK || (nFileSize < 0) || setNames.contains(sNameKey) ||
            (bUX && (i == 0) && (sName != QStringLiteral("0")))) {
            bResult = false;
            break;
        }
        setNames.insert(sNameKey);

        BURN_CAB_ENTRY entry = {};
        entry.nIndex = i;
        entry.sName = sName;
        entry.nSize = nFileSize;
        pEntries->append(entry);

        if (bUX && (i == 0)) {
            if ((nFileSize <= 0) || (nFileSize > BURN_MAX_MANIFEST_SIZE)) {
                bResult = false;
                break;
            }
            QBuffer manifestBuffer(pManifest);
            if (!manifestBuffer.open(QIODevice::ReadWrite) || !cabinet.unpackCurrent(&state, &manifestBuffer, pPdStruct) || (pManifest->size() != nFileSize)) {
                bResult = false;
                break;
            }
        }

        if (((i + 1) < state.nNumberOfRecords) && !cabinet.moveToNext(&state, pPdStruct)) {
            bResult = false;
            break;
        }
    }

    if (bInitialized && !cabinet.finishUnpack(&state, pPdStruct)) {
        bResult = false;
    }
    subDevice.close();
    return bResult && XBinary::isPdStructNotCanceled(pPdStruct);
}

static bool burnIsPackageElement(const QString &sName)
{
    return (sName == QStringLiteral("MsiPackage")) || (sName == QStringLiteral("ExePackage")) || (sName == QStringLiteral("MsuPackage")) ||
           (sName == QStringLiteral("MspPackage")) || (sName == QStringLiteral("BundlePackage"));
}

static bool burnReadPayloadAttributes(const QXmlStreamAttributes &attributes, bool bUX, bool bIsV4, BURN_PENDING_PAYLOAD *pPayload)
{
    if (!pPayload) return false;

    const QString sId = attributes.value(QStringLiteral("Id")).toString();
    const QString sFilePath = attributes.value(QStringLiteral("FilePath")).toString();
    const QString sFileSize = attributes.value(QStringLiteral("FileSize")).toString();
    const QString sHash = attributes.value(QStringLiteral("Hash")).toString();
    const QString sPackaging = attributes.value(QStringLiteral("Packaging")).toString();
    const QString sSourcePath = attributes.value(QStringLiteral("SourcePath")).toString();
    const QString sContainer = attributes.value(QStringLiteral("Container")).toString();

    qint64 nFileSize = -1;
    QByteArray baHash;
    QString sNormalizedPath;
    if (sId.isEmpty() || (sId.size() > 32767) || sId.contains(QChar(0)) || !burnIsSafeRelativePath(sFilePath, &sNormalizedPath) || sSourcePath.isEmpty() ||
        (sSourcePath.size() > 32767) || sSourcePath.contains(QChar(0))) {
        return false;
    }

    const qint32 nHashSize = bIsV4 ? 64 : 20;
    if (bUX && bIsV4 && sFileSize.isEmpty() && sHash.isEmpty()) {
        // WiX v4 intentionally writes only Id, FilePath, and SourcePath for UX
        // payloads. The physical CAB supplies their sizes.
    } else if (!burnParseUnsignedDecimal(sFileSize, &nFileSize) || !burnParseHash(sHash, nHashSize, &baHash)) {
        return false;
    }

    const bool bImplicitV4UX = bUX && bIsV4 && sPackaging.isEmpty();
    const bool bEmbedded = bImplicitV4UX || (sPackaging.compare(QStringLiteral("embedded"), Qt::CaseInsensitive) == 0);
    const bool bExternal = (sPackaging.compare(QStringLiteral("external"), Qt::CaseInsensitive) == 0);
    if ((!bEmbedded && !bExternal) || (bUX && (!bEmbedded || !sContainer.isEmpty()))) return false;

    pPayload->payload = XBurn::PAYLOAD_RECORD();
    pPayload->payload.containerType = bUX ? XBurn::CONTAINER_TYPE_UX : XBurn::CONTAINER_TYPE_UNKNOWN;
    pPayload->payload.nContainerIndex = bUX ? 0 : -1;
    pPayload->payload.nInnerIndex = -1;
    pPayload->payload.sSourcePath = sSourcePath;
    pPayload->payload.sFilePath = sNormalizedPath;
    pPayload->payload.sPayloadId = sId;
    pPayload->payload.nFileSize = nFileSize;
    pPayload->payload.baHash = baHash;
    pPayload->bUX = bUX;
    pPayload->bEmbedded = bEmbedded;
    pPayload->sContainerId = sContainer;

    // External records are represented by UNKNOWN and intentionally never
    // enter the physical record list.
    if (!bEmbedded) pPayload->payload.containerType = XBurn::CONTAINER_TYPE_UNKNOWN;
    return true;
}

static bool burnParseManifest(const QByteArray &baManifest, BURN_MANIFEST_INFO *pInfo, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pInfo || baManifest.isEmpty() || (baManifest.size() > BURN_MAX_MANIFEST_SIZE)) return false;
    *pInfo = BURN_MANIFEST_INFO();

    QXmlStreamReader xml(baManifest);
    QList<QString> listElements;
    QList<BURN_PENDING_PAYLOAD> listPayloads;
    QMap<QString, BURN_MANIFEST_CONTAINER> mapContainers;
    QMap<QString, BURN_PACKAGE_INFO> mapPayloadPackages;
    QSet<QString> setPayloadIds;
    QSet<QString> setPayloadRefs;
    QSet<QString> setPackageIds;
    BURN_PACKAGE_INFO currentPackage;
    QString sCurrentPackageElement;
    bool bRootSeen = false;
    bool bUXSeen = false;
    bool bChainSeen = false;
    bool bIsV4 = false;
    QString sManifestNamespace;
    qint32 nElements = 0;

    while (!xml.atEnd()) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        xml.readNext();

        if (xml.isDTD() || xml.isEntityReference()) return false;
        if (xml.isStartElement()) {
            if (++nElements > BURN_MAX_RECORDS * 8) return false;
            const QString sName = xml.name().toString();
            const QString sNamespace = xml.namespaceUri().toString();
            if (listElements.size() >= 64) return false;

            if (listElements.isEmpty()) {
                if (bRootSeen || (sName != QStringLiteral("BurnManifest"))) return false;
                if ((sNamespace != BURN_NAMESPACE_V3) && (sNamespace != BURN_NAMESPACE_V4)) return false;
                sManifestNamespace = sNamespace;
                bIsV4 = (sNamespace == BURN_NAMESPACE_V4);
                bRootSeen = true;
            } else {
                if (sNamespace != sManifestNamespace) return false;
            }

            if ((listElements.size() == 1) && (listElements.at(0) == QStringLiteral("BurnManifest"))) {
                if (sName == QStringLiteral("UX")) {
                    if (bUXSeen) return false;
                    bUXSeen = true;
                } else if (sName == QStringLiteral("Chain")) {
                    if (bChainSeen) return false;
                    bChainSeen = true;
                } else if (sName == QStringLiteral("Container")) {
                    BURN_MANIFEST_CONTAINER container = {};
                    container.sId = xml.attributes().value(QStringLiteral("Id")).toString();
                    const QString sFileSize = xml.attributes().value(QStringLiteral("FileSize")).toString();
                    const QString sHash = xml.attributes().value(QStringLiteral("Hash")).toString();
                    const QString sAttached = xml.attributes().value(QStringLiteral("Attached")).toString();
                    const QString sAttachedIndex = xml.attributes().value(QStringLiteral("AttachedIndex")).toString();
                    container.bAttached = (sAttached.compare(QStringLiteral("yes"), Qt::CaseInsensitive) == 0);
                    container.nAttachedIndex = -1;
                    if (container.sId.isEmpty() || (container.sId.size() > 32767) || container.sId.contains(QChar(0)) || mapContainers.contains(container.sId) ||
                        !burnParseUnsignedDecimal(sFileSize, &container.nFileSize) || !burnParseHash(sHash, bIsV4 ? 64 : 20, &container.baHash) ||
                        (!sAttached.isEmpty() && !container.bAttached)) {
                        return false;
                    }
                    if (container.bAttached) {
                        qint64 nIndex = -1;
                        if (!burnParseUnsignedDecimal(sAttachedIndex, &nIndex) || (nIndex < 1) || (nIndex > (std::numeric_limits<qint32>::max)())) return false;
                        container.nAttachedIndex = (qint32)nIndex;
                    } else if (!sAttachedIndex.isEmpty()) {
                        return false;
                    }
                    mapContainers.insert(container.sId, container);
                }
            }

            const bool bUXPayload = (sName == QStringLiteral("Payload")) && (listElements.size() == 2) && (listElements.at(0) == QStringLiteral("BurnManifest")) &&
                                    (listElements.at(1) == QStringLiteral("UX"));
            const bool bGlobalPayload = (sName == QStringLiteral("Payload")) && (listElements.size() == 1) && (listElements.at(0) == QStringLiteral("BurnManifest"));
            if (bUXPayload || bGlobalPayload) {
                BURN_PENDING_PAYLOAD payload = {};
                if (!burnReadPayloadAttributes(xml.attributes(), bUXPayload, bIsV4, &payload) || setPayloadIds.contains(payload.payload.sPayloadId)) {
                    return false;
                }
                setPayloadIds.insert(payload.payload.sPayloadId);
                listPayloads.append(payload);
            }

            if ((listElements.size() == 2) && (listElements.at(0) == QStringLiteral("BurnManifest")) && (listElements.at(1) == QStringLiteral("Chain")) &&
                burnIsPackageElement(sName)) {
                currentPackage.sId = xml.attributes().value(QStringLiteral("Id")).toString();
                currentPackage.sType = sName.left(sName.size() - QStringLiteral("Package").size());
                sCurrentPackageElement = sName;
                if (currentPackage.sId.isEmpty() || (currentPackage.sId.size() > 32767) || currentPackage.sId.contains(QChar(0)) ||
                    setPackageIds.contains(currentPackage.sId)) {
                    return false;
                }
                setPackageIds.insert(currentPackage.sId);
            } else if ((sName == QStringLiteral("PayloadRef")) && (listElements.size() == 3) && (listElements.at(0) == QStringLiteral("BurnManifest")) &&
                       (listElements.at(1) == QStringLiteral("Chain")) && (listElements.at(2) == sCurrentPackageElement) && !currentPackage.sId.isEmpty()) {
                const QString sPayloadId = xml.attributes().value(QStringLiteral("Id")).toString();
                if (sPayloadId.isEmpty() || (sPayloadId.size() > 32767) || sPayloadId.contains(QChar(0))) return false;
                setPayloadRefs.insert(sPayloadId);
                if (!mapPayloadPackages.contains(sPayloadId)) mapPayloadPackages.insert(sPayloadId, currentPackage);
            }

            listElements.append(sName);
        } else if (xml.isEndElement()) {
            const QString sName = xml.name().toString();
            if (listElements.isEmpty() || (listElements.constLast() != sName) || (xml.namespaceUri().toString() != sManifestNamespace)) {
                return false;
            }
            if ((listElements.size() == 3) && (sName == sCurrentPackageElement)) {
                currentPackage = BURN_PACKAGE_INFO();
                sCurrentPackageElement.clear();
            }
            listElements.removeLast();
        }
    }

    if (xml.hasError() || !listElements.isEmpty() || !bRootSeen || !bUXSeen || !bChainSeen) return false;
    for (const QString &sRef : setPayloadRefs) {
        if (!setPayloadIds.contains(sRef)) return false;
    }

    pInfo->bIsV4 = bIsV4;
    for (QMap<QString, BURN_MANIFEST_CONTAINER>::const_iterator it = mapContainers.constBegin(); it != mapContainers.constEnd(); ++it) {
        // Detached containers are declared by the manifest but are not bytes in
        // this bundle.  Accepting them would make a successful open silently
        // omit part of the bundle, so the embedded-only reader fails closed.
        if (!it.value().bAttached) return false;
        if (pInfo->mapAttachedContainers.contains(it.value().nAttachedIndex)) return false;
        pInfo->mapAttachedContainers.insert(it.value().nAttachedIndex, it.value());
    }

    for (BURN_PENDING_PAYLOAD pending : listPayloads) {
        XBurn::PAYLOAD_RECORD &payload = pending.payload;
        const BURN_PACKAGE_INFO package = mapPayloadPackages.value(payload.sPayloadId);
        payload.sPackageId = package.sId;
        payload.sPackageType = package.sType;

        if (pending.bUX) {
            qint32 nEmbeddedIndex = -1;
            if ((payload.containerType != XBurn::CONTAINER_TYPE_UX) || !burnEmbeddedNameIndex(payload.sSourcePath, QChar('u'), &nEmbeddedIndex)) return false;
            Q_UNUSED(nEmbeddedIndex)
            payload.nContainerIndex = 0;
            pInfo->listUXPayloads.append(payload);
        } else if (payload.containerType == XBurn::CONTAINER_TYPE_UNKNOWN) {
            if (!pending.bEmbedded) {
                if (!pending.sContainerId.isEmpty()) return false;
                return false;  // external payload: not physically extractable
            }
            if (pending.sContainerId.isEmpty()) return false;
            if (!mapContainers.contains(pending.sContainerId)) return false;
            const BURN_MANIFEST_CONTAINER container = mapContainers.value(pending.sContainerId);
            if (!container.bAttached) return false;

            qint32 nEmbeddedIndex = -1;
            if (!burnEmbeddedNameIndex(payload.sSourcePath, QChar('a'), &nEmbeddedIndex)) return false;
            Q_UNUSED(nEmbeddedIndex)
            payload.containerType = XBurn::CONTAINER_TYPE_ATTACHED;
            payload.nContainerIndex = container.nAttachedIndex;
            pInfo->mapAttachedPayloads[container.nAttachedIndex].append(payload);
        }
    }

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

static bool burnMapCabinetPayloads(const QList<BURN_CAB_ENTRY> &listEntries, const QList<XBurn::PAYLOAD_RECORD> &listManifestPayloads,
                                   XBurn::CONTAINER_TYPE containerType, qint32 nContainerIndex, QList<XBurn::PAYLOAD_RECORD> *pOutput)
{
    if (!pOutput) return false;
    const qint32 nFirstPhysical = (containerType == XBurn::CONTAINER_TYPE_UX) ? 1 : 0;
    if ((listEntries.size() - nFirstPhysical) != listManifestPayloads.size()) return false;

    QMap<QString, XBurn::PAYLOAD_RECORD> mapPayloads;
    for (const XBurn::PAYLOAD_RECORD &payload : listManifestPayloads) {
        if (mapPayloads.contains(payload.sSourcePath)) return false;
        mapPayloads.insert(payload.sSourcePath, payload);
    }

    for (qint32 i = nFirstPhysical; i < listEntries.size(); i++) {
        const BURN_CAB_ENTRY &entry = listEntries.at(i);
        if (!mapPayloads.contains(entry.sName)) return false;
        XBurn::PAYLOAD_RECORD payload = mapPayloads.take(entry.sName);
        qint32 nEmbeddedIndex = -1;
        const QChar prefix = (containerType == XBurn::CONTAINER_TYPE_UX) ? QChar('u') : QChar('a');
        if (!burnEmbeddedNameIndex(entry.sName, prefix, &nEmbeddedIndex) || ((payload.nFileSize >= 0) && (payload.nFileSize != entry.nSize))) {
            return false;
        }
        Q_UNUSED(nEmbeddedIndex)
        payload.containerType = containerType;
        payload.nContainerIndex = nContainerIndex;
        payload.nInnerIndex = entry.nIndex;
        payload.nFileSize = entry.nSize;
        pOutput->append(payload);
    }

    return mapPayloads.isEmpty();
}

static QString burnAddPublicPathSuffix(const QString &sPath, qint32 nSuffix)
{
    const qint32 nLeafOffset = sPath.lastIndexOf(QChar('/')) + 1;
    qint32 nExtensionOffset = sPath.lastIndexOf(QChar('.'));
    if (nExtensionOffset <= nLeafOffset) nExtensionOffset = sPath.size();
    return sPath.left(nExtensionOffset) + QStringLiteral(".burn-%1").arg(nSuffix) + sPath.mid(nExtensionOffset);
}

static bool burnAssignPublicPaths(QList<XBurn::PAYLOAD_RECORD> *pPayloads)
{
    if (!pPayloads) return false;
    QSet<QString> setPublicPaths;
    for (XBurn::PAYLOAD_RECORD &payload : *pPayloads) {
        QString sCandidate = payload.sFilePath;
        qint32 nSuffix = 2;
        while (setPublicPaths.contains(sCandidate.toCaseFolded())) {
            if (nSuffix > (pPayloads->size() + 1)) return false;
            sCandidate = burnAddPublicPathSuffix(payload.sFilePath, nSuffix++);
        }
        if (sCandidate.isEmpty() || (sCandidate.size() > 32767)) return false;
        payload.sPublicPath = sCandidate;
        setPublicPaths.insert(sCandidate.toCaseFolded());
    }
    return true;
}

static XBurn::CONTAINER_CONTEXT *burnGetContainer(XBurn::UNPACK_CONTEXT *pContext, qint32 nContainerIndex)
{
    if (!pContext || (nContainerIndex < 0) || (nContainerIndex >= pContext->listContainers.size())) return nullptr;
    return pContext->listContainers.at(nContainerIndex);
}

static bool burnPositionContainer(XBurn::CONTAINER_CONTEXT *pContainer, qint32 nTargetIndex, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pContainer || !pContainer->bInitialized || !pContainer->pCab || (nTargetIndex < 0) || (nTargetIndex >= pContainer->state.nNumberOfRecords) ||
        (pContainer->state.nCurrentIndex > nTargetIndex)) {
        return false;
    }
    while (pContainer->state.nCurrentIndex < nTargetIndex) {
        if (!pContainer->pCab->moveToNext(&pContainer->state, pPdStruct)) return false;
    }
    return pContainer->state.nCurrentIndex == nTargetIndex;
}

static bool burnFinishContainer(XBurn::CONTAINER_CONTEXT *pContainer)
{
    if (!pContainer) return true;
    bool bResult = true;
    if (pContainer->pCab) {
        if (pContainer->bInitialized) bResult = pContainer->pCab->finishUnpack(&pContainer->state, nullptr);
        delete pContainer->pCab;
        pContainer->pCab = nullptr;
    }
    if (pContainer->pSubDevice) {
        pContainer->pSubDevice->close();
        delete pContainer->pSubDevice;
        pContainer->pSubDevice = nullptr;
    }
    pContainer->bInitialized = false;
    pContainer->state = XBinary::UNPACK_STATE();
    return bResult;
}

static void burnInitializeContainerContext(XBurn::CONTAINER_CONTEXT *pContainer, qint64 nOffset, qint64 nSize)
{
    if (!pContainer) return;
    pContainer->pSubDevice = nullptr;
    pContainer->pCab = nullptr;
    pContainer->state = XBinary::UNPACK_STATE();
    pContainer->bInitialized = false;
    pContainer->nOuterOffset = nOffset;
    pContainer->nOuterSize = nSize;
}
}  // namespace

XBurn::UNPACK_LIFETIME_STATE::~UNPACK_LIFETIME_STATE()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) XBurn::deleteUnpackContext(pContext);
}

bool XBurn::deleteUnpackContext(UNPACK_CONTEXT *pContext)
{
    if (!pContext) return true;
    bool bResult = true;
    for (qint32 i = pContext->listContainers.size() - 1; i >= 0; --i) {
        CONTAINER_CONTEXT *pContainer = pContext->listContainers.at(i);
        if (!burnFinishContainer(pContainer)) bResult = false;
        delete pContainer;
    }
    pContext->listContainers.clear();
    if (pContext->pSourceValidator) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
    }
    delete pContext;
    return bResult;
}

XBurn::XBurn(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_internalInfo = INTERNAL_INFO();
    m_pUnpackLifetimeState = QSharedPointer<UNPACK_LIFETIME_STATE>::create();
    setIsArchive(true);
}

XBurn::~XBurn()
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    if (pLifetime) pLifetime->bOwnerAlive = false;
    m_pUnpackLifetimeState.clear();
}

bool XBurn::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(getInternalInfo(pPdStruct));
    return pInfo && pInfo->bIsValid;
}

bool XBurn::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBurn burn(pDevice);
    return burn.isValid(pPdStruct);
}

XBurn::INTERNAL_INFO XBurn::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

bool XBurn::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bAlreadyHandled = isInternalInfoHandled();
    if (!bAlreadyHandled) {
        const quint64 nTransaction = beginInternalInfoTransaction();
        if (!nTransaction) return false;

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

void *XBurn::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;
    return &m_internalInfo;
}

void XBurn::setInternalInfo(void *pInternalInfo)
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

XBinary::FT XBurn::getFileType()
{
    XPE pe(getDevice(), isImage(), getModuleAddress());
    return pe.is64() ? FT_PE64_WIXBURN : FT_PE32_WIXBURN;
}

XBurn::INTERNAL_INFO XBurn::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    if (isImage() || !getDevice() || !XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    XPE pe(getDevice(), false, getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;
    const qint64 nTotalSize = getSize();
    if (nTotalSize < BURN_SECTION_MIN_SIZE) return result;

    QList<XPE_DEF::IMAGE_SECTION_HEADER> listSectionHeaders = pe.getSectionHeaders(pPdStruct);
    QList<XPE::SECTION_RECORD> listSections = pe.getSectionRecords(&listSectionHeaders, pPdStruct);
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    XPE::SECTION_RECORD burnSection = {};
    burnSection.nOffset = -1;
    qint32 nBurnSections = 0;
    for (const XPE::SECTION_RECORD &section : listSections) {
        if (section.sName == QStringLiteral(".wixburn")) {
            burnSection = section;
            nBurnSections++;
        }
    }
    if ((nBurnSections != 1) || (burnSection.nSize < BURN_SECTION_MIN_SIZE) || !burnRangeIsValid(burnSection.nOffset, burnSection.nSize, nTotalSize)) {
        return result;
    }

    QByteArray baHeader = read_array_process(burnSection.nOffset, BURN_SECTION_MIN_SIZE, pPdStruct);
    if ((baHeader.size() != BURN_SECTION_MIN_SIZE) || (burnReadLE32(baHeader, 0) != BURN_SECTION_MAGIC) || (burnReadLE32(baHeader, 4) != BURN_SECTION_VERSION) ||
        (burnReadLE32(baHeader, 40) != BURN_CONTAINER_FORMAT_CAB)) {
        return result;
    }

    const quint32 nContainerCount = burnReadLE32(baHeader, 44);
    if ((nContainerCount < 1) || (nContainerCount > (quint32)BURN_MAX_CONTAINERS) || (nContainerCount > (quint32)((burnSection.nSize - BURN_SECTION_FIXED_SIZE) / 4))) {
        return result;
    }
    const qint64 nHeaderSize = BURN_SECTION_FIXED_SIZE + ((qint64)nContainerCount * 4);
    if ((nHeaderSize > baHeader.size()) && ((nHeaderSize > (std::numeric_limits<qint32>::max)()) ||
                                            ((baHeader = read_array_process(burnSection.nOffset, (qint32)nHeaderSize, pPdStruct)).size() != nHeaderSize))) {
        return result;
    }

    const QByteArray baBundleId = baHeader.mid(8, 16);
    const qint64 nStubSize = burnReadLE32(baHeader, 24);
    const qint64 nOriginalSignatureOffset = burnReadLE32(baHeader, 32);
    const qint64 nOriginalSignatureSize = burnReadLE32(baHeader, 36);
    QList<qint64> listContainerSizes;
    for (quint32 i = 0; i < nContainerCount; ++i) {
        listContainerSizes.append((qint64)burnReadLE32(baHeader, BURN_SECTION_FIXED_SIZE + ((qint32)i * 4)));
    }
    const qint64 nUXSize = listContainerSizes.constFirst();

    if (burnIsAllZero(baBundleId) || (nStubSize <= 0) || (nUXSize < (qint64)sizeof(XCab::CFHEADER)) || !burnRangeIsValid(0, nStubSize, nTotalSize) ||
        !burnRangeIsValid(nStubSize, nUXSize, nTotalSize) || ((burnSection.nOffset + burnSection.nSize) > nStubSize) ||
        ((nOriginalSignatureOffset == 0) != (nOriginalSignatureSize == 0))) {
        return result;
    }

    const qint64 nUXEnd = nStubSize + nUXSize;
    XBinary::OFFSETSIZE signature = pe.getSignOffsetSize();
    if ((signature.nOffset < 0) || (signature.nSize < 0) || ((signature.nSize > 0) && !burnRangeIsValid(signature.nOffset, signature.nSize, nTotalSize))) {
        return result;
    }

    qint64 nEngineEnd = nUXEnd;
    if (nOriginalSignatureOffset > 0) {
        if ((nOriginalSignatureOffset < nUXEnd) || !burnRangeIsValid(nOriginalSignatureOffset, nOriginalSignatureSize, nTotalSize)) return result;
        nEngineEnd = nOriginalSignatureOffset + nOriginalSignatureSize;
    } else if ((signature.nSize > 0) && (nContainerCount < 2)) {
        if (signature.nOffset < nUXEnd) return result;
        nEngineEnd = signature.nOffset + signature.nSize;
    }

    CONTAINER_RECORD uxContainer = {};
    uxContainer.containerType = CONTAINER_TYPE_UX;
    uxContainer.nContainerIndex = 0;
    uxContainer.nOffset = nStubSize;
    uxContainer.nSize = nUXSize;
    result.listContainers.append(uxContainer);

    qint64 nAttachedCursor = nEngineEnd;
    for (quint32 i = 1; i < nContainerCount; ++i) {
        const qint64 nContainerSize = listContainerSizes.at((qint32)i);
        if ((nContainerSize < (qint64)sizeof(XCab::CFHEADER)) || !burnRangeIsValid(nAttachedCursor, nContainerSize, nTotalSize)) {
            result.listContainers.clear();
            return result;
        }
        CONTAINER_RECORD container = {};
        container.containerType = CONTAINER_TYPE_ATTACHED;
        container.nContainerIndex = (qint32)i;
        container.nOffset = nAttachedCursor;
        container.nSize = nContainerSize;
        result.listContainers.append(container);
        nAttachedCursor += nContainerSize;
    }
    const qint64 nPhysicalEnd = (nContainerCount > 1) ? nAttachedCursor : nEngineEnd;

    // A valid writer leaves only the current Authenticode certificate after
    // the UX/attached contents.  For a one-container signed bundle EngineEnd
    // itself includes that certificate, so compare against the actual content
    // boundary rather than EngineEnd.
    const qint64 nContentEnd = (nContainerCount > 1) ? nPhysicalEnd : nUXEnd;
    if (signature.nSize > 0) {
        if ((signature.nOffset < nContentEnd) || ((signature.nOffset + signature.nSize) != nTotalSize)) return result;
    } else if (nPhysicalEnd != nTotalSize) {
        return result;
    }

    QList<QList<BURN_CAB_ENTRY> > listContainerEntries;
    QByteArray baManifest;
    for (qint32 i = 0; i < result.listContainers.size(); ++i) {
        const CONTAINER_RECORD &container = result.listContainers.at(i);
        QList<BURN_CAB_ENTRY> listEntries;
        if (!burnScanCabinet(getDevice(), container.nOffset, container.nSize, i == 0, &listEntries, (i == 0) ? &baManifest : nullptr, pPdStruct)) {
            result.listContainers.clear();
            return result;
        }
        listContainerEntries.append(listEntries);
    }

    BURN_MANIFEST_INFO manifestInfo = {};
    if (!burnParseManifest(baManifest, &manifestInfo, pPdStruct) || (manifestInfo.mapAttachedContainers.size() != ((qint32)nContainerCount - 1))) {
        result.listContainers.clear();
        return result;
    }
    for (qint32 i = 1; i < (qint32)nContainerCount; ++i) {
        if (!manifestInfo.mapAttachedContainers.contains(i)) {
            result.listContainers.clear();
            return result;
        }
        const BURN_MANIFEST_CONTAINER manifestContainer = manifestInfo.mapAttachedContainers.value(i);
        CONTAINER_RECORD &physicalContainer = result.listContainers[i];
        QCryptographicHash::Algorithm algorithm;
        if ((manifestContainer.nFileSize != physicalContainer.nSize) || !burnHashAlgorithm(manifestContainer.baHash, &algorithm)) {
            result.listContainers.clear();
            return result;
        }
        QByteArray baActualHash;
        if (!burnHashRange(getDevice(), physicalContainer.nOffset, physicalContainer.nSize, algorithm, &baActualHash, pPdStruct) ||
            (baActualHash != manifestContainer.baHash)) {
            result.listContainers.clear();
            return result;
        }
        physicalContainer.sId = manifestContainer.sId;
        physicalContainer.baHash = manifestContainer.baHash;
    }

    QList<PAYLOAD_RECORD> listPublicPayloads;
    PAYLOAD_RECORD manifestPayload = {};
    manifestPayload.containerType = CONTAINER_TYPE_UX;
    manifestPayload.nContainerIndex = 0;
    manifestPayload.nInnerIndex = listContainerEntries.constFirst().constFirst().nIndex;
    manifestPayload.sSourcePath = QStringLiteral("0");
    manifestPayload.sFilePath = QStringLiteral("manifest.xml");
    manifestPayload.sPayloadId = QStringLiteral("BurnManifest");
    manifestPayload.nFileSize = baManifest.size();
    manifestPayload.baHash = QCryptographicHash::hash(baManifest, manifestInfo.bIsV4 ? QCryptographicHash::Sha512 : QCryptographicHash::Sha1);
    listPublicPayloads.append(manifestPayload);

    if (!burnMapCabinetPayloads(listContainerEntries.at(0), manifestInfo.listUXPayloads, CONTAINER_TYPE_UX, 0, &listPublicPayloads)) {
        result.listContainers.clear();
        return result;
    }
    for (qint32 i = 1; i < (qint32)nContainerCount; ++i) {
        if (!burnMapCabinetPayloads(listContainerEntries.at(i), manifestInfo.mapAttachedPayloads.value(i), CONTAINER_TYPE_ATTACHED, i, &listPublicPayloads)) {
            result.listContainers.clear();
            return result;
        }
    }
    if (!burnAssignPublicPaths(&listPublicPayloads)) {
        result.listContainers.clear();
        return result;
    }

    result.bIsValid = true;
    result.baBundleId = baBundleId;
    result.listPayloads = listPublicPayloads;
    result.sVersion = pe.getResourcesVersionValue(QStringLiteral("FileVersion")).trimmed();
    if (result.sVersion.isEmpty()) result.sVersion = pe.getFileVersion().trimmed();
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) result.bIsValid = false;
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XBurn::getDefaultUnpackProperties()
{
    return XBinary::getDefaultUnpackProperties();
}

namespace {
static bool burnOpenContainer(XBurn::CONTAINER_CONTEXT *pContainer, QIODevice *pSource, const QMap<XBinary::UNPACK_PROP, QVariant> &mapProperties,
                              XBinary::PDSTRUCT *pPdStruct)
{
    if (!pContainer || !pSource || (pContainer->nOuterOffset < 0) || (pContainer->nOuterSize < (qint64)sizeof(XCab::CFHEADER))) return false;

    pContainer->pSubDevice = new SubDevice(pSource, pContainer->nOuterOffset, pContainer->nOuterSize);
    if (!pContainer->pSubDevice->open(QIODevice::ReadOnly)) {
        delete pContainer->pSubDevice;
        pContainer->pSubDevice = nullptr;
        return false;
    }

    pContainer->pCab = new XCab(pContainer->pSubDevice);
    if (!pContainer->pCab->initUnpack(&pContainer->state, mapProperties, pPdStruct)) {
        // XCab may allocate decoder state before reporting an initialization
        // failure.  Give it the matching cleanup call before destroying it.
        pContainer->pCab->finishUnpack(&pContainer->state, nullptr);
        delete pContainer->pCab;
        pContainer->pCab = nullptr;
        pContainer->pSubDevice->close();
        delete pContainer->pSubDevice;
        pContainer->pSubDevice = nullptr;
        pContainer->state = XBinary::UNPACK_STATE();
        return false;
    }
    pContainer->bInitialized = true;
    return true;
}

static bool burnResolveActiveRecord(XBurn *pThis, const QSharedPointer<XBurn::UNPACK_LIFETIME_STATE> &pLifetime, XBinary::UNPACK_STATE *pState,
                                    XBurn::UNPACK_CONTEXT **ppContext, XBurn::CONTAINER_CONTEXT **ppContainer, const XBurn::PAYLOAD_RECORD **ppPayload)
{
    if (!pThis || !pLifetime || !pState || !ppContext || !ppContainer || !ppPayload || !pState->pContext || !pState->baUnpackSourceToken.isEmpty() ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    XBurn::UNPACK_CONTEXT *pContext = static_cast<XBurn::UNPACK_CONTEXT *>(pState->pContext);
    if (!pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != pThis->getDevice()) || (pContext->nOwnerDeviceGeneration != pThis->getDeviceGeneration()) || !pContext->pSourceValidator ||
        (pContext->nCurrentPayload != pState->nCurrentIndex) || (pContext->listPayloads.size() != pState->nNumberOfRecords)) {
        return false;
    }

    const XBurn::PAYLOAD_RECORD *pPayload = &pContext->listPayloads.at(pContext->nCurrentPayload);
    XBurn::CONTAINER_CONTEXT *pContainer = burnGetContainer(pContext, pPayload->nContainerIndex);
    if (!pContainer || !pContainer->bInitialized || !pContainer->pCab || (pContainer->state.nCurrentIndex != pPayload->nInnerIndex) ||
        (pContainer->state.nCurrentOffset != pState->nCurrentOffset)) {
        return false;
    }

    *ppContext = pContext;
    *ppContainer = pContainer;
    *ppPayload = pPayload;
    return true;
}
}  // namespace

bool XBurn::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    BurnOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    if (pState->pContext) {
        UNPACK_CONTEXT *pOldContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pLifetime->setContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        pLifetime->setContexts.remove(pOldContext);
        pState->pContext = nullptr;
        const bool bClean = deleteUnpackContext(pOldContext);
        *pState = UNPACK_STATE();
        if (!pLifetime->bOwnerAlive || !bClean) return false;
    }
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    *pState = UNPACK_STATE();
    pState->mapUnpackProperties = mapProperties;

    QIODevice *guardedSource = getDevice();
    XArchive *pSourceValidator = new XArchive(guardedSource);
    UNPACK_STATE sourceValidationState = {};
    if (!pSourceValidator->bindUnpackSource(&sourceValidationState, pPdStruct)) {
        pSourceValidator->releaseUnpackSource(&sourceValidationState);
        delete pSourceValidator;
        return false;
    }

    XBurn detector(guardedSource, isImage(), getModuleAddress());
    const qint64 nTotalSize = guardedSource->size();
    const INTERNAL_INFO info = detector._detect(pPdStruct);
    if ((nTotalSize < 0) || !info.bIsValid || !pSourceValidator->isUnpackSourceCurrent(&sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive) {
        pSourceValidator->releaseUnpackSource(&sourceValidationState);
        delete pSourceValidator;
        return false;
    }

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pOwnerState = nullptr;
    for (const CONTAINER_RECORD &record : info.listContainers) {
        CONTAINER_CONTEXT *pContainer = new CONTAINER_CONTEXT;
        burnInitializeContainerContext(pContainer, record.nOffset, record.nSize);
        pContext->listContainers.append(pContainer);
    }
    pContext->listPayloads = info.listPayloads;
    pContext->nCurrentPayload = 0;
    pContext->pSourceValidator = pSourceValidator;
    pContext->sourceValidationState = UNPACK_STATE();

    if (!pContext->pSourceValidator->transferUnpackSourceOwnership(&sourceValidationState, &pContext->sourceValidationState)) {
        pContext->pSourceValidator->releaseUnpackSource(&sourceValidationState);
        deleteUnpackContext(pContext);
        return false;
    }

    bool bOpen = (pContext->listContainers.size() == info.listContainers.size()) && !pContext->listContainers.isEmpty();
    for (qint32 i = 0; bOpen && (i < pContext->listContainers.size()); ++i) {
        bOpen = burnOpenContainer(pContext->listContainers.at(i), guardedSource, mapProperties, pPdStruct);
    }
    if (!bOpen || !pLifetime->bOwnerAlive || (pContext->listContainers.constFirst()->state.nNumberOfRecords < 1)) {
        deleteUnpackContext(pContext);
        return false;
    }

    if (!pContext->listPayloads.isEmpty()) {
        const PAYLOAD_RECORD &firstPayload = pContext->listPayloads.constFirst();
        CONTAINER_CONTEXT *pFirstContainer = burnGetContainer(pContext, firstPayload.nContainerIndex);
        if (!burnPositionContainer(pFirstContainer, firstPayload.nInnerIndex, pPdStruct) || !pLifetime->bOwnerAlive) {
            deleteUnpackContext(pContext);
            return false;
        }
    }

    if (!pContext->pSourceValidator->validateAndFinalizeUnpackSource(&pContext->sourceValidationState, pPdStruct) ||
        !pLifetime->bOwnerAlive) {
        deleteUnpackContext(pContext);
        return false;
    }

    pState->nTotalSize = nTotalSize;
    pState->nNumberOfRecords = pContext->listPayloads.size();
    pState->nCurrentIndex = 0;
    if (!pContext->listPayloads.isEmpty()) {
        CONTAINER_CONTEXT *pFirstContainer = burnGetContainer(pContext, pContext->listPayloads.constFirst().nContainerIndex);
        if (!pFirstContainer) {
            deleteUnpackContext(pContext);
            return false;
        }
        pState->nCurrentOffset = pFirstContainer->state.nCurrentOffset;
        pState->mapArchiveProperties = pFirstContainer->state.mapArchiveProperties;
    } else {
        pState->nCurrentOffset = 0;
        pState->mapArchiveProperties = pContext->listContainers.constFirst()->state.mapArchiveProperties;
    }
    pContext->pOwnerState = pState;
    pState->pContext = pContext;
    pLifetime->setContexts.insert(pContext);
    return true;
}

XBinary::ARCHIVERECORD XBurn::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    BurnOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired() || !XBinary::isPdStructNotCanceled(pPdStruct)) return result;
    UNPACK_CONTEXT *pContext = nullptr;
    CONTAINER_CONTEXT *pContainer = nullptr;
    const PAYLOAD_RECORD *pPayload = nullptr;
    if (!burnResolveActiveRecord(this, pLifetime, pState, &pContext, &pContainer, &pPayload) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return result;
    }

    result = pContainer->pCab->infoCurrent(&pContainer->state, pPdStruct);
    if (!pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return ARCHIVERECORD();
    }

    bool bInnerSizeOK = false;
    const qint64 nInnerSize = result.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE).toLongLong(&bInnerSizeOK);
    if (!bInnerSizeOK || (nInnerSize != pPayload->nFileSize) || (result.mapProperties.value(FPART_PROP_ORIGINALNAME).toString() != pPayload->sSourcePath)) {
        return ARCHIVERECORD();
    }

    if (result.nStreamSize > 0) {
        if (!burnRangeIsValid(result.nStreamOffset, result.nStreamSize, pContainer->nOuterSize) ||
            (result.nStreamOffset > ((std::numeric_limits<qint64>::max)() - pContainer->nOuterOffset))) {
            return ARCHIVERECORD();
        }
        result.nStreamOffset += pContainer->nOuterOffset;
        result.mapProperties.insert(FPART_PROP_STREAMOFFSET, result.nStreamOffset);
        result.mapProperties.insert(FPART_PROP_STREAMSIZE, result.nStreamSize);
    }

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pPayload->sPublicPath);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pPayload->nFileSize);
    result.mapProperties.insert(FPART_PROP_RESOURCEID, pPayload->sPayloadId);
    QString sInfo = (pPayload->containerType == CONTAINER_TYPE_UX) ? QStringLiteral("WiX Burn UX payload") : QStringLiteral("WiX Burn attached payload");
    if (!pPayload->sPackageType.isEmpty()) {
        sInfo = QStringLiteral("WiX Burn %1 package").arg(pPayload->sPackageType);
        if (!pPayload->sPackageId.isEmpty()) sInfo += QStringLiteral(" (%1)").arg(pPayload->sPackageId);
    }
    if (pPayload->sPublicPath != pPayload->sFilePath) {
        sInfo += QStringLiteral("; manifest path: %1").arg(pPayload->sFilePath);
    }
    result.mapProperties.insert(FPART_PROP_INFO, sInfo);
    return result;
}

bool XBurn::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    BurnOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    QIODevice *guardedOutput = pDevice;
    if (!guardedOutput || !guardedOutput->isOpen() || !guardedOutput->isWritable() || guardedOutput->isSequential() ||
        (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) || !XBinary::isResizeEnable(guardedOutput) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_CONTEXT *pContext = nullptr;
    CONTAINER_CONTEXT *pContainer = nullptr;
    const PAYLOAD_RECORD *pPayload = nullptr;
    if (!burnResolveActiveRecord(this, pLifetime, pState, &pContext, &pContainer, &pPayload) ||
        XBinary::devicesAlias(pContext->pOuterSourceDevice, guardedOutput) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return false;
    }

    QTemporaryFile stage;
    if (!stage.open()) return false;
    // XFU-015: the inner unpackCurrent performs its own entry accounting and
    // debits its production into the stage; the publish copy is not charged.
    pContainer->state.spOutputBudget = pState->spOutputBudget;
    if (!pContainer->pCab->unpackCurrent(&pContainer->state, &stage, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return false;
    }

    QByteArray baActualHash;
    QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Sha1;
    const bool bHashOK = pPayload->baHash.isEmpty() || (burnHashAlgorithm(pPayload->baHash, &hashAlgorithm) &&
                                                        burnHashDevice(&stage, hashAlgorithm, &baActualHash, pPdStruct) && (baActualHash == pPayload->baHash));
    if ((stage.size() != pPayload->nFileSize) || !bHashOK || !pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) ||
        (pState->pContext != pContext)) {
        return false;
    }

    BurnPublisher publisher(pContext->pOuterSourceDevice);
    UNPACK_STATE publicationState = {};
    if (!publisher.bindUnpackSource(&publicationState, pPdStruct) || !publisher.validateAndFinalizeUnpackSource(&publicationState, pPdStruct) || !pLifetime->bOwnerAlive || !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !publisher.publishUnpackOutput(&stage, guardedOutput, &publicationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return false;
    }

    pState->nCurrentOffset = pContainer->state.nCurrentOffset;
    pState->mapArchiveProperties = pContainer->state.mapArchiveProperties;
    return true;
}

bool XBurn::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    BurnOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired() || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    UNPACK_CONTEXT *pContext = nullptr;
    CONTAINER_CONTEXT *pCurrentContainer = nullptr;
    const PAYLOAD_RECORD *pCurrentPayload = nullptr;
    if (!burnResolveActiveRecord(this, pLifetime, pState, &pContext, &pCurrentContainer, &pCurrentPayload) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return false;
    }

    const qint32 nNextPayload = pContext->nCurrentPayload + 1;
    if (nNextPayload >= pContext->listPayloads.size()) {
        // Match the streaming API convention used by the other static unpackers:
        // the final move reports false, but advances the state to the end sentinel.
        pContext->nCurrentPayload = nNextPayload;
        pState->nCurrentIndex = nNextPayload;
        pState->nCurrentOffset = 0;
        return false;
    }
    const PAYLOAD_RECORD &nextPayload = pContext->listPayloads.at(nNextPayload);
    CONTAINER_CONTEXT *pNextContainer = burnGetContainer(pContext, nextPayload.nContainerIndex);
    if (!burnPositionContainer(pNextContainer, nextPayload.nInnerIndex, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext) ||
        !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) || !pLifetime->bOwnerAlive ||
        !pLifetime->setContexts.contains(pContext) || (pState->pContext != pContext)) {
        return false;
    }

    pContext->nCurrentPayload = nNextPayload;
    pState->nCurrentIndex = nNextPayload;
    pState->nCurrentOffset = pNextContainer->state.nCurrentOffset;
    pState->mapArchiveProperties = pNextContainer->state.mapArchiveProperties;
    return true;
}

bool XBurn::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<UNPACK_LIFETIME_STATE> pLifetime = m_pUnpackLifetimeState;
    BurnOperationGuard operationGuard(pLifetime);
    if (!operationGuard.isAcquired()) return false;
    bool bResult = true;
    if (pState->pContext) {
        UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
        if (!pLifetime->setContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        pLifetime->setContexts.remove(pContext);
        pState->pContext = nullptr;
        bResult = deleteUnpackContext(pContext);
        if (!pLifetime->bOwnerAlive) return false;
    }
    *pState = UNPACK_STATE();
    return bResult;
}
