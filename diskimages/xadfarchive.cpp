/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Native, execution-free reader for standard AmigaDOS floppy images.
 * MIT License
 */
#include "xadfarchive.h"

#include <QtEndian>

#include <QBuffer>
#include <QPointer>
#include <QSet>
#include <QTimeZone>

#include <memory>
#include <new>

namespace {
const qint64 ADF_DD_IMAGE_SIZE = Q_INT64_C(901120);
const qint64 ADF_HD_IMAGE_SIZE = Q_INT64_C(1802240);
const qint32 ADF_BLOCK_SIZE = 512;
const qint32 ADF_MAX_BLOCK_COUNT = 3520;
const qint32 ADF_BOOT_BLOCKS = 2;
const qint32 ADF_HASH_ENTRIES = 72;
const qint32 ADF_MAX_NAME_SIZE = 30;
const qint32 ADF_MAX_DIRECTORY_DEPTH = 128;
const qint32 ADF_MAX_MEMBERS = ADF_MAX_BLOCK_COUNT - ADF_BOOT_BLOCKS - 1;
const qint32 ADF_OFS_DATA_OFFSET = 24;
const qint32 ADF_OFS_DATA_SIZE = ADF_BLOCK_SIZE - ADF_OFS_DATA_OFFSET;

const qint32 WORD_TYPE = 0;
const qint32 WORD_HEADER_KEY = 1;
const qint32 WORD_HIGH_SEQ = 2;
const qint32 WORD_DATA_SIZE = 3;
const qint32 WORD_FIRST_DATA = 4;
const qint32 WORD_CHECKSUM = 5;
const qint32 WORD_DATA_BLOCK_FIRST = 6;
const qint32 WORD_DATA_BLOCK_LAST = 77;
const qint32 WORD_BITMAP_FLAG = 78;
const qint32 WORD_BITMAP_PAGE_FIRST = 79;
const qint32 WORD_BITMAP_PAGE_LAST = 103;
const qint32 WORD_BITMAP_EXTENSION = 104;
const qint32 WORD_PROTECTION = 80;
const qint32 WORD_BYTE_SIZE = 81;
const qint32 WORD_DATE_DAYS = 105;
const qint32 WORD_DATE_MINS = 106;
const qint32 WORD_DATE_TICKS = 107;
const qint32 WORD_NAME = 108;
const qint32 WORD_HASH_CHAIN = 124;
const qint32 WORD_PARENT = 125;
const qint32 WORD_EXTENSION = 126;
const qint32 WORD_SEC_TYPE = 127;

const quint32 TYPE_HEADER = 2U;
const quint32 TYPE_DATA = 8U;
const quint32 TYPE_LIST = 16U;
const quint32 TYPE_DIRCACHE = 33U;
const qint32 ADF_MAX_DIRCACHE_RECORDS = 32;  // 488 payload bytes / 22-byte minimum record
const quint8 ADF_MAX_DOS_TYPE = 5U;          // DOS\0..DOS\5; DOS\6/7 long names are not read
const quint32 SEC_TYPE_ROOT = 1U;
const quint32 SEC_TYPE_USERDIR = 2U;
const quint32 SEC_TYPE_SOFTLINK = 3U;
const quint32 SEC_TYPE_LINKDIR = 4U;
const quint32 SEC_TYPE_FILE = 0xfffffffdU;
const quint32 SEC_TYPE_LINKFILE = 0xfffffffcU;

quint32 readBigEndian32(const QByteArray &baBlock, qint32 nWord)
{
    if ((nWord < 0) || ((nWord + 1) * 4 > baBlock.size())) return 0;
    const uchar *pData = reinterpret_cast<const uchar *>(baBlock.constData());
    return qFromBigEndian<quint32>(pData + nWord * 4);
}

quint32 readBigEndian32At(const QByteArray &baData, qint32 nOffset)
{
    if ((nOffset < 0) || (nOffset + 4 > baData.size())) return 0;
    const uchar *pData = reinterpret_cast<const uchar *>(baData.constData());
    return qFromBigEndian<quint32>(pData + nOffset);
}

bool isBlockChecksumValid(const QByteArray &baBlock)
{
    if (baBlock.size() != ADF_BLOCK_SIZE) return false;

    quint64 nSum = 0;
    for (qint32 i = 0; i < (ADF_BLOCK_SIZE / 4); ++i) {
        nSum += readBigEndian32(baBlock, i);
    }
    return (nSum & Q_UINT64_C(0xffffffff)) == 0;
}

bool isImageBlock(qint64 nBlock)
{
    return (nBlock >= 0) && (nBlock < ADF_MAX_BLOCK_COUNT);
}

bool isPhysicalDataBlock(qint64 nBlock, qint32 nBlockCount, qint32 nRootBlock)
{
    return (nBlock >= ADF_BOOT_BLOCKS) && (nBlock < nBlockCount) &&
           (nBlock != nRootBlock);
}

bool isSupportedSecondaryType(quint32 nType)
{
    return (nType == SEC_TYPE_FILE) || (nType == SEC_TYPE_USERDIR) ||
           (nType == SEC_TYPE_SOFTLINK) || (nType == SEC_TYPE_LINKDIR) ||
           (nType == SEC_TYPE_LINKFILE);
}

QString appendDuplicateSuffix(const QString &sName, qint32 nSuffix)
{
    const qint32 nDot = sName.lastIndexOf(QLatin1Char('.'));
    const QString sSuffix = QStringLiteral("_%1").arg(nSuffix);
    if (nDot > 0) return sName.left(nDot) + sSuffix + sName.mid(nDot);
    return sName + sSuffix;
}

bool decodeAmigaName(const QByteArray &baBlock, QString *pName,
                     QByteArray *pRawName, bool bAllowEmpty)
{
    if (!pName || (baBlock.size() != ADF_BLOCK_SIZE)) return false;

    const qint32 nOffset = WORD_NAME * 4;
    const quint8 nLength = static_cast<quint8>(baBlock.at(nOffset));
    if ((nLength > ADF_MAX_NAME_SIZE) ||
        (static_cast<quint8>(baBlock.at(nOffset + 31)) != 0U)) {
        return false;
    }
    if ((nLength == 0) && !bAllowEmpty) return false;

    const QByteArray baName = baBlock.mid(nOffset + 1, nLength);
    for (char cValue : baName) {
        const quint8 nValue = static_cast<quint8>(cValue);
        if ((nValue < 0x20U) || ((nValue >= 0x80U) && (nValue <= 0x9fU)) ||
            (nValue == 0x7fU) ||
            (nValue == static_cast<quint8>('/')) ||
            (nValue == static_cast<quint8>('\\')) ||
            (nValue == static_cast<quint8>(':'))) {
            return false;
        }
    }

    QString sName = QString::fromLatin1(baName)
                        .normalized(QString::NormalizationForm_C);
    if (!bAllowEmpty && (sName.isEmpty() || (sName == QLatin1String(".")) ||
                         (sName == QLatin1String("..")))) {
        return false;
    }
    if (!sName.isEmpty() && (XBinary::fixFileName(sName) != sName))
        return false;

    *pName = sName;
    if (pRawName) *pRawName = baName;
    return true;
}

// The international variants (DOS\2 and later) also fold the Latin-1 letters
// 0xE0-0xFE (except 0xF7, the division sign) to upper case in the hash.
qint32 amigaNameHash(const QByteArray &baName, bool bInternational)
{
    quint32 nHash = static_cast<quint32>(baName.size());
    for (char cValue : baName) {
        quint32 nValue = static_cast<quint8>(cValue);
        if ((nValue >= static_cast<quint32>('a')) &&
            (nValue <= static_cast<quint32>('z'))) {
            nValue -= static_cast<quint32>('a' - 'A');
        } else if (bInternational && (nValue >= 0xe0U) && (nValue <= 0xfeU) &&
                   (nValue != 0xf7U)) {
            nValue -= 0x20U;
        }
        nHash = (nHash * 13U + nValue) & 0x7ffU;
    }
    return static_cast<qint32>(nHash % ADF_HASH_ENTRIES);
}

QDateTime amigaDateTime(const QByteArray &baBlock)
{
    const qint64 nDays = readBigEndian32(baBlock, WORD_DATE_DAYS);
    const qint64 nMinutes = readBigEndian32(baBlock, WORD_DATE_MINS);
    const qint64 nTicks = readBigEndian32(baBlock, WORD_DATE_TICKS);
    if ((nDays > 1000000) || (nMinutes >= (24 * 60)) || (nTicks >= 3000))
        return QDateTime();

    const QDate dtDate = QDate(1978, 1, 1).addDays(nDays);
    const QTime tmTime = QTime(0, 0)
                             .addSecs(static_cast<int>(nMinutes * 60 + nTicks / 50))
                             .addMSecs(static_cast<int>((nTicks % 50) * 20));
    if (!dtDate.isValid() || !tmTime.isValid()) return QDateTime();
    return QDateTime(dtDate, tmTime, QTimeZone(0));
}
}  // namespace

