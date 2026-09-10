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
#include "ximparchive.h"

#include "Algos/ximpdecoder.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 IMP_HEADER_SIZE = 0x2a;
const qint64 IMP_RECORD_SIZE = 0x26;
const qint64 IMP_MAX_DIRECTORY = 0x400000;
const qint32 IMP_MAX_MEMBERS = 100000;
const quint16 IMP_UNSUPPORTED_FLAGS = 0x0005;

bool impRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XIMPArchive::XIMPArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XIMPArchive::~XIMPArchive()
{
}

bool XIMPArchive::parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XIMPArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < IMP_HEADER_SIZE) || (context.nInputSize > 0x7fffffffLL)) return false;

    const QByteArray baHeader = read_array_process(0, IMP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != IMP_HEADER_SIZE)) return false;
    if (baHeader.left(4) != QByteArray("IMP\n", 4)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    const quint16 nStoredCheck = qFromLittleEndian<quint16>(pHeader + 0x28);
    QByteArray baZeroed = baHeader;
    baZeroed[0x28] = (char)0;
    baZeroed[0x29] = (char)0;
    if ((quint16)(XIMPDecoder::crc32(baZeroed) & 0xffff) != nStoredCheck) return false;

    context.nDirectoryOffset = qFromLittleEndian<quint32>(pHeader + 4);
    context.nNumberOfFiles = qFromLittleEndian<quint32>(pHeader + 8);
    context.nFlags = qFromLittleEndian<quint16>(pHeader + 0x26);
    if (context.nFlags & IMP_UNSUPPORTED_FLAGS) return false;
    if (context.nNumberOfFiles > IMP_MAX_MEMBERS) return false;

    if (context.nNumberOfFiles > 0) {
        if (!impRangeWithin(context.nInputSize, context.nDirectoryOffset, 6)) return false;
        const QByteArray baTag = read_array_process(context.nDirectoryOffset, 6, pPdStruct);
        if (!guardedThis || !guardedSource || (baTag.size() != 6)) return false;
        if (baTag != QByteArray("IMPDE\0", 6)) return false;
    }

    if (!bFull || (context.nNumberOfFiles == 0)) {
        *pContext = context;
        return true;
    }

    const qint64 nDirectorySize = qMin(context.nInputSize - context.nDirectoryOffset, IMP_MAX_DIRECTORY);
    const QByteArray baDirectory = read_array_process(context.nDirectoryOffset, nDirectorySize, pPdStruct);
    if (!guardedThis || !guardedSource || (baDirectory.size() != nDirectorySize)) return false;

    QList<QByteArray> listChunks;
    if (!XIMPDecoder::decodeDirectory(baDirectory, (qint32)context.nNumberOfFiles, &listChunks, pPdStruct)) return false;

    qint32 nChunk = 0;
    qint64 nPosition = 0;
    for (qint64 nIndex = 0; nIndex < context.nNumberOfFiles; ++nIndex) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        // a member record never straddles a chunk
        while ((listChunks.at(nChunk).size() - nPosition) < IMP_RECORD_SIZE) {
            ++nChunk;
            if (nChunk >= listChunks.size()) return false;
            nPosition = 0;
        }
        const QByteArray &baChunk = listChunks.at(nChunk);
        const uchar *pRecord = (const uchar *)baChunk.constData() + nPosition;

        const quint16 nVersion = qFromLittleEndian<quint16>(pRecord);
        if ((nVersion & 0x0fff) >= 0x010b) return false;
        const qint64 nExtraLength = pRecord[0x0a];
        const qint64 nAuxLength = qFromLittleEndian<quint16>(pRecord + 0x18);
        const qint64 nNameLength = qFromLittleEndian<quint16>(pRecord + 0x1a);
        const qint64 nTotal = nNameLength + nExtraLength + nAuxLength + IMP_RECORD_SIZE;
        if ((baChunk.size() - nPosition) < nTotal) return false;

        QByteArray baZeroedRecord = baChunk.mid((int)nPosition, (int)nTotal);
        baZeroedRecord[0x24] = (char)0;
        baZeroedRecord[0x25] = (char)0;
        if ((quint16)(XIMPDecoder::crc32(baZeroedRecord) & 0xffff) != qFromLittleEndian<quint16>(pRecord + 0x24)) return false;

        MEMBER member = {};
        member.nRecordIndex = nIndex;
        member.nStreamBase = qFromLittleEndian<quint32>(pRecord + 4);
        member.nAttributes = pRecord[0x0b];
        member.nStreamOffset = qFromLittleEndian<quint32>(pRecord + 0x0c);
        member.nUncompressedSize = qFromLittleEndian<quint32>(pRecord + 0x10);
        member.nCRC = qFromLittleEndian<quint32>(pRecord + 0x14);
        member.nTime = qFromLittleEndian<quint16>(pRecord + 0x20) | ((quint32)qFromLittleEndian<quint16>(pRecord + 0x22) << 16);
        member.sFileName = QString::fromLatin1(baChunk.mid((int)(nPosition + IMP_RECORD_SIZE), (int)nNameLength)).replace(QChar('\\'), QChar('/'));
        if (!impRangeWithin(context.nInputSize, member.nStreamBase, 6)) return false;
        context.listMembers.append(member);

        nPosition += nTotal;
    }

    *pContext = context;

    return true;
}

