/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */
#include "xfpak.h"

#include "xfpakdecoder.h"

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <limits>
#include <new>

namespace {
const qint64 FPAK_GLOBAL_HEADER_SIZE = 16;
const qint64 FPAK_SEGMENT_HEADER_SIZE = 30;
const qint64 FPAK_MAX_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint32 FPAK_MAX_SEGMENTS = 100000;
const qint32 FPAK_MAX_VOLUMES = 99;

bool rangeWithin(qint64 total, qint64 offset, qint64 size)
{
    return (total >= 0) && (offset >= 0) && (size >= 0) &&
           (offset <= total) && (size <= total - offset);
}

bool readExactAt(QIODevice *device, qint64 offset, qint64 size,
                 QByteArray *data, XBinary::PDSTRUCT *pPdStruct)
{
    if (data) data->clear();
    QPointer<QIODevice> guarded(device);
    if (!data || !guarded) return false;
    const bool sequential = guarded->isSequential();
    if (!guarded || sequential) return false;
    const qint64 deviceSize = guarded->size();
    if (!guarded || !rangeWithin(deviceSize, offset, size) ||
        (size > (std::numeric_limits<qint32>::max)()))
        return false;
    const bool seeked = guarded->seek(offset);
    if (!guarded || !seeked)
        return false;

    data->resize(qint32(size));
    qint64 done = 0;
    while (done < size) {
        if (!guarded || !XBinary::isPdStructNotCanceled(pPdStruct)) {
            data->clear();
            return false;
        }
        const qint64 chunk = qMin<qint64>(size - done, 64 * 1024);
        const qint64 count = guarded->read(data->data() + done, chunk);
        if (!guarded || (count != chunk)) {
            data->clear();
            return false;
        }
        done += count;
    }
    return true;
}

quint16 readLE16(const QByteArray &data, qint32 offset)
{
    return qFromLittleEndian<quint16>(
        reinterpret_cast<const uchar *>(data.constData() + offset));
}

quint32 readLE32(const QByteArray &data, qint32 offset)
{
    return qFromLittleEndian<quint32>(
        reinterpret_cast<const uchar *>(data.constData() + offset));
}

bool printableAscii(const QByteArray &data)
{
    if (data.isEmpty() || data.contains('\0')) return false;
    for (char ch : data) {
        const quint8 value = quint8(ch);
        if ((value < 0x20U) || (value > 0x7eU)) return false;
    }
    return true;
}

QString safeName(const QByteArray &data)
{
    if (!printableAscii(data)) return QString();
    QString name = QString::fromLatin1(data);
    if ((name.trimmed() != name) || QDir::isAbsolutePath(name) ||
        name.contains(QLatin1Char(':')))
        return QString();

    // Member names carry DOS SUBDIRECTORY PATHS - 4497 of the 5169 segment
    // names in the reference family contain a backslash - so the separator
    // is translated rather than rejected. Every traversal protection is
    // kept, applied per component instead of to the whole string.
    name.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (name.startsWith(QLatin1Char('/')) || name.endsWith(QLatin1Char('/')))
        return QString();

    const QStringList listParts = name.split(QLatin1Char('/'));
    for (qint32 i = 0; i < listParts.count(); ++i) {
        const QString sPart = listParts.at(i);
        if (sPart.isEmpty() || (sPart == QLatin1String(".")) || (sPart == QLatin1String("..")) ||
            (XBinary::fixFileName(sPart) != sPart))
            return QString();
    }
    return name;
}

QString resolveCaseInsensitivePath(const QString &candidate)
{
    const QFileInfo exact(candidate);
    if (exact.exists() && exact.isFile()) return exact.absoluteFilePath();

    QDir directory(exact.absolutePath());
    const QString wanted = exact.fileName().toCaseFolded();
    const QFileInfoList files = directory.entryInfoList(
        QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &file : files) {
        if (file.fileName().toCaseFolded() == wanted)
            return file.absoluteFilePath();
    }
    return QString();
}

}  // namespace

XFpakArchive::XFpakArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

