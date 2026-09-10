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
#include "xvmdkarchive.h"

#include <QMap>
#include <QPointer>
#include <QSet>
#include <QVector>
#include <QtEndian>

#include <new>

namespace {
const qint64 VMDK_SECTOR = 512;
const qint64 VMDK_HEADER_SIZE = 512;
const qint64 VMDK_MAX_GD_ENTRIES = 1 << 22;
const qint64 VMDK_MAX_GTES = 1 << 20;
const qint64 VMDK_MAX_GRAIN_SECTORS = 1 << 16;
const qint64 VMDK_MAX_FAT_BYTES = 64 * 1024 * 1024;
const qint64 VMDK_MAX_GT_CACHE_BYTES = 64 * 1024 * 1024;
const qint64 VMDK_MAX_DIRECTORY_BYTES = 16 * 1024 * 1024;
const qint32 VMDK_MAX_MEMBERS = 200000;
const qint32 VMDK_MAX_RUNS = 200000;
const qint32 VMDK_MAX_DEPTH = 32;
const qint32 VMDK_MAX_MBR_DEPTH = 8;
const qint64 VMDK_HOLE = (qint64)-1;

const quint32 VMDK_MAGIC = 0x564d444b;  // 'KDMV'
const quint32 VMDK_RUN_MAGIC = 0x52444d56;  // 'VMDR'

bool vmdkIsExtendedType(quint8 nType)
{
    return (nType == 0x05) || (nType == 0x0f) || (nType == 0x85);
}

// A virtual byte range on the flat disk the sparse extent presents.
struct RANGE {
    qint64 nOffset;
    qint64 nSize;
};

// The sparse extent, read through the archive's own device so no part of the
// image has to be held in memory.
class SparseExtent {
public:
    SparseExtent(XVMDKArchive *pArchive, XBinary::PDSTRUCT *pPdStruct);

    bool open(const QByteArray &baHeader, qint64 nFileSize);
    qint64 getCapacity() const;
    qint64 getVersion() const;
    qint64 getGrainBytes() const;
    qint64 getFileSize() const;
    // File offset backing a virtual offset, or VMDK_HOLE when unallocated.
    qint64 mapOffset(qint64 nVirtualOffset);
    QByteArray read(qint64 nVirtualOffset, qint64 nSize);
    bool isAlive() const;

private:
    // Copies the entry out rather than handing back a pointer into the cache:
    // no interior pointer can then outlive the next insert.
    bool grainSector(qint64 nDirectoryIndex, qint64 nGrainIndex, quint32 *pnSector);

