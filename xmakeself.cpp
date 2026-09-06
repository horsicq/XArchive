/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xmakeself.h"

#include <QPointer>
#include <QRegularExpression>
#include <QtEndian>

#include <new>

namespace {
// makeself writes its banner, the sizes and the offset well inside the first
// few kilobytes; U3 scans the same 64 KiB window for the payload magic.
const qint64 MAKESELF_SCAN_SIZE = 0x10000;
// The generated wrapper is ~8-10 KiB for 2.1.x and never approaches this.
const qint64 MAKESELF_MAX_SCRIPT_SIZE = 0x100000;
const qint64 MAKESELF_MIN_SCRIPT_SIZE = 64;
const qint64 MAKESELF_TAR_BLOCK = 512;

bool makeselfLooksLikeTar(const QByteArray &baBlock)
{
    // A tar member header: 100-byte name, then octal fields, "ustar" at +257
    // for POSIX/GNU tar (GNU writes "ustar  \0", POSIX "ustar\0" "00").  The
    // header checksum is verified as well, which is what makes an otherwise
    // magic-free payload safe to claim.
    if (baBlock.size() < MAKESELF_TAR_BLOCK) return false;
    if (static_cast<quint8>(baBlock.at(0)) == 0) return false;
    if (baBlock.mid(257, 5) != QByteArray("ustar", 5)) return false;

    quint32 nStored = 0;
    bool bDigit = false;
    for (qint32 i = 148; i < 156; ++i) {
        const char c = baBlock.at(i);
        if ((c >= '0') && (c <= '7')) {
            nStored = (nStored * 8) + static_cast<quint32>(c - '0');
            bDigit = true;
        } else if ((c == ' ') || (c == '\0')) {
            if (bDigit) break;
        } else {
            return false;
        }
    }
    if (!bDigit) return false;

    quint32 nUnsigned = 0;
    for (qint32 i = 0; i < MAKESELF_TAR_BLOCK; ++i) {
        const quint8 nByte = ((i >= 148) && (i < 156))
                                 ? static_cast<quint8>(' ')
                                 : static_cast<quint8>(baBlock.at(i));
        nUnsigned += nByte;
    }
    return (nUnsigned == nStored);
}
}  // namespace

XMakeself::XMakeself(QIODevice *pDevice) : XArchive(pDevice)
{
}

XMakeself::~XMakeself()
{
}

XMakeself::PAYLOAD_KIND XMakeself::classifyPayload(const QByteArray &baMagic)
{
    if (baMagic.size() >= 3) {
        const quint8 b0 = static_cast<quint8>(baMagic.at(0));
        const quint8 b1 = static_cast<quint8>(baMagic.at(1));
        const quint8 b2 = static_cast<quint8>(baMagic.at(2));
        if ((b0 == 0x1f) && (b1 == 0x8b) && (b2 == 0x08)) {
            return PAYLOAD_KIND_GZIP;
        }
        if ((b0 == 0x1f) && (b1 == 0x9d)) return PAYLOAD_KIND_COMPRESS;
        if ((b0 == 'B') && (b1 == 'Z') && (b2 == 'h') && (baMagic.size() >= 4) &&
            (baMagic.at(3) >= '1') && (baMagic.at(3) <= '9')) {
            return PAYLOAD_KIND_BZIP2;
        }
        if ((baMagic.size() >= 6) && (b0 == 0xfd) &&
            (baMagic.mid(1, 5) == QByteArray("7zXZ\0", 5))) {
            return PAYLOAD_KIND_XZ;
        }
    }
    if (makeselfLooksLikeTar(baMagic)) return PAYLOAD_KIND_TAR;
    return PAYLOAD_KIND_UNKNOWN;
}

