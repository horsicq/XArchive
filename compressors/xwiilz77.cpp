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
#include "xwiilz77.h"

#include "Algos/xnintendolzdecoder.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 LZ77_TAG_SIZE = 4;
const qint64 LZ77_PROBE_SIZE = 12;  // tag + extended header
const quint8 LZ77_TYPE_LZ10 = 0x10U;
const quint8 LZ77_TYPE_LZ11 = 0x11U;
// IMD5 wrapper: "IMD5", u32 BE data size, 8 reserved bytes, 16-byte MD5.
const qint64 LZ77_IMD5_SIZE = 0x20;
const qint64 LZ77_IMD5_SIZE_OFFSET = 4;
// 4 tag + 4 header + 1 flag + 1 literal.
const qint64 LZ77_MIN_INPUT_TAGGED = 10;
// Without the tag the header is a single byte of magic; a toy stream must not
// be enough (see the header comment).
const qint64 LZ77_MIN_INPUT_BARE = 12;
const qint64 LZ77_MIN_PLAINTEXT_BARE = 16;
// A flag byte plus at least one literal: nothing shorter can be a stream.
const qint64 LZ77_MIN_STREAM_SIZE = 2;
// Same ceiling as LGCOMPRESS_MAX_UNCOMPRESSED_SIZE: the plaintext length comes
// out of the file, so the decoder must stay bounded.
const qint64 LZ77_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(512) * 1024 * 1024;
// Exact expansion bounds: LZ10's densest group is 1 flag + 8 two-byte
// references = 17 bytes for 8 x 18 = 144 plaintext bytes; LZ11's is 1 flag +
// 8 four-byte references = 33 bytes for 8 x 0x10110 = 526464.  The slack
// covers streams so short that the first (always literal) item dominates.
const qint64 LZ77_MAX_RATIO_LZ10 = 9;
const qint64 LZ77_MAX_RATIO_LZ11 = 16384;
const qint64 LZ77_RATIO_SLACK = 64;
// Trailing bytes tolerated behind the stream: tools align a tagged file to 4
// or 16 with any content; behind a bare stream only ndspy's unused-slot zeros
// (at most 7) plus alignment to 4 are known, all zero, and nlzss11's single
// 0xFF (the flag byte of a next group that never got an item, written only
// when the stream ends exactly on a group boundary - measured on 1.8).
const qint64 LZ77_MAX_TRAILING_TAGGED = 16;
const qint64 LZ77_MAX_TRAILING_BARE = 10;
const quint8 LZ77_NLZSS11_TRAILER = 0xFFU;

bool lz77IsImd5Wrapped(const uchar *pProbe, qint64 nProbeSize)
{
    if (nProbeSize < LZ77_IMD5_SIZE + LZ77_TAG_SIZE + 1) return false;
    if ((pProbe[0] != 'I') || (pProbe[1] != 'M') || (pProbe[2] != 'D') || (pProbe[3] != '5')) return false;
    const uchar *pTag = pProbe + LZ77_IMD5_SIZE;
    if ((pTag[0] != 'L') || (pTag[1] != 'Z') || (pTag[2] != '7') || (pTag[3] != '7')) return false;
    const uchar nType = pTag[LZ77_TAG_SIZE];

    return (nType == LZ77_TYPE_LZ10) || (nType == LZ77_TYPE_LZ11);
}

// The bare form's trailer: zeros only, optionally opened by nlzss11's 0xFF.
bool lz77IsBareTrailer(const uchar *pData, qint64 nSize)
{
    qint64 i = 0;
    if ((nSize > 0) && (pData[0] == LZ77_NLZSS11_TRAILER)) i = 1;
    for (; i < nSize; ++i) {
        if (pData[i] != 0) return false;
    }

    return true;
}
}  // namespace

XWiiLZ77::XWiiLZ77(QIODevice *pDevice) : XArchive(pDevice)
{
}

XWiiLZ77::~XWiiLZ77()
{
}

QString XWiiLZ77::restoreFileName(const QString &sContainerName)
{
    if (sContainerName.isEmpty()) return QStringLiteral("lz77_data");

    // One real-world suffix of this family is dropped, nothing else is touched:
    // "opening.bnr.lz77" -> "opening.bnr", "banner.arc" stays "banner.arc".
    const QString arrSuffixes[] = {QStringLiteral(".lz77"), QStringLiteral(".lz10"), QStringLiteral(".lz11"), QStringLiteral(".lz"), QStringLiteral(".cmp")};
    const qint32 nNumberOfSuffixes = 5;

    for (qint32 i = 0; i < nNumberOfSuffixes; ++i) {
        const QString &sSuffix = arrSuffixes[i];
        if ((sContainerName.size() > sSuffix.size()) && sContainerName.endsWith(sSuffix, Qt::CaseInsensitive)) {
            QString sResult = sContainerName;
            sResult.chop(sSuffix.size());
            return sResult;
        }
    }

    return sContainerName;
}

