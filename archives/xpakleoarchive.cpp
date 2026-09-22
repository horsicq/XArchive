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
#include "xpakleoarchive.h"

#include <QtEndian>

#include <new>

namespace {
const qint64 PAKLEO_BANNER_SIZE = 0x25;
const qint64 PAKLEO_RECORD_SIZE = 0x1a;
const qint32 PAKLEO_MAX_MEMBERS = 100000;

const quint8 PAKLEO_METHOD_STORED = 0;
const quint8 PAKLEO_METHOD_LEOLZW = 1;

const char *const PAKLEO_BANNER = "LEOLZW - (c) Leonardus Leonardi 1993\x1a";

bool pakleoRangeWithin(qint64 nTotalSize, qint64 nOffset, qint64 nSize)
{
    return (nTotalSize >= 0) && (nOffset >= 0) && (nSize >= 0) && (nOffset <= nTotalSize) && (nSize <= (nTotalSize - nOffset));
}

}  // namespace

XPAKLEOArchive::XPAKLEOArchive(QIODevice *pDevice) : XArchive(pDevice)
{
}

XPAKLEOArchive::~XPAKLEOArchive()
{
}

bool XPAKLEOArchive::parseContext(CONTEXT *pContext, PDSTRUCT *pPdStruct)
{
    if (!pContext || !isPdStructNotCanceled(pPdStruct)) return false;

    QIODevice *guardedSource = getDevice();
    if (guardedSource->isSequential()) return false;

    CONTEXT context = {};
    context.nInputSize = guardedSource->size();
    if (context.nInputSize < (PAKLEO_BANNER_SIZE + PAKLEO_RECORD_SIZE)) return false;

    const QByteArray baBanner = read_array_process(0, PAKLEO_BANNER_SIZE, pPdStruct);
    if (baBanner.size() != PAKLEO_BANNER_SIZE) return false;
    if (baBanner != QByteArray(PAKLEO_BANNER, (int)PAKLEO_BANNER_SIZE)) return false;

    qint64 nOffset = PAKLEO_BANNER_SIZE;
    while ((nOffset + PAKLEO_RECORD_SIZE) <= context.nInputSize) {
        if (!isPdStructNotCanceled(pPdStruct)) return false;
        if (context.listMembers.size() >= PAKLEO_MAX_MEMBERS) break;

        const QByteArray baRecord = read_array_process(nOffset, PAKLEO_RECORD_SIZE, pPdStruct);
        if (baRecord.size() != PAKLEO_RECORD_SIZE) return false;
        const uchar *pRecord = (const uchar *)baRecord.constData();

        // the method tag is the only thing the reference walk validates
        if ((pRecord[2] != '-') || (pRecord[3] != 'l') || (pRecord[4] != 'l') || (pRecord[6] != '-')) break;
        if ((pRecord[5] < '0') || (pRecord[5] > '9')) break;
        const quint8 nMethod = (quint8)(pRecord[5] - '0');

        const qint64 nCompressedSize = (qint32)qFromLittleEndian<quint32>(pRecord + 7);
        const qint64 nUncompressedSize = (qint32)qFromLittleEndian<quint32>(pRecord + 0x0b);
        if ((nCompressedSize < 0) || (nUncompressedSize < 0)) break;

        const qint32 nNameLength = pRecord[0x19];
        const qint64 nNameOffset = nOffset + PAKLEO_RECORD_SIZE;
        if (!pakleoRangeWithin(context.nInputSize, nNameOffset, nNameLength)) break;

        QByteArray baName;
        if (nNameLength > 0) {
            baName = read_array_process(nNameOffset, nNameLength, pPdStruct);
            if (baName.size() != nNameLength) return false;
        }

        const qint64 nDataOffset = nNameOffset + nNameLength;
        if (!pakleoRangeWithin(context.nInputSize, nDataOffset, nCompressedSize)) break;

        MEMBER member = {};
        member.nHeaderOffset = nOffset;
        member.nHeaderSize = PAKLEO_RECORD_SIZE + nNameLength;
        member.nDataOffset = nDataOffset;
        member.nCompressedSize = nCompressedSize;
        member.nUncompressedSize = nUncompressedSize;
        member.nCRC = qFromLittleEndian<quint32>(pRecord + 0x15);
        member.nTime = qFromLittleEndian<quint16>(pRecord + 0x0f) | ((quint32)qFromLittleEndian<quint16>(pRecord + 0x11) << 16);
        member.nMethod = nMethod;
        member.sFileName = QString::fromLatin1(baName).replace(QChar('\\'), QChar('/'));
        context.listMembers.append(member);

        nOffset = nDataOffset + nCompressedSize;
    }

    if (context.listMembers.isEmpty()) return false;

    context.nArchiveSize = qMin(nOffset, context.nInputSize);
    *pContext = context;

    return true;
}

bool XPAKLEOArchive::isValid(PDSTRUCT *pPdStruct)
{
    QIODevice *guardedSource = getDevice();
    if (!guardedSource) return false;
    const qint64 nSavedPosition = guardedSource->pos();
    CONTEXT context = {};
    const bool bResult = parseContext(&context, pPdStruct);
    if (guardedSource) guardedSource->seek(nSavedPosition);

    return bResult;
}

bool XPAKLEOArchive::isValid(QIODevice *pDevice, PDSTRUCT *pPdStruct)
{
    XPAKLEOArchive archive(pDevice);
    return archive.isValid(pPdStruct);
}

