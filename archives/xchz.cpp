/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xchz.h"

#include <QDateTime>
#include <QtEndian>

#include <new>

namespace {
// "SChF", "SChD" and "SChd" read as little-endian dwords.  The three tags
// differ only in the last byte, so the four-byte form is the only correct
// comparison.
const quint32 CHZ_TAG_FILE = 0x46684353;
const quint32 CHZ_TAG_DIRECTORY_ENTER = 0x44684353;
const quint32 CHZ_TAG_DIRECTORY_LEAVE = 0x64684353;

const qint64 CHZ_FILE_HEADER_SIZE = 0x18;
const qint64 CHZ_DIRECTORY_HEADER_SIZE = 10;
const qint64 CHZ_RECORDSIZE_OFFSET = 0x04;
const qint64 CHZ_UNCOMPRESSEDSIZE_OFFSET = 0x08;
const qint64 CHZ_DOSTIME_OFFSET = 0x10;
const qint64 CHZ_DOSDATE_OFFSET = 0x12;
const qint64 CHZ_METHOD_OFFSET = 0x14;
const qint64 CHZ_NAMELENGTH_OFFSET = 0x16;
const qint64 CHZ_DIRECTORY_GUARD_OFFSET = 8;
const qint64 CHZ_DIRECTORY_NAMELENGTH_OFFSET = 9;

const quint8 CHZ_METHOD_STORE = 0;
const quint8 CHZ_METHOD_CHARC = 1;

// The authoring tool is a 1990 DOS archiver working on floppies; these
// ceilings only stop a corrupt chain from asking for an unbounded read or an
// unbounded decode, they are not format limits.
const qint64 CHZ_MAX_MEMBERS = 65536;
const qint64 CHZ_MAX_MEMBER_SIZE = Q_INT64_C(0x20000000);
const qint32 CHZ_MAX_DIRECTORY_DEPTH = 64;
// The two observed ChSFX stubs are 1816 and 1850 bytes and neither contains a
// stray tag, but a scan that cannot survive a few misses would be brittle for
// no gain.
const qint32 CHZ_MAX_CANDIDATES = 256;
// Enough to cover both stubs plus room for a longer one; only used for the
// version string.
const qint64 CHZ_VERSION_SCAN_SIZE = 8192;

bool chzRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= nTotalSize - nOffset);
}
}  // namespace

XCHZ::XCHZ(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCHZ::~XCHZ()
{
}

// A ChArc name is a DOS 8.3 base name, and a directory record carries one path
// component.  Control bytes cannot occur in either, so rejecting them is what
// keeps the chain scan from accepting an accidental tag; the high half is left
// alone because the authoring tool is Russian and cp866 names are legitimate.
bool XCHZ::isPlainName(const QByteArray &baRaw)
{
    if (baRaw.isEmpty()) return false;

    for (qint32 i = 0; i < baRaw.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRaw.at(i));
        if ((nCharacter < 0x20) || (nCharacter == 0x7f)) return false;
    }

    return true;
}

// Names in the reference set are bare 8.3 ASCII and come back byte for byte.
// A separator is normalized to '/', and anything the host filesystem cannot
// represent becomes %XX, which is reversible and - unlike folding to '_' -
// cannot collapse two distinct members onto one output file.
QString XCHZ::sanitizeName(const QByteArray &baRaw)
{
    QString sResult;

    for (qint32 i = 0; i < baRaw.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRaw.at(i));
        if ((nCharacter == '\\') || (nCharacter == '/')) {
            sResult.append(QLatin1Char('/'));
            continue;
        }
        const bool bSafe = (nCharacter >= 0x20) && (nCharacter < 0x7f) && (nCharacter != '%') && (nCharacter != ':') && (nCharacter != '*') &&
                           (nCharacter != '?') && (nCharacter != '"') && (nCharacter != '<') && (nCharacter != '>') && (nCharacter != '|');
        if (bSafe) {
            sResult.append(QChar(static_cast<ushort>(nCharacter)));
        } else {
            QString sHex = QString::number(nCharacter, 16).toUpper();
            while (sHex.size() < 2) sHex.prepend(QLatin1Char('0'));
            sResult.append(QLatin1Char('%'));
            sResult.append(sHex);
        }
    }

    return sResult;
}