    XVMDKArchive *m_pArchive;
    XBinary::PDSTRUCT *m_pPdStruct;
    qint64 m_nFileSize;
    qint64 m_nVersion;
    qint64 m_nCapacity;
    qint64 m_nGrainSectors;
    qint64 m_nGrainBytes;
    qint64 m_nGTEs;
    qint64 m_nMaxCachedTables;
    QVector<quint32> m_listGD;
    QMap<quint32, QVector<quint32> > m_mapGT;
    bool m_bAlive;
};

SparseExtent::SparseExtent(XVMDKArchive *pArchive, XBinary::PDSTRUCT *pPdStruct)
{
    m_pArchive = pArchive;
    m_pPdStruct = pPdStruct;
    m_nFileSize = 0;
    m_nVersion = 0;
    m_nCapacity = 0;
    m_nGrainSectors = 0;
    m_nGrainBytes = 0;
    m_nGTEs = 0;
    m_nMaxCachedTables = 1;
    m_bAlive = true;
}

bool SparseExtent::isAlive() const
{
    return m_bAlive;
}

qint64 SparseExtent::getCapacity() const
{
    return m_nCapacity;
}

qint64 SparseExtent::getVersion() const
{
    return m_nVersion;
}

qint64 SparseExtent::getGrainBytes() const
{
    return m_nGrainBytes;
}

qint64 SparseExtent::getFileSize() const
{
    return m_nFileSize;
}

bool SparseExtent::open(const QByteArray &baHeader, qint64 nFileSize)
{
    if (baHeader.size() < VMDK_HEADER_SIZE) return false;
    const quint8 *pHeader = (const quint8 *)baHeader.constData();
    if (qFromLittleEndian<quint32>(pHeader) != VMDK_MAGIC) return false;

    m_nFileSize = nFileSize;
    m_nVersion = (qint64)qFromLittleEndian<quint32>(pHeader + 4);
    m_nCapacity = (qint64)qFromLittleEndian<quint64>(pHeader + 0x0c);
    m_nGrainSectors = (qint64)qFromLittleEndian<quint64>(pHeader + 0x14);
    m_nGTEs = (qint64)qFromLittleEndian<quint32>(pHeader + 0x2c);
    const qint64 nGDSector = (qint64)qFromLittleEndian<quint64>(pHeader + 0x38);
    const quint16 nCompress = qFromLittleEndian<quint16>(pHeader + 0x4d);

    // Only an uncompressed extent can be presented as a flat disk; the
    // deflate variant is a stream-optimized image with a different layout.
    if (nCompress > 1) return false;
    // Every one of these came out of the file, so none of them may be trusted
    // to be positive, in range, or free of overflow when multiplied.
    if ((m_nGrainSectors <= 0) || (m_nGrainSectors > VMDK_MAX_GRAIN_SECTORS)) return false;
    if ((m_nGTEs <= 0) || (m_nGTEs > VMDK_MAX_GTES)) return false;
    if ((m_nCapacity <= 0) || (m_nCapacity > ((qint64)1 << 40))) return false;
    if ((nFileSize < VMDK_HEADER_SIZE) || (nGDSector <= 0) || (nGDSector > (nFileSize / VMDK_SECTOR))) return false;

    m_nGrainBytes = m_nGrainSectors * VMDK_SECTOR;
    const qint64 nSpan = m_nGrainSectors * m_nGTEs;
    if (nSpan <= 0) return false;
    const qint64 nEntries = (m_nCapacity + nSpan - 1) / nSpan;
    if ((nEntries <= 0) || (nEntries > VMDK_MAX_GD_ENTRIES)) return false;

    const qint64 nGDOffset = nGDSector * VMDK_SECTOR;
    const qint64 nGDBytes = nEntries * 4;
    if ((nGDOffset < 0) || (nGDOffset > nFileSize) || (nGDBytes > (nFileSize - nGDOffset))) return false;

    const QByteArray baGD = m_pArchive->read_array_process(nGDOffset, nGDBytes, m_pPdStruct);
    if (baGD.size() != nGDBytes) return false;
    m_listGD.fill(0, (qint32)nEntries);
    const quint8 *pGD = (const quint8 *)baGD.constData();
    for (qint64 i = 0; i < nEntries; i++) {
        m_listGD[(qint32)i] = qFromLittleEndian<quint32>(pGD + i * 4);
    }

    // A hostile numGTEsPerGT would otherwise let the grain-table cache grow
    // without bound; keep the whole cache inside a fixed budget instead.
    m_nMaxCachedTables = VMDK_MAX_GT_CACHE_BYTES / (m_nGTEs * 4);
    if (m_nMaxCachedTables < 1) m_nMaxCachedTables = 1;

    return true;
}

bool SparseExtent::grainSector(qint64 nDirectoryIndex, qint64 nGrainIndex, quint32 *pnSector)
{
    *pnSector = 0;
    if ((nDirectoryIndex < 0) || (nDirectoryIndex >= m_listGD.size())) return false;
    if ((nGrainIndex < 0) || (nGrainIndex >= m_nGTEs)) return false;
    const quint32 nTableSector = m_listGD.at((qint32)nDirectoryIndex);
    // A zero at the directory level means the whole span is unallocated.
    if (nTableSector == 0) return false;

    if (!m_mapGT.contains(nTableSector)) {
        QVector<quint32> listTable;
        const qint64 nOffset = (qint64)nTableSector * VMDK_SECTOR;
        const qint64 nBytes = m_nGTEs * 4;
        if ((nOffset > 0) && (nOffset <= m_nFileSize) && (nBytes <= (m_nFileSize - nOffset))) {
            const QByteArray baTable = m_pArchive->read_array_process(nOffset, nBytes, m_pPdStruct);
            if (baTable.size() == nBytes) {
                listTable.fill(0, (qint32)m_nGTEs);
                const quint8 *pTable = (const quint8 *)baTable.constData();
                for (qint64 i = 0; i < m_nGTEs; i++) {
                    listTable[(qint32)i] = qFromLittleEndian<quint32>(pTable + i * 4);
                }
            }
        }
        if (m_mapGT.size() >= m_nMaxCachedTables) m_mapGT.clear();
        m_mapGT.insert(nTableSector, listTable);
    }

    const QVector<quint32> &listTable = m_mapGT[nTableSector];
    // An unreadable table is cached empty so it is not re-read on every grain.
    if (listTable.size() != (qint32)m_nGTEs) return false;
    *pnSector = listTable.at((qint32)nGrainIndex);

    return (*pnSector != 0);
}

qint64 SparseExtent::mapOffset(qint64 nVirtualOffset)
{
    if ((nVirtualOffset < 0) || (m_nGrainBytes <= 0) || (m_nGTEs <= 0)) return VMDK_HOLE;
    const qint64 nGrain = nVirtualOffset / m_nGrainBytes;
    const qint64 nInGrain = nVirtualOffset % m_nGrainBytes;
    quint32 nSector = 0;
    // A zero at either level means unallocated, which reads as zeros.
    if (!grainSector(nGrain / m_nGTEs, nGrain % m_nGTEs, &nSector)) return VMDK_HOLE;
    const qint64 nOffset = (qint64)nSector * VMDK_SECTOR + nInGrain;
    if (nOffset < 0) return VMDK_HOLE;

    return nOffset;
}

QByteArray SparseExtent::read(qint64 nVirtualOffset, qint64 nSize)
{
    QByteArray baResult;
    if ((nVirtualOffset < 0) || (nSize < 0) || (nSize > VMDK_MAX_DIRECTORY_BYTES) || (m_nGrainBytes <= 0)) return baResult;
    baResult.reserve((qint32)nSize);
    qint64 nOffset = nVirtualOffset;
    qint64 nLeft = nSize;
    while (nLeft > 0) {
        if (!XBinary::isPdStructNotCanceled(m_pPdStruct)) {
            m_bAlive = false;
            return QByteArray();
        }
        const qint64 nInGrain = nOffset % m_nGrainBytes;
        const qint64 nTake = qMin<qint64>(m_nGrainBytes - nInGrain, nLeft);
        const qint64 nFileOffset = mapOffset(nOffset);
        if (nFileOffset == VMDK_HOLE) {
            baResult.append((qint32)nTake, (char)0);
        } else {
            QByteArray baChunk = m_pArchive->read_array_process(nFileOffset, nTake, m_pPdStruct);
            if (baChunk.size() < nTake) baChunk.append((qint32)(nTake - baChunk.size()), (char)0);
            baResult.append(baChunk);
        }
        nOffset += nTake;
        nLeft -= nTake;
    }

    return baResult;
}

// A parsed FAT volume.  rd() multiplies by bytes-per-sector, not by 512: that
// is what the reference does, and every reference image has 512-byte sectors
// so the two agree there.
struct FatVolume {
    // A constructor, NOT a memset: this struct holds a QByteArray, and zeroing
    // its bytes destroys Qt's shared-data pointer, so the first assignment to
    // baFAT dereferences null.
    FatVolume()
    {
        nLBA = 0;
        nBPS = 0;
        nSPC = 0;
        nReserved = 0;
        nFATs = 0;
        nRootEntries = 0;
        nTotal = 0;
        nSPF = 0;
        nRootCluster = 0;
        nRootSectors = 0;
        nFirstData = 0;
        nClusterLimit = 0;
        nBits = 0;
    }