bool XFpakArchive::readVolume(QIODevice *pDevice, const QString &sPath,
                              bool bRequireContinuation,
                              bool bAllowTruncatedTail, VOLUME *pVolume,
                              PDSTRUCT *pPdStruct)
{
    if (pVolume) *pVolume = VOLUME();
    QPointer<QIODevice> guarded(pDevice);
    if (!pVolume || !guarded) return false;
    const bool sequential = guarded->isSequential();
    if (!guarded || sequential ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    const qint64 fileSize = guarded->size();
    if (!guarded || (fileSize < 4) || (fileSize > FPAK_MAX_SIZE))
        return false;

    QByteArray magic;
    if (!readExactAt(guarded.data(), 0, 4, &magic, pPdStruct) || !guarded)
        return false;
    const bool lead = magic == QByteArray("FPAK", 4);
    const bool continuation = magic == QByteArray("FPAC", 4);
    if ((!lead && !continuation) || (bRequireContinuation && !continuation))
        return false;

    qint64 position = 4;
    quint16 version = 0;
    qint64 totalPacked = 0;
    qint64 totalRaw = 0;
    if (lead) {
        QByteArray header;
        if (!readExactAt(guarded.data(), 0, FPAK_GLOBAL_HEADER_SIZE,
                         &header, pPdStruct) || !guarded)
            return false;
        version = readLE16(header, 4);
        totalPacked = readLE32(header, 6);
        totalRaw = readLE32(header, 10);
        const qint32 descriptionSize = readLE16(header, 14);
        // NOT a version: this is the number of members in the whole
        // distribution set, so a lead volume of a large set carries a large
        // value here. Restricting it to 1|2 rejected 61 of 101 volumes.
        if ((version < 1) || (totalPacked < 1) ||
            (totalPacked > FPAK_MAX_SIZE) || (totalRaw < 1) ||
            (totalRaw > FPAK_MAX_SIZE) || (descriptionSize < 1) ||
            (descriptionSize > 1024) ||
            !rangeWithin(fileSize, FPAK_GLOBAL_HEADER_SIZE,
                         descriptionSize))
            return false;
        QByteArray description;
        if (!readExactAt(guarded.data(), FPAK_GLOBAL_HEADER_SIZE,
                         descriptionSize, &description, pPdStruct) ||
            !guarded || !printableAscii(description))
            return false;
        position = FPAK_GLOBAL_HEADER_SIZE + descriptionSize;
    }

    QList<SEGMENT> segments;
    bool truncated = false;
    while (position < fileSize) {
        if (segments.count() >= FPAK_MAX_SEGMENTS) return false;

        // Everything the walk cannot make sense of ends the chain.  Whether
        // that is fatal, or simply the end of the recoverable part of a volume
        // whose middle was damaged, is decided once at the bottom of the block:
        // only a caller that owns the whole volume may keep what it walked, and
        // only behind at least one complete segment.  Device errors stay fatal
        // either way - they say nothing about the file.
        bool malformed =
            !rangeWithin(fileSize, position, FPAK_SEGMENT_HEADER_SIZE);

        QByteArray fixedHeader;
        if (!malformed) {
            if (!readExactAt(guarded.data(), position,
                             FPAK_SEGMENT_HEADER_SIZE, &fixedHeader,
                             pPdStruct) ||
                !guarded)
                return false;
            malformed = !fixedHeader.startsWith("FPPF");
        }

        // +4 is the PKZIP general-purpose flag word and +6 the compression
        // method, and both are live: the reference family carries flags 0
        // (3306 segments), 8 (984) and 14 (831), and method 0 (stored) on 48.
        // Only 0x02 (8 KiB dictionary) and 0x04 (literal tree) reach the codec.
        quint16 segmentFlags = 0;
        quint16 segmentMethod = 0;
        quint16 dosTime = 0;
        quint16 dosDate = 0;
        quint32 crc32 = 0;
        qint64 packedSize = 0;
        qint64 rawSize = 0;
        qint64 segmentSize = 0;
        qint32 nameSize = 0;
        if (!malformed) {
            segmentFlags = readLE16(fixedHeader, 4);
            segmentMethod = readLE16(fixedHeader, 6);
            dosTime = readLE16(fixedHeader, 8);
            dosDate = readLE16(fixedHeader, 10);
            crc32 = readLE32(fixedHeader, 12);
            packedSize = readLE32(fixedHeader, 16);
            rawSize = readLE32(fixedHeader, 20);
            segmentSize = readLE32(fixedHeader, 24);
            nameSize = readLE16(fixedHeader, 28);
            malformed =
                ((segmentMethod != 0) && (segmentMethod != 6)) ||
                (packedSize < 1) || (packedSize > FPAK_MAX_SIZE) ||
                (rawSize < 1) || (rawSize > FPAK_MAX_SIZE) ||
                (segmentSize < 1) || (segmentSize > packedSize) ||
                (nameSize < 1) || (nameSize > 512) ||
                !rangeWithin(fileSize, position + FPAK_SEGMENT_HEADER_SIZE,
                             qint64(nameSize) + segmentSize) ||
                ((dosTime || dosDate) &&
                 !XBinary::isValidDosDateTime(dosDate, dosTime));
        }

        QByteArray nameBytes;
        QString name;
        if (!malformed) {
            if (!readExactAt(guarded.data(),
                             position + FPAK_SEGMENT_HEADER_SIZE, nameSize,
                             &nameBytes, pPdStruct) ||
                !guarded)
                return false;
            name = safeName(nameBytes);
            malformed = name.isEmpty();
        }

        if (malformed) {
            if (!bAllowTruncatedTail || segments.isEmpty()) return false;
            truncated = true;
            break;
        }

        SEGMENT segment;
        segment.sPath = sPath;
        segment.nVolumeSize = fileSize;
        segment.nHeaderOffset = position;
        segment.nHeaderSize = FPAK_SEGMENT_HEADER_SIZE + nameSize;
        segment.nDataOffset = position + segment.nHeaderSize;
        segment.nDataSize = segmentSize;
        segment.nPackedSize = packedSize;
        segment.nRawSize = rawSize;
        segment.nCRC32 = crc32;
        segment.nMethod = segmentMethod;
        segment.nFlags = segmentFlags;
        segment.nDosTime = dosTime;
        segment.nDosDate = dosDate;
        segment.sFileName = name;
        segment.baPinnedHeader = fixedHeader + nameBytes;
        segments.append(segment);

        position = segment.nDataOffset + segment.nDataSize;
    }

    if (!guarded || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        segments.isEmpty() || (!truncated && (position != fileSize)))
        return false;

    pVolume->bLead = lead;
    pVolume->bTruncated = truncated;
    pVolume->nVersion = version;
    pVolume->nPackedSize = totalPacked;
    pVolume->nRawSize = totalRaw;
    pVolume->nFileSize = fileSize;
    pVolume->nParsedSize = position;
    pVolume->listSegments = segments;
    return true;
}

XFpakArchive::ASSEMBLY_STATUS XFpakArchive::assemble(
    const QList<SEGMENT> &listSegments, qint64 nExpectedPacked,
    qint64 nExpectedRaw, QList<MEMBER> *pComplete, MEMBER *pPartial) const
{
    if (pComplete) pComplete->clear();
    if (pPartial) *pPartial = MEMBER();
    if (!pComplete || !pPartial || listSegments.isEmpty() ||
        (nExpectedPacked < 1) || (nExpectedRaw < 1))
        return ASSEMBLY_MALFORMED;

    qint64 completedPacked = 0;
    qint64 completedRaw = 0;
    MEMBER current;
    for (const SEGMENT &segment : listSegments) {
        if (current.listSegments.isEmpty()) {
            current.nPackedSize = segment.nPackedSize;
            current.nRawSize = segment.nRawSize;
            current.nCRC32 = segment.nCRC32;
            current.nMethod = segment.nMethod;
            current.nFlags = segment.nFlags;
            current.nDosTime = segment.nDosTime;
            current.nDosDate = segment.nDosDate;
            current.sFileName = segment.sFileName;
        } else {
            const SEGMENT &first = current.listSegments.constFirst();
            if ((first.nPackedSize != segment.nPackedSize) ||
                (first.nRawSize != segment.nRawSize) ||
                (first.nCRC32 != segment.nCRC32) ||
                (first.nDosTime != segment.nDosTime) ||
                (first.nDosDate != segment.nDosDate) ||
                (first.sFileName != segment.sFileName))
                return ASSEMBLY_MALFORMED;
        }

        if (current.nCompressedSize >
            current.nPackedSize - segment.nDataSize)
            return ASSEMBLY_MALFORMED;
        current.listSegments.append(segment);
        current.nCompressedSize += segment.nDataSize;
        if (current.nCompressedSize == current.nPackedSize) {
            current.bComplete = true;
            if ((completedPacked > nExpectedPacked - current.nPackedSize) ||
                (completedRaw > nExpectedRaw - current.nRawSize))
                return ASSEMBLY_MALFORMED;
            completedPacked += current.nPackedSize;
            completedRaw += current.nRawSize;
            pComplete->append(current);
            current = MEMBER();
        }
    }

    if (!current.listSegments.isEmpty()) *pPartial = current;
    if ((completedPacked > nExpectedPacked) ||
        (completedRaw > nExpectedRaw))
        return ASSEMBLY_MALFORMED;
    if (current.listSegments.isEmpty() &&
        (completedPacked == nExpectedPacked) &&
        (completedRaw == nExpectedRaw))
        return ASSEMBLY_COMPLETE;
    return ASSEMBLY_INCOMPLETE;
}

bool XFpakArchive::buildContext(UNPACK_CONTEXT *pContext,
                                PDSTRUCT *pPdStruct)
{
    if (pContext) *pContext = UNPACK_CONTEXT();
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pContext || !guardedSource) return false;
    const bool sequential = guardedSource->isSequential();
    if (!guardedSource || sequential ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    VOLUME first;
    if (!readVolume(guardedSource.data(), QString(), false, true, &first,
                    pPdStruct) ||
        !guardedSource)
        return false;

    QList<MEMBER> members;
    if (!first.bLead) {
        qint32 part = 0;
        for (const SEGMENT &segment : first.listSegments) {
            MEMBER member;
            member.listSegments.append(segment);
            member.nCompressedSize = segment.nDataSize;
            member.nPackedSize = segment.nPackedSize;
            member.nRawSize = segment.nRawSize;
            member.nCRC32 = segment.nCRC32;
            member.nDosTime = segment.nDosTime;
            member.nDosDate = segment.nDosDate;
            // The codec profile has to be carried across too.  Leaving it at
            // the struct defaults decoded every member of a continuation volume
            // as method 6 with flags 0, which silently loses the 8 KiB
            // dictionary (0x02) and the literal tree (0x04): the assemble()
            // path below copies both, this one did not.
            member.nMethod = segment.nMethod;
            member.nFlags = segment.nFlags;
            member.sFileName = segment.sFileName;
            member.bComplete = segment.nDataSize == segment.nPackedSize;
            if (!member.bComplete)
                member.sFileName += QStringLiteral(".part%1")
                                        .arg(++part, 2, 10,
                                             QLatin1Char('0'));
            members.append(member);
        }
    } else {
        QList<SEGMENT> physical = first.listSegments;
        QList<MEMBER> complete;
        MEMBER partial;
        ASSEMBLY_STATUS status =
            assemble(physical, first.nPackedSize, first.nRawSize, &complete,
                     &partial);
        if (status == ASSEMBLY_MALFORMED) return false;

        QString mediaBase;
        QFile *sourceFile = qobject_cast<QFile *>(guardedSource.data());
        if (sourceFile && !sourceFile->fileName().isEmpty()) {
            const QFileInfo sourceInfo(sourceFile->fileName());
            if (sourceInfo.suffix().compare(QLatin1String("pak"),
                                            Qt::CaseInsensitive) == 0) {
                mediaBase = QDir(sourceInfo.absolutePath())
                                .filePath(sourceInfo.completeBaseName());
            }
        }

        // A truncated volume is short of bytes that belong to IT, not to a
        // continuation, so its shortfall must not be made up from the media
        // set: appending a sibling's segments to a chain that stopped early
        // would splice the wrong data onto the interrupted member.
        for (qint32 volumeNumber = 1;
             (status == ASSEMBLY_INCOMPLETE) && !first.bTruncated &&
             (volumeNumber <= FPAK_MAX_VOLUMES) && !mediaBase.isEmpty();
             ++volumeNumber) {
            const QString path = resolveCaseInsensitivePath(
                QStringLiteral("%1.PA%2").arg(mediaBase).arg(volumeNumber));
            if (path.isEmpty()) break;

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly) || file.isSequential())
                return false;
            VOLUME next;
            const bool volumeOk = readVolume(&file, path, true, false,
                                             &next, pPdStruct);
            file.close();
            if (!volumeOk || !guardedSource) return false;
            physical += next.listSegments;
            status = assemble(physical, first.nPackedSize, first.nRawSize,
                              &complete, &partial);
            if (status == ASSEMBLY_MALFORMED) return false;
        }

        members = complete;
        if (status == ASSEMBLY_INCOMPLETE) {
            // A split stream gives us a concrete partial member to expose.
            // If the global totals instead claim an entirely absent member,
            // there is no trustworthy name/header to list and the lead
            // header is malformed (or a whole continuation was skipped) -
            // unless the walk itself already reported where the volume stopped
            // making sense, which explains the shortfall on its own.
            if (partial.listSegments.isEmpty()) {
                if (!first.bTruncated) return false;
            } else {
                partial.sFileName += QStringLiteral(".part01");
                members.append(partial);
            }
        }
    }

    if (!guardedSource || members.isEmpty() ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    pContext->nSourceSize = first.nFileSize;
    pContext->nArchiveSize = first.nFileSize;
    pContext->nVersion = first.nVersion;
    pContext->listMembers = members;
    return true;
}

