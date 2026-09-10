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
#include "xobfuscatedarchive.h"

#include "Algos/xobfuscationdecoder.h"

#include "Algos/xdiskimagedecoder.h"

#include <QFileInfo>
#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 OBFUSCATED_MIN_SIZE = 32;
const qint64 OBFUSCATED_HEADER_SIZE = 8;

// what the archive underneath has to start with
const char OBFUSCATED_ZIP_PLAIN[] = "\x50\x4b\x03\x04";
const char OBFUSCATED_ARJ_PLAIN[] = "\x60\xea";

}  // namespace

XObfuscatedArchive::XObfuscatedArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XObfuscatedArchive::~XObfuscatedArchive()
{
}

bool XObfuscatedArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XObfuscatedArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < OBFUSCATED_MIN_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, OBFUSCATED_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != OBFUSCATED_HEADER_SIZE)) return false;

    const QByteArray baZip = QByteArray(OBFUSCATED_ZIP_PLAIN, 4);
    const QByteArray baArj = QByteArray(OBFUSCATED_ARJ_PLAIN, 2);

    QString sExtension;
    XObfuscationDecoder::METHOD method = XObfuscationDecoder::detect(baHeader, baZip);
    if (method.transform != XObfuscationDecoder::TRANSFORM_UNKNOWN) {
        sExtension = QStringLiteral(".zip");
    } else {
        method = XObfuscationDecoder::detect(baHeader, baArj);
        // ARJ's plaintext is only two bytes, so a lone XOR key would match far
        // too much; require the byte at +9 to decode to a sane host-OS id as
        // The reference implementation does before believing it
        if (method.transform != XObfuscationDecoder::TRANSFORM_UNKNOWN) sExtension = QStringLiteral(".arj");
    }
    if (method.transform == XObfuscationDecoder::TRANSFORM_UNKNOWN) return false;

    if (sExtension == QStringLiteral(".arj")) {
        QByteArray baProbe;
        if (!XObfuscationDecoder::decode(read_array_process(0, 12, pPdStruct), method, &baProbe, pPdStruct)) return false;
        if (!guardedThis || !guardedSource || (baProbe.size() != 12)) return false;
        // ARJ basic header: u16 magic, u16 header size, u8 first header size,
        // u8 archiver version, u8 minimum version, u8 host OS (0..11)
        if ((quint8)baProbe.at(4) < 0x1e) return false;
        if ((quint8)baProbe.at(7) > 11) return false;
        const quint16 nHeaderSize = ((quint8)baProbe.at(3) << 8) | (quint8)baProbe.at(2);
        if (nHeaderSize >= 0xa29) return false;
    }

    context.nDataOffset = 0;
    context.nCompressedSize = context.nInputSize;
    context.nUncompressedSize = context.nInputSize;
    context.baProperty = XObfuscationDecoder::methodToProperty(method);
    context.sReportedMethod = XObfuscationDecoder::methodToString(method);

    QString sName = QFileInfo(getDeviceFileName(guardedSource.data())).fileName();
    if (!guardedThis || !guardedSource) return false;
    if (sName.isEmpty()) sName = QStringLiteral("archive");
    context.sFileName = sName + sExtension;

    *pContext = context;

    return true;
}

bool XObfuscatedArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XObfuscatedArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XObfuscatedArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XObfuscatedArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XObfuscatedArchive(pDevice);
}

QList<QString> XObfuscatedArchive::getSearchSignatures()
{
    // no signature: the whole point is that the magic is transformed
    return QList<QString>();
}

XBinary::FT XObfuscatedArchive::getFileType()
{
    return FT_OBFUSCATED_ARCHIVE;
}

XBinary::MODE XObfuscatedArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XObfuscatedArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XObfuscatedArchive::getArch()
{
    return QString();
}

qint32 XObfuscatedArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XObfuscatedArchive::getFileFormatExt()
{
    return QStringLiteral("dsk");
}

QString XObfuscatedArchive::getFileFormatExtsString()
{
    return QStringLiteral("Obfuscated archive (*.zip;*.arj)");
}

QString XObfuscatedArchive::getMIMEString()
{
    return QStringLiteral("application/x-apricot-image");
}

QString XObfuscatedArchive::getVersion()
{
    return QString();
}

qint64 XObfuscatedArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XObfuscatedArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XObfuscatedArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

bool XObfuscatedArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XObfuscatedArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = OBFUSCATED_HEADER_SIZE;
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
        if (context.nUncompressedSize >= 0) part.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, context.nUncompressedSize);
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEOBFUSCATE);
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, context.sReportedMethod);
        part.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, context.baProperty);
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

QMap<XBinary::UNPACK_PROP, QVariant> XObfuscatedArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XObfuscatedArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XObfuscatedArchive> guardedThis(this);
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
    if (!parseContext(pContext, pPdStruct) || !guardedThis || !guardedSource) {
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

XBinary::ARCHIVERECORD XObfuscatedArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress, &m_bNestedUnpackInfoAuthorized);
    if (!operationGuard.isAllowed() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return ARCHIVERECORD();
    }
    CONTEXT *pContext = (CONTEXT *)pState->pContext;
    if (!pContext) return ARCHIVERECORD();

    ARCHIVERECORD result = {};
    result.nStreamOffset = pContext->nDataOffset;
    result.nStreamSize = pContext->nCompressedSize;
    result.mapProperties.insert(FPART_PROP_ORIGINALNAME, pContext->sFileName);
    result.mapProperties.insert(FPART_PROP_COMPRESSEDSIZE, pContext->nCompressedSize);
    if (pContext->nUncompressedSize >= 0) result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, HANDLE_METHOD_DEOBFUSCATE);
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, pContext->sReportedMethod);
    result.mapProperties.insert(FPART_PROP_COMPRESSPROPERTIES, pContext->baProperty);
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XObfuscatedArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XObfuscatedArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XObfuscatedArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
