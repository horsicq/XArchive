/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xwinimageziparchive.h"

#include <QHash>
#include <QSet>
#include <QtEndian>

#include <limits>

namespace {
const qint64 WINIMAGE_ZIP_MIN_SIZE = 22 + 30 + 46;
const qint64 WINIMAGE_ZIP_MAX_SIZE = Q_INT64_C(2) * 1024 * 1024 * 1024;
const qint64 WINIMAGE_ZIP_MAX_CENTRAL_SIZE = Q_INT64_C(64) * 1024 * 1024;
const qint32 WINIMAGE_ZIP_ECD_ATTEMPTS = 256;
const qint32 WINIMAGE_ZIP_ECD_SIZE = 22;
const qint32 WINIMAGE_ZIP_CFD_SIZE = 46;
const qint32 WINIMAGE_ZIP_LFD_SIZE = 30;
const quint32 WINIMAGE_ZIP_SIGNATURE_CFD = 0x02014B50;
const quint32 WINIMAGE_ZIP_SIGNATURE_LFD = 0x04034B50;
// The loader tag the WinImage builder writes immediately in front of the
// appended archive, at the end of its "RsDl" overlay header. Requiring it
// keeps the hybrid interpretation of the footer confined to the one builder
// that is known to emit it.
const qint32 WINIMAGE_ZIP_TAG_SIZE = 4;
// The builder emits Store and Deflate only. Every other PKZIP method is left
// to XZip so that no member can reach a decoder without its own profile.
const quint16 WINIMAGE_ZIP_METHOD_STORE = 0;
const quint16 WINIMAGE_ZIP_METHOD_DEFLATE = 8;
}  // namespace

XWinImageZipArchive::XWinImageZipArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_ZIP)
{
}

bool XWinImageZipArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWinImageZipArchive archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XWinImageZipArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                             XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWinImageZipArchive(pDevice);
}

