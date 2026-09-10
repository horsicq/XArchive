/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xspis.h"

#include <QBuffer>
#include <QDir>
#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <limits>
#include <new>

namespace {
const qint64 SPIS_HEADER_SIZE = 21;
const qint64 SPIS_RECORD_HEADER_SIZE = 25;
const qint64 SPIS_MAX_MEMBER_SIZE = 512LL * 1024 * 1024;
const qint32 SPIS_MAX_RECORDS = 100000;
const qint32 SPIS_MAX_NAME_SIZE = 4096;
const qint32 SPIS_SNIFF_SIZE = 64;
// the flags word accepts 0, 1 and 2
const quint32 SPIS_FLAG_MAX = 2;
// flag 2 additionally XOR masks every non-stored member's payload
const quint32 SPIS_FLAG_OBFUSCATED = 2;
// Flags 1 and 2 bias the stored member checksum by this constant, which is
// literal in the reference. Under flag 2 the four little-endian bytes of it
// are ALSO the repeating XOR mask over each non-stored member's payload,
// cycling from that member's data offset - stored members are not masked.
const quint32 SPIS_FLAG_KEY = 0x01f4410bU;

quint16 spisRead16(const char *pData)
{
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(pData));
}

// The reference defines four cases here and honouring only the
// plain equality drops members even from archives that otherwise extract - one
// 199-member archive yielded 165.  A stored member whose stored value is zero
// is not verified at all, and the two obfuscation flags bias the sum by
// SPIS_FLAG_KEY.
bool spisChecksumMatches(const XSPIS::MEMBER &member, quint32 nSum)
{
    if ((member.method == XSPIS::METHOD_NON) && (member.nChecksum == 0)) return true;
    if (nSum == member.nChecksum) return true;
    if (member.nFlags == 0) return false;
    if (member.method == XSPIS::METHOD_NON) nSum = 0;
    return (quint32)(nSum + SPIS_FLAG_KEY) == member.nChecksum;
}

quint32 spisRead32(const char *pData)
{
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(pData));
}
}  // namespace

XSPIS::XSPIS(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSPIS::METHOD XSPIS::tagMethod(const QByteArray &baTag)
{
    if (baTag == QByteArrayLiteral("NON")) return METHOD_NON;
    if (baTag == QByteArrayLiteral("RLE")) return METHOD_RLE;
    if (baTag == QByteArrayLiteral("LZH")) return METHOD_LZH;
    if (baTag == QByteArrayLiteral("CUS")) return METHOD_CUS;
    if (baTag == QByteArrayLiteral("LH5")) return METHOD_LH5;
    return METHOD_INVALID;
}

XBinary::HANDLE_METHOD XSPIS::handleMethod(METHOD method)
{
    switch (method) {
        case METHOD_NON: return HANDLE_METHOD_STORE;
        case METHOD_RLE: return HANDLE_METHOD_SPIS_RLE;
        case METHOD_LZH: return HANDLE_METHOD_LZH1;
        case METHOD_LH5: return HANDLE_METHOD_LZH5;
        case METHOD_CUS:
        case METHOD_INVALID:
        default: return HANDLE_METHOD_UNKNOWN;
    }
}

QString XSPIS::methodName(METHOD method)
{
    switch (method) {
        case METHOD_NON: return QStringLiteral("NON");
        case METHOD_RLE: return QStringLiteral("RLE");
        case METHOD_LZH: return QStringLiteral("LZH");
        case METHOD_CUS: return QStringLiteral("CUS");
        case METHOD_LH5: return QStringLiteral("LH5");
        case METHOD_INVALID:
        default: return QStringLiteral("Unknown");
    }
}

QString XSPIS::safeMemberName(const QByteArray &baName)
{
    if (baName.isEmpty() || baName.contains('\0')) return QString();

    QString sName = QString::fromLatin1(baName).normalized(QString::NormalizationForm_C);
    sName.replace(QLatin1Char('\\'), QLatin1Char('/'));
    // Names are stored with a LEADING separator ("\\dir\\file"); the
    // reference strips it rather than treating the name as absolute.
    while (sName.startsWith(QLatin1Char('/'))) sName.remove(0, 1);
    if (sName.isEmpty() || QDir::isAbsolutePath(sName) || sName.contains(QLatin1Char(':'))) return QString();

    const QStringList listParts = sName.split(QLatin1Char('/'), Qt::KeepEmptyParts);
    for (const QString &sPart : listParts) {
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String(".."))) return QString();
    }
    return (XBinary::fixFileName(sName) == sName) ? sName : QString();
}

