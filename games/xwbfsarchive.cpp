/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native archive reader for Nintendo Wii Backup File System images.
 */
#include "xwbfsarchive.h"

#include <QSet>

#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {
constexpr qint64 WBFS_HEADER_FIXED_SIZE = 12;
constexpr qint64 WBFS_DISC_HEADER_SIZE = 0x100;
constexpr qint64 WBFS_WII_SECTOR_SIZE = 0x8000;
constexpr quint8 WBFS_WII_SECTOR_SHIFT = 15;
constexpr qint64 WBFS_WII_SECTORS_PER_DISC = 143432LL * 2;
constexpr quint32 WBFS_WII_DISC_MAGIC = 0x5D1C9EA3U;
constexpr quint8 WBFS_MIN_HD_SECTOR_SHIFT = 9;
constexpr quint8 WBFS_MAX_HD_SECTOR_SHIFT = 16;
constexpr quint8 WBFS_MIN_BLOCK_SHIFT = 18;
constexpr quint8 WBFS_MAX_BLOCK_SHIFT = 31;
constexpr qint64 WBFS_MAX_DISCS = 4096;
constexpr qint64 WBFS_COPY_BUFFER_SIZE = 0x10000;

bool wbfsRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) &&
           (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool wbfsCheckedMultiply(qint64 nLeft, qint64 nRight, qint64 *pnResult)
{
    if (!pnResult || (nLeft < 0) || (nRight < 0)) return false;
    if ((nLeft != 0) &&
        (nRight > ((std::numeric_limits<qint64>::max)() / nLeft))) {
        return false;
    }
    *pnResult = nLeft * nRight;
    return true;
}

bool wbfsCheckedAdd(qint64 nLeft, qint64 nRight, qint64 *pnResult)
{
    if (!pnResult || (nLeft < 0) || (nRight < 0) ||
        (nRight > ((std::numeric_limits<qint64>::max)() - nLeft))) {
        return false;
    }
    *pnResult = nLeft + nRight;
    return true;
}

bool wbfsShiftToSize(quint8 nShift, qint64 *pnResult)
{
    if (!pnResult || (nShift >= 63)) return false;
    const quint64 nValue = ((quint64)1) << nShift;
    if (nValue > (quint64)(std::numeric_limits<qint64>::max)())
        return false;
    *pnResult = (qint64)nValue;
    return true;
}

bool wbfsAlignUp(qint64 nValue, qint64 nAlignment, qint64 *pnResult)
{
    if (!pnResult || (nValue < 0) || (nAlignment <= 0)) return false;
    const qint64 nRemainder = nValue % nAlignment;
    if (nRemainder == 0) {
        *pnResult = nValue;
        return true;
    }
    return wbfsCheckedAdd(nValue, nAlignment - nRemainder, pnResult);
}

quint16 wbfsReadBE16(const uchar *pData)
{
    return (quint16)(((quint16)pData[0] << 8) | (quint16)pData[1]);
}

quint32 wbfsReadBE32(const uchar *pData)
{
    return ((quint32)pData[0] << 24) | ((quint32)pData[1] << 16) |
           ((quint32)pData[2] << 8) | (quint32)pData[3];
}

bool wbfsDiscId(const uchar *pData, QString *psResult)
{
    if (!pData || !psResult) return false;
    QByteArray baId;
    baId.reserve(6);
    for (qint32 i = 0; i < 6; ++i) {
        const uchar nValue = pData[i];
        const bool bAlphaNumeric = ((nValue >= '0') && (nValue <= '9')) ||
                                   ((nValue >= 'A') && (nValue <= 'Z')) ||
                                   ((nValue >= 'a') && (nValue <= 'z'));
        if (!bAlphaNumeric) return false;
        baId.append((char)nValue);
    }
    *psResult = QString::fromLatin1(baId);
    return true;
}

QString wbfsDiscTitle(const uchar *pData)
{
    if (!pData) return QString();
    qint32 nLength = 0;
    while ((nLength < 64) && pData[nLength]) ++nLength;
    return QString::fromLatin1(reinterpret_cast<const char *>(pData),
                               nLength)
        .trimmed();
}
}  // namespace

