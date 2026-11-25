/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xfatimage.h"

#include <QtEndian>

#include <memory>
#include <new>

// The rules are stated in xfatimage.h and are shared, step for step, with
// xxfclib (src/scan/xx_scan_floppy.c): change both sides together.

namespace {
const quint32 FAT_SECTOR_SIZE = 512;
const quint32 FAT_ENTRY_SIZE = 32;
const qint32 FAT_MAX_RECORDS = 10000;
const qint32 FAT_MAX_DEPTH = 8;
const qint32 FAT_MAX_DIR_BYTES = 0x200000;

quint32 fatU16(const uchar *pData)
{
    return qFromLittleEndian<quint16>(pData);
}

quint32 fatU32(const uchar *pData)
{
    return qFromLittleEndian<quint32>(pData);
}
}  // namespace

XFATImage::XFATImage(QIODevice *pDevice) : XArchive(pDevice)
{
}

XFATImage::~XFATImage()
{
}

// The BPB of a 512-byte boot sector and the layout that follows from it.
bool XFATImage::readGeometry(const uchar *pBoot, qint64 nImageSize, GEOMETRY *pGeometry)
{
    if (nImageSize < (qint64)FAT_SECTOR_SIZE) return false;
    if (fatU16(pBoot + 11) != FAT_SECTOR_SIZE) return false;
    pGeometry->nSectorsPerCluster = pBoot[13];
    if (!pGeometry->nSectorsPerCluster || (pGeometry->nSectorsPerCluster & (pGeometry->nSectorsPerCluster - 1)) || (pGeometry->nSectorsPerCluster > 64)) return false;
    pGeometry->nReservedSectors = fatU16(pBoot + 14);
    if (!pGeometry->nReservedSectors) pGeometry->nReservedSectors = 1;
    pGeometry->nNumberOfFats = pBoot[16];
    if ((pGeometry->nNumberOfFats != 1) && (pGeometry->nNumberOfFats != 2)) return false;
    pGeometry->nRootEntries = fatU16(pBoot + 17);
    if (!pGeometry->nRootEntries || (pGeometry->nRootEntries % 16)) return false;
    pGeometry->nTotalSectors = fatU16(pBoot + 19);
    if (!pGeometry->nTotalSectors) pGeometry->nTotalSectors = fatU32(pBoot + 32);
    pGeometry->nFatSectors = fatU16(pBoot + 22);
    if (!pGeometry->nFatSectors || !pGeometry->nTotalSectors) return false;

    const quint32 nMetaSectors = pGeometry->nReservedSectors + pGeometry->nNumberOfFats * pGeometry->nFatSectors + pGeometry->nRootEntries / 16;
    if (nMetaSectors >= pGeometry->nTotalSectors) return false;
    pGeometry->nClusters = (pGeometry->nTotalSectors - nMetaSectors) / pGeometry->nSectorsPerCluster;
    if (!pGeometry->nClusters || (pGeometry->nClusters >= 65525)) return false;
    pGeometry->bFat16 = (pGeometry->nClusters >= 4085);

    const quint64 nNeeded = pGeometry->bFat16 ? ((quint64)pGeometry->nClusters + 2) * 2 : (((quint64)pGeometry->nClusters + 2) * 3 + 1) / 2;
    if (nNeeded > (quint64)pGeometry->nFatSectors * FAT_SECTOR_SIZE) return false;
    pGeometry->nFatBytes = (quint32)nNeeded;
    pGeometry->nFatOffset = (qint64)pGeometry->nReservedSectors * FAT_SECTOR_SIZE;
    pGeometry->nRootOffset = (qint64)(pGeometry->nReservedSectors + pGeometry->nNumberOfFats * pGeometry->nFatSectors) * FAT_SECTOR_SIZE;
    pGeometry->nDataOffset = (qint64)nMetaSectors * FAT_SECTOR_SIZE;
    pGeometry->nClusterSize = pGeometry->nSectorsPerCluster * FAT_SECTOR_SIZE;

    // The BPB volume is 1/2 .. 5/4 of the image, and cluster 2 is inside it.
    const qint64 nVolumeSize = (qint64)pGeometry->nTotalSectors * FAT_SECTOR_SIZE;
    if ((nVolumeSize > nImageSize + nImageSize / 4) || (nVolumeSize < nImageSize / 2)) return false;
    if (pGeometry->nDataOffset + (qint64)pGeometry->nClusterSize > nImageSize) return false;
    pGeometry->nImageSize = nImageSize;

    return true;
}

bool XFATImage::isDataCluster(const GEOMETRY &geometry, quint32 nCluster)
{
    return (nCluster >= 2) && (nCluster < geometry.nClusters + 2);
}

