/*
 * DiscJuggler CDI track reader.  The field layout is based on the public
 * No$cash CDI description and cross-checked against the independent cditools
 * implementation.  No third-party source code is included here.
 */
#include "xdiskjugglerarchive.h"

#include <QtEndian>
#include <QHash>
#include <QPointer>
#include <QSet>

#include <cstring>
#include <limits>

namespace {
const qint64 MAX_CDI_HEADER_SIZE = Q_INT64_C(16) * 1024 * 1024;
const qint64 MAX_CDI_IMAGE_SIZE = Q_INT64_C(2) * 1024 * 1024 * 1024;
const uchar CDI_MARK[10] = {0, 0, 1, 0, 0, 0, 0xff, 0xff, 0xff, 0xff};

bool within(qint64 total, qint64 offset, qint64 size)
{
    return offset >= 0 && size >= 0 && offset <= total &&
           size <= total - offset;
}
}  // namespace

struct XDiskJugglerArchive::ENTRY_CONTEXT
{
    qint64 nHeaderPosition;
    QSet<QString> *pUsedFiles;
    QSet<QString> *pUsedDirectories;
    QHash<QString, qint32> *pNextSuffixes;
    QHash<QString, QString> *pResolvedDirectories;
    QList<ENTRY> *pEntries;
};

XDiskJugglerArchive::XDiskJugglerArchive(QIODevice *pDevice)
    : XGameStoreArchiveBase(pDevice, FT_DISKJUGGLER_CDI)
{
}