XWBFSArchive::XWBFSArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWBFSArchive::~XWBFSArchive()
{
}

bool XWBFSArchive::isValid(PDSTRUCT *pPdStruct)
{
    UNPACK_STATE state = {};
    const bool bResult = initUnpack(&state, QMap<UNPACK_PROP, QVariant>(),
                                    pPdStruct);
    const bool bFinished = finishUnpack(&state, nullptr);
    return bResult && bFinished;
}

bool XWBFSArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWBFSArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XWBFSArchive::getFileType()
{
    return FT_WBFS;
}

XBinary::MODE XWBFSArchive::getMode()
{
    return MODE_DATA;
}

QString XWBFSArchive::getMIMEString()
{
    return QStringLiteral("application/x-wbfs");
}

QString XWBFSArchive::getFileFormatExt()
{
    return QStringLiteral("wbfs");
}

QString XWBFSArchive::getFileFormatExtsString()
{
    return QStringLiteral("Nintendo Wii WBFS image (*.wbfs)");
}

QList<QString> XWBFSArchive::getSearchSignatures()
{
    return {QStringLiteral("'WBFS'")};
}

XBinary *XWBFSArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWBFSArchive(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant>
XWBFSArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XWBFSArchive::scanArchive(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !getDevice() ||
        getDevice()->isSequential() || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nSourceSize = getSize();
    if ((nSourceSize < WBFS_HEADER_FIXED_SIZE)) return false;
    const QByteArray baFixedHeader = read_array_process(
        0, WBFS_HEADER_FIXED_SIZE, pPdStruct);
    if ((baFixedHeader.size() != WBFS_HEADER_FIXED_SIZE) ||
        !isPdStructNotCanceled(pPdStruct) ||
        (memcmp(baFixedHeader.constData(), "WBFS", 4) != 0)) {
        return false;
    }

    const uchar *pFixedHeader = reinterpret_cast<const uchar *>(
        baFixedHeader.constData());
    const quint32 nHDSectors = wbfsReadBE32(pFixedHeader + 4);
    const quint8 nHDSectorShift = pFixedHeader[8];
    const quint8 nWbfsBlockShift = pFixedHeader[9];
    if ((nHDSectors == 0) || (nHDSectorShift < WBFS_MIN_HD_SECTOR_SHIFT) ||
        (nHDSectorShift > WBFS_MAX_HD_SECTOR_SHIFT) ||
        (nWbfsBlockShift < WBFS_MIN_BLOCK_SHIFT) ||
        (nWbfsBlockShift > WBFS_MAX_BLOCK_SHIFT) ||
        (nWbfsBlockShift < WBFS_WII_SECTOR_SHIFT)) {
        return false;
    }

    qint64 nHDSectorSize = 0;
    qint64 nWbfsBlockSize = 0;
    qint64 nDeclaredSize = 0;
    if (!wbfsShiftToSize(nHDSectorShift, &nHDSectorSize) ||
        !wbfsShiftToSize(nWbfsBlockShift, &nWbfsBlockSize) ||
        !wbfsCheckedMultiply((qint64)nHDSectors, nHDSectorSize,
                             &nDeclaredSize) ||
        (nDeclaredSize != nSourceSize) ||
        (nHDSectorSize < WBFS_HEADER_FIXED_SIZE) ||
        !wbfsRangeWithin(nSourceSize, 0, nHDSectorSize)) {
        return false;
    }

    // libwbfs represents the disc lookup table in physical WBFS blocks.  Its
    // geometry deliberately truncates the small tail which does not fill a
    // complete block; the reconstructed ISO retains that tail as zeroes.
    const qint32 nBlockDelta = (qint32)nWbfsBlockShift -
                                (qint32)WBFS_WII_SECTOR_SHIFT;
    if ((nBlockDelta < 0) || (nBlockDelta >= 63)) return false;
    const qint64 nWiiSectors =
        ((qint64)nHDSectors / WBFS_WII_SECTOR_SIZE) * nHDSectorSize;
    const qint64 nWbfsBlocks = nWiiSectors >> nBlockDelta;
    const qint64 nMapEntries = WBFS_WII_SECTORS_PER_DISC >> nBlockDelta;
    if ((nWbfsBlocks <= 1) || (nWbfsBlocks > 0xFFFF) ||
        (nMapEntries <= 0) || (nMapEntries > 0xFFFF)) {
        return false;
    }

    qint64 nMapBytes = 0;
    qint64 nRawDiscInfoSize = 0;
    qint64 nDiscInfoSize = 0;
    if (!wbfsCheckedMultiply(nMapEntries, 2, &nMapBytes) ||
        !wbfsCheckedAdd(WBFS_DISC_HEADER_SIZE, nMapBytes,
                        &nRawDiscInfoSize) ||
        !wbfsAlignUp(nRawDiscInfoSize, nHDSectorSize, &nDiscInfoSize) ||
        (nDiscInfoSize <= 0) || ((nDiscInfoSize % nHDSectorSize) != 0)) {
        return false;
    }

    const qint64 nDiscInfoSectors = nDiscInfoSize / nHDSectorSize;
    const qint64 nFreeBlockLBA =
        (nWbfsBlockSize - (nWbfsBlocks / 8)) >> nHDSectorShift;
    if ((nDiscInfoSectors <= 0) || (nFreeBlockLBA <= 1)) return false;
    qint64 nMaxDiscSlots = (nFreeBlockLBA - 1) / nDiscInfoSectors;
    const qint64 nTableSlots = nHDSectorSize - WBFS_HEADER_FIXED_SIZE;
    nMaxDiscSlots = qMin(nMaxDiscSlots, nTableSlots);
    if ((nTableSlots <= 0) || (nMaxDiscSlots < 0)) return false;

    const QByteArray baHeader = read_array_process(0, nHDSectorSize,
                                                    pPdStruct);
    if ((baHeader.size() != nHDSectorSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const uchar *pHeader = reinterpret_cast<const uchar *>(
        baHeader.constData());
    QSet<quint16> setMappedBlocks;
    QSet<QString> setFileNames;
    qint64 nArchiveEnd = nHDSectorSize;
    qint64 nDiscCount = 0;

    for (qint64 i = 0; i < nTableSlots; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (!pHeader[WBFS_HEADER_FIXED_SIZE + i]) continue;
        if ((i >= nMaxDiscSlots) || (++nDiscCount > WBFS_MAX_DISCS)) {
            return false;
        }

        qint64 nSlotOffset = 0;
        qint64 nDiscInfoOffset = 0;
        if (!wbfsCheckedMultiply(i, nDiscInfoSize, &nSlotOffset) ||
            !wbfsCheckedAdd(nHDSectorSize, nSlotOffset, &nDiscInfoOffset) ||
            !wbfsRangeWithin(nSourceSize, nDiscInfoOffset, nDiscInfoSize)) {
            return false;
        }
        const QByteArray baDiscInfo = read_array_process(
            nDiscInfoOffset, nDiscInfoSize, pPdStruct);
        if ((baDiscInfo.size() != nDiscInfoSize) ||
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const uchar *pDiscInfo = reinterpret_cast<const uchar *>(
            baDiscInfo.constData());
        QString sGameId;
        if (!wbfsDiscId(pDiscInfo, &sGameId) ||
            (wbfsReadBE32(pDiscInfo + 0x18) != WBFS_WII_DISC_MAGIC)) {
            return false;
        }

        const QByteArray baBlockMap = baDiscInfo.mid(WBFS_DISC_HEADER_SIZE,
                                                      nMapBytes);
        if (baBlockMap.size() != nMapBytes ||
            (wbfsReadBE16(reinterpret_cast<const uchar *>(
                baBlockMap.constData())) == 0)) {
            return false;
        }

        qint64 nMappedBlocks = 0;
        qint64 nHighestMappedIndex = -1;
        for (qint64 j = 0; j < nMapEntries; ++j) {
            const quint16 nPhysicalBlock = wbfsReadBE16(
                reinterpret_cast<const uchar *>(baBlockMap.constData()) +
                (j * 2));
            if (!nPhysicalBlock) continue;
            qint64 nPhysicalOffset = 0;
            qint64 nPhysicalEnd = 0;
            if (((qint64)nPhysicalBlock >= nWbfsBlocks) ||
                setMappedBlocks.contains(nPhysicalBlock) ||
                !wbfsCheckedMultiply((qint64)nPhysicalBlock,
                                     nWbfsBlockSize, &nPhysicalOffset) ||
                !wbfsCheckedAdd(nPhysicalOffset, nWbfsBlockSize,
                                &nPhysicalEnd) ||
                !wbfsRangeWithin(nSourceSize, nPhysicalOffset,
                                 nWbfsBlockSize)) {
                return false;
            }
            setMappedBlocks.insert(nPhysicalBlock);
            ++nMappedBlocks;
            nHighestMappedIndex = j;
            nArchiveEnd = qMax(nArchiveEnd, nPhysicalEnd);
        }
        if (nMappedBlocks <= 0) return false;

        // A single-layer disc occupies the first half of the lookup table.
        // An upper-half mapping makes it a dual-layer output.  Report the
        // conventional Wii disc length, including a final unmapped partial
        // block when the block-map geometry rounds down.
        const qint64 nSingleLayerBlocks = nMapEntries / 2;
        if ((nSingleLayerBlocks <= 0) ||
            (nHighestMappedIndex >= nMapEntries)) {
            return false;
        }
        qint64 nVirtualSize = 0;
        qint64 nMappedSize = 0;
        const qint64 nWiiSectorsInOutput =
            (nHighestMappedIndex >= nSingleLayerBlocks)
                ? WBFS_WII_SECTORS_PER_DISC
                : (WBFS_WII_SECTORS_PER_DISC / 2);
        if (!wbfsCheckedMultiply(nWiiSectorsInOutput,
                                 WBFS_WII_SECTOR_SIZE, &nVirtualSize) ||
            !wbfsCheckedMultiply(nMappedBlocks, nWbfsBlockSize,
                                 &nMappedSize) ||
            (nVirtualSize <= 0)) {
            return false;
        }

        const QString sBaseFileName = fixFileName(sGameId);
        if (sBaseFileName.isEmpty()) return false;
        QString sFileName = sBaseFileName;
        qint64 nSuffix = i + 1;
        while (setFileNames.contains(sFileName)) {
            sFileName = sBaseFileName + QStringLiteral("-%1").arg(nSuffix++);
        }
        setFileNames.insert(sFileName);

        ENTRY entry = {};
        entry.nDiscInfoOffset = nDiscInfoOffset;
        entry.nDiscInfoSize = nDiscInfoSize;
        entry.nWbfsBlockSize = nWbfsBlockSize;
        entry.nMapEntries = nMapEntries;
        entry.nMappedSize = nMappedSize;
        entry.nVirtualSize = nVirtualSize;
        entry.baBlockMap = baBlockMap;
        entry.sFileName = sFileName + QStringLiteral(".iso");
        entry.sGameId = sGameId;
        entry.sGameTitle = wbfsDiscTitle(pDiscInfo + 0x20);
        pContext->listEntries.append(entry);
        nArchiveEnd = qMax(nArchiveEnd, nDiscInfoOffset + nDiscInfoSize);
    }

    if (!wbfsRangeWithin(nSourceSize, 0, nArchiveEnd)) return false;
    pContext->nHeaderSize = nHDSectorSize;
    pContext->nArchiveEnd = nArchiveEnd;
    pContext->nSourceSize = nSourceSize;
    pContext->nWbfsBlockSize = nWbfsBlockSize;
    pContext->nMapEntries = nMapEntries;
    pContext->nDiscInfoSize = nDiscInfoSize;
    return isPdStructNotCanceled(pPdStruct);
}

bool XWBFSArchive::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct))
        return false;

    OUTPUT_POLICY policy = {};
    CONTEXT *pContext = nullptr;
    bool bResult = false;
    if (!resolveUnpackOutputPolicy(mapProperties, &policy)) goto failed;
    pContext = new (std::nothrow) CONTEXT;
    if (!pContext || !scanArchive(pContext, pPdStruct) ||
        !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        goto failed;
    }
    if ((pContext->nSourceSize != guardedSource->size()) ||
        !wbfsRangeWithin(pContext->nSourceSize, 0,
                         pContext->nArchiveEnd)) {
        goto failed;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.count();
    pState->nCurrentOffset = pContext->listEntries.isEmpty()
        ? pContext->nHeaderSize
        : pContext->listEntries.constFirst().nDiscInfoOffset;
    pState->nTotalSize = pContext->nSourceSize;
    pState->pContext = pContext;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        QStringLiteral("WBFS: %1 embedded Wii disc%2")
            .arg(pContext->listEntries.count())
            .arg((pContext->listEntries.count() == 1) ? QString() :
                 QStringLiteral("s")));
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct))
        goto failed;
    bResult = true;

