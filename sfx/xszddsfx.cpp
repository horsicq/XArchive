/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xszddsfx.h"

#include <QRegularExpression>
#include <QScopedValueRollback>

#include <new>

#include "subdevice.h"
#include "xatarist.h"
#include "xcom.h"
#include "xelf.h"
#include "xmsdos.h"
#include "xne.h"
#include "xpe.h"
#include "xszdd.h"

namespace {

const qint64 SZDDSFX_SCAN_LIMIT = 16LL * 1024 * 1024;
const qint32 SZDDSFX_MAX_ENTRIES = 65536;
const qint64 SZDD_STANDARD_HEADER_SIZE = 14;
const qint64 SZDD_LEGACY_HEADER_SIZE = 12;

bool isStandardSignature(const uchar *pData, qint64 nAvailable)
{
    return (nAvailable >= 8) && (pData[0] == 'S') && (pData[1] == 'Z') && (pData[2] == 'D') && (pData[3] == 'D') && (pData[4] == 0x88) &&
           (pData[5] == 0xF0) && (pData[6] == 0x27) && ((pData[7] == 0x33) || (pData[7] == 0x3A));
}

bool isLegacySignature(const uchar *pData, qint64 nAvailable)
{
    return (nAvailable >= 8) && (pData[0] == 'Z') && (pData[1] == 'D') && (pData[2] == 'D') && (pData[3] == 0x88) && (pData[4] == 0xF0) &&
           (pData[5] == 0x27) && ((pData[6] == 0x33) || (pData[6] == 0x3A)) && (pData[7] == 'A');
}

quint32 readLe32(const uchar *pData)
{
    return (quint32)pData[0] | ((quint32)pData[1] << 8) | ((quint32)pData[2] << 16) | ((quint32)pData[3] << 24);
}

QString suffixForMissingCharacter(quint8 nMissingChar)
{
    // A stream embedded without its original compressed filename carries only
    // the final character that COMPRESS.EXE replaced with '_'. Preserve the
    // common Microsoft installer extensions where that byte is sufficient to
    // identify them; otherwise keep the byte visible without inventing a name.
    switch (QChar(nMissingChar).toLower().toLatin1()) {
        case 'e': return QStringLiteral(".exe");
        case 'f': return QStringLiteral(".inf");
        case 'i': return QStringLiteral(".ini");
        case 'l': return QStringLiteral(".dll");
        case 'm': return QStringLiteral(".com");
        case 'p': return QStringLiteral(".hlp");
        case 's': return QStringLiteral(".sys");
        case 't': return QStringLiteral(".txt");
        case 'v': return QStringLiteral(".drv");
        default: break;
    }

    if (((nMissingChar >= 'A') && (nMissingChar <= 'Z')) || ((nMissingChar >= 'a') && (nMissingChar <= 'z'))) {
        return QStringLiteral(".szdd-") + QChar(nMissingChar);
    }

    return QString();
}

bool readAsciiString(const QByteArray &baData, qint32 nOffset,
                     QString *pString, qint32 *pNextOffset)
{
    if (!pString || !pNextOffset || (nOffset < 0) ||
        (nOffset >= baData.size())) {
        return false;
    }
    const qint32 nEnd = baData.indexOf('\0', nOffset);
    const qint32 nLength = nEnd - nOffset;
    if ((nEnd < 0) || (nLength <= 0) || (nLength > 32)) return false;
    for (qint32 i = nOffset; i < nEnd; ++i) {
        const uchar nByte = static_cast<uchar>(baData.at(i));
        if ((nByte < 0x20) || (nByte > 0x7E)) return false;
    }
    *pString = QString::fromLatin1(baData.constData() + nOffset, nLength);
    *pNextOffset = nEnd + 1;
    return true;
}

class SZDDSFX_OPERATION_STATE_DELETER {
public:
    explicit SZDDSFX_OPERATION_STATE_DELETER(
        const QSharedPointer<XSzddSFX::SZDDSFX_UNPACK_DEFERRED_CLEANUP> &pCleanup)
        : m_pCleanup(pCleanup)
    {
    }

    void operator()(bool *pValue) const
    {
        delete pValue;
    }

private:
    QSharedPointer<XSzddSFX::SZDDSFX_UNPACK_DEFERRED_CLEANUP> m_pCleanup;
};

}  // namespace