// Whether the whole cluster lies inside the image.
bool XFATImage::isClusterInImage(const GEOMETRY &geometry, quint32 nCluster)
{
    return geometry.nDataOffset + (qint64)(nCluster - 1) * geometry.nClusterSize <= geometry.nImageSize;
}

qint64 XFATImage::getClusterOffset(const GEOMETRY &geometry, quint32 nCluster)
{
    return geometry.nDataOffset + (qint64)(nCluster - 2) * geometry.nClusterSize;
}

bool XFATImage::isNameValid(const uchar *pEntry)
{
    for (qint32 i = 0; i < 11; i++) {
        const uchar nChar = pEntry[i];
        if ((i == 0) && (nChar == 0x05)) continue;
        if ((nChar < 0x20) || (nChar == 0x7F) || (nChar == '"') || (nChar == '*') || (nChar == '/') || (nChar == ':') || (nChar == '<') || (nChar == '>') ||
            (nChar == '?') || (nChar == '\\') || (nChar == '|')) {
            return false;
        }
    }

    return pEntry[0] != 0x20;
}

// One root entry: 2 the end marker, 0 not counted (deleted, long-name piece),
// 1 live, -1 impossible.
qint32 XFATImage::classifyEntry(const GEOMETRY &geometry, const uchar *pEntry)
{
    const uchar nAttributes = pEntry[11];

    if (!pEntry[0]) return 2;
    if (pEntry[0] == 0xE5) return 0;
    if ((nAttributes & 0x3F) == 0x0F) return 0;
    if (nAttributes & 0xC0) return -1;
    if ((nAttributes & 0x18) == 0x18) return -1;
    if (!isNameValid(pEntry)) return -1;
    if (nAttributes & 0x08) return 1;

    const quint32 nCluster = fatU16(pEntry + 26);
    const quint32 nSize = fatU32(pEntry + 28);

    if (nAttributes & 0x10) return isDataCluster(geometry, nCluster) ? 1 : -1;
    if (!nCluster) return nSize ? -1 : 1;
    if (!isDataCluster(geometry, nCluster)) return -1;
    if ((quint64)nSize > (quint64)geometry.nClusters * geometry.nClusterSize) return -1;

    return 1;
}

// The boot sector's BPB and a root directory that looks like one.
bool XFATImage::detect(GEOMETRY *pGeometry, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pGeometry || !guardedSource || !guardedSource->isOpen() || !guardedSource->isReadable() || guardedSource->isSequential() ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nImageSize = getSize();
    if (nImageSize < (qint64)FAT_SECTOR_SIZE) return false;

    const QByteArray baBoot = read_array_process(0, FAT_SECTOR_SIZE, pPdStruct);
    if (baBoot.size() != (qint32)FAT_SECTOR_SIZE) return false;

    GEOMETRY geometry = {};
    if (!readGeometry((const uchar *)baBoot.constData(), nImageSize, &geometry)) return false;

    const qint32 nRootSize = (qint32)(geometry.nRootEntries * FAT_ENTRY_SIZE);
    const QByteArray baRoot = read_array_process(geometry.nRootOffset, nRootSize, pPdStruct);
    if (baRoot.size() != nRootSize) return false;

    quint32 nLive = 0;
    quint32 nBad = 0;
    for (quint32 i = 0; i < geometry.nRootEntries; i++) {
        const qint32 nVerdict = classifyEntry(geometry, (const uchar *)baRoot.constData() + i * FAT_ENTRY_SIZE);
        if (nVerdict == 2) break;
        if (nVerdict == 1) nLive++;
        if (nVerdict < 0) nBad++;
    }
    if (!nLive || (nBad * 4 > nLive)) return false;

    *pGeometry = geometry;

    return XBinary::isPdStructNotCanceled(pPdStruct);
}

quint32 XFATImage::getFatEntry(const CONTEXT &context, quint32 nCluster)
{
    const uchar *pFat = (const uchar *)context.baFat.constData();

    if (context.geometry.bFat16) return fatU16(pFat + (qint64)nCluster * 2);

    const quint32 nValue = fatU16(pFat + nCluster + nCluster / 2);

    return (nCluster & 1) ? (nValue >> 4) : (nValue & 0xFFF);
}

// Whether the chain from nCluster holds nSize bytes inside the image, and
// whether those clusters follow one another.
bool XFATImage::isChainValid(const CONTEXT &context, quint32 nCluster, quint32 nSize, bool *pbContiguous)
{
    quint64 nNeeded = ((quint64)nSize + context.geometry.nClusterSize - 1) / context.geometry.nClusterSize;
    *pbContiguous = true;

    while (nNeeded--) {
        if (!isDataCluster(context.geometry, nCluster) || !isClusterInImage(context.geometry, nCluster)) return false;
        const quint32 nNext = getFatEntry(context, nCluster);
        if (nNeeded && (nNext != nCluster + 1)) *pbContiguous = false;
        nCluster = nNext;
    }

    return true;
}

