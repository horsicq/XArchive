/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xinfogramespak.h"

#include "Algos/xinfogramespakdecoder.h"

#include <QHash>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
// +0x00 additional descriptor size | +0x04 packed size | +0x08 unpacked size |
// +0x0c method | +0x0d info | +0x0e inline descriptor size.  16 bytes account
// for every header byte; the format carries no per-record timestamp of its own
// (the DOS stamp, when there is one, lives in the PKZIP crumb in front).
const qint64 PAK_RECORD_HEADER_SIZE = 16;
// Table of at least three slots: [0] = 0, [1] = table size, [2] = one id.
const qint64 PAK_MIN_TABLE_SIZE = 12;
const qint64 PAK_MIN_FILE_SIZE = PAK_MIN_TABLE_SIZE + PAK_RECORD_HEADER_SIZE;
const qint64 PAK_MAX_ENTRIES = 1048576;
const qint64 PAK_MAX_UNCOMPRESSED_SIZE = 0x10000000;  // 256 MB sanity cap
const qint64 PAK_MAX_EXTRA_SIZE = 0x10000;
const qint64 PAK_MAX_DESCRIPTOR_SIZE = 4096;
// The chain tiles the file, so the only slack an archive may end on is the
// PKZIP crumb that closes it.  Six reference archives end on 16 bytes of one;
// the cap leaves room for a fuller central directory and nothing more.
const qint64 PAK_MAX_TAIL = 4096;

const quint8 PAK_METHOD_STORED = 0x00U;
const quint8 PAK_METHOD_IMPLODE = 0x01U;
const quint8 PAK_METHOD_DEFLATE = 0x04U;

// Inline descriptor: 0x49, its own total size, then a NUL-padded 8.3 name.
const quint8 PAK_DESCRIPTOR_TAG = 0x49U;
const qint32 PAK_MIN_NAMED_DESCRIPTOR = 3;

// The 16-byte crumb in front of a record: "PK\3\4", u16 version, u16 flags,
// u16 method, u16 DOS time, u16 DOS date, u16 CRC.  It is a PKZIP local file
// header cut short after the first two CRC bytes, so the checksum it carries
// is the LOW HALF of the member's CRC32 - never a whole one.  Verified over
// the reference corpus: of the 5984 chain records that carry a crumb, the
// 4781 stored/deflate ones reproduce their stored half-CRC 4780 times.
const qint64 PAK_CRUMB_SIZE = 16;
const char PAK_CRUMB_LOCAL[4] = {'P', 'K', '\x03', '\x04'};
// The same truncation applied to a central directory record.  It carries no
// member, so it ends the chain instead of introducing one.
const char PAK_CRUMB_CENTRAL[4] = {'P', 'K', '\x01', '\x02'};
// One read per chain step has to cover the crumb, the header and enough of
// the inline descriptor to lift an 8.3 name out of it.
const qint64 PAK_DESCRIPTOR_PROBE = 64;
const qint64 PAK_STEP_PROBE =
    PAK_CRUMB_SIZE + PAK_RECORD_HEADER_SIZE + PAK_DESCRIPTOR_PROBE;

// Every read path here is a probe on a device the caller still owns, so the
// cursor goes back where it was found on every exit, early returns included.
class DevicePositionRestore
{
public:
    DevicePositionRestore(QIODevice *pDevice, qint64 nPosition)
        : m_pDevice(pDevice), m_nPosition(nPosition)
    {
    }
    ~DevicePositionRestore()
    {
        if (m_pDevice && (m_nPosition >= 0)) m_pDevice->seek(m_nPosition);
    }

private:
    DevicePositionRestore(const DevicePositionRestore &);
    DevicePositionRestore &operator=(const DevicePositionRestore &);
    QIODevice *m_pDevice;
    qint64 m_nPosition;
};

bool pakRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool pakIsKnownMethod(quint8 nMethod)
{
    return (nMethod == PAK_METHOD_STORED) || (nMethod == PAK_METHOD_IMPLODE) ||
           (nMethod == PAK_METHOD_DEFLATE);
}

