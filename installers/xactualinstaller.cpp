/* Copyright (c) 2017-2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xactualinstaller.h"

#include <algorithm>

#include <QBuffer>
#include <QScopedValueRollback>
#include <QSet>

#include "xpe.h"
#include "subdevice.h"
#include "xzip.h"

namespace {
class ACTUALINSTALLER_OPERATION_STATE_DELETER {
public:
    explicit ACTUALINSTALLER_OPERATION_STATE_DELETER(const QSharedPointer<XActualInstaller::UNPACK_DEFERRED_CLEANUP> &pCleanup) : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XActualInstaller::UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

QString actualNameKey(const QString &sName)
{
    return sName.toCaseFolded().normalized(QString::NormalizationForm_C);
}

QString actualUniqueOutputName(const QString &sPreferred, qint32 nRecordIndex, const QSet<QString> &setDeclaredNames, QSet<QString> *pSetAssignedNames,
                               qint32 nMaximumRecords)
{
    if (!pSetAssignedNames || sPreferred.isEmpty() || (nRecordIndex < 0)) return QString();

    const QString sPreferredKey = actualNameKey(sPreferred);
    if (!pSetAssignedNames->contains(sPreferredKey)) {
        pSetAssignedNames->insert(sPreferredKey);
        return sPreferred;
    }

    const QString sBase = QString("file_%1").arg(nRecordIndex, 4, 10, QChar('0'));
    for (qint32 i = 0; i <= nMaximumRecords; i++) {
        const QString sCandidate = (i == 0) ? sBase : QString("%1_%2").arg(sBase).arg(i);
        const QString sCandidateKey = actualNameKey(sCandidate);
        if (!setDeclaredNames.contains(sCandidateKey) && !pSetAssignedNames->contains(sCandidateKey)) {
            pSetAssignedNames->insert(sCandidateKey);
            return sCandidate;
        }
    }

    return QString();
}
}  // namespace

XActualInstaller::UNPACK_DEFERRED_CLEANUP::~UNPACK_DEFERRED_CLEANUP()
{
    const QSet<UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();
    for (UNPACK_CONTEXT *pContext : contexts) {
        if (pContext->pArchive) {
            pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
            delete pContext->pArchive;
        }
        if (pContext->pSourceValidator) {
            pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
            delete pContext->pSourceValidator;
        }
        if (pContext->pSubDevice) {
            pContext->pSubDevice->close();
            delete pContext->pSubDevice;
        }
        delete pContext;
    }
}

XActualInstaller::XActualInstaller(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XBinary(pDevice, bIsImage, nModuleAddress)
{
    m_internalInfo = INTERNAL_INFO();
    setIsArchive(true);
}

XActualInstaller::~XActualInstaller()
{
}

bool XActualInstaller::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    const INTERNAL_INFO *pInfo = static_cast<const INTERNAL_INFO *>(getInternalInfo(pPdStruct));
    return pInfo && pInfo->bIsValid;
}

bool XActualInstaller::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XActualInstaller x(pDevice);
    return x.isValid(pPdStruct);
}

XActualInstaller::INTERNAL_INFO XActualInstaller::_getInternalInfo(PDSTRUCT *pPdStruct)
{
    return _detect(pPdStruct);
}

// Cache format-specific parsing together with the XBinary memory map.
bool XActualInstaller::handleInternalInfo(PDSTRUCT *pPdStruct)
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

void *XActualInstaller::getInternalInfo(PDSTRUCT *pPdStruct)
{
    const bool bHandled = handleInternalInfo(pPdStruct);
    if (!bHandled) return nullptr;

    return &m_internalInfo;
}

void XActualInstaller::setInternalInfo(void *pInternalInfo)
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

XBinary::FT XActualInstaller::getFileType()
{
    XPE pe(getDevice());

    if (pe.isValid() && pe.is64()) {
        return FT_PE64_ACTUALINSTALLER;
    }

    return FT_PE32_ACTUALINSTALLER;
}

static bool actualLocalRangeLessThan(const QPair<qint64, qint64> &a, const QPair<qint64, qint64> &b)
{
    return (a.first < b.first) || ((a.first == b.first) && (a.second < b.second));
}

qint64 XActualInstaller::_getZipSize(qint64 nArchiveOffset, qint64 nPayloadEnd, bool *pbHasSetupIni, PDSTRUCT *pPdStruct)
{
    const quint32 nSignatureECD = 0x06054B50;
    const quint32 nSignatureCFD = 0x02014B50;
    const quint32 nSignatureLFD = 0x04034B50;
    const quint32 nSignatureDataDescriptor = 0x08074B50;
    const qint64 nECDSize = 22;
    const qint64 nCFDSize = 46;
    const qint64 nLFDSize = 30;

    if (pbHasSetupIni) *pbHasSetupIni = false;

    if ((nArchiveOffset < 0) || (nPayloadEnd <= nArchiveOffset) || ((nPayloadEnd - nArchiveOffset) < nLFDSize) || (read_uint32(nArchiveOffset) != nSignatureLFD)) {
        return 0;
    }

    qint64 nSearchOffset = nArchiveOffset;

    while ((nSearchOffset >= nArchiveOffset) && ((nPayloadEnd - nSearchOffset) >= nECDSize) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        qint64 nECDOffset = find_uint32(nSearchOffset, nPayloadEnd - nSearchOffset, nSignatureECD, false, pPdStruct);

        if ((nECDOffset < 0) || ((nPayloadEnd - nECDOffset) < nECDSize)) break;

        nSearchOffset = nECDOffset + 4;

        quint16 nDiskNumber = read_uint16(nECDOffset + 4);
        quint16 nStartDisk = read_uint16(nECDOffset + 6);
        quint16 nDiskRecords = read_uint16(nECDOffset + 8);
        quint16 nTotalRecords = read_uint16(nECDOffset + 10);
        quint32 nCentralDirectorySize = read_uint32(nECDOffset + 12);
        quint32 nCentralDirectoryRelativeOffset = read_uint32(nECDOffset + 16);
        quint16 nCommentSize = read_uint16(nECDOffset + 20);

        if ((nDiskNumber != 0) || (nStartDisk != 0) || (nDiskRecords != nTotalRecords) || (nTotalRecords == 0) ||
            (nCommentSize > (nPayloadEnd - nECDOffset - nECDSize))) {
            continue;
        }

        qint64 nCentralDirectoryOffset = nArchiveOffset + (qint64)nCentralDirectoryRelativeOffset;
        if ((nCentralDirectoryOffset < nArchiveOffset) || (nCentralDirectoryOffset > nECDOffset) ||
            ((qint64)nCentralDirectorySize != (nECDOffset - nCentralDirectoryOffset))) {
            continue;
        }

        qint64 nCurrentOffset = nCentralDirectoryOffset;
        bool bValid = true;
        bool bFirstLocalHeaderReferenced = false;
        qint32 nSetupIniCount = 0;
        QSet<qint64> setLocalHeaderOffsets;
        QList<QPair<qint64, qint64>> listLocalRanges;

        for (quint32 i = 0; i < nTotalRecords; i++) {
            if (((nECDOffset - nCurrentOffset) < nCFDSize) || (read_uint32(nCurrentOffset) != nSignatureCFD)) {
                bValid = false;
                break;
            }

            quint16 nFlags = read_uint16(nCurrentOffset + 8);
            quint16 nMethod = read_uint16(nCurrentOffset + 10);
            quint32 nCRC32 = read_uint32(nCurrentOffset + 16);
            quint32 nCompressedSize = read_uint32(nCurrentOffset + 20);
            quint32 nUncompressedSize = read_uint32(nCurrentOffset + 24);
            quint16 nFileNameSize = read_uint16(nCurrentOffset + 28);
            quint16 nExtraFieldSize = read_uint16(nCurrentOffset + 30);
            quint16 nFileCommentSize = read_uint16(nCurrentOffset + 32);
            quint16 nRecordStartDisk = read_uint16(nCurrentOffset + 34);
            quint32 nLocalHeaderRelativeOffset = read_uint32(nCurrentOffset + 42);
            qint64 nRecordSize = nCFDSize + (qint64)nFileNameSize + (qint64)nExtraFieldSize + (qint64)nFileCommentSize;

            if ((nRecordStartDisk != 0) || (nRecordSize > (nECDOffset - nCurrentOffset))) {
                bValid = false;
                break;
            }

            qint64 nLocalHeaderOffset = nArchiveOffset + (qint64)nLocalHeaderRelativeOffset;
            if ((nLocalHeaderOffset < nArchiveOffset) || ((nCentralDirectoryOffset - nLocalHeaderOffset) < nLFDSize) ||
                (read_uint32(nLocalHeaderOffset) != nSignatureLFD) || setLocalHeaderOffsets.contains(nLocalHeaderOffset)) {
                bValid = false;
                break;
            }
            setLocalHeaderOffsets.insert(nLocalHeaderOffset);
            bFirstLocalHeaderReferenced |= (nLocalHeaderOffset == nArchiveOffset);

            quint16 nLocalFlags = read_uint16(nLocalHeaderOffset + 6);
            quint16 nLocalMethod = read_uint16(nLocalHeaderOffset + 8);
            quint32 nLocalCRC32 = read_uint32(nLocalHeaderOffset + 14);
            quint32 nLocalCompressedSize = read_uint32(nLocalHeaderOffset + 18);
            quint32 nLocalUncompressedSize = read_uint32(nLocalHeaderOffset + 22);
            quint16 nLocalFileNameSize = read_uint16(nLocalHeaderOffset + 26);
            quint16 nLocalExtraFieldSize = read_uint16(nLocalHeaderOffset + 28);
            qint64 nLocalDataOffset = nLocalHeaderOffset + nLFDSize + (qint64)nLocalFileNameSize + (qint64)nLocalExtraFieldSize;

            if ((nLocalFlags != nFlags) || (nLocalMethod != nMethod) || (nLocalDataOffset > nCentralDirectoryOffset) ||
                ((qint64)nCompressedSize > (nCentralDirectoryOffset - nLocalDataOffset)) || (nLocalFileNameSize != nFileNameSize) ||
                (read_array(nLocalHeaderOffset + nLFDSize, nLocalFileNameSize) != read_array(nCurrentOffset + nCFDSize, nFileNameSize)) ||
                (!(nLocalFlags & 0x0008) && ((nLocalCRC32 != nCRC32) || (nLocalCompressedSize != nCompressedSize) || (nLocalUncompressedSize != nUncompressedSize)))) {
                bValid = false;
                break;
            }

            qint64 nLocalRecordEnd = nLocalDataOffset + (qint64)nCompressedSize;
            if (nLocalFlags & 0x0008) {
                const qint64 nDescriptorOffset = nLocalRecordEnd;
                const qint64 nDescriptorAvailable = nCentralDirectoryOffset - nDescriptorOffset;
                // The signature is optional, and an unsigned descriptor may
                // legitimately have a CRC equal to the signature value. Test
                // both layouts against the authenticated central-directory
                // values instead of consuming the first DWORD as a signature.
                const bool bUnsignedDescriptor = (nDescriptorAvailable >= 12) && (read_uint32(nDescriptorOffset) == nCRC32) &&
                                                 (read_uint32(nDescriptorOffset + 4) == nCompressedSize) && (read_uint32(nDescriptorOffset + 8) == nUncompressedSize);
                const bool bSignedDescriptor = (nDescriptorAvailable >= 16) && (read_uint32(nDescriptorOffset) == nSignatureDataDescriptor) &&
                                               (read_uint32(nDescriptorOffset + 4) == nCRC32) && (read_uint32(nDescriptorOffset + 8) == nCompressedSize) &&
                                               (read_uint32(nDescriptorOffset + 12) == nUncompressedSize);
                if (bUnsignedDescriptor == bSignedDescriptor) {
                    bValid = false;
                    break;
                }
                nLocalRecordEnd = nDescriptorOffset + (bSignedDescriptor ? 16 : 12);
            }

            listLocalRanges.append(qMakePair(nLocalHeaderOffset, nLocalRecordEnd));

            if (pbHasSetupIni) {
                QByteArray baName = read_array(nCurrentOffset + nCFDSize, nFileNameSize);
                if (baName.toLower() == QByteArrayLiteral("aisetup.ini")) nSetupIniCount++;
            }

            nCurrentOffset += nRecordSize;
        }

        if (bValid) {
            std::sort(listLocalRanges.begin(), listLocalRanges.end(), actualLocalRangeLessThan);

            for (qint32 i = 1; i < listLocalRanges.size(); i++) {
                if (listLocalRanges.at(i).first < listLocalRanges.at(i - 1).second) {
                    bValid = false;
                    break;
                }
            }
        }

        if (bValid && bFirstLocalHeaderReferenced && (nCurrentOffset == nECDOffset)) {
            if (pbHasSetupIni) *pbHasSetupIni = (nSetupIniCount == 1);
            return (nECDOffset + nECDSize + nCommentSize) - nArchiveOffset;
        }
    }

    return 0;
}

XActualInstaller::INTERNAL_INFO XActualInstaller::_detect(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO result = {};
    result.nArchiveOffset = -1;

    XPE pe(getDevice(), isImage(), getModuleAddress());
    if (!pe.isValid(pPdStruct)) return result;

    const qint64 nTotalSize = getSize();
    qint64 nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    if ((nOverlayOffset <= 0) || (nOverlayOffset >= nTotalSize)) return result;

    // Authenticate both concatenated ZIPs. Merely finding "aisetup.ini" in the
    // overlay is insufficient: arbitrary PE overlay data can contain both the
    // local-header bytes and marker text without being an Actual Installer.
    qint64 nFilesArchiveSize = _getZipSize(nOverlayOffset, nTotalSize, nullptr, pPdStruct);
    if (nFilesArchiveSize <= 0) return result;

    qint64 nMetadataOffset = nOverlayOffset + nFilesArchiveSize;
    bool bHasSetupIni = false;
    qint64 nMetadataArchiveSize = _getZipSize(nMetadataOffset, nTotalSize, &bHasSetupIni, pPdStruct);
    if ((nMetadataArchiveSize <= 0) || !bHasSetupIni) return result;

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    result.bIsValid = true;
    result.nArchiveOffset = nOverlayOffset;
    result.nArchiveSize = nFilesArchiveSize + nMetadataArchiveSize;

    // Edition from the .rsrc DFM caption ("Actual Installer Free" for the Free edition).
    if (find_ansiString(0, nTotalSize, "Actual Installer Free", pPdStruct) != -1) {
        result.sVersion = "Free";
    }

    if (!XBinary::isPdStructNotCanceled(pPdStruct)) result.bIsValid = false;

    return result;
}

// --- streaming extraction: delegate to the XArchive ZIP handler ---

QMap<XBinary::UNPACK_PROP, QVariant> XActualInstaller::getDefaultUnpackProperties()
{
    QMap<XBinary::UNPACK_PROP, QVariant> result = XBinary::getDefaultUnpackProperties();

    QIODevice *pDevice = getDevice();

    if (pDevice) {
        INTERNAL_INFO info = _detect(nullptr);

        if (info.bIsValid && (info.nArchiveOffset >= 0) && (info.nArchiveSize > 0)) {
            qint64 nPayloadEnd = info.nArchiveOffset + info.nArchiveSize;

            qint64 nArchiveOffset = info.nArchiveOffset;

            for (qint32 i = 0; (i < 2) && (nArchiveOffset < nPayloadEnd); i++) {
                qint64 nArchiveSize = _getZipSize(nArchiveOffset, nPayloadEnd, nullptr, nullptr);

                if (nArchiveSize <= 0) {
                    break;
                }

                SubDevice subDevice(pDevice, nArchiveOffset, nArchiveSize);

                if (!subDevice.open(QIODevice::ReadOnly)) {
                    break;
                }

                {
                    XZip archive(&subDevice);
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

                nArchiveOffset += nArchiveSize;
            }
        }
    }

    return result;
}

bool XActualInstaller::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
        if (pOldContext->pSourceValidator) {
            pOldContext->pSourceValidator->releaseUnpackSource(&pOldContext->sourceValidationState);
            delete pOldContext->pSourceValidator;
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
    XArchive initialSourceValidator(guardedSource);
    UNPACK_STATE initialSourceState = {};
    if (!initialSourceValidator.bindUnpackSource(&initialSourceState, pPdStruct)) return false;
    XActualInstaller detector(guardedSource, isImage(), getModuleAddress());
    INTERNAL_INFO info = detector._detect(pPdStruct);
    if (!info.bIsValid || (info.nArchiveOffset < 0) || (info.nArchiveSize <= 0)) return false;
    const qint64 nTotalSize = guardedSource->size();
    if ((nTotalSize < 0)) return false;

    const qint64 nPayloadEnd = info.nArchiveOffset + info.nArchiveSize;

    qint64 nFilesArchiveSize = detector._getZipSize(info.nArchiveOffset, nPayloadEnd, nullptr, pPdStruct);
    if ((nFilesArchiveSize <= 0) || !initialSourceValidator.isUnpackSourceCurrent(&initialSourceState, pPdStruct))
        return false;

    UNPACK_CONTEXT *pContext = new UNPACK_CONTEXT;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pSubDevice = new SubDevice(guardedSource, info.nArchiveOffset, nFilesArchiveSize);
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();
    pContext->pSourceValidator = new XArchive(guardedSource);
    pContext->sourceValidationState = UNPACK_STATE();

    if (!pContext->pSourceValidator->bindUnpackSource(&pContext->sourceValidationState, pPdStruct)) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    if (!pContext->pSubDevice->open(QIODevice::ReadOnly)) {
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    pContext->pArchive = new XZip(pContext->pSubDevice);
    if (!pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct)) {
        pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
        pContext->pSubDevice->close();
        delete pContext->pSubDevice;
        delete pContext;
        return false;
    }

    // Actual Installer stores the real payload names in aisetup.ini inside a
    // second ZIP immediately following the numeric-name payload ZIP.
    qint64 nMetadataOffset = info.nArchiveOffset + nFilesArchiveSize;
    qint64 nMetadataSize = detector._getZipSize(nMetadataOffset, nPayloadEnd, nullptr, pPdStruct);
    bool bMetadataValid = false;
    if (nMetadataSize > 0) {
        SubDevice metadataDevice(guardedSource, nMetadataOffset, nMetadataSize);
        if (metadataDevice.open(QIODevice::ReadOnly)) {
            XZip metadataArchive(&metadataDevice);
            UNPACK_STATE metadataState = {};
            if (metadataArchive.initUnpack(&metadataState, mapProperties, pPdStruct)) {
                bool bIniFound = false;
                bool bIniMappingValid = true;
                if (metadataState.nNumberOfRecords > 0) {
                    do {
                        ARCHIVERECORD metadataRecord = metadataArchive.infoCurrent(&metadataState, pPdStruct);
                        QString sName = metadataRecord.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
                        if (QString::compare(sName, QStringLiteral("aisetup.ini"), Qt::CaseInsensitive) != 0) continue;
                        if (bIniFound) {
                            pContext->mapRecordNames.clear();
                            break;
                        }
                        bIniFound = true;

                        qint64 nIniSize = metadataRecord.mapProperties.value(FPART_PROP_UNCOMPRESSEDSIZE, Q_INT64_C(-1)).toLongLong();
                        if ((nIniSize < 0) || (nIniSize > (16 << 20))) break;

                        QByteArray baIni;
                        QBuffer output(&baIni);
                        if (output.open(QIODevice::ReadWrite) && metadataArchive.unpackCurrent(&metadataState, &output, pPdStruct) && (baIni.size() == nIniSize)) {
                            QString sIni = QString::fromUtf8(baIni);
                            if (sIni.contains(QChar::ReplacementCharacter)) break;
                            QStringList listLines = sIni.split(QChar('\n'));
                            bool bFilesSection = false;
                            QSet<qint32> setDeclaredIndexes;

                            for (int i = 0; i < listLines.size(); i++) {
                                QString sLine = listLines.at(i).trimmed();
                                if (sLine.startsWith('[') && sLine.endsWith(']')) {
                                    bFilesSection = (QString::compare(sLine, QStringLiteral("[Files]"), Qt::CaseInsensitive) == 0);
                                    continue;
                                }
                                if (!bFilesSection || sLine.isEmpty()) continue;

                                int nEquals = sLine.indexOf('=');
                                if (nEquals <= 0) continue;
                                bool bIndexValid = false;
                                qint32 nIndex = sLine.left(nEquals).trimmed().toInt(&bIndexValid);
                                if (!bIndexValid || (nIndex < 0)) continue;
                                if (setDeclaredIndexes.contains(nIndex)) {
                                    bIniMappingValid = false;
                                    break;
                                }
                                setDeclaredIndexes.insert(nIndex);

                                QString sPath = sLine.mid(nEquals + 1);
                                int nFlags = sPath.lastIndexOf('?');
                                if (nFlags >= 0) sPath.truncate(nFlags);
                                sPath.replace('\\', '/');
                                QString sFileName = sPath.mid(sPath.lastIndexOf('/') + 1).trimmed();
                                if (sFileName.isEmpty() || (sFileName == ".") || (sFileName == "..") || sFileName.contains('/') || sFileName.contains('\\') ||
                                    sFileName.contains(':')) {
                                    continue;
                                }
                                static const QString sInvalidNameChars = QStringLiteral("<>:\"/\\|?*");
                                bool bSafeName = true;
                                for (int j = 0; j < sFileName.size(); j++) {
                                    if (!sFileName.at(j).isPrint() || sInvalidNameChars.contains(sFileName.at(j))) {
                                        bSafeName = false;
                                        break;
                                    }
                                }
                                static const QSet<QString> setReservedNames = {
                                    QStringLiteral("CON"),  QStringLiteral("PRN"),  QStringLiteral("AUX"),    QStringLiteral("NUL"),     QStringLiteral("COM1"),
                                    QStringLiteral("COM2"), QStringLiteral("COM3"), QStringLiteral("COM4"),   QStringLiteral("COM5"),    QStringLiteral("COM6"),
                                    QStringLiteral("COM7"), QStringLiteral("COM8"), QStringLiteral("COM9"),   QStringLiteral("LPT1"),    QStringLiteral("LPT2"),
                                    QStringLiteral("LPT3"), QStringLiteral("LPT4"), QStringLiteral("LPT5"),   QStringLiteral("LPT6"),    QStringLiteral("LPT7"),
                                    QStringLiteral("LPT8"), QStringLiteral("LPT9"), QStringLiteral("CONIN$"), QStringLiteral("CONOUT$"), QStringLiteral("CLOCK$")};
                                QString sStem = sFileName.section('.', 0, 0).toUpper();
                                sStem.replace(QChar(0x00B9), QLatin1Char('1'));
                                sStem.replace(QChar(0x00B2), QLatin1Char('2'));
                                sStem.replace(QChar(0x00B3), QLatin1Char('3'));
                                if (!bSafeName || (sFileName.size() > 255) || sFileName.endsWith('.') || sFileName.endsWith(' ') || setReservedNames.contains(sStem)) {
                                    continue;
                                }
                                pContext->mapRecordNames.insert(nIndex, sFileName);
                            }
                        }
                        break;
                    } while (metadataArchive.moveToNext(&metadataState, pPdStruct));
                }
                bool bFinishOK = metadataArchive.finishUnpack(&metadataState, pPdStruct);
                bool bCompleteMap = guardedSource && bIniFound && bIniMappingValid && bFinishOK &&
                                    (pContext->mapRecordNames.size() == pContext->innerState.nNumberOfRecords);
                QSet<QString> setDeclaredNames;
                for (qint32 i = 0; bCompleteMap && (i < pContext->innerState.nNumberOfRecords); i++) {
                    QString sMappedName = pContext->mapRecordNames.value(i);
                    if (sMappedName.isEmpty()) bCompleteMap = false;
                    else setDeclaredNames.insert(actualNameKey(sMappedName));
                }
                QSet<QString> setAssignedNames;
                for (qint32 i = 0; bCompleteMap && (i < pContext->innerState.nNumberOfRecords); i++) {
                    QString sUniqueName =
                        actualUniqueOutputName(pContext->mapRecordNames.value(i), i, setDeclaredNames, &setAssignedNames, pContext->innerState.nNumberOfRecords);
                    if (sUniqueName.isEmpty()) bCompleteMap = false;
                    else pContext->mapRecordNames.insert(i, sUniqueName);
                }
                bMetadataValid = bCompleteMap;
            }
            metadataDevice.close();
        }
    }

    // The first ZIP is intentionally numeric. Authenticate that every central
    // directory record is a unique canonical index covered by aisetup.ini;
    // otherwise an unmapped or path-bearing ZIP name could escape the metadata
    // naming contract exposed by infoCurrent().
    if (bMetadataValid) {
        QSet<qint32> setPayloadIndexes;
        const qint32 nPayloadRecords = pContext->innerState.nNumberOfRecords;
        for (qint32 i = 0; i < nPayloadRecords; i++) {
            ARCHIVERECORD record = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
                bMetadataValid = false;
                break;
            QString sInnerName = record.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
            bool bIndexValid = false;
            qint32 nIndex = sInnerName.toInt(&bIndexValid);
            if (!bIndexValid || (QString::number(nIndex) != sInnerName) || !pContext->mapRecordNames.contains(nIndex) || setPayloadIndexes.contains(nIndex)) {
                bMetadataValid = false;
                break;
            }
            setPayloadIndexes.insert(nIndex);
            if (((i + 1) < nPayloadRecords) && !pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct)) {
                bMetadataValid = false;
                break;
            }
                bMetadataValid = false;
                break;
        }
        if (setPayloadIndexes.size() != pContext->innerState.nNumberOfRecords) bMetadataValid = false;
        if (bMetadataValid) {
            bMetadataValid = pContext->pArchive->finishUnpack(&pContext->innerState, nullptr) && guardedSource;
            if (bMetadataValid) {
                pContext->innerState = UNPACK_STATE();
                bMetadataValid = pContext->pArchive->initUnpack(&pContext->innerState, mapProperties, pPdStruct) && guardedSource &&
                                 (pContext->innerState.nNumberOfRecords == nPayloadRecords);
            }
        }
    }

    if (!bMetadataValid || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !pContext->pSourceValidator->validateAndFinalizeUnpackSource(&pContext->sourceValidationState, pPdStruct)) {
        pContext->pArchive->finishUnpack(&pContext->innerState, nullptr);
        delete pContext->pArchive;
        pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
        delete pContext->pSourceValidator;
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

XBinary::ARCHIVERECORD XActualInstaller::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive)
        return result;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) ||
        (pState->pContext != pContext))
        return result;
    if ((pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return result;
    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return ARCHIVERECORD();
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) ||
        (pState->pContext != pContext))
        return ARCHIVERECORD();

    QString sInnerName = result.mapProperties.value(FPART_PROP_ORIGINALNAME).toString();
    bool bIndexValid = false;
    qint32 nIndex = sInnerName.toInt(&bIndexValid);
    if (bIndexValid && pContext->mapRecordNames.contains(nIndex)) {
        result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->mapRecordNames.value(nIndex));
    }

    return result;
}

bool XActualInstaller::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedOutput = pDevice;
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput->isOpen() || !guardedOutput->isWritable() ||
        guardedOutput->isSequential() || (guardedOutput->openMode() & (QIODevice::Append | QIODevice::Text)) ||
        !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive)
        return false;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) ||
        (pState->pContext != pContext))
        return false;
    if ((pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput, pPdStruct);
    if (pState->pContext != pContext) return false;
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) ||
        (pState->pContext != pContext)) {
        if (guardedOutput) {
            XBinary::resize(guardedOutput, 0);
            if (guardedOutput) guardedOutput->seek(0);
        }
        return false;
    }
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XActualInstaller::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    UNPACK_CONTEXT *pContext = static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice || (pContext->pOuterSourceDevice != getDevice()) ||
        (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive)
        return false;
    if (!pContext->pSourceValidator || !pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) ||
        (pState->pContext != pContext))
        return false;
    if ((pContext->innerState.nCurrentIndex != pState->nCurrentIndex) || (pContext->innerState.nCurrentOffset != pState->nCurrentOffset) ||
        (pContext->innerState.nNumberOfRecords != pState->nNumberOfRecords))
        return false;
    bool bResult = pContext->pArchive->moveToNext(&pContext->innerState, pPdStruct);
    if (pState->pContext != pContext) return false;
    if (!pContext->pSourceValidator->isUnpackSourceCurrent(&pContext->sourceValidationState, pPdStruct) ||
        (pState->pContext != pContext))
        return false;
    pState->nCurrentIndex = pContext->innerState.nCurrentIndex;
    pState->nCurrentOffset = pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XActualInstaller::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        if (pContext->pSourceValidator) {
            pContext->pSourceValidator->releaseUnpackSource(&pContext->sourceValidationState);
            delete pContext->pSourceValidator;
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