bool XFpakArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> source(getDevice());
    const qint64 savedPosition = source ? source->pos() : -1;
    VOLUME volume;
    const bool result = source &&
        readVolume(source.data(), QString(), false, true, &volume,
                   pPdStruct);
    if (source && (savedPosition >= 0)) source->seek(savedPosition);
    return result;
}

bool XFpakArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XFpakArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XFpakArchive::createInstance(QIODevice *pDevice, bool bIsImage,
                                      XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XFpakArchive(pDevice);
}

XBinary::FT XFpakArchive::getFileType()
{
    return FT_FPAK;
}

XBinary::MODE XFpakArchive::getMode()
{
    return MODE_DATA;
}

qint32 XFpakArchive::getType()
{
    return TYPE_ARCHIVE;
}

XBinary::ENDIAN XFpakArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XFpakArchive::getArch()
{
    return QString();
}

QString XFpakArchive::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XFpakArchive::getFileFormatExtsString()
{
    return QStringLiteral("FoxPro Distribution Kit archive (*.pak;*.pa?)");
}

QString XFpakArchive::getMIMEString()
{
    return QStringLiteral("application/x-foxpro-fpak");
}

qint64 XFpakArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> source(getDevice());
    const qint64 savedPosition = source ? source->pos() : -1;
    VOLUME volume;
    const qint64 result = source &&
                                 readVolume(source.data(), QString(), false, true,
                                            &volume, pPdStruct)
                             // where the chain actually ended, which is the
                             // whole volume unless it was cut short
                             ? volume.nParsedSize
                             : 0;
    if (source && (savedPosition >= 0)) source->seek(savedPosition);
    return result;
}