QString XCHZ::methodToString(quint8 nMethod)
{
    if (nMethod == CHZ_METHOD_STORE) return QStringLiteral("Store");
    if (nMethod == CHZ_METHOD_CHARC) return QStringLiteral("ChArc");

    return QStringLiteral("Unknown (%1)").arg(nMethod);
}

// Method 0 is only a stored member when the payload is exactly as long as the
// declared output; that is the reference implementation's own test, and a
// record that fails it is corrupt rather than empty.  Every other value but 1
// is left unclaimed on purpose.
XBinary::HANDLE_METHOD XCHZ::memberHandleMethod(const MEMBER &member)
{
    if (member.nMethod == CHZ_METHOD_STORE) {
        if (member.nPackedSize == member.nUncompressedSize) return HANDLE_METHOD_STORE;
        return HANDLE_METHOD_UNKNOWN;
    }
    if (member.nMethod == CHZ_METHOD_CHARC) {
        if ((member.nPackedSize == 0) && (member.nUncompressedSize == 0)) return HANDLE_METHOD_STORE;
        return HANDLE_METHOD_CHARC;
    }

    return HANDLE_METHOD_UNKNOWN;
}

void XCHZ::fillRecordProperties(const MEMBER &member, QMap<FPART_PROP, QVariant> *pMapProperties)
{
    if (!pMapProperties) return;

    pMapProperties->insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    pMapProperties->insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    pMapProperties->insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    pMapProperties->insert(FPART_PROP_HANDLEMETHOD, memberHandleMethod(member));
    pMapProperties->insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    pMapProperties->insert(FPART_PROP_ISFOLDER, false);

    if (member.nDosDate != 0) {
        const QDateTime dtModified = dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
        if (dtModified.isValid()) {
            pMapProperties->insert(FPART_PROP_DATETIME, dtModified);
            pMapProperties->insert(FPART_PROP_MTIME, dtModified);
        }
    }
}

