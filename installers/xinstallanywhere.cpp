/* Copyright (c) 2026 hors<horsicq@gmail.com>
 *
 * MIT License
 */

#include "xinstallanywhere.h"

#include <QPointer>

#include <new>

namespace {
// Everything the walker needs sits in the first ~1 KiB of every observed
// generator output (the second "#!/bin/sh" lands at offset 515/517).  4 KiB is
// headroom for other InstallAnywhere versions, and it caps how much of an
// arbitrary text file we are willing to touch before deciding.
const qint64 IA_SCAN_SIZE = 4096;
const qint64 IA_MIN_INPUT_SIZE = 1024;
const qint64 IA_DEFAULT_BLOCKSIZE = 32768;
const qint64 IA_MIN_BLOCKSIZE = 512;
const qint64 IA_MAX_BLOCKSIZE = 1048576;
const qint64 IA_MAGIC_SIZE = 4;

// Version-agnostic on purpose: the same USE.SH layout was emitted by the 4.x
// through 7.x generators, and pinning "7.0.0" would silently drop the others.
const char IA_BANNER[] = "InstallAnywhere (tm) UNIX Self Extractor";
const char IA_SHEBANG[] = "#!/bin/sh";

bool iaRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return nTotalSize >= 0 && nOffset >= 0 && nSize >= 0 &&
           nOffset <= nTotalSize && nSize <= nTotalSize - nOffset;
}

bool iaIsPowerOfTwo(qint64 nValue)
{
    return nValue > 0 && (nValue & (nValue - 1)) == 0;
}

bool iaIsNameCharacter(char cCharacter, bool bFirst)
{
    if ((cCharacter >= 'A' && cCharacter <= 'Z') ||
        (cCharacter >= 'a' && cCharacter <= 'z') || cCharacter == '_') {
        return true;
    }
    return !bFirst && (cCharacter >= '0' && cCharacter <= '9');
}

// Line-anchored NAME=value scan over the generator-emitted preamble.  The same
// names are re-assigned later inside the script body, so only the FIRST
// occurrence of each name is kept and the scan is bounded to the block that
// ends at the second "#!/bin/sh".
QMap<QByteArray, QByteArray> iaParseAssignments(const QByteArray &baBuffer,
                                                qint64 nLimit)
{
    QMap<QByteArray, QByteArray> mapResult;
    qint64 nOffset = 0;
    const qint64 nEnd = qMin<qint64>(nLimit, baBuffer.size());
    while (nOffset < nEnd) {
        qint64 nLineEnd = baBuffer.indexOf('\n', static_cast<int>(nOffset));
        if (nLineEnd < 0 || nLineEnd > nEnd) nLineEnd = nEnd;
        QByteArray baLine =
            baBuffer.mid(static_cast<int>(nOffset),
                         static_cast<int>(nLineEnd - nOffset));
        nOffset = nLineEnd + 1;
        while (baLine.endsWith('\r')) baLine.chop(1);
        const int nEqual = baLine.indexOf('=');
        // Assignments are emitted at column 0 and unquoted on the left; an
        // indented line belongs to the script body, not to the header block.
        if (nEqual <= 0) continue;
        bool bValidName = true;
        for (int i = 0; i < nEqual; i++) {
            if (!iaIsNameCharacter(baLine.at(i), i == 0)) {
                bValidName = false;
                break;
            }
        }
        if (!bValidName) continue;
        const QByteArray baName = baLine.left(nEqual);
        if (mapResult.contains(baName)) continue;
        QByteArray baValue = baLine.mid(nEqual + 1).trimmed();
        if (baValue.size() >= 2 && baValue.startsWith('"') &&
            baValue.endsWith('"')) {
            baValue = baValue.mid(1, baValue.size() - 2);
        }
        mapResult.insert(baName, baValue);
    }
    return mapResult;
}

// Shell truth semantics: `[ $VAR ]` is false for both an absent and an empty
// variable, so "present" here means present AND non-empty.
bool iaHasValue(const QMap<QByteArray, QByteArray> &mapValues,
                const char *pName)
{
    const QMap<QByteArray, QByteArray>::const_iterator it =
        mapValues.constFind(QByteArray(pName));
    return (it != mapValues.constEnd()) && !it.value().isEmpty();
}

