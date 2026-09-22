/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xampk.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 AMPK_HEADER_SIZE = 20;
// The smallest legal archive is the 20-byte header plus one file record
// carrying a one-character name and an empty payload.
const qint64 AMPK_MIN_FILE_SIZE = 40;
const qint64 AMPK_RECORD_PREFIX_SIZE = 6;
const qint64 AMPK_FILE_TRAILER_SIZE = 18;
const qint64 AMPK_TERMINATOR_SIZE = 6;
// 6 prefix + 1 name byte + 18 trailer: the minimum a file record costs, used to
// reject headers whose declared member count cannot physically fit.
const qint64 AMPK_MIN_FILE_RECORD_SIZE =
    AMPK_RECORD_PREFIX_SIZE + 1 + AMPK_FILE_TRAILER_SIZE;
const qint64 AMPK_MAX_RECORD_PROBE =
    AMPK_RECORD_PREFIX_SIZE + 255 + AMPK_FILE_TRAILER_SIZE;

const quint8 AMPK_RECORD_END = 0x00;
const quint8 AMPK_RECORD_DIRECTORY = 0x01;
const quint8 AMPK_RECORD_FILE = 0x02;

const quint8 AMPK_METHOD_STORE = 0;
const quint8 AMPK_METHOD_LZARI = 1;
const quint8 AMPK_METHOD_LZSS = 2;
const quint8 AMPK_METHOD_LZHUF = 3;

const quint8 AMPK_VERSION_MIN = 2;
const quint8 AMPK_VERSION_MAX = 4;

// The declared counts are 16-bit fields, so these caps can never reject a
// well-formed archive; they only bound the walk over a corrupt one.
const qint32 AMPK_MAX_MEMBERS = 65535;
const qint32 AMPK_MAX_DIRECTORIES = 65535;
const qint32 AMPK_MAX_DEPTH = 64;

bool ampkRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

// Names are ISO-8859-1 Amiga file names: high bytes are legitimate (28 members
// in the reference corpus carry German umlauts), but no control character and
// no path separator ever occurs, so those still mark a desynchronised walk.
bool ampkIsValidName(const QByteArray &baName)
{
    if (baName.isEmpty()) return false;
    for (char c : baName) {
        const quint8 nCharacter = static_cast<quint8>(c);
        if (nCharacter < 0x20 || nCharacter == 0x7f || nCharacter == '/' ||
            nCharacter == '\\') {
            return false;
        }
    }
    return true;
}

QString ampkJoinPath(const QList<QString> &listDirectories,
                     const QString &sName)
{
    QString sResult;
    for (const QString &sDirectory : listDirectories) {
        sResult.append(sDirectory);
        sResult.append(QLatin1Char('/'));
    }
    sResult.append(sName);
    return sResult;
}
}  // namespace

XAMPK::XAMPK(QIODevice *pDevice) : XArchive(pDevice)
{
}

XAMPK::~XAMPK()
{
}