bool XIMPArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XIMPArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XIMPArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XIMPArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XIMPArchive(pDevice);
}

QList<QString> XIMPArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'IMP'0A");
}

XBinary::FT XIMPArchive::getFileType()
{
    return FT_IMP;
}

XBinary::MODE XIMPArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XIMPArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XIMPArchive::getArch()
{
    return QString();
}

qint32 XIMPArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XIMPArchive::getFileFormatExt()
{
    return QStringLiteral("imp");
}

QString XIMPArchive::getFileFormatExtsString()
{
    return QStringLiteral("IMP (*.imp)");
}

QString XIMPArchive::getMIMEString()
{
    return QStringLiteral("application/x-imp");
}

QString XIMPArchive::getVersion()
{
    return QString();
}

qint64 XIMPArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XIMPArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XIMPArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XIMPArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XIMPArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    const bool bFull = ((nFileParts & FILEPART_STREAM) != 0);
    if (!parseContext(&context, bFull, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = IMP_HEADER_SIZE;
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
            part.nFileOffset = member.nStreamBase;
            part.nFileSize = context.nInputSize - member.nStreamBase;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, part.nFileSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IMP);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("IMP"));
            part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES,
                                      XIMPDecoder::packProperties(member.nStreamOffset, member.nUncompressedSize, member.nAttributes));
            listResult.append(part);
        }
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

QMap<XBinary::UNPACK_PROP, QVariant> XIMPArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XIMPArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XIMPArchive> guardedThis(this);
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
    if (!parseContext(pContext, true, pPdStruct) || !guardedThis || !guardedSource || pContext->listMembers.isEmpty()) {
        if (guardedThis) releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    pState->mapUnpackProperties = mapProperties;
    pState->nCurrentOffset = pContext->listMembers.at(0).nStreamBase;
    pState->nTotalSize = pContext->nInputSize;
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

XBinary::ARCHIVERECORD XIMPArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    // the member header lives in the DECOMPRESSED directory and so has no file
    // offset of its own; the stream base stands in for it
    if (pState->nCurrentOffset != member.nStreamBase) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nStreamBase;
    result.nStreamSize = pContext->nInputSize - member.nStreamBase;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, result.nStreamSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_IMP);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("IMP"));
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES,
                                XIMPDecoder::packProperties(member.nStreamOffset, member.nUncompressedSize, member.nAttributes));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XIMPArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nStreamBase;
        return true;
    }
    pState->nCurrentOffset = pContext->nInputSize;

    return false;
}

bool XIMPArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XIMPArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
