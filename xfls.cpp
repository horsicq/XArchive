/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free reader for IBM SaveRam/SaveRam2 FLS archives.
 * MIT License
 */
#include "xfls.h"

#include <QHash>
#include <QPointer>
#include <QSet>

namespace {
const qint64 FLS_RECORD_SIZE = 44;
const qint32 FLS_MAX_PATH_COMPONENTS = 64;

QString decodeCP437(const QByteArray &baData)
{
    static const quint16 g_anCP437HighBytes[128] = {
        0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
        0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
        0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
        0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
        0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
        0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
        0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x03BC, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
        0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
    };

    QString sResult;
    sResult.reserve(baData.size());
    for (char cValue : baData) {
        const quint8 nValue = static_cast<quint8>(cValue);
        sResult.append(QChar((nValue < 0x80) ? nValue
                                             : g_anCP437HighBytes[nValue - 0x80]));
    }
    return sResult;
}

bool isSafeLeafName(const QString &sName)
{
    if (sName.isEmpty() || sName == QStringLiteral(".") ||
        sName == QStringLiteral("..") || sName.contains(QLatin1Char('/')) ||
        sName.contains(QLatin1Char('\\')) || sName.contains(QLatin1Char(':'))) {
        return false;
    }
    for (QChar cValue : sName) {
        if (cValue.unicode() < 0x20) return false;
    }
    return XBinary::fixFileName(sName) == sName;
}
}  // namespace

XFLS::XFLS(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_FLS)
{
}

bool XFLS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFLS archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XFLS::createInstance(QIODevice *pDevice, bool bIsImage,
                              XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFLS(pDevice);
}

