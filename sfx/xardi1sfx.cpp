/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xardi1sfx.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <limits>
#include <new>

#include "subdevice.h"
#include "xdeflatedecoder.h"

namespace {
// The 51-byte EOF trailer.  Only the four year digits move, so the constant
// prefix and suffix are matched literally and the digits are range-checked.
const qint64 ARDI1_TRAILER_SIZE = 51;
const qint64 ARDI1_TAIL_TEXT_SIZE = 31;
const char ARDI1_TAIL_PREFIX[] = {'A', 'R', 'D', 'I', '-', '(', 'C', ')', '1', '9', '9', '1', '-'};
const qint64 ARDI1_TAIL_PREFIX_SIZE = 13;
const char ARDI1_TAIL_SUFFIX[] = {'-', 'D', 'a', 'n', 'i', 'e', 'l', ' ', 'V', 'a', 'l', 'o', 't', '\x00'};
const qint64 ARDI1_TAIL_SUFFIX_SIZE = 14;

const qint64 ARDI1_RECORD_SIZE = 0x33;
const qint64 ARDI1_OFF_BYTESPERSECTOR = 0x00;
const qint64 ARDI1_OFF_MEDIA = 0x0A;
const qint64 ARDI1_OFF_TOTALSECTORS = 0x08;
const qint64 ARDI1_OFF_SECTORSPERTRACK = 0x0D;
const qint64 ARDI1_OFF_HEADS = 0x0F;
const qint64 ARDI1_OFF_SIGNATURE = 0x24;
const qint64 ARDI1_OFF_COMPRESSEDSIZE = 0x26;
const qint64 ARDI1_OFF_CONSTANT = 0x2A;
const qint64 ARDI1_OFF_ZERO = 0x2E;
const qint64 ARDI1_OFF_CRC32 = 0x2F;

const quint16 ARDI1_SIGNATURE = 0x55AA;
const quint16 ARDI1_BYTES_PER_SECTOR = 512;
const quint32 ARDI1_CONSTANT = 0x00000485;

// The five bytes at +0x2A..+0x2E, used as the search needle.  The 0x55AA word
// two bytes earlier would be far commoner and the compressed size between them
// varies, so this run is the longest fixed one the header has.
const char ARDI1_NEEDLE[] = {'\x85', '\x04', '\x00', '\x00', '\x00'};
const qint64 ARDI1_NEEDLE_SIZE = 5;

// A stray 0x00000485 inside the compressed bytes costs one arithmetic test, so
// the cap only exists to keep a hostile file from turning that into a long
// scan.  No real carrier needs more than the first candidate.
const qint32 ARDI1_MAX_CANDIDATES = 64;

// The prologue is 4,563 or 4,570 bytes on every known build.  The ceiling is
// far above that and exists so a corrupt stream cannot make the measuring pass
// inflate the whole image.
const qint64 ARDI1_MAX_PROLOGUE = 0x10000;
const qint32 ARDI1_MAX_PROLOGUE_RECORDS = 64;
const quint8 ARDI1_PROLOGUE_END = 0xFF;

// The builder's free-text description of the image: [quint32 count][count
// NUL-terminated strings].  Only the first is published; the message table
// names the three of them "Label text", "Message to user text" and "Technical
// data text", matching the stub's /PL, /PM and /PR print switches.
const quint8 ARDI1_PROLOGUE_TAG_TEXT = 0x02;
const qint32 ARDI1_MAX_LABEL_CHARS = 200;

// Collects the inflated prefix and then refuses to grow, which stops the
// decoder instead of letting it inflate the whole diskette image just to find
// out where the prologue ended.
class ARDI1PrologueDevice : public QIODevice {
public:
    ARDI1PrologueDevice() : m_bCapped(false)
    {
    }

    const QByteArray &data() const
    {
        return m_baData;
    }

    bool isCapped() const
    {
        return m_bCapped;
    }

    bool isSequential() const override
    {
        return true;
    }

protected:
    qint64 readData(char *pData, qint64 nMaxSize) override
    {
        Q_UNUSED(pData)
        Q_UNUSED(nMaxSize)
        return -1;
    }

