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
#include "xsfxgziparchive.h"

#include "Algos/include/zlib.h"

#include <QFileInfo>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 SFXGZIP_MIN_MEMBER_SIZE = 18;
const qint64 SFXGZIP_MAX_HEADER_SIZE = 0x10000;
const qint32 SFXGZIP_MAX_CANDIDATES = 4096;
const qint32 SFXGZIP_MAX_TRIALS = 64;
const qint64 SFXGZIP_TRIAL_CHUNK = 0x10000;
// A detection-time inflate must terminate; no reference member comes close to
// this, and a stream that does is reported as unusable rather than expanded.
const qint64 SFXGZIP_MAX_UNCOMPRESSED_SIZE = Q_INT64_C(0x80000000);

bool sfxGzipRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// The classic gzip test: 0x1f 0x8b, method 8 and no reserved flag bit.  Returns
// the offset of the deflate stream and the FNAME field when present.
bool sfxGzipParseHeader(const QByteArray &baHeader, qint64 *pnDataRelative, QString *psFileName)
{
    if (!pnDataRelative || !psFileName) return false;
    const qint64 nSize = baHeader.size();
    if (nSize < 10) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if ((pHeader[0] != 0x1f) || (pHeader[1] != 0x8b) || (pHeader[2] != 8)) return false;
    const quint8 nFlags = pHeader[3];
    if ((nFlags & 0xe0) != 0) return false;

    qint64 nPosition = 10;
    if (nFlags & 0x04) {  // FEXTRA
        if ((nPosition + 2) > nSize) return false;
        const qint64 nExtraLength = (qint64)qFromLittleEndian<quint16>(pHeader + nPosition);
        nPosition += 2 + nExtraLength;
        if (nPosition > nSize) return false;
    }
    if (nFlags & 0x08) {  // FNAME
        QByteArray baName;
        while (true) {
            if (nPosition >= nSize) return false;
            const char cByte = (char)pHeader[nPosition++];
            if (cByte == (char)0) break;
            if (baName.size() < 4096) baName.append(cByte);
        }
        *psFileName = QString::fromLatin1(baName);
    }
    if (nFlags & 0x10) {  // FCOMMENT
        while (true) {
            if (nPosition >= nSize) return false;
            if ((char)pHeader[nPosition++] == (char)0) break;
        }
    }
    if (nFlags & 0x02) {  // FHCRC
        nPosition += 2;
        if (nPosition > nSize) return false;
    }

    *pnDataRelative = nPosition;

    return true;
}

}  // namespace

XSFXGzipArchive::XSFXGzipArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSFXGzipArchive::~XSFXGzipArchive()
{
}

bool XSFXGzipArchive::tryMemberAt(qint64 nOffset, qint64 nInputSize, CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext) return false;
    QIODevice *guardedSource = getDevice();
    if (!sfxGzipRangeWithin(nInputSize, nOffset, SFXGZIP_MIN_MEMBER_SIZE)) return false;

    const qint64 nHeaderSize = qMin(SFXGZIP_MAX_HEADER_SIZE, nInputSize - nOffset);
    const QByteArray baHeader = read_array_process(nOffset, nHeaderSize, pPdStruct);
    if ((baHeader.size() != nHeaderSize)) return false;

    qint64 nDataRelative = 0;
    QString sFileName;
    if (!sfxGzipParseHeader(baHeader, &nDataRelative, &sFileName)) return false;

    const qint64 nDataOffset = nOffset + nDataRelative;
    if (!sfxGzipRangeWithin(nInputSize, nDataOffset, 1)) return false;

    // Raw deflate: the gzip wrapper has already been consumed above.
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) return false;

    QByteArray baOutput;
    baOutput.resize((qint32)SFXGZIP_TRIAL_CHUNK);
    if (baOutput.size() != SFXGZIP_TRIAL_CHUNK) {
        inflateEnd(&stream);
        return false;
    }

    qint64 nInputPosition = nDataOffset;
    qint64 nConsumed = 0;
    qint64 nProduced = 0;
    bool bDone = false;
    bool bFailed = false;

    while (!bDone && !bFailed) {
        if (!isPdStructNotCanceled(pPdStruct)) {
            bFailed = true;
            break;
        }
        const qint64 nAvailable = qMin(SFXGZIP_TRIAL_CHUNK, nInputSize - nInputPosition);
        if (nAvailable <= 0) {
            bFailed = true;
            break;
        }
        const QByteArray baInput = read_array_process(nInputPosition, nAvailable, pPdStruct);
        if ((baInput.size() != nAvailable)) {
            bFailed = true;
            break;
        }
        nInputPosition += nAvailable;

        stream.next_in = (Bytef *)baInput.constData();
        stream.avail_in = (uInt)nAvailable;
        while ((stream.avail_in > 0) && !bDone && !bFailed) {
            stream.next_out = (Bytef *)baOutput.data();
            stream.avail_out = (uInt)SFXGZIP_TRIAL_CHUNK;
            const int nStatus = inflate(&stream, Z_NO_FLUSH);
            nProduced += (qint64)(SFXGZIP_TRIAL_CHUNK - (qint64)stream.avail_out);
            if (nProduced > SFXGZIP_MAX_UNCOMPRESSED_SIZE) bFailed = true;
            if (nStatus == Z_STREAM_END) {
                bDone = true;
            } else if (nStatus != Z_OK) {
                bFailed = true;
            }
        }
        nConsumed = (qint64)stream.total_in;
    }
    nConsumed = (qint64)stream.total_in;
    inflateEnd(&stream);

    if (bFailed || !bDone || (nProduced <= 0) || (nConsumed <= 0)) return false;
    if (!sfxGzipRangeWithin(nInputSize, nDataOffset, nConsumed)) return false;

    pContext->nStubSize = nOffset;
    pContext->nDataOffset = nDataOffset;
    pContext->nCompressedSize = nConsumed;
    pContext->nUncompressedSize = nProduced;
    pContext->sFileName = sFileName;

    return true;
}

