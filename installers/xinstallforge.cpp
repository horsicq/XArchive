/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xinstallforge.h"

#include <QDir>
#include <QScopedValueRollback>
#include <QSet>

#include "xpe.h"
#include "subdevice.h"
#include "xbzip2.h"
#include "xsevenzip.h"
#include "xtar_bzip2.h"
#include "xtar_gz.h"

namespace {

class INSTALLFORGE_OPERATION_STATE_DELETER {
public:
    explicit INSTALLFORGE_OPERATION_STATE_DELETER(const QSharedPointer<XInstallForge::UNPACK_DEFERRED_CLEANUP> &pCleanup) : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XInstallForge::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

struct InstallForgeCRCProgressBridge {
    XBinary::PDSTRUCT *pOriginal;
    XBinary::PDSTRUCTLIFETIME originalLifetime;
};

void installForgeCRCProgressCallback(void *pUserData, XBinary::PDSTRUCT *pLocalProgress)
{
    InstallForgeCRCProgressBridge *pBridge = static_cast<InstallForgeCRCProgressBridge *>(pUserData);
    if (!pBridge || !pLocalProgress) return;

    if (!XBinary::isPdStructLifetimeAlive(pBridge->originalLifetime) || !XBinary::isPdStructNotCanceled(pBridge->pOriginal)) {
        XBinary::setPdStructStopped(pLocalProgress);
    }
}

class XInstallForgeTarBzip2 : public XTAR_BZIP2 {
public:
    explicit XInstallForgeTarBzip2(QIODevice *pDevice) : XTAR_BZIP2(pDevice)
    {
    }

protected:
    bool getOuterStreamInfo(qint64 &nOuterStreamOffset, qint64 &nOuterStreamSize, HANDLE_METHOD &handleMethod) override
    {
        nOuterStreamOffset = 0;
        nOuterStreamSize = getSize();
        handleMethod = HANDLE_METHOD_BZIP2;
        return nOuterStreamSize > 0;
    }
};

class XInstallForgeTarGzip : public XTAR_GZ {
public:
    explicit XInstallForgeTarGzip(QIODevice *pDevice) : XTAR_GZ(pDevice)
    {
    }

