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
#include "xinstallshield.h"

#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <limits>

namespace {
const quint32 ISHIELD_CAB_SIGNATURE = 0x28635349U;  // "ISc(" little endian.
const qint64 ISHIELD_COMMON_HEADER_SIZE = 20;
const qint64 ISHIELD_VOLUME_HEADER_V5_SIZE = 40;
const qint64 ISHIELD_VOLUME_HEADER_V6_SIZE = 64;
const qint64 ISHIELD_MAX_CATALOG_SIZE = 256LL * 1024 * 1024;
const qint64 ISHIELD_SCAN_CHUNK = 4LL * 1024 * 1024;
const qint32 ISHIELD_MAX_BLOBS = 64;
const quint16 ISHIELD_PE_MACHINE_I386 = 0x014c;
const quint16 ISHIELD_PE_MACHINE_AMD64 = 0x8664;
const quint16 ISHIELD_PE_MACHINE_ARM64 = 0xaa64;

quint16 blobLE16(const QByteArray &baData, qint32 nOffset)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

quint32 blobLE32(const QByteArray &baData, qint32 nOffset)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

quint64 blobLE64(const QByteArray &baData, qint32 nOffset)
{
    return qFromLittleEndian<quint64>(reinterpret_cast<const uchar *>(baData.constData() + nOffset));
}

qint32 blobMajorVersion(quint32 nVersion)
{
    const quint8 nFamily = static_cast<quint8>(nVersion >> 24);
    if (nFamily == 1) return static_cast<qint32>((nVersion >> 12) & 0x0f);
    if ((nFamily == 2) || (nFamily == 4)) {
        const quint32 nValue = nVersion & 0xffffU;
        return nValue ? static_cast<qint32>(nValue / 100U) : 0;
    }
    return -1;
}

struct BLOB_CANDIDATE {
    quint64 nOffset = 0;
    quint32 nVersion = 0;
    quint32 nDescriptorOffset = 0;
    quint32 nDescriptorSize = 0;
    qint32 nMajorVersion = 0;
    QByteArray baHeader;
};

// A candidate is plausible when its common header parses with a sane version
// and, depending on the descriptor size, either a bounded catalog descriptor
// or a bounded volume data offset.  The InstallShield engine's own "ISc("
// compare immediates fail these checks (their trailing bytes decode to wild
// versions and offsets).
bool validateBlobHeader(const QByteArray &baHeader, quint64 nBlobOffset, qint64 nDeviceSize, BLOB_CANDIDATE *pCandidate)
{
    if (!pCandidate || (baHeader.size() < (ISHIELD_COMMON_HEADER_SIZE + ISHIELD_VOLUME_HEADER_V6_SIZE))) return false;
    if (blobLE32(baHeader, 0) != ISHIELD_CAB_SIGNATURE) return false;
    const quint32 nVersion = blobLE32(baHeader, 4);
    const qint32 nMajorVersion = blobMajorVersion(nVersion);
    if ((nMajorVersion < 0) || (nMajorVersion > 32)) return false;
    const quint32 nDescriptorOffset = blobLE32(baHeader, 12);
    const quint32 nDescriptorSize = blobLE32(baHeader, 16);
    const quint64 nMaxExtent = static_cast<quint64>(nDeviceSize) - nBlobOffset;

    if (nDescriptorSize) {
        if ((nDescriptorOffset < ISHIELD_COMMON_HEADER_SIZE) || (nDescriptorSize > ISHIELD_MAX_CATALOG_SIZE)) return false;
        if ((static_cast<quint64>(nDescriptorOffset) + 0x30) > nMaxExtent) return false;
    } else {
        const qint64 nVolumeHeaderSize = (nMajorVersion <= 5) ? ISHIELD_VOLUME_HEADER_V5_SIZE : ISHIELD_VOLUME_HEADER_V6_SIZE;
        const quint64 nDataOffset = (nMajorVersion <= 5) ? blobLE32(baHeader, ISHIELD_COMMON_HEADER_SIZE) : blobLE64(baHeader, ISHIELD_COMMON_HEADER_SIZE);
        if ((nDataOffset < static_cast<quint64>(ISHIELD_COMMON_HEADER_SIZE + nVolumeHeaderSize)) || (nDataOffset > nMaxExtent)) return false;
    }

    pCandidate->nOffset = nBlobOffset;
    pCandidate->nVersion = nVersion;
    pCandidate->nDescriptorOffset = nDescriptorOffset;
    pCandidate->nDescriptorSize = nDescriptorSize;
    pCandidate->nMajorVersion = nMajorVersion;
    pCandidate->baHeader = baHeader.left(ISHIELD_COMMON_HEADER_SIZE + ISHIELD_VOLUME_HEADER_V6_SIZE);
    return true;
}
}  // namespace