    qint64 nLBA;
    qint64 nBPS;
    qint64 nSPC;
    qint64 nReserved;
    qint64 nFATs;
    qint64 nRootEntries;
    qint64 nTotal;
    qint64 nSPF;
    qint64 nRootCluster;
    qint64 nRootSectors;
    qint64 nFirstData;
    qint64 nClusterLimit;
    qint32 nBits;
    QByteArray baFAT;
};

qint64 fatSectorOffset(const FatVolume &volume, qint64 nSector)
{
    if ((volume.nBPS <= 0) || (nSector < 0)) return -1;
    return (volume.nLBA + nSector) * volume.nBPS;
}

bool fatOpen(SparseExtent *pExtent, qint64 nLBA, FatVolume *pVolume)
{
    if (!pVolume || (nLBA < 0)) return false;
    FatVolume volume;
    volume.nLBA = nLBA;

    const QByteArray baBoot = pExtent->read(nLBA * VMDK_SECTOR, VMDK_SECTOR);
    if (baBoot.size() != VMDK_SECTOR) return false;
    const quint8 *pBoot = (const quint8 *)baBoot.constData();

    volume.nBPS = (qint64)qFromLittleEndian<quint16>(pBoot + 0x0b);
    volume.nSPC = pBoot[0x0d];
    volume.nReserved = (qint64)qFromLittleEndian<quint16>(pBoot + 0x0e);
    volume.nFATs = pBoot[0x10];
    volume.nRootEntries = (qint64)qFromLittleEndian<quint16>(pBoot + 0x11);
    const qint64 nTotal16 = (qint64)qFromLittleEndian<quint16>(pBoot + 0x13);
    volume.nSPF = (qint64)qFromLittleEndian<quint16>(pBoot + 0x16);
    const qint64 nTotal32 = (qint64)qFromLittleEndian<quint32>(pBoot + 0x20);

    if ((volume.nBPS != 512) && (volume.nBPS != 1024) && (volume.nBPS != 2048) && (volume.nBPS != 4096)) return false;
    if ((volume.nSPC == 0) || (volume.nFATs == 0) || (volume.nReserved == 0)) return false;

    volume.nTotal = nTotal16 ? nTotal16 : nTotal32;
    if (volume.nSPF == 0) {
        volume.nSPF = (qint64)qFromLittleEndian<quint32>(pBoot + 0x24);
        volume.nRootCluster = (qint64)qFromLittleEndian<quint32>(pBoot + 0x2c);
    } else {
        volume.nRootCluster = 0;
    }
    if ((volume.nSPF == 0) || (volume.nTotal == 0)) return false;
    // sectors-per-FAT is a raw u32 from the boot sector: bound it before it is
    // multiplied by the FAT count and the sector size.
    if (volume.nSPF > (VMDK_MAX_FAT_BYTES / volume.nBPS)) return false;

    volume.nRootSectors = (volume.nRootEntries * 32 + volume.nBPS - 1) / volume.nBPS;
    volume.nFirstData = volume.nReserved + volume.nFATs * volume.nSPF + volume.nRootSectors;
    if (volume.nFirstData < 0) return false;
    const qint64 nClusters = (volume.nTotal - volume.nFirstData) / volume.nSPC;
    if (nClusters < 1) return false;
    volume.nClusterLimit = nClusters + 2;
    if (nClusters < 4085) {
        volume.nBits = 12;
    } else if (nClusters < 65525) {
        volume.nBits = 16;
    } else {
        volume.nBits = 32;
    }

    const qint64 nFatBytes = volume.nSPF * volume.nBPS;
    if ((nFatBytes <= 0) || (nFatBytes > VMDK_MAX_FAT_BYTES)) return false;
    volume.baFAT = pExtent->read(fatSectorOffset(volume, volume.nReserved), nFatBytes);
    if (volume.baFAT.size() != nFatBytes) return false;

    *pVolume = volume;

    return true;
}

qint64 fatNextCluster(const FatVolume &volume, qint64 nCluster)
{
    const quint8 *pFat = (const quint8 *)volume.baFAT.constData();
    const qint64 nFatSize = volume.baFAT.size();

    if (volume.nBits == 12) {
        const qint64 nIndex = nCluster + (nCluster >> 1);
        if ((nIndex < 0) || ((nIndex + 1) >= nFatSize)) return 0xfff;
        const qint64 nValue = pFat[nIndex] | ((qint64)pFat[nIndex + 1] << 8);
        return (nCluster & 1) ? (nValue >> 4) : (nValue & 0xfff);
    }
    if (volume.nBits == 16) {
        const qint64 nIndex = nCluster * 2;
        if ((nIndex < 0) || ((nIndex + 1) >= nFatSize)) return 0xffff;
        return (qint64)qFromLittleEndian<quint16>(pFat + nIndex);
    }
    const qint64 nIndex = nCluster * 4;
    if ((nIndex < 0) || ((nIndex + 3) >= nFatSize)) return 0x0fffffff;

    return (qint64)(qFromLittleEndian<quint32>(pFat + nIndex) & 0x0fffffff);
}

bool fatIsEndOfChain(const FatVolume &volume, qint64 nCluster)
{
    if (nCluster < 2) return true;
    if (volume.nBits == 12) return (nCluster >= 0xff8);
    if (volume.nBits == 16) return (nCluster >= 0xfff8);

    return (nCluster >= 0x0ffffff8);
}

// The cluster chain as virtual byte ranges, adjacent clusters merged.
bool fatChain(const FatVolume &volume, qint64 nFirstCluster, qint64 nMaxBytes, QList<RANGE> *pListRanges, qint64 *pnTotal, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pListRanges) return false;
    QSet<qint64> setSeen;
    qint64 nCluster = nFirstCluster;
    qint64 nTotal = 0;
    const qint64 nClusterBytes = volume.nSPC * volume.nBPS;
    if (nClusterBytes <= 0) return false;