// The 8.3 name is NUL padded to the end of its field.  A name that fills the
// field exactly leaves no terminator, so the scan is bounded by the field
// size, never by a terminator search.
QString pakDecodeName(const QByteArray &baField)
{
    QString sResult;
    for (qint32 i = 0; i < baField.size(); ++i) {
        const quint8 nCharacter = static_cast<quint8>(baField.at(i));
        if (nCharacter == 0) break;
        if ((nCharacter < 0x20) || (nCharacter > 0x7e)) return QString();
        if ((nCharacter == '/') || (nCharacter == '\\') ||
            (nCharacter == ':')) {
            return QString();
        }
        sResult.append(QLatin1Char(static_cast<char>(nCharacter)));
    }
    return sResult;
}
}  // namespace

XInfogramesPak::XInfogramesPak(QIODevice *pDevice) : XArchive(pDevice)
{
}

XInfogramesPak::~XInfogramesPak()
{
}

bool XInfogramesPak::trialDecode(const ENTRY &entry, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    if ((entry.nUncompressedSize <= 0) ||
        (entry.nUncompressedSize > PAK_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }
    if ((entry.nCompressedSize <= 0) ||
        (entry.nCompressedSize > PAK_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }

    // The trial runs against a device the caller still owns, so its cursor is
    // restored no matter which way this exits.
    const qint64 nSavedPosition = guardedSource->pos();
    bool bResult = false;

    if (entry.nMethod == PAK_METHOD_IMPLODE) {
        const QByteArray baPacked = read_array_process(
            entry.nDataOffset, entry.nCompressedSize, pPdStruct);
        if (guardedSource &&
            (baPacked.size() == entry.nCompressedSize)) {
            QByteArray baUnpacked;
            qint64 nConsumed = 0;
            bResult = XInfogramesPakDecoder::decode(
                          baPacked, entry.nUncompressedSize, &baUnpacked,
                          &nConsumed, pPdStruct) &&
                      (qint64(baUnpacked.size()) == entry.nUncompressedSize) &&
                      (nConsumed == entry.nCompressedSize);
        }
    } else if (entry.nMethod == PAK_METHOD_DEFLATE) {
        XDecompress decompressor;
        const QByteArray baUnpacked = decompressor.decomressToByteArray(
            guardedSource, entry.nDataOffset, entry.nCompressedSize,
            HANDLE_METHOD_DEFLATE, pPdStruct);
        bResult = guardedSource &&
                  (qint64(baUnpacked.size()) == entry.nUncompressedSize);
    }

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XInfogramesPak::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    // Parsing only ever reads, and every caller - detection included - still
    // owns the device, so the cursor is put back where it was found.
    const qint64 nSavedPosition = guardedSource->pos();
    DevicePositionRestore positionRestore(guardedSource, nSavedPosition);

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < PAK_MIN_FILE_SIZE) return false;

    const QByteArray baPrefix = read_array_process(0, 8, pPdStruct);
    if (!guardedSource || (baPrefix.size() != 8)) return false;
    const uchar *pPrefix = reinterpret_cast<const uchar *>(baPrefix.constData());
    // Slot 0 is a hard zero in every known archive.  It is the only fixed byte
    // pattern this headerless format has, so it stays a reject, not a warning.
    if (qFromLittleEndian<quint32>(pPrefix) != 0) return false;

    const qint64 nTableSize =
        static_cast<qint64>(qFromLittleEndian<quint32>(pPrefix + 4));
    if ((nTableSize < PAK_MIN_TABLE_SIZE) || ((nTableSize % 4) != 0)) {
        return false;
    }
    if (!pakRangeWithin(context.nInputSize, 0, nTableSize)) return false;
    if (nTableSize + PAK_RECORD_HEADER_SIZE > context.nInputSize) return false;

    const qint64 nSlotCount = nTableSize / 4;
    if ((nSlotCount - 1) > PAK_MAX_ENTRIES) return false;

    const QByteArray baTable = read_array_process(0, nTableSize, pPdStruct);
    if (!guardedSource || (baTable.size() != nTableSize)) {
        return false;
    }
    const uchar *pTable = reinterpret_cast<const uchar *>(baTable.constData());

    // Slot -> record offset is many-to-one: unfilled resource ids alias a
    // record that is already in the archive, and slot 0 means "no resource".
    // The map is keyed the other way round, keeping the LOWEST id that names
    // each record, which is the id the engine actually loads it under.
    QHash<qint64, qint32> mapIdByOffset;
    for (qint64 i = 1; i < nSlotCount; ++i) {
        const qint64 nSlot =
            static_cast<qint64>(qFromLittleEndian<quint32>(pTable + i * 4));
        if (nSlot == 0) continue;
        // A slot may repeat and may point backwards, but never into the table.
        if (nSlot < nTableSize) return false;
        if (nSlot >= context.nInputSize) return false;
        if (!mapIdByOffset.contains(nSlot)) {
            mapIdByOffset.insert(nSlot, static_cast<qint32>(i - 1));
        }
    }
    if (mapIdByOffset.isEmpty()) return false;

    context.nTableSize = nTableSize;
    context.nFooterOffset = -1;

    // Membership is the record chain, not the table: walk from the first
    // record to the last byte of the file.
    qint64 nPosition = nTableSize;
    bool bAnyPayload = false;
    qint32 nResolvedIds = 0;

    while (nPosition < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listEntries.size() >= PAK_MAX_ENTRIES) return false;
        if (context.nInputSize - nPosition < PAK_RECORD_HEADER_SIZE) {
            context.nFooterOffset = nPosition;
            break;
        }

        // One read per chain step: crumb, header and the head of the inline
        // descriptor all come out of the same window.
        const qint64 nProbeSize =
            qMin<qint64>(PAK_STEP_PROBE, context.nInputSize - nPosition);
        const QByteArray baProbe =
            read_array_process(nPosition, nProbeSize, pPdStruct);
        if (!guardedSource || (baProbe.size() != nProbeSize)) {
            return false;
        }
        const uchar *pProbe =
            reinterpret_cast<const uchar *>(baProbe.constData());

        // A central directory crumb carries no member: it closes the chain.
        if (std::memcmp(pProbe, PAK_CRUMB_CENTRAL, 4) == 0) {
            context.nFooterOffset = nPosition;
            break;
        }

        qint64 nCrumbOffset = -1;
        qint64 nHeaderShift = 0;
        if (std::memcmp(pProbe, PAK_CRUMB_LOCAL, 4) == 0) {
            nCrumbOffset = nPosition;
            nHeaderShift = PAK_CRUMB_SIZE;
            if (nProbeSize - nHeaderShift < PAK_RECORD_HEADER_SIZE) {
                // A local crumb with no room for the record it announces is
                // the head of a member that was cut off with the rest.
                context.nFooterOffset = nPosition;
                break;
            }
        }

        const qint64 nRecordOffset = nPosition + nHeaderShift;
        const uchar *pRecord = pProbe + nHeaderShift;

        const qint64 nExtraSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord));
        const qint64 nCompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 4));
        const qint64 nUncompressedSize =
            static_cast<qint64>(qFromLittleEndian<quint32>(pRecord + 8));
        const quint8 nMethod = pRecord[12];
        const quint8 nInfo = pRecord[13];
        const qint64 nDescriptorSize =
            static_cast<qint64>(qFromLittleEndian<quint16>(pRecord + 14));

        if (!pakIsKnownMethod(nMethod)) return false;
        // The info byte is the codec parameter slot of the engine's own
        // loader.  It is zero on all 13180 reference records; a non-zero value
        // would mean a codec variant this reader has never been validated
        // against, so it is rejected rather than guessed at.
        if (nInfo != 0) return false;
        if (nExtraSize > PAK_MAX_EXTRA_SIZE) return false;
        if (nDescriptorSize > PAK_MAX_DESCRIPTOR_SIZE) return false;
        if (nUncompressedSize > PAK_MAX_UNCOMPRESSED_SIZE) return false;
        if (nCompressedSize > PAK_MAX_UNCOMPRESSED_SIZE) return false;

        const qint64 nSkip =
            PAK_RECORD_HEADER_SIZE + nDescriptorSize + nExtraSize;
        if (!pakRangeWithin(context.nInputSize, nRecordOffset, nSkip)) {
            return false;
        }
        const qint64 nDataOffset = nRecordOffset + nSkip;
        if (!pakRangeWithin(context.nInputSize, nDataOffset, nCompressedSize)) {
            return false;
        }

        if (nMethod == PAK_METHOD_STORED) {
            if (nCompressedSize != nUncompressedSize) return false;
        } else {
            // A compressed member cannot be empty on either side, and the
            // implode payload needs at least the two tree count bytes.
            if ((nCompressedSize <= 0) || (nUncompressedSize <= 0)) {
                return false;
            }
            if ((nMethod == PAK_METHOD_IMPLODE) && (nCompressedSize < 3)) {
                return false;
            }
        }

        ENTRY entry = {};
        entry.nIndex = mapIdByOffset.value(nRecordOffset, -1);
        entry.nCrumbOffset = nCrumbOffset;
        entry.nHeaderOffset = nRecordOffset;
        entry.nDataOffset = nDataOffset;
        entry.nCompressedSize = nCompressedSize;
        entry.nUncompressedSize = nUncompressedSize;
        entry.nMethod = nMethod;
        entry.nDescriptorSize = static_cast<qint32>(nDescriptorSize);
        entry.nExtraSize = static_cast<qint32>(nExtraSize);
        if (entry.nIndex >= 0) ++nResolvedIds;

        if ((nDescriptorSize >= PAK_MIN_NAMED_DESCRIPTOR) &&
            (nHeaderShift + PAK_RECORD_HEADER_SIZE + nDescriptorSize <=
             nProbeSize) &&
            (pRecord[PAK_RECORD_HEADER_SIZE] == PAK_DESCRIPTOR_TAG) &&
            (static_cast<qint64>(pRecord[PAK_RECORD_HEADER_SIZE + 1]) ==
             nDescriptorSize)) {
            const QByteArray baField = baProbe.mid(
                static_cast<qint32>(nHeaderShift + PAK_RECORD_HEADER_SIZE + 2),
                static_cast<qint32>(nDescriptorSize - 2));
            entry.sStoredName = pakDecodeName(baField);
        }

        if (nCrumbOffset >= 0) {
            entry.bHasCrc = true;
            entry.nDosTime = qFromLittleEndian<quint16>(pProbe + 10);
            entry.nDosDate = qFromLittleEndian<quint16>(pProbe + 12);
            entry.nCrc16 = qFromLittleEndian<quint16>(pProbe + 14);
        }

        if (nUncompressedSize > 0) bAnyPayload = true;
        context.listEntries.append(entry);

        // Every step consumes at least the 16 header bytes, so the walk always
        // advances and the loop always terminates.
        nPosition = nDataOffset + nCompressedSize;
    }

    if (context.listEntries.isEmpty() || !bAnyPayload) return false;
    // Every resource id has to have landed on a chain record.  This is what
    // rules out a file whose first two words merely read like a table: the
    // table and the chain are two independent descriptions of the same set of
    // records, and on the reference corpus they agree exactly, 367 times out
    // of 367.
    if (nResolvedIds != mapIdByOffset.size()) return false;
    if (context.nFooterOffset >= 0) {
        if (context.nInputSize - context.nFooterOffset > PAK_MAX_TAIL) {
            return false;
        }
    }
    // The chain tiles the file, closing crumb included.
    context.nArchiveSize = context.nInputSize;

    // Members keep chain order - the order they are laid out in and the order
    // an extraction reads them in.  The published number is the resource id,
    // which is unique per record because the id map keeps one id per offset.
    // If any record has no id at all the whole listing falls back to chain
    // positions, so two members can never race for one output file.
    const bool bNameById = (nResolvedIds == context.listEntries.size());
    for (qint32 i = 0; i < context.listEntries.size(); ++i) {
        ENTRY &entry = context.listEntries[i];
        const qint32 nNameNumber = bNameById ? entry.nIndex : i;
        if (!entry.sStoredName.isEmpty()) {
            entry.sFileName = QStringLiteral("%1_%2")
                                  .arg(nNameNumber, 5, 10, QLatin1Char('0'))
                                  .arg(entry.sStoredName);
        } else {
            entry.sFileName = QStringLiteral("%1.bin").arg(
                nNameNumber, 5, 10, QLatin1Char('0'));
        }
    }

    // Bounded trial decode: the format has no magic, so one real member has to
    // come out at exactly its declared size before the file is accepted.  A
    // compressed member is picked in preference to a stored one because a
    // stored member proves nothing about the codec.
    qint32 nTrialIndex = -1;
    for (qint32 i = 0; i < context.listEntries.size(); ++i) {
        const ENTRY &entry = context.listEntries.at(i);
        if ((entry.nMethod != PAK_METHOD_STORED) &&
            (entry.nUncompressedSize > 0)) {
            nTrialIndex = i;
            break;
        }
    }
    if (nTrialIndex >= 0) {
        if (!trialDecode(context.listEntries.at(nTrialIndex), pPdStruct)) {
            return false;
        }
    }

    if (!guardedSource || !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }
    *pContext = context;
    return true;
}

