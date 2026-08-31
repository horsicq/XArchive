/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free reader for InstallShield 3 self-extractors.
 * MIT License
 */
#include "xis3sfxarchive.h"

#include <QHash>
#include <QPointer>
#include <QSet>

#include "../Formats/exec/xne.h"
#include "../Formats/exec/xpe.h"

namespace {
const qint64 IS3_DESCRIPTOR_SCAN_LIMIT = Q_INT64_C(16) * 1024 * 1024;
const uchar IS3_DESCRIPTOR_MAGIC[8] = {
    0x94, 0x01, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00
};
const uchar IS3_PATH_KEY[8] = {
    0xca, 0xda, 0x7a, 0x5b, 0x4a, 0x76, 0x3e, 0xa0
};

quint8 rotateLeft8(quint8 nValue, quint8 nShift)
{
    nShift &= 7U;
    if (nShift == 0) return nValue;
    return static_cast<quint8>((nValue << nShift) |
                               (nValue >> (8U - nShift)));
}
}  // namespace

XIS3SFXArchive::XIS3SFXArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_INSTALLSHIELD3_SFX)
{
}

bool XIS3SFXArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIS3SFXArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIS3SFXArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                        XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIS3SFXArchive(pDevice);
}

bool XIS3SFXArchive::hasZip64Extra(qint64 nExtraOffset,
                                   qint64 nExtraSize,
                                   PDSTRUCT *pPdStruct)
{
    qint64 nExtraPosition = nExtraOffset;
    const qint64 nExtraEnd = nExtraOffset + nExtraSize;
    while ((nExtraPosition < nExtraEnd) &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (nExtraEnd - nExtraPosition < 4) return true;
        const quint16 nExtraID = read_uint16(nExtraPosition);
        const qint64 nFieldSize = read_uint16(nExtraPosition + 2);
        nExtraPosition += 4;
        if ((nFieldSize > nExtraEnd - nExtraPosition) ||
            (nExtraID == 0x0001U)) {
            return true;
        }
        nExtraPosition += nFieldSize;
    }
    return nExtraPosition != nExtraEnd;
}