// The chain has no count and no terminator: landing on the last byte of the
// file IS the terminator.  The one exception is a split archive, where the
// last record of the volume declares a payload the volume does not hold; that
// record is dropped and *pbTruncated says so, but at least one complete record
// must precede it or the candidate is refused outright.
bool XCHZ::walkChain(qint64 nStart, qint64 nInputSize, QList<MEMBER> *pListMembers, bool *pbTruncated, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pListMembers || !pbTruncated) return false;
    pListMembers->clear();
    *pbTruncated = false;
    if ((nStart < 0) || (nStart >= nInputSize)) return false;

    qint64 nOffset = nStart;
    QStringList listDirectories;

    while (nOffset < nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (pListMembers->size() >= CHZ_MAX_MEMBERS) return false;
        if (!chzRangeWithin(nInputSize, nOffset, 4)) return false;

        const QByteArray baTag = read_array_process(nOffset, 4, pPdStruct);
        if ((baTag.size() != 4)) return false;
        const quint32 nTag = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTag.constData()));

        if (nTag == CHZ_TAG_FILE) {
            if (!chzRangeWithin(nInputSize, nOffset, CHZ_FILE_HEADER_SIZE)) return false;

            const QByteArray baHeader = read_array_process(nOffset, CHZ_FILE_HEADER_SIZE, pPdStruct);
            if ((baHeader.size() != CHZ_FILE_HEADER_SIZE)) return false;
            const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

            const qint64 nRecordSize = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + CHZ_RECORDSIZE_OFFSET));
            const qint64 nUncompressedSize = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + CHZ_UNCOMPRESSEDSIZE_OFFSET));
            const qint64 nNameLength = static_cast<qint64>(qFromLittleEndian<quint16>(pHeader + CHZ_NAMELENGTH_OFFSET));

            // The reference implementation's own three structural rules.
            if (nNameLength == 0) return false;
            if (nRecordSize < (CHZ_FILE_HEADER_SIZE + nNameLength)) return false;
            if (nUncompressedSize < 0) return false;
            if (nUncompressedSize > CHZ_MAX_MEMBER_SIZE) return false;
            if (!chzRangeWithin(nInputSize, nOffset + CHZ_FILE_HEADER_SIZE, nNameLength)) return false;

            const QByteArray baName = read_array_process(nOffset + CHZ_FILE_HEADER_SIZE, nNameLength, pPdStruct);
            if ((baName.size() != nNameLength)) return false;
            if (!isPlainName(baName)) return false;

            const qint64 nPackedSize = nRecordSize - CHZ_FILE_HEADER_SIZE - nNameLength;
            if (nPackedSize > CHZ_MAX_MEMBER_SIZE) return false;

            if ((nOffset + nRecordSize) > nInputSize) {
                // A volume boundary: the header is whole but its payload is in
                // the next volume, so the record is not in this file.
                if (pListMembers->isEmpty()) return false;
                *pbTruncated = true;
                break;
            }

            MEMBER member = {};
            member.nHeaderOffset = nOffset;
            member.nDataOffset = nOffset + CHZ_FILE_HEADER_SIZE + nNameLength;
            member.nPackedSize = nPackedSize;
            member.nUncompressedSize = nUncompressedSize;
            member.nDosTime = qFromLittleEndian<quint16>(pHeader + CHZ_DOSTIME_OFFSET);
            member.nDosDate = qFromLittleEndian<quint16>(pHeader + CHZ_DOSDATE_OFFSET);
            member.nMethod = pHeader[CHZ_METHOD_OFFSET];
            member.sFileName = sanitizeName(baName);
            if (member.sFileName.isEmpty()) return false;
            if (!listDirectories.isEmpty()) {
                member.sFileName = listDirectories.join(QLatin1Char('/')) + QLatin1Char('/') + member.sFileName;
            }
            // A member with bytes to produce and no bytes to produce them from
            // is a corrupt record, not an empty file.
            if ((member.nPackedSize == 0) && (member.nUncompressedSize != 0)) return false;

            pListMembers->append(member);
            nOffset += nRecordSize;
        } else if (nTag == CHZ_TAG_DIRECTORY_ENTER) {
            if (!chzRangeWithin(nInputSize, nOffset, CHZ_DIRECTORY_HEADER_SIZE)) return false;

            const QByteArray baHeader = read_array_process(nOffset, CHZ_DIRECTORY_HEADER_SIZE, pPdStruct);
            if ((baHeader.size() != CHZ_DIRECTORY_HEADER_SIZE)) return false;
            const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

            if (pHeader[CHZ_DIRECTORY_GUARD_OFFSET] != 0) return false;
            const qint64 nNameLength = static_cast<qint64>(pHeader[CHZ_DIRECTORY_NAMELENGTH_OFFSET]);
            if (nNameLength == 0) return false;
            if (!chzRangeWithin(nInputSize, nOffset + CHZ_DIRECTORY_HEADER_SIZE, nNameLength)) return false;

            const QByteArray baName = read_array_process(nOffset + CHZ_DIRECTORY_HEADER_SIZE, nNameLength, pPdStruct);
            if ((baName.size() != nNameLength)) return false;
            if (!isPlainName(baName)) return false;
            if (listDirectories.size() >= CHZ_MAX_DIRECTORY_DEPTH) return false;

            const QString sDirectory = sanitizeName(baName);
            if (sDirectory.isEmpty()) return false;
            listDirectories.append(sDirectory);
            nOffset += CHZ_DIRECTORY_HEADER_SIZE + nNameLength;
        } else if (nTag == CHZ_TAG_DIRECTORY_LEAVE) {
            if (listDirectories.isEmpty()) return false;
            listDirectories.removeLast();
            nOffset += 4;
        } else {
            return false;
        }
    }

    if (!*pbTruncated && (nOffset != nInputSize)) {
        pListMembers->clear();
        return false;
    }

    if (pListMembers->isEmpty()) return false;

    return true;
}