XBinary::OSNAME XFpakArchive::getOsName()
{
    return OSNAME_MSDOS;
}

QString XFpakArchive::getVersion()
{
    QPointer<QIODevice> source(getDevice());
    if (!source) return QString();
    const bool sequential = source->isSequential();
    if (!source || sequential) return QString();
    const qint64 sourceSize = source->size();
    if (!source || (sourceSize < 6)) return QString();
    const qint64 savedPosition = source->pos();
    if (!source || (savedPosition < 0)) return QString();
    QByteArray header;
    const QString result =
        readExactAt(source.data(), 0, 6, &header, nullptr) && source &&
                header.startsWith("FPAK")
            ? QString::number(readLE16(header, 4))
            : QString();
    if (source && (savedPosition >= 0)) source->seek(savedPosition);
    return result;
}

QList<QString> XFpakArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'FPAK'|'FPAC'");
}

QMap<XBinary::UNPACK_PROP, QVariant>
XFpakArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XFpakArchive::initUnpack(
    UNPACK_STATE *pState,
    const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XFpakArchive> guardedThis(this);
    if (m_bUnpackOperationInProgress) return false;
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;

    UNPACK_CONTEXT *oldContext =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    *pState = UNPACK_STATE();
    delete oldContext;
    if (!guardedThis || !XBinary::isPdStructNotCanceled(pPdStruct) ||
        !bindUnpackSource(pState, pPdStruct) || !guardedThis)
        return false;

    UNPACK_CONTEXT *context = new (std::nothrow) UNPACK_CONTEXT;
    if (!context || !buildContext(context, pPdStruct) || !guardedThis) {
        delete context;
        if (guardedThis) releaseUnpackSource(pState);
        *pState = UNPACK_STATE();
        return false;
    }

    pState->pContext = context;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = context->listMembers.count();
    pState->nCurrentOffset =
        context->listMembers.constFirst().listSegments.constFirst()
            .nHeaderOffset;
    pState->nTotalSize = context->nSourceSize;
    pState->mapUnpackProperties = mapProperties;

    if (!validateAndFinalizeUnpackSource(pState, context, pPdStruct)) {
        if (!guardedThis) {
            delete context;
            *pState = UNPACK_STATE();
            return false;
        }
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete context;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XFpakArchive::memberRecord(const MEMBER &member,
                                                   qint32 nIndex) const
{
    ARCHIVERECORD result = {};
    if (member.listSegments.isEmpty()) return result;
    const SEGMENT &first = member.listSegments.constFirst();
    result.nStreamOffset = first.nDataOffset;
    result.nStreamSize = first.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                member.bComplete ? member.nPackedSize
                                                 : member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.bComplete ? member.nRawSize
                                                 : member.nCompressedSize);
    result.mapProperties.insert(
        FPART_PROP_HANDLEMETHOD,
        member.bComplete ? HANDLE_METHOD_FPAK_COMPRESSED
                         : HANDLE_METHOD_UNKNOWN);
    if (member.bComplete) {
        // unpackCurrent() below hands nMethod/nFlags straight to the decoder,
        // so the record itself never needed them - but a record that names a
        // codec without its profile is not self-describing, and the shared
        // chain's decFpakProfile() answers a missing property with a REAL
        // profile (method 6, flags 0) rather than refusing.  Any generic
        // consumer of this record would therefore decode a stored member as
        // Implode, and lose the 8 KiB dictionary (0x02) and literal tree
        // (0x04) bits, exactly as the continuation-volume bug once did.
        QByteArray profile(4, 0);
        qToLittleEndian<quint16>(
            member.nMethod, reinterpret_cast<uchar *>(profile.data()));
        qToLittleEndian<quint16>(
            member.nFlags, reinterpret_cast<uchar *>(profile.data()) + 2);
        result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, profile);
    }
    result.mapProperties.insert(
        FPART_PROP_REPORTEDMETHOD,
        member.bComplete
            ? QStringLiteral("FoxPro FPAK compression")
            : QStringLiteral("FoxPro FPAK split stream (incomplete)"));
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET,
                                first.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, first.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, quint32(0644));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_ENCRYPTED, false);
    if (member.bComplete) {
        result.mapProperties.insert(
            FPART_PROP_CRC_TYPE,
            quint32(CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF));
        result.mapProperties.insert(FPART_PROP_RESULTCRC, member.nCRC32);
        const QDateTime dateTime = XBinary::dosDateTimeToQDateTime(
            member.nDosDate, member.nDosTime);
        if (dateTime.isValid()) {
            result.mapProperties.insert(FPART_PROP_DATETIME, dateTime);
            result.mapProperties.insert(FPART_PROP_MTIME, dateTime);
        }
    }

    bool externalExtent = member.listSegments.count() != 1;
    for (const SEGMENT &segment : member.listSegments)
        externalExtent = externalExtent || !segment.sPath.isEmpty();
    if (externalExtent && !XBinary::markArchiveStreamRecord(&result, nIndex))
        return ARCHIVERECORD();
    return result;
}

