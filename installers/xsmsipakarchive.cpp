/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xsmsipakarchive.h"

#include "Algos/xdcldecoder.h"

#include <QFileInfo>
#include <QtEndian>

#include <cstring>

namespace {
const char SMSIPAK_MAGIC[8] = {'S', 'M', 'S', 'I', 'P', 'A', 'K', ' '};
const quint16 SMSIPAK_STAMP = 0x1a07;
const qint64 SMSIPAK_HEADER_SIZE = 14;
const qint64 SMSIPAK_RECORD_SIZE = 34;
const qint32 SMSIPAK_NAME_SIZE = 12;
const qint64 SMSIPAK_INDEX_HEADER_SIZE = 6;
const quint8 SMSIPAK_METHOD_DCL = 1;
const quint8 SMSIPAK_METHOD_STORE = 2;

const char PSN_MAGIC[] =
    "PSNcompress-Copyright\xae 3M Company ALL RIGHTS RESERVED";
const qint64 PSN_MAGIC_SIZE = 53;
const qint64 PSN_VERSION_SIZE = 4;
const qint64 PSN_SIZE_DIGITS = 8;
const qint64 PSN_HEADER_SIZE =
    PSN_MAGIC_SIZE + PSN_VERSION_SIZE + PSN_SIZE_DIGITS;
const qint64 PSN_MAX_STREAM = Q_INT64_C(256) * 1024 * 1024;
const qint64 PSN_MAX_OUTPUT = Q_INT64_C(512) * 1024 * 1024;

bool hexDigitValue(quint8 nCharacter, quint32 *pValue)
{
    if ((nCharacter >= '0') && (nCharacter <= '9')) {
        *pValue = quint32(nCharacter - '0');
        return true;
    }
    if ((nCharacter >= 'A') && (nCharacter <= 'F')) {
        *pValue = quint32(nCharacter - 'A') + 10U;
        return true;
    }
    if ((nCharacter >= 'a') && (nCharacter <= 'f')) {
        *pValue = quint32(nCharacter - 'a') + 10U;
        return true;
    }
    return false;
}
}  // namespace

XSmsiPakArchive::XSmsiPakArchive(QIODevice *pDevice, FT fileType)
    : XGameStoreArchiveBase(pDevice, fileType)
{
}

XBinary *XSmsiPakArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                         XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSmsiPakArchive(pDevice, getFileType());
}

bool XSmsiPakArchive::isValid(QIODevice *pDevice, FT fileType,
                              PDSTRUCT *pPdStruct)
{
    if ((fileType != FT_SMSIPAK) && (fileType != FT_PSN_COMPRESS)) {
        return false;
    }

    XSmsiPakArchive archive(pDevice, fileType);
    return archive.isValid(pPdStruct);
}

XBinary::FT XSmsiPakArchive::detectFileType(QIODevice *pDevice,
                                            PDSTRUCT *pPdStruct)
{
    if (isValid(pDevice, FT_SMSIPAK, pPdStruct)) return FT_SMSIPAK;
    if (isValid(pDevice, FT_PSN_COMPRESS, pPdStruct)) return FT_PSN_COMPRESS;

    return FT_UNKNOWN;
}

bool XSmsiPakArchive::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                                 PDSTRUCT *pPdStruct)
{
    const FT fileType = getFileType();

    if (fileType == FT_SMSIPAK) {
        return scanSmsiPak(pEntries, pArchiveEnd, pPdStruct);
    }
    if (fileType == FT_PSN_COMPRESS) {
        return scanPsnCompress(pEntries, pArchiveEnd, pPdStruct);
    }

    return false;
}

