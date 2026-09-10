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
#include "xcflarchive.h"

#include "Algos/include/zlib.h"

#include <QPointer>
#include <QtEndian>

#include <cstring>
#include <new>

namespace {
const qint64 CFL_HEADER_SIZE = 0x0c;
const qint64 CFL_ENTRY_SIZE = 0x0e;
const qint64 CFL_MAX_DIRECTORY_SIZE = 0x20000000;
const qint32 CFL_MAX_MEMBERS = 1000000;

const quint16 CFL_METHOD_STORED = 0;
const quint16 CFL_METHOD_ZLIB = 1;
// RFC 1950 CMF+FLG.  The archive stores those two bytes and the raw DEFLATE
// that follows them, but never the Adler-32 trailer.
const qint64 CFL_ZLIB_HEADER_SIZE = 2;

bool cflIsZlibHeader(const QByteArray &baHeader)
{
    if (baHeader.size() != CFL_ZLIB_HEADER_SIZE) return false;
    const quint8 nCMF = (quint8)baHeader.at(0);
    const quint8 nFLG = (quint8)baHeader.at(1);
    const quint16 nWord = (quint16)(((quint16)nCMF << 8) | nFLG);

    // DEFLATE, a window no larger than 32 KiB, the RFC's check value, and no
    // preset dictionary (which the shared decoder cannot supply).
    return ((nCMF & 0x0F) == 8) && ((nCMF >> 4) <= 7) && ((nWord % 31) == 0) && ((nFLG & 0x20) == 0);
}

bool cflRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// A zlib stream that has to expand to exactly nUncompressedSize bytes; the
// reference rejects any other outcome.  Z_OK and Z_BUF_ERROR are accepted
// alongside Z_STREAM_END on purpose: the archive omits the Adler-32 trailer, so
// a directory block that produced every byte it promised still never reaches
// the end-of-stream state.
bool cflInflate(const QByteArray &baPacked, qint64 nUncompressedSize, QByteArray *pbaResult)
{
    if (!pbaResult || (nUncompressedSize < 0) || (nUncompressedSize > CFL_MAX_DIRECTORY_SIZE)) return false;

    QByteArray baOut;
    baOut.resize((qint32)nUncompressedSize);
    if (baOut.size() != nUncompressedSize) return false;
    if (nUncompressedSize == 0) {
        *pbaResult = baOut;
        return true;
    }
    if (baPacked.isEmpty() || (baPacked.size() > CFL_MAX_DIRECTORY_SIZE)) return false;

    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit(&stream) != Z_OK) return false;

    stream.next_in = (Bytef *)baPacked.constData();
    stream.avail_in = (uInt)baPacked.size();
    stream.next_out = (Bytef *)baOut.data();
    stream.avail_out = (uInt)baOut.size();

    const int nStatus = inflate(&stream, Z_FINISH);
    const qint64 nProduced = (qint64)stream.total_out;
    inflateEnd(&stream);

    if ((nStatus != Z_STREAM_END) && (nStatus != Z_OK) && (nStatus != Z_BUF_ERROR)) return false;
    if (nProduced != nUncompressedSize) return false;

    *pbaResult = baOut;

    return true;
}

}  // namespace

XCFLArchive::XCFLArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCFLArchive::~XCFLArchive()
{
}

bool XCFLArchive::readBlock(qint64 nOffset, qint64 nExpandedSize, qint64 nInputSize, QByteArray *pbaResult, qint64 *pnBlockSize, PDSTRUCT *pPdStruct)
{
    if (!pbaResult || !pnBlockSize) return false;

    QPointer<XCFLArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    if (!cflRangeWithin(nInputSize, nOffset, 8)) return false;

    const QByteArray baBlockHeader = read_array_process(nOffset, 8, pPdStruct);
    if (!guardedThis || !guardedSource || (baBlockHeader.size() != 8)) return false;
    const uchar *pBlockHeader = (const uchar *)baBlockHeader.constData();
    const qint32 nMethod = (qint32)qFromLittleEndian<quint32>(pBlockHeader);
    const qint64 nBlockSize = (qint32)qFromLittleEndian<quint32>(pBlockHeader + 4);

    if (nMethod == (qint32)CFL_METHOD_STORED) {
        if (nBlockSize != nExpandedSize) return false;
        if (!cflRangeWithin(nInputSize, nOffset + 8, nExpandedSize)) return false;
        const QByteArray baStored = read_array_process(nOffset + 8, nExpandedSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baStored.size() != nExpandedSize)) return false;
        *pbaResult = baStored;
        *pnBlockSize = 8 + nExpandedSize;
        return true;
    }
    if (nMethod != (qint32)CFL_METHOD_ZLIB) return false;
    if ((nBlockSize < 4) || (nBlockSize > CFL_MAX_DIRECTORY_SIZE)) return false;
    if (!cflRangeWithin(nInputSize, nOffset + 8, 4)) return false;

    const QByteArray baExpandedSize = read_array_process(nOffset + 8, 4, pPdStruct);
    if (!guardedThis || !guardedSource || (baExpandedSize.size() != 4)) return false;
    if ((qint64)(qint32)qFromLittleEndian<quint32>((const uchar *)baExpandedSize.constData()) != nExpandedSize) return false;

    const qint64 nPackedSize = nBlockSize - 4;
    if (!cflRangeWithin(nInputSize, nOffset + 12, nPackedSize)) return false;
    const QByteArray baPacked = read_array_process(nOffset + 12, nPackedSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baPacked.size() != nPackedSize)) return false;

    if (!cflInflate(baPacked, nExpandedSize, pbaResult)) return false;
    *pnBlockSize = 8 + 4 + nPackedSize;

    return true;
}