    qint64 writeData(const char *pData, qint64 nMaxSize) override
    {
        if ((nMaxSize < 0) || ((nMaxSize > 0) && !pData)) return -1;
        // The decoder writes in chunks of XBinary::getBufferSize(), which is
        // 0x4000 by default but which a caller may raise to 512 KiB through
        // XOptions ID_FEATURE_READBUFFERSIZE.  Refusing an over-cap chunk
        // WHOLESALE would leave this buffer empty at any of those settings and
        // the prologue unmeasurable, so keep the prefix that fits and only then
        // stop the decoder by failing the write.
        const qint64 nRoom = ARDI1_MAX_PROLOGUE - static_cast<qint64>(m_baData.size());
        if (nMaxSize > nRoom) {
            if (nRoom > 0) m_baData.append(pData, static_cast<int>(nRoom));
            m_bCapped = true;
            return -1;
        }
        m_baData.append(pData, static_cast<int>(nMaxSize));
        return nMaxSize;
    }

private:
    QByteArray m_baData;
    bool m_bCapped;
};
}  // namespace

XARDI1SFX::XARDI1SFX(QIODevice *pDevice) : XArchive(pDevice)
{
}

XARDI1SFX::~XARDI1SFX()
{
}

// A candidate is only the record header when the record it describes ends on
// the exact byte the EOF trailer starts on.  That equality, not the format
// constant, is what makes the predicate safe: it ties the located header to
// the one position-fixed marker the file has.
bool XARDI1SFX::acceptRecordAt(qint64 nRecordOffset, qint64 nStreamEnd, HEADER *pHeader, PDSTRUCT *pPdStruct)
{
    QPointer<XARDI1SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pHeader || !guardedSource) return false;
    if ((nRecordOffset < 0) || (nRecordOffset > nStreamEnd - ARDI1_RECORD_SIZE)) return false;

    const QByteArray baRecord = read_array_process(nRecordOffset, ARDI1_RECORD_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baRecord.size() != ARDI1_RECORD_SIZE)) return false;

    const uchar *pData = reinterpret_cast<const uchar *>(baRecord.constData());

    // The reference implementation's own predicate, field for field.
    if (qFromLittleEndian<quint16>(pData + ARDI1_OFF_SIGNATURE) != ARDI1_SIGNATURE) return false;
    if (qFromLittleEndian<quint32>(pData + ARDI1_OFF_CONSTANT) != ARDI1_CONSTANT) return false;
    if (pData[ARDI1_OFF_ZERO] != 0) return false;
    if (qFromLittleEndian<quint16>(pData + ARDI1_OFF_BYTESPERSECTOR) != ARDI1_BYTES_PER_SECTOR) return false;

    // Only "not zero" is required, which is also all the reference requires.
    // The field is a quint16, so the image it describes is at most 32 MiB and
    // needs no separate ceiling; inventing one (say the 5,760 sectors of a
    // 2.88 MiB diskette) would narrow the reader on no evidence, since every
    // carrier measured reports 2,880 and the format's own text speaks of an
    // "Image for a %lu bytes diskette" without naming a limit.
    const quint16 nTotalSectors = qFromLittleEndian<quint16>(pData + ARDI1_OFF_TOTALSECTORS);
    if (nTotalSectors == 0) return false;

    const quint32 nCompressedSize = qFromLittleEndian<quint32>(pData + ARDI1_OFF_COMPRESSEDSIZE);
    if (nCompressedSize == 0) return false;
    if (static_cast<qint64>(nCompressedSize) > std::numeric_limits<qint64>::max() - (nRecordOffset + ARDI1_RECORD_SIZE)) return false;
    if (nRecordOffset + ARDI1_RECORD_SIZE + static_cast<qint64>(nCompressedSize) != nStreamEnd) return false;

    HEADER header = {};
    header.nRecordOffset = nRecordOffset;
    header.nBytesPerSector = ARDI1_BYTES_PER_SECTOR;
    header.nTotalSectors = nTotalSectors;
    header.nMediaDescriptor = pData[ARDI1_OFF_MEDIA];
    header.nSectorsPerTrack = qFromLittleEndian<quint16>(pData + ARDI1_OFF_SECTORSPERTRACK);
    header.nNumberOfHeads = qFromLittleEndian<quint16>(pData + ARDI1_OFF_HEADS);
    header.nCompressedSize = static_cast<qint64>(nCompressedSize);
    header.nImageCRC32 = qFromLittleEndian<quint32>(pData + ARDI1_OFF_CRC32);
    header.nStreamOffset = nRecordOffset + ARDI1_RECORD_SIZE;
    header.nImageSize = static_cast<qint64>(nTotalSectors) * static_cast<qint64>(ARDI1_BYTES_PER_SECTOR);

    *pHeader = header;
    return true;
}