bool XWiiLZ77::parseContext(CONTEXT *pContext, bool bVerifyPayload, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XWiiLZ77> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < LZ77_MIN_INPUT_TAGGED) return false;

    const qint64 nProbeSize = qMin(context.nInputSize, LZ77_IMD5_SIZE + LZ77_PROBE_SIZE);
    const QByteArray baProbe = read_array_process(0, nProbeSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baProbe.size() != nProbeSize)) return false;
    const uchar *pProbe = reinterpret_cast<const uchar *>(baProbe.constData());

    // The IMD5 wrapper only ever carries the tagged form; the bare form is not
    // looked for behind it (a one-byte magic at 0x20 would be noise).
    if (lz77IsImd5Wrapped(pProbe, nProbeSize)) {
        context.bImd5 = true;
        context.nBaseOffset = LZ77_IMD5_SIZE;
        context.nImd5DeclaredSize = static_cast<qint64>(qFromBigEndian<quint32>(pProbe + LZ77_IMD5_SIZE_OFFSET));
    }

    XNintendoLZDecoder::HEADER header = {};
    if (!XNintendoLZDecoder::parseHeader(reinterpret_cast<const quint8 *>(pProbe + context.nBaseOffset), nProbeSize - context.nBaseOffset, &header)) {
        return false;
    }
    if (context.bImd5 && !header.bHasTag) return false;

    context.bHasTag = header.bHasTag;
    context.bExtendedLength = header.bExtendedLength;
    context.nType = static_cast<quint8>(header.variant);
    context.nHeaderSize = context.nBaseOffset + header.nHeaderSize;
    context.nDataOffset = context.nHeaderSize;
    context.nCompressedSize = context.nInputSize - context.nDataOffset;
    context.nUncompressedSize = header.nUncompressedSize;

    const bool bBare = !context.bHasTag;
    const qint64 nMinInput = bBare ? LZ77_MIN_INPUT_BARE : (context.nBaseOffset + LZ77_MIN_INPUT_TAGGED);
    if (context.nInputSize < nMinInput) return false;
    if (context.nCompressedSize < LZ77_MIN_STREAM_SIZE) return false;

    // A zero plaintext length would write an empty file at exit 0 instead of
    // failing (parseHeader already refuses it); the ceiling bounds the decoder.
    if ((context.nUncompressedSize < 1) || (context.nUncompressedSize > LZ77_MAX_UNCOMPRESSED_SIZE)) return false;
    if (bBare && (context.nUncompressedSize < LZ77_MIN_PLAINTEXT_BARE)) return false;

    // Upper bound on what a stream can consume: the all-literal worst case is
    // one packed byte per plaintext byte plus one flag byte per eight items
    // (every reference form costs at most 4 bytes for at least 3 bytes of
    // output, so nothing is denser).  A tail longer than that plus the
    // tolerated trailer can never pass the trailing check, so it is refused
    // here, before the whole tail is read into memory; with the plaintext
    // already capped at 512 MiB this also keeps the ratio multiply below 2^44.
    const qint64 nMaxTrailing = bBare ? LZ77_MAX_TRAILING_BARE : LZ77_MAX_TRAILING_TAGGED;
    const qint64 nMaxConsumable = context.nUncompressedSize + ((context.nUncompressedSize + 7) / 8);
    if (context.nCompressedSize > nMaxConsumable + nMaxTrailing) return false;

    const qint64 nMaxRatio = (context.nType == LZ77_TYPE_LZ11) ? LZ77_MAX_RATIO_LZ11 : LZ77_MAX_RATIO_LZ10;
    if (context.nUncompressedSize > (context.nCompressedSize * nMaxRatio) + LZ77_RATIO_SLACK) return false;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (!guardedThis || !guardedSource) return false;
    QString sContainerName;
    if (!sDeviceName.isEmpty()) {
        sContainerName = QFileInfo(sDeviceName).fileName();
    }
    context.sFileName = restoreFileName(sContainerName);

    if (bVerifyPayload) {
        const QByteArray baPacked = read_array_process(context.nDataOffset, context.nCompressedSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baPacked.size() != context.nCompressedSize)) return false;

        // The whole gate: the stream has to reproduce the declared length
        // exactly, with no truncation, no reference before the start and no
        // reference past the end.  The scan keeps a 4 KiB ring, not the output.
        qint64 nConsumed = 0;
        if (!XNintendoLZDecoder::scan(reinterpret_cast<const quint8 *>(baPacked.constData()), context.nCompressedSize, header.variant, context.nUncompressedSize,
                                      &nConsumed, pPdStruct)) {
            return false;
        }
        if (!guardedThis || !guardedSource) return false;
        if ((nConsumed < LZ77_MIN_STREAM_SIZE) || (nConsumed > context.nCompressedSize)) return false;

        const qint64 nTrailing = context.nCompressedSize - nConsumed;
        if (bBare) {
            if (nTrailing > LZ77_MAX_TRAILING_BARE) return false;
            if (!lz77IsBareTrailer(reinterpret_cast<const uchar *>(baPacked.constData()) + nConsumed, nTrailing)) return false;
        } else {
            if (nTrailing > LZ77_MAX_TRAILING_TAGGED) return false;
        }
        context.nConsumedSize = nConsumed;
    }

    // The trailing slack is the format's own padding, so the container extends
    // to the end of the input.
    context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return guardedThis && guardedSource && isPdStructNotCanceled(pPdStruct);
}