// pData[nSize] right-trimmed of spaces; bytes outside 0x20..0x7E and the path
// separators become '_'.
QString XFATImage::getNamePart(const uchar *pData, qint32 nSize)
{
    QString sResult;

    while (nSize && (pData[nSize - 1] == ' ')) nSize--;
    for (qint32 i = 0; i < nSize; i++) {
        const uchar nChar = pData[i];
        sResult += ((nChar >= 0x20) && (nChar < 0x7F) && (nChar != '/') && (nChar != '\\')) ? QLatin1Char((char)nChar) : QLatin1Char('_');
    }

    return sResult;
}

QString XFATImage::getShortName(const uchar *pEntry)
{
    uchar baseName[8];
    for (qint32 i = 0; i < 8; i++) baseName[i] = pEntry[i];
    if (baseName[0] == 0x05) baseName[0] = 0xE5;

    QString sResult = getNamePart(baseName, 8);
    const QString sExtension = getNamePart(pEntry + 8, 3);
    if (!sExtension.isEmpty()) sResult += QLatin1Char('.') + sExtension;

    return sResult;
}

// The clusters of a directory, each read once for the whole walk.
QByteArray XFATImage::readDirectory(CONTEXT *pContext, quint32 nCluster, PDSTRUCT *pPdStruct)
{
    const GEOMETRY &geometry = pContext->geometry;
    QByteArray baResult;
    char *pSeen = pContext->baSeen.data();

    while (isDataCluster(geometry, nCluster) && !(pSeen[nCluster >> 3] & (1 << (nCluster & 7))) && isClusterInImage(geometry, nCluster) &&
           (baResult.size() < FAT_MAX_DIR_BYTES)) {
        pSeen[nCluster >> 3] = (char)(pSeen[nCluster >> 3] | (1 << (nCluster & 7)));
        const QByteArray baCluster = read_array_process(getClusterOffset(geometry, nCluster), geometry.nClusterSize, pPdStruct);
        if (baCluster.size() != (qint32)geometry.nClusterSize) break;
        baResult.append(baCluster);
        nCluster = getFatEntry(*pContext, nCluster);
    }

    return baResult;
}

void XFATImage::walkEntries(CONTEXT *pContext, const QByteArray &baEntries, const QString &sPrefix, qint32 nDepth, PDSTRUCT *pPdStruct)
{
    const GEOMETRY &geometry = pContext->geometry;
    const qint32 nSize = baEntries.size();

    for (qint32 nPosition = 0; nPosition + (qint32)FAT_ENTRY_SIZE <= nSize; nPosition += FAT_ENTRY_SIZE) {
        const uchar *pEntry = (const uchar *)baEntries.constData() + nPosition;
        const uchar nAttributes = pEntry[11];

        if (pContext->bStop || !XBinary::isPdStructNotCanceled(pPdStruct)) return;
        if (!pEntry[0]) return;
        if ((pEntry[0] == 0xE5) || (pEntry[0] == 0x2E)) continue;
        if (((nAttributes & 0x3F) == 0x0F) || (nAttributes & 0x08)) continue;

        const quint32 nCluster = fatU16(pEntry + 26);
        const quint32 nFileSize = fatU32(pEntry + 28);
        const QString sPath = sPrefix + getShortName(pEntry);

        if (nAttributes & 0x10) {
            if (nDepth >= FAT_MAX_DEPTH) continue;
            const QByteArray baDirectory = readDirectory(pContext, nCluster, pPdStruct);
            walkEntries(pContext, baDirectory, sPath + QLatin1Char('/'), nDepth + 1, pPdStruct);
            continue;
        }

        if (!nFileSize || ((quint64)nFileSize > (quint64)geometry.nClusters * geometry.nClusterSize)) continue;

        bool bContiguous = true;
        if (!isChainValid(*pContext, nCluster, nFileSize, &bContiguous)) continue;
        if ((pContext->nListedSize + nFileSize > pContext->nListedLimit) || (pContext->listMembers.count() >= FAT_MAX_RECORDS)) {
            pContext->bStop = true;
            return;
        }
        pContext->nListedSize += nFileSize;

        MEMBER member = {};
        member.sPath = sPath;
        member.nOffset = getClusterOffset(geometry, nCluster);
        member.nSize = nFileSize;
        member.nCluster = nCluster;
        member.bContiguous = bContiguous;
        pContext->listMembers.append(member);
    }
}