XSzddSFX::SZDDSFX_UNPACK_DEFERRED_CLEANUP::~SZDDSFX_UNPACK_DEFERRED_CLEANUP()
{
    const QSet<SZDDSFX_UNPACK_CONTEXT *> contexts = setContexts;
    setContexts.clear();

    for (SZDDSFX_UNPACK_CONTEXT *pContext : contexts) {
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

XSzddSFX::XSzddSFX(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XSFX(pDevice, bIsImage, nModuleAddress, ARC_SZDD)
{
    m_pSzddUnpackDeferredCleanup = QSharedPointer<SZDDSFX_UNPACK_DEFERRED_CLEANUP>::create();
    const QSharedPointer<SZDDSFX_UNPACK_DEFERRED_CLEANUP> pDeferredCleanup = m_pSzddUnpackDeferredCleanup;
    m_pSzddUnpackOperationState = QSharedPointer<bool>(
        new bool(false), SZDDSFX_OPERATION_STATE_DELETER(pDeferredCleanup));
}

XSzddSFX::~XSzddSFX()
{
    if (m_pSzddUnpackOperationState) *m_pSzddUnpackOperationState = true;
    if (m_pSzddUnpackDeferredCleanup) {
        m_pSzddUnpackDeferredCleanup->setContexts.unite(m_setSzddUnpackContexts);
        m_setSzddUnpackContexts.clear();
    }
    m_pSzddUnpackDeferredCleanup.clear();
    m_pSzddUnpackOperationState.clear();
}

bool XSzddSFX::isValid(PDSTRUCT *pPdStruct)
{
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    QPointer<XSzddSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource) return false;

    XMSDOS msdos(guardedSource.data(), isImage(), getModuleAddress());
    const bool bMSDOS = msdos.isValid(pPdStruct);
    XNE ne(guardedSource.data(), isImage(), getModuleAddress());
    const bool bNE = ne.isValid(pPdStruct);
    XPE pe(guardedSource.data(), isImage(), getModuleAddress());
    const bool bPE = pe.isValid(pPdStruct);
    XELF elf(guardedSource.data(), isImage(), getModuleAddress());
    const bool bELF = elf.isValid(pPdStruct);
    XAtariST atariST(guardedSource.data(), isImage(), getModuleAddress());
    const bool bAtariST = atariST.isValid(pPdStruct);
    const bool bCOM = !bMSDOS && (XBinary::getDeviceFileSuffix(guardedSource.data()).compare(QStringLiteral("COM"), Qt::CaseInsensitive) == 0) &&
                      XCOM::isValid(guardedSource.data(), isImage(), getModuleAddress(), pPdStruct);
    if (!guardedThis || !guardedSource || (!bMSDOS && !bNE && !bPE && !bELF && !bAtariST && !bCOM)) return false;

    QList<SZDDSFX_ENTRY> listEntries;
    const bool bScan = guardedThis->_scanStreams(&listEntries, pPdStruct);
    return bScan && guardedThis && !listEntries.isEmpty();
}

bool XSzddSFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSzddSFX sfx(pDevice);
    return sfx.isValid(pPdStruct);
}

QMap<XBinary::UNPACK_PROP, QVariant> XSzddSFX::getDefaultUnpackProperties()
{
    XSZDD archive;
    return archive.getDefaultUnpackProperties();
}

bool XSzddSFX::_scanStreams(QList<SZDDSFX_ENTRY> *pList, PDSTRUCT *pPdStruct)
{
    if (!pList || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    pList->clear();

    QPointer<XSzddSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nTotalSize = guardedSource->size();
    if (!guardedThis || !guardedSource || (nTotalSize < SZDD_LEGACY_HEADER_SIZE)) return false;
    const qint64 nScanSize = qMin(nTotalSize, SZDDSFX_SCAN_LIMIT);
    const QByteArray baData = guardedThis->read_array_process(0, nScanSize, pPdStruct);
    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || (baData.size() != nScanSize)) return false;

    QList<SZDDSFX_ENTRY> listCandidates;
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData());
    qint64 nOffset = 0;

    while ((nOffset <= nScanSize - 8) && XBinary::isPdStructNotCanceled(pPdStruct)) {
        const qint64 nAvailable = nScanSize - nOffset;
        const bool bStandard = isStandardSignature(pData + nOffset, nAvailable);
        const bool bLegacy = !bStandard && isLegacySignature(pData + nOffset, nAvailable);
        qint64 nHeaderSize = 0;
        qint64 nSizeOffset = 0;
        quint8 nMissingChar = 0;

        if (bStandard && (nAvailable >= SZDD_STANDARD_HEADER_SIZE) && (pData[nOffset + 8] == 'A')) {
            nHeaderSize = SZDD_STANDARD_HEADER_SIZE;
            nSizeOffset = nOffset + 10;
            nMissingChar = pData[nOffset + 9];
        } else if (bLegacy && (nAvailable >= SZDD_LEGACY_HEADER_SIZE)) {
            nHeaderSize = SZDD_LEGACY_HEADER_SIZE;
            nSizeOffset = nOffset + 8;
        }

        if (nHeaderSize) {
            const quint32 nUncompressedSize = readLe32(pData + nSizeOffset);
            if ((nUncompressedSize > 0) && (nUncompressedSize <= 0x40000000U)) {
                SZDDSFX_ENTRY entry = {};
                entry.nHeaderOffset = nOffset;
                entry.nHeaderSize = nHeaderSize;
                entry.nStreamSize = 0;
                entry.nUncompressedSize = nUncompressedSize;
                entry.nMissingChar = nMissingChar;
                listCandidates.append(entry);
                if (listCandidates.count() > SZDDSFX_MAX_ENTRIES) return false;

                // The standard signature contains the legacy signature one
                // byte later. Resume after the complete accepted signature.
                nOffset += 8;
                continue;
            }
        }

        ++nOffset;
    }

    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || listCandidates.isEmpty()) return false;

    // Apply the same conservative framing guard as XSZDD, using the next
    // accepted same-family header (or physical EOF) as the compressed bound.
    QList<SZDDSFX_ENTRY> listAccepted;
    for (qint32 i = 0; i < listCandidates.count(); ++i) {
        SZDDSFX_ENTRY entry = listCandidates.at(i);
        const qint64 nEndOffset = (i + 1 < listCandidates.count()) ? listCandidates.at(i + 1).nHeaderOffset : nTotalSize;
        if ((nEndOffset <= entry.nHeaderOffset) || (nEndOffset > nTotalSize)) continue;

        qint64 nStreamSize = nEndOffset - entry.nHeaderOffset;
        const qint64 nTrimWindow = qMin<qint64>(qMax<qint64>(0, nStreamSize - 8), 4096);
        if (nTrimWindow > 0) {
            const QByteArray baTail = guardedThis->read_array_process(nEndOffset - nTrimWindow, nTrimWindow, pPdStruct);
            if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || (baTail.size() != nTrimWindow)) return false;
            qint64 nTrailingZeroes = 0;
            while ((nTrailingZeroes < baTail.size()) && (baTail.at(baTail.size() - 1 - nTrailingZeroes) == 0)) ++nTrailingZeroes;
            nStreamSize -= nTrailingZeroes;
        }

        const qint64 nCompressedSize = nStreamSize - entry.nHeaderSize;
        const qint64 nMinimumCompressedSize = (entry.nUncompressedSize + 1023) / 1024;
        if ((nCompressedSize <= 0) || (nCompressedSize < nMinimumCompressedSize)) continue;
        entry.nStreamSize = nStreamSize;
        listAccepted.append(entry);
    }

    if (listAccepted.isEmpty()) return false;

    // A rejected false header must not keep truncating its predecessor.
    for (qint32 i = 0; i < listAccepted.count(); ++i) {
        SZDDSFX_ENTRY &entry = listAccepted[i];
        const qint64 nEndOffset = (i + 1 < listAccepted.count()) ? listAccepted.at(i + 1).nHeaderOffset : nTotalSize;
        qint64 nStreamSize = nEndOffset - entry.nHeaderOffset;
        const qint64 nTrimWindow = qMin<qint64>(qMax<qint64>(0, nStreamSize - 8), 4096);
        if (nTrimWindow > 0) {
            const QByteArray baTail = guardedThis->read_array_process(nEndOffset - nTrimWindow, nTrimWindow, pPdStruct);
            if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || (baTail.size() != nTrimWindow)) return false;
            qint64 nTrailingZeroes = 0;
            while ((nTrailingZeroes < baTail.size()) && (baTail.at(baTail.size() - 1 - nTrailingZeroes) == 0)) ++nTrailingZeroes;
            nStreamSize -= nTrailingZeroes;
        }
        entry.nStreamSize = nStreamSize;
    }

    const QList<QString> listRecoveredNames = guardedThis->_recoverNames(listAccepted.count(), listAccepted.first().nHeaderOffset, pPdStruct);
    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QString sOuterBaseName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    if (sOuterBaseName.isEmpty()) sOuterBaseName = QStringLiteral("szdd");

    for (qint32 i = 0; i < listAccepted.count(); ++i) {
        if (listRecoveredNames.count() == listAccepted.count()) {
            listAccepted[i].sName = listRecoveredNames.at(i);
        } else {
            listAccepted[i].sName = QStringLiteral("%1_%2").arg(sOuterBaseName).arg(i + 1);
            listAccepted[i].sName += suffixForMissingCharacter(listAccepted.at(i).nMissingChar);
        }
        if (listAccepted.at(i).sName.isEmpty()) listAccepted[i].sName = QStringLiteral("szdd_%1").arg(i + 1);
    }

    *pList = listAccepted;
    return true;
}