struct XADFArchive::PARSER {
    QPointer<XADFArchive> guardedArchive;
    CONTEXT *pContext;
    PDSTRUCT *pPdStruct;
    QSet<qint32> stReservedBlocks;
    QSet<qint32> stMetadataBlocks;
    QSet<qint32> stDataBlocks;
    QSet<qint32> stParsedDirectories;
    QSet<QString> stOutputPaths;

    PARSER(XADFArchive *pArchive, CONTEXT *pParseContext,
           PDSTRUCT *pProgress)
        : guardedArchive(pArchive), pContext(pParseContext),
          pPdStruct(pProgress)
    {
    }

    bool isUsableBlock(qint64 nBlock) const
    {
        return isPhysicalDataBlock(nBlock, pContext->nBlockCount, pContext->nRootBlock) &&
               !stReservedBlocks.contains(static_cast<qint32>(nBlock));
    }

    bool isReady() const
    {
        return guardedArchive && pContext &&
               XBinary::isPdStructNotCanceled(pPdStruct);
    }

    bool readBlock(qint32 nBlock, QByteArray *pBlock)
    {
        if (!isReady() || !isPhysicalDataBlock(nBlock, pContext->nBlockCount, pContext->nRootBlock) || !pBlock)
            return false;
        if (!guardedArchive->readBlock(nBlock, pBlock, pPdStruct) ||
            !guardedArchive || !isBlockChecksumValid(*pBlock) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }
        return true;
    }