// The ChSFX stub is a 16-bit DOS image, so a PE / NE / LE / LX carrier is not
// one no matter what its overlay holds.  Refusing them also keeps the scan
// below off the large executables.
bool XCHZ::isDosCarrier(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < 2) return false;

    const QByteArray baMagic = read_array_process(0, 2, pPdStruct);
    if ((baMagic.size() != 2)) return false;
    const bool bMZ = (static_cast<quint8>(baMagic.at(0)) == 'M') && (static_cast<quint8>(baMagic.at(1)) == 'Z');
    const bool bZM = (static_cast<quint8>(baMagic.at(0)) == 'Z') && (static_cast<quint8>(baMagic.at(1)) == 'M');
    if (!bMZ && !bZM) return false;

    if (nInputSize < 0x40) return true;

    const QByteArray baHeader = read_array_process(0, 0x40, pPdStruct);
    if ((baHeader.size() != 0x40)) return false;
    const uchar *pHeader = reinterpret_cast<const uchar *>(baHeader.constData());

    const quint16 nRelocationOffset = qFromLittleEndian<quint16>(pHeader + 0x18);
    if (nRelocationOffset < 0x40) return true;

    const qint64 nNewHeader = static_cast<qint64>(qFromLittleEndian<qint32>(pHeader + 0x3c));
    if ((nNewHeader <= 0) || (nNewHeader > (nInputSize - 2))) return true;

    const QByteArray baNew = read_array_process(nNewHeader, 2, pPdStruct);
    if ((baNew.size() != 2)) return false;
    if (baNew == QByteArray("PE", 2)) return false;
    if (baNew == QByteArray("NE", 2)) return false;
    if (baNew == QByteArray("LE", 2)) return false;
    if (baNew == QByteArray("LX", 2)) return false;

    return true;
}

bool XCHZ::locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pnContainerOffset || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < (CHZ_FILE_HEADER_SIZE + 1)) return false;

    QList<MEMBER> listMembers;
    bool bTruncated = false;

    // The bare .chz container starts at offset 0 and costs one chain walk.
    if (walkChain(0, nInputSize, &listMembers, &bTruncated, pPdStruct)) {
        *pnContainerOffset = 0;
        return true;
    }
    if (!guardedSource) return false;

    if (!isDosCarrier(pPdStruct)) return false;

    char szTag[3] = {};
    szTag[0] = 'S';
    szTag[1] = 'C';
    szTag[2] = 'h';

    qint64 nSearchOffset = 1;
    for (qint32 i = 0; i < CHZ_MAX_CANDIDATES; i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nSearchOffset >= nInputSize) break;

        const qint64 nCandidate = find_array(nSearchOffset, nInputSize - nSearchOffset, szTag, 3, pPdStruct);
        if (!guardedSource) return false;
        if (nCandidate <= 0) break;

        if (walkChain(nCandidate, nInputSize, &listMembers, &bTruncated, pPdStruct)) {
            *pnContainerOffset = nCandidate;
            return true;
        }
        if (!guardedSource) return false;

        nSearchOffset = nCandidate + 1;
    }

    return false;
}

bool XCHZ::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();

    if (!locateContainer(&context.nContainerOffset, pPdStruct)) return false;
    if (!walkChain(context.nContainerOffset, context.nInputSize, &context.listMembers, &context.bTruncated, pPdStruct)) {
        return false;
    }

    // A complete chain closes on the last byte of the file, so the container
    // plus its carrier is the whole file; a split volume ends at the last
    // record this file actually holds.
    if (context.bTruncated && !context.listMembers.isEmpty()) {
        const MEMBER &memberLast = context.listMembers.last();
        context.nArchiveSize = memberLast.nDataOffset + memberLast.nPackedSize;
    } else {
        context.nArchiveSize = context.nInputSize;
    }

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XCHZ::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    qint64 nContainerOffset = 0;
    const bool bResult = locateContainer(&nContainerOffset, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XCHZ::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCHZ archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XCHZ::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XCHZ(pDevice);
}