bool XSmsiPakArchive::scanSmsiPak(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                                  PDSTRUCT *pPdStruct)
{
    const qint64 nTotalSize = getSize();
    if (!isPdStructNotCanceled(pPdStruct) ||
        (nTotalSize < (SMSIPAK_HEADER_SIZE + SMSIPAK_RECORD_SIZE))) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, SMSIPAK_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != SMSIPAK_HEADER_SIZE)) return false;
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if (memcmp(pHeader, SMSIPAK_MAGIC, sizeof(SMSIPAK_MAGIC)) != 0) {
        return false;
    }
    if (qFromLittleEndian<quint16>(pHeader + 8) != SMSIPAK_STAMP) return false;

    // +10 is where the members stop, not the file size: the volume index and,
    // on the volumes that carry one, a cross-volume remainder follow it.
    const qint64 nMemberEnd = qint64(qFromLittleEndian<quint32>(pHeader + 10));
    if ((nMemberEnd <= SMSIPAK_HEADER_SIZE) || (nMemberEnd > nTotalSize)) {
        return false;
    }

    QList<ENTRY> listEntries;
    qint64 nPosition = SMSIPAK_HEADER_SIZE;

    while ((nPosition < nMemberEnd) && (listEntries.size() < MAX_RECORDS) &&
           isPdStructNotCanceled(pPdStruct)) {
        if ((nMemberEnd - nPosition) < SMSIPAK_RECORD_SIZE) return false;

        const QByteArray baRecord =
            read_array_process(nPosition, SMSIPAK_RECORD_SIZE, pPdStruct);
        if ((baRecord.size() != SMSIPAK_RECORD_SIZE)) {
            return false;
        }
        const uchar *pRecord =
            reinterpret_cast<const uchar *>(baRecord.constData());

        QString sFileName;
        if (!decodeName(pRecord, SMSIPAK_NAME_SIZE, true, &sFileName)) {
            return false;
        }
        if (qFromLittleEndian<quint16>(pRecord + 32) != 0) return false;

        const quint8 nMethod = pRecord[23];
        if ((nMethod != SMSIPAK_METHOD_DCL) &&
            (nMethod != SMSIPAK_METHOD_STORE)) {
            return false;
        }

        const qint64 nUncompressedSize =
            qint64(qint32(qFromLittleEndian<quint32>(pRecord + 24)));
        const qint64 nPackedSize =
            qint64(qint32(qFromLittleEndian<quint32>(pRecord + 28)));
        if ((nUncompressedSize < 0) || (nPackedSize < 0)) return false;
        if ((nMemberEnd - (nPosition + SMSIPAK_RECORD_SIZE)) < nPackedSize) {
            return false;
        }
        if ((nMethod == SMSIPAK_METHOD_STORE) &&
            (nPackedSize != nUncompressedSize)) {
            return false;
        }

        ENTRY entry = {};
        entry.nHeaderOffset = nPosition;
        entry.nHeaderSize = SMSIPAK_RECORD_SIZE;
        entry.nDataOffset = nPosition + SMSIPAK_RECORD_SIZE;
        entry.nDataSize = nPackedSize;
        entry.nUncompressedSize = nUncompressedSize;
        entry.handleMethod = (nMethod == SMSIPAK_METHOD_STORE)
                                 ? HANDLE_METHOD_STORE
                                 : HANDLE_METHOD_PKWARE_DCL_IMPLODE;
        entry.bCRC32Defined = true;
        entry.nCRC32 = qFromLittleEndian<quint32>(pRecord + 17);
        entry.mtDateTime = XBinary::dosDateTimeToQDateTime(
            qFromLittleEndian<quint16>(pRecord + 13),
            qFromLittleEndian<quint16>(pRecord + 15));
        entry.sFileName = sFileName;
        listEntries.append(entry);

        nPosition = entry.nDataOffset + nPackedSize;
    }

    if (!isPdStructNotCanceled(pPdStruct) ||
        listEntries.isEmpty() || (nPosition != nMemberEnd)) {
        return false;
    }

    // The volume index repeats every record offset, so it is both the end of
    // the container and a cross-check on the walk above. It is treated as
    // optional: a volume that does not carry one still reads.
    qint64 nArchiveEnd = nMemberEnd;
    const qint64 nIndexSize =
        SMSIPAK_INDEX_HEADER_SIZE + qint64(listEntries.size()) * 4;
    if ((nTotalSize - nMemberEnd) >= nIndexSize) {
        const QByteArray baIndex =
            read_array_process(nMemberEnd, nIndexSize, pPdStruct);
        if ((baIndex.size() != nIndexSize)) return false;
        const uchar *pIndex =
            reinterpret_cast<const uchar *>(baIndex.constData());
        bool bIndexValid =
            (qFromLittleEndian<quint32>(pIndex) == 0) &&
            (qint64(qFromLittleEndian<quint16>(pIndex + 4)) ==
             qint64(listEntries.size()));
        for (qint32 i = 0; bIndexValid && (i < listEntries.size()); ++i) {
            const qint64 nRecordOffset =
                qint64(qFromLittleEndian<quint32>(
                    pIndex + SMSIPAK_INDEX_HEADER_SIZE + qint64(i) * 4));
            if (nRecordOffset != listEntries.at(i).nHeaderOffset) {
                bIndexValid = false;
            }
        }
        if (bIndexValid) nArchiveEnd = nMemberEnd + nIndexSize;
    }

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nArchiveEnd;

    return true;
}