QList<QString> XSzddSFX::_recoverNames(qint32 nExpectedCount, qint64 nFirstHeaderOffset, PDSTRUCT *pPdStruct)
{
    QList<QString> result;
    if ((nExpectedCount < 2) || (nFirstHeaderOffset <= 0) || !XBinary::isPdStructNotCanceled(pPdStruct)) return result;

    QPointer<XSzddSFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource || (nFirstHeaderOffset > guardedSource->size()) || (nFirstHeaderOffset > SZDDSFX_SCAN_LIMIT)) return result;

    const QByteArray baData = guardedThis->read_array_process(0, nFirstHeaderOffset, pPdStruct);
    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct) || (baData.size() != nFirstHeaderOffset)) return result;

    static const QRegularExpression rePacked(QStringLiteral("^[A-Za-z0-9_.-]{1,12}\\.[A-Za-z0-9]{2}_$"));
    static const QRegularExpression reExpanded(QStringLiteral("^[A-Za-z0-9_.-]{1,12}\\.[A-Za-z0-9]{3}$"));

    for (qint32 nStart = 0; nStart < baData.size(); ++nStart) {
        if ((nStart > 0) && (baData.at(nStart - 1) != 0)) continue;
        qint32 nCursor = nStart;
        bool bMatch = true;

        for (qint32 i = 0; i < nExpectedCount; ++i) {
            QString sPacked;
            qint32 nNext = 0;
            if (!readAsciiString(baData, nCursor, &sPacked, &nNext) ||
                !rePacked.match(sPacked).hasMatch()) {
                bMatch = false;
                break;
            }
            nCursor = nNext;
        }
        if (!bMatch) continue;

        QList<QString> listNames;
        QSet<QString> setNames;
        for (qint32 i = 0; i < nExpectedCount; ++i) {
            QString sExpanded;
            qint32 nNext = 0;
            if (!readAsciiString(baData, nCursor, &sExpanded, &nNext) ||
                !reExpanded.match(sExpanded).hasMatch() ||
                setNames.contains(sExpanded)) {
                bMatch = false;
                break;
            }
            listNames.append(sExpanded);
            setNames.insert(sExpanded);
            nCursor = nNext;
        }

        if (bMatch && (listNames.count() == nExpectedCount)) return listNames;
    }

    return result;
}