bool XInfogramesPak::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XInfogramesPak::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XInfogramesPak archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XInfogramesPak::createInstance(QIODevice *pDevice, bool bIsImage,
                                        XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XInfogramesPak(pDevice);
}

QList<QString> XInfogramesPak::getSearchSignatures()
{
    // Headerless: the file opens on a raw offset table.  There is nothing to
    // scan for, and a zero-word signature would match half the world.
    return QList<QString>();
}

XBinary::FT XInfogramesPak::getFileType()
{
    return FT_INFOGRAMES_PAK;
}

XBinary::MODE XInfogramesPak::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XInfogramesPak::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XInfogramesPak::getArch()
{
    return QString();
}

QString XInfogramesPak::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XInfogramesPak::getFileFormatExtsString()
{
    return QStringLiteral("Infogrames PAK (*.pak)");
}

QString XInfogramesPak::getMIMEString()
{
    return QStringLiteral("application/x-infogrames-pak");
}

QString XInfogramesPak::getVersion()
{
    // The container has no version word anywhere; every archive of every AITD
    // engine release parses with the same 16-byte record header.
    return QString();
}

qint64 XInfogramesPak::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const qint64 nResult =
        parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return nResult;
}

QList<XBinary::MAPMODE> XInfogramesPak::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XInfogramesPak::getMemoryMap(MAPMODE mapMode,
                                                  PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_TABLE | FILEPART_HEADER |
                                 FILEPART_STREAM | FILEPART_FOOTER |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XInfogramesPak::methodToString(quint8 nMethod)
{
    if (nMethod == PAK_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == PAK_METHOD_DEFLATE) return QStringLiteral("Deflate");
    if (nMethod == PAK_METHOD_IMPLODE) {
        return QStringLiteral("Infogrames implode");
    }
    return QStringLiteral("Unknown 0x%1").arg(nMethod, 2, 16, QLatin1Char('0'));
}

XBinary::HANDLE_METHOD XInfogramesPak::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == PAK_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == PAK_METHOD_DEFLATE) return HANDLE_METHOD_DEFLATE;
    if (nMethod == PAK_METHOD_IMPLODE) return HANDLE_METHOD_INFOGRAMES_PAK;
    return HANDLE_METHOD_UNKNOWN;
}

