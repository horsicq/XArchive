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
#include "xvmspcsiarchive.h"

#include "Algos/xvmspcsidecoder.h"

#include <QPointer>
#include <QtEndian>

#include <limits>
#include <new>

namespace {
const qint64 PCSI_HEADER_SIZE = 0x50;
const qint64 PCSI_TABLE_OFFSET = 0x200;
const qint64 PCSI_TABLE_HEADER_SIZE = 0x14;
const qint64 PCSI_MIN_SIZE = PCSI_TABLE_OFFSET + PCSI_TABLE_HEADER_SIZE;
const qint32 PCSI_MAX_RECORDS = 0x1000000;
// The whole container has to be resident for the record walk, so a file larger
// than a QByteArray can hold is rejected before anything is read.
const qint64 PCSI_MAX_INPUT_SIZE = 0x20000000;
}  // namespace

XVMSPCSIArchive::XVMSPCSIArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XVMSPCSIArchive::~XVMSPCSIArchive()
{
}

bool XVMSPCSIArchive::parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XVMSPCSIArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < PCSI_MIN_SIZE) || (context.nInputSize > PCSI_MAX_INPUT_SIZE)) return false;

    const QByteArray baHeader = read_array_process(0, PCSI_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != PCSI_HEADER_SIZE)) return false;
    if (!XVMSPCSIDecoder::isBannerValid(baHeader)) return false;

    const uchar *pHeader = (const uchar *)baHeader.constData();
    const quint32 nTableSize = qFromLittleEndian<quint32>(pHeader + 0x38);
    const qint32 nRecordCount = (qint32)qFromLittleEndian<quint32>(pHeader + 0x40);
    if ((nRecordCount < 1) || (nRecordCount > PCSI_MAX_RECORDS)) return false;
    if (nTableSize < (quint32)PCSI_TABLE_HEADER_SIZE) return false;
    if ((qint64)nTableSize > (context.nInputSize - PCSI_TABLE_OFFSET)) return false;

    context.nRecordCount = nRecordCount;
    context.nUncompressedSize = -1;
    context.bUncompressedSizeKnown = false;
    // The kit always comes back out under this name; the container stores none.
    context.sFileName = QStringLiteral("FILE.PCSI");

    if (bMeasure) {
        const QByteArray baFile = read_array_process(0, context.nInputSize, pPdStruct);
        if (!guardedThis || !guardedSource || (baFile.size() != context.nInputSize)) return false;
        qint64 nUncompressedSize = 0;
        if (!XVMSPCSIDecoder::measure(baFile, &nUncompressedSize, pPdStruct)) return false;
        if ((nUncompressedSize <= 0) || (nUncompressedSize > (qint64)(std::numeric_limits<qint32>::max)())) return false;
        context.nUncompressedSize = nUncompressedSize;
        context.bUncompressedSizeKnown = true;
    }

    *pContext = context;

    return true;
}

bool XVMSPCSIArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XVMSPCSIArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XVMSPCSIArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XVMSPCSIArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XVMSPCSIArchive(pDevice);
}

QList<QString> XVMSPCSIArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'OpenVMS DCX PCSI Compressed File'");
}

XBinary::FT XVMSPCSIArchive::getFileType()
{
    return FT_VMSPCSI;
}

XBinary::MODE XVMSPCSIArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XVMSPCSIArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XVMSPCSIArchive::getArch()
{
    return QString();
}

qint32 XVMSPCSIArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XVMSPCSIArchive::getFileFormatExt()
{
    return QStringLiteral("pcsi$compressed");
}

QString XVMSPCSIArchive::getFileFormatExtsString()
{
    return QStringLiteral("OpenVMS DCX PCSI (*.pcsi$compressed)");
}

QString XVMSPCSIArchive::getMIMEString()
{
    return QStringLiteral("application/x-vms-pcsi");
}

QString XVMSPCSIArchive::getVersion()
{
    return QString();
}

qint64 XVMSPCSIArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XVMSPCSIArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XVMSPCSIArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XVMSPCSIArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XVMSPCSIArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, true, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PCSI_TABLE_OFFSET;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = tr("Header");
        listResult.append(part);
    }
    if ((nFileParts & FILEPART_STREAM) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_STREAM;
        part.nFileOffset = 0;
        part.nFileSize = context.nInputSize;
        part.nVirtualAddress = XADDR_MAX;
        part.sName = context.sFileName;
        part.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, context.nInputSize);
        if (context.bUncompressedSizeKnown) part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, context.bUncompressedSizeKnown ? HANDLE_METHOD_VMSPCSI : HANDLE_METHOD_UNKNOWN);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("OpenVMS DCX"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XVMSPCSIArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XVMSPCSIArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XVMSPCSIArchive> guardedThis(this);
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
    pState->nCurrentOffset = 0;
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

XBinary::ARCHIVERECORD XVMSPCSIArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext) return ARCHIVERECORD();
    if (pState->nCurrentOffset != 0) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = 0;
    result.nStreamSize = pContext->nInputSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nInputSize);
    if (pContext->bUncompressedSizeKnown) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, pContext->bUncompressedSizeKnown ? HANDLE_METHOD_VMSPCSI : HANDLE_METHOD_UNKNOWN);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("OpenVMS DCX"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XVMSPCSIArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XVMSPCSIArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XVMSPCSIArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
