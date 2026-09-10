/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xaodos.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 AODOS_BLOCK_SIZE = 512;
// Blocks 0..19 are the system area: boot block, directory table and the
// byte-per-block allocation map.  User data always begins after it, which is
// what bounds the directory walk.
const qint64 AODOS_SYSTEM_AREA_SIZE = 20 * AODOS_BLOCK_SIZE;
const qint64 AODOS_DIRECTORY_OFFSET = 0x140;  // octal 0500
const qint64 AODOS_ENTRY_SIZE = 24;
const qint64 AODOS_ENTRY_NAME_OFFSET = 2;
const qint64 AODOS_ENTRY_NAME_SIZE = 14;
// (20 * 512 - 0x140) / 24 -- the directory cannot outgrow the system area.
const qint32 AODOS_MAX_ENTRIES = 413;
const qint64 AODOS_MIN_SIZE = AODOS_SYSTEM_AREA_SIZE + AODOS_BLOCK_SIZE;
// Generous ceiling: the largest BK volume in circulation is the 800 KiB
// floppy, hard-disk partitions of a few MiB exist.  It only exists to keep a
// huge unrelated file from being walked.
const qint64 AODOS_MAX_SIZE = 0x1000000;
const quint16 AODOS_ENTRY_DELETED = 0xffffU;
const quint8 AODOS_FILLER_BYTE = 0xf6U;
const qint32 AODOS_MAX_TREE_DEPTH = 16;
const qint32 AODOS_MAX_NAME_COLLISIONS = 1024;

// The BK boot prologue: PDP-11 words 000240 (NOP) and 000426 (BR .+56).
const quint8 AODOS_MAGIC[4] = {0xa0U, 0x00U, 0x16U, 0x01U};

// KOI8-R high half.  MKDOS/ANDOS/AO-DOS names are KOI8, and there is no
// first-party KOI8 helper in the tree; a 128-entry table is smaller and more
// predictable than pulling in a QTextCodec dependency.
const ushort AODOS_KOI8R_HIGH[128] = {
    0x2500, 0x2502, 0x250c, 0x2510, 0x2514, 0x2518, 0x251c, 0x2524,
    0x252c, 0x2534, 0x253c, 0x2580, 0x2584, 0x2588, 0x258c, 0x2590,
    0x2591, 0x2592, 0x2593, 0x2320, 0x25a0, 0x2219, 0x221a, 0x2248,
    0x2264, 0x2265, 0x00a0, 0x2321, 0x00b0, 0x00b2, 0x00b7, 0x00f7,
    0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556,
    0x2557, 0x2558, 0x2559, 0x255a, 0x255b, 0x255c, 0x255d, 0x255e,
    0x255f, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565,
    0x2566, 0x2567, 0x2568, 0x2569, 0x256a, 0x256b, 0x256c, 0x00a9,
    0x044e, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e,
    0x043f, 0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
    0x044c, 0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a,
    0x042e, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
    0x0425, 0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e,
    0x041f, 0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
    0x042c, 0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a};

bool aodosIsAllFiller(const QByteArray &baValue)
{
    if (baValue.isEmpty()) return false;
    for (char cValue : baValue) {
        if (static_cast<quint8>(cValue) != AODOS_FILLER_BYTE) return false;
    }
    return true;
}

QString aodosAppendDuplicateSuffix(const QString &sLeaf, qint32 nSuffix)
{
    const QString sSuffix = QStringLiteral("~%1").arg(nSuffix);
    const qint32 nDotPosition = sLeaf.lastIndexOf(QLatin1Char('.'));
    if ((nDotPosition > 0) && ((sLeaf.length() - nDotPosition) <= 32)) {
        return sLeaf.left(nDotPosition) + sSuffix + sLeaf.mid(nDotPosition);
    }
    return sLeaf + sSuffix;
}
}  // namespace