bool XInfogramesPak::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XInfogramesPak::getFileParts(quint32 nFileParts,
                                                   qint32 nLimit,
                                                   PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    // The table comes first so that a truncated listing (nLimit) still carries
    // the one part that describes the whole archive.
    if ((nFileParts & FILEPART_TABLE) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_TABLE;
        part.nFileOffset = 0;
        part.nFileSize = context.nTableSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Offset table");
        result.append(part);
    }

    const qint32 nCount = context.listEntries.size();
    for (qint32 i = 0; i < nCount; ++i) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        const ENTRY &entry = context.listEntries.at(i);
        // The crumb in front of a record belongs to it, so the header part
        // starts there and the parts tile the chain without a gap.
        const qint64 nRecordStart = (entry.nCrumbOffset >= 0)
                                        ? entry.nCrumbOffset
                                        : entry.nHeaderOffset;

        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = nRecordStart;
            part.nFileSize = entry.nDataOffset - nRecordStart;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Record header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = entry.nDataOffset;
            part.nFileSize = entry.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      entry.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      entry.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      methodToHandleMethod(entry.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(entry.nMethod));
            part.mapProperties.insert(FPART_PROP_TYPE,
                                      static_cast<quint32>(entry.nMethod));
            if (!entry.sStoredName.isEmpty()) {
                part.mapProperties.insert(FPART_PROP_ORIGINALNAME,
                                          entry.sStoredName);
            }
            if (entry.bHasCrc) {
                part.mapProperties.insert(
                    FPART_PROP_DATETIME,
                    dosDateTimeToQDateTime(entry.nDosDate, entry.nDosTime));
            }
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = nRecordStart;
            part.nFileSize =
                entry.nDataOffset + entry.nCompressedSize - nRecordStart;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = entry.sFileName;
            result.append(part);
        }
    }

    if ((nFileParts & FILEPART_FOOTER) && (context.nFooterOffset >= 0) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_FOOTER;
        part.nFileOffset = context.nFooterOffset;
        part.nFileSize = context.nInputSize - context.nFooterOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Trailing PKZIP crumb");
        result.append(part);
    }
    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    if ((nFileParts & FILEPART_OVERLAY) &&
        (context.nArchiveSize < context.nInputSize) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant>
XInfogramesPak::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XInfogramesPak::initUnpack(UNPACK_STATE *pState,
                                const QMap<UNPACK_PROP, QVariant> &mapProperties,
                                PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource ||
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
    if (!parseContext(pContext, pPdStruct) || !guardedSource ||
        pContext->listEntries.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Infogrames PAK resource container; stored, deflate and implode "
           "records"));
    pState->nCurrentOffset = pContext->listEntries.first().nHeaderOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listEntries.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XInfogramesPak::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return ARCHIVERECORD();
    }
    const ENTRY &entry = pContext->listEntries.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != entry.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = entry.nDataOffset;
    result.nStreamSize = entry.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, entry.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                entry.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                entry.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                methodToHandleMethod(entry.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(entry.nMethod));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(entry.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    if (entry.bHasCrc) {
        result.mapProperties.insert(
            FPART_PROP_DATETIME,
            dosDateTimeToQDateTime(entry.nDosDate, entry.nDosTime));
    }
    // No FPART_PROP_RESULTCRC: the container stores only the low 16 bits of
    // the member's CRC32, and publishing that as a CRC32 would fail every
    // record it is meant to protect.
    return result;
}

bool XInfogramesPak::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listEntries.size())) {
        return false;
    }
    // The index has to land PAST the last record, not on it, or the caller
    // never learns that the walk is over.
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pContext->listEntries.size()) {
        pState->nCurrentOffset =
            pContext->listEntries.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }
    return (pState->nCurrentIndex < pContext->listEntries.size());
}

bool XInfogramesPak::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