bool XIS3SFXArchive::scanFormat(QList<ENTRY> *pEntries,
                                qint64 *pArchiveEnd,
                                PDSTRUCT *pPdStruct)
{
    QPointer<XIS3SFXArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || nTotalSize < 0x24 ||
        nTotalSize > 0xffffffffLL ||
        !isPdStructNotCanceled(pPdStruct) ||
        read_uint16(0) != 0x5a4dU) {
        return false;
    }

    const qint64 nProbeSize = qMin(nTotalSize, IS3_DESCRIPTOR_SCAN_LIMIT);
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedThis || baProbe.size() != nProbeSize) return false;
    const QByteArray baMagic(
        reinterpret_cast<const char *>(IS3_DESCRIPTOR_MAGIC),
        sizeof(IS3_DESCRIPTOR_MAGIC));
    const qint64 nDescriptorOffset = baProbe.indexOf(baMagic);
    if (nDescriptorOffset < 0 ||
        !rangeWithin(nTotalSize, nDescriptorOffset, 0x24)) {
        return false;
    }

    const qint64 nDataPosition = read_uint32(nDescriptorOffset + 0x08);
    const qint64 nRecordCount = read_uint32(nDescriptorOffset + 0x0c);
    const qint64 nDeclaredSize = read_uint32(nDescriptorOffset + 0x10);
    if (!guardedThis || nDeclaredSize != nTotalSize ||
        nRecordCount < 1 || nRecordCount > MAX_RECORDS ||
        read_uint32(nDescriptorOffset + 0x18) != 0 ||
        nDataPosition <= nDescriptorOffset ||
        !rangeWithin(nTotalSize, nDataPosition, 12)) {
        return false;
    }

    const QByteArray baKey = read_array_process(
        nDescriptorOffset + 0x1c, sizeof(IS3_PATH_KEY), pPdStruct);
    if (!guardedThis || baKey.size() != sizeof(IS3_PATH_KEY)) return false;
    uchar pathKey[sizeof(IS3_PATH_KEY)] = {};
    for (qint32 i = 0; i < qint32(sizeof(IS3_PATH_KEY)); ++i) {
        pathKey[i] = static_cast<uchar>(baKey.at(i));
        if (pathKey[i] != IS3_PATH_KEY[i]) return false;
    }

    // PE variants place the table exactly at the end of the last raw section.
    // NE variants have no equivalent dependable overlay calculation, so their
    // authenticated descriptor supplies the table offset.
    XPE pe(getDevice());
    if (pe.isValid(pPdStruct)) {
        if (!guardedThis || pe.getOverlayOffset(pPdStruct) != nDataPosition)
            return false;
    } else {
        XNE ne(getDevice());
        if (!ne.isValid(pPdStruct) || !guardedThis) return false;
    }

    QList<ENTRY> entries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;
    qint64 nPosition = nDataPosition;

    for (qint64 nRecordIndex = 0;
         nRecordIndex < nRecordCount &&
         isPdStructNotCanceled(pPdStruct);
         ++nRecordIndex) {
        const qint64 nRecordOffset = nPosition;
        if (!rangeWithin(nTotalSize, nRecordOffset, 4)) return false;
        const qint64 nPathSize = read_uint32(nRecordOffset);
        if (nPathSize < 1 || nPathSize > 1024 ||
            !rangeWithin(nTotalSize, nRecordOffset, 4 + nPathSize + 8)) {
            return false;
        }

        const QByteArray baEncodedPath = read_array_process(
            nRecordOffset + 4, nPathSize, pPdStruct);
        if (!guardedThis || baEncodedPath.size() != nPathSize) return false;
        QByteArray baPath(nPathSize, Qt::Uninitialized);
        for (qint64 i = 0; i < nPathSize; ++i) {
            const quint8 nEncoded = static_cast<quint8>(baEncodedPath.at(i));
            const quint8 nDecoded = rotateLeft8(
                nEncoded ^ pathKey[i & 7], static_cast<quint8>((i + 1) & 7));
            if (nDecoded < 0x20 || nDecoded > 0x7e) return false;
            baPath[static_cast<int>(i)] = static_cast<char>(nDecoded);
        }

        QString sPath = QString::fromLatin1(baPath);
        sPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
        while (sPath.endsWith(QLatin1Char('/'))) sPath.chop(1);
        QString sName = sPath.section(QLatin1Char('/'), -1);
        sName = XBinary::fixFileName(sName);
        QString sUniqueName;
        if (sName.isEmpty() ||
            !makeUniquePath(sName, &usedFiles, &usedDirectories,
                            &nextSuffixes, &resolvedDirectories,
                            &sUniqueName)) {
            return false;
        }

        const qint64 nTimeOffset = nRecordOffset + 4 + nPathSize;
        const quint16 nDosDate = read_uint16(nTimeOffset);
        const quint16 nDosTime = read_uint16(nTimeOffset + 2);
        const qint64 nOuterDataSize = read_uint32(nTimeOffset + 4);
        const qint64 nOuterDataOffset = nTimeOffset + 8;
        if (!rangeWithin(nTotalSize, nOuterDataOffset, nOuterDataSize))
            return false;

        ENTRY entry = {};
        entry.nHeaderOffset = nRecordOffset;
        entry.nHeaderSize = nOuterDataOffset - nRecordOffset;
        entry.nDataOffset = nOuterDataOffset;
        entry.nDataSize = nOuterDataSize;
        entry.nUncompressedSize = nOuterDataSize;
        entry.handleMethod = HANDLE_METHOD_STORE;
        entry.mtDateTime = XBinary::dosDateTimeToQDateTime(nDosDate, nDosTime);
        entry.sFileName = sUniqueName;

        if (nOuterDataSize >= 30 + 46 + 22 &&
            read_uint32(nOuterDataOffset) == 0x04034b50U) {
            const quint16 nFlags = read_uint16(nOuterDataOffset + 6);
            const quint16 nMethod = read_uint16(nOuterDataOffset + 8);
            const quint32 nCRC32 = read_uint32(nOuterDataOffset + 14);
            const qint64 nPackedSize = read_uint32(nOuterDataOffset + 18);
            const qint64 nRawSize = read_uint32(nOuterDataOffset + 22);
            const qint64 nLocalNameSize = read_uint16(nOuterDataOffset + 26);
            const qint64 nLocalExtraSize = read_uint16(nOuterDataOffset + 28);
            const qint64 nInnerHeaderSize =
                30 + nLocalNameSize + nLocalExtraSize;
            const qint64 nInnerDataOffset =
                nOuterDataOffset + nInnerHeaderSize;
            const qint64 nCentralRelative = nInnerHeaderSize + nPackedSize;
            const qint64 nCentralOffset =
                nOuterDataOffset + nCentralRelative;
            const qint64 nEOCDRelative = nOuterDataSize - 22;
            const qint64 nEOCDOffset = nOuterDataOffset + nEOCDRelative;

            bool bZipValid = nLocalNameSize > 0 &&
                (nFlags & 0x0009U) == 0 &&
                (nMethod == 0 || nMethod == 8) &&
                (nMethod != 0 || nPackedSize == nRawSize) &&
                nPackedSize != 0xffffffffLL &&
                nRawSize != 0xffffffffLL &&
                rangeWithin(nOuterDataOffset + nOuterDataSize,
                            nOuterDataOffset, nInnerHeaderSize) &&
                rangeWithin(nOuterDataOffset + nOuterDataSize,
                            nInnerDataOffset, nPackedSize) &&
                nCentralRelative < nEOCDRelative &&
                rangeWithin(nEOCDOffset, nCentralOffset, 46) &&
                read_uint32(nCentralOffset) == 0x02014b50U &&
                read_uint32(nEOCDOffset) == 0x06054b50U;

            QByteArray baLocalName;
            QByteArray baCentralName;
            if (bZipValid) {
                const quint16 nCentralFlags = read_uint16(nCentralOffset + 8);
                const quint16 nCentralMethod = read_uint16(nCentralOffset + 10);
                const quint32 nCentralCRC32 = read_uint32(nCentralOffset + 16);
                const qint64 nCentralPackedSize = read_uint32(nCentralOffset + 20);
                const qint64 nCentralRawSize = read_uint32(nCentralOffset + 24);
                const qint64 nCentralNameSize = read_uint16(nCentralOffset + 28);
                const qint64 nCentralExtraSize = read_uint16(nCentralOffset + 30);
                const qint64 nCentralCommentSize = read_uint16(nCentralOffset + 32);
                const quint16 nCentralDisk = read_uint16(nCentralOffset + 34);
                const qint64 nLocalOffset = read_uint32(nCentralOffset + 42);
                const qint64 nCentralRecordSize =
                    46 + nCentralNameSize + nCentralExtraSize +
                    nCentralCommentSize;

                bZipValid = nCentralNameSize == nLocalNameSize &&
                    (nCentralFlags & 0x0009U) == 0 &&
                    nCentralMethod == nMethod &&
                    nCentralCRC32 == nCRC32 &&
                    nCentralPackedSize == nPackedSize &&
                    nCentralRawSize == nRawSize &&
                    nCentralDisk <= 1 && nLocalOffset == 0 &&
                    nCentralPackedSize != 0xffffffffLL &&
                    nCentralRawSize != 0xffffffffLL &&
                    nCentralRelative + nCentralRecordSize == nEOCDRelative &&
                    rangeWithin(nEOCDOffset, nCentralOffset,
                                nCentralRecordSize) &&
                    !hasZip64Extra(nOuterDataOffset + 30 + nLocalNameSize,
                                   nLocalExtraSize, pPdStruct) &&
                    !hasZip64Extra(nCentralOffset + 46 + nCentralNameSize,
                                   nCentralExtraSize, pPdStruct);

                if (bZipValid) {
                    baLocalName = read_array_process(
                        nOuterDataOffset + 30, nLocalNameSize, pPdStruct);
                    baCentralName = read_array_process(
                        nCentralOffset + 46, nCentralNameSize, pPdStruct);
                    bZipValid = guardedThis &&
                        baLocalName.size() == nLocalNameSize &&
                        baCentralName.size() == nCentralNameSize &&
                        !baLocalName.contains('\0') &&
                        baLocalName == baCentralName;
                }

                if (bZipValid) {
                    bZipValid = read_uint16(nEOCDOffset + 4) == 0 &&
                        read_uint16(nEOCDOffset + 6) == 0 &&
                        read_uint16(nEOCDOffset + 8) == 1 &&
                        read_uint16(nEOCDOffset + 10) == 1 &&
                        read_uint32(nEOCDOffset + 12) == nCentralRecordSize &&
                        read_uint32(nEOCDOffset + 16) == nCentralRelative &&
                        read_uint16(nEOCDOffset + 20) == 0;
                }
            }

            if (bZipValid) {
                entry.nHeaderSize = nInnerDataOffset - nRecordOffset;
                entry.nDataOffset = nInnerDataOffset;
                entry.nDataSize = nPackedSize;
                entry.nUncompressedSize = nRawSize;
                entry.handleMethod = nMethod == 8
                    ? HANDLE_METHOD_DEFLATE : HANDLE_METHOD_STORE;
                entry.bCRC32Defined = true;
                entry.nCRC32 = nCRC32;
            }
        }

        entries.append(entry);
        nPosition = nOuterDataOffset + nOuterDataSize;
    }

    if (!guardedThis || !isPdStructNotCanceled(pPdStruct) ||
        entries.size() != nRecordCount || nPosition != nTotalSize) {
        return false;
    }

    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