    bool claimMetadataBlock(qint32 nBlock)
    {
        if (!isUsableBlock(nBlock) || stMetadataBlocks.contains(nBlock) ||
            stDataBlocks.contains(nBlock)) {
            return false;
        }
        stMetadataBlocks.insert(nBlock);
        return true;
    }

    bool claimDataBlock(qint32 nBlock)
    {
        if (!isUsableBlock(nBlock) || stMetadataBlocks.contains(nBlock) ||
            stDataBlocks.contains(nBlock)) {
            return false;
        }
        stDataBlocks.insert(nBlock);
        return true;
    }

    // DOS\4 / DOS\5: word 126 of the root and of every user directory points
    // to a chain of directory-cache blocks (type 33).  The cache duplicates
    // the hash-table entries and is never used for listing here; each block is
    // authenticated (checksum, self key, owning directory, record count) and
    // claimed as metadata so no file can alias it.
    bool validateDirCacheChain(qint64 nFirstBlock, qint32 nDirectoryBlock)
    {
        if (nFirstBlock == 0) return false;
        QSet<qint32> stChain;
        qint64 nBlock = nFirstBlock;
        while (nBlock != 0) {
            if (!isReady() || !isUsableBlock(nBlock) ||
                stChain.contains(static_cast<qint32>(nBlock)) ||
                !claimMetadataBlock(static_cast<qint32>(nBlock))) {
                return false;
            }
            stChain.insert(static_cast<qint32>(nBlock));
            QByteArray baCache;
            if (!readBlock(static_cast<qint32>(nBlock), &baCache) ||
                (readBigEndian32(baCache, WORD_TYPE) != TYPE_DIRCACHE) ||
                (readBigEndian32(baCache, WORD_HEADER_KEY) !=
                 static_cast<quint32>(nBlock)) ||
                (readBigEndian32(baCache, WORD_HIGH_SEQ) !=
                 static_cast<quint32>(nDirectoryBlock)) ||
                (readBigEndian32(baCache, WORD_DATA_SIZE) >
                 static_cast<quint32>(ADF_MAX_DIRCACHE_RECORDS))) {
                return false;
            }
            nBlock = readBigEndian32(baCache, WORD_FIRST_DATA);
        }
        return true;
    }

    bool isDirectoryExtensionValid(qint32 nDirectoryBlock, quint32 nExtension)
    {
        if (!pContext->bDirCache) return nExtension == 0U;
        return validateDirCacheChain(nExtension, nDirectoryBlock);
    }

    bool makeUniquePath(const QString &sParent, const QString &sLeaf,
                        QString *pResult)
    {
        if (!pResult || sLeaf.isEmpty()) return false;

        for (qint32 nSuffix = 1; nSuffix <= (ADF_MAX_MEMBERS + 1); ++nSuffix) {
            const QString sCandidateLeaf = (nSuffix == 1)
                ? sLeaf
                : appendDuplicateSuffix(sLeaf, nSuffix);
            const QString sCandidate = sParent.isEmpty()
                ? sCandidateLeaf
                : (sParent + QLatin1Char('/') + sCandidateLeaf);
            const QString sKey = sCandidate.toCaseFolded();
            if ((XBinary::fixFileName(sCandidate) != sCandidate) ||
                stOutputPaths.contains(sKey)) {
                continue;
            }
            stOutputPaths.insert(sKey);
            *pResult = sCandidate;
            return true;
        }
        return false;
    }

    bool appendPointers(const QByteArray &baHeader, qint32 nCount,
                        QList<DATA_BLOCK> *pDataBlocks)
    {
        if (!pDataBlocks || (nCount < 0) || (nCount > ADF_HASH_ENTRIES))
            return false;

        const qint32 nFirstUsed = WORD_DATA_BLOCK_LAST + 1 - nCount;
        for (qint32 i = WORD_DATA_BLOCK_FIRST; i < nFirstUsed; ++i) {
            if (readBigEndian32(baHeader, i) != 0U) return false;
        }
        for (qint32 i = WORD_DATA_BLOCK_LAST; i >= nFirstUsed; --i) {
            const qint64 nPointer = readBigEndian32(baHeader, i);
            if (!isUsableBlock(nPointer) ||
                !claimDataBlock(static_cast<qint32>(nPointer))) {
                return false;
            }
            DATA_BLOCK dataBlock = {};
            dataBlock.nBlock = static_cast<qint32>(nPointer);
            dataBlock.nPayloadOffset = pContext->bOFS ? ADF_OFS_DATA_OFFSET : 0;
            pDataBlocks->append(dataBlock);
        }
        return true;
    }