bool XWiiLZ77::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;
    CONTEXT context = {};
    // One to five bytes of magic and no checksum: only the trial decode that
    // reproduces the declared plaintext length keeps this class from claiming
    // unrelated files and writing garbage at exit 0.
    const bool bResult = parseContext(&context, true, pPdStruct);
    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XWiiLZ77::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XWiiLZ77 archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XWiiLZ77::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XWiiLZ77(pDevice);
}

QList<QString> XWiiLZ77::getSearchSignatures()
{
    // Tagged form only: "LZ77" followed by the type byte.  The bare form's
    // one-byte magic would be noise.
    return QList<QString>() << QStringLiteral("4C5A373710") << QStringLiteral("4C5A373711");
}

XBinary::FT XWiiLZ77::getFileType()
{
    return FT_WII_LZ77;
}

XBinary::MODE XWiiLZ77::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XWiiLZ77::getEndian()
{
    // The header word is little endian; the big-endian bit order of the
    // references is a codec detail, not a file property.
    return ENDIAN_LITTLE;
}

QString XWiiLZ77::getArch()
{
    return QString();
}

qint32 XWiiLZ77::getType()
{
    return TYPE_ARCHIVE;
}

QString XWiiLZ77::getFileFormatExt()
{
    return QStringLiteral("lz77");
}

QString XWiiLZ77::getFileFormatExtsString()
{
    return QStringLiteral("Nintendo LZ77 (*.lz77 *.lz *.lz10 *.lz11)");
}

QString XWiiLZ77::getMIMEString()
{
    return QStringLiteral("application/x-nintendo-lz77");
}

QString XWiiLZ77::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();

    return (context.nType == LZ77_TYPE_LZ11) ? QStringLiteral("LZ11") : QStringLiteral("LZ10");
}

qint64 XWiiLZ77::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XWiiLZ77::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XWiiLZ77::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XWiiLZ77::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XWiiLZ77::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    // Strong parse: the plaintext length published here must be one the
    // decoder can reproduce.
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    const HANDLE_METHOD handleMethod = (context.nType == LZ77_TYPE_LZ11) ? HANDLE_METHOD_NINTENDO_LZ11 : HANDLE_METHOD_NINTENDO_LZ10;
    const QString sReportedMethod = (context.nType == LZ77_TYPE_LZ11) ? QStringLiteral("Nintendo LZ11") : QStringLiteral("Nintendo LZ10");

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nHeaderSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nCompressedSize);
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethod);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, sReportedMethod);
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && context.bImd5 && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = 0;
        part.nFileSize = LZ77_IMD5_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = QStringLiteral("IMD5");
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_REGION;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        listResult.append(part);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, static_cast<qint32>(listResult.size()))) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nArchiveSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XWiiLZ77::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XWiiLZ77::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XWiiLZ77> guardedThis(this);
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
    // bVerifyPayload = true: the decoder takes the plaintext length as an
    // INPUT, so a length it cannot reproduce would fail late or truncate.
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    QString sInfo = (pContext->nType == LZ77_TYPE_LZ11) ? tr("Nintendo LZ77 compressed file; LZ11 payload") : tr("Nintendo LZ77 compressed file; LZ10 payload");
    if (pContext->bImd5) {
        sInfo += QStringLiteral("; ") + tr("IMD5 wrapper, declared size %1").arg(QString::number(pContext->nImd5DeclaredSize));
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, sInfo);
    pState->nCurrentOffset = pContext->nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
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

XBinary::ARCHIVERECORD XWiiLZ77::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nDataOffset) return ARCHIVERECORD();

    const HANDLE_METHOD handleMethod = (pContext->nType == LZ77_TYPE_LZ11) ? HANDLE_METHOD_NINTENDO_LZ11 : HANDLE_METHOD_NINTENDO_LZ10;
    const QString sReportedMethod = (pContext->nType == LZ77_TYPE_LZ11) ? QStringLiteral("Nintendo LZ11") : QStringLiteral("Nintendo LZ10");

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethod);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, sReportedMethod);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);
    // No checksum and no time stamp property: the container carries neither.

    return result;
}

bool XWiiLZ77::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // Mode A: the single record is the last one, so the cursor moves to the
    // end of the container and the call reports "no further record".
    ++pState->nCurrentIndex;
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XWiiLZ77::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XWiiLZ77::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