bool XCFLArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XCFLArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < CFL_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, CFL_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != CFL_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("CFL3", 4)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    const qint64 nDirectoryOffset = (qint32)qFromLittleEndian<quint32>(pHeader + 4);
    const qint64 nDirectorySize = (qint32)qFromLittleEndian<quint32>(pHeader + 8);
    if (nDirectoryOffset < CFL_HEADER_SIZE) return false;
    if ((nDirectorySize <= 0) || (nDirectorySize >= CFL_MAX_DIRECTORY_SIZE)) return false;

    QByteArray baDirectory;
    qint64 nDirectoryBlockSize = 0;
    if (!readBlock(nDirectoryOffset, nDirectorySize, context.nInputSize, &baDirectory, &nDirectoryBlockSize, pPdStruct)) return false;
    if (!guardedThis || !guardedSource || (baDirectory.size() != nDirectorySize)) return false;

    const uchar *pDirectory = (const uchar *)baDirectory.constData();
    qint64 nPosition = 0;
    qint64 nArchiveEnd = qMin(nDirectoryOffset + nDirectoryBlockSize, context.nInputSize);

    while ((nPosition + CFL_ENTRY_SIZE) <= nDirectorySize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= CFL_MAX_MEMBERS) break;

        const uchar *pEntry = pDirectory + nPosition;
        const qint64 nSize = (qint32)qFromLittleEndian<quint32>(pEntry);
        const qint64 nDataOffset = (qint32)qFromLittleEndian<quint32>(pEntry + 4);
        const quint16 nMethod = qFromLittleEndian<quint16>(pEntry + 8);
        const qint32 nNameLength = (qint32)qFromLittleEndian<quint16>(pEntry + 12);

        // The reference stops walking the directory here rather than failing:
        // this is also how it copes with the padding some archives leave after
        // the last record.
        if ((nSize < 0) || (nDataOffset < CFL_HEADER_SIZE) || (nNameLength == 0)) break;
        if ((nPosition + CFL_ENTRY_SIZE + nNameLength) > nDirectorySize) break;

        MEMBER member = {};
        // The entry's own position inside the INFLATED directory; it is a
        // description, never a device coordinate.
        member.nHeaderOffset = nPosition;
        member.nUncompressedSize = nSize;
        member.nMethod = nMethod;
        member.sFileName = QString::fromLatin1((const char *)(pEntry + CFL_ENTRY_SIZE), nNameLength);
        nPosition += CFL_ENTRY_SIZE + nNameLength;

        if (nSize == 0) {
            // No data block at all, so the record's offset is never read; keep
            // it inside the device anyway, since a record must not advertise an
            // extent the source cannot answer for.
            member.nDataOffset = qMin(nDataOffset, context.nInputSize);
            member.nCompressedSize = 0;
            member.nMethod = CFL_METHOD_STORED;
            context.listMembers.append(member);
            continue;
        }

        if (nMethod == CFL_METHOD_STORED) {
            if (!cflRangeWithin(context.nInputSize, nDataOffset, nSize)) return false;
            member.nDataOffset = nDataOffset;
            member.nCompressedSize = nSize;
            nArchiveEnd = qMax(nArchiveEnd, nDataOffset + nSize);
        } else if (nMethod == CFL_METHOD_ZLIB) {
            if (!cflRangeWithin(context.nInputSize, nDataOffset, 8)) return false;
            const QByteArray baBlockHeader = read_array_process(nDataOffset, 8, pPdStruct);
            if (!guardedThis || !guardedSource || (baBlockHeader.size() != 8)) return false;
            const uchar *pBlockHeader = (const uchar *)baBlockHeader.constData();
            const qint64 nBlockSize = (qint32)qFromLittleEndian<quint32>(pBlockHeader);
            const qint64 nExpandedSize = (qint32)qFromLittleEndian<quint32>(pBlockHeader + 4);
            if ((nBlockSize < 4) || (nExpandedSize != nSize)) return false;
            const qint64 nPackedSize = nBlockSize - 4;
            if (!cflRangeWithin(context.nInputSize, nDataOffset + 8, nPackedSize)) return false;
            // THE STORED STREAM CARRIES NO ADLER-32.  It is a two-byte RFC 1950
            // header followed by raw DEFLATE and nothing else: blockSize - 4
            // counts exactly those bytes and the next block starts immediately
            // after them.  Publishing it as HANDLE_METHOD_ZLIB made the shared
            // zlib decoder claim the last four DEFLATE bytes as the checksum
            // footer, so every member decoded short (474 -> 466 for the first
            // one) and the archive extracted nothing.  Hand the decoder the raw
            // DEFLATE extent instead.
            if (nPackedSize < CFL_ZLIB_HEADER_SIZE) return false;
            const QByteArray baStreamHeader = read_array_process(nDataOffset + 8, CFL_ZLIB_HEADER_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource || (baStreamHeader.size() != CFL_ZLIB_HEADER_SIZE)) return false;
            if (!cflIsZlibHeader(baStreamHeader)) return false;
            member.nDataOffset = nDataOffset + 8 + CFL_ZLIB_HEADER_SIZE;
            member.nCompressedSize = nPackedSize - CFL_ZLIB_HEADER_SIZE;
            nArchiveEnd = qMax(nArchiveEnd, nDataOffset + 8 + nPackedSize);
        } else {
            return false;
        }

        context.listMembers.append(member);
    }

    if (context.listMembers.isEmpty()) return false;

    context.nDirectoryOffset = nDirectoryOffset;
    context.nDirectoryBlockSize = nDirectoryBlockSize;
    context.nArchiveSize = qMin(nArchiveEnd, context.nInputSize);
    *pContext = context;

    return true;
}