    while ((!fatIsEndOfChain(volume, nCluster)) && (nCluster < volume.nClusterLimit)) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        if (setSeen.contains(nCluster)) break;
        setSeen.insert(nCluster);
        if (setSeen.size() > VMDK_MAX_RUNS) return false;

        const qint64 nSector = volume.nFirstData + (nCluster - 2) * volume.nSPC;
        const qint64 nOffset = fatSectorOffset(volume, nSector);
        if (nOffset < 0) return false;

        if ((!pListRanges->isEmpty()) && ((pListRanges->last().nOffset + pListRanges->last().nSize) == nOffset)) {
            (*pListRanges)[pListRanges->size() - 1].nSize += nClusterBytes;
        } else {
            RANGE range = {};
            range.nOffset = nOffset;
            range.nSize = nClusterBytes;
            pListRanges->append(range);
        }
        nTotal += nClusterBytes;
        if ((nMaxBytes >= 0) && (nTotal >= nMaxBytes)) break;
        nCluster = fatNextCluster(volume, nCluster);
    }

    if (pnTotal) *pnTotal = nTotal;

    return true;
}

QByteArray fatReadRanges(SparseExtent *pExtent, const QList<RANGE> &listRanges, qint64 nMaxBytes)
{
    QByteArray baResult;
    for (qint32 i = 0; i < listRanges.count(); i++) {
        if ((nMaxBytes >= 0) && ((qint64)baResult.size() >= nMaxBytes)) break;
        qint64 nTake = listRanges.at(i).nSize;
        if (nMaxBytes >= 0) nTake = qMin<qint64>(nTake, nMaxBytes - (qint64)baResult.size());
        if ((qint64)baResult.size() + nTake > VMDK_MAX_DIRECTORY_BYTES) return baResult;
        const QByteArray baChunk = pExtent->read(listRanges.at(i).nOffset, nTake);
        if (baChunk.size() != nTake) return baResult;
        baResult.append(baChunk);
    }

    return baResult;
}