    bool initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct = nullptr) override
    {
        const XBinary::PDSTRUCTLIFETIME progressLifetime = XBinary::retainPdStructLifetime(pPdStruct);
        if (!XTAR_GZ::initUnpack(pState, mapProperties, pPdStruct) || (pPdStruct && !XBinary::isPdStructLifetimeAlive(progressLifetime))) return false;

        const qint64 nCompressedSize = getSize();
        if ((pPdStruct && !XBinary::isPdStructLifetimeAlive(progressLifetime))) return false;
        QIODevice *guardedDecompressed = m_pDecompressedData;
        const qint64 nDecompressedSize = guardedDecompressed ? guardedDecompressed->size() : -1;
        if (!guardedDecompressed || (pPdStruct && !XBinary::isPdStructLifetimeAlive(progressLifetime))) return false;
        bool bIntegrityValid = (nCompressedSize >= 18) && (nDecompressedSize >= 0);
        if (bIntegrityValid) {
            const quint32 nStoredSize = read_uint32(nCompressedSize - 4);
            if (!guardedDecompressed || (pPdStruct && !XBinary::isPdStructLifetimeAlive(progressLifetime))) return false;
            bIntegrityValid = ((quint64)nDecompressedSize == (quint64)nStoredSize) && XBinary::isPdStructNotCanceled(pPdStruct);
        }

        if (bIntegrityValid) {
            const quint32 nStoredCRC32 = read_uint32(nCompressedSize - 8);
            if (!guardedDecompressed || (pPdStruct && !XBinary::isPdStructLifetimeAlive(progressLifetime))) return false;

            // The footer check must not invoke the caller callback after the
            // base archive has published state owned by pState.  A private
            // progress object keeps cancellation responsive and cleanup safe.
            XBinary::PDSTRUCT crcProgress = XBinary::createPdStruct();
            InstallForgeCRCProgressBridge progressBridge = {pPdStruct, progressLifetime};
            if (pPdStruct) {
                const XBinary::PDSTRUCT progressSnapshot = XBinary::getPdStructSnapshot(pPdStruct);
                crcProgress.nBufferSize.storeRelease(progressSnapshot.nBufferSize.loadAcquire());
                crcProgress.nFileBufferSize.storeRelease(progressSnapshot.nFileBufferSize.loadAcquire());
                XBinary::setPdStructCallback(&crcProgress, installForgeCRCProgressCallback, &progressBridge);
            }
            bIntegrityValid = XBinary::checkCRC(guardedDecompressed, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF, nStoredCRC32, &crcProgress) &&
                              guardedDecompressed && (!pPdStruct || XBinary::isPdStructLifetimeAlive(progressLifetime)) && XBinary::isPdStructNotCanceled(pPdStruct);
        }

        if (!bIntegrityValid) {
            XTAR_GZ::finishUnpack(pState, nullptr);
            return false;
        }

        return true;
    }
};

bool decodeInstallForgeName(const QString &sEncodedName, QString *pResult)
{
    if (!pResult || sEncodedName.isEmpty() || (sEncodedName.size() > 0x20000)) return false;

    const QByteArray baEncoded = sEncodedName.toLatin1();
    const QByteArray baDecoded = QByteArray::fromBase64(baEncoded);

    // InstallForge stores entry names as canonical base64(UTF-16LE(name)).
    if (baDecoded.isEmpty() || (baDecoded.size() > 0x10000) || ((baDecoded.size() & 1) != 0) || (baDecoded.toBase64() != baEncoded)) return false;

    QString sResult;
    sResult.reserve(baDecoded.size() / 2);

    for (qint32 i = 0; i < baDecoded.size(); i += 2) {
        const quint16 nCodeUnit = static_cast<quint8>(baDecoded.at(i)) | (static_cast<quint16>(static_cast<quint8>(baDecoded.at(i + 1))) << 8);
        if (nCodeUnit == 0) {
            if (i != (baDecoded.size() - 2)) return false;
            break;
        }

        if ((nCodeUnit >= 0xD800) && (nCodeUnit <= 0xDBFF)) {
            if (i + 3 >= baDecoded.size()) return false;
            const quint16 nLow = static_cast<quint8>(baDecoded.at(i + 2)) | (static_cast<quint16>(static_cast<quint8>(baDecoded.at(i + 3))) << 8);
            if ((nLow < 0xDC00) || (nLow > 0xDFFF)) return false;
            sResult.append(QChar(nCodeUnit));
            sResult.append(QChar(nLow));
            i += 2;
            continue;
        }
        if ((nCodeUnit >= 0xDC00) && (nCodeUnit <= 0xDFFF)) return false;
        sResult.append(QChar(nCodeUnit));
    }

    if (sResult.isEmpty()) return false;

    sResult.replace('\\', '/');
    sResult = sResult.normalized(QString::NormalizationForm_C);
    if (QDir::isAbsolutePath(sResult) || sResult.startsWith('/') || sResult.contains(':')) return false;

    static const QString sForbidden = QStringLiteral("<>:\"\\|?*");
    static const QSet<QString> setReserved = {QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),  QStringLiteral("NUL"),  QStringLiteral("COM1"),
                                              QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"), QStringLiteral("COM5"), QStringLiteral("COM6"),
                                              QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"), QStringLiteral("LPT1"), QStringLiteral("LPT2"),
                                              QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"), QStringLiteral("LPT6"), QStringLiteral("LPT7"),
                                              QStringLiteral("LPT8"), QStringLiteral("LPT9")};
    const QStringList listParts = sResult.split('/');
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == ".") || (sPart == "..") || (sPart.size() > 255) || sPart.endsWith(' ') || sPart.endsWith('.')) {
            return false;
        }
        for (QChar character : sPart) {
            const ushort nCodeUnit = character.unicode();
            const bool bSurrogate = (nCodeUnit >= 0xD800) && (nCodeUnit <= 0xDFFF);
            if ((!bSurrogate && !character.isPrint()) || (character == QChar::ReplacementCharacter) || sForbidden.contains(character)) return false;
        }
        QString sDeviceName = sPart.section('.', 0, 0).toUpper();
        bool bReservedDigit =
            (sDeviceName.size() == 4) && (((sDeviceName.at(3) >= QChar('1')) && (sDeviceName.at(3) <= QChar('9'))) || (sDeviceName.at(3) == QChar(0x00B9)) ||
                                          (sDeviceName.at(3) == QChar(0x00B2)) || (sDeviceName.at(3) == QChar(0x00B3)));
        bool bNumberedDevice = (sDeviceName.startsWith("COM") || sDeviceName.startsWith("LPT")) && bReservedDigit;
        if (setReserved.contains(sDeviceName) || (sDeviceName == "CLOCK$") || (sDeviceName == "CONIN$") || (sDeviceName == "CONOUT$") || bNumberedDevice) {
            return false;
        }
    }

    *pResult = sResult;
    return true;
}

}  // namespace

XInstallForge::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
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

XInstallForge::XInstallForge(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_internalInfo = INTERNAL_INFO();
    setIsArchive(true);
}

XInstallForge::~XInstallForge()
{
}

bool XInstallForge::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(getInternalInfo(pPdStruct));
    return pInfo && pInfo->bIsValid;
}

bool XInstallForge::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XInstallForge x(pDevice);
    return x.isValid(pPdStruct);
}