bool XSzddSFX::_releaseEntry(SZDDSFX_UNPACK_CONTEXT *pContext)
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

bool XSzddSFX::_bindEntry(SZDDSFX_UNPACK_CONTEXT *pContext, qint32 nIndex, PDSTRUCT *pPdStruct)
{
    if (!pContext || (nIndex < 0) || (nIndex >= pContext->listEntries.count()) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (!_releaseEntry(pContext)) return false;

    const SZDDSFX_ENTRY &entry = pContext->listEntries.at(nIndex);
    SubDevice *pSubDevice = new (std::nothrow) SubDevice(pContext->pOuterSourceDevice.data(), entry.nHeaderOffset, entry.nStreamSize);
    if (!pSubDevice) return false;
    pSubDevice->setProperty("FileName", entry.sName);
    if (!pSubDevice->open(QIODevice::ReadOnly)) {
        delete pSubDevice;
        return false;
    }

    XSZDD *pArchive = new (std::nothrow) XSZDD(pSubDevice);
    if (!pArchive) {
        pSubDevice->close();
        delete pSubDevice;
        return false;
    }

    // XArchive binds source ownership to the UNPACK_STATE object's address.
    // Initialize the context-owned state in place; copying a successfully
    // initialized temporary would make finish/move fail the ownership check.
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

bool XSzddSFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pSzddUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSzddSFX> guardedThis(this);

    if (pState->pContext) {
        SZDDSFX_UNPACK_CONTEXT *pOldContext = static_cast<SZDDSFX_UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setSzddUnpackContexts.contains(pOldContext) || (pOldContext->pOwnerState != pState)) return false;
        m_setSzddUnpackContexts.remove(pOldContext);
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

    // Keep the established executable-carrier attribution and false-positive
    // semantics. Enumeration itself is the dedicated one-pass scan below.
    if (!guardedThis->isValid(pPdStruct) || !guardedThis || !guardedSource) return false;

    QList<SZDDSFX_ENTRY> listEntries;
    if (!guardedThis->_scanStreams(&listEntries, pPdStruct) || !guardedThis || !guardedSource || listEntries.isEmpty()) return false;

    SZDDSFX_UNPACK_CONTEXT *pContext = new (std::nothrow) SZDDSFX_UNPACK_CONTEXT;
    if (!pContext) return false;
    pContext->listEntries = listEntries;
    pContext->pOuterSourceDevice = guardedSource;
    pContext->nOwnerDeviceGeneration = getDeviceGeneration();
    pContext->pOwnerState = pState;
    pContext->pSubDevice = nullptr;
    pContext->pArchive = nullptr;
    pContext->innerState = UNPACK_STATE();
    pContext->mapUnpackProperties = mapProperties;

    if (!guardedThis->_bindEntry(pContext, 0, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) guardedThis->_releaseEntry(pContext);
        delete pContext;
        return false;
    }

    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = listEntries.count();
    pState->nCurrentOffset = listEntries.first().nHeaderOffset;
    pState->nTotalSize = guardedSource->size();
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    pState->pContext = pContext;
    m_setSzddUnpackContexts.insert(pContext);
    return true;
}

XBinary::ARCHIVERECORD XSzddSFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    ARCHIVERECORD result = {};
    QSharedPointer<bool> pOperationState = m_pSzddUnpackOperationState;
    if (!pOperationState || *pOperationState) return result;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSzddSFX> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return result;

    SZDDSFX_UNPACK_CONTEXT *pContext = static_cast<SZDDSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setSzddUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive ||
        (pContext->innerState.nCurrentIndex != 0) || (pContext->innerState.nNumberOfRecords != 1) ||
        (pState->nCurrentIndex >= pContext->listEntries.count()))
        return result;

    result = pContext->pArchive->infoCurrent(&pContext->innerState, pPdStruct);
    if (!guardedThis || !m_setSzddUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return ARCHIVERECORD();
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->listEntries.at(pState->nCurrentIndex).sName);
    return result;
}

