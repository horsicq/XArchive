/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xesparchive.h"

#include "Algos/xaindecoder.h"

#include <QtEndian>

#include <memory>
#include <new>

namespace {
const qint64 ESP_HEADER_SIZE = 10;
const qint32 ESP_RECORD_SIZE = 28;
const qint32 ESP_NAME_SIZE = 13;
const qint32 ESP_PATH_BUFFER_SIZE = 0x4000;
const qint32 ESP_MAX_MEMBERS = 65535;
const qint64 ESP_MAX_DIRECTORY = 4 * 1024 * 1024;
const qint64 ESP_MAX_CONTENT = 512 * 1024 * 1024;
const qint32 ESP_MAX_CANDIDATES = 64;

const quint8 ESP_METHOD_MASK = 0x07;
const quint8 ESP_FLAG_MULTIMEDIA = 0x08;
const quint8 ESP_FLAG_RESERVED = 0x10;
const quint8 ESP_FLAG_PASSWORD = 0x40;
const quint8 ESP_METHOD_STORED = 4;

const quint8 ESP_VERSION_SCRAMBLED = 0x15;  // strictly ABOVE this the streams are masked
const quint8 ESP_VERSION_CHANNELS = 0x19;   // from this up bit 3 means /MM2, the three-channel split

const quint8 ESP_ATTRIBUTE_DIRECTORY = 0x10;

const quint16 ESP_PARENT_ROOT = 0xffff;

// The keystream of the stream mask: 0x4B697947 rotated right by eight after
// every byte, which is the ASCII of "GyiK" over and over.
const quint32 ESP_SCRAMBLE_KEY = 0x4B697947;

const quint8 ESP_FILTER_NONE = 0;
const quint8 ESP_FILTER_DELTA = 1;
const quint8 ESP_FILTER_CHANNELS = 2;

bool espRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}
}  // namespace

XESPArchive::XESPArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XESPArchive::~XESPArchive()
{
}

void XESPArchive::descramble(QByteArray *pbaData)
{
    if (!pbaData) return;

    quint32 nKey = ESP_SCRAMBLE_KEY;
    quint8 *pData = (quint8 *)pbaData->data();
    const qint64 nSize = pbaData->size();
    for (qint64 i = 0; i < nSize; ++i) {
        pData[i] = (quint8)(pData[i] ^ (quint8)(nKey & 0xff));
        nKey = (nKey >> 8) | (nKey << 24);
    }
}

// One accumulator for the whole stream: every byte is the running sum of the
// bytes before it.  The reference implementation runs this over each freshly
// decoded chunk in turn without ever resetting, so running it once over the
// whole stream produces the same bytes.
void XESPArchive::applyDeltaFilter(QByteArray *pbaData)
{
    if (!pbaData) return;

    quint8 nAccumulator = 0;
    quint8 *pData = (quint8 *)pbaData->data();
    const qint64 nSize = pbaData->size();
    for (qint64 i = 0; i < nSize; ++i) {
        nAccumulator = (quint8)(nAccumulator + pData[i]);
        pData[i] = nAccumulator;
    }
}

// The container states the destination path and it is published, with the DOS
// separator normalized the way every other reader in this tree normalizes it.
// Nothing is stripped, so two members cannot be made to collide here.
QString XESPArchive::publishedName(const QByteArray &baRawPath)
{
    QString sResult = QString::fromLatin1(baRawPath);
    sResult.replace(QLatin1Char('\\'), QLatin1Char('/'));

    return sResult;
}

QString XESPArchive::methodToString(quint8 nMethod)
{
    if (nMethod == ESP_METHOD_STORED) return QStringLiteral("Stored");

    return QStringLiteral("ESP %1").arg(nMethod);
}

