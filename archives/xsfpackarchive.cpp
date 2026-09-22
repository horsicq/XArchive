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
#include "xsfpackarchive.h"

#include <QFileInfo>
#include <QtEndian>

#include <new>

namespace {
const qint64 SFPACK_MIN_SIZE = 0x20;
const qint64 SFPACK_HEADER_SIZE = 0x10;
const qint64 SFPACK_FIRST_READ = 0x10000;
const qint32 SFPACK_MAX_PREFIX_STEPS = 8;
}  // namespace

XSFPACKArchive::XSFPACKArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XSFPACKArchive::~XSFPACKArchive()
{
}

bool XSFPACKArchive::parseContext(CONTEXT *pContext, bool bMeasure, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nUncompressedSize = -1;
    context.nInputSize = guardedSource->size();
    if ((context.nInputSize < SFPACK_MIN_SIZE) || (context.nInputSize > 0x7fffffffLL)) return false;

    // The structure lives in a prefix of the file whose length is only known
    // once it is parsed, so grow the prefix on demand instead of reading the
    // whole file - isValid() must stay cheap on a multi-megabyte sample bank.
    qint64 nWanted = qMin(context.nInputSize, SFPACK_FIRST_READ);
    QByteArray baPrefix;
    bool bParsed = false;
    for (qint32 nStep = 0; nStep < SFPACK_MAX_PREFIX_STEPS; ++nStep) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if ((nWanted <= 0) || (nWanted > context.nInputSize)) return false;
        baPrefix = read_array_process(0, nWanted, pPdStruct);
        if (!guardedSource || (baPrefix.size() != nWanted)) return false;

        qint64 nNeeded = 0;
        if (XSFPACKDecoder::parseHeader(baPrefix, context.nInputSize, &context.header, &nNeeded)) {
            bParsed = true;
            break;
        }
        if ((nNeeded <= nWanted) || (nNeeded > context.nInputSize)) return false;
        nWanted = nNeeded;
    }
    if (!bParsed) return false;

    if (bMeasure) {
        qint64 nSize = 0;
        if (!XSFPACKDecoder::measure(baPrefix, context.header, &nSize, pPdStruct)) return false;
        if (!guardedSource) return false;
        context.nUncompressedSize = nSize;
    }

    QString sName = QFileInfo(getDeviceFileName(guardedSource)).completeBaseName();
    if (!guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("sfpack");
    context.sFileName = sName + QStringLiteral(".sf2");

    *pContext = context;

    return true;
}

bool XSFPACKArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, false, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XSFPACKArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XSFPACKArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XSFPACKArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XSFPACKArchive(pDevice);
}

QList<QString> XSFPACKArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'SFPK'0001");
}

XBinary::FT XSFPACKArchive::getFileType()
{
    return FT_SFPACK;
}

XBinary::MODE XSFPACKArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XSFPACKArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XSFPACKArchive::getArch()
{
    return QString();
}

qint32 XSFPACKArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XSFPACKArchive::getFileFormatExt()
{
    return QStringLiteral("sfpack");
}

QString XSFPACKArchive::getFileFormatExtsString()
{
    return QStringLiteral("SFPack (*.sfpack)");
}

QString XSFPACKArchive::getMIMEString()
{
    return QStringLiteral("application/x-sfpack");
}

QString XSFPACKArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, false, nullptr)) return QString();

    return QString::number(context.header.nDeclaredSize);
}

qint64 XSFPACKArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, false, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XSFPACKArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XSFPACKArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XSFPACKArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XSFPACKArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    const bool bMeasure = ((nFileParts & FILEPART_STREAM) != 0);
    if (!parseContext(&context, bMeasure, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = SFPACK_HEADER_SIZE;
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
        if (context.nUncompressedSize >= 0) part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SFPACK);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("SFPack"));
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

QMap<XBinary::UNPACK_PROP, QVariant> XSFPACKArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XSFPACKArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!pState || !guardedSource || guardedSource->isSequential() || m_bUnpackOperationInProgress) return false;
    if ((pState->pContext || !pState->baUnpackSourceToken.isEmpty()) && !ownsUnpackSource(pState)) return false;
    if (!finishUnpack(pState, nullptr) || !guardedSource || !isPdStructNotCanceled(pPdStruct)) return false;

    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !bindUnpackSource(pState, pPdStruct)) return false;

    CONTEXT *pContext = new (std::nothrow) CONTEXT;
    if (!pContext) {
        releaseUnpackSource(pState);
        return false;
    }
    if (!parseContext(pContext, true, pPdStruct) || !guardedSource) {
        releaseUnpackSource(pState);
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

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!guardedSource || !bFinalized) {
        pState->pContext = nullptr;
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XSFPACKArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    if (pContext->nUncompressedSize >= 0) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_SFPACK);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, QStringLiteral("SFPack"));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XSFPACKArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XSFPACKArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XSFPACKArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