QString XSPIS::payloadExtension(const QByteArray &baPrefix)
{
    if (baPrefix.startsWith(QByteArrayLiteral("BM"))) return QStringLiteral("bmp");
    if (baPrefix.startsWith(QByteArrayLiteral("MZ"))) return QStringLiteral("exe");
    if (baPrefix.startsWith(QByteArrayLiteral("PK\x03\x04"))) return QStringLiteral("zip");
    if (baPrefix.startsWith(QByteArrayLiteral("GIF87a")) || baPrefix.startsWith(QByteArrayLiteral("GIF89a"))) return QStringLiteral("gif");
    if (baPrefix.startsWith(QByteArray::fromHex("89504e470d0a1a0a"))) return QStringLiteral("png");
    if (baPrefix.size() >= 12 && baPrefix.startsWith(QByteArrayLiteral("RIFF")) && (baPrefix.mid(8, 4) == QByteArrayLiteral("WAVE"))) return QStringLiteral("wav");
    return QStringLiteral("bin");
}

QDateTime XSPIS::dosDateTime(quint32 nValue)
{
    return XBinary::dosDateTimeToQDateTime(static_cast<quint16>(nValue >> 16), static_cast<quint16>(nValue));
}

bool XSPIS::parseInternalInfo(INTERNAL_INFO *pInfo, PDSTRUCT *pPdStruct)
{
    if (pInfo) *pInfo = INTERNAL_INFO();
    if (!pInfo || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSPIS> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedThis || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nFileSize = getSize();
    if (!guardedThis || !guardedSource || (nFileSize < SPIS_HEADER_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, SPIS_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SPIS_HEADER_SIZE) || (memcmp(baHeader.constData(), "SPIS\x1a", 5) != 0)) return false;

    const METHOD headerMethod = tagMethod(baHeader.mid(5, 3));
    const quint32 nTotalRawSize = spisRead32(baHeader.constData() + 8);
    const quint8 nArchiveType = static_cast<quint8>(baHeader.at(12));
    const quint32 nHeaderChecksum = spisRead32(baHeader.constData() + 13);
    const quint32 nFlags = spisRead32(baHeader.constData() + 17);
    // The word at +17 is a FLAGS field, not a reserved zero: the reference
    // accepts 0, 1 and 2, and flag 2 means the member payloads are XOR
    // obfuscated (see the note on the key below).
    if ((headerMethod == METHOD_INVALID) || (nArchiveType > 1) || (nFlags > SPIS_FLAG_MAX) || (nTotalRawSize == 0) ||
        (nTotalRawSize > SPIS_MAX_MEMBER_SIZE)) {
        return false;
    }

    QList<MEMBER> listMembers;
    if (nArchiveType == 0) {
        const qint64 nPackedSize = nFileSize - SPIS_HEADER_SIZE;
        if ((nPackedSize <= 0) || ((headerMethod == METHOD_NON) && (nPackedSize != nTotalRawSize))) return false;

        QByteArray baPrefix;
        const qint64 nSniffPackedSize = qMin<qint64>(nPackedSize, 512);
        const QByteArray baPackedPrefix = read_array_process(SPIS_HEADER_SIZE, nSniffPackedSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baPackedPrefix.size() != nSniffPackedSize)) return false;
        if (headerMethod == METHOD_NON) {
            baPrefix = baPackedPrefix.left(SPIS_SNIFF_SIZE);
        } else if (headerMethod == METHOD_RLE) {
            bool bEscape = false;
            quint8 nLast = 0;
            for (qint32 i = 0; (i < baPackedPrefix.size()) && (baPrefix.size() < SPIS_SNIFF_SIZE); ++i) {
                const quint8 nValue = static_cast<quint8>(baPackedPrefix.at(i));
                if (!bEscape) {
                    if (nValue == 0x94) {
                        bEscape = true;
                    } else {
                        baPrefix.append(static_cast<char>(nValue));
                        nLast = nValue;
                    }
                } else {
                    bEscape = false;
                    if (nValue >= 2) {
                        const qint32 nRepeat = qMin<qint32>(nValue - 1, SPIS_SNIFF_SIZE - baPrefix.size());
                        baPrefix.append(QByteArray(nRepeat, static_cast<char>(nLast)));
                    } else if (nValue == 0) {
                        baPrefix.append(static_cast<char>(0x94));
                    }
                    nLast = 0x94;
                }
            }
        }

        QString sBaseName = XBinary::fixFileName(XBinary::getDeviceFileBaseName(guardedSource.data()));
        if (!guardedThis || !guardedSource) return false;
        if (sBaseName.isEmpty()) sBaseName = QStringLiteral("payload");

        MEMBER member;
        member.nHeaderOffset = 0;
        member.nHeaderSize = SPIS_HEADER_SIZE;
        member.nDataOffset = SPIS_HEADER_SIZE;
        member.nPackedSize = nPackedSize;
        member.nRawSize = nTotalRawSize;
        member.nChecksum = nHeaderChecksum;
        member.nFlags = nFlags;
        member.method = headerMethod;
        member.sName = sBaseName + QLatin1Char('.') + payloadExtension(baPrefix);
        listMembers.append(member);
    } else {
        // Field @13 is NOT a checksum in the multi-member layout - the
        // reference never reads it, and 27 of the 73 archives carry
        // 0xAF00010D there.

        // The reference reads 21 bytes at the top of EVERY iteration and seeks
        // back 21 when they do not carry the magic: a record may be preceded by
        // a REPEATED archive header.  Four corpus archives hold 5..22 of those,
        // and the outer nTotalRawSize then covers only the members ahead of the
        // first one.  Every header declares the raw total of its OWN segment,
        // so the sum is carried and verified per segment rather than globally -
        // with no inner header that is exactly the old whole-file rule.
        quint64 nSegmentRawSum = 0;
        quint32 nSegmentTotal = nTotalRawSize;
        qint32 nInnerHeaders = 0;
        qint64 nOffset = SPIS_HEADER_SIZE;
        while (nOffset < nFileSize) {
            if (!XBinary::isPdStructNotCanceled(pPdStruct) || (listMembers.count() >= SPIS_MAX_RECORDS)) return false;

            if (nFileSize - nOffset >= SPIS_HEADER_SIZE) {
                const QByteArray baInner = read_array_process(nOffset, SPIS_HEADER_SIZE, pPdStruct);
                if (!guardedThis || !guardedSource || (baInner.size() != SPIS_HEADER_SIZE)) return false;
                if (memcmp(baInner.constData(), "SPIS\x1a", 5) == 0) {
                    // ("SPIS\x1a" can never be mistaken for a record header: it
                    // would mean a name of 0x5053 bytes, far over the cap.)
                    if (nInnerHeaders >= SPIS_MAX_RECORDS) return false;
                    const METHOD innerMethod = tagMethod(baInner.mid(5, 3));
                    const quint32 nInnerTotal = spisRead32(baInner.constData() + 8);
                    const quint8 nInnerType = static_cast<quint8>(baInner.at(12));
                    const quint32 nInnerFlags = spisRead32(baInner.constData() + 17);
                    if ((innerMethod == METHOD_INVALID) || (nInnerType != 1) || (nInnerFlags > SPIS_FLAG_MAX) || (nInnerTotal == 0) ||
                        (nInnerTotal > SPIS_MAX_MEMBER_SIZE)) {
                        return false;
                    }
                    // the segment that just ended has to account for exactly the
                    // raw bytes its own header declared
                    if (nSegmentRawSum != nSegmentTotal) return false;
                    nSegmentRawSum = 0;
                    nSegmentTotal = nInnerTotal;
                    ++nInnerHeaders;
                    nOffset += SPIS_HEADER_SIZE;
                    continue;
                }
            }

            if (nFileSize - nOffset < SPIS_RECORD_HEADER_SIZE) return false;
            const QByteArray baRecord = read_array_process(nOffset, SPIS_RECORD_HEADER_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource || (baRecord.size() != SPIS_RECORD_HEADER_SIZE)) return false;

            const quint16 nNameSize = spisRead16(baRecord.constData());
            const quint32 nDosDateTime = spisRead32(baRecord.constData() + 2);
            const quint16 nAttributes = spisRead16(baRecord.constData() + 6);
            const quint32 nRawSize = spisRead32(baRecord.constData() + 8);
            const quint32 nPackedSize = spisRead32(baRecord.constData() + 12);
            const quint8 nMethod = static_cast<quint8>(baRecord.at(16));
            const quint32 nChecksum = spisRead32(baRecord.constData() + 17);
            const quint32 nRecordFlags = spisRead32(baRecord.constData() + 21);
            if ((nNameSize == 0) || (nNameSize > SPIS_MAX_NAME_SIZE) || (nMethod >= METHOD_INVALID) || (nRecordFlags > SPIS_FLAG_MAX) || (nRawSize == 0) ||
                (nRawSize > SPIS_MAX_MEMBER_SIZE) || (nPackedSize == 0) || ((nMethod == METHOD_NON) && (nPackedSize != nRawSize))) {
                return false;
            }
            if (nNameSize > nFileSize - nOffset - SPIS_RECORD_HEADER_SIZE) return false;
            // The reference's loop stops at EOF, not at the end of the last
            // member, so an archive whose FINAL payload was cut short still
            // walks.  Only the last member may be short, and only behind at
            // least one whole member, so a lone bogus record is still refused.
            const qint64 nAvailable = nFileSize - nOffset - SPIS_RECORD_HEADER_SIZE - nNameSize;
            const bool bTruncated = (static_cast<qint64>(nPackedSize) > nAvailable);
            if (bTruncated && (listMembers.isEmpty() || (nAvailable <= 0))) return false;

            const QByteArray baName = read_array_process(nOffset + SPIS_RECORD_HEADER_SIZE, nNameSize, pPdStruct);
            if (!guardedThis || !guardedSource || (baName.size() != nNameSize)) return false;
            const QString sName = safeMemberName(baName);
            if (sName.isEmpty()) return false;

            MEMBER member;
            member.nHeaderOffset = nOffset;
            member.nHeaderSize = SPIS_RECORD_HEADER_SIZE + nNameSize;
            member.nDataOffset = nOffset + member.nHeaderSize;
            member.nPackedSize = bTruncated ? nAvailable : static_cast<qint64>(nPackedSize);
            member.bTruncated = bTruncated;
            member.nRawSize = nRawSize;
            member.nChecksum = nChecksum;
            member.nFlags = nRecordFlags;
            member.nDosDateTime = nDosDateTime;
            member.nAttributes = nAttributes;
            member.method = static_cast<METHOD>(nMethod);
            member.sName = sName;
            listMembers.append(member);

            nSegmentRawSum += nRawSize;
            if (nSegmentRawSum > nSegmentTotal) return false;
            nOffset = member.nDataOffset + member.nPackedSize;
        }
        if ((nOffset != nFileSize) || listMembers.isEmpty() || (nSegmentRawSum != nSegmentTotal)) return false;
    }

    if (!guardedThis || !guardedSource || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    pInfo->bIsValid = true;
    pInfo->nFileSize = nFileSize;
    pInfo->nArchiveType = nArchiveType;
    pInfo->nTotalRawSize = nTotalRawSize;
    pInfo->listMembers = listMembers;
    return true;
}

bool XSPIS::isValid(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO info;
    return parseInternalInfo(&info, pPdStruct);
}

bool XSPIS::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSPIS archive(pDevice);
    return archive.isValid(pPdStruct);
}