bool XAMPK::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < AMPK_MIN_FILE_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, AMPK_HEADER_SIZE, pPdStruct);
    if (baHeader.size() != AMPK_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if (qFromBigEndian<quint32>(pHeader) != 0x414d504bU) return false;  // "AMPK"

    context.nVersion = pHeader[4];
    if ((context.nVersion < AMPK_VERSION_MIN) ||
        (context.nVersion > AMPK_VERSION_MAX) || (pHeader[5] != 0)) {
        return false;
    }
    context.nDeclaredDirectoryCount = qFromBigEndian<quint16>(pHeader + 6);
    context.nDeclaredFileCount = qFromBigEndian<quint16>(pHeader + 8);
    context.nDeclaredUncompressedSize = qFromBigEndian<quint32>(pHeader + 10);
    context.nDeclaredDataSize = qFromBigEndian<quint32>(pHeader + 14);
    // Bytes 0x12..0x13 are reserved and are NOT tested: two corpus archives
    // (both Oberon disks) carry 0x0100 there while every other file has 0.

    if ((context.nDeclaredFileCount == 0) ||
        (context.nDeclaredUncompressedSize == 0) ||
        (context.nDeclaredDataSize == 0)) {
        return false;
    }
    if (static_cast<qint64>(context.nDeclaredDataSize) >
        context.nInputSize - AMPK_HEADER_SIZE) {
        return false;
    }
    // Every member costs at least a minimal record header on top of its
    // payload, so the non-payload remainder has to cover all of them.
    if ((context.nInputSize -
         static_cast<qint64>(context.nDeclaredDataSize)) <
        AMPK_MIN_FILE_RECORD_SIZE *
            static_cast<qint64>(context.nDeclaredFileCount)) {
        return false;
    }
    // Deliberately loose: no archive in the family claims more than a fourfold
    // expansion, and this is what keeps random 20-byte "AMPK" prefixes out.
    if (static_cast<qint64>(context.nDeclaredUncompressedSize) <
        static_cast<qint64>(context.nDeclaredDataSize) / 4) {
        return false;
    }

    qint64 nOffset = AMPK_HEADER_SIZE;
    QList<QString> listDirectories;
    quint64 nTotalUncompressedSize = 0;
    quint64 nTotalDataSize = 0;
    bool bWalkComplete = false;

    while (nOffset < context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;

        const qint64 nProbeSize =
            qMin<qint64>(AMPK_MAX_RECORD_PROBE, context.nInputSize - nOffset);
        if (nProbeSize < AMPK_RECORD_PREFIX_SIZE) break;
        const QByteArray baProbe =
            read_array_process(nOffset, nProbeSize, pPdStruct);
        if (baProbe.size() != nProbeSize) {
            return false;
        }
        const uchar *pProbe =
            reinterpret_cast<const uchar *>(baProbe.constData());
        const quint8 nRecordType = pProbe[0];
        const qint64 nNameSize = pProbe[1];

        if (nRecordType == AMPK_RECORD_END) {
            // Six zero bytes pop one directory.  There is no archive-level end
            // marker: the record stream simply runs out at the file size.
            if (baProbe.mid(0, static_cast<qint32>(AMPK_TERMINATOR_SIZE)) !=
                QByteArray(static_cast<qint32>(AMPK_TERMINATOR_SIZE), '\0')) {
                break;
            }
            if (listDirectories.isEmpty()) break;
            listDirectories.removeLast();
            nOffset += AMPK_TERMINATOR_SIZE;
        } else if (nRecordType == AMPK_RECORD_DIRECTORY) {
            if ((nNameSize == 0) ||
                (nProbeSize < AMPK_RECORD_PREFIX_SIZE + nNameSize)) {
                break;
            }
            const QByteArray baName =
                baProbe.mid(static_cast<qint32>(AMPK_RECORD_PREFIX_SIZE),
                            static_cast<qint32>(nNameSize));
            if (!ampkIsValidName(baName)) break;
            if ((listDirectories.size() >= AMPK_MAX_DEPTH) ||
                (context.nDirectoryCount >= AMPK_MAX_DIRECTORIES)) {
                break;
            }
            // Bytes +2..+5 hold a subtree size hint that is off by the name
            // length and zero for empty directories.  It is never used for
            // navigation; the record stream itself is walked instead.
            listDirectories.append(QString::fromLatin1(baName));
            ++context.nDirectoryCount;
            nOffset += AMPK_RECORD_PREFIX_SIZE + nNameSize;
        } else if (nRecordType == AMPK_RECORD_FILE) {
            if ((nNameSize == 0) ||
                (nProbeSize <
                 AMPK_RECORD_PREFIX_SIZE + nNameSize +
                     AMPK_FILE_TRAILER_SIZE)) {
                break;
            }
            if (qFromBigEndian<quint32>(pProbe + 2) != 0) break;
            const QByteArray baName =
                baProbe.mid(static_cast<qint32>(AMPK_RECORD_PREFIX_SIZE),
                            static_cast<qint32>(nNameSize));
            if (!ampkIsValidName(baName)) break;

            const uchar *pTrailer =
                pProbe + AMPK_RECORD_PREFIX_SIZE + nNameSize;
            MEMBER member = {};
            member.nRecordOffset = nOffset;
            member.nHeaderSize =
                AMPK_RECORD_PREFIX_SIZE + nNameSize + AMPK_FILE_TRAILER_SIZE;
            member.nDataOffset = nOffset + member.nHeaderSize;
            member.nUncompressedSize = qFromBigEndian<quint32>(pTrailer);
            member.nDeclaredPackedSize = qFromBigEndian<quint32>(pTrailer + 4);
            member.nMethod = pTrailer[8];
            member.nAttributes = pTrailer[13];
            member.sFileName =
                ampkJoinPath(listDirectories, QString::fromLatin1(baName));

            if (member.nMethod > AMPK_METHOD_LZHUF) break;
            // THE rule of this format: for a stored member the compressedSize
            // field holds the packer's would-be-compressed size and is junk
            // (observed deltas from -274249 to +16738).  Advancing by it
            // desynchronises the walk at the first stored member.
            member.nDataSize = (member.nMethod == AMPK_METHOD_STORE)
                                   ? member.nUncompressedSize
                                   : member.nDeclaredPackedSize;
            if (!ampkRangeWithin(context.nInputSize, member.nDataOffset,
                                 member.nDataSize)) {
                break;
            }
            // A zero-size member with a payload cannot be described by this
            // record layout; it is the shape a desynchronised walk produces.
            if ((member.nUncompressedSize == 0) && (member.nDataSize != 0)) {
                break;
            }
            if (context.listMembers.size() >= AMPK_MAX_MEMBERS) break;

            nTotalUncompressedSize +=
                static_cast<quint64>(member.nUncompressedSize);
            nTotalDataSize += static_cast<quint64>(member.nDataSize);
            context.listMembers.append(member);
            nOffset = member.nDataOffset + member.nDataSize;
        } else {
            break;
        }

        if (nOffset == context.nInputSize) {
            bWalkComplete = true;
            break;
        }
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = nOffset;
    context.nFirstMemberOffset = context.listMembers.first().nRecordOffset;
    // A directory whose subtree runs to EOF may have its terminator omitted (4
    // corpus archives do this), so a non-empty directory stack at EOF is legal
    // and must not count as damage.
    context.bComplete =
        bWalkComplete &&
        (context.listMembers.size() ==
         static_cast<qint32>(context.nDeclaredFileCount)) &&
        (context.nDirectoryCount ==
         static_cast<qint32>(context.nDeclaredDirectoryCount)) &&
        (nTotalUncompressedSize ==
         static_cast<quint64>(context.nDeclaredUncompressedSize)) &&
        (nTotalDataSize == static_cast<quint64>(context.nDeclaredDataSize));

    *pContext = context;
    return guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XAMPK::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XAMPK::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XAMPK archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XAMPK::createInstance(QIODevice *pDevice, bool bIsImage,
                               XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XAMPK(pDevice);
}

QList<QString> XAMPK::getSearchSignatures()
{
    return {QStringLiteral("'AMPK'")};
}

XBinary::FT XAMPK::getFileType()
{
    return FT_AMPK;
}

XBinary::MODE XAMPK::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XAMPK::getEndian()
{
    return ENDIAN_BIG;
}

QString XAMPK::getArch()
{
    return QString();
}

QString XAMPK::getFileFormatExt()
{
    return QStringLiteral("ampk");
}

QString XAMPK::getFileFormatExtsString()
{
    return QStringLiteral("Amiga AMPK archive (*.ampk)");
}

QString XAMPK::getMIMEString()
{
    return QStringLiteral("application/x-ampk");
}

QString XAMPK::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVersion);
}

