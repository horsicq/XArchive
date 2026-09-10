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
#include "xzlwbarchive.h"

#include <QBuffer>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 ZLWB_HEADER_SIZE = 0x1e;
const qint64 ZLWB_BLOB_HEADER_SIZE = 0x10;
const qint64 ZLWB_RECORD_SHORT = 0x110;
const qint64 ZLWB_RECORD_LONG = 0x228;
const qint32 ZLWB_MAX_MEMBERS = 100000;
const qint64 ZLWB_MAX_BLOB_SIZE = 0x100000;

bool zlwbRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

// One directory blob: a complete zlib stream that inflates to exactly the
// record size the blob header declares.
bool zlwbInflateRecord(const QByteArray &baBlob, qint64 nRecordSize, QByteArray *pbaResult, XBinary::PDSTRUCT *pPdStruct)
{
    if (!pbaResult) return false;
    pbaResult->clear();
    if (baBlob.isEmpty() || (nRecordSize <= 0)) return false;

    QByteArray baInput(baBlob);
    QByteArray baOutput;
    QBuffer inputBuffer(&baInput);
    QBuffer outputBuffer(&baOutput);
    // ReadWrite rather than WriteOnly: decompress_zlib authenticates the RFC
    // 1950 Adler32 footer by reading the finished output device back.
    if (!inputBuffer.open(QIODevice::ReadOnly) || !outputBuffer.open(QIODevice::ReadWrite)) return false;

    XBinary::DATAPROCESS_STATE state = {};
    state.pDeviceInput = &inputBuffer;
    state.pDeviceOutput = &outputBuffer;
    state.nInputOffset = 0;
    state.nInputLimit = baInput.size();
    state.nProcessedOffset = 0;
    state.nProcessedLimit = nRecordSize;
    state.mapUnpackProperties.insert(XBinary::UNPACK_PROP_MAX_OUTPUT_SIZE, nRecordSize);

    const bool bResult = XDeflateDecoder::decompress_zlib(&state, pPdStruct) && !state.bReadError && !state.bWriteError;
    inputBuffer.close();
    outputBuffer.close();
    if (!bResult || (baOutput.size() != nRecordSize)) return false;

    *pbaResult = baOutput;

    return true;
}

// Delphi ShortString: a length byte and that many characters.
QString zlwbShortString(const QByteArray &baRecord, qint32 nOffset)
{
    if ((nOffset < 0) || (nOffset >= baRecord.size())) return QString();
    const qint32 nLength = (quint8)baRecord.at(nOffset);
    if ((nLength <= 0) || ((nOffset + 1 + nLength) > baRecord.size())) return QString();

    return QString::fromLatin1(baRecord.constData() + nOffset + 1, nLength);
}
}  // namespace

XZLWBArchive::XZLWBArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XZLWBArchive::~XZLWBArchive()
{
}