bool XSPIS::handleInternalInfo(PDSTRUCT *pPdStruct)
{
    QPointer<XSPIS> guardedThis(this);
    if (!isInternalInfoHandled()) {
        INTERNAL_INFO info;
        if (!parseInternalInfo(&info, pPdStruct) || !guardedThis) return false;
        if (!XArchive::handleInternalInfo(pPdStruct) || !guardedThis) return false;
        XArchive::INTERNAL_INFO *pBase = static_cast<XArchive::INTERNAL_INFO *>(XArchive::getInternalInfo(pPdStruct));
        if (!guardedThis || !pBase) return false;
        static_cast<XArchive::INTERNAL_INFO &>(info) = *pBase;
        m_internalInfo = info;
    }
    return true;
}

void *XSPIS::getInternalInfo(PDSTRUCT *pPdStruct)
{
    return handleInternalInfo(pPdStruct) ? &m_internalInfo : nullptr;
}

void XSPIS::setInternalInfo(void *pInternalInfo)
{
    if (pInternalInfo) {
        m_internalInfo = *static_cast<INTERNAL_INFO *>(pInternalInfo);
        XArchive::setInternalInfo(static_cast<XArchive::INTERNAL_INFO *>(&m_internalInfo));
    } else {
        m_internalInfo = INTERNAL_INFO();
        XArchive::setInternalInfo(nullptr);
    }
}