XInstallForge::INTERNAL_INFO XInstallForge::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XInstallForge::handleInternalInfo(PDSTRUCT *pPdStruct)
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

void *XInstallForge::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XInstallForge::setInternalInfo(void *pInternalInfo)
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

XBinary::FT XInstallForge::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_INSTALLFORGE;
    }

    return FT_PE32_INSTALLFORGE;
}

XInstallForge::INTERNAL_INFO XInstallForge::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nArchiveOffset = -1;

    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;

    const qint64 nTotalSize = getSize();
    qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if ((nOverlayOffset <= 0) || (nOverlayOffset >= nTotalSize)) return result;

    // 13-byte "IFSETUP_START" marker (each byte +1) at the overlay start, then an
    // 8-byte LE length, then the file payload. The payload container depends on the
    // chosen compressor: 7z (lzma/store), raw BZip2 ("BZh"), or gzip/deflate (1F 8B).
    static const char kMarker[13] = {0x4A, 0x47, 0x54, 0x46, 0x55, 0x56, 0x51, 0x60, 0x54, 0x55, 0x42, 0x53, 0x55};
    QByteArray baHead = read_array_process(nOverlayOffset, 29, pPdStruct);
    if (baHead.size() < 29) return result;
    if (memcmp(baHead.constData(), kMarker, 13) != 0) return result;

    const quint8 *pLen = (const quint8 *)baHead.constData() + 13;
    quint64 nLen = 0;
    for (int i = 0; i < 8; i++) nLen |= ((quint64)pLen[i]) << (8 * i);

    const quint8 *pPayload = (const quint8 *)baHead.constData() + 21;
    result.payload = PAYLOAD_UNKNOWN;
    if ((pPayload[0] == 0x37) && (pPayload[1] == 0x7A) && (pPayload[2] == 0xBC) && (pPayload[3] == 0xAF) && (pPayload[4] == 0x27) && (pPayload[5] == 0x1C)) {
        result.payload = PAYLOAD_7Z;
    } else if ((pPayload[0] == 0x42) && (pPayload[1] == 0x5A) && (pPayload[2] == 0x68) && (pPayload[3] >= '1') && (pPayload[3] <= '9')) {
        result.payload = PAYLOAD_BZIP2;  // "BZh"
    } else if ((pPayload[0] == 0x1F) && (pPayload[1] == 0x8B) && (pPayload[2] == 8) && ((pPayload[3] & 0xE0) == 0)) {
        result.payload = PAYLOAD_GZIP;
    }
    if (result.payload == PAYLOAD_UNKNOWN) return result;

    qint64 nArcOff = nOverlayOffset + 21;
    quint64 nAvailableSize = (nArcOff <= nTotalSize) ? (quint64)(nTotalSize - nArcOff) : 0;

    quint64 nMinimumSize = (result.payload == PAYLOAD_7Z) ? 32 : ((result.payload == PAYLOAD_GZIP) ? 18 : 14);
    if ((nLen < nMinimumSize) || (nLen > nAvailableSize)) return result;

    // Authenticate the declared container extent. GZIP additionally requires
    // full stream/footer authentication because its lightweight header parser
    // cannot validate CRC32 or ISIZE.
    SubDevice payloadDevice(getDevice(), nArcOff, (qint64)nLen);
    if (!payloadDevice.open(QIODevice::ReadOnly)) return result;
    bool bContainerValid = false;
    if (result.payload == PAYLOAD_7Z) {
        XSevenZip archive(&payloadDevice);
        bContainerValid = archive.isValid(pPdStruct) && (archive.getFileFormatSize(pPdStruct) == (qint64)nLen);
    } else if (result.payload == PAYLOAD_BZIP2) {
        XBZIP2 archive(&payloadDevice);
        bContainerValid = archive.isValid(pPdStruct);
    } else if (result.payload == PAYLOAD_GZIP) {
        XInstallForgeTarGzip archive(&payloadDevice);
        UNPACK_STATE state = {};
        QMap<UNPACK_PROP, QVariant> mapProperties;
        bContainerValid = archive.initUnpack(&state, mapProperties, pPdStruct);
        if (bContainerValid) bContainerValid = archive.finishUnpack(&state, pPdStruct);
    }
    payloadDevice.close();
    if (!bContainerValid || !XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    result.bIsValid = true;
    result.nArchiveOffset = nArcOff;
    result.nArchiveSize = (qint64)nLen;

    // Engine version from the RT_VERSION "Comments" field ("Created with InstallForge X.Y.Z").
    QString sComments = pe.getResourcesVersionValue("Comments").trimmed();
    int nIdx = sComments.indexOf("InstallForge");
    if (nIdx >= 0) {
        QString sTail = sComments.mid(nIdx + 12).trimmed();
        if (!sTail.isEmpty()) result.sVersion = sTail;
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) result.bIsValid = false;

    return result;
}