bool iaGetNumber(const QMap<QByteArray, QByteArray> &mapValues,
                 const char *pName, qint64 *pnValue)
{
    const QMap<QByteArray, QByteArray>::const_iterator it =
        mapValues.constFind(QByteArray(pName));
    if ((it == mapValues.constEnd()) || it.value().isEmpty()) return false;
    const QByteArray &baValue = it.value();
    for (int i = 0; i < baValue.size(); i++) {
        if (baValue.at(i) < '0' || baValue.at(i) > '9') return false;
    }
    bool bOk = false;
    const qint64 nValue = baValue.toLongLong(&bOk);
    if (!bOk || nValue < 0) return false;
    *pnValue = nValue;
    return true;
}

qint64 iaCeilBlocks(qint64 nSize, qint64 nBlockSize)
{
    if (nBlockSize <= 0) return -1;
    return (nSize + nBlockSize - 1) / nBlockSize;
}

QString iaParseBannerVersion(const QByteArray &baBuffer)
{
    const int nBanner = baBuffer.indexOf(IA_BANNER);
    if (nBanner < 0) return QString();
    const int nVersion =
        baBuffer.indexOf("Version ", nBanner + qstrlen(IA_BANNER));
    if (nVersion < 0 || nVersion > nBanner + 128) return QString();
    int nStart = nVersion + 8;
    int nEnd = nStart;
    while (nEnd < baBuffer.size()) {
        const char cCharacter = baBuffer.at(nEnd);
        if ((cCharacter >= '0' && cCharacter <= '9') || cCharacter == '.') {
            nEnd++;
        } else {
            break;
        }
    }
    return (nEnd > nStart) ? QString::fromLatin1(baBuffer.mid(nStart,
                                                             nEnd - nStart))
                           : QString();
}
}  // namespace

XInstallAnywhere::XInstallAnywhere(QIODevice *pDevice) : XArchive(pDevice)
{
}

XInstallAnywhere::~XInstallAnywhere()
{
}

