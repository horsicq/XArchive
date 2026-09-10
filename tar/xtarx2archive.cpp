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
#include "xtarx2archive.h"

#include "Algos/xtarx2decoder.h"

#include <QFileInfo>
#include <QPointer>

#include <new>
#include <string.h>
#include <zlib.h>

namespace {
const qint64 TARX2_HEADER_SIZE = 0x10;
const qint64 TARX2_MIN_SIZE = TARX2_HEADER_SIZE + 8;
const qint64 TARX2_MAX_INPUT_SIZE = Q_INT64_C(512) * 1024 * 1024;
// The same ceiling XFilteredArchive puts on a decoded layer.
const qint64 TARX2_MAX_OUTPUT_SIZE = Q_INT64_C(512) * 1024 * 1024;
const qint64 TARX2_CHUNK_SIZE = 0x10000;  // a multiple of the 8-byte cipher block
}  // namespace

XTARX2Archive::XTARX2Archive(QIODevice *pDevice) : XArchive(pDevice), m_nMeasuredInputSize(-1), m_nMeasuredTarSize(-1), m_nMeasuredGeneration(0)
{
}

// Decrypt the payload a chunk at a time straight into the inflater and count
// what comes out, keeping neither the plaintext nor the tar.  The reference's
// 43 MiB sample would otherwise need a second whole-file buffer just to be
// measured, and the answer is needed several times per listing.
bool XTARX2Archive::measureTar(qint64 nOffset, qint64 nSize, qint64 *pnTarSize, PDSTRUCT *pPdStruct)
{
    if (!pnTarSize) return false;
    *pnTarSize = -1;

    QPointer<XTARX2Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;

    const qint64 nCipherSize = (nSize / XTARX2Decoder::BLOCK_SIZE) * XTARX2Decoder::BLOCK_SIZE;
    if (nCipherSize < XTARX2Decoder::BLOCK_SIZE) return false;

    QByteArray baPlain((qint32)TARX2_CHUNK_SIZE, (char)0);
    QByteArray baScratch((qint32)TARX2_CHUNK_SIZE, (char)0);
    if ((baPlain.size() != (qint32)TARX2_CHUNK_SIZE) || (baScratch.size() != (qint32)TARX2_CHUNK_SIZE)) return false;

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit2(&stream, 16 + MAX_WBITS) != Z_OK) return false;

    qint64 nPosition = 0;
    qint64 nProduced = 0;
    bool bFinished = false;
    bool bFailed = false;

    while (!bFinished && !bFailed && (nPosition < nCipherSize)) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            bFailed = true;
            break;
        }
        const qint64 nChunk = qMin<qint64>(TARX2_CHUNK_SIZE, nCipherSize - nPosition);
        const QByteArray baCipher = read_array_process(nOffset + nPosition, nChunk, pPdStruct);
        if (!guardedThis || !guardedSource || ((qint64)baCipher.size() != nChunk)) {
            bFailed = true;
            break;
        }
        if (!XTARX2Decoder::decryptChunk(baCipher.constData(), nChunk, baPlain.data())) {
            bFailed = true;
            break;
        }
        nPosition += nChunk;

        stream.next_in = (Bytef *)baPlain.data();
        stream.avail_in = (uInt)nChunk;
        while ((stream.avail_in > 0) && !bFinished && !bFailed) {
            const uInt nAvailInBefore = stream.avail_in;
            stream.next_out = (Bytef *)baScratch.data();
            stream.avail_out = (uInt)TARX2_CHUNK_SIZE;
            const int nStatus = inflate(&stream, Z_NO_FLUSH);
            nProduced += (qint64)(TARX2_CHUNK_SIZE - stream.avail_out);
            if (nProduced > TARX2_MAX_OUTPUT_SIZE) bFailed = true;
            else if (nStatus == Z_STREAM_END) bFinished = true;
            else if (nStatus != Z_OK) bFailed = true;
            else if ((stream.avail_in == nAvailInBefore) && (stream.avail_out == (uInt)TARX2_CHUNK_SIZE)) bFailed = true;
        }
    }

    inflateEnd(&stream);
    if (bFailed || !bFinished || !guardedThis || !guardedSource) return false;

    *pnTarSize = nProduced;

    return true;
}

XTARX2Archive::~XTARX2Archive()
{
}