QString fatName83(const quint8 *pRaw)
{
    QString sBase;
    QString sExtension;
    for (qint32 i = 0; i < 8; i++) sBase.append(QChar((ushort)pRaw[i]));
    for (qint32 i = 8; i < 11; i++) sExtension.append(QChar((ushort)pRaw[i]));
    while ((!sBase.isEmpty()) && (sBase.at(sBase.size() - 1) == QChar(' '))) sBase.chop(1);
    while ((!sExtension.isEmpty()) && (sExtension.at(sExtension.size() - 1) == QChar(' '))) sExtension.chop(1);
    if (!sExtension.isEmpty()) return sBase + QChar('.') + sExtension;

    return sBase;
}
}  // namespace

const qint64 XVMDKDecoder::MAX_UNCOMPRESSED_SIZE;
const qint32 XVMDKDecoder::RUN_SIZE;
const qint32 XVMDKDecoder::DESCRIPTOR_HEADER_SIZE;

QByteArray XVMDKDecoder::startDescriptor()
{
    QByteArray baResult(DESCRIPTOR_HEADER_SIZE, (char)0);
    qToLittleEndian<quint32>(VMDK_RUN_MAGIC, (quint8 *)baResult.data());

    return baResult;
}

bool XVMDKDecoder::appendRun(QByteArray *pbaDescriptor, qint64 nRelativeOffset, qint64 nSize)
{
    if (!pbaDescriptor) return false;
    if (pbaDescriptor->size() < DESCRIPTOR_HEADER_SIZE) return false;
    if (nSize <= 0) return false;
    if (pbaDescriptor->size() > ((qint32)0x7fffffff - RUN_SIZE)) return false;

    QByteArray baRun(RUN_SIZE, (char)0);
    quint8 *pRun = (quint8 *)baRun.data();
    qToLittleEndian<quint64>((nRelativeOffset < 0) ? (quint64)0xffffffffffffffffULL : (quint64)nRelativeOffset, pRun);
    qToLittleEndian<quint64>((quint64)nSize, pRun + 8);
    pbaDescriptor->append(baRun);
    qToLittleEndian<quint32>((quint32)((pbaDescriptor->size() - DESCRIPTOR_HEADER_SIZE) / RUN_SIZE), (quint8 *)pbaDescriptor->data() + 4);

    return true;
}

void XVMDKDecoder::setSize(QByteArray *pbaDescriptor, qint64 nSize)
{
    if (!pbaDescriptor || (pbaDescriptor->size() < DESCRIPTOR_HEADER_SIZE)) return;
    qToLittleEndian<quint64>((quint64)nSize, (quint8 *)pbaDescriptor->data() + 8);
}

bool XVMDKDecoder::decode(const QByteArray &baPacked, const QByteArray &baProperties, qint64 nUncompressedSize, QByteArray *pbaResult,
                          XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    if ((nUncompressedSize < 0) || (nUncompressedSize > MAX_UNCOMPRESSED_SIZE)) return false;
    if (baProperties.size() < DESCRIPTOR_HEADER_SIZE) return false;

    const quint8 *pDescriptor = (const quint8 *)baProperties.constData();
    if (qFromLittleEndian<quint32>(pDescriptor) != VMDK_RUN_MAGIC) return false;
    const qint64 nRunCount = (qint64)qFromLittleEndian<quint32>(pDescriptor + 4);
    const qint64 nDeclaredSize = (qint64)qFromLittleEndian<quint64>(pDescriptor + 8);
    if (nDeclaredSize != nUncompressedSize) return false;
    if ((nRunCount < 0) || (baProperties.size() < (DESCRIPTOR_HEADER_SIZE + nRunCount * RUN_SIZE))) return false;

    QByteArray baResult;
    baResult.reserve((qint32)qMin<qint64>(nUncompressedSize + 1, 0x400000));

    for (qint64 i = 0; i < nRunCount; i++) {
        if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
        const quint8 *pRun = pDescriptor + DESCRIPTOR_HEADER_SIZE + i * RUN_SIZE;
        const quint64 nRawOffset = qFromLittleEndian<quint64>(pRun);
        const qint64 nSize = (qint64)qFromLittleEndian<quint64>(pRun + 8);
        if ((nSize <= 0) || (nSize > nUncompressedSize)) return false;
        if (((qint64)baResult.size() + nSize) > nUncompressedSize) return false;

        if (nRawOffset == 0xffffffffffffffffULL) {
            // A hole: unallocated grains read as zeros.
            baResult.append((qint32)nSize, (char)0);
            continue;
        }
        const qint64 nOffset = (qint64)nRawOffset;
        if ((nOffset < 0) || (nOffset > baPacked.size()) || (nSize > ((qint64)baPacked.size() - nOffset))) return false;
        baResult.append(baPacked.constData() + nOffset, (qint32)nSize);
    }

    if ((qint64)baResult.size() != nUncompressedSize) return false;
    *pbaResult = baResult;

    return true;
}