bool XInstallAnywhere::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XInstallAnywhere> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < IA_MIN_INPUT_SIZE) return false;

    const qint64 nScanSize = qMin<qint64>(IA_SCAN_SIZE, context.nInputSize);
    const QByteArray baHeader = read_array_process(0, nScanSize, pPdStruct);
    if (!guardedThis || !guardedSource || baHeader.size() != nScanSize) {
        return false;
    }

    // Exactly "#!/bin/sh" + newline.  "#! /bin/sh" (with a space) is the shar
    // generator's spelling and must not be claimed here.
    if (!baHeader.startsWith(IA_SHEBANG)) return false;
    const char cAfterShebang = baHeader.at(static_cast<int>(qstrlen(IA_SHEBANG)));
    if (cAfterShebang != '\n' && cAfterShebang != '\r') return false;
    if (!baHeader.contains(IA_BANNER)) return false;

    // The generator emits a second "#!/bin/sh" right after the variable block;
    // it is the only reliable end marker, since the variable set itself varies
    // by InstallAnywhere version and by web-vs-bundled build.
    const int nSecondShebang = baHeader.indexOf(IA_SHEBANG, 1);
    const qint64 nHeaderEnd =
        (nSecondShebang > 0) ? nSecondShebang : baHeader.size();

    const QMap<QByteArray, QByteArray> mapValues =
        iaParseAssignments(baHeader, nHeaderEnd);

    context.nBlockSize = IA_DEFAULT_BLOCKSIZE;
    if (mapValues.contains(QByteArrayLiteral("BLOCKSIZE"))) {
        if (!iaGetNumber(mapValues, "BLOCKSIZE", &context.nBlockSize)) {
            return false;
        }
    }
    if (!iaIsPowerOfTwo(context.nBlockSize) ||
        context.nBlockSize < IA_MIN_BLOCKSIZE ||
        context.nBlockSize > IA_MAX_BLOCKSIZE) {
        return false;
    }

    qint64 nArchRealSize = 0;
    if (!iaGetNumber(mapValues, "ARCHREALSIZE", &nArchRealSize) ||
        (nArchRealSize <= 0)) {
        return false;
    }

    context.bVMIncluded = iaHasValue(mapValues, "JRESTART");
    context.sResourceDir =
        QString::fromLatin1(mapValues.value(QByteArrayLiteral("RESOURCE_DIR")));

    qint64 nCurrentBlock = 0;
    if (context.bVMIncluded) {
        qint64 nJreStart = 0;
        qint64 nJreRealSize = 0;
        if (!iaGetNumber(mapValues, "JRESTART", &nJreStart) ||
            !iaGetNumber(mapValues, "JREREALSIZE", &nJreRealSize) ||
            (nJreRealSize <= 0)) {
            return false;
        }
        MEMBER member = {};
        member.nDataOffset = nJreStart * context.nBlockSize;
        member.nSize = nJreRealSize;
        member.sFileName = QStringLiteral("vm.tar.Z");
        if (!iaRangeWithin(context.nInputSize, member.nDataOffset,
                           member.nSize)) {
            return false;
        }
        context.listMembers.append(member);
        nCurrentBlock = nJreStart + iaCeilBlocks(nJreRealSize,
                                                 context.nBlockSize);
    } else {
        // Web-installer variant: the script skips straight to $ARCHSTART.  It
        // is not represented in any sample here, so decline cleanly rather
        // than guessing the default block index of the bundled variant.
        if (!iaGetNumber(mapValues, "ARCHSTART", &nCurrentBlock)) return false;
    }

    {
        MEMBER member = {};
        member.nDataOffset = nCurrentBlock * context.nBlockSize;
        member.nSize = nArchRealSize;
        member.sFileName = QStringLiteral("installer.zip");
        if (!iaRangeWithin(context.nInputSize, member.nDataOffset,
                           member.nSize)) {
            return false;
        }
        context.listMembers.append(member);
        nCurrentBlock += iaCeilBlocks(nArchRealSize, context.nBlockSize);
    }

    // Redundant block counts the generator also writes; when present they must
    // agree with the byte lengths, which is what keeps a text file that merely
    // happens to carry these names from parsing as a container.
    qint64 nArchSize = 0;
    if (iaGetNumber(mapValues, "ARCHSIZE", &nArchSize) && (nArchSize > 0) &&
        (iaCeilBlocks(nArchRealSize, context.nBlockSize) != nArchSize)) {
        return false;
    }

    if (iaHasValue(mapValues, "RESSIZE")) {
        qint64 nResSize = 0;
        if (!iaGetNumber(mapValues, "RESSIZE", &nResSize)) return false;
        // RESSIZE=0 makes the script `touch` an empty Resource1.zip; there is
        // no payload to point at, so emit no member for it.
        if (nResSize > 0) {
            qint64 nResRealSize = 0;
            if (!iaGetNumber(mapValues, "RESREALSIZE", &nResRealSize) ||
                (nResRealSize <= 0) ||
                (iaCeilBlocks(nResRealSize, context.nBlockSize) != nResSize)) {
                return false;
            }
            MEMBER member = {};
            member.nDataOffset = nCurrentBlock * context.nBlockSize;
            // The resource dd uses count=$RESSIZE BLOCKS and relies on EOF to
            // truncate the tail; the real length is RESREALSIZE, and
            // RESSIZE*BLOCKSIZE deliberately overshoots the file end.
            member.nSize = nResRealSize;
            member.sFileName = QStringLiteral("Resource1.zip");
            if (!iaRangeWithin(context.nInputSize, member.nDataOffset,
                               member.nSize)) {
                return false;
            }
            context.listMembers.append(member);
        }
    }

    if (context.listMembers.isEmpty()) return false;

    context.nPreambleSize = context.listMembers.first().nDataOffset;
    if (context.nPreambleSize < nHeaderEnd) return false;

    qint64 nPreviousEnd = context.nPreambleSize;
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        const MEMBER &member = context.listMembers.at(i);
        if ((member.nDataOffset < nPreviousEnd) || (member.nSize <= 0)) {
            return false;
        }
        nPreviousEnd = member.nDataOffset + member.nSize;
    }
    context.nArchiveSize = nPreviousEnd;

    // Cheap magic spot-check at the derived offsets.  The arithmetic above is
    // driven entirely by numbers in a text header, so this is the only thing
    // that proves the offsets actually land on payload.
    for (qint32 i = 0; i < context.listMembers.size(); i++) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        const MEMBER &member = context.listMembers.at(i);
        const QByteArray baMagic =
            read_array_process(member.nDataOffset, IA_MAGIC_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource ||
            baMagic.size() != IA_MAGIC_SIZE) {
            return false;
        }
        const quint8 nByte0 = static_cast<quint8>(baMagic.at(0));
        const quint8 nByte1 = static_cast<quint8>(baMagic.at(1));
        if (context.bVMIncluded && (i == 0)) {
            // Named vm.tar.Z but written by gzip in practice; the script tries
            // `gzip -d` first and falls back to `uncompress`, so accept both.
            if (!((nByte0 == 0x1fU) &&
                  ((nByte1 == 0x8bU) || (nByte1 == 0x9dU)))) {
                return false;
            }
        } else if (!baMagic.startsWith("PK")) {
            return false;
        }
    }

    context.sVersion = iaParseBannerVersion(baHeader);

    *pContext = context;
    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XInstallAnywhere::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource && nSavedPosition >= 0) {
        guardedSource->seek(nSavedPosition);
    }
    return bResult;
}