bool XZLWBArchive::parseContext(CONTEXT *pContext, bool bWalkMembers, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XZLWBArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < ZLWB_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, ZLWB_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != ZLWB_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("ZLWB", 4)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();
    if (pHeader[4] != 0x1a) return false;
    if (qFromLittleEndian<quint32>(pHeader + 5) != 1) return false;

    const qint32 nNumberOfMembers = (qint32)qFromLittleEndian<quint32>(pHeader + 0x12);
    const qint64 nDirectoryOffset = (qint32)qFromLittleEndian<quint32>(pHeader + 0x16);
    if ((nNumberOfMembers < 0) || (nNumberOfMembers > ZLWB_MAX_MEMBERS)) return false;

    context.nArchiveSize = ZLWB_HEADER_SIZE;

    if (bWalkMembers && (nNumberOfMembers > 0)) {
        if (!zlwbRangeWithin(context.nInputSize, nDirectoryOffset, 0)) return false;

        qint64 nPosition = nDirectoryOffset;
        for (qint32 i = 0; i < nNumberOfMembers; ++i) {
            if (!isPdStructNotCanceled(pPdStruct)) return false;
            if (!zlwbRangeWithin(context.nInputSize, nPosition, ZLWB_BLOB_HEADER_SIZE)) return false;

            const QByteArray baBlobHeader = read_array_process(nPosition, ZLWB_BLOB_HEADER_SIZE, pPdStruct);
            if (!guardedThis || !guardedSource || (baBlobHeader.size() != ZLWB_BLOB_HEADER_SIZE)) return false;
            const uchar *pBlobHeader = (const uchar *)baBlobHeader.constData();
            const qint64 nBlobSize = (qint32)qFromLittleEndian<quint32>(pBlobHeader + 4);
            const qint64 nRecordSize = (qint32)qFromLittleEndian<quint32>(pBlobHeader + 8);
            if ((nBlobSize < 0) || (nBlobSize > ZLWB_MAX_BLOB_SIZE)) return false;
            // The record size is not a hint: it names the layout.
            if ((nRecordSize != ZLWB_RECORD_SHORT) && (nRecordSize != ZLWB_RECORD_LONG)) return false;
            if (!zlwbRangeWithin(context.nInputSize, nPosition + ZLWB_BLOB_HEADER_SIZE, nBlobSize)) return false;

            const QByteArray baBlob = read_array_process(nPosition + ZLWB_BLOB_HEADER_SIZE, nBlobSize, pPdStruct);
            if (!guardedThis || !guardedSource || (baBlob.size() != nBlobSize)) return false;

            QByteArray baRecord;
            if (!zlwbInflateRecord(baBlob, nRecordSize, &baRecord, pPdStruct)) return false;
            if (!guardedThis || !guardedSource) return false;
            const uchar *pRecord = (const uchar *)baRecord.constData();

            MEMBER member = {};
            member.nHeaderOffset = nPosition;
            member.nHeaderSize = ZLWB_BLOB_HEADER_SIZE + nBlobSize;

            if (nRecordSize == ZLWB_RECORD_SHORT) {
                member.sFileName = zlwbShortString(baRecord, 0);
                member.nDataOffset = (qint32)qFromLittleEndian<quint32>(pRecord + 0x100);
                member.nCompressedSize = (qint32)qFromLittleEndian<quint32>(pRecord + 0x104);
                member.nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pRecord + 0x108);
                member.nCRC = qFromLittleEndian<quint32>(pRecord + 0x10c);
            } else {
                // The full path is preferred over the bare name; it is what the
                // reference rebuilds the member's location from.
                member.sFileName = zlwbShortString(baRecord, 0x106);
                if (member.sFileName.isEmpty()) member.sFileName = zlwbShortString(baRecord, 6);
                member.nDataOffset = (qint32)qFromLittleEndian<quint32>(pRecord + 0x218);
                member.nCompressedSize = (qint32)qFromLittleEndian<quint32>(pRecord + 0x21c);
                member.nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pRecord + 0x220);
                member.nCRC = qFromLittleEndian<quint32>(pRecord + 0x224);
            }

            if ((member.nDataOffset < 0) || (member.nCompressedSize < 0) || (member.nUncompressedSize < 0)) return false;
            if (!zlwbRangeWithin(context.nInputSize, member.nDataOffset, member.nCompressedSize)) return false;

            context.listMembers.append(member);

            nPosition += ZLWB_BLOB_HEADER_SIZE + nBlobSize;
            if (nPosition > context.nArchiveSize) context.nArchiveSize = nPosition;
            if ((member.nDataOffset + member.nCompressedSize) > context.nArchiveSize) context.nArchiveSize = member.nDataOffset + member.nCompressedSize;
        }
    }

    if (context.nArchiveSize > context.nInputSize) context.nArchiveSize = context.nInputSize;
    *pContext = context;

    return true;
}

bool XZLWBArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XZLWBArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XZLWBArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XZLWBArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XZLWBArchive(pDevice);
}

QList<QString> XZLWBArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'ZLWB'1A01000000");
}

XBinary::FT XZLWBArchive::getFileType()
{
    return FT_ZLWB;
}

XBinary::MODE XZLWBArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XZLWBArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XZLWBArchive::getArch()
{
    return QString();
}

qint32 XZLWBArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XZLWBArchive::getFileFormatExt()
{
    return QStringLiteral("zlw");
}

QString XZLWBArchive::getFileFormatExtsString()
{
    return QStringLiteral("ZLWB (*.zlw)");
}

QString XZLWBArchive::getMIMEString()
{
    return QStringLiteral("application/x-zlwb");
}

QString XZLWBArchive::getVersion()
{
    return QStringLiteral("1");
}

qint64 XZLWBArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, true, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XZLWBArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XZLWBArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

bool XZLWBArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XZLWBArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = ZLWB_HEADER_SIZE;
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
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XZLWBArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XZLWBArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XZLWBArchive> guardedThis(this);
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
    // An archive with no members still opens; the cursor simply starts at the
    // end of it.
    pState->nCurrentOffset = pContext->listMembers.isEmpty() ? pContext->nArchiveSize : pContext->listMembers.at(0).nHeaderOffset;
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

XBinary::ARCHIVERECORD XZLWBArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_ZLIB);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("Deflate"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XZLWBArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XZLWBArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XZLWBArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