bool XCFLArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XCFLArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCFLArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XCFLArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCFLArchive(pDevice);
}

QList<QString> XCFLArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'CFL3'");
}

XBinary::FT XCFLArchive::getFileType()
{
    return FT_CFL;
}

XBinary::MODE XCFLArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XCFLArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCFLArchive::getArch()
{
    return QString();
}

qint32 XCFLArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XCFLArchive::getFileFormatExt()
{
    return QStringLiteral("dat");
}

QString XCFLArchive::getFileFormatExtsString()
{
    return QStringLiteral("CFL (*.dat)");
}

QString XCFLArchive::getMIMEString()
{
    return QStringLiteral("application/x-cfl");
}

QString XCFLArchive::getVersion()
{
    return QStringLiteral("3");
}

qint64 XCFLArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XCFLArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XCFLArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XCFLArchive::methodToString(quint16 nMethod)
{
    if (nMethod == CFL_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == CFL_METHOD_ZLIB) return QStringLiteral("Zlib");
    return QStringLiteral("Unknown %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XCFLArchive::methodToHandleMethod(quint16 nMethod)
{
    if (nMethod == CFL_METHOD_STORED) return HANDLE_METHOD_STORE;
    // Not HANDLE_METHOD_ZLIB: the stored stream has no Adler-32 trailer, and
    // the published extent already skips the two-byte RFC 1950 header, so what
    // the decoder receives is raw DEFLATE.
    if (nMethod == CFL_METHOD_ZLIB) return HANDLE_METHOD_DEFLATE;
    return HANDLE_METHOD_UNKNOWN;
}

bool XCFLArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XCFLArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = CFL_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nDataOffset;
            part.nFileSize = member.nCompressedSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
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

QMap<XBinary::UNPACK_PROP, QVariant> XCFLArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCFLArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XCFLArchive> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nDataOffset;
    pState->nTotalSize = pContext->nArchiveSize;
    pState->nCurrentIndex = 0;
    pState->nNumberOfRecords = pContext->listMembers.size();
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

XBinary::ARCHIVERECORD XCFLArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nDataOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XCFLArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return false;
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return false;

    ++pState->nCurrentIndex;
    if (pState->nCurrentIndex < pState->nNumberOfRecords) {
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nDataOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XCFLArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XCFLArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