    bool parseFile(qint32 nHeaderBlock, const QByteArray &baHeader,
                   MEMBER *pMember)
    {
        if (!pMember || !isReady() ||
            (readBigEndian32(baHeader, WORD_TYPE) != TYPE_HEADER) ||
            (readBigEndian32(baHeader, WORD_HEADER_KEY) !=
             static_cast<quint32>(nHeaderBlock)) ||
            (readBigEndian32(baHeader, WORD_DATA_SIZE) != 0U) ||
            (readBigEndian32(baHeader, WORD_SEC_TYPE) != SEC_TYPE_FILE)) {
            return false;
        }

        const quint32 nPointers = readBigEndian32(baHeader, WORD_HIGH_SEQ);
        const qint64 nSize = readBigEndian32(baHeader, WORD_BYTE_SIZE);
        const qint64 nPayloadCapacity = pContext->bOFS
            ? ADF_OFS_DATA_SIZE : ADF_BLOCK_SIZE;
        if ((nPointers > static_cast<quint32>(ADF_HASH_ENTRIES)) ||
            (nSize > (static_cast<qint64>(ADF_MAX_MEMBERS) *
                      nPayloadCapacity))) {
            return false;
        }

        QList<DATA_BLOCK> listDataBlocks;
        if (!appendPointers(baHeader, static_cast<qint32>(nPointers),
                            &listDataBlocks))
            return false;

        QSet<qint32> stExtensions;
        qint64 nExtension = readBigEndian32(baHeader, WORD_EXTENSION);
        while (nExtension != 0) {
            if (!isUsableBlock(nExtension) ||
                stExtensions.contains(static_cast<qint32>(nExtension)) ||
                !claimMetadataBlock(static_cast<qint32>(nExtension))) {
                return false;
            }
            stExtensions.insert(static_cast<qint32>(nExtension));

            QByteArray baExtension;
            if (!readBlock(static_cast<qint32>(nExtension), &baExtension) ||
                (readBigEndian32(baExtension, WORD_TYPE) != TYPE_LIST) ||
                (readBigEndian32(baExtension, WORD_HEADER_KEY) !=
                 static_cast<quint32>(nExtension)) ||
                (readBigEndian32(baExtension, WORD_DATA_SIZE) != 0U) ||
                (readBigEndian32(baExtension, WORD_FIRST_DATA) != 0U) ||
                (readBigEndian32(baExtension, WORD_HASH_CHAIN) != 0U) ||
                (readBigEndian32(baExtension, WORD_PARENT) !=
                 static_cast<quint32>(nHeaderBlock)) ||
                (readBigEndian32(baExtension, WORD_SEC_TYPE) != SEC_TYPE_FILE)) {
                return false;
            }

            const quint32 nExtensionPointers =
                readBigEndian32(baExtension, WORD_HIGH_SEQ);
            if ((nExtensionPointers == 0U) ||
                (nExtensionPointers >
                 static_cast<quint32>(ADF_HASH_ENTRIES)) ||
                !appendPointers(baExtension,
                                static_cast<qint32>(nExtensionPointers),
                                &listDataBlocks)) {
                return false;
            }
            nExtension = readBigEndian32(baExtension, WORD_EXTENSION);
        }

        const qint64 nExpectedBlocks = nSize / nPayloadCapacity +
            ((nSize % nPayloadCapacity) ? 1 : 0);
        if ((listDataBlocks.size() != nExpectedBlocks) ||
            ((nSize == 0) &&
             (readBigEndian32(baHeader, WORD_FIRST_DATA) != 0U)) ||
            ((nSize > 0) &&
             (readBigEndian32(baHeader, WORD_FIRST_DATA) !=
              static_cast<quint32>(listDataBlocks.constFirst().nBlock)))) {
            return false;
        }

        qint64 nRemaining = nSize;
        for (qint32 i = 0; i < listDataBlocks.size(); ++i) {
            DATA_BLOCK &dataBlock = listDataBlocks[i];
            if (!pContext->bOFS) {
                dataBlock.nPayloadSize = static_cast<qint32>(
                    qMin<qint64>(nRemaining, ADF_BLOCK_SIZE));
                nRemaining -= dataBlock.nPayloadSize;
                continue;
            }

            QByteArray baData;
            if (!readBlock(dataBlock.nBlock, &baData) ||
                (readBigEndian32(baData, WORD_TYPE) != TYPE_DATA) ||
                (readBigEndian32(baData, WORD_HEADER_KEY) !=
                 static_cast<quint32>(nHeaderBlock)) ||
                (readBigEndian32(baData, WORD_HIGH_SEQ) !=
                 static_cast<quint32>(i + 1))) {
                return false;
            }

            const qint64 nDataSize = readBigEndian32(baData, WORD_DATA_SIZE);
            const quint32 nExpectedNext = (i + 1 < listDataBlocks.size())
                ? static_cast<quint32>(listDataBlocks.at(i + 1).nBlock)
                : 0U;
            if ((nDataSize <= 0) || (nDataSize > ADF_OFS_DATA_SIZE) ||
                (readBigEndian32(baData, WORD_FIRST_DATA) != nExpectedNext) ||
                (nDataSize > nRemaining)) {
                return false;
            }
            dataBlock.nPayloadSize = static_cast<qint32>(nDataSize);
            nRemaining -= nDataSize;
        }

        if (nRemaining != 0) return false;
        pMember->nHeaderBlock = nHeaderBlock;
        pMember->nSize = nSize;
        pMember->nStoredSize = nSize;
        pMember->nProtection = readBigEndian32(baHeader, WORD_PROTECTION);
        pMember->mtDateTime = amigaDateTime(baHeader);
        pMember->listDataBlocks = listDataBlocks;
        return true;
    }