failed:
    if (!bResult) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
    }
    return bResult;
}

XBinary::ARCHIVERECORD XWBFSArchive::infoCurrent(UNPACK_STATE *pState,
                                                  PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if ((nCurrentSize != pContext->nSourceSize) ||
        (pState->nTotalSize != nCurrentSize) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return ARCHIVERECORD();
    }
    const ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (!wbfsRangeWithin(nCurrentSize, entry.nDiscInfoOffset,
                         entry.nDiscInfoSize) ||
        (entry.nWbfsBlockSize != pContext->nWbfsBlockSize) ||
        (entry.nMapEntries != pContext->nMapEntries) ||
        (entry.baBlockMap.size() != (entry.nMapEntries * 2)) ||
        (entry.nVirtualSize <= 0) || (entry.sFileName.isEmpty())) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nDiscInfoOffset;
    result.nStreamSize = entry.nDiscInfoSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                entry.nVirtualSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                entry.nMappedSize);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("WBFS virtual Wii disc"));
    result.mapProperties.insert(FPART_PROP_FILEMODE, (quint32)0644);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return markArchiveStreamRecord(&result, pState->nCurrentIndex)
        ? result
        : ARCHIVERECORD();
}

bool XWBFSArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                  PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    QIODevice *guardedOutput = pDevice;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !guardedSource ||
        !guardedOutput || !pState->pContext ||
        devicesAlias(guardedSource, guardedOutput) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    const qint64 nCurrentSize = getSize();
    if ((nCurrentSize != pContext->nSourceSize) ||
        (pState->nTotalSize != nCurrentSize) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pContext->listEntries.count())) {
        return false;
    }
    const ENTRY entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (!wbfsRangeWithin(nCurrentSize, entry.nDiscInfoOffset,
                         entry.nDiscInfoSize) ||
        (entry.nWbfsBlockSize != pContext->nWbfsBlockSize) ||
        (entry.nMapEntries != pContext->nMapEntries) ||
        (entry.baBlockMap.size() != (entry.nMapEntries * 2)) ||
        (entry.nVirtualSize <= 0) ||
        !isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                   entry.nVirtualSize)) {
        if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                       entry.nVirtualSize)) {
            setPdStructErrorString(
                pPdStruct,
                tr("Unpacked output exceeds the configured limit"));
        }
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                 entry.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(entry.nVirtualSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(
                    pPdStruct,
                    tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    // createFileBuffer selects a temporary file for this multi-gigabyte
    // logical image.  Its zero-initialized sparse extent is the deliberate
    // representation of unmapped WBFS blocks; only mapped blocks are copied.
    std::unique_ptr<QIODevice> pStage(createFileBuffer(entry.nVirtualSize,
                                                        pPdStruct));
    if (!pStage || (pStage->size() != entry.nVirtualSize) ||
        !pStage->seek(0) || !guardedSource ||
        !guardedOutput || !isUnpackSourceCurrent(pState, pPdStruct)) {
        return false;
    }
    QByteArray baBuffer((qint32)WBFS_COPY_BUFFER_SIZE, '\0');
    if (baBuffer.size() != WBFS_COPY_BUFFER_SIZE) return false;

    qint64 nMapBlocksToProcess = entry.nVirtualSize / entry.nWbfsBlockSize;
    if ((entry.nVirtualSize % entry.nWbfsBlockSize) != 0)
        ++nMapBlocksToProcess;
    nMapBlocksToProcess = qMin(nMapBlocksToProcess, entry.nMapEntries);
    if (nMapBlocksToProcess <= 0)
        return false;
    const uchar *pMap = reinterpret_cast<const uchar *>(
        entry.baBlockMap.constData());

    for (qint64 i = 0; i < nMapBlocksToProcess; ++i) {
        if (!guardedSource || !guardedOutput ||
            !isUnpackSourceCurrent(pState, pPdStruct) ||
            !isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        const quint16 nPhysicalBlock = wbfsReadBE16(pMap + (i * 2));
        if (!nPhysicalBlock) continue;

        qint64 nPhysicalOffset = 0;
        qint64 nLogicalOffset = 0;
        if (!wbfsCheckedMultiply((qint64)nPhysicalBlock,
                                 entry.nWbfsBlockSize, &nPhysicalOffset) ||
            !wbfsCheckedMultiply(i, entry.nWbfsBlockSize,
                                 &nLogicalOffset) ||
            !wbfsRangeWithin(nCurrentSize, nPhysicalOffset,
                             entry.nWbfsBlockSize) ||
            (nLogicalOffset >= entry.nVirtualSize)) {
            return false;
        }

        const qint64 nBlockOutputSize = qMin(
            entry.nWbfsBlockSize, entry.nVirtualSize - nLogicalOffset);
        qint64 nCopied = 0;
        while (nCopied < nBlockOutputSize) {
            if (!guardedSource || !guardedOutput ||
                !isUnpackSourceCurrent(pState, pPdStruct) ||
                !isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
            const qint64 nChunkSize = qMin<qint64>(
                baBuffer.size(), nBlockOutputSize - nCopied);
            if ((read_array_process(nPhysicalOffset + nCopied,
                                    baBuffer.data(), nChunkSize,
                                    pPdStruct) != nChunkSize) || !guardedSource || !guardedOutput ||
                (safeWriteData(pStage.get(), nLogicalOffset + nCopied,
                               baBuffer.constData(), nChunkSize,
                               pPdStruct) != nChunkSize)) {
                return false;
            }
            nCopied += nChunkSize;
        }
    }

    if (!guardedSource || !guardedOutput ||
        (pStage->size() != entry.nVirtualSize) || !pStage->seek(0) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const bool bPublished = publishUnpackOutput(pStage.get(),
                                                 guardedOutput, pState,
                                                 pPdStruct);
    if (bPublished) {
        pState->nCurrentOffset = entry.nDiscInfoOffset +
                                 entry.nDiscInfoSize;
    }
    return bPublished;
}

bool XWBFSArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if ((pState->nTotalSize != pContext->nSourceSize) ||
        (pState->nNumberOfRecords != pContext->listEntries.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listEntries.at(
            pState->nCurrentIndex).nDiscInfoOffset;
        return true;
    }
    pState->nCurrentOffset = pState->nTotalSize;
    return false;
}

bool XWBFSArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();
    return true;
}

QList<XBinary::FPART_PROP> XWBFSArchive::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_UNCOMPRESSEDSIZE,
            FPART_PROP_COMPRESSEDSIZE, FPART_PROP_REPORTEDMETHOD,
            FPART_PROP_FILEMODE, FPART_PROP_HANDLEMETHOD};
}
