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
#include "xsilmarils.h"

#include "Algos/xsilmarilsdecoder.h"

#include <QFileInfo>
#include <QPointer>

#include <cstring>
#include <new>

namespace {

const qint32 SILMARILS_SHORT_HEADER_SIZE = 6;
const qint32 SILMARILS_LONG_HEADER_SIZE = 14;
const qint32 SILMARILS_TABLE_OFFSET = 6;
const qint32 SILMARILS_TABLE_SIZE = 8;

const quint32 SILMARILS_VERSION = 1;

// The 0xA1 parameter block never varies: all 135 bit-stream members of the
// reference corpus carry these exact eight bytes.  It is the only thing that
// makes that method's header strong enough to detect on.
const quint8 SILMARILS_CODE_TABLE[SILMARILS_TABLE_SIZE] = {0x0B, 0x09, 0x0A, 0x0B, 0x07, 0x05, 0x06, 0x07};

// The size field is 24 bits, so nothing beyond this can be described; the file
// cap keeps the byte-run trial walk in isValid bounded.
const qint64 SILMARILS_MAX_RAW_SIZE = Q_INT64_C(0x00FFFFFF);
const qint64 SILMARILS_MAX_FILE_SIZE = Q_INT64_C(64) * 1024 * 1024;

const QString SILMARILS_FALLBACK_NAME = QStringLiteral("silmarils.bin");

quint32 silmarilsReadU32(const QByteArray &baData, qint32 nOffset, bool bBigEndian)
{
    const quint8 b0 = static_cast<quint8>(baData.at(nOffset));
    const quint8 b1 = static_cast<quint8>(baData.at(nOffset + 1));
    const quint8 b2 = static_cast<quint8>(baData.at(nOffset + 2));
    const quint8 b3 = static_cast<quint8>(baData.at(nOffset + 3));

    if (bBigEndian) {
        return (static_cast<quint32>(b0) << 24) | (static_cast<quint32>(b1) << 16) | (static_cast<quint32>(b2) << 8) | static_cast<quint32>(b3);
    }

    return (static_cast<quint32>(b3) << 24) | (static_cast<quint32>(b2) << 16) | (static_cast<quint32>(b1) << 8) | static_cast<quint32>(b0);
}

}  // namespace

XSilmarils::XSilmarils(QIODevice *pDevice) : XArchive(pDevice)
{
}

QString XSilmarils::deriveMemberName()
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return SILMARILS_FALLBACK_NAME;

    const QString sDeviceName = XBinary::getDeviceFileName(guardedSource.data());
    if (sDeviceName.isEmpty()) return SILMARILS_FALLBACK_NAME;

    const QString sFileName = QFileInfo(sDeviceName).fileName();
    if (sFileName.isEmpty()) return SILMARILS_FALLBACK_NAME;

    return sFileName;
}

bool XSilmarils::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

XBinary::HANDLE_METHOD XSilmarils::handleMethodOf(const CONTEXT &context)
{
    if (context.nMethod == static_cast<quint32>(METHOD_BYTERUN)) {
        return HANDLE_METHOD_SILMARILS;
    }

    // 0xA1 is a real, understood container carrying a codec this build cannot
    // decode.  Reporting UNKNOWN makes extraction refuse loudly instead of
    // publishing whatever a wrong decoder happens to produce.
    return HANDLE_METHOD_UNKNOWN;
}

QString XSilmarils::reportedMethodOf(const CONTEXT &context)
{
    if (context.nMethod == static_cast<quint32>(METHOD_BYTERUN)) {
        return QStringLiteral("Silmarils byte-run (0x81)");
    }

    return QStringLiteral("Silmarils bitstream (0xA1, unsupported)");
}

