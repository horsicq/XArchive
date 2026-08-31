/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xrncarchive.h"

#include "Algos/xancientdecoder.h"

#include <QPointer>
#include <QSet>
#include <QtEndian>

#include <limits>

namespace {
quint32 readBE32(const QByteArray &data, qint32 offset)
{
    if (offset < 0 || offset > data.size() - 4) return 0;
    return qFromBigEndian<quint32>(
        reinterpret_cast<const uchar *>(data.constData() + offset));
}

struct RncDirectoryEntry {
    qint64 nHeaderOffset = 0;
    qint64 nHeaderSize = 0;
    qint64 nDataOffset = 0;
    QString sName;
};
}  // namespace

XRncArchive::XRncArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_RNC)
{
}

bool XRncArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XRncArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XRncArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                     XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XRncArchive(pDevice);
}

bool XRncArchive::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                             PDSTRUCT *pPdStruct)
{
    QPointer<XRncArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || nTotalSize < 24 ||
        nTotalSize > (std::numeric_limits<qint32>::max)() ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const QByteArray header = read_array(0, 11);
    if (!guardedThis || header.size() != 11 ||
        header.left(4) != QByteArray("RNCA", 4)) {
        return false;
    }

    const qint64 nFirstDataOffset =
        qFromBigEndian<quint16>(
            reinterpret_cast<const uchar *>(header.constData() + 4));
    const qint64 nRepeatedDataOffset =
        qFromBigEndian<quint16>(
            reinterpret_cast<const uchar *>(header.constData() + 8));
    if (nFirstDataOffset < 17 || nFirstDataOffset >= nTotalSize ||
        nRepeatedDataOffset != nFirstDataOffset || header.at(10) != 0) {
        return false;
    }

    // Bytes 11..firstData-2 are {ASCIIZ name, BE32 absolute offset}
    // records.  The final byte before the first member is a mandatory zero
    // sentinel; requiring the directory to tile this range rules out an
    // accidental RNCA prefix without relying on the undocumented checksum at
    // bytes 6..7.
    const QByteArray directory = read_array(11, nFirstDataOffset - 11);
    if (!guardedThis || directory.size() != nFirstDataOffset - 11 ||
        directory.isEmpty() || directory.at(directory.size() - 1) != 0) {
        return false;
    }

    QList<RncDirectoryEntry> directoryEntries;
    qint32 nPosition = 0;
    const qint32 nDirectoryRecordsEnd = directory.size() - 1;
    while (nPosition < nDirectoryRecordsEnd) {
        if (!guardedThis || !isPdStructNotCanceled(pPdStruct) ||
            directoryEntries.size() >= MAX_RECORDS) {
            return false;
        }
        const qint32 nNameEnd = directory.indexOf('\0', nPosition);
        if (nNameEnd <= nPosition || nNameEnd > nDirectoryRecordsEnd - 4) {
            return false;
        }
        QString sName;
        if (!decodeName(
                reinterpret_cast<const uchar *>(directory.constData() +
                                                  nPosition),
                nNameEnd - nPosition + 1, true, &sName)) {
            return false;
        }
        const qint64 nMemberOffset = readBE32(directory, nNameEnd + 1);
        if (nMemberOffset < nFirstDataOffset || nMemberOffset >= nTotalSize ||
            (!directoryEntries.isEmpty() &&
             nMemberOffset <= directoryEntries.constLast().nDataOffset)) {
            return false;
        }

        RncDirectoryEntry directoryEntry;
        directoryEntry.nHeaderOffset = 11 + nPosition;
        directoryEntry.nHeaderSize =
            (nNameEnd + 5) - nPosition;
        directoryEntry.nDataOffset = nMemberOffset;
        directoryEntry.sName = sName;
        directoryEntries.append(directoryEntry);
        nPosition = nNameEnd + 5;
    }

    if (directoryEntries.isEmpty() || nPosition != nDirectoryRecordsEnd ||
        directoryEntries.constFirst().nDataOffset != nFirstDataOffset) {
        return false;
    }

    QList<ENTRY> entries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;

    for (qint32 i = 0; i < directoryEntries.size(); ++i) {
        if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;
        const RncDirectoryEntry &source = directoryEntries.at(i);
        const qint64 nContainerEnd = (i + 1 < directoryEntries.size())
            ? directoryEntries.at(i + 1).nDataOffset
            : nTotalSize;
        if (!rangeWithin(nTotalSize, source.nDataOffset,
                         nContainerEnd - source.nDataOffset) ||
            nContainerEnd - source.nDataOffset < 8) {
            return false;
        }

        const QByteArray memberHeader = read_array(source.nDataOffset, 18);
        if (!guardedThis || memberHeader.size() < 8 ||
            memberHeader.left(3) != QByteArray("RNC", 3)) {
            return false;
        }

        const quint8 nMethod = quint8(memberHeader.at(3));
        const qint64 nRawSize = readBE32(memberHeader, 4);
        qint64 nCanonicalSize = 0;
        ENTRY entry = {};
        if (nMethod == 0) {
            nCanonicalSize = 8 + nRawSize;
            if (nCanonicalSize < 8 ||
                !rangeWithin(nTotalSize, source.nDataOffset,
                             nCanonicalSize)) {
                return false;
            }
            entry.nDataOffset = source.nDataOffset + 8;
            entry.nDataSize = nRawSize;
            entry.nUncompressedSize = nRawSize;
            entry.handleMethod = HANDLE_METHOD_STORE;
        } else if (nMethod == 1 || nMethod == 2) {
            if (memberHeader.size() != 18) return false;
            const qint64 nPackedSize = readBE32(memberHeader, 8);
            nCanonicalSize = 18 + nPackedSize;
            if (nCanonicalSize < 18 ||
                !rangeWithin(nTotalSize, source.nDataOffset,
                             nCanonicalSize) ||
                nCanonicalSize > XAncientDecoder::MAX_PACKED_SIZE) {
                return false;
            }
            const QByteArray packed =
                read_array(source.nDataOffset, nCanonicalSize);
            XAncientDecoder::INFO decoderInfo;
            if (!guardedThis || packed.size() != nCanonicalSize ||
                !XAncientDecoder::describe(
                    packed, XAncientDecoder::TYPE_RNC, &decoderInfo) ||
                decoderInfo.packedSize != nCanonicalSize ||
                decoderInfo.rawSize != nRawSize) {
                return false;
            }
            entry.nDataOffset = source.nDataOffset;
            entry.nDataSize = nCanonicalSize;
            entry.nUncompressedSize =
                decoderInfo.imageSize >= 0 ? decoderInfo.imageSize : nRawSize;
            entry.handleMethod = HANDLE_METHOD_RNC;
        } else {
            return false;
        }

        const qint64 nCanonicalEnd = source.nDataOffset + nCanonicalSize;
        const qint64 nTrailingSize = nContainerEnd - nCanonicalEnd;
        // The observed writer appends a four-byte archive checksum after the
        // final member.  No padding is permitted between members.
        if ((i + 1 < directoryEntries.size() && nTrailingSize != 0) ||
            (i + 1 == directoryEntries.size() &&
             nTrailingSize != 0 && nTrailingSize != 4)) {
            return false;
        }

        QString sUniqueName;
        if (!makeUniquePath(source.sName, &usedFiles, &usedDirectories,
                            &nextSuffixes, &resolvedDirectories,
                            &sUniqueName)) {
            return false;
        }
        entry.nHeaderOffset = source.nHeaderOffset;
        entry.nHeaderSize = source.nHeaderSize;
        entry.sFileName = sUniqueName;
        entries.append(entry);
    }

    if (!guardedThis || entries.size() != directoryEntries.size() ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