    bool parseDirectory(qint32 nDirectoryBlock, const QByteArray &baDirectory,
                        const QString &sDirectoryPath, qint32 nDepth)
    {
        if (!isReady() || (nDepth > ADF_MAX_DIRECTORY_DEPTH) ||
            stParsedDirectories.contains(nDirectoryBlock)) {
            return false;
        }
        stParsedDirectories.insert(nDirectoryBlock);

        for (qint32 nBucket = 0; nBucket < ADF_HASH_ENTRIES; ++nBucket) {
            qint64 nEntryBlock = readBigEndian32(
                baDirectory, WORD_DATA_BLOCK_FIRST + nBucket);
            QSet<qint32> stHashChain;
            while (nEntryBlock != 0) {
                if (!isUsableBlock(nEntryBlock) ||
                    stHashChain.contains(static_cast<qint32>(nEntryBlock)) ||
                    !isReady()) {
                    return false;
                }
                stHashChain.insert(static_cast<qint32>(nEntryBlock));

                QByteArray baEntry;
                const qint32 nCurrentEntry = static_cast<qint32>(nEntryBlock);
                if (!readBlock(nCurrentEntry, &baEntry) ||
                    (readBigEndian32(baEntry, WORD_TYPE) != TYPE_HEADER) ||
                    (readBigEndian32(baEntry, WORD_HEADER_KEY) !=
                     static_cast<quint32>(nCurrentEntry)) ||
                    (readBigEndian32(baEntry, WORD_PARENT) !=
                     static_cast<quint32>(nDirectoryBlock)) ||
                    !isSupportedSecondaryType(
                        readBigEndian32(baEntry, WORD_SEC_TYPE)) ||
                    !claimMetadataBlock(nCurrentEntry)) {
                    return false;
                }

                QString sLeafName;
                QByteArray baRawName;
                if (!decodeAmigaName(baEntry, &sLeafName, &baRawName, false) ||
                    (amigaNameHash(baRawName, pContext->bInternational) !=
                     nBucket)) {
                    return false;
                }

                nEntryBlock = readBigEndian32(baEntry, WORD_HASH_CHAIN);
                const quint32 nSecondaryType =
                    readBigEndian32(baEntry, WORD_SEC_TYPE);
                if (nSecondaryType == SEC_TYPE_FILE) {
                    if (pContext->listMembers.size() >= ADF_MAX_MEMBERS)
                        return false;
                    MEMBER member = {};
                    if (!makeUniquePath(sDirectoryPath, sLeafName,
                                        &member.sPath) ||
                        !parseFile(nCurrentEntry, baEntry, &member)) {
                        return false;
                    }
                    pContext->listMembers.append(member);
                } else if (nSecondaryType == SEC_TYPE_USERDIR) {
                    if ((readBigEndian32(baEntry, WORD_HIGH_SEQ) != 0U) ||
                        (readBigEndian32(baEntry, WORD_DATA_SIZE) != 0U) ||
                        (readBigEndian32(baEntry, WORD_FIRST_DATA) != 0U) ||
                        !isDirectoryExtensionValid(
                            nCurrentEntry,
                            readBigEndian32(baEntry, WORD_EXTENSION)) ||
                        (pContext->listMembers.size() >= ADF_MAX_MEMBERS)) {
                        return false;
                    }
                    MEMBER member = {};
                    member.bIsDirectory = true;
                    member.nHeaderBlock = nCurrentEntry;
                    member.nProtection =
                        readBigEndian32(baEntry, WORD_PROTECTION);
                    member.mtDateTime = amigaDateTime(baEntry);
                    if (!makeUniquePath(sDirectoryPath, sLeafName,
                                        &member.sPath)) {
                        return false;
                    }
                    pContext->listMembers.append(member);
                    if (!parseDirectory(nCurrentEntry, baEntry, member.sPath,
                                        nDepth + 1)) {
                        return false;
                    }
                }
                // Hard and soft links have a different block layout.  Their
                // hash-chain entry was fully authenticated above, but they
                // intentionally remain out of this regular-file/directory
                // reader rather than being followed as host filesystem links.
            }
        }
        return isReady();
    }