XAODOS::XAODOS(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAODOS::~XAODOS()
{
}

QString XAODOS::decodeKOI8Name(const QByteArray &baName)
{
    QByteArray baTrimmed = baName;
    while (!baTrimmed.isEmpty() &&
           (static_cast<quint8>(baTrimmed.at(baTrimmed.size() - 1)) == 0x20U)) {
        baTrimmed.chop(1);
    }

    QString sResult;
    sResult.reserve(baTrimmed.size());
    for (char cValue : baTrimmed) {
        const quint8 nValue = static_cast<quint8>(cValue);
        // AO-DOS names legally contain '/' and '\' (the sample set has
        // "ANIDOC/MVI").  They must be neutralized here, before the leaf is
        // joined into a path, or the member silently gains a directory level.
        if ((nValue == static_cast<quint8>('/')) ||
            (nValue == static_cast<quint8>('\\'))) {
            sResult.append(QLatin1Char('_'));
        } else if (nValue < 0x80U) {
            sResult.append(QLatin1Char(static_cast<char>(nValue)));
        } else {
            sResult.append(QChar(AODOS_KOI8R_HIGH[nValue - 0x80U]));
        }
    }

    // The extraction pipeline refuses any path fixFileName() would alter, so
    // the class must produce the fixed form itself ("PRN" -> "_PRN",
    // "MK->AO" -> "MK-_AO") rather than leaving the member unextractable.
    return XBinary::fixFileName(sResult);
}

QString XAODOS::readVersion(const QByteArray &baBootBlock)
{
    // The version lives in the boot code as an in-band ASCII string, at a
    // different offset per generation: "AODOSV2.10+" (v2.x) or "DOS V1.77/10"
    // (v1.x).  Search rather than hard-code an offset.
    struct TAG {
        const char *pPrefix;
        qint32 nPrefixSize;
    };
    const TAG tags[2] = {{"AODOSV", 6}, {"DOS V", 5}};

    for (qint32 i = 0; i < 2; ++i) {
        const qint32 nPosition = baBootBlock.indexOf(tags[i].pPrefix);
        if (nPosition < 0) continue;
        QString sVersion;
        for (qint32 j = nPosition + tags[i].nPrefixSize;
             j < baBootBlock.size(); ++j) {
            const char cValue = baBootBlock.at(j);
            // Stop on the first non-version character.  The v1.77 string is
            // not space-terminated: the entry-count word at 0x18 follows it
            // immediately, and its low byte can be a printable ASCII letter.
            const bool bVersionCharacter =
                ((cValue >= '0') && (cValue <= '9')) || (cValue == '.') ||
                (cValue == '/') || (cValue == '+') || (cValue == '-');
            if (!bVersionCharacter) break;
            sVersion.append(QLatin1Char(cValue));
            if (sVersion.size() >= 16) break;
        }
        if (!sVersion.isEmpty()) return sVersion;
    }
    return QString();
}

bool XAODOS::TREE::claimPath(const QString &sParent, const QString &sLeaf,
                             QString *pResult)
{
    if (!pResult || sLeaf.isEmpty()) return false;

    for (qint32 nSuffix = 1; nSuffix <= AODOS_MAX_NAME_COLLISIONS; ++nSuffix) {
        const QString sCandidateLeaf =
            (nSuffix == 1) ? sLeaf
                           : aodosAppendDuplicateSuffix(sLeaf, nSuffix);
        const QString sCandidate =
            sParent.isEmpty() ? sCandidateLeaf
                              : (sParent + QLatin1Char('/') + sCandidateLeaf);
        // sLeaf already went through fixFileName(); the guard only catches a
        // suffix that would reintroduce a hazard, so it can never spin.
        if ((XBinary::fixFileName(sCandidate) != sCandidate) ||
            stUsedPaths.contains(sCandidate.toCaseFolded())) {
            continue;
        }
        stUsedPaths.insert(sCandidate.toCaseFolded());
        *pResult = sCandidate;
        return true;
    }
    return false;
}

bool XAODOS::TREE::resolve(quint32 nIndex, QString *pPath, qint32 nDepth)
{
    if (!pPath) return false;
    if (nIndex == 0) {
        *pPath = QString();  // The root directory has no path prefix.
        return true;
    }
    // Nothing on disk forbids a parent cycle (dir 3 -> dir 5 -> dir 3), so the
    // hop cap is what keeps a corrupt image from recursing forever.
    if (nDepth > AODOS_MAX_TREE_DEPTH) return false;
    if (mapPath.contains(nIndex)) {
        *pPath = mapPath.value(nIndex);
        return true;
    }
    if (!mapLeaf.contains(nIndex)) return false;

    QString sParentPath;
    if (!resolve(mapParent.value(nIndex), &sParentPath, nDepth + 1)) {
        return false;
    }
    QString sPath;
    if (!claimPath(sParentPath, mapLeaf.value(nIndex), &sPath)) return false;
    mapPath.insert(nIndex, sPath);
    *pPath = sPath;
    return true;
}

bool XAODOS::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XAODOS> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    // A sector dump is by construction a whole number of 512-byte blocks; the
    // sector-multiple test is the cheapest way to drop the bulk of unrelated
    // files before the magic is even read.
    if ((context.nInputSize < AODOS_MIN_SIZE) ||
        (context.nInputSize > AODOS_MAX_SIZE) ||
        ((context.nInputSize % AODOS_BLOCK_SIZE) != 0)) {
        return false;
    }
    context.nTotalBlocks = context.nInputSize / AODOS_BLOCK_SIZE;
    context.nDirectoryOffset = AODOS_DIRECTORY_OFFSET;

    const QByteArray baBootBlock =
        read_array_process(0, AODOS_BLOCK_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baBootBlock.size() != AODOS_BLOCK_SIZE)) {
        return false;
    }
    if (std::memcmp(baBootBlock.constData(), AODOS_MAGIC,
                    sizeof(AODOS_MAGIC)) != 0) {
        return false;
    }
    // MKDOS, ANDOS and NORD-DOS boot blocks share the same PDP-11 prologue;
    // only the in-band ID string separates AO-DOS from them.  It sits at 0x05
    // in v2.x images and at 0x102 in v1.77 images, so search the whole block.
    if (baBootBlock.indexOf("AO-DOS") < 0) return false;

    const uchar *pBootBlock =
        reinterpret_cast<const uchar *>(baBootBlock.constData());
    context.nDeclaredEntryCount = qFromLittleEndian<quint16>(pBootBlock + 0x18);
    context.sVersion = readVersion(baBootBlock);

    const qint64 nDirectoryAreaSize =
        AODOS_SYSTEM_AREA_SIZE - AODOS_DIRECTORY_OFFSET;
    const QByteArray baDirectory = read_array_process(
        AODOS_DIRECTORY_OFFSET, nDirectoryAreaSize, pPdStruct);
    if (!guardedThis || !guardedSource ||
        (baDirectory.size() != nDirectoryAreaSize)) {
        return false;
    }

    QList<ENTRY> listEntries;
    qint64 nChainEnd = -1;  // Running end of the contiguous extent chain.
    qint64 nFirstExtentOffset = -1;
    bool bTerminatorFound = false;
    qint64 nDirectoryEnd = AODOS_DIRECTORY_OFFSET;

    for (qint32 nIndex = 0; nIndex < AODOS_MAX_ENTRIES; ++nIndex) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const qint64 nEntryOffset = nIndex * AODOS_ENTRY_SIZE;
        if ((nEntryOffset + AODOS_ENTRY_SIZE) > baDirectory.size()) return false;

        const uchar *pEntry =
            reinterpret_cast<const uchar *>(baDirectory.constData()) +
            nEntryOffset;
        ENTRY entry = {};
        entry.nEntryOffset = AODOS_DIRECTORY_OFFSET + nEntryOffset;
        entry.nEntryWord = qFromLittleEndian<quint16>(pEntry);
        entry.baName = baDirectory.mid(
            static_cast<qint32>(nEntryOffset + AODOS_ENTRY_NAME_OFFSET),
            static_cast<qint32>(AODOS_ENTRY_NAME_SIZE));
        entry.nStartBlock = qFromLittleEndian<quint16>(pEntry + 16);
        entry.nBlockCount = qFromLittleEndian<quint16>(pEntry + 18);
        entry.nLoadAddress = qFromLittleEndian<quint16>(pEntry + 20);
        entry.nSizeBytes = qFromLittleEndian<quint16>(pEntry + 22);
        if (entry.baName.size() != AODOS_ENTRY_NAME_SIZE) return false;

        const qint64 nExtentEnd = static_cast<qint64>(entry.nStartBlock) +
                                  static_cast<qint64>(entry.nBlockCount);

        // The terminator is a real free-space record, not filler.  It must be
        // recognized by the FULL 14-byte name being 0xF6: 0xF6 is KOI8-R 'Zh'
        // and is legal as the first byte of a genuine name.
        if (aodosIsAllFiller(entry.baName)) {
            if ((entry.nEntryWord != AODOS_ENTRY_DELETED) ||
                (nExtentEnd != context.nTotalBlocks) ||
                ((nChainEnd >= 0) &&
                 (static_cast<qint64>(entry.nStartBlock) != nChainEnd))) {
                return false;
            }
            context.nFirstFreeBlock = entry.nStartBlock;
            context.nFreeBlockCount = entry.nBlockCount;
            nDirectoryEnd = entry.nEntryOffset + AODOS_ENTRY_SIZE;
            bTerminatorFound = true;
            break;
        }

        for (char cValue : entry.baName) {
            if (static_cast<quint8>(cValue) < 0x20U) return false;
        }
        if (static_cast<quint8>(entry.baName.at(0)) == 0x20U) return false;
        if (nExtentEnd > context.nTotalBlocks) return false;
        // A deleted entry keeps a stale byte size that can exceed its own
        // allocation (the discarded MAKEBOOT record in the v1.77 sample claims
        // 10652 bytes over 17 blocks), so the size test applies to live
        // entries only.  Its extent still takes part in the chain below.
        if ((entry.nEntryWord != AODOS_ENTRY_DELETED) &&
            (static_cast<qint64>(entry.nSizeBytes) >
             (static_cast<qint64>(entry.nBlockCount) * AODOS_BLOCK_SIZE))) {
            return false;
        }
        if (entry.nBlockCount > 0) {
            if (nChainEnd < 0) {
                if (entry.nStartBlock < 1) return false;
                nFirstExtentOffset =
                    static_cast<qint64>(entry.nStartBlock) * AODOS_BLOCK_SIZE;
            } else if (static_cast<qint64>(entry.nStartBlock) != nChainEnd) {
                return false;
            }
            nChainEnd = nExtentEnd;
        }

        listEntries.append(entry);
    }

    // Running to the 413-entry cap means the table never terminated, which no
    // real volume does; and a volume with no allocated extent at all carries
    // no evidence that this is an AO-DOS image rather than a coincidence.
    if (!bTerminatorFound || (nChainEnd < 0)) return false;
    // The directory shares the system area with the allocation map, so it can
    // never reach into the first member's data.
    if ((nFirstExtentOffset >= 0) && (nDirectoryEnd > nFirstExtentOffset)) {
        return false;
    }
    context.nRawEntryCount = listEntries.size();
    context.nDirectorySize = nDirectoryEnd - AODOS_DIRECTORY_OFFSET;

    // Pass 1: register the directory nodes so file entries can be attached to
    // them regardless of the order the two appear in the table.
    TREE tree;
    for (const ENTRY &entry : listEntries) {
        if (entry.nEntryWord == AODOS_ENTRY_DELETED) continue;
        const quint32 nOwnIndex = entry.nEntryWord & 0xffU;
        if (nOwnIndex == 0) continue;  // A file, handled in pass 2.
        if (tree.mapLeaf.contains(nOwnIndex)) return false;  // Duplicate index.
        const QString sLeaf = decodeKOI8Name(entry.baName);
        if (sLeaf.isEmpty()) return false;
        tree.mapLeaf.insert(nOwnIndex, sLeaf);
        tree.mapParent.insert(nOwnIndex, entry.nEntryWord >> 8);
    }

    // Pass 2: emit members in on-disk order, so nCurrentOffset advances
    // monotonically through the directory table during streaming.
    for (const ENTRY &entry : listEntries) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        // 0xFFFF has a non-zero low byte, so it MUST be tested before the
        // "low byte non-zero means directory" rule or every deleted record is
        // listed as a folder.
        if (entry.nEntryWord == AODOS_ENTRY_DELETED) continue;

        MEMBER member = {};
        member.nEntryOffset = entry.nEntryOffset;
        member.nEntryWord = entry.nEntryWord;
        member.nStartBlock = entry.nStartBlock;
        member.nBlockCount = entry.nBlockCount;
        member.nLoadAddress = entry.nLoadAddress;

        const quint32 nOwnIndex = entry.nEntryWord & 0xffU;
        if (nOwnIndex != 0) {
            QString sPath;
            if (!tree.resolve(nOwnIndex, &sPath, 0) || sPath.isEmpty()) {
                return false;
            }
            member.bIsDirectory = true;
            // Folder records carry a junk start block (20 in the v2.10
            // sample); the entry itself is the only in-range anchor.
            member.nDataOffset = entry.nEntryOffset;
            member.nDataSize = 0;
            member.sPath = sPath;
        } else {
            QString sParentPath;
            // An unresolvable parent means the entry-word interpretation does
            // not hold for this image.  Failing closed is better than filing
            // members under a fabricated tree.
            if (!tree.resolve(entry.nEntryWord >> 8, &sParentPath, 0)) {
                return false;
            }
            const QString sLeaf = decodeKOI8Name(entry.baName);
            QString sPath;
            if (sLeaf.isEmpty() || !tree.claimPath(sParentPath, sLeaf, &sPath)) {
                return false;
            }
            member.bIsDirectory = false;
            member.nDataOffset =
                static_cast<qint64>(entry.nStartBlock) * AODOS_BLOCK_SIZE;
            member.nDataSize = entry.nSizeBytes;
            member.sPath = sPath;
            if ((member.nDataOffset < 0) ||
                (member.nDataSize > (context.nInputSize - member.nDataOffset))) {
                return false;
            }
        }
        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XAODOS::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XAODOS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAODOS archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAODOS::createInstance(QIODevice *pDevice, bool bIsImage,
                                XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAODOS(pDevice);
}