qint64 XMakeself::gzipHeaderSize(const QByteArray &baPayloadHead)
{
    if (baPayloadHead.size() < 10) return -1;
    const quint8 nFlags = static_cast<quint8>(baPayloadHead.at(3));
    if (nFlags & 0xe0) return -1;  // reserved bits must be zero
    qint64 nOffset = 10;
    if (nFlags & 0x04) {  // FEXTRA
        if ((nOffset + 2) > baPayloadHead.size()) return -1;
        const qint64 nExtra = static_cast<qint64>(qFromLittleEndian<quint16>(
            reinterpret_cast<const uchar *>(baPayloadHead.constData()) +
            nOffset));
        nOffset += 2 + nExtra;
    }
    if (nFlags & 0x08) {  // FNAME
        while ((nOffset < baPayloadHead.size()) &&
               (baPayloadHead.at(nOffset) != '\0')) {
            ++nOffset;
        }
        ++nOffset;
    }
    if (nFlags & 0x10) {  // FCOMMENT
        while ((nOffset < baPayloadHead.size()) &&
               (baPayloadHead.at(nOffset) != '\0')) {
            ++nOffset;
        }
        ++nOffset;
    }
    if (nFlags & 0x02) nOffset += 2;  // FHCRC
    if (nOffset > baPayloadHead.size()) return -1;
    return nOffset;
}

QString XMakeself::extractShellValue(const QByteArray &baScript,
                                     const char *pKey)
{
    // Matches a shell assignment at the start of a line: key="value".
    const QByteArray baPattern = QByteArray(pKey) + "=\"";
    qint32 nPosition = 0;
    while (nPosition >= 0) {
        nPosition = baScript.indexOf(baPattern, nPosition);
        if (nPosition < 0) break;
        if ((nPosition == 0) || (baScript.at(nPosition - 1) == '\n')) {
            const qint32 nStart = nPosition + baPattern.size();
            const qint32 nEnd = baScript.indexOf('"', nStart);
            if (nEnd < 0) return QString();
            return QString::fromLatin1(baScript.mid(nStart, nEnd - nStart));
        }
        nPosition += baPattern.size();
    }
    return QString();
}

