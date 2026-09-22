/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xsolarispackage.h"

#include <QDateTime>

#include <cstring>
#include <new>

namespace {
// "# PaCkAgE DaTaStReAm\n".  The trailing newline is load bearing: the sibling
// datastream flavour spells "# PaCkAgE DaTaStReAm:zip\n" and shares the first
// 20 bytes, so accepting a 20-byte prefix would swallow a format this class
// cannot decode.
const char SOLPKG_MAGIC[] = "# PaCkAgE DaTaStReAm\n";
const qint64 SOLPKG_MAGIC_SIZE = 21;
// pkgtrans writes the whole descriptive header into one 512-byte block and the
// first cpio archive starts at the block boundary right after it.
const qint64 SOLPKG_BLOCK_SIZE = 512;
const qint64 SOLPKG_HEADER_BLOCK_SIZE = 512;

const qint64 SOLPKG_NEWC_HEADER_SIZE = 110;
const qint64 SOLPKG_ODC_HEADER_SIZE = 76;
const qint64 SOLPKG_BINARY_HEADER_SIZE = 26;

// The reference implementation reads at most 0x103 name bytes into a 0x104
// buffer and seeks over the remainder, and refuses a namesize of 0x801 or more
// outright.  Both limits are reproduced so a corrupt length cannot be turned
// into an unbounded read.
const qint64 SOLPKG_MAX_NAMESIZE = 0x800;
const qint64 SOLPKG_MAX_NAME_READ = 0x103;

const qint32 SOLPKG_MAX_MEMBERS = 500000;
const qint32 SOLPKG_MAX_PARTS = 8192;
const qint32 SOLPKG_MAX_RECORDS_PER_PART = 500000;

const char SOLPKG_TRAILER[] = "TRAILER!!!";

const quint32 SOLPKG_S_IFMT = 0170000U;
const quint32 SOLPKG_S_IFDIR = 0040000U;

bool solpkgParseUnsigned(const QByteArray &baField, qint32 nBase, quint64 *pnValue)
{
    // cpio pads its ASCII fields with NULs and (in odc streams written by some
    // producers) with blanks; both are accepted, but a digit outside the base
    // is a hard reject so that random binary never parses as a header.
    QByteArray baTrimmed = baField;
    baTrimmed.replace('\0', ' ');
    baTrimmed = baTrimmed.trimmed();

    if (baTrimmed.isEmpty()) {
        *pnValue = 0;
        return true;
    }

    bool bOk = false;
    const quint64 nValue = baTrimmed.toULongLong(&bOk, nBase);
    if (!bOk) return false;

    *pnValue = nValue;
    return true;
}
}  // namespace