QList<QString> XCHZ::getSearchSignatures()
{
    return {QStringLiteral("'SChF'"), QStringLiteral("'SChD'")};
}

XBinary::FT XCHZ::getFileType()
{
    return FT_CHZ;
}

XBinary::MODE XCHZ::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XCHZ::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCHZ::getArch()
{
    return QString();
}

// The bare container keeps the authoring tool's own extension; the
// self-extractor is an executable and has to say so.
QString XCHZ::getFileFormatExt()
{
    QIODevice *guardedSource = getDevice();
    if (guardedSource && !guardedSource->isSequential() && (guardedSource->size() >= 4)) {
        const QByteArray baTag = read_array_process(0, 4, nullptr);
        if (baTag.size() == 4) {
            const quint32 nTag = qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(baTag.constData()));
            if ((nTag == CHZ_TAG_FILE) || (nTag == CHZ_TAG_DIRECTORY_ENTER) || (nTag == CHZ_TAG_DIRECTORY_LEAVE)) {
                return QStringLiteral("chz");
            }
        }
    }

    return QStringLiteral("exe");
}

QString XCHZ::getFileFormatExtsString()
{
    return QStringLiteral("ChArc archive (*.chz *.exe)");
}

QString XCHZ::getMIMEString()
{
    return QStringLiteral("application/x-charc");
}

// The container itself carries no version field.  The self-extracting stub
// signs itself, so report that and nothing when there is no stub.
QString XCHZ::getVersion()
{
    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return QString();

    const qint64 nInputSize = guardedSource->size();
    qint64 nScanSize = nInputSize;
    if (nScanSize > CHZ_VERSION_SCAN_SIZE) nScanSize = CHZ_VERSION_SCAN_SIZE;
    if (nScanSize <= 0) return QString();

    const QByteArray baHead = read_array_process(0, nScanSize, nullptr);
    if (!guardedSource) return QString();

    const qint32 nFound = baHead.indexOf(QByteArray("ChSFX (small) v", 15));
    if (nFound < 0) return QString();

    QString sResult;
    for (qint32 i = nFound + 15; i < baHead.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baHead.at(i));
        if ((nCharacter < '0') || ((nCharacter > '9') && (nCharacter != '.'))) break;
        sResult.append(QChar(static_cast<ushort>(nCharacter)));
        if (sResult.size() >= 8) break;
    }

    return sResult;
}

qint64 XCHZ::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XCHZ::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XCHZ::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }

    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XCHZ::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XCHZ::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && (context.nContainerOffset > 0) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nContainerOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nPackedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            fillRecordProperties(member, &part.mapProperties);
            listResult.append(part);
        }
    }

    // A split volume ends on a record whose payload is in the next file; the
    // bytes after the last complete record are not part of this container.
    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QList<XBinary::FPART_PROP> XCHZ::getAvailableFPARTProperties()
{
    return {FPART_PROP_ORIGINALNAME, FPART_PROP_COMPRESSEDSIZE, FPART_PROP_UNCOMPRESSEDSIZE, FPART_PROP_HANDLEMETHOD,
            FPART_PROP_REPORTEDMETHOD, FPART_PROP_ISFOLDER, FPART_PROP_DATETIME, FPART_PROP_MTIME};
}

QMap<XBinary::UNPACK_PROP, QVariant> XCHZ::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCHZ::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    if (pContext->bTruncated) {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("ChArc archive (split volume; the last record continues in the next volume)"));
    } else {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("ChArc archive"));
    }
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XCHZ::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nPackedSize;
    fillRecordProperties(member, &result.mapProperties);

    return result;
}

bool XCHZ::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }

    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XCHZ::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}