bool XSilmarils::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSilmarils> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = getSize();
    if (!guardedThis || !guardedSource) return false;
    if ((context.nInputSize <= SILMARILS_LONG_HEADER_SIZE) || (context.nInputSize > SILMARILS_MAX_FILE_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, SILMARILS_LONG_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SILMARILS_LONG_HEADER_SIZE)) return false;

    // The version word is the byte-order oracle; nothing else in the header is
    // constant across both builds.
    const quint8 nVer0 = static_cast<quint8>(baHeader.at(4));
    const quint8 nVer1 = static_cast<quint8>(baHeader.at(5));

    if ((nVer0 == 0x01U) && (nVer1 == 0x00U)) {
        context.bBigEndian = false;
    } else if ((nVer0 == 0x00U) && (nVer1 == 0x01U)) {
        context.bBigEndian = true;
    } else {
        return false;
    }

    context.nVersion = SILMARILS_VERSION;

    const quint32 nPacked = silmarilsReadU32(baHeader, 0, context.bBigEndian);
    context.nMethod = nPacked >> 24;

    const qint64 nRawSize = static_cast<qint64>(nPacked & 0x00FFFFFFU);

    if ((context.nMethod != static_cast<quint32>(METHOD_BYTERUN)) && (context.nMethod != static_cast<quint32>(METHOD_BITSTREAM))) {
        return false;
    }

    // rawSize counts the six-byte header, so anything at or below it describes
    // an empty member and is not a container this class should claim.
    if ((nRawSize <= SILMARILS_SHORT_HEADER_SIZE) || (nRawSize > SILMARILS_MAX_RAW_SIZE)) return false;

    context.nUncompressedSize = nRawSize - SILMARILS_SHORT_HEADER_SIZE;

    // Every member of the reference corpus - both methods, both byte orders -
    // decodes to a whole number of 8-byte resource units.  It is a cheap, very
    // discriminating extra constraint on a header this thin.
    if (context.nUncompressedSize % 8) return false;

    if (context.nMethod == static_cast<quint32>(METHOD_BITSTREAM)) {
        if (std::memcmp(baHeader.constData() + SILMARILS_TABLE_OFFSET, SILMARILS_CODE_TABLE, static_cast<size_t>(SILMARILS_TABLE_SIZE)) != 0) {
            return false;
        }
        context.nHeaderSize = SILMARILS_LONG_HEADER_SIZE;
        context.bSupported = false;
    } else {
        context.nHeaderSize = SILMARILS_SHORT_HEADER_SIZE;
        context.bSupported = true;
    }

    context.nStreamOffset = context.nHeaderSize;
    context.nStreamSize = context.nInputSize - context.nHeaderSize;
    if (context.nStreamSize <= 0) return false;

    if (context.bSupported) {
        // Structural ceiling on the byte-run codec: a run token spends two
        // input bytes per output byte at worst, a literal run one count byte
        // per 127, and the final literal run may overshoot by at most 127.
        // Anything fatter cannot be this stream, and the check also bounds what
        // the trial walk below is allowed to read during detection.
        if (context.nStreamSize > (2 * context.nUncompressedSize + 128)) return false;

        // A six-byte header with a version word is far too weak to hand an
        // arbitrary file to a decoder.  Walk the real token grammar: it has to
        // produce exactly the declared plaintext length and land exactly on the
        // last input byte.  That is what keeps this class from stealing files
        // and, equally, from being stolen from.
        const QByteArray baStream = read_array_process(context.nStreamOffset, context.nStreamSize, pPdStruct);
        if (!guardedThis || !guardedSource || (static_cast<qint64>(baStream.size()) != context.nStreamSize)) return false;

        if (!XSilmarilsDecoder::probe(baStream, context.nUncompressedSize)) return false;
    }

    context.sFileName = deriveMemberName();
    if (!guardedThis || !guardedSource) return false;

    *pContext = context;

    return isPdStructNotCanceled(pPdStruct);
}

bool XSilmarils::isValid(PDSTRUCT *pPdStruct)
{
    // Detection runs on a device the caller still owns: snapshot the cursor and
    // put it back whatever the outcome.
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    return bResult;
}

bool XSilmarils::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSilmarils archive(pDevice);

    return archive.isValid(pPdStruct);
}

XBinary *XSilmarils::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    return new XSilmarils(pDevice);
}