// The ESP2EXE stub is a 16-bit DOS image, so a PE / NE / LE / LX carrier is not
// one no matter what its overlay holds.  Refusing them also keeps the scan below
// off the large executables.
bool XESPArchive::isDosCarrier(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < 2) return false;

    const QByteArray baMagic = read_array_process(0, 2, pPdStruct);
    if ((baMagic.size() != 2)) return false;
    const bool bMZ = ((quint8)baMagic.at(0) == 'M') && ((quint8)baMagic.at(1) == 'Z');
    const bool bZM = ((quint8)baMagic.at(0) == 'Z') && ((quint8)baMagic.at(1) == 'M');
    if (!bMZ && !bZM) return false;

    if (nInputSize < 0x40) return true;

    const QByteArray baHeader = read_array_process(0, 0x40, pPdStruct);
    if ((baHeader.size() != 0x40)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    const quint16 nRelocationOffset = qFromLittleEndian<quint16>(pHeader + 0x18);
    if (nRelocationOffset < 0x40) return true;

    const qint64 nNewHeader = (qint64)qFromLittleEndian<qint32>(pHeader + 0x3c);
    if ((nNewHeader <= 0) || (nNewHeader > (nInputSize - 2))) return true;

    const QByteArray baNew = read_array_process(nNewHeader, 2, pPdStruct);
    if ((baNew.size() != 2)) return false;
    if (baNew == QByteArray("PE", 2)) return false;
    if (baNew == QByteArray("NE", 2)) return false;
    if (baNew == QByteArray("LE", 2)) return false;
    if (baNew == QByteArray("LX", 2)) return false;

    return true;
}

// Offset 0 first, which is the bare ".esp" case and costs one directory decode.
// Otherwise the carrier must be a plain 16-bit DOS image and a bounded scan
// takes the first "ESP>" whose directory actually decodes into records.
bool XESPArchive::locateContainer(qint64 *pnContainerOffset, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pnContainerOffset || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < (ESP_HEADER_SIZE + 1)) return false;

    CONTEXT context = {};
    if (readHeader(0, &context, pPdStruct) && readDirectory(&context, pPdStruct)) {
        *pnContainerOffset = 0;
        return true;
    }
    if (!guardedSource) return false;

    if (!isDosCarrier(pPdStruct)) return false;

    char szTag[4] = {};
    szTag[0] = 'E';
    szTag[1] = 'S';
    szTag[2] = 'P';
    szTag[3] = '>';

    qint64 nSearchOffset = 1;
    for (qint32 i = 0; i < ESP_MAX_CANDIDATES; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (nSearchOffset >= nInputSize) break;

        const qint64 nCandidate = find_array(nSearchOffset, nInputSize - nSearchOffset, szTag, 4, pPdStruct);
        if (!guardedSource) return false;
        if (nCandidate <= 0) break;

        CONTEXT candidate = {};
        if (readHeader(nCandidate, &candidate, pPdStruct) && readDirectory(&candidate, pPdStruct)) {
            *pnContainerOffset = nCandidate;
            return true;
        }
        if (!guardedSource) return false;

        nSearchOffset = nCandidate + 1;
    }

    return false;
}