XBinary::ARCHIVERECORD XFpakArchive::infoCurrent(UNPACK_STATE *pState,
                                                  PDSTRUCT *pPdStruct)
{
    QPointer<XFpakArchive> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                           &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return ARCHIVERECORD();
    UNPACK_CONTEXT *context =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nTotalSize != context->nSourceSize) ||
        (pState->nNumberOfRecords != context->listMembers.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return ARCHIVERECORD();
    return memberRecord(context->listMembers.at(pState->nCurrentIndex),
                        pState->nCurrentIndex);
}

bool XFpakArchive::readMemberData(const MEMBER &member, QByteArray *pData,
                                  PDSTRUCT *pPdStruct)
{
    if (pData) pData->clear();
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pData || !guardedSource || member.listSegments.isEmpty() ||
        (member.nCompressedSize < 1) ||
        (member.nCompressedSize > (std::numeric_limits<qint32>::max)()))
        return false;
    pData->reserve(qint32(member.nCompressedSize));

    for (const SEGMENT &segment : member.listSegments) {
        QFile sibling;
        QIODevice *device = guardedSource.data();
        if (!segment.sPath.isEmpty()) {
            sibling.setFileName(segment.sPath);
            if (!sibling.open(QIODevice::ReadOnly) || sibling.isSequential())
                return false;
            device = &sibling;
        }
        if (!device || (device->size() != segment.nVolumeSize)) return false;

        QByteArray header;
        QByteArray data;
        if (!readExactAt(device, segment.nHeaderOffset,
                         segment.nHeaderSize, &header, pPdStruct) ||
            (header != segment.baPinnedHeader) || !guardedSource)
            return false;
        // Reacquire the caller-owned device after the first callback-capable
        // read; a QIODevice may delete itself from a read implementation.
        if (segment.sPath.isEmpty()) device = guardedSource.data();
        if (!readExactAt(device, segment.nDataOffset, segment.nDataSize,
                         &data, pPdStruct) || !guardedSource)
            return false;
        pData->append(data);
    }
    return guardedSource &&
           (pData->size() == member.nCompressedSize) &&
           XBinary::isPdStructNotCanceled(pPdStruct);
}