bool XTARX2Archive::parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XTARX2Archive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < TARX2_MIN_SIZE) || (context.nInputSize > TARX2_MAX_INPUT_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, TARX2_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || ((qint64)baHeader.size() != TARX2_HEADER_SIZE)) return false;
    if (!XTARX2Decoder::isValidHeader(baHeader)) return false;

    context.nDataOffset = TARX2_HEADER_SIZE;
    context.nCompressedSize = context.nInputSize - TARX2_HEADER_SIZE;
    context.nUncompressedSize = 0;
    context.bSizeKnown = false;

    if (bMeasure) {
        // The size of the record is the size of the TAR, because that is what
        // the decoder emits; see Algos/xtarx2decoder.h for why the gzip layer
        // is not left for XFilteredArchive to unwrap.  Measuring means a
        // complete decrypt-and-inflate pass, so the answer is remembered for
        // the lifetime of this reader - initUnpack() and getFileParts() both
        // ask for it.
        const quint64 nGeneration = getDeviceGeneration();
        qint64 nTarSize = -1;
        if ((m_nMeasuredTarSize >= 0) && (m_nMeasuredInputSize == context.nInputSize) && (m_nMeasuredGeneration == nGeneration)) {
            nTarSize = m_nMeasuredTarSize;
        } else if (measureTar(context.nDataOffset, context.nCompressedSize, &nTarSize, pPdStruct)) {
            if (!guardedThis || !guardedSource) return false;
            m_nMeasuredInputSize = context.nInputSize;
            m_nMeasuredGeneration = nGeneration;
            m_nMeasuredTarSize = nTarSize;
        }
        if (!guardedThis || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;
        if (nTarSize >= 0) {
            context.nUncompressedSize = nTarSize;
            context.bSizeKnown = true;
        }
    }

    QString sName = QFileInfo(getDeviceFileName(guardedSource.data())).completeBaseName();
    if (!guardedThis || !guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("archive");
    context.sFileName = sName + QStringLiteral(".tar");

    *pContext = context;

    return true;
}

bool XTARX2Archive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XTARX2Archive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XTARX2Archive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XTARX2Archive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XTARX2Archive(pDevice);
}

QList<QString> XTARX2Archive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("7876957d........9c3bad45");
}

XBinary::FT XTARX2Archive::getFileType()
{
    return FT_TARX2;
}

XBinary::MODE XTARX2Archive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XTARX2Archive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XTARX2Archive::getArch()
{
    return QString();
}

qint32 XTARX2Archive::getType()
{
    return TYPE_ARCHIVE;
}

QString XTARX2Archive::getFileFormatExt()
{
    return QStringLiteral("tarx");
}

QString XTARX2Archive::getFileFormatExtsString()
{
    return QStringLiteral("TARX 2 (*.tarx)");
}

QString XTARX2Archive::getMIMEString()
{
    return QStringLiteral("application/x-tarx");
}

QString XTARX2Archive::getVersion()
{
    return QStringLiteral("2");
}

qint64 XTARX2Archive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XTARX2Archive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XTARX2Archive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XTARX2Archive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XTARX2Archive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    const bool bNeedStream = ((nFileParts & FILEPART_STREAM) != 0);
    if (!parseContext(&context, bNeedStream, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = TARX2_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = context.nDataOffset;
        part.nFileSize = context.nCompressedSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nCompressedSize);
        if (context.bSizeKnown) part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, context.bSizeKnown ? HANDLE_METHOD_TARX2 : HANDLE_METHOD_UNKNOWN);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Blowfish/Gzip"));
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_DATA) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_DATA;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Data");
        listResult.append(part);
    }

    return listResult;
}

QMap<XBinary::UNPACK_PROP, QVariant> XTARX2Archive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XTARX2Archive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XTARX2Archive> guardedThis(this);
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
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->nDataOffset;
    pState->nTotalSize = pContext->nInputSize;
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

XBinary::ARCHIVERECORD XTARX2Archive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != pContext->nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    if (pContext->bSizeKnown) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->bSizeKnown ? HANDLE_METHOD_TARX2 : HANDLE_METHOD_UNKNOWN);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Blowfish/Gzip"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XTARX2Archive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XTARX2Archive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    Q_UNUSED(pPdStruct)
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;

    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    releaseUnpackSource(pState);
    pState->pContext = nullptr;
    delete pContext;
    *pState = UNPACK_STATE();

    return true;
}

QList<XBinary::FPART_PROP> XTARX2Archive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