bool XSmsiPakArchive::scanPsnCompress(QList<ENTRY> *pEntries,
                                      qint64 *pArchiveEnd,
                                      PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nTotalSize = getSize();
    if (!isPdStructNotCanceled(pPdStruct) ||
        (nTotalSize < (PSN_HEADER_SIZE + 3))) {
        return false;
    }

    const QByteArray baHeader =
        read_array_process(0, PSN_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != PSN_HEADER_SIZE)) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if (memcmp(pHeader, PSN_MAGIC, size_t(PSN_MAGIC_SIZE)) != 0) return false;
    if (memcmp(pHeader + PSN_MAGIC_SIZE, "0001",
               size_t(PSN_VERSION_SIZE)) != 0) {
        return false;
    }

    quint32 nDeclared = 0;
    for (qint64 i = 0; i < PSN_SIZE_DIGITS; ++i) {
        quint32 nDigit = 0;
        if (!hexDigitValue(pHeader[PSN_MAGIC_SIZE + PSN_VERSION_SIZE + i],
                           &nDigit)) {
            return false;
        }
        nDeclared = (nDeclared << 4U) | nDigit;
    }

    const qint64 nPackedSize = nTotalSize - PSN_HEADER_SIZE;
    if (nPackedSize > PSN_MAX_STREAM) return false;

    const QByteArray baPacked =
        read_array_process(PSN_HEADER_SIZE, nPackedSize, pPdStruct);
    if ((baPacked.size() != nPackedSize)) {
        return false;
    }

    qint64 nConsumed = 0;
    qint64 nRawSize = 0;
    if (!XDclDecoder::scan(
            reinterpret_cast<const uchar *>(baPacked.constData()), nPackedSize,
            PSN_MAX_OUTPUT, &nConsumed, &nRawSize)) {
        return false;
    }
    if (nRawSize < 0) return false;

    // The writer stores 0xFFFFFFFF when it did not know the plaintext size in
    // advance, which is how the reference reads it too - as a signed value
    // whose -1 means "unknown". Anything else has to match the trial decode.
    const qint64 nDeclaredSize = qint64(qint32(nDeclared));
    if ((nDeclaredSize >= 0) && (nDeclaredSize != nRawSize)) return false;

    // Nothing in the container names the payload, so the member is named after
    // the file that holds it - the same name the reference publishes. There is
    // exactly one member, so it can never collide with a sibling.
    QString sFileName =
        QFileInfo(getDeviceFileName(guardedSource)).fileName();
    if (sFileName.isEmpty()) sFileName = QStringLiteral("data");

    ENTRY entry = {};
    entry.nHeaderOffset = 0;
    entry.nHeaderSize = PSN_HEADER_SIZE;
    entry.nDataOffset = PSN_HEADER_SIZE;
    entry.nDataSize = nPackedSize;
    entry.nUncompressedSize = nRawSize;
    entry.handleMethod = HANDLE_METHOD_PKWARE_DCL_IMPLODE;
    entry.sFileName = sFileName;

    QList<ENTRY> listEntries;
    listEntries.append(entry);

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;

    return true;
}