qint64 XAMPK::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XAMPK::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XAMPK::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(
            FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XAMPK::methodToString(quint8 nMethod)
{
    QString sName;
    if (nMethod == AMPK_METHOD_STORE) {
        sName = QStringLiteral("Stored");
    } else if (nMethod == AMPK_METHOD_LZARI) {
        sName = QStringLiteral("LZARI");
    } else if (nMethod == AMPK_METHOD_LZSS) {
        sName = QStringLiteral("LZSS");
    } else if (nMethod == AMPK_METHOD_LZHUF) {
        sName = QStringLiteral("LZHUF");
    } else {
        sName = QStringLiteral("Unknown");
    }
    return QStringLiteral("AMPK %1 %2").arg(nMethod).arg(sName);
}

XBinary::HANDLE_METHOD XAMPK::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == AMPK_METHOD_STORE) return HANDLE_METHOD_STORE;
    if (nMethod == AMPK_METHOD_LZARI) return HANDLE_METHOD_AMPK_LZARI;
    if (nMethod == AMPK_METHOD_LZSS) return HANDLE_METHOD_AMPK_LZSS;
    // Method 3 is plain Okumura/Yoshizaki LZHUF with no EOF symbol, which is
    // exactly the -lh1- decoder.  HANDLE_METHOD_ARCV_LZHUF60 looks closer by
    // name but carries an EOF symbol and diverges from the first symbol on.
    if (nMethod == AMPK_METHOD_LZHUF) return HANDLE_METHOD_LZH1;
    return HANDLE_METHOD_UNKNOWN;
}

bool XAMPK::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XAMPK::getFileParts(quint32 nFileParts, qint32 nLimit,
                                          PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = AMPK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Archive header");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_HEADER) {
            FPART part = {};
            part.filePart = FILEPART_HEADER;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = member.nHeaderSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = tr("Member header");
            result.append(part);
        }
        if ((nFileParts & FILEPART_STREAM) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                      member.nDataSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      methodToString(member.nMethod));
            part.mapProperties.insert(FPART_PROP_TYPE,
                                      static_cast<quint32>(member.nMethod));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nRecordOffset;
            part.nFileSize = member.nHeaderSize + member.nDataSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            result.append(part);
        }
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
    // Only a damaged archive leaves a tail: a healthy one ends exactly on the
    // last record boundary and has no overlay at all.
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

QMap<XBinary::UNPACK_PROP, QVariant> XAMPK::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XAMPK::initUnpack(UNPACK_STATE *pState,
                       const QMap<UNPACK_PROP, QVariant> &mapProperties,
                       PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) ||
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
    if (!parseContext(pContext, pPdStruct) ||
        pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        pContext->bComplete
            ? tr("Amiga AMPK archive")
            : tr("Amiga AMPK archive; damaged, listing is incomplete"));
    pState->nCurrentOffset = pContext->nFirstMemberOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
    if (!bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }
    return true;
}

XBinary::ARCHIVERECORD XAMPK::infoCurrent(UNPACK_STATE *pState,
                                          PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress,
                                          &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return ARCHIVERECORD();
    }
    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nRecordOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nDataSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nDataSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_TYPE,
                                static_cast<quint32>(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XAMPK::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        pState->nCurrentIndex < 0 ||
        pState->nCurrentIndex >= pState->nNumberOfRecords) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext || pState->nCurrentIndex >= pContext->listMembers.size()) {
        return false;
    }
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset =
            pContext->listMembers.at(pState->nCurrentIndex).nRecordOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XAMPK::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
