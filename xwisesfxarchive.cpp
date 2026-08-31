/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free reader for Wise installer member streams.
 * MIT License
 */
#include "xwisesfxarchive.h"

#include <QHash>
#include <QPointer>
#include <QSet>
#include <QtEndian>
#include <limits>
#include <zlib.h>

#include "../Formats/exec/xne.h"
#include "../Formats/exec/xpe.h"

namespace {
const qint64 WISE_STUB_PROBE_SIZE = 128 * 1024;
const qint64 WISE_HEADER_SCAN_SIZE = 16 * 1024;
const qint64 WISE_PROBE_OUTPUT_LIMIT = 1024 * 1024;
const qint64 WISE_CAPTURE_LIMIT = 64 * 1024;
const qint64 WISE_MAX_OUTPUT_SIZE = Q_INT64_C(8) * 1024 * 1024 * 1024;
const qint32 WISE_PROBE_CHUNK_SIZE = 64 * 1024;
const qint32 WISE_STREAM_CHUNK_SIZE = 1024 * 1024;

struct WiseInflateInfo {
    qint64 nCompressedSize = 0;
    qint64 nRawSize = 0;
    quint32 nCRC32 = 0;
    QByteArray baPrefix;
};

bool wiseHasStubMarker(const QByteArray &baData)
{
    return baData.toLower().contains(QByteArrayLiteral("wisemain"));
}

bool wiseHasPlausibleFirstOutput(const QByteArray &baInput, qint64 nOffset)
{
    if (nOffset < 0 || nOffset >= baInput.size()) return false;
    const qint64 nAvailable = baInput.size() - nOffset;
    if (nAvailable < 4 || nAvailable > std::numeric_limits<uInt>::max())
        return false;

    z_stream stream = {};
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return false;
    uchar output[8] = {};
    stream.next_in = reinterpret_cast<Bytef *>(
        const_cast<char *>(baInput.constData() + nOffset));
    stream.avail_in = static_cast<uInt>(nAvailable);
    stream.next_out = output;
    stream.avail_out = sizeof(output);
    const int nResult = inflate(&stream, Z_NO_FLUSH);
    const qint32 nProduced = sizeof(output) - stream.avail_out;
    inflateEnd(&stream);

    if ((nResult != Z_OK && nResult != Z_STREAM_END) || nProduced < 4)
        return false;
    // Every known Wise color table or script begins with a small aligned
    // little-endian record/header size. This cheaply rejects code/data bytes
    // that happen to form a syntactically valid raw-DEFLATE prefix.
    return output[1] == 0 && output[0] >= 4 && output[0] <= 0xa0 &&
           ((output[0] & 1U) == 0);
}

QString wiseExtension(const QByteArray &baPrefix)
{
    if (baPrefix.startsWith("MZ")) return QStringLiteral("exe");
    if (baPrefix.startsWith("BM")) return QStringLiteral("bmp");
    if (baPrefix.startsWith("RIFF")) return QStringLiteral("riff");
    if (baPrefix.startsWith("PK\x03\x04")) return QStringLiteral("zip");
    if (baPrefix.startsWith("\x89PNG\r\n\x1a\n")) return QStringLiteral("png");
    if (baPrefix.startsWith("GIF87a") || baPrefix.startsWith("GIF89a"))
        return QStringLiteral("gif");
    if (baPrefix.startsWith("?\x5f\x03\x00")) return QStringLiteral("hlp");
    if (baPrefix.startsWith("MSCF")) return QStringLiteral("cab");
    return QStringLiteral("bin");
}

bool wiseLooksLikeScript(const QByteArray &baPrefix)
{
    const QByteArray baLower = baPrefix.toLower();
    return baLower.contains("maindir") || baLower.contains("%win%") ||
           baLower.contains("wise0001") ||
           baLower.contains("installation aborted") ||
           baLower.contains("install.log");
}

bool wiseLooksLikeColors(const QByteArray &baPrefix, qint64 nRawSize)
{
    if (baPrefix.size() < 4 || nRawSize < 40) return false;
    const quint32 nHeaderSize = qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(baPrefix.constData()));
    return (nHeaderSize == 12) || (nHeaderSize == 40) ||
           (nHeaderSize == 64) || (nHeaderSize == 108) ||
           (nHeaderSize == 124);
}