bool XSzddSFX::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pSzddUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSzddSFX> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !guardedOutput || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;

    SZDDSFX_UNPACK_CONTEXT *pContext = static_cast<SZDDSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setSzddUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive ||
        (pContext->innerState.nCurrentIndex != 0) || (pContext->innerState.nNumberOfRecords != 1) ||
        (pState->nCurrentIndex >= pContext->listEntries.count()))
        return false;

    pContext->innerState.spOutputBudget = pState->spOutputBudget;
    const bool bResult = pContext->pArchive->unpackCurrent(&pContext->innerState, guardedOutput.data(), pPdStruct);
    if (!guardedThis || !guardedOutput || !m_setSzddUnpackContexts.contains(pContext) || (pState->pContext != pContext)) return false;
    pState->nCurrentOffset = pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset + pContext->innerState.nCurrentOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return bResult;
}

bool XSzddSFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QSharedPointer<bool> pOperationState = m_pSzddUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSzddSFX> guardedThis(this);
    if (!pState || !pState->baUnpackSourceToken.isEmpty() || !pState->pContext || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;

    SZDDSFX_UNPACK_CONTEXT *pContext = static_cast<SZDDSFX_UNPACK_CONTEXT *>(pState->pContext);
    if (!m_setSzddUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState) || !pContext->pOuterSourceDevice ||
        (pContext->pOuterSourceDevice != getDevice()) || (pContext->nOwnerDeviceGeneration != getDeviceGeneration()) || !pContext->pArchive ||
        (pState->nCurrentIndex >= pContext->listEntries.count()))
        return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        _releaseEntry(pContext);
        return false;
    }

    if (!guardedThis->_bindEntry(pContext, pState->nCurrentIndex, pPdStruct) || !guardedThis) return false;
    pState->nCurrentOffset = pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset;
    pState->mapArchiveProperties = pContext->innerState.mapArchiveProperties;
    return true;
}

bool XSzddSFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    if (!pState || !pState->baUnpackSourceToken.isEmpty()) return false;
    QSharedPointer<bool> pOperationState = m_pSzddUnpackOperationState;
    if (!pOperationState || *pOperationState) return false;
    QScopedValueRollback<bool> operationGuard(*pOperationState, true);
    QPointer<XSzddSFX> guardedThis(this);
    bool bResult = true;

    if (pState->pContext) {
        SZDDSFX_UNPACK_CONTEXT *pContext = static_cast<SZDDSFX_UNPACK_CONTEXT *>(pState->pContext);
        if (!m_setSzddUnpackContexts.contains(pContext) || (pContext->pOwnerState != pState)) return false;
        m_setSzddUnpackContexts.remove(pContext);
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