XSolarisPackage::XSolarisPackage(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSolarisPackage::~XSolarisPackage()
{
}

bool XSolarisPackage::rangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

bool XSolarisPackage::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

XSolarisPackage::CPIO_DIALECT XSolarisPackage::classifyMagic(const char *pMagic)
{
    if (std::memcmp(pMagic, "07070", 5) == 0) {
        if (pMagic[5] == '1') return CPIO_DIALECT_NEWC;
        if (pMagic[5] == '2') return CPIO_DIALECT_CRC;
        if (pMagic[5] == '7') return CPIO_DIALECT_ODC;
        return CPIO_DIALECT_UNKNOWN;
    }

    const quint16 nWord = static_cast<quint16>(static_cast<quint8>(pMagic[0])) | (static_cast<quint16>(static_cast<quint8>(pMagic[1])) << 8);
    // 070707 octal is 0x71c7.  A stream written on the other endianness shows
    // up as the byte-swapped 0xc771.
    if (nWord == 0xc771) return CPIO_DIALECT_BINARY_BE;
    if (nWord == 0x71c7) return CPIO_DIALECT_BINARY_LE;

    return CPIO_DIALECT_UNKNOWN;
}

QString XSolarisPackage::dialectToString(CPIO_DIALECT dialect)
{
    if (dialect == CPIO_DIALECT_NEWC) return QStringLiteral("cpio newc (070701)");
    if (dialect == CPIO_DIALECT_CRC) return QStringLiteral("cpio crc (070702)");
    if (dialect == CPIO_DIALECT_ODC) return QStringLiteral("cpio odc (070707)");
    if (dialect == CPIO_DIALECT_BINARY_BE) return QStringLiteral("cpio binary (big-endian)");
    if (dialect == CPIO_DIALECT_BINARY_LE) return QStringLiteral("cpio binary (little-endian)");
    return QStringLiteral("Unknown");
}

QString XSolarisPackage::normalizeName(const QByteArray &baName, bool *pbOk)
{
    *pbOk = false;

    // The name field is NUL terminated inside its own namesize; everything past
    // the first NUL is padding.
    const int nTerminator = baName.indexOf('\0');
    QByteArray baRaw = (nTerminator >= 0) ? baName.left(nTerminator) : baName;

    for (int i = 0; i < baRaw.size(); i++) {
        const quint8 nCharacter = static_cast<quint8>(baRaw.at(i));
        if (nCharacter < 0x20) return QString();  // control bytes are never a legal path here
    }

    QString sName = QString::fromLatin1(baRaw).trimmed();

    // pkgtrans emits "CSWfoo/pkginfo" style relative names, but a hand-built
    // part can carry "./" or a leading separator; both are stripped so the
    // member lands under the output root instead of escaping it.
    while (sName.size() > 1) {
        if ((sName.at(0) == QLatin1Char('.')) && ((sName.at(1) == QLatin1Char('/')) || (sName.at(1) == QLatin1Char('\\')))) {
            sName.remove(0, 2);
        } else if ((sName.at(0) == QLatin1Char('/')) || (sName.at(0) == QLatin1Char('\\'))) {
            sName.remove(0, 1);
        } else {
            break;
        }
    }

    *pbOk = true;
    return sName;
}

bool XSolarisPackage::readRawHeader(qint64 nOffset, qint64 nInputSize, RAWHEADER *pHeader, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pHeader || !guardedSource) return false;

    if (!rangeWithin(nInputSize, nOffset, 6)) return false;

    const QByteArray baMagic = read_array_process(nOffset, 6, pPdStruct);
    if (!guardedSource || (baMagic.size() != 6)) return false;

    const CPIO_DIALECT dialect = classifyMagic(baMagic.constData());
    if (dialect == CPIO_DIALECT_UNKNOWN) return false;

    qint64 nHeaderSize = 0;
    qint64 nAlignMask = 0;

    if ((dialect == CPIO_DIALECT_NEWC) || (dialect == CPIO_DIALECT_CRC)) {
        nHeaderSize = SOLPKG_NEWC_HEADER_SIZE;
        nAlignMask = 3;
    } else if (dialect == CPIO_DIALECT_ODC) {
        nHeaderSize = SOLPKG_ODC_HEADER_SIZE;
        nAlignMask = 0;
    } else {
        nHeaderSize = SOLPKG_BINARY_HEADER_SIZE;
        nAlignMask = 1;
    }

    if (!rangeWithin(nInputSize, nOffset, nHeaderSize)) return false;

    const QByteArray baHeader = read_array_process(nOffset, nHeaderSize, pPdStruct);
    if (!guardedSource || (baHeader.size() != nHeaderSize)) return false;

    quint64 nMode = 0;
    quint64 nUID = 0;
    quint64 nGID = 0;
    quint64 nMTime = 0;
    quint64 nNameSize = 0;
    quint64 nFileSize = 0;

    if ((dialect == CPIO_DIALECT_NEWC) || (dialect == CPIO_DIALECT_CRC)) {
        // magic[6] ino[8] mode[8] uid[8] gid[8] nlink[8] mtime[8] filesize[8]
        // devmajor[8] devminor[8] rdevmajor[8] rdevminor[8] namesize[8] check[8]
        if (!solpkgParseUnsigned(baHeader.mid(14, 8), 16, &nMode)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(22, 8), 16, &nUID)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(30, 8), 16, &nGID)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(46, 8), 16, &nMTime)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(54, 8), 16, &nFileSize)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(94, 8), 16, &nNameSize)) return false;
    } else if (dialect == CPIO_DIALECT_ODC) {
        // magic[6] dev[6] ino[6] mode[6] uid[6] gid[6] nlink[6] rdev[6]
        // mtime[11] namesize[6] filesize[11]
        if (!solpkgParseUnsigned(baHeader.mid(18, 6), 8, &nMode)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(24, 6), 8, &nUID)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(30, 6), 8, &nGID)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(48, 11), 8, &nMTime)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(59, 6), 8, &nNameSize)) return false;
        if (!solpkgParseUnsigned(baHeader.mid(65, 11), 8, &nFileSize)) return false;
    } else {
        // magic dev ino mode uid gid nlink rdev mtime[2] namesize filesize[2],
        // all 16-bit; the 32-bit values are stored high half first.
        const uchar *pRaw = reinterpret_cast<const uchar *>(baHeader.constData());
        const bool bBigEndian = (dialect == CPIO_DIALECT_BINARY_BE);

        struct Reader {
            const uchar *pData;
            bool bBigEndian;
            quint32 word(int nPos) const
            {
                const quint32 nLow = pData[nPos];
                const quint32 nHigh = pData[nPos + 1];
                return bBigEndian ? ((nLow << 8) | nHigh) : ((nHigh << 8) | nLow);
            }
        };
        const Reader reader = {pRaw, bBigEndian};

        nMode = reader.word(6);
        nUID = reader.word(8);
        nGID = reader.word(10);
        nMTime = (static_cast<quint64>(reader.word(16)) << 16) | reader.word(18);
        nNameSize = reader.word(20);
        nFileSize = (static_cast<quint64>(reader.word(22)) << 16) | reader.word(24);
    }

    if (nNameSize > static_cast<quint64>(SOLPKG_MAX_NAMESIZE)) return false;
    if (nNameSize == 0) return false;
    if (nFileSize > static_cast<quint64>(nInputSize)) return false;

    qint64 nNamePos = nOffset + nHeaderSize;
    qint64 nNameRead = static_cast<qint64>(nNameSize);
    qint64 nNameSkip = 0;
    if (nNameRead > SOLPKG_MAX_NAME_READ) {
        nNameSkip = nNameRead - SOLPKG_MAX_NAME_READ;
        nNameRead = SOLPKG_MAX_NAME_READ;
    }

    if (!rangeWithin(nInputSize, nNamePos, nNameRead + nNameSkip)) return false;

    const QByteArray baName = read_array_process(nNamePos, nNameRead, pPdStruct);
    if (!guardedSource || (baName.size() != nNameRead)) return false;

    bool bNameOk = false;
    const QString sFileName = normalizeName(baName, &bNameOk);
    if (!bNameOk) return false;

    qint64 nDataOffset = nNamePos + nNameRead + nNameSkip;
    nDataOffset = (nDataOffset + nAlignMask) & ~nAlignMask;
    if (!rangeWithin(nInputSize, nDataOffset, static_cast<qint64>(nFileSize))) return false;

    pHeader->nHeaderOffset = nOffset;
    pHeader->nHeaderSize = nDataOffset - nOffset;
    pHeader->nDataOffset = nDataOffset;
    pHeader->nDataSize = static_cast<qint64>(nFileSize);
    pHeader->nAlignMask = nAlignMask;
    pHeader->nMTime = nMTime;
    pHeader->nMode = static_cast<quint32>(nMode);
    pHeader->nUID = static_cast<quint32>(nUID);
    pHeader->nGID = static_cast<quint32>(nGID);
    pHeader->dialect = dialect;
    pHeader->sFileName = sFileName;

    return true;
}