bool wiseInflateStream(XBinary *pBinary, qint64 nOffset, qint64 nInputLimit,
                       qint64 nOutputLimit, WiseInflateInfo *pInfo,
                       XBinary::PDSTRUCT *pPdStruct,
                       bool bAllowTruncatedEOF = false)
{
    if (!pBinary || !pInfo || nOffset < 0 || nInputLimit <= 0 ||
        nOutputLimit <= 0 ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    z_stream stream = {};
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return false;

    const qint32 nChunkSize = (nOutputLimit <= WISE_PROBE_OUTPUT_LIMIT)
        ? WISE_PROBE_CHUNK_SIZE : WISE_STREAM_CHUNK_SIZE;
    QByteArray baInput(nChunkSize, Qt::Uninitialized);
    QByteArray baOutput(nChunkSize, Qt::Uninitialized);
    QByteArray baPrefix;
    baPrefix.reserve(qMin<qint64>(WISE_CAPTURE_LIMIT, nOutputLimit));
    qint64 nInputPosition = 0;
    qint64 nOutputSize = 0;
    quint32 nCRC32 = crc32(0L, Z_NULL, 0);
    bool bFinished = false;
    bool bFailed = false;
    bool bInputExhausted = false;

    while (!bFinished && !bFailed &&
           XBinary::isPdStructNotCanceled(pPdStruct)) {
        if (stream.avail_in == 0) {
            const qint64 nToRead = qMin<qint64>(nChunkSize,
                                                nInputLimit - nInputPosition);
            if (nToRead <= 0) {
                bInputExhausted = true;
                bFailed = true;
                break;
            }
            const QByteArray baRead = pBinary->read_array_process(
                nOffset + nInputPosition, nToRead, pPdStruct);
            if (baRead.size() != nToRead) {
                bFailed = true;
                break;
            }
            memcpy(baInput.data(), baRead.constData(), size_t(nToRead));
            stream.next_in = reinterpret_cast<Bytef *>(baInput.data());
            stream.avail_in = static_cast<uInt>(nToRead);
            nInputPosition += nToRead;
        }

        stream.next_out = reinterpret_cast<Bytef *>(baOutput.data());
        stream.avail_out = static_cast<uInt>(nChunkSize);
        const int nResult = inflate(&stream, Z_NO_FLUSH);
        const qint64 nProduced = nChunkSize - stream.avail_out;
        if (nProduced > 0) {
            if (nOutputSize > nOutputLimit - nProduced) {
                bFailed = true;
                break;
            }
            nCRC32 = crc32(nCRC32,
                           reinterpret_cast<const Bytef *>(baOutput.constData()),
                           static_cast<uInt>(nProduced));
            if (baPrefix.size() < WISE_CAPTURE_LIMIT) {
                const qint64 nCapture = qMin<qint64>(
                    nProduced, WISE_CAPTURE_LIMIT - baPrefix.size());
                baPrefix.append(baOutput.constData(), nCapture);
            }
            nOutputSize += nProduced;
        }

        if (nResult == Z_STREAM_END) {
            bFinished = true;
        } else if (nResult != Z_OK || (nProduced == 0 && stream.avail_in == 0)) {
            bFailed = true;
        }
    }

    const qint64 nConsumed = static_cast<qint64>(stream.total_in);
    inflateEnd(&stream);
    const bool bAcceptedTruncated = bAllowTruncatedEOF && bInputExhausted &&
                                    nConsumed == nInputLimit &&
                                    nOutputSize > 0;
    if ((!bFinished && !bAcceptedTruncated) ||
        (bFailed && !bAcceptedTruncated) || nConsumed <= 0 ||
        nOutputSize < 0 ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    pInfo->nCompressedSize = nConsumed;
    pInfo->nRawSize = nOutputSize;
    pInfo->nCRC32 = nCRC32;
    pInfo->baPrefix = baPrefix;
    return true;
}
}  // namespace

XWiseSFXArchive::XWiseSFXArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_WISE_SFX)
{
}

bool XWiseSFXArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWiseSFXArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWiseSFXArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                         XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWiseSFXArchive(pDevice);
}