bool XInstallAnywhere::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XInstallAnywhere archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XInstallAnywhere::createInstance(QIODevice *pDevice, bool bIsImage,
                                          XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XInstallAnywhere(pDevice);
}

QList<QString> XInstallAnywhere::getSearchSignatures()
{
    // "#!/bin/sh" at offset 0; the banner is what actually discriminates, but
    // the shebang is the only fixed-offset anchor the signature engine can use.
    return {QStringLiteral("'#!/bin/sh'")};
}

XBinary::FT XInstallAnywhere::getFileType()
{
    return FT_INSTALLANYWHERE_SFX;
}

XBinary::MODE XInstallAnywhere::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XInstallAnywhere::getEndian()
{
    return ENDIAN_UNKNOWN;
}

QString XInstallAnywhere::getArch()
{
    return QString();
}

QString XInstallAnywhere::getFileFormatExt()
{
    return QStringLiteral("bin");
}

QString XInstallAnywhere::getFileFormatExtsString()
{
    return QStringLiteral("InstallAnywhere UNIX Self Extractor (*.bin *.sh)");
}

QString XInstallAnywhere::getMIMEString()
{
    return QStringLiteral("application/x-installanywhere");
}

QString XInstallAnywhere::getVersion()
{
    CONTEXT context = {};
    return parseContext(&context, nullptr) ? context.sVersion : QString();
}

qint64 XInstallAnywhere::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XInstallAnywhere::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XInstallAnywhere::getMemoryMap(MAPMODE mapMode,
                                                    PDSTRUCT *pPdStruct)
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

bool XInstallAnywhere::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return nLimit <= 0 || nCurrentCount < nLimit;
}

QList<XBinary::FPART> XInstallAnywhere::getFileParts(quint32 nFileParts,
                                                     qint32 nLimit,
                                                     PDSTRUCT *pPdStruct)
{
    QList<FPART> result;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return result;

    if ((nFileParts & FILEPART_HEADER) && (context.nPreambleSize > 0) &&
        canAppendPart(nLimit, result.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nPreambleSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Shell preamble");
        result.append(part);
    }

    for (const MEMBER &member : context.listMembers) {
        if (!isPdStructNotCanceled(pPdStruct) ||
            !canAppendPart(nLimit, result.size())) {
            break;
        }
        if (nFileParts & FILEPART_STREAM) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE,
                                      member.nSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD,
                                      HANDLE_METHOD_STORE);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                      QStringLiteral("Stored (dd block slice)"));
            result.append(part);
        }
        if ((nFileParts & FILEPART_REGION) &&
            canAppendPart(nLimit, result.size())) {
            FPART part = {};
            part.filePart = FILEPART_REGION;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nSize;
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
XInstallAnywhere::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XInstallAnywhere::initUnpack(
    UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties,
    PDSTRUCT *pPdStruct)
{
    QPointer<XInstallAnywhere> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource ||
        pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    QString sInfo = QStringLiteral("InstallAnywhere UNIX Self Extractor");
    if (!pContext->sVersion.isEmpty()) {
        sInfo += QStringLiteral(" ") + pContext->sVersion;
    }
    if (!pContext->sResourceDir.isEmpty()) {
        sInfo += QStringLiteral("; ") + pContext->sResourceDir;
    }
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, sInfo);
    pState->nCurrentOffset = pContext->listMembers.first().nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
    pState->pContext = pContext;

    const bool bFinalized =
        guardedThis->validateAndFinalizeUnpackSource(pState, pContext,
                                                     pPdStruct);
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

XBinary::ARCHIVERECORD XInstallAnywhere::infoCurrent(UNPACK_STATE *pState,
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
    if (pState->nCurrentOffset != member.nDataOffset) {
        return ARCHIVERECORD();
    }

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_STORE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD,
                                QStringLiteral("Stored (dd block slice)"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    return result;
}

bool XInstallAnywhere::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
            pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;
    return false;
}

bool XInstallAnywhere::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