    bool parseRoot()
    {
        if (!isReady()) return false;

        const QByteArray baBoot = guardedArchive->read_array_process(
            0, 12, pPdStruct);
        if (!guardedArchive || (baBoot.size() != 12) ||
            (baBoot.left(3) != QByteArray("DOS", 3)) ||
            (static_cast<quint8>(baBoot.at(3)) > ADF_MAX_DOS_TYPE) ||
            ((readBigEndian32At(baBoot, 8) !=
              0U) && (readBigEndian32At(baBoot, 8) !=
                      static_cast<quint32>(pContext->nRootBlock)))) {
            return false;
        }
        // DOS\n flag bits: 1 = FFS, 2 = international, 4 = directory cache
        // (which implies the international hash).
        pContext->nDosType = static_cast<quint8>(baBoot.at(3));
        pContext->bOFS = (pContext->nDosType & 1U) == 0U;
        pContext->bInternational = (pContext->nDosType & 6U) != 0U;
        pContext->bDirCache = (pContext->nDosType & 4U) != 0U;

        QByteArray baRoot;
        if (!guardedArchive->readBlock(pContext->nRootBlock, &baRoot, pPdStruct) ||
            !guardedArchive || !isBlockChecksumValid(baRoot) ||
            (readBigEndian32(baRoot, WORD_TYPE) != TYPE_HEADER) ||
            (readBigEndian32(baRoot, WORD_HEADER_KEY) != 0U) ||
            (readBigEndian32(baRoot, WORD_HIGH_SEQ) != 0U) ||
            (readBigEndian32(baRoot, WORD_DATA_SIZE) !=
             static_cast<quint32>(ADF_HASH_ENTRIES)) ||
            (readBigEndian32(baRoot, WORD_FIRST_DATA) != 0U) ||
            (readBigEndian32(baRoot, WORD_HASH_CHAIN) != 0U) ||
            (readBigEndian32(baRoot, WORD_PARENT) != 0U) ||
            (readBigEndian32(baRoot, WORD_SEC_TYPE) != SEC_TYPE_ROOT) ||
            (readBigEndian32(baRoot, WORD_BITMAP_EXTENSION) != 0U)) {
            return false;
        }

        const quint32 nBitmapFlag = readBigEndian32(baRoot, WORD_BITMAP_FLAG);
        if ((nBitmapFlag != 0U) && (nBitmapFlag != 0xffffffffU)) return false;

        QString sVolumeName;
        if (!decodeAmigaName(baRoot, &sVolumeName, nullptr, true)) return false;
        pContext->sVolumeName = sVolumeName;

        stReservedBlocks.insert(0);
        stReservedBlocks.insert(1);
        stReservedBlocks.insert(pContext->nRootBlock);
        bool bBitmapPresent = false;
        for (qint32 i = WORD_BITMAP_PAGE_FIRST;
             i <= WORD_BITMAP_PAGE_LAST; ++i) {
            const qint64 nBitmapBlock = readBigEndian32(baRoot, i);
            if (nBitmapBlock == 0) continue;
            if (!isPhysicalDataBlock(nBitmapBlock, pContext->nBlockCount, pContext->nRootBlock) ||
                stReservedBlocks.contains(static_cast<qint32>(nBitmapBlock))) {
                return false;
            }
            QByteArray baBitmap;
            if (!guardedArchive->readBlock(static_cast<qint32>(nBitmapBlock),
                                           &baBitmap, pPdStruct) ||
                !guardedArchive || !isBlockChecksumValid(baBitmap)) {
                return false;
            }
            stReservedBlocks.insert(static_cast<qint32>(nBitmapBlock));
            bBitmapPresent = true;
        }
        if (!bBitmapPresent) return false;

        // The reference implementation does not traverse the root's word126 as a
        // file extension. The ordinary Ancient DD/HD images repeat their
        // bitmap pointer here. Accept that authenticated alias, not an
        // unvalidated directory-cache or arbitrary block reference.
        const quint32 nRootExtension = readBigEndian32(baRoot, WORD_EXTENSION);
        stMetadataBlocks.insert(pContext->nRootBlock);
        if (pContext->bDirCache) {
            // DOS\4 / DOS\5: the root's word 126 is its directory-cache chain.
            if (!validateDirCacheChain(nRootExtension, pContext->nRootBlock))
                return false;
        } else if (nRootExtension != 0U &&
            (nRootExtension >= static_cast<quint32>(pContext->nBlockCount) ||
             nRootExtension < ADF_BOOT_BLOCKS ||
             nRootExtension == static_cast<quint32>(pContext->nRootBlock) ||
             !stReservedBlocks.contains(static_cast<qint32>(nRootExtension)))) {
            return false;
        }

        return parseDirectory(pContext->nRootBlock, baRoot, QString(), 0);
    }
};