XBinary::FT XSPIS::getFileType()
{
    return FT_SPIS;
}

XBinary::MODE XSPIS::getMode()
{
    return MODE_DATA;
}

qint32 XSPIS::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XSPIS::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSPIS::getArch()
{
    return QString();
}

QString XSPIS::getFileFormatExt()
{
    return QStringLiteral("spis");
}

QString XSPIS::getFileFormatExtsString()
{
    return QStringLiteral("SPIS payload (*.spis)");
}

QString XSPIS::getMIMEString()
{
    return QStringLiteral("application/x-spis");
}

qint64 XSPIS::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    INTERNAL_INFO info;
    return parseInternalInfo(&info, pPdStruct) ? info.nFileSize : 0;
}

XBinary::OSNAME XSPIS::getOsName()
{
    return OSNAME_MSDOS;
}

QList<QString> XSPIS::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'SPIS'1A");
}

XBinary *XSPIS::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSPIS(pDevice);
}

QMap<XBinary::UNPACK_PROP, QVariant> XSPIS::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSPIS::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSPIS> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    SPIS_UNPACK_CONTEXT *pOldContext = static_cast<SPIS_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    *pState = UNPACK_STATE();
    delete pOldContext;
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    if (!bindUnpackSource(pState, pPdStruct) || !guardedThis) return false;

    INTERNAL_INFO info;
    if (!parseInternalInfo(&info, pPdStruct) || !guardedThis) {
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }
    for (const MEMBER &member : info.listMembers) {
        if (!XBinary::isUnpackOutputSizeAllowed(mapProperties, member.nRawSize)) {
            XBinary::setPdStructErrorString(pPdStruct, tr("SPIS output exceeds the configured limit"));
            releaseUnpackSource(pState);
            *pState = UNPACK_STATE();
            return false;
        }
    }

    SPIS_UNPACK_CONTEXT *pContext = new (std::nothrow) SPIS_UNPACK_CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    pContext->info = info;
    pState->pContext = pContext;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = info.listMembers.count();
    pState->nCurrentOffset = info.listMembers.first().nHeaderOffset;
    pState->nTotalSize = info.nFileSize;
    pState->mapUnpackProperties = mapProperties;
    if (!validateAndFinalizeUnpackSource(pState, pContext, pPdStruct)) {
        if (!guardedThis) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XSPIS::rawRecord(const MEMBER &member) const
{
    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nPackedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sName);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nRawSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nPackedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethod(member.method));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodName(member.method));
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, false);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_FILEMODE, (member.nAttributes & 1) ? static_cast<quint32>(0444) : static_cast<quint32>(0644));
    result.mapProperties.insert(FPART_PROP_ISREADONLY, (member.nAttributes & 0x01) != 0);
    result.mapProperties.insert(FPART_PROP_ISHIDDEN, (member.nAttributes & 0x02) != 0);
    result.mapProperties.insert(FPART_PROP_ISSYSTEM, (member.nAttributes & 0x04) != 0);
    result.mapProperties.insert(FPART_PROP_ISARCHIVE, (member.nAttributes & 0x20) != 0);
    result.mapProperties.insert(FPART_PROP_CHECKSUM, QStringLiteral("%1").arg(member.nChecksum, 8, 16, QLatin1Char('0')));
    result.mapProperties.insert(FPART_PROP_CHECKSUMTYPE, QStringLiteral("SPIS byte sum"));
    const QDateTime dateTime = dosDateTime(member.nDosDateTime);
    if (dateTime.isValid()) {
        // DATETIME drives the shared folder-extraction timestamp path; MTIME is
        // also published for newer archive metadata consumers.
        result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);
        result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
    }
    return result;
}