XVMDKArchive::XVMDKArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XVMDKArchive::~XVMDKArchive()
{
}

namespace {
// One directory, walked exactly as the reference does.
struct WalkContext {
    SparseExtent *pExtent;
    const FatVolume *pVolume;
    qint32 nPartition;
    QSet<qint64> setDirectories;
    QList<XVMDKArchive::MEMBER> *pListMembers;
    XBinary::PDSTRUCT *pPdStruct;
};

bool fatBuildMember(WalkContext *pContext, const QString &sName, qint64 nCluster, qint64 nSize, XVMDKArchive::MEMBER *pMember)
{
    XVMDKArchive::MEMBER member = {};
    member.sFileName = sName;
    member.nUncompressedSize = nSize;
    member.baDescriptor = XVMDKDecoder::startDescriptor();
    XVMDKDecoder::setSize(&member.baDescriptor, nSize);

    if ((nSize == 0) || (nCluster < 2)) {
        member.nSpanOffset = 0;
        member.nSpanSize = 0;
        member.nHeaderOffset = 0;
        *pMember = member;
        return true;
    }

    QList<RANGE> listRanges;
    qint64 nTotal = 0;
    if (!fatChain(*(pContext->pVolume), nCluster, nSize, &listRanges, &nTotal, pContext->pPdStruct)) return false;

    // Resolve the virtual ranges to file ranges, merging what is adjacent, and
    // learn the span the record has to publish.  The walk steps one grain at a
    // time because a virtual range can straddle allocated and unallocated
    // grains, and only the allocated part occupies bytes in the file.
    QList<RANGE> listRuns;
    qint64 nProduced = 0;
    qint64 nMinOffset = -1;
    qint64 nMaxEnd = -1;
    const qint64 nGrainBytes = pContext->pExtent->getGrainBytes();
    const qint64 nFileSize = pContext->pExtent->getFileSize();
    if ((nGrainBytes <= 0) || (nFileSize <= 0)) return false;

    for (qint32 i = 0; (i < listRanges.count()) && (nProduced < nSize); i++) {
        qint64 nOffset = listRanges.at(i).nOffset;
        qint64 nLeft = qMin<qint64>(listRanges.at(i).nSize, nSize - nProduced);
        if (nOffset < 0) return false;
        while (nLeft > 0) {
            if (!XBinary::isPdStructNotCanceled(pContext->pPdStruct)) return false;
            const qint64 nInGrain = nOffset % nGrainBytes;
            const qint64 nTake = qMin<qint64>(nGrainBytes - nInGrain, nLeft);
            const qint64 nMapped = pContext->pExtent->mapOffset(nOffset);
            if (nTake <= 0) return false;

            // Only the part of the grain that is actually present in the file
            // is a run; anything past the end reads as zeros, which is what a
            // short read gives the reference.
            qint64 nPresent = 0;
            if ((nMapped != VMDK_HOLE) && (nMapped < nFileSize)) {
                nPresent = qMin<qint64>(nTake, nFileSize - nMapped);
            }

            for (qint32 nPart = 0; nPart < 2; nPart++) {
                const bool bHole = (nPart != 0);
                const qint64 nPartSize = bHole ? (nTake - nPresent) : nPresent;
                if (nPartSize <= 0) continue;
                const qint64 nPartOffset = bHole ? VMDK_HOLE : nMapped;

                if ((!listRuns.isEmpty()) && (!bHole) && (listRuns.last().nOffset != VMDK_HOLE) &&
                    ((listRuns.last().nOffset + listRuns.last().nSize) == nPartOffset)) {
                    listRuns[listRuns.size() - 1].nSize += nPartSize;
                } else if ((!listRuns.isEmpty()) && bHole && (listRuns.last().nOffset == VMDK_HOLE)) {
                    listRuns[listRuns.size() - 1].nSize += nPartSize;
                } else {
                    if (listRuns.count() >= VMDK_MAX_RUNS) return false;
                    RANGE run = {};
                    run.nOffset = nPartOffset;
                    run.nSize = nPartSize;
                    listRuns.append(run);
                }
                if (!bHole) {
                    if ((nMinOffset < 0) || (nPartOffset < nMinOffset)) nMinOffset = nPartOffset;
                    if ((nMaxEnd < 0) || ((nPartOffset + nPartSize) > nMaxEnd)) nMaxEnd = nPartOffset + nPartSize;
                }
            }

            nOffset += nTake;
            nLeft -= nTake;
            nProduced += nTake;
        }
    }

    if (nProduced < nSize) {
        // The chain ran out early; the reference emits the short content.
        member.nUncompressedSize = nProduced;
        XVMDKDecoder::setSize(&member.baDescriptor, nProduced);
    }

    member.nSpanOffset = (nMinOffset < 0) ? 0 : nMinOffset;
    member.nSpanSize = (nMaxEnd < 0) ? 0 : (nMaxEnd - member.nSpanOffset);
    member.nHeaderOffset = member.nSpanOffset;

    for (qint32 i = 0; i < listRuns.count(); i++) {
        const qint64 nRunOffset = (listRuns.at(i).nOffset == VMDK_HOLE) ? VMDK_HOLE : (listRuns.at(i).nOffset - member.nSpanOffset);
        if (!XVMDKDecoder::appendRun(&member.baDescriptor, nRunOffset, listRuns.at(i).nSize)) return false;
    }

    *pMember = member;

    return true;
}

bool fatWalk(WalkContext *pContext, const QByteArray &baDirectory, const QString &sPrefix, qint32 nDepth)
{
    if (nDepth > VMDK_MAX_DEPTH) return true;

    for (qint32 nOffset = 0; (nOffset + 32) <= baDirectory.size(); nOffset += 32) {
        if (!XBinary::isPdStructNotCanceled(pContext->pPdStruct)) return false;
        if (pContext->pListMembers->size() >= VMDK_MAX_MEMBERS) return true;
        const quint8 *pEntry = (const quint8 *)baDirectory.constData() + nOffset;
        if (pEntry[0] == 0x00) break;
        if (pEntry[0] == 0xe5) continue;
        const quint8 nAttributes = pEntry[0x0b];
        if (nAttributes == 0x0f) continue;
        if (nAttributes & 0x08) continue;
        if (pEntry[0] == (quint8)'.') continue;

        qint64 nCluster = (qint64)qFromLittleEndian<quint16>(pEntry + 0x1a);
        if (pContext->pVolume->nBits == 32) nCluster |= ((qint64)qFromLittleEndian<quint16>(pEntry + 0x14)) << 16;
        const qint64 nSize = (qint64)qFromLittleEndian<quint32>(pEntry + 0x1c);
        const QString sName = sPrefix + QChar('/') + fatName83(pEntry);

        if (nAttributes & 0x10) {
            if (nCluster < 2) continue;
            if (pContext->setDirectories.contains(nCluster)) continue;
            pContext->setDirectories.insert(nCluster);
            QList<RANGE> listRanges;
            qint64 nTotal = 0;
            if (!fatChain(*(pContext->pVolume), nCluster, -1, &listRanges, &nTotal, pContext->pPdStruct)) return false;
            const QByteArray baChild = fatReadRanges(pContext->pExtent, listRanges, -1);
            if (!pContext->pExtent->isAlive()) return false;
            if (!fatWalk(pContext, baChild, sName, nDepth + 1)) return false;
            continue;
        }

        XVMDKArchive::MEMBER member = {};
        if (!fatBuildMember(pContext, QString("FAT.Partition.%1").arg(pContext->nPartition) + sName, nCluster, nSize, &member)) return false;
        pContext->pListMembers->append(member);
    }

    return true;
}

bool vmdkPartitions(SparseExtent *pExtent, qint64 nBase, qint64 nEbrBase, bool bHasEbrBase, qint32 nDepth, QList<qint64> *pListStarts)
{
    if (!pExtent || !pListStarts || (nDepth > VMDK_MAX_MBR_DEPTH) || (nBase < 0)) return true;
    const QByteArray baMBR = pExtent->read(nBase * VMDK_SECTOR, VMDK_SECTOR);
    if (baMBR.size() != VMDK_SECTOR) return true;
    const quint8 *pMBR = (const quint8 *)baMBR.constData();
    if ((pMBR[510] != 0x55) || (pMBR[511] != 0xaa)) return true;

    for (qint32 i = 0; i < 4; i++) {
        const quint8 *pEntry = pMBR + 0x1be + i * 16;
        const quint8 nType = pEntry[4];
        const qint64 nStart = (qint64)qFromLittleEndian<quint32>(pEntry + 8);
        const qint64 nCount = (qint64)qFromLittleEndian<quint32>(pEntry + 12);
        if ((nType == 0) || (nCount == 0)) continue;
        if (vmdkIsExtendedType(nType)) {
            const qint64 nNextBase = nDepth ? ((bHasEbrBase ? nEbrBase : 0) + nStart) : nStart;
            if (!vmdkPartitions(pExtent, nNextBase, bHasEbrBase ? nEbrBase : nStart, true, nDepth + 1, pListStarts)) return false;
        } else {
            if (pListStarts->count() > 128) return true;
            pListStarts->append(nBase + nStart);
        }
    }

    return true;
}
}  // namespace