XADFArchive::XADFArchive(QIODevice *pDevice)
    : XArchive(pDevice)
{
}

XADFArchive::~XADFArchive()
{
}

bool XADFArchive::readBlock(qint32 nBlock, QByteArray *pBlock,
                             PDSTRUCT *pPdStruct)
{
    if (!pBlock || !isImageBlock(nBlock) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const QByteArray baBlock = read_array_process(
        static_cast<qint64>(nBlock) * ADF_BLOCK_SIZE, ADF_BLOCK_SIZE,
        pPdStruct);
    if (baBlock.size() != ADF_BLOCK_SIZE) return false;
    *pBlock = baBlock;
    return true;
}

bool XADFArchive::parseImage(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    QPointer<XADFArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pContext || !guardedThis || !guardedSource ||
        !guardedSource->isOpen() || !guardedSource->isReadable() ||
        guardedSource->isSequential() || !guardedThis || !guardedSource ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    const qint64 nImageSize = getSize();
    if (!guardedThis || !guardedSource ||
        ((nImageSize != ADF_DD_IMAGE_SIZE) && (nImageSize != ADF_HD_IMAGE_SIZE))) {
        return false;
    }
    // The reference implementation chooses block880 for DD and block1760 for HD images;
    // it derives the root from the geometry instead of the optional boot field.
    pContext->nImageSize = nImageSize;
    pContext->nBlockCount = static_cast<qint32>(nImageSize / ADF_BLOCK_SIZE);
    pContext->nRootBlock = pContext->nBlockCount / 2;
    PARSER parser(guardedThis.data(), pContext, pPdStruct);
    return parser.parseRoot() && guardedThis && guardedSource &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XADFArchive::isValid(PDSTRUCT *pPdStruct)
{
    UNPACK_STATE state = {};
    const bool bResult = initUnpack(&state, QMap<UNPACK_PROP, QVariant>(),
                                    pPdStruct);
    const bool bFinished = finishUnpack(&state, nullptr);
    return bResult && bFinished;
}

bool XADFArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XADFArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary::FT XADFArchive::getFileType()
{
    return FT_AMIGA_ADF;
}

XBinary::MODE XADFArchive::getMode()
{
    return MODE_DATA;
}

QString XADFArchive::getMIMEString()
{
    return QStringLiteral("application/x-amiga-disk-image");
}

QString XADFArchive::getFileFormatExt()
{
    return QStringLiteral("adf");
}

QString XADFArchive::getFileFormatExtsString()
{
    return QStringLiteral("AmigaDOS DD/HD floppy disk image (*.adf)");
}

QList<QString> XADFArchive::getSearchSignatures()
{
    return {QStringLiteral("'DOS'00"), QStringLiteral("'DOS'01"),
            QStringLiteral("'DOS'02"), QStringLiteral("'DOS'03"),
            QStringLiteral("'DOS'04"), QStringLiteral("'DOS'05")};
}

XBinary *XADFArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                     XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XADFArchive(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant>
XADFArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XADFArchive::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XADFArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedThis || !guardedSource ||
        guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct))
        return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) goto failed;
    if (!parseImage(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        goto failed;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.count();
    pState->nCurrentOffset = pContext->listMembers.isEmpty()
        ? pContext->nImageSize
        : static_cast<qint64>(pContext->listMembers.constFirst().nHeaderBlock) *
              ADF_BLOCK_SIZE;
    pState->nTotalSize = pContext->nImageSize;
    pState->pContext = pContext;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct))
        goto failed;
    return true;

failed:
    if (guardedThis) releaseUnpackSource(pState);
    delete pContext;
    *pState = UNPACK_STATE();
    return false;
}

XBinary::ARCHIVERECORD XADFArchive::infoCurrent(UNPACK_STATE *pState,
                                                 PDSTRUCT *pPdStruct)
{
    QPointer<XADFArchive> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(
        &m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext ||
        !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return ARCHIVERECORD();
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nImageSize) ||
        (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);
    if (member.sPath.isEmpty() || (member.nHeaderBlock < ADF_BOOT_BLOCKS) ||
        (member.nHeaderBlock >= pContext->nBlockCount) || (member.nSize < 0) ||
        (member.nStoredSize < 0)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = static_cast<qint64>(member.nHeaderBlock) *
        ADF_BLOCK_SIZE;
    result.nStreamSize = member.nStoredSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sPath);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.nStoredSize);
    QString sMethod = pContext->bOFS ? QStringLiteral("AmigaDOS OFS")
                                     : QStringLiteral("AmigaDOS FFS");
    if (pContext->bDirCache) {
        sMethod += QStringLiteral(" dircache");
    } else if (pContext->bInternational) {
        sMethod += QStringLiteral(" international");
    }
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, sMethod);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsDirectory);
    if (member.mtDateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, member.mtDateTime);
        result.mapProperties.insert(FPART_PROP_MTIME, member.mtDateTime);
    }
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex))
        return ARCHIVERECORD();
    return result;
}