bool XFLS::scanFormat(QList<ENTRY> *pEntries, qint64 *pArchiveEnd,
                      PDSTRUCT *pPdStruct)
{
    QPointer<XFLS> guardedThis(this);
    const qint64 nTotalSize = getSize();
    if (!guardedThis || nTotalSize < (2 + 2 * FLS_RECORD_SIZE + 9) ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const qint64 nRecords = read_uint16(0);
    if (nRecords < 2 || nRecords > MAX_RECORDS ||
        nRecords > ((nTotalSize - 4) / FLS_RECORD_SIZE)) {
        return false;
    }
    const qint64 nFFPosition = 2 + nRecords * FLS_RECORD_SIZE;
    if (!rangeWithin(nTotalSize, nFFPosition, 9) ||
        read_uint16(nFFPosition) != 0xffffU ||
        read_array(nFFPosition + 2, 7) != QByteArray("SaveRam", 7)) {
        return false;
    }

    // Header-record discriminator, followed by the first member marker.
    if (read_uint8(2) != 0xfeU || read_uint8(3) != 0U ||
        read_array(4, 6) != QByteArray("\x00\x00\x01\x00\x00\xff", 6) ||
        read_uint8(2 + FLS_RECORD_SIZE) != 0xfeU) {
        return false;
    }

    const qint64 nNameBlockSize = read_uint32(2 + 23);
    if (nNameBlockSize < 9 || nNameBlockSize > (nTotalSize - nFFPosition)) {
        return false;
    }
    const qint64 nDataStart = nFFPosition + nNameBlockSize;
    if (!rangeWithin(nTotalSize, nFFPosition, nNameBlockSize)) return false;

    QList<ENTRY> listEntries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirectories;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirectories;
    qint64 nExpectedDataOffset = nDataStart;

    for (qint64 i = 1; i < nRecords; ++i) {
        if (!guardedThis || !isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nRecordOffset = 2 + i * FLS_RECORD_SIZE;
        const QByteArray baMemberTag = read_array(nRecordOffset + 2, 6);
        if (!rangeWithin(nTotalSize, nRecordOffset, FLS_RECORD_SIZE) ||
            read_uint8(nRecordOffset) != 0xfeU ||
            baMemberTag.left(5) != QByteArray("\x00\x00\x00\x01\x20", 5) ||
            (static_cast<quint8>(baMemberTag.at(5)) != 0x4dU &&
             static_cast<quint8>(baMemberTag.at(5)) != 0x4fU)) {
            return false;
        }

        const quint8 nCompressedFlag = read_uint8(nRecordOffset + 1);
        const qint64 nNameReference = read_uint32(nRecordOffset + 8);
        const qint64 nPathComponents = read_uint16(nRecordOffset + 12);
        const qint64 nTailSize = read_uint32(nRecordOffset + 14);
        const qint64 nDataOffset = read_uint32(nRecordOffset + 19);
        const qint64 nCompressedSize = read_uint32(nRecordOffset + 23);
        const quint16 nDosDate = read_uint16(nRecordOffset + 27);
        const quint16 nDosTime = read_uint16(nRecordOffset + 29);
        const qint64 nUncompressedSize = read_uint32(nRecordOffset + 31);

        if (nCompressedFlag > 1 || nPathComponents < 1 ||
            nPathComponents > FLS_MAX_PATH_COMPONENTS ||
            nNameReference < 2 ||
            nNameReference > (nNameBlockSize - nPathComponents * 4) ||
            nDataOffset != nExpectedDataOffset || nCompressedSize < 0 ||
            !rangeWithin(nTotalSize, nDataOffset, nCompressedSize) ||
            nTailSize > (nTotalSize - nDataOffset - nCompressedSize) ||
            (nTailSize != 0 && i != (nRecords - 1))) {
            return false;
        }

        const qint64 nLastReferenceOffset =
            nFFPosition + nNameReference + (nPathComponents - 1) * 4;
        const qint64 nNameOffset = nFFPosition + read_uint32(nLastReferenceOffset);
        if (nNameOffset < (nFFPosition + 2) || nNameOffset >= nDataStart ||
            !rangeWithin(nDataStart, nNameOffset, 1)) {
            return false;
        }
        const qint64 nNameSize = read_uint8(nNameOffset);
        if (nNameSize < 1 || !rangeWithin(nDataStart, nNameOffset + 1, nNameSize)) {
            return false;
        }
        const QString sLeafName = decodeCP437(read_array(nNameOffset + 1, nNameSize));
        if (!isSafeLeafName(sLeafName)) return false;

        if ((nCompressedFlag == 0 && nCompressedSize != nUncompressedSize) ||
            (nCompressedFlag == 1 &&
             (nCompressedSize < 1 || read_uint8(nDataOffset) != 0x53U))) {
            return false;
        }

        QString sUniqueName;
        if (!makeUniquePath(sLeafName, &usedFiles, &usedDirectories,
                            &nextSuffixes, &resolvedDirectories,
                            &sUniqueName)) {
            return false;
        }

        ENTRY entry = {};
        entry.nHeaderOffset = nRecordOffset;
        entry.nHeaderSize = FLS_RECORD_SIZE;
        entry.nDataOffset = nDataOffset;
        entry.nDataSize = nCompressedSize;
        entry.nUncompressedSize = nUncompressedSize;
        entry.handleMethod = (nCompressedFlag == 0)
            ? HANDLE_METHOD_STORE : HANDLE_METHOD_FLS_LZ;
        entry.mtDateTime = XBinary::dosDateTimeToQDateTime(nDosDate, nDosTime);
        entry.sFileName = sUniqueName;
        listEntries.append(entry);

        nExpectedDataOffset += nCompressedSize + nTailSize;
    }

    if (!guardedThis || !isPdStructNotCanceled(pPdStruct) ||
        listEntries.size() != (nRecords - 1) ||
        nExpectedDataOffset != nTotalSize) {
        return false;
    }

    if (pEntries) *pEntries = listEntries;
    if (pArchiveEnd) *pArchiveEnd = nTotalSize;
    return true;
}