XInstallShield::XInstallShield(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress) : XISCab(pDevice)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
}

XInstallShield::MEDIA_LAYOUT XInstallShield::_scanMedia(PDSTRUCT *pPdStruct) const
{
    MEDIA_LAYOUT result;

    QPointer<XInstallShield> guardedThis(const_cast<XInstallShield *>(this));
    QPointer<QIODevice> guardedDevice(guardedThis ? guardedThis->getDevice() : nullptr);
    if (!guardedThis || !guardedDevice || !XBinary::isPdStructNotCanceled(pPdStruct)) return result;
    if (!guardedDevice->isOpen() || !guardedDevice->isReadable() || guardedDevice->isSequential()) return result;

    const qint64 nDeviceSize = guardedDevice->size();
    if (!guardedThis || !guardedDevice || (nDeviceSize < 0x200)) return result;
    const qint64 nSavedPosition = guardedDevice->pos();
    if (nSavedPosition < 0) return result;

    // The host must be a PE.  Parse MZ -> e_lfanew -> PE signature/machine
    // directly so the scan does not depend on the XPE reader.
    bool bHostValid = false;
    bool bIs64 = false;
    {
        if (!guardedDevice->seek(0)) return result;
        const QByteArray baDos = guardedDevice->read(0x40);
        if (guardedThis && guardedDevice && (baDos.size() == 0x40) && (blobLE16(baDos, 0) == 0x5a4d)) {
            const quint32 nPeOffset = blobLE32(baDos, 0x3c);
            if ((nPeOffset >= 0x40) && ((static_cast<qint64>(nPeOffset) + 6) <= nDeviceSize) && guardedDevice->seek(nPeOffset)) {
                const QByteArray baPe = guardedDevice->read(6);
                if (guardedThis && guardedDevice && (baPe.size() == 6) && (blobLE32(baPe, 0) == 0x00004550U)) {
                    const quint16 nMachine = blobLE16(baPe, 4);
                    if (nMachine == ISHIELD_PE_MACHINE_I386) {
                        bHostValid = true;
                    } else if ((nMachine == ISHIELD_PE_MACHINE_AMD64) || (nMachine == ISHIELD_PE_MACHINE_ARM64)) {
                        bHostValid = true;
                        bIs64 = true;
                    }
                }
            }
        }
    }

    QList<BLOB_CANDIDATE> listBlobs;
    if (bHostValid) {
        const QByteArray baNeedle("ISc(", 4);
        qint64 nChunkStart = 0;
        while ((nChunkStart < nDeviceSize) && (listBlobs.count() < ISHIELD_MAX_BLOBS) && XBinary::isPdStructNotCanceled(pPdStruct)) {
            const qint64 nChunkSize = qMin<qint64>(ISHIELD_SCAN_CHUNK, nDeviceSize - nChunkStart);
            if (!guardedDevice->seek(nChunkStart)) break;
            const QByteArray baChunk = guardedDevice->read(nChunkSize);
            if (!guardedThis || !guardedDevice || (baChunk.size() != nChunkSize)) break;
            qint32 nSearchPos = 0;
            while (listBlobs.count() < ISHIELD_MAX_BLOBS) {
                const qint32 nHit = baChunk.indexOf(baNeedle, nSearchPos);
                if (nHit < 0) break;
                const quint64 nBlobOffset = static_cast<quint64>(nChunkStart) + static_cast<quint64>(nHit);
                if (!guardedDevice->seek(static_cast<qint64>(nBlobOffset))) break;
                const QByteArray baHeader = guardedDevice->read(ISHIELD_COMMON_HEADER_SIZE + ISHIELD_VOLUME_HEADER_V6_SIZE);
                if (!guardedThis || !guardedDevice) break;
                BLOB_CANDIDATE candidate;
                if (validateBlobHeader(baHeader, nBlobOffset, nDeviceSize, &candidate)) {
                    // A media set shares one generation; drop mismatched hits.
                    if (listBlobs.isEmpty() || (candidate.nMajorVersion == listBlobs.first().nMajorVersion)) {
                        listBlobs.append(candidate);
                    }
                }
                nSearchPos = nHit + 1;
            }
            // Overlap so a signature straddling the chunk boundary is found.
            if ((nChunkStart + nChunkSize) >= nDeviceSize) break;
            nChunkStart += nChunkSize - 3;
        }
    }

    // The device may have been destroyed by a re-entrant callback during the
    // reads above: re-check the guards before touching it, and treat a failed
    // cursor restoration as a failed scan (the caller's position contract
    // would otherwise be silently broken).
    if (!guardedThis || !guardedDevice) return result;
    const bool bRestored = guardedDevice->seek(nSavedPosition);
    if (!guardedThis || !guardedDevice || !bRestored || listBlobs.isEmpty()) return result;

    // Blobs are already in ascending offset order; each blob extends to the
    // next blob's start (the last one to end of file).  The catalog is the
    // blob that carries a descriptor; the remaining blobs are the data
    // volumes, numbered 1..N in on-disk order — the order InstallShield
    // stores them and the order their first/last file indexes ascend.
    qint32 nCatalogIndex = -1;
    for (qint32 i = 0; i < listBlobs.count(); ++i) {
        if (listBlobs.at(i).nDescriptorSize) {
            nCatalogIndex = i;
            break;
        }
    }
    if (nCatalogIndex < 0) return result;

    quint32 nVolumeNumber = 0;
    for (qint32 i = 0; i < listBlobs.count(); ++i) {
        const quint64 nStart = listBlobs.at(i).nOffset;
        const quint64 nEnd = (i + 1 < listBlobs.count()) ? listBlobs.at(i + 1).nOffset : static_cast<quint64>(nDeviceSize);
        if (nEnd <= nStart) return result;
        if (i == nCatalogIndex) {
            result.nCatalogOffset = nStart;
            result.nCatalogSize = nEnd - nStart;
        } else {
            EMBEDDED_VOLUME volume;
            volume.nOffset = nStart;
            volume.nSize = nEnd - nStart;
            volume.baPinnedHeader = listBlobs.at(i).baHeader;
            result.mapVolumes.insert(++nVolumeNumber, volume);
        }
    }

    if (!result.nCatalogSize || result.mapVolumes.isEmpty()) return result;
    result.nMajorVersion = listBlobs.at(nCatalogIndex).nMajorVersion;
    result.bIs64 = bIs64;
    result.bIsValid = true;
    return result;
}