bool XESPArchive::readHeader(qint64 nContainerOffset, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (!espRangeWithin(nInputSize, nContainerOffset, ESP_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array_process(nContainerOffset, ESP_HEADER_SIZE, pPdStruct);
    if ((baHeader.size() != ESP_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if ((pHeader[0] != 'E') || (pHeader[1] != 'S') || (pHeader[2] != 'P') || (pHeader[3] != '>')) return false;

    const quint8 nVersion = pHeader[4];
    const quint8 nFlags = pHeader[5];
    const qint64 nDirectoryOffset = (qint64)(qint32)qFromLittleEndian<quint32>(pHeader + 6);

    // The reference recogniser's own three tests: a directory behind the
    // header, a method it can drive, and the unused bit clear.
    if (nDirectoryOffset <= 9) return false;
    const quint8 nMethod = (quint8)(nFlags & ESP_METHOD_MASK);
    if (nMethod > ESP_METHOD_STORED) return false;
    if (nFlags & ESP_FLAG_RESERVED) return false;

    const qint64 nAbsoluteDirectory = nContainerOffset + nDirectoryOffset;
    if (!espRangeWithin(nInputSize, nAbsoluteDirectory, 1)) return false;

    pContext->nInputSize = nInputSize;
    pContext->nContainerOffset = nContainerOffset;
    pContext->nArchiveSize = nInputSize;
    pContext->nDirectoryOffset = nAbsoluteDirectory;
    pContext->nVersion = nVersion;
    pContext->nFlags = nFlags;
    pContext->nMethod = nMethod;
    pContext->bEncrypted = ((nFlags & ESP_FLAG_PASSWORD) != 0);
    pContext->bScrambled = (nVersion > ESP_VERSION_SCRAMBLED);
    pContext->bMaterialized = false;
    pContext->nFilter = ESP_FILTER_NONE;
    if (nFlags & ESP_FLAG_MULTIMEDIA) {
        pContext->nFilter = (nVersion < ESP_VERSION_CHANNELS) ? ESP_FILTER_DELTA : ESP_FILTER_CHANNELS;
    }

    return true;
}

bool XESPArchive::readDirectory(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    pContext->listMembers.clear();

    const qint64 nStreamSize = pContext->nInputSize - pContext->nDirectoryOffset;
    if (nStreamSize <= 0) return false;

    QByteArray baStream = read_array_process(pContext->nDirectoryOffset, nStreamSize, pPdStruct);
    if ((baStream.size() != nStreamSize)) return false;
    if (pContext->bScrambled) descramble(&baStream);

    // The directory ends on its own escape, so a budget is asked for and the
    // short read that comes back is what there is.
    const qint64 nBudget = qMin((qint64)ESP_MAX_MEMBERS * ESP_RECORD_SIZE, ESP_MAX_DIRECTORY);
    QByteArray baDirectory;
    XAINDecoder::decode(baStream, 0, nBudget, &baDirectory, pPdStruct);
    if (!guardedSource) return false;
    if (baDirectory.size() < ESP_RECORD_SIZE) return false;

    // Rebuilt byte for byte, because the parent field is an offset into it.
    QByteArray baPathBuffer;
    const qint32 nRecords = (qint32)(baDirectory.size() / ESP_RECORD_SIZE);
    for (qint32 i = 0; i < nRecords; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const uchar *pRecord = (const uchar *)baDirectory.constData() + ((qint64)i * ESP_RECORD_SIZE);

        const qint64 nSharedPrefix = (qint64)(qint32)qFromLittleEndian<quint32>(pRecord + 0);
        const quint16 nParent = qFromLittleEndian<quint16>(pRecord + 4);
        const quint8 nAttributes = pRecord[0x13];
        const quint16 nDosTime = qFromLittleEndian<quint16>(pRecord + 0x14);
        const quint16 nDosDate = qFromLittleEndian<quint16>(pRecord + 0x16);
        const qint64 nSize = (qint64)(qint32)qFromLittleEndian<quint32>(pRecord + 0x18);

        if ((nSharedPrefix < 0) || (nSize < 0) || (nSharedPrefix > nSize)) break;

        QByteArray baName((const char *)(pRecord + 6), ESP_NAME_SIZE);
        const qint32 nTerminator = baName.indexOf('\0');
        if (nTerminator >= 0) baName.truncate(nTerminator);
        if (baName.isEmpty()) break;

        QByteArray baParentPath;
        if (nParent != ESP_PARENT_ROOT) {
            if ((qint32)nParent >= baPathBuffer.size()) break;
            const qint32 nEnd = baPathBuffer.indexOf('\0', (qint32)nParent);
            if (nEnd < 0) break;
            baParentPath = baPathBuffer.mid((qint32)nParent, nEnd - (qint32)nParent);
            baParentPath.append('\\');
        }

        const QByteArray baFullPath = baParentPath + baName;

        MEMBER member = {};
        member.nSharedPrefix = nSharedPrefix;
        member.nSize = nSize;
        member.nContentOffset = -1;
        member.nDosDate = nDosDate;
        member.nDosTime = nDosTime;
        member.nAttributes = nAttributes;
        member.bIsFolder = ((nAttributes & ESP_ATTRIBUTE_DIRECTORY) != 0);
        member.sFileName = publishedName(baFullPath);

        if (member.bIsFolder) {
            if ((baPathBuffer.size() + baFullPath.size() + 1) > ESP_PATH_BUFFER_SIZE) break;
            baPathBuffer.append(baFullPath);
            baPathBuffer.append('\0');
        }

        pContext->listMembers.append(member);
        if (pContext->listMembers.size() >= ESP_MAX_MEMBERS) break;
    }

    return !pContext->listMembers.isEmpty();
}

// Replays the reference implementation's rolling prefix buffer over the one
// solid stream, so that every member's bytes exist before any of them is served.
bool XESPArchive::materialize(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    if (pContext->bEncrypted || (pContext->nFilter == ESP_FILTER_CHANNELS)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;

    qint64 nStreamTotal = 0;
    qint64 nContentTotal = 0;
    for (qint32 i = 0; i < pContext->listMembers.size(); ++i) {
        const MEMBER &member = pContext->listMembers.at(i);
        if (member.bIsFolder) continue;
        nStreamTotal += (member.nSize - member.nSharedPrefix);
        nContentTotal += member.nSize;
        // Both are capped: the inherited prefixes mean the spliced content can
        // be arbitrarily larger than the stream that feeds it.
        if ((nStreamTotal > ESP_MAX_CONTENT) || (nContentTotal > ESP_MAX_CONTENT)) return false;
    }

    const qint64 nBodyOffset = pContext->nContainerOffset + ESP_HEADER_SIZE;
    const qint64 nBodySize = pContext->nDirectoryOffset - nBodyOffset;
    if (nBodySize < 0) return false;

    QByteArray baStream;
    if (nStreamTotal == 0) {
        // Nothing to feed the codec: an archive of directories and empty
        // members never opens its stream, and handing zero bytes to a bit
        // reader is not a reading of it that terminates usefully.
    } else if (pContext->nMethod == ESP_METHOD_STORED) {
        // Stored bytes never pass through the codec's byte fill, so they are
        // not masked.
        if (nStreamTotal > nBodySize) return false;
        baStream = read_array_process(nBodyOffset, nStreamTotal, pPdStruct);
        if ((baStream.size() != nStreamTotal)) return false;
    } else {
        QByteArray baBody = read_array_process(nBodyOffset, nBodySize, pPdStruct);
        if ((baBody.size() != nBodySize)) return false;
        if (pContext->bScrambled) descramble(&baBody);
        if (!XAINDecoder::decode(baBody, 0, nStreamTotal, &baStream, pPdStruct)) return false;
        if ((baStream.size() != nStreamTotal)) return false;
    }

    if (pContext->nFilter == ESP_FILTER_DELTA) applyDeltaFilter(&baStream);

    QByteArray baContent;
    QByteArray baPrefixBuffer;
    qint64 nStreamPosition = 0;
    const qint32 nCount = pContext->listMembers.size();
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        MEMBER &member = pContext->listMembers[i];
        if (member.bIsFolder) continue;

        // The buffer is refilled for the NEXT record, whatever kind it is.
        qint64 nNextPrefix = 0;
        if ((i + 1) < nCount) nNextPrefix = pContext->listMembers.at(i + 1).nSharedPrefix;
        if (nNextPrefix < 0) nNextPrefix = 0;

        if (member.nSharedPrefix > (qint64)baPrefixBuffer.size()) return false;

        member.nContentOffset = baContent.size();
        baContent.append(baPrefixBuffer.constData(), (qint32)member.nSharedPrefix);
        qint64 nRemaining = member.nSize - member.nSharedPrefix;

        const qint64 nOldLength = baPrefixBuffer.size();
        const qint64 nFill = nNextPrefix - nOldLength;
        if (nFill > 0) {
            if (nFill > nRemaining) return false;
            if ((nStreamPosition + nFill) > baStream.size()) return false;
            baPrefixBuffer.append(baStream.constData() + nStreamPosition, (qint32)nFill);
            baContent.append(baStream.constData() + nStreamPosition, (qint32)nFill);
            nStreamPosition += nFill;
            nRemaining -= nFill;
        }
        if ((qint64)baPrefixBuffer.size() > nNextPrefix) baPrefixBuffer.truncate((qint32)nNextPrefix);

        if (nRemaining > 0) {
            if ((nStreamPosition + nRemaining) > baStream.size()) return false;
            baContent.append(baStream.constData() + nStreamPosition, (qint32)nRemaining);
            nStreamPosition += nRemaining;
        }

        if ((baContent.size() - member.nContentOffset) != member.nSize) return false;
    }

    pContext->baContent = baContent;
    pContext->bMaterialized = true;

    return true;
}

bool XESPArchive::parseContext(CONTEXT *pContext, bool bMaterialize, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    qint64 nContainerOffset = 0;
    if (!locateContainer(&nContainerOffset, pPdStruct)) return false;
    if (!readHeader(nContainerOffset, &context, pPdStruct)) return false;
    if (!readDirectory(&context, pPdStruct)) return false;

    if (bMaterialize && !materialize(&context, pPdStruct)) {
        if (!guardedSource) return false;
        // A password-protected or /MM2 archive still lists; only its bytes are
        // withheld, and unpackCurrent() is what refuses them.
        context.baContent.clear();
        context.bMaterialized = false;
    }

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XESPArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XESPArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XESPArchive archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XESPArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XESPArchive(pDevice);
}

QList<QString> XESPArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ESP>'");
}

XBinary::FT XESPArchive::getFileType()
{
    return FT_ESP;
}

XBinary::MODE XESPArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XESPArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XESPArchive::getArch()
{
    return QString();
}

qint32 XESPArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XESPArchive::getFileFormatExt()
{
    return QStringLiteral("esp");
}

QString XESPArchive::getFileFormatExtsString()
{
    return QStringLiteral("ESP (*.esp)");
}

QString XESPArchive::getMIMEString()
{
    return QStringLiteral("application/x-esp");
}

QString XESPArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();

    // The version byte is a FORMAT revision and its own documentation writes it
    // in hex (15h ESP 1.5, 16h ESP 1.6+, 17h written with /ME2, 19h written with
    // /MM2), so it is published that way rather than translated into an archiver
    // version this reader would have to guess for any byte the documentation
    // does not list.
    return QStringLiteral("0x%1").arg(context.nVersion, 2, 16, QLatin1Char('0'));
}

qint64 XESPArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QMap<XBinary::UNPACK_PROP, QVariant> XESPArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XESPArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    UNPACK_CONTEXT *pContext = new (std::nothrow) UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(&pContext->context, true, pPdStruct) || pContext->context.listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->context.nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->context.listMembers.size();
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

XBinary::ARCHIVERECORD XESPArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    const UNPACK_CONTEXT *pContext = (const UNPACK_CONTEXT *)pState->pContext;
    if (pState->nNumberOfRecords != pContext->context.listMembers.size()) return ARCHIVERECORD();

    const MEMBER &member = pContext->context.listMembers.at(pState->nCurrentIndex);

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = 0;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, member.bIsFolder);
    result.mapProperties.insert(FPART_PROP_ISREADONLY, (member.nAttributes & 0x01) != 0);
    result.mapProperties.insert(FPART_PROP_ISHIDDEN, (member.nAttributes & 0x02) != 0);
    result.mapProperties.insert(FPART_PROP_ISSYSTEM, (member.nAttributes & 0x04) != 0);
    result.mapProperties.insert(FPART_PROP_ISARCHIVE, (member.nAttributes & 0x20) != 0);
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, pContext->context.bEncrypted);

    const QDateTime dateTime = XBinary::dosDateTimeToQDateTime(member.nDosDate, member.nDosTime);
    if (dateTime.isValid()) {
        result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);
        result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
    }

    if (member.bIsFolder) {
        result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, (qint64)0);
        result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
        result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Directory"));
        return result;
    }

    // NO per-member packed size is published.  Every member comes out of the
    // one solid stream, so the only packed number that exists belongs to the
    // container; publishing the member's own size in that column would report
    // a 100% ratio for an archive that is in fact compressed three to one.
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_ISSOLID, true);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->context.bMaterialized ? HANDLE_METHOD_STORE : HANDLE_METHOD_UNKNOWN);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(pContext->context.nMethod));

    // A member is spliced out of a solid stream and a rolling prefix buffer, so
    // it is not a byte range of this device and must not advertise one.  This
    // also replaces the HANDLE_METHOD above, which is what keeps a generic
    // consumer from resolving STORE against the compressed carrier.
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();

    return result;
}