bool XWinImageZipArchive::readCentralDirectory(qint64 nCentralOffset,
                                               qint64 nCentralSize,
                                               qint32 nRecords,
                                               QList<CENTRALRECORD> *pRecords,
                                               PDSTRUCT *pPdStruct)
{
    if (!pRecords || (nCentralOffset < 0) || (nCentralSize <= 0) ||
        (nCentralSize > WINIMAGE_ZIP_MAX_CENTRAL_SIZE) ||
        (nCentralSize < (qint64)nRecords * WINIMAGE_ZIP_CFD_SIZE)) {
        return false;
    }

    const QByteArray baCentral =
        read_array_process(nCentralOffset, nCentralSize, pPdStruct);
    if ((baCentral.size() != nCentralSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const uchar *pCentral =
        reinterpret_cast<const uchar *>(baCentral.constData());
    qint64 nPosition = 0;

    for (qint32 i = 0; i < nRecords; ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            ((nCentralSize - nPosition) < WINIMAGE_ZIP_CFD_SIZE)) {
            return false;
        }
        const uchar *p = pCentral + nPosition;
        if (qFromLittleEndian<quint32>(p) != WINIMAGE_ZIP_SIGNATURE_CFD) {
            return false;
        }

        CENTRALRECORD record;
        record.nFlags = qFromLittleEndian<quint16>(p + 8);
        record.nMethod = qFromLittleEndian<quint16>(p + 10);
        record.nDosTime = qFromLittleEndian<quint16>(p + 12);
        record.nDosDate = qFromLittleEndian<quint16>(p + 14);
        record.nCRC32 = qFromLittleEndian<quint32>(p + 16);
        record.nCompressedSize = qFromLittleEndian<quint32>(p + 20);
        record.nUncompressedSize = qFromLittleEndian<quint32>(p + 24);
        const quint16 nNameLength = qFromLittleEndian<quint16>(p + 28);
        const quint16 nExtraLength = qFromLittleEndian<quint16>(p + 30);
        const quint16 nCommentLength = qFromLittleEndian<quint16>(p + 32);
        const quint16 nStartDisk = qFromLittleEndian<quint16>(p + 34);
        record.nLocalHeaderOffset = qFromLittleEndian<quint32>(p + 42);

        const qint64 nRecordSize = (qint64)WINIMAGE_ZIP_CFD_SIZE +
                                   nNameLength + nExtraLength + nCommentLength;
        // ZIP64 sentinels, encryption and multi-volume members all need a
        // reader this adapter deliberately does not have.
        if ((nRecordSize > (nCentralSize - nPosition)) || (nNameLength == 0) ||
            (nStartDisk != 0) || (record.nFlags & 0x0001) ||
            (record.nCompressedSize == 0xFFFFFFFF) ||
            (record.nUncompressedSize == 0xFFFFFFFF) ||
            (record.nLocalHeaderOffset == 0xFFFFFFFF) ||
            ((record.nMethod != WINIMAGE_ZIP_METHOD_STORE) &&
             (record.nMethod != WINIMAGE_ZIP_METHOD_DEFLATE)) ||
            ((record.nMethod == WINIMAGE_ZIP_METHOD_STORE) &&
             (record.nCompressedSize != record.nUncompressedSize))) {
            return false;
        }

        record.baName = baCentral.mid((qint32)(nPosition + WINIMAGE_ZIP_CFD_SIZE),
                                      nNameLength);
        if (record.baName.size() != nNameLength) return false;

        pRecords->append(record);
        nPosition += nRecordSize;
    }

    // A trailing gap would mean the record count and the declared directory
    // size disagree, which this reconciliation is not entitled to guess at.
    return (nPosition == nCentralSize);
}

bool XWinImageZipArchive::walkLocalHeaders(const QList<CENTRALRECORD> &listRecords,
                                           qint64 nArchiveOffset,
                                           qint64 nCentralOffset,
                                           QList<ENTRY> *pEntries,
                                           PDSTRUCT *pPdStruct)
{
    QSet<QString> usedFiles;
    QSet<QString> usedDirs;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirs;
    // The walk runs in the outer executable's own coordinates, which is what
    // makes the link check below a real check instead of a tautology.
    qint64 nPosition = nArchiveOffset;

    const qint32 nCount = listRecords.count();
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            ((nCentralOffset - nPosition) < WINIMAGE_ZIP_LFD_SIZE)) {
            return false;
        }
        const CENTRALRECORD &record = listRecords.at(i);

        // The whole point of the format quirk: only the EOCD's pointer to the
        // central directory carries the archive-relative delta, while every
        // central record already stores an absolute outer offset. So the link
        // must name exactly the position the walk reached - the next real,
        // non-overlapping local header - and nothing else. A record aimed at
        // the central directory, at a member's payload, or backwards into the
        // stub therefore fails here regardless of how consistent it looks.
        if ((qint64)record.nLocalHeaderOffset != nPosition) return false;

        const QByteArray baLocal =
            read_array_process(nPosition, WINIMAGE_ZIP_LFD_SIZE, pPdStruct);
        if (baLocal.size() != WINIMAGE_ZIP_LFD_SIZE) return false;
        const uchar *p = reinterpret_cast<const uchar *>(baLocal.constData());
        if (qFromLittleEndian<quint32>(p) != WINIMAGE_ZIP_SIGNATURE_LFD) return false;

        const quint16 nFlags = qFromLittleEndian<quint16>(p + 6);
        const quint16 nMethod = qFromLittleEndian<quint16>(p + 8);
        const quint32 nCRC32 = qFromLittleEndian<quint32>(p + 14);
        const quint32 nCompressedSize = qFromLittleEndian<quint32>(p + 18);
        const quint32 nUncompressedSize = qFromLittleEndian<quint32>(p + 22);
        const quint16 nNameLength = qFromLittleEndian<quint16>(p + 26);
        const quint16 nExtraLength = qFromLittleEndian<quint16>(p + 28);

        // Only a local header that carries its own sizes can be walked; a
        // streamed member (bit 3) hides them in a trailing descriptor and its
        // successor could not be located without the central directory.
        if ((nFlags & 0x0009) || (nMethod != record.nMethod) ||
            (nCRC32 != record.nCRC32) ||
            (nCompressedSize != record.nCompressedSize) ||
            (nUncompressedSize != record.nUncompressedSize) ||
            (nNameLength != record.baName.size())) {
            return false;
        }

        const QByteArray baName = read_array_process(
            nPosition + WINIMAGE_ZIP_LFD_SIZE, nNameLength, pPdStruct);
        if ((baName.size() != nNameLength) ||
            (baName != record.baName)) {
            return false;
        }

        const qint64 nDataOffset =
            nPosition + WINIMAGE_ZIP_LFD_SIZE + nNameLength + nExtraLength;
        if ((nDataOffset < nPosition) || (nDataOffset > nCentralOffset) ||
            ((qint64)nCompressedSize > (nCentralOffset - nDataOffset))) {
            return false;
        }

        QString sUnique;
        if (!decodeName(reinterpret_cast<const uchar *>(record.baName.constData()),
                        record.baName.size(), false, &sUnique)) {
            return false;
        }
        QString sResolved;
        if (!makeUniquePath(sUnique, &usedFiles, &usedDirs, &nextSuffixes,
                            &resolvedDirs, &sResolved)) {
            return false;
        }

        ENTRY entry = {};
        entry.nHeaderOffset = nPosition;
        entry.nHeaderSize = nDataOffset - nPosition;
        entry.nDataOffset = nDataOffset;
        entry.nDataSize = nCompressedSize;
        entry.nUncompressedSize = nUncompressedSize;
        entry.handleMethod = (nMethod == WINIMAGE_ZIP_METHOD_DEFLATE)
                                 ? HANDLE_METHOD_DEFLATE
                                 : HANDLE_METHOD_STORE;
        entry.bCRC32Defined = true;
        entry.nCRC32 = nCRC32;
        entry.sFileName = sResolved;
        if (XBinary::isValidDosDateTime(record.nDosDate, record.nDosTime)) {
            entry.mtDateTime = XBinary::dosDateTimeToQDateTime(record.nDosDate,
                                                               record.nDosTime);
        }
        pEntries->append(entry);

        nPosition = nDataOffset + nCompressedSize;
    }

    // The members must tile the whole area between the archive start and the
    // central directory. Anything else means the sequential walk was not the
    // real member order.
    return (nPosition == nCentralOffset);
}