bool XMakeself::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XMakeself> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    context.nUncompressedSize = -1;
    if (context.nInputSize < (MAKESELF_MIN_SCRIPT_SIZE + MAKESELF_TAR_BLOCK)) {
        return false;
    }

    const qint64 nScanSize = qMin<qint64>(MAKESELF_SCAN_SIZE,
                                          context.nInputSize);
    const QByteArray baScan = read_array_process(0, nScanSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baScan.size() != nScanSize)) {
        return false;
    }

    // The wrapper is a shell script and says so, and it says which tool wrote
    // it.  Both are required: "#!/bin/sh" alone matches half the scripts on a
    // Unix system.
    if (!baScan.startsWith("#!")) return false;
    const qint32 nShebangEnd = baScan.indexOf('\n');
    if ((nShebangEnd < 0) || (nShebangEnd > 128)) return false;
    const QByteArray baShebang = baScan.left(nShebangEnd);
    if (!baShebang.contains("sh")) return false;

    const qint32 nBanner = baScan.indexOf("Makeself");
    if (nBanner < 0) return false;
    // The two variables the extractor itself reads.
    if (baScan.indexOf("\nfilesizes=") < 0) return false;
    if (baScan.indexOf("\noffset=") < 0) return false;

    QRegularExpression versionExpression(
        QStringLiteral("Makeself\\s+version\\s+([0-9][0-9.]*)|"
                       "Makeself\\s+([0-9][0-9.]*)"));
    const QRegularExpressionMatch versionMatch = versionExpression.match(
        QString::fromLatin1(baScan.mid(nBanner, 64)));
    if (versionMatch.hasMatch()) {
        context.sVersion = versionMatch.captured(1).isEmpty()
                               ? versionMatch.captured(2)
                               : versionMatch.captured(1);
    }
    context.sLabel = extractShellValue(baScan, "label");

    // filesizes="N" or, in split archives, filesizes="N1 N2 N3".
    const QString sFileSizes = extractShellValue(baScan, "filesizes");
    qint64 nTotal = 0;
    bool bTotalKnown = !sFileSizes.isEmpty();
    if (bTotalKnown) {
        const QStringList listParts =
            sFileSizes.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (listParts.isEmpty()) bTotalKnown = false;
        for (qint32 i = 0; bTotalKnown && (i < listParts.size()); ++i) {
            bool bOk = false;
            const qint64 nValue = listParts.at(i).toLongLong(&bOk);
            if (!bOk || (nValue < 0) ||
                (nValue > (context.nInputSize - nTotal))) {
                bTotalKnown = false;
                break;
            }
            nTotal += nValue;
        }
    }

    qint64 nPayloadOffset = -1;
    PAYLOAD_KIND payloadKind = PAYLOAD_KIND_UNKNOWN;
    if (bTotalKnown && (nTotal > 0) && (nTotal < context.nInputSize)) {
        const qint64 nCandidate = context.nInputSize - nTotal;
        if ((nCandidate >= MAKESELF_MIN_SCRIPT_SIZE) &&
            (nCandidate <= MAKESELF_MAX_SCRIPT_SIZE)) {
            const QByteArray baMagic = read_array_process(
                nCandidate,
                qMin<qint64>(MAKESELF_TAR_BLOCK, context.nInputSize - nCandidate),
                pPdStruct);
            if (!guardedThis || !guardedSource) return false;
            payloadKind = classifyPayload(baMagic);
            if (payloadKind != PAYLOAD_KIND_UNKNOWN) nPayloadOffset = nCandidate;
        }
    }

    if (nPayloadOffset < 0) {
        // U3's FUN_00653690 fallback: scan the first 64 KiB for the payload's
        // own magic.  Needed when filesizes is absent or has been edited.
        const qint64 nLimit = baScan.size() - 4;
        for (qint64 i = MAKESELF_MIN_SCRIPT_SIZE; i < nLimit; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            const quint8 b0 = static_cast<quint8>(baScan.at(i));
            if ((b0 != 0x1f) && (b0 != 'B') && (b0 != 0xfd)) continue;
            const PAYLOAD_KIND kind = classifyPayload(baScan.mid(i, 6));
            if ((kind != PAYLOAD_KIND_UNKNOWN) && (kind != PAYLOAD_KIND_TAR)) {
                nPayloadOffset = i;
                payloadKind = kind;
                break;
            }
        }
    }
    if ((nPayloadOffset < 0) || (payloadKind == PAYLOAD_KIND_UNKNOWN)) {
        return false;
    }

    context.nScriptSize = nPayloadOffset;
    context.nPayloadOffset = nPayloadOffset;
    context.nPayloadSize = context.nInputSize - nPayloadOffset;
    context.payloadKind = payloadKind;
    context.nStreamOffset = nPayloadOffset;
    context.nStreamSize = context.nPayloadSize;

    if (payloadKind == PAYLOAD_KIND_GZIP) {
        const QByteArray baHead = read_array_process(
            nPayloadOffset, qMin<qint64>(4096, context.nPayloadSize),
            pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        const qint64 nHeaderSize = gzipHeaderSize(baHead);
        if ((nHeaderSize < 10) || (nHeaderSize >= context.nPayloadSize)) {
            return false;
        }
        // Deflate data sits between the header and the 8-byte CRC32/ISIZE
        // trailer; ISIZE is the plaintext length modulo 2^32.
        if (context.nPayloadSize < (nHeaderSize + 8)) return false;
        context.nStreamOffset = nPayloadOffset + nHeaderSize;
        context.nStreamSize = context.nPayloadSize - nHeaderSize - 8;
        const QByteArray baTrailer = read_array_process(
            context.nInputSize - 4, 4, pPdStruct);
        if (!guardedThis || !guardedSource || (baTrailer.size() != 4)) {
            return false;
        }
        context.nUncompressedSize = static_cast<qint64>(
            qFromLittleEndian<quint32>(
                reinterpret_cast<const uchar *>(baTrailer.constData())));
    } else if (payloadKind == PAYLOAD_KIND_TAR) {
        context.nUncompressedSize = context.nPayloadSize;
    }
    if (context.nStreamSize <= 0) return false;

    QString sBaseName = XBinary::getDeviceFileBaseName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    if (sBaseName.isEmpty()) sBaseName = QStringLiteral("makeself");
    context.sFileName = sBaseName + QStringLiteral(".tar");

    if (!isPdStructNotCanceled(pPdStruct)) return false;
    *pContext = context;
    return true;
}

XBinary::HANDLE_METHOD XMakeself::handleMethod(const CONTEXT &context) const
{
    switch (context.payloadKind) {
        case PAYLOAD_KIND_TAR: return HANDLE_METHOD_STORE;
        case PAYLOAD_KIND_GZIP: return HANDLE_METHOD_DEFLATE;
        case PAYLOAD_KIND_BZIP2: return HANDLE_METHOD_BZIP2;
        case PAYLOAD_KIND_COMPRESS: return HANDLE_METHOD_COMPRESS;
        case PAYLOAD_KIND_XZ: return HANDLE_METHOD_XZ;
        default: return HANDLE_METHOD_UNKNOWN;
    }
}

