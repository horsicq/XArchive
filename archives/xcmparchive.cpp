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
#include "xcmparchive.h"

#include <QPointer>
#include <QtEndian>

#include <new>

namespace {
const qint64 CMP_HEADER_SIZE = 0x3d;
const qint32 CMP_NAME_OFFSET = 0x28;
const qint32 CMP_NAME_SIZE = 15;
const quint16 CMP_METHOD_LZW = 1;
const quint16 CMP_METHOD_LZSS = 2;
const qint32 CMP_DCL_HEADER_SIZE = 2;
const quint8 CMP_DCL_MAX_LITERALMODE = 1;
const quint8 CMP_DCL_MIN_DICTBITS = 4;
const quint8 CMP_DCL_MAX_DICTBITS = 6;
}  // namespace

XCMPArchive::XCMPArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XCMPArchive::~XCMPArchive()
{
}

bool XCMPArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QPointer<XCMPArchive> guardedThis(this);
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource || guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize <= CMP_HEADER_SIZE) return false;

    const QByteArray baHeader = read_array_process(0, CMP_HEADER_SIZE, pPdStruct);
    if (!guardedThis || !guardedSource || (baHeader.size() != CMP_HEADER_SIZE)) return false;
    const uchar *pHeader = (const uchar *)baHeader.constData();

    if (qFromLittleEndian<quint16>(pHeader) != 0x007fU) return false;
    if (qFromLittleEndian<quint16>(pHeader + 2) != 0x003dU) return false;
    const quint16 nMethod = qFromLittleEndian<quint16>(pHeader + 4);
    if ((nMethod != CMP_METHOD_LZW) && (nMethod != CMP_METHOD_LZSS)) return false;
    if (qFromLittleEndian<quint16>(pHeader + 0x37) != 0x1000U) return false;
    const qint32 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pHeader + 0x39);
    if (nUncompressedSize <= 0) return false;

    QByteArray baName = baHeader.mid(CMP_NAME_OFFSET, CMP_NAME_SIZE);
    const qint32 nZero = baName.indexOf('\0');
    if (nZero >= 0) baName.truncate(nZero);
    if (baName.isEmpty()) return false;
    for (qint32 i = 0; i < baName.size(); ++i) {
        const quint8 nCharacter = (quint8)baName.at(i);
        if (nCharacter < 0x20) return false;
    }

    context.nMethod = nMethod;
    context.nVariant = qFromLittleEndian<quint16>(pHeader + 0x0e);
    context.nDataOffset = CMP_HEADER_SIZE;
    context.nCompressedSize = context.nInputSize - CMP_HEADER_SIZE;
    context.nUncompressedSize = nUncompressedSize;
    context.sFileName = QString::fromLatin1(baName);
    context.bIsDcl = false;

    // Method 2 covers two different codecs.  The newer one is a PKWARE Data
    // Compression Library stream and it names itself in its own first two
    // bytes - literal mode 0 or 1, then a dictionary size of 4, 5 or 6 - so the
    // stream decides, not the container variant word.  A stream too short to
    // carry that header is simply left with the old codec; the sniff never
    // rejects a container the header check already accepted.
    if ((nMethod == CMP_METHOD_LZSS) && (context.nCompressedSize >= CMP_DCL_HEADER_SIZE)) {
        const QByteArray baStream = read_array_process(CMP_HEADER_SIZE, CMP_DCL_HEADER_SIZE, pPdStruct);
        if (!guardedThis || !guardedSource) return false;
        if (baStream.size() == CMP_DCL_HEADER_SIZE) {
            const quint8 nLiteralMode = (quint8)baStream.at(0);
            const quint8 nDictionaryBits = (quint8)baStream.at(1);
            if ((nLiteralMode <= CMP_DCL_MAX_LITERALMODE) && (nDictionaryBits >= CMP_DCL_MIN_DICTBITS) && (nDictionaryBits <= CMP_DCL_MAX_DICTBITS)) {
                context.bIsDcl = true;
            }
        }
    }

    *pContext = context;

    return true;
}

bool XCMPArchive::isValid(PDSTRUCT *pPdStruct)
{
    QPointer<QIODevice> guardedSource(getDevice());
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XCMPArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XCMPArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XCMPArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XCMPArchive(pDevice);
}

QList<QString> XCMPArchive::getSearchSignatures()
{
    return QList<QString>();
}

XBinary::FT XCMPArchive::getFileType()
{
    return FT_CMP_ARCHIVE;
}

XBinary::MODE XCMPArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XCMPArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XCMPArchive::getArch()
{
    return QString();
}

qint32 XCMPArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XCMPArchive::getFileFormatExt()
{
    return QStringLiteral("cmp");
}

QString XCMPArchive::getFileFormatExtsString()
{
    return QStringLiteral("CMP (*.cmp)");
}

QString XCMPArchive::getMIMEString()
{
    return QStringLiteral("application/x-cmp");
}

QString XCMPArchive::getVersion()
{
    CONTEXT context = {};
    if (!parseContext(&context, nullptr)) return QString();
    return QString::number(context.nVariant);
}

qint64 XCMPArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nInputSize : 0;
}

QList<XBinary::MAPMODE> XCMPArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XCMPArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA, pPdStruct);
}

QString XCMPArchive::methodToString(quint16 nMethod, bool bIsDcl)
{
    if (nMethod == CMP_METHOD_LZW) return QStringLiteral("LZW");
    if (nMethod == CMP_METHOD_LZSS) return bIsDcl ? QStringLiteral("PKWARE DCL") : QStringLiteral("LZSS");
    return QStringLiteral("Unknown");
}

XBinary::HANDLE_METHOD XCMPArchive::methodToHandleMethod(quint16 nMethod, bool bIsDcl)
{
    if (nMethod == CMP_METHOD_LZW) return HANDLE_METHOD_CMP_LZW;
    if (nMethod == CMP_METHOD_LZSS) return bIsDcl ? HANDLE_METHOD_PKWARE_DCL_IMPLODE : HANDLE_METHOD_CMP_LZSS;
    return HANDLE_METHOD_UNKNOWN;
}

bool XCMPArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XCMPArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = CMP_HEADER_SIZE;
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
        part.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(context.nMethod, context.bIsDcl));
        part.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(context.nMethod, context.bIsDcl));
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

QMap<XBinary::UNPACK_PROP, QVariant> XCMPArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XCMPArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
{
    QPointer<XCMPArchive> guardedThis(this);
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

XBinary::ARCHIVERECORD XCMPArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_UNCOMPRESSEDSIZE, pContext->nUncompressedSize);
    result.mapProperties.insert(FPART_PROP_HANDLEMETHOD, methodToHandleMethod(pContext->nMethod, pContext->bIsDcl));
    result.mapProperties.insert(FPART_PROP_REPORTEDMETHOD, methodToString(pContext->nMethod, pContext->bIsDcl));
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XCMPArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
{
    UNPACK_OPERATION_GUARD operationGuard(&m_bUnpackOperationInProgress);
    if (!operationGuard.isAcquired() || !pState || !isUnpackSourceCurrent(pState, pPdStruct) || (pState->nNumberOfRecords != 1) ||
        (pState->nCurrentIndex != 0)) {
        return false;
    }
    ++pState->nCurrentIndex;

    return false;
}

bool XCMPArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XCMPArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