bool XESPArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    QIODevice *guardedOutput = pDevice;
    QIODevice *guardedSource = getDevice();
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !pDevice || !isUnpackOutputSupported(guardedOutput) ||
        devicesAlias(guardedSource, guardedOutput) || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    const UNPACK_CONTEXT *pContext = (const UNPACK_CONTEXT *)pState->pContext;
    if (pState->nNumberOfRecords != pContext->context.listMembers.size()) return false;

    const MEMBER &member = pContext->context.listMembers.at(pState->nCurrentIndex);
    if (member.bIsFolder) return false;
    if (!pContext->context.bMaterialized || (member.nContentOffset < 0)) {
        if (pContext->context.bEncrypted) {
            setPdStructErrorString(pPdStruct, tr("The archive is password protected"));
        }
        return false;
    }
    if ((member.nContentOffset + member.nSize) > pContext->context.baContent.size()) return false;
    if (!isUnpackOutputSizeAllowed(pState->mapUnpackProperties, member.nSize)) return false;

    // This route materializes its own output, so it must charge the operation
    // budget itself: publishUnpackOutput never debits the copy.
    if (pState->spOutputBudget) {
        if (!pState->spOutputBudget->beginEntry(pState->nCurrentIndex, member.sFileName)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
        if (!pState->spOutputBudget->debit(member.nSize)) {
            if (pState->spOutputBudget->isEnforcing()) {
                setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
                return false;
            }
            OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
        }
    }

    std::unique_ptr<QIODevice> pStage(createFileBuffer(member.nSize, pPdStruct));
    if (!pStage) return false;
    if (member.nSize > 0) {
        const qint64 nWritten = pStage->write(pContext->context.baContent.constData() + member.nContentOffset, member.nSize);
        if (nWritten != member.nSize) return false;
    }
    if (!pStage->seek(0) || !isUnpackSourceCurrent(pState, pPdStruct)) return false;

    const bool bResult = publishUnpackOutput(pStage.get(), guardedOutput, pState, pPdStruct);
    if (!bResult) return false;

    return true;
}

bool XESPArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) return true;
    pState->nCurrentOffset = pState->nTotalSize;

    return false;
}

bool XESPArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    UNPACK_CONTEXT *pContext = (UNPACK_CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XESPArchive::getAvailableFPARTProperties()
{
    // COMPRESSEDSIZE appears only on directory records, where it is the honest
    // zero; a file member has no packed size of its own.
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER << FPART_PROP_ISSOLID << FPART_PROP_DATETIME << FPART_PROP_MTIME
                               << FPART_PROP_ENCRYPTED << FPART_PROP_ISREADONLY << FPART_PROP_ISHIDDEN << FPART_PROP_ISSYSTEM << FPART_PROP_ISARCHIVE;
}