XBinary::ARCHIVERECORD XSPIS::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSPIS> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext) return ARCHIVERECORD();
    if (!isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) return ARCHIVERECORD();

    SPIS_UNPACK_CONTEXT *pContext = static_cast<SPIS_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->info.listMembers.count()) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nTotalSize != pContext->info.nFileSize)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = rawRecord(pContext->info.listMembers.at(pState->nCurrentIndex));
    if (!markArchiveStreamRecord(&result, pState->nCurrentIndex)) return ARCHIVERECORD();
    return result;
}

bool XSPIS::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    QPointer<XSPIS> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    QPointer<QIODevice> guardedSource(getDevice());
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !guardedThis || !guardedOutput || !guardedSource ||
        !isUnpackOutputSupported(guardedOutput.data()) || XBinary::devicesAlias(guardedSource.data(), guardedOutput.data()) ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    SPIS_UNPACK_CONTEXT *pContext = static_cast<SPIS_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->info.listMembers.count()) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords) || (pState->nTotalSize != pContext->info.nFileSize)) {
        return false;
    }
    const MEMBER member = pContext->info.listMembers.at(pState->nCurrentIndex);
    if ((handleMethod(member.method) == HANDLE_METHOD_UNKNOWN) || !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties, member.nRawSize)) return false;
    if (member.bTruncated) {
        // The archive ends inside this member's payload.  The format
        // authenticates every member by a sum over its DECOMPRESSED bytes, so
        // there is nothing to check a salvaged prefix against; it is refused
        // rather than published as if it were the file.
        XBinary::setPdStructErrorString(pPdStruct, tr("SPIS member is cut short by the end of the archive"));
        return false;
    }

    QIODevice *pWorkDevice = XBinary::createFileBuffer(member.nRawSize, pPdStruct);
    if (!pWorkDevice) return false;

    if (pState->spOutputBudget && !pState->spOutputBudget->beginEntry(pState->nCurrentIndex, member.sName)) {
        if (pState->spOutputBudget->isEnforcing()) {
            XBinary::setPdStructErrorString(pPdStruct, tr("Unpacked output exceeds the configured limit"));
            XBinary::freeFileBuffer(&pWorkDevice);
            return false;
        }
        XBinary::OUTPUT_BUDGET::noteShadowRefusal(pState->spOutputBudget.data());
    }

    XDecompress decompressor;
    connect(&decompressor, &XDecompress::errorMessage, this, &XBinary::errorMessage);
    connect(&decompressor, &XDecompress::infoMessage, this, &XBinary::infoMessage);
    ARCHIVERECORD record = rawRecord(member);

    // Under flag 2 the compressed payload is XOR masked and the codec must not
    // see it that way, so it is unmasked into a private buffer and the record
    // re-pointed at that. Stored members are never masked.
    QByteArray baUnmasked;
    QBuffer bufferUnmasked;
    QIODevice *pSourceDevice = guardedSource.data();
    if ((member.nFlags == SPIS_FLAG_OBFUSCATED) && (member.method != METHOD_NON)) {
        if ((member.nPackedSize < 0) || (member.nPackedSize > SPIS_MAX_MEMBER_SIZE)) return false;
        baUnmasked = read_array_process(member.nDataOffset, member.nPackedSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baUnmasked.size() != member.nPackedSize)) return false;
        quint8 *pMask = (quint8 *)baUnmasked.data();
        for (qint64 i = 0; i < member.nPackedSize; ++i) {
            pMask[i] = (quint8)(pMask[i] ^ (quint8)((SPIS_FLAG_KEY >> (8 * (i & 3))) & 0xff));
        }
        bufferUnmasked.setBuffer(&baUnmasked);
        if (!bufferUnmasked.open(QIODevice::ReadOnly)) return false;
        pSourceDevice = &bufferUnmasked;
        record.nStreamOffset = 0;
    }

    bool bResult = decompressor.decompressArchiveRecord(record, pSourceDevice, pWorkDevice, pState->mapUnpackProperties, pPdStruct, pState->spOutputBudget);
    bResult = bResult && guardedThis && guardedOutput && guardedSource && (pWorkDevice->size() == member.nRawSize) &&
              isUnpackSourceCurrent(pState, pPdStruct) && guardedThis && XBinary::isPdStructNotCanceled(pPdStruct);

    quint32 nChecksum = 0;
    if (bResult) {
        bResult = pWorkDevice->seek(0);
        QByteArray baBuffer(0x10000, '\0');
        qint64 nRemaining = member.nRawSize;
        while (bResult && (nRemaining > 0)) {
            const qint64 nRead = pWorkDevice->read(baBuffer.data(), qMin<qint64>(baBuffer.size(), nRemaining));
            if (nRead <= 0) {
                bResult = false;
                break;
            }
            for (qint64 i = 0; i < nRead; ++i) nChecksum += static_cast<quint8>(baBuffer.at(static_cast<qint32>(i)));
            nRemaining -= nRead;
            bResult = XBinary::isPdStructNotCanceled(pPdStruct);
        }
        bResult = bResult && (nRemaining == 0) && spisChecksumMatches(member, nChecksum);
        if (!bResult && XBinary::isPdStructNotCanceled(pPdStruct)) XBinary::setPdStructErrorString(pPdStruct, tr("SPIS byte-sum mismatch"));
    }

    if (bResult) {
        bResult = guardedThis && guardedOutput && guardedSource && isUnpackSourceCurrent(pState, pPdStruct) && guardedThis &&
                  publishUnpackOutput(pWorkDevice, guardedOutput.data(), pState, pPdStruct);
    }
    XBinary::freeFileBuffer(&pWorkDevice);
    if (bResult && guardedThis) pState->nCurrentOffset = member.nDataOffset + member.nPackedSize;
    return bResult && guardedThis && guardedOutput && guardedSource;
}

bool XSPIS::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    QPointer<XSPIS> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext || !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !XBinary::isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    SPIS_UNPACK_CONTEXT *pContext = static_cast<SPIS_UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != pContext->info.listMembers.count()) || (pState->nCurrentIndex < 0) || (pState->nCurrentIndex >= pState->nNumberOfRecords)) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        pState->nCurrentOffset = pState->nTotalSize;
        return false;
    }
    pState->nCurrentOffset = pContext->info.listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    return true;
}

bool XSPIS::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    SPIS_UNPACK_CONTEXT *pContext = static_cast<SPIS_UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete pContext;
    return true;
}

QList<XBinary::FPART_PROP> XSPIS::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_REPORTEDMETHOD
                               << FPART_PROP_ENCRYPTED << FPART_PROP_FILEMODE << FPART_PROP_ISFOLDER << FPART_PROP_DATETIME << FPART_PROP_MTIME << FPART_PROP_ISREADONLY
                               << FPART_PROP_ISHIDDEN << FPART_PROP_ISSYSTEM << FPART_PROP_ISARCHIVE << FPART_PROP_CHECKSUM << FPART_PROP_CHECKSUMTYPE;
}