bool XWiseSFXArchive::scanFormat(QList<ENTRY> *pEntries,
                                 qint64 *pArchiveEnd,
                                 PDSTRUCT *pPdStruct)
{
    QPointer<XWiseSFXArchive> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || nTotalSize < 0x3000 ||
        !isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray baStub = read_array_process(
        0, qMin(nTotalSize, WISE_STUB_PROBE_SIZE), pPdStruct);
    if (!guardedThis || !wiseHasStubMarker(baStub)) return false;

    qint64 nOverlayOffset = -1;
    XPE pe(getDevice());
    if (pe.isValid(pPdStruct)) {
        nOverlayOffset = pe.getOverlayOffset(pPdStruct);
    } else {
        XNE ne(getDevice());
        if (!ne.isValid(pPdStruct)) return false;
        nOverlayOffset = ne.getOverlayOffset(pPdStruct);
    }
    if (!guardedThis || nOverlayOffset < 0 ||
        nOverlayOffset >= nTotalSize - 8) return false;

    // Wise's optional ZIP mode uses absolute file offsets in the central
    // directory. The first local member (the installer color table) is not
    // indexed there, so a generic embedded-ZIP reader cannot model the whole
    // sequence. Validate both tables here and expose every local member.
    const qint64 nZipSearchEnd = qMin(nTotalSize - 30,
                                      nOverlayOffset + WISE_HEADER_SCAN_SIZE);
    qint64 nZipOffset = find_signature(nOverlayOffset,
                                       nZipSearchEnd - nOverlayOffset,
                                       "'PK'0304", nullptr, pPdStruct);
    if (nZipOffset >= nOverlayOffset) {
        QList<ENTRY> zipEntries;
        QSet<QString> usedFiles;
        QSet<QString> usedDirectories;
        QHash<QString, qint32> nextSuffixes;
        QHash<QString, QString> resolvedDirectories;
        QSet<qint64> localOffsets;
        qint64 nPosition = nZipOffset;
        bool bZipValid = true;
        while (rangeWithin(nTotalSize, nPosition, 30) &&
               read_uint32(nPosition) == 0x04034b50U &&
               zipEntries.size() < MAX_RECORDS) {
            const quint16 nFlags = read_uint16(nPosition + 6);
            const quint16 nMethod = read_uint16(nPosition + 8);
            const quint32 nCRC32 = read_uint32(nPosition + 14);
            const qint64 nPackedSize = read_uint32(nPosition + 18);
            const qint64 nRawSize = read_uint32(nPosition + 22);
            const qint64 nNameSize = read_uint16(nPosition + 26);
            const qint64 nExtraSize = read_uint16(nPosition + 28);
            const qint64 nHeaderSize = 30 + nNameSize + nExtraSize;
            const qint64 nDataOffset = nPosition + nHeaderSize;
            if ((nFlags & 0x0009U) ||
                (nMethod != 0 && nMethod != 8) ||
                (nMethod == 0 && nPackedSize != nRawSize) ||
                !rangeWithin(nTotalSize, nPosition, nHeaderSize) ||
                !rangeWithin(nTotalSize, nDataOffset, nPackedSize)) {
                bZipValid = false;
                break;
            }

            QString sName;
            if (nNameSize > 0) {
                const QByteArray baName = read_array_process(
                    nPosition + 30, nNameSize, pPdStruct);
                if (baName.size() != nNameSize || baName.contains('\0')) {
                    bZipValid = false;
                    break;
                }
                sName = (nFlags & 0x0800U)
                    ? QString::fromUtf8(baName) : QString::fromLatin1(baName);
                sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
                sName = XBinary::fixFileName(sName);
            }
            if (sName.isEmpty()) {
                sName = zipEntries.isEmpty()
                    ? QStringLiteral("WiseColors.dib")
                    : QStringLiteral("entry_%1.bin")
                          .arg(zipEntries.size(), 5, 10, QLatin1Char('0'));
            }
            QString sUniqueName;
            if (!makeUniquePath(sName, &usedFiles, &usedDirectories,
                                &nextSuffixes, &resolvedDirectories,
                                &sUniqueName)) {
                bZipValid = false;
                break;
            }

            ENTRY entry = {};
            entry.nHeaderOffset = nPosition;
            entry.nHeaderSize = nHeaderSize;
            entry.nDataOffset = nDataOffset;
            entry.nDataSize = nPackedSize;
            entry.nUncompressedSize = nRawSize;
            entry.handleMethod = nMethod == 8 ? HANDLE_METHOD_DEFLATE
                                               : HANDLE_METHOD_STORE;
            entry.bCRC32Defined = true;
            entry.nCRC32 = nCRC32;
            entry.sFileName = sUniqueName;
            localOffsets.insert(nPosition);
            zipEntries.append(entry);
            nPosition = nDataOffset + nPackedSize;
        }

        const qint64 nEOCDOffset = find_signature(
            qMax(nPosition, nTotalSize - (Q_INT64_C(65535) + 22)),
            nTotalSize - qMax(nPosition,
                              nTotalSize - (Q_INT64_C(65535) + 22)),
            "'PK'0506", nullptr, pPdStruct);
        if (bZipValid && zipEntries.size() >= 2 && nEOCDOffset >= nPosition &&
            rangeWithin(nTotalSize, nEOCDOffset, 22)) {
            const quint16 nDisk = read_uint16(nEOCDOffset + 4);
            const quint16 nCentralDisk = read_uint16(nEOCDOffset + 6);
            const quint16 nDiskEntries = read_uint16(nEOCDOffset + 8);
            const quint16 nTotalEntries = read_uint16(nEOCDOffset + 10);
            const qint64 nCentralSize = read_uint32(nEOCDOffset + 12);
            const qint64 nCentralOffset = read_uint32(nEOCDOffset + 16);
            const qint64 nCommentSize = read_uint16(nEOCDOffset + 20);
            bZipValid = nDisk == 0 && nCentralDisk == 0 &&
                        nDiskEntries == nTotalEntries &&
                        nTotalEntries <= zipEntries.size() &&
                        zipEntries.size() - nTotalEntries <= 4 &&
                        nCentralOffset == nPosition &&
                        nCentralSize == nEOCDOffset - nCentralOffset &&
                        nEOCDOffset + 22 + nCommentSize == nTotalSize;

            qint64 nCentralPosition = nCentralOffset;
            qint32 nCentralCount = 0;
            while (bZipValid && nCentralPosition < nEOCDOffset) {
                if (!rangeWithin(nEOCDOffset, nCentralPosition, 46) ||
                    read_uint32(nCentralPosition) != 0x02014b50U) {
                    bZipValid = false;
                    break;
                }
                const qint64 nNameSize = read_uint16(nCentralPosition + 28);
                const qint64 nExtraSize = read_uint16(nCentralPosition + 30);
                const qint64 nRecordCommentSize =
                    read_uint16(nCentralPosition + 32);
                const qint64 nLocalOffset = read_uint32(nCentralPosition + 42);
                const qint64 nRecordSize = 46 + nNameSize + nExtraSize +
                                           nRecordCommentSize;
                if (!rangeWithin(nEOCDOffset, nCentralPosition, nRecordSize) ||
                    !localOffsets.contains(nLocalOffset)) {
                    bZipValid = false;
                    break;
                }
                nCentralPosition += nRecordSize;
                ++nCentralCount;
            }
            bZipValid = bZipValid && nCentralPosition == nEOCDOffset &&
                        nCentralCount == nTotalEntries;
        } else {
            bZipValid = false;
        }

        if (bZipValid && isPdStructNotCanceled(pPdStruct)) {
            if (pEntries) *pEntries = zipEntries;
            if (pArchiveEnd) *pArchiveEnd = nTotalSize;
            return true;
        }
    }

    // Wise revisions use several fixed-size headers. Rather than keying on
    // version-specific stub lengths, authenticate the first raw-DEFLATE member
    // by its immediately following CRC32. The scan is restricted to the small
    // executable overlay header and is gated by the WiseMain stub marker.
    qint64 nFirstStreamOffset = -1;
    WiseInflateInfo firstInfo;
    const qint64 nSearchEnd = qMin(nTotalSize - 8,
                                   nOverlayOffset + WISE_HEADER_SCAN_SIZE);
    const QByteArray baHeaderProbe = read_array_process(
        nOverlayOffset,
        qMin<qint64>(nTotalSize - nOverlayOffset, 64 * 1024), pPdStruct);
    if (!guardedThis || baHeaderProbe.size() < 4) return false;
    for (qint64 nCandidate = nOverlayOffset;
         nCandidate < nSearchEnd && isPdStructNotCanceled(pPdStruct);
         ++nCandidate) {
        const quint8 nFirstByte = read_uint8(nCandidate);
        if (((nFirstByte >> 1) & 3U) == 3U) continue;
        if (!wiseHasPlausibleFirstOutput(baHeaderProbe,
                                         nCandidate - nOverlayOffset))
            continue;
        WiseInflateInfo info;
        if (!wiseInflateStream(this, nCandidate, nTotalSize - nCandidate,
                               WISE_PROBE_OUTPUT_LIMIT, &info, pPdStruct))
            continue;
        const qint64 nCRCOffset = nCandidate + info.nCompressedSize;
        if (!rangeWithin(nTotalSize, nCRCOffset, 4) ||
            read_uint32(nCRCOffset) != info.nCRC32) continue;
        nFirstStreamOffset = nCandidate;
        firstInfo = info;
        break;
    }
    if (!guardedThis || nFirstStreamOffset < 0) return false;

    QList<ENTRY> entries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;
    qint64 nPosition = nFirstStreamOffset;
    qint32 nIndex = 0;
    while (nPosition < nTotalSize && entries.size() < MAX_RECORDS &&
           isPdStructNotCanceled(pPdStruct)) {
        WiseInflateInfo info;
        if (nIndex == 0) {
            info = firstInfo;
        } else if (!wiseInflateStream(this, nPosition,
                                      nTotalSize - nPosition,
                                      WISE_MAX_OUTPUT_SIZE, &info,
                                      pPdStruct)) {
            if (entries.size() < 2 ||
                !wiseInflateStream(this, nPosition,
                                   nTotalSize - nPosition,
                                   WISE_MAX_OUTPUT_SIZE, &info,
                                   pPdStruct, true)) {
                return false;
            }
        }
        if (info.nCompressedSize <= 0 || info.nRawSize < 0 ||
            !rangeWithin(nTotalSize, nPosition, info.nCompressedSize))
            return false;

        const qint64 nCRCOffset = nPosition + info.nCompressedSize;
        qint64 nCRCPadding = -1;
        for (qint64 nPadding = 0; nPadding <= 3; ++nPadding) {
            if (!rangeWithin(nTotalSize, nCRCOffset, nPadding + 4)) break;
            bool bZeroPadding = true;
            for (qint64 i = 0; i < nPadding; ++i) {
                if (read_uint8(nCRCOffset + i) != 0) {
                    bZeroPadding = false;
                    break;
                }
            }
            if (bZeroPadding &&
                read_uint32(nCRCOffset + nPadding) == info.nCRC32) {
                nCRCPadding = nPadding;
                break;
            }
        }
        const bool bHasCRC = nCRCPadding >= 0;
        const bool bAtEOF = (nCRCOffset == nTotalSize);
        if (!bHasCRC && !bAtEOF) return false;

        QString sName;
        if (wiseLooksLikeScript(info.baPrefix)) {
            sName = QStringLiteral("WiseScript.bin");
        } else if (wiseLooksLikeColors(info.baPrefix, info.nRawSize)) {
            sName = QStringLiteral("WiseColors.dib");
        } else {
            sName = QStringLiteral("entry_%1.%2")
                        .arg(nIndex, 5, 10, QLatin1Char('0'))
                        .arg(wiseExtension(info.baPrefix));
        }
        QString sUniqueName;
        if (!makeUniquePath(sName, &usedFiles, &usedDirectories,
                            &nextSuffixes, &resolvedDirectories,
                            &sUniqueName)) return false;

        ENTRY entry = {};
        entry.nHeaderOffset = (nIndex == 0) ? nOverlayOffset : nPosition;
        entry.nHeaderSize = (nIndex == 0)
            ? (nFirstStreamOffset - nOverlayOffset) : 0;
        entry.nDataOffset = nPosition;
        entry.nDataSize = info.nCompressedSize;
        entry.nUncompressedSize = info.nRawSize;
        entry.handleMethod = bAtEOF && !bHasCRC
            ? HANDLE_METHOD_WISE_DEFLATE : HANDLE_METHOD_DEFLATE;
        entry.bCRC32Defined = bHasCRC;
        entry.nCRC32 = info.nCRC32;
        entry.sFileName = sUniqueName;
        entries.append(entry);

        nPosition = nCRCOffset +
                    (bHasCRC ? (nCRCPadding + 4) : 0);
        ++nIndex;
        if (bAtEOF) break;
    }

    if (!guardedThis || entries.size() < 2 || nPosition != nTotalSize ||
        !isPdStructNotCanceled(pPdStruct)) return false;
    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