bool XVMDKArchive::parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XVMDKArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < VMDK_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, VMDK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != VMDK_HEADER_SIZE)) return false;

    SparseExtent extent(this, pPdStruct);
    if (!extent.open(baHeader, context.nInputSize)) return false;
    if (!guardedThis || !guardedSource) return false;
    context.nVersion = extent.getVersion();
    context.nCapacity = extent.getCapacity();

    if (!bFull) {
        *pContext = context;
        return true;
    }

    QList<qint64> listStarts;
    if (!vmdkPartitions(&extent, 0, 0, false, 0, &listStarts)) return false;
    if (!guardedThis || !guardedSource || !extent.isAlive()) return false;
    // No partition table at all: the reference offers the whole disk to the
    // filesystem probe.
    if (listStarts.isEmpty()) listStarts.append(0);

    for (qint32 i = 0; i < listStarts.count(); i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        FatVolume volume;
        if (!fatOpen(&extent, listStarts.at(i), &volume)) continue;
        if (!guardedThis || !guardedSource || !extent.isAlive()) return false;

        QList<RANGE> listRootRanges;
        QByteArray baRoot;
        if (volume.nBits == 32) {
            qint64 nTotal = 0;
            if (!fatChain(volume, volume.nRootCluster, -1, &listRootRanges, &nTotal, pPdStruct)) return false;
            baRoot = fatReadRanges(&extent, listRootRanges, -1);
        } else {
            const qint64 nRootBytes = volume.nRootSectors * volume.nBPS;
            if ((nRootBytes <= 0) || (nRootBytes > VMDK_MAX_DIRECTORY_BYTES)) continue;
            baRoot = extent.read(fatSectorOffset(volume, volume.nReserved + volume.nFATs * volume.nSPF), nRootBytes);
        }
        if (!guardedThis || !guardedSource || !extent.isAlive()) return false;

        WalkContext walkContext;
        walkContext.pExtent = &extent;
        walkContext.pVolume = &volume;
        walkContext.nPartition = i + 1;
        walkContext.pListMembers = &context.listMembers;
        walkContext.pPdStruct = pPdStruct;
        if (!fatWalk(&walkContext, baRoot, QString(), 0)) return false;
        if (!guardedThis || !guardedSource) return false;
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;

    return true;
}