QList<QString> XAODOS::getSearchSignatures()
{
    // 000240 / 000426 -- the BK boot prologue, shared with the other BK DOSes;
    // isValid() narrows it with the in-band "AO-DOS" marker.
    return {QStringLiteral("A0001601")};
}

XBinary::FT XAODOS::getFileType()
{
    return FT_AODOS;
}

XBinary::MODE XAODOS::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAODOS::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XAODOS::getArch()
{
    return QStringLiteral("PDP11");
}

QString XAODOS::getFileFormatExt()
{
    return QStringLiteral("img");
}

QString XAODOS::getFileFormatExtsString()
{
    return QStringLiteral("AO-DOS disk image (*.img *.bkd *.dsk)");
}

QString XAODOS::getMIMEString()
{
    return QStringLiteral("application/x-aodos-disk-image");
}

QString XAODOS::getVersion()
{
    CONTEXT context = {};
    return parseContext(&context, nullptr) ? context.sVersion : QString();
}

qint64 XAODOS::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    // The volume describes itself down to the last block: the extent chain
    // ends at the free-space record, whose start plus length is exactly the
    // image size.  There is no trailing slack to trim.
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XAODOS::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAODOS::getMemoryMap(MAPMODE mapMode,
                                          PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_TABLE |
                                 FILEPART_STREAM,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XAODOS::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XAODOS::getFileParts(quint32 nFileParts, qint32 nLimit,
                                           PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = AODOS_DIRECTORY_OFFSET;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Boot block");
        result.append(part);
    }
    if ((nFileParts & FILEPART_TABLE) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_TABLE;
        part.nFileOffset = context.nDirectoryOffset;
        part.nFileSize = context.nDirectorySize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Directory");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (member.bIsDirectory) continue;  // A folder owns no extent.
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sPath;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored"));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize =
                static_cast<qint64>(member.nBlockCount) * AODOS_BLOCK_SIZE;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sPath;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QList<XBinary::FPART_PROP> XAODOS::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME,   FPART_PROP_UNCOMPRESSEDSIZE,
            FPART_PROP_COMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER,
            FPART_PROP_INFO};
}

QMap<XBinary::UNPACK_PROP, QVariant> XAODOS::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAODOS::initUnpack(UNPACK_STATE *pState,
                        const QMap<UNPACK_PROP, QVariant> &mapProperties,
                        PDSTRUCT *pPdStruct)
{
    QPointer<XAODOS> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        pContext->sVersion.isEmpty()
            ? tr("AO-DOS volume")
            : tr("AO-DOS volume %1").arg(pContext->sVersion));
    pState->nCurrentOffset = pContext->listMembers.first().nEntryOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
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

XBinary::ARCHIVERECORD XAODOS::infoCurrent(UNPACK_STATE *pState,
                                           PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext ||
        (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nEntryOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sPath);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsDirectory);
    if (!member.bIsDirectory) {
        // The PDP-11 load address is the only per-member metadata the format
        // carries; it is octal by convention on this machine.
        result.mapProperties.insert(
            FPART_PROP_INFO,
            QStringLiteral("Load address 0%1")
                .arg(static_cast<quint32>(member.nLoadAddress), 6, 8,
                     QLatin1Char('0')));
    }
    return result;
}

bool XAODOS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext ||
        (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nEntryOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;
    return false;
}

bool XAODOS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