// --- streaming extraction: delegate to the XArchive 7z handler ---

QMap<XBinary::UNPACK_PROP, QVariant> XInstallForge::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();

    QIODevice *pDevice = getDevice();

    if (pDevice) {
        INTERNAL_INFO info = _detect(nullptr);

        if (info.bIsValid && (info.nArchiveOffset >= 0) && (info.nArchiveSize > 0)) {
            SubDevice subDevice(pDevice, info.nArchiveOffset, info.nArchiveSize);

            if (subDevice.open(QIODevice::ReadOnly)) {
                XArchive *pArchive = nullptr;

                switch (info.payload) {
                    case PAYLOAD_BZIP2: pArchive = new XInstallForgeTarBzip2(&subDevice); break;
                    case PAYLOAD_GZIP: pArchive = new XInstallForgeTarGzip(&subDevice); break;
                    case PAYLOAD_7Z:
                    default: pArchive = new XSevenZip(&subDevice); break;
                }

                QMap<UNPACK_PROP, QVariant> mapInnerProperties = pArchive->getDefaultUnpackProperties();

                if (mapInnerProperties.contains(UNPACK_PROP_PASSWORD)) {
                    result.insert(UNPACK_PROP_PASSWORD, mapInnerProperties.value(UNPACK_PROP_PASSWORD));
                }

                for (QMap<UNPACK_PROP, QVariant>::const_iterator it = mapInnerProperties.constBegin(); it != mapInnerProperties.constEnd(); ++it) {
                    if (XBinary::isUnpackCRCProperty(it.key())) {
                        result.insert(it.key(), it.value());
                    }
                }

                delete pArchive;
                subDevice.close();
            }
        }
    }

    return result;
}

bool XInstallForge::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    XInstallForge detector(guardedSource, isImage(), getModuleAddress());
    INTERNAL_INFO info = detector._detect(pPdStruct);
    if (!info.bIsValid || (info.nArchiveOffset < 0) || (info.nArchiveSize <= 0)) return false;
    const qint64 nTotalSize = guardedSource->size();
    if ((nTotalSize < 0)) return false;

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

    switch (info.payload) {
        case PAYLOAD_BZIP2: pContext->pArchive = new XInstallForgeTarBzip2(pContext->pSubDevice); break;
        case PAYLOAD_GZIP: pContext->pArchive = new XInstallForgeTarGzip(pContext->pSubDevice); break;
        case PAYLOAD_7Z:
        default: pContext->pArchive = new XSevenZip(pContext->pSubDevice); break;
    }
    if (!pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct)) {
        pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pSubDevice->close();
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    const qint32 nRecords = pContext->innerState.nNumberOfRecords;
    bool bNamesValid = (nRecords > 0) && (nRecords <= 0x10000) && XBinary::isPdStructNotCanceled(pPdStruct);
    QSet<QString> setNames;
    // Compressed TAR handlers locate the current record by offset, not just by
    // index, so validate through the archive's normal traversal API.
    if (bNamesValid) pContext->listDecodedNames.reserve(nRecords);
    for (qint32 i = 0; bNamesValid && (i < nRecords); i++) {
        ARCHIVERECORD record = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
        QString sDecodedName;
        if (!decodeInstallForgeName(record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString(), &sDecodedName)) {
            bNamesValid = false;
            break;
        }
        QString sNameKey = sDecodedName.toCaseFolded();
        if (setNames.contains(sNameKey)) {
            bNamesValid = false;
            break;
        }
        setNames.insert(sNameKey);
        pContext->listDecodedNames.append(sDecodedName);
        if (((i + 1) < nRecords) && !pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct)) {
            bNamesValid = false;
        }
    }
    if (bNamesValid) {
        bNamesValid = pContext->pArchive->finishUnpack(&pContext->innerState, nullptr) && guardedSource;
        if (bNamesValid) {
            pContext->innerState = UNPACK_STATE();
            bNamesValid = pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct) && guardedSource &&
                          (pContext->innerState.nNumberOfRecords == nRecords);
        }
    }
    if (!bNamesValid || !XBinary::isPdStructNotCanceled(pPdStruct)) {
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

XBinary::ARCHIVERECORD XInstallForge::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive)
        return result;
    if (pState->nCurrentIndex >= pContext->listDecodedNames.size()) return result;
    if ((pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return result;
    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return ARCHIVERECORD();
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->listDecodedNames.at(pState->nCurrentIndex));
    return result;
}

bool XInstallForge::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedOutput = pDevice;
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive)
        return false;
    if ((pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput, pPdStruct);
    if (pState->pContext != pContext) return false;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XInstallForge::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive)
        return false;
    if ((pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    bool bResult = pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return false;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XInstallForge::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
