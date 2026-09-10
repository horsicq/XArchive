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
#include "xzxziparchive.h"

#include "Algos/xzxzipdecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 ZXZIP_HEADER_SIZE = 0x11;
const qint64 ZXZIP_ENTRY_SIZE = 0x16;
const qint32 ZXZIP_MAX_MEMBERS = 4096;

bool zxzipRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

quint16 zxzipHobetaChecksum(const uchar *pData, qint32 nCount)
{
    quint32 nSum = 0;
    for (qint32 i = 0; i < nCount; ++i) nSum += pData[i];

    return (quint16)(((nSum & 0xffff) * 0x101 + 0x69) & 0xffff);
}

}  // namespace

XZXZIPArchive::XZXZIPArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZXZIPArchive::~XZXZIPArchive()
{
}

bool XZXZIPArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XZXZIPArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (ZXZIP_HEADER_SIZE + ZXZIP_ENTRY_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, ZXZIP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != ZXZIP_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (baHeader.mid(8, 3) != QByteArray("ZIP", 3)) return false;
    if (pHeader[0x0d] != 0) return false;
    const qint64 nTotal = qFromLittleEndian<quint16>(pHeader + 0x0b);
    if ((nTotal >> 8) > pHeader[0x0e]) return false;
    if (zxzipHobetaChecksum(pHeader, 15) != qFromLittleEndian<quint16>(pHeader + 0x0f)) return false;
    for (qint32 i = 0; i < 9; ++i) {
        if (pHeader[i] < 0x20) return false;
    }
    context.sArchiveName = QString::fromLatin1(baHeader.left(8)).trimmed();

    qint64 nOffset = ZXZIP_HEADER_SIZE;
    qint64 nRemaining = nTotal;
    while (nRemaining > 0) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= ZXZIP_MAX_MEMBERS) break;
        if (!zxzipRangeWithin(context.nInputSize, nOffset, ZXZIP_ENTRY_SIZE)) break;

        const QByteArray baEntry = read_array_process(nOffset, ZXZIP_ENTRY_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource || (baEntry.size() != ZXZIP_ENTRY_SIZE)) return false;
        const uchar *pEntry = (const uchar *)baEntry.constData();

        bool bNameOk = true;
        for (qint32 i = 0; i < 9; ++i) {
            if (pEntry[i] < 0x20) bNameOk = false;
        }
        if (!bNameOk) break;

        qint64 nPacked = qFromLittleEndian<quint16>(pEntry + 0x0e);
        const qint64 nDataOffset = nOffset + ZXZIP_ENTRY_SIZE;
        // The last member of a truncated archive declares more packed bytes
        // than the file holds; the reference still writes what it managed to
        // decode, so the member is listed rather than dropped - its extraction
        // then fails, because a short codec stream cannot fill the member out.
        bool bTruncated = false;
        if (!zxzipRangeWithin(context.nInputSize, nDataOffset, nPacked)) {
            if (nDataOffset >= context.nInputSize) break;
            nPacked = context.nInputSize - nDataOffset;
            bTruncated = true;
        }

        qint64 nDataSize = 0;
        qint64 nPaddedSize = 0;
        if (!XZXZIPDecoder::memberSize(baEntry, &nDataSize, &nPaddedSize)) break;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nPacked;
        member.nUncompressedSize = XZXZIPDecoder::HOBETA_SIZE + nPaddedSize;
        member.nCRC = qFromLittleEndian<quint32>(pEntry + 0x10);
        member.nMethod = pEntry[0x14];
        member.nSubMethod = pEntry[0x15];
        member.baEntry = baEntry;

        // TR-DOS names are 8 characters plus a one-letter type; the emitted
        // file is a Hobeta image, whose usual naming is "name.$<type>"
        const QString sBase = QString::fromLatin1(baEntry.left(8)).trimmed();
        const QChar cType = QChar((quint16)pEntry[8]);
        member.sFileName = (sBase.isEmpty() ? QString("file%1").arg(context.listMembers.size()) : sBase) + QStringLiteral(".$") + cType;
        context.listMembers.append(member);

        nOffset = nDataOffset + nPacked;
        nRemaining -= (nPacked + ZXZIP_ENTRY_SIZE);
        if (bTruncated) break;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XZXZIPArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZXZIPArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZXZIPArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZXZIPArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZXZIPArchive(pDevice);
}

QList<QString> XZXZIPArchive::getSearchSignatures()
{
    // "ZIP" sits at offset 8, after the eight-character archive name
    return QList<QString>() << QStringLiteral("................'ZIP'");
}

XBinary::FT XZXZIPArchive::getFileType()
{
    return FT_ZXZIP;
}

XBinary::MODE XZXZIPArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZXZIPArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZXZIPArchive::getArch()
{
    return QString();
}

qint32 XZXZIPArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZXZIPArchive::getFileFormatExt()
{
    return QStringLiteral("$z");
}

QString XZXZIPArchive::getFileFormatExtsString()
{
    return QStringLiteral("ZXZIP archive");
}

QString XZXZIPArchive::getMIMEString()
{
    return QStringLiteral("application/x-zxzip");
}

QString XZXZIPArchive::getVersion()
{
    return QString();
}

qint64 XZXZIPArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XZXZIPArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZXZIPArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XZXZIPArchive::methodToString(quint8 nMethod)
{
    if (nMethod == XZXZIPDecoder::METHOD_STORE) return QStringLiteral("Stored");
    if (nMethod == XZXZIPDecoder::METHOD_ZX_UNSUPPORTED) return QStringLiteral("ZXZIP 1 (unsupported)");
    if (nMethod == XZXZIPDecoder::METHOD_SHRINK) return QStringLiteral("Shrink");
    if (nMethod == XZXZIPDecoder::METHOD_LZH) return QStringLiteral("ZXZIP LZH");

    return QStringLiteral("Unknown %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XZXZIPArchive::methodToHandleMethod(quint8 nMethod)
{
    // every method, STORE included, goes through the decoder: the emitted file
    // is a Hobeta header plus the data plus padding
    if (nMethod == XZXZIPDecoder::METHOD_ZX_UNSUPPORTED) return HANDLE_METHOD_UNKNOWN;
    if (nMethod > XZXZIPDecoder::METHOD_LZH) return HANDLE_METHOD_UNKNOWN;

    return HANDLE_METHOD_ZXZIP;
}

bool XZXZIPArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZXZIPArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ZXZIP_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const MEMBER &member = context.listMembers.at(i);

        if (nFileParts & FILEPART_STREAM) {
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
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XZXZIPDecoder::packProperties(member.baEntry));
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

QMap<XBinary::UNPACK_PROP, QVariant> XZXZIPArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZXZIPArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XZXZIPArchive> guardedThis(this);
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
    pState->nCurrentOffset = pContext->listMembers.at(0).nHeaderOffset;
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

XBinary::ARCHIVERECORD XZXZIPArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nHeaderOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nDataOffset;
    result.nStreamSize = member.nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nCompressedSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(member.nMethod));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(member.nMethod));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, XZXZIPDecoder::packProperties(member.baEntry));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZXZIPArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nHeaderOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XZXZIPArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZXZIPArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
