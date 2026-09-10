/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xbthpak.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

#include "Algos/xbthpakdecoder.h"

namespace {
const qint64 BTHPAK_HEADER_SIZE = 8;
// A block needs at least a four-byte pair table, the two-byte length field and
// five packed bytes, so anything below this cannot hold one complete block.
const qint64 BTHPAK_MIN_FILE_SIZE = BTHPAK_HEADER_SIZE + 11;
const qint64 BTHPAK_MAX_UNCOMPRESSED_SIZE = 0x40000000;
// Detection must not pull an unbounded amount of data into memory just to walk
// a chain.  Everything in the known family is under 300 KB; beyond this limit
// only a bounded prefix is proved, which is enough to reject junk.
const qint64 BTHPAK_FULL_SCAN_LIMIT = 16 * 1024 * 1024;
const qint64 BTHPAK_PARTIAL_WINDOW = 1024 * 1024;
const qint32 BTHPAK_PARTIAL_BLOCKS = 64;

bool bthpakIsExtensionCharacter(quint8 nCharacter)
{
    // The header's fourth byte is the restored final extension character.
    // Restricting it to upper-case alphanumerics is what separates this magic
    // from arbitrary data that happens to start with "PAK"; the whole family
    // uses only 'P', 'V', 'D', 'L' and 'E'.
    return ((nCharacter >= 'A') && (nCharacter <= 'Z')) ||
           ((nCharacter >= '0') && (nCharacter <= '9'));
}

// The container stores no file name at all, only the last character of the
// original extension, so the member name has to come from the device.
QString bthpakMemberName(QIODevice *pDevice, quint8 nExtensionCharacter)
{
    const QChar cExtension =
        QChar::fromLatin1(static_cast<char>(nExtensionCharacter));
    QPointer<QIODevice> guardedDevice(pDevice);
    const QString sDeviceName =
        guardedDevice ? XBinary::getDeviceFileName(guardedDevice.data())
                      : QString();

    QString sResult;
    if (guardedDevice && !sDeviceName.isEmpty()) {
        const QFileInfo fileInfo(sDeviceName);
        const QString sBaseName = fileInfo.completeBaseName();
        const QString sSuffix = fileInfo.suffix();
        if (!sBaseName.isEmpty()) {
            if (sSuffix.endsWith(QLatin1Char('_'))) {
                // The MS-DOS distribution convention: the media carries .BM_
                // and only the header byte can turn it back into .BMP.
                sResult = sBaseName + QLatin1Char('.') +
                          sSuffix.left(sSuffix.size() - 1) + cExtension;
            } else if (sSuffix.isEmpty()) {
                sResult = sBaseName + QLatin1Char('.') + cExtension;
            } else {
                sResult = sBaseName + QLatin1Char('.') + sSuffix;
            }
        }
    }

    // Nested extraction hands us a device with no name; never emit an empty
    // member name or a bare "_".
    if (sResult.isEmpty()) {
        sResult = QStringLiteral("pak_data.") + cExtension;
    }
    return sResult;
}
}  // namespace

XBTHPAK::XBTHPAK(QIODevice *pDevice) : XArchive(pDevice)
{
}

XBTHPAK::~XBTHPAK()
{
}

bool XBTHPAK::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XBTHPAK> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < BTHPAK_MIN_FILE_SIZE) return false;

    const QByteArray baHeader =
        read_array_process(0, BTHPAK_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource ||
        baHeader.size() != BTHPAK_HEADER_SIZE) {
        return false;
    }
    const uchar *pHeader =
        reinterpret_cast<const uchar *>(baHeader.constData());
    if ((pHeader[0] != 'P') || (pHeader[1] != 'A') || (pHeader[2] != 'K') ||
        !bthpakIsExtensionCharacter(pHeader[3])) {
        return false;
    }
    context.nExtensionCharacter = pHeader[3];
    context.nUncompressedSize =
        static_cast<qint64>(qFromLittleEndian<quint32>(pHeader + 4));
    // There is no compressed-size field to cross-check against, so this is the
    // only size the decoder can be given.  It is exact on every genuine file
    // and one member of the family actually expands, so it must NOT be clamped
    // against the payload size the way SZDD's heuristic does.
    if ((context.nUncompressedSize <= 0) ||
        (context.nUncompressedSize > BTHPAK_MAX_UNCOMPRESSED_SIZE)) {
        return false;
    }

    context.nPayloadOffset = BTHPAK_HEADER_SIZE;
    context.nPayloadSize = context.nInputSize - BTHPAK_HEADER_SIZE;

    const bool bPartialScan = (context.nPayloadSize > BTHPAK_FULL_SCAN_LIMIT);
    const qint64 nScanSize =
        bPartialScan ? BTHPAK_PARTIAL_WINDOW : context.nPayloadSize;
    const QByteArray baPayload =
        read_array_process(context.nPayloadOffset, nScanSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baPayload.size() != nScanSize)) {
        return false;
    }

    // The load-bearing check.  Nothing is expanded here: the walk only parses
    // each block's pair table and skips its packed bytes, and the chain has to
    // land exactly on EOF because the format has no end marker.
    qint64 nConsumed = 0;
    qint32 nBlockCount = 0;
    if (!XBTHPAKDecoder::scanChain(baPayload, !bPartialScan,
                                   bPartialScan ? BTHPAK_PARTIAL_BLOCKS : 0,
                                   &nConsumed, &nBlockCount, pPdStruct)) {
        return false;
    }
    if (!bPartialScan && (nConsumed != context.nPayloadSize)) return false;

    context.bPartialScan = bPartialScan;
    context.nBlockCount = nBlockCount;
    context.nArchiveSize = context.nInputSize;
    context.sFileName =
        bthpakMemberName(guardedSource.data(), context.nExtensionCharacter);
    if (!guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    *pContext = context;
    return true;
}