bool XInstallShield::isValid(PDSTRUCT *pPdStruct)
{
    return _scanMedia(pPdStruct).bIsValid;
}

bool XInstallShield::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XInstallShield installer(pDevice);
    return installer.isValid(pPdStruct);
}

XBinary::FT XInstallShield::getFileType()
{
    return _scanMedia(nullptr).bIs64 ? FT_PE64_INSTALLSHIELD : FT_PE32_INSTALLSHIELD;
}

XBinary::MODE XInstallShield::getMode()
{
    return _scanMedia(nullptr).bIs64 ? MODE_64 : MODE_32;
}

QString XInstallShield::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XInstallShield::getFileFormatExtsString()
{
    return QStringLiteral("InstallShield installer (*.exe)");
}

QString XInstallShield::getMIMEString()
{
    return QStringLiteral("application/vnd.microsoft.portable-executable");
}

QString XInstallShield::getVersion()
{
    const MEDIA_LAYOUT layout = _scanMedia(nullptr);
    return (layout.bIsValid && layout.nMajorVersion) ? QString::number(layout.nMajorVersion) : QString();
}

QList<QString> XInstallShield::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ISc('");
}

XBinary *XInstallShield::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    return new XInstallShield(pDevice, bIsImage, nModuleAddress);
}

bool XInstallShield::_loadCatalog(UNPACK_CONTEXT *pContext, PDSTRUCT *pPdStruct) const
{
    if (!pContext) return false;
    pContext->baCatalog.clear();
    pContext->sMediaPrefix.clear();
    pContext->sSourcePath.clear();
    pContext->sContainerPath.clear();
    pContext->mapEmbeddedVolumes.clear();
    pContext->common = COMMON_HEADER();

    QPointer<XInstallShield> guardedThis(const_cast<XInstallShield *>(this));
    QPointer<QIODevice> guardedDevice(guardedThis ? guardedThis->getDevice() : nullptr);
    if (!guardedThis || !guardedDevice) return false;

    // The segment machinery reads volumes through file paths, so embedded
    // media requires a file-backed source device.
    QFile *pSourceFile = dynamic_cast<QFile *>(guardedDevice.data());
    const QString sContainerPath = pSourceFile ? QFileInfo(pSourceFile->fileName()).absoluteFilePath() : QString();
    if (sContainerPath.isEmpty()) {
        XBinary::setPdStructErrorString(pPdStruct, tr("InstallShield embedded media requires a file-backed source"));
        return false;
    }

    const MEDIA_LAYOUT layout = _scanMedia(pPdStruct);
    if (!guardedThis || !guardedDevice || !layout.bIsValid) return false;
    if ((layout.nCatalogSize > static_cast<quint64>(ISHIELD_MAX_CATALOG_SIZE)) ||
        (layout.nCatalogSize > static_cast<quint64>((std::numeric_limits<qint32>::max)()))) {
        return false;
    }

    const qint64 nSavedPosition = guardedDevice->pos();
    if (!guardedThis || !guardedDevice || (nSavedPosition < 0)) return false;
    const bool bSeeked = guardedDevice->seek(static_cast<qint64>(layout.nCatalogOffset));
    if (!guardedThis || !guardedDevice || !bSeeked) return false;
    const QByteArray baCatalog = guardedDevice->read(static_cast<qint64>(layout.nCatalogSize));
    if (!guardedThis || !guardedDevice) return false;
    const bool bRestored = guardedDevice->seek(nSavedPosition);
    if (!bRestored || (static_cast<quint64>(baCatalog.size()) != layout.nCatalogSize)) return false;
    if ((baCatalog.size() < ISHIELD_COMMON_HEADER_SIZE) || (blobLE32(baCatalog, 0) != ISHIELD_CAB_SIGNATURE)) return false;

    COMMON_HEADER header;
    header.nVersion = blobLE32(baCatalog, 4);
    header.nVolumeInfo = blobLE32(baCatalog, 8);
    header.nDescriptorOffset = blobLE32(baCatalog, 12);
    header.nDescriptorSize = blobLE32(baCatalog, 16);
    header.nMajorVersion = blobMajorVersion(header.nVersion);
    if (!header.nDescriptorSize || (header.nMajorVersion < 0) || (header.nMajorVersion > 32)) return false;

    pContext->baCatalog = baCatalog;
    pContext->common = header;
    pContext->sSourcePath = sContainerPath;
    pContext->sContainerPath = sContainerPath;
    pContext->mapEmbeddedVolumes = layout.mapVolumes;
    return true;
}