bool XFpakArchive::unpackCurrent(UNPACK_STATE *pState, QIODevice *pDevice,
                                 PDSTRUCT *pPdStruct)
{
    QPointer<XFpakArchive> guardedThis(this);
    QPointer<QIODevice> guardedOutput(pDevice);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !guardedOutput || !isUnpackOutputSupported(guardedOutput.data()) ||
        !guardedThis || !isUnpackSourceCurrent(pState, pPdStruct) ||
        !guardedThis || !guardedOutput ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;

    UNPACK_CONTEXT *context =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != context->listMembers.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    const MEMBER member = context->listMembers.at(pState->nCurrentIndex);
    if (!member.bComplete) {
        XBinary::setPdStructErrorString(
            pPdStruct, tr("FoxPro FPAK continuation volume is missing"));
        return false;
    }

    qint64 outputLimit = -1;
    if (!XBinary::getUnpackOutputLimit(pState->mapUnpackProperties,
                                       &outputLimit) ||
        ((outputLimit >= 0) && (member.nRawSize > outputLimit)) ||
        !XBinary::isUnpackOutputSizeAllowed(pState->mapUnpackProperties,
                                            member.nRawSize)) {
        XBinary::setPdStructErrorString(
            pPdStruct, tr("Unpacked output exceeds the configured limit"));
        return false;
    }
    if (pState->spOutputBudget &&
        !pState->spOutputBudget->beginEntry(pState->nCurrentIndex,
                                             member.sFileName)) {
        if (pState->spOutputBudget->isEnforcing()) return false;
        XBinary::OUTPUT_BUDGET::noteShadowRefusal(
            pState->spOutputBudget.data());
    }
    if (pState->spOutputBudget &&
        !pState->spOutputBudget->debit(member.nRawSize)) {
        if (pState->spOutputBudget->isEnforcing()) return false;
        XBinary::OUTPUT_BUDGET::noteShadowRefusal(
            pState->spOutputBudget.data());
    }
    if (member.nCompressedSize >
        (std::numeric_limits<qint64>::max)() - member.nRawSize)
        return false;
    XBinary::UNPACK_MEMORY_RESERVATION reservation;
    if (!reservation.acquire(pState->mapUnpackProperties,
                             member.nCompressedSize + member.nRawSize))
        return false;

    QByteArray packed;
    if (!readMemberData(member, &packed, pPdStruct) || !guardedThis ||
        !guardedOutput)
        return false;
    QByteArray unpacked;
    qint64 consumed = 0;
    const bool bDecoded =
        XFpakDecoder::decode(packed, member.nMethod, member.nFlags,
                             member.nRawSize, &unpacked, &consumed, pPdStruct);
    // Teardown and cancellation are checked before any verdict on the bytes:
    // neither is evidence that the member is bad.
    if (!guardedThis || !guardedOutput) return false;
    if (!XBinary::isPdStructNotCanceled(pPdStruct)) return false;
    // Past this point the reader has recognised the member's method and flags
    // and the volume walk has validated its extent, so a stream that will not
    // decode, stops short, or decodes to the wrong bytes is damaged DATA - not
    // an unsupported codec.  Without these the caller can only fall back to the
    // generic "Cannot unpack archive member", which reads as "this build cannot
    // handle the format" and sends the reader hunting for a decoder bug.
    if (!bDecoded) {
        XBinary::setPdStructErrorString(
            pPdStruct,
            tr("FoxPro FPAK member data is corrupt: the compressed stream does "
               "not decode"));
        return false;
    }
    if (consumed != member.nPackedSize) {
        XBinary::setPdStructErrorString(
            pPdStruct, tr("FoxPro FPAK member data is corrupt: the compressed "
                          "stream ended after %1 of %2 bytes")
                           .arg(consumed)
                           .arg(member.nPackedSize));
        return false;
    }

    QBuffer crcDevice(&unpacked);
    const bool bCrcOpened = crcDevice.open(QIODevice::ReadOnly);
    const bool bCrcMatched =
        bCrcOpened && XBinary::checkCRC(&crcDevice,
                                        CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF,
                                        member.nCRC32, pPdStruct);
    crcDevice.close();
    if (!guardedThis || !guardedOutput) return false;
    if (!bCrcMatched) {
        if (bCrcOpened && XBinary::isPdStructNotCanceled(pPdStruct)) {
            XBinary::setPdStructErrorString(
                pPdStruct,
                tr("FoxPro FPAK member data is corrupt: CRC-32 mismatch"));
        }
        return false;
    }

    QBuffer stage(&unpacked);
    if (!stage.open(QIODevice::ReadOnly)) return false;
    const bool result = publishUnpackOutput(
        &stage, guardedOutput.data(), pState, pPdStruct);
    stage.close();
    return result && guardedThis && guardedOutput;
}