bool XWinImageZipArchive::scanFormat(QList<ENTRY> *pEntries,
                                     qint64 *pArchiveEnd,
                                     PDSTRUCT *pPdStruct)
{
    PDSTRUCT localPdStruct = {};
    if (!pPdStruct) {
        localPdStruct = XBinary::createPdStruct();
        pPdStruct = &localPdStruct;
    }
    const qint64 nTotalSize = getSize();
    if ((nTotalSize < WINIMAGE_ZIP_MIN_SIZE) ||
        (nTotalSize > WINIMAGE_ZIP_MAX_SIZE) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nMaximumSearchSize = 0xFFFF + WINIMAGE_ZIP_ECD_SIZE;
    const qint64 nSearchOffset = qMax((qint64)0, nTotalSize - nMaximumSearchSize);
    const qint64 nSearchSize = nTotalSize - nSearchOffset;
    if (nSearchSize > (std::numeric_limits<qint32>::max)()) return false;
    const QByteArray baTail = read_array_process(nSearchOffset, nSearchSize, pPdStruct);
    if ((baTail.size() != nSearchSize) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    static const QByteArray baEcdSignature("PK\x05\x06", 4);
    static const QByteArray baWinImageTag("WSfx", WINIMAGE_ZIP_TAG_SIZE);
    qint32 nCandidatePosition = baTail.lastIndexOf(baEcdSignature);
    qint32 nAttempt = 0;

    while ((nCandidatePosition >= 0) && (nAttempt < WINIMAGE_ZIP_ECD_ATTEMPTS) &&
           isPdStructNotCanceled(pPdStruct)) {
        ++nAttempt;
        const qint64 nEcdOffset = nSearchOffset + nCandidatePosition;
        if ((nTotalSize - nEcdOffset) >= WINIMAGE_ZIP_ECD_SIZE) {
            const uchar *p =
                reinterpret_cast<const uchar *>(baTail.constData()) + nCandidatePosition;
            const quint16 nDiskNumber = qFromLittleEndian<quint16>(p + 4);
            const quint16 nStartDisk = qFromLittleEndian<quint16>(p + 6);
            const quint16 nDiskRecords = qFromLittleEndian<quint16>(p + 8);
            const quint16 nTotalRecords = qFromLittleEndian<quint16>(p + 10);
            const quint32 nCentralSize = qFromLittleEndian<quint32>(p + 12);
            const quint32 nStoredCentralOffset = qFromLittleEndian<quint32>(p + 16);
            const quint16 nCommentLength = qFromLittleEndian<quint16>(p + 20);
            const qint64 nArchiveEnd =
                nEcdOffset + WINIMAGE_ZIP_ECD_SIZE + nCommentLength;

            if ((nDiskNumber == 0) && (nStartDisk == 0) &&
                (nDiskRecords == nTotalRecords) && (nTotalRecords != 0) &&
                (nTotalRecords != 0xFFFF) && (nCentralSize != 0xFFFFFFFF) &&
                (nStoredCentralOffset != 0xFFFFFFFF) &&
                (nTotalRecords <= MAX_RECORDS) && (nArchiveEnd <= nTotalSize) &&
                ((qint64)nCentralSize <= nEcdOffset)) {
                // The end-of-central-directory record itself stays conforming
                // apart from one field, so the archive's true start offset is
                // arithmetic, not a guess: the central directory ends where
                // the footer begins, and the stored pointer to it is expressed
                // in the appended archive's own coordinates.
                const qint64 nCentralOffset = nEcdOffset - (qint64)nCentralSize;
                const qint64 nArchiveOffset =
                    nCentralOffset - (qint64)nStoredCentralOffset;
                // A conforming ZIP, and an SFX whose footer is fully absolute,
                // both land on zero. Only a genuine prefix delta continues,
                // and the WinImage loader tag must sit right in front of it.
                if ((nArchiveOffset > WINIMAGE_ZIP_TAG_SIZE) &&
                    (nArchiveOffset < nCentralOffset) &&
                    (read_array_process(nArchiveOffset - WINIMAGE_ZIP_TAG_SIZE,
                                        WINIMAGE_ZIP_TAG_SIZE,
                                        pPdStruct) == baWinImageTag)) {
                    QList<CENTRALRECORD> listRecords;
                    QList<ENTRY> listEntries;
                    if (readCentralDirectory(nCentralOffset, nCentralSize,
                                             nTotalRecords, &listRecords,
                                             pPdStruct) &&
                        walkLocalHeaders(listRecords, nArchiveOffset,
                                         nCentralOffset, &listEntries,
                                         pPdStruct) &&
                        !listEntries.isEmpty()) {
                        if (pEntries) *pEntries = listEntries;
                        if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;
                        return true;
                    }
                }
            }
        }

        nCandidatePosition = (nCandidatePosition > 0)
                                 ? baTail.lastIndexOf(baEcdSignature,
                                                      nCandidatePosition - 1)
                                 : -1;
    }

    return false;
}