QList<QString> XSilmarils::getSearchSignatures()
{
    // Only the 0xA1 method carries enough constant bytes to scan for: the
    // method byte plus the version word plus the fixed parameter block, in both
    // byte orders.  The 0x81 method has no signature at all and is reached
    // through isValid only.
    return {QStringLiteral("......A101000B090A0B07050607"), QStringLiteral("A1......00010B090A0B07050607")};
}

XBinary::FT XSilmarils::getFileType()
{
    return FT_SILMARILS;
}

XBinary::MODE XSilmarils::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSilmarils::getEndian()
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, nullptr);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (bResult && context.bBigEndian) return ENDIAN_BIG;

    return ENDIAN_LITTLE;
}

QString XSilmarils::getArch()
{
    return QString();
}

qint32 XSilmarils::getType()
{
    return XArchive::TYPE_ARCHIVE;
}

QString XSilmarils::getFileFormatExt()
{
    return QStringLiteral("io");
}

QString XSilmarils::getFileFormatExtsString()
{
    return QStringLiteral("Silmarils resource (*.io *.co *.do)");
}

QString XSilmarils::getMIMEString()
{
    return QStringLiteral("application/x-silmarils");
}

QString XSilmarils::getVersion()
{
    QPointer<QIODevice> guardedSource(getDevice());
    const qint64 nSavedPosition = guardedSource ? guardedSource->pos() : -1;

    CONTEXT context = {};
    const bool bResult = parseContext(&context, nullptr);

    if (guardedSource && (nSavedPosition >= 0)) {
        guardedSource->seek(nSavedPosition);
    }

    if (!bResult) return QString();

    return QString::number(static_cast<qint32>(context.nVersion));
}

qint64 XSilmarils::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};

    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XSilmarils::getMapModesList()
{
    return {MAPMODE_REGIONS, MAPMODE_STREAMS, MAPMODE_DATA};
}

XBinary::_MEMORY_MAP XSilmarils::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART> XSilmarils::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;

    if ((nLimit < -1) || (nLimit == 0) || (nFileParts == 0)) return listResult;

    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_HEADER;
        record.nFileOffset = 0;
        record.nFileSize = context.nHeaderSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Header");
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_STREAM;
        record.nFileOffset = context.nStreamOffset;
        record.nFileSize = context.nStreamSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        record.mapProperties.insert(FPART_PROP_ORIGINALNAME, context.sFileName);
        record.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nStreamSize);
        record.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        record.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethodOf(context));
        record.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethodOf(context));
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_REGION) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_REGION;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = context.sFileName;
        listResult.append(record);
    }

    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART record = {};
        record.filePart = FILEPART_DATA;
        record.nFileOffset = 0;
        record.nFileSize = context.nInputSize;
        record.nVirtualAddress = XADDR_MAX;
        record.sName = tr("Data");
        listResult.append(record);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XSilmarils::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSilmarils::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSilmarils> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());

    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) {
        return false;
    }
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) {
        return false;
    }
    if (!finishUnpack(pState, nullptr) || !guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) {
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
        if (guardedThis) guardedThis->releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->mapArchiveProperties.insert(FPART_PROP_INFO, tr("Silmarils resource; single member"));
    pState->nCurrentOffset = pContext->nStreamOffset;
    pState->nTotalSize = pContext->nInputSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = 1;
    pState->pContext = pContext;

    // Binding only stages the source.  Without this finalize the listing works
    // and every extraction silently writes nothing.
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

XBinary::ARCHIVERECORD XSilmarils::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nStreamOffset;
    result.nStreamSize = pContext->nStreamSize;

    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, handleMethodOf(*pContext));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, reportedMethodOf(*pContext));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XSilmarils::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }

    CONTEXT *pContext = static_cast<CONTEXT *>(pState->pContext);
    if (!pContext) return false;

    // The index must move PAST the last record; stopping one short makes both
    // the GUI and the CLI list nothing at all.
    pState->nCurrentIndex++;

    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->nStreamOffset;
        return true;
    }

    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XSilmarils::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