bool XARDI1SFX::readHeader(HEADER *pHeader, PDSTRUCT *pPdStruct)
{
    QPointer<XARDI1SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pHeader || !guardedSource || guardedSource->isSequential()) return false;

    const qint64 nInputSize = guardedSource->size();
    if (nInputSize < ARDI1_TRAILER_SIZE + ARDI1_RECORD_SIZE + 1) return false;

    // Gate on the EOF trailer first.  It is exact and costs one short read, so
    // nothing else in this reader ever runs on a file that is not an ARDI
    // diskette self-extractor.
    const QByteArray baTail = read_array_process(nInputSize - ARDI1_TAIL_TEXT_SIZE, ARDI1_TAIL_TEXT_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baTail.size() != ARDI1_TAIL_TEXT_SIZE)) return false;
    if (memcmp(baTail.constData(), ARDI1_TAIL_PREFIX, ARDI1_TAIL_PREFIX_SIZE) != 0) return false;
    if (memcmp(baTail.constData() + ARDI1_TAIL_PREFIX_SIZE + 4, ARDI1_TAIL_SUFFIX, ARDI1_TAIL_SUFFIX_SIZE) != 0) return false;

    QString sYear;
    for (qint32 i = 0; i < 4; i++) {
        const char cDigit = baTail.at(static_cast<int>(ARDI1_TAIL_PREFIX_SIZE + i));
        if ((cDigit < '0') || (cDigit > '9')) return false;
        sYear.append(QChar::fromLatin1(cDigit));
    }

    const qint64 nStreamEnd = nInputSize - ARDI1_TRAILER_SIZE;
    if (nStreamEnd <= ARDI1_RECORD_SIZE) return false;

    qint64 nSearchOffset = 0;
    qint32 nAttempt = 0;

    while ((nAttempt < ARDI1_MAX_CANDIDATES) && (nSearchOffset + ARDI1_NEEDLE_SIZE <= nStreamEnd) && isPdStructNotCanceled(pPdStruct)) {
        const qint64 nFound = find_array(nSearchOffset, nStreamEnd - nSearchOffset, ARDI1_NEEDLE, ARDI1_NEEDLE_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (nFound < 0) return false;

        nAttempt++;
        nSearchOffset = nFound + 1;

        HEADER header = {};
        if (acceptRecordAt(nFound - ARDI1_OFF_CONSTANT, nStreamEnd, &header, pPdStruct)) {
            header.sTrailerYear = sYear;
            *pHeader = header;
            return true;
        }
        if (!guardedThis || !guardedSource) return false;
    }

    return false;
}