bool XFpakArchive::moveToNext(UNPACK_STATE *pState,
                              PDSTRUCT *pPdStruct)
{
    QPointer<XFpakArchive> guardedThis(this);
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !pState->pContext ||
        !isUnpackSourceCurrent(pState, pPdStruct) || !guardedThis ||
        !XBinary::isPdStructNotCanceled(pPdStruct))
        return false;
    UNPACK_CONTEXT *context =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    if ((pState->nNumberOfRecords != context->listMembers.count()) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords))
        return false;
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex >= pState->nNumberOfRecords) {
        pState->nCurrentOffset = context->nArchiveSize;
        return false;
    }
    pState->nCurrentOffset =
        context->listMembers.at(pState->nCurrentIndex)
            .listSegments.constFirst().nHeaderOffset;
    return true;
}

bool XFpakArchive::finishUnpack(UNPACK_STATE *pState,
                                PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState))
        return false;
    UNPACK_CONTEXT *context =
        static_cast<UNPACK_CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    pState->nCurrentOffset = 0;
    pState->nTotalSize = 0;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 0;
    pState->mapUnpackProperties.clear();
    pState->mapArchiveProperties.clear();
    delete context;
    return true;
}

QList<XBinary::FPART_PROP> XFpakArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>()
           << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE
           << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_STREAMOFFSET
           << FPART_PROP_STREAMSIZE << FPART_PROP_HANDLEMETHOD
           << FPART_PROP_COMPRESSPROPERTIES
           << FPART_PROP_REPORTEDMETHOD << FPART_PROP_RESULTCRC
           << FPART_PROP_CRC_TYPE << FPART_PROP_HEADER_OFFSET
           << FPART_PROP_HEADER_SIZE << FPART_PROP_ENCRYPTED
           << FPART_PROP_FILEMODE << FPART_PROP_ISFOLDER
           << FPART_PROP_DATETIME << FPART_PROP_MTIME;
}