XBinary *XPAKLEOArchive::createInstance(QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)
    return new XPAKLEOArchive(pDevice);
}

QList<QString> XPAKLEOArchive::getSearchSignatures()
{
    return QList<QString>() << QStringLiteral("'LEOLZW - (c) Leonardus Leonardi 1993'1A");
}

XBinary::FT XPAKLEOArchive::getFileType()
{
    return FT_PAKLEO;
}

XBinary::MODE XPAKLEOArchive::getMode()
{
    return MODE_DATA;
}

XBinary::ENDIAN XPAKLEOArchive::getEndian()
{
    return ENDIAN_LITTLE;
}

QString XPAKLEOArchive::getArch()
{
    return QString();
}

qint32 XPAKLEOArchive::getType()
{
    return TYPE_ARCHIVE;
}

QString XPAKLEOArchive::getFileFormatExt()
{
    return QStringLiteral("pll");
}

QString XPAKLEOArchive::getFileFormatExtsString()
{
    return QStringLiteral("PAKLEO (*.pll)");
}

QString XPAKLEOArchive::getMIMEString()
{
    return QStringLiteral("application/x-pakleo");
}

QString XPAKLEOArchive::getVersion()
{
    return QString();
}

qint64 XPAKLEOArchive::getFileFormatSize(PDSTRUCT *pPdStruct)
{
    CONTEXT context = {};
    return parseContext(&context, pPdStruct) ? context.nArchiveSize : 0;
}

QList<XBinary::MAPMODE> XPAKLEOArchive::getMapModesList()
{
    return QList<MAPMODE>() << MAPMODE_REGIONS << MAPMODE_STREAMS << MAPMODE_DATA;
}

XBinary::_MEMORY_MAP XPAKLEOArchive::getMemoryMap(MAPMODE mapMode, PDSTRUCT *pPdStruct)
{
    if (mapMode == MAPMODE_UNKNOWN) mapMode = MAPMODE_DATA;
    if (mapMode == MAPMODE_REGIONS) return _getMemoryMap(FILEPART_HEADER | FILEPART_STREAM | FILEPART_OVERLAY, pPdStruct);
    if (mapMode == MAPMODE_STREAMS) return _getMemoryMap(FILEPART_STREAM, pPdStruct);
    return _getMemoryMap(FILEPART_DATA | FILEPART_OVERLAY, pPdStruct);
}

QString XPAKLEOArchive::methodToString(quint8 nMethod)
{
    if (nMethod == PAKLEO_METHOD_STORED) return QStringLiteral("Stored");
    if (nMethod == PAKLEO_METHOD_LEOLZW) return QStringLiteral("LEOLZW");
    return QStringLiteral("Unknown %1").arg(nMethod);
}

XBinary::HANDLE_METHOD XPAKLEOArchive::methodToHandleMethod(quint8 nMethod)
{
    if (nMethod == PAKLEO_METHOD_STORED) return HANDLE_METHOD_STORE;
    if (nMethod == PAKLEO_METHOD_LEOLZW) return HANDLE_METHOD_PAKLEO;
    return HANDLE_METHOD_UNKNOWN;
}

bool XPAKLEOArchive::canAppendPart(qint32 nLimit, qint32 nCurrentCount)
{
    return (nLimit <= 0) || (nCurrentCount < nLimit);
}

QList<XBinary::FPART> XPAKLEOArchive::getFileParts(quint32 nFileParts, qint32 nLimit, PDSTRUCT *pPdStruct)
{
    QList<FPART> listResult;
    CONTEXT context = {};
    if (!parseContext(&context, pPdStruct)) return listResult;

    if ((nFileParts & FILEPART_HEADER) && canAppendPart(nLimit, listResult.size())) {
        FPART part = {};
        part.filePart = FILEPART_HEADER;
        part.nFileOffset = 0;
        part.nFileSize = PAKLEO_BANNER_SIZE;
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

QMap<XBinary::UNPACK_PROP, QVariant> XPAKLEOArchive::getDefaultUnpackProperties()
{
    return XArchive::getDefaultUnpackProperties();
}

bool XPAKLEOArchive::initUnpack(UNPACK_STATE *pState, const QMap<UNPACK_PROP, QVariant> &mapProperties, PDSTRUCT *pPdStruct)
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
    if (!parseContext(pContext, pPdStruct) || pContext->listMembers.isEmpty()) {
        releaseUnpackSource(pState);
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

    const bool bFinalized = validateAndFinalizeUnpackSource(pState, pContext, pPdStruct);
    if (!bFinalized) {
        releaseUnpackSource(pState);
        delete pContext;
        *pState = UNPACK_STATE();
        return false;
    }

    return true;
}

XBinary::ARCHIVERECORD XPAKLEOArchive::infoCurrent(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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
    result.mapProperties.insert(FPART_PROP_ISFOLDER, false);

    return result;
}

bool XPAKLEOArchive::moveToNext(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

bool XPAKLEOArchive::finishUnpack(UNPACK_STATE *pState, PDSTRUCT *pPdStruct)
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

QList<XBinary::FPART_PROP> XPAKLEOArchive::getAvailableFPARTProperties()
{
    return QList<FPART_PROP>() << FPART_PROP_ORIGINALNAME << FPART_PROP_COMPRESSEDSIZE << FPART_PROP_UNCOMPRESSEDSIZE << FPART_PROP_HANDLEMETHOD
                               << FPART_PROP_REPORTEDMETHOD << FPART_PROP_ISFOLDER;
}