bool XADFArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                PDSTRUCT *pPdStruct)
{
    QPointer<XADFArchive> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !guardedThis || !guardedOutput || !guardedSource ||
        !isUnpackOutputSupported(guardedOutput.data()) ||
        devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nImageSize) ||
        (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    const MEMBER member = pContext->listMembers.at(pState->nCurrentIndex);

    if (member.bIsDirectory) {
        QBuffer emptyStage;
        if (!emptyStage.open(QIODevice::ReadWrite) || !guardedThis ||
            !guardedOutput || !isUnpackSourceCurrent(pState, pPdStruct)) {
            return false;
        }
        return publishUnpackOutput(&emptyStage, guardedOutput.data(), pState,
                                   pPdStruct) && guardedThis;
    }

    if ((member.nSize < 0) || (member.nStoredSize < 0) ||
        !isUnpackOutputSizeAllowed(pState->mapUnpackProperties, member.nSize)) {
        XBinary::setPdStructErrorString(
            pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }

    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                                member.sPath)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(member.nSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                XBinary::setPdStructErrorString(
                    pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            XBinary::OUTPUT_BUDGET::noteShadowRefusal(
                pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(member.nSize, pPdStruct));
    if (!pStage || !pStage->seek(0) || !guardedThis || !guardedOutput ||
        !guardedSource) {
        return false;
    }

    qint64 nWritten = 0;
    for (const DATA_BLOCK &dataBlock : member.listDataBlocks) {
        if (!guardedThis || !guardedOutput || !guardedSource ||
            (dataBlock.nBlock < ADF_BOOT_BLOCKS) ||
            (dataBlock.nBlock >= pContext->nBlockCount) ||
            (dataBlock.nPayloadOffset < 0) ||
            (dataBlock.nPayloadSize < 0) ||
            (dataBlock.nPayloadOffset > ADF_BLOCK_SIZE) ||
            (dataBlock.nPayloadSize >
             (ADF_BLOCK_SIZE - dataBlock.nPayloadOffset)) ||
            (dataBlock.nPayloadSize > (member.nSize - nWritten)) ||
            !XBinary::isPdStructNotCanceled(pPdStruct)) {
            return false;
        }

        const QByteArray baBlock = read_array_process(
            static_cast<qint64>(dataBlock.nBlock) * ADF_BLOCK_SIZE,
            ADF_BLOCK_SIZE, pPdStruct);
        if (!guardedThis || !guardedOutput || !guardedSource ||
            (baBlock.size() != ADF_BLOCK_SIZE) ||
            !isUnpackSourceCurrent(pState, pPdStruct)) {
            return false;
        }

        qint64 nBlockWritten = 0;
        while (nBlockWritten < dataBlock.nPayloadSize) {
            if (!guardedThis || !guardedOutput || !guardedSource ||
                !XBinary::isPdStructNotCanceled(pPdStruct)) {
                return false;
            }
            const qint64 nWrite = pStage->write(
                baBlock.constData() + dataBlock.nPayloadOffset + nBlockWritten,
                dataBlock.nPayloadSize - nBlockWritten);
            if ((nWrite <= 0) ||
                (nWrite > (dataBlock.nPayloadSize - nBlockWritten))) {
                return false;
            }
            nBlockWritten += nWrite;
        }
        nWritten += dataBlock.nPayloadSize;
    }

    if ((nWritten != member.nSize) || (pStage->size() != member.nSize) ||
        !pStage->seek(0) || !guardedThis || !guardedOutput ||
        !guardedSource || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput.data(),
                                             pState, pPdStruct);
    if (bResult && guardedThis) pState->nCurrentOffset = member.nSize;
    return bResult && guardedThis;
}

bool XADFArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XADFArchive> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    const CONTEXT *pContext = static_cast<const CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nTotalSize != pContext->nImageSize) ||
        (pState->nNumberOfRecords != pContext->listMembers.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = static_cast<qint64>(
            pContext->listMembers.at(pState->nCurrentIndex).nHeaderBlock) *
            ADF_BLOCK_SIZE;
        return true;
    }
    pState->nCurrentOffset = pContext->nImageSize;
    return false;
}

bool XADFArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XADFArchive::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_UNCOMPRESSEDSIZE,
            FPART_PROP_COMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER,
            FPART_PROP_DATETIME, FPART_PROP_MTIME};
}