bool XDiskJugglerArchive::isValid(QIODevice *pDevice,
                                  PDSTRUCT *pPdStruct)
{
    XDiskJugglerArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XDiskJugglerArchive::createInstance(QIODevice *pDevice,
                                             bool bIsImage,
                                             XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XDiskJugglerArchive(pDevice);
}

bool XDiskJugglerArchive::appendEntry(ENTRY_CONTEXT *pContext,
                                      qint64 nOffset, qint64 nPacked,
                                      qint64 nRaw, HANDLE_METHOD method,
                                      const QString &sSourceName,
                                      qint64 nDescriptor)
{
    if (!pContext || !pContext->pEntries ||
        !rangeWithin(pContext->nHeaderPosition, nOffset, nPacked)) return false;
    QString sName;
    if (!makeUniquePath(sSourceName, pContext->pUsedFiles,
                        pContext->pUsedDirectories,
                        pContext->pNextSuffixes,
                        pContext->pResolvedDirectories, &sName)) return false;
    ENTRY entry = {};
    entry.nHeaderOffset = pContext->nHeaderPosition + nDescriptor;
    entry.nHeaderSize = 20;
    entry.nDataOffset = nOffset;
    entry.nDataSize = nPacked;
    entry.nUncompressedSize = nRaw;
    entry.handleMethod = method;
    entry.sFileName = sName;
    pContext->pEntries->append(entry);
    return pContext->pEntries->size() <= MAX_RECORDS;
}

bool XDiskJugglerArchive::scanFormat(QList<ENTRY> *pEntries,
                                     qint64 *pArchiveEnd,
                                     PDSTRUCT *pPdStruct)
{
    QPointer<XDiskJugglerArchive> guardedThis(this);
    const qint64 total = getSize();
    if (!guardedThis || total < 16 || total > MAX_CDI_IMAGE_SIZE ||
        !isPdStructNotCanceled(pPdStruct)) return false;

    const QByteArray footer = read_array_process(total - 8, 8, pPdStruct);
    if (!guardedThis || footer.size() != 8) return false;
    const uchar *pf = reinterpret_cast<const uchar *>(footer.constData());
    const quint32 version = qFromLittleEndian<quint32>(pf);
    const quint32 headerOffset = qFromLittleEndian<quint32>(pf + 4);
    if ((version != 0x80000004U) && (version != 0x80000005U) &&
        (version != 0x80000006U)) return false;

    const qint64 headerPos = (version == 0x80000006U)
                                 ? total - qint64(headerOffset)
                                 : qint64(headerOffset);
    if (headerPos <= 0 || headerPos >= total - 8) return false;
    const qint64 headerSize = total - headerPos;
    if (headerSize < 16 || headerSize > MAX_CDI_HEADER_SIZE ||
        headerSize > (std::numeric_limits<qint32>::max)()) return false;

    const QByteArray header = read_array_process(headerPos, headerSize,
                                                 pPdStruct);
    if (!guardedThis || header.size() != headerSize) return false;
    const uchar *p = reinterpret_cast<const uchar *>(header.constData());
    const qint32 sessions = qFromLittleEndian<quint16>(p);
    if (sessions < 1 || sessions > 99) return false;

    struct TRACK {
        qint32 session = -1;
        qint32 number = -1;
        qint32 mode = -1;
        qint32 sectorSize = 0;
        qint64 pregap = 0;
        qint64 sectors = 0;
        qint64 descriptor = 0;
    };
    QList<TRACK> tracks;

    // Every track block contains two adjacent ten-byte sentinels.  Some CDI
    // producers use a shorter legacy block tail than the nominal E4h layout,
    // so locate the authenticated pair and validate all fields and the final
    // data extent instead of stepping by a producer-specific block size.
    for (qint64 first = 0; first <= headerSize - 20; ++first) {
        if ((first & 0x3fff) == 0 &&
            !isPdStructNotCanceled(pPdStruct)) return false;
        if (std::memcmp(p + first, CDI_MARK, 10) != 0 ||
            std::memcmp(p + first + 10, CDI_MARK, 10) != 0) continue;

        const qint64 th = first + 8;
        if (!within(headerSize, th + 0x10, 1)) continue;
        const qint32 fileNameLength = p[th + 0x10];
        const qint64 indexCountPos = th + 0x30 + fileNameLength;
        if (!within(headerSize, indexCountPos, 2)) continue;
        const qint32 indexCount =
            qFromLittleEndian<quint16>(p + indexCountPos);
        if (indexCount < 1 || indexCount > 64) continue;
        const qint64 indicesSize = qint64(indexCount) * 4;
        if (!within(headerSize, indexCountPos + 2, indicesSize + 4))
            continue;
        const quint32 cdTextCount = qFromLittleEndian<quint32>(
            p + indexCountPos + 2 + indicesSize);
        if (cdTextCount > 4096) continue;
        const qint64 cdTextSize = qint64(cdTextCount) * 18;
        const qint64 fixed = th + 0x36 + fileNameLength + indicesSize +
                             cdTextSize;
        if (!within(headerSize, fixed, 0x2e)) continue;

        const qint32 mode = p[fixed + 2];
        const quint32 readMode =
            qFromLittleEndian<quint32>(p + fixed + 0x2a);
        if (mode < 0 || mode > 2 || readMode > 2) continue;
        const qint32 session = qFromLittleEndian<quint32>(p + fixed + 0x0a);
        const qint32 number = qFromLittleEndian<quint32>(p + fixed + 0x0e);
        if (session < 0 || session >= sessions || number < 0 ||
            number > 999) continue;

        const qint64 pregap = qFromLittleEndian<quint32>(
            p + indexCountPos + 2);
        qint64 sectors = 0;
        if (indexCount >= 2) {
            sectors = qFromLittleEndian<quint32>(p + indexCountPos + 6);
        } else {
            sectors = qFromLittleEndian<quint32>(p + fixed + 0x16);
        }
        if (sectors <= 0 || sectors > 1000000 || pregap > 1000000)
            continue;

        TRACK track;
        track.session = session;
        track.number = number;
        track.mode = mode;
        track.sectorSize = readMode == 0 ? 2048
                              : (readMode == 1 ? 2336 : 2352);
        track.pregap = pregap;
        track.sectors = sectors;
        track.descriptor = first;
        tracks.append(track);
        first += 19;
    }
    if (tracks.isEmpty() || tracks.size() > MAX_RECORDS) return false;

    qint32 expectedSession = 0;
    qint32 expectedTrack = 0;
    qint64 dataOffset = 0;
    QList<ENTRY> entries;
    QSet<QString> usedFiles;
    QSet<QString> usedDirs;
    QHash<QString, qint32> nextSuffixes;
    QHash<QString, QString> resolvedDirs;
    ENTRY_CONTEXT entryContext = {headerPos, &usedFiles, &usedDirs,
                                  &nextSuffixes, &resolvedDirs, &entries};

    for (const TRACK &track : tracks) {
        if (track.session == expectedSession + 1 && track.number == 0) {
            ++expectedSession;
            expectedTrack = 0;
        }
        if (track.session != expectedSession || track.number != expectedTrack)
            return false;
        ++expectedTrack;

        const qint64 pregapBytes = track.pregap * track.sectorSize;
        const qint64 packedBytes = track.sectors * track.sectorSize;
        if (track.pregap &&
            !appendEntry(&entryContext, dataOffset, pregapBytes, pregapBytes,
                    HANDLE_METHOD_STORE,
                    QStringLiteral("session_%1/track_%2.pregap.bin")
                        .arg(track.session + 1, 2, 10, QLatin1Char('0'))
                        .arg(track.number + 1, 2, 10, QLatin1Char('0')),
                    track.descriptor)) return false;
        dataOffset += pregapBytes;

        HANDLE_METHOD method = HANDLE_METHOD_STORE;
        qint64 rawBytes = packedBytes;
        QString extension = QStringLiteral("bin");
        if (track.mode != 0) {
            extension = QStringLiteral("iso");
            rawBytes = track.sectors * 2048;
            if (track.sectorSize == 2336) {
                method = HANDLE_METHOD_CDI_2336;
            } else if (track.sectorSize == 2352) {
                method = track.mode == 1 ? HANDLE_METHOD_CDI_MODE1_2352
                                         : HANDLE_METHOD_CDI_MODE2_2352;
            }
        }
        if (!appendEntry(&entryContext, dataOffset, packedBytes, rawBytes, method,
                    QStringLiteral("session_%1/track_%2.%3")
                        .arg(track.session + 1, 2, 10, QLatin1Char('0'))
                        .arg(track.number + 1, 2, 10, QLatin1Char('0'))
                        .arg(extension),
                    track.descriptor)) return false;
        dataOffset += packedBytes;
    }

    // DiscJuggler can retain one trailing, empty session descriptor after a
    // multisession write. It owns no sectors and therefore has no track block.
    const qint32 representedSessions = expectedSession + 1;
    if ((sessions < representedSessions) ||
        (sessions > representedSessions + 1) || dataOffset != headerPos ||
        entries.isEmpty() || !isPdStructNotCanceled(pPdStruct)) return false;
    if (pEntries) *pEntries = entries;
    if (pArchiveEnd) *pArchiveEnd = total;
    return true;
}