// The prologue length is not recorded anywhere, so it is measured by inflating
// a bounded prefix of the stream and walking the tagged records until the 0xFF
// terminator.  The output device refuses to grow past the cap, which aborts
// the decoder rather than letting it inflate the whole diskette image; the
// decoder therefore reports failure on every real carrier and the buffer, not
// its return value, is what says whether the walk succeeded.
//
// The same pass collects the container's own description text (record tag
// 0x02) so that nothing the container states about the image is dropped.
bool XARDI1SFX::measurePrologue(const HEADER &header, qint64 *pnPrologueSize, QString *psLabel, PDSTRUCT *pPdStruct)
{
    QPointer<XARDI1SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pnPrologueSize || !guardedSource) return false;

    SubDevice subDevice(guardedSource.data(), header.nStreamOffset, header.nCompressedSize);
    ARDI1PrologueDevice prologueDevice;
    if (!subDevice.open(QIODevice::ReadOnly)) return false;
    if (!prologueDevice.open(QIODevice::WriteOnly)) {
        subDevice.close();
        return false;
    }

    XBinary::DATAPROCESS_STATE state = {};
    state.mapProperties.insert(XBinary::FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
    state.pDeviceInput = &subDevice;
    state.pDeviceOutput = &prologueDevice;
    state.nInputOffset = 0;
    state.nInputLimit = header.nCompressedSize;
    state.nProcessedOffset = 0;
    state.nProcessedLimit = -1;

    const bool bDecoded = XDeflateDecoder::decompress(&state, pPdStruct);

    prologueDevice.close();
    subDevice.close();
    if (!guardedThis || !guardedSource) return false;

    // Failure is expected and means nothing on its own -- it is how the cap
    // stops the decoder.  Failure WITHOUT the cap having fired is a real
    // decode error and is refused.
    if (!bDecoded && !prologueDevice.isCapped()) return false;

    const QByteArray baPrologue = prologueDevice.data();
    const uchar *pData = reinterpret_cast<const uchar *>(baPrologue.constData());
    const qint64 nAvailable = static_cast<qint64>(baPrologue.size());

    qint64 nPosition = 0;
    for (qint32 i = 0; i < ARDI1_MAX_PROLOGUE_RECORDS; i++) {
        if (nPosition + 1 > nAvailable) return false;
        const quint8 nTag = pData[nPosition];
        nPosition++;
        if (nTag == ARDI1_PROLOGUE_END) {
            *pnPrologueSize = nPosition;
            return true;
        }
        if (nPosition + 4 > nAvailable) return false;
        const quint32 nLength = qFromLittleEndian<quint32>(pData + nPosition);
        nPosition += 4;
        if (static_cast<qint64>(nLength) > nAvailable - nPosition) return false;

        if ((nTag == ARDI1_PROLOGUE_TAG_TEXT) && psLabel && psLabel->isEmpty() && (nLength > 4)) {
            const qint64 nTextBase = nPosition + 4;
            qint64 nZero = nTextBase;
            while ((nZero < nPosition + static_cast<qint64>(nLength)) && (pData[nZero] != 0)) {
                nZero++;
            }
            if (nZero > nTextBase) {
                QString sText = QString::fromLatin1(baPrologue.constData() + nTextBase, static_cast<int>(nZero - nTextBase));
                sText.replace(QChar::fromLatin1('\r'), QChar::fromLatin1(' '));
                sText.replace(QChar::fromLatin1('\n'), QChar::fromLatin1(' '));
                sText = sText.simplified();
                if (sText.size() > ARDI1_MAX_LABEL_CHARS) sText = sText.left(ARDI1_MAX_LABEL_CHARS);
                *psLabel = sText;
            }
        }

        nPosition += static_cast<qint64>(nLength);
    }

    return false;
}

bool XARDI1SFX::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XARDI1SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nPrologueSize = -1;
    if (!readHeader(&context.header, pPdStruct) || !guardedThis || !guardedSource) return false;

    context.nTotalSize = context.nInputSize;

    // The stub writes sectors to a physical diskette by default and takes the
    // file name from the user in its "/D=filename.img" mode, so the container
    // carries neither a name nor a destination path.  The carrier's own base
    // name plus ".img" is what the reference implementation publishes and what
    // the format's own CLI calls the file; it is a DERIVED name, and the two
    // strings the container does hold (the tag-0x02 label text and the image's
    // FAT12 volume label) both repeat across carriers, so neither can be used
    // instead.  See the header comment for the measured collisions.
    QString sBaseName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    if (sBaseName.isEmpty()) sBaseName = QStringLiteral("disk");
    context.sImageName = sBaseName + QStringLiteral(".img");

    qint64 nPrologueSize = -1;
    QString sLabel;
    if (measurePrologue(context.header, &nPrologueSize, &sLabel, pPdStruct) && guardedThis && guardedSource) {
        if ((nPrologueSize >= 0) && (nPrologueSize <= std::numeric_limits<qint64>::max() - context.header.nImageSize)) {
            context.nPrologueSize = nPrologueSize;
            context.nBlockSize = nPrologueSize + context.header.nImageSize;
            context.sLabel = sLabel;
        }
    }
    if (!guardedThis || !guardedSource) return false;

    *pContext = context;
    return true;
}

