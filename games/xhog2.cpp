// Reconstruction of the format recognizer and extractor.
#include "xhog2.h"
#include <cstring>

XHOG2::XHOG2(QIODevice *device) : XGameStoreArchiveBase(device, FT_DESCENT_HOG2) {}
bool XHOG2::isValid(QIODevice *device, PDSTRUCT *progress) { XHOG2 archive(device); return archive.isValid(progress); }
XBinary *XHOG2::createInstance(QIODevice *device, bool image, XADDR address)
{ Q_UNUSED(image) Q_UNUSED(address) return new XHOG2(device); }
QString XHOG2::getFileFormatExt() { return QStringLiteral("hog"); }
QString XHOG2::getFileFormatExtsString() { return QStringLiteral("Descent 3 HOG2 archive (*.hog;*.mn3)"); }
QString XHOG2::getMIMEString() { return QStringLiteral("application/octet-stream"); }
QString XHOG2::getVersion() { return QStringLiteral("2"); }
QList<QString> XHOG2::getSearchSignatures() { return {QStringLiteral("'HOG2'")}; }

bool XHOG2::scanFormat(QList<ENTRY> *entries, qint64 *archiveEnd, PDSTRUCT *progress)
{
    QPointer<XHOG2> owner(this);
    const qint64 total = getSize();
    if (!owner || total < 68 || !isPdStructNotCanceled(progress)) return false;
    const QByteArray header = read_array_process(0, 68, progress);
    if (!owner || header.size() != 68 || std::memcmp(header.constData(), "HOG2", 4)) return false;
    const uchar *h = reinterpret_cast<const uchar *>(header.constData());
    const quint32 count = readLE32(h + 4), firstData = readLE32(h + 8);
    // The reference implementation's catalogue recognizer requires at least one entry and exact adjacency
    // of header, complete 48-byte table, and the first stored payload.
    if (!count || count > MAX_RECORDS || qint64(firstData) != 68 + qint64(count) * 48 || !rangeWithin(total, 68, qint64(count) * 48)) return false;
    QSet<QString> usedFiles, usedDirectories;
    QHash<QString,qint32> suffixes;
    QHash<QString,QString> resolvedDirectories;
    qint64 offset = firstData;
    for (quint32 index = 0; index < count; ++index) {
        if (!isPdStructNotCanceled(progress)) return false;
        const qint64 tableOffset = 68 + qint64(index) * 48;
        const QByteArray row = read_array_process(tableOffset, 48, progress);
        if (!owner || row.size() != 48) return false;
        const uchar *p = reinterpret_cast<const uchar *>(row.constData());
        const quint32 size = readLE32(p + 40), timestamp = readLE32(p + 44);
        QString name, uniqueName;
        // The original Descent 3 structure separates a 36-byte filename from
        // four flags bytes. Flags do not select a compression method.
        if (size > 0x7fffffffU || !decodeName(p, 36, false, &name) ||
            !makeUniquePath(name, &usedFiles, &usedDirectories, &suffixes, &resolvedDirectories, &uniqueName) || !rangeWithin(total, offset, size)) return false;
        if (entries) {
            ENTRY entry;
            entry.nHeaderOffset = tableOffset; entry.nHeaderSize = 48;
            entry.nDataOffset = offset; entry.nDataSize = size;
            entry.sFileName = uniqueName;
            entry.mtDateTime = QDateTime::fromSecsSinceEpoch(timestamp).toUTC();
            entries->append(entry);
        }
        offset += size;
    }
    if (offset != total || !isPdStructNotCanceled(progress)) return false;
    if (archiveEnd) *archiveEnd = offset;
    return true;
}