bool XSFXGzipArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;
    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (SFXGZIP_MIN_MEMBER_SIZE + 1)) return false;

    // Offset zero is a plain gzip file and belongs to that reader.
    qint64 nSearchOffset = 1;
    qint32 nCandidates = 0;
    qint32 nTrials = 0;
    bool bFound = false;

    while ((nCandidates < SFXGZIP_MAX_CANDIDATES) && (nTrials < SFXGZIP_MAX_TRIALS)) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nSearchOffset + SFXGZIP_MIN_MEMBER_SIZE) > context.nInputSize) break;

        const qint64 nHit = find_array(nSearchOffset, context.nInputSize - nSearchOffset, "\x1f\x8b\x08", 3, pPdStruct);
        if (nHit < 0) break;
        ++nCandidates;
        nSearchOffset = nHit + 1;

        ++nTrials;
        if (tryMemberAt(nHit, context.nInputSize, &context, pPdStruct)) {
            bFound = true;
            break;
        }
    }

    if (!bFound) return false;

    if (context.sFileName.isEmpty()) {
        // No FNAME field: the reference falls back to the archive's own name
        // with the extension dropped.
        context.sFileName = QFileInfo(getDeviceFileName(guardedSource)).completeBaseName();
    }
    if (context.sFileName.isEmpty()) context.sFileName = QStringLiteral("sfxgzip");

    *pContext = context;

    return true;
}

bool XSFXGzipArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XSFXGzipArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSFXGzipArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSFXGzipArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSFXGzipArchive(pDevice);
}

QList<QString> XSFXGzipArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("1F8B08");
}

XBinary::FT XSFXGzipArchive::getFileType()
{
    return FT_SFXGZIP;
}

XBinary::MODE XSFXGzipArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSFXGzipArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSFXGzipArchive::getArch()
{
    return QString();
}

qint32 XSFXGzipArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XSFXGzipArchive::getFileFormatExt()
{
    return QStringLiteral("exe");
}

QString XSFXGzipArchive::getFileFormatExtsString()
{
    return QStringLiteral("SFX GZIP (*.exe)");
}

QString XSFXGzipArchive::getMIMEString()
{
    return QStringLiteral("application/gzip");
}

QString XSFXGzipArchive::getVersion()
{
    return QString();
}

qint64 XSFXGzipArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return 0;

    // The gzip trailer (CRC32 plus ISIZE) follows the deflate stream.
    return qMin(context.nDataOffset + context.nCompressedSize + 8, context.nInputSize);
}

QList<XBinary::MAPMODE> XSFXGzipArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XSFXGzipArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XSFXGzipArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSFXGzipArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = context.nDataOffset;
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
        part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("GZIP / Deflate"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XSFXGzipArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSFXGzipArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, pPdStruct)) {
        releaseUnpackSource(pState);
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

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
            delete pContext;
            *pState = UNPACK_STATE();
            return false;
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XSFXGzipArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentOffset != pContext->nDataOffset)) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEFLATE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("GZIP / Deflate"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XSFXGzipArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XSFXGzipArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XSFXGzipArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