// The one member.  The whole deflate stream is a single solid block whose
// leading bytes are the stub's own message table; the image is the window that
// follows, so the record is published exactly the way a 7z solid substream is.
XBinary::ARCHIVERECORD XARDI1SFX::imageRecord(const CONTEXT &context)
{
    ARCHIVERECORD result = {};
    result.nStreamOffset = context.header.nStreamOffset;
    result.nStreamSize = context.header.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sImageName);
    result.mapProperties.insert(FPART_PROP_STREAMOFFSET, context.header.nStreamOffset);
    result.mapProperties.insert(FPART_PROP_STREAMSIZE, context.header.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.header.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.header.nImageSize);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    result.mapProperties.insert(FPART_PROP_EXT, QStringLiteral("img"));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate"));
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
    // The header CRC-32 covers the diskette image only, which is precisely the
    // window published here, so it authenticates this member.
    result.mapProperties.insert(FPART_PROP_CRC_TYPE, CRC_TYPE_FFFFFFFF_EDB88320_FFFFFFFFF);
    result.mapProperties.insert(FPART_PROP_RESULTCRC, context.header.nImageCRC32);
    // The container's own description of the image.  It is a label, not a name
    // and not a path -- it repeats across carriers -- so it is published as
    // information and nothing is derived from it.
    if (!context.sLabel.isEmpty()) {
        result.mapProperties.insert(FPART_PROP_INFO, context.sLabel);
    }

    if (context.nPrologueSize >= 0) {
        result.mapProperties.insert(FPART_PROP_ISSOLID, true);
        result.mapProperties.insert(FPART_PROP_SOLIDFOLDERINDEX, (qint64)0);
        result.mapProperties.insert(FPART_PROP_STREAMUNPACKEDSIZE, context.nBlockSize);
        result.mapProperties.insert(FPART_PROP_SUBSTREAMOFFSET, context.nPrologueSize);
    }

    return result;
}

bool XARDI1SFX::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    HEADER header = {};
    const bool bResult = readHeader(&header, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XARDI1SFX::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XARDI1SFX archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XARDI1SFX::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XARDI1SFX(pDevice);
}

QList<QString> XARDI1SFX::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append(QStringLiteral("'ARDI-(C)1991-'"));
    return listResult;
}

XBinary::FT XARDI1SFX::getFileType()
{
    return FT_ARDI1_SFX;
}

XBinary::MODE XARDI1SFX::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XARDI1SFX::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XARDI1SFX::getArch()
{
    return QString();
}

QString XARDI1SFX::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XARDI1SFX::getFileFormatExtsString()
{
    return QStringLiteral("ARDI diskette SFX (*.exe)");
}

QString XARDI1SFX::getMIMEString()
{
    return QStringLiteral("application/x-ardi-diskette-sfx");
}

QString XARDI1SFX::getVersion()
{
    // Deliberately NOT parseContext(): the version is in the EOF trailer and
    // reading it must not drag in the prologue measuring pass, which inflates.
    HEADER header = {};
    if (!readHeader(&header, nullptr)) return QString();
    return QStringLiteral("1991-") + header.sTrailerYear;
}

qint64 XARDI1SFX::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nTotalSize : 0;
}

QList<XBinary::MAPMODE> XARDI1SFX::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XARDI1SFX::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_REGION, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XARDI1SFX::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XARDI1SFX::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.header.nStreamOffset;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        const ARCHIVERECORD record = imageRecord(context);
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = record.nStreamOffset;
        part.nFileSize = record.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sImageName;
        part.mapProperties = record.mapProperties;
        listResult.append(part);
    }

    // The 51 bytes behind the stream are the format's own EOF trailer, not an
    // appended overlay, so they are published as a region of the container.
    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nInputSize - ARDI1_TRAILER_SIZE;
        part.nFileSize = ARDI1_TRAILER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Trailer");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nTotalSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XARDI1SFX::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XARDI1SFX::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XARDI1SFX> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    if (pContext->nPrologueSize >= 0) {
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("ARDI self-extracting diskette image; the stub writes the image to a diskette or to a file the "
                                                                "user names, so the container stores no name and no destination path and the member name is "
                                                                "derived from the carrier"));
    } else {
        // Without the prologue length the image cannot be separated from the
        // stub's message table, so the member is published as a located stream
        // that carries no substream window and will not decode.
        pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("ARDI self-extracting diskette image; the leading record block could not be measured, so the "
                                                                "image cannot be separated from it"));
    }
    pState->nCurrentOffset = pContext->header.nStreamOffset;
    pState->nTotalSize = pContext->nTotalSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XARDI1SFX::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex != 0) || (pState->nNumberOfRecords != 1)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->header.nStreamOffset) return ARCHIVERECORD();

    return imageRecord(*pContext);
}

bool XARDI1SFX::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nTotalSize;
    return false;
}

bool XARDI1SFX::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
