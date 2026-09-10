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
#include "xsquashfsarchive.h"

#include <QPointer>

#include <new>

namespace {
const qint64 SQUASHFS_HEADER_SIZE = 0x80;
}  // namespace

XSquashFSArchive::XSquashFSArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSquashFSArchive::~XSquashFSArchive()
{
}

bool XSquashFSArchive::parseContext(CONTEXT *pContext, bool bFull, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XSquashFSArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < SQUASHFS_HEADER_SIZE) return false;
    if (context.nInputSize > XSquashFSDecoder::MAX_INPUT_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, SQUASHFS_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != SQUASHFS_HEADER_SIZE)) return false;
    if (!XSquashFSDecoder::parseSuperBlock(baHeader, context.nInputSize, &context.superBlock)) return false;

    context.nArchiveSize = context.nInputSize;
    if ((context.superBlock.nBytesUsed > 0) && (context.superBlock.nBytesUsed <= context.nInputSize)) {
        context.nArchiveSize = context.superBlock.nBytesUsed;
    }

    if (!bFull) {
        *pContext = context;
        return true;
    }

    // The inode and directory tables are scattered across the whole image and
    // every metadata block is compressed, so the walk works on the image as a
    // whole rather than on a window of it.
    const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
    if (!guardedThis || !guardedSource || (baFile.size() != context.nInputSize)) return false;
    if (!XSquashFSDecoder::listMembers(baFile, &context.listMembers, pPdStruct)) return false;
    if (!guardedThis || !guardedSource) return false;
    if (context.listMembers.isEmpty()) return false;

    *pContext = context;

    return true;
}

bool XSquashFSArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XSquashFSArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSquashFSArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSquashFSArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSquashFSArchive(pDevice);
}

QList<QString> XSquashFSArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'hsqs'") << QStringLiteral("'sqsh'") << QStringLiteral("'hsqt'") << QStringLiteral("'shsq'");
}

XBinary::FT XSquashFSArchive::getFileType()
{
    return FT_SQUASHFS;
}

XBinary::MODE XSquashFSArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSquashFSArchive::getEndian()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return ENDIAN_LITTLE;

    return context.superBlock.bBigEndian ? ENDIAN_BIG : ENDIAN_LITTLE;
}

QString XSquashFSArchive::getArch()
{
    return QString();
}

qint32 XSquashFSArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XSquashFSArchive::getFileFormatExt()
{
    return QStringLiteral("squashfs");
}

QString XSquashFSArchive::getFileFormatExtsString()
{
    return QStringLiteral("SquashFS (*.squashfs *.sqsh *.sfs)");
}

QString XSquashFSArchive::getMIMEString()
{
    return QStringLiteral("application/x-squashfs");
}

QString XSquashFSArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();

    return QString("%1.%2").arg(context.superBlock.nMajor).arg(context.superBlock.nMinor);
}

qint64 XSquashFSArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XSquashFSArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XSquashFSArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XSquashFSArchive::compressorToString(qint32 nCompressor)
{
    if (nCompressor == 0) return QStringLiteral("SquashFS/gzip");
    if (nCompressor == 2) return QStringLiteral("SquashFS/LZMA");
    if (nCompressor == 3) return QStringLiteral("SquashFS/LZO");
    if (nCompressor == 4) return QStringLiteral("SquashFS/XZ");
    if (nCompressor == 5) return QStringLiteral("SquashFS/LZ4");
    if (nCompressor == 6) return QStringLiteral("SquashFS/ZSTD");

    return QStringLiteral("SquashFS/%1").arg(nCompressor);
}

bool XSquashFSArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSquashFSArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SQUASHFS_HEADER_SIZE;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }

    for (qint32 i = 0; i < context.listMembers.size(); ++i) {
        if (!isPdStructNotCanceled(pPdStruct) || !canAppendPart(nLimit, listResult.size())) break;
        const XSquashFSDecoder::MEMBER &member = context.listMembers.at(i);

        if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
            FPART part = {};
            part.filePart = FILEPART_STREAM;
            part.nFileOffset = member.nSpanOffset;
            part.nFileSize = member.nSpanSize;
            part.nVirtualAddress = XADDR_MAX;
            part.sName = member.sFileName;
            part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSpanSize);
            part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
            part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE : HANDLE_METHOD_SQUASHFS);
            part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, compressorToString(context.superBlock.nCompressor));
            if (member.nUncompressedSize != 0) part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baDescriptor);
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

QMap<XBinary::UNPACK_PROP, QVariant> XSquashFSArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSquashFSArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XSquashFSArchive> guardedThis(this);
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
    pState->nCurrentOffset = pContext->listMembers.at(0).nSpanOffset;
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

XBinary::ARCHIVERECORD XSquashFSArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nCurrentIndex < 0) ||
        (pState->nCurrentIndex >= pState->nNumberOfRecords)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext || (pState->nCurrentIndex >= pContext->listMembers.size())) return ARCHIVERECORD();

    const XSquashFSDecoder::MEMBER &member = pContext->listMembers.at(pState->nCurrentIndex);
    if (pState->nCurrentOffset != member.nSpanOffset) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = member.nSpanOffset;
    result.nStreamSize = member.nSpanSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, member.sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, member.nSpanSize);
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, member.nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, (member.nUncompressedSize == 0) ? HANDLE_METHOD_STORE : HANDLE_METHOD_SQUASHFS);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, compressorToString(pContext->superBlock.nCompressor));
    if (member.nUncompressedSize != 0) result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, member.baDescriptor);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XSquashFSArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
        pState->nCurrentOffset = pContext->listMembers.at(pState->nCurrentIndex).nSpanOffset;
        return true;
    }
    pState->nCurrentOffset = pContext->nArchiveSize;

    return false;
}

bool XSquashFSArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XSquashFSArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_COMPRESSPROPERTIES << FPART_PROP_ISFOLDER;
}