bool XSolarisPackage::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nNumberOfParts = 0;
    context.nFirstMemberOffset = 0;
    // Header block plus at least the shortest cpio header the walk can accept.
    if (context.nInputSize < (SOLPKG_HEADER_BLOCK_SIZE + SOLPKG_BINARY_HEADER_SIZE)) return false;

    const QByteArray baHeaderBlock = read_array_process(0, SOLPKG_HEADER_BLOCK_SIZE, pPdStruct);
    if (!guardedSource || (baHeaderBlock.size() != SOLPKG_HEADER_BLOCK_SIZE)) return false;
    if (std::memcmp(baHeaderBlock.constData(), SOLPKG_MAGIC, static_cast<size_t>(SOLPKG_MAGIC_SIZE)) != 0) return false;

    // "<pkgabbrev> <nparts> <nblocks>\n" - informational only.  It is published
    // for the UI but never used to bound the walk, because a datastream built
    // by hand routinely disagrees with its own counters.
    {
        const int nLineEnd = baHeaderBlock.indexOf('\n', static_cast<int>(SOLPKG_MAGIC_SIZE));
        if (nLineEnd > static_cast<int>(SOLPKG_MAGIC_SIZE)) {
            const QByteArray baLine = baHeaderBlock.mid(static_cast<int>(SOLPKG_MAGIC_SIZE), nLineEnd - static_cast<int>(SOLPKG_MAGIC_SIZE));
            const QList<QByteArray> listFields = baLine.simplified().split(' ');
            if (listFields.size() >= 1) context.sPackageAbbrev = QString::fromLatin1(listFields.at(0));
            if (listFields.size() >= 2) context.nNumberOfParts = listFields.at(1).toInt();
        }
    }

    qint64 nOffset = SOLPKG_HEADER_BLOCK_SIZE;
    qint32 nPartIndex = 0;
    bool bAnyHeader = false;

    while ((nOffset < context.nInputSize) && (nPartIndex < SOLPKG_MAX_PARTS) && isPdStructNotCanceled(pPdStruct)) {
        qint32 nRecordsInPart = 0;
        bool bTrailer = false;

        while ((nRecordsInPart < SOLPKG_MAX_RECORDS_PER_PART) && isPdStructNotCanceled(pPdStruct)) {
            if (nOffset >= context.nInputSize) break;

            RAWHEADER header = {};
            if (!readRawHeader(nOffset, context.nInputSize, &header, pPdStruct)) break;
            if (!guardedSource) return false;

            bAnyHeader = true;
            nRecordsInPart++;

            if (header.sFileName == QLatin1String(SOLPKG_TRAILER)) {
                nOffset = header.nDataOffset;
                bTrailer = true;
                break;
            }

            const bool bIsFolder = ((header.nMode & SOLPKG_S_IFMT) == SOLPKG_S_IFDIR);
            // Directory records carry no payload; emitting them would only add
            // empty files.  Everything else - including a genuinely zero-length
            // regular file - is published.
            if (!bIsFolder) {
                if (context.listMembers.size() >= SOLPKG_MAX_MEMBERS) return false;

                MEMBER member = {};
                member.nHeaderOffset = header.nHeaderOffset;
                member.nHeaderSize = header.nHeaderSize;
                member.nDataOffset = header.nDataOffset;
                member.nDataSize = header.nDataSize;
                member.nMTime = header.nMTime;
                member.nMode = header.nMode;
                member.nUID = header.nUID;
                member.nGID = header.nGID;
                member.nPart = nPartIndex;
                member.dialect = header.dialect;
                member.sFileName = header.sFileName;
                context.listMembers.append(member);
            }

            qint64 nNext = header.nDataOffset + header.nDataSize;
            nNext = (nNext + header.nAlignMask) & ~header.nAlignMask;
            // readRawHeader() has already proven the payload fits, so the only
            // way past EOF here is the tail padding of the very last record.
            if (nNext > context.nInputSize) {
                nOffset = context.nInputSize;
                break;
            }
            if (nNext <= nOffset) break;  // no forward progress: stop rather than spin
            nOffset = nNext;
        }

        nPartIndex++;

        // Without a trailer this part is the tail of a truncated stream; stop
        // here and treat everything after it as overlay.
        if (!bTrailer) break;

        // Round UP to the next 512-byte boundary; an archive that already ends
        // on one starts the next part at that same offset, so this must not be
        // written as an unconditional "advance by a block".
        nOffset = (nOffset + (SOLPKG_BLOCK_SIZE - 1)) & ~(SOLPKG_BLOCK_SIZE - 1);
        if (nOffset >= context.nInputSize) break;
    }

    if (!guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;
    // A datastream whose first cpio archive does not even start with a legal
    // cpio magic is not something this class can decode, magic or no magic.
    if (!bAnyHeader) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    if (!context.listMembers.isEmpty()) {
        context.nFirstMemberOffset = context.listMembers.first().nHeaderOffset;
    } else {
        context.nFirstMemberOffset = SOLPKG_HEADER_BLOCK_SIZE;
    }

    *pContext = context;
    return true;
}