bool XVMDKArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XVMDKArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XVMDKArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XVMDKArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XVMDKArchive(pDevice);
}

QList<QString> XVMDKArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'KDMV'");
}

XBinary::FT XVMDKArchive::getFileType()
{
    return FT_VMDK;
}

XBinary::MODE XVMDKArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XVMDKArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XVMDKArchive::getArch()
{
    return QString();
}

qint32 XVMDKArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XVMDKArchive::getFileFormatExt()
{
    return QStringLiteral("vmdk");
}

QString XVMDKArchive::getFileFormatExtsString()
{
    return QStringLiteral("VMware disk image (*.vmdk)");
}

QString XVMDKArchive::getMIMEString()
{
    return QStringLiteral("application/x-vmdk");
}

QString XVMDKArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();

    return QString::number(context.nVersion);
}

qint64 XVMDKArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XVMDKArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XVMDKArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XVMDKArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XVMDKArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = VMDK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nSpanOffset;
            part.nFileSize = member.nSpanSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSpanSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE : HANDLE_METHOD_VMDK);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("FAT"));
            if (member.nUncompressedSize != 0) part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baDescriptor);
            listResult.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XVMDKArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XVMDKArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XVMDKArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedThis || !guardedSource || !bFinalized) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XVMDKArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nSpanOffset;
    result.nStreamSize = member.nSpanSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSpanSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE : HANDLE_METHOD_VMDK);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("FAT"));
    if (member.nUncompressedSize != 0) result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baDescriptor);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XVMDKArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XVMDKArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XVMDKArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