bool XFATImage::parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pContext || !detect(&pContext->geometry, pPdStruct) || !guardedSource) return false;

    const GEOMETRY &geometry = pContext->geometry;

    pContext->nListedSize = 0;
    pContext->nListedLimit = (quint64)geometry.nClusters * geometry.nClusterSize * 2;
    pContext->bStop = false;
    pContext->listMembers.clear();

    pContext->baFat = read_array_process(geometry.nFatOffset, geometry.nFatBytes, pPdStruct);
    if (pContext->baFat.size() != (qint32)geometry.nFatBytes) return false;
    // One byte past the last entry: a FAT12 entry is read as two bytes.
    pContext->baFat.append('\0');
    pContext->baSeen = QByteArray((qint32)((geometry.nClusters + 2) / 8 + 1), '\0');

    const qint32 nRootSize = (qint32)(geometry.nRootEntries * FAT_ENTRY_SIZE);
    const QByteArray baRoot = read_array_process(geometry.nRootOffset, nRootSize, pPdStruct);
    if (baRoot.size() != nRootSize) return false;

    walkEntries(pContext, baRoot, QString(), 0, pPdStruct);

    return guardedSource && XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XFATImage::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    const qint64 nSavedPosition = guardedSource->pos();
    GEOMETRY geometry = {};
    const bool bResult = detect(&geometry, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XFATImage::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFATImage archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XFATImage::getFileType()
{
    return FT_FAT;
}

XBinary::MODE XFATImage::getMode()
{
    return MODE_DATA;
}

QString XFATImage::getMIMEString()
{
    return QStringLiteral("application/x-raw-disk-image");
}

QString XFATImage::getFileFormatExt()
{
    return QStringLiteral("img");
}

QString XFATImage::getFileFormatExtsString()
{
    return QStringLiteral("FAT12/FAT16 floppy disk image (*.st *.img *.ima *.dsk)");
}

QList<QString> XFATImage::getSearchSignatures()
{
    // No magic: the BPB and the root directory identify the image.
    return QList<QString>();
}

XBinary *XFATImage::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFATImage(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XFATImage::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XFATImage::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseImage(pContext, pPdStruct) || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.count();
    pState->nCurrentOffset = pContext->listMembers.isEmpty() ? pContext->geometry.nImageSize : pContext->listMembers.constFirst().nOffset;
    pState->nTotalSize = pContext->geometry.nImageSize;
    pState->pContext = pContext;

    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XFATImage::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->geometry.nImageSize) || (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    if (member.sPath.isEmpty() || (member.nOffset < pContext->geometry.nDataOffset)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sPath);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)member.nSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)member.nSize);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    if (member.bContiguous) {
        // A stored extent of the image.
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    } else {
        // Scattered clusters: no extent, only unpackCurrent() assembles it.
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Store"));
        if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    }

    return result;
}

bool XFATImage::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedOutput || !guardedSource || !isUnpackOutputSupported(guardedOutput) ||
        devicesAlias(guardedSource, guardedOutput) || !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->geometry.nImageSize) || (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    const GEOMETRY &geometry = pContext->geometry;
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    const qint64 nMemberSize = member.nSize;

    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, nMemberSize)) {
        XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, member.sPath)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(nMemberSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(nMemberSize, pPdStruct));
    if (!pStage || !pStage->seek(0) || !guardedOutput || !guardedSource) return false;

    quint32 nCluster = member.nCluster;
    qint64 nRemaining = nMemberSize;
    while (nRemaining > 0) {
        const qint64 nAmount = qMin(nRemaining, (qint64)geometry.nClusterSize);
        if (!guardedOutput || !guardedSource || !isDataCluster(geometry, nCluster) || !isClusterInImage(geometry, nCluster) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const QByteArray baCluster = read_array_process(getClusterOffset(geometry, nCluster), nAmount, pPdStruct);
        if (!guardedOutput || !guardedSource || (baCluster.size() != nAmount) || !isUnpackSourceCurrent(pState, pPdStruct)) return false;

        qint64 nWritten = 0;
        while (nWritten < nAmount) {
            const qint64 nWrite = pStage->write(baCluster.constData() + nWritten, nAmount - nWritten);
            if ((nWrite <= 0) || (nWrite > nAmount - nWritten)) return false;
            nWritten += nWrite;
        }

        nRemaining -= nAmount;
        nCluster = getFatEntry(*pContext, nCluster);
    }

    if ((pStage->size() != nMemberSize) || !pStage->seek(0) || !guardedOutput || !guardedSource || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
    if (bResult) pState->nCurrentOffset = nMemberSize;

    return bResult;
}

bool XFATImage::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->geometry.nImageSize) || (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->geometry.nImageSize;

    return false;
}

bool XFATImage::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XFATImage::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