QString XMakeself::methodName(PAYLOAD_KIND payloadKind)
{
    switch (payloadKind) {
        case PAYLOAD_KIND_TAR: return QStringLiteral("Store");
        case PAYLOAD_KIND_GZIP: return QStringLiteral("gzip");
        case PAYLOAD_KIND_BZIP2: return QStringLiteral("bzip2");
        case PAYLOAD_KIND_COMPRESS: return QStringLiteral("compress");
        case PAYLOAD_KIND_XZ: return QStringLiteral("xz");
        default: return QString();
    }
}

bool XMakeself::isValid(PDSTRUCT *pPdStruct)
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

bool XMakeself::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XMakeself archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XMakeself::createInstance(QIODevice *pDevice, bool bIsImage,
                                   XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XMakeself(pDevice);
}

QList<QString> XMakeself::getSearchSignatures()
{
    QList<QString> listResult;
    listResult.append("'#!/bin/sh'");
    return listResult;
}

XBinary::FT XMakeself::getFileType()
{
    return FT_MAKESELF;
}

XBinary::MODE XMakeself::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XMakeself::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XMakeself::getArch()
{
    return QString();
}

XBinary::OSNAME XMakeself::getOsName()
{
    return OSNAME_UNIX;
}

QString XMakeself::getFileFormatExt()
{
    return QStringLiteral("run");
}

QString XMakeself::getFileFormatExtsString()
{
    return QStringLiteral("Makeself self-extracting archive (*.run *.sh)");
}

QString XMakeself::getMIMEString()
{
    return QStringLiteral("application/x-shellscript");
}

QString XMakeself::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return context.sVersion;
}

qint64 XMakeself::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    // The payload runs to the end of the file, so the container always spans
    // the whole file.
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XMakeself::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XMakeself::getMemoryMap(MAPMODE mapMode,
                                             PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) {
        return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    }
    if (mapMode == MAPMODE_STREAMS) {
        return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    }
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XMakeself::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XMakeself::getFileParts(quint32 nFileParts, qint32 nLimit,
                                              PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, 0)) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nScriptSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Shell script");
        result.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nStreamOffset;
        part.nFileSize = context.nStreamSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                  context.nStreamSize);
        if (context.nUncompressedSize >= 0) {
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      context.nUncompressedSize);
        }
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                  handleMethod(context));
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                  methodName(context.payloadKind));
        part.mapProperties.insert(FPART_PROP_ISFOLDER, false);
        result.append(part);
    }

    if ((nFileParts & FILEPART_REGION) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nPayloadOffset;
        part.nFileSize = context.nPayloadSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Payload");
        result.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        result.append(part);
    }
    return result;
}

QMap<XBinary::UNPACK_PROP, QVariant> XMakeself::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XMakeself::initUnpack(UNPACK_STATE *pState,
                           const QMap<UNPACK_PROP, QVariant> &mapProperties,
                           PDSTRUCT *pPdStruct)
{
    QPointer<XMakeself> guardedThis(this);
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
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) {
        return false;
    }

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
    QString sInfo = tr("Makeself self-extracting archive");
    if (!pContext->sVersion.isEmpty()) {
        sInfo += QStringLiteral(" %1").arg(pContext->sVersion);
    }
    if (!pContext->sLabel.isEmpty()) {
        sInfo += QStringLiteral("; %1").arg(pContext->sLabel);
    }
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, sInfo);
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

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

XBinary::ARCHIVERECORD XMakeself::infoCurrent(UNPACK_STATE *pState,
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
    if (!pContext || (pState->nCurrentOffset != pContext->nStreamOffset)) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE,
                                pContext->nStreamSize);
    if (pContext->nUncompressedSize >= 0) {
        result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                    pContext->nUncompressedSize);
    }
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                handleMethod(*pContext));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                methodName(pContext->payloadKind));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // The script carries a CRCsum and an MD5 of the payload, but both are
    // written as decimal/hex text over the *compressed* bytes and makeself
    // itself skips them when they read "0000000000"; they are not published as
    // a member checksum.
    return result;
}

bool XMakeself::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nInputSize;
    return (pState->nCurrentIndex < pState->nNumberOfRecords);
}

bool XMakeself::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