bool XSolarisPackage::isValid(PDSTRUCT *pPdStruct)
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

bool XSolarisPackage::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSolarisPackage archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSolarisPackage::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSolarisPackage(pDevice);
}

QList<QString> XSolarisPackage::getSearchSignatures()
{
    return {QStringLiteral("'# PaCkAgE DaTaStReAm'0A")};
}

XBinary::FT XSolarisPackage::getFileType()
{
    return FT_SOLARIS_PKG;
}

XBinary::MODE XSolarisPackage::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSolarisPackage::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSolarisPackage::getArch()
{
    return QString();
}

QString XSolarisPackage::getFileFormatExt()
{
    return QStringLiteral("pkg");
}

QString XSolarisPackage::getFileFormatExtsString()
{
    return QStringLiteral("Solaris package datastream (*.pkg *.img)");
}

QString XSolarisPackage::getMIMEString()
{
    return QStringLiteral("application/x-svr4-package");
}

QString XSolarisPackage::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    if (context.sPackageAbbrev.isEmpty()) return QString();
    return context.sPackageAbbrev;
}

qint64 XSolarisPackage::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSolarisPackage::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSolarisPackage::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;

    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QList<XBinary::FPART> XSolarisPackage::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if (nFileParts & FILEPART_HEADER) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SOLPKG_HEADER_BLOCK_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    const qint32 nNumberOfMembers = context.listMembers.size();
    for (qint32 i = 0; (i < nNumberOfMembers) && isPdStructNotCanceled(pPdStruct); i++) {
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Record header");
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
            // Every member of an uncompressed datastream is verbatim cpio
            // payload, so the generic chain copies it out as-is.
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, dialectToString(member.dialect));
            part.mapProperties.insert(FPART_PROP_FILEMODE, member.nMode);
            part.mapProperties.insert(FPART_PROP_UID, member.nUID);
            part.mapProperties.insert(FPART_PROP_GID, member.nGID);
            part.mapProperties.insert(FPART_PROP_ISFOLDER, false);
            listResult.append(part);
        }

        if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nHeaderOffset;
            part.nFileSize = member.nHeaderSize + member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            listResult.append(part);
        }
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

    if ((nFileParts & FILEPART_OVERLAY) && (context.nArchiveSize < context.nInputSize) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_OVERLAY;
        part.nFileOffset = context.nArchiveSize;
        part.nFileSize = context.nInputSize - context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Overlay");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSolarisPackage::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSolarisPackage::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
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

    if (!parseContext(pContext, pPdStruct) || !guardedSource || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("SVR4/Solaris package datastream; uncompressed cpio parts"));
    if (!pContext->sPackageAbbrev.isEmpty()) {
        pState->mapArchiveProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sPackageAbbrev);
    }
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XSolarisPackage::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return ARCHIVERECORD();
    }

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, dialectToString(member.dialect));
    result.mapProperties.insert(FPART_PROP_HEADER_OFFSET, member.nHeaderOffset);
    result.mapProperties.insert(FPART_PROP_HEADER_SIZE, member.nHeaderSize);
    result.mapProperties.insert(FPART_PROP_FILEMODE, member.nMode);
    result.mapProperties.insert(FPART_PROP_UID, member.nUID);
    result.mapProperties.insert(FPART_PROP_GID, member.nGID);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
#if QT_VERSION >= QT_VERSION_CHECK(5, 8, 0)
    result.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromSecsSinceEpoch(static_cast<qint64>(member.nMTime)));
#else
    result.mapProperties.insert(FPART_PROP_DATETIME, QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(member.nMTime) * 1000));
#endif

    return result;
}

bool XSolarisPackage::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) {
        return false;
    }

    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pContext->listMembers.size()) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
    } else {
        pState->nCurrentOffset = pContext->nArchiveSize;
    }

    return (pState->nCurrentIndex < pContext->listMembers.size());
}

bool XSolarisPackage::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}