bool XBTHPAK::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XBTHPAK::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XBTHPAK archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XBTHPAK::createInstance(QIODevice *pDevice, bool bIsImage,
                                 XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XBTHPAK(pDevice);
}

QList<QString> XBTHPAK::getSearchSignatures()
{
    // Only three bytes are constant; the fourth is the restored extension
    // character.  The structural chain walk in isValid() is what makes this
    // narrow magic safe.
    return {QStringLiteral("'PAK'")};
}

XBinary::FT XBTHPAK::getFileType()
{
    return FT_BTH_PAK;
}

XBinary::MODE XBTHPAK::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XBTHPAK::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XBTHPAK::getArch()
{
    return QString();
}

QString XBTHPAK::getFileFormatExt()
{
    return QStringLiteral("pak");
}

QString XBTHPAK::getFileFormatExtsString()
{
    return QStringLiteral("Beat The House PAK (*.pak)");
}

QString XBTHPAK::getMIMEString()
{
    return QStringLiteral("application/x-bth-pak");
}

QString XBTHPAK::getVersion()
{
    // The header has no version field at all.
    return QString();
}

qint64 XBTHPAK::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XBTHPAK::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XBTHPAK::getMemoryMap(MAPMODE mapMode,
                                           PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM |
                                 FILEPART_OVERLAY,
                             pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XBTHPAK::methodToString()
{
    return QStringLiteral("Byte Pair Encoding");
}

bool XBTHPAK::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XBTHPAK::getFileParts(quint32 nFileParts, qint32 nLimit,
                                            PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = BTHPAK_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        result.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nPayloadOffset;
        part.nFileSize = context.nPayloadSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nPayloadSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                  context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  HANDLE_METHOD_BPE_GAGE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString());
        result.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
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

    // The format is defined as payload-to-EOF, so the archive always spans the
    // whole file and this branch never fires today.  It is kept so the part
    // list stays correct if a wrapped/embedded shape ever turns up.
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

QMap<XBinary::UNPACK_PROP, QVariant> XBTHPAK::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XBTHPAK::initUnpack(UNPACK_STATE *pState,
                         const QMap<UNPACK_PROP, QVariant> &mapProperties,
                         PDSTRUCT *pPdStruct)
{
    QPointer<XBTHPAK> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!pState || !guardedSource || guardedSource->isSequential() ||
        m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) &&
        !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource ||
        !isPdStructNotCanceled(pPdStruct)) {
        return false;
    }

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() ||
        !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct) || !guardedThis ||
        !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(
        FPART_PROP_INFO,
        tr("Beat The House PAK; single member, name restored from the "
           "truncated extension"));
    pState->nCurrentOffset = 0;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding alone only stages the source; without this the listing works
    // while extraction silently produces nothing.
    const bool bFinalized = guardedThis->validateAndFinalizeUnpackSource(
        pState, pContext, pPdStruct);
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

XBinary::ARCHIVERECORD XBTHPAK::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nPayloadOffset;
    result.nStreamSize = pContext->nPayloadSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nPayloadSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                HANDLE_METHOD_BPE_GAGE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString());
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XBTHPAK::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState ||
        !isUnpackSourceCurrent(pState, pPdStruct) ||
        (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // The index must advance before the answer is computed: returning false
    // without advancing at the last record makes the whole listing come back
    // empty.  With a single member this always advances then returns false.
    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XBTHPAK::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
